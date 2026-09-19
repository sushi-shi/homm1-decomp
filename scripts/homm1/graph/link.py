"""homm1.graph.link - PHASE 2: base objs -> candidate HOMM1.EXE + .map.

    python3 -m homm1.graph.link [--out E] [--objs-dir D] [--res R] [--order F]

Graph phase 2, opt-in (`ninja candidate` / `homm1 link`): the pinned VC4
link.exe run under wine over our
base objects. The deliverable is the `.map`: every function's link-assigned
RVA and its source object, which cross-referenced with the retail RVAs is what
recovers the original build order (intra-TU order = source-definition order,
cross-TU = object link order).

There is no `/FORCE`: unresolved externals and duplicate definitions are the
link phase's findings. The current pilot is expected to fail until its missing
definitions and program entry point are reconstructed.

The VC4 objects request LIBC and OLDNAMES. Win32 libraries are passed in retail
import-descriptor order; missing named vendor libraries are synthesized from
the retail import table by `homm1.graph.implib`.

Every link is fresh: the prior image and `.ilk` are deleted before LINK.EXE is
run. HoMM1 retail is non-incremental, so the default is `/INCREMENTAL:NO`.
"""

from __future__ import annotations

import collections
import re
import struct
import sys
from pathlib import Path

from homm1.core.paths import REPO
from homm1.tool import ToolError
from homm1.tool.wine import winepath

#: HoMM1's explicit library line, in retail import-descriptor order. The RAD
#: libraries are synthesized from the retail import table when they carry
#: named exports. smkwai32 is ordinal-only and therefore awaits its original
#: import library or reviewed symbol map; it is unnecessary until a matching
#: source object references one of those functions.
LINK_LIBS = ["libc.lib", "oldnames.lib", "kernel32.lib", "user32.lib",
             "gdi32.lib", "advapi32.lib", "winmm.lib", "netapi32.lib",
             "wing32.lib", "wail32.lib"]

#: LIBC defines _WinMainCRTStartup and calls the program's _WinMain@16.
ENTRY = "WinMainCRTStartup"

def unresolved(output: str) -> set[str]:
    """The DECORATED unresolved-external names in a link log.

    LNK2001 prints a C symbol bare (`_malloc`) but a C++ one as demangled prose
    FOLLOWED by the real name in parentheses. A `(\\S+)` grab therefore collapses
    every C++ blocker into the few distinct first words of that prose, which
    silently hid the entire C++ backlog from the punch list. Take the trailing
    parenthesised name when there is one.
    """
    out = set()
    for ln in output.splitlines():
        m = re.search(r"unresolved external symbol (.*)$", ln)
        if not m:
            continue
        rest = m.group(1).strip()
        paren = re.search(r"\(([^()]+)\)\s*$", rest)
        out.add(paren.group(1) if paren else rest.split()[0])
    return out


def classify(sym: str) -> str:
    """Which link blocker `sym` is - the three buckets that need three fixes."""
    if sym.startswith("__imp_"):
        return "import (no import lib on the line)"
    if sym.startswith("?"):
        return "C++ (undefined method/variable - reconstruction backlog)"
    return "C (undefined free function/variable)"


def has_rsrc(exe: Path) -> bool:
    """True when the PE carries a .rsrc section - read straight out of the
    section table, so the check costs nothing and cannot be skipped."""
    try:
        data = exe.read_bytes()
        pe = struct.unpack_from("<I", data, 0x3C)[0]
        n = struct.unpack_from("<H", data, pe + 6)[0]
        first = pe + 24 + struct.unpack_from("<H", data, pe + 20)[0]
        return any(data[first + i * 40:first + i * 40 + 8].rstrip(b"\0") == b".rsrc"
                   for i in range(n))
    except (OSError, struct.error, IndexError):
        return False


def collect_objs(objs_dir: Path, *, order: Path | None = None,
                 explicit: list[str] = ()) -> list[Path]:
    """The objects and their link ORDER.

    `order` (one stem or path per line) wins - that is how a hypothesised
    retail link order is tested; then explicit paths; then every manifest-owned
    `*.obj` in the directory, sorted. The manifest filter matters: deleting a
    [[unit]] does not delete its stale object, and a bare glob then links the
    orphan, which surfaces as a phantom LNK2005 against the TU that legitimately
    owns the symbol now - and with no /FORCE that FAILS the link and looks like
    a real identity defect.
    """
    if order is not None:
        objs = []
        for ln in Path(order).read_text().splitlines():
            s = ln.strip()
            if not s or s.startswith("#"):
                continue
            p = Path(s) if Path(s).suffix else objs_dir / f"{s}.obj"
            if not p.exists():
                raise ToolError(f"order entry not found: {s} ({p})")
            objs.append(p)
        return objs
    if explicit:
        return [Path(o) for o in explicit]
    if not objs_dir.is_dir():
        raise ToolError(f"--objs-dir not found: {objs_dir}")
    from homm1.manifest import units as manifest_units
    owned = {u["unit"] for u in manifest_units()}
    objs, orphans = [], []
    for p in sorted(objs_dir.glob("*.obj")):
        (objs if p.stem in owned else orphans).append(p)
    if orphans:
        print(f"[link] skipping {len(orphans)} orphaned obj(s) with no [[unit]]: "
              + ", ".join(p.name for p in orphans[:6])
              + (" ..." if len(orphans) > 6 else ""))
    return objs


def candidate(out: Path, objs_dir: Path, *, mapfile: Path | None = None,
              res: Path | None = None, order: Path | None = None,
              explicit: list[str] = (), extra_libs: list[str] = (),
              incremental: bool = False,
              base: str = "0x400000", keep_all: bool = True,
              extra_flags: list[str] = (), dry_run: bool = False) -> dict:
    """Link the candidate image; returns {objs, libs, unresolved, duplicates}.

    `dry_run` assembles the response file and stops before link.exe - the way
    to inspect the object order and the library line without a linker.
    """
    from homm1.graph import implib
    from homm1.tool import link as link_tool

    out = Path(out).resolve()
    mapf = Path(mapfile).resolve() if mapfile else out.with_suffix(".map")
    out.parent.mkdir(parents=True, exist_ok=True)

    objs = collect_objs(Path(objs_dir), order=order, explicit=explicit)
    if not objs:
        raise ToolError("no objects to link")

    rsp_lines = [
        f"/OUT:{winepath(out)}", f"/MAP:{winepath(mapf)}",
        "/NOLOGO", "/SUBSYSTEM:WINDOWS", f"/BASE:{base}",
        "/INCREMENTAL:YES" if incremental else "/INCREMENTAL:NO",
        f"/ENTRY:{ENTRY}",
    ]
    if keep_all:
        # Keep every function so the map is complete. VC4's linker predates
        # /OPT:NOICF; it performs no identical-COMDAT folding by default.
        rsp_lines.append("/OPT:NOREF")
    rsp_lines += list(extra_flags)

    libs = list(extra_libs)
    made = implib.on_disk() if dry_run else implib.ensure_all()
    available = {p.name.lower(): str(p) for p in made}
    available.update({p.name.lower(): str(p)
                      for _dll, _names, p in implib.survey() if p is not None})
    for name in LINK_LIBS:
        if name not in available:
            path = implib.toolchain_lib(Path(name).stem)
            if path is not None:
                available[name] = str(path)
    libs += [available.get(n, n) for n in LINK_LIBS]       # substitute IN PLACE
    rsp_lines += [winepath(x) if Path(x).exists() else x for x in libs]
    rsp_lines += [f'"{winepath(o)}"' for o in objs]
    if res is not None:
        rsp_lines.append(f'"{winepath(Path(res).resolve())}"')

    # VC5 link has a short argv limit under wine, hence the response file.
    rsp = out.parent / f"{out.stem}.objs.rsp"
    rsp.write_text("\n".join(rsp_lines) + "\n")
    if dry_run:
        print(f"[link] dry run: {len(objs)} obj(s) + {len(libs)} lib(s) -> {rsp}")
        return {"objs": len(objs), "libs": len(libs), "rsp": rsp,
                "unresolved": [], "duplicates": 0}
    for stale in (out, mapf, out.with_suffix(".ilk")):
        stale.unlink(missing_ok=True)

    logf = out.parent / f"{out.stem}.link.log"
    try:
        output = link_tool.link([f"@{winepath(rsp)}"], cwd=out.parent,
                                expect=[out, mapf])
    except ToolError as e:
        logf.write_text(str(e))
        raise
    logf.write_text(output)

    if res is not None and not has_rsrc(out):
        raise ToolError(
            f"{out.name} has no .rsrc although --res {res} was on the link "
            "line. Every MFC dialog is created from a DIALOG resource, so this "
            "binary has no working settings/multiplayer/save screens.")
    if res is None:
        # The image is knowingly incomplete and nothing else says so: the
        # configure-time explanation lives in a generated manifest nobody
        # reads, and the .map - which is what phase 2 is for - is unaffected.
        print(f"[link] no .res on the link line, so {out.name} has NO .rsrc: "
              "every MFC dialog is created from a DIALOG resource, so the "
              "image is a link-ORDER artifact (the .map), not a runnable game.")

    # No /FORCE: an unresolved extern or a duplicate FAILS the link above, so
    # reaching here means both are zero. They are still reported (and asserted)
    # because a silent regression to non-zero would mean the link stopped being
    # an oracle.
    dups = sum(1 for ln in output.splitlines() if "LNK4006" in ln)
    unres = sorted(unresolved(output))
    (out.parent / f"{out.stem}.unresolved.txt").write_text("\n".join(unres) + "\n")
    print(f"[link] {len(objs)} obj(s) + {len(libs)} explicit lib(s) -> {out} "
          f"({out.stat().st_size:,} B) + {mapf.name}")
    print(f"[link] {len(unres)} unresolved external(s), {dups} dup-symbol "
          "warning(s)  (no /FORCE - a real link)")
    for bucket, n in sorted(collections.Counter(
            classify(s) for s in unres).items(), key=lambda kv: -kv[1]):
        print(f"[link]   {n:5d}  {bucket}")
    if unres or dups:
        raise ToolError(f"link is no longer clean: {len(unres)} unresolved, "
                        f"{dups} duplicate(s). Fix the source - never re-add "
                        "/FORCE (see the module docstring).")
    return {"objs": len(objs), "libs": len(libs), "unresolved": unres,
            "duplicates": dups, "exe": out, "map": mapf}


def main() -> int:
    import argparse
    from homm1 import graph
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--out", type=Path, default=REPO / graph.CANDIDATE_EXE)
    ap.add_argument("--map", dest="mapfile", type=Path,
                    help="map path (default: <out> with a .map suffix)")
    ap.add_argument("--objs-dir", type=Path, default=REPO / graph.BASE_DIR)
    ap.add_argument("--obj", action="append", default=[],
                    help="explicit object (repeatable)")
    ap.add_argument("--order", type=Path,
                    help="file listing object stems/paths in link order")
    ap.add_argument("--res", type=Path, help=".RES for the candidate's resources")
    ap.add_argument("--lib", action="append", default=[],
                    help="extra import/static lib (repeatable)")
    ap.add_argument("--incremental", action="store_true",
                    help="experiment with /INCREMENTAL:YES (retail is NO)")
    ap.add_argument("--base", default="0x400000", help="image base (/BASE)")
    ap.add_argument("--opt-ref", dest="keep_all", action="store_false",
                    help="let the linker strip/fold unreferenced COMDATs")
    ap.add_argument("--dry-run", action="store_true",
                    help="assemble the response file and stop before link.exe")
    ap.add_argument("flags", nargs=argparse.REMAINDER,
                    help="extra link flags after `--`")
    a = ap.parse_args()
    extra = a.flags[1:] if a.flags and a.flags[0] == "--" else a.flags
    try:
        candidate(a.out, a.objs_dir, mapfile=a.mapfile, res=a.res, order=a.order,
                  explicit=a.obj, extra_libs=a.lib,
                  incremental=a.incremental, base=a.base,
                  keep_all=a.keep_all, extra_flags=extra, dry_run=a.dry_run)
    except (ToolError, OSError) as e:
        print(f"[link] {e}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

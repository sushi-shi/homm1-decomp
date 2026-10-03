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

The VC4 objects request LIBC and OLDNAMES; retail's runtime is LIBCMT, which
replaces LIBC after the Win32 libraries. Win32 libraries are passed in retail
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

#: HoMM1's explicit library line. LINK emits import descriptors in the order
#: it first pulls each library, and lays hint/name entries in pull order.
#: Retail's descriptors read WINMM, KERNEL32, USER32, GDI32, ADVAPI32,
#: NETAPI32, smkwai32, WING32, wail32, yet wail32's hint/names come after the
#: second-pass USER32/GDI32 entries (0x004d72b4). Only BASE's soundmgr calls
#: AIL, so wail32.lib was searched before the BASE library: it pulls nothing
#: in the first pass and everything in the second. Any slot between gdi32.lib
#: and the BASE library gives the same image. smkwai32 and WING32 follow the
#: BASE library, because their thunks follow the BASE run.
#: LINK searches libraries in this order before the objects' default
#: libraries (OLDNAMES) and the CRT, so the descriptors and the smkwai32/WING32
#: jump thunks land ahead of the CRT, exactly where retail has them
#: (thunks at 0x00480188, LIBCMT from 0x004801f0). The vendor libraries are
#: synthesized by `homm1.graph.implib` from the retail import table plus the
#: reviewed import-thunk names in function_referents.tsv.
LINK_LIBS = ["winmm.lib", "kernel32.lib", "user32.lib", "gdi32.lib",
             "advapi32.lib", "netapi32.lib", "wail32.lib", "smkwai32.lib",
             "wing32.lib"]

#: Retail's C runtime is the VC4.0 multithreaded LIBCMT.LIB, not the
#: single-threaded LIBC.LIB the objects request: retail carries LIBCMT's
#: _mtinit/_getptd (TlsAlloc, TlsGetValue, TlsSetValue, GetCurrentThreadId,
#: SetLastError), _lock/_unlock and the *_lk stream/file variants. Against
#: LIBCMT the DNA census finds 189 exact CRT bodies (35052 of 44700 band
#: bytes); against LIBC only 106 (14686). See evidence/link-layout.md. It
#: follows the import libraries, the position of the objects' default
#: library, so the import thunks still precede the CRT.
CRT_LIBRARY = "libcmt.lib"
CRT_REPLACES = "libc.lib"

#: The module definition: export directory (AppAbout, AppWndProc), module
#: name and stack reserve - see the file's own header.
MODULE_DEF = REPO / "config/heroes.def"

#: Retail .text has NETAPI32's `Netbios` jump thunk at 0x0047343c, between
#: comwin (the last game object of the SOURCE run) and the BASE run starting
#: at 0x00473450. A thunk is an import-library member, and LINK places
#: library members after every object on the line, in the order it pulls
#: them; so everything from here on was itself pulled from a library searched
#: after netapi32.lib (and after wail32.lib, see LINK_LIBS) - the BASE
#: library. Its member order is VC4 LINK's pull order (first reference in
#: the undefined-symbol list), not a list we choose.
BASE_LIBRARY_FROM = 0x00073450
BASE_LIBRARY = "base.lib"
BASE_LIBRARY_AFTER = "wail32.lib"

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
    for p in sorted(objs_dir.rglob("*.obj")):
        unit = p.relative_to(objs_dir).with_suffix("").as_posix()
        (objs if unit in owned else orphans).append(p)
    if orphans:
        print(f"[link] skipping {len(orphans)} orphaned obj(s) with no [[unit]]: "
              + ", ".join(p.name for p in orphans[:6])
              + (" ..." if len(orphans) > 6 else ""))
    return objs


def _unit_of(obj: Path) -> str | None:
    """`BASE/WINMGR` for build/objdiff/base/BASE/WINMGR.obj or the MASM OMF
    twin under build/link/omf - the last two path components, sans suffix."""
    parts = Path(obj).with_suffix("").parts
    return "/".join(parts[-2:]) if len(parts) >= 2 else None


def first_claimed_rva(obj: Path, claims_dir: Path | None = None) -> int | None:
    """The unit's lowest claimed function RVA (dynamic initializers excluded)."""
    from homm1 import graph
    claims_dir = Path(claims_dir or REPO / graph.CLAIMS_DIR)
    unit = _unit_of(obj)
    f = claims_dir / f"{unit}.tsv" if unit else None
    lo = None
    if f is not None and f.is_file():
        for ln in f.read_text().splitlines():
            c = ln.split("\t")
            if (len(c) > 4 and c[0].startswith("0x") and c[3] == "func"
                    and c[4] != "src_dyninit"):
                rva = int(c[0], 16)
                lo = rva if lo is None else min(lo, rva)
    return lo


def retail_code_order(objs: list[Path], claims_dir: Path | None = None
                      ) -> tuple[list[Path], list[Path]]:
    """`objs` sorted by each unit's lowest claimed retail function RVA.

    LINK lays .text out in object order, so the order in which retail places
    each TU's first function IS the original link order (intra-TU order is
    source order). Dynamic-initializer pins are excluded from the key: they
    are compiler-emitted and their TU position is a separate question.
    Returns (ordered, unplaced) - objects with no claim keep their relative
    order and follow the placed ones.
    """
    keyed, unplaced = [], []
    for i, obj in enumerate(objs):
        lo = first_claimed_rva(obj, claims_dir)
        (keyed.append((lo, i, obj)) if lo is not None else unplaced.append(obj))
    return [o for _lo, _i, o in sorted(keyed)] + unplaced, unplaced


def candidate(out: Path, objs_dir: Path, *, mapfile: Path | None = None,
              res: Path | None = None, order: Path | None = None,
              explicit: list[str] = (), extra_libs: list[str] = (),
              incremental: bool = False,
              base: str = "0x400000", keep_all: bool = False,
              extra_flags: list[str] = (), dry_run: bool = False,
              retail_order: bool = True, base_library: bool = True) -> dict:
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
    members: list[Path] = []
    if order is None and retail_order:
        objs, unplaced = retail_code_order(objs)
        print(f"[link] object order: retail code order of each unit's first "
              f"claimed function ({len(objs) - len(unplaced)} placed"
              + (f", {len(unplaced)} unclaimed appended" if unplaced else "")
              + ")")
        if base_library:
            members = [o for o in objs
                       if (first_claimed_rva(o) or -1) >= BASE_LIBRARY_FROM]
            objs = [o for o in objs if o not in members]
            print(f"[link] {len(objs)} explicit object(s); {len(members)} "
                  f"BASE member(s) in {BASE_LIBRARY}, searched after "
                  f"{BASE_LIBRARY_AFTER}")

    # No /ENTRY: LINK's default for /SUBSYSTEM:WINDOWS is the CRT's
    # WinMainCRTStartup, and naming it up front pulls wincrt0.obj to the head
    # of the CRT, whereas retail's CRT begins with exsup.obj.
    rsp_lines = [
        f"/OUT:{winepath(out)}", f"/MAP:{winepath(mapf)}",
        "/NOLOGO", "/SUBSYSTEM:WINDOWS", f"/BASE:{base}",
        "/INCREMENTAL:YES" if incremental else "/INCREMENTAL:NO",
        f"/DEF:{winepath(MODULE_DEF)}",
    ]
    if keep_all:
        # Keep every unreferenced COMDAT/thunk so the map is complete. Retail
        # was linked with the default /OPT:REF: of the 196 import slots only
        # the 17 smkwai32/WING32 thunks that code calls directly survive.
        rsp_lines.append("/OPT:NOREF")
    rsp_lines.append(f"/NODEFAULTLIB:{CRT_REPLACES}")
    rsp_lines += list(extra_flags)

    libs = list(extra_libs)
    made = implib.on_disk() if dry_run else implib.ensure_all()
    available = {p.name.lower(): str(p) for p in made}
    available.update({p.name.lower(): str(p)
                      for _dll, _names, p in implib.survey() if p is not None})
    for name in (*LINK_LIBS, CRT_LIBRARY):
        if name not in available:
            path = implib.toolchain_lib(Path(name).stem)
            if path is not None:
                available[name] = str(path)
    libs += [available.get(n, n) for n in (*LINK_LIBS, CRT_LIBRARY)]  # substitute IN PLACE
    if members:
        base_lib = out.parent / BASE_LIBRARY
        at = next((i + 1 for i, x in enumerate(libs)
                   if Path(x).name.lower() == BASE_LIBRARY_AFTER), len(libs))
        libs.insert(at, str(base_lib))
        lib_rsp = out.parent / f"{out.stem}.lib.rsp"
        lib_rsp.write_text("\n".join(["/NOLOGO", f"/OUT:{winepath(base_lib)}",
                                      *[f'"{winepath(o)}"' for o in members]])
                           + "\n")
        if not dry_run:
            base_lib.unlink(missing_ok=True)
            # `-lib` must be LINK's first argument; inside a response file
            # it is read as a link option.
            link_tool.link(["-lib", f"@{winepath(lib_rsp)}"], cwd=out.parent,
                           expect=[base_lib])
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
        full = getattr(e, "output", None) or str(e)
        logf.write_text(full)
        unres = sorted(unresolved(full))
        (out.parent / f"{out.stem}.unresolved.txt").write_text(
            "".join(f"{s}\n" for s in unres))
        if unres:
            print(f"[link] {len(unres)} unresolved external(s) -> "
                  f"{out.stem}.unresolved.txt")
            for bucket, n in sorted(collections.Counter(
                    classify(s) for s in unres).items(), key=lambda kv: -kv[1]):
                print(f"[link]   {n:5d}  {bucket}")
        raise
    logf.write_text(output)

    if res is not None and not has_rsrc(out):
        raise ToolError(
            f"{out.name} has no .rsrc although --res {res} was on the link "
            "line: the game window would have no icon, menus or About box.")
    if res is None:
        # The image is knowingly incomplete and nothing else says so: the
        # configure-time explanation lives in a generated manifest nobody
        # reads, and the .map - which is what phase 2 is for - is unaffected.
        print(f"[link] no .res on the link line, so {out.name} has NO .rsrc "
              "(icon, MNU* menus, HEROES About dialog): the image is a "
              "link-ORDER artifact (the .map), not a runnable game.")

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


from homm1.core.usage import logged


@logged
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
    ap.add_argument("--keep-all", dest="keep_all", action="store_true",
                    help="/OPT:NOREF: keep unreferenced COMDATs and import "
                         "thunks (a complete map; retail used /OPT:REF)")
    ap.add_argument("--manifest-order", dest="retail_order",
                    action="store_false",
                    help="keep the given object order instead of sorting by "
                         "each unit's first claimed retail RVA")
    ap.add_argument("--no-base-library", dest="base_library",
                    action="store_false",
                    help="link the BASE units as explicit objects instead of "
                         "the retail BASE library")
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
                  keep_all=a.keep_all, extra_flags=extra, dry_run=a.dry_run,
                  retail_order=a.retail_order, base_library=a.base_library)
    except (ToolError, OSError) as e:
        print(f"[link] {e}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

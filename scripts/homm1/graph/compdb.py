"""homm1.graph.compdb - units.toml -> build/clangd/compile_commands.json.

    python3 -m homm1.graph.compdb            # (re)generate + coverage check
    python3 -m homm1.graph.compdb --check    # verify the existing file only

The clangd compilation database is ADDITIVE tooling that runs alongside the
matching build; it never touches it. The matching build compiles with MSVC
4.0's CL.EXE under wine, which clang-based consumers cannot invoke, so this
emits clang-cl driver entries that point clang at the VC4 headers and ask it
to emulate cl 10.00 (`_MSC_VER=1000`). Parse-only - no
wine, no CL.EXE.

Consumers: clangd (the .clangd file points CompileFlags.CompilationDatabase
at build/clangd/), homm1.tool.clang (per-TU extraction flags - a unit with
no entry silently falls back to bare MS flags, which is why generation always
ends with a coverage check THROUGH the consumer's own parser), the homm1.lsp
verbs, and homm1.verify.fingerprints.

Mechanics kept from the proven frozen generator (scripts/homm1-old/init/
clangd.py):
  * lowercase-symlink mirrors of the toolchain include dirs under
    build/clangd/inc-lower/ - the 1990s headers are ALL-UPPERCASE on disk
    (STRING.H, AFXWIN.H) but sources include them lowercase, and clang on
    case-sensitive Linux cannot find them otherwise; `/imsvc <mirror>` first,
    the real directory after;
  * ONE shared flag set for every unit. The manifest's [flags] profiles differ
    only in /GX and /GR, which alter cl's EH tables and RTTI emission, not
    clang's parse/navigation - the frozen generator never mapped them and that
    uniform set is the proven state every fragment was extracted under;
  * write-if-changed: the labels edges depend on this file, so an unchanged
    payload must not bump its mtime (restat then stops the cascade anyway,
    but only after re-running 300 edges).

The ninja `compdb` edge (graph/emit.py) re-runs this on a units.toml or
module change. A toolchain bump moves $MSVC_DIR, which ninja
cannot see - after re-pinning, run `python3 -m homm1.graph.compdb` once (the
mirror marker then rebuilds the symlink mirrors too).
"""

from __future__ import annotations

import json
import os
import shutil
import subprocess
import sys
from pathlib import Path

from homm1.core.paths import BUILD, INCLUDE, REPO, VENDOR, msvc_dir

OUT_DIR = BUILD / "clangd"
OUT_FILE = OUT_DIR / "compile_commands.json"
MIRROR_DIR = OUT_DIR / "inc-lower"

#: MSVC 4.0 == cl 10.00 == _MSC_VER 1000.
MSC_COMPAT = "10.00"
TARGET = "i386-pc-windows-msvc"

#: The matching build's environment: 32-bit Windows app, static ANSI/MBCS
#: MFC 4.2 (NAFXCW.LIB). _AFXDLL and _UNICODE are deliberately NOT defined.
DEFINES = ["/D_X86_", "/DWIN32", "/D_WINDOWS", "/D_MBCS"]


def resolve_include_dirs() -> tuple[Path, str]:
    """Return the installed VC4 include tree used by clang tooling."""
    msvc_inc = msvc_dir() / "include"
    if not msvc_inc.is_dir():
        raise SystemExit("[compdb] ERROR: VC4 headers are missing; run "
                         "`homm1 toolchain install --id vc40 --media ...`")
    return msvc_inc, str(msvc_dir())


def build_lowercase_mirror(real: Path, mirror: Path) -> Path:
    """Recursive lowercase-symlink mirror of `real`, so <string.h> resolves.

    Every FOO.H under `real` gets a lowercase symlink `foo.h` (to the real,
    ABSOLUTE path) under `mirror`, preserving (lowercased) subdir structure.
    Rebuilt only when `real` changes (a `.src` marker guards it) so a
    toolchain bump does not leave dangling symlinks.
    """
    marker = mirror.parent / (mirror.name + ".src")
    if mirror.is_dir() and marker.is_file() and marker.read_text() == str(real):
        return mirror
    if mirror.exists():
        shutil.rmtree(mirror)
    for root, _dirs, files in os.walk(real):
        rel = os.path.relpath(root, real)
        low = mirror if rel == "." else mirror / rel.lower()
        low.mkdir(parents=True, exist_ok=True)
        for fn in files:
            link = low / fn.lower()
            if not link.exists():
                link.symlink_to(os.path.join(root, fn))
    marker.parent.mkdir(parents=True, exist_ok=True)
    marker.write_text(str(real))
    return mirror


def base_flags(msvc_inc: Path, msvc_low: Path) -> list[str]:
    """The clang-cl flag set shared by every unit.

    /imsvc marks the toolchain headers as SYSTEM includes (diagnostics inside
    the ancient MFC/CRT headers are silenced). Lowercase mirrors FIRST so a
    lowercase `#include <string.h>` resolves; the real (uppercase) dirs follow
    for exact-case includes; DX before MSVC in both tiers so the DX6 SDK wins
    over VC5's DirectX 3-era copies.
    """
    vendor_dirs = sorted(d for d in VENDOR.iterdir() if d.is_dir()) \
        if VENDOR.is_dir() else []
    return [
        f"--target={TARGET}",
        f"-fms-compatibility-version={MSC_COMPAT}",
        "-fms-extensions",
        # `&Temporary()` is MSVC C4238, a nonstandard extension the retail
        # sources use; clang errors on it by default.
        "-Wno-address-of-temporary",
        # VC5 accepts SDK HRESULT macros such as DIERR_INSUFFICIENTPRIVS as
        # signed switch labels even when their `long` literal is unsigned.
        "-Wno-c++11-narrowing",
        # MFC's headers only parse under MSVC's lazy template semantics.
        "-fdelayed-template-parsing",
        "/imsvc", str(msvc_low),
        "/imsvc", str(msvc_inc),
        # our own headers - NOT /imsvc, so diagnostics in our code surface.
        "/I", str(INCLUDE),
        # vendored SDK headers (vendor/<sdk>/, one dir deep).
        *[f for d in vendor_dirs
          for f in ("/I", str(d))],
        *DEFINES,
    ]


def generate(quiet: bool = False) -> bool:
    """(Re)write the compdb from config/units.toml. Returns True if changed."""
    from homm1.manifest import flag_profiles, units
    msvc_inc, provenance = resolve_include_dirs()
    msvc_low = build_lowercase_mirror(msvc_inc, MIRROR_DIR / "msvc")
    shared = base_flags(msvc_inc, msvc_low)

    # Most cl profiles differ only in optimisation/code-generation switches,
    # which the source probes deliberately do not inherit.  ABI switches are
    # different: /Gr changes the mangled name of every unqualified free
    # function.  Omitting it made extraction claim a cdecl spelling while the
    # VC4 object contained the fastcall body.  Keep the small semantic subset
    # that affects declarations and symbol identity.
    profiles = flag_profiles()
    abi_prefixes = ("/Gd", "/Gr", "/Gz", "/Zp", "/D", "/U")

    def abi_flags(unit: dict) -> list[str]:
        return [flag for flag in profiles.get(unit.get("flags", ""), [])
                if flag.startswith(abi_prefixes)]

    cpp_units = [u for u in units()
                 if Path(u["source"]).suffix.lower() != ".asm"]
    entries = [{
        "directory": str(REPO),
        "file": u["source"],
        # clang-cl driver form; clangd/clang parse it internally.
        "arguments": ["clang-cl", "/c", u["source"], *shared, *abi_flags(u)],
    } for u in cpp_units]

    OUT_DIR.mkdir(parents=True, exist_ok=True)
    payload = json.dumps(entries, indent=2) + "\n"
    changed = not (OUT_FILE.exists()
                   and OUT_FILE.read_text(encoding="utf-8") == payload)
    if changed:
        OUT_FILE.write_text(payload, encoding="utf-8")
    if changed or not quiet:
        print(f"[compdb] {'wrote' if changed else 'unchanged'} "
              f"{OUT_FILE.relative_to(REPO)} ({len(entries)} units)")
    if not quiet:
        print(f"[compdb] include dirs ({provenance}):")
        print(f"    MSVC/MFC : {msvc_inc}")
        print(f"    lowercase mirrors -> {MIRROR_DIR}")
        print("[compdb] clang-cl flags per unit:")
        print("    clang-cl /c <src> " + " ".join(shared))
    return changed


def dead_include_dirs(db: dict) -> list[str]:
    """The `/imsvc` and `/I` directories the stored entries name that are GONE.

    A toolchain re-pin moves $MSVC_DIR/$DXSDK_DIR, and ninja cannot see that -
    no edge depends on the environment - so the compdb keeps naming the OLD
    /nix/store path. Once that path is garbage-collected every entry is
    unusable, and unit COVERAGE (which only asks whether a source has a row)
    still reports 300/300. Answering "full coverage" for a database that
    cannot resolve <string.h> is the lie this closes.
    """
    wanted: set[str] = set()
    for args in db.values():
        args = list(args)
        for i, arg in enumerate(args[:-1]):
            if arg in ("/imsvc", "-imsvc", "/I", "-I"):
                wanted.add(args[i + 1])
    return sorted(d for d in wanted if not os.path.isdir(d))


def check(quiet: bool = False) -> list[str]:
    """Coverage through the CONSUMER's parser: every manifest unit must have
    an entry in homm1.tool.clang.compdb()'s dict - the exact join extraction
    performs, so a unit missing here is a unit that would silently fall back
    to bare MS flags. Returns the problems (empty = full coverage)."""
    from homm1.manifest import units
    from homm1.tool import clang
    db = clang.compdb()
    us = [u for u in units()
          if Path(u["source"]).suffix.lower() != ".asm"]
    problems = []
    if not db:
        return [f"{OUT_FILE.relative_to(REPO)} is missing or unparsable - "
                f"EVERY unit would fall back to bare MS flags"]
    srcs = {}
    for u in us:
        srcs[os.path.realpath(str(REPO / u["source"]))] = u["unit"]
    missing = [unit for src, unit in srcs.items() if src not in db]
    problems += [f"unit '{u}' has NO compdb entry (bare-flag fallback)"
                 for u in sorted(missing)]
    stale = sorted(os.path.relpath(src, REPO) for src in db if src not in srcs)
    problems += [f"stale entry (not a manifest unit): {s}" for s in stale]
    dead = dead_include_dirs(db)
    problems += [f"include dir no longer exists: {d} - the toolchain moved; "
                 "re-run `python3 -m homm1.graph.compdb`" for d in dead]
    if not quiet or problems:
        print(f"[compdb] coverage: {len(us) - len(missing)}/{len(us)} units "
              f"have an entry ({len(stale)} stale, {len(dead)} dead include "
              "dir(s))")
    return problems


def main() -> int:
    import argparse
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--check", action="store_true",
                    help="verify the existing file only; write nothing")
    ap.add_argument("--quiet", action="store_true",
                    help="print only changes and problems (the ninja edge)")
    a = ap.parse_args()
    if not a.check:
        generate(a.quiet)
    problems = check(a.quiet)
    for p in problems:
        print(f"[compdb] {p}", file=sys.stderr)
    return 1 if problems else 0


if __name__ == "__main__":
    raise SystemExit(main())

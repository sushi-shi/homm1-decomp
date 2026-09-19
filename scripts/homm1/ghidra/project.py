"""homm1.ghidra.project - build, refresh and read back the viewer database.

    homm1 ghidra build  [--force] [--aggressive] [--no-bookmarks]
    homm1 ghidra update [--force] [--no-bookmarks]
    homm1 ghidra verify [rva ...]
    homm1 ghidra status

`build` imports the retail image into build/ghidra/homm1.gpr, auto-analyzes it
once, then applies the export. `update` re-applies the export to the EXISTING
project without re-importing or re-analyzing; because the payload carries a
content digest and project.py stamps it, an update with unchanged knowledge is
a hash compare and exits without booting a JVM.

The dirtiness chain is the pipeline's own: bindings.tsv is write-if-changed, so
an unchanged Model produces a byte-identical payload, hence an identical digest,
hence a no-op.
"""

from __future__ import annotations

import json
import shutil
from pathlib import Path

from homm1.core.paths import BUILD, retail_exe
from homm1.ghidra import export
from homm1.tool import ToolError, ghidra

PROJECT_DIR = BUILD / "ghidra"
PROJECT_NAME = "homm1"
#: the program's name INSIDE the project - the retail image lives in the nix
#: store, and its file name would otherwise drag the store hash into the UI
PROGRAM_NAME = "HOMM1.EXE"
STAMP = PROJECT_DIR / "applied.json"
HERE = Path(__file__).resolve().parent
APPLY = HERE / "apply.py"
DUMP = HERE / "dump.py"

#: the three addresses the acceptance round-trip names: a matched method, a
#: library vtable, and a function-local static in .bss
SAMPLE_RVAS = (0x153810, 0x1EE54C, 0x2BF228)


def project_file() -> Path:
    return PROJECT_DIR / f"{PROJECT_NAME}.gpr"


def exists() -> bool:
    return project_file().is_file()


def _stamp() -> dict:
    try:
        return json.loads(STAMP.read_text())
    except Exception:
        return {}


def _write_stamp(doc: dict) -> None:
    STAMP.write_text(json.dumps(
        {"digest": doc["digest"], "schema": doc["schema"],
         "exe": doc["exe"], "counts": doc["counts"]}, indent=1) + "\n")


def _apply_env(bookmarks: bool) -> dict[str, str]:
    return {"HOMM1_GHIDRA_PAYLOAD": str(export.PAYLOAD),
            "HOMM1_GHIDRA_BOOKMARKS": "1" if bookmarks else "0"}


def _payload_line(doc: dict, changed: bool) -> None:
    print(f"[ghidra] payload {export.PAYLOAD} "
          f"{'UPDATED' if changed else 'unchanged'} "
          f"digest={doc['digest'][:12]}")


def _report(doc: dict) -> None:
    c = doc["counts"]
    print(f"[ghidra] functions labeled: {c['functions']} "
          f"({c['functions_claimed']} claimed, {c['functions_census_only']} "
          f"census-only; {c['functions_thunk']} thunks, {c['functions_eh']} "
          f"eh funclets)")
    print(f"[ghidra] data labeled: {c['data']} ({c['data_claimed']} claimed, "
          f"{c['data_census_only']} census-only; {c['vtables']} vtables, "
          f"{c['strings']} strings)")
    print(f"[ghidra] bands annotated: {len(doc['bands'])} | units: "
          f"{c['units']} | aliases carried: {c['aliases']} | pad rows "
          f"dropped: {c['pad_rows_dropped']}")


def build(*, force: bool = False, aggressive: bool = False,
          bookmarks: bool = True, timeout: float | None = None) -> int:
    """Create the project from scratch: import, analyze, apply."""
    ghidra.install_dir()        # fail on a missing Ghidra before any work
    doc, changed = export.write()
    _payload_line(doc, changed)
    if force and PROJECT_DIR.is_dir():
        for stale in (project_file(), PROJECT_DIR / f"{PROJECT_NAME}.rep",
                      PROJECT_DIR / f"{PROJECT_NAME}.lock",
                      PROJECT_DIR / f"{PROJECT_NAME}.lock~"):
            if stale.is_dir():
                shutil.rmtree(stale)
            else:
                stale.unlink(missing_ok=True)
        STAMP.unlink(missing_ok=True)
    _report(doc)
    print(f"[ghidra] Ghidra {ghidra.version()} -> {project_file()}")
    ghidra.headless(retail_exe(), PROJECT_DIR, PROJECT_NAME, [APPLY],
                    analyze=True, aggressive=aggressive,
                    program_name=PROGRAM_NAME,
                    env=_apply_env(bookmarks), timeout=timeout)
    _write_stamp(doc)
    print(f"[ghidra] project ready: open {project_file()} in Ghidra")
    return 0


def update(*, force: bool = False, bookmarks: bool = True,
           timeout: float | None = None) -> int:
    """Re-apply the export to the existing project; a no-op when unchanged."""
    if not exists():
        print(f"[ghidra] no project at {project_file()} - run "
              f"`homm1 ghidra build` first")
        return 2
    doc, changed = export.write()
    _payload_line(doc, changed)
    if not force and _stamp().get("digest") == doc["digest"]:
        print(f"[ghidra] up to date (digest={doc['digest'][:12]}); "
              f"nothing to re-apply")
        return 0
    ghidra.install_dir()
    _report(doc)
    ghidra.headless(retail_exe(), PROJECT_DIR, PROJECT_NAME, [APPLY],
                    analyze=False, program_name=PROGRAM_NAME,
                    env=_apply_env(bookmarks), timeout=timeout)
    _write_stamp(doc)
    print("[ghidra] labels re-applied")
    return 0


def verify(rvas=(), *, timeout: float | None = None) -> int:
    """Read chosen addresses back OUT of the applied database."""
    if not exists():
        print(f"[ghidra] no project at {project_file()} - run "
              f"`homm1 ghidra build` first")
        return 2
    ghidra.install_dir()
    rvas = tuple(rvas) or SAMPLE_RVAS
    ghidra.headless(retail_exe(), PROJECT_DIR, PROJECT_NAME, [DUMP],
                    analyze=False, program_name=PROGRAM_NAME, timeout=timeout,
                    env={"HOMM1_GHIDRA_DUMP":
                         ",".join(hex(r) for r in rvas)})
    return 0


def status() -> int:
    """What exists, what it holds, and whether it is stale - no JVM."""
    doc = export.payload()
    stamp = _stamp()
    print(f"[ghidra] install     : "
          f"{'Ghidra ' + ghidra.version() if ghidra.available() else 'NOT AVAILABLE'}")
    print(f"[ghidra] project     : {project_file()} "
          f"{'(present)' if exists() else '(absent)'}")
    print(f"[ghidra] payload     : {export.PAYLOAD} "
          f"{'(present)' if export.PAYLOAD.is_file() else '(absent)'}")
    print(f"[ghidra] model digest: {doc['digest'][:12]}")
    print(f"[ghidra] applied     : {stamp.get('digest', '(never)')[:12]}")
    if not exists():
        print("[ghidra] state       : run `homm1 ghidra build`")
    elif stamp.get("digest") == doc["digest"]:
        print("[ghidra] state       : up to date")
    else:
        print("[ghidra] state       : STALE - run `homm1 ghidra update`")
    _report(doc)
    return 0


def main(argv: list[str] | None = None) -> int:
    import argparse
    import sys
    ap = argparse.ArgumentParser(
        prog="homm1 ghidra", description=__doc__,
        formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("verb", choices=("build", "update", "verify", "status"))
    ap.add_argument("rvas", nargs="*",
                    help="verify: addresses to read back (hex, e.g. 0x153810)")
    ap.add_argument("--force", action="store_true",
                    help="build: discard the project first; update: re-apply "
                         "even when the digest is unchanged")
    ap.add_argument("--aggressive", action="store_true",
                    help="build: enable Ghidra's Aggressive Instruction Finder")
    ap.add_argument("--no-bookmarks", action="store_true",
                    help="build/update: skip the per-row bookmarks")
    ap.add_argument("--timeout", type=float, default=None,
                    help="seconds to allow the headless run (default: none)")
    a = ap.parse_args(argv)
    try:
        if a.verb == "build":
            return build(force=a.force, aggressive=a.aggressive,
                         bookmarks=not a.no_bookmarks, timeout=a.timeout)
        if a.verb == "update":
            return update(force=a.force, bookmarks=not a.no_bookmarks,
                          timeout=a.timeout)
        if a.verb == "verify":
            rvas = []
            for r in a.rvas:
                try:
                    rvas.append(int(r, 0))
                except ValueError:
                    print(f"[ghidra] verify: {r!r} is not an address "
                          "(hex, e.g. 0x153810)", file=sys.stderr)
                    return 2
            return verify(rvas, timeout=a.timeout)
        return status()
    except ToolError as e:
        print(f"[ghidra] {e}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())

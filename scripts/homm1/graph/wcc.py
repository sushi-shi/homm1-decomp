"""Compile with Watcom 10.0a, retain OMF for linking, convert COFF for objdiff."""

from __future__ import annotations

import os
from pathlib import Path
import shutil
import sys

from homm1.graph.cc import install
from homm1.tool import ToolError
from homm1.tool import objconv_omf, wcc386


def install_raw(data: bytes, out: Path) -> bool:
    if out.exists() and out.read_bytes() == data:
        return False
    out.parent.mkdir(parents=True, exist_ok=True)
    tmp = out.with_name(f"{out.name}.{os.getpid()}.install")
    try:
        tmp.write_bytes(data)
        tmp.replace(out)
    except OSError:
        tmp.unlink(missing_ok=True)
        raise
    return True


def compile_unit(src: Path | str, coff_out: Path | str, omf_out: Path | str,
                 flags: list[str]) -> tuple[bool, bool]:
    src, coff_out, omf_out = Path(src), Path(coff_out), Path(omf_out)
    scratch = coff_out.parent / ".tmp" / f"wcc-{os.getpid()}"
    scratch.mkdir(parents=True, exist_ok=True)
    # Both public outputs conventionally end in .obj; keep distinct staging
    # names so the converter never unlinks its own input.
    omf = scratch / f"{omf_out.stem}.omf.obj"
    coff = scratch / f"{coff_out.stem}.coff.obj"
    try:
        wcc386.compile(src, omf, flags)
        objconv_omf.convert(omf, coff)
        return install(coff.read_bytes(), coff_out), \
            install_raw(omf.read_bytes(), omf_out)
    finally:
        shutil.rmtree(scratch, ignore_errors=True)


from homm1.core.usage import logged


@logged
def main(argv: list[str] | None = None) -> int:
    import argparse
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--out", required=True)
    ap.add_argument("--omf-out", required=True)
    ap.add_argument("--src", required=True)
    ap.add_argument("--unit")
    ap.add_argument("flags", nargs=argparse.REMAINDER)
    a = ap.parse_args(argv)
    flags = a.flags[1:] if a.flags and a.flags[0] == "--" else a.flags
    try:
        compile_unit(a.src, a.out, a.omf_out, flags)
    except (ToolError, OSError) as e:
        print(f"[wcc386] {a.unit or a.src}: {e}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

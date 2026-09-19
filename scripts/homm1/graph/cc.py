"""homm1.graph.cc - the `cl` edge driver: compile, stabilise, write-if-changed.

    python3 -m homm1.graph.cc --out <obj> --src <src> [--unit U] -- <cl flags>

homm1.tool.cl runs the compiler (and already owns the wine plumbing: the
persistent wineserver, the temp-FILE capture and process-group kill that keep
an unreaped grandchild from wedging ninja's pipe, and "the produced .obj is
the success signal, never the return code"). This module is what makes the
EDGE incremental, which needs two more things cl 5.0 does not give:

  * cl stamps every COFF header with the wall-clock TimeDateStamp, so two
    compiles of one unchanged TU differ in bytes 4..7 AND NOWHERE ELSE
    (measured on this toolchain). Left alone, every rebuild dirties every
    object and everything downstream of it re-runs for no reason.
  * ninja prunes a subtree only when an output's mtime does not move, so an
    object whose content did not change must not be rewritten at all.

So: compile into `<base-dir>/.tmp/`, compare against the installed object
with the TimeDateStamp field masked out, and install only on a real
difference - zeroing the stamp on the way in, so objects converge to
byte-reproducible content. With `restat = 1` on the `cl` rule that makes an
unchanged recompile a genuine no-op for labels / normalize / report.

Zeroing is matching-NEUTRAL: TimeDateStamp is COFF header metadata, lives in
no section, is named by no relocation, and neither objdiff, the delinker nor
link.exe reads it. The temp directory is a dotted subdirectory of the object
tree on purpose - the model's `build/objdiff/base` readers glob `*.obj`, and
a sibling temp file would enrol into the data manifest as a phantom unit.

Both scratch paths are per-PROCESS. Two `homm1 build` runs in one tree (an
orchestrator and a matcher, two agents, a rebuild started while the last one
still ran) are not serialised by anything - ninja takes no lock - and both
scratch names used to be shared:

  * a shared `.tmp/<unit>.obj` let one run's `cl.compile()` unlink the other's
    finished object before its `install()` read it, which surfaced as "cl
    produced no object (rc=0)" with an EMPTY diagnostic - the compiler blamed
    for a collision - and as a FileNotFoundError traceback out of the rename;
  * a shared `<unit>.obj.install` is worse, because it makes the rename stop
    being atomic ACROSS processes: A can `os.replace` the temp while B is
    still writing its own bytes into it, so the object PUBLISHED under a valid
    name is a prefix of B's payload. Observed once in this tree -
    homm1.compare.normalize failed a real build with "COFF string table is
    not final" and a census found exactly one invalid object - and transient,
    since the next write repaired it.

`install()` additionally refuses a payload that is not a complete COFF. That
covers the other way an incomplete object reaches the tree (a cl killed
mid-write, a full disk), which nothing checked before: the edge must FAIL, not
enrol a short object that labels, the delink manifests and normalize then read.
"""

from __future__ import annotations

import os
import shutil
import struct
import sys
from pathlib import Path

from homm1.tool import ToolError

#: COFF file header: Machine(2) NumberOfSections(2) TimeDateStamp(4) ...
_MACHINE_I386 = 0x14C
_TIMESTAMP_OFFSET = 4
_HEADER_SIZE = 20
_SECTION_SIZE = 40
_SYMBOL_SIZE = 18


def stabilise(data: bytes) -> bytes:
    """`data` with the COFF TimeDateStamp zeroed; non-COFF input unchanged."""
    if len(data) < 20 or struct.unpack_from("<H", data, 0)[0] != _MACHINE_I386:
        return data
    buf = bytearray(data)
    struct.pack_into("<I", buf, _TIMESTAMP_OFFSET, 0)
    return bytes(buf)


def coff_defect(data: bytes) -> str | None:
    """A one-line reason `data` is not a COMPLETE i386 COFF, else None.

    Structural reach only - every offset the header promises must land inside
    the file. That is exactly what a torn write breaks and what no consumer
    downstream re-checks before trusting the object.
    """
    if len(data) < _HEADER_SIZE:
        return f"{len(data)} byte(s) - shorter than a COFF file header"
    machine, nsec = struct.unpack_from("<HH", data, 0)
    if machine != _MACHINE_I386:
        return f"machine 0x{machine:x} is not i386 (0x{_MACHINE_I386:x})"
    symptr, nsym = struct.unpack_from("<II", data, 8)
    end = _HEADER_SIZE + _SECTION_SIZE * nsec
    if end > len(data):
        return f"{nsec} section header(s) run past the end of the file"
    for i in range(nsec):
        raw = _HEADER_SIZE + _SECTION_SIZE * i
        size, ptr = struct.unpack_from("<II", data, raw + 16)
        nrel, = struct.unpack_from("<H", data, raw + 32)
        relptr, = struct.unpack_from("<I", data, raw + 24)
        if ptr and ptr + size > len(data):
            return f"section {i + 1}'s raw data runs past the end of the file"
        if relptr and relptr + 10 * nrel > len(data):
            return f"section {i + 1}'s relocations run past the end of the file"
    if not symptr:
        return None                       # a stripped object carries no table
    strtab = symptr + _SYMBOL_SIZE * nsym
    if strtab + 4 > len(data):
        return "the symbol table runs past the end of the file"
    strsize, = struct.unpack_from("<I", data, strtab)
    if strsize >= 4 and strtab + strsize > len(data):
        return "the string table is truncated"
    return None


def install(new: bytes, out: Path) -> bool:
    """Write `new` to `out` if its stable form differs; True when it changed.

    Raises ToolError rather than installing an incomplete object.
    """
    defect = coff_defect(new)
    if defect is not None:
        raise ToolError(f"refusing to install {out.name}: {defect}")
    stable = stabilise(new)
    if out.exists() and stabilise(out.read_bytes()) == stable:
        return False
    tmp = out.with_name(f"{out.name}.{os.getpid()}.install")
    try:
        tmp.write_bytes(stable)
        tmp.replace(out)
    except OSError:
        tmp.unlink(missing_ok=True)
        raise
    return True


def compile_unit(src: Path | str, out: Path | str, flags: list[str]) -> bool:
    """Compile one TU into `out`. Returns True when the object changed."""
    from homm1.tool import cl

    src, out = Path(src), Path(out)
    out.parent.mkdir(parents=True, exist_ok=True)
    # Per-PROCESS scratch DIRECTORY, so the staged file keeps the unit's own
    # name (cl's /Fo is verbatim) while two concurrent builds of one unit
    # cannot delete or half-read each other's object.
    scratch = out.parent / ".tmp" / str(os.getpid())
    scratch.mkdir(parents=True, exist_ok=True)
    staged = scratch / out.name
    try:
        cl.compile(src, staged, flags)
        return install(staged.read_bytes(), out)
    finally:
        shutil.rmtree(scratch, ignore_errors=True)


def main(argv: list[str] | None = None) -> int:
    import argparse
    ap = argparse.ArgumentParser(
        prog="homm1.graph.cc", description=__doc__,
        formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--out", required=True)
    ap.add_argument("--src", required=True)
    ap.add_argument("--unit", help="manifest unit name (diagnostics only)")
    ap.add_argument("flags", nargs=argparse.REMAINDER,
                    help="cl flags after `--`")
    a = ap.parse_args(argv)
    flags = a.flags[1:] if a.flags and a.flags[0] == "--" else a.flags
    try:
        compile_unit(a.src, a.out, flags)
    except ToolError as e:
        print(f"[cl] {a.unit or a.src}: {e}", file=sys.stderr)
        return 1
    except OSError as e:
        # An unwritable/full object tree used to reach the ninja log as a
        # PermissionError traceback out of the staging mkdir.
        print(f"[cl] {a.unit or a.src}: cannot write {a.out}: {e}",
              file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

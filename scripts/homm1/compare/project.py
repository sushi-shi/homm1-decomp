"""Write the objdiff project file pairing normalized base <-> normalized target.

    python3 -m homm1.compare.project --target-dir T --out-dir O

  base   : ./base/<unit>.obj        (VC4 /Od, then data-name normalized)
  target : ./target/<unit>.c.obj    (delinked, then normalized)

Symbols are pre-named on both sides (cdecl `_<name>`), so objdiff pairs them
directly with no `symbol_mappings` overlay. EVERY manifest unit gets a base
entry. Which units have a TARGET is read off the target directory the delinker
wrote, never predicted; a unit with no delinked target pairs against an empty
`dummy.obj` and lists at 0%.

That distinction is load-bearing. Predicting the named set from a label census
is what once left two data-only units (`logicdispatchinit`, `stringstaticpool`)
pointing at the dummy while their real target objs sat unopened beside them:
objdiff scores an empty pairing 100.00% on every measure with zero totals, so
both reported MATCHING while being entirely unscored.
"""

from __future__ import annotations

import argparse
import json
import struct
from pathlib import Path

from homm1.compare.normalize import target_object

SCHEMA = "https://raw.githubusercontent.com/encounter/objdiff/main/config.schema.json"
#: Strict relocation scoring: require name/address identity AND the pointed-to
#: data value. This makes REL32 callee identity visible and keeps the DATA-level
#: check; our pinned objdiff also compares absolute DIR32 addends.
OPTIONS = {"functionRelocDiffs": "all"}
DEFAULT_PLATFORM = "win32"
DEFAULT_COMPILER = "vc40"


def write_dummy(path: Path) -> None:
    """A minimal valid empty i386 COFF (.text, no symbols) for units whose
    target side has no named functions yet, so objdiff still lists them."""
    symbol_table_offset = 20 + 40  # header + 1 section header
    header = struct.pack(
        "<HHIIIHH",
        0x14C,                 # Machine: IMAGE_FILE_MACHINE_I386
        1,                     # NumberOfSections
        0,                     # TimeDateStamp
        symbol_table_offset,   # PointerToSymbolTable
        0,                     # NumberOfSymbols
        0,                     # SizeOfOptionalHeader
        0,                     # Characteristics
    )
    section = struct.pack(
        "<8sIIIIIIHHI",
        b".text\0\0\0",
        0, 0, 0, 0, 0, 0, 0, 0,
        0x60000020,            # CODE | EXECUTE | READ
    )
    string_table = struct.pack("<I", 4)  # empty string table (size field only)
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_bytes(header + section + string_table)


def project(units: list[dict], target_dir: Path, out_dir: Path, *,
            platform: str = DEFAULT_PLATFORM,
            compiler: str = DEFAULT_COMPILER,
            base_subdir: str = "base", target_subdir: str = "target") -> Path:
    """Emit `<out_dir>/objdiff.json` (+ dummy.obj). Returns the project file."""
    target_dir, out_dir = Path(target_dir), Path(out_dir)
    out_dir.mkdir(parents=True, exist_ok=True)
    write_dummy(out_dir / "dummy.obj")

    entries = []
    for u in units:
        unit = u["unit"] if isinstance(u, dict) else str(u)
        delinked = target_object(target_dir, unit)
        target_path = (f"./{target_subdir}/"
                       f"{delinked.relative_to(target_dir).as_posix()}"
                       if delinked is not None else "./dummy.obj")
        entries.append({
            "name": unit,
            "target_path": target_path,
            "base_path": f"./{base_subdir}/{unit}.obj",
            "scratch": {"platform": platform, "compiler": compiler},
        })
    obj = {
        "$schema": SCHEMA,
        "build_base": False,
        "build_target": False,
        "options": OPTIONS,
        "watch_patterns": ["*.obj"],
        "units": entries,
    }
    path = out_dir / "objdiff.json"
    with path.open("w", encoding="utf-8") as f:
        json.dump(obj, f, indent=2)
        f.write("\n")
    return path


def main(argv=None) -> int:
    ap = argparse.ArgumentParser(
        prog="python3 -m homm1.compare.project", description=__doc__,
        formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--target-dir", required=True, type=Path)
    ap.add_argument("--out-dir", required=True, type=Path)
    a = ap.parse_args(argv)
    from homm1.manifest import load
    manifest = load()
    build = manifest.get("build", {})
    path = project(manifest.get("unit", []), a.target_dir, a.out_dir,
                   platform=build.get("platform", DEFAULT_PLATFORM),
                   compiler=build.get("compiler", DEFAULT_COMPILER))
    print(f"[project] {path}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

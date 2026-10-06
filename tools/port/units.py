#!/usr/bin/env python3
"""List the sources of a program for the native build.

    python3 tools/port/units.py --target game|editor

Reads the program's units from build.json, the Visual C++ build's manifest,
so both builds compile the same game code. The units bound to Windows and the
assembly routines are replaced by their native counterparts under src/PORT;
the port's own additions for the program follow. Prints one path relative to
the repository per line.
"""
from __future__ import annotations

import argparse
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]

# Windows-bound units and their native counterparts.
REPLACED = {
    "src/BASE/Audio.cpp": "src/PORT/BASE/Audio.cpp",
    "src/SOURCE/kbwin.cpp": "src/PORT/SOURCE/kbwin.cpp",
    "src/SOURCE/wingraph.cpp": "src/PORT/SOURCE/wingraph.cpp",
    "src/SOURCE/netwin.cpp": "src/PORT/SOURCE/netwin.cpp",
    "src/SOURCE/comwin.cpp": "src/PORT/SOURCE/comwin.cpp",
}

# Native units with no Visual C++ counterpart, per program.
ADDED = {
    "game": ["src/PORT/SOURCE/Smacker.cpp", "src/PORT/SOURCE/Menu.cpp"],
    "editor": ["src/PORT/SOURCE/Menu.cpp"],
}

# The entry point, kept apart so that tests can link a program without it.
MAIN = "src/PORT/SOURCE/Main.cpp"


def sources(target: str) -> list[str]:
    manifest = json.loads((ROOT / "build.json").read_text())
    units = manifest["targets"][target]["units"]
    result = []
    for unit in units:
        source = unit["source"]
        if source.endswith(".asm"):
            # The assembly routines' portable C++ is named after the unit.
            source = f"src/PORT/BASE/{unit['unit'].split('/')[-1]}.cpp"
        source = REPLACED.get(source, source)
        if not (ROOT / source).is_file():
            raise SystemExit(f"{target}: no native source for unit {unit['unit']} ({source})")
        if source not in result:
            result.append(source)
    for source in ADDED[target]:
        if source not in result:
            result.append(source)
    return result


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__,
                                     formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--target", required=True, choices=sorted(ADDED))
    parser.add_argument("--main", action="store_true", help="print only the entry point")
    args = parser.parse_args()
    print(MAIN if args.main else "\n".join(sources(args.target)))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

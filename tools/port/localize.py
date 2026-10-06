#!/usr/bin/env python3
"""Resolve localization::Tr/Chars calls for the native build.

    python3 tools/port/localize.py --locale ru --out BUILD/localized [--stamp FILE]

Renders every source and header under src/, include/ and vendor/ into OUT with
the selected catalog, as build.py does for the Visual C++ build: each message
becomes a literal in the language's Windows code page, the encoding the game's
fonts index. Files whose rendered text is unchanged are not rewritten, so an
incremental build recompiles only what changed.
"""
from __future__ import annotations

import argparse
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT))

from catalog import Catalog  # noqa: E402

SUFFIXES = (".cpp", ".h", ".c", ".hpp", ".inc")


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__,
                                     formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--locale", default="ru")
    parser.add_argument("--out", type=Path, required=True)
    parser.add_argument("--stamp", type=Path)
    args = parser.parse_args()
    try:
        catalog = Catalog.load(ROOT)
    except ValueError as error:
        raise SystemExit(f"locales/ is invalid (python3 catalog.py check):\n{error}")
    written = 0
    for directory in ("src", "include", "vendor"):
        for source in sorted((ROOT / directory).rglob("*")):
            if not source.is_file() or source.suffix not in SUFFIXES:
                continue
            target = args.out / source.relative_to(ROOT)
            text = catalog.render(source.read_text(encoding="utf-8"), locale=args.locale,
                                  expanded=True)
            data = text.encode("utf-8", "surrogateescape")
            if target.exists() and target.read_bytes() == data:
                continue
            target.parent.mkdir(parents=True, exist_ok=True)
            target.write_bytes(data)
            written += 1
    if args.stamp:
        args.stamp.parent.mkdir(parents=True, exist_ok=True)
        args.stamp.write_text(f"{args.locale}\n")
    print(f"[localize] {args.locale}: {written} file(s) updated in {args.out}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

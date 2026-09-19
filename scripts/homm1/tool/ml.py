"""Assemble source with the pinned period MASM as OMF or comparison-only COFF."""

from __future__ import annotations

from pathlib import Path

from homm1.tool import ToolError
from homm1.tool.wine import era_tool, run, winepath


def assemble(src: Path | str, out: Path | str, *, coff: bool = False) -> None:
    src, out = Path(src).resolve(), Path(out).resolve()
    if not src.is_file():
        raise ToolError(f"assembly source is missing: {src}")
    out.parent.mkdir(parents=True, exist_ok=True)
    out.unlink(missing_ok=True)
    assembler = era_tool("ml.exe")
    flags = ["/nologo", "/c"]
    if coff:
        flags.append("/coff")
    argv = ["wine", str(assembler), *flags,
            f"/Fo{winepath(out)}", winepath(src)]
    output, rc = run(argv, cwd=out.parent, success=out)
    if not out.is_file():
        raise ToolError(f"MASM failed for {src.name} (rc={rc}): "
                        f"{output.strip()}")


def main() -> int:
    import argparse
    import sys
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--src", required=True)
    parser.add_argument("--out", required=True)
    parser.add_argument(
        "--coff", action="store_true",
        help="emit COFF for objdiff; the retail link continues to use OMF")
    args = parser.parse_args()
    try:
        assemble(args.src, args.out, coff=args.coff)
    except (ToolError, OSError) as error:
        print(f"[ml] {error}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

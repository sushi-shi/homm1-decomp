"""Convert a Watcom Easy-OMF object to comparison-only i386 COFF.

Watcom names its code segment ``_TEXT`` while the delinker emits ``.text``.
Rename that section during conversion so objdiff can pair the code sections;
the native OMF object used by the linker remains untouched.
"""

from __future__ import annotations

from pathlib import Path
import subprocess

from homm1.tool import ToolError
from homm1.tool.wine import require


def convert(src: Path | str, out: Path | str) -> str:
    src, out = Path(src).resolve(), Path(out).resolve()
    if not src.is_file():
        raise ToolError(f"OMF object missing: {src}")
    out.parent.mkdir(parents=True, exist_ok=True)
    out.unlink(missing_ok=True)
    result = subprocess.run(
        [require("objconv-omf"), "-fcoff32", "-v0", "-nr:_TEXT:.text",
         str(src), str(out)],
        stdin=subprocess.DEVNULL, capture_output=True, text=True)
    output = result.stdout + result.stderr
    if result.returncode or not out.is_file():
        out.unlink(missing_ok=True)
        tail = "\n".join(output.strip().splitlines()[-12:]) or \
            "(objconv-omf said nothing)"
        raise ToolError(f"OMF conversion failed for {src.name}:\n{tail}")
    return output


from homm1.core.usage import logged


@logged
def main() -> int:
    import argparse
    import sys
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--out", required=True)
    ap.add_argument("--src", required=True)
    a = ap.parse_args()
    try:
        convert(a.src, a.out)
    except (ToolError, OSError) as e:
        print(f"[objconv-omf] {e}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

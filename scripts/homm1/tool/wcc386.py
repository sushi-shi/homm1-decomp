"""Run the pinned September 1994 Watcom C/386 10.0a compiler.

The compiler emits Phar Lap Easy OMF.  Conversion for objdiff belongs to the
graph driver; this layer returns the compiler's original object unchanged.
"""

from __future__ import annotations

import os
from pathlib import Path
import re

from homm1 import toolchain
from homm1.core.paths import INCLUDE
from homm1.tool import ToolError
from homm1.tool.wine import require, run, winepath


def compile(src: Path | str, out: Path | str, flags: list[str], *,
            timeout: float | None = None) -> str:
    """Compile one C TU to its native Watcom OMF object."""
    src, out = Path(src).resolve(), Path(out).resolve()
    if not src.is_file():
        raise ToolError(f"source missing: {src}")
    root = toolchain.verify("watcom10")
    compiler = root / "BINNT/WCC386.EXE"
    require("wine")
    out.parent.mkdir(parents=True, exist_ok=True)
    out.unlink(missing_ok=True)

    environment = dict(os.environ)
    environment["WATCOM"] = winepath(root)
    environment["INCLUDE"] = winepath(INCLUDE)
    # Compiler-specific environment options must not amend the manifest.
    environment.pop("WCC386", None)
    argv = ["wine", str(compiler), f"-i={winepath(INCLUDE)}", *flags,
            f"-fo={winepath(out)}", winepath(src)]
    output, rc = run(argv, cwd=out.parent, env=environment,
                     timeout=timeout, success=out)
    if not out.is_file():
        tail = "\n".join(output.strip().splitlines()[-12:]) or \
            "(wcc386 said nothing)"
        if not re.search(r"\b(error|fatal)\b", output, re.I):
            tail += f"\n(wcc386 produced no object; return code {rc})"
        raise ToolError(f"wcc386 produced no object for {src.name}:\n{tail}")
    if out.read_bytes()[:1] != b"\x80":
        out.unlink(missing_ok=True)
        raise ToolError(f"wcc386 output for {src.name} is not OMF")
    return output


from homm1.core.usage import logged


@logged
def main() -> int:
    import argparse
    import sys
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--out", required=True)
    ap.add_argument("--src", required=True)
    ap.add_argument("flags", nargs=argparse.REMAINDER)
    a = ap.parse_args()
    flags = a.flags[1:] if a.flags and a.flags[0] == "--" else a.flags
    try:
        compile(a.src, a.out, flags)
    except (ToolError, OSError) as e:
        print(f"[wcc386] {e}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

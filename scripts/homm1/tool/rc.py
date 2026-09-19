"""homm1.tool.rc - the era resource compiler (.rc -> .res).

    homm1 tool rc --out <res> --src <rc>

In-process (same function):
    from homm1.tool import rc
    rc.compile(rc_path, res_path)

RC.EXE (toolchain r2+, from VS97 SHAREDIDE/BIN) is a thin driver over
RCDLL.DLL. Repo include/ and the source's own directory are passed as /i;
system resource headers (winres/afxres) resolve via the wine registry INCLUDE.
The success signal is the produced .res.
"""

from __future__ import annotations

from pathlib import Path

from homm1.core.paths import INCLUDE
from homm1.tool import ToolError
from homm1.tool.wine import era_tool, run, winepath


def compile(src: Path | str, out: Path | str, *, flags: list[str] = (),
            extra_includes: list[Path] = (), timeout: float | None = None) -> str:
    """Compile one .rc; return rc.exe's output. Raises ToolError without a .res."""
    src, out = Path(src).resolve(), Path(out).resolve()
    if not src.exists():
        raise ToolError(f"resource script missing: {src}")
    out.parent.mkdir(parents=True, exist_ok=True)
    out.unlink(missing_ok=True)

    rc_exe = era_tool("rc.exe")
    inc = [src.parent, *( [INCLUDE] if INCLUDE.is_dir() else [] ), *extra_includes]
    argv = ["wine", str(rc_exe), *[f"/i{winepath(d)}" for d in inc],
            *flags, f"/fo{winepath(out)}", winepath(src)]
    output, rc_ = run(argv, cwd=out.parent, timeout=timeout, success=out)
    if not out.exists():
        tail = "\n".join(output.strip().splitlines()[-12:])
        raise ToolError(f"rc produced no .res for {src.name} (rc={rc_}):\n{tail}")
    return output


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
        compile(a.src, a.out, flags=flags)
    except (ToolError, OSError) as e:
        print(f"[rc] {e}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

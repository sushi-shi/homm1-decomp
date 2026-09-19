"""homm1.tool.cl - the era compiler.

    homm1 tool cl --out <obj> --src <src> -- /nologo /c /O2 /MT

In-process (same function):
    from homm1.tool import cl
    cl.compile(src, obj, ["/nologo", "/c", "/O2", "/MT"])

ninja rule lines invoke `python3 -m homm1.tool.cl` directly - the exact
interpreter+module with no wrapper dispatch; the umbrella `homm1 tool cl`
forwards to the same main().

System headers/libs come from the wine registry INCLUDE/LIB (init_prefix);
repo-local include/ and vendor/<sdk>/ dirs are passed as /I so
`#include <Module/Foo.h>` and the vendored SDK headers resolve. Wine spews
unrelated noise and can return a non-cl exit code, so the success signal is
the produced .obj, never the return code alone.
"""

from __future__ import annotations

import re
from pathlib import Path

from homm1.core.paths import INCLUDE, VENDOR
from homm1.tool import ToolError
from homm1.tool.wine import era_tool, run, winepath


def repo_include_flags() -> list[str]:
    dirs = []
    if INCLUDE.is_dir():
        dirs.append(INCLUDE)
    if VENDOR.is_dir():
        dirs += sorted(d for d in VENDOR.iterdir() if d.is_dir())
    return [f"/I{winepath(d)}" for d in dirs]


def compile(src: Path | str, out: Path | str, flags: list[str], *,
            extra_includes: list[Path] = (), timeout: float | None = None) -> str:
    """Compile one TU; return cl's output. Raises ToolError without an .obj."""
    src, out = Path(src).resolve(), Path(out).resolve()
    if not src.exists():
        raise ToolError(f"source missing: {src}")
    out.parent.mkdir(parents=True, exist_ok=True)
    out.unlink(missing_ok=True)

    cl_exe = era_tool("cl.exe")
    argv = ["wine", str(cl_exe), *repo_include_flags(),
            *[f"/I{winepath(d)}" for d in extra_includes],
            *flags, f"/Fo{winepath(out)}", winepath(src)]
    output, rc = run(argv, cwd=out.parent, timeout=timeout, success=out)
    if not out.exists():
        tail = "\n".join(output.strip().splitlines()[-12:]) or "(cl said nothing)"
        if not re.search(r"\b(error|fatal)\b", output, re.I):
            # cl naming no diagnostic at all means the object did not survive
            # rather than fail to compile - the output tree moved, filled up,
            # or another writer removed it.
            tail += (f"\n(cl reported no error; {out} simply is not there - "
                     "check the output tree, not the source)")
        raise ToolError(f"cl produced no object for {src.name} (rc={rc}):\n{tail}")
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
        print(f"[cl] {e}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

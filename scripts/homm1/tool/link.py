"""homm1.tool.link - the era linker (the pinned VC4 LINK.EXE).

    homm1 tool link --expect <exe> -- /NOLOGO /OUT:<exe.w> <objs.w...>

In-process (same function):
    from homm1.tool import link
    link.link(["/NOLOGO", "/OUT:" + wine.winepath(exe), *obj_args], expect=[exe])

Callers build the full argument list themselves (link lines are the caller's
policy - candidate link order, /DLL /NOENTRY import-lib synthesis, ...); this
module only guarantees the tool loads with MSPDB40.DLL and that the
expected artifacts exist afterwards. Libraries resolve via the wine registry
LIB (init_prefix).
"""

from __future__ import annotations

from pathlib import Path

from homm1.tool import ToolError
from homm1.tool.wine import ensure_link_deps, era_tool, run


def link(args: list[str], *, cwd: Path | None = None,
         expect: list[Path] = (), timeout: float | None = None) -> str:
    """Run link.exe with `args`; verify every `expect` path exists after."""
    ensure_link_deps()
    link_exe = era_tool("link.exe")
    expect = [Path(p) for p in expect]
    for p in expect:
        p.unlink(missing_ok=True)
    output, rc = run(["wine", str(link_exe), *args], cwd=cwd, timeout=timeout,
                     success=expect[0] if expect else None)
    missing = [p for p in expect if not p.exists()]
    if missing or (not expect and rc != 0):
        tail = "\n".join(output.strip().splitlines()[-12:])
        what = missing[0].name if missing else f"rc={rc}"
        raise ToolError(f"link failed ({what}):\n{tail}")
    return output


def main() -> int:
    import argparse
    import sys
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--expect", action="append", default=[],
                    help="artifact that must exist afterwards (repeatable)")
    ap.add_argument("args", nargs=argparse.REMAINDER)
    a = ap.parse_args()
    args = a.args[1:] if a.args and a.args[0] == "--" else a.args
    try:
        out = link(args, expect=[Path(p) for p in a.expect])
        if out.strip():
            print(out)
    except (ToolError, OSError) as e:
        print(f"[link] {e}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

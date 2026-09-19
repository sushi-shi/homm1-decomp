"""homm1.tool.objdiff - the objdiff report generator.

    homm1 tool objdiff --project <dir> --out <report.json>

In-process (same function):
    from homm1.tool import objdiff
    objdiff.report(project_dir, report_json)

objdiff-cli (pinned 3.7.3, on PATH from the dev shell) reads `<project>/objdiff.json`
and scores every unit pairing in it, writing the progress report. It is a native
tool - no wine. The success signal is the produced report file, never the return
code alone.

This module RUNS objdiff-cli; it does not interpret the report. Reading and
diffing scores is homm1.compare's business.
"""

from __future__ import annotations

import json
import shutil
import subprocess
from pathlib import Path

from homm1.tool import ToolError

OBJDIFF_CLI = "objdiff-cli"


def version(exe: str = OBJDIFF_CLI) -> str:
    """objdiff-cli's self-reported version, e.g. `objdiff-cli 3.7.3`."""
    if shutil.which(exe) is None:
        raise ToolError(f"{exe} not found on PATH - run inside `nix develop`")
    proc = subprocess.run([exe, "--version"], capture_output=True, text=True)
    return (proc.stdout or proc.stderr).strip()


def report(project_dir: Path | str, out: Path | str, *,
           deduplicate: bool = False, format: str = "json",
           config: list[str] = (), exe: str = OBJDIFF_CLI,
           timeout: float | None = None) -> Path:
    """Generate the progress report for a project dir. Returns the report path."""
    project_dir, out = Path(project_dir).resolve(), Path(out).resolve()
    if not (project_dir / "objdiff.json").exists():
        raise ToolError(f"no objdiff.json in {project_dir}")
    if shutil.which(exe) is None:
        raise ToolError(f"{exe} not found on PATH - run inside `nix develop`")
    out.parent.mkdir(parents=True, exist_ok=True)
    out.unlink(missing_ok=True)

    argv = [exe, "report", "generate", "-p", str(project_dir), "-o", str(out)]
    if deduplicate:
        argv.append("-d")
    if format != "json":
        argv += ["-f", format]
    for item in config:
        argv += ["-c", item]
    proc = subprocess.run(argv, capture_output=True, text=True, timeout=timeout)
    if not out.exists():
        tail = "\n".join((proc.stderr or proc.stdout).strip().splitlines()[-12:])
        raise ToolError(f"objdiff-cli produced no report (rc={proc.returncode}):\n{tail}")
    return out


def load(path: Path | str) -> dict:
    """Read a generated report. A convenience for callers; no interpretation."""
    return json.loads(Path(path).read_text(encoding="utf-8"))


from homm1.core.usage import logged


@logged
def main() -> int:
    import argparse
    import sys
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--project", required=True)
    ap.add_argument("--out", required=True)
    ap.add_argument("--deduplicate", action="store_true")
    ap.add_argument("--format", default="json")
    ap.add_argument("-c", "--config", action="append", default=[])
    a = ap.parse_args()
    try:
        path = report(a.project, a.out, deduplicate=a.deduplicate,
                      format=a.format, config=a.config)
    except ToolError as e:
        print(f"[objdiff] {e}", file=sys.stderr)
        return 1
    print(f"[objdiff] {path}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

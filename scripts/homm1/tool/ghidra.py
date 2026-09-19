"""homm1.tool.ghidra - headless Ghidra, for the one-way viewer export.

    homm1 tool ghidra --exe E --project-dir D --project N --script S [...]

In-process (same function):
    from homm1.tool import ghidra
    out = ghidra.headless(exe, project_dir, "homm1", [apply_py],
                          analyze=False, env={"HOMM1_GHIDRA_PAYLOAD": p})

Ghidra 12.0.4 ships `support/analyzeHeadless`, but its launcher has no PyGhidra
path: a `.py` postScript is routed to the PyGhidra provider and then dies with
"Ghidra was not started with PyGhidra". So the program this module runs is the
PyGhidra bootstrap `homm1/ghidra/headless.py`, executed by THIS interpreter in
a CHILD process - pyghidra.start() boots a JVM that never unloads, and the
pipeline's python must not carry one.

This module RUNS it and returns the output; what to apply and what the result
means is homm1.ghidra's business. Ghidra is optional: no GHIDRA_INSTALL_DIR
and no importable pyghidra is a ToolError, never a silent skip.
"""

from __future__ import annotations

import os
import subprocess
import sys
from pathlib import Path

from homm1.tool import ToolError

#: the PyGhidra bootstrap this module drives (a sibling package's script, not
#: an import - the tool layer stays free of homm1.ghidra's policy)
DRIVER = Path(__file__).resolve().parents[1] / "ghidra" / "headless.py"


def install_dir() -> Path:
    """$GHIDRA_INSTALL_DIR, verified to look like a Ghidra installation."""
    value = os.environ.get("GHIDRA_INSTALL_DIR")
    if not value:
        raise ToolError("$GHIDRA_INSTALL_DIR unset - run inside `nix develop` "
                        "(Ghidra is optional; the export needs it)")
    path = Path(value)
    if not (path / "Ghidra").is_dir():
        raise ToolError(f"$GHIDRA_INSTALL_DIR={path} is not a Ghidra install")
    return path


def version() -> str:
    """The installed Ghidra's version string."""
    props = install_dir() / "Ghidra/application.properties"
    for line in props.read_text().splitlines() if props.is_file() else []:
        if line.startswith("application.version="):
            return line.split("=", 1)[1].strip()
    return "unknown"


def available() -> bool:
    """True when a headless run could actually start."""
    try:
        install_dir()
    except ToolError:
        return False
    import importlib.util
    return importlib.util.find_spec("pyghidra") is not None


def headless(exe: Path | str, project_dir: Path | str, project: str,
             scripts: list[Path | str], *, analyze: bool = True,
             aggressive: bool = False, program_name: str | None = None,
             env: dict[str, str] | None = None,
             timeout: float | None = None, echo: bool = True) -> str:
    """Import/open <project_dir>/<project>.gpr and run `scripts` over it.

    Returns the driver's combined output; raises ToolError on a non-zero exit."""
    install_dir()
    if not DRIVER.is_file():
        raise ToolError(f"PyGhidra bootstrap missing: {DRIVER}")
    exe = Path(exe)
    if not exe.is_file():
        raise ToolError(f"image missing: {exe}")
    project_dir = Path(project_dir)
    project_dir.mkdir(parents=True, exist_ok=True)

    argv = [sys.executable, str(DRIVER), "--exe", str(exe),
            "--project-dir", str(project_dir), "--project", project]
    if program_name:
        argv += ["--program-name", program_name]
    for script in scripts:
        argv += ["--script", str(script)]
    if not analyze:
        argv.append("--no-analyze")
    if aggressive:
        argv.append("--aggressive")

    proc = subprocess.Popen(argv, stdout=subprocess.PIPE,
                            stderr=subprocess.STDOUT, text=True,
                            env={**os.environ, **(env or {})})
    lines = []
    try:
        for line in proc.stdout:
            lines.append(line)
            if echo:
                print(line, end="", flush=True)
        proc.wait(timeout=timeout)
    except subprocess.TimeoutExpired:
        proc.kill()
        raise ToolError(f"headless Ghidra timed out after {timeout}s") from None
    output = "".join(lines)
    if proc.returncode != 0:
        tail = "\n".join(output.strip().splitlines()[-15:])
        raise ToolError(f"headless Ghidra failed (rc {proc.returncode}):\n{tail}")
    return output


def main(argv: list[str] | None = None) -> int:
    import argparse
    ap = argparse.ArgumentParser(prog="homm1 tool ghidra",
                                 description=__doc__.splitlines()[0])
    ap.add_argument("--exe", required=True)
    ap.add_argument("--project-dir", required=True)
    ap.add_argument("--project", default="homm1")
    ap.add_argument("--program-name")
    ap.add_argument("--script", action="append", default=[])
    ap.add_argument("--no-analyze", action="store_true")
    ap.add_argument("--aggressive", action="store_true")
    a = ap.parse_args(argv)
    try:
        headless(a.exe, a.project_dir, a.project, a.script,
                 analyze=not a.no_analyze, aggressive=a.aggressive,
                 program_name=a.program_name)
    except ToolError as e:
        print(f"[ghidra] {e}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

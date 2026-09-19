#!/usr/bin/env python3
"""PyGhidra bootstrap: open/create the viewer project and run GhidraScripts.

    headless.py --exe E --project-dir D --project N --script S [--script S2]
                [--no-analyze] [--aggressive]

Run in its own interpreter by homm1.tool.ghidra (the only layer that spawns
processes). It is deliberately dependency-free - stdlib + pyghidra - because
pyghidra.start() boots a JVM into the calling process and never unloads it;
keeping that out of the pipeline's python is the whole reason this is a
separate program.

Why not analyzeHeadless: Ghidra 12.0.4's support/analyzeHeadless has no
PyGhidra launch path (nothing in support/launch.sh mentions it), so a .py
postScript dies with "Ghidra was not started with PyGhidra". Booting PyGhidra
here and running the same scripts through pyghidra.ghidra_script gives them
the identical GhidraScript environment (currentProgram, monitor, state).

<project-dir>/<project>.{gpr,rep} is a NON-nested project layout. The first run
imports HOMM1.EXE and auto-analyzes it (minutes); afterwards the program is
reused and --no-analyze skips straight to the scripts.
"""

import argparse
import sys
from pathlib import Path

# Also support the script-path launch used by tool.ghidra without PYTHONPATH.
if __package__ in (None, ""):
    sys.path.insert(0, str(Path(__file__).resolve().parents[2]))


from homm1.core.usage import logged


@logged
def main(argv=None):
    ap = argparse.ArgumentParser(prog="headless.py", description=__doc__)
    ap.add_argument("--exe", required=True)
    ap.add_argument("--project-dir", required=True)
    ap.add_argument("--project", required=True)
    ap.add_argument("--program-name",
                    help="name the program carries inside the project "
                         "(default: the image's file name, which for a "
                         "nix-store path drags the store hash along)")
    ap.add_argument("--script", action="append", default=[],
                    help="GhidraScript to run, in order")
    ap.add_argument("--no-analyze", action="store_true")
    ap.add_argument("--aggressive", action="store_true",
                    help="enable the Aggressive Instruction Finder (~4x the "
                         "analysis phase; the export seeds its own admitted "
                         "starts, so this is rarely needed)")
    a = ap.parse_args(argv)

    import pyghidra
    pyghidra.start()

    from ghidra.app.script import GhidraScriptUtil
    from ghidra.program.flatapi import FlatProgramAPI
    from ghidra.program.model.listing import Program
    from pyghidra.core import _analyze_program, _setup_project

    gproject, program = _setup_project(
        binary_path=a.exe,
        project_location=a.project_dir,
        project_name=a.project,
        program_name=a.program_name,
        nested_project_location=False,
    )
    project = gproject.getProject()

    failed = []
    GhidraScriptUtil.acquireBundleHostReference()
    try:
        if not a.no_analyze:
            if a.aggressive:
                opts = program.getOptions(Program.ANALYSIS_PROPERTIES)
                tx = program.startTransaction("enable-aif")
                try:
                    opts.setBoolean("Aggressive Instruction Finder", True)
                finally:
                    program.endTransaction(tx, True)
            # only actually analyzes a program that has never been analyzed
            _analyze_program(FlatProgramAPI(program), program)
        for script in a.script:
            print("[headless] running %s" % Path(script).name, flush=True)
            _, errs = pyghidra.ghidra_script(script, project, program=program)
            # PyGhidraScript CATCHES a script exception and writes it to the
            # error writer, so the only failure signal is that text; without
            # this check a half-applied (rolled back) run would exit 0 and the
            # caller would stamp it as applied.
            if "Traceback" in str(errs):
                failed.append(Path(script).name)
    finally:
        GhidraScriptUtil.releaseBundleHostReference()
        gproject.save(program)
        gproject.close()
    if failed:
        print("[headless] FAILED: %s (see the traceback above)"
              % ", ".join(failed), flush=True)
        return 1
    print("[headless] done: %s/%s" % (a.project_dir, a.project), flush=True)
    return 0


if __name__ == "__main__":
    sys.exit(main())

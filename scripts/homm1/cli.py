"""HoMM1 matching-decompilation command line.

The target-specific input and compiler setup are kept here because the retail
executable and compiler media cannot be fetched by the repository.
"""

from __future__ import annotations

import importlib
import json
import os
from pathlib import Path
import subprocess
import sys


TOOLS = ("wine", "cl", "ml", "link", "rc", "delinker", "pdbutil", "objdiff", "objdump",
         "clangd", "ghidra", "merge_units")


def _init(argv: list[str]) -> int:
    import argparse
    from homm1.core.inputs import REPO, read_verified, stage_executable, targets
    from homm1.core.image import Image

    ap = argparse.ArgumentParser(prog="homm1 init")
    ap.add_argument("--exe", type=Path)
    ap.add_argument("--editor-exe", type=Path)
    a = ap.parse_args(argv)
    pins = targets()
    stage_executable(pins["game"], a.exe)
    selected = ["game"]
    editor = pins["editor"]
    if a.editor_exe is not None or os.environ.get(editor.env_var) \
            or editor.destination.exists():
        stage_executable(editor, a.editor_exe)
        selected.append("editor")
    output = REPO / "build/analysis"
    output.mkdir(parents=True, exist_ok=True)
    for key in selected:
        pin = pins[key]
        report = Image(read_verified(pin, pin.destination)).report()
        path = output / f"{key}.json"
        path.write_text(json.dumps(report, indent=2) + "\n")
        print(f"{key}: verified {pin.name}; report: {path.relative_to(REPO)}")
    print("Retail workspace ready. Run `homm1 toolchain install`, initialise "
          "Wine, then run `homm1 build`.")
    return 0


def _inspect(argv: list[str]) -> int:
    import argparse
    from homm1.core.inputs import read_verified, targets
    from homm1.core.image import Image

    ap = argparse.ArgumentParser(prog="homm1 inspect")
    ap.add_argument("--target", choices=("game", "editor"), default="game")
    ap.add_argument("--json", action="store_true")
    a = ap.parse_args(argv)
    pin = targets()[a.target]
    report = Image(read_verified(pin, pin.destination)).report()
    if a.json:
        print(json.dumps(report, indent=2))
        return 0
    print(f"{a.target}: sha256 {report['sha256']}")
    print(f"base 0x{report['image_base']:08X}, entry 0x{report['entry_va']:08X}, "
          f"linker {report['linker'][0]}.{report['linker'][1]:02d}")
    for section in report["sections"]:
        print(f"{section['name']:8} RVA 0x{section['rva']:08X} "
              f"virtual {section['virtual_size']:7} raw {section['raw_size']:7}")
    return 0


def _toolchain(argv: list[str]) -> int:
    import argparse
    from homm1 import toolchain
    ap = argparse.ArgumentParser(prog="homm1 toolchain")
    ap.add_argument("action", choices=("install", "check", "symbols"))
    from homm1.core.paths import compiler_id
    ap.add_argument("--id", choices=sorted(toolchain.pins()), default=compiler_id())
    ap.add_argument("--media", type=Path)
    ap.add_argument("--patch", type=Path, help="pinned service pack for the selected compiler")
    ap.add_argument("--archive", type=Path,
                    help="install the pinned combined release from a local archive")
    a = ap.parse_args(argv)
    if a.patch is not None and a.media is None:
        ap.error("--patch requires --media")
    if a.media is not None and a.archive is not None:
        ap.error("--media and --archive are mutually exclusive")
    try:
        toolchain.command(a)
    except (OSError, ValueError, subprocess.SubprocessError) as exc:
        print(f"[toolchain] {exc}", file=sys.stderr)
        return 1
    return 0


from homm1.core.usage import logged


@logged
def main(argv: list[str] | None = None) -> int:
    argv = list(sys.argv[1:] if argv is None else argv)
    # `homm1 --image editor <command>` selects the retail image for this
    # process and every child it starts (homm1.core.paths, $HOMM1_IMAGE).
    while argv and (argv[0] == "--image" or argv[0].startswith("--image=")):
        key = argv[0].partition("=")[2] if "=" in argv[0] else (argv[1] if len(argv) > 1 else "")
        argv = argv[1:] if "=" in argv[0] else argv[2:]
        from homm1.core.paths import IMAGE_ENV, images
        if key not in images():
            print(f"homm1: --image expects one of {images()}", file=sys.stderr)
            return 2
        os.environ[IMAGE_ENV] = key
    if not argv or argv[0] in ("-h", "--help"):
        print(__doc__.strip())
        print("\noptions: --image {game,editor} selects the retail image (default game)")
        print("\ncommands: init inspect toolchain configure build link match play labels "
              "model delink compare audit sema walls permute lsp ghidra verify workflow clean "
              "localization tool")
        return 0 if argv else 2
    cmd, rest = argv[0], argv[1:]
    if cmd == "init":
        return _init(rest)
    if cmd == "inspect":
        return _inspect(rest)
    if cmd == "toolchain":
        return _toolchain(rest)
    if cmd in ("labels", "model", "delink", "compare"):
        module = {
            "labels": "homm1.retail_labels.source",
            "model": "homm1.model",
            "delink": "homm1.delink.run",
            "compare": "homm1.compare.run",
        }[cmd]
        sys.argv = [f"homm1 {cmd}", *rest]
        return importlib.import_module(module).main()
    if cmd == "workflow":
        from homm1.workflow import main as workflow_main
        return workflow_main(rest)
    if cmd == "localization":
        from homm1.graph.localization import main as localization_main
        return localization_main(rest)
    if cmd == "clean":
        from homm1.clean.run import main as clean_main
        return clean_main(rest)
    if cmd == "audit":
        audits = {"dna-bands": "dna_bands", "usage": "usage",
                  "census": "census", "placements": "placements"}
        if not rest or rest[0] not in audits:
            print("homm1 audit: expected " + ", ".join(audits), file=sys.stderr)
            return 2
        return importlib.import_module(f"homm1.audit.{audits[rest[0]]}").main(rest[1:])
    if cmd in ("sema", "ghidra", "verify", "walls", "lsp"):
        return importlib.import_module(f"homm1.{cmd}").main(rest)
    if cmd == "permute":
        if not rest or rest[0] in ("-h", "--help"):
            print("homm1 permute candidates [options]\n"
                  "homm1 permute campaign [--rva <rva>] [options]\n"
                  "homm1 permute state --source <tu.cpp> --rva <rva> [options]\n"
                  "homm1 permute variants <tu.cpp> <rva> [options]\n"
                  "  candidates: classify every live source-owned residual\n"
                  "  campaign: run N islands and retain M distinct best solutions\n"
                  "  state: classified, disposable compiler-state search\n"
                  "  variants: reviewed exact axes x AST shapes x TU state")
            return 0 if rest else 2
        if rest[0] in ("candidates", "campaign"):
            from homm1.permute.campaign import main as campaign_main
            return campaign_main(rest)
        if rest[0] not in ("state", "variants"):
            print("homm1 permute: unknown verb " + repr(rest[0])
                  + " (have: candidates, campaign, state, variants)", file=sys.stderr)
            return 2
        verb, permute_args = rest[0], rest[1:]
        if any(value in ("-h", "--help") for value in permute_args):
            if verb == "state":
                from homm1.permute.tu_state_noise import main as permute_main
            else:
                from homm1.permute.match_variants import main as permute_main
            return permute_main(permute_args)
        rva_arg = (
            next((
                permute_args[index + 1]
                for index, value in enumerate(permute_args[:-1])
                if value == "--rva"
            ), None)
            if verb == "state"
            else (permute_args[1] if len(permute_args) >= 2 else None)
        )
        if rva_arg is None:
            print(f"homm1 permute {verb}: an RVA is required", file=sys.stderr)
            return 2
        from contextlib import redirect_stdout
        from io import StringIO
        from homm1.walls.diagnose import diagnose
        diagnosis = StringIO()
        with redirect_stdout(diagnosis):
            result = diagnose(rva_arg)
        report = diagnosis.getvalue()
        print(report, end="")
        if result or "class: REGALLOC/SCHEDULING" not in report:
            print(f"homm1 permute {verb}: refused - permutation requires a "
                  "REGALLOC/SCHEDULING diagnosis", file=sys.stderr)
            return 2
        from homm1.model import resolve
        from homm1.verify.baseline import load as load_baseline
        rva = int(rva_arg, 0)
        if rva >= 0x400000:
            rva -= 0x400000
        binding = next((row for row in resolve().functions if row.rva == rva), None)
        bank = load_baseline().get((binding.unit.rsplit("/", 1)[-1], binding.name)) if binding else None
        if bank and bank["hist"] >= 100.0:
            print(f"homm1 permute {verb}: refused - historical MAX is already "
                  "100%", file=sys.stderr)
            return 2
        if verb == "state":
            from homm1.permute.tu_state_noise import main as permute_main
        else:
            from homm1.permute.match_variants import main as permute_main
        return permute_main(permute_args)
    if cmd in ("build", "link", "match", "play"):
        from homm1.graph.verbs import VERBS
        return VERBS[cmd](rest)
    if cmd == "configure":
        from homm1.graph.emit import main as configure
        sys.argv = ["homm1 configure", *rest]
        return configure()
    if cmd == "tool":
        if not rest or rest[0] not in TOOLS:
            print(f"homm1 tool: pick one of {', '.join(TOOLS)}", file=sys.stderr)
            return 2
        module = importlib.import_module(f"homm1.tool.{rest[0]}")
        sys.argv = [f"homm1 tool {rest[0]}", *rest[1:]]
        return module.main()
    print(f"homm1: unknown command {cmd!r}", file=sys.stderr)
    return 2


if __name__ == "__main__":
    raise SystemExit(main())

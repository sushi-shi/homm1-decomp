"""HoMM1 matching-decompilation command line.

The matching commands are the Gruntz command surface, with the target-specific
input and VC4 setup kept here because the retail executable and compiler media
cannot be fetched by the repository.
"""

from __future__ import annotations

import importlib
import json
import os
from pathlib import Path
import subprocess
import sys


TOOLS = ("wine", "cl", "wcc386", "objconv_omf", "ml", "link", "rc", "delinker", "pdbutil", "objdiff", "objdump",
         "clangd", "ghidra")


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


def _test(_argv: list[str]) -> int:
    from homm1.core.paths import REPO
    return subprocess.run(
        [sys.executable, "-m", "unittest", "discover", "-s", "tests", "-v"],
        cwd=REPO, env={**os.environ, "PYTHONPATH": str(REPO / "scripts")}).returncode


def _toolchain(argv: list[str]) -> int:
    import argparse
    from homm1 import toolchain
    ap = argparse.ArgumentParser(prog="homm1 toolchain")
    ap.add_argument("action", choices=("install", "check", "symbols"))
    ap.add_argument("--id", choices=sorted(toolchain.pins()), default="vc40")
    ap.add_argument("--media", type=Path)
    ap.add_argument("--archive", type=Path,
                    help="install the pinned combined release from a local archive")
    a = ap.parse_args(argv)
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
    if not argv or argv[0] in ("-h", "--help"):
        print(__doc__.strip())
        print("\ncommands: init inspect toolchain configure build link match labels "
              "model delink compare audit sema ghidra verify tool test")
        return 0 if argv else 2
    cmd, rest = argv[0], argv[1:]
    if cmd == "init":
        return _init(rest)
    if cmd == "inspect":
        return _inspect(rest)
    if cmd == "test":
        return _test(rest)
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
    if cmd == "audit":
        audits = {"dna-bands": "dna_bands", "tooling": "tooling"}
        if not rest or rest[0] not in audits:
            print("homm1 audit: expected " + ", ".join(audits), file=sys.stderr)
            return 2
        return importlib.import_module(f"homm1.audit.{audits[rest[0]]}").main(rest[1:])
    if cmd in ("sema", "ghidra", "verify"):
        return importlib.import_module(f"homm1.{cmd}").main(rest)
    if cmd in ("build", "link", "match"):
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

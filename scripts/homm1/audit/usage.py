"""Check that every tooling entry point keeps usage logging.

Every module-level ``main``/``cli_main`` under scripts/homm1 must carry the
``homm1.core.usage.logged`` decorator, so direct module runs and batch
commands are recorded like CLI commands.
"""
from __future__ import annotations

import argparse
import ast

from homm1.core.paths import REPO
from homm1.core.usage import logged


def uninstrumented() -> list[str]:
    missing = []
    for path in sorted((REPO / "scripts/homm1").rglob("*.py")):
        tree = ast.parse(path.read_text())
        for node in tree.body:
            if isinstance(node, (ast.FunctionDef, ast.AsyncFunctionDef)) \
                    and node.name in ("main", "cli_main") \
                    and not any(isinstance(d, ast.Name) and d.id == "logged"
                                for d in node.decorator_list):
                missing.append(str(path.relative_to(REPO)))
    return missing


@logged
def main(argv=None) -> int:
    argparse.ArgumentParser(prog="homm1 audit usage", description=__doc__).parse_args(argv)
    missing = uninstrumented()
    for path in missing:
        print(f"[usage] {path}: main() lacks @logged")
    print(f"[usage] {'ok' if not missing else f'{len(missing)} uninstrumented entry point(s)'}")
    return 1 if missing else 0

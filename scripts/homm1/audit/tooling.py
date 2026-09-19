"""Inventory the pinned Gruntz tooling port, including every omitted module.

This is a structural audit, not a claim of behavioral equivalence. HoMM2's
separate capability review is recorded in docs/tooling-inheritance.md.
"""
from __future__ import annotations

import argparse
import ast
from collections import Counter
import json
from pathlib import Path
import subprocess

from homm1.core.paths import REPO
from homm1.core.usage import logged

GRUNTZ_REV = "b1de0e555576a215898907b8ec8ed5423368883e"


def _git(repo, *args):
    return subprocess.run(["git", "-C", str(repo), *args], check=True,
                          capture_output=True, text=True).stdout


def _normalized(text):
    text = text.replace("GRUNTZ", "HOMM1").replace("Gruntz", "Homm1")
    text = text.replace("gruntz", "homm1").replace("GZ_", "H1_")
    tree = ast.parse(text)
    # Instrumentation is expected everywhere and is checked by test_usage.
    tree.body = [n for n in tree.body if not
                 (isinstance(n, ast.ImportFrom) and n.module == "homm1.core.usage")]
    for node in ast.walk(tree):
        if isinstance(node, (ast.FunctionDef, ast.AsyncFunctionDef)):
            node.decorator_list = [d for d in node.decorator_list if not
                                   (isinstance(d, ast.Name) and d.id == "logged")]
        # Prose changes alone are not an implementation adaptation.
        if isinstance(node, (ast.Module, ast.ClassDef, ast.FunctionDef,
                             ast.AsyncFunctionDef)) and node.body:
            first = node.body[0]
            if isinstance(first, ast.Expr) and isinstance(first.value, ast.Constant) \
                    and isinstance(first.value.value, str):
                node.body.pop(0)
    return ast.dump(tree, include_attributes=False)


def inventory(donor: Path):
    prefix = "scripts/gruntz/"
    paths = _git(donor, "ls-tree", "-r", "--name-only", GRUNTZ_REV, prefix).splitlines()
    if not paths:
        raise ValueError("pinned Gruntz tree has no tooling modules")
    rows = []
    for path in paths:
        if not path.endswith(".py"):
            continue
        relative = path.removeprefix(prefix)
        destination = REPO / "scripts/homm1" / relative
        entry = {"module": relative}
        if not destination.is_file():
            entry["status"] = "missing"
        else:
            original = _git(donor, "show", f"{GRUNTZ_REV}:{path}")
            adapted = destination.read_text()
            entry["status"] = ("equivalent_ast_after_rename" if
                               _normalized(original) == _normalized(adapted)
                               else "adapted_review_required")
        rows.append(entry)
    return {"donor": "gruntz", "revision": GRUNTZ_REV,
            "scope": "all Python modules under scripts/gruntz; AST comparison is structural only",
            "counts": dict(sorted(Counter(r["status"] for r in rows).items())),
            "modules": rows}


@logged
def main(argv=None) -> int:
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--gruntz", type=Path, default=REPO.parents[1] / "gruntz",
                    help="local Gruntz checkout containing the pinned donor commit")
    ap.add_argument("--json", action="store_true")
    args = ap.parse_args(argv)
    report = inventory(args.gruntz)
    if args.json:
        print(json.dumps(report, indent=2))
    else:
        print(f"Gruntz {GRUNTZ_REV}: {len(report['modules'])} Python modules")
        for status, count in report["counts"].items():
            print(f"  {status}: {count}")
        for row in report["modules"]:
            if row["status"] == "missing":
                print(f"  MISSING {row['module']}")
        print("Presence/AST equality does not prove working command or target parity.")
        print("Capability findings and dispositions: docs/tooling-inheritance.md")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

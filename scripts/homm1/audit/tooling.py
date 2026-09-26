"""Inventory the pinned Gruntz tooling port, including every omitted module.

This is a structural audit, not a claim of behavioral equivalence. HoMM2's
separate capability review is recorded in docs/tooling-inheritance.md.
"""
from __future__ import annotations

import argparse
import ast
from collections import Counter
import json
import hashlib
from pathlib import Path
import subprocess

from homm1.core.paths import REPO
from homm1.core.usage import logged

GRUNTZ_REV = "b1de0e555576a215898907b8ec8ed5423368883e"
GITEN_REV = "39384dc6726478357b5efd42c66522781e8310fe"


def _git(repo, *args):
    return subprocess.run(["git", "-C", str(repo), *args], check=True,
                          capture_output=True, text=True).stdout


def _normalized(text):
    text = text.replace("GITEN", "HOMM1").replace("Giten", "Homm1").replace("giten", "homm1")
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


def inventory(donor: Path, name="gruntz"):
    revision = {"gruntz": GRUNTZ_REV, "giten": GITEN_REV}[name]
    prefix = f"scripts/{name}/"
    paths = _git(donor, "ls-tree", "-r", "--name-only", revision, prefix).splitlines()
    if not paths:
        raise ValueError(f"pinned {name} tree has no tooling modules")
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
            original = _git(donor, "show", f"{revision}:{path}")
            adapted = destination.read_text()
            entry["status"] = ("equivalent_ast_after_rename" if
                               _normalized(original) == _normalized(adapted)
                               else "adapted_review_required")
        rows.append(entry)
    return {"donor": name, "revision": revision,
            "scope": f"all Python modules under scripts/{name}; AST comparison is structural only",
            "counts": dict(sorted(Counter(r["status"] for r in rows).items())),
            "modules": rows}


def whole_tree(donor: Path):
    """Account for every pinned donor path, including game facts and workflow."""
    paths = _git(donor, "ls-tree", "-r", "--name-only", GITEN_REV).splitlines()
    mapping = {
        "include/rva.h": "include/match.h",
        "scripts/giten/core/test_usage.py": "tests/test_usage.py",
        "scripts/giten/delink/test_pdb_synth.py": "tests/test_pdb_synth.py",
        "docs/toolchain-vc50-sp3.md": "docs/compiler.md",
        "scripts/create-toolchain-release.py": "scripts/toolchain/create-toolchain-release.py",
        "scripts/create-toolchain-release.nix": "scripts/toolchain/create-toolchain-release.nix",
    }
    exceptions = {
        "scripts/giten/delink/reloc_image.py": ("inapplicable", "HoMM1 retains retail relocations; Giten synthesizes .reloc for /FIXED DDS.EXE"),
        "scripts/giten/tool/cdfs.py": ("inapplicable", "Giten disc extraction; HoMM1 uses hash-pinned local PE inputs"),
        "scripts/giten/verify/placement.py": ("deferred", "Data-claim extent and declaration placement audit needs HoMM1 fixtures; data campaign follows code"),
        "scripts/giten/walls/test_inline_measure.py": ("deferred", "VC5 budget model not calibrated for VC4"),
        "scripts/giten/retail_labels/test_censuses.py": ("deferred", "Executable-section data census controls accompany the deferred text-data delinker patch"),
        "nix/patches/vostok-iat-in-rdata.patch": ("inapplicable", "HoMM1 imports are modeled from its own .idata; donor IAT-in-rdata layout differs"),
        "nix/patches/vostok-text-data-symbols.patch": ("deferred", "Require HoMM1 code-section data evidence before changing the pinned delinker"),
        "docs/todos/rule-exceptions.tsv": ("inapplicable", "Donor rule exceptions are not HoMM1 authorizations"),
        "docs/todos/syntactic-recovery.tsv": ("adapted", "HoMM1 derives build/match/syntactic-recovery.tsv; no foreign task rows imported"),
    }
    rows = []
    for path in paths:
        original = subprocess.check_output(["git", "-C", str(donor), "show", f"{GITEN_REV}:{path}"])
        dest = mapping.get(path, path.replace("scripts/giten/", "scripts/homm1/")
                           .replace("editor/nvim/lua/giten/", "editor/nvim/lua/homm1/")
                           .replace("editor/nvim/plugin/giten.lua", "editor/nvim/plugin/homm1.lua"))
        target = REPO / dest
        row = {"path": path, "donor_sha256": hashlib.sha256(original).hexdigest()}
        if path in exceptions:
            row["status"], row["reason"] = exceptions[path]
        elif (path.startswith(("src/", "include/", "config/retail/", "config/cleanliness/")) and path not in mapping) or path == "config/match_baseline.tsv":
            row.update(status="target_specific", reason="Preserve HoMM1 source, retail facts and campaign ledger; never copy donor game facts")
        elif target.is_file() or target.is_symlink():
            payload = target.readlink().as_posix().encode() if target.is_symlink() else target.read_bytes()
            row.update(destination=dest, local_sha256=hashlib.sha256(payload).hexdigest(),
                       status="retained" if payload == original else "adapted_review_required")
        else:
            row.update(destination=dest, status="missing", reason="No local implementation or recorded disposition")
        rows.append(row)
    return {"donor": "giten", "revision": GITEN_REV,
            "scope": "every tracked donor path; content presence is not behavioral parity",
            "counts": dict(sorted(Counter(r["status"] for r in rows).items())), "files": rows}


@logged
def main(argv=None) -> int:
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--gruntz", type=Path, default=REPO.parents[1] / "gruntz",
                    help="local Gruntz checkout containing the pinned donor commit")
    ap.add_argument("--json", action="store_true")
    ap.add_argument("--giten", type=Path, help="audit the pinned Giten donor instead")
    ap.add_argument("--whole-tree", action="store_true",
                    help="inventory every Giten path, including skills, docs, hooks and editor")
    args = ap.parse_args(argv)
    if args.whole_tree and not args.giten:
        ap.error("--whole-tree requires --giten PATH")
    report = (whole_tree(args.giten) if args.whole_tree else
              inventory(args.giten, "giten") if args.giten else inventory(args.gruntz))
    if args.json:
        print(json.dumps(report, indent=2))
    else:
        items = report.get("files", report.get("modules", []))
        print(f"{report['donor']} {report['revision']}: {len(items)} inventoried paths")
        for status, count in report["counts"].items():
            print(f"  {status}: {count}")
        for row in items:
            if row["status"] == "missing":
                print(f"  MISSING {row.get('path', row.get('module'))}")
        print("Presence/AST equality does not prove working command or target parity.")
        print("Capability findings and dispositions: docs/tooling-inheritance.md")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

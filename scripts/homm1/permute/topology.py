"""Small CFG-topology clue metric for permutation candidate ranking.

Exactness remains the byte/extent/ordered-relocation gate.  This module only
ranks sub-100 clues by instruction count and branch/return skeleton so a fuzzy
winner with worse structure does not hide a more useful candidate.
"""

from __future__ import annotations

import re
from pathlib import Path

from homm1.delink.coffx import Obj
from homm1.walls.pairscan import functions, insns


_TARGET = re.compile(r"(?:0x)?([0-9a-f]+)$", re.I)


def function_topology(path: Path, symbol: str) -> dict:
    obj = Obj(path)
    extent = functions(obj).get(symbol)
    if extent is None:
        raise ValueError(f"{path}: function not found: {symbol}")
    secnum, lo, hi = extent
    decoded = insns(obj, secnum, lo, hi)
    flow = []
    returns = 0
    for offset, mnemonic, operands in decoded:
        lower = mnemonic.lower()
        if lower.startswith("ret"):
            returns += 1
            flow.append(("ret", None))
        elif lower.startswith("j") or lower.startswith("loop"):
            match = _TARGET.search(operands.strip())
            target = int(match.group(1), 16) if match else None
            flow.append((lower, target))
    return {
        "instructions": len(decoded),
        "branches": sum(1 for mnemonic, _target in flow if not mnemonic.startswith("ret")),
        "returns": returns,
        "flow": [[mnemonic, target] for mnemonic, target in flow],
    }


def compare_topology(candidate: dict, retail: dict) -> dict:
    candidate_flow = candidate["flow"]
    retail_flow = retail["flow"]
    prefix = 0
    for left, right in zip(candidate_flow, retail_flow):
        if left != right:
            break
        prefix += 1
    return {
        "exact": candidate == retail,
        "instruction_delta": abs(candidate["instructions"] - retail["instructions"]),
        "branch_delta": abs(candidate["branches"] - retail["branches"]),
        "return_delta": abs(candidate["returns"] - retail["returns"]),
        "leading_exact_flow": prefix,
        "candidate": candidate,
        "retail": retail,
    }


def topology_rank(metrics: dict, score: float) -> tuple:
    return (
        -int(metrics["exact"]),
        metrics["branch_delta"],
        metrics["return_delta"],
        metrics["instruction_delta"],
        -metrics["leading_exact_flow"],
        -score,
    )

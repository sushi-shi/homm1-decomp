"""homm1.verify.universe - the reconstruction-target denominator, from the
Model.

The match-% denominator is every in-.text reconstruction target; the carve-out
categories (EH funclets, compiler helpers, CRT/MFC library, jump thunks,
linker pad) are NOT independent targets and leave the denominator, surfaced as
their own README rows (counted, not hidden). Ported precedence per census row:
a source claim outranks everything, then kind=thunk, kind=helper, the dyninit
`$E` pins, the static-lib labels, kind=eh; the remainder is an unclaimed
target.
"""

from __future__ import annotations

import bisect
import csv

from homm1.core.paths import REPO

#: winning channels that make a row a CLAIMED reconstruction target
_TARGET_CHANNELS = ("src", "src_compgen", "functions_zlib")

CATEGORIES = (
    ("eh", "EH unwind funclets",
     "compiler /GX EH; match with their parent function"),
    ("compiler", "private lifecycle/cleanup helpers",
     "volatile `$E<n>` dyninit families and kind=helper forwarders"),
    ("library", "CRT/MFC library",
     "VC4 archive DNA: exact/prefix/order identities and proven link band"),
    ("thunk", "jump thunks",
     "linker ILT jmp-table + thunk-kind census rows"),
    ("pad", "linker pad",
     "kind=pad census rows (alignment fill, no body)"),
)


def category(binding) -> str:
    if binding.channel in _TARGET_CHANNELS:
        return "target"
    if binding.kind == "thunk":
        return "thunk"
    if binding.kind == "helper":
        return "compiler"
    if binding.channel == "src_dyninit":
        return "compiler"
    if binding.channel == "functions_static_libs":
        return "library"
    if binding.kind == "eh":
        return "eh"
    if binding.kind == "pad":
        return "pad"
    return "target"          # unclaimed reconstruction target


def engine_universe(model=None) -> dict:
    """{'real_fn','real_code','unmatched_fn','unmatched_code','categories'}.

    real_* is the match-% denominator (claimed + unclaimed targets); each
    category row is (label, fn, code, note). Sizes are the Model's extents
    (claimed size where a channel states one, else the census-derived span).
    """
    if model is None:
        from homm1.model import resolve
        model = resolve()
    counts: dict[str, int] = {}
    code: dict[str, int] = {}
    unmatched_fn = unmatched_code = 0
    for b in model.functions:
        cat = category(b)
        counts[cat] = counts.get(cat, 0) + 1
        code[cat] = code.get(cat, 0) + b.size
        if cat == "target" and not b.channel:
            unmatched_fn += 1
            unmatched_code += b.size
    # EH children and linker fill are subranges of the original Ghidra parent
    # rows.  They deliberately stay out of functions.tsv because pdb_synth
    # already carves EH and extra starts perturb Vostok's TU partition.  The
    # committed DNA report is the byte-exact reporting partition.
    evidence = REPO / "evidence/homm1-dna-bands.tsv"
    evidence_counts: dict[str, int] = {}
    evidence_code: dict[str, int] = {}
    structural = []
    class_category = {
        "eh-funclet": "eh", "compiler-helper": "compiler",
        "helper-order": "compiler", "import-thunk": "thunk",
        "linker-pad": "pad", "crt-exact": "library",
        "crt-prefix": "library", "crt-order": "library",
        "crt-band": "library",
    }
    if evidence.is_file():
        with evidence.open(newline="") as stream:
            rows = csv.DictReader(
                (line for line in stream if not line.startswith("#")),
                delimiter="\t",
            )
            for row in rows:
                key = class_category.get(row["class"])
                if key is None:
                    continue
                size = int(row["size"], 16)
                evidence_counts[key] = evidence_counts.get(key, 0) + 1
                evidence_code[key] = evidence_code.get(key, 0) + size
                structural.append((int(row["rva"], 16), size, key))

        starts = [b.rva for b in model.functions]
        # Remove embedded EH/pad bytes from a target parent.  Library/thunk
        # parents are already excluded, so their padding must not be removed
        # twice.  Do the same for unmatched code only when its parent has no
        # winning claim.
        for rva, size, key in structural:
            index = bisect.bisect_right(starts, rva) - 1
            if index < 0:
                continue
            owner = model.functions[index]
            if category(owner) == "target":
                code["target"] = code.get("target", 0) - size
                if not owner.channel:
                    unmatched_code -= size
                if key not in ("eh", "pad"):
                    counts["target"] = counts.get("target", 0) - 1
                    if not owner.channel:
                        unmatched_fn -= 1
    return {
        "real_fn": counts.get("target", 0),
        "real_code": code.get("target", 0),
        "unmatched_fn": unmatched_fn,
        "unmatched_code": unmatched_code,
        "categories": [(label, evidence_counts.get(key, counts.get(key, 0)),
                         evidence_code.get(key, code.get(key, 0)), note)
                       for key, label, note in CATEGORIES],
    }

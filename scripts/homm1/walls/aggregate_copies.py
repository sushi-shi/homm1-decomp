"""homm1.walls.aggregate_copies - the rep-movs aggregate-copy sieve.

A `rep movs[bwd]` is often cl's whole-aggregate copy. A count mismatch on a
sub-100% function is a SOURCE/CFG LEAD, not by itself a modelling statement:
cl can tail-merge two identical copy arms on one side while leaving both on
the other. Proven-at-100 current dips are excluded from this structural sieve.

After `homm1 walls diagnose` rules out duplicated/merged blocks, direction is
useful: target > base suggests an aggregate we transcribed as fields; base >
target suggests an invented copy. Until then, inspect both copy neighborhoods.

Match the MNEMONIC exactly (movsx/movzx sign-extends and inline switch-table
bytes decoding as bare `movs` are the two proven traps): `rep movs*` only.

    homm1 walls aggregate-copies [--unit U] [--max N]
"""

from __future__ import annotations

from homm1.core.usage import logged

import re
import sys

from homm1.delink.coffx import Obj
from homm1.walls import pairscan

# binutils Intel spelling: `rep movs DWORD PTR ...` / `rep movsd`
REP_MOVS = re.compile(r"^rep\s+movs")


def scan(unit_filter=None):
    pairscan.require_pairs({unit_filter} if unit_filter else None)
    from homm1.walls import inventory
    below: dict[str, list[tuple[str, float]]] = {}
    for row in inventory.build(unit_filter, 100.0):
        if not row["proven"]:
            below.setdefault(row["unit"], []).append((row["symbol"], row["cur"]))
    hits = []
    for unit, fns in sorted(below.items()):
        if unit_filter and unit != unit_filter:
            continue
        pair = pairscan.pairs({unit}).get(unit)
        if not pair:
            continue
        try:
            bobj, tobj = Obj(pair[0]), Obj(pair[1])
        except (ValueError, OSError):
            continue
        bf, tf = pairscan.functions(bobj), pairscan.functions(tobj)
        for sym, pct in fns:
            if sym not in bf or sym not in tf:
                continue
            n_b = sum(1 for _o, mn, _p in pairscan.insns(bobj, *bf[sym])
                      if REP_MOVS.match(mn))
            n_t = sum(1 for _o, mn, _p in pairscan.insns(tobj, *tf[sym])
                      if REP_MOVS.match(mn))
            if n_b != n_t:
                hits.append((n_t - n_b, unit, pct, n_b, n_t, sym))
    hits.sort(reverse=True)
    return hits


@logged
def main(argv=None) -> int:
    import argparse
    ap = argparse.ArgumentParser(prog="homm1 walls aggregate-copies",
                                 description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--unit", help="restrict to one unit of config/units.toml")
    ap.add_argument("--max", type=int, default=None,
                    help="ratchet: exit 1 when the mismatch count exceeds N")
    a = ap.parse_args(argv)
    from homm1.tool import ToolError
    from homm1.walls import check_unit
    check_unit(a.unit)
    try:
        hits = scan(a.unit)
    except ToolError as e:
        print(f"[walls aggregate-copies] {e}", file=sys.stderr)
        return 2
    for d, unit, pct, nb, nt, name in hits:
        role = "retail has more surviving copy blocks" if d > 0 else \
               "base has more surviving copy blocks"
        print(f"{d:+d}  {unit:<18} {pct:6.2f}  base {nb} / target {nt}  "
              f"{role:<40}  {name[:64]}")
    print(f"aggregate-copy source/CFG leads: {len(hits)}")
    if a.max is not None and len(hits) > a.max:
        print(f"aggregate-copies: {len(hits)} exceeds the {a.max} ratchet")
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

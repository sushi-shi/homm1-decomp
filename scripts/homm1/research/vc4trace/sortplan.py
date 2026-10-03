"""Group a function's local declaration orders by the operand order VC4's sortnode gives them.

    python3 -m homm1.research.vc4trace.sortplan TRACE ILPREFIX FUNC [--od] [--limit N]

Replays every traced statement of FUNC (sortsim) for each permutation of its
leading locals (handles are consecutive in declaration order) and prints the
outcome classes with one representative order each.  --od keeps only orders
whose /Od stack slots (homm1.core.od_slots) equal the current ones.  Compile
one representative per class to find the retail one.
"""
from __future__ import annotations

import argparse
import collections
import itertools

from homm1.core.od_slots import slot_order
from homm1.core.usage import logged
from homm1.research.vc4trace import il
from homm1.research.vc4trace.sortsim import parse, predicted


def function_statements(stmts, fh):
    return [s for s in stmts if s[0] == fh]


def leaf_handles(stmts):
    return {n.h for s in stmts for n in s[2].values() if n.h is not None}


def local_block(ilprefix, stmts):
    """(names in declaration order, first handle) of the function's locals."""
    recs = il.locals_of(ilprefix, leaf_handles(stmts))
    locs = sorted((h, n) for h, n, k in recs if k == 'local')
    if not locs:
        return [], None
    base = locs[0][0]
    run = []
    for h, n in locs:                         # the leading declaration run; later block-scoped
        if h != base + len(run):              # locals (after statement labels) stay put
            break
        run.append(n)
    return run, base


def outcome(stmts, hmap):
    return tuple(predicted(s, hmap)[0] for s in stmts)


def order_map(names, base, perm, inner=lambda h: h):
    m = {base + names.index(n): base + i for i, n in enumerate(perm)}
    return lambda h: inner(m.get(h, h))


def orders(names, od=True, limit=None):
    cur = slot_order(names)
    count = 0
    for perm in itertools.permutations(names):
        if od and slot_order(list(perm)) != cur:
            continue
        yield perm
        count += 1
        if limit and count >= limit:
            return


def classes(stmts, names, base, od=True, inner=lambda h: h, limit=None):
    """OrderedDict outcome -> [orders], current order first."""
    out = collections.OrderedDict()
    out.setdefault(outcome(stmts, inner), []).append(tuple(names))
    for perm in orders(names, od, limit):
        out.setdefault(outcome(stmts, order_map(names, base, perm, inner)), []).append(perm)
    return out


@logged
def main(argv: list[str] | None = None) -> int:
    ap = argparse.ArgumentParser(prog='python3 -m homm1.research.vc4trace.sortplan', description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument('trace'); ap.add_argument('ilprefix'); ap.add_argument('func')
    ap.add_argument('--od', action='store_true'); ap.add_argument('--limit', type=int)
    a = ap.parse_args(argv)
    fh, dec = il.function_handle(a.ilprefix, a.func)
    st = function_statements(parse(a.trace), fh)
    names, base = local_block(a.ilprefix, st)
    print(f'{dec}: handle {fh:#x}, {len(st)} sorted statements, locals {names}')
    if not names:
        return 0
    cl = classes(st, names, base, a.od, limit=a.limit)
    ref = next(iter(cl))
    print(f'{sum(len(v) for v in cl.values()) - 1} orders -> {len(cl)} outcome classes')
    for i, (o, labels) in enumerate(cl.items()):
        diff = [j for j, (x, y) in enumerate(zip(o, ref)) if x != y]
        print(f'class {i}: {len(labels)} orders, statements differing from current: {diff}'
              + ('  (CURRENT)' if o == ref else ''))
        print('   e.g.', ' '.join(labels[-1]))
    return 0


if __name__ == '__main__':
    raise SystemExit(main())

"""Outcome class of a function's sorted trees per handle shift.

    python3 -m homm1.research.vc4trace.fnshift TRACE ILPREFIX FUNC LO [--max N]

Handles >= LO (hex) move by N = 0..max-1 (a declaration run of N handles
inserted where LO is allocated); equal class numbers mean equal operand order.
"""
from __future__ import annotations

import argparse

from homm1.core.usage import logged
from homm1.research.vc4trace import il
from homm1.research.vc4trace.sortplan import function_statements, outcome
from homm1.research.vc4trace.sortsim import parse, shift_map


def shift_classes(stmts, lo, maxn):
    seen, row = {}, []
    for n in range(maxn):
        row.append(seen.setdefault(outcome(stmts, shift_map(n, lo)), len(seen)))
    return row


@logged
def main(argv: list[str] | None = None) -> int:
    ap = argparse.ArgumentParser(prog='python3 -m homm1.research.vc4trace.fnshift', description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument('trace'); ap.add_argument('ilprefix'); ap.add_argument('func'); ap.add_argument('lo')
    ap.add_argument('--max', type=int, default=32)
    a = ap.parse_args(argv)
    fh, dec = il.function_handle(a.ilprefix, a.func)
    st = function_statements(parse(a.trace), fh)
    print(dec, 'statements', len(st), 'class per N:', ' '.join(map(str, shift_classes(st, int(a.lo, 16), a.max))))
    return 0


if __name__ == '__main__':
    raise SystemExit(main())

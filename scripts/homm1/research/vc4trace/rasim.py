"""rasim.py - /O2 register allocation: trace reader and simplify/select replay.

    python3 -m homm1.research.vc4trace.rasim check TRACE
    python3 -m homm1.research.vc4trace.rasim show TRACE ILPREFIX FUNC

TRACE comes from `trace --spec ra-graph.spec` (F/A/W/P/E/C lines; see ra-graph.spec).
`check` replays every traced allocator pass (simplify then select) from the
traced worklist, degrees and edges and compares removal order and colours with
the trace. `show` prints one function's last pass.
"""
from __future__ import annotations

import argparse

from homm1.core.usage import logged
from homm1.research.vc4trace import il

# register numbers as C2 prints them (x86 encoding) and its select order (table 0x47f390)
REG = {0: 'eax', 1: 'ecx', 2: 'edx', 3: 'ebx', 4: 'esp', 5: 'ebp', 6: 'esi', 7: 'edi'}
SELECT_ORDER = (0, 1, 2, 6, 7, 3, 5)
BUCKET_BASE = 0x48a540


class Pass:
    def __init__(self):
        self.ids = []          # (id, bucket, type) in assignment order
        self.work = []         # (id, degree, weight) in prepend order
        self.removed = []      # (id, degree, weight)
        self.edges = {}        # id -> set(neighbour ids)
        self.colour = {}       # id -> reg number

    @property
    def order(self):
        """Simplify list order, head first."""
        return [w[0] for w in reversed(self.work)]


def parse(path):
    """[(function C1 handle, [Pass, ...])] from a ra-graph.spec trace."""
    fns, pend_ids, cur, ps = [], [], None, None
    for line in open(path, errors='replace'):
        p = line.split()
        if not p or len(p[0]) > 1:
            continue
        t = p[0]
        try:
            v = [int(x, 16) for x in p[1:]]
        except ValueError:
            continue
        if t == 'A':
            pend_ids.append((v[0] & 0xffff, (v[1] - BUCKET_BASE) // 4, v[2]))
        elif t == 'F':
            cur = (v[0], [])
            fns.append(cur)
            ps = None
        elif cur is None:
            continue
        elif t == 'W':
            if ps is None or ps.removed or ps.colour:
                ps = Pass()
                ps.ids = pend_ids or (cur[1][-1].ids if cur[1] else [])
                pend_ids = []
                cur[1].append(ps)
            ps.work.append(tuple(v))
        elif t == 'P':
            ps.removed.append(tuple(v))
        elif t == 'E':
            ps.edges.setdefault(v[0], set()).add(v[1])
            ps.edges.setdefault(v[1], set()).add(v[0])
        elif t == 'C' and ps is not None:
            ps.colour[v[0]] = v[1]
    return fns


def simplify(ps, k, order=None, metric_kind=3):
    """Removal order: highest degree below K (first in list on ties); else lowest spill
    metric (m3 = weight*100/degree or m4 = weight*100, computed at pass start; 3000000 for
    weight 30000 or degree 0; first in list on ties)."""
    order = list(order or ps.order)
    deg = {i: d for i, d, _w in ps.work}
    weight = {i: w for i, _d, w in ps.work}
    met = {}
    for i, d, w in ps.work:
        met[i] = 3000000 if w >= 30000 or d <= 0 else (w * 100 // d if metric_kind == 3 else w * 100)
    out = []
    while order:
        best = None
        for n in order:
            if deg[n] < k and (best is None or deg[n] > deg[best]):
                best = n
        if best is None:
            best = min(order, key=lambda n: met[n])
        order.remove(best)
        out.append(best)
        for m in ps.edges.get(best, ()):
            if m in deg:
                deg[m] -= 1
    return out


def select(ps, removed):
    """Colours popped in reverse removal order; physical neighbours (id < 8) block their register."""
    colour = {}
    for n in reversed(removed):
        used = set()
        for m in ps.edges.get(n, ()):
            if m < 8:
                used.add(m)
            elif m in colour:
                used.add(colour[m])
        for r in SELECT_ORDER:
            if r not in used:
                colour[n] = r
                break
    return colour


def check(fns, verbose=True):
    bad = total = 0
    for h, passes in fns:
        for k, ps in enumerate(passes):
            if not ps.work:
                continue
            total += 1
            got = [r[0] for r in ps.removed]
            ok = next((f'{kk}/m{mk}' for kk in (6, 7) for mk in (3, 4) if simplify(ps, kk, metric_kind=mk) == got), None)
            col = select(ps, got)
            miss = [n for n in ps.colour if col.get(n) != ps.colour[n]]
            if ok is None or miss:
                bad += 1
                if verbose:
                    print(f'fn {h:#x} pass {k}: simplify {"K=" + str(ok) if ok else "MISMATCH"}; colour '
                          f'mismatches {[(hex(n), REG.get(ps.colour[n]), REG.get(col.get(n))) for n in miss]}')
    print(f'{total - bad}/{total} passes replayed exactly')
    return bad


def find(fns, names, func):
    for h, passes in fns:
        if func in names.get(h, ''):
            return h, passes
    raise SystemExit(f'{func} not traced')


def show(h, ps, names):
    print(f'{names.get(h, hex(h))}: last pass')
    deg = {i: d for i, d, _w in ps.work}
    weight = {i: w for i, _d, w in ps.work}
    for i, b, t in ps.ids:
        nb = sorted(m for m in ps.edges.get(i, ()) if m >= 0x20)
        phys = sorted(REG.get(m, hex(m)) for m in ps.edges.get(i, ()) if m < 0x20)
        print(f'  {i:x} bucket {b:2x} type {t:x} deg {deg.get(i, "-")} w {weight.get(i, "-")} '
              f'reg {REG.get(ps.colour.get(i), "-")} nb {" ".join(f"{m:x}" for m in nb)} phys {" ".join(phys)}')
    print('  list   :', ' '.join(f'{i:x}' for i in ps.order))
    print('  removed:', ' '.join(f'{r[0]:x}' for r in ps.removed))


@logged
def main(argv: list[str] | None = None) -> int:
    ap = argparse.ArgumentParser(prog='python3 -m homm1.research.vc4trace.rasim', description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    sub = ap.add_subparsers(dest='cmd', required=True)
    c = sub.add_parser('check'); c.add_argument('trace')
    s = sub.add_parser('show'); s.add_argument('trace'); s.add_argument('il'); s.add_argument('func')
    a = ap.parse_args(argv)
    fns = parse(a.trace)
    if a.cmd == 'check':
        return 1 if check(fns) else 0
    names = il.globals_by_handle(a.il)
    h, passes = find(fns, names, a.func)
    show(h, [p for p in passes if p.work][-1], names)
    return 0


if __name__ == '__main__':
    raise SystemExit(main())

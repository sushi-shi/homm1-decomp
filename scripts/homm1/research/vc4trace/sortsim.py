"""Replay VC4 C2's sortnode pass from a tracer log with substituted C1 handles.

The sortnode tracer (sort.spec) logs every expression tree C2 sorts: node
records (op, children before the exchange, weight), symbol leaves with their
C1 handle, exchanges and reassociations.  This module re-walks the same trees
with the formulas read from C2.EXE - weight 0x40846e, walker 0x408d62,
exchange 0x413ded (comparison mirror table 0x481c0c), reassociation 0x413e27 -
under a handle map, and compares sorted trees.

    python3 -m homm1.research.vc4trace.sortsim check TRACE
    python3 -m homm1.research.vc4trace.sortsim diff TRACE_A TRACE_B (+N@LO | MAPFILE | auto)

check replays a trace with its own handles (must be exact).  diff predicts
B's sorted trees from A under the map (+N@LO: handles >= LO move by N;
MAPFILE: 'old new' hex pairs; auto: align A's and B's leaves) and compares
them with B's traced trees.
"""
from __future__ import annotations

import argparse
import collections
import struct
from functools import lru_cache

from homm1.core.usage import logged
from homm1.research.vc4trace.common import c2_exe, k4


@lru_cache(maxsize=1)
def tables():
    d = c2_exe().read_bytes()
    off = lambda va: va - 0x47e000 + 0x7ba00            # .data of the pinned C2.EXE
    flags = [struct.unpack_from('<H', d, off(0x484828) + 2 * i)[0] for i in range(0x100)]
    cmpswap = {i: struct.unpack_from('<I', d, off(0x481c0c) + 4 * i)[0] for i in range(0x1f, 0x25)}
    return flags, cmpswap


def _sar4(x): return x >> 4
def _sbyte(x):
    x &= 0xff
    return x - 256 if x & 0x80 else x


class Node:
    __slots__ = ('id', 'op', 'w', 'k0', 'k1', 'tw', 'h', 'kind', 'L')

    def __init__(self, nid, op, w, k0, k1, tw, L):
        self.id, self.op, self.w, self.k0, self.k1, self.tw, self.L = nid, op, w, k0, k1, tw, L
        self.h = None
        self.kind = None


def parse(path):
    """-> [(function handle, root, {id: Node}, events)] in compile order."""
    stmts, cur, leaves = [], None, {}
    for line in open(path, errors='replace'):
        p = line.split()
        if not p or len(p[0]) != 1 or p[0] not in 'SNLXR':
            continue
        try:
            v = [int(x, 16) for x in p[1:]]
        except ValueError:
            continue
        if p[0] == 'S':
            cur = (v[1], v[0], {}, [])
            stmts.append(cur)
            leaves = {}
            continue
        if cur is None:
            continue
        nodes, ev = cur[2], cur[3]
        if p[0] == 'L':
            leaves[v[0]] = (v[1], v[2])
        elif p[0] == 'N':
            nid, op, w, L, R, k0, k1, tw = v
            if nid not in nodes:
                n = Node(nid, op, w, k0, k1, tw, L)
                if nid in leaves:
                    n.h, n.kind = leaves[nid]
                nodes[nid] = n
            ev.append(('N', nid, w, L, R))
        elif p[0] == 'X':
            ev.append(('X', v[0], v[1], v[2]))
        elif p[0] == 'R':
            ev.append(('R', v[0], v[1], v[2], v[3]))
    return stmts


class Sim:
    """One sortnode walk of a traced tree under a handle map."""

    def __init__(self, nodes, hmap):
        self.flags, self.cmpswap = tables()
        self.n, self.hmap, self.G, self.ev = nodes, hmap, 0, []
        self.kids = {nid: [nd.k0, nd.k1] for nid, nd in nodes.items()}
        self.op = {nid: nd.op for nid, nd in nodes.items()}
        self.unmodeled = set()
        # 0x34 conversions: 0x4138c3 either strips them (weight passes through) or not
        self.pass34 = {nid: nd.w == nd.L for nid, nd in nodes.items() if nd.op == 0x34}

    def weight(self, nid, L, R):
        nd, op = self.n[nid], self.op[nid]
        cls = self.flags[op & 0xff] & 3
        if cls == 2:
            if op == 0x26:
                if nd.h is None or nd.kind == 0x18:
                    return 0x10
                return 0x10 | k4(self.hmap(nd.h))
            return nd.w                                   # constants: handle-free, from the trace
        if cls == 1:
            if op == 0x31:
                return L
            b = _sbyte(op)
            if op == 0x34 and self.pass34.get(nid):
                return L
            rank = 0x10 if op == 0x30 else 0x20
            return ((_sar4(b) - b + L) & 0xf) | ((L + rank) & 0xfff0)
        if cls == 0 and op == 0x44:
            return R
        add = 0x70 if cls == 3 else 0x20
        return ((_sar4(op) - op + R + L) & 0xf) | ((R + L + add) & 0xfff0)

    def walk(self, nid, parent=0):
        if nid == 0 or nid not in self.n:
            return 0
        f = self.flags[self.op[nid] & 0xff]
        edi = ebp = 0
        if f & 0x2000:
            edi = self.walk(self.kids[nid][1], nid)       # right child first
        if (f & 3) != 2:
            ebp = self.walk(self.kids[nid][0], nid)
        w = self.weight(nid, ebp, edi)
        self.ev.append(('N', nid, w, ebp, edi))
        if f & 8:
            if f & 4 and self.rotate(nid, edi, self.G):
                edi = self.G
                self.walk(self.kids[nid][0], nid)
                self.G = edi
                return w
            self.ev.append(('X', nid, ebp, edi))
            if edi > ebp:
                k = self.kids[nid]
                k[0], k[1] = k[1], k[0]
                if f & 0x10:
                    self.op[nid] = self.cmpswap[self.op[nid]]
                edi = ebp
        self.G = edi
        return w

    def rotate(self, nid, R, prevG):
        lc = self.kids[nid][0]
        if lc not in self.n or self.op[lc] != self.op[nid]:
            return False
        if self.n[nid].tw & 0xc00:
            self.unmodeled.add(nid)                       # float/pointer branch: not modelled
        if prevG >= R:
            return False
        self.ev.append(('R', nid, lc, R, prevG))
        a, b = self.kids[lc]
        c = self.kids[nid][1]
        self.kids[lc] = [a, c]                            # ((a op b) op c) -> ((a op c) op b)
        self.kids[nid][1] = b
        return True


def sig(nodes, kids, nid, hm, ops=None, depth=0):
    """Structural signature of a sorted subtree (ops, mapped handles, constant weights)."""
    flags = tables()[0]
    if nid not in nodes or depth > 60:
        return '?'
    n = nodes[nid]
    op = ops.get(nid, n.op) if ops else n.op
    f = flags[op & 0xff]
    if (f & 3) == 2:
        return f'{n.op:x}:{hm(n.h):x}' if n.h is not None else f'{n.op:x}#{n.w:x}'
    k = kids.get(nid, [n.k0, n.k1])
    s = f'{op:x}(' + sig(nodes, kids, k[0], hm, ops, depth + 1)
    if f & 0x2000:
        s += ',' + sig(nodes, kids, k[1], hm, ops, depth + 1)
    return s + ')'


def traced_tree(nodes, ev):
    """(kids, ops) after the traced exchanges and rotations."""
    flags, cmpswap = tables()
    kids = {nid: [n.k0, n.k1] for nid, n in nodes.items()}
    ops = {nid: n.op for nid, n in nodes.items()}
    for e in ev:
        if e[0] == 'X' and e[3] > e[2]:
            k = kids[e[1]]
            k[0], k[1] = k[1], k[0]
            if flags[ops[e[1]] & 0xff] & 0x10:
                ops[e[1]] = cmpswap[ops[e[1]]]
        elif e[0] == 'R':
            nid, lc = e[1], e[2]
            a, b = kids[lc]
            c = kids[nid][1]
            kids[lc] = [a, c]
            kids[nid][1] = b
    return kids, ops


def predicted(stmt, hmap, label=lambda h: h):
    """Sorted-tree signature of one traced statement under HMAP (and the Sim).
    Leaves are labelled by LABEL(original handle) - identity keeps outcomes
    comparable across maps."""
    fn, root, nodes, _ev = stmt
    s = Sim(nodes, hmap)
    s.walk(root)
    return sig(nodes, s.kids, root, label, s.op), s


def traced(stmt, hmap=lambda h: h):
    fn, root, nodes, ev = stmt
    kids, ops = traced_tree(nodes, ev)
    return sig(nodes, kids, root, hmap, ops)


def shift_map(n, lo=0):
    return lambda h: h + n if h >= lo else h


def mkmap(spec):
    if spec[0] in '+-':
        k, _, lo = spec.partition('@')
        return shift_map(int(k, 0), int(lo, 16) if lo else 0)
    m = {}
    for line in open(spec):
        a, b = line.split()[:2]
        m[int(a, 16)] = int(b, 16)
    return lambda h: m.get(h, h)


def leafmap(A, B):
    """Handle map aligning A's and B's leaves statement by statement (validation only)."""
    m, bad = {}, 0
    for (_fa, _ra, na, ea), (_fb, _rb, nb, eb) in zip(A, B):
        la = [na[e[1]].h for e in ea if e[0] == 'N' and na[e[1]].h is not None]
        lb = [nb[e[1]].h for e in eb if e[0] == 'N' and nb[e[1]].h is not None]
        if len(la) != len(lb):
            bad += 1
            continue
        for x, y in zip(la, lb):
            if m.setdefault(x, y) != y:
                bad += 1
    return m, bad


def check(path):
    st = parse(path)
    ok = 0
    bad = collections.Counter()
    for stmt in st:
        s = Sim(stmt[2], lambda h: h)
        s.walk(stmt[1])
        if s.ev == stmt[3]:
            ok += 1
        else:
            bad[stmt[0]] += 1
    print(f'statements {len(st)}: exact {ok}, differ {sum(bad.values())}',
          {hex(k): v for k, v in bad.most_common(8)})
    return 0 if not bad else 1


def diff(pa, pb, spec):
    A, B = parse(pa), parse(pb)
    if spec == 'auto':
        m, bad = leafmap(A, B)
        shifts = collections.Counter(y - x for x, y in m.items())
        print('auto map:', len(m), 'handles, inconsistent', bad, 'shifts', dict(shifts.most_common(4)))
        hm = lambda h: m.get(h, h)
    else:
        hm = mkmap(spec)
    if len(A) != len(B):
        print('statement counts differ', len(A), len(B))
    same = changed = wrong = 0
    for a, b in zip(A, B):
        p = predicted(a, hm, hm)[0]
        tb = traced(b)
        if p != tb:
            wrong += 1
            if wrong <= 8:
                print('WRONG fn', hex(a[0]), '\n  predicted', p[:200], '\n  traced   ', tb[:200])
        elif traced(a, hm) == tb:
            same += 1
        else:
            changed += 1
    print(f'statements {len(A)}: unchanged&predicted {same}, changed&predicted {changed}, mispredicted {wrong}')
    return 0 if not wrong else 1


@logged
def main(argv: list[str] | None = None) -> int:
    ap = argparse.ArgumentParser(prog='python3 -m homm1.research.vc4trace.sortsim', description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    sub = ap.add_subparsers(dest='cmd', required=True)
    c = sub.add_parser('check'); c.add_argument('trace')
    d = sub.add_parser('diff'); d.add_argument('a'); d.add_argument('b'); d.add_argument('map')
    a = ap.parse_args(argv)
    return check(a.trace) if a.cmd == 'check' else diff(a.a, a.b, a.map)


if __name__ == '__main__':
    raise SystemExit(main())

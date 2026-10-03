"""rasolve.py - which region handle offsets give a function retail's /O2 colouring.

    python3 -m homm1.research.vc4trace.rasolve UNIT FUNC [--point P]... [--grid 32] [--verify N]

Read-only on src/; compiles go to build/research/vc4trace/rasolve/.

1. Trace UNIT with ra-graph.spec (base) and once more per insertion POINT with one
   typedef inserted there.  A POINT is `before:TEXT`, `after:TEXT` (a source line
   containing TEXT) or `fn:DECORATED` (before that function's VA line).  The
   default points are `before:` the .cpp's first #include, `after:` its last
   #include and `fn:FUNC`.
2. A range's *region* is the set of probes that moved its C1 bucket by one.  Ranges
   no probe moves (inline-expanded locals) keep their bucket.
3. For every combination of offsets (0..grid-1 per point) the replay recomputes the
   range buckets, renumbers the ranges (bucket order, newer symbols first inside a
   bucket), rebuilds the simplify list and replays simplify and select on the base
   graph (rasim).  Combinations are grouped by the colouring they predict.
4. --verify compiles up to N representatives per colouring class (typedef runs at
   the points) and prints the objdis distance to retail, so each class is labelled
   by a measured distance.  Distance 0 means retail's colouring is reachable with
   those region offsets; realise them with authentic declarations or include order.
"""
from __future__ import annotations

import argparse
import itertools
import re
from pathlib import Path

from homm1.core.usage import logged
from homm1.research.vc4trace import common, il, objdis, rasim, trace

SPEC = common.HERE / 'ra-graph.spec'


def point_offset(text, point, decorated=None):
    kind, _, arg = point.partition(':')
    if kind == 'before':
        return text.index(arg)
    if kind == 'after':
        i = text.index(arg)
        return text.index('\n', i) + 1
    if kind == 'fn':
        from homm1.research.vc4trace.solver import function_offsets
        return function_offsets(text, [arg])[arg]
    raise SystemExit(f'bad point {point!r}')


def realise(text, offs, counts, tag='ZzRa'):
    t = text
    for k in sorted(range(len(offs)), key=lambda k: -offs[k]):
        t = t[:offs[k]] + ''.join(f'typedef int {tag}{k}_{q};\n' for q in range(counts[k])) + t[offs[k]:]
    return t


class Work:
    def __init__(self, unit, out=None):
        self.unit = unit
        self.src, self.mode = common.unit_info(unit)
        self.out = Path(out or common.WORK / 'rasolve' / unit.replace('/', '_'))
        self.out.mkdir(parents=True, exist_ok=True)

    def _put(self, text, tag):
        d = self.out / tag
        d.mkdir(parents=True, exist_ok=True)
        p = d / self.src.name
        if not p.exists() or p.read_text() != text:
            p.write_text(text)
            for f in d.glob('*.obj'):
                f.unlink()
        return d, p

    def traced(self, text, tag):
        d, p = self._put(text, tag)
        log = d / f'{self.src.stem}.trace'
        if not log.exists() or not (d / f'{self.src.stem}.obj').exists():
            for _ in range(3):
                try:
                    trace.run(p, self.mode, spec=SPEC, out_dir=d)
                    break
                except Exception as exc:      # winepath timeouts under load
                    err = exc
            else:
                raise err
        return log, d / 'il' / self.src.stem, d / f'{self.src.stem}.obj'

    def compiled(self, text, tag):
        d, p = self._put(text, tag)
        obj = d / f'{self.src.stem}.obj'
        if not obj.exists():
            for _ in range(3):
                try:
                    common.compile_separately(p, self.mode, out_dir=d)
                    break
                except Exception as exc:
                    err = exc
            else:
                raise err
        return obj


def all_passes(trace_path, il_prefix, func):
    fns = rasim.parse(trace_path)
    names = il.globals_by_handle(il_prefix)
    h, passes = rasim.find(fns, names, func)
    return [p for p in passes if p.work]


def last_pass(trace_path, il_prefix, func):
    return all_passes(trace_path, il_prefix, func)[-1]


def rounds(passes):
    """Group allocator passes into rounds: consecutive passes over the same graph (the driver
    tries both spill metrics, then re-runs the chosen one); a round ends when spill code
    changes the graph."""
    out = []
    for p in passes:
        key = (frozenset(w[0] for w in p.work), frozenset((a, b) for a, s in p.edges.items() for b in s))
        if out and out[-1][0] == key:
            out[-1][1].append(p)
        else:
            out.append((key, [p]))
    return [ps for _k, ps in out]


def spilled(p, colour=None):
    colour = p.colour if colour is None else colour
    return frozenset(w[0] for w in p.work if w[0] not in colour)


def pass_mode(p):
    """(K, metric) that reproduces a traced pass."""
    got = [r[0] for r in p.removed]
    return next(((k, m) for k in (6, 7) for m in (3, 4) if rasim.simplify(p, k, metric_kind=m) == got), None)


def regions(base, probes):
    """{range id: set(probe index)} from the bucket moves of each probe."""
    b0 = {i: b for i, b, _t in base.ids}
    out = {i: set() for i in b0}
    for k, pr in enumerate(probes):
        # match ranges across compiles by assignment-independent identity: type and weight and degree
        sig = lambda ps: {i: (t, w_d(ps, i)) for i, _b, t in ps.ids}
        base_sig, pr_sig = sig(base), sig(pr)
        pb = {i: b for i, b, _t in pr.ids}
        used = set()
        for i in b0:
            cands = [j for j in pb if j not in used and pr_sig[j] == base_sig[i]]
            moved = [j for j in cands if pb[j] == (b0[i] + 1) % 32]
            same = [j for j in cands if pb[j] == b0[i]]
            j = (moved or same or cands or [None])[0]
            if j is None:
                continue
            used.add(j)
            if pb[j] == (b0[i] + 1) % 32:
                out[i].add(k)
    return out


def w_d(ps, i):
    for j, d, w in ps.work:
        if j == i:
            return (d, w)
    return None


def renumber(base, reg, shifts):
    """{base range id: id after shifting each region's buckets}."""
    rank0 = {i: n for n, (i, _b, _t) in enumerate(base.ids)}
    nb = {i: (b + sum(shifts[p] for p in reg[i])) % 32 for i, b, _t in base.ids}
    newer = lambda i: -len(reg[i])
    order_ids = sorted(nb, key=lambda i: (nb[i], newer(i), rank0[i]))
    return {i: 0x4020 + n for n, i in enumerate(order_ids)}


def list_order(p, newid):
    nid = lambda i: newid.get(i, i)
    temps = [i for i in p.order if i >= 0x8000]
    users = sorted((w[0] for w in p.work if w[0] < 0x8000), key=lambda i: (-(nid(i) & 31), nid(i)))
    return temps + users


def predict_rounds(passes, reg, shifts):
    """Final colouring after replaying every allocator round under shifted ids, or None when
    a round's chosen spill set differs from the traced one (the next graph is then unknown).
    Within a round C2 runs both spill metrics and keeps the smaller spilled weight."""
    base = passes[0]
    newid = renumber(base, reg, shifts)
    rs = rounds(passes)
    for n, rp in enumerate(rs):
        g = rp[-1]
        if n == len(rs) - 1:
            k, m = pass_mode(g)
            return rasim.select(g, rasim.simplify(g, k, order=list_order(g, newid), metric_kind=m))
        weight = {i: w for i, _d, w in g.work}
        best = None
        for p in rp[:-1] or rp:
            k, m = pass_mode(p)
            col = rasim.select(g, rasim.simplify(g, k, order=list_order(g, newid), metric_kind=m))
            sp = spilled(g, col)
            cost = sum(weight[i] for i in sp)
            if best is None or cost < best[0]:
                best = (cost, sp)
        if best[1] != spilled(g):
            return None
    return None


def predict(base, reg, shifts, k, mk):
    """Colouring {range id: reg} after shifting each region's buckets."""
    rank0 = {i: n for n, (i, _b, _t) in enumerate(base.ids)}
    nb = {i: (b + sum(shifts[p] for p in reg[i])) % 32 for i, b, _t in base.ids}
    newer = lambda i: -len(reg[i])         # symbols later in the TU sit under more points
    order_ids = sorted(nb, key=lambda i: (nb[i], newer(i), rank0[i]))
    newid = {i: 0x4020 + n for n, i in enumerate(order_ids)}
    work_ids = [w[0] for w in base.work]
    temps = [i for i in base.order if i >= 0x8000]
    # ranges numbered elsewhere (spill/temporary user ids) keep their ids; the list runs
    # by id & 31 from bucket 31 down
    nid = lambda i: newid.get(i, i)
    users = sorted((i for i in work_ids if i < 0x8000), key=lambda i: (-(nid(i) & 31), nid(i)))
    rem = rasim.simplify(base, k, order=temps + users, metric_kind=mk)
    return rasim.select(base, rem)


@logged
def main(argv: list[str] | None = None) -> int:
    ap = argparse.ArgumentParser(prog='python3 -m homm1.research.vc4trace.rasolve', description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument('unit'); ap.add_argument('func')
    ap.add_argument('--point', action='append')
    ap.add_argument('--grid', type=int, default=32)
    ap.add_argument('--verify', type=int, default=1, help='compiles per colouring class (0: none)')
    ap.add_argument('--validate', type=int, default=0,
                    help='trace N offset combinations and compare predicted with traced colourings')
    ap.add_argument('--out')
    a = ap.parse_args(argv)
    W = Work(a.unit, a.out)
    text = W.src.read_text()
    incs = [m for m in re.finditer(r'^#include[^\n]*$', text, re.M)]
    points = a.point or [f'before:{incs[0].group(0)}', f'after:{incs[-1].group(0)}', f'fn:{a.func}']
    offs = [point_offset(text, p) for p in points]
    retail = objdis.normalized(objdis.retail_obj(a.unit), a.func)

    tr, ilp, obj = W.traced(text, 'base')
    passes = all_passes(tr, ilp, a.func)
    base = passes[0]
    k, mk = pass_mode(passes[-1])
    probes = []
    for n, p in enumerate(points):
        cnt = [1 if q == n else 0 for q in range(len(points))]
        ptr, pil, _ = W.traced(realise(text, offs, cnt), f'probe{n}')
        probes.append(all_passes(ptr, pil, a.func)[0])
    reg = regions(base, probes)
    names = {i: f'{i:x}(b{b:x},{"".join(str(p) for p in sorted(reg[i])) or "-"})' for i, b, _t in base.ids}
    print(f'[rasolve] {a.unit} {a.func}: {len(passes)} passes in {len(rounds(passes))} rounds; points {points}')
    print('[rasolve] ranges (id(bucket,regions)):', ' '.join(names[i] for i, _b, _t in base.ids))
    print('[rasolve] base distance', objdis.distance(obj, a.unit, a.func, retail))

    if a.validate:
        import random
        rnd = random.Random(1)
        ok = unk = chg = 0
        sig = lambda ps, i: (next((t for j, _b, t in ps.ids if j == i), None), w_d(ps, i))
        for n in range(a.validate):
            s = tuple(rnd.randrange(a.grid) for _ in points)
            col = predict_rounds(passes, reg, s)
            ptr, pil, _ = W.traced(realise(text, offs, list(s)), 'val_' + '_'.join(map(str, s)))
            got = last_pass(ptr, pil, a.func)
            gp = all_passes(ptr, pil, a.func)
            same_graph = [sorted((w[1], w[2]) for w in q.work) for q in gp] == \
                [sorted((w[1], w[2]) for w in q.work) for q in passes]
            if not same_graph:
                chg += 1
                print(f'  validate {s}: the traced graphs differ (operand order / IL changed), not an RA prediction')
                continue
            if col is None:
                unk += 1
                print(f'  validate {s}: spill set changes (not predicted)')
                continue
            want = sorted((sig(passes[-1], i), c) for i, c in col.items() if any(i == j for j, _b, _t in base.ids))
            have = sorted((sig(got, i), c) for i, c in got.colour.items() if any(i == j for j, _b, _t in got.ids))
            same = want == have
            ok += same
            if not same:
                print(f'  validate {s}: predicted != traced')
        print(f'[rasolve] validation: {ok}/{a.validate - unk - chg} predicted colourings matched the trace; '
              f'{unk} states change the spill set; {chg} states change the graph itself')

    classes = {}
    for shifts in itertools.product(range(a.grid), repeat=len(points)):
        col = predict_rounds(passes, reg, shifts)
        key = ('spill set changes',) if col is None else tuple(sorted((i, c) for i, c in col.items() if c > 2))
        classes.setdefault(key, []).append(shifts)
    print(f'[rasolve] {len(classes)} predicted colouring classes over {a.grid ** len(points)} offset combinations')
    for key, shifts in sorted(classes.items(), key=lambda kv: -len(kv[1])):
        dists = []
        for s in shifts[:a.verify]:
            o = W.compiled(realise(text, offs, list(s)), 'v_' + '_'.join(map(str, s)))
            dists.append((s, objdis.distance(o, a.unit, a.func, retail)))
        desc = key[0] if key and isinstance(key[0], str) else ' '.join(f'{i:x}={rasim.REG[c]}' for i, c in key)
        print(f'  class {desc}: {len(shifts)} combos, e.g. {shifts[0]}; measured {dists}')
    return 0


if __name__ == '__main__':
    raise SystemExit(main())

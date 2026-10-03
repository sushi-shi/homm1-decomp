"""Whole-TU operand-order solver: which authentic source changes put every residue in its retail class.

    python3 -m homm1.research.vc4trace.solver UNIT [--funcs A,B,...] [--probes 64] [--dmax 48]
        [--verify] [--out DIR]

Read-only on src/.  Steps:

1. Trace UNIT once with the sortnode tracer (sortsim replay must be exact).
2. Probe: compile scratch copies with N = 0..probes-1 typedefs at the top of
   the TU and measure every function's distance to retail (objdis).  The replay
   predicts each function's outcome class at every N, so each probe labels a
   class with a distance; equal classes must give equal distances (checked).
   The best-scoring class's sorted trees become the function's target.
3. Search, scoring states by the replay alone (no compiles):
   - local declaration orders that keep the /Od stack slots (sortplan),
   - a shift D_F of each function's own handles, i.e. handles inserted (or
     removed) between functions: control-flow spellings of earlier functions
     (docs/patterns/vc4-control-flow-consumes-c1-handles.md) or declarations,
   - a whole-TU shift T (include/declaration order before the code).
   Dynamic programming over the functions in source order picks the cheapest
   insertion schedule; candidates are ranked local order > control-flow
   spelling > include/declaration order.
4. --verify realizes the best schedule in a scratch copy (typedef runs stand in
   for the authentic edit) and recompiles to confirm the predicted distances.
"""
from __future__ import annotations

import argparse
import collections
import json
import re
from pathlib import Path

from homm1.core.usage import logged
from homm1.research.vc4trace import common, il, objdis, sortplan, trace
from homm1.research.vc4trace.sortsim import Sim, leafmap, parse, predicted, shift_map

# Handle cost of code-identical spellings (vc4-control-flow-consumes-c1-handles.md):
# the delta when the left form is rewritten as the right one.
SPELLINGS = [
    (re.compile(r'\bif \(([^()]*(?:\([^()]*\))*[^()]*)&&'), 'if (A && B) S  ->  if (A) if (B) S', +1),
    (re.compile(r'\bstatic_cast<\w+>\('), 'drop a value-preserving static_cast', -1),
    (re.compile(r'\(short\)\s*\('), 'drop a value-preserving (short) cast', -1),
]


class Unit:
    def __init__(self, unit, out):
        self.unit = unit
        self.src, self.mode = common.unit_info(unit)
        self.out = Path(out or common.WORK / 'solve' / unit.replace('/', '_'))
        self.out.mkdir(parents=True, exist_ok=True)

    def compile(self, text, tag, traced=False):
        d = self.out / tag
        d.mkdir(parents=True, exist_ok=True)
        path = d / self.src.name
        obj = d / f'{self.src.stem}.obj'
        cached = path.exists() and path.read_text() == text and obj.exists()
        if cached and (not traced or (d / f'{self.src.stem}.trace').exists()):
            return obj, d / 'il' / self.src.stem, (d / f'{self.src.stem}.trace') if traced else None
        path.write_text(text)
        if traced:
            log = trace.run(path, self.mode, out_dir=d)
            return d / f'{self.src.stem}.obj', d / 'il' / self.src.stem, log
        obj, ilp, _ = common.compile_separately(path, self.mode, out_dir=d)
        return obj, ilp, None


_DECL = re.compile(r'^\s+[A-Za-z_][\w\s\*:<>,]*\b(\w+)(\[[^\]]*\])?;\s*$')


def reorder_locals(text, offset, order):
    """Rewrite the leading local declaration block of the function defined after OFFSET."""
    b = text.index('{', text.index('(', offset)) + 1
    lines = text[b:].split('\n')
    i, decl = 1, {}
    while i < len(lines) and _DECL.match(lines[i]):
        decl[_DECL.match(lines[i]).group(1)] = lines[i]
        i += 1
    if set(order) != set(decl):
        return None
    return text[:b] + '\n'.join([lines[0]] + [decl[n] for n in order] + lines[i:])


def top_insert(text, n, tag='ZzSolveTop'):
    return ''.join(f'typedef int {tag}{k};\n' for k in range(n)) + text


def insert_before(text, offset, n, tag):
    return text[:offset] + ''.join(f'typedef int {tag}{k};\n' for k in range(n)) + text[offset:]


def function_offsets(text, decorated):
    """Source offset of the VA(...) line before each function definition (best effort by name)."""
    out = {}
    for dec in decorated:
        m = re.match(r'\?(\w+)@(?:(\w+)@)?@', dec)
        if not m:
            continue
        name = (m.group(2) + '::' if m.group(2) else '') + m.group(1)
        if m.group(1) == m.group(2) or dec.startswith('??0'):
            continue
        for hit in re.finditer(r'\nVA\([^\n]*\)\n(?:[^\n]*\n)?[^\n]*\b' + re.escape(name) + r'\s*\(', text):
            out[dec] = hit.start() + 1
            break
    return out


@logged
def main(argv: list[str] | None = None) -> int:
    ap = argparse.ArgumentParser(prog='python3 -m homm1.research.vc4trace.solver', description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument('unit')
    ap.add_argument('--funcs', help='comma-separated function names to report (default: every residue)')
    ap.add_argument('--probes', type=int, default=64)
    ap.add_argument('--dmax', type=int, default=48)
    ap.add_argument('--orders', type=int, default=5040, help='local orders tried per function')
    ap.add_argument('--reps', type=int, default=16, help='local-order classes kept per function')
    ap.add_argument('--beam', type=int, default=12)
    ap.add_argument('--rounds', type=int, default=8, help='measuring compiles per tier')
    ap.add_argument('--tops', type=int, default=3, help='whole-TU shifts searched in tier 3')
    ap.add_argument('--verify', action='store_true')
    ap.add_argument('--out')
    a = ap.parse_args(argv)
    U = Unit(a.unit, a.out)
    text = U.src.read_text()

    # 1. trace once
    obj, ilp, log = U.compile(text, 'base', traced=True)
    stmts = parse(log)
    names = il.globals_by_handle(ilp)
    by_fn = collections.OrderedDict()
    for s in stmts:
        by_fn.setdefault(s[0], []).append(s)
    fns = [fh for fh in by_fn if fh in names]
    print(f'[solver] {a.unit}: {len(stmts)} sorted trees in {len(fns)} functions')

    # first handle the .cpp allocates and the first handle of every function body (its sy block)
    probe1 = U.compile(top_insert(text, 1), 'probe1t', traced=True)
    m, _bad = leafmap(stmts, parse(probe1[2]))
    lo_top = min((x for x, y in m.items() if y != x), default=0)
    after = U.compile(text[:_after_includes(text)] + 'typedef int ZzSolveAfter;\n' + text[_after_includes(text):],
                      'probe1a', traced=True)
    m2, _ = leafmap(stmts, parse(after[2]))
    lo_cpp = min((x for x, y in m2.items() if y != x), default=0)
    fn_lo, fn_syms = {}, {}
    for fh in fns:
        hs = sortplan.leaf_handles(by_fn[fh])
        recs = il.locals_of(ilp, hs)
        own = [h for h, _n, _k in recs]
        fn_lo[fh] = min(own) if own else None
        fn_syms[fh] = own
    o2 = U.mode.startswith('o2')

    def outcome(fh, hm):
        """Sorted trees, plus for /O2 the register-allocator tie order: C2 numbers a
        function's symbols by C1 handle & 31 (vc4-global-register-allocation-is-chaitin-briggs)."""
        o = sortplan.outcome(by_fn[fh], hm)
        if o2:
            o = (o, tuple(sorted(fn_syms[fh], key=lambda h: (hm(h) & 31, hm(h)))))
        return o
    print(f'[solver] handles: whole-TU shift from {lo_top:#x}, .cpp from {lo_cpp:#x}')

    # 2. probes
    retail = {fh: objdis.normalized(objdis.retail_obj(a.unit), names[fh]) for fh in fns}
    dist0 = {fh: objdis.distance(obj, a.unit, names[fh], retail[fh]) for fh in fns}
    table = {fh: {} for fh in fns}                      # outcome -> distance
    incons = collections.Counter()
    for n in range(a.probes):
        pobj = obj if n == 0 else U.compile(top_insert(text, n), f'p{n}')[0]
        hm = shift_map(n, lo_top)
        for fh in fns:
            o = outcome(fh, hm)
            dd = objdis.distance(pobj, a.unit, names[fh], retail[fh])
            if table[fh].setdefault(o, dd) != dd:
                incons[fh] += 1
                table[fh][o] = min(table[fh][o], dd)
    target = {fh: min(table[fh].items(), key=lambda kv: kv[1]) for fh in fns}
    residues = [fh for fh in fns if dist0[fh] > 0]
    want = set(a.funcs.split(',')) if a.funcs else None
    if want:
        residues = [fh for fh in residues if any(names[fh].startswith('?' + w + '@') for w in want)]
    print(f'[solver] probes {a.probes}: class/distance inconsistencies {sum(incons.values())}'
          + (f' in {[names[f] for f in incons]}' if incons else ''))
    for fh in fns:
        if dist0[fh] or fh in residues:
            print(f'   {names[fh]}: now {dist0[fh]}, best probe class {target[fh][1]}'
                  f' ({len(table[fh])} classes seen)')

    order_fns = sorted(fns, key=lambda fh: fn_lo[fh] if fn_lo[fh] is not None else 1 << 30)
    offs = function_offsets(text, [names[f] for f in fns])
    blocks = {fh: sortplan.local_block(ilp, by_fn[fh]) for fh in fns}

    def hmap(fh, T, Df, perm):
        lo_f = fn_lo[fh] if fn_lo[fh] is not None else lo_cpp
        def hm(h):
            h2 = h + T if h >= lo_top else h
            return h2 + Df if h >= lo_f else h2
        names_l, base = blocks[fh]
        return sortplan.order_map(names_l, base, perm, hm) if perm and names_l else hm

    memo = {}

    def evaluate(fh, T, Df, perm):
        """(cost, outcome, known): the probed distance of the predicted class, or an
        optimistic estimate for a class no compile has measured yet."""
        key = (fh, T, Df, perm)
        o = memo.get(key)
        if o is None:
            o = memo[key] = outcome(fh, hmap(fh, T, Df, perm))
        if o in table[fh]:
            return table[fh][o], o, True
        tgt, best = target[fh]
        trees, tt = (o[0], tgt[0]) if o2 else (o, tgt)
        miss = sum(x != y for x, y in zip(trees, tt))
        ra = (o[1] != tgt[1]) if o2 else False
        return best + 2 * miss + (6 if ra else 0), o, False

    # local-order representatives per function: one order per predicted class at the base state
    reps = {}
    for fh in fns:
        names_l, base = blocks[fh]
        seen = {outcome(fh, lambda h: h): None}
        if fh in residues and names_l:
            for perm in sortplan.orders(names_l, od=U.mode.startswith('od'), limit=a.orders):
                seen.setdefault(outcome(fh, sortplan.order_map(names_l, base, perm)), perm)
        reps[fh] = list(seen.values())[:a.reps]

    def search(Ts, allow_shift, allow_local):
        best = None
        for T in Ts:
            prev = {0: (0, [])}
            for fh in order_fns:
                perms = reps[fh] if allow_local else [None]
                cur = {}
                for D, (c0, path) in prev.items():
                    for D2 in (range(D, a.dmax) if allow_shift else [D]):
                        for perm in perms:
                            c = c0 + 10 * evaluate(fh, T, D2, perm)[0] + (D2 != D) + (perm is not None)
                            if c < cur.get(D2, (1 << 30,))[0]:
                                cur[D2] = (c, path + [(fh, D2, perm)])
                prev = dict(sorted(cur.items(), key=lambda kv: kv[1][0])[:a.beam])
            c, path = min(prev.values(), key=lambda v: v[0])
            if best is None or c < best[0]:
                best = (c, T, path)
        return best

    def realize(T, path):
        vt, D, edits = text, 0, []
        for fh, D2, perm in path:
            if names[fh] not in offs:
                continue
            if D2 != D:
                edits.append((offs[names[fh]], 1, D2 - D))
            if perm:
                edits.append((offs[names[fh]], 0, perm))
            D = D2
        for off, kind, val in sorted(edits, key=lambda e: (e[0], e[1]), reverse=True):
            vt = reorder_locals(vt, off, val) or vt if kind == 0 else insert_before(vt, off, val, 'ZzSolveAt%x_' % off)
        return top_insert(vt, T)

    def solve(label, Ts, allow_shift, allow_local):
        for rnd in range(a.rounds):
            c, T, path = search(Ts, allow_shift, allow_local)
            unknown = [fh for fh, D2, perm in path if not evaluate(fh, T, D2, perm)[2]]
            if not unknown:
                break
            vobj = U.compile(realize(T, path), f'{label}_r{rnd}')[0]
            for fh, D2, perm in path:
                o = evaluate(fh, T, D2, perm)[1]
                table[fh].setdefault(o, objdis.distance(vobj, a.unit, names[fh], retail[fh]))
                if table[fh][o] < target[fh][1]:
                    target[fh] = (o, table[fh][o])
        c, T, path = search(Ts, allow_shift, allow_local)
        return T, path

    def show(title, T, path):
        print(title + (f' (whole-TU shift T={T})' if T else ''))
        D, total = 0, 0
        prev_off = 0
        for fh, D2, perm in path:
            c, _o, known = evaluate(fh, T, D2, perm)
            total += c
            if D2 != D:
                print(f'   before {names[fh]}: {D2 - D:+d} handles (cumulative {D2})')
                seg = text[prev_off:offs.get(names[fh], prev_off)]
                hints = [(desc, delta, seg.count('\n', 0, mm.start()) + text.count('\n', 0, prev_off) + 1)
                         for rx, desc, delta in SPELLINGS for mm in rx.finditer(seg)]
                for desc, delta, line in hints[:6]:
                    print(f'      spelling candidate line {line}: {desc} ({delta:+d})')
            if perm:
                print(f'   {names[fh]}: local order {" ".join(perm)}')
            if fh in residues or c:
                print(f'      {names[fh]}: {dist0[fh]} -> {c}' + ('' if known else ' (estimate)'))
            D = D2
            prev_off = offs.get(names[fh], prev_off)
        print(f'   total distance {total} (now {sum(dist0.values())})')
        return total

    plans = sorted((sum(evaluate(fh, T, 0, None)[0] for fh in fns), T) for T in range(a.probes))
    Ts = sorted({0, *[T for _c, T in plans[:a.tops]]})
    print('\n== candidates (read-only; nothing applied; typedef runs only measure) ==')
    tiers = []
    T1, P1 = solve('t1', [0], False, True)
    tiers.append(('Tier 1 - local declaration orders only', T1, P1))
    T2, P2 = solve('t2', [0], True, True)
    tiers.append(('Tier 2 - plus handles inserted between functions (control-flow spelling / declarations)', T2, P2))
    T3, P3 = solve('t3', Ts, True, True)
    tiers.append(('Tier 3 - plus a whole-TU shift (include / declaration order before the code)', T3, P3))
    verified = []
    for title, T, path in tiers:
        tot = show(title, T, path)
        if a.verify:
            vobj = U.compile(realize(T, path), 'verify_' + title.split()[1])[0]
            got = {names[fh]: objdis.distance(vobj, a.unit, names[fh], retail[fh]) for fh in fns}
            print(f'   [verify] compiled total {sum(got.values())}: ' +
                  ', '.join(f'{k.split("@")[0][1:]}={v}' for k, v in got.items() if v))
            verified.append([title, T, got])
    report = {'unit': a.unit, 'verified': verified, 'lo_top': lo_top, 'lo_cpp': lo_cpp,
              'now': {names[f]: dist0[f] for f in fns},
              'tiers': [[title, T, [[names[f], d, list(p) if p else None] for f, d, p in path]]
                        for title, T, path in tiers]}
    (U.out / 'solver.json').write_text(json.dumps(report, indent=1))
    print(f'[solver] report {U.out / "solver.json"}')
    return 0


def _after_includes(text):
    ends = [m.end() for m in re.finditer(r'^#include[^\n]*\n', text, re.M)]
    return ends[-1] if ends else 0


if __name__ == '__main__':
    raise SystemExit(main())

"""homm1.walls.aggdecl - the AGGREGATE-VS-SCALAR DECLARATION sieve.

A two-field destination written at the end of a computation has two models,
and the retail bytes separate them. cl 5.0 lowers a whole-object assignment as
ONE COPY: a load pair off consecutive addresses feeding the two stores. Two
independent scalars are two statements fed by whatever registers produced
them, which the scheduler may separate. So per member displacement pair
(K, K+4) each side reads COPY-shape or SCALAR-shape, and a DISAGREEMENT is a
modelling question in a named direction:

  OVER-AGGREGATED   we copy, retail does not - our source declares one object
                    where retail's declares two scalars
  UNDER-AGGREGATED  retail copies, we do not - the mirror

CGrunt::ClaimSwitchTile 0x52c70 is the worked positive and is the fixture the
`--control` self-test runs: retail stores the two halves straight out of EBX
and EDI with a `mov ecx,esi` BETWEEN them, and the `Coord next` model we had
reloaded the struct's frame home into an adjacent store pair instead.

FOUR THINGS MAKE THE IDENTICAL COPY SHAPE AND ARE NOT AN AGGREGATE. Each was
measured on a function whose two sides are otherwise the same code, and each
cost a false row before it was separated out:

  WALK   the source register is a `*p++` cursor and C2 pre-loaded two steps of
         the walk. CDDrawWorkerHost::ReadPlaneObjects reads its record with one
         `*p++` per member on both sides, and retail pairs the loads at exactly
         one of sixty sites.
  ARG    the source slots are INCOMING STACK ARGUMENTS nothing in the function
         ever wrote. Storing parameter N at member M and parameter N+1 at
         member M+4 is two scalar stores; which parameter lands in which
         register is allocation. StreamFeeder::FeederStart emits the identical
         63 instructions on both sides and the pair reads COPY on one side and
         SEP on the other purely from that rotation.
  COPYF  a frame-sourced copy is also what a SPILL of two scalars looks like:
         cl gives them adjacent slots. ClaimSwitchTile still reads copy-shape
         under the two-scalar model that its own default arm proves correct.
  COPYM  a member-sourced copy is also what `a->x = b->x; a->y = b->y;` looks
         like once the scheduler pairs the loads.

So a row is a LEAD to adjudicate against the disassembly, never a verdict. The
sub-kind is printed to say which adjudication applies.

TWO EXTRACTION BUGS THIS SIEVE IS BUILT AGAINST, both of which produced a
confident one-directional census before they were fixed:

  * the source load must be found by REACHING DEFINITION, not by a fixed
    look-back window. Searching a fixed number of instructions before the FIRST
    store makes the answer depend on where the scheduler put the two loads:
    CBattlezMapConfig::TrackAssignedEnemy emits the identical eight-instruction
    RECT copy on both sides, and a ONE-INSTRUCTION rotation put one load inside
    the window on one side and outside it on the other. That alone accounted
    for 44 of the 55 rows the first sweep reported.
  * the displacement must be UNFOLDED through `lea R,[base+K]`. cl parks
    `obj+0x60` in a register and stores at `[R+0x8]`, which is member 0x68;
    keying on the raw displacement reads one member as two different ones on
    the two sides, and every key on that side shifts at once.

A frame-based destination is dropped rather than compared: the two sides do not
agree on frame offsets. A register that received a frame address is a frame
base however it is spelled.

`--reads` runs the READ side of the same question, and it is the more
productive half. A whole-object copy loads BOTH halves before anything is
tested; `a.m_x != X || a.m_y != Y` short-circuits, so cl tests the first half
straight out of memory (`cmp [obj+K],r`) and only touches the second on the
fall-through. Three kinds: PAIR (both halves in registers, no conditional
branch between), SPLIT (a branch separates the touches), MEMOP (a half is
consumed as a memory operand and never loaded). CTriggerMgr::UseToyAt
0x6e120 is the worked positive: one `Coord` copy per compared pair took it
87.70 -> 88.73 and made the whole region instruction-for-instruction retail's.
Calibration on the 100% rows: 1515 comparable keys, 1515 agree, zero rows.

    homm1 walls aggdecl [--todo] [--reads] [--unit U] [--fn F] [--calibrate]
                         [--control] [--explain UNIT SYM DISP]
"""

from __future__ import annotations

from homm1.core.usage import logged

import re
import sys
from collections import defaultdict

from homm1.delink.coffx import Obj
from homm1.walls import pairscan

MEM = re.compile(r"^(?:(?:BYTE|WORD|DWORD|QWORD|XMMWORD) PTR )?\[([^\]]+)\]$")
REG32 = {"eax", "ecx", "edx", "ebx", "esp", "ebp", "esi", "edi"}

#: a register loaded with a FRAME ADDRESS
FRAME_ADDR = re.compile(
    r"^(?:lea|mov)\s+(e[a-z][a-z]),"
    r"(?:esp|\[esp(?:\+e[a-z]{2}(?:\*\d)?)?(?:[+-]0x[0-9a-f]+)?\])$")
WRITES_DST = re.compile(
    r"^(?:mov|lea|add|sub|and|or|xor|imul|movzx|movsx|sar|shl|shr|sal|rol|ror"
    r"|neg|not|inc|dec|pop|xchg|adc|sbb|cmov\w*|set\w*)\b")
BIAS = re.compile(r"^(?:add|sub)\s+(e[a-z][a-z]),0x[0-9a-f]+$")
BIAS_LEA = re.compile(r"^lea\s+(e[a-z][a-z]),\[(e[a-z][a-z])[+-]0x[0-9a-f]+\]$")
REG_COPY = re.compile(r"^mov\s+(e[a-z][a-z]),(e[a-z][a-z])$")
#: the base FOLD: `lea R,[S]` / `lea R,[S+-0xNN]`
LEA_FOLD = re.compile(
    r"^lea\s+(e[a-z][a-z]),\[(e[a-z][a-z])(?:([+-])(0x[0-9a-f]+))?\]$")
CALL_CLOBBER = ("eax", "ecx", "edx")

#: how far apart the two stores of one pair may sit
WINDOW = 8
#: how far a cursor bump may sit from the pair and still explain it
WALK_SPAN = 8
AGGREGATE = ("COPYF", "COPYM", "ARG", "WALK")

#: `--reads`: the READ side of the same question. A whole-object copy loads
#: BOTH halves before anything is tested; `a.m_x != X || a.m_y != Y` short-
#: circuits, so cl tests the first half straight out of memory and only
#: touches the second on the fall-through. CTriggerMgr::UseToyAt 0x6e120
#: is the worked positive (87.70 -> 88.73).
COND_JUMP = re.compile(r"^j(?!mp$)[a-z]+$")
READ_WINDOW = 10


def _analyse(ins):
    """(frame taint per instruction, base-unfold map per instruction).

    unfold[i][R] = (root_token, bias): a store `[R+d]` at instruction i
    addresses `root_token + bias + d`.
    """
    live: set[str] = set()
    unfold: dict[str, tuple[str, int]] = {r: (f"{r}@in", 0) for r in REG32}
    taints, unfolds = [], []
    for i, (_off, mn, ops) in enumerate(ins):
        taints.append(frozenset(live))
        unfolds.append(dict(unfold))
        asm = f"{mn} {ops}".strip()
        m = FRAME_ADDR.match(asm)
        if m:
            live.add(m.group(1))
        elif mn == "call":
            live -= set(CALL_CLOBBER)
            for r in CALL_CLOBBER:
                unfold[r] = (f"{r}@{i}", 0)
            continue
        else:
            copy, bias = REG_COPY.match(asm), BIAS.match(asm) or BIAS_LEA.match(asm)
            if copy:
                (live.add if copy.group(2) in live else live.discard)(copy.group(1))
            elif bias:
                src = bias.group(bias.lastindex)
                (live.add if src in live else live.discard)(bias.group(1))
            elif WRITES_DST.match(asm) and "," in ops:
                live.discard(ops.split(",")[0].strip())
        fold = LEA_FOLD.match(asm)
        if fold:
            dst, src, sign, imm = fold.groups()
            root, at = unfold[src]
            off = int(imm, 16) * (-1 if sign == "-" else 1) if imm else 0
            unfold[dst] = (root, at + off)
            continue
        copy = REG_COPY.match(asm)
        if copy:
            unfold[copy.group(1)] = unfold[copy.group(2)]
            continue
        if BIAS.match(asm):
            reg = BIAS.match(asm).group(1)
            unfold[reg] = (f"{reg}@{i}", 0)
            continue
        if WRITES_DST.match(asm) and "," in ops:
            dst = ops.split(",")[0].strip()
            if dst in REG32:
                unfold[dst] = (f"{dst}@{i}", 0)
        elif mn in ("pop", "inc", "dec", "neg", "not") and ops.strip() in REG32:
            unfold[ops.strip()] = (f"{ops.strip()}@{i}", 0)
    return taints, unfolds


def _split_mem(op: str):
    op = op.strip()
    if not op.startswith("DWORD PTR ["):
        return None
    m = MEM.match(op)
    if not m:
        return None
    inner = m.group(1)
    d = re.search(r"([+-])(0x[0-9a-f]+)$", inner)
    if d:
        return inner[: d.start()].strip(), \
            int(d.group(2), 16) * (-1 if d.group(1) == "-" else 1)
    return inner.strip(), 0


def _base_regs(base: str) -> set:
    return {t for t in re.split(r"[+*]", base) if t in REG32}


def classify(ins, window: int = WINDOW):
    """{(root, K): [(kind, gap, order, index)]} - displacements UNFOLDED,
    frame-based destinations dropped."""
    taints, unfolds = _analyse(ins)
    stores, loads, frame_stores = [], {}, set()
    for i, (_off, mn, ops) in enumerate(ins):
        if mn != "mov":
            continue
        parts = [p.strip() for p in ops.split(",")]
        if len(parts) != 2:
            continue
        dst, src = parts
        mem = _split_mem(dst)
        if mem is not None:
            base, disp = mem
            if _base_regs(base) & taints[i] or base.startswith("esp"):
                if base in REG32:
                    root, at = unfolds[i][base]
                    frame_stores.add((root, at + disp))
                continue
            if base in REG32:
                root, at = unfolds[i][base]
                stores.append((i, root, at + disp, src))
            continue
        if dst in REG32:
            mem = _split_mem(src)
            if mem is not None:
                base, disp = mem
                if base in REG32:
                    root, at = unfolds[i][base]
                    loads[i] = (dst, root, at + disp)
                else:
                    loads[i] = (dst, base, disp)

    def reaching_load(store_i, reg):
        if reg not in REG32:
            return None
        for j in range(store_i - 1, -1, -1):
            _off, mn, ops = ins[j]
            if mn == "call":
                return None
            if j in loads and loads[j][0] == reg:
                return loads[j]
            asm = f"{mn} {ops}".strip()
            if WRITES_DST.match(asm) and "," in ops:
                if ops.split(",")[0].strip() == reg:
                    return None
            elif mn in ("pop", "inc", "dec", "neg", "not") and ops.strip() == reg:
                return None
        return None

    def kind_of(load_a, load_b, lo_i, hi_i):
        root = load_a[1]
        for j in range(max(0, lo_i - WALK_SPAN), min(len(ins), hi_i + WALK_SPAN)):
            _off, mn, ops = ins[j]
            bump = BIAS.match(f"{mn} {ops}".strip())
            if bump and bump.group(1) in root:
                return "WALK"
        if not str(root).startswith("esp"):
            return "COPYM"
        if (root, load_a[2]) not in frame_stores \
                and (root, load_b[2]) not in frame_stores:
            return "ARG"
        return "COPYF"

    out = defaultdict(list)
    by_root = defaultdict(list)
    for (i, root, disp, src) in stores:
        by_root[root].append((i, disp, src))
    for root, group in by_root.items():
        for a in range(len(group)):
            for b in range(len(group)):
                if a == b:
                    continue
                ia, da, sa = group[a]
                ib, db, sb = group[b]
                if db != da + 4:
                    continue
                lo_i, hi_i = (ia, ib) if ia < ib else (ib, ia)
                if hi_i - lo_i > window:
                    continue
                la, lb = reaching_load(ia, sa), reaching_load(ib, sb)
                if la and lb and la[1] == lb[1] and lb[2] == la[2] + 4:
                    kind = kind_of(la, lb, lo_i, hi_i)
                else:
                    kind = "ADJ" if hi_i == lo_i + 1 else "SEP"
                out[(root, da)].append(
                    (kind, hi_i - lo_i - 1, "asc" if ia < ib else "desc", lo_i))
    return out


def classify_reads(ins, window: int = READ_WINDOW):
    """{(root, K): [kind]} for each consecutive dword pair that is READ.

    PAIR   both halves land in registers with no conditional branch between
           them - a whole-object copy
    SPLIT  a conditional branch separates the two touches - `||` short-circuit
    MEMOP  a half is consumed as a memory operand (`cmp [obj+K],r`) and never
           loaded, which only a field-wise test produces
    """
    taints, unfolds = _analyse(ins)
    touches = []
    for i, (_off, mn, ops) in enumerate(ins):
        for operand in [o.strip() for o in ops.split(",")]:
            mem = _split_mem(operand)
            if mem is None:
                continue
            base, disp = mem
            if base not in REG32 or _base_regs(base) & taints[i] \
                    or base.startswith("esp"):
                continue
            parts = [o.strip() for o in ops.split(",")]
            loaded = (mn == "mov" and len(parts) == 2
                      and parts[0] in REG32 and parts[1] == operand)
            if mn == "mov" and not loaded:
                continue          # a STORE, which classify() owns
            root, at = unfolds[i][base]
            touches.append((i, root, at + disp, loaded))
    first = {}
    for i, root, disp, loaded in touches:
        first.setdefault((root, disp), (i, loaded))
    out = defaultdict(list)
    for (root, disp), (ia, load_a) in first.items():
        hi = first.get((root, disp + 4))
        if hi is None:
            continue
        ib, load_b = hi
        lo_i, hi_i = (ia, ib) if ia < ib else (ib, ia)
        if hi_i - lo_i > window:
            continue
        if not (load_a and load_b):
            kind = "MEMOP"
        elif any(COND_JUMP.match(ins[j][1]) for j in range(lo_i + 1, hi_i)):
            kind = "SPLIT"
        else:
            kind = "PAIR"
        out[(root, disp)].append((kind, hi_i - lo_i - 1, "asc", lo_i))
    return out


def keyed(cl):
    """{unfolded member displacement: [kind, ...]} - the base ROOT is not
    comparable across the two sides, the unfolded displacement is."""
    out = defaultdict(list)
    for (_root, disp), v in cl.items():
        out[disp].extend(x[0] for x in v)
    return out


def sides(bobj, tobj, bf, tf, sym, reads=False):
    fn = classify_reads if reads else classify
    return (keyed(fn(pairscan.insns(bobj, *bf[sym]))),
            keyed(fn(pairscan.insns(tobj, *tf[sym]))))


def scan(unit_filter=None, fn_filter=None, todo=False, calibrate=False,
         reads=False):
    from homm1.walls import inventory
    pairscan.require_pairs({unit_filter} if unit_filter else None)
    rows = inventory.build(unit_filter, 100.0, todo=todo)
    wanted = defaultdict(list)
    if calibrate:
        scores, _live = pairscan.scores()
        for (unit, sym), pct in scores.items():
            if pct >= 100.0 and not (unit_filter and unit != unit_filter):
                wanted[unit].append((sym, pct, ""))
    else:
        for r in rows:
            wanted[r["unit"]].append((r["symbol"], r["cur"], r["rva"]))
    over, under, tally = [], [], defaultdict(int)
    n_fn = n_keys = unpaired = 0
    for unit, fns in sorted(wanted.items()):
        pair = pairscan.pairs({unit}).get(unit)
        if not pair:
            continue
        try:
            bobj, tobj = Obj(pair[0]), Obj(pair[1])
        except (ValueError, OSError):
            continue
        bf, tf = pairscan.functions(bobj), pairscan.functions(tobj)
        for sym, pct, rva in fns:
            if fn_filter and fn_filter not in sym:
                continue
            if sym not in bf or sym not in tf:
                unpaired += 1
                continue
            n_fn += 1
            bk, tk = sides(bobj, tobj, bf, tf, sym, reads)
            for disp in set(bk) | set(tk):
                b, t = sorted(bk.get(disp, [])), sorted(tk.get(disp, []))
                n_keys += 1
                tally[(tuple(b), tuple(t))] += 1
                if not b or not t:
                    continue
                whole = ("PAIR",) if reads else AGGREGATE
                agg_b = any(k in whole for k in b)
                agg_t = any(k in whole for k in t)
                if agg_b and not agg_t:
                    over.append((pct, unit, sym, rva, disp, b, t))
                if agg_t and not agg_b:
                    under.append((pct, unit, sym, rva, disp, b, t))
    return over, under, tally, n_fn, n_keys, unpaired


# ---------------------------------------------------------------- self test
def _parse(text):
    out = []
    for i, ln in enumerate(text.strip().splitlines()):
        ln = ln.split(";")[0].strip()
        if not ln:
            continue
        t = ln.split(None, 1)
        out.append((i, t[0], t[1] if len(t) > 1 else ""))
    return out


#: ClaimSwitchTile 0x52c70: the `Coord next` model we had, and retail
FIXTURE_AGGREGATE = """
mov DWORD PTR [esp+0x18],ebx
mov DWORD PTR [esp+0x1c],edi
mov ecx,esi
call 0x1000
mov eax,DWORD PTR [esp+0x18]
mov ecx,DWORD PTR [esp+0x1c]
mov DWORD PTR [esi+0x17c],eax
mov DWORD PTR [esi+0x180],ecx
mov ecx,esi
"""
FIXTURE_SCALARS = """
mov DWORD PTR [esi+0x17c],ebx
mov ecx,esi
mov DWORD PTR [esi+0x180],edi
"""
#: ReadPlaneObjects: a `*p++` walk whose two steps C2 pre-loaded
FIXTURE_WALK = """
mov eax,DWORD PTR [ebp+0x0]
mov ecx,DWORD PTR [ebp+0x4]
add ebp,0x4
mov DWORD PTR [esi+0x20],eax
add ebp,0x4
mov DWORD PTR [esi+0x24],ecx
"""
#: StreamFeeder::FeederStart: two incoming parameters, not an object
FIXTURE_ARG = """
mov eax,DWORD PTR [esp+0xc]
mov ecx,DWORD PTR [esp+0x10]
mov DWORD PTR [esi+0x10],eax
mov DWORD PTR [esi+0x14],ecx
"""
#: UseToyAt 0x6e120: the pair-load retail emits, and the `||` we had
FIXTURE_PAIR_READ = """
mov eax,DWORD PTR [ebp+0x17c]
mov ecx,DWORD PTR [ebp+0x180]
cmp eax,edx
jne 0x100
cmp ecx,DWORD PTR [esp+0x24]
je 0x200
"""
FIXTURE_SPLIT_READ = """
cmp DWORD PTR [ebp+0x17c],eax
jne 0x100
mov ecx,DWORD PTR [ebp+0x180]
mov edx,DWORD PTR [esp+0x34]
cmp ecx,edx
je 0x200
"""
#: TrackAssignedEnemy: `lea ebx,[edi+0x60]` makes `[eax+0x8]` member 0x68
FIXTURE_FOLD = """
lea ebx,[edi+0x60]
mov eax,ebx
mov DWORD PTR [eax],ecx
mov DWORD PTR [eax+0x4],edx
mov DWORD PTR [eax+0x8],ecx
mov DWORD PTR [eax+0xc],edx
"""


def control() -> bool:
    """Re-prove the detector on the worked positive and the three shapes that
    are NOT an aggregate. A sieve that answers zero has to answer this first."""
    ok = True
    cases = [
        ("known positive (ours)", FIXTURE_AGGREGATE, 0x17c, ["COPYF"]),
        ("known positive (retail)", FIXTURE_SCALARS, 0x17c, ["SEP"]),
        ("walk negative", FIXTURE_WALK, 0x20, ["WALK"]),
        ("argument negative", FIXTURE_ARG, 0x10, ["ARG"]),
    ]
    for name, text, disp, want in cases:
        got = keyed(classify(_parse(text))).get(disp)
        good = got == want
        ok &= good
        print(f"  {'OK  ' if good else 'FAIL'} {name:24s} +0x{disp:x} -> {got}")
    for name, text, disp, want in (
            ("pair read (retail)", FIXTURE_PAIR_READ, 0x17c, ["PAIR"]),
            ("split read (ours)", FIXTURE_SPLIT_READ, 0x17c, ["MEMOP"])):
        got = keyed(classify_reads(_parse(text))).get(disp)
        good = got == want
        ok &= good
        print(f"  {'OK  ' if good else 'FAIL'} {name:24s} +0x{disp:x} -> {got}")
    fold = set(keyed(classify(_parse(FIXTURE_FOLD))))
    good = fold == {0x60, 0x64, 0x68}
    ok &= good
    print(f"  {'OK  ' if good else 'FAIL'} base fold                    -> "
          f"{sorted(hex(x) for x in fold)}")
    return ok


def explain(unit: str, sym: str, disp: int, ctx: int = 6,
            reads: bool = False) -> int:
    """Print the instruction neighbourhood behind one key on both sides."""
    pair = pairscan.require_pairs({unit}).get(unit)
    for tag, path in (("OURS", pair[0]), ("RETAIL", pair[1])):
        obj = Obj(path)
        fns = pairscan.functions(obj)
        names = [k for k in fns if sym in k]
        if not names:
            print(f"==== {tag}: no symbol matching {sym!r}")
            continue
        ins = pairscan.insns(obj, *fns[names[0]])
        cl = (classify_reads if reads else classify)(ins)
        hits = [(k, v) for k, v in cl.items() if k[1] == disp]
        print(f"==== {tag}  ({len(ins)} insns)  +0x{disp:x} -> "
              f"{[x[0] for _k, v in hits for x in v]}")
        for i in sorted({x[3] for _k, v in hits for x in v}):
            for j in range(max(0, i - ctx), min(len(ins), i + ctx + 3)):
                _off, mn, ops = ins[j]
                print(f"  [{j:3d}] {mn} {ops}")
            print("   --")
    return 0


@logged
def main(argv=None) -> int:
    import argparse
    from homm1.walls import check_unit
    ap = argparse.ArgumentParser(
        prog="homm1 walls aggdecl", description=__doc__,
        formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--todo", action="store_true")
    ap.add_argument("--reads", action="store_true",
                    help="the READ side: does each side load BOTH halves "
                         "before testing either")
    ap.add_argument("--unit")
    ap.add_argument("--fn", help="substring of the mangled name")
    ap.add_argument("--calibrate", action="store_true",
                    help="rows on 100%% functions - the detector-bug rate")
    ap.add_argument("--control", action="store_true",
                    help="re-prove the detector on its fixtures and stop")
    ap.add_argument("--explain", nargs=3, metavar=("UNIT", "SYM", "DISP"),
                    help="dump both sides around one key")
    ap.add_argument("--limit", type=int, default=0)
    a = ap.parse_args(argv)
    if a.control:
        return 0 if control() else 1
    if a.explain:
        return explain(a.explain[0], a.explain[1], int(a.explain[2], 0),
                       reads=a.reads)
    unit = check_unit(a.unit)
    over, under, tally, n_fn, n_keys, unpaired = scan(
        unit, a.fn, a.todo, a.calibrate, a.reads)
    print(f"functions screened: {n_fn}   member pair-keys: {n_keys}"
          f"   unpaired symbols: {unpaired}")
    what = "read the pair whole" if a.reads else "copy"
    for title, rows in ((f"OVER-AGGREGATED (we {what}, retail does not)", over),
                        (f"UNDER-AGGREGATED (retail {what}s, we do not)"
                         .replace("wholes", "whole"), under)):
        print(f"\n{title}: {len(rows)}")
        for pct, u, sym, rva, disp, b, t in sorted(rows)[:a.limit or None]:
            print(f"  {rva or '':8s} {u:22s} {pct:6.2f} +0x{disp:x}"
                  f"  ours={b} retail={t}  {sym}")
    agree = sum(v for k, v in tally.items() if k[0] == k[1])
    print(f"\nagreeing keys {agree}/{n_keys}; the top classes:")
    for k, v in sorted(tally.items(), key=lambda kv: -kv[1])[:12]:
        print(f"  {k}  {v}")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))

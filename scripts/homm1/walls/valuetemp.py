"""homm1.walls.valuetemp - the by-value struct temp sieve.

An inlined accessor returning a two-field struct BY VALUE materialises a frame
temp. The half whose field is READ gets folded into its consumer (cl re-loads
the source member instead), and the UNREAD half's store is emitted and left
dead. Two calls therefore leave two dead stores in adjacent frame slots, fed by
two adjacent member offsets through one base register:

    mov  <rA>,[<b>+N]     mov  <rB>,[<b>+N+4]
    mov  [esp+K],<rA>     mov  [esp+K+4],<rB>       both dead

  target-only   retail takes the pair by value where we read the members -
                give the class the accessor and consume it as an rvalue
                (`Coord LastTilePx() { return m_lastTilePx; }`, used as
                `LastTilePx().m_x`, NOT bound to a named local).
  base-only     the mirror: we copy a pair retail reads in place.

ONLY A MEMBER PAIR IS COMPARABLE. A pair read from another FRAME slot is a local
aggregate copy, and its source offset is a frame offset, which the two sides do
not agree on: RouteToNearbyEnemy emits the identical eleven-instruction RECT
copy on both sides, at [esp+0x6c] on ours and [esp+0x80] on retail's, and keying
on that offset reported it as an asymmetry in BOTH directions at once. Those
pairs are counted and never compared. The base REGISTER is not comparable
either - the two sides put `this` in different registers - so a member pair is
keyed on its member offset alone.

DEAD is decided by the EVENT ORDER on the slot: a store is dead when the next
event on its slot is another store (the RectContains form, where the real value
overwrites the temp) or nothing at all (the GetModeSize form). Three things make
the order the only workable rule.

ESP IS TRACKED, and a CALL RESTORES THE FRAME LEVEL - argument pushes are gone
once it returns. Counting pushes without that drifts the delta upward
monotonically, so one `[esp+0x54]` normalises to five different slots across a
body and the same physical slot before and after a call reads as two.

An ADDRESS-TAKEN aggregate names only its BASE, so a per-dword read scan calls
its interior fields dead; an `lea` observes the whole object it points at, and
it keeps observing it, because the pointer outlives the `lea`.

And a store is only dead against its OWN slot's later events - the RectContains
temp escapes the same slot its successor already killed.

Each row also carries where the POINTER came from, because that is a second cl
5.0 fact: a load through a pointer read from a GLOBAL blocks the dead-store
elimination the same load through a pointer PARAMETER allows. `--global` keeps
only the rows whose pair reaches its member through a global.

`--calibrate` is the negative control this sieve exists to have: it reports the
rows on 100.00% functions, where the two sides ARE the same bytes and any row is
a detector bug. A clean zero from a detector nobody has seen fire is not a
result - `--control` re-proves it against the rows both mechanisms were
established on (the temp on RectContains/RectContainsGated, the global pointer
on SaveScreenshot), and the header line's provenance histogram says how big each
class is, so a `--global` zero is a measurement rather than an empty query.

    homm1 walls valuetemp [--unit U] [--fn SUB] [--global]
                           [--calibrate] [--control]
"""

from __future__ import annotations

from homm1.core.usage import logged

import argparse
import collections
import re
import sys

from homm1.delink.coffx import Obj
from homm1.walls import check_unit, pairscan

STORE = re.compile(r"^(?:DWORD PTR )?\[esp(?:\+0x([0-9a-f]+))?\],(e[a-z][a-z])$")
LOAD = re.compile(r"^(e[a-z][a-z]),(?:DWORD PTR )?\[(e[a-z][a-z])\+0x([0-9a-f]+)\]$")
ESPREF = re.compile(r"\[esp(?:\+0x([0-9a-f]+))?\]")
LEA = re.compile(r"^e[a-z][a-z],\[esp(?:\+0x([0-9a-f]+))?\]$")
ABS_LOAD = re.compile(r"^(e[a-z][a-z]),(?:DWORD PTR )?ds:0x[0-9a-f]+$")
FRAME_LOAD = re.compile(r"^(e[a-z][a-z]),(?:DWORD PTR )?\[esp(?:\+0x([0-9a-f]+))?\]$")
REG_MOVE = re.compile(r"^(e[a-z][a-z]),(e[a-z][a-z])$")
# An address-taken frame object is live THROUGHOUT, and only its BASE appears in
# the operand - so a per-dword read scan calls its interior fields dead. That is
# the sieve's one false-positive class (CBattlezMapConfig::ScanRegion built two
# adjacent RECTs, `lea`'d both and pushed one; the sieve read the second RECT's
# right/bottom stores as a Coord temp). An address-take therefore covers a whole
# object: up to the next address-taken base, capped at the widest aggregate this
# codebase passes that way.
WIDEST_AGGREGATE = 0x10          # RECT; Coord is 8
SAVE_REGS = ("ebx", "ebp", "esi", "edi")
FS_INSTALL = "DWORD PTR fs:0x0,esp"      # the /GX registration node going live
# (unit, symbol, member offset, which sides must carry it - "" = NEITHER). The
# first two took the accessor and agree. UseEquippedToolAt is the NEGATIVE: it read
# target-only while esp tracking ignored callee-popped arguments, and under a
# correct frame level retail's two stores are a live local Coord whose address
# it passes (`lea edx,[esp+0x20]` at slot -0x8), not a dead by-value temp.
CONTROL = ()  # Requires HoMM1-specific live calibration evidence.
# (unit, symbol, member offset, provenance). The PROVENANCE walker's own known
# positive: SaveScreenshot reaches its pair through a pointer read from a global,
# which is the alias fact that keeps the store alive. It is at 100.00%, so both
# sides carry it and this controls the walker, not the asymmetry.
PROV_CONTROL = ()  # Requires HoMM1-specific live calibration evidence.
LOOKBACK = 40
# One temp is emitted as one run, so its two dead stores are neighbours. Without
# a bound, two unrelated dead stores at adjacent slots read as a pair.
PAIR_SPAN = 24


def _frame_level(ins) -> int:
    """esp's depth once the frame is established - what a call restores to.

    Getting this wrong is not a rounding error: the level is what a call
    restores to, so a level short by the saved registers makes every post-call
    slot collide with a pre-call one (measured on CTriggerMgr::UseEquippedToolAt,
    whose `sub esp,0x14` PRECEDES four pushes, and whose switch table decodes as
    trailing garbage so the epilogue cannot be read backwards either).

    The prologue is `sub esp,N` plus the callee-save pushes, and cl 5.0
    interleaves `mov`s and non-esp `lea`s among them - so the run cannot be cut
    at the first non-push. What ends it is an ARGUMENT push, and cl 5.0 saves
    only ebx/ebp/esi/edi and each only once, so the first push that is not a
    fresh one of those is the boundary. A /GX function is the exception: it
    pushes -1, the handler and the old fs:0 chain FIRST, and cl 5.0 does not
    give it an ebp frame - the registration install is what ends its preamble."""
    eh = any(mn == "mov" and ops == FS_INSTALL for _o, mn, ops in ins[:12])
    d, saved, installed = 0, set(), not eh
    for _off, mn, ops in ins:
        if mn == "push":
            if installed:
                if ops not in SAVE_REGS or ops in saved:
                    break
                saved.add(ops)
            d += 4
        elif mn == "sub" and ops.startswith("esp,0x"):
            d += int(ops.split("0x", 1)[1], 16)
        elif mn == "mov" and ops == FS_INSTALL:
            installed = True
        elif mn in ("call", "ret") or mn.startswith("j") \
                or (mn == "lea" and "[esp" in ops):
            break
    return d


def _esp_trace(ins):
    """[(mnemonic, operands, delta)] - esp's offset below its value at entry.

    A CALL RESTORES THE FRAME LEVEL. Argument pushes are popped by the callee
    under this codebase's `__thiscall`/`__stdcall`, and by the caller's own
    `add esp,N` under `__cdecl`; either way esp is back at the frame level once
    the call returns. Counting pushes without that made the delta drift upward
    monotonically - measured on CBattlezMapConfig::RepathAroundBlockedTiles, one
    `[esp+0x54]` normalised to five different slots across the body - so the
    same physical slot before and after a call read as two, which is a false
    dead store in one direction and a missed one in the other."""
    out, d = [], _frame_level(ins)
    level, d, after_call = d, 0, False
    for _off, mn, ops in ins:
        out.append((mn, ops, d))
        if mn == "push":
            d += 4
        elif mn == "pop":
            d = max(level, d - 4)
        elif mn in ("sub", "add") and ops.startswith("esp,0x"):
            n = int(ops.split("0x", 1)[1], 16)
            if mn == "sub":
                d += n
            elif not after_call:      # a cdecl cleanup the call already undid
                d = max(level, d - n)
        elif mn in ("call", "ret"):
            d = level
        after_call = mn == "call"
    return out


def _source_of(ins, i, reg):
    """(base register, offset, load index) for the member `reg` holds at i."""
    for j in range(i - 1, max(-1, i - LOOKBACK), -1):
        _off, mn, ops = ins[j]
        if mn == "mov":
            m = LOAD.match(ops)
            if m and m.group(1) == reg:
                return m.group(2), int(m.group(3), 16), j
        if ops.startswith(f"{reg},") or ops == reg:
            return None          # some other definition of reg
    return None


def _provenance(ins, tr, i, reg, depth=4):
    """Where the POINTER in `reg` at index i came from.

    `glob`  it was loaded from a global (`mov reg,ds:0x0`, a DIR32 site).
    `param` it traces back to an incoming argument or the entry `this`.
    `local` it came out of the frame.

    The distinction is a cl 5.0 alias-analysis fact: a load through a pointer
    read from a GLOBAL blocks the dead-store elimination that the same load
    through a pointer PARAMETER allows, so a `glob` pair is a store the source
    can steer and a `param` pair is not."""
    if depth <= 0:
        return "?"
    # the WHOLE prefix, not LOOKBACK: a receiver is loaded once in the prologue
    # and used hundreds of instructions later
    for j in range(i - 1, -1, -1):
        _off, mn, ops = ins[j]
        if mn == "call":
            if reg in ("eax", "ecx", "edx"):
                return "?"          # clobbered; the value is a call result
            continue
        if mn == "pop":
            if ops == reg:
                return "?"
            continue
        if mn in ("push", "cmp", "test") or not ops.startswith(f"{reg},"):
            continue                # a read, not a definition
        if ABS_LOAD.match(ops):
            return "glob"
        m = FRAME_LOAD.match(ops)
        if m:
            slot = (int(m.group(2), 16) if m.group(2) else 0) - tr[j][2]
            return "param" if slot > 0 else "local"
        m = LOAD.match(ops)
        if m:
            return _provenance(ins, tr, j, m.group(2), depth - 1)
        m = REG_MOVE.match(ops)
        if m and mn == "mov":
            return _provenance(ins, tr, j, m.group(2), depth - 1)
        return "?"
    return "param" if reg in ("ecx", "edx") else "?"


def _escape_slots(base: int, bases: list[int]) -> range:
    """The dword slots one `lea`-taken frame address keeps live: up to the next
    address-taken base, capped at the widest aggregate passed that way."""
    nxt = next((b for b in bases if b > base), base + WIDEST_AGGREGATE)
    return range(base, min(nxt, base + WIDEST_AGGREGATE), 4)


def _pairs(ins):
    """({(base kind, member offset N)}, {pair: pointer provenance}) for each dead
    adjacent store pair fed by [b+N],[b+N+4].

    A store is dead when the next event on its slot is another store - the
    RectContains form, where the real value overwrites the temp - or when the
    slot is never observed at all. Order is what separates a kill from a read,
    and it is why an escape cannot be a set: RectContains OVERWRITES the temp
    slot and then takes that slot's address, so a set-based escape screen loses
    the established positive. But an escape is not a point event either. The
    pointer outlives the `lea`, so a slot whose address is taken ANYWHERE is
    observed by any store with no killing store after it - CBattlezMapConfig::
    RepathAroundBlockedTiles fills a RECT and schedules the last field's store
    after the `lea`+pushes that consume it.

    Deadness is a property of ONE STORE, not of the slot: a slot cl reuses holds
    several stores with different sources, and attributing the first store's
    verdict to a later store's source reports a pair that is not there."""
    tr = _esp_trace(ins)
    events: dict[int, list[tuple[int, str]]] = {}
    src_of: dict[tuple[int, int], tuple[str, int]] = {}
    leas: list[tuple[int, int]] = []
    for i, (mn, ops, d) in enumerate(tr):
        m = STORE.match(ops) if mn == "mov" else None
        if m is not None:
            slot = (int(m.group(1), 16) if m.group(1) else 0) - d
            events.setdefault(slot, []).append((i, "w"))
            src = _source_of(ins, i, m.group(2))
            if src:
                src_of[(slot, i)] = src
            continue
        if mn == "lea" and (g := LEA.match(ops)):
            leas.append((i, (int(g.group(1), 16) if g.group(1) else 0) - d))
        for g in ESPREF.finditer(ops):
            events.setdefault((int(g.group(1), 16) if g.group(1) else 0) - d,
                              []).append((i, "r"))
    bases = sorted({b for _i, b in leas})
    escaped: set[int] = set()
    for i, base in leas:
        for slot in _escape_slots(base, bases):
            events.setdefault(slot, []).append((i, "r"))
            escaped.add(slot)

    dead: dict[int, tuple[str, int, int, int]] = {}
    for slot, evs in events.items():
        evs.sort()
        for pos, (i, kind) in enumerate(evs):
            src = src_of.get((slot, i)) if kind == "w" else None
            if src is None:
                continue
            after = evs[pos + 1:]
            killed = bool(after) and after[0][1] == "w"
            if killed or (not after and slot not in escaped):
                dead[slot] = (src[0], src[1], src[2], i)
                break
    out, prov, esp = set(), {}, 0
    for k, (b, n, load_at, at) in dead.items():
        # A pair read from another FRAME SLOT is a local aggregate copy, and its
        # source offset is a frame offset - which is NOT comparable across the
        # two sides. CBattlezMapConfig::RouteToNearbyEnemy emits the identical
        # eleven-instruction RECT copy on both sides at [esp+0x6c] and
        # [esp+0x80], and keying on the offset read that as an asymmetry in both
        # directions at once. They are counted, never compared.
        hi = dead.get(k + 4)
        if not (hi and hi[0] == b and hi[1] == n + 4
                and abs(hi[3] - at) <= PAIR_SPAN):
            continue
        if b == "esp":
            esp += 1
            continue
        out.add(("mem", n))
        prov[("mem", n)] = _provenance(ins, tr, load_at, b)
    return out, prov, esp


def temps(ins):
    return _pairs(ins)[0]


def _sides(bobj, tobj, bf, tf, sym):
    bsec, bs, be = bf[sym]
    tsec, ts, te = tf[sym]
    return (_pairs(pairscan.insns(bobj, bsec, bs, be)),
            _pairs(pairscan.insns(tobj, tsec, ts, te)))


def scan(unit_filter=None, fn_filter=None, calibrate=False):
    sc, live = pairscan.scores()
    agree = collections.Counter()
    local = 0
    miss, extra, counts = [], [], []
    for unit, (base, target) in sorted(
            pairscan.require_pairs({unit_filter} if unit_filter else None).items()):
        if live and unit not in live:
            continue
        try:
            bobj, tobj = Obj(base), Obj(target)
        except (ValueError, OSError):
            continue
        bf, tf = pairscan.functions(bobj), pairscan.functions(tobj)
        for sym in sorted(set(bf) & set(tf)):
            if fn_filter and fn_filter not in sym:
                continue
            pct = sc.get((unit, sym))
            if pct is None:
                continue
            if calibrate != (pct >= 100.0):
                continue
            (b, bp, be), (t, tp, te) = _sides(bobj, tobj, bf, tf, sym)
            local += be + te
            if be != te:
                counts.append((pct, unit, sym, be, te))
            if b == t:
                for pair in t:
                    agree[tp.get(pair, "?")] += 1
                continue
            if t - b:
                miss.append((pct, unit, sym, sorted(t - b), tp))
            if b - t:
                extra.append((pct, unit, sym, sorted(b - t), bp))
    return agree, local, miss, extra, counts


def _label(pairs, prov=None) -> str:
    out = []
    for k, n in pairs:
        where = (prov or {}).get((k, n))
        out.append(f"{'esp' if k == 'esp' else ''}+{n:#x}"
                   + (f"[{where}]" if where and where != "esp" else ""))
    return "/".join(out)


def _show(title, rows, only_global=False):
    if only_global:
        rows = [r for r in rows
                if any(r[4].get(pair) == "glob" for pair in r[3])]
    print(f"\n{title}: {len(rows)}")
    for pct, unit, sym, offs, prov in sorted(rows):
        print(f"{pct:7.2f}  {unit:<22} {_label(offs, prov):<30} {sym}")


@logged
def main(argv=None) -> int:
    ap = argparse.ArgumentParser(description=__doc__.strip().splitlines()[0])
    ap.add_argument("--unit")
    ap.add_argument("--fn", help="substring of the mangled name")
    ap.add_argument("--calibrate", action="store_true",
                    help="rows on 100%% functions - the detector-bug rate")
    ap.add_argument("--control", action="store_true",
                    help="re-prove the detector on the established rows")
    ap.add_argument("--global", dest="only_global", action="store_true",
                    help="only rows whose pointer came out of a global")
    a = ap.parse_args(argv)
    unit = check_unit(a.unit)

    if a.control and not CONTROL:
        print("No HoMM1 live value-temp controls are calibrated yet.")
        return 2
    if a.control:
        bad = 0
        pairs = pairscan.require_pairs({u for u, _s, _o, _w in CONTROL})
        for u, sym, want, sides in CONTROL:
            bobj, tobj = (Obj(p) for p in pairs[u])
            bf, tf = pairscan.functions(bobj), pairscan.functions(tobj)
            if sym not in bf or sym not in tf:
                print(f"MISSING  {u}/{sym}")
                bad += 1
                continue
            b, t = _sides(bobj, tobj, bf, tf, sym)
            b, t = b[0], t[0]
            hit = ("mem", want)
            ok = ((hit in b) == ("b" in sides)) and ((hit in t) == ("t" in sides))
            bad += not ok
            print(f"{'ok  ' if ok else 'FAIL'}  {u:<16} +{want:#x} on "
                  f"{sides or 'neither':<7}  "
                  f"base={_label(sorted(b))!r} target={_label(sorted(t))!r}  {sym}")
        pairs = pairscan.require_pairs({u for u, _s, _o, _p in PROV_CONTROL})
        for u, sym, want, where in PROV_CONTROL:
            bobj, tobj = (Obj(p) for p in pairs[u])
            bf, tf = pairscan.functions(bobj), pairscan.functions(tobj)
            (_b, bp, _be), (_t, tp, _te) = _sides(bobj, tobj, bf, tf, sym)
            hit = ("mem", want)
            ok = bp.get(hit) == where and tp.get(hit) == where
            bad += not ok
            print(f"{'ok  ' if ok else 'FAIL'}  {u:<16} +{want:#x} pointer "
                  f"from {where:<7}  base={bp.get(hit)!r} target={tp.get(hit)!r}"
                  f"  {sym[:44]}")
        print("\nthe detector fires on every established row"
              if not bad else f"\n{bad} control(s) FAILED - the detector is "
                              "measuring nothing; fix it before reading a sweep")
        return 1 if bad else 0

    agree, local, miss, extra, counts = scan(unit, a.fn, a.calibrate)
    if a.calibrate:
        print(f"CALIBRATION over 100.00% rows - every row below is a detector bug")
    hist = ", ".join(f"{n} {k}" for k, n in sorted(agree.items(),
                                                   key=lambda kv: -kv[1]))
    print(f"carrying the temp on BOTH sides at the same member: "
          f"{sum(agree.values())}" + (f"  ({hist})" if hist else ""))
    print(f"local aggregate copies seen (frame offsets - counted, never "
          f"compared): {local}")
    _show("TARGET-ONLY (retail takes the pair by value, we do not)", miss,
          a.only_global)
    _show("BASE-ONLY (we take it by value, retail does not)", extra,
          a.only_global)
    if not a.only_global:
        print(f"\nLOCAL AGGREGATE COPY COUNT differs (frame offsets are not "
              f"comparable, the COUNT is): {len(counts)}")
        for pct, u, sym, be, te in sorted(counts):
            print(f"{pct:7.2f}  {u:<22} base {be} vs target {te:<10} {sym}")
    return 0


if __name__ == "__main__":
    sys.exit(main())

"""homm1.walls.global_refs - the global read-COUNT sieve (the cached-global
bug class).

cl 5.0 re-materialises a global at every use unless the SOURCE hoisted it,
so the number of DIR32 relocations naming a symbol inside one function is a
direct readout of how many times the source mentioned it - invisible to
every masked diff.

  base < target   WE OVER-CACHED (`CFoo* reg = g_x;` collapsed N reads) -
                  the expensive direction (the local eats a callee-saved
                  register for the whole body).
  base > target   we invented a read / retail hoisted one we did not.

WINDOWING IS THE WHOLE TOOL (ported): each side is cut at the NEXT DEFINED
SYMBOL in its own section - never clamp the target to the base's length (it
hides retail's tail). The ported false-positive filters: addend past the
named symbol's extent (the delinker's unsized-datum fallback) dropped;
`___except_list` dropped; self-references (jump tables) dropped; a name only
ONE side references inside the function dropped (this sieve answers "how
many times", never "which symbol" - that is a wrong-referent question).
`--calibrate` = the rows on 100.00% functions, the detector-bug rate.

    homm1 walls global-refs [--unit U] [--fn SUB] [--rel32] [--calibrate]
"""

from __future__ import annotations

from homm1.core.usage import logged

import argparse
import collections

from homm1.delink.coffx import Obj
from homm1.walls import pairscan
from homm1.walls.pairscan import DIR32, REL32, canon

STRUCTURAL = {"___except_list", "__except_list"}


def _extents(obj: Obj) -> dict[str, int]:
    """{canon symbol: bytes it owns} across every section (max per name)."""
    out: dict[str, int] = {}
    for secnum in range(1, obj.nsec + 1):
        members = obj.section_members(secnum)
        size = len(obj.section_payload(secnum)) \
            or obj.section_table[secnum - 1]["size"]
        owners = sorted((v, n) for v, n, _s in members)
        for i, (val, nm) in enumerate(owners):
            end = owners[i + 1][0] if i + 1 < len(owners) else size
            key = canon(nm)
            out[key] = max(out.get(key, 0), end - val)
    return out


def _universe(obj: Obj) -> set[str]:
    names: set[str] = set()
    for secnum in range(1, obj.nsec + 1):
        for _v, n, _s in obj.section_members(secnum):
            names.add(canon(n))
        for name, _typ in obj.typed_relocations(secnum).values():
            names.add(canon(name))
    return names


def _refs(obj, secnum, lo, hi, want, dropped, bounds):
    c: collections.Counter = collections.Counter()
    for _off, nm, ty, addend in pairscan.fn_relocs(obj, secnum, lo, hi):
        if ty != want:
            continue
        nm = canon(nm)
        if addend and addend >= bounds.get(nm, 0):
            dropped["addend past the symbol (unsized-datum fallback)"] += 1
            continue
        c[nm] += 1
    return c


class Row:
    __slots__ = ("unit", "sym", "size", "pct", "over", "under", "self_delta")

    def __init__(self, unit, sym, size, pct, over, under, self_delta):
        self.unit, self.sym, self.size, self.pct = unit, sym, size, pct
        self.over, self.under, self.self_delta = over, under, self_delta

    @property
    def magnitude(self) -> int:
        return sum(self.over.values()) + sum(self.under.values())

    @property
    def rank(self) -> int:
        return self.magnitude * self.size


def scan(unit_filter=None, want=DIR32, keep_self=False, both_sides=False):
    sc, live = pairscan.scores()
    sizes = {}
    rows, seen = [], 0
    dropped: collections.Counter = collections.Counter()
    all_pairs = pairscan.require_pairs({unit_filter} if unit_filter else None)
    for unit in sorted(all_pairs):
        if unit_filter and unit != unit_filter:
            continue
        if live and unit not in live:
            dropped["unit not in report.json (stale normalized pair)"] += 1
            continue
        base, target = all_pairs[unit]
        try:
            bobj, tobj = Obj(base), Obj(target)
        except (ValueError, OSError):
            continue
        bf, tf = pairscan.functions(bobj), pairscan.functions(tobj)
        buniv, tuniv = _universe(bobj), _universe(tobj)
        bext, text_ = _extents(bobj), _extents(tobj)
        for sym in sorted(set(bf) & set(tf)):
            if pairscan.COMPGEN.match(sym):
                dropped["compiler-generated COMDAT"] += 1
                continue
            bsec, bs, be = bf[sym]
            tsec, ts, te = tf[sym]
            b = _refs(bobj, bsec, bs, be, want, dropped, bext)
            t = _refs(tobj, tsec, ts, te, want, dropped, text_)
            for name in [n for n in b if n not in tuniv]:
                dropped["symbol the target never names"] += b.pop(name)
            for name in [n for n in t if n not in buniv]:
                dropped["symbol the base never names"] += t.pop(name)
            for name in STRUCTURAL:
                if b.pop(name, 0) or t.pop(name, 0):
                    dropped["___except_list prologue"] += 1
            self_delta = b.pop(canon(sym), 0) - t.pop(canon(sym), 0)
            if not keep_self and self_delta:
                dropped["self-reference (jump table)"] += 1
            elif keep_self and self_delta:
                (b if self_delta > 0 else t)[canon(sym)] = abs(self_delta)
            seen += 1
            over, under = t - b, b - t
            if not both_sides:
                shared = set(b) & set(t)
                for c in (over, under):
                    for name in [n for n in c if n not in shared]:
                        dropped["one side never names it in THIS fn"] += c.pop(name)
            if not over and not under:
                continue
            pct = sc.get((unit, sym), 0.0)
            size = sizes.get((unit, sym), be - bs)
            rows.append(Row(unit, sym, size, pct, over, under, self_delta))
    rows.sort(key=lambda r: (-r.rank, r.unit, r.sym))
    return rows, seen, dropped


def _fmt(counter) -> str:
    return " ".join(f"{n}{'' if c == 1 else f' x{c}'}"
                    for n, c in sorted(counter.items(),
                                       key=lambda kv: -kv[1])) or "-"


@logged
def main(argv=None) -> int:
    ap = argparse.ArgumentParser(
        prog="homm1 walls global-refs", description=__doc__,
        formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--unit", help="restrict to one unit of config/units.toml")
    ap.add_argument("--fn", help="substring filter on the symbol; OVERRIDES "
                                 "--calibrate and the --min/--max-pct window")
    ap.add_argument("--rel32", action="store_true",
                    help="count REL32 (call) referents instead of DIR32 (data)")
    ap.add_argument("--self", action="store_true",
                    help="keep self-references (jump tables) instead of dropping")
    ap.add_argument("--one-sided", action="store_true",
                    help="keep names only ONE side references in the function")
    ap.add_argument("--calibrate", action="store_true",
                    help="show the rows on 100.00%% functions - the detector-bug "
                         "rate, which must be 0")
    ap.add_argument("--min-pct", type=float, default=0.0,
                    help="score window floor for the listing")
    ap.add_argument("--max-pct", type=float, default=100.0,
                    help="score window ceiling for the listing")
    ap.add_argument("--top", type=int, default=40, help="rows to print")
    args = ap.parse_args(argv)

    from homm1.walls import check_unit
    check_unit(args.unit)
    want = REL32 if args.rel32 else DIR32
    rows, seen, dropped = scan(args.unit, want, getattr(args, "self"),
                               args.one_sided)
    exact = [r for r in rows if r.pct >= 100.0]
    shown = exact if args.calibrate else [
        r for r in rows if args.min_pct <= r.pct <= args.max_pct
        and r.pct < 100.0]
    if args.fn:
        shown = [r for r in rows if args.fn in r.sym]

    print(f"objs: {pairscan.NORM}   reloc: "
          f"{'REL32 (calls)' if args.rel32 else 'DIR32 (data)'}")
    if dropped:
        print("filtered: " + ", ".join(f"{k}={v}"
                                       for k, v in sorted(dropped.items())))
    print(f"paired functions: {seen}   differing: {len(rows)}   "
          f"of those 100%-exact (FALSE POSITIVES): {len(exact)}"
          f"  [{100.0 * len(exact) / max(len(rows), 1):.1f}%]\n")
    for r in shown[:args.top]:
        print(f"{r.pct:6.2f}%  {r.size:>6}B  rank {r.rank:>7}  {r.unit}  {r.sym}")
        if r.over:
            print(f"        base UNDER-reads (we over-cached): {_fmt(r.over)}")
        if r.under:
            print(f"        base OVER-reads  (we invented):    {_fmt(r.under)}")
    if len(shown) > args.top:
        print(f"\n... {len(shown) - args.top} more")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

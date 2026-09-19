"""homm1.sema.match - the objdiff scores for a unit or a function.

    python3 -m homm1.sema.match cimage           # a unit's function table
    python3 -m homm1.sema.match 0x153810         # one function's row
    python3 -m homm1.sema.match CImage::RenderFrame
    python3 -m homm1.sema.match --worst 20       # the lowest-scoring units

A pure read of the compare slice's report (build/objdiff/compare-new/report.json,
falling back to the banked build/objdiff/report.json) - compare OWNS the scores,
sema only joins them to the Model, so a function found by rva is looked up under
the name its winning claim gives it. A unit missing from the report is reported
as missing, never as zero.
"""

from __future__ import annotations

import sys

from homm1 import manifest
from homm1.sema import run
from homm1.sema.index import index, short_name
from homm1.sema.report import REPORTS, report


def _pct(m: dict, key: str = "fuzzy_match_percent") -> float:
    return float(m.get(key) or 0.0)


def unit_table(name: str) -> tuple[list[str], int]:
    rep = report()
    u = rep.unit(name)
    if u is None:
        known = name in {x["unit"] for x in manifest.units()}
        return ([f"unit {name!r} has no row in report.json"
                 + (" (it is in units.toml - build/compare it first)" if known
                    else " and is not in config/units.toml")], 1)
    m = u["measures"]
    out = [f"unit {name}: {_pct(m):.2f}% fuzzy   "
           f"{m.get('matched_functions', 0)}/{m.get('total_functions', 0)} "
           f"functions exact   code {m.get('matched_code', 0)}/"
           f"{m.get('total_code', 0)} B",
           f"{'fuzzy%':>8}  {'size':>6}  symbol"]
    for fn in sorted(u.get("functions", []), key=lambda f: (_pct(f), f["name"])):
        out.append(f"{_pct(fn):>8.2f}  {int(fn.get('size') or 0):>6}  "
                   f"{short_name(fn['name'])}")
    return out, 0


def function_rows(token: str) -> tuple[list[str], int]:
    idx, rep = index(), report()
    hits = idx.resolve_name(token)
    out: list[str] = []
    rc = 1
    names = []
    for rva in hits:
        b = idx.at(rva)
        if b is not None and b.name:
            names.append((b, b.name))
    if not names:
        names = [(None, token)]
    for b, name in names:
        rows = rep.fn_rows(name)
        if not rows:
            out.append(f"{name}: no row in report.json"
                       + (f" (claim unit {b.unit})" if b is not None and b.unit
                          else ""))
            continue
        rc = 0
        for unit, pct in rows:
            where = "" if b is None or unit == b.unit else f"  [scored in {unit}]"
            out.append(f"{pct:>8.2f}%  {short_name(name)}"
                       + (f"  0x{b.rva:08x}" if b is not None else "")
                       + f"  [{unit}]{where}"
                       + ("  (EXACT)" if pct >= 100.0 else ""))
            u = rep.unit(unit)
            if u is not None:
                m = u["measures"]
                out.append(f"           unit {unit}: {_pct(m):.2f}% fuzzy, "
                           f"{m.get('matched_functions', 0)}/"
                           f"{m.get('total_functions', 0)} exact")
    return out, rc


def worst(n: int) -> list[str]:
    rep = report()
    units = sorted(rep.data.get("units", []),
                   key=lambda u: (_pct(u["measures"]), u["name"]))
    out = [f"{'fuzzy%':>8}  {'fns':>4} {'exact':>5}  unit"]
    for u in units[:n]:
        m = u["measures"]
        out.append(f"{_pct(m):>8.2f}  {m.get('total_functions', 0):>4} "
                   f"{m.get('matched_functions', 0):>5}  {u['name']}")
    overall = rep.measures
    out.append(f"[overall {_pct(overall):.2f}% fuzzy over "
               f"{len(units)} unit(s); {overall.get('matched_functions', 0)}/"
               f"{overall.get('total_functions', 0)} functions exact]")
    return out


from homm1.core.usage import logged


@logged
def main(argv: list[str] | None = None) -> int:
    import argparse
    ap = argparse.ArgumentParser(prog="homm1 sema match",
                                 description=__doc__.split("\n\n")[0])
    ap.add_argument("target", nargs="?", help="unit name, rva or function name")
    ap.add_argument("--worst", type=int, nargs="?", const=20,
                    help="the N lowest-scoring units")
    args = ap.parse_args(argv)
    rep = report()
    if not rep.exists:
        print(f"no compare report ({' or '.join(str(p) for p in REPORTS)}) "
              "- run `homm1 compare` first", file=sys.stderr)
        return 2
    if rep.path != REPORTS[0]:
        print(f"[sema] reading the BANKED {rep.path} ({REPORTS[0]} is absent) "
              "- these scores predate the current build", file=sys.stderr)
    if args.worst is not None or not args.target:
        print("\n".join(worst(20 if args.worst is None else args.worst)))
        return 0
    if rep.unit(args.target) is not None or \
            args.target in {u["unit"] for u in manifest.units()}:
        lines, rc = unit_table(args.target)
    else:
        lines, rc = function_rows(args.target)
    print("\n".join(lines))
    return rc


if __name__ == "__main__":
    sys.exit(run(__name__, sys.argv[1:]))

"""homm1.sema.map - the retail address-space map.

    python3 -m homm1.sema.map                     # whole-image overview
    python3 -m homm1.sema.map at 0x153810         # the row here, with neighbours
    python3 -m homm1.sema.map range 0x153000 0x154000
    python3 -m homm1.sema.map gaps --min 64       # unclaimed .text runs
    python3 -m homm1.sema.map units --top 30      # per-unit contribution
    python3 -m homm1.sema.map find CImage::Blit

What lives where, straight off the Model: every admitted census row, whether a
channel claims it and which unit owns it. Two kinds of unclaimed space are
distinguished because they mean different things - a row nothing claims (a
body still to reconstruct) and a HOLE between a claim's stated extent and the
next admitted start (the claim is short, or the census is missing a start).
"""

from __future__ import annotations

import bisect
import sys
from collections import Counter

from homm1.sema import die, parse_rva, run
from homm1.sema.index import index


def row_line(b, idx) -> str:
    return (f"0x{b.rva:08x} 0x{b.size:<6x} {b.space:<5} {b.kind or '-':<7} "
            f"{b.channel or '(unclaimed)':<24} {idx.display(b, b.rva)}"
            + (f"  [{b.unit}]" if b.unit else ""))


def overview() -> list[str]:
    idx = index()
    out = ["retail address-space overview (admitted census rows x Model claims)"]
    for space, rows in (("text", idx.functions),
                        ("data", [b for b in idx.data])):
        total = sum(b.size for b in rows)
        claimed = sum(b.size for b in rows if b.channel)
        out.append(f"\n{space}: {len(rows)} row(s), 0x{total:x} B  "
                   f"claimed 0x{claimed:x} B ({100.0 * claimed / max(total, 1):.1f}%)")
        by_kind = Counter()
        for b in rows:
            by_kind[b.kind or "(plain)"] += b.size
        out.append("  kinds : " + ", ".join(
            f"{k} 0x{v:x}" for k, v in by_kind.most_common()))
        by_ch = Counter()
        for b in rows:
            by_ch[b.channel or "(unclaimed)"] += b.size
        out.append("  channels: " + ", ".join(
            f"{k} 0x{v:x}" for k, v in by_ch.most_common()))
    spaces = Counter()
    for b in idx.data:
        spaces[b.space] += b.size
    out.append("\ndata regions: " + ", ".join(f"{k} 0x{v:x}"
                                              for k, v in spaces.most_common()))
    units = idx.units()
    out.append(f"units claiming rows: {len(units)}")
    holes = gap_runs()
    out.append(f"unclaimed .text: {len(holes)} run(s), "
               f"0x{sum(h[1] - h[0] for h in holes):x} B "
               "(every run, down to 1 B; `sema map gaps --min 1` lists them - "
               "the verb's own default is --min 32)")
    return out


def gap_runs(min_size: int = 1) -> list[tuple[int, int, str]]:
    """Maximal runs of .text no claim covers: [(lo, hi, why)]. `why` separates
    a row nothing claims from a hole between a claim's stated extent and the
    next admitted start (a short claim, or a missing census start)."""
    idx = index()
    if not idx.functions:
        return []
    merged: list[list[int]] = []
    for b in idx.functions:
        if not b.channel:
            continue
        if merged and b.rva <= merged[-1][1]:
            merged[-1][1] = max(merged[-1][1], b.rva + b.size)
        else:
            merged.append([b.rva, b.rva + b.size])
    lo_edge = idx.functions[0].rva
    hi_edge = idx.functions[-1].rva + idx.functions[-1].size
    runs, cursor = [], lo_edge
    for lo, hi in merged:
        if cursor < lo:
            runs.append((cursor, lo))
        cursor = max(cursor, hi)
    if cursor < hi_edge:
        runs.append((cursor, hi_edge))
    out = []
    for lo, hi in runs:
        if hi - lo < min_size:
            continue
        row = idx.func(lo)
        why = ("unclaimed row(s)" if row is not None and not row.channel
               else "hole past a claim's extent")
        i = bisect.bisect_left(idx.fstarts, lo)
        j = bisect.bisect_left(idx.fstarts, hi)
        kinds = {b.kind or "plain" for b in idx.functions[i:j]}
        if kinds:
            why += " [" + ",".join(sorted(kinds)) + "]"
        out.append((lo, hi, why))
    return out


def at(rva: int, window: int = 3) -> tuple[list[str], int]:
    """(lines, rc) - rc 1 is the answered-NO case: no admitted row covers it."""
    idx = index()
    b = idx.covering(rva)
    out = []
    if b is None:
        from homm1.sema.image import retail
        sec = retail().section_of(rva)
        out.append(f"0x{rva:08x}: no admitted row covers this address"
                   + (f" (section {sec['name']})" if sec
                      else " (outside every section of the image)"))
        prev = idx.preceding_func(rva)
        if prev is not None:
            out.append(f"  previous row: {row_line(prev, idx)} "
                       f"(ends 0x{prev.rva + prev.size:08x})")
        return out, 1
    rows = idx.functions if b.space == "text" else idx.data
    i = rows.index(b)
    for j in range(max(0, i - window), min(len(rows), i + window + 1)):
        mark = " <==" if j == i else "    "
        out.append(mark + " " + row_line(rows[j], idx))
    if b.rva != rva:
        out.append(f"[0x{rva:08x} is +0x{rva - b.rva:x} into the marked row]")
    return out, 0


def rows_in(lo: int, hi: int) -> list[str]:
    idx = index()
    out = [row_line(b, idx) for b in idx.functions + idx.data
           if lo <= b.rva < hi]
    out.append(f"[{len(out)} row(s) in 0x{lo:08x}..0x{hi:08x}]")
    return out


def units_table(top: int) -> list[str]:
    idx = index()
    rows = []
    for unit, bindings in idx.units().items():
        text = sum(b.size for b in bindings if b.space == "text")
        data = sum(b.size for b in bindings if b.space != "text")
        lo = min(b.rva for b in bindings)
        hi = max(b.rva + b.size for b in bindings if b.space == "text") \
            if any(b.space == "text" for b in bindings) else lo
        rows.append((text, data, len(bindings), unit, lo, hi))
    rows.sort(reverse=True)
    out = [f"{'text B':>8} {'data B':>7} {'rows':>5}  unit                 span"]
    for text, data, n, unit, lo, hi in rows[:top]:
        out.append(f"{text:>8} {data:>7} {n:>5}  {unit:<20} "
                   f"0x{lo:06x}..0x{hi:06x}")
    out.append(f"[{len(rows)} unit(s) claim rows]")
    return out


def find(pattern: str, limit: int = 100) -> tuple[list[str], int]:
    idx = index()
    low = pattern.lower()
    hits = [b for b in idx.functions + idx.data
            if b.name and (low in b.name.lower()
                           or low in idx.display(b, b.rva).lower())]
    out = [row_line(b, idx) for b in hits[:limit]]
    if len(hits) > limit:
        out.append(f"... (+{len(hits) - limit} more)")
    out.append(f"[{len(hits)} binding(s) match {pattern!r}]")
    return out, (0 if hits else 1)


from homm1.core.usage import logged


@logged
def main(argv: list[str] | None = None) -> int:
    import argparse
    ap = argparse.ArgumentParser(prog="homm1 sema map",
                                 description=__doc__.split("\n\n")[0])
    ap.add_argument("rest", nargs="*", help="at <rva> | range <lo> [hi] | gaps | "
                                            "units | find <pattern>")
    ap.add_argument("--min", type=int, default=32, help="gaps: smallest run")
    ap.add_argument("--top", type=int, default=40, help="units: rows to show")
    args = ap.parse_args(argv)
    rest = list(args.rest)
    if not rest:
        print("\n".join(overview()))
        return 0
    verb, rest = rest[0], rest[1:]
    idx = index()
    if verb == "at":
        if not rest:
            die("map at <rva>")
        lines, rc = at(parse_rva(rest[0]))
        print("\n".join(lines))
        return rc
    if verb == "range":
        if not rest:
            die("map range <lo> [<hi>]  (or <lo>-<hi>)")
        if "-" in rest[0] and len(rest) == 1:
            lo, hi = (parse_rva(x) for x in rest[0].split("-", 1))
        else:
            lo = parse_rva(rest[0])
            hi = parse_rva(rest[1]) if len(rest) > 1 else lo + 0x1000
        print("\n".join(rows_in(lo, hi)))
        return 0
    if verb == "gaps":
        runs = gap_runs(args.min)
        for lo, hi, why in runs:
            near = idx.preceding_func(lo)
            out = f"0x{lo:08x}..0x{hi:08x}  0x{hi - lo:>6x} B  {why}"
            if near is not None:
                out += f"   after {idx.display(near, near.rva)}"
            print(out)
        print(f"[{len(runs)} run(s) >= {args.min} B, "
              f"0x{sum(h[1] - h[0] for h in runs):x} B total]")
        return 0
    if verb == "units":
        print("\n".join(units_table(args.top)))
        return 0
    if verb == "find":
        if not rest:
            die("map find <pattern>")
        lines, rc = find(rest[0])
        print("\n".join(lines))
        return rc
    die(f"unknown map verb {verb!r} (at | range | gaps | units | find)")
    return 2


if __name__ == "__main__":
    sys.exit(run(__name__, sys.argv[1:]))

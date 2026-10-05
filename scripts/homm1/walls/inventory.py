"""homm1.walls.inventory - the wall worklist, derived, never hand-kept.

    homm1 walls inventory [--unit U] [--below PCT] [--todo]
                           [--json] [--limit N]

A wall is a function scoring below 100% fuzzy in the CURRENT compare report.
The worklist is a JOIN of three read-only inputs and nothing else:

  * build/objdiff/compare-new/report.json (falling back to
    build/objdiff/report.json) - the current per-function scores;
  * the Model (homm1.model.resolve) - rva/unit/size/channel per name;
  * config/match_baseline.tsv - best_pct (historical MAX) + src_hash per rva.

Campaign order (AGENTS.md): ascending historical MAX - the lowest bank is
the biggest structural question. cur < best is a REGRESSION flag, not a
wall class; best == 100 with cur < 100 means the implementation already
proved the body and the current dip is TU-state, not structure.

HEADROOM IS `hist` MINUS THE BANK, NEVER `hist` MINUS `cur`, and the two
readings disagree on most rows that look like the biggest prizes in the queue.
`cur` below the BANK is this exact source scoring lower than it already has,
which is TU composition and no action. The BANK below `hist` is a source EDIT
that gave the peak up, so some earlier IMPLEMENTATION reached a mark nothing
in the tree now reproduces - that, and only that, is a question. Measured
2026-08-23 over the 578-row todo queue: 66 rows have hist above cur and 20 of
them are the first kind. `CGruntSelectedSprite::Update` reads as the biggest
opportunity in the whole queue at cur 84.85 against hist 99.24, and its bank
is also 99.24 - that source already reached the peak and there is nothing to
do. The `L` flag marks the 46 that are real; the deepest is
`CDDrawSurfaceMgr::SnapshotChildren`, banked at 70.12 against a 77.52 peak.

An `L` row is a question, not a promise, so read `walls priors` before working
one: some of these peaks were given up DELIBERATELY, because the shape that
scored them is refuted by retail's own bytes. `CSBI_ImageSet::SetupImage`
(bank 68.31, hist 74.63) is exactly that - the 74.63 spelling tested `owner`
before `host`, and retail tests `[esp+0xc]`, the second parameter, first.
Recovering that number would mean re-introducing a wrong guard order.

The report also scores the carved EH band (`__ehreg$*` / `__ehunwind$*`),
which homm1.verify.scores excludes from the gate because those funclets are
not reconstruction targets. They are KEPT here (they are real sub-100 rows)
but counted separately in the header, so the worklist total is never read as
a body count.

``--todo`` is Codex's explicit campaign queue.  It removes EH-band funclets,
functions already proven exact historically, and only those functions Codex
personally recorded as ``bounded`` or ``exact`` at the current source hash.
Inherited ``@early-stop`` markers do not affect it.  Hash-valid ``open``
reviews remain in the queue with their recorded class and next evidence-bearing
action.
"""

from __future__ import annotations

from homm1.core.usage import logged

import json

from homm1.core.paths import BUILD, IMAGE_BUILD, REPO
from homm1.verify.scores import is_eh_band

BASELINE = REPO / "config/match_baseline.tsv"
REPORTS = (IMAGE_BUILD / "objdiff/compare-new/report.json",
           IMAGE_BUILD / "objdiff/report.json")


def report_scores() -> tuple[str, dict[tuple[str, str], float]]:
    """(report path used, {(unit, symbol): fuzzy%})."""
    for path in REPORTS:
        if path.is_file():
            doc = json.loads(path.read_text())
            out = {}
            for u in doc.get("units", []):
                uname = u["name"]
                for f in u.get("functions", []):
                    out[(uname, f["name"])] = float(
                        f.get("fuzzy_match_percent") or 0.0)
            return str(path), out
    raise SystemExit("[walls] no report.json - run `homm1 compare` first")


EPS = 0.01   # the report's raw float vs the 4-decimal stored best: strict `<`
#              flags pure quantization jitter (352 rows measured) as regression


def baseline_rows() -> dict[int, tuple[float, float, str]]:
    """{rva: (best_pct, hist_pct, src_hash)} from the baseline's function
    rows: unit function best_pct cur_pct tries src_hash rva hist_pct state.
    hist is the cross-hash historical MAX (campaign order); best is the
    current implementation's bank (the regression gate)."""
    out: dict[int, tuple[float, float, str]] = {}
    if not BASELINE.is_file():
        return out
    for line in BASELINE.read_text().splitlines():
        if line.startswith("#") or not line.strip():
            continue
        cols = line.split("\t")
        if len(cols) >= 7 and cols[6].startswith("0x"):
            try:
                best = float(cols[2])
                hist = float(cols[7]) if len(cols) >= 8 and cols[7] else best
                out[int(cols[6], 16)] = (best, hist, cols[5])
            except ValueError:
                continue
    return out


def build(
    unit: str | None = None,
    below: float = 100.0,
    todo: bool = False,
) -> list[dict]:
    from homm1.model import resolve
    from homm1.walls.reviews import TERMINAL_STATUSES, current as current_reviews
    _path, scores = report_scores()
    best = baseline_rows()
    reviews = current_reviews() if todo else {}
    by_name: dict[tuple[str, str], object] = {}
    for b in resolve().functions:
        if b.name:
            by_name[(b.unit, b.name)] = b
    rows = []
    for (u, sym), pct in scores.items():
        if pct >= below or (unit and u != unit):
            continue
        b = by_name.get((u, sym))
        rva = b.rva if b else None
        bank, hist, src_hash = best.get(rva, (None, None, ""))
        review = reviews.get(rva)
        if todo and (
            is_eh_band(sym)
            or hist == 100.0
            or (review is not None and review["status"] in TERMINAL_STATUSES)
        ):
            continue
        rows.append({
            "rva": f"0x{rva:06x}" if rva is not None else "",
            "unit": u, "symbol": sym, "cur": pct,
            "hist_max": hist, "size": f"0x{b.size:x}" if b else "",
            "bank": bank,
            # `hist` minus the BANK, not minus `cur`: the difference between
            # the two is the difference between a question and a non-question.
            # cur < bank is this source scoring lower than it already has,
            # which is TU composition and no action; bank < hist is a source
            # EDIT that gave the peak up, so some earlier implementation
            # reached a mark nothing in the tree reproduces. Only the second
            # is headroom.
            "lost": (round(hist - bank, 4)
                     if bank is not None and hist is not None
                     and bank < hist - EPS else 0.0),
            "regressed": bank is not None and pct < bank - EPS,
            "proven": hist == 100.0,
            "review_status": review["status"] if review else "",
            "review_class": review["wall_class"] if review else "",
            "review_evidence": review["evidence"] if review else "",
        })
    # ascending historical MAX, unknowns last, then ascending current
    rows.sort(key=lambda r: (r["hist_max"] is None,
                             r["hist_max"] if r["hist_max"] is not None else 0,
                             r["cur"]))
    return rows


@logged
def main(argv=None) -> int:
    import argparse
    ap = argparse.ArgumentParser(
        prog="homm1 walls inventory", description=__doc__,
        formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--unit", help="restrict to one unit of config/units.toml")
    ap.add_argument("--below", type=float, default=100.0,
                    help="score ceiling for a row to count as a wall")
    ap.add_argument("--limit", type=int, default=40, help="rows to print")
    ap.add_argument(
        "--todo",
        action="store_true",
        help="exclude proven rows and Codex-bounded/exact reviews at this source hash",
    )
    ap.add_argument("--json", action="store_true", help="the rows as JSON")
    a = ap.parse_args(argv)
    from homm1.walls import check_unit
    check_unit(a.unit)
    rows = build(a.unit, a.below, a.todo)
    excluded = build(a.unit, a.below, False) if a.todo else []
    if a.json:
        print(json.dumps(rows, indent=2))
        return 0
    n_reg = sum(r["regressed"] for r in rows)
    n_prov = sum(r["proven"] for r in rows)
    n_eh = sum(1 for r in rows if is_eh_band(r["symbol"]))
    lost = [r for r in rows if r["lost"]]
    queue = " todo" if a.todo else ""
    print(f"[walls] {len(rows)}{queue} function(s) below {a.below:g}%  "
          f"({n_prov} proven-at-100 dips, {n_reg} below their bank"
          + (f", {n_eh} EH-band funclets - scored, NOT reconstruction targets"
             if n_eh else "") + ")")
    if a.todo:
        # The queue is NOT the complete sub-100 set: say what it dropped, so no
        # reader mistakes it for one. A terminal review is reviewer progress,
        # NOT proof a reconstruction is correct - a "closed" row has since been
        # taken to 100.00 EXACT by a later lane.
        shown = {r["rva"] for r in rows}
        gone = [r for r in excluded if r["rva"] not in shown
                and not is_eh_band(r["symbol"])]
        from homm1.walls.reviews import TERMINAL_STATUSES, current as _cur
        _rv = _cur()
        n_term = sum(1 for r in gone if r["rva"]
                     and (_rv.get(int(r["rva"], 16)) or {}).get("status")
                     in TERMINAL_STATUSES)
        n_p100 = sum(1 for r in gone if r["proven"])
        if gone:
            print(f"        EXCLUDED from this queue: {len(gone)} sub-100 row(s) "
                  f"- {n_term} carry a terminal review, {n_p100} already reached "
                  f"100 once. Neither is proof; `--below 100` lists them.")
    print(f"        {len(lost)} row(s) carry LOST headroom (L): the bank sits "
          f"below hist, so a source EDIT gave up a peak nothing in the tree "
          f"reproduces.")
    print(f"        A row whose hist is above its CUR but not above its BANK "
          f"is not one of them - that source already reached hist and the "
          f"dip is TU composition.")
    print(f"        Read `walls priors` before working an L row: some peaks "
          f"were given up deliberately, because the shape that scored them is "
          f"refuted by retail's bytes.")
    print(f"{'rva':>10}  {'hist':>6}  {'bank':>6}  {'cur':>6}  {'size':>7}  "
          f"unit/symbol")
    for r in rows[:a.limit]:
        hist = f"{r['hist_max']:6.2f}" if r["hist_max"] is not None else "     ?"
        bank = f"{r['bank']:6.2f}" if r["bank"] is not None else "     ?"
        flag = " L" if r["lost"] else (
            " R" if r["regressed"] else ("  " if not r["proven"] else " P"))
        review = ""
        if a.todo and r["review_status"]:
            review = f" [{r['review_status']}/{r['review_class']}]"
        print(f"{r['rva']:>10}  {hist}  {bank}  {r['cur']:6.2f}  "
              f"{r['size']:>7}{flag} {r['unit']}/{r['symbol'][:60]}{review}")
    if len(rows) > a.limit:
        print(f"  ... {len(rows) - a.limit} more (--limit)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

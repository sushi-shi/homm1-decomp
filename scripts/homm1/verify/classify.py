"""homm1.verify.classify - the rva-keyed regression computation.

IDENTITY IS THE RVA, NOT THE NAME OR THE UNIT. A best-ever measures a BODY;
the name is only our label for it, and labels get reassigned (a fold renames a
method, a COMDAT re-home moves it between units). The Model
(homm1.model.resolve) is the one rva/unit/name join - no label re-joins here.

Bucket doctrine (ported):
  * BELOW-BEST IS TESTED FIRST - an edited function's drop below its own
    high-water is still a REGRESS; TOUCHED means "edited and NOT below best".
  * a vanished (unit, fn) whose rva is scored under a new key is MOVED (cross
    unit) or RENAMED (in place), gated against the same body's best there.
  * a vanished row whose rva is no longer scored at all is LOST - unless its
    baseline row is already banked `state=absent` (KNOWN-ABSENT: the loss was
    adjudicated and banked; the MAX is preserved for a later emitter).
  * REGRESS splits into carried (the committed snapshot ALREADY had the row
    below its best - inherited debt) and fresh (new since the snapshot - the
    set a lane is actually answerable for).
"""

from __future__ import annotations

from homm1.verify.baseline import EPS, below_best
from homm1.verify.fingerprints import real_edit


def model_rvas() -> dict[tuple[str, str], int]:
    """{(unit, mangled): retail rva} from the Model's function bindings."""
    from homm1.model import resolve
    out: dict[tuple[str, str], int] = {}
    for b in resolve().functions:
        if b.name and b.unit:
            # objdiff reports the object basename even when config keeps the
            # donor directory (SOURCE/KB). Use the same identity convention as
            # scores.functions; duplicate basenames are rejected by the model's
            # unique-name/TU gates rather than guessed here.
            out.setdefault((b.unit.rsplit("/", 1)[-1], b.name), b.rva)
    return out


def index_by_rva(rows: dict[tuple[str, str], dict]) -> dict[int, tuple[str, str]]:
    """{rva: key} over rows whose rva is UNIQUE. Non-unique addresses are
    dropped rather than guessed at: carrying a high-water onto the wrong body
    is exactly the erosion the ratchet exists to prevent."""
    by: dict[int, tuple[str, str]] = {}
    dup: set[int] = set()
    for key, row in rows.items():
        addr = row.get("addr")
        if addr is None:
            continue
        if addr in by:
            dup.add(addr)
        else:
            by[addr] = key
    for addr in dup:
        by.pop(addr, None)
    return by


def rebound(prev_addr: int | None, cur_addr: int | None) -> bool:
    """True when this NAME now labels a DIFFERENT BODY (its rva moved).

    Unknown on either side returns False = "same body" DELIBERATELY: that
    keeps the ratchet, so an unvouchable row can only over-report a regression
    (loud, cheap) instead of silently eroding a peak (invisible, costly).
    Never use the source fingerprint for this - it answers "did the TEXT
    change", a different question."""
    return prev_addr is not None and cur_addr is not None \
        and prev_addr != cur_addr


def migrations(cur: dict, base_funcs: dict,
               rvas: dict) -> dict[tuple[str, str], tuple[str, str]]:
    """{old_key: new_key}: baseline rows whose BODY (rva) is scored under a
    new key that has no baseline row of its own - the high-water travels."""
    base_by_addr = index_by_rva(base_funcs)
    out: dict[tuple[str, str], tuple[str, str]] = {}
    taken: set[tuple[str, str]] = set()
    for key in cur:
        if key in base_funcs:
            continue
        addr = rvas.get(key)
        old = base_by_addr.get(addr) if addr is not None else None
        if old is not None and old != key and old not in cur \
                and old not in taken:
            out[old] = key
            taken.add(old)
    return out


def classify(cur: dict, base_funcs: dict, fp, rvas: dict):
    """Yield (kind, unit, fn, cur_pct, best_pct) for every interesting delta."""
    migrated = migrations(cur, base_funcs, rvas)
    moved_from = {new: old for old, new in migrated.items()}
    here = {addr for key, addr in rvas.items()
            if key in cur and addr is not None}

    for key, pct in sorted(cur.items()):
        unit, fn = key
        prev = base_funcs.get(key)
        old_key = moved_from.get(key)
        if prev is None and old_key is not None:
            prev = base_funcs[old_key]
        if prev is None:
            yield ("NEW", unit, fn, pct, None)
            continue
        if old_key is not None:          # informational; the gating is below
            yield ("MOVED" if old_key[0] != unit else "RENAMED",
                   unit, fn, pct, prev["best"])
        if below_best(pct, prev["best"]):  # BELOW-BEST FIRST
            yield ("REGRESS", unit, fn, pct, prev["best"])
        elif real_edit(prev["fp"], fp(*key)):
            yield ("TOUCHED", unit, fn, pct, prev["best"])
        elif pct > prev["best"] + EPS:
            yield ("IMPROVE", unit, fn, pct, prev["best"])

    for key, prev in sorted(base_funcs.items()):
        if key in cur or key in migrated:
            continue
        unit, fn = key
        addr = prev.get("addr")
        # vanished: a rename (rva scored ANYWHERE) or a real source edit ->
        # REMOVED; banked state=absent -> KNOWN-ABSENT (adjudicated at a
        # bank; the MAX is preserved); otherwise a genuine LOST.
        renamed = addr is not None and addr in here
        if renamed or real_edit(prev["fp"], fp(*key)):
            yield ("REMOVED", unit, fn, None, prev["best"])
        elif prev.get("state") == "absent":
            yield ("KNOWN-ABSENT", unit, fn, None, prev["best"])
        else:
            yield ("LOST", unit, fn, None, prev["best"])


def buckets_of(cur, base_funcs, fp, rvas) -> dict[str, list]:
    buckets: dict[str, list] = {}
    for kind, unit, fn, pct, best in classify(cur, base_funcs, fp, rvas):
        buckets.setdefault(kind, []).append((unit, fn, pct, best))
    return buckets


def currency(cur, base_funcs, regress) -> dict:
    """How far the committed snapshot is from this build, and which reported
    dips it already contained (carried = inherited debt, not this lane's;
    fresh = new since the snapshot - the set a lane is answerable for)."""
    drift = sum(1 for key, prev in base_funcs.items()
                if key in cur and abs(cur[key] - prev["cur"]) > EPS)
    carried = fresh = 0
    for unit, fn, _pct, _best in regress:
        prev = base_funcs.get((unit, fn))
        if prev is None:                 # a MOVED row, gated at its new home
            fresh += 1
        elif below_best(prev["cur"], prev["best"]):
            carried += 1
        else:
            fresh += 1
    return {"snapshot_drift": drift, "compared_rows": len(cur),
            "regress_carried": carried, "regress_fresh": fresh}


def fresh_regressions(cur, base_funcs, regress) -> list:
    out = []
    for unit, fn, pct, best in regress:
        prev = base_funcs.get((unit, fn))
        if prev is None or not below_best(prev["cur"], prev["best"]):
            out.append((unit, fn, pct, best))
    return out

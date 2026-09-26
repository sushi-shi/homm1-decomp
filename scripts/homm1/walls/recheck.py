"""homm1.walls recheck - re-run a review's certifications against today's pair.

A review that says "base/retail agree on 24 calls, 96 branches and 64 relocs" is
not prose: it is a checkable assertion about the normalized pair.  Nothing
re-ran it.  `CTriggerMgr::PlaceObjectFull` carried exactly that certification
while the base had drifted to 25/95/65 - a later commit traded one cross-jump
for another, the score ROSE, and no source reader could see it because the
review's own numbers were never measured again.

This re-measures every count a review asserts and prints HOLD or BROKEN per
claim.  It reads the same normalized base/target objs `homm1 walls diagnose`
reads, so a BROKEN row is a live divergence, not a stale cache.

The recorded wall CLASS is re-run too, through `diagnose.ladder` itself so the
sweep and the single-row view cannot answer differently.  It is scored
SEPARATELY from the counts and a disagreement is not a failure: the review names
the CAUSE it traced, the ladder names the FIRST divergence, and on this ledger
that vocabulary gap accounts for most disagreements (a regalloc cause whose
branch delta is downstream reads `cfg` here).  A STALE review whose class AND
counts both still hold is a verdict the edit did not invalidate.

    homm1 walls recheck                 every review
    homm1 walls recheck <rva|name>...   selected rows
    homm1 walls recheck --source        the SOURCE notes instead of the ledger
    homm1 walls recheck --broken        only rows with a failed count claim
    homm1 walls recheck --strict        exit 1 when any count claim is broken

There are TWO stores of written verdicts, and `homm1 walls priors` already
reads both: the review ledger and the `//` block above each `RVA()` pin.  Only
the ledger was ever re-measured.  `--source` runs the SAME extractor and the
SAME measurement over the source notes, because a note that says "the frame is
retail's 0x7c" or "313 conditional branches and 1 ret on BOTH sides" is exactly
as checkable, and exactly as unre-run, as a ledger row.  Measured on the first
sweep, 2026-08-23: nine notes stated something the pair no longer holds, four of
them describing a FRAME difference that later work had already closed - a reader
would have gone hunting for a frame that already matches.

`gate_findings` is the same sweep as the `review-claims` row of `homm1 verify
check --tier normal`, so every build re-measures BOTH stores.  Only a BROKEN
claim - the two sides stopped agreeing - reaches the gate; a DRIFT, where they
still agree but not at the stated number, is a retired RULER and is reported by
the verb only.  It belongs in a
tier because it is the drift the MAX gate cannot see: the MAX gate watches the
SCORE, and the commit that broke `PlaceObjectFull`'s certification raised it.

Only AGREEMENT claims are extracted, and deliberately so.  "base 59/retail 43
calls" states a known divergence whose direction depends on the writer's word
order; asserting it back would test the parser, not the tree.  An agreement
claim has one reading: both sides hold N.  Two spellings carry it -

    N/N <unit>                     "2/2 calls", "4/4 DispatchMove calls"
    <agreement trigger> ... N <unit>   "Base/retail agree on 18 calls, 1 return"

- and a sentence that also carries a divergence marker (vs, versus, against,
  differ, while, whereas, short, only) is skipped whole, because those sentences
  mix both kinds and the parser cannot tell which number belongs to which.

`referents` is NOT checked: a review's "ordered referents" is the semdiff
referent SEQUENCE, which is shorter than the relocation count (PlaceObjectFull:
43 referents against 65 relocs), so scoring it against `relocs` would invent a
failure.  Unmatched quantities are reported as such rather than guessed.
"""

from __future__ import annotations

from homm1.core.usage import logged

from collections import Counter
import re

from homm1.delink.coffx import Obj

# The quantities this tool can measure from the normalized pair.  `returns` and
# `rets` are one quantity; `instructions` and `insns` are one quantity.
_UNIT = {
    "call": "calls", "calls": "calls",
    "branch": "branches", "branches": "branches",
    "return": "returns", "returns": "returns", "ret": "returns", "rets": "returns",
    "reloc": "relocs", "relocs": "relocs",
    "relocation": "relocs", "relocations": "relocs",
    "instruction": "insns", "instructions": "insns",
    "insn": "insns", "insns": "insns",
}
_UNIT_RE = "|".join(sorted(_UNIT, key=len, reverse=True))

# "4/4 DispatchMove calls" - one qualifier may sit between the pair and the unit.
# It must not be a connective: `returns 2/2 and relocs 249/249` otherwise reads
# as "relocs 2", which is how the first pass invented two failures out of two
# holds (0x065e80, 0x0f42f0).  Both word orders occur - `22/22 calls` and
# `Calls 31/31` - and the reverse spelling is where `ordered relocs 312/312`
# lives, so both are matched.
_STOP = frozenset(
    "and or the a an plus with of in at on to versus vs but then also each both "
    "all only now still exact exactly are is was were has have had its their "
    "than from for".split()
)
_PAIR = re.compile(
    rf"(?<![\d.])(?P<a>\d+)\s*/\s*(?P<b>\d+)\s+(?P<qual>[A-Za-z_][\w:]*\s+)?"
    rf"(?P<unit>{_UNIT_RE})\b",
    re.I,
)
_PAIR_REV = re.compile(
    rf"\b(?P<unit>{_UNIT_RE})\s+(?P<a>\d+)\s*/\s*(?P<b>\d+)(?!\d)(?!\.\d)", re.I
)
_COUNT = re.compile(rf"(?<![\d./x])(\d+)\s+(?:ordered\s+)?({_UNIT_RE})\b", re.I)
_EXTENT = re.compile(r"(?<![\w.])(?:0x([0-9a-f]+))\s*(?:bytes?|B\b|extent)", re.I)

_AGREE = re.compile(
    r"\b(agree|agrees|agreed|identical|exact|exactly|both|same|match|matches)\b", re.I
)
_DIVERGE = re.compile(
    r"\b(vs|versus|against|differ|differs|differing|while|whereas|short|only|"
    r"instead|prior|banked|historical|recorded|fell|falls|rose|raising|"
    r"delta|deltas|because|though|although|but|residue|residual|extra|missing|"
    r"ahead|behind|costs?|gains?)\b",
    re.I,
)
# A clause measuring two SOURCE CANDIDATES against each other is not a claim
# about base against retail, and its "both" means the two spellings.
# `InitFromSurface` reads "...both compile byte-identically at 77.50 with 30
# instructions, 3 branches and 2 returns" while that same review states retail
# has 3 returns to the base's 2; `DrawWrapped` states 59/59 calls measured under
# a DISPOSABLE inline_depth(0) probe that the tree does not carry.  Neither is a
# certification of the committed pair, so the clause is skipped whole.
_CANDIDATE = re.compile(
    r"\b(compiles?|compiled|byte-identical(?:ly)?|byte-flat|variants?|controls?|"
    r"campaigns?|islands?|trials?|disposable|probes?|experiments?|candidates?|"
    r"rejected|tested)\b",
    re.I,
)


def _sentences(text: str) -> list[str]:
    """Split into clauses on `. ` and `; `, never inside a hex literal or a
    decimal score.  The semicolon matters: the certification that drifted was
    written `...agree at 24 calls, 96 branches, and 64 relocs; base has 15 vs
    retail 16 returns because...` - one sentence carrying both an agreement and
    a divergence.  Splitting only on `.` reads the `vs` and discards the whole
    assertion, which is how the known positive escaped the first pass."""
    out, buf = [], []
    for i, ch in enumerate(text):
        buf.append(ch)
        if ch in ".;" and (i + 1 >= len(text) or text[i + 1] in " \t"):
            out.append("".join(buf))
            buf = []
    if buf:
        out.append("".join(buf))
    return out


def claims(evidence: str) -> tuple[list[tuple[str, int, str]], list[str]]:
    """([(unit, n, sentence)], [skipped sentence]) - the agreement assertions.

    A claim means BOTH sides were certified at `n`.  Sentences that mix an
    agreement with a divergence are skipped and returned so the caller can say
    what it declined to read."""
    found: list[tuple[str, int, str]] = []
    skipped: list[str] = []
    for s in _sentences(evidence):
        if _CANDIDATE.search(s):
            if _COUNT.search(s) or _PAIR.search(s) or _PAIR_REV.search(s):
                skipped.append(s.strip())
            continue
        pairs = []
        for m in _PAIR.finditer(s):
            qual = (m.group("qual") or "").strip().lower()
            if qual in _STOP:
                continue
            pairs.append((m, _UNIT[m.group("unit").lower()]))
        pairs += [(m, _UNIT[m.group("unit").lower()]) for m in _PAIR_REV.finditer(s)]
        equal = [(u, int(m.group("a")), s)
                 for m, u in pairs if m.group("a") == m.group("b")]
        found.extend(equal)
        if not _AGREE.search(s):
            continue
        if _DIVERGE.search(s):
            if not equal and _COUNT.search(s):
                skipped.append(s.strip())
            continue
        spans = [m.span() for m, _u in pairs]
        for m in _COUNT.finditer(s):
            if any(a <= m.start() < b for a, b in spans):
                continue  # already read as the N/N form
            found.append((_UNIT[m.group(2).lower()], int(m.group(1)), s))
        for m in _EXTENT.finditer(s):
            found.append(("bytes", int(m.group(1), 16), s))
    # de-duplicate: one sentence may state the same quantity twice
    seen, uniq = set(), []
    for unit, n, s in found:
        if (unit, n) in seen:
            continue
        seen.add((unit, n))
        uniq.append((unit, n, s))
    return uniq, skipped


def measure(binding) -> tuple[dict[str, tuple[int, int]], str] | str:
    """({unit: (base, target)}, wall class) from the pair, or an error string.

    The wall class comes from `diagnose.ladder`, the same function that prints,
    so the sweep and the single-row view can never answer differently."""
    from collections import Counter

    from homm1.tool import ToolError
    from homm1.walls.diagnose import (
        NORM, _call_targets, _find_function, _jump_table_bytes, _referents,
        _skeleton, ladder,
    )

    base_p = NORM / "base" / f"{binding.unit}.obj"
    tgt_p = next(
        (p for p in (NORM / "target" / f"{binding.unit}.c.obj",
                     NORM / "target" / f"{binding.unit}.obj") if p.is_file()),
        None,
    )
    if not base_p.is_file() or tgt_p is None:
        return f"normalized pair missing for {binding.unit}"
    side, extra = {}, {}
    for tag, path in (("base", base_p), ("target", tgt_p)):
        payload, rel, size = _find_function(Obj(path), binding.name)
        if payload is None:
            return f"{tag} obj does not define {binding.name}"
        try:
            mask, calls, br, rets, insns, asm = _skeleton(
                payload, rel, data=_jump_table_bytes(rel, binding.name)
            )
        except ToolError as e:
            return str(e)
        side[tag] = {"calls": calls, "branches": br, "returns": rets,
                     "insns": insns, "relocs": len(rel), "bytes": size}
        extra[tag] = (mask, _referents(rel),
                      Counter(n for n, _a in _call_targets(rel, asm, binding.name)))
    wall = ladder(extra["base"][0], extra["target"][0],
                  extra["base"][1], extra["target"][1],
                  extra["base"][2], extra["target"][2],
                  (side["base"]["branches"], side["base"]["returns"],
                   side["target"]["branches"], side["target"]["returns"]))
    return {u: (side["base"][u], side["target"][u]) for u in side["base"]}, wall


def sweep(wanted: set[int] | None = None) -> list[dict]:
    """One record per review row: the re-measured verdicts, in rva order.

    The printer and the build gate both consume this, so the two can never
    answer differently - the same reason `measure` calls `diagnose.ladder`
    instead of reimplementing the class ladder."""
    from homm1.walls import reviews
    from homm1.walls.diagnose import _locate, named_functions

    rows = reviews.load()
    fresh = set(reviews.current())
    named = named_functions()
    out: list[dict] = []
    for rva in sorted(rows):
        if wanted and rva not in wanted:
            continue
        row = rows[rva]
        stated, skipped = claims(row["evidence"])
        rec = {
            "rva": rva, "row": row, "stated": stated, "skipped": skipped,
            "fresh": rva in fresh, "binding": None, "verdicts": [],
            "wall": None, "error": None,
        }
        b, why = _locate(f"0x{rva:x}", named)
        if b is None:
            rec["error"] = ("unresolved", why)
            out.append(rec)
            continue
        rec["binding"] = b
        got = measure(b)
        if isinstance(got, str):
            rec["error"] = ("unmeasured", got)
            out.append(rec)
            continue
        counts, wall = got
        rec["wall"] = wall
        rec["verdicts"] = [(counts[u][0] == n == counts[u][1], u, n, *counts[u])
                           for u, n, _s in stated]
        out.append(rec)
    return out


def _verdict(stated: int, base: int, target: int) -> str:
    """HOLD / DRIFT / BROKEN - and DRIFT is the distinction that matters.

    A note whose number no longer measures is not automatically a note whose
    CLAIM has failed.  The claim is that the two sides AGREE; the number is how
    the writer's instrument counted at the time, and several of these notes were
    written against tools that are now retired - the `--branches --diff` that
    said "20 branches" counted CONDITIONAL branches, where this counts every
    branch, so the same function reads 22 with nothing whatever having changed.
    Collapsing that into BROKEN buries the four rows where the two sides really
    did stop agreeing under the ones where only the ruler changed.
    """
    if base != target:
        return "BROKEN"
    return "HOLD" if base == stated else "DRIFT"


def source_sweep(wanted: set[int] | None = None) -> list[dict]:
    """One record per SOURCE note that states a measurable count, in rva order.

    Same `claims()` extractor and same `measure()` as the ledger sweep - the two
    stores must not be able to answer differently about the same pair.  The note
    text comes from `walls.priors.block_above`, so the blank-line tolerance and
    the formatter-directive filter are shared too rather than re-derived.
    """
    from homm1.walls import priors
    from homm1.walls.diagnose import _locate, named_functions

    named = named_functions()
    out: list[dict] = []
    for rva, sites in sorted(priors._pin_sites().items()):
        if wanted and rva not in wanted:
            continue
        for rel, line in sites:
            note = " ".join(x.lstrip("/*").strip() for x in
                            priors._comment_above(rel, line))
            if not note.strip():
                continue
            stated, skipped = claims(note)
            if not stated:
                continue
            rec = {"rva": rva, "site": (rel, line), "note": note,
                   "stated": stated, "skipped": skipped, "binding": None,
                   "verdicts": [], "wall": None, "error": None}
            b, why = _locate(f"0x{rva:x}", named)
            if b is None:
                rec["error"] = ("unresolved", why)
                out.append(rec)
                continue
            rec["binding"] = b
            got = measure(b)
            if isinstance(got, str):
                rec["error"] = ("unmeasured", got)
                out.append(rec)
                continue
            counts, wall = got
            rec["wall"] = wall
            rec["verdicts"] = [(_verdict(n, *counts[u]), u, n, *counts[u])
                               for u, n, _s in stated]
            out.append(rec)
    return out


def source_gate_findings() -> list[str]:
    """Every count a SOURCE note certifies, re-measured against today's pair.

    Only BROKEN reaches the gate.  A DRIFT - the two sides still agree, but not
    at the number the note states - is a retired RULER, not a failed claim: the
    `--branches --diff` several of these notes were written against counted
    CONDITIONAL branches where the pair reader counts every branch, so the same
    unchanged function reads 22 against a stated 20.  Failing a build on that
    would bury the rows where the sides really did stop agreeing.  `homm1 walls
    recheck --source` prints both classes.
    """
    out: list[str] = []
    for rec in source_sweep():
        rva = rec["rva"]
        rel, line = rec["site"]
        if rec["error"]:
            why, detail = rec["error"]
            out.append(f"0x{rva:06x}: source note at {rel}:{line} states "
                       f"{len(rec['stated'])} count(s) but the pair is "
                       f"{why} - {detail}")
            continue
        for verdict, unit, n, base, target in rec["verdicts"]:
            if verdict != "BROKEN":
                continue
            fmt = (lambda v: f"{v:#x}") if unit == "bytes" else str
            out.append(f"0x{rva:06x} {rec['binding'].name}: the note at "
                       f"{rel}:{line} states both sides at {fmt(n)} {unit}, and "
                       f"they now differ - base {fmt(base)} / target "
                       f"{fmt(target)} - re-derive it "
                       f"(`homm1 walls diagnose 0x{rva:x}`)")
    return out


def gate_findings() -> list[str]:
    """BOTH written-verdict stores, re-measured: the ledger and the source notes.

    `walls priors` has always read both stores; only one of them was ever
    re-measured.  A note above an `RVA()` pin that says "the frame is retail's
    0x7c" or "313 conditional branches and 1 ret on BOTH sides" is exactly as
    checkable, and was exactly as unre-run, as a ledger row - and nine of them
    were stating something the pair no longer held when the sweep was first
    written, four describing a frame difference that later work had already
    closed, which sends a reader hunting for a frame that already matches."""
    return _ledger_gate_findings() + source_gate_findings()


def _ledger_gate_findings() -> list[str]:
    """Every count a review certifies, re-measured against today's pair.

    This is the drift the MAX gate structurally CANNOT see.  The MAX gate
    watches the SCORE, so a commit that trades one cross-jump for another -
    `PlaceObjectFull` gaining the 16th `ret` retail has while losing a merge
    elsewhere, calls +1 / branches -1 / relocs +1 - passes it with the score
    going UP, and no source reader can see it either because the source hash
    did not move.  A review's "base and retail agree on 24 calls, 96 branches
    and 64 relocs" is the only record of that shape, and until it is
    re-measured it is prose."""
    out: list[str] = []
    for rec in sweep():
        rva = rec["rva"]
        if rec["error"]:
            why, detail = rec["error"]
            if why == "unresolved":
                out.append(f"0x{rva:06x}: review row names no claimed function "
                           f"({detail})")
            elif rec["stated"]:
                # A row whose pair cannot be read is not a row that passed.
                out.append(f"0x{rva:06x}: {len(rec['stated'])} certified "
                           f"count(s) unmeasurable - {detail}")
            continue
        name = rec["binding"].name
        for ok, unit, n, base, target in rec["verdicts"]:
            if ok:
                continue
            fmt = (lambda v: f"{v:#x}") if unit == "bytes" else str
            out.append(f"0x{rva:06x} {name}: review certifies both sides at "
                       f"{fmt(n)} {unit}, now base {fmt(base)} / target "
                       f"{fmt(target)} - re-review or re-certify "
                       f"(`homm1 walls diagnose 0x{rva:x}`)")
    return out


def _print_source(a, wanted: set[int]) -> int:
    """The `--source` view: same verdict vocabulary as the ledger view."""
    n_rows = n_claims = n_hold = n_drift = n_broken = 0
    broken_rows: list[int] = []
    drift_rows: list[int] = []
    for rec in source_sweep(wanted):
        rva = rec["rva"]
        rel, line = rec["site"]
        if rec["error"]:
            why, detail = rec["error"]
            print(f"  0x{rva:06x} {why.upper()} {detail}   {rel}:{line}")
            broken_rows.append(rva)
            continue
        verdicts = rec["verdicts"]
        n_rows += 1
        n_claims += len(verdicts)
        n_hold += sum(1 for v, *_ in verdicts if v == "HOLD")
        n_drift += sum(1 for v, *_ in verdicts if v == "DRIFT")
        n_broken += sum(1 for v, *_ in verdicts if v == "BROKEN")
        row_broken = any(v != "HOLD" for v, *_ in verdicts)
        if any(v == "BROKEN" for v, *_ in verdicts):
            broken_rows.append(rva)
        elif row_broken:
            drift_rows.append(rva)
        if a.broken and not row_broken:
            continue
        print(f"0x{rva:06x} {rec['wall']:8} {rec['binding'].name}  "
              f"[{rec['binding'].unit}]   {rel}:{line}")
        for verdict, unit, n, base, target in verdicts:
            fmt = (lambda v: f"{v:#x}") if unit == "bytes" else str
            print(f"    {verdict:6} {unit:9} note states both {fmt(n):>7}   "
                  f"now base {fmt(base):>7}  target {fmt(target):>7}")
        if a.skipped:
            for s in rec["skipped"]:
                print(f"    (skipped, mixed sentence) {s}")
    print(f"\n[recheck] {n_rows} source note(s) state counts: {n_claims} claim(s), "
          f"{n_hold} hold, {n_drift} drift (sides still agree, the stated number "
          f"is from a retired instrument), {n_broken} broken")
    if broken_rows:
        print("[recheck] the two sides stopped agreeing: "
              + " ".join(f"0x{r:06x}" for r in broken_rows))
    if drift_rows:
        print("[recheck] number drifted, claim intact: "
              + " ".join(f"0x{r:06x}" for r in drift_rows))
    return 1 if (a.strict and broken_rows) else 0


@logged
def main(argv=None) -> int:
    import argparse

    from homm1.walls.diagnose import _locate

    ap = argparse.ArgumentParser(
        prog="homm1 walls recheck",
        description=__doc__,
        formatter_class=argparse.RawDescriptionHelpFormatter,
    )
    ap.add_argument("target", nargs="*", help="hex rva, mangled name, or CClass::Member")
    ap.add_argument("--source", action="store_true",
                    help="re-run the SOURCE notes above each RVA() pin, not the ledger")
    ap.add_argument("--broken", action="store_true", help="only rows with a failed claim")
    ap.add_argument("--strict", action="store_true", help="exit 1 when any claim is broken")
    ap.add_argument("--skipped", action="store_true",
                    help="also print the mixed sentences the parser declined to read")
    a = ap.parse_args(argv)

    wanted = set()
    for token in a.target:
        b, why = _locate(token)
        if b is None:
            print(f"[recheck] {why}")
            return 2
        wanted.add(b.rva)

    if a.source:
        return _print_source(a, wanted)

    n_rows = n_claims = n_hold = n_broken = 0
    unparsed: list[int] = []
    broken_rows: list[int] = []
    n_class = Counter()
    for rec in sweep(wanted):
        rva, row = rec["rva"], rec["row"]
        if rec["error"]:
            why, detail = rec["error"]
            if why == "unresolved":
                print(f"  0x{rva:06x} UNRESOLVED {detail}")
            elif rec["stated"]:
                print(f"  0x{rva:06x} UNMEASURED {detail}")
            if rec["stated"]:
                broken_rows.append(rva)
            continue
        # The CLASS is an assertion too, and it is the one a matcher acts on.
        # It is scored separately: the review names the CAUSE it traced, this
        # names the FIRST divergence, so a disagreement is a lead, not a defect.
        same_class = rec["wall"] == row["wall_class"]
        n_class["same" if same_class else "differs"] += 1
        verdicts = rec["verdicts"]
        n_claims += len(verdicts)
        n_hold += sum(1 for ok, *_ in verdicts if ok)
        n_broken += sum(1 for ok, *_ in verdicts if not ok)
        n_rows += bool(rec["stated"])
        if not rec["stated"]:
            unparsed.append(rva)
        row_broken = any(not ok for ok, *_ in verdicts)
        if row_broken:
            broken_rows.append(rva)
        if a.broken and not row_broken:
            continue
        freshness = "current" if rec["fresh"] else "STALE"
        print(f"0x{rva:06x} {freshness:7} {row['status']:8} {row['wall_class']:8} "
              f"{rec['binding'].name}  [{rec['binding'].unit}]")
        print(f"    {'HOLD  ' if same_class else 'DIFFERS'} class"
              + ("" if same_class else
                 f"     review says {row['wall_class']}, first divergence is "
                 f"{rec['wall']}"))
        for ok, unit, n, base, target in verdicts:
            tag = "HOLD  " if ok else "BROKEN"
            fmt = (lambda v: f"{v:#x}") if unit == "bytes" else str
            print(f"    {tag} {unit:9} certified both {fmt(n):>7}   "
                  f"now base {fmt(base):>7}  target {fmt(target):>7}")
        if a.skipped:
            for s in rec["skipped"]:
                print(f"    (skipped, mixed sentence) {s}")

    print(f"\n[recheck] {n_rows} review(s) state counts: {n_claims} claim(s), "
          f"{n_hold} hold, {n_broken} broken across {len(broken_rows)} row(s)")
    print(f"[recheck] class: {n_class['same']} agree with today's first "
          f"divergence, {n_class['differs']} differ (a differing class is a "
          f"vocabulary gap as often as a stale verdict - the review names the "
          f"CAUSE, this names the FIRST divergence)")
    if unparsed:
        print(f"[recheck] {len(unparsed)} review(s) state no measurable count: "
              + " ".join(f"0x{r:06x}" for r in unparsed))
    if broken_rows:
        print("[recheck] re-review: "
              + " ".join(f"0x{r:06x}" for r in broken_rows))
    return 1 if (a.strict and broken_rows) else 0


if __name__ == "__main__":
    raise SystemExit(main())

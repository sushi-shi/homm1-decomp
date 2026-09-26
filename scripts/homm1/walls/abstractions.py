"""Derive the semantic-abstraction queue for every current sub-100 function.

    homm1 walls abstractions [--todo] [--unit U] [--module M] [--level L]
                              [--no-aggregate-sieves] [--json] [--limit N]

``walls diagnose`` names the FIRST differing machine-code feature.  That is
necessary, but it is not always the source level at fault: an aggregate copied
as scalars, an inline helper written as a function instead of a macro, or an
open-coded operation can all survive the diagnose ladder as REGALLOC.  This
command adds that missing routing layer without claiming that a mechanical
lead proves an original spelling.

The levels are deliberately semantic, highest first:

  identity    storage/referent ownership or alias identity
  object      ABI, type, object, aggregate, or by-value boundary
  call        out-of-line versus expanded helper boundary
  textual     inline-function, function-like-macro, or repeated expansion
  algorithm   branch/return topology and control-flow structure
  expression  spelling, lifetime, scheduling, and register allocation
  pairing     normalized-pair/report anomaly; repair the evidence first
  state       this source already reached exact; current gap is TU state
  generated   EH-band funclet; route through its source-owned parent

The queue remains ordered by ascending historical MAX, as the wall campaign
requires.  The level changes WHICH evidence and lever to use on a row, not
which bank is allowed to be skipped.  ``--todo`` removes EH funclets,
historically exact rows, and hash-current terminal reviews using the same
rules as ``walls inventory --todo``.  No queue is persisted.

Aggregate results are leads.  Each underlying sieve has calibrated exclusions
(walks, incoming arguments, spills, merged copy blocks); inspect its named
neighbourhood before changing a declaration.  ``--no-aggregate-sieves`` is a
fast census which retains pair and source-origin classification only.
"""

from __future__ import annotations

from homm1.core.usage import logged

from collections import Counter, defaultdict
import json
from pathlib import Path
import re

from homm1.core.paths import REPO
from homm1.verify.readme import unit_modules
from homm1.verify.scores import is_eh_band
from homm1.walls import inventory


LEVEL_ORDER = {
    "identity": 0,
    "object": 1,
    "call": 2,
    "textual": 3,
    "algorithm": 4,
    "expression": 5,
    "pairing": 6,
    "state": 7,
    "generated": 8,
}

NEXT_ACTION = {
    "identity": "adjudicate ordered referents and repair the owner/alias claim",
    "object": "adjudicate the named aggregate/type lead against its instruction neighbourhood",
    "call": "run inline-model --gap; combine COMDAT, source, /Ob0, and site-topology evidence",
    "textual": "Cartesian-test authentic inline, macro, and open-code origins before local spelling",
    "algorithm": "reconstruct the first CFG delta: arm, loop, return, or tail-sharing shape",
    "expression": "run semdiff, then test source origin before schedule/regalloc permutations",
    "pairing": "repair the normalized pair, report join, or source ownership before source edits",
    "state": "restore or replay the banked compiler/TU state; do not rewrite proven source",
    "generated": "inspect the source-owned parent with eh-frame; do not edit the funclet",
}

_RVA = r"RVA\s*\(\s*0x0*{rva:x}\s*,"
_INCLUDE = re.compile(r'^\s*#\s*include\s*[<\"]([^>\"]+)[>\"]', re.M)
_MACRO = re.compile(r"^\s*#\s*define\s+([A-Za-z_]\w*)\s*\(", re.M)
_INLINE = re.compile(
    r"\b(?:static\s+)?inline\b(?:(?![;{}#]).|\n){0,400}?"
    r"(?:[A-Za-z_]\w*::)*(~?[A-Za-z_]\w*)\s*\(",
    re.M,
)
_CALL = re.compile(r"\b([A-Za-z_]\w*)\s*\(")
_KEYWORDS = frozenset((
    "if", "for", "while", "switch", "return", "sizeof", "catch",
    "static_cast", "reinterpret_cast", "const_cast", "dynamic_cast",
))
_INFRA_MACROS = frozenset((
    "RVA", "RVA_DECL", "DATA", "RVA_COMPGEN", "RVA_DYNINIT", "DATA_COMPGEN",
    "ASSERT", "_ASSERTE", "TRACE", "UNREFERENCED_PARAMETER",
    # Strict-enum source-compatibility wrappers.  They are build plumbing,
    # not candidates for an original game-operation abstraction.
    "AT", "IDX", "HAS", "BIT",
))


def _code_mask(text: str) -> str:
    """Blank comments and literals while preserving offsets and newlines."""
    out = list(text)
    i, n = 0, len(text)
    while i < n:
        if text.startswith("//", i):
            j = text.find("\n", i + 2)
            j = n if j < 0 else j
            for k in range(i, j):
                out[k] = " "
            i = j
            continue
        if text.startswith("/*", i):
            j = text.find("*/", i + 2)
            j = n - 2 if j < 0 else j
            for k in range(i, min(n, j + 2)):
                if out[k] != "\n":
                    out[k] = " "
            i = min(n, j + 2)
            continue
        if text[i] in ('"', "'"):
            quote, j = text[i], i + 1
            while j < n:
                if text[j] == "\\":
                    j += 2
                    continue
                if text[j] == quote:
                    j += 1
                    break
                j += 1
            for k in range(i, min(n, j)):
                if out[k] != "\n":
                    out[k] = " "
            i = j
            continue
        i += 1
    return "".join(out)


def function_body(text: str, rva: int) -> str:
    """The brace-balanced body following an RVA anchor, or an empty string."""
    masked = _code_mask(text)
    mark = re.search(_RVA.format(rva=rva), masked, re.I)
    if not mark:
        mark = re.search(r"\bVA\s*\(\s*0x0*" + f"{rva + 0x400000:x}" + r"\s*,", masked, re.I)
    if not mark:
        return ""
    start = masked.find("{", mark.end())
    if start < 0:
        return ""
    depth = 0
    for i in range(start, len(masked)):
        if masked[i] == "{":
            depth += 1
        elif masked[i] == "}":
            depth -= 1
            if depth == 0:
                return text[start + 1:i]
    return ""


class SourceIndex:
    """Small include-aware index of source-visible inline and macro origins."""

    def __init__(self):
        self._text: dict[Path, str] = {}
        self._closure: dict[Path, set[Path]] = {}
        self._defs: dict[Path, tuple[set[str], set[str]]] = {}

    def text(self, path: Path) -> str:
        path = path.resolve()
        if path not in self._text:
            self._text[path] = path.read_text(errors="replace") if path.is_file() else ""
        return self._text[path]

    @staticmethod
    def _resolve_include(owner: Path, name: str) -> Path | None:
        for candidate in (owner.parent / name, REPO / "include" / name, REPO / name):
            if candidate.is_file():
                return candidate.resolve()
        return None

    def closure(self, source: Path) -> set[Path]:
        source = source.resolve()
        if source in self._closure:
            return self._closure[source]
        seen, stack = set(), [source]
        while stack:
            path = stack.pop()
            if path in seen:
                continue
            seen.add(path)
            for name in _INCLUDE.findall(self.text(path)):
                child = self._resolve_include(path, name)
                if child is not None and child not in seen:
                    stack.append(child)
        self._closure[source] = seen
        return seen

    def definitions(self, path: Path) -> tuple[set[str], set[str]]:
        path = path.resolve()
        if path not in self._defs:
            text = self.text(path)
            macros = set(_MACRO.findall(text)) - _INFRA_MACROS
            inlines = set(_INLINE.findall(_code_mask(text))) - _KEYWORDS
            self._defs[path] = macros, inlines
        return self._defs[path]

    def origin(self, source: Path, rva: int) -> dict:
        source = source.resolve()
        body = function_body(self.text(source), rva)
        if not body:
            return {"body_found": False, "macros": [], "inlines": [], "promote": False}
        calls = Counter(name for name in _CALL.findall(_code_mask(body))
                        if name not in _KEYWORDS)
        macros: dict[str, str] = {}
        inlines: dict[str, str] = {}
        for path in self.closure(source):
            mdefs, idefs = self.definitions(path)
            where = "local" if path == source else path.name
            for name in calls.keys() & mdefs:
                macros.setdefault(name, where)
            for name in calls.keys() & idefs:
                inlines.setdefault(name, where)
        # A macro shadows an inline spelling of the same token.
        for name in macros:
            inlines.pop(name, None)
        macro_rows = [{"name": n, "count": calls[n], "origin": macros[n]}
                      for n in sorted(macros)]
        inline_rows = [{"name": n, "count": calls[n], "origin": inlines[n]}
                       for n in sorted(inlines)]
        promote = any(r["count"] >= 2 for r in macro_rows) or any(
            r["count"] >= 2 or r["origin"] == "local" for r in inline_rows
        )
        return {"body_found": True, "macros": macro_rows,
                "inlines": inline_rows, "promote": promote}


def aggregate_leads(todo: bool = False) -> dict[tuple[str, str], list[str]]:
    """Join the calibrated aggregate sieves into per-function routing leads."""
    from homm1.walls import aggregate_copies, aggdecl, aggscan, valuetemp

    out: dict[tuple[str, str], list[str]] = defaultdict(list)
    for reads, label in ((False, "aggregate-write"), (True, "aggregate-read")):
        over, under, _tally, _nf, _nk, _unpaired = aggdecl.scan(
            todo=todo, reads=reads)
        for direction, rows in (("over", over), ("under", under)):
            for _pct, unit, sym, _rva, disp, base, retail in rows:
                copy_side = base if direction == "over" else retail
                # aggdecl deliberately reports ARG and WALK so its standalone
                # output can explain why a copy-shaped pair is not an object.
                # They are calibrated negatives, not abstraction-level leads.
                if not reads and not (set(copy_side) - {"ARG", "WALK"}):
                    continue
                out[(unit, sym)].append(
                    f"{label}:{direction}@+0x{disp:x} base={','.join(base)} "
                    f"retail={','.join(retail)}")

    for _delta, unit, _pct, base, retail, sym in aggregate_copies.scan():
        out[(unit, sym)].append(f"aggregate-copy-count base={base} retail={retail}")

    _agree, _local, missing, extra, counts = valuetemp.scan()
    for label, rows in (("retail-only value-temp", missing),
                        ("base-only value-temp", extra)):
        for _pct, unit, sym, pairs, _prov in rows:
            offsets = ",".join(f"+0x{n:x}" for _kind, n in pairs)
            out[(unit, sym)].append(f"{label}@{offsets}")
    for _pct, unit, sym, base, retail in counts:
        out[(unit, sym)].append(
            f"local-aggregate-copy-count base={base} retail={retail}")

    res = aggscan.sweep()
    base = aggscan.perfunction(res["ours"], res["both"])
    retail = aggscan.perfunction(res["retail"], res["both"])
    for key in set(base) | set(retail):
        if base.get(key, []) != retail.get(key, []):
            out[key].append(
                f"by-value-hole base={base.get(key, [])} retail={retail.get(key, [])}")
    return dict(out)


def choose_level(pair_class: str, aggregate: list[str], origin: dict,
                 eh_band: bool) -> tuple[str, list[str]]:
    """Choose the highest evidence-bearing source level, not a verdict."""
    if eh_band:
        return "generated", ["EH-band funclet"]
    if pair_class == "referent":
        return "identity", ["masked bytes agree but ordered referents differ"]
    if aggregate:
        return "object", aggregate
    if pair_class == "inline":
        return "call", ["call-target multisets differ"]
    if origin.get("promote"):
        evidence = []
        for kind in ("inlines", "macros"):
            for row in origin.get(kind, ()):
                if row["count"] >= 2 or (kind == "inlines" and row["origin"] == "local"):
                    evidence.append(
                        f"{kind[:-1]} {row['name']} x{row['count']} ({row['origin']})")
        return "textual", evidence
    if pair_class == "cfg":
        return "algorithm", ["call sets agree; branch/return skeleton differs"]
    if pair_class == "regalloc":
        return "expression", ["calls and branch skeleton agree; instruction stream differs"]
    return "pairing", [f"normalized pair classification is {pair_class}"]


def queue_priority(row: dict) -> tuple:
    """Campaign order stays historical-MAX first; level routes equal banks."""
    hist = row["hist_max"]
    return (hist is None, hist if hist is not None else 101.0,
            LEVEL_ORDER[row["level"]], row["cur"], int(row["rva"] or "0", 0))


def build(unit: str | None = None, modules: set[str] | None = None,
          below: float = 100.0, todo: bool = False,
          with_aggregates: bool = True) -> list[dict]:
    from homm1.permute.campaign import classified_candidates
    from homm1.walls.reviews import current as current_reviews, load as all_reviews
    from homm1.verify.fingerprints import fingerprinter, is_fallback
    from homm1 import manifest

    raw = inventory.build(unit, below, todo=todo)
    module_by_unit = unit_modules()
    if modules:
        raw = [r for r in raw if module_by_unit.get(r["unit"]) in modules]
    wanted = {(r["unit"], r["symbol"]) for r in raw}
    classified = {
        (r["unit"], r["symbol"]): r
        for r in classified_candidates(
            unit=unit, below=below, include_eh=False,
            rvas={int(r["rva"], 0) for r in raw if r["rva"]})
        if (r["unit"], r["symbol"]) in wanted
    }
    aggregates = aggregate_leads(todo=todo) if with_aggregates else {}
    sources = {u["unit"]: u.get("source", "") for u in manifest.units()}
    source_index = SourceIndex()
    live_reviews, reviews = current_reviews(), all_reviews()
    fingerprint, _cpp_of, _stale_units = fingerprinter()

    rows = []
    for row in raw:
        key = (row["unit"], row["symbol"])
        rva = int(row["rva"], 0) if row["rva"] else 0
        eh_band = is_eh_band(row["symbol"])
        pair = classified.get(key, {})
        pair_class = "generated" if eh_band else pair.get("classification", "unavailable")
        source_name = sources.get(row["unit"], "")
        source = REPO / source_name if source_name else Path()
        origin = (source_index.origin(source, rva)
                  if rva and source_name and source.is_file() and not eh_band
                  else {"body_found": False, "macros": [], "inlines": [],
                        "promote": False})
        aggregate = aggregates.get(key, [])
        level, evidence = choose_level(pair_class, aggregate, origin, eh_band)
        if row["proven"] and not eh_band:
            level = "state"
            evidence = ["this source has a banked historical MAX of 100"]
        review = reviews.get(rva)
        review_state = "current" if rva in live_reviews else ("stale" if review else "")
        source_hash = fingerprint(row["unit"], row["symbol"])
        terminal = bool(review_state == "current" and review
                        and review["status"] in ("bounded", "exact"))
        result = {
            **row,
            "module": module_by_unit.get(row["unit"], "?"),
            "pair_class": pair_class,
            "level": level,
            "evidence": evidence,
            "next_action": NEXT_ACTION[level],
            "origin": origin,
            "source": source_name,
            "source_hash": source_hash,
            "fingerprint_cache_fresh": row["unit"] not in _stale_units,
            "fingerprint_function_scoped": not is_fallback(source_hash),
            "review_state": review_state,
            "review_status": review["status"] if review else "",
            "review_class": review["wall_class"] if review else "",
            "actionable": not (eh_band or row["proven"] or terminal),
        }
        rows.append(result)
    rows.sort(key=queue_priority)
    for rank, row in enumerate(rows, 1):
        row["rank"] = rank
    return rows


@logged
def main(argv=None) -> int:
    import argparse
    from homm1.walls import check_unit

    ap = argparse.ArgumentParser(
        prog="homm1 walls abstractions", description=__doc__,
        formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--unit")
    ap.add_argument("--module", action="append", default=[],
                    help="source module/path component; repeatable")
    ap.add_argument("--below", type=float, default=100.0)
    ap.add_argument("--todo", action="store_true",
                    help="the actionable queue (same exclusions as inventory --todo)")
    ap.add_argument("--level", choices=tuple(LEVEL_ORDER))
    ap.add_argument("--no-aggregate-sieves", action="store_true",
                    help="skip aggregate/type leads for a faster census")
    ap.add_argument("--limit", type=int, default=40)
    ap.add_argument("--json", action="store_true")
    a = ap.parse_args(argv)
    check_unit(a.unit)
    known_modules = set(unit_modules().values())
    unknown = set(a.module) - known_modules
    if unknown:
        ap.error("unknown module(s): " + ", ".join(sorted(unknown)))
    rows = build(a.unit, set(a.module), a.below, a.todo,
                 not a.no_aggregate_sieves)
    if a.level:
        rows = [r for r in rows if r["level"] == a.level]
    if a.json:
        print(json.dumps(rows[:a.limit] if a.limit else rows, indent=2))
        return 0

    counts = Counter(r["level"] for r in rows)
    stale_units = sorted({r["unit"] for r in rows
                          if not r["fingerprint_cache_fresh"]})
    unmapped_fingerprints = sum(
        not r["fingerprint_function_scoped"] and r["level"] != "generated"
        for r in rows)
    scope = "todo " if a.todo else ""
    print(f"[abstractions] {len(rows)} {scope}sub-{a.below:g} row(s): "
          + ", ".join(f"{k}={counts[k]}" for k in LEVEL_ORDER if counts[k]))
    print("  ordered by historical MAX; level selects the evidence and lever")
    if stale_units:
        print(f"  WARNING: {len(stale_units)} unit fingerprint cache(s) are stale: "
              f"{', '.join(stale_units[:8])}"
              + (" ..." if len(stale_units) > 8 else ""))
        print("           hash-scoped terminal reviews in those units cannot be excluded; "
              "run `homm1 verify fingerprints` (or a full build)")
    if unmapped_fingerprints:
        print(f"  WARNING: {unmapped_fingerprints} source-owned row(s) lack a "
              "function-scoped fingerprint; reviews cannot suppress them")
    print(f"{'rank':>4} {'rva':>10} {'hist':>7} {'bank':>7} {'cur':>8} "
          f"{'level':>10}  unit/symbol")
    for row in rows[:a.limit]:
        hist = "?" if row["hist_max"] is None else f"{row['hist_max']:.2f}"
        bank = "?" if row["bank"] is None else f"{row['bank']:.2f}"
        print(f"{row['rank']:4d} {row['rva']:>10} {hist:>7} {bank:>7} "
              f"{row['cur']:8.3f} {row['level']:>10}  "
              f"{row['unit']}/{row['symbol'][:58]}")
        print(f"     evidence: {'; '.join(row['evidence'])[:180]}")
        print(f"     next: {row['next_action']}")
    if len(rows) > a.limit:
        print(f"  ... {len(rows) - a.limit} more (--limit; --json for complete records)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

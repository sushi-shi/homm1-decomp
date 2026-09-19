"""homm1.verify.scores - the compare report, loaded once, EH band carved.

READ EVERY PERCENT THROUGH `fn_fuzzy()`: objdiff serializes with serde's
skip-the-default rule, so a function scored at exactly 0.0% has NO
`fuzzy_match_percent` key at all. Defaulting the missing key to anything but
0.0 is a silent falsification. The same discipline gates every measure: an
ABSENT measure (e.g. a data-less unit's matched_data_percent) is NOT-REPORTED,
never 0 - `measure()` returns None for it and renderers print n/a.

The carved EH funclets (`__ehreg$*` / `__ehunwind$*`) are scored but are not
reconstruction targets; they leave both the rows and the aggregate arithmetic
(objdiff's measures are exact sums over the per-function rows, so removing a
known subset is arithmetic, not re-estimation).
"""

from __future__ import annotations

import json
from pathlib import Path

from homm1.core.paths import BUILD

REPORTS = (BUILD / "objdiff/compare-new/report.json",
           BUILD / "objdiff/report.json")

EH_BAND_PREFIXES = ("__ehreg$", "__ehunwind$")
EXACT = 99.995


def fn_fuzzy(fn: dict) -> float:
    """Missing `fuzzy_match_percent` means 0.0, never 'unknown', never 100."""
    return float(fn.get("fuzzy_match_percent") or 0.0)


def measure(measures: dict, key: str) -> float | None:
    """A measure, or None when ABSENT (NOT-REPORTED - never coerced to 0)."""
    v = measures.get(key)
    return None if v is None else float(v)


def is_eh_band(name: str) -> bool:
    return name.startswith(EH_BAND_PREFIXES)


def report_path() -> Path:
    for path in REPORTS:
        if path.is_file():
            return path
    raise SystemExit(f"no report ({REPORTS[0]}) - run `homm1 build` first")


def load(path: Path | None = None) -> dict:
    """The report document with the EH band carved out (in place).

    An explicit `--report` path that is missing, unreadable or not objdiff
    JSON is an OPERATOR error, not a crash: say which file and what it must
    be. (A missing DEFAULT report is answered by report_path().)
    """
    path = path or report_path()
    try:
        text = Path(path).read_text()
    except FileNotFoundError:
        raise SystemExit(f"no report at {path} - pass an objdiff report.json "
                         f"(`homm1 build` writes {REPORTS[0]})") from None
    except OSError as exc:
        raise SystemExit(f"cannot read the report {path}: {exc}") from None
    try:
        doc = json.loads(text)
    except json.JSONDecodeError as exc:
        raise SystemExit(
            f"{path} is not valid JSON ({exc}) - objdiff-cli writes it whole, "
            f"so a truncated file means the compare step was interrupted: "
            f"re-run `homm1 compare` (or `homm1 build`)") from None
    if not isinstance(doc, dict) or "units" not in doc:
        raise SystemExit(f"{path} is JSON but not an objdiff report (no "
                         f"`units` key) - point --report at "
                         f"build/objdiff/compare-new/report.json")
    split_eh_band(doc)
    return doc


def split_eh_band(doc: dict) -> list:
    """Strip the EH band rows from `doc` and return [(unit, name, size, pct)]."""
    removed = []
    for unit in doc.get("units", []):
        rows = unit.get("functions")
        if not rows:
            continue
        band = [row for row in rows if is_eh_band(row["name"])]
        if not band:
            continue
        unit["functions"] = [row for row in rows if not is_eh_band(row["name"])]
        _subtract(unit.setdefault("measures", {}), band)
        removed.extend(
            (unit.get("name", ""), row["name"], int(row.get("size") or 0),
             fn_fuzzy(row)) for row in band)
    if removed:
        _subtract(doc.setdefault("measures", {}),
                  [{"size": size, "fuzzy_match_percent": pct}
                   for _u, _n, size, pct in removed])
    return removed


def _subtract(measures: dict, rows: list) -> None:
    """Remove `rows`' contribution from one objdiff `measures` block."""
    total_code = int(measures.get("total_code") or 0)
    weighted = float(measures.get("fuzzy_match_percent") or 0.0) * total_code
    for row in rows:
        size = int(row.get("size") or 0)
        pct = fn_fuzzy(row)
        total_code -= size
        weighted -= size * pct
        measures["total_functions"] = \
            int(measures.get("total_functions") or 0) - 1
        if pct >= EXACT:
            measures["matched_functions"] = \
                int(measures.get("matched_functions") or 0) - 1
            measures["matched_code"] = \
                int(measures.get("matched_code") or 0) - size
    measures["total_code"] = str(total_code)
    measures["matched_code"] = str(int(measures.get("matched_code") or 0))
    measures["fuzzy_match_percent"] = \
        (weighted / total_code) if total_code else 0.0
    measures["matched_code_percent"] = (
        100.0 * int(measures["matched_code"]) / total_code
        if total_code else 0.0)
    total_functions = int(measures.get("total_functions") or 0)
    measures["matched_functions_percent"] = (
        100.0 * int(measures.get("matched_functions") or 0) / total_functions
        if total_functions else 0.0)


def functions(doc: dict) -> dict[tuple[str, str], float]:
    """{(unit, mangled): fuzzy%} over an EH-carved report."""
    out: dict[tuple[str, str], float] = {}
    for u in doc.get("units", []):
        unit = u.get("name", "").split("/")[-1]
        for fn in u.get("functions", []):
            out[(unit, fn["name"])] = fn_fuzzy(fn)
    return out


def unit_measures(doc: dict) -> dict[str, dict]:
    return {u.get("name", "").split("/")[-1]: u.get("measures", {})
            for u in doc.get("units", [])}


def hard_failures(doc: dict) -> list[str]:
    """Report defects that must NEVER pass a gate.

    A unit whose every total is zero/absent is a dummy.obj pairing: objdiff
    scores that 100.00% on every measure with zero totals, so the unit reports
    MATCHING while being entirely unscored. A 100% there is a hard failure,
    never a pass. (A data-only unit is fine: its total_data is nonzero.)
    """
    fails = []
    for u in doc.get("units", []):
        m = u.get("measures", {})
        totals = [int(m.get(k) or 0)
                  for k in ("total_code", "total_functions", "total_data")]
        if any(totals):
            continue
        fails.append(
            f"unit {u.get('name', '?')!r} has zero totals (code/functions/"
            f"data) yet reports "
            f"{float(m.get('fuzzy_match_percent') or 0.0):.2f}% - a dummy.obj "
            f"pairing, entirely unscored")
    return fails

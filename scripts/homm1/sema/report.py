"""homm1.sema.report - the compare slice's report.json, read only.

The compare slice OWNS the scores; sema only reads the report it left behind.
`homm1 compare` writes <out-dir>/report.json and the default out-dir is
build/objdiff/compare-new; build/objdiff/report.json is the older REFERENCE
copy it diffs against. Read them in that order - the same precedence
homm1.verify.scores and homm1.walls.inventory use - or sema answers a
current question with a banked number.

Keyed the way objdiff keys it: unit name plus the symbol name cl emitted, which
is exactly the Binding's `name`, so a score joins to the Model with no
second-guessing.
"""

from __future__ import annotations

import json
from functools import lru_cache

from homm1.core.paths import BUILD

#: current first, the banked reference second (homm1.verify.scores.REPORTS)
REPORTS = (BUILD / "objdiff/compare-new/report.json",
           BUILD / "objdiff/report.json")
REPORT = REPORTS[0]


def report_path():
    """The freshest report on disk, or REPORTS[0] when neither exists."""
    for path in REPORTS:
        if path.is_file():
            return path
    return REPORTS[0]


class Report:
    def __init__(self, path=None):
        self.path = path or report_path()
        self.data = json.loads(self.path.read_text()) if self.path.is_file() else {}

    @property
    def exists(self) -> bool:
        return bool(self.data)

    @property
    def measures(self) -> dict:
        return self.data.get("measures", {})

    def units(self) -> dict[str, dict]:
        return {u.get("name", ""): u for u in self.data.get("units", [])}

    def unit(self, name: str) -> dict | None:
        return self.units().get(name)

    def functions(self, unit: str | None = None) -> dict[tuple[str, str], dict]:
        """{(unit, symbol): function row}."""
        out = {}
        for u in self.data.get("units", []):
            if unit is not None and u.get("name") != unit:
                continue
            for fn in u.get("functions", []):
                out[(u.get("name", ""), fn.get("name", ""))] = fn
        return out

    def fn_rows(self, name: str) -> list[tuple[str, float]]:
        """[(unit, fuzzy%)] for every scored copy of a symbol. A COMDAT the
        delinker attributed to ONE unit is scored there, which need not be the
        unit the Model's claim came from - so the caller is told where."""
        return sorted((u, float(fn.get("fuzzy_match_percent") or 0.0))
                      for (u, n), fn in self.functions().items() if n == name)

    def fn_pct(self, name: str, unit: str | None = None) -> float | None:
        """The fuzzy % of one symbol; unit-qualified when the unit is known."""
        rows = self.fn_rows(name)
        for u, pct in rows:
            if unit is None or u == unit:
                return pct
        return rows[0][1] if rows else None


@lru_cache(maxsize=1)
def report() -> Report:
    return Report()

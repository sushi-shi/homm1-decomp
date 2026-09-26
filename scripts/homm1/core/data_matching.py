"""homm1.core.data_matching - the one committed data-matching switch.

    config/compare.toml
        [compare]
        data_matching = false

`true` is the strict mode: every datum a reconstructed function references
must carry a provided identity, relocation targets are scored by name and
addend, and the data-placement gates fail. `false` relaxes exactly that and
nothing else; each consumer states what it relaxes and keeps a counted
worklist of what the relaxation hides. Every consumer reads the switch through
`enabled()`, so there is one spelling and one default.

A missing file reads as strict: relaxing must be stated, never inherited.
"""

from __future__ import annotations

import tomllib
from pathlib import Path

from homm1.core.paths import CONFIG

COMPARE_TOML = CONFIG / "compare.toml"
KEY = "data_matching"


def enabled(path: Path | None = None) -> bool:
    """True when data matching is ON (strict), False when relaxed."""
    path = Path(path or COMPARE_TOML)
    if not path.is_file():
        return True
    with open(path, "rb") as f:
        doc = tomllib.load(f)
    value = doc.get("compare", {}).get(KEY)
    if not isinstance(value, bool):
        raise SystemExit(f"{path}: [compare] {KEY} must be true or false "
                         f"(got {value!r})")
    return value


def label(on: bool | None = None) -> str:
    """`data_matching = true|false`, the spelling logs and banners use."""
    on = enabled() if on is None else on
    return f"{KEY} = {'true' if on else 'false'}"

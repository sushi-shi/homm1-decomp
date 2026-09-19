"""homm1.manifest - config/units.toml, the per-TU build manifest."""

from __future__ import annotations

import tomllib
from pathlib import Path

from homm1.core.paths import CONFIG


def load(path: Path | None = None) -> dict:
    return tomllib.load(open(path or CONFIG / "units.toml", "rb"))


def units(path: Path | None = None) -> list[dict]:
    """[{unit, source, flags}] in manifest order."""
    return list(load(path).get("unit", []))


def flag_profiles(path: Path | None = None) -> dict[str, list[str]]:
    return dict(load(path).get("flags", {}))


def by_unit(path: Path | None = None) -> dict[str, dict]:
    return {u["unit"]: u for u in units(path)}

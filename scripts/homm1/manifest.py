"""homm1.manifest - config/units.toml, the per-TU build manifest.

A [[unit]] links into the images its `images` list names (default: the game
only). `units()` answers for the selected image (`homm1 --image`), so every
consumer sees exactly the units of the program it is reconstructing;
`all_units()` is the whole manifest. An image other than the game may add
compile defines under `[images.<key>]`; the same source then compiles once per
image with that image's context (see homm1.core.paths).
"""

from __future__ import annotations

import tomllib
from pathlib import Path

from homm1.core.paths import CONFIG, DEFAULT_IMAGE, image_key


def load(path: Path | None = None) -> dict:
    return tomllib.load(open(path or CONFIG / "units.toml", "rb"))


def unit_images(unit: dict) -> list[str]:
    """The images a manifest unit links into."""
    return list(unit.get("images", [DEFAULT_IMAGE]))


def all_units(path: Path | None = None) -> list[dict]:
    """Every [[unit]] in manifest order, whatever its images."""
    return list(load(path).get("unit", []))


def units(path: Path | None = None, image: str | None = None) -> list[dict]:
    """[{unit, source, flags}] of the selected image, in manifest order."""
    key = image or image_key()
    return [u for u in all_units(path) if key in unit_images(u)]


def image_defines(image: str | None = None, path: Path | None = None) -> list[str]:
    """`/D` flags the image adds to every unit it compiles (none for the game)."""
    key = image or image_key()
    table = load(path).get("images", {}).get(key, {})
    return [f"/D{d}" for d in table.get("defines", [])]


def flag_profiles(path: Path | None = None) -> dict[str, list[str]]:
    return dict(load(path).get("flags", {}))


def by_unit(path: Path | None = None) -> dict[str, dict]:
    return {u["unit"]: u for u in units(path)}

"""homm1.retail_labels.fragments - extraction's per-TU cache, parse-only.

build/gen/claims/<unit>.tsv is extract's CACHE of the source macros (the
macros in src/ are the storage). Same Claim shape as the provider tables.
"""

from __future__ import annotations

from pathlib import Path

from homm1.core.paths import BUILD
from homm1.core.tsv import read as read_tsv
from homm1.retail_labels import Claim

FRAGMENTS = BUILD / "gen/claims"

HEADER = ["rva", "size", "name", "kind", "channel", "type"]


def fragment_path(unit: str) -> Path:
    return FRAGMENTS / f"{unit}.tsv"


def unit_claims(unit: str) -> list[Claim]:
    path = fragment_path(unit)
    if not path.is_file():
        return []
    _b, _h, raw = read_tsv(path)
    out = []
    for r in raw:
        size = int(r["size"], 16) if r["size"].strip() else None
        meta = {"type": r["type"]} if r["type"].strip() else {}
        out.append(Claim(int(r["rva"], 16), r["name"], r["kind"],
                         r["channel"], size, unit, meta))
    return out


def units() -> list[str]:
    """Manifest unit names represented by the fragment tree.

    Unit names intentionally retain donor directories (for example
    ``SOURCE/HISCORE``), so a recursive walk must recover the path relative to
    ``FRAGMENTS`` rather than just ``Path.stem``.
    """
    if not FRAGMENTS.is_dir():
        return []
    return [p.relative_to(FRAGMENTS).with_suffix("").as_posix()
            for p in sorted(FRAGMENTS.rglob("*.tsv"))]


def all_claims() -> list[Claim]:
    out: list[Claim] = []
    for unit in units():
        out.extend(unit_claims(unit))
    return out

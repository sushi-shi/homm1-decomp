"""homm1.retail_labels.fragments - extraction's per-TU cache, parse-only.

<image build>/gen/claims/<unit>.tsv is extract's CACHE of the source macros
(the macros in src/ are the storage). Same Claim shape as the provider tables.

Each row records the image whose addresses its macro spells (`space`, see
homm1.retail_labels.source.claim_space). The selected image reads its own
rows as written; a game-space row of another image's unit (shared source)
is read through that image's placements.tsv, which joins it by (kind, game
rva, name) to the image's own retail address. A game-space row with no
placement is not a claim in that image.
"""

from __future__ import annotations

from functools import lru_cache
from pathlib import Path

from homm1.core.paths import DEFAULT_IMAGE, IMAGE_BUILD, image_key, retail_dir
from homm1.core.tsv import read as read_tsv
from homm1.retail_labels import Claim

FRAGMENTS = IMAGE_BUILD / "gen/claims"

HEADER = ["rva", "size", "name", "kind", "channel", "type", "space"]


def fragment_path(unit: str) -> Path:
    return FRAGMENTS / f"{unit}.tsv"


@lru_cache(maxsize=None)
def placements(image: str) -> dict[tuple[str, int, str], tuple[int, int | None]]:
    """{(kind, game rva, name): (rva, size)} of a non-game image."""
    path = retail_dir(image) / "placements.tsv"
    if not path.is_file():
        return {}
    _b, _h, raw = read_tsv(path)
    return {(r["kind"], int(r["game_rva"], 16), r["name"]):
            (int(r["rva"], 16), int(r["size"], 16) if r["size"] else None)
            for r in raw}


def unit_claims(unit: str) -> list[Claim]:
    path = fragment_path(unit)
    if not path.is_file():
        return []
    image = image_key()
    _b, _h, raw = read_tsv(path)
    out = []
    for r in raw:
        size = int(r["size"], 16) if r["size"].strip() else None
        meta = {"type": r["type"]} if r["type"].strip() else {}
        rva, space = int(r["rva"], 16), r.get("space") or DEFAULT_IMAGE
        if space != image:
            if image == DEFAULT_IMAGE:
                continue          # another image's address: not a game claim
            placed = placements(image).get((r["kind"], rva, r["name"]))
            if placed is None:
                continue
            meta["game_rva"] = rva
            rva = placed[0]
        out.append(Claim(rva, r["name"], r["kind"], r["channel"], size, unit, meta))
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

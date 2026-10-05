"""homm1.retail_labels.providers - the committed claim channels, parse-only.

No policy here: LOW rows are returned (the model filters), alias multi-rows
per rva are returned in file order (the model picks + records aliases).
"""

from __future__ import annotations

from pathlib import Path

from homm1.core.paths import RETAIL
from homm1.core.tsv import read as read_tsv, rint
from homm1.retail_labels import Claim


def _rows(name: str, path: Path | None):
    # Another image may not carry every provider table; the game must.
    from homm1.core.paths import DEFAULT_IMAGE, image_key
    if path is None and image_key() != DEFAULT_IMAGE and not (RETAIL / name).is_file():
        return []
    _b, _h, raw = read_tsv(path or RETAIL / name)
    return raw


def functions_static_libs(path: Path | None = None) -> list[Claim]:
    return [Claim(int(r["rva"], 16), r["name"], "func", "functions_static_libs",
                  None, "", {"lib": r["lib"], "confidence": r["confidence"],
                             "source": r["source"]})
            for r in _rows("functions_static_libs.tsv", path)]


def functions_referents(path: Path | None = None) -> list[Claim]:
    """Reviewed linker names, without source-body or extent authority."""
    return [Claim(int(r["rva"], 16), r["name"], "func", "functions_referents",
                  None, "", {"source": r["provenance"]})
            for r in _rows("function_referents.tsv", path)]


def data_vtables(path: Path | None = None) -> list[Claim]:
    return [Claim(int(r["rva"], 16), r["name"], "data", "data_vtables",
                  rint(r["size"]), "", {"vkind": r["kind"], "note": r["note"]})
            for r in _rows("data_vtables.tsv", path)]


def data_static_libs(path: Path | None = None) -> list[Claim]:
    return [Claim(int(r["rva"], 16), r["name"], "data", "data_static_libs",
                  rint(r["size"]), r["unit"], {"note": r["note"]})
            for r in _rows("data_static_libs.tsv", path)]


def data_compgen(path: Path | None = None) -> list[Claim]:
    return [Claim(int(r["rva"], 16), r["name"], "data", "data_compgen",
                  rint(r["size"]), r["owner"], {"class": r["class"]})
            for r in _rows("data_compgen.tsv", path)]


def all_claims() -> list[Claim]:
    return (functions_static_libs() + data_vtables() + data_static_libs()
            + data_compgen() + functions_referents())

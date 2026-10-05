"""Reviewed MASM units and their fixed retail claims.

Assembly has no C++ VA
annotations; target facts are loaded from config/retail/asm_claims.tsv.
"""

from __future__ import annotations

from dataclasses import dataclass
from pathlib import Path


@dataclass(frozen=True)
class FixedAsmClaim:
    va: int
    size: int
    name: str
    kind: str = "func"


@dataclass(frozen=True)
class FixedAsmUnit:
    source: str
    claims: tuple[FixedAsmClaim, ...]


def _read_units(path: Path | None = None) -> dict[str, FixedAsmUnit]:
    # Assembly claims spell game addresses; another image places them
    # through its placements (homm1.retail_labels.fragments).
    from homm1.core.paths import RETAIL_ROOT
    from homm1.core.tsv import read
    result = {}
    for row in read(path or RETAIL_ROOT / "asm_claims.tsv")[2]:
        name, source = row["unit"], row["source"]
        previous = result.get(name, FixedAsmUnit(source, ()))
        if previous.source != source:
            raise ValueError(f"{name}: inconsistent assembly source paths")
        claim = FixedAsmClaim(int(row["rva"], 16) + 0x00400000,
                              int(row["size"], 16), row["name"], row["kind"])
        if claim.size <= 0 or any(c.name == claim.name for c in previous.claims):
            raise ValueError(f"{name}: invalid or duplicate assembly claim {claim.name}")
        result[name] = FixedAsmUnit(source, previous.claims + (claim,))
    return result


UNITS = _read_units()


def unit(name: str, configured_source: str | None = None) -> FixedAsmUnit | None:
    result = UNITS.get(name)
    if result is not None and configured_source is not None:
        if Path(configured_source).as_posix() != result.source:
            raise ValueError(
                f"{name} must use fixed MASM source {result.source}, got "
                f"{configured_source}"
            )
    return result


def fragment_rows(name: str, configured_source: str) -> list[dict[str, str]]:
    record = unit(name, configured_source)
    if record is None:
        raise ValueError(f"{name} is not a fixed MASM unit")
    return [{
        "rva": f"0x{claim.va - 0x00400000:08x}",
        "size": f"0x{claim.size:x}",
        "name": claim.name,
        "kind": claim.kind,
        "channel": "src",
        "type": "",
        "space": "game",
    } for claim in record.claims]

"""Reviewed MASM units and their fixed retail claims.

Ported directly from HoMM2 Buka's fixed_asm module.  Assembly has no C++ VA
annotations, so this small table is the source of its model claims.
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


UNITS = {
    "BASE/BITS": FixedAsmUnit(
        source="src/BASE/BITS.asm",
        claims=(
            FixedAsmClaim(0x0047BAC8, 0x2E, "_BitTest"),
            FixedAsmClaim(0x0047BAF6, 0x20, "_BitSet"),
            FixedAsmClaim(0x0047BB16, 0x22, "_BitClear"),
        ),
    ),
    "BASE/TILE": FixedAsmUnit(
        source="src/BASE/TILE.asm",
        claims=(
            FixedAsmClaim(0x0047D110, 0x134, "_TileToBitmap"),
        ),
    ),
}


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
    } for claim in record.claims]

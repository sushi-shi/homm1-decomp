"""Donor fixed-MASM provider; no HoMM1 claims are admitted.

This is deliberately not an assembly annotation framework.  The Buka BITS/TILE facts are not HoMM1 evidence and are not copied.
"""

from __future__ import annotations

from dataclasses import dataclass
from pathlib import Path


@dataclass(frozen=True)
class FixedAsmClaim:
    rva: int
    size: int
    name: str
    kind: str


@dataclass(frozen=True)
class FixedAsmUnit:
    source: str
    claims: tuple[FixedAsmClaim, ...]


# No reviewed original MASM units have been admitted for HoMM1.
UNITS = {}


def unit(unit: str, configured_source: str | None = None) -> FixedAsmUnit | None:
    """Return a fixed unit and reject manifest drift when one is configured."""
    result = UNITS.get(unit)
    if result is not None and configured_source is not None:
        if Path(configured_source).as_posix() != result.source:
            raise ValueError(
                f"{unit} must use fixed MASM source {result.source}, got "
                f"{configured_source}"
            )
    return result


def claims(kind: str | None = None):
    for unit_name, record in UNITS.items():
        for claim in record.claims:
            if kind is None or claim.kind == kind:
                yield unit_name, record.source, claim

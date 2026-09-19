"""MSVC 4.x /Od local-name hash and stack-slot ordering.

The front end stores locals in sixteen identifier-hash buckets.  The back end
walks those buckets in ascending order and each bucket newest-first, so source
names affect frame layout even though the names never reach machine code.

Ported from the proven HoMM2 matcher model.  It lives in ``core`` because both
interactive frame inspection and source-layout searches consume it.
"""

from __future__ import annotations


def ident_hash(name: str) -> int:
    value = 0
    for character in name:
        value = ((value >> 4) + value * 4 + ord(character)) & 0xFFFFFFFF
    return value


def key16(name: str) -> int:
    value = ident_hash(name)
    return (value ^ (value >> 16)) & 0xFFFF


def bucket(name: str) -> int:
    return key16(name) & 0xF


def slot_order(names: list[str] | tuple[str, ...]) -> list[str]:
    """Return declaration names in shallowest-to-deepest frame-slot order."""
    positions = {name: index for index, name in enumerate(names)}
    return sorted(names, key=lambda name: (bucket(name), -positions[name]))


def predict_offsets(
        names: list[str] | tuple[str, ...],
        sizes: dict[str, int] | None = None,
) -> dict[str, int]:
    offset = 0
    result = {}
    for name in slot_order(names):
        offset -= sizes.get(name, 4) if sizes else 4
        result[name] = offset
    return result

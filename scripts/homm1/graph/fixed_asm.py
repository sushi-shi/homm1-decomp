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
    "BASE/BMAP2": FixedAsmUnit(
        source="src/BASE/BMAP2.asm",
        claims=(
            FixedAsmClaim(0x0047920c, 0x74,
                          "?BlitBitmap@@YAXPAVbitmap@@HHHH0HH@Z"),
            FixedAsmClaim(0x00479280, 0xE6,
                          "?MoveBitmapArea@@YAXPAVbitmap@@HHHHHH@Z"),
            FixedAsmClaim(0x00479366, 0x4B,
                          "?DimBitmapArea@@YAXPAVbitmap@@HHHH@Z"),
            FixedAsmClaim(0x004793b2, 0x43,
                          "?FillBitmapArea@@YAXPAVbitmap@@HHHHH@Z"),
        ),
    ),
    "BASE/BITS": FixedAsmUnit(
        source="src/BASE/BITS.asm",
        claims=(
            FixedAsmClaim(0x0047ba14, 0x2E, "_BitTest"),
            FixedAsmClaim(0x0047ba42, 0x20, "_BitSet"),
            FixedAsmClaim(0x0047ba62, 0x22, "_BitClear"),
        ),
    ),
    "BASE/Icon2b": FixedAsmUnit(
        source="src/BASE/Icon2b.asm",
        claims=(
            FixedAsmClaim(0x0047d6e0, 0xD5,
                          "?IconToBitmap@@YAXPAVicon@@PAVbitmap@@HHHH@Z"),
            FixedAsmClaim(0x0047d7b6, 0xCB,
                          "?FlipIconToBitmap@@YAXPAVicon@@PAVbitmap@@HHHH@Z"),
            FixedAsmClaim(0x0047d882, 0xD5,
                          "?MonoIconToBitmap@@YAXPAVicon@@PAVbitmap@@HHHHH@Z"),
            FixedAsmClaim(0x0047d958, 0xD3,
                          "?FlipMonoIconToBitmap@@YAXPAVicon@@PAVbitmap@@HHHHH@Z"),
            FixedAsmClaim(0x0047da2c, 0xD9,
                          "?DimIconToBitmap@@YAXPAVicon@@PAVbitmap@@HHHH@Z"),
            FixedAsmClaim(0x0047db06, 0xD6,
                          "?FlipDimIconToBitmap@@YAXPAVicon@@PAVbitmap@@HHHH@Z"),
        ),
    ),
    "BASE/Icon2bc": FixedAsmUnit(
        source="src/BASE/Icon2bc.asm",
        claims=(
            FixedAsmClaim(0x0047b47c, 0x25F,
                          "?ClippedIconToBitmap@@YAXPAVicon@@PAVbitmap@@HHHH@Z"),
            FixedAsmClaim(0x0047b6dc, 0x258,
                          "?FlipClippedIconToBitmap@@YAXPAVicon@@PAVbitmap@@HHHH@Z"),
        ),
    ),
    "BASE/TILE": FixedAsmUnit(
        source="src/BASE/TILE.asm",
        claims=(
            FixedAsmClaim(0x0047be68, 0x134, "_TileToBitmap"),
        ),
    ),
    "BASE/MAKEFILEID": FixedAsmUnit(
        source="src/BASE/MAKEFILEID.asm",
        claims=(
            FixedAsmClaim(0x0047b450, 0x2A, "?MAKEFILEID@@YAIPAD@Z"),
        ),
    ),
    "BASE/LZHUFDEC": FixedAsmUnit(
        source="vendor/lzhuf/decoder/Decoder.asm",
        claims=(
            FixedAsmClaim(0x0047f8b8, 0x4D, "_LzhufMemmove"),
            FixedAsmClaim(0x0047f905, 0x85, "_GetBit"),
            FixedAsmClaim(0x0047f98a, 0xC9, "_DecodePosition"),
            FixedAsmClaim(0x0047fa53, 0x119, "_UpdateDecoderTree"),
            FixedAsmClaim(0x0047fb6c, 0x153, "_ReconstructDecoderTree"),
            FixedAsmClaim(0x0047fcbf, 0x129, "_Decode"),
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

"""homm1.sema.dump_target - the matcher's target dump: bytes, relocations, asm.

    python3 -m homm1.sema.dump_target 0x153810 [0x1549d0 ...]
    python3 -m homm1.sema.dump_target CImage::RenderFrame --no-disasm
    python3 -m homm1.sema.dump_target 0x1ee54c --hex        # a data row

What one retail function (or datum) is made of: its admitted extent, the
ordered relocation sites with each target resolved through the Model (the
load-bearing address operands - vftables, globals, pooled literals, imports),
and the annotated disassembly. The reloc list is the ordered referent set an
exact match has to reproduce, which is why it prints even with --no-disasm.
"""

from __future__ import annotations

import sys

from homm1.sema import run
from homm1.sema.disasm import extent, listing
from homm1.sema.image import retail
from homm1.sema.index import index


def hexdump(rva: int, size: int, width: int = 16) -> list[str]:
    img = retail()
    blob = img.read(rva, size) or b""
    out = []
    for off in range(0, len(blob), width):
        chunk = blob[off:off + width]
        text = "".join(chr(c) if 0x20 <= c < 0x7F else "." for c in chunk)
        out.append(f"  {rva + off:06x}  {chunk.hex(' '):<{width * 3}} {text}")
    return out


def relocations(rva: int, size: int) -> list[str]:
    """The ordered address operands inside [rva, rva+size)."""
    idx, img = index(), retail()
    rows = img.relocs_in(rva, rva + size)
    if not rows:
        return ["Relocations: none (self-contained / rel32 calls only)"]
    out = [f"Relocations ({len(rows)} address operand(s), in order):"]
    for site, tgt in rows:
        s = img.string_at(tgt)
        row = idx.at(tgt)
        extra = f"   = {s[1]!r}" if s is not None and (row is None or not row.name) else ""
        out.append(f"  @+0x{site - rva:<4x} 0x{site:06x} -> 0x{tgt:08x} "
                   f"[{img.section_name(tgt)}]  {idx.ref_label(tgt)}{extra}")
    return out


def dump(target: str, *, size: str | None = None, no_disasm: bool = False,
         want_hex: bool = False) -> list[str]:
    idx = index()
    hits = idx.resolve_name(target)
    rva = hits[0] if len(hits) == 1 else None
    b = idx.at(rva) if rva is not None else None
    if b is not None and b.space != "text":
        out = [f"{'=' * 72}", f"{idx.display(b, rva)}  @ RVA 0x{rva:06x} "
               f"(VA 0x{rva + retail().base:08x})  size 0x{b.size:x} B  "
               f"[{b.space} {b.kind or 'data'}]", "=" * 72]
        out += relocations(rva, b.size)
        return out + (hexdump(rva, b.size) if want_hex else [])
    rva, sz, b = extent(target, size)
    out = ["=" * 72,
           f"{idx.display(b, rva)}  @ RVA 0x{rva:06x} "
           f"(VA 0x{rva + retail().base:08x})  size 0x{sz:x} B", "=" * 72]
    out += relocations(rva, sz)
    if want_hex:
        out += ["", "Bytes:"] + hexdump(rva, sz)
    if not no_disasm:
        out += ["", "Disassembly (RVA-aligned, Model-annotated):"]
        out += listing(rva, sz, b)[1:]
    return out


def main(argv: list[str] | None = None) -> int:
    import argparse
    ap = argparse.ArgumentParser(prog="homm1 sema dump",
                                 description=__doc__.split("\n\n")[0])
    ap.add_argument("target", nargs="+", help="hex RVAs or names")
    ap.add_argument("--size", help="explicit extent in hex")
    ap.add_argument("--no-disasm", action="store_true")
    ap.add_argument("--hex", action="store_true", dest="want_hex",
                    help="also dump the raw bytes")
    args = ap.parse_args(argv)
    for t in args.target:
        print("\n".join(dump(t, size=args.size, no_disasm=args.no_disasm,
                             want_hex=args.want_hex)))
    return 0


if __name__ == "__main__":
    sys.exit(run(__name__, sys.argv[1:]))

"""homm1.sema.vtable - a vtable's slots, or which vtable holds a function.

    python3 -m homm1.sema.vtable 0x1ee54c          # dump the table's slots
    python3 -m homm1.sema.vtable 0x1c9c08          # which slot(s) hold this fn
    python3 -m homm1.sema.vtable --list CFile      # vtable rows by name

The table itself is the evidence: the admitted data row states the extent, so
the slot COUNT is a fact rather than a scan heuristic, and every slot is a
relocated dword the Model resolves to a function - through the linker's ILT
forwarders, which is what a vtable actually stores for a cross-band callee.
"""

from __future__ import annotations

import sys

from homm1.sema import die, resolve_target, run
from homm1.sema.image import retail
from homm1.sema.index import index


def vtable_rows() -> list:
    """Every admitted vtable data row, in address order."""
    return [b for b in index().data if b.kind == "vtable"]


def slots(b) -> list[tuple[int, int, int | None]]:
    """[(slot_index, stored_rva, body_rva_or_None)] for a vtable row."""
    img = retail()
    out = []
    for k in range(b.size // 4):
        v = img.u32(b.rva + k * 4)
        if v is None:
            break
        tgt = v - img.base
        out.append((k, tgt, img.jmp_target(tgt) if img.is_text(tgt) else None))
    return out


def dump(b) -> list[str]:
    idx, img = index(), retail()
    out = [f"vtable {b.name or '(unnamed)'}  @0x{b.rva:08x}  0x{b.size:x} B  "
           f"= {b.size // 4} slot(s)" + (f"  [{b.unit}]" if b.unit else "")
           + (f"  ({b.channel})" if b.channel else "")]
    for k, tgt, body in slots(b):
        via = f"   (via thunk 0x{tgt:06x})" if body is not None and body != tgt else ""
        real = body if body is not None and body != tgt else tgt
        row = idx.func(real)
        tag = "" if row is not None else "   <- no admitted function row"
        if not img.is_text(tgt):
            tag = f"   <- NOT .text ({img.section_name(tgt)}) - past the table?"
        out.append(f"  [{k:2d}] +0x{k * 4:<3x} 0x{real:08x}  "
                   f"{idx.ref_label(real)}{via}{tag}")
    return out


def find_holding(fn: int) -> list[tuple[object, int, int | None]]:
    """[(vtable_binding, slot_index, via_thunk_rva)] holding `fn` - directly or
    through an ILT forwarder."""
    idx, img = index(), retail()
    wanted = {fn: None}
    for th in img.thunks_to(fn):
        wanted[th] = th
    hits = []
    for tgt, via in wanted.items():
        for site in img.referents.get(tgt, ()):
            row = idx.data_owner(site)
            if row is not None and row.kind == "vtable":
                hits.append((row, (site - row.rva) // 4, via))
    return sorted(hits, key=lambda h: (h[0].rva, h[1]))


def holders(fn: int) -> tuple[list[str], int]:
    idx = index()
    hits = find_holding(fn)
    if not hits:
        return ([f"no vtable slot holds {idx.label(fn)} (not a virtual, or "
                 "dispatched through a command table)"], 1)
    out = [f"vtable slots holding {idx.label(fn)}:"]
    for row, k, via in hits:
        out.append(f"  {row.name or '(unnamed vtable)'} @0x{row.rva:08x}  "
                   f"slot[{k}] (+0x{k * 4:x})"
                   + (f"  via thunk 0x{via:06x}" if via else ""))
    return out, 0


def main(argv: list[str] | None = None) -> int:
    import argparse
    ap = argparse.ArgumentParser(prog="homm1 sema vtable",
                                 description=__doc__.split("\n\n")[0])
    ap.add_argument("target", nargs="?", help="vtable rva, or a function rva/name")
    ap.add_argument("--list", dest="pattern", nargs="?", const="",
                    help="list vtable rows whose name contains PATTERN")
    ap.add_argument("--dump", action="store_true", help="force the slot dump")
    ap.add_argument("--holds", action="store_true",
                    help="force the 'which vtable holds this fn' lookup")
    args = ap.parse_args(argv)
    idx = index()
    if args.pattern is not None:
        rows = [b for b in vtable_rows() if args.pattern in (b.name or "")]
        for b in rows:
            print(f"0x{b.rva:08x}  {b.size // 4:3d} slots  {b.name}")
        print(f"[{len(rows)} vtable row(s)]")
        return 0 if rows else 1
    if not args.target:
        die("give a vtable rva, a function rva/name, or --list")
    hits = resolve_target(args.target)
    rc = 0
    for rva in hits:
        b = idx.datum(rva)
        if args.dump or (not args.holds and b is not None and b.kind == "vtable"):
            if b is None or b.kind != "vtable":
                die(f"0x{rva:08x} is not an admitted vtable row")
            print("\n".join(dump(b)))
            continue
        lines, r = holders(rva)
        print("\n".join(lines))
        rc = rc or r
    return rc


if __name__ == "__main__":
    sys.exit(run(__name__, sys.argv[1:]))

"""homm1.sema.rva - the one-shot address dossier.

    python3 -m homm1.sema.rva 0x2bf228
    python3 -m homm1.sema.rva 0x153810 --refs

Everything the Model knows about one address: which admitted census row owns
it, the claim that WON the row (name, unit, channel, stated extent) and every
claim that LOST it - the aliases are the point, because a losing claim is the
other identity something in the tree still believes. Plus the section/region,
the objdiff score when a unit claims it, and how many references reach it.

An address inside a row reports the row and the offset; a linker thunk slot is
chased to the body and the BODY's dossier is printed under it.
"""

from __future__ import annotations

import sys

from homm1.sema import resolve_target, run
from homm1.sema.image import retail
from homm1.sema.index import index
from homm1.sema.report import report


def dossier(rva: int, refs: bool = False, depth: int = 0) -> tuple[list[str], int]:
    idx, img = index(), retail()
    out = [f"RVA 0x{rva:08x}  (VA 0x{rva + img.base:08x})  [{img.section_name(rva)}]"]
    b = idx.at(rva)
    off = 0
    if b is None:
        b = idx.covering(rva)
        off = rva - b.rva if b is not None else 0
    if b is None:
        out.append("  row       : (no admitted census row covers this address)")
        return out, 1
    if off:
        out.append(f"  inside    : the row at 0x{b.rva:08x} (+0x{off:x} of "
                   f"0x{b.size:x})")
    out.append(f"  row       : 0x{b.rva:08x}..0x{b.rva + b.size:08x}  "
               f"0x{b.size:x} B  kind={b.kind or '(plain)'}  space={b.space}")
    if b.name:
        out.append(f"  binding   : {b.name}")
        out.append(f"              {idx.display(b, b.rva)}"
                   + (f"   [{b.unit}]" if b.unit else "")
                   + f"   channel={b.channel}")
    else:
        out.append("  binding   : (unclaimed - structure only, no channel names "
                   "this row)")
    if b.also:
        shown = ", ".join(b.also[:8])
        more = f" (+{len(b.also) - 8} more)" if len(b.also) > 8 else ""
        out.append(f"  also units: {shown}{more}  - the same header-inline claim "
                   "arrives from these TUs")
    if b.aliases:
        out.append(f"  aliases   : {len(b.aliases)} losing claim(s) on this row:")
        for a in b.aliases:
            size = f" size 0x{a.size:x}" if a.size else ""
            unit = f" [{a.unit}]" if a.unit else ""
            out.append(f"              {a.channel:<24} {a.name}{unit}{size}")
    else:
        out.append("  aliases   : (none - one channel claims this row)")

    if b.space == "text":
        rep = report()
        rows = rep.fn_rows(b.name) if b.name else []
        for unit, pct in rows:
            where = f"   [scored in {unit}]" if unit != b.unit else ""
            out.append(f"  match     : {pct:.2f}% fuzzy"
                       + ("  (EXACT)" if pct >= 100.0 else "") + where)
        if not rows and b.unit and rep.exists:
            out.append("  match     : (no row in report.json for this symbol)")
        callers = len(img.call_index.get(b.rva, ()))
        out.append(f"  callers   : {callers} direct rel32 site(s)")
    nrefs = len(img.refs_to_range(b.rva, b.rva + max(b.size, 1)))
    out.append(f"  refs      : {nrefs} relocated reference(s) into this row"
               + ("" if not nrefs else "   (`sema xref` names them)"))
    if refs and nrefs:
        from homm1.sema.xref import references
        out += references(b.rva, b.size)
    s = img.string_at(rva)
    if s is not None:
        out.append(f"  string    : {s[1]!r}"
                   + (f" (starts 0x{s[0]:08x})" if s[0] != rva else ""))
    body = img.jmp_target(rva) if img.is_text(rva) else None
    if body is not None and body != rva and depth < 2:
        out.append(f"  thunk     : forwards to 0x{body:08x} - the body's dossier:")
        sub, _ = dossier(body, refs=refs, depth=depth + 1)
        out += ["    " + line for line in sub]
    return out, 0


from homm1.core.usage import logged


@logged
def main(argv: list[str] | None = None) -> int:
    import argparse
    ap = argparse.ArgumentParser(prog="homm1 sema rva",
                                 description=__doc__.split("\n\n")[0])
    ap.add_argument("addr", nargs="+", help="hex RVAs (or names)")
    ap.add_argument("--refs", action="store_true",
                    help="list the referencing sites, not just the count")
    args = ap.parse_args(argv)
    rc = 0
    for token in args.addr:
        for rva in resolve_target(token):
            lines, r = dossier(rva, refs=args.refs)
            print("\n".join(lines))
            rc = rc or r
    return rc


if __name__ == "__main__":
    sys.exit(run(__name__, sys.argv[1:]))

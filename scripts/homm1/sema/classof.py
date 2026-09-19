"""homm1.sema.classof - a class's vtables, members and slot topology.

    python3 -m homm1.sema.classof CMirrorFile
    python3 -m homm1.sema.classof 0x1c9c08        # which slot(s) hold this fn
    python3 -m homm1.sema.classof CImage --members

Everything the Model can say about one class: the admitted `??_7` table(s)
with every slot resolved, which OTHER classes' methods those slots point at
(the implementations this class inherits - the base chain as the linker
actually wired it, not as a header claims), the RTTI rows, and the member
functions claimed for the class with their objdiff scores.

Give a function instead of a class name and the lookup runs the other way:
the vtable slot(s) that hold it, then that class's table.
"""

from __future__ import annotations

import sys

from homm1.sema import die, run
from homm1.sema.index import index, short_name, split_mangled
from homm1.sema.report import report
from homm1.sema.vtable import dump, find_holding, slots, vtable_rows


def class_of(name: str) -> str | None:
    """The class a mangled name belongs to, or None for a free symbol."""
    bare = name[1:] if name.startswith("_?") else name
    special, tokens = split_mangled(bare)
    if not tokens:
        return None
    if special:
        return tokens[0]
    return tokens[1] if len(tokens) > 1 and not tokens[1].startswith("?") else None


def class_vtables(cls: str) -> list:
    return [b for b in vtable_rows()
            if b.name and (b.name.startswith(f"??_7{cls}@@")
                           or b.name.startswith(f"??_7{cls}@"))
            and class_of(b.name) == cls]


def members(cls: str) -> list:
    idx = index()
    return [b for b in idx.functions + idx.data
            if b.name and class_of(b.name) == cls]


def describe(cls: str, want_members: bool = False) -> tuple[list[str], int]:
    idx, rep = index(), report()
    tables = class_vtables(cls)
    rows = members(cls)
    if not tables and not rows:
        return ([f"no binding names class '{cls}' (try `sema vtable --list {cls}` "
                 "or an exact mangled name)"], 1)
    out = [f"class {cls}"]
    for b in tables:
        out += ["", *("  " + line for line in dump(b))]
        origin: dict[str, int] = {}
        for _k, tgt, body in slots(b):
            row = idx.func(body if body is not None else tgt)
            owner = class_of(row.name) if row is not None and row.name else None
            origin[owner or "(unattributed)"] = origin.get(owner or "(unattributed)", 0) + 1
        summary = ", ".join(f"{k} x{v}" for k, v in sorted(origin.items(),
                                                           key=lambda kv: -kv[1]))
        out.append(f"    slot origins: {summary}")
        out.append("    (a slot implemented by another class is INHERITED - the "
                   "base chain as linked)")
    rtti = [b for b in idx.data if b.name and b.kind == "rtti" and cls in b.name]
    if rtti:
        out += ["", f"  RTTI rows ({len(rtti)}):"]
        for b in rtti[:8]:
            out.append(f"    0x{b.rva:08x}  0x{b.size:x} B  {b.name}")
    if want_members or not tables:
        out += ["", f"  members claimed for {cls} ({len(rows)}):"]
        for b in sorted(rows, key=lambda b: b.rva):
            scores = rep.fn_rows(b.name) if b.space == "text" else []
            pct = f"  {scores[0][1]:6.2f}%" if scores else ""
            out.append(f"    0x{b.rva:08x} 0x{b.size:<5x} {b.space:<5} "
                       f"{short_name(b.name):<44} [{b.unit or '-'}]{pct}")
    else:
        out.append(f"\n  {len(rows)} member binding(s) - add --members to list them")
    return out, 0


from homm1.core.usage import logged


@logged
def main(argv: list[str] | None = None) -> int:
    import argparse
    ap = argparse.ArgumentParser(prog="homm1 sema class",
                                 description=__doc__.split("\n\n")[0])
    ap.add_argument("name", help="class name, or a function rva / mangled name")
    ap.add_argument("--members", action="store_true",
                    help="list the class's member bindings")
    args = ap.parse_args(argv)
    idx = index()
    looks_fn = args.name.lower().startswith("0x") or args.name.startswith("?")
    if looks_fn:
        hits = idx.resolve_name(args.name)
        if not hits:
            die(f"'{args.name}' resolves to no binding")
        rc = 0
        for rva in hits:
            holding = find_holding(rva)
            b = idx.func(rva)
            if not holding:
                print(f"no vtable slot holds {idx.label(rva)}")
                rc = 1
            for row, k, via in holding:
                print(f"{row.name} @0x{row.rva:08x}  slot[{k}] (+0x{k * 4:x})"
                      + (f"  via thunk 0x{via:06x}" if via else ""))
            owner = class_of(b.name) if b is not None and b.name else None
            if owner:
                lines, r = describe(owner)
                print("\n".join(lines))
                rc = rc or r
        return rc
    lines, rc = describe(args.name, want_members=args.members)
    print("\n".join(lines))
    return rc


if __name__ == "__main__":
    sys.exit(run(__name__, sys.argv[1:]))

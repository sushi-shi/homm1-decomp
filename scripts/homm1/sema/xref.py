"""homm1.sema.xref - who reaches this address?

    python3 -m homm1.sema.xref 0x136180 0x139bf0
    python3 -m homm1.sema.xref CGameApp::CloseResources --tree
    python3 -m homm1.sema.xref --callees 0x136180
    python3 -m homm1.sema.xref 0x2bf314          # a datum: its referent sites

Two edge kinds, both from the retail image:

  * rel32 `call`/`jmp` sites in .text (the direct call graph), attributed to
    the function whose admitted extent contains the site - size-bounded, so a
    site past a body's end rolls up to the unclaimed tail instead of
    manufacturing a phantom edge;
  * RELOCATED references - every DIR32 fixup whose stored address lands in the
    target. That is the vtable slot, the fn-ptr/command table entry, the
    `push offset fn` registration and every global operand, and it is why a
    function with no rel32 caller is still reached. Both views print by
    default; --flat drops the reference list, --tree adds caller ancestry.
"""

from __future__ import annotations

import struct
import sys

from homm1.sema import run
from homm1.sema.image import retail
from homm1.sema.index import index

MAX_REFS = 24


def site_where(rva: int, offsets: bool = True) -> str:
    """Where a reference SITE lives: the owning function, the owning datum, or
    the unclaimed tail it fell into."""
    idx, img = index(), retail()
    if img.is_text(rva):
        fn = idx.owner(rva)
        if fn is not None:
            off = f"+0x{rva - fn.rva:x}" if rva != fn.rva and offsets else ""
            return f"in {idx.display(fn, rva)}{off}" + (
                f" [{fn.unit}]" if fn.unit else "")
        prev = idx.preceding_func(rva)
        if prev is not None:
            return (f"in the unclaimed tail after {idx.display(prev, prev.rva)} "
                    f"(0x{prev.rva + prev.size:06x}..)")
        return "in .text (no admitted row)"
    b = idx.data_owner(rva)
    if b is None:
        return f"in {img.section_name(rva)} (no admitted row)"
    off = f"+0x{rva - b.rva:x}" if rva != b.rva else ""
    return (f"in {idx.display(b, rva)}{off} [{b.kind or b.space}]"
            + (f" ({b.unit})" if b.unit else ""))


def callers(target: int, raw: bool = False) -> list[str]:
    img = retail()
    sites = img.call_index.get(target, [])
    if not sites:
        return ["  (no direct call/jmp rel32 site in .text)"]
    out, seen = [], set()
    for site, op in sorted(sites):
        kind = "call" if op == 0xE8 else "jmp "
        if raw:
            out.append(f"  {kind} @0x{site:06x}  {site_where(site)}")
            continue
        where = site_where(site, offsets=False)     # one line per caller
        if (kind, where) not in seen:
            seen.add((kind, where))
            n = sum(1 for s, o in sites
                    if o == op and site_where(s, offsets=False) == where)
            out.append(f"  {kind} {where}" + (f"   (x{n})" if n > 1 else ""))
    return out


def references(target: int, size: int) -> list[str]:
    """Relocated references to `target`, then the ones landing in its interior
    (a stored `clip+0x64` references clip's extent, not its address)."""
    img = retail()
    refs = img.refs_to_range(target, max(target + size, target + 1))
    exact = [(s, t) for s, t in refs if t == target]
    interior = [(s, t) for s, t in refs if t != target]
    out = []

    def rows(items, head):
        out.append(head)
        for site, tgt in items[:MAX_REFS]:
            plus = f" -> +0x{tgt - target:x}" if tgt != target else ""
            out.append(f"     @0x{site:06x} [{img.section_name(site)}]{plus}"
                       f"   {site_where(site)}")
        if len(items) > MAX_REFS:
            out.append(f"     ... (+{len(items) - MAX_REFS} more)")

    if exact:
        rows(exact, f"  -- {len(exact)} relocated reference(s) to this address "
                    "(vtable slot / fn-ptr table / address-taking / operand):")
    if interior:
        rows(interior, f"  -- {len(interior)} reference(s) into its interior "
                       f"(+0x{interior[0][1] - target:x}"
                       f"..+0x{interior[-1][1] - target:x}):")
    return out


def effective_incoming(target: int):
    """Reachability edges after transparent linker-thunk forwarding.

    A retail reference commonly names the five-byte jump INSIDE an admitted
    incremental-link thunk rather than the final body.  Walking only rel32
    edges through that jump loses vtable slots and stored callback pointers,
    making a live function look dead.  ``op is None`` denotes such a
    relocated reference; ``via`` records the thunk-entry addresses traversed.
    """
    idx, img = index(), retail()

    def effective(rva: int, via: tuple, guard: set):
        if rva in guard:
            return
        guard.add(rva)
        if via:
            for site, _tgt in sorted(img.refs_to_range(rva, rva + 1)):
                yield (idx.owner(site), None, site, via)
        for site, op in sorted(img.call_index.get(rva, ())):
            owner = idx.owner(site)
            if owner is not None and owner.rva == target:
                continue                                   # intra-function
            if img.jmp_target(site) == rva and (owner is None
                                                or owner.kind == "thunk"):
                yield from effective(site, via + (site,), guard)  # forwarder
            elif owner is not None and owner.kind == "thunk":
                yield from effective(owner.rva, via + (owner.rva,), guard)
            else:
                yield (owner, op, site, via)

    yield from effective(target, (), set())


def is_effectively_reached(target: int, size: int = 1) -> bool:
    """Whether anything outside the body reaches it, including through a
    linker thunk by rel32 or relocation."""
    img = retail()
    return bool(img.refs_to_range(target, target + max(size, 1))
                or next(effective_incoming(target), None) is not None)


def caller_tree(target: int, depth_cap: int = 4) -> list[str]:
    """Caller ancestry: THROUGH thunk forwarders, expanding through
    reconstructed callers, stopping at the unreconstructed frontier."""
    idx = index()
    out, seen = [], set()

    def walk(rva: int, depth: int):
        if depth_cap and depth > depth_cap:
            out.append("  " * (depth + 1) + "... (--depth cap)")
            return
        rows, uniq = [], set()
        for owner, op, site, via in effective_incoming(rva):
            key = (owner.rva if owner else ("gap", site), op)
            if key in uniq:
                continue
            uniq.add(key)
            rows.append((owner, op, site, via))
        if not rows and depth == 0:
            out.append("  (no direct call/jmp rel32 caller in .text)")
        for owner, op, site, via in rows:
            kind = "ref " if op is None else "call" if op == 0xE8 else "jmp "
            pad = "  " * (depth + 1)
            via_s = f"  (via {len(via)} thunk(s))" if via else ""
            if op is None:
                out.append(pad + f"<- {kind} {site_where(site)}{via_s}")
                continue
            if owner is None:
                out.append(pad + f"<- {kind} {site_where(site)}{via_s}")
                continue
            if owner.rva in seen:
                out.append(pad + f"<- {kind} {idx.label(owner.rva)}{via_s} (*seen)")
                continue
            seen.add(owner.rva)
            frontier = "" if idx.is_src(owner) else "  [not reconstructed - frontier]"
            out.append(pad + f"<- {kind} {idx.label(owner.rva)}{via_s}{frontier}")
            if idx.is_src(owner):
                walk(owner.rva, depth + 1)
    walk(target, 0)
    return out


def callees(target: int) -> list[str]:
    """The forward view: rel32 callees, indirect sites, referenced data."""
    idx, img = index(), retail()
    b = idx.func(target) or idx.owner(target)
    if b is None:
        return ["  (no admitted function row here)"]
    lo, hi = b.rva, b.rva + b.size
    blob = img.read(lo, b.size) or b""
    out, seen = [], []
    indirect = 0
    for i in range(max(0, len(blob) - 4)):
        if blob[i] in (0xE8, 0xE9):
            tgt = lo + i + 5 + struct.unpack_from("<i", blob, i + 1)[0]
            if img.is_text(tgt) and not (lo <= tgt < hi) and tgt not in seen:
                seen.append(tgt)
        elif blob[i] == 0xFF and i + 1 < len(blob) and (blob[i + 1] >> 3) & 7 in (2, 4):
            indirect += 1
    for tgt in seen:
        out.append(f"  -> {idx.ref_label(tgt)}   (0x{tgt:06x})")
    if not seen:
        out.append("  (no direct call/jmp rel32 callee)")
    data_refs = []
    for _site, tgt in img.relocs_in(lo, hi):
        lbl = idx.ref_label(tgt)
        if lbl not in data_refs and not img.is_text(tgt):
            data_refs.append(lbl)
    if data_refs:
        out.append(f"  -- references {len(data_refs)} datum/data label(s): "
                   + ", ".join(data_refs[:MAX_REFS])
                   + (" ..." if len(data_refs) > MAX_REFS else ""))
    if indirect:
        out.append(f"  (~{indirect} indirect call/jmp site(s) - vtable / IAT / "
                   "fn-ptr dispatch, invisible to a rel32 scan)")
    return out


def query(targets: list[str], *, mode: str = "default", raw: bool = False,
          depth: int = 4, flat: bool = False) -> int:
    idx = index()
    hit = 0
    for t in targets:
        rvas = idx.resolve_name(t)
        if not rvas:
            print(f"[sema] '{t}' resolves to no binding", file=sys.stderr)
            continue
        for rva in rvas:
            b = idx.at(rva)
            size = b.size if b is not None else 1
            print(f"\n==== {mode if mode != 'default' else 'xrefs'} of "
                  f"{idx.label(rva)} ====")
            lines: list[str] = []
            if mode == "callees":
                lines = callees(rva)
            elif mode == "tree":
                lines = caller_tree(rva, depth)
                if not flat:
                    lines += references(rva, size)
            else:
                if retail().is_text(rva):
                    lines = callers(rva, raw=raw)
                if not flat:
                    lines += references(rva, size)
            found = bool((is_effectively_reached(rva, size)
                          if mode == "tree"
                          else retail().call_index.get(rva)
                          or retail().refs_to_range(rva, rva + max(size, 1)))
                         or mode == "callees")
            print("\n".join(lines) if lines else "  (nothing reaches this address)")
            hit += bool(found)
    return 0 if hit else 1


def main(argv: list[str] | None = None) -> int:
    import argparse
    ap = argparse.ArgumentParser(prog="homm1 sema xref",
                                 description=__doc__.split("\n\n")[0])
    ap.add_argument("target", nargs="+", help="hex RVAs or names")
    ap.add_argument("--tree", action="store_true", help="caller ancestry")
    ap.add_argument("--callees", action="store_true", help="the forward view")
    ap.add_argument("--flat", action="store_true",
                    help="direct rel32 sites only (drop the reference list)")
    ap.add_argument("--raw", action="store_true", help="one line per site")
    ap.add_argument("--depth", type=int, default=4, help="--tree depth cap (0 = none)")
    args = ap.parse_args(argv)
    mode = "callees" if args.callees else "tree" if args.tree else "default"
    return query(args.target, mode=mode, raw=args.raw, depth=args.depth,
                 flat=args.flat)


if __name__ == "__main__":
    sys.exit(run(__name__, sys.argv[1:]))

"""homm1.verify.alloc_size - the retail ALLOCATION-SIZE oracle (full tier).

`push <n>; call ??2@YAPAXI@Z` is ground truth for sizeof(C): the immediate
retail baked into every `new C` site. The gate compares it with clang's
sizeof over our real member list; a uniquely-defined class whose computed
size differs will emit the wrong allocation immediate at every new-site.

ATTRIBUTION (ported): scan forward from each site to the enclosing
function's end, tracking which registers provably hold the pointer
`operator new` returned (first `mov reg,eax` before any call, then copies;
eax/ecx/edx die at calls). Tiers, strongest first: an OWNED `??_7` vptr
stamp at displacement 0 (the most-derived class on the RTTI spine wins - an
inlined ctor chain stamps each base in turn); an owned ctor call; then the
unowned stamps/ctors bounded by the NEXT allocation. `mov [ebp],imm32` has
no mod=00 encoding - cl spells it C7 45 00 - and missing that form once
mis-attributed three CSBI_ImageSet sites.

The computed side is a libclang record-layout harvest over the compdb
(cached in build/gen/class_sizes.json, keyed on a src/include tree hash).

    python3 -m homm1.verify.alloc_size [--all] [--sites] [--class C]
"""

from __future__ import annotations

import bisect
import hashlib
import json
import struct
from collections import defaultdict

from homm1.core.paths import BUILD, REPO

_CACHE = BUILD / "gen/class_sizes.json"
_WINDOW = 4096


def _disp0_store(tb, o):
    """(True, length) if tb[o:] is `mov [reg+0], imm32` (C7 /0)."""
    m = tb[o + 1]
    mod, rm = m >> 6, m & 7
    if mod == 0 and rm not in (4, 5):
        return True, 6
    if mod == 1 and rm != 4 and tb[o + 2] == 0:
        return True, 7
    return False, 0


def _cls_of(mangled: str) -> str:
    body = mangled[4:] if mangled.startswith("??_7") else mangled[3:]
    return body.split("@@", 1)[0]


class Sweep:
    def __init__(self):
        from homm1.model import resolve
        from homm1.sema.image import retail
        self.img = retail()
        t = self.img.pe.section(".text")
        self.tva = t["va"]
        self.tb = self.img.pe.data[t["rptr"]:t["rptr"] + t["rsize"]]
        m = resolve()
        self.fn_rows = m.functions
        self.fn_starts = [b.rva for b in m.functions]
        self.fname = {b.rva: b.name for b in m.functions if b.name}
        self.fsize = {b.rva: b.size for b in m.functions}
        self.thunks = {b.rva for b in m.functions if b.kind == "thunk"}
        self.gsyms = {b.rva: b.name for b in m.data if b.name}
        new_rva = next((b.rva for b in m.functions
                        if b.name == "??2@YAPAXI@Z"), None)
        if new_rva is None:
            raise SystemExit("[alloc-size] ??2@YAPAXI@Z (operator new) is not "
                             "a claimed function - cannot locate new-sites")
        targets = {new_rva} | set(self.img.thunks_to(new_rva))
        self.sites = sorted(
            s for t2 in targets for s, op in self.img.call_index.get(t2, [])
            if op == 0xE8 and self._owner(s) not in self.thunks)
        self._insns: dict[int, tuple[str, str]] | None = None
        self._addrs: list[int] | None = None

    # --- decoded .text (one objdump pass) -----------------------------------
    def _decode(self):
        if self._insns is not None:
            return
        from homm1.tool import objdump
        text = objdump.disassemble(self.tb, vma=self.tva)
        ins: dict[int, tuple[str, str]] = {}
        for line in text.splitlines():
            if ":\t" not in line:
                continue
            addr, rest = line.split(":\t", 1)
            parts = rest.split("\t")
            asm = parts[-1].strip()
            if not asm:
                continue
            toks = asm.split(None, 1)
            try:
                ins[int(addr.strip(), 16)] = (toks[0],
                                              toks[1] if len(toks) > 1 else "")
            except ValueError:
                continue
        self._insns = ins
        self._addrs = sorted(ins)

    def _pushed_size(self, site):
        """The LAST `push imm` before the call, stopping at any call - cl
        slots unrelated work between the push and the call, so walk the
        DECODED stream back rather than requiring byte adjacency."""
        self._decode()
        i = bisect.bisect_left(self._addrs, site) - 1
        for _ in range(12):
            if i < 0:
                return None
            a = self._addrs[i]
            mn, ops = self._insns[a]
            if mn in ("call", "ret", "leave"):
                return None
            if mn == "push":
                ops = ops.strip()
                if ops.startswith("0x"):
                    try:
                        return int(ops, 16)
                    except ValueError:
                        return None
                return None
            i -= 1
        return None

    def _owner(self, rva):
        i = bisect.bisect_right(self.fn_starts, rva) - 1
        if i < 0:
            return None
        start = self.fn_starts[i]
        sz = self.fsize.get(start)
        if sz and rva >= start + sz:
            return None
        return start

    def _callee(self, p):
        rel = struct.unpack_from("<i", self.tb, p - self.tva + 1)[0]
        tgt = p + 5 + rel
        body = self.img.jmp_target(tgt)
        return body if body is not None else tgt

    def _name_of(self, rva):
        return self.fname.get(rva, "")

    def _next_site(self, site):
        i = bisect.bisect_right(self.sites, site)
        return self.sites[i] if i < len(self.sites) else 1 << 62

    def _fence(self, site):
        """The enclosing function's end (NOT the next new-site: a factory
        allocates sub-parts before stamping the outer object; register
        ownership, not distance, separates the objects)."""
        lim = site + _WINDOW
        own = self._owner(site)
        if own is not None:
            sz = self.fsize.get(own)
            if sz:
                lim = min(lim, own + sz)
        return lim

    def attribute(self, site):
        """(stamps, ctors, vector): [(rva, name, owned)] each, plus whether a
        vector ctor/dtor iterator follows."""
        stamps, ctors, vector = [], [], False
        live, seen_call = set(), False
        p, lim = site + 5, self._fence(site)
        tb, tva = self.tb, self.tva
        base = self.img.base
        while p < lim:
            o = p - tva
            if o + 7 >= len(tb):
                break
            by = tb[o]
            if by == 0xE8:
                tgt = self._callee(p)
                nm = self._name_of(tgt)
                if nm.startswith(("??_E", "??_H", "??_I")):
                    vector = True
                if nm.startswith("??0"):
                    ctors.append((p, nm, 1 in live))
                live -= {0, 1, 2}
                seen_call = True
                p += 5
                continue
            if by == 0x8B and (tb[o + 1] & 0xC0) == 0xC0:
                dst, src = (tb[o + 1] >> 3) & 7, tb[o + 1] & 7
                if src == 0 and not live and not seen_call:
                    live.add(dst)
                elif src in live:
                    live.add(dst)
                elif dst in live:
                    live.discard(dst)
                p += 2
                continue
            if by == 0xC7:
                hit, ln = _disp0_store(tb, o)
                if hit:
                    imm = struct.unpack_from("<I", tb, o + ln - 4)[0]
                    g = self.gsyms.get(imm - base)
                    if g and g.startswith("??_7"):
                        stamps.append((p, g, (tb[o + 1] & 7) in live))
                    p += ln
                    continue
            p += 1
        return stamps, ctors, vector

    def _ancestors(self, cls: str) -> set[str]:
        return rtti_ancestors().get(cls, set())

    def _most_derived(self, names):
        if len(names) == 1:
            return next(iter(names))
        for cand in names:
            if names - {cand} <= self._ancestors(cand):
                return cand
        return None

    def rows(self):
        out = []
        for site in self.sites:
            size = self._pushed_size(site)
            if size is None:
                continue
            stamps, ctors, vector = self.attribute(site)
            own_stamps = {_cls_of(n) for _p, n, o in stamps if o}
            own_ctors = {_cls_of(n) for _p, n, o in ctors if o}
            nxt = self._next_site(site)
            near_stamps = [s for s in stamps if s[0] < nxt]
            near_ctors = [c for c in ctors if c[0] < nxt]
            if own_stamps:
                cls = self._most_derived(own_stamps | own_ctors)
                tier = "vtbl"
                if cls is None:
                    cls, tier = _cls_of([s for s in stamps if s[2]][-1][1]), \
                        "vtbl?"
            elif own_ctors:
                if len(own_ctors) != 1:
                    continue
                cls, tier = next(iter(own_ctors)), "ctor"
            elif near_stamps:
                cls = self._most_derived({_cls_of(n)
                                          for _p, n, _o in near_stamps})
                if cls is None:
                    continue
                tier = "vtbl?"
            elif near_ctors:
                names = {_cls_of(n) for _p, n, _o in near_ctors}
                if len(names) != 1:
                    continue
                cls, tier = next(iter(names)), "ctor?"
            else:
                continue
            own = self._owner(site)
            out.append((site, size, cls, tier,
                        self._name_of(own) if own else "?", vector))
        return out


_RTTI_ANC: dict[str, set[str]] | None = None


def rtti_ancestors() -> dict[str, set[str]]:
    """{class: ancestor names} from the image's RTTI base-class arrays."""
    global _RTTI_ANC
    if _RTTI_ANC is not None:
        return _RTTI_ANC
    from homm1.sema.image import retail
    from homm1.verify import vtable_scan as vs
    img = retail()
    base = img.base
    out: dict[str, set[str]] = {}
    for v in vs.scan():
        if not v["decorated"] or v["base_off"]:
            continue
        col_va = img.u32(v["start"] - 4)
        if col_va is None:
            continue
        chd = img.u32(col_va - base + 16)
        if chd is None:
            continue
        nbase = img.u32(chd - base + 8)
        bca = img.u32(chd - base + 12)
        if nbase is None or bca is None or not 0 < nbase < 64:
            continue
        anc = set()
        for i in range(nbase):
            bcd = img.u32(bca - base + 4 * i)
            if bcd is None:
                continue
            ptd = img.u32(bcd - base)
            if ptd is None:
                continue
            b = img.read(ptd - base + 8, 128)
            if not b:
                continue
            name = b.split(b"\0", 1)[0].decode("latin1")
            anc.add(vs.demangle(name).split("::")[-1])
        cls = vs.demangle(v["decorated"]).split("::")[-1]
        out[cls] = anc - {cls}
    _RTTI_ANC = out
    return out


# --------------------------------------------------------------------------- #
# the computed side: libclang record layouts over the compdb                  #
# --------------------------------------------------------------------------- #
def _tree_hash() -> str:
    h = hashlib.sha1()
    for root in ("src", "include"):
        for p in sorted((REPO / root).rglob("*")):
            if p.suffix in (".h", ".cpp", ".hpp", ".inl") and p.is_file():
                st = p.stat()
                h.update(f"{p}:{st.st_mtime_ns}:{st.st_size}\n".encode())
    return h.hexdigest()[:16]


def computed_sizes(rebuild: bool = False) -> tuple[dict[str, int], set[str]]:
    """({class: sizeof}, cross-TU-conflicting names), cached on a tree hash."""
    want = _tree_hash()
    if not rebuild and _CACHE.is_file():
        try:
            doc = json.loads(_CACHE.read_text())
            if doc.get("tree_hash") == want:
                return doc["sizes"], set(doc.get("conflicts", []))
        except (json.JSONDecodeError, KeyError):
            pass
    import clang.cindex as cidx

    from homm1.tool.clang import compdb
    db = compdb()
    sizes: dict[str, int] = {}
    conflicts: set[str] = set()
    index = cidx.Index.create()
    for src, flags in sorted(db.items()):
        args = ["--driver-mode=cl", *flags]
        try:
            tu = index.parse(src, args=args)
        except cidx.TranslationUnitLoadError:
            continue
        stack = [tu.cursor]
        while stack:
            node = stack.pop()
            for ch in node.get_children():
                if ch.kind in (cidx.CursorKind.NAMESPACE,
                               cidx.CursorKind.TRANSLATION_UNIT,
                               cidx.CursorKind.CLASS_DECL,
                               cidx.CursorKind.STRUCT_DECL,
                               cidx.CursorKind.UNION_DECL,
                               cidx.CursorKind.LINKAGE_SPEC):
                    stack.append(ch)
            if node.kind not in (cidx.CursorKind.CLASS_DECL,
                                 cidx.CursorKind.STRUCT_DECL):
                continue
            if not node.is_definition() or not node.spelling:
                continue
            sz = node.type.get_size()
            if sz is None or sz <= 0:
                continue
            name = node.spelling
            if name in sizes and sizes[name] != sz:
                conflicts.add(name)
            else:
                sizes.setdefault(name, sz)
    _CACHE.parent.mkdir(parents=True, exist_ok=True)
    _CACHE.write_text(json.dumps({"tree_hash": want, "sizes": sizes,
                                  "conflicts": sorted(conflicts)}))
    return sizes, conflicts


def def_counts() -> dict[str, int]:
    from homm1.verify.srcscan import iter_class_defs
    c: dict[str, int] = defaultdict(int)
    for name, _p, _l, _b in iter_class_defs():
        c[name] += 1
    return c


def classify_rows(rows, comp, conflicts, ndefs, include_ctor_tier=False):
    """(bad_comp, split, multi_def, unresolved, unmodelled, ok)."""
    strong, weak = defaultdict(set), defaultdict(set)
    for _site, size, cls, tier, _own, _vec in rows:
        (strong if tier.startswith("vtbl") else weak)[cls].add(size)
    bad, split, multi, unresolved, unmodelled, ok = [], [], [], [], [], []
    names = set(strong) | (set(weak) if include_ctor_tier else set())
    for cls in sorted(names):
        sizes = strong.get(cls) or weak.get(cls)
        tier = "vtbl" if cls in strong else "ctor"
        if len(sizes) > 1:
            split.append((cls, sorted(sizes), tier))
            continue
        n = next(iter(sizes))
        c = comp.get(cls)
        if cls not in ndefs:
            unmodelled.append((cls, n, tier))
        elif c is None:
            unresolved.append((cls, n, tier))
        elif c != n and (ndefs[cls] > 1 or cls in conflicts):
            multi.append((cls, n, c, tier, ndefs[cls]))
        elif c != n:
            bad.append((cls, n, c, tier))
        else:
            ok.append((cls, n, tier))
    return bad, split, multi, unresolved, unmodelled, ok


def gate_findings() -> list[str]:
    """The gate: a uniquely-defined class whose computed sizeof disagrees
    with retail's allocation immediate (the strong vtbl tier only)."""
    sw = Sweep()
    rows = sw.rows()
    comp, conflicts = computed_sizes()
    bad, _split, _multi, _unres, _unmod, _ok = classify_rows(
        rows, comp, conflicts, def_counts())
    out = [f"alloc-size: {cls} retail sizeof 0x{n:x} != computed 0x{c:x} "
           f"({c - n:+#x}) [{tier}] - reconstructed `new {cls}` emits the "
           f"wrong allocation immediate"
           for cls, n, c, tier in sorted(bad, key=lambda r: -abs(r[1] - r[2]))]
    if not comp:
        # No computed side at all: every retail site is "unresolved", so the
        # comparison cannot disagree. main() warns about this; the GATE must
        # too, or an empty libclang harvest silently reads as zero defects.
        out.append("alloc-size: the libclang record-layout harvest produced 0 "
                   "class sizes, so no retail allocation immediate had "
                   "anything to disagree with. Needs the compdb: run "
                   "`python3 -m homm1.graph.compdb` (or `homm1 build`).")
    return out


from homm1.core.usage import logged


@logged
def main(argv=None) -> int:
    import argparse
    ap = argparse.ArgumentParser(prog="homm1 verify alloc-size",
                                 description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--all", action="store_true",
                    help="list every attributed site, not just the disagreements")
    ap.add_argument("--sites", action="store_true",
                    help="dump the raw `push <n>; call ??2` sites and their attribution")
    ap.add_argument("--class", dest="klass",
                    help="restrict every listing to one class")
    ap.add_argument("--ctor-tier", action="store_true",
                    help="show the ctor-call attribution tier alongside the vptr tier")
    ap.add_argument("--rebuild-cache", action="store_true",
                    help="re-harvest the libclang record layouts (ignore the cache)")
    a = ap.parse_args(argv)

    sw = Sweep()
    rows = sw.rows()
    if a.klass:
        rows = [r for r in rows if r[2] == a.klass]
    if a.sites:
        for site, size, cls, tier, own, vec in rows:
            v = " [vector?]" if vec else ""
            print(f"0x{site:08x}  0x{size:<5x}  {tier:4}  {cls}{v}   <- {own}")
        return 0
    comp, conflicts = computed_sizes(rebuild=a.rebuild_cache)
    if not comp:
        print("[alloc-size] no computed sizes (libclang harvest empty?)")
    bad, split, multi, unresolved, unmodelled, ok = classify_rows(
        rows, comp, conflicts, def_counts(), a.ctor_tier)

    def hx(v):
        return "-" if v is None else f"0x{v:x}"

    if bad:
        print(f"\n=== WE EMIT THE WRONG sizeof ({len(bad)})")
        for cls, n, c, tier in sorted(bad, key=lambda r: -abs(r[1] - r[2])):
            print(f"  {cls:<34} retail {hx(n):>7}  computed {hx(c):>7} "
                  f"({c - n:+#x})  [{tier}]")
    if split:
        print(f"\n=== AMBIGUOUS ({len(split)}) - several sizes per class "
              f"(array new / misattributed chain / two classes)")
        for cls, sizes, tier in split:
            print(f"  {cls:<34} {', '.join(hx(s) for s in sizes)}  [{tier}]")
    if multi:
        print(f"\n=== MULTIPLE SOURCE DEFINITIONS ({len(multi)}) - one size "
              f"per name is not sound here")
        for cls, n, c, tier, count in multi:
            print(f"  {cls:<34} retail {hx(n):>7}  indexed {hx(c):>7} "
                  f"[{tier}, {count} defs]")
    if unresolved:
        print(f"\n=== SOURCE LAYOUT UNRESOLVED ({len(unresolved)})")
        for cls, n, tier in unresolved:
            print(f"  {cls:<34} retail {hx(n):>7}  [{tier}]")
    if unmodelled:
        print(f"\n=== UNMODELLED ({len(unmodelled)}) - retail news a class we "
              f"have no definition for")
        for cls, n, tier in unmodelled:
            print(f"  {cls:<34} retail {hx(n):>7}  [{tier}]")
    if a.all and ok:
        print(f"\n=== AGREES ({len(ok)})")
        for cls, n, tier in ok:
            print(f"  {cls:<34} 0x{n:x}  [{tier}]")
    print(f"\n{len(sw.sites)} operator-new sites, {len(rows)} attributed with "
          f"a constant size; wrong sizeof {len(bad)}, ambiguous {len(split)}, "
          f"multi-def {len(multi)}, unresolved {len(unresolved)}, "
          f"unmodelled {len(unmodelled)}, agree {len(ok)}")
    return 1 if bad else 0


if __name__ == "__main__":
    raise SystemExit(main())

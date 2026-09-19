"""homm1.verify.assert_relocs - the reloc-TARGET multiset audit (full tier).

Order-INDEPENDENT: for every NEAR-exact (>=99.5%) function, resolve the
base object's relocations to a MULTISET of final retail RVAs (symbol rva +
the addend read from the instruction bytes; REL32 = the symbol's own rva)
and compare with what retail's linked bytes actually reference across the
same extent. Works below 100%, where objdiff's aligned residue is hard to
read; also reports FABRICATED externals (a '?' symbol nothing defines - a
guaranteed unresolved external at link).

Ground truth is read, never guessed (the ported design): the retail-side
value comes from the IMAGE at each delinked reloc site's rva (DIR32 = the
stored VA, REL32 = site+4+disp), thunk-chased - so delinker naming артефacts
cannot enter. Base-side names resolve through the Model (winners + aliases,
static-libs and zlib labels included); `??_C@` string literals resolve by
CONTENT against the bytes at the retail-referenced address; COMMONs, weak
externals and own-obj definitions always link and are never FAKE.

    python3 -m homm1.verify.assert_relocs [--unit U] [0xRVA]
"""

from __future__ import annotations

import re
import struct
import sys
from collections import Counter

from homm1.core.paths import BUILD
from homm1.delink.coffx import Obj
from homm1.walls import pairscan
from homm1.walls.pairscan import DIR32, REL32, canon

THRESHOLD = 99.5
BASE_DIR = BUILD / "objdiff/base"
TARGET_DIR = BUILD / "objdiff/target-new"

_IMAGE_SYM_CLASS_EXTERNAL = 2
_IMAGE_SYM_CLASS_WEAK_EXTERNAL = 105


def defined_syms(obj: Obj) -> set[str]:
    """Every symbol the object ITSELF resolves: sectioned definitions, COFF
    COMMONs (section 0, non-zero value), and weak externals whose default
    resolves here (cl's ??_E -> ??_G alias)."""
    b = obj.buf
    out, weak, byidx = set(), [], {}
    i = 0
    while i < obj.nsym:
        base = obj.symptr + i * 18
        nm = obj.sym_name(i)
        val, sec = struct.unpack_from("<Ih", b, base + 8)
        scl, naux = struct.unpack_from("<BB", b, base + 16)
        byidx[i] = nm
        if sec > 0:
            out.add(nm)
        elif sec == 0 and val and scl == _IMAGE_SYM_CLASS_EXTERNAL:
            out.add(nm)
        elif scl == _IMAGE_SYM_CLASS_WEAK_EXTERNAL and naux:
            weak.append((nm, struct.unpack_from("<I", b, base + 18)[0]))
        i += 1 + naux
    for nm, tag in weak:
        if byidx.get(tag) in out:
            out.add(nm)
    return out


def _obj_strings(obj: Obj) -> dict[str, bytes]:
    """{??_C name: payload bytes} defined in this object."""
    out = {}
    for idx, value, secnum in obj.iter_symbols():
        name = obj.sym_name(idx)
        if name.startswith("??_C@") and secnum >= 1:
            cs = obj.cstring(secnum, value)
            if cs is not None:
                out[name] = cs
    return out


class Resolver:
    def __init__(self):
        from homm1.model import resolve
        from homm1.sema.image import retail
        self.img = retail()
        m = resolve()
        self.model = m
        names: dict[str, set[int]] = {}
        for b in m.functions + m.data:
            if b.name:
                names.setdefault(canon(b.name), set()).add(b.rva)
            for a in b.aliases:
                if a.name:
                    names.setdefault(canon(a.name), set()).add(b.rva)
        # underscore-tolerant lookups for C-decorated spellings
        self.names = names
        self.fn_extent = {b.rva: b.size for b in m.functions}
        self.fn_of_name = {b.name: b.rva for b in m.functions if b.name}
        self.units = {}
        for b in m.functions:
            if b.name and b.unit:
                self.units.setdefault(b.unit, {})[b.name] = b.rva

    def chase(self, rva: int, depth: int = 0) -> int:
        while depth < 4:
            nxt = self.img.jmp_target(rva)
            if nxt is None:
                return rva
            rva = nxt
            depth += 1
        return rva

    def rva_of(self, name: str) -> set[int]:
        c = canon(name)
        hits = self.names.get(c)
        if hits:
            return hits
        if c.startswith("_"):
            hits = self.names.get(c[1:])
            if hits:
                return hits
        return self.names.get("_" + c, set())

    def resolve_base(self, name: str, typ: int, addend: int) -> set[int]:
        rvas = self.rva_of(name)
        if not rvas:
            return set()
        if typ == REL32:
            return {self.chase(r) for r in rvas}
        return {self.chase((r + addend) & 0xFFFFFFFF) for r in rvas}

    def retail_value(self, site_rva: int, typ: int) -> int | None:
        if typ == REL32:
            b = self.img.read(site_rva, 4)
            if b is None:
                return None
            disp = struct.unpack("<i", b)[0]
            return self.chase((site_rva + 4 + disp) & 0xFFFFFFFF)
        v = self.img.u32(site_rva)
        if v is None or not (self.img.base <= v < self.img.base + 0x400000):
            return None
        return self.chase(v - self.img.base)


_CALLJMP = re.compile(r"^(?:call|jmp)$")


def _retail_rel32(resolver: Resolver, fn_rva: int, ext: int) -> list[int]:
    """Decoded call/jmp rel32 targets of the retail extent that leave the
    function (or re-enter at its head - recursion)."""
    from homm1.tool import objdump
    blob = resolver.img.read(fn_rva, ext)
    if not blob:
        return []
    out = []
    for line in objdump.disassemble(blob, vma=fn_rva).splitlines():
        if ":\t" not in line:
            continue
        asm = line.split("\t")[-1].strip()
        toks = asm.split(None, 1)
        if len(toks) != 2 or not _CALLJMP.match(toks[0]):
            continue
        op = toks[1].strip()
        if not re.fullmatch(r"0x[0-9a-f]+", op):
            continue
        t = int(op, 16)
        if t == fn_rva or not fn_rva <= t < fn_rva + ext:
            if resolver.img.is_text(t):
                out.append(t)
    return out


def _is_fake(name: str, defined: set[str], resolver: Resolver,
             libs: set[str]) -> bool:
    if not name.startswith("?") or name.startswith("??_C@") \
            or name.startswith(("$SG", "_$SG")):
        return False
    if name in defined:
        return False
    if resolver.rva_of(name):
        return False
    return not (name in libs or name.lstrip("_") in libs)


def audit(unit_filter=None, review_rva=None):
    from homm1.verify.undefined_closure import lib_symbols
    from homm1.walls.inventory import report_scores
    resolver = Resolver()
    libs = lib_symbols()
    _path, sc = report_scores()
    near: dict[str, list[str]] = {}
    for (u, sym), pct in sc.items():
        if pct >= THRESHOLD:
            near.setdefault(u, []).append(sym)
    findings, seen = [], 0
    for unit in sorted(near):
        if unit_filter and unit != unit_filter:
            continue
        base_p = BASE_DIR / f"{unit}.obj"
        if not base_p.is_file():
            continue
        try:
            bobj = Obj(base_p)
        except (ValueError, OSError):
            continue
        bf = pairscan.functions(bobj)
        bdef = defined_syms(bobj)
        strs = _obj_strings(bobj)
        for name in sorted(near[unit]):
            fn_rva = resolver.fn_of_name.get(name)
            bkey = name if name in bf else None
            if bkey is None or fn_rva is None:
                continue
            if review_rva is not None and fn_rva != review_rva:
                continue
            seen += 1
            # --- base multiset ------------------------------------------------
            bsec, bs, be = bf[bkey]
            bvas: Counter = Counter()
            va_sym: dict[int, str] = {}
            unresolved_strings: Counter = Counter()
            for _off, rname, typ, addend in pairscan.fn_relocs(bobj, bsec,
                                                               bs, be):
                if typ not in (DIR32, REL32):
                    continue
                if rname.startswith("__imp__"):
                    continue
                if _is_fake(rname, bdef, resolver, libs):
                    findings.append((unit, name, f"FAKE ref '{rname}' - "
                                     f"defined nowhere (base obj, Model, "
                                     f".LIBs) - an unresolved external"))
                    continue
                cands = resolver.resolve_base(rname, typ, addend)
                if not cands:
                    if rname in strs:
                        unresolved_strings[strs[rname]] += 1
                    continue
                if len(cands) == 1:
                    v = next(iter(cands))
                    bvas[v] += 1
                    va_sym.setdefault(v, rname)
                else:
                    # ambiguous (content-hashed collisions): accept any; the
                    # retail side decides which one it was
                    for v in cands:
                        va_sym.setdefault(v, rname)
                    bvas[sorted(cands)[0]] += 1
                    va_sym["__multi__%d" % id(cands)] = rname
            # --- retail multiset (the linked image itself) -------------------
            # DIR32: every .reloc HIGHLOW fixup inside the extent (this also
            # covers `call [IAT]` operands the delinker leaves reloc-less);
            # REL32: call/jmp targets decoded from the retail bytes - a
            # target inside the extent is a local branch (no reloc on the
            # base side either) unless it is the entry itself (recursion).
            ext = resolver.fn_extent.get(fn_rva, be - bs)
            tvas: Counter = Counter()
            for _site, tgt in resolver.img.relocs_in(fn_rva, fn_rva + ext):
                tvas[resolver.chase(tgt)] += 1
            for t in _retail_rel32(resolver, fn_rva, ext):
                tvas[resolver.chase(t)] += 1
            # cancel base ??_C references by CONTENT against retail values
            extra_t = tvas - bvas
            for content, k in unresolved_strings.items():
                for v in list(extra_t):
                    got = resolver.img.read(v, len(content))
                    if got == content:
                        take = min(k, extra_t[v])
                        extra_t[v] -= take
                        tvas[v] -= take
                        k -= take
                    if k <= 0:
                        break
            for v, n in (bvas - tvas).items():
                bs_name = va_sym.get(v, "?")
                if "_00A@" in bs_name:
                    continue
                # ambiguous multi-candidate names: cleared if ANY candidate
                # is retail-referenced
                cands = resolver.rva_of(bs_name)
                if len(cands) > 1 and any(tvas.get(c, 0) for c in cands):
                    continue
                findings.append((unit, name,
                                 f"WRONG: base references 0x{v:x} "
                                 f"({bs_name}) x{n} - retail never does "
                                 f"(or fewer)"))
    return findings, seen


def gate_findings() -> list[str]:
    findings, seen = audit()
    out = [f"assert-relocs [{u}] {n[:60]}: {p}" for u, n, p in findings]
    if not seen:
        # 0 audited functions and 0 defects read exactly like a clean tree.
        out.append(f"assert-relocs: 0 function(s) reached the >={THRESHOLD}% "
                   f"threshold, so NOTHING was audited - an unbuilt tree or a "
                   f"broken report/Model join, never a pass. Run "
                   f"`homm1 build` and re-run.")
    return out


def main(argv=None) -> int:
    import argparse
    ap = argparse.ArgumentParser(prog="homm1 verify assert-relocs",
                                 description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("rva", nargs="?", help="review ONE function at this rva")
    ap.add_argument("--unit",
                    help="audit one unit only")
    a = ap.parse_args(argv)
    review = int(a.rva, 16) if a.rva else None
    findings, seen = audit(a.unit, review)
    for u, n, p in findings:
        print(f"  {u:<22} {n[:46]:<46} {p}")
    fake = sum(1 for _u, _n, p in findings if p.startswith("FAKE"))
    print(f"\nassert-relocs: {seen} near-exact fn(s) audited "
          f"(>={THRESHOLD}%), {len(findings)} defect(s) [{fake} FAKE, "
          f"{len(findings) - fake} WRONG]")
    if findings:
        print("A FAKE ref is a symbol nothing DEFINES; a WRONG row points at "
              "an address retail never references from this body.",
              file=sys.stderr)
        return 1
    print("relocs OK: every near-exact function's targets resolve to the "
          "retail address multiset.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

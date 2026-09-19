"""homm1.verify.vtables - the vtable tier (full): coverage, virtuality,
slot binding.

  COVERAGE     every REAL vtable the image scan discovers (verify.vtable_scan:
               RTTI + code-ref starts, MI secondaries included) is admitted -
               a `vtable`-kind census row claimed by data_vtables /
               data_static_libs, or interior to another admitted claim (a
               fn-ptr table like zlib's configuration_table). Anything else
               is a GAME vtable with no binding: FATAL.
  VIRTUALITY   every primary game vtable's class is REAL: defined in source,
               and its resolved virtual count (own + transitive bases, MFC
               bases credited) covers the vtable's slot count. A fabricated
               name or a de-virtualized shell is FATAL. A catalogued rva the
               scan finds no vtable at is UNVERIFIABLE - reported, never
               asserted (the old default-to-1 guess made the check vacuous).
  SLOT BINDING every slot's bound body is a `virtual` of the class or its
               base closure - read off the MANGLED name's access code (the
               compiler's own word), never re-parsed from source. WIRING
               (bound as a non-virtual) and MISBOUND (foreign class) are
               FATAL; UNBOUND (unreconstructed) is info. The old 259-row
               baseline reached 0 and was DELETED - the gate is pure
               fail-closed (an absent baseline is the empty set, permanently).

    homm1 verify vtables            # the three checks, exit 1 on any
    homm1 verify vtables --info     # + every UNBOUND slot
"""

from __future__ import annotations

import re
import sys

_PRIMARY_RE = re.compile(r"^\?\?_7([A-Za-z_]\w*)@@6B@$")

# MSVC access+storage codes at the char after the qualifier-list `@@`.
VIRTUAL = set("EFMNUV")
VTHUNK = set("GHOPWX")
NONVIRT_METHOD = set("ABIJQR")
STATIC_METHOD = set("CDKLST")
FREE_FN = set("YZ")
_KIND = [(VIRTUAL, "virtual"), (VTHUNK, "virtual-thunk"),
         (NONVIRT_METHOD, "non-virtual"), (STATIC_METHOD, "static"),
         (FREE_FN, "free function")]
PURE = ("__purecall", "___purecall", "_purecall")

# Library bases: slot credit + ancestry for the closure test.
LIB_BASE_SLOTS = {"CObject": 5, "CCmdTarget": 8, "CWnd": 100, "CDialog": 104,
                  "CFile": 12, "CException": 4, "CGdiObject": 4}
LIB_BASES = {"CCmdTarget": ["CObject"], "CWnd": ["CCmdTarget"],
             "CDialog": ["CWnd"]}


def split_mangled(sym: str):
    """sym -> (name, [qualifiers], storage_char) or None for a C symbol."""
    if not sym.startswith("?"):
        return None
    body = sym[1:]
    if body.startswith("?"):
        rest = body[1:]
        if not rest:
            return None
        width = 2 if rest[0] == "_" else 1
        name, rest = "??" + rest[:width], "@" + rest[width:]
    else:
        i = body.find("@")
        if i < 0:
            return None
        name, rest = body[:i], body[i:]
    j = rest.find("@@")
    if j < 0:
        return None
    quals = [q for q in rest[:j].split("@") if q]
    tail = rest[j + 2:]
    return name, quals, (tail[0] if tail else "")


def classify_storage(storage: str) -> str:
    for codes, label in _KIND:
        if storage in codes:
            return label
    return "unknown"


def _model_rows():
    """(game {rva: class}, lib_rvas, claimed_spans, fn_syms {rva: (name, unit)})."""
    from homm1.model import resolve
    m = resolve()
    game: dict[int, str] = {}
    lib: set[int] = set()
    spans: list[tuple[int, int]] = []
    for b in m.data:
        if b.channel:
            spans.append((b.rva, b.rva + max(b.size, 1)))
        if b.kind != "vtable":
            continue
        if b.channel == "data_vtables":
            mm = _PRIMARY_RE.match(b.name)
            if mm:
                game[b.rva] = mm.group(1)
            else:
                game[b.rva] = ""          # secondary/template: covered, unnamed
        elif b.channel == "data_static_libs":
            lib.add(b.rva)
    syms = {b.rva: (b.name, b.unit) for b in m.functions
            if b.name and (b.channel.startswith("src")
                           or b.channel == "functions_zlib")}
    spans.sort()
    return game, lib, spans, syms


def _inside_claim(rva: int, spans) -> bool:
    import bisect
    i = bisect.bisect_right(spans, (rva, 1 << 62)) - 1
    return i >= 0 and spans[i][0] <= rva < spans[i][1]


def base_closure(name, classes, seen=None):
    if seen is None:
        seen = set()
    if name in seen:
        return seen
    seen.add(name)
    for b in LIB_BASES.get(name, ()):
        base_closure(b, classes, seen)
    for b in classes.get(name, (0, []))[1]:
        base_closure(b, classes, seen)
    return seen


def resolved_virtuals(name, classes, seen=None):
    if seen is None:
        seen = set()
    if name in seen:
        return 0
    seen.add(name)
    if name in LIB_BASE_SLOTS:
        return LIB_BASE_SLOTS[name]
    if name not in classes:
        return 0
    own, bases = classes[name]
    return own + sum(resolved_virtuals(b, classes, seen) for b in bases)


def resolve_slot(syms, raw, body):
    """The bound symbol for a slot: the RAW slot target first (honours a
    genuine tail-call virtual), the thunk-chased body second."""
    for r in (raw, body):
        if r in syms:
            return (r, *syms[r])
    return None


def analyse():
    """(coverage_gaps, virtuality_violations, unverifiable, wiring_violations,
    unbound, n_vt, n_slots)."""
    from homm1.verify import vtable_scan as vs
    from homm1.verify.srcscan import index_classes
    game, lib, spans, syms = _model_rows()
    classes = index_classes()

    gaps = []
    for v in vs.real_vtables():
        rva = v["start"]
        if rva in game or rva in lib or _inside_claim(rva, spans):
            continue
        gaps.append((rva, v["size"], v["conf"], v["rtti"] or "",
                     v["base_off"] or 0))

    virt, unverifiable = [], []
    wiring, unbound = [], []
    n_vt = n_slots = 0
    for rva, cls in sorted(game.items()):
        if not cls or rva in lib:
            continue
        vt = vs.vtable_at(rva)
        if vt is None:
            unverifiable.append((cls, rva))
            continue
        n_vt += 1
        n_slots += vt["size"]
        if cls not in classes:
            virt.append((cls, rva, vt["size"], 0,
                         "no class definition (fabricated name)"))
        else:
            nv = resolved_virtuals(cls, classes)
            if nv < vt["size"]:
                virt.append((cls, rva, vt["size"], nv,
                             "under-virtualized (slots not backed by "
                             "virtuals)"))
        allowed = base_closure(cls, classes)
        for k, _sr, raw, body in vs.iter_slots(vt):
            hit = resolve_slot(syms, raw, body)
            if hit is None:
                unbound.append((rva, cls, k, body))
                continue
            r, sym, unit = hit
            if sym in PURE:
                continue
            parts = split_mangled(sym)
            if parts is None:
                wiring.append(("WIRING", rva, cls, k, r, sym, unit,
                               "C-linkage symbol at a vtable slot"))
                continue
            _name, quals, storage = parts
            kind = classify_storage(storage)
            if kind not in ("virtual", "virtual-thunk"):
                wiring.append(("WIRING", rva, cls, k, r, sym, unit,
                               f"bound as a {kind}, not a virtual"))
                continue
            owner = quals[0] if quals else "?"
            if owner not in allowed:
                wiring.append(("MISBOUND", rva, cls, k, r, sym, unit,
                               f"virtual of {owner}, not in {cls}'s base "
                               f"closure"))
    return gaps, virt, unverifiable, wiring, unbound, n_vt, n_slots


def gate_findings() -> list[str]:
    gaps, virt, _unv, wiring, _unb, _nv, _ns = analyse()
    out = []
    for rva, size, conf, cls, boff in sorted(gaps):
        sec = f" +{boff} (SECONDARY)" if boff else ""
        out.append(f"vtable-coverage: 0x{rva:06x} sz={size} {conf} "
                   f"{cls or '(non-rtti)'}{sec} UNCOVERED - add the row to "
                   f"data_vtables.tsv or data_static_libs.tsv")
    for cls, rva, slots, nv, why in sorted(virt, key=lambda v: v[1]):
        out.append(f"vtable-virtuality: 0x{rva:06x} {cls} slots={slots} "
                   f"virtuals={nv} {why}")
    for kind, rva_v, cls, k, r, sym, unit, why in wiring:
        out.append(f"vtable-slot-binding [{kind}]: 0x{r:06x} {cls}[{k}] "
                   f"{sym} ({unit}) - {why} (vtable 0x{rva_v:06x})")
    return out


def main(argv=None) -> int:
    import argparse
    ap = argparse.ArgumentParser(prog="homm1 verify vtables",
                                 description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    # (there was a `--list` here that nothing ever read: it parsed, printed
    # nothing extra, and exited 0 - removed rather than left as a silent flag)
    ap.add_argument("--info", action="store_true",
                    help="also list the UNBOUND (unreconstructed) slots")
    a = ap.parse_args(argv)
    gaps, virt, unv, wiring, unbound, n_vt, n_slots = analyse()
    bad = gate_findings()
    for b in bad:
        print("  " + b, file=sys.stderr)
    if a.info and unbound:
        print(f"{len(unbound)} slot(s) UNBOUND (unreconstructed - info only):")
        for rva_v, cls, k, r in unbound[:40]:
            print(f"  0x{r:06x}  {cls}[{k}]  (vtable 0x{rva_v:06x})")
        if len(unbound) > 40:
            print(f"  ... {len(unbound) - 40} more")
    if bad:
        print(f"vtables: FATAL - {len(gaps)} uncovered, {len(virt)} "
              f"virtuality, {len(wiring)} wiring finding(s)", file=sys.stderr)
        return 1
    print(f"vtables: OK - coverage complete; {n_vt} primary game vtable(s) / "
          f"{n_slots} slot(s) modelled by real virtuals and wired to them "
          f"({len(unbound)} unbound/unreconstructed"
          + (f"; {len(unv)} catalogued rva(s) unverifiable - the scan found "
             f"no vtable there" if unv else "") + ")")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

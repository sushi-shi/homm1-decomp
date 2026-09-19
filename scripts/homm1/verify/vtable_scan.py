"""homm1.verify.vtable_scan - every vtable in HOMM1.EXE, with exact sizes.

Three independent retail signals (ported; rebuilt on homm1.sema.image):
  1. a maximal stride-4 run of DATA-section .reloc sites whose stored values
     all point into .text is a band of one-or-more adjacent vtables;
  2. /GR RTTI: `vtable-4` holds a Complete Object Locator pointer - CONFIRMS
     a start, names it, and carries the MI base offset;
  3. a CODE DIR32 onto a run member is a ctor/dtor vptr stamp - a real start
     even without RTTI. `call/jmp ds:[slot]` (FF15/FF25 devirtualised calls)
     are excluded - they point mid-vtable and fabricate phantom starts.

Cutting each run at {COL starts} u {code-referenced starts} de-merges
adjacent vtables and yields exact per-vtable entry counts. Confidence:
`rtti` / `code-ref` (>=2 slots, .rdata) / `code-ref-weak` / `unref` (run
head with no signal: EH/jump tables, NOT a vtable).

    python3 -m homm1.verify.vtable_scan [--new] [--holds 0xRVA] [--dump 0xRVA]
"""

from __future__ import annotations

import re
from functools import lru_cache


def demangle(s: str) -> str:
    for pre in (".?AV", ".?AU", ".?AW", ".?AT"):
        if s.startswith(pre):
            s = s[len(pre):]
            break
    if s.endswith("@@"):
        s = s[:-2]
    parts = [x for x in s.split("@") if x]
    return "::".join(reversed(parts)) if parts else s


def vftable_name(decorated: str) -> str:
    """'.?AVCFoo@@' -> '??_7CFoo@@6B@' (base_off==0 primary tables only)."""
    s = decorated
    for pre in (".?AV", ".?AU", ".?AW", ".?AT"):
        if s.startswith(pre):
            s = s[len(pre):]
            break
    return "??_7" + s + "6B@"


@lru_cache(maxsize=1)
def scan() -> list[dict]:
    """The discovered vtable list (each: start, size, sec, rtti, decorated,
    base_off, code_refs, head_of_run, first, conf)."""
    from homm1.sema.image import retail
    img = retail()
    base = img.base

    # --- RTTI: TypeDescriptors + COLs ---------------------------------------
    td: dict[int, str] = {}          # td VA -> decorated name
    data = img.pe.data
    for m in re.finditer(rb"\.\?A[VUWT][\w@?$]+@@\x00", data):
        so = m.start()
        rva = None
        for s in img.pe.sections:
            if s["rptr"] <= so < s["rptr"] + s["rsize"]:
                rva = so - s["rptr"] + s["va"]
                break
        if rva is None:
            continue
        td[(rva - 8) + base] = m.group(0)[:-1].decode("latin1")
    col: dict[int, tuple[str, int]] = {}
    rd = img.pe.section(".rdata")
    for a in range(rd["va"], rd["va"] + rd["rsize"] - 20, 4):
        if img.u32(a) != 0:
            continue
        ptd = img.u32(a + 12)
        if ptd in td:
            col[a + base] = (td[ptd], img.u32(a + 4))

    # --- data-section code-pointer runs -------------------------------------
    reloc = img.reloc
    relset = set(reloc)
    sites = sorted(s for s in reloc if not img.is_text(s))
    runs: list[tuple[int, int]] = []
    i, n = 0, len(sites)
    while i < n:
        s0 = sites[i]
        if not img.is_text(reloc[s0]):
            i += 1
            continue
        last, j = s0, i + 1
        while j < n and sites[j] == last + 4 and img.is_text(reloc[sites[j]]):
            last = sites[j]
            j += 1
        runs.append((s0, last + 4))
        i = j if j > i else i + 1
    members: set[int] = set()
    for lo, hi in runs:
        members.update(range(lo, hi, 4))

    # --- code-referenced starts (vptr stamps) -------------------------------
    code_ref: dict[int, int] = {}
    for s, t in reloc.items():
        if not img.is_text(s) or t not in members:
            continue
        b = img.read(s - 2, 2)
        if b and b[0] == 0xFF and b[1] in (0x15, 0x25):
            continue                      # devirtualised call through a slot
        code_ref[t] = code_ref.get(t, 0) + 1

    col_start: dict[int, tuple[str, int]] = {}
    for lo, hi in runs:
        pv = img.u32(lo - 4) if (lo - 4) in relset else None
        if pv in col:
            col_start[lo] = col[pv]
    for a in members:
        if (a - 4) in relset:
            pv = img.u32(a - 4)
            if pv in col:
                col_start.setdefault(a, col[pv])

    known = set(col_start) | set(code_ref)
    out: list[dict] = []
    for lo, hi in runs:
        cuts = sorted(a for a in known if lo <= a < hi)
        if not cuts or cuts[0] != lo:
            cuts = [lo] + cuts
        cuts = sorted(set(cuts))
        for k, st in enumerate(cuts):
            en = cuts[k + 1] if k + 1 < len(cuts) else hi
            c = col_start.get(st)
            v = dict(start=st, size=(en - st) // 4,
                     sec=img.section_name(st),
                     rtti=demangle(c[0]) if c else None,
                     decorated=c[0] if c else None,
                     base_off=c[1] if c else None,
                     code_refs=code_ref.get(st, 0),
                     head_of_run=(st == lo),
                     first=img.u32(st) - base)
            v["conf"] = confidence(v)
            out.append(v)
    return out


def confidence(v: dict) -> str:
    if v["rtti"]:
        return "rtti"
    if v["code_refs"] and v["size"] >= 2 and v["sec"] == ".rdata":
        return "code-ref"
    if v["code_refs"]:
        return "code-ref-weak"
    return "unref"


REAL_CONF = {"rtti", "code-ref", "code-ref-weak"}


def real_vtables() -> list[dict]:
    return [v for v in scan() if v["conf"] in REAL_CONF]


def vtable_at(start: int) -> dict | None:
    from homm1.sema.image import retail
    if start >= retail().base:
        start -= retail().base
    return next((v for v in scan() if v["start"] == start), None)


def iter_slots(v: dict):
    """(slot_index, slot_rva, raw_target_rva, body_rva) - ILT thunks chased."""
    from homm1.sema.image import retail
    img = retail()
    for k in range(v["size"]):
        sr = v["start"] + k * 4
        w = img.u32(sr)
        if w is None:
            continue
        raw = w - img.base
        body = img.jmp_target(raw) if img.is_text(raw) else None
        yield k, sr, raw, (raw if body is None else body)


def _fn_label(rva: int) -> str:
    from homm1.model import resolve
    import bisect
    m = resolve()
    starts = [b.rva for b in m.functions]
    i = bisect.bisect_right(starts, rva) - 1
    if i >= 0:
        b = m.functions[i]
        if b.rva <= rva < b.rva + b.size:
            nm = b.name or f"sub_{b.rva:06x}"
            return nm if rva == b.rva else f"{nm}+0x{rva - b.rva:x}"
    return f"sub_{rva:06x}"


from homm1.core.usage import logged


@logged
def main(argv=None) -> int:
    import argparse
    ap = argparse.ArgumentParser(prog="homm1 verify vtable-scan",
                                 description=__doc__)
    ap.add_argument("--new", action="store_true",
                    help="only starts with no admitted vtable/static-libs row")
    ap.add_argument("--holds", help="which vtable slot(s) resolve to this fn")
    ap.add_argument("--dump", help="the slots of the vtable at this start")
    a = ap.parse_args(argv)

    if a.holds:
        fn = int(a.holds, 16)
        hits = [(v, k, raw != fn) for v in scan()
                if v["sec"] in (".rdata", ".data")
                for k, _sr, raw, body in iter_slots(v) if body == fn]
        print(f"# vtable slots holding 0x{fn:06x} {_fn_label(fn)}")
        if not hits:
            print("  (none)")
            return 1
        for v, k, via in hits:
            cls = v["rtti"] or f"non-rtti {_fn_label(v['first'])}"
            print(f"  vtable 0x{v['start']:06x}  {v['conf']:<9} slot[{k}] "
                  f"(+0x{k * 4:x}){'  via thunk' if via else ''}   {cls}")
        return 0

    if a.dump:
        vt = vtable_at(int(a.dump, 16))
        if vt is None:
            print("# no discovered vtable starts there")
            return 1
        cls = vt["rtti"] or f"non-rtti (first: {_fn_label(vt['first'])})"
        print(f"# vtable 0x{vt['start']:06x}  {vt['conf']}  size={vt['size']}"
              f"  code-refs={vt['code_refs']}  {cls}")
        for k, _sr, raw, body in iter_slots(vt):
            via = f"  (via thunk 0x{raw:x})" if body != raw else ""
            print(f"  slot[{k:2}] (+0x{k * 4:02x}): 0x{body:06x} "
                  f"{_fn_label(body)}{via}")
        return 0

    vts = scan()
    real = [v for v in vts if v["conf"] in REAL_CONF]
    from homm1.model import resolve
    admitted = {b.rva for b in resolve().data if b.kind == "vtable"}
    show = [v for v in real if v["start"] not in admitted] if a.new else real
    n_rtti = sum(1 for v in real if v["conf"] == "rtti")
    print(f"# VTABLES: {len(real)} real ({n_rtti} rtti), "
          f"{len(vts) - len(real)} unref run-heads excluded; "
          f"{len(real) - len([v for v in real if v['start'] not in admitted])}"
          f" admitted, {len([v for v in real if v['start'] not in admitted])}"
          f" NOT admitted")
    for v in sorted(show, key=lambda v: v["start"]):
        cls = v["rtti"] or _fn_label(v["first"])
        bo = f" [base+{v['base_off']}]" if v["base_off"] else ""
        mark = "NEW" if v["start"] not in admitted else "ok "
        print(f"  0x{v['start']:06x} {v['sec']:<6} {v['size']:>3} "
              f"{v['conf']:<13} {mark} {cls}{bo}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

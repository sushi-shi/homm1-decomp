"""homm1.walls.eh_frame - the /GX frame-presence + unwind-state sieve.

`/GX` is project-wide (741 retail EH functions across 186 TUs), and cl 5.0
emits the registration frame IFF the function owns an object whose dtor must
run during unwind - so a presence DISAGREEMENT between the normalized pair
is a hard SOURCE fact, and every row is tagged with its CAUSE:

  INLINE_CUT      one side CALLS a ctor/dtor COMDAT the other never calls -
                  same object, different inline cut (per new-site; a census
                  entry, not a worklist item).
  EXIT_MERGE      the SAME ctor/dtor, a different NUMBER of call sites (exit
                  blocks merged under a ||/&& guard).
  STATE_FLOW      both sides have an EH frame and the same ctor/dtor call set,
                  but state stores were duplicated or placed differently;
                  inspect CFG/lifetimes, do not infer another object.
  MISSING_OBJECT  retail owns a destructible object our source never
                  declared, proven by a retail-only EH frame.
  EXTRA_OBJECT    the mirror - we invented one.

The secondary sieve (--states) compares the unwind-STATE store counts where
BOTH sides carry a frame - roughly the number of destructible objects whose
lifetimes overlap a call. --calibrate measures both signals on the 100.00%
functions (byte-identical, must agree; any row there is a detector bug).

The detector is the ported SEQUENCE (never one instruction): `mov fs:[0],esp`
+ `push -1` + the fs:[0] load inside the prologue window; state slots from
the prologue-formula and teardown anchors, one arg-push region of slack.

    homm1 walls eh-frame [--states] [--calibrate] [--unit U] [--tsv OUT]
"""

from __future__ import annotations

from homm1.core.usage import logged

import argparse
import re
import sys
from collections import Counter

from homm1.delink.coffx import Obj
from homm1.walls import pairscan
from homm1.walls.pairscan import REL32

PROLOGUE_WINDOW = 14
ARG_DEPTH = 8
SAVED = ("ebx", "ebp", "esi", "edi")

_FS0 = r"(?:DWORD PTR )?fs:0x?0"
FS_STORE = re.compile(rf"^{_FS0},esp$")
FS_LOAD = re.compile(rf"^e[a-z][a-z],{_FS0}$")
FS_RESTORE = re.compile(rf"^{_FS0},(?!esp$)")
PUSH_M1 = re.compile(r"^0xffffffff$")
SUB_ESP = re.compile(r"^esp,(0x[0-9a-f]+)$")
IMM = re.compile(r"^0x[0-9a-f]+$")
REG = re.compile(r"^[a-z]{2,3}$")
ESP_MEM = re.compile(r"^\[esp\+(0x[0-9a-f]+)\]$")
CTOR_DTOR = re.compile(r"^\?\?(?:[01]|_[DEGHI])")


def _split_ops(ops: str) -> tuple[str, str]:
    """Intel `dst,src` split at the comma OUTSIDE brackets."""
    depth = 0
    for i, ch in enumerate(ops):
        if ch == "[":
            depth += 1
        elif ch == "]":
            depth -= 1
        elif ch == "," and depth == 0:
            return ops[:i].strip(), ops[i + 1:].strip()
    return ops.strip(), ""


def _strip_size(op: str) -> str:
    return re.sub(r"^(?:BYTE|WORD|DWORD|QWORD) PTR ", "", op)


def has_eh(insns) -> bool:
    head = insns[:PROLOGUE_WINDOW]
    store = load = push = False
    for _o, mn, ops in head:
        if mn.startswith("mov"):
            if FS_STORE.match(ops):
                store = True
            elif FS_LOAD.match(ops):
                load = True
        elif mn.startswith("push") and PUSH_M1.match(ops.strip()):
            push = True
    return store and load and push


def has_unwind(insns) -> bool:
    return any(mn.startswith("mov") and FS_RESTORE.match(ops)
               and not FS_STORE.match(ops)
               for _o, mn, ops in insns[PROLOGUE_WINDOW:])


def framed(insns) -> bool:
    head = insns[:PROLOGUE_WINDOW + 8]
    for i, (_o, mn, ops) in enumerate(head):
        if mn.startswith("push") and ops.strip() == "ebp" and i + 1 < len(head):
            nxt = head[i + 1]
            if nxt[1].startswith("mov") and nxt[2].replace(" ", "") == "ebp,esp":
                return True
    return False


def prologue_slot(insns) -> int:
    """`sub K + 4*saves + 8` past the fs store - the state's frame-base slot."""
    k, saves, seen = 0, 0, False
    for _o, mn, ops in insns[:PROLOGUE_WINDOW + 10]:
        if mn.startswith("mov") and FS_STORE.match(ops):
            seen = True
            continue
        if not seen:
            continue
        m = SUB_ESP.match(ops.replace(" ", "")) if mn.startswith("sub") else None
        if m:
            k += int(m.group(1), 16)
        elif mn.startswith("push") and ops.strip() in SAVED:
            saves += 1
        elif mn.startswith(("j", "call", "ret", "loop")):
            break
    return k + 4 * saves + 8


def teardown_slots(insns) -> set[int]:
    """`D + 8 + 4*pops` per unwind teardown (pops between the load and the
    fs:[0] store added back - cl interleaves them freely)."""
    out = set()
    for i, (_o, mn, ops) in enumerate(insns):
        if not (mn.startswith("mov") and FS_RESTORE.match(ops)
                and not FS_STORE.match(ops)):
            continue
        _dst, reg = _split_ops(ops)
        m, pops, load = None, 0, None
        for j in range(i - 1, max(-1, i - 24), -1):
            _oj, mj, oj = insns[j]
            if mj.startswith(("call", "j", "ret", "loop")):
                break
            if mj.startswith("pop"):
                pops += 1 if load is not None else 0
                continue
            if load is None and mj.startswith("mov"):
                djst, sj = _split_ops(oj)
                if djst.strip() == reg:
                    m = ESP_MEM.match(_strip_size(sj).replace(" ", ""))
                    load = j
                    if m is None:
                        break
        if load is not None and m is not None:
            d = int(m.group(1), 16) + 8 + 4 * pops
            if d >= 0:
                out.add(d)
    return out


def eh_states(insns):
    """(slot, [(offset, state|None)]) - stores into the unwind-state slot.
    A heuristic lead (the ported two-anchor + arg-push-slack rule), not the
    exact signal presence is; COUNT is exact, the value set a lower bound."""
    if framed(insns):
        cands = {"[ebp-0x4]": {"[ebp-0x4]"}}
    else:
        cands = {}
        for d in {prologue_slot(insns)} | teardown_slots(insns):
            cands[f"[esp+0x{d:x}]"] = {f"[esp+0x{d + 4 * k:x}]"
                                       for k in range(1, ARG_DEPTH + 1)}
    hits = {c: [] for c in cands}
    for off, mn, ops in insns:
        if not mn.startswith("mov") or "," not in ops:
            continue
        dst_raw, src = _split_ops(ops)
        byte_wide = dst_raw.startswith("BYTE PTR")
        dst = _strip_size(dst_raw).replace(" ", "")
        src = src.strip()
        imm = int(src, 16) if IMM.match(src) else None
        val = None if imm is None else \
            (-1 if imm in (0xFFFFFFFF, 0xFF) else imm)
        for c, deep in cands.items():
            if dst == c:
                if imm is not None:
                    hits[c].append((off, val))
                elif REG.match(src):
                    hits[c].append((off, None))
            elif dst in deep and (imm is not None or byte_wide):
                if imm is not None and not (-1 <= (val if val is not None
                                                   else imm) <= 0x40):
                    continue
                hits[c].append((off, val))
    if not any(hits.values()):
        return (sorted(cands)[-1] if cands else "-"), []
    slot = max(hits, key=lambda d: (len(hits[d]), d))
    return slot, sorted(hits[slot])


def rel32_calls(obj: Obj, fns) -> dict[str, list]:
    """{symbol: [(offset, callee)]} per function window."""
    out = {}
    for sym, (secnum, lo, hi) in fns.items():
        out[sym] = [(off, nm)
                    for off, nm, ty, _a in pairscan.fn_relocs(obj, secnum, lo, hi)
                    if ty == REL32]
    return out


def ctor_delta(base_calls, tgt_calls):
    b = Counter(n for _o, n in base_calls if CTOR_DTOR.match(n))
    t = Counter(n for _o, n in tgt_calls if CTOR_DTOR.match(n))
    shared = {n for n in set(b) & set(t) if b[n] != t[n]}
    return (sorted(set(t) - set(b)), sorted(set(b) - set(t)),
            sorted(f"{n} x{b[n]}/{t[n]}" for n in shared))


def cause(verdict, delta, only_t, only_b, resited):
    retail_side = verdict == "TARGET_ONLY" or (verdict == "BOTH" and delta > 0)
    if only_t or only_b:
        return "INLINE_CUT"
    if resited:
        return "EXIT_MERGE"
    if verdict == "BOTH":
        return "STATE_FLOW"
    return "MISSING_OBJECT" if retail_side else "EXTRA_OBJECT"


def classify(base_ins, tgt_ins):
    be, te = has_eh(base_ins), has_eh(tgt_ins)
    if be and not te:
        return "BASE_ONLY"
    if te and not be:
        return "TARGET_ONLY"
    return "BOTH" if be else "NEITHER"


def scan(units=None):
    from homm1.model import resolve
    sc, _live = pairscan.scores()
    rvas = {b.name: b.rva for b in resolve().functions if b.name}
    rows = []
    all_pairs = pairscan.pairs(units)
    by_unit: dict[str, list] = {}
    for (u, sym), pct in sc.items():
        by_unit.setdefault(u, []).append((sym, pct))
    for unit, fns_scored in sorted(by_unit.items()):
        pair = all_pairs.get(unit)
        if not pair:
            continue
        try:
            bobj, tobj = Obj(pair[0]), Obj(pair[1])
        except (ValueError, OSError):
            continue
        bf, tf = pairscan.functions(bobj), pairscan.functions(tobj)
        bc, tc = rel32_calls(bobj, bf), rel32_calls(tobj, tf)
        for sym, pct in fns_scored:
            if sym not in bf or sym not in tf:
                continue
            bi = pairscan.insns(bobj, *bf[sym])
            ti = pairscan.insns(tobj, *tf[sym])
            if not bi or not ti:
                continue
            bslot, bst = eh_states(bi) if has_eh(bi) else ("", [])
            tslot, tst = eh_states(ti) if has_eh(ti) else ("", [])
            src, slot, states = ((ti, tslot, tst) if has_eh(ti)
                                 else (bi, bslot, bst))
            only_t, only_b, resited = ctor_delta(bc.get(sym, []),
                                                 tc.get(sym, []))
            verdict = classify(bi, ti)
            why = cause(verdict, len(tst) - len(bst), only_t, only_b, resited)
            rva = rvas.get(sym)
            rows.append(dict(
                unit=unit, name=sym,
                rva=f"0x{rva:06x}" if rva is not None else "",
                fuzzy=pct, size=bf[sym][2] - bf[sym][1],
                verdict=verdict, cause=why,
                extra_ctors=only_t, our_ctors=only_b, resited=resited,
                base_insn=len(bi), tgt_insn=len(ti), slot=slot,
                states=sorted({s for _o, s in states if s is not None}),
                base_states=len(bst), tgt_states=len(tst),
                first=states[0][0] if states else None,
                last=states[-1][0] if states else None,
                unwind=has_unwind(src),
            ))
    return rows


def calibrate(rows):
    exact = [r for r in rows if r["fuzzy"] >= 100.0]
    bad = [r for r in exact if r["verdict"] in ("BASE_ONLY", "TARGET_ONLY")]
    eh = [r for r in exact if r["verdict"] == "BOTH"]
    sbad = [r for r in eh if r["base_states"] != r["tgt_states"]]
    return exact, bad, eh, sbad


@logged
def main(argv=None):
    ap = argparse.ArgumentParser(prog="homm1 walls eh-frame",
                                 description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--unit", action="append",
                    help="restrict to a unit of config/units.toml (repeatable)")
    ap.add_argument("--direction", choices=("target", "base", "both"),
                    default="both", help="which presence disagreement to list")
    ap.add_argument("--states", action="store_true",
                    help="the secondary sieve: unwind-STATE store counts")
    ap.add_argument("--min", type=float, default=0.0, help="score window floor")
    ap.add_argument("--max", type=float, default=100.0,
                    help="score window ceiling")
    ap.add_argument("--calibrate", action="store_true",
                    help="the rows on 100.00%% functions - the detector-bug rate")
    ap.add_argument("--rva", help="show only this function, hex rva (the "
                                  "presence listing only - not --states/"
                                  "--calibrate)")
    ap.add_argument("--detail", action="store_true",
                    help="add the state bracket and the ctor/dtor call delta")
    ap.add_argument("--tsv", help="also write every row to this TSV")
    a = ap.parse_args(argv)

    from homm1.tool import ToolError
    from homm1.walls import check_unit
    for unit in a.unit or ():
        check_unit(unit)
    if a.rva is not None:
        try:
            a.rva = f"0x{int(a.rva, 16):06x}"     # accept 153810 and 0x153810
        except ValueError:
            sys.exit(f"[walls eh-frame] --rva {a.rva!r} is not a hex address")
    units = set(a.unit) if a.unit else None
    pairscan.require_pairs(units)
    try:
        rows = scan(units)
    except ToolError as e:
        sys.exit(f"[walls eh-frame] {e}")
    if not rows:
        sys.exit("no scoring functions in the normalized pairs - the report "
                 "and the objs disagree; re-run `homm1 compare`")
    exact, bad, eh_exact, sbad = calibrate(rows)
    tally = Counter(r["verdict"] for r in rows)
    print(f"{len(rows)} scoring functions in "
          f"{len({r['unit'] for r in rows})} units")
    for k in ("BOTH", "NEITHER", "TARGET_ONLY", "BASE_ONLY"):
        print(f"  {k:<12} {tally.get(k, 0):4d}")
        if k == "TARGET_ONLY":
            sub = Counter(r["cause"] for r in rows if r["verdict"] == k)
            for c in ("INLINE_CUT", "EXIT_MERGE", "MISSING_OBJECT"):
                if sub.get(c):
                    print(f"    {c:<14} {sub[c]:4d}")
    print(f"calibration on the {len(exact)} functions at 100.00%:")
    print(f"  presence   {len(bad)} disagree  "
          f"({100.0 * len(bad) / max(1, len(exact)):.2f}% false-positive rate)")
    print(f"  state cnt  {len(sbad)} of {len(eh_exact)} EH-framed disagree  "
          f"({100.0 * len(sbad) / max(1, len(eh_exact)):.2f}%)")
    if a.tsv:
        import csv
        with open(a.tsv, "w", newline="") as fh:
            w = csv.writer(fh, delimiter="\t")
            w.writerow("verdict cause rva unit fuzzy size base_states "
                       "tgt_states states slot first last extra_ctors "
                       "name".split())
            for r in sorted(rows, key=lambda r: (r["verdict"], -r["size"])):
                w.writerow([r["verdict"], r["cause"], r["rva"], r["unit"],
                            f"{r['fuzzy']:.2f}", r["size"], r["base_states"],
                            r["tgt_states"],
                            ",".join(str(s) for s in r["states"]), r["slot"],
                            "" if r["first"] is None else f"0x{r['first']:x}",
                            "" if r["last"] is None else f"0x{r['last']:x}",
                            " ".join(r["extra_ctors"] + r["resited"]),
                            r["name"]])
        print(f"wrote {a.tsv}")

    if a.calibrate:                      # after --tsv: the flag is not a mode
        for r in sorted(bad, key=lambda r: -r["size"]):
            print(f"  FP-presence {r['verdict']:<11} {r['rva']:<9} "
                  f"{r['unit']:<22} {r['name']}")
        for r in sorted(sbad, key=lambda r: -r["size"]):
            print(f"  FP-states   base={r['base_states']:<3} "
                  f"tgt={r['tgt_states']:<3} {r['rva']:<9} {r['unit']:<22} "
                  f"{r['name']}")
        return 0

    if a.states:
        d = [r for r in rows if r["verdict"] == "BOTH"
             and r["base_states"] != r["tgt_states"]
             and a.min <= r["fuzzy"] <= a.max]
        d.sort(key=lambda r: -abs(r["tgt_states"] - r["base_states"]))
        print(f"\n{len(d)} BOTH row(s) with differing unwind-state counts")
        for r in d:
            print(f"{r['tgt_states'] - r['base_states']:+3d} states  "
                  f"{r['size']:5d} B  {r['rva']:<9} {r['unit']:<22} "
                  f"{r['fuzzy']:6.2f}%  base={r['base_states']} "
                  f"tgt={r['tgt_states']}  {r['cause']:<14} {r['name']}")
            if a.detail and (r["extra_ctors"] or r["resited"]):
                print("            ctor/dtor call delta: "
                      + " ".join(r["extra_ctors"] + r["resited"]))
        return 0

    want = {"target": ("TARGET_ONLY",), "base": ("BASE_ONLY",),
            "both": ("TARGET_ONLY", "BASE_ONLY")}[a.direction]
    hits = [r for r in rows if r["verdict"] in want
            and a.min <= r["fuzzy"] <= a.max
            and (not a.rva or r["rva"] == a.rva)]
    hits.sort(key=lambda r: (r["verdict"], -r["size"]))
    print()
    for r in hits:
        print(f"{r['verdict']:<11} {r['cause']:<14} {r['size']:5d} B  "
              f"{r['rva']:<9} {r['unit']:<22} {r['fuzzy']:6.2f}%  "
              f"states={','.join(str(s) for s in r['states']) or '-':<10} "
              f"{r['name']}")
        if a.detail and r["first"] is not None:
            print(f"            unwind-state slot {r['slot']}, lifetime "
                  f"bracket 0x{r['first']:x}..0x{r['last']:x}"
                  + ("" if r["unwind"] else "  [NO TEARDOWN - suspect decode]"))
        if a.detail and (r["extra_ctors"] or r["resited"]):
            print("            ctor/dtor call delta: "
                  + " ".join(r["extra_ctors"] + r["resited"]))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

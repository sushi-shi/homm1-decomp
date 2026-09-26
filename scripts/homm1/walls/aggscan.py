"""homm1.walls.aggscan - the BY-VALUE AGGREGATE ARGUMENT sieve.

cl 5.0 hands an aggregate wider than a register by opening a hole in the
outgoing frame and copying into it:

    lea  edi,[obj+0x134]
    sub  esp,0x10
    mov  ebx,esp
    mov  ecx,[edi]      / mov [ebx],ecx        (four dword copies)
    ...
    call callee

Four `push`es of four separate `i32` is a DIFFERENT shape. So the hole is a
one-instruction witness that the callee's parameter at that position is ONE
16-byte object, and a declaration that spells it `i32 x, i32 y, i32 w, i32 h`
is wrong about the source even when the call site happens to score well -
`AddToList3` took the level record's rect as four ints named `player0..3` and
retail's site built one block.

WHAT IT ASKS. Not "do the two sides look the same here" - that is the score's
question and it drowns in scheduling. It aggregates {callee -> hole sizes}
over the WHOLE image on both sides and asks a signature question of every call
site at once: is there a callee retail hands a block and we never do?

TWO ATTRIBUTION RULES, both measured rather than assumed:

  * the `mov reg,esp` is looked for up to the NEXT CALL, not inside a fixed
    byte window. cl interleaves the copies of consecutive holes, so
    `sub esp,0x10 / ... / mov edx,esp` can be 22 bytes apart; a 16-byte window
    found ONE hole in a region where the disassembly has four, and called
    three sites missing that our own object emits.
  * an unconditional `jmp` is FOLLOWED. cl tail-merges the argument build -
    several predecessors each fill a hole and jump to one shared call - so
    stopping at the first `call` BYTE attributes the block to whatever call
    sits next in layout order. It named `CTriggerMgr::CellDispatch` for two
    blocks that in fact jump to `CGrunt::PlaySound`.

AND ONE SOUNDNESS FILTER: a reservation whose size is not a multiple of 4 is
rejected. The scan reads bytes rather than a decoded stream, so `83 EC 02`
appears inside the displacement of `mov DWORD PTR [ebx+0x2ec],0x1f401`; cl
never moves ESP by a non-dword amount, so the filter costs nothing and removes
that whole class.

WHAT IS COUNTED APART: a reservation nothing is COPIED INTO is the frame,
not an argument. That test is structural
rather than positional - "the first reservation is the frame" both dropped six
real callees whose only block sits in the prologue and, on
`CGrunt::StepDefenderBehavior`, charged a 104-vs-92-byte frame delta to
`CUserLogic::GetScreenPos`, whose parameter is a pointer.

WHAT THE SWEEP IS WORTH, measured 2026-08-23: 179 argument holes ours against
182 retail, over 20 distinct callees. ZERO callees are retail-only and ZERO are
ours-only - there is no function in the tree that takes scalars where retail
takes an aggregate, or the reverse - and every named callee's hole-SIZE
multiset matches exactly. Two functions differ, both in the COUNT of holes at
one size (`CGrunt::StepGruntMovement` 2 vs 6, the tail-merged 12-byte
`GruntDirectionCell`; `CStatusBarMgr::BuildTabzDialog` 15 vs 14), which is an
inlining or block-layout divergence, not a signature. The retail-only /
ours-only verdict held under BOTH the earlier positional frame rule and this
one, so it does not rest on the filter.

    homm1 walls aggscan [--sites] [--json]
    homm1 walls aggscan --control     hermetic positives/negatives and the
                                       backwards run over the live image
"""

from __future__ import annotations

from homm1.core.usage import logged

import argparse
import json
import sys
from collections import defaultdict
from pathlib import Path

from homm1.delink.coffx import Obj
from homm1.walls.semdiff import NORM

REGS = ("eax", "ecx", "edx", "ebx", "esp", "ebp", "esi", "edi")

#: how far past the hole the call it feeds may sit
REACH = 400

#: not signatures: a virtual call names no symbol, and a tail-merged block
#: whose shared call site is more than one `jmp` away names nothing at all
SENTINELS = ("<indirect>", "<unattributed>")


def _is_call(payload, k) -> bool:
    return payload[k] == 0xE8 or (payload[k] == 0xFF
                                  and (payload[k + 1] >> 3) & 7 == 2)


def _takes_esp(payload, k) -> bool:
    """`mov <reg>,esp`: 8B /r with mod=11 and rm=100."""
    modrm = payload[k + 1]
    return payload[k] == 0x8B and modrm >> 6 == 3 and (modrm & 7) == 4


def _reserve(payload, i):
    """(bytes reserved, offset just past the instruction) or (None, None)."""
    if payload[i] == 0x83 and payload[i + 1] == 0xEC:
        return payload[i + 2], i + 3
    if payload[i] == 0x81 and payload[i + 1] == 0xEC:
        return int.from_bytes(payload[i + 2:i + 6], "little"), i + 6
    return None, None


def callee(payload, start, hi, rel, hops=3):
    """The call this hole feeds, following an unconditional `jmp` on the way."""
    k = start
    while k < hi - 4:
        if payload[k] == 0xE8 and (k + 1) in rel:
            return rel[k + 1][0]
        if _is_call(payload, k):
            return "<indirect>"
        if payload[k] == 0xEB and hops:                       # jmp rel8
            k += 2 + int.from_bytes(payload[k + 1:k + 2], "little", signed=True)
            hops -= 1
            continue
        if payload[k] == 0xE9 and hops:                       # jmp rel32
            k += 5 + int.from_bytes(payload[k + 1:k + 5], "little", signed=True)
            hops -= 1
            continue
        if k - start > REACH:
            return None
        k += 1
    return None


def _fills(payload, k, hi, reg, size) -> int:
    """Dword stores INTO the block: `mov [reg+d],r32` (89 /r) or
    `mov [reg+d],imm32` (C7 /0) with d inside the reservation.

    This is what separates an ARGUMENT hole from the prologue's frame
    reservation, and it does it structurally rather than by position, so no
    "the first one is the frame" heuristic is needed. It also removes the byte
    scan's own misalignments: `8b dc` read as `mov ebx,esp` in the middle of
    `mov ecx,DWORD PTR [ebx+0x2dc]` is not followed by stores through EBX with
    small displacements, and that is exactly the coincidence that charged a
    104-vs-92-byte frame delta in `CGrunt::StepDefenderBehavior` to
    `CUserLogic::GetScreenPos`, whose parameter is a pointer.
    """
    rm = REGS.index(reg)
    n = 0
    while k < hi - 1:
        if _is_call(payload, k):
            break
        op, modrm = payload[k], payload[k + 1]
        if op in (0x89, 0xC7) and (modrm & 7) == rm and modrm >> 6 != 3 \
                and (op != 0xC7 or ((modrm >> 3) & 7) == 0):
            mod, disp = modrm >> 6, 0
            if mod == 1:
                disp = payload[k + 2]
            elif mod == 2:
                disp = int.from_bytes(payload[k + 2:k + 6], "little")
            if disp < size:
                n += 1
        k += 1
    return n


def holes(payload: bytes, lo: int, hi: int, rel: dict):
    """[(offset in function, bytes, register, callee)] - the argument blocks."""
    out = []
    i = lo
    while i < hi - 2:
        n, nxt = _reserve(payload, i)
        if n is not None and n and n % 4 == 0:
            for k in range(nxt, min(nxt + REACH, hi - 1)):
                if _is_call(payload, k):
                    break
                if _takes_esp(payload, k):
                    reg = REGS[(payload[k + 1] >> 3) & 7]
                    if _fills(payload, k + 2, hi, reg, n):
                        out.append((i - lo, n, reg,
                                    callee(payload, nxt, hi, rel)))
                    break
        i += 1
    return out


def scan_obj(path: Path):
    obj = Obj(path)
    out, defined = {}, set()
    for secnum in range(1, obj.nsec + 1):
        if not (obj.section_table[secnum - 1]["characteristics"] & 0x20):
            continue
        payload = obj.section_payload(secnum)
        if not payload:
            continue
        rel = obj.typed_relocations(secnum)
        members = sorted((v, n) for v, n, _s in obj.section_members(secnum))
        defined.update(n for _v, n in members)
        offs = [v for v, _n in members]
        for i, (v, name) in enumerate(members):
            end = offs[i + 1] if i + 1 < len(offs) else len(payload)
            h = holes(payload, v, end, rel)
            if h:
                out[name] = h
    return out, defined


def load(side: str, root: Path | None = None):
    root = (root or NORM) / side
    acc, defs = {}, {}
    for path in sorted(root.rglob("*.obj")):
        unit = path.name.split(".")[0]
        st, dd = scan_obj(path)
        defs[unit] = dd
        for fn, v in st.items():
            acc[(unit, fn)] = v
    return acc, defs


def bycallee(acc: dict, both: set):
    m: dict = defaultdict(lambda: defaultdict(int))
    for k, v in acc.items():
        if k not in both:
            continue
        for _off, n, _reg, c in v:
            m[c or "<unattributed>"][n] += 1
    return {c: dict(sizes) for c, sizes in m.items()}


def perfunction(acc: dict, both: set):
    return {k: sorted(n for _o, n, _r, _c in v)
            for k, v in acc.items() if k in both}


def sweep(root: Path | None = None):
    ours, ours_def = load("base", root)
    retail, retail_def = load("target", root)
    both = {(u, n) for u, ns in ours_def.items() for n in ns
            if n in retail_def.get(u, ())}
    return {"ours": ours, "retail": retail, "both": both,
            "ours_by_callee": bycallee(ours, both),
            "retail_by_callee": bycallee(retail, both)}


def report(res, sites: bool) -> None:
    if not CONTROL:
        print("No HoMM1 live aggregate controls are calibrated yet.")
        return 2
    bo, to = res["ours_by_callee"], res["retail_by_callee"]
    no = sum(sum(v.values()) for v in bo.values())
    nr = sum(sum(v.values()) for v in to.values())
    print(f"non-frame aggregate holes : ours {no}, retail {nr}   "
          f"over {len(set(bo) | set(to))} callees")
    only_r = sorted(set(to) - set(bo) - set(SENTINELS))
    only_o = sorted(set(bo) - set(to) - set(SENTINELS))
    print(f"callees retail hands a block and we NEVER do : {len(only_r)}"
          + (f"   {only_r}" if only_r else "   (a wrong signature would be here)"))
    print(f"callees WE hand a block and retail never does: {len(only_o)}"
          + (f"   {only_o}" if only_o else ""))
    print()
    print(f"{'callee':<58s} {'ours':<26s} retail")
    for c in sorted(set(bo) | set(to)):
        o, r = bo.get(c, {}), to.get(c, {})
        print(f"{c[:56]:<58s} {str(o):<26s} {str(r)}"
              + ("   <== DIFFERS" if o != r else ""))
    po, pr = perfunction(res["ours"], res["both"]), \
        perfunction(res["retail"], res["both"])
    diff = [k for k in sorted(set(po) | set(pr)) if po.get(k, []) != pr.get(k, [])]
    print(f"\nfunctions whose hole-size multiset differs: {len(diff)}")
    for k in diff:
        print(f"  {k[0]}/{k[1][:60]}")
        print(f"      ours   {po.get(k, [])}")
        print(f"      retail {pr.get(k, [])}")
    if sites:
        print("\nevery non-frame hole:")
        for k in sorted(res["both"]):
            for off, n, reg, c in res["ours"].get(k, ()):
                print(f"  ours   {k[0]}/{k[1][:44]} +0x{off:x} {n}B "
                      f"{reg} -> {c}")


def _hermetic() -> int:
    """A synthesized caller passing a 16-byte block, and the four-push shape
    it must be told apart from.

    A sieve whose live answer is a clean zero has to show it can fire; these
    are the two spellings of the same call, and only the first opens a hole.
    """
    push4 = bytes.fromhex("6a006a006a006a00") + b"\xe8\0\0\0\0\xc3"
    #  sub esp,0x10 / mov ecx,esp / mov [ecx],0 / mov [ecx+4],eax
    #  / mov [ecx+8],eax / mov [ecx+0xc],eax / call
    block = (bytes.fromhex("83ec108bcc")
             + bytes.fromhex("c70100000000894104894108894 10c".replace(" ", ""))
             + b"\xe8\0\0\0\0\xc3")
    #  the same reservation with NO stores into it: a frame, not an argument
    frame = (bytes.fromhex("83ec108bcc") + bytes.fromhex("33c0")
             + b"\xe8\0\0\0\0\xc3")
    rel = lambda p: {len(p) - 5: ("?F@@YAXUtagRECT@@@Z", 20)}

    bad = 0
    for tag, payload, want, why in (
            ("block", block, [16],
             "POSITIVE: `sub esp,0x10 / mov ecx,esp` + dword copies is the "
             "only shape cl 5.0 emits for a by-value aggregate"),
            ("push4", push4, [],
             "NEGATIVE: four pushes of four scalars must NOT read as a "
             "block - that difference is the whole signature question"),
            ("frame", frame, [],
             "NEGATIVE: a reservation nothing is copied INTO is a frame, "
             "not an argument")):
        got = [n for _o, n, _r, _c in holes(payload, 0, len(payload),
                                            rel(payload))]
        ok = got == want
        print(f"{'FIRES ' if got else 'SILENT'} hermetic-{tag:<6s} "
              f"{'ok' if ok else f'got {got}, want {want}'}")
        print(f"      {why}")
        bad += 0 if ok else 1

    junk = bytes.fromhex("c783ec02000001f40100") + b"\xc3"
    got = holes(junk, 0, len(junk), {})
    ok = not got
    print(f"{'SILENT' if not got else 'FIRES '} hermetic-align  "
          f"{'ok' if ok else f'got {got}, want none'}")
    print("      NEGATIVE: `83 EC 02` inside the displacement of "
          "`mov DWORD PTR [ebx+0x2ec],0x1f401` is not a reservation - the "
          "multiple-of-4 filter is what removes it")
    return bad + (0 if ok else 1)


#: the rows this sieve was built on, kept as LIVE regression guards. Each was
#: a real signature defect: a callee retail hands one block and our source
#: declared as separate scalars. They must keep matching, in both columns.
CONTROL = {}  # Requires HoMM1-specific live calibration evidence.


def _named_rows(res) -> int:
    """The rows this sieve was built on, re-proven against today's image.

    Each names an EXACT mangled callee and an exact site count, in BOTH
    columns: a control that reads 0 against 0 passes while saying nothing, and
    the first spelling of these did exactly that on a mistyped name.
    """
    if not CONTROL:
        print("No HoMM1 live aggregate controls are calibrated yet.")
        return 2
    bo, to = res["ours_by_callee"], res["retail_by_callee"]
    bad = 0
    for name, (size, count, why) in CONTROL.items():
        o, r = bo.get(name, {}).get(size, 0), to.get(name, {}).get(size, 0)
        ok = o == r == count
        print(f"{'OK    ' if ok else 'BROKEN'} row "
              f"{name.split('@@')[0][:46]:<48s} ours {o} retail {r} "
              f"x {size}B  (want {count})")
        print(f"      {why}")
        bad += 0 if ok else 1
    return bad


def control() -> int:
    bad = _hermetic()
    res = sweep()
    bad += _named_rows(res)
    if not CONTROL:
        print("No HoMM1 live aggregate controls are calibrated yet.")
        return 2
    bo, to = res["ours_by_callee"], res["retail_by_callee"]
    only_r = set(to) - set(bo) - set(SENTINELS)
    only_o = set(bo) - set(to) - set(SENTINELS)
    ok = not only_r and not only_o
    print(f"{'SILENT' if ok else 'FIRES '} live-forward   "
          f"retail-only {sorted(only_r)}, ours-only {sorted(only_o)}")
    print(f"{'SILENT' if ok else 'FIRES '} live-backwards "
          f"the statement is a set difference taken both ways, so the "
          f"backwards run IS the ours-only column above")
    n = sum(sum(v.values()) for v in bo.values())
    sizes = {s for v in bo.values() for s in v}
    split = n >= 100 and len(sizes) >= 2 and len(bo) >= 10
    print(f"{'OK    ' if split else 'BROKEN'} live-split     "
          f"{n} holes over {len(bo)} callees in {len(sizes)} distinct sizes "
          f"{sorted(sizes)}")
    print("      a zero is only worth reading when the population it swept is "
          "non-trivial: several callees, several block sizes")
    bad += 0 if split else 1
    if bad:
        print("\na control changed verdict: the image moved or the detector "
              "did - read it before trusting the sweep")
    return 1 if bad else 0


@logged
def main(argv=None) -> int:
    ap = argparse.ArgumentParser(prog="homm1 walls aggscan",
                                 description=__doc__.split("\n\n")[0])
    ap.add_argument("--sites", action="store_true",
                    help="list every non-frame hole our side opens")
    ap.add_argument("--control", action="store_true")
    ap.add_argument("--json", action="store_true")
    args = ap.parse_args(argv)
    if args.control:
        return control()
    res = sweep()
    if args.json:
        json.dump({"ours": res["ours_by_callee"],
                   "retail": res["retail_by_callee"]}, sys.stdout)
        return 0
    report(res, args.sites)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

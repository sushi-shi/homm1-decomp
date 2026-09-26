"""homm1.walls.semdiff - OPERAND-LEVEL adjudication of a normalized pair.

`walls diagnose` CLASSIFIES a wall (referent -> inline/call-set -> cfg ->
regalloc). It does not ADJUDICATE it: its call-set and branch-skeleton view
cannot see a member read at the wrong displacement, a swapped store source,
a dropped `fild`, or a mask that lost a bit. Those are the live-bug classes,
and they are all multiset questions over the two sides' operands.

This module answers them. It reads the same evidence objdiff scored (the
normalized pair under build/objdiff/compare-new) and compares, as multisets:

    fp      FP opcode counts        cl NEVER adds or drops a conversion to
                                    schedule - any delta is semantic
    disp    member displacements    [reg+N] with reg != esp: the fields the
                                    function actually reads and writes
    store   store targets           the displacement side of `mov [reg+N],x`
    imm     immediates              constants, masks, magic numbers
    mnem    mnemonic counts         the coarse shape, printed last

The EXCLUSIVE section is printed FIRST and is the whole point: a key one side
uses and the other never touches. `b3/t4` on a shared key is almost always
scheduling; `b0/t2` is a constant, offset or conversion that exists on one
side only, and that is where the four bugs of 2026-08-20 were found.

Two more sections cover what a value-level multiset structurally cannot see,
because objdiff MASKS relocated operands:

    referent sequence   the ORDERED list of relocation targets. A swapped
                        pair of string keys, or a global bound to the wrong
                        symbol, is invisible in the scored bytes and obvious
                        here.
    cmd/key pairing     the ordered (pushed small immediate -> string
                        referent) pairs: "which widget got which command id
                        and which sprite key", which neither the value
                        multiset nor the referent order decides alone.

FALSE-POSITIVE TAXONOMY. Every one of these was observed producing a
difference on a pair whose semantics are identical; the filters that can be
applied mechanically are applied, the rest are for the reader:

  * register mirrors        `cmp a,b/jge` == `cmp b,a/jle`; `and al,0xe0` ==
                            `and ecx,0xffffffe0` (cl uses the 2-byte AL form
                            when the value lands in EAX); `cdq` == `sar 31`
                            for the signed-modulo sign mask. FILTERED: no.
  * cross-jump merge degree one side shares a cleanup/call site the other
                            duplicates. Changes call-SITE counts, never the
                            set of paths that reach the call. FILTERED: no.
  * lea-folded displacement retail `lea eax,[base+0x150]` then `[eax+0x24]`
                            vs ours `[base+0x174]` - the same address, the
                            array base folded into the member offset.
                            FILTERED: no (compare the SUM).
  * operand width           a byte store read back as a dword when the upper
                            bytes are known zero. FILTERED: no.
  * rep-stos first-dword    a `memset` whose first element is stored
                            separately from the `rep stos` tail.
  * RMW split/fold          `add [m],1` vs `mov r,[m]; inc r; mov [m],r`
                            changes the disp count for one member.
  * FP-stack housekeeping   `fmulp` vs `fmul st,st(1)` + a later `fstp` -
                            the POP discipline, not a conversion. fld/fstp
                            deltas are noise; fild/fistp deltas are not.
  * jump-table data         a function's own index/target table decodes as
                            junk instructions with huge displacements.
                            FILTERED: yes - every line whose referent is the
                            function itself is dropped.
  * byte-continuation       the second line of a long instruction carries
                            bytes and no mnemonic. FILTERED: yes.
  * frame-size immediates   `sub esp,N` / `add esp,N` / `ret N` are the
                            local-slot count and the callee cleanup, never
                            semantics. FILTERED: yes.
  * one-past-end referent   `sym+size` aliases the NEXT symbol's name, so
                            one side reads `foo+0x400` and the other
                            `bar+1` for the same address. FILTERED: no.

FALSE-NEGATIVE LIMIT: identical operand multisets and ordered referents do
not prove identical side-effect reachability. Moving ResetCell across the
current-player guard in LoadEntranceConfig preserved both. Audit branch
destinations and per-edge call traces.

    homm1 walls semdiff <rva|name> [--top N] [--all]
    homm1 walls semsweep <tsv> [--first N] [--last N] [--fp-only]

semsweep screens a worklist (the `walls inventory` TSV shape: unit, name,
rva, pct, size, class, detail) and prints ONE line per row unless the row has
an exclusive key or an FP delta - the shape that made a 379-row lane
tractable in one pass.
"""

from __future__ import annotations

from homm1.core.usage import logged

import argparse
import re
from collections import Counter
from difflib import SequenceMatcher

from homm1.core.paths import BUILD
from homm1.delink.coffx import Obj
from homm1.tool import objdump
from homm1.walls.diagnose import _find_function, _jump_table_bytes, _locate

NORM = BUILD / "objdiff/compare-new"

#: `[reg+N]` / `[reg+idx*s+N]`, esp deliberately excluded - a stack slot is a
#: frame-layout accident, a member displacement is the class model.
MEM = re.compile(r"\[(e[a-d]x|e[sd]i|ebx|ecx|ebp)(?:\+e[a-z]{2}\*\d)?"
                 r"([+-]0x[0-9a-f]+)?\]")
#: the same operand with EBP as its base, and the two spellings that make EBP a
#: FRAME POINTER. cl 5.0 at /O2 usually spends EBP as a general register, and
#: then `[ebp+N]` IS a member displacement - but it does give a handful of
#: functions an ebp frame, and there `[ebp-N]` is a stack slot the two sides do
#: not agree on. Measured 2026-08-23 over the 595-row todo queue: 8 rows
#: establish one and 3 address slots through it (`WarpTextureBlit` 65/67
#: operands, `FillPolygon` 37/38, `CWwdSpatialMgr::ScrollTo` 4/4).
EBP_MEM = re.compile(r"\[ebp(?:\+e[a-z]{2}(?:\*\d)?)?([+-]0x[0-9a-f]+)?\]")
EBP_FRAME = re.compile(
    r"^(?:mov\s+ebp,esp|lea\s+ebp,\[esp(?:[+-]0x[0-9a-f]+)?\])$")
IMM = re.compile(r"(?<![\[+])\b0x[0-9a-f]+\b")
FP = re.compile(r"^(fild|fistp?|fld|fstp?|fmul|fdiv|fadd|fsub|fcom|fchs|fabs"
                r"|frndint|fxch|fsqrt|fnstsw)\w*")
BYTES_ONLY = re.compile(r"^(?:[0-9a-f]{2} ?)+$")
FRAME = re.compile(r"^(sub|add)\s+esp,")
NOT_A_VALUE = re.compile(r"^(j\w+|call|loop|ret)$")


def ebp_is_frame(*sides) -> bool:
    """Whether EBP is a frame pointer, in which case its `[ebp+-N]` operands
    are stack slots and not member displacements.

    Take BOTH sides. cl gives one side an ebp frame and not the other
    (`CGrunt::StepBrickLayerBehavior` is ours, retail's ebp is a general
    register there), and masking only the side that has one is a mask that
    manufactures a difference. If either side's ebp is a frame pointer, the
    operand is not comparable on that row and both sides mask.
    """
    return any(EBP_FRAME.match(ln.asm) for lines in sides for ln in lines)


class Line:
    """One decoded instruction: address, asm text, and the relocation
    referent whose operand it carries (masked to 0 in the scored bytes)."""

    __slots__ = ("addr", "asm", "ref")

    def __init__(self, addr: int, asm: str, ref: str | None):
        self.addr, self.asm, self.ref = addr, asm, ref


def _decode(body: bytes, rel: dict, self_name: str | None = None) -> list[Line]:
    """Disassemble one function window and attach each relocation to the
    instruction whose bytes contain it.

    Given the function's own name, two normalizations that every consumer
    needs and none of them can do afterwards:

      * the function's own jump/index TABLE is DATA embedded in .text, and
        objdump decodes it as instructions.  The offsets a self-referent
        relocation covers are the table (the rule `walls diagnose` already
        applies through `_jump_table_bytes`), and a line STARTING inside them
        is dropped.  A real self-transfer is never dropped by this: its
        relocation sits at `addr+1`, so the instruction's own address is not
        covered.
      * a relocation that names the function ITSELF on a real instruction is
        a self-transfer - a recursive `call`, a tail `jmp`, or the indirect
        `jmp` that ADDRESSES the table - and the delinked target resolves it
        inside its own section with NO relocation at all.  The two sides then
        disagree about a name that only means "here".  Measured 2026-08-23 on
        the exact-row reflexivity control: `zPTree::Walk` and
        `CDDrawSubMgrLeaf::ScanTree` are both byte-identical to retail and
        both read as a one-instruction `selection` residual purely from this.
        The referent is dropped; the instruction is kept.
    """
    table = _jump_table_bytes(rel, self_name) if self_name else ()
    out: list[Line] = []
    for line in objdump.disassemble(body, vma=0).splitlines():
        if ":\t" not in line:
            continue
        head, rest = line.split(":\t", 1)
        try:
            addr = int(head.strip(), 16)
        except ValueError:
            continue
        if addr in table:
            continue
        parts = rest.split("\t")
        asm = " ".join((parts[-1] if len(parts) > 1 else parts[0]).split())
        nbytes = len((parts[0] if len(parts) > 1 else "").split())
        ref = None
        for off, (target, _add) in rel.items():
            if addr <= off < addr + max(nbytes, 1):
                ref = re.sub(r"\+0x[0-9a-f]+$", "", target)
                break
        if self_name and ref == self_name:
            ref = None
        out.append(Line(addr, asm, ref))
    return out


def pair_lines(token: str):
    """(binding, base lines, target lines) for one claimed function."""
    b, why = _locate(token)
    if b is None:
        raise SystemExit(f"[semdiff] {why}")
    base = Obj(NORM / "base" / f"{b.unit}.obj")
    tgt_path = NORM / "target" / f"{b.unit}.c.obj"
    if not tgt_path.exists():
        tgt_path = NORM / "target" / f"{b.unit}.obj"
    tgt = Obj(tgt_path)
    bb, brel, _bs = _find_function(base, b.name)
    tb, trel, _ts = _find_function(tgt, b.name)
    return b, _decode(bb, brel, b.name), _decode(tb, trel, b.name)


def features(lines: list[Line], self_name: str = "",
             ebp_frame: bool | None = None) -> dict[str, Counter]:
    """The five multisets, with the mechanical filters applied.

    Dropped here: the function's own jump/index table (self-annotated lines,
    which decode as junk), byte-continuation lines, the frame-size and
    callee-cleanup immediates, and - in the handful of functions cl gives an
    ebp frame - the `[ebp+-N]` operands, which are stack slots there and not
    member displacements. Pass `ebp_frame` from BOTH sides (`ebp_is_frame(base,
    target)`); reading it off one side masks asymmetrically.
    """
    if ebp_frame is None:
        ebp_frame = ebp_is_frame(lines)
    disp, imm, mnem, fp, store = (Counter() for _ in range(5))
    for ln in lines:
        if self_name and ln.ref == self_name:
            continue
        asm = ln.asm
        if not asm or BYTES_ONLY.match(asm):
            continue
        if FRAME.match(asm):
            continue
        if ebp_frame:
            asm = EBP_MEM.sub("[esp]", asm)
        op = asm.split()[0]
        mnem[op] += 1
        m = FP.match(asm)
        if m:
            fp[m.group(1)] += 1
        for _reg, d in MEM.findall(asm):
            disp[d or "+0x0"] += 1
        if op == "mov" and "," in asm:
            dst = asm.split(",", 1)[0]
            for _reg, d in MEM.findall(dst):
                store[d or "+0x0"] += 1
        if not NOT_A_VALUE.match(op):
            for v in IMM.findall(asm):
                if int(v, 16) > 3:
                    imm[v] += 1
    return dict(fp=fp, disp=disp, store=store, imm=imm, mnem=mnem)


def exclusive(fb: dict, ft: dict) -> list[tuple[str, str, int, int]]:
    """Keys ONE side uses and the other never touches - the semantic signal."""
    out = []
    for kind in ("fp", "disp", "store", "imm"):
        cb, ct = fb[kind], ft[kind]
        for key in sorted(set(cb) | set(ct)):
            if (cb[key] == 0) != (ct[key] == 0):
                out.append((kind, key, cb[key], ct[key]))
    return out


def referent_runs(lines: list[Line]) -> list[str]:
    """The ordered referent sequence, consecutive duplicates collapsed."""
    out: list[str] = []
    for ln in lines:
        if ln.ref and (not out or out[-1] != ln.ref):
            out.append(ln.ref)
    return out


def cmd_key_pairs(lines: list[Line]) -> list[tuple[str, str]]:
    """Ordered (pushed small immediate -> next string referent) pairs."""
    out, pending = [], None
    for ln in lines:
        m = re.fullmatch(r"push (0x[0-9a-f]+)", ln.asm)
        if m and 0x20 <= int(m.group(1), 16) <= 0x200:
            pending = m.group(1)
        if ln.ref and ln.ref.startswith("??_C@") and pending:
            out.append((pending, ln.ref[:52]))
            pending = None
    return out


def _seq_report(label: str, rb: list, rt: list, ctx: int = 6) -> int:
    sm = SequenceMatcher(None, rb, rt, autojunk=False)
    ops = [o for o in sm.get_opcodes() if o[0] != "equal"]
    print(f"== {label}: base {len(rb)} target {len(rt)}, "
          f"{len(ops)} divergence(s)")
    for op, i1, i2, j1, j2 in ops:
        print(f"   {op} base[{i1}:{i2}] target[{j1}:{j2}]")
        for x in rb[i1:i2][:ctx]:
            print(f"      B {x}")
        for x in rt[j1:j2][:ctx]:
            print(f"      T {x}")
    return len(ops)


def adjudicate(token: str, top: int = 30, show_all: bool = False) -> int:
    b, lb, lt = pair_lines(token)
    ebp = ebp_is_frame(lb, lt)
    fb, ft = features(lb, b.name, ebp), features(lt, b.name, ebp)
    exc = exclusive(fb, ft)

    print(f"{b.name}  0x{b.rva:06x}  [{b.unit}]")
    print(f"== EXCLUSIVE (one side only) -- the semantic signal: {len(exc)}")
    for kind, key, u, v in exc:
        print(f"   {kind:6} {key:>14}  base {u:3d}  target {v:3d}")
    if not exc:
        print("   (none)")

    for kind in ("fp", "disp", "store", "imm", "mnem"):
        cb, ct = fb[kind], ft[kind]
        keys = sorted(set(cb) | set(ct), key=lambda x: -abs(cb[x] - ct[x]))
        diffs = [(x, cb[x], ct[x]) for x in keys if cb[x] != ct[x]]
        if not diffs and not show_all:
            print(f"== {kind}: identical "
                  f"(base {sum(cb.values())} target {sum(ct.values())})")
            continue
        print(f"== {kind}: {len(diffs)} differing key(s)")
        for x, u, v in diffs[:top]:
            print(f"   {x:>14}  base {u:3d}  target {v:3d}")

    _seq_report("referent sequence", referent_runs(lb), referent_runs(lt))
    _seq_report("cmd/key pairing", cmd_key_pairs(lb), cmd_key_pairs(lt))
    return 0


def screen(token: str) -> tuple[list, list]:
    """(exclusive keys, FP deltas) - the two sweep-visible signals."""
    b, lb, lt = pair_lines(token)
    ebp = ebp_is_frame(lb, lt)
    fb, ft = features(lb, b.name, ebp), features(lt, b.name, ebp)
    fpd = [(k, fb["fp"][k], ft["fp"][k])
           for k in sorted(set(fb["fp"]) | set(ft["fp"]))
           if fb["fp"][k] != ft["fp"][k]]
    return exclusive(fb, ft), fpd


def sweep(rows, first: int, last: int, fp_only: bool) -> int:
    flagged = 0
    for n, row in enumerate(rows, 1):
        if n < first or (last and n > last):
            continue
        rva, name = row["rva"], row["name"]
        try:
            exc, fpd = screen(rva)
        except SystemExit as exc_err:
            print(f"### {n} {rva} {name} -- {exc_err}")
            continue
        if fp_only:
            exc = []
        if not exc and not fpd:
            print(f"### {n} {rva} {row['pct']} {row['cls']} {name} -- CLEAN")
            continue
        flagged += 1
        print(f"### {n} {rva} {row['pct']} {row['cls']} {name}")
        for kind, key, u, v in exc[:8]:
            print(f"   {kind:6} {key:>14}  base {u:3d}  target {v:3d}")
        for k, u, v in fpd:
            print(f"   FP!    {k:>14}  base {u:3d}  target {v:3d}")
    print(f"\nflagged {flagged} row(s)")
    return 0


def _read_worklist(path: str) -> list[dict]:
    rows = []
    with open(path) as fh:
        for raw in fh:
            f = raw.rstrip("\n").split("\t")
            if len(f) < 3:
                continue
            rows.append({"unit": f[0], "name": f[1], "rva": f[2],
                         "pct": f[3] if len(f) > 3 else "",
                         "size": f[4] if len(f) > 4 else "",
                         "cls": f[5] if len(f) > 5 else ""})
    return rows


@logged
def main(argv=None) -> int:
    ap = argparse.ArgumentParser(prog="homm1 walls semdiff",
                                 description=__doc__.split("\n\n")[0])
    ap.add_argument("token", help="hex rva, mangled name, or CClass::Member")
    ap.add_argument("--top", type=int, default=30)
    ap.add_argument("--all", action="store_true",
                    help="print identical sections too")
    args = ap.parse_args(argv)
    return adjudicate(args.token, args.top, args.all)


@logged
def sweep_main(argv=None) -> int:
    ap = argparse.ArgumentParser(prog="homm1 walls semsweep",
                                 description="screen a worklist TSV")
    ap.add_argument("tsv", help="worklist: unit, name, rva, pct, size, class")
    ap.add_argument("--first", type=int, default=1)
    ap.add_argument("--last", type=int, default=0)
    ap.add_argument("--fp-only", action="store_true",
                    help="report only FP-opcode deltas")
    args = ap.parse_args(argv)
    return sweep(_read_worklist(args.tsv), args.first, args.last,
                 args.fp_only)


if __name__ == "__main__":
    raise SystemExit(main())

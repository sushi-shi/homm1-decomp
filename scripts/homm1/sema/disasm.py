"""homm1.sema.disasm - annotated retail i386 assembly.

    python3 -m homm1.sema.disasm 0x153810
    python3 -m homm1.sema.disasm CImage::RenderFrame --lite
    python3 -m homm1.sema.disasm 0x153810 --blocks
    python3 -m homm1.sema.disasm 0x1eb2f0 --switch

The retail bytes, decoded and labelled with the Model: every relocated
operand is named by the binding it lands in (`clip+0x64`, a pooled literal's
text, a vtable slot), every rel32 call/jmp by the function it reaches
(through the linker's ILT forwarders), and every intra-function branch by its
offset. Assembly only - nothing here decompiles.

Views: the default annotated listing, `--lite` (instructions only), `--blocks`
(basic blocks with in-edges and loop/tail marks) and `--switch` (dereference
the jump table behind an indirect `jmp`).
"""

from __future__ import annotations

import re
import sys

from homm1.sema import SemaError, die, run
from homm1.sema.image import retail
from homm1.sema.index import index
from homm1.tool import ToolError, objdump

_ROW = re.compile(r"^\s*([0-9a-f]+):\t([0-9a-f ]+?)\s*\t(\S.*?)\s*$")
_BRANCH = re.compile(r"^(call|jmp|loop\w*|j[a-z]{1,3})\s+0x([0-9a-f]+)$")
_JCC = ("jmp", "je", "jne", "jz", "jnz", "ja", "jae", "jb", "jbe", "jg", "jge",
        "jl", "jle", "js", "jns", "jo", "jno", "jp", "jnp", "jcxz", "jecxz")


class Insn:
    __slots__ = ("rva", "raw", "text")

    def __init__(self, rva: int, raw: bytes, text: str):
        self.rva, self.raw, self.text = rva, raw, text

    @property
    def end(self) -> int:
        return self.rva + len(self.raw)

    @property
    def mnemonic(self) -> str:
        return self.text.split()[0] if self.text else ""

    def branch_target(self) -> int | None:
        m = _BRANCH.match(re.sub(r"\s+", " ", self.text))
        return int(m.group(2), 16) if m else None


def extent(target: str, size: str | None = None) -> tuple[int, int, object]:
    """(rva, size, binding) for a disasm target: a function start, an interior
    address (the containing function is used) or an explicit --size window."""
    idx = index()
    hits = idx.resolve_name(target)
    if not hits:
        die(f"'{target}' is not a hex RVA and no binding carries that name")
    if len(hits) > 1:
        lines = "\n".join("  " + idx.label(r) for r in hits[:12])
        more = f"\n  ... (+{len(hits) - 12} more)" if len(hits) > 12 else ""
        die(f"'{target}' is ambiguous ({len(hits)} bindings):\n{lines}{more}")
    rva = hits[0]
    b = idx.func(rva) or idx.owner(rva)
    if size is not None:
        return rva, int(size, 16), b
    if b is None:
        nxt = idx.next_start(rva)
        die(f"0x{rva:08x} starts no admitted function row"
            + (f" (the next start is 0x{nxt:08x}; pass --size)" if nxt else ""))
    return b.rva, b.size, b


def decode(rva: int, size: int) -> list[Insn]:
    """The retail bytes at [rva, rva+size), decoded."""
    img = retail()
    blob = img.read(rva, size)
    if blob is None:
        die(f"0x{rva:08x}+0x{size:x} is not mapped in the retail image")
    try:
        text = objdump.disassemble(blob, rva)
    except ToolError as e:
        raise SemaError(str(e)) from e
    out: list[Insn] = []
    for line in text.splitlines():
        m = _ROW.match(line)
        if not m:
            continue
        raw = bytes.fromhex(m.group(2).replace(" ", ""))
        insn = re.sub(r"\s+", " ", m.group(3)).strip()
        if out and not insn:                       # byte-wrap continuation
            out[-1].raw += raw
            continue
        out.append(Insn(int(m.group(1), 16), raw, insn))
    return out


def annotate(insn: Insn, lo: int, hi: int) -> str:
    """The `; ...` note for one instruction: named relocated operands, the
    callee a rel32 reaches, or the block a local branch goes to."""
    idx, img = index(), retail()
    notes = []
    sites = img.relocs_in(insn.rva, insn.end)
    for site, tgt in sites:
        note = idx.ref_label(tgt)
        row, s = idx.at(tgt), img.string_at(tgt)
        if s is not None and (row is None or not row.name):
            note = f"{note} = {s[1]!r}"      # a bare pooled literal
        notes.append(f"{note} (@+0x{site - insn.rva:x})" if len(sites) > 1
                     else note)
    tgt = insn.branch_target()
    if tgt is not None:
        if lo <= tgt < hi:
            notes.append(f"-> +0x{tgt - lo:x}")
        else:
            body = img.jmp_target(tgt)
            b = idx.func(tgt)
            if body is not None and b is not None and b.kind == "thunk":
                notes.append(f"-> {idx.label(body)} (via ILT 0x{tgt:06x})")
            else:
                notes.append(f"-> {idx.label(tgt)}")
    return "  ; " + "  ".join(notes) if notes else ""


def header(rva: int, size: int, b) -> list[str]:
    idx = index()
    out = [f"== {idx.display(b, rva)}  0x{rva:08x}  0x{size:x} B"
           + (f"  [{b.unit}]" if b is not None and b.unit else "")
           + (f"  ({b.channel})" if b is not None and b.channel else "")]
    if b is not None and b.name and b.name != idx.display(b, rva):
        out.append(f"   {b.name}")
    if b is not None and b.aliases:
        out.append("   aliases: " + ", ".join(
            f"{a.name} ({a.channel})" for a in b.aliases))
    return out


def listing(rva: int, size: int, b, lite: bool = False) -> list[str]:
    out = header(rva, size, b)
    for insn in decode(rva, size):
        note = annotate(insn, rva, rva + size)
        if lite:
            out.append(f"    {insn.text}{note}")
        else:
            out.append(f"  {insn.rva:06x}: {insn.raw.hex(' '):<22} "
                       f"{insn.text}{note}")
    return out


def blocks(rva: int, size: int) -> list[str]:
    """Basic blocks: leaders, in-edges, loop heads and shared ret tails."""
    insns = decode(rva, size)
    if not insns:
        return ["(no instructions decoded)"]
    addrs = {i.rva for i in insns}
    lo, hi = insns[0].rva, insns[-1].rva

    def local(i: Insn) -> int | None:
        t = i.branch_target()
        return t if t is not None and lo <= t <= hi and t in addrs else None

    leaders, edges = {lo}, {}
    for n, i in enumerate(insns):
        nxt = insns[n + 1].rva if n + 1 < len(insns) else None
        t = local(i)
        if t is not None:
            leaders.add(t)
            edges.setdefault(i.rva, []).append(t)
            if nxt and i.mnemonic != "jmp":
                edges.setdefault(i.rva, []).append(nxt)
        if nxt and (i.mnemonic in _JCC or i.mnemonic.startswith(("ret", "loop"))):
            leaders.add(nxt)
    preds: dict[int, list[int]] = {}
    for n, i in enumerate(insns):
        for d in edges.get(i.rva, ()):
            preds.setdefault(d, []).append(i.rva)
        nxt = insns[n + 1].rva if n + 1 < len(insns) else None
        if (nxt in leaders and local(i) is None and i.mnemonic != "jmp"
                and not i.mnemonic.startswith("ret")):
            preds.setdefault(nxt, []).append(i.rva)

    out = []
    for n, i in enumerate(insns):
        if i.rva in leaders:
            ins = preds.get(i.rva, [])
            tag = "  <=== LOOP HEAD" if any(p > i.rva for p in ins) else ""
            if len(ins) > 2 and any(x.mnemonic.startswith("ret")
                                    for x in insns[n:n + 8]):
                tag += f"  <=== COMMON TAIL ({len(ins)} in-edges)"
            src = ", ".join(f"@{p:x}" for p in sorted(ins)) or "entry"
            out += ["", f"block @{i.rva:06x}:   in: {src}{tag}"]
        t = local(i)
        arrow = (f"   -> @{t:06x}" + ("  ^loop" if t <= i.rva else "")
                 if t is not None else "")
        out.append(f"  {i.rva:06x}:  {i.text}{arrow}"
                   + annotate(i, rva, rva + size))
    return out


def switch_tables(rva: int, size: int) -> tuple[list[str], int]:
    """Dereference the jump table(s) behind an indirect `jmp [reg*4+TABLE]`.

    MSVC 5.0 emits a dense form (`jmp [reg*4+TARGETS]`) and a sparse one that
    routes through an index byte (`mov dl,[reg+INDEX]`), where several source
    cases share one slot - that sharing IS the evidence that a case RUN reaches
    one arm. The selector is usually biased first (`add eax,-2`, `lea ecx,[eax-1]`),
    so table index 0 is rarely source case 0; printing indices as case labels
    would be a lie."""
    img = retail()
    insns = decode(rva, size)
    texts = [i.text for i in insns]
    ji = next((n for n, t in enumerate(texts)
               if re.search(r"jmp\s+DWORD PTR \[e[a-z]{2}\*4\+0x[0-9a-f]+\]", t)),
              None)
    if ji is None:
        return (["[no indirect `jmp [reg*4+TABLE]` here - not a jump-table "
                 "switch]"], 1)
    tva = int(re.search(r"\*4\+0x([0-9a-f]+)\]", texts[ji]).group(1), 16)
    iva = bias = n = None
    for t in reversed(texts[max(0, ji - 14):ji]):
        m = re.search(r"mov\s+[a-z]l,BYTE PTR \[e[a-z]{2}\+0x([0-9a-f]+)\]", t)
        if m and iva is None:
            iva = int(m.group(1), 16)
        m = re.search(r"cmp\s+e[a-z]{2},0x([0-9a-f]+)", t)
        if m and n is None:
            n = int(m.group(1), 16) + 1
        m = re.search(r"add\s+e[a-z]{2},0x([0-9a-f]+)", t)
        if m and bias is None:
            v = int(m.group(1), 16)
            bias = -(0x100000000 - v) if v > 0x7FFFFFFF else v
        m = re.search(r"sub\s+e[a-z]{2},0x([0-9a-f]+)", t)
        if m and bias is None:
            bias = -int(m.group(1), 16)
        m = re.search(r"lea\s+e[a-z]{2},\[e[a-z]{2}([+-])0x([0-9a-f]+)\]", t)
        if m and bias is None:
            bias = (1 if m.group(1) == "+" else -1) * int(m.group(2), 16)
    head = f"[switch @ 0x{rva:06x}: targets 0x{tva - img.base:x}"
    if iva:
        head += f", index bytes 0x{iva - img.base:x}"
    if n is not None:
        head += f", {n} case slot(s)"
    lo = -bias if bias else 0
    if bias:
        head += f", selector biased by {bias} -> first case is {lo}"
    out = [head + "]"]
    if n is None:
        return (out + ["  (no `cmp reg,N` bound found - read the guard by hand)"], 1)
    runs: dict[tuple, list[int]] = {}
    for k in range(n):
        slot = (img.read(iva - img.base + k, 1)[0] if iva else k)
        t = img.u32(tva - img.base + slot * 4) - img.base
        runs.setdefault((slot, t) if iva else (None, t), []).append(k + lo)
    idx = index()
    for (slot, t), cases in sorted(runs.items(), key=lambda kv: kv[1][0]):
        cs = ",".join(str(c) for c in cases)
        out.append(f"  case {cs:<24} -> 0x{t:06x} (+0x{t - rva:x})"
                   + (f"   [slot {slot}]" if iva else "")
                   + ("   <- ONE arm, shared by a run" if len(cases) > 1 else "")
                   + ("" if idx.owner(t) and idx.owner(t).rva == rva
                      else f"   {idx.label(t)}"))
    out.append("  (cases sharing a target are ONE source arm)")
    return (out, 0)


from homm1.core.usage import logged


@logged
def main(argv: list[str] | None = None) -> int:
    import argparse
    ap = argparse.ArgumentParser(prog="homm1 sema disasm",
                                 description=__doc__.split("\n\n")[0])
    ap.add_argument("target", help="hex RVA, mangled name or CClass::Member")
    ap.add_argument("--size", help="decode this many bytes instead of the "
                                   "binding's extent (hex)")
    ap.add_argument("--lite", action="store_true", help="instructions only")
    ap.add_argument("--blocks", action="store_true", help="basic-block view")
    ap.add_argument("--switch", action="store_true",
                    help="dereference the jump table behind an indirect jmp")
    args = ap.parse_args(argv)
    rva, size, b = extent(args.target, args.size)
    if args.switch:
        lines, rc = switch_tables(rva, size)
        print("\n".join(lines))
        return rc
    lines = (blocks(rva, size) if args.blocks
             else listing(rva, size, b, lite=args.lite))
    print("\n".join(lines))
    return 0


if __name__ == "__main__":
    sys.exit(run(__name__, sys.argv[1:]))

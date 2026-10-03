"""Capstone listings of VC4 COFF objects and a retail distance per function.

    python3 -m homm1.research.vc4trace.objdis OBJ [FUNC]
    python3 -m homm1.research.vc4trace.objdis --distance UNIT FUNC [OBJ]

--distance counts differing normalized lines (addresses, branch targets and
relocation names masked) between OBJ (default: the current compare-new base
object) and the delinked retail target object of UNIT.
"""
from __future__ import annotations

import argparse
import difflib
import re
import struct
from pathlib import Path

import capstone

from homm1.core.inputs import REPO
from homm1.core.usage import logged


def listing(path, want=''):
    """[(function name, [(address, text, relocation name or '')])] for code symbols matching WANT."""
    d = Path(path).read_bytes()
    _m, nsec, _t, symptr, nsym, opt, _c = struct.unpack_from('<HHIIIHH', d, 0)
    strtab = symptr + nsym * 18

    def name(raw):
        if raw[:4] == b'\0\0\0\0':
            o = strtab + struct.unpack_from('<I', raw, 4)[0]
            return d[o:d.find(b'\0', o)].decode('latin-1')
        return raw.rstrip(b'\0').decode('latin-1')
    syms, i = {}, 0
    while i < nsym:
        o = symptr + i * 18
        v, sec, ty, st, na = struct.unpack_from('<IhHBB', d, o + 8)
        syms[i] = (name(d[o:o + 8]), v, sec, st, ty)
        i += 1 + na
    md = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)
    out = []
    for k in range(nsec):
        o = 20 + opt + 40 * k
        size, ptr, rptr, _l, nrel = struct.unpack_from('<IIIIH', d, o + 16)
        if not struct.unpack_from('<I', d, o + 36)[0] & 0x20:
            continue
        code = d[ptr:ptr + size]
        rel = {}
        for r in range(nrel):
            va, si, _ty = struct.unpack_from('<IIH', d, rptr + 10 * r)
            rel[va] = syms[si][0]
        fns = sorted((v, n) for n, v, sec, st, ty in syms.values()
                     if sec == k + 1 and (ty & 0x20 or st == 2) and not n.startswith(('.', '$')))
        for j, (start, fname) in enumerate(fns):
            if want not in fname:
                continue
            end = fns[j + 1][0] if j + 1 < len(fns) else len(code)
            rows = []
            for ins in md.disasm(code[start:end], start):
                r = [rel[a] for a in range(ins.address, ins.address + ins.size) if a in rel]
                rows.append((ins.address, f'{ins.mnemonic} {ins.op_str}'.strip(), r[0] if r else ''))
            out.append((fname, rows))
    return out


def normalized(path, func):
    fns = [rows for fname, rows in listing(path, func) if fname == func or fname.startswith('?' + func + '@')]
    if not fns:
        fns = [rows for _f, rows in listing(path, func)]
    lines = []
    for _a, text, rel in (fns[0] if fns else []):
        if rel:   # relocated operand: the addend is data-matching territory (relaxed)
            text = re.sub(r'\[(?:0x[0-9a-f]+|\d+)\]', '[R]', text)
            text = re.sub(r'\[([a-z]{3}(?:\*\d)?(?: \+ [a-z]{3}(?:\*\d)?)?)(?: [+-] (?:0x[0-9a-f]+|\d+))?\]',
                          r'[\1+R]', text)
            text = re.sub(r', (?:0x[0-9a-f]+|\d+)$', ', R', text)
            text = re.sub(r'^push (?:0x[0-9a-f]+|\d+)$', 'push R', text)
        text = re.sub(r'^(j[a-z]+|call) +0x[0-9a-f]+$', r'\1 L', text)
        text = re.sub(r'0x[0-9a-f]{3,}', 'ADDR', text)
        lines.append(text)
    return lines


def retail_obj(unit):
    return REPO / f'build/objdiff/compare-new/target/{unit}.c.obj'


def canonical(obj):
    """A compare-canonical copy of a raw compiler object (jump tables and private
    labels named the way the compare-new base objects are), cached beside it."""
    from homm1.compare.canonicalize import canonicalize_coff
    obj = Path(obj)
    out = obj.with_suffix('.canon.obj')
    if not out.exists() or out.stat().st_mtime < obj.stat().st_mtime:
        out.write_bytes(canonicalize_coff(obj.read_bytes()).data)
    return out


def distance(obj, unit, func, retail=None):
    """Differing normalized lines of FUNC in OBJ against retail (0 = code-identical).
    Trailing int3/nop padding is ignored."""
    if 'compare-new' not in str(obj):
        obj = canonical(obj)
    a = normalized(obj, func)
    b = retail if retail is not None else normalized(retail_obj(unit), func)
    strip = lambda xs: [x for x in xs if x not in ('nop', 'int3')]
    sm = difflib.SequenceMatcher(None, strip(a), strip(b), autojunk=False)
    return sum(max(i2 - i1, j2 - j1) for tag, i1, i2, j1, j2 in sm.get_opcodes() if tag != 'equal')


@logged
def main(argv: list[str] | None = None) -> int:
    ap = argparse.ArgumentParser(prog='python3 -m homm1.research.vc4trace.objdis', description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument('args', nargs='+')
    ap.add_argument('--distance', action='store_true')
    a = ap.parse_args(argv)
    if a.distance:
        unit, func = a.args[:2]
        obj = a.args[2] if len(a.args) > 2 else REPO / f'build/objdiff/compare-new/base/{unit}.obj'
        print(distance(obj, unit, func))
        return 0
    for fname, rows in listing(a.args[0], a.args[1] if len(a.args) > 1 else ''):
        print(f'== {fname}')
        for addr, text, rel in rows:
            print(f'  {addr:5x}: {text}' + (f'   ; {rel}' if rel else ''))
    return 0


if __name__ == '__main__':
    raise SystemExit(main())

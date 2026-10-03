"""tracer.py - a ptrace-free "debugger" for the pinned VC4 C2.EXE (or C1XX.EXE).

Yama ptrace_scope=2 on this host forbids ptrace (winedbg cannot launch or attach,
gdb cannot attach), so tracepoints are compiled INTO a patched copy of the
executable: each tracepoint address gets a 5-byte `jmp` to a cave in a new
`.trc` section that does pushad/pushfd, printf(fmt, args...) through the
MSVCRT40 import, restores state, replays the displaced instructions and jumps
back.  The original toolchain file is never modified.

usage: python3 -m homm1.research.vc4trace.tracer SPEC OUT.EXE [--c2 IN.EXE]
SPEC lines:   ADDR | "printf format" | arg arg ...
  ADDR   hex VA of an instruction boundary (>=5 bytes of whole instructions are
         displaced; rel8/rel32 jmp/jcc/call among them are re-targeted)
  arg    eax..edi | esp (value at the tracepoint) | d[X+n] u4 | w[X+n] u2 |
         sw[X+n] s2 | b[X+n] u1 | sb[X+n] s1   with X itself an arg (nesting ok)
Lines starting with '#' are comments.  `@limit N` after the args stops the
tracepoint printing after N hits (per tracepoint counter).
"""
from __future__ import annotations

import argparse
import re
import struct

import capstone

from homm1.core.usage import logged
from homm1.research.vc4trace.common import c2_exe

def _abs(v): return v
REG_OFF = {'edi': 4, 'esi': 8, 'ebp': 12, 'esp': 16, 'ebx': 20, 'edx': 24, 'ecx': 28, 'eax': 32}

def parse_arg(s):
    s = s.strip()
    if s in REG_OFF: return ('reg', s)
    if re.fullmatch(r'(0x[0-9a-fA-F]+|\d+)', s): return ('imm', int(s, 0))
    m = re.fullmatch(r'(d|w|sw|b|sb)\[(.*)\]', s)
    if not m: raise SystemExit(f'bad tracepoint argument {s!r}')
    size, inner = m.groups()
    # split trailing +n / -n at top level
    depth = 0; cut = None
    for i in range(len(inner) - 1, -1, -1):
        c = inner[i]
        if c == ']': depth += 1
        elif c == '[': depth -= 1
        elif c in '+-' and depth == 0: cut = i; break
    if cut is None: base, disp = inner, 0
    else: base, disp = inner[:cut], int(inner[cut:].replace(' ', ''), 0)
    return ('mem', size, parse_arg(base), disp)

def emit_arg(a, depth):
    """code leaving the arg value in eax; depth = dwords pushed since pushfd."""
    if a[0] == 'imm':
        return b'\xb8' + struct.pack('<I', a[1] & 0xffffffff)      # mov eax,imm
    if a[0] == 'reg':
        return b'\x8b\x84\x24' + struct.pack('<i', REG_OFF[a[1]] + 4 * depth)   # mov eax,[esp+off]
    _, size, base, disp = a
    code = emit_arg(base, depth)
    op = {'d': b'\x8b\x80', 'w': b'\x0f\xb7\x80', 'sw': b'\x0f\xbf\x80', 'b': b'\x0f\xb6\x80', 'sb': b'\x0f\xbe\x80'}[size]
    return code + op + struct.pack('<i', disp)

class PE:
    def __init__(self, path):
        self.d = bytearray(open(path, 'rb').read()); d = self.d
        self.pe = struct.unpack_from('<I', d, 0x3c)[0]
        self.nsec = struct.unpack_from('<H', d, self.pe + 6)[0]
        self.opt = struct.unpack_from('<H', d, self.pe + 20)[0]
        self.base = struct.unpack_from('<I', d, self.pe + 52)[0]
        self.salign, self.falign = struct.unpack_from('<II', d, self.pe + 56)
        self.sht = self.pe + 24 + self.opt
    def secs(self):
        for k in range(self.nsec):
            o = self.sht + 40 * k
            vs, va, rs, rp = struct.unpack_from('<IIII', self.d, o + 8)
            yield o, self.d[o:o + 8].rstrip(b'\0'), va, vs, rs, rp
    def off(self, va):
        rva = va - self.base
        for o, n, sva, vs, rs, rp in self.secs():
            if sva <= rva < sva + rs: return rp + rva - sva
        raise ValueError(hex(va))
    def add_section(self, name, size):
        last = list(self.secs())[-1]
        va = (last[2] + max(last[3], last[4]) + self.salign - 1) // self.salign * self.salign
        rp = (len(self.d) + self.falign - 1) // self.falign * self.falign
        rs = (size + self.falign - 1) // self.falign * self.falign
        o = self.sht + 40 * self.nsec
        assert o + 40 <= struct.unpack_from('<I', self.d, self.pe + 24 + 60)[0], 'no header room'
        self.d[o:o + 40] = name.ljust(8, b'\0') + struct.pack('<IIIIIIHHI', rs, va, rs, rp, 0, 0, 0, 0, 0xE0000020)
        self.nsec += 1; struct.pack_into('<H', self.d, self.pe + 6, self.nsec)
        struct.pack_into('<I', self.d, self.pe + 24 + 56, (va + rs + self.salign - 1) // self.salign * self.salign)
        self.d += b'\0' * (rp - len(self.d)) + b'\0' * rs
        return self.base + va, rp

def imp(pe, dll, fn):
    rva = struct.unpack_from('<I', pe.d, pe.pe + 24 + 104)[0]; o = pe.off(pe.base + rva)
    while True:
        ilt, _, _, name, iat = struct.unpack_from('<IIIII', pe.d, o)
        if not name: break
        nm = pe.d[pe.off(pe.base + name):].split(b'\0')[0].decode()
        if nm.lower() == dll.lower():
            i = 0
            while True:
                e = struct.unpack_from('<I', pe.d, pe.off(pe.base + (ilt or iat)) + 4 * i)[0]
                if not e: break
                if not e & 0x80000000 and pe.d[pe.off(pe.base + e) + 2:].split(b'\0')[0].decode() == fn:
                    return pe.base + iat + 4 * i
                i += 1
        o += 20
    raise SystemExit(f'{dll}!{fn} not imported')

def relocate(ins, at):
    """re-encode one displaced instruction for address `at`."""
    b = bytes(ins.bytes); m = ins.mnemonic
    if m in ('call', 'jmp') or (m.startswith('j') and m != 'jmp' and ins.op_str.startswith('0x')):
        if not ins.op_str.startswith('0x'): return b
        tgt = int(ins.op_str, 16)
        if m == 'call': return b'\xe8' + struct.pack('<i', tgt - (at + 5))
        if m == 'jmp': return b'\xe9' + struct.pack('<i', tgt - (at + 5))
        cc = b[0] - 0x70 if b[0] < 0x80 else b[1] - 0x80
        return bytes([0x0f, 0x80 + cc]) + struct.pack('<i', tgt - (at + 6))
    if m in ('loop', 'jecxz'): raise SystemExit('cannot relocate ' + m)
    return b

def patch(spec, dst, src=None):
    """Write a copy of SRC (default: the pinned C2.EXE) with SPEC's tracepoints to DST."""
    pe = PE(src or c2_exe()); printf = imp(pe, 'MSVCRT40.dll', 'printf')
    tps = []
    for line in open(spec):
        line = line.strip()
        if not line or line.startswith('#'): continue
        addr, fmt, args = [x.strip() for x in line.split('|', 2)]
        limit = None; post = '@post' in args; args = args.replace('@post', '')
        m = re.search(r'@limit\s+(\d+)', args)
        if m: limit = int(m.group(1)); args = args[:m.start()]
        fmt = bytes(fmt.strip('"'), 'latin-1').decode('unicode_escape').encode('latin-1')
        tps.append((int(addr, 16), fmt, args.split(), limit, post))
    cave_va, cave_off = pe.add_section(b'.trc', 0x20000)
    md = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)
    cur = 0; blob = bytearray()
    for addr, fmt, args, limit, post in tps:
        o = pe.off(addr); disp = []; n = 0
        for ins in md.disasm(bytes(pe.d[o:o + 32]), addr):
            disp.append(ins); n += ins.size
            if n >= 5 or ins.mnemonic == 'ret': break
        assert n >= 5, f'{addr:#x}: only {n} bytes before ret'
        here = cave_va + len(blob)
        # data: counter dword + fmt
        cnt_va = here; blob += b'\0\0\0\0'
        fmt_va = cave_va + len(blob); blob += fmt + b'\0'
        while len(blob) % 16: blob += b'\xcc'
        code_va = cave_va + len(blob)
        c = bytearray()
        tail = disp
        if post:   # run the displaced instructions up to a final ret first, trace, then ret
            assert disp[-1].mnemonic == 'ret', 'post needs the displaced run to end in ret'
            for ins in disp[:-1]: c += relocate(ins, code_va + len(c))
            tail = disp[-1:]
        c += b'\x60\x9c'                                           # pushad; pushfd
        skip_fix = None
        if limit is not None:
            c += b'\x81\x3d' + struct.pack('<II', cnt_va, limit)    # cmp dword [cnt], limit
            c += b'\x0f\x83'; skip_fix = len(c); c += b'\0\0\0\0'   # jae skip
            c += b'\xff\x05' + struct.pack('<I', cnt_va)            # inc dword [cnt]
        pa = [parse_arg(a) for a in args]
        for depth, a in enumerate(reversed(pa)):
            c += emit_arg(a, depth) + b'\x50'                      # push eax
        c += b'\x68' + struct.pack('<I', fmt_va)                    # push fmt
        c += b'\xff\x15' + struct.pack('<I', printf)                # call [printf]
        c += b'\x81\xc4' + struct.pack('<I', 4 * (len(pa) + 1))     # add esp,n
        if skip_fix is not None: struct.pack_into('<i', c, skip_fix, len(c) - (skip_fix + 4))
        c += b'\x9d\x61'                                            # popfd; popad
        for ins in tail: c += relocate(ins, code_va + len(c))
        if tail[-1].mnemonic != 'ret':
            c += b'\xe9' + struct.pack('<i', (addr + n) - (code_va + len(c) + 5))
        blob += c
        while len(blob) % 16: blob += b'\xcc'
        patch = b'\xe9' + struct.pack('<i', code_va - (addr + 5)) + b'\x90' * (n - 5)
        pe.d[o:o + n] = patch
    pe.d[cave_off:cave_off + len(blob)] = blob
    open(dst, 'wb').write(pe.d)
    return len(tps), cave_va, len(blob)


@logged
def main(argv: list[str] | None = None) -> int:
    ap = argparse.ArgumentParser(prog='python3 -m homm1.research.vc4trace.tracer', description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument('spec'); ap.add_argument('out')
    ap.add_argument('--c2', help='executable to patch (default: the pinned C2.EXE)')
    a = ap.parse_args(argv)
    n, cave, size = patch(a.spec, a.out, a.c2)
    print(f'{n} tracepoint(s), cave {cave:#x}+{size:#x}')
    return 0


if __name__ == '__main__':
    raise SystemExit(main())

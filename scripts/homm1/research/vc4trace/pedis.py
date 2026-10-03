"""Capstone listing of the pinned C2.EXE (or another PE) at a virtual address.

    python3 -m homm1.research.vc4trace.pedis VA [COUNT] [--pe PATH]
"""
from __future__ import annotations

import argparse
import struct

import capstone

from homm1.core.usage import logged
from homm1.research.vc4trace.common import c2_exe


@logged
def main(argv: list[str] | None = None) -> int:
    ap = argparse.ArgumentParser(prog='python3 -m homm1.research.vc4trace.pedis', description=__doc__)
    ap.add_argument('va'); ap.add_argument('count', nargs='?', type=int, default=60)
    ap.add_argument('--pe')
    a = ap.parse_args(argv)
    d = open(a.pe or c2_exe(), 'rb').read()
    pe = struct.unpack_from('<I', d, 0x3c)[0]
    nsec, opt = struct.unpack_from('<H', d, pe + 6)[0], struct.unpack_from('<H', d, pe + 20)[0]
    base = struct.unpack_from('<I', d, pe + 52)[0]
    va = int(a.va, 16)
    for k in range(nsec):
        vs, vaddr, rs, rp = struct.unpack_from('<IIII', d, pe + 24 + opt + 40 * k + 8)
        if vaddr <= va - base < vaddr + rs:
            o = rp + va - base - vaddr
            break
    else:
        raise SystemExit(f'{a.va} is outside the image')
    md = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)
    for i, ins in enumerate(md.disasm(d[o:o + 16 * a.count], va)):
        if i >= a.count:
            break
        print(f'{ins.address:08x}: {ins.mnemonic:6s} {ins.op_str}')
    return 0


if __name__ == '__main__':
    raise SystemExit(main())

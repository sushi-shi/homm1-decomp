"""homm1.verify.claim_size - an RVA() claim covers its whole contribution (fast tier).

    python3 -m homm1.verify.claim_size

A function's `RVA(addr, size)` must reach the end of everything cl emitted for
it: code, alignment filler and the jump/index tables that follow the body
(docs/build-system.md). A code-only size leaves the tables outside the claim;
their relocations then bleed into the next function's comparison and the
claimed function scores its own residue against a truncated target.

For each claim the bytes between `addr + size` and the next admitted function
start must be at most 15 bytes of 0x90/0xcc alignment padding. Anything else
(a table's addresses, more code) means the size stops short.
"""

from __future__ import annotations

from homm1.core.usage import logged

import bisect
import re
import sys

from homm1.core.paths import RETAIL
from homm1.core.pe import image
from homm1.verify.srcscan import blank_comments, rel, source_files

_RVA = re.compile(r"^VA\((0x[0-9a-fA-F]+),\s*(0x[0-9a-fA-F]+|\d+)\)", re.M)
PADDING = {0x90, 0xCC}
MAX_PAD = 15


def _starts() -> list[int]:
    return sorted(int(line.split("\t", 1)[0], 16)
                  for line in (RETAIL / "functions.tsv").read_text().splitlines()
                  if line.startswith("0x"))


def gate_findings() -> list[str]:
    img = image()
    text = img.section(".text")
    lo, raw = text["va"], img.data[text["rptr"]:text["rptr"] + text["rsize"]]
    hi = lo + len(raw)
    starts = _starts()
    out = []
    for path in source_files((".c", ".cpp")):
        code = blank_comments(path.read_text(encoding="utf-8", errors="replace"))
        for m in _RVA.finditer(code):
            rva, size = int(m.group(1), 16) - 0x400000, int(m.group(2), 0)
            if not lo <= rva < hi:
                continue
            i = bisect.bisect_right(starts, rva)
            nxt = starts[i] if i < len(starts) else hi
            if size <= 0 or rva + size > nxt:
                out.append(f"{rel(path)}: claim at {rva:#x} crosses next function {nxt:#x}")
                continue
            gap = raw[rva + size - lo:nxt - lo]
            if len(gap) > MAX_PAD or any(b not in PADDING for b in gap):
                end = nxt
                while end > rva + size and raw[end - 1 - lo] in PADDING:
                    end -= 1
                out.append(f"{rel(path)}: VA(0x{rva + 0x400000:08x}, 0x{size:x}) stops "
                           f"{end - rva - size:#x} bytes short of its contribution "
                           f"(0x{end - rva:x}; next start 0x{nxt:06x})")
    return out


@logged
def main(argv=None) -> int:
    import argparse
    argparse.ArgumentParser(description=__doc__).parse_args(argv)
    findings = gate_findings()
    for f in findings:
        print(f"[claim-size] {f}")
    print(f"[claim-size] {len(findings)} finding(s)")
    return 1 if findings else 0


if __name__ == "__main__":
    raise SystemExit(main())

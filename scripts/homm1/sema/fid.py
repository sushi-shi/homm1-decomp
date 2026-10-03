"""Read-only cross-build fingerprints; candidate identities are not claims.

Mask PE HIGHLOW fields and decoded relative branch operands, then search the
other image using the longest unmasked byte run. Validate the entire body and
relocation-site shape. Link order disambiguates duplicate bodies only between
unique anchors. No similarity threshold silently admits a changed function.
"""
from __future__ import annotations

import argparse
import bisect
import hashlib
import json
from pathlib import Path
from collections import Counter

import capstone

from homm1.core.pe import Pe
from homm1.core.tsv import read
from homm1.core.usage import logged
from homm1.sema.image import Image


def body(image: Image, start: int, end: int) -> tuple[bytes, bytearray]:
    """Bytes and mask; padding-only census entries yield an empty body.

    These are discovery fingerprints, not strict matching scores. In particular,
    referent identity and branch destinations still require independent review.
    """
    raw = image.read(start, end - start)
    if raw is None:
        raise ValueError(f"unmapped census extent: 0x{start:x}..0x{end:x}")
    raw = raw.rstrip(b"\xcc\x90")
    mask = bytearray(len(raw))
    for site, _ in image.relocs_in(start, start + len(raw)):
        mask[site-start:site-start+4] = b"\1" * 4
    decoder = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)
    decoder.detail = True
    for ins in decoder.disasm(raw, start):
        if ((ins.group(capstone.CS_GRP_CALL) or ins.group(capstone.CS_GRP_JUMP))
                and ins.imm_size in (1, 2, 4)
                and ins.imm_offset + ins.imm_size <= ins.size):
            off = ins.address-start+ins.imm_offset
            mask[off:off+ins.imm_size] = b"\1" * ins.imm_size
    return raw, mask


def candidates(old, new, start, end):
    raw, mask = body(old, start, end)
    runs, pos = [], 0
    while pos < len(raw):
        if mask[pos]:
            pos += 1
            continue
        stop = pos + 1
        while stop < len(raw) and not mask[stop]:
            stop += 1
        runs.append((stop-pos, pos))
        pos = stop
    if not runs:
        return [], len(raw)
    size, offset = max(runs)
    if size < 3:
        return [], len(raw)
    text = new.read(new.text_lo, new.text_hi-new.text_lo)
    needle = raw[offset:offset+size]
    hits, pos = [], 0
    reloc_offsets = {s-start for s, _ in old.relocs_in(start, start+len(raw))}
    while (pos := text.find(needle, pos)) >= 0:
        at = new.text_lo+pos-offset
        pos += 1
        if at < new.text_lo or at+len(raw) > new.text_hi:
            continue
        other = new.read(at, len(raw))
        if any(a != b and not m for a, b, m in zip(raw, other, mask)):
            continue
        if {s-at for s, _ in new.relocs_in(at, at+len(raw))} != reloc_offsets:
            continue
        hits.append(at)
    return hits, len(raw)


def compare(old_path, new_path, census):
    old, new = Image(Pe(old_path)), Image(Pe(new_path))
    starts = sorted(int(r['rva'], 16) for r in read(census)[2])
    if (not starts or len(starts) != len(set(starts))
            or starts[0] < old.text_lo or starts[-1] >= old.text_hi):
        raise ValueError("census must contain unique starts within the old .text")
    rows = []
    for start, end in zip(starts, starts[1:]+[old.text_hi]):
        hits, size = candidates(old, new, start, end)
        raw, mask = body(old, start, end)
        normalized = bytes(0 if m else b for b, m in zip(raw, mask))
        fingerprint = hashlib.sha256(normalized + mask).hexdigest() if raw else None
        rows.append(dict(old_rva=start, size=size, fid=fingerprint, candidates=hits))
    anchors = {r['old_rva']: r['candidates'][0] for r in rows if len(r['candidates']) == 1}
    # Verify the order assumption instead of assuming every unique hit is sound.
    ordered = sorted(anchors)
    inversions = [(a, b) for a, b in zip(ordered, ordered[1:]) if anchors[a] >= anchors[b]]
    if inversions:
        raise ValueError(f'non-monotonic unique anchors: {inversions}')
    for r in rows:
        start = r['old_rva']
        i = bisect.bisect_left(ordered, start)
        lo = anchors[ordered[i-1]] if i else new.text_lo-1
        hi = anchors[ordered[i]] if i < len(ordered) and ordered[i] != start else new.text_hi
        if i < len(ordered) and ordered[i] == start:
            hi = anchors[ordered[i+1]] if i+1 < len(ordered) else new.text_hi
        hits = [v for v in r['candidates'] if lo < v < hi]
        r['status'] = 'unique-fid' if len(r['candidates']) == 1 else 'order-fid' if len(hits) == 1 else 'unresolved'
        r['new_rva'] = hits[0] if len(hits) == 1 else None
        r['bracket'] = [lo, hi]
    return dict(old_sha256=hashlib.sha256(Path(old_path).read_bytes()).hexdigest(),
                new_sha256=hashlib.sha256(Path(new_path).read_bytes()).hexdigest(), functions=rows)


@logged
def main(argv=None):
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--old', type=Path, required=True)
    ap.add_argument('--new', type=Path, required=True)
    ap.add_argument('--census', type=Path, required=True)
    ap.add_argument('--out', type=Path, required=True)
    a = ap.parse_args(argv)
    if a.out.resolve() in {p.resolve() for p in (a.old, a.new, a.census)}:
        ap.error("--out must differ from the input files")
    report = compare(a.old, a.new, a.census)
    a.out.parent.mkdir(parents=True, exist_ok=True)
    a.out.write_text(json.dumps(report, indent=2)+'\n')
    print(dict(Counter(r['status'] for r in report['functions'])))
    return 0


if __name__ == '__main__':
    raise SystemExit(main())

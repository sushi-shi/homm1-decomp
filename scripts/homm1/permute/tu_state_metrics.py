#!/usr/bin/env python3
"""COFF metrics for controlled TU-state and source-shape experiments.

Raw ``.text`` bytes and the ordered relocation stream are measured separately.
This supports both one-COMDAT-per-function ``/O2`` objects and shared-section
objects without depending on disassembler formatting.
"""

from __future__ import annotations

from homm1.core.usage import logged

import argparse
import hashlib
import struct
import sys
from pathlib import Path


_I386_RELOCATION_WIDTHS = {
    0x0001: 2,  # DIR16
    0x0002: 2,  # REL16
    0x0006: 4,  # DIR32
    0x0007: 4,  # DIR32NB
    0x0009: 2,  # SEG12
    0x000A: 2,  # SECTION
    0x000B: 4,  # SECREL
    0x000C: 4,  # TOKEN
    0x000D: 1,  # SECREL7
    0x0014: 4,  # REL32
}


def _sha(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()[:16]


def _cstr(data: bytes) -> str:
    return data.split(b"\0", 1)[0].decode("latin1", "replace")


def read_coff(path: Path):
    data = path.read_bytes()
    if len(data) < 20:
        raise ValueError(f"{path}: truncated COFF header")
    _machine, nsec, _stamp, symptr, nsym, optsz, _chars = struct.unpack_from(
        "<HHLLLHH", data
    )
    sec_off = 20 + optsz
    str_off = symptr + nsym * 18
    strtab = data[str_off:] if str_off < len(data) else b""

    def name_at(raw: bytes) -> str:
        zero, off = struct.unpack("<LL", raw)
        if zero == 0 and off and off < len(strtab):
            return _cstr(strtab[off:])
        return _cstr(raw)

    sections = {}
    for i in range(nsec):
        off = sec_off + i * 40
        raw_name = data[off : off + 8]
        (_vsize, _vaddr, size, rawptr, relptr, _lineptr, nrel, _nline, chars) = (
            struct.unpack_from("<LLLLLLHHL", data, off + 8)
        )
        sections[i + 1] = {
            "name": name_at(raw_name),
            "bytes": data[rawptr : rawptr + size] if rawptr else b"",
            "relptr": relptr,
            "nrel": nrel,
            "chars": chars,
        }

    symbols = {}
    section_functions = {}
    # Static function symbols (e.g. VC4's dynamic-initializer `_$E1`) end the
    # preceding function's extent but get no row of their own.
    section_boundaries = {}
    i = 0
    while i < nsym:
        off = symptr + i * 18
        raw_name = data[off : off + 8]
        value, secnum, typ, storage, naux = struct.unpack_from(
            "<LhHBB", data, off + 8
        )
        name = name_at(raw_name)
        symbols[i] = name
        if secnum > 0 and storage == 2 and typ == 0x20:
            section_functions.setdefault(secnum, []).append((value, name))
        if secnum > 0 and storage in (2, 3) and typ == 0x20:
            section_boundaries.setdefault(secnum, set()).add(value)
        i += 1 + naux

    rows = []
    for secnum, functions in sorted(section_functions.items()):
        sec = sections[secnum]
        if sec["name"] != ".text":
            continue
        relocs = []
        for j in range(sec["nrel"]):
            roff = sec["relptr"] + j * 10
            offset, symidx, reltype = struct.unpack_from("<LLH", data, roff)
            width = _I386_RELOCATION_WIDTHS.get(reltype)
            addend = sec["bytes"][offset : offset + width].hex() \
                if width is not None else None
            relocs.append(
                (offset, reltype, symbols.get(symidx, "#" + str(symidx)), addend)
            )
        starts = sorted(section_boundaries.get(secnum, set())
                        | set(value for value, _name in functions))
        next_start = {
            start: starts[index + 1] if index + 1 < len(starts)
            else len(sec["bytes"])
            for index, start in enumerate(starts)
        }
        for start, function in sorted(functions):
            end = next_start[start]
            body = sec["bytes"][start:end]
            selected = [reloc for reloc in relocs if start <= reloc[0] < end]
            targets = [
                f"{reltype:04x}:{target}"
                for _offset, reltype, target, _addend in selected
            ]
            stream = [
                f"{offset - start:08x}:{reltype:04x}:{target}:"
                f"{addend if addend is not None else '<unknown>'}"
                for offset, reltype, target, addend in selected
            ]
            rows.append({
                "function": function,
                "bytes": body,
                "size": len(body),
                "text_sha": _sha(body),
                "relocs": len(targets),
                "reloc_sha": _sha("\n".join(targets).encode()),
                "reloc_stream": stream,
                "reloc_detail_sha": _sha("\n".join(stream).encode()),
                "reloc_stream_complete": all(
                    addend is not None
                    for _offset, _reltype, _target, addend in selected
                ),
            })
    return _sha(data), rows


def source_hashes():
    """Current source fingerprints through the supported verify layer."""
    try:
        from homm1.verify.fingerprints import load_cache
    except ImportError:
        return {}
    _by_rva, by_name = load_cache()
    return by_name


@logged
def main(argv: list[str] | None = None) -> int:
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("object", type=Path)
    ap.add_argument("--unit", help="config unit name, e.g. gamelevel")
    ap.add_argument("--function", action="append", help="exact mangled symbol filter")
    ap.add_argument("--compare", type=Path, help="second COFF object; report byte delta")
    args = ap.parse_args(argv)
    object_sha, rows = read_coff(args.object)
    comparison = {}
    if args.compare:
        _comparison_sha, comparison_rows = read_coff(args.compare)
        comparison = {row["function"]: row for row in comparison_rows}
    hashes = source_hashes() if args.unit else {}
    wanted = set(args.function or ())
    suffix = "\tcompare_size\tdiff_bytes" if args.compare else ""
    print("object_sha\tfunction\tsource_sha\ttext_size\ttext_sha\trelocs"
          "\treloc_sha" + suffix)
    for row in rows:
        if wanted and row["function"] not in wanted:
            continue
        source_hash = hashes.get((args.unit, row["function"]), "-") \
            if args.unit else "-"
        compare_suffix = ""
        if args.compare:
            other = comparison.get(row["function"])
            if other is None:
                compare_suffix = "\t-\t-"
            else:
                common = min(row["size"], other["size"])
                differences = sum(
                    left != right
                    for left, right in zip(
                        row["bytes"][:common], other["bytes"][:common]
                    )
                ) + abs(row["size"] - other["size"])
                compare_suffix = f"\t{other['size']}\t{differences}"
        print(
            f"{object_sha}\t{row['function']}\t{source_hash}\t{row['size']}\t"
            f"{row['text_sha']}\t{row['relocs']}\t{row['reloc_sha']}"
            + compare_suffix
        )
    return 0


if __name__ == "__main__":
    sys.exit(main())


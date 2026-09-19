"""homm1 sema frame - show VC4 local names and /Od slot buckets.

    homm1 sema frame BASE/LZHUF
    homm1 sema frame EncodeData
    homm1 sema frame BASE/LZHUF --function Decode

The names and offsets come from S_BPREL32 records in the candidate object's
``.debug$S`` section.  HoMM1 uses the older CV4 record IDs while the HoMM2
donor tool used CV5; this reader accepts both.  Retail has no local-name debug
records, so the output names candidate declarations and supplies the proven
MSVC /Od identifier bucket needed to steer their retail stack order.
"""

from __future__ import annotations

import struct
import sys
from pathlib import Path

from homm1 import manifest
from homm1.core.od_slots import bucket
from homm1.core.paths import BUILD
from homm1.sema import die, run
from homm1.sema.index import index, short_name

S_END = 0x0006
S_BLOCK32 = 0x0207
S_BPREL32_CV4 = 0x0200
S_BPREL32_CV5 = 0x1006
S_LPROC32_CV4 = 0x0204
S_GPROC32_CV4 = 0x0205
S_LPROC32_CV5 = 0x100B
S_GPROC32_CV5 = 0x100A


def _sections(data: bytes):
    section_count = struct.unpack_from("<H", data, 2)[0]
    optional_size = struct.unpack_from("<H", data, 16)[0]
    symbol_offset, symbol_count = struct.unpack_from("<II", data, 8)
    string_table = symbol_offset + symbol_count * 18
    for section_index in range(section_count):
        header = 20 + optional_size + section_index * 40
        raw_name = data[header:header + 8]
        if raw_name[:1] == b"/":
            start = string_table + int(raw_name[1:].rstrip(b"\0"))
            name = data[start:data.find(b"\0", start)]
        else:
            name = raw_name.rstrip(b"\0")
        size, pointer = struct.unpack_from("<II", data, header + 16)
        yield name, data[pointer:pointer + size]


def _records(blob: bytes):
    cursor = 4 if len(blob) >= 4 and \
        struct.unpack_from("<I", blob, 0)[0] in (1, 2) else 0
    while cursor + 4 <= len(blob):
        record_length, record_type = struct.unpack_from("<HH", blob, cursor)
        if record_length < 2 or cursor + 2 + record_length > len(blob):
            break
        yield record_type, blob[cursor + 4:cursor + 2 + record_length]
        cursor += 2 + record_length


def _length_prefixed(body: bytes, offset: int) -> str:
    if offset >= len(body):
        return ""
    length = body[offset]
    return body[offset + 1:offset + 1 + length].decode("latin-1", "replace")


def _procedure_name(body: bytes) -> str:
    # CV4 puts the Pascal string at 33; later CodeView revisions moved it.
    for offset in (33, 35, 36, 34, 32):
        name = _length_prefixed(body, offset)
        if name:
            return name
    return ""


def frame_names(data: bytes) -> dict[str, list[tuple[int, str]]]:
    """Return ``{source function: [(ebp displacement, local name), ...]}``."""
    frames: dict[str, list[tuple[int, str]]] = {}
    proc_types = {S_LPROC32_CV4, S_GPROC32_CV4,
                  S_LPROC32_CV5, S_GPROC32_CV5}
    for section_name, blob in _sections(data):
        if section_name != b".debug$S":
            continue
        scopes: list[str | None] = []
        for record_type, body in _records(blob):
            if record_type in proc_types:
                function = _procedure_name(body)
                scopes.append(function)
                frames.setdefault(function, [])
            elif record_type == S_BLOCK32:
                scopes.append(None)
            elif record_type == S_END:
                if scopes:
                    scopes.pop()
            elif record_type in (S_BPREL32_CV4, S_BPREL32_CV5):
                function = next(
                    (scope for scope in reversed(scopes) if scope is not None),
                    None,
                )
                if function is None or len(body) < 7:
                    continue
                name_offset = 6 if record_type == S_BPREL32_CV4 else 8
                name = _length_prefixed(body, name_offset)
                frames[function].append((struct.unpack_from("<i", body, 0)[0], name))
    return frames


def _resolve(token: str) -> tuple[str, str | None]:
    units = manifest.by_unit()
    if token in units:
        return token, None
    hits = index().resolve_name(token)
    for rva in hits:
        binding = index().at(rva)
        if binding is not None and binding.unit and binding.name:
            return binding.unit, short_name(binding.name)
    die(f"{token!r} is neither a configured unit nor a claimed function")


def _render(function: str, entries: list[tuple[int, str]]) -> list[str]:
    lines = [f"== {function}"]
    seen = set()
    for displacement, name in sorted(entries, key=lambda entry: -entry[0]):
        if (displacement, name) in seen:
            continue
        seen.add((displacement, name))
        location = f"ebp{displacement:+#07x}" if displacement < 0 else \
            f"ebp+0x{displacement:x}"
        lines.append(f"  {location:13} bucket={bucket(name):2d}  {name}")
    if not entries:
        lines.append("  (no stack locals; arguments/register locals are not S_BPREL32)")
    return lines


from homm1.core.usage import logged


@logged
def main(argv: list[str] | None = None) -> int:
    import argparse
    parser = argparse.ArgumentParser(prog="homm1 sema frame",
                                     description=__doc__.split("\n\n")[0])
    parser.add_argument("target", help="configured unit or claimed function")
    parser.add_argument("--function", help="filter a unit by source-level name")
    parser.add_argument("--object", type=Path,
                        help="candidate COFF object (default: build/objdiff/base/<unit>.obj)")
    args = parser.parse_args(argv)
    unit, inferred = _resolve(args.target)
    wanted = args.function or inferred
    path = args.object or BUILD / "objdiff/base" / f"{unit}.obj"
    if not path.is_file():
        die(f"candidate object is absent: {path}; build {unit} first")
    frames = frame_names(path.read_bytes())
    selected = [(name, entries) for name, entries in frames.items()
                if not wanted or wanted.lower() in name.lower()]
    if not selected:
        print(f"{unit}: no CodeView procedure matching {wanted!r} in {path}")
        return 1
    print(f"candidate locals: {unit} ({path})")
    print("\n".join(line for name, entries in selected
                    for line in _render(name, entries)))
    return 0


if __name__ == "__main__":
    sys.exit(run(__name__, sys.argv[1:]))

"""Canonicalize MSVC compiler-private symbol names in a disposable COFF copy.

The transform is deliberately local to one object.  It does not consult a
manifest, a paired object, source text, or retail addresses.  Symbol indices do
not change. Compiler-private data receives stable, content-derived names.
`$E<n>` text helpers are also canonicalized when their object evidence is
complete, but that comparison aid does not make the ordinal a stable source
identity. A COFF weak external is retargeted to the auxiliary DEFAULT the linker
would have chosen (cl's `??_E<C>` vector-deleting-destructor slot -> `??_G<C>`).
In
embedded .text jump tables, same-function DIR32 references to volatile local
labels are rewritten to the containing external function plus an equivalent
owner-relative addend; all resolved section offsets are proved unchanged.
COFF COMMON symbols (cl's tentative definitions for a header-inline's local
static + its `??_B` guard) are first materialized into `.bss` exactly as the
linker would allocate them, so the delinked target's section symbol for the
same datum has a base-side counterpart to pair with.

Ported from the sibling homm2-decomp project (docs: data-symbol-normalization).
MSVC 5.0 emits the same `$SG`/`$T`/`name$S<n>` compiler-private data forms and the
same `$L<n>` in-.text jump-table labels as MSVC 4.2, so the classifier ports
unchanged. The normalized copies live under build/objdiff/normalized/ and are
matching-NEUTRAL: the real base/delink objs are untouched; only objdiff's INPUT
is this content-addressed view, so a base `$SG30360` "hi\\0" and a delinked
target constant with the same bytes pair by name. A fail-closed reparse proves
that ONLY symbol names + authorized jump-table reloc fields changed and every
resolved offset is identical, so normalization can only sharpen objdiff, never
inflate a false match.
"""

from __future__ import annotations

import argparse
import bisect
import csv
import hashlib
import io
import json
import os
import re
import struct
import tempfile
from collections import Counter, defaultdict
from dataclasses import dataclass
from pathlib import Path

# The one sanctioned cross-slice import, and only for its pure name-spelling
# helpers (registration_symbol/unwind_symbol/funcinfo_symbol/unwindmap_symbol/
# is_band_symbol are string functions). Nothing here opens the retail image.
from homm1.core import msvc_names
from homm1.delink import eh_band


SYMBOL_SIZE = 18
VOLATILE_SG = re.compile(r"^\$SG[0-9]+$")
VOLATILE_T = re.compile(r"^\$T[0-9]+$")
# The trailing ordinal is optional: cl's own object carries its per-object
# CodeView counter, while the delinked target carries the canonical spelling
# the Model binds (bare `$S`, or `$S<rva>` where several units share a name).
# Both are the same family and must reach the same content-addressed name.
NAMED_STATIC = re.compile(r"^.+\$S[0-9]*$")
# EVERY `$S<n>` counter in the name is volatile, not just the trailing one, and so
# is a function-local static's lexical-scope number - see core.msvc_names, which
# owns the masking both this normalizer and the labelling side apply:
# `_?$S47@?1??GetRect@CButeMgr@@...@4EA$S20267` -> `_?$S@?1??GetRect@...$S`.
# Proven collision-free: no base obj holds two distinct symbols that mask together
# (13 symbols tree-wide carry more than one `$S<n>`, 43 a scope number).
# `$E<n>` - a compiler-generated dynamic-initializer / EH-cleanup TEXT funclet for a
# file-scope object with a non-trivial ctor (e.g. `static CString g_worldName[8]={...}`).
# The `<n>` is a per-object counter that renumbers on ANY static-init add/remove in the
# TU, so - exactly like `$S<n>` local statics - it must be content-addressed, not pinned
# by a fixed number. The disposable comparison copy hashes the body and its
# recorded relocations where possible. Delinked target helpers can lack those
# relocation records, so this is not a source-label authority and an `_$E<n>`
# VA_COMPGEN claim is forbidden.
VOLATILE_E = re.compile(r"^_?\$E[0-9]+$")
# A delinker-enrolled per-TU copy of a header static (config/retail/data_compgen.tsv):
# the copies share one source name, so the manifest disambiguates each with its
# retail RVA (`?s_gruntDirEast_245168@@3UGruntDirectionCell@@A`). cl spells the
# same object `_s_gruntDirEast$S<n>` - the volatile-ordinal form this module
# already content-addresses - so the enrolled spelling is rewritten onto that
# family (prefix `_s_gruntDirEast$S`) and the two sides pair by source identity.
# The pattern is proven collision-free against every base obj: no cl output names
# any symbol `?s_<name>_<hex>@@3<type>A`.
DELINKED_STATIC_COPY = re.compile(
    r"^\?(?P<stem>s_\w+?)_[0-9a-f]{5,7}@@3.+A$")
MSVC_CTOR = re.compile(r"^\?\?0(?P<class_name>[^@]+)@@")
MSVC_DTOR = re.compile(r"^\?\?1(?P<class_name>[^@]+)@@")

INITIALIZED_DATA = 0x00000040
UNINITIALIZED_DATA = 0x00000080
MEM_EXECUTE = 0x20000000
MEM_WRITE = 0x80000000
LNK_NRELOC_OVFL = 0x01000000

RELOCATION_WIDTHS = {
    0x0001: 2,  # IMAGE_REL_I386_DIR16
    0x0002: 2,  # IMAGE_REL_I386_REL16
    0x0006: 4,  # IMAGE_REL_I386_DIR32
    0x0007: 4,  # IMAGE_REL_I386_DIR32NB
    0x000A: 2,  # IMAGE_REL_I386_SECTION
    0x000B: 4,  # IMAGE_REL_I386_SECREL
    0x0014: 4,  # IMAGE_REL_I386_REL32
}
DIR32 = 0x0006
FUNCTION_TYPE = 0x0020
EXTERNAL_STORAGE = 2
LABEL_STORAGE = 6
WEAK_EXTERNAL_STORAGE = 105
STATIC_STORAGE = 3
ALIGNMENT_FILL = {0x90, 0xCC}
ALIGNMENT_FILL_LIMIT = 16


@dataclass(frozen=True)
class Section:
    index: int
    header_offset: int
    name: str
    raw_size: int
    raw_offset: int
    reloc_offset: int
    reloc_count: int
    characteristics: int


@dataclass(frozen=True)
class Symbol:
    index: int
    offset: int
    name: str
    value: int
    section: int
    typ: int
    storage_class: int
    aux_count: int


@dataclass(frozen=True)
class Relocation:
    section: int
    site: int
    symbol_index: int
    typ: int
    offset: int = 0


@dataclass(frozen=True)
class JumpTableRewrite:
    relocation_offset: int
    section: int
    site: int
    original_symbol_index: int
    owner_symbol_index: int
    original_addend: int
    owner_addend: int
    resolved_offset: int


@dataclass(frozen=True)
class Definition:
    symbol: Symbol
    section: Section
    storage: str
    start: int
    end: int


@dataclass(frozen=True)
class CanonicalRow:
    original_name: str
    canonical_name: str
    family: str
    storage: str
    section_ordinal: int
    section_offset: int
    physical_size: int
    meaningful_size: int
    occurrence: int
    digest: str
    proof: str
    preview: str


@dataclass(frozen=True)
class CanonicalizedObject:
    data: bytes
    rows: tuple[CanonicalRow, ...]


# SEH bookkeeping the two sides spell differently (docs/referent-debt-ddrawmgr.tsv
# classes b1/b2). Both funclet symbols are renamed to the OWNER-derived names the
# EH band carve uses (`__ehreg$<owner>` / `__ehunwind$<owner>`, homm1.delink.eh_band),
# so the reference is compared against a named span rather than a shared placeholder;
# and the base's DIR32 to the absolute CRT `__except_list` (value 0, no .reloc entry
# survives linking, so the delinked target has a bare fs:[0] operand) is REMOVED
# rather than retyped - objdiff refuses IMAGE_REL_I386_ABSOLUTE.
_EH_TARGET_FUNCLET = re.compile(r"^FUN_[0-9a-f]{8}$")
PUSH_IMM32 = 0x68
MOV_EAX_IMM32 = 0xB8
#: `_s_FuncInfo::pUnwindMap` - the word that names the record's own unwind map.
FUNCINFO_UNWIND_MAP = 8
DUP_PREFIX = "$dup$"
ANON_DATA_PREFIX = "$anon_data_"


class CoffObject:
    def __init__(self, payload: bytes):
        self.data = bytes(payload)
        if len(self.data) < 20:
            raise ValueError("short COFF object")
        machine, section_count = struct.unpack_from("<HH", self.data, 0)
        if machine != 0x14C:
            raise ValueError(f"unsupported COFF machine 0x{machine:x}")
        self.section_count = section_count
        self.symbol_offset = struct.unpack_from("<I", self.data, 8)[0]
        self.symbol_count = struct.unpack_from("<I", self.data, 12)[0]
        optional_size = struct.unpack_from("<H", self.data, 16)[0]
        first_section = 20 + optional_size
        section_end = first_section + section_count * 40
        if section_end > len(self.data):
            raise ValueError("truncated COFF section table")
        self.string_offset = self.symbol_offset + self.symbol_count * SYMBOL_SIZE
        if self.string_offset + 4 > len(self.data):
            raise ValueError("missing COFF string table")
        self.string_size = struct.unpack_from("<I", self.data, self.string_offset)[0]
        if self.string_size < 4 or self.string_offset + self.string_size != len(self.data):
            raise ValueError("COFF string table is not final")
        self.sections = self._read_sections(first_section)
        self.symbols = self._read_symbols()
        self.relocations = self._read_relocations()

    def _string_name(self, offset: int) -> str:
        if not 4 <= offset < self.string_size:
            raise ValueError(f"invalid COFF string offset {offset}")
        start = self.string_offset + offset
        try:
            end = self.data.index(b"\0", start, self.string_offset + self.string_size)
        except ValueError as error:
            raise ValueError("unterminated COFF string") from error
        return self.data[start:end].decode("latin-1")

    def _symbol_name(self, offset: int) -> str:
        raw = self.data[offset:offset + 8]
        zero, string_offset = struct.unpack("<II", raw)
        if zero == 0:
            return self._string_name(string_offset)
        return raw.split(b"\0", 1)[0].decode("latin-1")

    def _section_name(self, offset: int) -> str:
        raw = self.data[offset:offset + 8].split(b"\0", 1)[0]
        if raw.startswith(b"/") and raw[1:].isdigit():
            return self._string_name(int(raw[1:]))
        return raw.decode("latin-1")

    def _read_sections(self, first: int) -> tuple[Section, ...]:
        rows = []
        for zero_index in range(self.section_count):
            offset = first + zero_index * 40
            raw_size, raw_offset, reloc_offset = struct.unpack_from(
                "<III", self.data, offset + 16)
            reloc_count = struct.unpack_from("<H", self.data, offset + 32)[0]
            characteristics = struct.unpack_from("<I", self.data, offset + 36)[0]
            if raw_offset and raw_offset + raw_size > len(self.data):
                raise ValueError("COFF section raw data is out of bounds")
            relocation_bytes = (10 if characteristics & LNK_NRELOC_OVFL
                                else reloc_count * 10)
            if reloc_count and reloc_offset + relocation_bytes > len(self.data):
                raise ValueError("COFF relocation table is out of bounds")
            rows.append(Section(
                zero_index + 1, offset, self._section_name(offset), raw_size,
                raw_offset, reloc_offset, reloc_count, characteristics,
            ))
        return tuple(rows)

    def _read_symbols(self) -> dict[int, Symbol]:
        if self.symbol_offset + self.symbol_count * SYMBOL_SIZE > len(self.data):
            raise ValueError("COFF symbol table is out of bounds")
        rows = {}
        index = 0
        while index < self.symbol_count:
            offset = self.symbol_offset + index * SYMBOL_SIZE
            value, section, typ, storage_class, aux_count = struct.unpack_from(
                "<IhHBB", self.data, offset + 8)
            if index + aux_count >= self.symbol_count:
                raise ValueError("COFF auxiliary symbols exceed the symbol table")
            rows[index] = Symbol(
                index, offset, self._symbol_name(offset), value, section, typ,
                storage_class, aux_count,
            )
            index += 1 + aux_count
        return rows

    def _read_relocations(self) -> tuple[Relocation, ...]:
        rows = []
        for section in self.sections:
            count = section.reloc_count
            first = 0
            if section.characteristics & LNK_NRELOC_OVFL:
                if count != 0xFFFF:
                    raise ValueError("COFF relocation overflow flag/count disagree")
                if section.reloc_offset + 10 > len(self.data):
                    raise ValueError("missing COFF relocation overflow record")
                count, symbol_index, typ = struct.unpack_from(
                    "<IIH", self.data, section.reloc_offset)
                if count < 1 or symbol_index or typ:
                    raise ValueError("invalid COFF relocation overflow record")
                first = 1
            for index in range(first, count):
                offset = section.reloc_offset + index * 10
                if offset + 10 > len(self.data):
                    raise ValueError("COFF relocation table is out of bounds")
                site, symbol_index, typ = struct.unpack_from("<IIH", self.data, offset)
                if site >= section.raw_size:
                    raise ValueError("COFF relocation site is outside its section")
                if symbol_index not in self.symbols:
                    raise ValueError("COFF relocation targets an auxiliary/missing symbol")
                rows.append(Relocation(
                    section.index, site, symbol_index, typ, offset))
        return tuple(rows)

    def section_bytes(self, section: Section) -> bytes:
        if section.raw_offset == 0:
            return bytes(section.raw_size)
        return self.data[section.raw_offset:section.raw_offset + section.raw_size]


def add_function_padding_boundaries(
        payload: bytes, claims: tuple[tuple[str, int], ...]) -> bytes:
    """Append donor-style local symbols at reviewed function ends.

    Vostok keeps linker alignment fill in the delinked ``.text`` section.
    Objdiff otherwise infers the preceding function through that fill.  HoMM2's
    comparison pipeline adds a local boundary symbol when the reviewed end is
    followed by fewer than 16 bytes consisting solely of NOP/INT3.  No payload
    byte is inserted, removed, or changed.
    """
    if not claims:
        return payload
    coff = CoffObject(payload)
    by_name: dict[str, list[Symbol]] = defaultdict(list)
    for symbol in coff.symbols.values():
        if (symbol.section > 0 and symbol.typ == FUNCTION_TYPE and
                symbol.storage_class == EXTERNAL_STORAGE and
                coff.sections[symbol.section - 1].characteristics & MEM_EXECUTE):
            by_name[symbol.name].append(symbol)
    boundaries = []
    for name, size in claims:
        functions = by_name.get(name, ())
        if len(functions) != 1 or size <= 0:
            continue
        function = functions[0]
        section = coff.sections[function.section - 1]
        claim_end = function.value + size
        span_end = min(
            (symbol.value for symbol in coff.symbols.values()
             if symbol.section == function.section
             and symbol.value > function.value),
            default=section.raw_size)
        if not claim_end < span_end <= section.raw_size:
            continue
        if span_end - claim_end >= ALIGNMENT_FILL_LIMIT:
            continue
        fill = coff.section_bytes(section)[claim_end:span_end]
        if any(byte not in ALIGNMENT_FILL for byte in fill):
            continue
        boundaries.append((f"$fnpad@{claim_end:x}", claim_end,
                           function.section))
    if not boundaries:
        return payload

    string_table = bytearray(payload[coff.string_offset:])
    string_offsets = {}
    cursor = 4
    while cursor < len(string_table):
        end = string_table.index(0, cursor)
        string_offsets[bytes(string_table[cursor:end])] = cursor
        cursor = end + 1

    records = bytearray()
    for name, value, section in sorted(boundaries,
                                       key=lambda item: (item[2], item[1])):
        encoded = name.encode("latin-1")
        if len(encoded) <= 8:
            name_field = encoded.ljust(8, b"\0")
        else:
            offset = string_offsets.get(encoded)
            if offset is None:
                offset = len(string_table)
                string_offsets[encoded] = offset
                string_table.extend(encoded + b"\0")
            name_field = struct.pack("<II", 0, offset)
        records.extend(name_field)
        records.extend(struct.pack("<IhHBB", value, section, 0,
                                   STATIC_STORAGE, 0))
    struct.pack_into("<I", string_table, 0, len(string_table))
    result = bytearray(payload[:coff.string_offset] + records + string_table)
    struct.pack_into("<I", result, 12, coff.symbol_count + len(boundaries))

    reparsed = CoffObject(bytes(result))
    if any(reparsed.section_bytes(after) != coff.section_bytes(before)
           for before, after in zip(coff.sections, reparsed.sections)):
        raise RuntimeError("function boundary normalization changed section bytes")
    return bytes(result)


def _storage(section: Section) -> str | None:
    flags = section.characteristics
    if flags & MEM_EXECUTE:
        return "text"
    if flags & UNINITIALIZED_DATA:
        return "bss"
    if not flags & INITIALIZED_DATA:
        return None
    return "data" if flags & MEM_WRITE else "rdata"


# cl 5.0's own .bss characteristics (CNT_UNINITIALIZED_DATA | ALIGN_8BYTES |
# MEM_READ | MEM_WRITE), used when a materialized COMMON needs a host section
# the object does not otherwise have.
BSS_CHARACTERISTICS = 0xC0400080


def _common_symbols(coff: CoffObject) -> list[Symbol]:
    """COFF COMMON symbols: tentative definitions whose Value IS their size."""
    return [
        symbol for symbol in coff.symbols.values()
        if symbol.section == 0 and symbol.value > 0
        and symbol.storage_class == EXTERNAL_STORAGE
        and symbol.typ == 0 and symbol.aux_count == 0
    ]


def materialize_commons(payload: bytes) -> tuple[bytes, tuple]:
    """Allocate each COMMON symbol into a .bss section, as the linker would.

    cl 5.0 emits a function-local static of a HEADER inline (and its `??_B`
    dynamic-init guard byte) as a COFF COMMON into every TU that instantiates
    the inline; the linker merges the copies into one `.bss` slot. The delinked
    target reads the LINKED image and so carries the datum as an ordinary
    section symbol, which a COMMON - sectionless by definition - can never pair
    with. Do here what the linker did (the same doctrine as the weak-external
    resolution below): extend the object's `.bss` (or append an empty one) and
    move each COMMON into it. Offsets are this allocator's, not retail's, but
    BSS extents are inferred on both sides and therefore not compared
    (nix/patches/objdiff-bss-inferred-extent.patch); only the symbol's
    existence, name and storage - the facts the two objects genuinely state -
    enter the comparison. The transform is verified fail-closed by
    `_assert_only_materialization_changes`.
    """
    coff = CoffObject(payload)
    commons = sorted(_common_symbols(coff), key=lambda s: s.index)
    if not commons:
        return payload, ()
    bss_sections = [s for s in coff.sections if _storage(s) == "bss"]
    data = bytearray(payload)
    if bss_sections:
        host = bss_sections[-1]
        host_index = host.index
        cursor = host.raw_size
    else:
        # Append a .bss header after the last section header. Section raw data,
        # relocation tables, the symbol table and the string table all live
        # after the header block, so they shift by exactly one 40-byte header;
        # every stored file offset is fixed up by the same amount. The new
        # section is LAST, so no existing symbol's section index moves.
        optional_size = struct.unpack_from("<H", payload, 16)[0]
        first_section = 20 + optional_size
        insert_at = first_section + coff.section_count * 40
        host_index = coff.section_count + 1
        cursor = 0
        header = struct.pack(
            "<8sIIIIIIHHI", b".bss", 0, 0, 0, 0, 0, 0, 0, 0,
            BSS_CHARACTERISTICS)
        data = bytearray(payload[:insert_at] + header + payload[insert_at:])
        struct.pack_into("<H", data, 2, coff.section_count + 1)
        struct.pack_into("<I", data, 8, coff.symbol_offset + 40)
        for section in coff.sections:
            for field_offset, value in (
                    (20, section.raw_offset), (24, section.reloc_offset)):
                if value:
                    struct.pack_into(
                        "<I", data, section.header_offset + field_offset,
                        value + 40)
            lineno = struct.unpack_from(
                "<I", payload, section.header_offset + 28)[0]
            if lineno:
                struct.pack_into(
                    "<I", data, section.header_offset + 28, lineno + 40)
    shift = len(data) - len(payload)
    allocations = []
    for symbol in commons:
        size = symbol.value
        cursor = (cursor + 3) & ~3
        allocations.append((symbol, cursor))
        struct.pack_into("<I", data, symbol.offset + shift + 8, cursor)
        struct.pack_into("<h", data, symbol.offset + shift + 12, host_index)
        cursor += size
    if bss_sections:
        struct.pack_into("<I", data, host.header_offset + 16, cursor)
    else:
        new_header_offset = 20 + optional_size + coff.section_count * 40
        struct.pack_into("<I", data, new_header_offset + 16, cursor)
    result = bytes(data)
    _assert_only_materialization_changes(coff, payload, result, allocations)
    return result, tuple(
        (symbol.name, offset, symbol.value) for symbol, offset in allocations)


def _assert_only_materialization_changes(
        original: CoffObject, payload: bytes, result: bytes,
        allocations) -> None:
    new = CoffObject(result)
    appended = new.section_count == original.section_count + 1
    if not appended and new.section_count != original.section_count:
        raise RuntimeError("materialization changed the section count unexpectedly")
    moved = {symbol.index: offset for symbol, offset in allocations}
    host_index = (new.section_count if appended
                  else max(s.index for s in original.sections
                           if _storage(s) == "bss"))
    for index, before in original.symbols.items():
        after = new.symbols[index]
        if index in moved:
            if (after.section != host_index or after.value != moved[index] or
                    after.name != before.name or
                    after.storage_class != before.storage_class):
                raise RuntimeError(
                    f"materialized COMMON {before.name} landed wrong")
        elif (before.name, before.value, before.section, before.typ,
              before.storage_class, before.aux_count) != (
                  after.name, after.value, after.section, after.typ,
                  after.storage_class, after.aux_count):
            raise RuntimeError(
                f"materialization touched unrelated symbol {before.name}")
    if len(original.relocations) != len(new.relocations):
        raise RuntimeError("materialization changed relocation count")
    for before, after in zip(original.relocations, new.relocations):
        if (before.section, before.site, before.symbol_index, before.typ) != (
                after.section, after.site, after.symbol_index, after.typ):
            raise RuntimeError("materialization changed a relocation")
    host = new.sections[host_index - 1]
    if _storage(host) != "bss" or host.raw_offset != 0 or host.reloc_count:
        raise RuntimeError("materialization host section is not a bare .bss")
    # Recompute the allocation to prove every recorded offset and the host size.
    cursor = 0 if appended else next(
        s.raw_size for s in original.sections if s.index == host_index)
    for symbol, offset in allocations:
        cursor = (cursor + 3) & ~3
        if offset != cursor:
            raise RuntimeError("materialization allocation drifted")
        cursor += symbol.value
    if host.raw_size != cursor:
        raise RuntimeError("materialization host size mismatch")
    for before in original.sections:
        after = new.sections[before.index - 1]
        if before.name != after.name or before.characteristics != after.characteristics:
            raise RuntimeError("materialization changed a section identity")
        if before.index != host_index and before.raw_size != after.raw_size:
            raise RuntimeError("materialization changed a section size")
        if before.raw_offset == 0:
            # Uninitialized: no file bytes exist; only the host may grow.
            if after.raw_offset != 0:
                raise RuntimeError("materialization gave a BSS section raw data")
        elif original.section_bytes(before) != new.section_bytes(after):
            raise RuntimeError("materialization changed section payload bytes")
    if (payload[original.string_offset:] !=
            result[new.string_offset:]):
        raise RuntimeError("materialization changed the string table")


def _definitions(coff: CoffObject) -> tuple[Definition, ...]:
    by_section: dict[int, list[Symbol]] = defaultdict(list)
    for symbol in coff.symbols.values():
        if (symbol.section > 0 and symbol.typ in (0, FUNCTION_TYPE) and
                symbol.storage_class in (2, 3) and symbol.aux_count == 0 and
                _storage(coff.sections[symbol.section - 1]) is not None):
            by_section[symbol.section].append(symbol)
    rows = []
    for section_index, symbols in by_section.items():
        section = coff.sections[section_index - 1]
        offsets = sorted({symbol.value for symbol in symbols})
        next_offset = {
            value: offsets[index + 1] if index + 1 < len(offsets) else section.raw_size
            for index, value in enumerate(offsets)
        }
        symbols_at_offset = defaultdict(list)
        for symbol in symbols:
            symbols_at_offset[symbol.value].append(symbol.name)
        aliases = {
            offset: names for offset, names in symbols_at_offset.items()
            if len(names) > 1 and any(_family(name) is not None for name in names)
        }
        if aliases:
            raise ValueError(
                f"same-offset compiler-private data aliases in section "
                f"{section.index}: {aliases}")
        for symbol in symbols:
            end = next_offset[symbol.value]
            if not 0 <= symbol.value < end <= section.raw_size:
                raise ValueError(
                    f"invalid data extent for {symbol.name} at section "
                    f"{section.index}+0x{symbol.value:x}")
            rows.append(Definition(
                symbol, section, _storage(section) or "", symbol.value, end,
            ))
    return tuple(sorted(rows, key=lambda row: (
        row.section.index, row.start, row.symbol.index,
    )))


def _family(name: str) -> tuple[str, str | None] | None:
    if VOLATILE_SG.fullmatch(name):
        return "sg", None
    if VOLATILE_T.fullmatch(name):
        return "t", None
    if VOLATILE_E.fullmatch(name):
        return "e", None
    if NAMED_STATIC.fullmatch(name):
        return "named", msvc_names.mask(name)
    match = DELINKED_STATIC_COPY.fullmatch(name)
    if match:
        return "named", f"_{match.group('stem')}$S"
    return None


def _is_canonical_candidate(definition: Definition) -> bool:
    family = _family(definition.symbol.name)
    if family is None:
        return False
    if family[0] == "e":
        # VC5 emits local `$E<n>` definitions while the delinker exposes an
        # RVA-pinned target helper as external. Both are real definitions.
        return (
            definition.storage == "text"
            and definition.symbol.storage_class in (2, 3)
        )
    if family[0] in ("named", "t", "sg"):
        # cl keeps `name$S<n>`/`$T<n>`/`$SG<n>` data TU-local while the delinker
        # exposes an RVA-pinned copy (e.g. a DATA_COMPGEN FP-pool claim) as
        # external. Both are real definitions; content-addressing pairs them.
        return definition.storage != "text" and definition.symbol.storage_class in (2, 3)
    return definition.storage != "text" and definition.symbol.storage_class == 3


def _compiler_private_definition_aliases(
        coff: CoffObject, candidates: dict[int, Definition]) -> dict[int, int]:
    """Pair an undefined `$E` duplicate with its unique same-object definition.

    The retail delinker can preserve both a defined compiler helper and an
    undefined external symbol with the same volatile `$E<n>` spelling. Parent
    helpers may relocate through that undefined symbol. Resolve only an exact,
    unique same-name duplicate; ambiguous or differently shaped symbols remain
    untouched.
    """
    definitions_by_name: dict[str, list[int]] = defaultdict(list)
    for symbol_index, definition in candidates.items():
        if _family(definition.symbol.name) == ("e", None):
            definitions_by_name[definition.symbol.name].append(symbol_index)

    aliases = {}
    for symbol in coff.symbols.values():
        if (
            symbol.index in candidates
            or symbol.section != 0
            or symbol.value != 0
            or symbol.typ not in (0, FUNCTION_TYPE)
            or symbol.storage_class != EXTERNAL_STORAGE
            or symbol.aux_count != 0
            or _family(symbol.name) != ("e", None)
        ):
            continue
        definitions = definitions_by_name.get(symbol.name, ())
        if len(definitions) == 1:
            aliases[symbol.index] = definitions[0]
    return aliases


def _identity_span(kind: str, physical_size: int, meaningful_size: int) -> int:
    # TEXT COMDATs and packed target text align the same helper differently.
    # Padding is not part of a function's identity. Data allocation spans remain
    # significant because they are the only proven object extent.
    return meaningful_size if kind == "text" else physical_size


def _relocation_width(typ: int) -> int:
    try:
        return RELOCATION_WIDTHS[typ]
    except KeyError as error:
        raise ValueError(f"unsupported i386 relocation type 0x{typ:x}") from error


def _float_width(payload: bytes):
    """Infer only widths proved by this allocation's bytes and physical span."""
    if len(payload) == 4:
        return 4, "extent-4"
    if len(payload) == 8 and any(payload[4:]):
        return 8, "extent-8-nonzero-upper-dword"
    return None, "ambiguous-content-width"


def _is_string(payload: bytes, relocations: list[Relocation]) -> int | None:
    if relocations:
        return None
    terminator = payload.find(b"\0")
    if terminator < 0 or any(payload[terminator + 1:]):
        return None
    return terminator + 1


def _escaped_preview(payload: bytes, limit: int = 48) -> str:
    shown = payload[:limit]
    text = "".join(
        chr(byte) if 0x20 <= byte < 0x7F and chr(byte) not in "\\\""
        else f"\\x{byte:02x}"
        for byte in shown
    )
    return text + ("..." if len(payload) > limit else "")


def _record_bytes(record: dict) -> bytes:
    return json.dumps(record, sort_keys=True, separators=(",", ":")).encode("ascii")


def _stable_relocation_name(name: str) -> str:
    """Normalize semantic aliases emitted by cl versus the retail delinker.

    VC5 COFF names a constructor/destructor by its decorated symbol, while the
    delinker's import from the stripped image commonly names the same target
    `Class` / `~Class`. The spelling difference must not give otherwise
    identical compiler-private helpers different content identities.
    """
    if name == "_atexit":
        return "atexit"
    match = MSVC_CTOR.match(name)
    if match:
        return match.group("class_name")
    match = MSVC_DTOR.match(name)
    if match:
        return f"~{match.group('class_name')}"
    return name


def _guarded_static_dtor_guard_site(payload: bytes, relative: int) -> bool:
    """Identify the two relocations to VC5's private template-dtor guard byte."""
    return (
        len(payload) == 0x1F
        and relative in (0x2, 0x10)
        and payload[0:2] == b"\x8a\x0d"
        and payload[6] == 0xB0
        and payload[7] != 0
        and payload[8:14] == b"\x84\xc8\x75\x12\x0a\xc8"
        and payload[14:16] == b"\x88\x0d"
        and payload[20] == 0xB9
        and payload[25] == 0xE9
        and payload[30] == 0xC3
    )


def _digest(record: bytes, seen: dict[str, bytes]) -> str:
    value = hashlib.sha256(record).hexdigest()
    previous = seen.get(value)
    if previous is not None and previous != record:
        raise ValueError(f"SHA-256 collision for canonical data record {value}")
    seen[value] = record
    return value


def _function_ranges(coff: CoffObject) -> dict[int, tuple[tuple[int, int, Symbol], ...]]:
    """Return unambiguous external-function ownership ranges per text section."""
    by_section: dict[int, dict[int, list[Symbol]]] = defaultdict(
        lambda: defaultdict(list))
    for symbol in coff.symbols.values():
        if (symbol.section > 0 and symbol.typ == FUNCTION_TYPE and
                symbol.storage_class == EXTERNAL_STORAGE and
                coff.sections[symbol.section - 1].characteristics & MEM_EXECUTE):
            by_section[symbol.section][symbol.value].append(symbol)
    result = {}
    for section_index, by_start in by_section.items():
        starts = sorted(by_start)
        ranges = []
        for index, start in enumerate(starts):
            if len(by_start[start]) != 1:
                continue
            end = starts[index + 1] if index + 1 < len(starts) else (
                coff.sections[section_index - 1].raw_size)
            ranges.append((start, end, by_start[start][0]))
        result[section_index] = tuple(ranges)
    return result


def _function_owner(ranges, section: int, offset: int) -> Symbol | None:
    for start, end, symbol in ranges.get(section, ()):
        if start <= offset < end:
            return symbol
    return None


def _rewrite_jump_table_relocations(
        original: CoffObject, payload: bytes) -> tuple[bytes, tuple[JumpTableRewrite, ...]]:
    """Rewrite same-function local-label DIR32 sites to owner+relative addend."""
    data = bytearray(payload)
    ranges = _function_ranges(original)
    rewrites = []
    for relocation in original.relocations:
        if relocation.typ != DIR32:
            continue
        section = original.sections[relocation.section - 1]
        if not section.characteristics & MEM_EXECUTE:
            continue
        target = original.symbols[relocation.symbol_index]
        if (target.section != relocation.section or target.typ != 0 or
                target.storage_class != 6):
            continue
        site_owner = _function_owner(ranges, relocation.section, relocation.site)
        target_owner = _function_owner(ranges, target.section, target.value)
        if site_owner is None or target_owner is None or site_owner != target_owner:
            continue
        if relocation.site + 4 > section.raw_size:
            raise ValueError("DIR32 jump-table relocation crosses .text payload")
        operand_offset = section.raw_offset + relocation.site
        original_addend = struct.unpack_from("<I", original.data, operand_offset)[0]
        owner_addend = (
            original_addend + target.value - target_owner.value) & 0xFFFFFFFF
        resolved_offset = (target.value + original_addend) & 0xFFFFFFFF
        if ((target_owner.value + owner_addend) & 0xFFFFFFFF != resolved_offset):
            raise RuntimeError("jump-table owner/addend resolution changed")
        struct.pack_into("<I", data, operand_offset, owner_addend)
        struct.pack_into("<I", data, relocation.offset + 4, target_owner.index)
        rewrites.append(JumpTableRewrite(
            relocation.offset, relocation.section, relocation.site,
            target.index, target_owner.index, original_addend, owner_addend,
            resolved_offset,
        ))
    return bytes(data), tuple(rewrites)


def _rewrite_names(coff: CoffObject, renames: dict[int, str]) -> bytes:
    data = bytearray(coff.data[:coff.string_offset])
    strings = bytearray(struct.pack("<I", 4))
    offsets: dict[bytes, int] = {}

    def encoded(name: str, section=False) -> bytes:
        raw = name.encode("latin-1")
        if len(raw) <= 8:
            return raw.ljust(8, b"\0")
        offset = offsets.get(raw)
        if offset is None:
            offset = len(strings)
            offsets[raw] = offset
            strings.extend(raw + b"\0")
        if section:
            value = f"/{offset}".encode("ascii")
            if len(value) > 8:
                raise ValueError("COFF long section-name offset does not fit")
            return value.ljust(8, b"\0")
        return struct.pack("<II", 0, offset)

    for section in coff.sections:
        data[section.header_offset:section.header_offset + 8] = encoded(
            section.name, section=True)
    for symbol in coff.symbols.values():
        data[symbol.offset:symbol.offset + 8] = encoded(
            renames.get(symbol.index, symbol.name))
    struct.pack_into("<I", strings, 0, len(strings))
    data.extend(strings)
    return bytes(data)


def _assert_only_canonical_changes(
        original: CoffObject, payload: bytes, renames: dict[int, str],
        jump_table_rewrites: tuple[JumpTableRewrite, ...],
        dup_retargets: dict[int, int] | None = None) -> None:
    dup_retargets = dup_retargets or {}
    normalized = CoffObject(payload)
    rewrites_by_offset = {
        rewrite.relocation_offset: rewrite for rewrite in jump_table_rewrites
    }
    if len(rewrites_by_offset) != len(jump_table_rewrites):
        raise RuntimeError("duplicate jump-table rewrite record")
    if (original.section_count != normalized.section_count or
            original.symbol_count != normalized.symbol_count or
            original.symbol_offset != normalized.symbol_offset):
        raise RuntimeError("canonical COFF topology postcondition failed")
    if len(original.sections) != len(normalized.sections):
        raise RuntimeError("canonical COFF section-count postcondition failed")
    for before, after in zip(original.sections, normalized.sections):
        if before.name != after.name:
            raise RuntimeError("canonical COFF changed a decoded section name")
        if (before.raw_size, before.raw_offset, before.reloc_offset,
                before.reloc_count, before.characteristics) != (
                after.raw_size, after.raw_offset, after.reloc_offset,
                after.reloc_count, after.characteristics):
            raise RuntimeError("canonical COFF changed section metadata")
        before_payload = bytearray(original.section_bytes(before))
        after_payload = bytearray(normalized.section_bytes(after))
        for rewrite in jump_table_rewrites:
            if rewrite.section != before.index:
                continue
            expected_before = struct.pack("<I", rewrite.original_addend)
            expected_after = struct.pack("<I", rewrite.owner_addend)
            site = rewrite.site
            if (before_payload[site:site + 4] != expected_before or
                    after_payload[site:site + 4] != expected_after):
                raise RuntimeError("canonical COFF emitted an unexpected jump-table addend")
            before_payload[site:site + 4] = bytes(4)
            after_payload[site:site + 4] = bytes(4)
        if before_payload != after_payload:
            raise RuntimeError("canonical COFF changed unexpected section payload bytes")
    if len(original.relocations) != len(normalized.relocations):
        raise RuntimeError("canonical COFF changed relocation count")
    for before, after in zip(original.relocations, normalized.relocations):
        rewrite = rewrites_by_offset.get(before.offset)
        if rewrite is not None:
            expected_symbol = rewrite.owner_symbol_index
        else:
            expected_symbol = dup_retargets.get(
                before.symbol_index, before.symbol_index)
        if (before.offset, before.section, before.site, before.typ) != (
                after.offset, after.section, after.site, after.typ):
            raise RuntimeError("canonical COFF changed relocation site/type/order")
        if after.symbol_index != expected_symbol:
            raise RuntimeError("canonical COFF changed an unexpected relocation target")
        if rewrite is not None and before.symbol_index != rewrite.original_symbol_index:
            raise RuntimeError("jump-table rewrite source-symbol postcondition failed")
    if set(original.symbols) != set(normalized.symbols):
        raise RuntimeError("canonical COFF changed symbol indices")
    for index, before in original.symbols.items():
        after = normalized.symbols[index]
        if after.name != renames.get(index, before.name):
            raise RuntimeError("canonical COFF emitted an unexpected symbol name")
        if (before.value, before.section, before.typ, before.storage_class,
                before.aux_count) != (
                after.value, after.section, after.typ, after.storage_class,
                after.aux_count):
            raise RuntimeError("canonical COFF changed symbol metadata")
        aux_start = before.offset + SYMBOL_SIZE
        aux_end = aux_start + before.aux_count * SYMBOL_SIZE
        if original.data[aux_start:aux_end] != payload[aux_start:aux_end]:
            raise RuntimeError("canonical COFF changed auxiliary symbol bytes")

    # Everything before the string table must be identical after masking only
    # the section-name and primary-symbol-name fields that are allowed to move
    # between inline and string-table encodings.
    before_prefix = bytearray(original.data[:original.string_offset])
    after_prefix = bytearray(payload[:normalized.string_offset])
    for section in original.sections:
        before_prefix[section.header_offset:section.header_offset + 8] = bytes(8)
        after_prefix[section.header_offset:section.header_offset + 8] = bytes(8)
    for symbol in original.symbols.values():
        before_prefix[symbol.offset:symbol.offset + 8] = bytes(8)
        after_prefix[symbol.offset:symbol.offset + 8] = bytes(8)
    for rewrite in jump_table_rewrites:
        section = original.sections[rewrite.section - 1]
        operand = section.raw_offset + rewrite.site
        before_prefix[operand:operand + 4] = bytes(4)
        after_prefix[operand:operand + 4] = bytes(4)
        before_prefix[rewrite.relocation_offset + 4:
                      rewrite.relocation_offset + 8] = bytes(4)
        after_prefix[rewrite.relocation_offset + 4:
                     rewrite.relocation_offset + 8] = bytes(4)
    for relocation in original.relocations:
        if relocation.symbol_index in dup_retargets:
            before_prefix[relocation.offset + 4:relocation.offset + 8] = bytes(4)
            after_prefix[relocation.offset + 4:relocation.offset + 8] = bytes(4)
    if before_prefix != after_prefix:
        raise RuntimeError(
            "canonical COFF changed bytes outside symbol names/jump-table relocations")

    normalized_relocations = {row.offset: row for row in normalized.relocations}
    for rewrite in jump_table_rewrites:
        before_relocation = next(
            row for row in original.relocations
            if row.offset == rewrite.relocation_offset)
        after_relocation = normalized_relocations[rewrite.relocation_offset]
        before_target = original.symbols[before_relocation.symbol_index]
        after_target = normalized.symbols[after_relocation.symbol_index]
        before_section = original.sections[before_relocation.section - 1]
        after_section = normalized.sections[after_relocation.section - 1]
        before_addend = struct.unpack_from(
            "<I", original.data,
            before_section.raw_offset + before_relocation.site)[0]
        after_addend = struct.unpack_from(
            "<I", payload,
            after_section.raw_offset + after_relocation.site)[0]
        before_resolved = (
            before_target.section,
            (before_target.value + before_addend) & 0xFFFFFFFF,
        )
        after_resolved = (
            after_target.section,
            (after_target.value + after_addend) & 0xFFFFFFFF,
        )
        expected = (rewrite.section, rewrite.resolved_offset)
        if before_resolved != expected or after_resolved != expected:
            raise RuntimeError(
                "jump-table relocation resolved-target postcondition failed")


#: `mov fs:[0],esp` - the instruction that makes a pushed record the ACTIVE
#: exception registration. cl 5.0 emits it within a few instructions of the
#: registration `push`, whatever else the prologue interleaves.
SEH_INSTALL = bytes.fromhex("64892500000000")
SEH_INSTALL_WINDOW = 24


def _installs_seh_frame(data: bytes, start: int) -> bool:
    return SEH_INSTALL in data[start:start + SEH_INSTALL_WINDOW]


def _eh_funclet_owners(
        coff: CoffObject,
        stubs: list[tuple[str, "Symbol"]] | None = None) -> dict[int, str]:
    """{symbol index -> canonical EH band name} for one side's funclet symbols.

    Both sides spell the SAME machinery differently and no source edit aligns them:
      * base (cl): the function's EH funclets live in their own small EXECUTE
        COMDAT labelled only with class-6 `$L` symbols - one per unwind funclet
        plus one on the registration stub - and the prologue pushes the stub's
        label. Each label is renamed to the band name for its position.
      * target (delinked retail): homm1.delink.eh_band already carved those spans
        under those names, so nothing is renamed there. A group the carve did NOT
        reach still resolves to an UNDEFINED `FUN_<rva>` plus a nonzero addend; its
        stub symbol gets the same owner-derived name (and the stored addend is
        zeroed in `_canonicalize_eh_relocations`) so the reference still co-names.

    The owner is the function containing the `push`, which is the same mangled name
    on both sides, and the funclet index is its position in ADDRESS order, which is
    unwind-state order on both sides - so the two sides land on the same symbol
    names without either knowing anything about the other.

    `stubs`, when given, collects `(owner, stub label)` for every BASE-side group -
    the entry point `_eh_funcinfo_owners` needs to walk on to the `.xdata$x` tables.
    """
    functions: dict[int, list[tuple[int, str]]] = {}
    for symbol in coff.symbols.values():
        if symbol.section <= 0 or symbol.storage_class != EXTERNAL_STORAGE:
            continue
        if coff.sections[symbol.section - 1].characteristics & MEM_EXECUTE:
            functions.setdefault(symbol.section, []).append((symbol.value, symbol.name))
    for entries in functions.values():
        entries.sort()

    def owner_of(section: int, site: int) -> str | None:
        entries = functions.get(section)
        if not entries:
            return None
        index = bisect.bisect_right(entries, (site + 1, "")) - 1
        return entries[index][1] if index >= 0 else None

    labels: dict[int, list[Symbol]] = {}
    for symbol in coff.symbols.values():
        if (symbol.section > 0 and symbol.storage_class == LABEL_STORAGE and
                symbol.name.startswith("$L") and
                coff.sections[symbol.section - 1].characteristics & MEM_EXECUTE):
            labels.setdefault(symbol.section, []).append(symbol)
    for entries in labels.values():
        entries.sort(key=lambda s: (s.value, s.index))

    if stubs is None:
        stubs = []
    out: dict[int, str] = {}
    for relocation in coff.relocations:
        if relocation.typ != DIR32:
            continue
        site_section = coff.sections[relocation.section - 1]
        if not site_section.characteristics & MEM_EXECUTE:
            continue
        operand = site_section.raw_offset + relocation.site
        if _byte_at(coff.data, operand - 1) != PUSH_IMM32:
            continue
        target = coff.symbols[relocation.symbol_index]
        base_side = (target.section > 0 and target.storage_class == LABEL_STORAGE and
                     target.name.startswith("$L") and
                     target.section != relocation.section and
                     coff.sections[target.section - 1].characteristics & MEM_EXECUTE)
        # On the delinked side the only structure available is "an undefined
        # FUN_<rva>", which is ALSO what a `push <$E atexit thunk>; call
        # _atexit` looks like - and naming one of those `__ehreg$<owner>`
        # asserts an EH registration that does not exist. The push has to be
        # the one that INSTALLS the frame, so require the `mov fs:[0],esp`
        # that always follows it inside the prologue: 911/911 of the base
        # side's structurally-identified registrations satisfy it and 0/12 of
        # the delinked FUN_<rva> pushes in the tree do.
        target_side = (target.section == 0
                       and _EH_TARGET_FUNCLET.match(target.name)
                       and _installs_seh_frame(coff.data, operand + 4))
        if not (base_side or target_side):
            continue
        owner = owner_of(relocation.section, relocation.site)
        if owner is None:
            continue
        out.setdefault(target.index, eh_band.registration_symbol(owner))
        if base_side:
            index = 0
            for symbol in labels[target.section]:
                if symbol.value >= target.value:
                    break
                out.setdefault(symbol.index, eh_band.unwind_symbol(owner, index))
                index += 1
            stubs.append((owner, target))
    return out


def _eh_funcinfo_owners(coff: CoffObject) -> dict[int, str]:
    """{symbol index -> canonical `.xdata$x` name} for cl's own EH state tables.

    The companion of `_eh_funclet_owners` for the DATUM the registration stub
    loads. cl labels the `_s_FuncInfo` record and the unwind map that follows it
    `$T<n>` - TU-local ordinals nothing can reproduce - and the delinked target
    carries the owner-derived names `homm1.delink.eh_band` gives them, so this
    renames the base's two labels to the same thing.

    Both are located STRUCTURALLY, never by name: the stub's `mov eax,imm32` is at
    `stub + 0` and its operand's relocation at `stub + 1` names the record; the
    record's `pUnwindMap` word is at `record + 8` and its relocation names the map.
    Anything that does not have that exact shape is left alone.
    """
    stubs: list[tuple[str, Symbol]] = []
    _eh_funclet_owners(coff, stubs)
    if not stubs:
        return {}
    by_site = {(relocation.section, relocation.site): relocation
               for relocation in coff.relocations if relocation.typ == DIR32}

    def datum_at(section: int, site: int) -> Symbol | None:
        relocation = by_site.get((section, site))
        if relocation is None:
            return None
        symbol = coff.symbols[relocation.symbol_index]
        if symbol.section <= 0:
            return None
        if coff.sections[symbol.section - 1].characteristics & MEM_EXECUTE:
            return None
        return symbol

    out: dict[int, str] = {}
    for owner, stub in stubs:
        section = coff.sections[stub.section - 1]
        if _byte_at(coff.data, section.raw_offset + stub.value) != MOV_EAX_IMM32:
            continue
        record = datum_at(stub.section, stub.value + 1)
        if record is None:
            continue
        out.setdefault(record.index, eh_band.funcinfo_symbol(owner))
        unwind_map = datum_at(record.section, record.value + FUNCINFO_UNWIND_MAP)
        if unwind_map is not None and unwind_map.index != record.index:
            out.setdefault(unwind_map.index, eh_band.unwindmap_symbol(owner))
    return out


def _byte_at(data: bytes, offset: int) -> int:
    return data[offset] if 0 <= offset < len(data) else -1


#: cl's grouped EH sections and the section the LINKER folds each into. `$x` is an
#: ordering key, not part of the section's identity (the same reading the delinker's
#: own data-section manifest applies to `.rdata$r`), and retail's image proves the
#: destination: it has no `.xdata` at all and every `FuncInfo` sits in `.rdata`.
_EH_SECTION_GROUP = {".text$x": ".text", ".xdata$x": ".rdata"}


def _canonicalize_eh_section_names(payload: bytes) -> tuple[bytes, tuple[CanonicalRow, ...]]:
    """Fold cl's `.text$x` / `.xdata$x` onto the section the linker put them in.

    THIS IS WHAT MADE THE WHOLE BAND UNSCORABLE, and it is invisible in a byte
    diff. It was found under `functionRelocDiffs = data_value`, whose relocation
    comparison reduced to

        section_name_eq(left, right) && <the referenced bytes are equal>

    - the target symbol's NAME was deliberately not consulted. cl emits a /GX
    function's funclets into a `.text$x` COMDAT and its EH state tables into
    `.xdata$x`; the delinked target has neither name, because retail's linker
    folded them into `.text` and `.rdata` and that is what the delinker rebuilds.
    So `section_name_eq` was false for EVERY one of them, and the mismatch was not
    the funclets' - it landed on the OWNER, whose prologue `push OFFSET <stub>` is
    a relocation into `.text$x`. 750 /GX functions each carried one guaranteed
    mismatching instruction that had nothing to do with their code.

    Renaming the base's grouped sections to the group states exactly what the
    linker did, and it is the only side that can move: the target genuinely has one
    `.text` and one `.rdata`. Nothing but the 8-byte name field is touched - size,
    characteristics, COMDAT selection, payload, symbols and relocations all stay.
    """
    section_count = struct.unpack_from("<H", payload, 2)[0]
    first_section = 20 + struct.unpack_from("<H", payload, 16)[0]
    data = bytearray(payload)
    renamed = Counter()
    for index in range(section_count):
        offset = first_section + index * 40
        name = payload[offset:offset + 8].split(b"\0", 1)[0].decode("latin-1")
        group = _EH_SECTION_GROUP.get(name)
        if group is None:
            continue
        data[offset:offset + 8] = group.encode("latin-1").ljust(8, b"\0")
        renamed[(name, group)] += 1
    if not renamed:
        return payload, ()
    return bytes(data), tuple(
        CanonicalRow(name, group, "eh", "section", 0, 0, 0, 0, count, "-",
                     "eh-grouped-section-folded-into-linker-destination", "")
        for (name, group), count in sorted(renamed.items()))


def _canonicalize_eh_relocations(payload: bytes) -> tuple[bytes, tuple[CanonicalRow, ...]]:
    """Drop the base's absolute `__except_list` relocs; zero uncarved funclet addends.

    The base references the absolute CRT `__except_list` (value 0) with a DIR32 the
    linked image cannot carry (.reloc has no entry for absolute fixups), so the
    target's `fs:[0]` operand has no relocation at all. The base-side relocation is
    REMOVED rather than retyped - objdiff rejects IMAGE_REL_I386_ABSOLUTE.

    A funclet push that `homm1.delink.eh_band` did not carve still points into an
    UNDEFINED `FUN_<rva>` at a nonzero addend; the stored addend is zeroed so it
    co-names with the base's `push <label>+0` (the rename itself is done in
    `_eh_funclet_owners`, through the shared symbol-rename path).
    """
    coff = CoffObject(payload)
    data = bytearray(payload)

    zeroed_sites: list[int] = []
    dropped: set[int] = set()
    for relocation in coff.relocations:
        if relocation.typ != DIR32:
            continue
        site_section = coff.sections[relocation.section - 1]
        if not site_section.characteristics & MEM_EXECUTE:
            continue
        target = coff.symbols[relocation.symbol_index]
        if target.section == 0 and target.name == "__except_list":
            dropped.add(relocation.offset)
            continue
        if target.section == 0 and eh_band.is_band_symbol(
                target.name.removeprefix(DUP_PREFIX)):
            operand = site_section.raw_offset + relocation.site
            stored = struct.unpack_from("<I", payload, operand)[0]
            if stored != 0 and payload[operand - 1] == PUSH_IMM32:
                zeroed_sites.append(operand)

    if not (zeroed_sites or dropped):
        return payload, ()

    for operand in zeroed_sites:
        struct.pack_into("<I", data, operand, 0)
    if dropped:
        for offset in sorted(dropped, reverse=True):
            del data[offset:offset + 10]

        def shifted(pointer: int) -> int:
            return pointer - 10 * sum(1 for o in dropped if o < pointer)

        symbol_pointer = struct.unpack_from("<I", payload, 8)[0]
        struct.pack_into("<I", data, 8, shifted(symbol_pointer))
        for section in coff.sections:
            raw_ptr, reloc_ptr = struct.unpack_from(
                "<II", payload, section.header_offset + 20)
            n_reloc = struct.unpack_from(
                "<H", payload, section.header_offset + 32)[0]
            dropped_here = sum(
                1 for o in dropped if reloc_ptr <= o < reloc_ptr + n_reloc * 10)
            struct.pack_into(
                "<II", data, section.header_offset + 20,
                shifted(raw_ptr) if raw_ptr else 0,
                shifted(reloc_ptr) if reloc_ptr else 0)
            struct.pack_into(
                "<H", data, section.header_offset + 32, n_reloc - dropped_here)

    result = bytes(data)
    _assert_only_eh_changes(coff, payload, result, zeroed_sites, dropped)
    rows = []
    if zeroed_sites:
        rows.append(CanonicalRow(
            "eh-funclet-push", "eh-funclet-push", "eh", "text",
            0, 0, 0, 0, len(zeroed_sites), "-",
            "uncarved-funclet-addend-zeroed", ""))
    if dropped:
        rows.append(CanonicalRow(
            "__except_list", "__except_list", "eh", "undefined",
            0, 0, 0, 0, len(dropped), "-",
            "absolute-fixup-dropped-no-reloc-survives-linking", ""))
    return result, tuple(rows)


def _assert_only_eh_changes(original: CoffObject, before: bytes, after: bytes,
                            zeroed_sites: list, dropped: set) -> None:
    reparsed = CoffObject(after)
    if len(reparsed.relocations) != len(original.relocations) - len(dropped):
        raise RuntimeError("EH canonicalization dropped the wrong reloc count")
    survivors = [r for r in original.relocations if r.offset not in dropped]
    for old, new in zip(survivors, reparsed.relocations):
        if (old.section, old.site, old.typ, old.symbol_index) != (
                new.section, new.site, new.typ, new.symbol_index):
            raise RuntimeError("EH canonicalization disturbed a surviving reloc")
    for index, symbol in original.symbols.items():
        if reparsed.symbols[index].name != symbol.name:
            raise RuntimeError("EH canonicalization renamed a symbol")
    zeroed = set(zeroed_sites)
    for section_old, section_new in zip(original.sections, reparsed.sections):
        if not section_old.raw_offset:
            continue  # uninitialized section: no on-disk payload to compare
        raw_old = before[section_old.raw_offset:
                         section_old.raw_offset + section_old.raw_size]
        raw_new = after[section_new.raw_offset:
                        section_new.raw_offset + section_new.raw_size]
        if raw_old == raw_new:
            continue
        masked_old, masked_new = bytearray(raw_old), bytearray(raw_new)
        for operand in zeroed:
            relative = operand - section_old.raw_offset
            if 0 <= relative < section_old.raw_size:
                masked_old[relative:relative + 4] = bytes(4)
                masked_new[relative:relative + 4] = bytes(4)
        if masked_old != masked_new:
            raise RuntimeError(
                "EH canonicalization changed section bytes outside push operands")


def canonicalize_coff(payload: bytes) -> CanonicalizedObject:
    """Return a normalized comparison copy and its readable rename records."""
    payload, materialized = materialize_commons(payload)
    coff = CoffObject(payload)
    definitions = _definitions(coff)
    definition_by_symbol = {row.symbol.index: row for row in definitions}
    section_relocations: dict[int, list[Relocation]] = defaultdict(list)
    for relocation in coff.relocations:
        section_relocations[relocation.section].append(relocation)

    candidates = {
        row.symbol.index: row for row in definitions
        if _is_canonical_candidate(row)
    }
    definition_aliases = _compiler_private_definition_aliases(coff, candidates)
    kinds: dict[int, tuple[str, bytes, str, str]] = {}
    for definition in candidates.values():
        family = _family(definition.symbol.name)
        raw = coff.section_bytes(definition.section)[definition.start:definition.end]
        own_relocs = [row for row in section_relocations[definition.section.index]
                      if definition.start <= row.site < definition.end]
        kind, meaningful, proof = "data", raw, "physical-span"
        if definition.storage == "bss":
            # An uninitialized allocation STATES no bytes: `section_bytes` above
            # synthesised the zeros, and the physical span is the object PLUS
            # whatever hole-filling its allocator chose (cl packs 4-byte ints
            # into the gaps; the delinker appends per-definition) - the same
            # two-allocators argument as nix/patches/objdiff-bss-inferred-extent
            # .patch. Neither quantity is part of the object's identity, so a
            # BSS static's canonical name carries only its source name, storage
            # and occurrence.
            meaningful, proof = b"", "bss-no-content"
        elif family and family[0] == "e":
            # Base COMDATs and packed delinked target text have different
            # alignment spans. The helper body ends before their trailing NOPs.
            meaningful = raw.rstrip(b"\x90")
            if not meaningful:
                raise ValueError(
                    f"empty compiler-private text helper {definition.symbol.name}")
            kind, proof = "text", "trailing-nop-padding"
        elif family and family[0] == "sg":
            string_size = _is_string(raw, own_relocs)
            if string_size is not None:
                kind, meaningful, proof = "string", raw[:string_size], "nul-terminated"
        elif family and family[0] == "t":
            width, proof = _float_width(raw)
            if width == 4:
                kind, meaningful = "f32", raw[:4]
            elif width == 8:
                kind, meaningful = "f64", raw[:8]
        kinds[definition.symbol.index] = (kind, meaningful, proof, _escaped_preview(meaningful))

    digest_records: dict[str, bytes] = {}
    dependencies = {}
    for symbol_index, definition in candidates.items():
        family = _family(definition.symbol.name)
        assert family is not None
        _kind, meaningful, _proof, _preview = kinds[symbol_index]
        dependencies[symbol_index] = set()
        for relocation in section_relocations[definition.section.index]:
            if not definition.start <= relocation.site < definition.end:
                continue
            relative = relocation.site - definition.start
            if (
                family[0] == "e"
                and _guarded_static_dtor_guard_site(meaningful, relative)
            ):
                continue
            target_index = definition_aliases.get(
                relocation.symbol_index, relocation.symbol_index)
            if target_index in candidates:
                dependencies[symbol_index].add(target_index)

    levels = {}
    finding_level = set()

    def level(symbol_index):
        if symbol_index in levels:
            return levels[symbol_index]
        if symbol_index in finding_level:
            raise ValueError(
                f"cyclic compiler-private data initializer at "
                f"{coff.symbols[symbol_index].name}")
        finding_level.add(symbol_index)
        value = 0
        if dependencies[symbol_index]:
            value = 1 + max(level(target) for target in dependencies[symbol_index])
        finding_level.remove(symbol_index)
        levels[symbol_index] = value
        return value

    for symbol_index in candidates:
        level(symbol_index)

    occurrences = defaultdict(int)
    renames = {}
    existing_names = {
        symbol.name: symbol.index for symbol in coff.symbols.values()
        if _family(symbol.name) is None
    }
    canonical_owners = {}
    resolved = {}

    for current_level in range(max(levels.values(), default=-1) + 1):
        prepared = []
        for symbol_index, definition in candidates.items():
            if levels[symbol_index] != current_level:
                continue
            family = _family(definition.symbol.name)
            assert family is not None
            kind, meaningful, proof, preview = kinds[definition.symbol.index]
            masked = bytearray(meaningful)
            reloc_rows = []
            for relocation in section_relocations[definition.section.index]:
                if not definition.start <= relocation.site < definition.end:
                    continue
                relative = relocation.site - definition.start
                width = _relocation_width(relocation.typ)
                if relative + width > len(meaningful):
                    raise ValueError(
                        f"relocation crosses meaningful payload for "
                        f"{definition.symbol.name}")
                addend = int.from_bytes(
                    masked[relative:relative + width], "little", signed=True)
                masked[relative:relative + width] = bytes(width)
                target = coff.symbols[relocation.symbol_index]
                target_index = definition_aliases.get(
                    relocation.symbol_index, relocation.symbol_index)
                if (
                    family[0] == "e"
                    and _guarded_static_dtor_guard_site(meaningful, relative)
                ):
                    target_identity = ("compiler-static-dtor-guard",)
                    addend = 0
                elif target_index in candidates:
                    target_identity = (
                        "canonical", renames[target_index])
                else:
                    target_identity = (
                        "symbol", _stable_relocation_name(target.name))
                reloc_rows.append((
                    relative, relocation.typ, width, addend, target_identity,
                ))
            reloc_rows.sort(key=lambda row: _record_bytes({"relocation": row}))
            physical_size = definition.end - definition.start
            record = _record_bytes({
                "schema": "homm1-anon-symbol-v2",
                "kind": kind,
                "storage": definition.storage,
                "span": 0 if definition.storage == "bss"
                else _identity_span(kind, physical_size, len(meaningful)),
                "meaningful_size": len(meaningful),
                "payload": bytes(masked).hex(),
                "relocations": reloc_rows,
            })
            record_digest = _digest(record, digest_records)
            if kind == "f32":
                identity = f"{int.from_bytes(meaningful, 'little'):08x}"
                base_name = f"$anon_f32_{identity}"
            elif kind == "f64":
                identity = f"{int.from_bytes(meaningful, 'little'):016x}"
                base_name = f"$anon_f64_{identity}"
            elif kind == "string":
                identity = _digest(meaningful, digest_records)
                base_name = f"$anon_str_{identity}"
            else:
                identity = record_digest
                base_name = f"{ANON_DATA_PREFIX}{identity}"
            prefix = family[1]
            if family[0] == "named":
                base_name = f"{prefix}{kind}_{definition.storage}_{identity}"
            display_digest = identity if kind in ("string", "data") else record_digest
            prepared.append((
                definition, family[0], kind, meaningful, proof, preview,
                display_digest, base_name, record,
            ))

        for (definition, family, kind, meaningful, proof, preview,
             digest, base_name, record) in sorted(
                 prepared, key=lambda row: (
                     row[0].section.index, row[0].start)):
            occurrence = occurrences[base_name]
            occurrences[base_name] += 1
            canonical = f"{base_name}_{occurrence}"
            collision = existing_names.get(canonical)
            if collision is not None and collision != definition.symbol.index:
                raise ValueError(
                    f"canonical symbol name collides with existing symbol: {canonical}")
            collision = canonical_owners.get(canonical)
            if collision is not None and collision != definition.symbol.index:
                raise ValueError(f"duplicate canonical symbol name: {canonical}")
            canonical_owners[canonical] = definition.symbol.index
            renames[definition.symbol.index] = canonical
            resolved[definition.symbol.index] = (
                definition, family, kind, meaningful, proof, preview,
                digest, canonical, occurrence, record,
            )

    for alias_index, definition_index in definition_aliases.items():
        renames[alias_index] = renames[definition_index]

    rows = []
    for definition in sorted(candidates.values(), key=lambda row: (
            row.section.index, row.start)):
        (_definition, family, _kind, meaningful, proof, preview,
         digest, canonical, occurrence, _record) = resolved[definition.symbol.index]
        rows.append(CanonicalRow(
            definition.symbol.name, canonical, family, definition.storage,
            definition.section.index, definition.start, definition.end - definition.start,
            len(meaningful), occurrence, digest, proof, preview,
        ))

    for symbol in sorted(coff.symbols.values(), key=lambda row: row.index):
        family = _family(symbol.name)
        if family is None or symbol.index in candidates:
            continue
        alias_target = definition_aliases.get(symbol.index)
        if alias_target is not None:
            (_definition, _target_family, _kind, _meaningful, _proof, _preview,
             digest, canonical, occurrence, _record) = resolved[alias_target]
            rows.append(CanonicalRow(
                symbol.name, canonical, family[0], "undefined", 0, 0, 0, 0,
                occurrence, digest, "alias-of-definition", "",
            ))
            continue
        definition = definition_by_symbol.get(symbol.index)
        if definition is not None:
            storage = definition.storage
            section_ordinal = definition.section.index
            section_offset = definition.start
            physical_size = definition.end - definition.start
            status = "defined-nonprivate"
        else:
            storage = "common" if symbol.section == 0 and symbol.value else "undefined"
            section_ordinal = max(symbol.section, 0)
            section_offset = symbol.value
            physical_size = 0
            status = storage
        rows.append(CanonicalRow(
            symbol.name, symbol.name, family[0], storage, section_ordinal,
            section_offset, physical_size, 0, 0, "-", f"skipped-{status}", "",
        ))

    # Delinker artifact: an ILT-thunk-reached function with inline .text jump
    # tables is emitted TWICE - the real .text definition plus a size-0 UNDEFINED
    # external of the same name. objdiff pairs the base function against the
    # undefined copy and scores a byte-correct body 0%. Rename the undefined
    # duplicate out of the pairing namespace (`$dup$` PREFIX - objdiff pairs on
    # DEMANGLED names and `?f@C@...$suffix` still demangles to the original
    # signature), and RETARGET every relocation that referenced the duplicate to
    # the real definition: both symbols resolve to the same linked address, so the
    # retarget is byte-neutral, and call sites then co-name with the base side
    # instead of displaying `call $dup$...` (an ARG_MISMATCH per site otherwise).
    defined_external_index = {}
    for symbol in coff.symbols.values():
        if symbol.section > 0 and symbol.storage_class == EXTERNAL_STORAGE:
            defined_external_index.setdefault(symbol.name, symbol.index)
    dup_retargets = {}
    for symbol in coff.symbols.values():
        if (symbol.section == 0 and symbol.value == 0 and
                symbol.storage_class == EXTERNAL_STORAGE and
                symbol.index not in renames and
                symbol.name in defined_external_index):
            renames[symbol.index] = "$dup$" + symbol.name
            dup_retargets[symbol.index] = defined_external_index[symbol.name]
            rows.append(CanonicalRow(
                symbol.name, renames[symbol.index], "dup", "undefined",
                0, 0, 0, 0, 0, "-", "undef-dup-of-definition", "",
            ))

    # A COFF WEAK EXTERNAL (IMAGE_SYM_CLASS_WEAK_EXTERNAL) is a reference the LINKER
    # resolves to the DEFAULT named by its auxiliary record whenever nothing supplies
    # a strong definition. cl spells a class's vector-deleting destructor slot that
    # way: `??_E<C>@@UAEPAXI@Z` weak, defaulting to the scalar `??_G<C>@@UAEPAXI@Z`.
    # The delinked target reads the LINKED image and so correctly names `??_G`, which
    # made every such vtable slot report a naming difference the source cannot fix -
    # measured 477 sites over 190 vtables. Do here what the linker did. The resolution
    # is local and total: all 508 weak externals in the tree are that one `??_E`/`??_G`
    # pair, and no object anywhere defines one of those names strongly (normalize_objs
    # re-proves that over the whole processed set, and fails the build if it changes).
    for symbol in coff.symbols.values():
        if symbol.storage_class != WEAK_EXTERNAL_STORAGE or symbol.aux_count < 1:
            continue
        tag = struct.unpack_from("<I", coff.data, symbol.offset + SYMBOL_SIZE)[0]
        default = coff.symbols.get(tag)
        if default is None or default.index == symbol.index:
            continue
        dup_retargets[symbol.index] = default.index
        rows.append(CanonicalRow(
            symbol.name, symbol.name, "weak", "undefined", 0, 0, 0, 0, 0, "-",
            "weak-external-resolved-to-" + default.name, "",
        ))
    # A default may itself be an undefined duplicate that the pass above retargeted.
    for index in list(dup_retargets):
        seen, target = {index}, dup_retargets[index]
        while target in dup_retargets and target not in seen:
            seen.add(target)
            target = dup_retargets[target]
        dup_retargets[index] = target

    for name, offset, size in materialized:
        rows.append(CanonicalRow(
            name, name, "common", "bss", 0, offset, size, 0, 0, "-",
            "materialized-common-into-bss", ""))

    # EH funclet symbols -> the owner-derived names the band carve uses, so a
    # `push <registration stub>` is compared against a NAMED span on both sides.
    for index, canonical in _eh_funclet_owners(coff).items():
        if index in renames:
            continue
        renames[index] = canonical
        symbol = coff.symbols[index]
        rows.append(CanonicalRow(
            symbol.name, canonical, "eh",
            "text" if symbol.section > 0 else "undefined",
            symbol.section, symbol.value, 0, 0, 0, "-",
            "eh-funclet-owner-derived-name", ""))

    # ... and the `.xdata$x` tables the registration stub loads. These OVERRIDE the
    # content-addressed `$anon_data_<digest>` name the pass above gave cl's `$T<n>`
    # labels: an anon digest pairs two anonymous definitions, and the delinked side
    # is not anonymous - it carries the owner-derived name, so the digest could
    # never pair with it. The override is applied after every digest is computed,
    # so no other symbol's identity moves.
    eh_data = _eh_funcinfo_owners(coff)
    if eh_data:
        superseded = {(coff.symbols[index].name, coff.symbols[index].section,
                       coff.symbols[index].value) for index in eh_data}
        rows = [row for row in rows
                if (row.original_name, row.section_ordinal, row.section_offset)
                not in superseded]
        for index, canonical in eh_data.items():
            renames[index] = canonical
            symbol = coff.symbols[index]
            rows.append(CanonicalRow(
                symbol.name, canonical, "eh", "rdata",
                symbol.section, symbol.value, 0, 0, 0, "-",
                "eh-funcinfo-owner-derived-name", ""))

    normalized = _rewrite_names(coff, renames)
    normalized, jump_table_rewrites = _rewrite_jump_table_relocations(
        coff, normalized)
    if dup_retargets:
        data = bytearray(normalized)
        jt_offsets = {rw.relocation_offset for rw in jump_table_rewrites}
        for relocation in coff.relocations:
            if (relocation.symbol_index in dup_retargets and
                    relocation.offset not in jt_offsets):
                struct.pack_into(
                    "<I", data, relocation.offset + 4,
                    dup_retargets[relocation.symbol_index])
        normalized = bytes(data)
    _assert_only_canonical_changes(
        coff, normalized, renames, jump_table_rewrites, dup_retargets)
    normalized, eh_rows = _canonicalize_eh_relocations(normalized)
    rows.extend(eh_rows)
    normalized, eh_section_rows = _canonicalize_eh_section_names(normalized)
    rows.extend(eh_section_rows)
    return CanonicalizedObject(normalized, tuple(rows))


SIDECAR_HEADER = (
    "original_name", "canonical_name", "family", "storage", "section_ordinal",
    "section_offset", "physical_size", "meaningful_size", "occurrence", "sha256",
    "proof", "preview",
)


def sidecar_bytes(rows: tuple[CanonicalRow, ...]) -> bytes:
    stream = io.StringIO(newline="")
    writer = csv.writer(stream, delimiter="\t", lineterminator="\n")
    writer.writerow(SIDECAR_HEADER)
    for row in rows:
        writer.writerow((
            row.original_name, row.canonical_name, row.family, row.storage,
            row.section_ordinal, f"0x{row.section_offset:x}", f"0x{row.physical_size:x}",
            f"0x{row.meaningful_size:x}", row.occurrence, row.digest, row.proof, row.preview,
        ))
    return stream.getvalue().encode("utf-8")


def _atomic_write(path: Path, payload: bytes) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    with tempfile.NamedTemporaryFile(
            dir=path.parent, prefix=f".{path.name}.", delete=False) as stream:
        temporary = Path(stream.name)
        stream.write(payload)
    os.replace(temporary, path)


def corpus_summary(roots: list[Path]) -> dict:
    summary = {"schema": 1, "roots": {}}
    for root in roots:
        counts = defaultdict(int)
        for path in sorted(root.rglob("*.obj")):
            result = canonicalize_coff(path.read_bytes())
            counts["objects"] += 1
            for row in result.rows:
                counts["rows"] += 1
                counts[f"family:{row.family}"] += 1
                counts[f"proof:{row.proof}"] += 1
                if row.canonical_name.startswith("$anon_f32_"):
                    counts["kind:f32"] += 1
                elif row.canonical_name.startswith("$anon_f64_"):
                    counts["kind:f64"] += 1
                elif row.canonical_name.startswith("$anon_str_"):
                    counts["kind:string"] += 1
                elif row.canonical_name.startswith(ANON_DATA_PREFIX):
                    counts["kind:data"] += 1
                elif row.canonical_name == row.original_name:
                    counts["kind:skipped"] += 1
                else:
                    counts["kind:named-static"] += 1
        summary["roots"][str(root)] = dict(sorted(counts.items()))
    return summary


from homm1.core.usage import logged


@logged
def main(argv=None):
    parser = argparse.ArgumentParser(
        prog="python3 -m homm1.compare.canonicalize", description=__doc__,
        formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--input", type=Path)
    parser.add_argument("--output", type=Path)
    parser.add_argument("--sidecar", type=Path)
    parser.add_argument("--summary-root", type=Path, action="append")
    parser.add_argument("--summary-output", type=Path)
    args = parser.parse_args(argv)
    if args.summary_root:
        if args.input or args.output or args.sidecar:
            parser.error("summary mode cannot be combined with object mode")
        payload = (json.dumps(corpus_summary(args.summary_root), indent=2,
                              sort_keys=True) + "\n").encode("utf-8")
        if args.summary_output:
            _atomic_write(args.summary_output, payload)
        else:
            print(payload.decode("utf-8"), end="")
        return 0
    if not args.input or not args.output or not args.sidecar:
        parser.error("object mode requires --input, --output, and --sidecar")
    resolved_paths = [path.resolve() for path in (
        args.input, args.output, args.sidecar,
    )]
    if len(set(resolved_paths)) != len(resolved_paths):
        parser.error("input, output, and sidecar paths must be distinct")
    result = canonicalize_coff(args.input.read_bytes())
    _atomic_write(args.output, result.data)
    _atomic_write(args.sidecar, sidecar_bytes(result.rows))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

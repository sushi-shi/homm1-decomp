"""Exact i386 COFF-to-retail comparison; relocation fields are resolved, not masked.

Bootstrap scope: one exported function and one RVA claim per source/object.
Unsupported topology fails closed instead of manufacturing a matching target.
"""
from dataclasses import dataclass
import re
import struct

from homm1.core.coff import CoffObject, DIR32, REL32
from homm1.core import manifest


@dataclass(frozen=True)
class Claim:
    rva: int
    size: int
    symbol: str


def source_claim(source, image):
    text = source.read_text()
    # Strip comments before reading the deliberately tiny bootstrap annotation grammar.
    text = re.sub(r'/\*.*?\*/|//[^\n]*', '', text, flags=re.S)
    annotations = re.findall(r'\bRVA\(\s*(0x[0-9a-fA-F]+)\s*,\s*(0x[0-9a-fA-F]+)\s*\)', text)
    if len(annotations) != 1:
        raise ValueError(f'{source}: bootstrap requires exactly one RVA annotation')
    rva, size = (int(value, 16) for value in annotations[0])
    section = image.section_of(rva)
    if size <= 0 or section is None or not section.executable or rva + size > section.rva + section.size:
        raise ValueError('claim is outside file-backed executable code')
    exports = [row for row in image.exports() if row['rva'] == rva and not row['forwarder']]
    if len(exports) != 1 or len(exports[0]['names']) != 1:
        raise ValueError('bootstrap claims must identify one named retail export')
    name = exports[0]['names'][0]
    # Bind by source name and explicit stdcall ABI; adding other signatures needs a real claim extractor.
    declaration = re.findall(r'extern\s+"C"\s+BOOL\s+__stdcall\s+' + re.escape(name)
                             + r'\(HWND\s+\w+,\s*UINT\s+\w+,\s*WPARAM\s+\w+,\s*LPARAM\s+\w+\)', text)
    if len(declaration) != 1:
        raise ValueError('bootstrap supports the evidenced four-argument Win32 dialog ABI only')
    return Claim(rva, size, f'_{name}@16')


def retail_relocations(image, claim, retail=None):
    rows = manifest.table('reloc_referents.tsv',
                          ('function_rva', 'site_rva', 'kind', 'symbol', 'target_rva', 'addend', 'provenance'), retail)
    result = []
    base_sites = {r['rva'] for r in image.relocations() if r['type'] == 3
                  and claim.rva <= r['rva'] < claim.rva + claim.size}
    kinds = manifest.check_retail(image, retail)
    imports = {s['iat_rva']: s['name'] for d in image.imports() for s in d['symbols']}
    seen = set()
    covered = set()
    for row in rows:
        if int(row['function_rva'], 0) != claim.rva:
            continue
        site = int(row['site_rva'], 0)
        target = int(row['target_rva'], 0)
        addend = int(row['addend'], 0)
        if site in seen or not claim.rva <= site <= claim.rva + claim.size - 4:
            raise ValueError('duplicate or out-of-range retail relocation site')
        if any(abs(site - other) < 4 for other in seen):
            raise ValueError('overlapping relocation fields')
        seen.add(site)
        if row['kind'] == 'DIR32':
            if site not in base_sites:
                raise ValueError('DIR32 site lacks a retail HIGHLOW relocation')
            if not row['symbol'].startswith('__imp__') or not re.fullmatch(
                    re.escape('__imp__' + str(imports.get(target))) + r'@\d+', row['symbol']):
                raise ValueError('DIR32 provider must name the exact retail IAT entry')
            covered.add(site)
            typ, expected = DIR32, image.image_base + target + addend
        elif row['kind'] == 'REL32':
            if image.read(site - 1, 1) not in (b'\xe8', b'\xe9') or kinds.get(target) != '':
                raise ValueError('REL32 requires a direct call/jump to an admitted function body')
            typ, expected = REL32, target + addend - (site + 4)
        else:
            raise ValueError(f'unsupported relocation kind {row["kind"]}')
        encoded = struct.unpack('<I', image.read(site, 4))[0]
        if encoded != expected & 0xffffffff or not row['provenance']:
            raise ValueError('retail relocation target/addend disagrees with encoded bytes')
        result.append(dict(site=site - claim.rva, typ=typ, symbol=row['symbol'],
                           target_rva=target, addend=addend))
    if covered != base_sites:
        raise ValueError('reviewed relocations do not cover every retail HIGHLOW field')
    return sorted(result, key=lambda row: row['site'])


def function_payload(obj, symbol):
    functions = [s for s in obj.symbols.values() if s.section > 0 and s.typ & 0x20 and s.storage_class == 2]
    if len(functions) != 1 or functions[0].name != symbol or functions[0].value != 0:
        raise ValueError('bootstrap object must contain exactly the claimed function at section offset zero')
    section = obj.section(functions[0].section)
    if not section.characteristics & 0x20000000:
        raise ValueError('function does not belong to executable code')
    # All emitted code is accounted for; no second section can hide a helper body.
    if any(s.raw_size and s.characteristics & 0x20000000 and s.index != section.index for s in obj.sections):
        raise ValueError('unexpected additional code section')
    return section, bytearray(obj.section_bytes(section))


def compare(obj, image, claim, references):
    section, code = function_payload(obj, claim.symbol)
    targets = {row['symbol']: image.image_base + row['target_rva'] for row in references}
    expected_relocs = {(row['site'], row['typ'], row['symbol'], row['addend'] & 0xffffffff) for row in references}
    actual_relocs = set()
    seen = set()
    for reloc in obj.relocations:
        if reloc.section != section.index:
            # MSVC /O2 emits FPO records referring to the function. These are
            # debugger metadata, not additional code or runtime storage.
            if obj.section(reloc.section).name == '.debug$F':
                continue
            raise ValueError('relocations outside the claimed code are not supported yet')
        if reloc.typ not in (DIR32, REL32) or reloc.site + 4 > len(code):
            raise ValueError('unsupported or out-of-bounds COFF relocation')
        if any(abs(reloc.site - site) < 4 for site in seen):
            raise ValueError('overlapping COFF relocation fields')
        seen.add(reloc.site)
        symbol = obj.symbols[reloc.symbol_index]
        if symbol.section != 0 or symbol.name not in targets:
            raise ValueError(f'unresolved relocation referent {symbol.name}')
        addend = struct.unpack_from('<I', code, reloc.site)[0]
        actual_relocs.add((reloc.site, reloc.typ, symbol.name, addend))
        value = targets[symbol.name] + addend
        if reloc.typ == REL32:
            value -= image.image_base + claim.rva + reloc.site + 4
        struct.pack_into('<I', code, reloc.site, value & 0xffffffff)
    retail = image.read(claim.rva, claim.size)
    mismatches = [i for i, (a, b) in enumerate(zip(code, retail)) if a != b]
    same_relocations = actual_relocs == expected_relocs
    return dict(symbol=claim.symbol, rva=claim.rva, retail_size=len(retail), compiled_size=len(code),
                exact=code == retail and same_relocations, relocations_exact=same_relocations,
                relocation_count=len(actual_relocs), differing_offsets=mismatches,
                matched_bytes=sum(a == b for a, b in zip(code, retail)))


def target_object(image, claim, references):
    """Carve retail bytes into an independent one-function COFF, reversing reviewed relocations."""
    code = bytearray(image.read(claim.rva, claim.size))
    names = [claim.symbol] + sorted({r['symbol'] for r in references})
    for row in references:
        struct.pack_into('<I', code, row['site'], row['addend'] & 0xffffffff)
    relocation_data = b''.join(struct.pack('<IIH', r['site'], names.index(r['symbol']), r['typ'])
                               for r in references)
    symbols_offset = 60 + len(code) + len(relocation_data)
    strings = bytearray(b'\0' * 4)
    symbols = bytearray()
    for i, name in enumerate(names):
        encoded = name.encode('ascii')
        if len(encoded) <= 8:
            symbols += encoded.ljust(8, b'\0')
        else:
            symbols += struct.pack('<II', 0, len(strings))
            strings += encoded + b'\0'
        symbols += struct.pack('<IhHBB', 0, 1 if i == 0 else 0, 0x20, 2, 0)
    struct.pack_into('<I', strings, 0, len(strings))
    header = struct.pack('<HHIIIHH', 0x14c, 1, 0, symbols_offset, len(names), 0, 0)
    section = b'.text\0\0\0' + struct.pack('<IIIIIIHHI', 0, 0, len(code), 60,
                                            60 + len(code), 0, len(references), 0, 0x60000020)
    return header + section + code + relocation_data + symbols + strings

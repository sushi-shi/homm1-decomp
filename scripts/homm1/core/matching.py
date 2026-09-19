"""Exact i386 COFF-to-retail comparison; relocation fields are resolved, not masked.

Bootstrap scope: one exported function and one RVA claim per source/object.
Unsupported topology fails closed instead of manufacturing a matching target.
"""
from dataclasses import dataclass
import re
import struct

from homm1.core.coff import CoffObject, DIR32, REL32
from homm1.core import manifest
from homm1.core.disasm import code_instructions


@dataclass(frozen=True)
class Claim:
    rva: int
    size: int
    symbol: str
    unit: str = ''
    source: str = ''
    src_hash: str = ''
    parent: int | None = None


def source_claim(source, image):
    from homm1.labels import definitions
    claims = definitions(source)
    if len(claims) != 1:
        raise ValueError('single-function compatibility API requires one claim')
    return claims[0]


def retail_relocations(image, claim, retail=None):
    rows = manifest.table('reloc_referents.tsv',
                          ('function_rva', 'site_rva', 'kind', 'symbol', 'target_rva', 'addend', 'provenance'), retail)
    result = []
    base_sites = {r['rva'] for r in image.relocations() if r['type'] == 3
                  and claim.rva <= r['rva'] < claim.rva + claim.size}
    kinds = manifest.check_retail(image, retail)
    imports = {s['iat_rva']: s['name'] for d in image.imports() for s in d['symbols']}
    data_path = (retail or manifest.CONFIG / 'retail') / 'data_symbols.tsv'
    data_rows = manifest.table('data_symbols.tsv', ('rva', 'size', 'symbol', 'provenance'), retail) if data_path.exists() else []
    data_symbols = {}
    for row in data_rows:
        rva, size = int(row['rva'], 0), int(row['size'], 0)
        section = image.section_of(rva)
        if not section or section.executable or size <= 0 or rva + size > section.rva + section.mapped or not row['provenance']:
            raise ValueError('invalid referenced data identity or extent')
        if row['symbol'] in data_symbols:
            raise ValueError('duplicate data symbol identity')
        data_symbols[row['symbol']] = (rva, size)
    decoded, absolute_fields = code_instructions(image, claim, retail)
    relative_fields, required_relative = set(), set()
    for ins in decoded:
        if ins.disp_size == 4:
            absolute_fields.add(ins.address + ins.disp_offset)
        if ins.imm_size == 4:
            absolute_fields.add(ins.address + ins.imm_offset)
            if ins.mnemonic == 'call' or ins.mnemonic.startswith('j'):
                site = ins.address + ins.imm_offset
                relative_fields.add(site)
                target = ins.operands[0].imm
                if ins.mnemonic == 'call' or not claim.rva <= target < claim.rva + claim.size:
                    required_relative.add(site)
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
            if site not in base_sites or site not in absolute_fields:
                raise ValueError('DIR32 site lacks a retail HIGHLOW relocation')
            if target in imports:
                if not re.fullmatch(re.escape('__imp__' + str(imports[target])) + r'(?:@\d+)?', row['symbol']):
                    raise ValueError('DIR32 provider must name the exact retail IAT entry')
            elif row['symbol'] in data_symbols:
                base, size = data_symbols[row['symbol']]
                if target != base or not 0 <= addend < size:
                    raise ValueError('DIR32 disagrees with referenced data identity/extent')
            elif target not in kinds:
                raise ValueError('DIR32 requires an admitted code or data identity')
            covered.add(site)
            typ, expected = DIR32, image.image_base + target + addend
        elif row['kind'] == 'REL32':
            if site not in relative_fields or kinds.get(target) not in ('', 'eh', 'helper', 'thunk'):
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
    if not required_relative <= {claim.rva + r['site'] for r in result if r['typ'] == REL32}:
        raise ValueError('reviewed relocations do not cover every outgoing direct call/jump')
    return sorted(result, key=lambda row: row['site'])


def function_extent(obj, symbol, claims=()):
    found = [s for s in obj.symbols.values() if s.section > 0 and s.name == symbol]
    if len(found) != 1:
        raise ValueError(f'object does not confirm exactly one definition of {symbol}')
    function = found[0]
    section = obj.section(function.section)
    if not section.characteristics & 0x20000000:
        raise ValueError('function does not belong to executable code')
    names = {c.symbol for c in claims}
    boundaries = [s.value for s in obj.symbols.values() if s.section == section.index
                  and s.value > function.value and (s.typ & 0x20 or s.name in names)]
    end = min(boundaries, default=section.raw_size)
    # VC4 /Z7 emits classic COFF function-definition auxiliaries, including
    # the measured code size. No retail size participates in this calculation.
    if function.typ & 0x20 and function.aux_count:
        size = struct.unpack_from('<I', obj.data, function.offset + 18 + 4)[0]
        if size:
            end = min(end, function.value + size)
    if not function.value < end <= section.raw_size:
        raise ValueError(f'invalid object function extent for {symbol}')
    return section, function.value, end


def function_payload(obj, symbol, claims=()):
    section, start, end = function_extent(obj, symbol, claims)
    return section, bytearray(obj.section_bytes(section)[start:end])


def confirm_object(obj, claims):
    names = {c.symbol for c in claims}
    emitted = {s.name for s in obj.symbols.values() if s.section > 0 and s.typ & 0x20
               and obj.section(s.section).characteristics & 0x20000000}
    if emitted - names:
        raise ValueError(f'emitted functions lack source/generated ownership: {sorted(emitted - names)}')
    spans = {}
    for claim in claims:
        section, start, end = function_extent(obj, claim.symbol, claims)
        spans.setdefault(section.index, []).append((start, end))
    for section in obj.sections:
        if not section.characteristics & 0x20000000 or not section.raw_size:
            continue
        covered = bytearray(section.raw_size)
        for start, end in spans.get(section.index, []):
            if any(covered[start:end]):
                raise ValueError('overlapping compiled function ownership')
            covered[start:end] = b'\1' * (end - start)
        if any(not owner and byte not in (0x90, 0xcc) for owner, byte in zip(covered, obj.section_bytes(section))):
            raise ValueError('unexplained executable bytes outside compiled function ownership')


def compare(obj, image, claim, references, claims=(), namespace=None):
    section, start, end = function_extent(obj, claim.symbol, claims)
    code = bytearray(obj.section_bytes(section)[start:end])
    targets = {row['symbol']: image.image_base + row['target_rva'] for row in references}
    targets.update({c.symbol: image.image_base + c.rva for c in claims})
    targets.update({name: image.image_base + rva for name, rva in (namespace or {}).items()})
    expected_relocs = {(row['site'], row['typ'], row['symbol'], row['addend'] & 0xffffffff) for row in references}
    actual_relocs = set()
    seen = set()
    for reloc in obj.relocations:
        if reloc.section != section.index or not start <= reloc.site < end:
            continue
        site = reloc.site - start
        if reloc.typ not in (DIR32, REL32) or site + 4 > len(code):
            raise ValueError('unsupported or out-of-bounds COFF relocation')
        if any(abs(site - previous) < 4 for previous in seen):
            raise ValueError('overlapping COFF relocation fields')
        seen.add(site)
        symbol = obj.symbols[reloc.symbol_index]
        name = symbol.name
        addend = struct.unpack_from('<I', code, site)[0]
        if name not in targets and symbol.section == section.index and start <= symbol.value < end:
            name, addend = claim.symbol, (addend + symbol.value - start) & 0xffffffff
            targets[name] = image.image_base + claim.rva
        if name not in targets:
            raise ValueError(f'unresolved relocation referent {name}')
        actual_relocs.add((site, reloc.typ, name, addend))
        value = targets[name] + addend
        if reloc.typ == REL32:
            value -= image.image_base + claim.rva + site + 4
        struct.pack_into('<I', code, site, value & 0xffffffff)
    retail = image.read(claim.rva, claim.size)
    mismatches = [i for i, (a, b) in enumerate(zip(code, retail)) if a != b]
    mismatches.extend(range(min(len(code), len(retail)), max(len(code), len(retail))))
    same_relocations = actual_relocs == expected_relocs
    return dict(symbol=claim.symbol, rva=claim.rva, retail_size=len(retail), compiled_size=len(code),
                exact=code == retail and same_relocations, relocations_exact=same_relocations,
                relocation_count=len(actual_relocs), differing_offsets=mismatches,
                matched_bytes=sum(a == b for a, b in zip(code, retail)),
                actual_relocations=sorted(actual_relocs), expected_relocations=sorted(expected_relocs),
                resolved_hex=code.hex())


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

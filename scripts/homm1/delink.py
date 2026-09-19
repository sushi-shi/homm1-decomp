"""Sparse synthetic PDB -> pinned vostok -> independently validated COFF.

PDB YAML/C13 construction and the empty symbol-stream repair are adapted from
Gruntz delink/pdb_synth. Zero-size external records are identities, not extents.
"""
import hashlib
import json
from pathlib import Path
import re
import shutil
import struct
import subprocess
import tempfile

from homm1.core.inputs import REPO, targets
from homm1.core.coff import CoffObject
from homm1.normalized_freshness import write_stamp


def quote(text):
    return "'" + text.replace("'", "''") + "'"


def yaml_text(image, claims, references):
    files = {c.rva: 'c:\\homm1\\' + c.unit + '.cpp' for c in claims}
    lines = ['MSF:', '  SuperBlock:', '    BlockSize: 4096',
             'PdbStream:', '  Age: 1', "  Guid: '{00000000-0000-0000-0000-000000000000}'",
             '  Signature: 0', '  Features: [ VC140 ]', '  Version: VC70',
             'DbiStream:', '  VerHeader: V70', '  Age: 1', '  BuildNumber: 0',
             '  PdbDllVersion: 0', '  PdbDllRbld: 0', '  Flags: 0', '  MachineType: x86',
             '  Modules:', "    - Module: 'c:\\homm1\\retail.obj'",
             "      ObjFile: 'c:\\homm1\\retail.obj'", '      SourceFiles:']
    paths = sorted(set(files.values()))
    lines += ['        - ' + quote(p) for p in paths]
    lines += ['      Subsections:', '        - !FileChecksums', '          Checksums:']
    for path in paths:
        lines += ['            - FileName: ' + quote(path), '              Kind: MD5',
                  '              Checksum: ' + hashlib.md5(path.encode()).hexdigest().upper()]
    def segment(rva):
        section = image.section_of(rva)
        if section is None:
            raise ValueError(f'PDB identity outside image: {rva:#x}')
        return image.sections.index(section) + 1, rva - section.rva
    for claim in claims:
        seg, off = segment(claim.rva)
        lines += ['        - !Lines', f'          CodeSize: {claim.size}', '          Flags: []',
                  f'          RelocOffset: {off}', f'          RelocSegment: {seg}',
                  '          Blocks:', '            - FileName: ' + quote(files[claim.rva]),
                  '              Lines:', '                - Offset: 0', '                  LineStart: 1',
                  '                  EndDelta: 0', '                  IsStatement: true', '              Columns: []']
    lines += ['      Modi:', '        Records:']
    symbols = {c.symbol: (c.rva, c.size, True) for c in claims}
    for refs in references.values():
        for ref in refs:
            identity = (ref['target_rva'], 0, image.section_of(ref['target_rva']).executable)
            if ref['symbol'] in symbols and symbols[ref['symbol']][0] != identity[0]:
                raise ValueError('conflicting PDB symbol identities')
            symbols.setdefault(ref['symbol'], identity)
    for name, (rva, size, code) in sorted(symbols.items(), key=lambda item: (item[1][0], item[0])):
        seg, off = segment(rva)
        if code:
            lines += ['          - Kind: S_GPROC32', '            ProcSym:', f'              CodeSize: {size}',
                      '              DbgStart: 0', '              DbgEnd: 0', '              FunctionType: 0',
                      f'              Offset: {off}', f'              Segment: {seg}', '              Flags: []',
                      '              DisplayName: ' + quote(name),
                      '          - Kind: S_END', '            ScopeEndSym: {}']
        else:
            lines += ['          - Kind: S_LDATA32', '            DataSym:', '              Type: 0',
                      f'              Offset: {off}', f'              Segment: {seg}',
                      '              DisplayName: ' + quote(name)]
    lines += ['StringTable:'] + ['  - ' + quote(p) for p in paths]
    return '\n'.join(lines) + '\n'


def repair_empty_symbol_stream(path):
    """llvm yaml2pdb leaves the DBI symbol-record stream nil; point at an empty stream."""
    data = bytearray(path.read_bytes())
    if not data.startswith(b'Microsoft C/C++ MSF 7.00\r\n\x1aDS\0\0\0'):
        raise ValueError('unexpected synthetic PDB format')
    block_size, _, _, directory_size, _, map_block = struct.unpack_from('<6I', data, 32)
    count = (directory_size + block_size - 1) // block_size
    blocks = struct.unpack_from(f'<{count}I', data, map_block * block_size)
    directory = b''.join(data[b * block_size:(b + 1) * block_size] for b in blocks)[:directory_size]
    count, = struct.unpack_from('<I', directory)
    sizes = struct.unpack_from(f'<{count}I', directory, 4)
    position, streams = 4 + 4 * count, []
    for size in sizes:
        length = 0 if size == 0xffffffff else (size + block_size - 1) // block_size
        streams.append(struct.unpack_from(f'<{length}I', directory, position))
        position += 4 * length
    if len(streams) <= 3 or not streams[3] or 0 not in sizes:
        raise ValueError('synthetic PDB has no DBI or empty stream')
    struct.pack_into('<H', data, streams[3][0] * block_size + 0x14, sizes.index(0))
    path.write_bytes(data)


def sparse_relocation_view(image, claims):
    """Select directory records for admitted code; never change code/data bytes.

The pinned delinker otherwise attempts to own every unrelated data relocation.
This generated view is not a retail input or an exactness oracle: comparison
and fixup-coverage validation always use the original hash-verified image.
"""
    data = bytearray(image.data)
    rva, size = image.directory(5)
    end = rva + size
    while rva < end:
        page, length = struct.unpack('<II', image.read(rva, 8))
        for at in range(rva + 8, rva + length, 2):
            value, = struct.unpack('<H', image.read(at, 2))
            site = page + (value & 0xfff)
            if not any(c.rva <= site < c.rva + c.size for c in claims):
                struct.pack_into('<H', data, image.offset(at, 2), 0)
        rva += length
    return bytes(data)


def normalize(raw, image, claims, references):
    """Remove delinker padding/data ownership and restore reviewed code fixups.

Every retained byte is first resolved and checked against the ORIGINAL image.
In particular, a missing delinker fixup cannot conceal an address mismatch.
Candidate objects never pass through this retail-extent normalization.
"""
    obj = CoffObject(raw)
    namespace = {c.symbol: c.rva for c in claims}
    for refs in references.values():
        namespace.update({r['symbol']: r['target_rva'] for r in refs})
    payloads, covered = [], {}
    for claim in claims:
        found = [s for s in obj.symbols.values() if s.name == claim.symbol and s.section > 0]
        if len(found) != 1:
            raise ValueError(f'delinker did not bind {claim.symbol} uniquely')
        symbol = found[0]
        section = obj.section(symbol.section)
        start, end = symbol.value, symbol.value + claim.size
        if not section.characteristics & 0x20000000 or end > section.raw_size:
            raise ValueError('delinker output truncated claimed code')
        mask = covered.setdefault(section.index, bytearray(section.raw_size))
        if any(mask[start:end]):
            raise ValueError('overlapping delinker definitions')
        mask[start:end] = b'\1' * claim.size
        code = bytearray(obj.section_bytes(section)[start:end])
        refs = references[claim.rva]
        expected = {(r['site'], r['typ'], r['symbol'], r['addend'] & 0xffffffff) for r in refs}
        seen = set()
        for reloc in obj.relocations:
            if reloc.section != section.index or not start <= reloc.site < end:
                continue
            site = reloc.site - start
            if site + 4 > len(code):
                raise ValueError('delinker relocation crosses function boundary')
            target = obj.symbols[reloc.symbol_index].name
            addend, = struct.unpack_from('<I', code, site)
            identity = (site, reloc.typ, target, addend)
            if identity not in expected or site in seen:
                raise ValueError(f'delinker produced an unreviewed relocation: {identity}')
            seen.add(site)
            value = image.image_base + namespace[target] + addend
            if reloc.typ == 0x14:
                value -= image.image_base + claim.rva + site + 4
            struct.pack_into('<I', code, site, value & 0xffffffff)
        if bytes(code) != image.read(claim.rva, claim.size):
            raise ValueError(f'delinker bytes do not resolve to original retail for {claim.symbol}')
        for ref in refs:
            struct.pack_into('<I', code, ref['site'], ref['addend'] & 0xffffffff)
        payloads.append((claim, bytes(code), refs))
    for section in obj.sections:
        if section.characteristics & 0x20000000:
            mask = covered.get(section.index, bytearray(section.raw_size))
            if any(not used and byte not in (0x90, 0xcc) for used, byte in zip(mask, obj.section_bytes(section))):
                raise ValueError('unexplained code in delinker output')
    return code_object(payloads)


def code_object(payloads):
    """Canonical per-function sections; referenced data remains undefined/unscored."""
    definitions = {claim.symbol: index + 1 for index, (claim, _, _) in enumerate(payloads)}
    names = list(definitions) + sorted({r['symbol'] for _, _, refs in payloads for r in refs} - definitions.keys())
    indices = {name: index for index, name in enumerate(names)}
    offset = 20 + 40 * len(payloads)
    headers, content = bytearray(), bytearray()
    for claim, code, refs in payloads:
        relocations = b''.join(struct.pack('<IIH', r['site'], indices[r['symbol']], r['typ']) for r in refs)
        headers += b'.text\0\0\0' + struct.pack('<IIIIIIHHI', 0, 0, len(code), offset,
                                               offset + len(code), 0, len(refs), 0, 0x60000020)
        content += code + relocations
        offset += len(code) + len(relocations)
    strings, symbols = bytearray(b'\0' * 4), bytearray()
    for name in names:
        encoded = name.encode('ascii')
        if len(encoded) <= 8:
            symbols += encoded.ljust(8, b'\0')
        else:
            symbols += struct.pack('<II', 0, len(strings))
            strings += encoded + b'\0'
        symbols += struct.pack('<IhHBB', 0, definitions.get(name, 0), 32 if name in definitions else 0, 2, 0)
    struct.pack_into('<I', strings, 0, len(strings))
    return struct.pack('<HHIIIHH', 0x14c, len(payloads), 0, offset, len(names), 0, 0) + headers + content + symbols + strings


def _generate_unit(image, claims, references):
    for tool in ('llvm-pdbutil', 'vostok-delinker'):
        if not shutil.which(tool):
            raise ValueError(f'{tool} required; enter nix develop')
    help_text = subprocess.check_output(['vostok-delinker', '--help'], text=True)
    for option in ('--pdb-path', '--exe-path', '--output-path', '--engine-path'):
        if option not in help_text:
            raise ValueError('vostok-delinker does not implement the pinned interface')
    destination = REPO / 'build/delink' / claims[0].unit
    destination.mkdir(parents=True, exist_ok=True)
    # Fresh scratch output prevents successful reads from an earlier generation.
    with tempfile.TemporaryDirectory(prefix='run-', dir=destination) as directory:
        scratch = Path(directory)
        yaml = scratch / 'retail.yaml'
        pdb = scratch / 'retail.pdb'
        yaml.write_text(yaml_text(image, claims, references))
        subprocess.run(['llvm-pdbutil', 'yaml2pdb', str(yaml), '--pdb=' + str(pdb)],
                       check=True, capture_output=True, text=True, timeout=120)
        repair_empty_symbol_stream(pdb)
        output = scratch / 'objects'
        output.mkdir()
        view = scratch / 'code-view.exe'
        view.write_bytes(sparse_relocation_view(image, claims))
        result = subprocess.run(['vostok-delinker', '--pdb-path', str(pdb),
                                 '--exe-path', str(view),
                                 '--output-path', str(output), '--engine-path', 'c:\\homm1\\'],
                                capture_output=True, text=True, timeout=120)
        (destination / 'last.log').write_text(result.stdout + result.stderr)
        if result.returncode:
            raise ValueError('vostok delinking failed:\n' + (result.stdout + result.stderr)[-4000:])
        objects = {}
        for unit in sorted({c.unit for c in claims}):
            matches = list(output.rglob(unit + '.cpp.obj'))
            if len(matches) != 1:
                raise ValueError(f'vostok did not emit exactly one object for {unit}: {list(output.rglob("*.obj"))}')
            payload = normalize(matches[0].read_bytes(), image, [c for c in claims if c.unit == unit], references)
            objects[unit] = payload
        for path in (yaml, pdb):
            shutil.copyfile(path, destination / path.name)
        shutil.copyfile(view, destination / 'code-view.exe')
        for unit in objects:
            raw = destination / (unit + '.raw.obj')
            shutil.copyfile(next(output.rglob(unit + '.cpp.obj')), raw)
            write_stamp(raw, {'image': destination / 'code-view.exe',
                              'pdb': destination / 'retail.pdb',
                              'delinker': Path(shutil.which('vostok-delinker'))})
        return objects


def stamp_target(unit, target):
    write_stamp(target, {'raw': REPO / 'build/delink' / unit / (unit + '.raw.obj'),
                         'normalizer': Path(__file__),
                         'referents': REPO / 'config/retail/reloc_referents.tsv'})


def generate(image, claims, references):
    # Each unit has its own symbol namespace, including ordinary static names.
    # Zero-size cross-unit referents remain identities, not owned definitions.
    objects = {}
    for unit in sorted({c.unit for c in claims}):
        owned = [c for c in claims if c.unit == unit]
        scoped = {c.rva: references[c.rva] for c in owned}
        objects.update(_generate_unit(image, owned, scoped))
    return objects


def command(_args):
    from homm1 import build, model
    image = build.image()
    claims, refs = model.resolve(image)
    objects = generate(image, claims, refs)
    path = REPO / 'build/objdiff/target'
    path.mkdir(parents=True, exist_ok=True)
    for unit, payload in objects.items():
        target = path / (unit + '.obj')
        target.write_bytes(payload)
        stamp_target(unit, target)
    print(json.dumps(dict(units=list(objects), scope='claimed code; referenced data identities only'), indent=2))

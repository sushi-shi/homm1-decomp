"""Gruntz-style build contracts and retail census/provider invariants."""
import csv
from pathlib import Path
import tomllib

from homm1.core.inputs import REPO

CONFIG = REPO / 'config'
FUNCTION_KINDS = {'', 'thunk', 'eh', 'helper', 'pad'}
DATA_KINDS = {'', 'string', 'fppool', 'vtable', 'rtti', 'ehtable', 'guard', 'common', 'copy', 'pad'}


def load(path: Path | None = None):
    return tomllib.loads((path or CONFIG / 'units.toml').read_text())


def table(name, columns, retail=None):
    with ((retail or CONFIG / 'retail') / name).open(newline='') as handle:
        reader = csv.DictReader((line for line in handle if line.strip() and not line.startswith('#')),
                                delimiter='\t')
        if tuple(reader.fieldnames or ()) != columns:
            raise ValueError(f'{name}: expected columns {columns}')
        rows = list(reader)
    if any(None in row or any(value is None for value in row.values()) for row in rows):
        raise ValueError(f'{name}: malformed TSV row')
    return rows


def check_retail(image, retail=None):
    bases = {}
    for filename, kinds, code in [('functions.tsv', FUNCTION_KINDS, True),
                                  ('data.tsv', DATA_KINDS, False)]:
        rows = table(filename, ('rva', 'kind'), retail)
        admitted = {}
        previous = -1
        for row in rows:
            rva = int(row['rva'], 0)
            section = image.section_of(rva)
            if rva <= previous:
                raise ValueError(f'{filename}: RVAs must be unique and ascending')
            if section is None or section.executable != code:
                raise ValueError(f'{filename}: {row["rva"]} is outside the correct address space')
            if code and rva >= section.rva + section.size:
                raise ValueError(f'{filename}: function start has no file bytes')
            if not code and section.name not in ('.data', '.rdata', '.bss'):
                raise ValueError(f'{filename}: data census only covers .data/.rdata/.bss')
            if row['kind'] not in kinds:
                raise ValueError(f'{filename}: unsupported kind {row["kind"]!r}')
            admitted[rva] = row['kind']
            previous = rva
        bases[filename] = admitted
    exports = {(row['rva'], name, row['ordinal']) for row in image.exports()
               if row['forwarder'] is None for name in row['names']}
    seen = set()
    for row in table('functions_exports.tsv', ('rva', 'name', 'ordinal', 'provenance'), retail):
        rva = int(row['rva'], 0)
        if bases['functions.tsv'].get(rva) != '':
            raise ValueError('export provider RVA must be an admitted function body')
        key = (rva, row['name'], int(row['ordinal']))
        if key in seen or key not in exports or not row['provenance']:
            raise ValueError(f'export provider disagrees with retail or duplicates a row: {key}')
        seen.add(key)
    retail_root = retail or CONFIG / 'retail'
    root = retail_root.parent.parent
    if (retail_root / 'functions_correspondence.tsv').exists():
        from homm1.correspondence import load as correspondences
        correspondences(root, image)
    return bases['functions.tsv']

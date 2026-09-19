"""One join of sparse retail facts, source claims and reviewed referents."""
from dataclasses import asdict, replace
import json

from homm1 import labels
from homm1.core import manifest
from homm1.core.inputs import REPO
from homm1.core.matching import retail_relocations


def validate(claims, image, admitted):
    symbols, spans = set(), []
    for claim in sorted(claims, key=lambda c: c.rva):
        section = image.section_of(claim.rva)
        if claim.size <= 0 or not section or not section.executable or claim.rva + claim.size > section.rva + section.size:
            raise ValueError(f'claim {claim.symbol} is outside file-backed executable code')
        if admitted.get(claim.rva) not in ({'eh', 'helper', 'thunk'} if claim.parent is not None else {''}):
            raise ValueError(f'claim {claim.symbol} lacks an admitted retail function of the correct kind')
        if spans and claim.rva < spans[-1][1]:
            raise ValueError('overlapping or duplicate reconstruction claims')
        if claim.symbol in symbols:
            raise ValueError(f'duplicate source symbol {claim.symbol}')
        symbols.add(claim.symbol)
        spans.append((claim.rva, claim.rva + claim.size))


def resolve(image, config=None, entries=None):
    if config is None:
        from homm1.build import units
        config, entries = units()
    admitted = manifest.check_retail(image)
    data_bases = {int(r['rva'], 0) for r in manifest.table('data.tsv', ('rva', 'kind'))}
    for row in manifest.table('data_symbols.tsv', ('rva', 'size', 'symbol', 'provenance')):
        if int(row['rva'], 0) not in data_bases:
            raise ValueError('data identity provider has no sparse census row')
    declarations = {}
    claims = [replace(c, unit=u['unit']) for u in entries
              for c in labels.definitions(REPO / u['source'], config['build']['compiler'], config['flags'][u['flags']], declarations)]
    validate(claims, image, admitted)
    references = {c.rva: retail_relocations(image, c) for c in claims}
    return claims, references


def command(_args):
    from homm1.build import image
    claims, references = resolve(image())
    print(json.dumps(dict(scope='sparse code claims; no data matching',
                          claims=[asdict(c) for c in claims], references=references), indent=2))

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
        identity = (claim.unit or claim.source, claim.symbol) if claim.linkage == 'internal' else ('', claim.symbol)
        if identity in symbols:
            raise ValueError(f'duplicate source symbol {claim.symbol}')
        symbols.add(identity)
        spans.append((claim.rva, claim.rva + claim.size))


def resolve(image, config=None, entries=None, source_claims=None):
    if config is None:
        from homm1.build import units
        config, entries = units()
    admitted = manifest.check_retail(image)
    data_bases = {int(r['rva'], 0) for r in manifest.table('data.tsv', ('rva', 'kind'))}
    for row in manifest.table('data_symbols.tsv', ('rva', 'size', 'symbol', 'provenance')):
        if int(row['rva'], 0) not in data_bases:
            raise ValueError('data identity provider has no sparse census row')
    declarations = {}
    claims = source_claims if source_claims is not None else [replace(c, unit=u['unit']) for u in entries
              for c in labels.definitions(REPO / u['source'], config['build']['compiler'], config['flags'][u['flags']], declarations)]
    validate(claims, image, admitted)
    references = {c.rva: retail_relocations(image, c) for c in claims}
    validate_referents(claims, references)
    return claims, references


def validate_referents(claims, references):
    owned = {c.rva: c for c in claims}
    for caller in claims:
        names = {c.symbol: c.rva for c in claims if c.unit == caller.unit}
        for ref in references[caller.rva]:
            target = owned.get(ref['target_rva'])
            if target and target.symbol != ref['symbol']:
                raise ValueError('retail referent disagrees with source symbol identity')
            if target and target.linkage == 'internal' and caller.unit != target.unit:
                raise ValueError('internal source symbol referenced from a different unit')
            if ref['symbol'] in names and names[ref['symbol']] != ref['target_rva']:
                raise ValueError('conflicting relocation identities in one source unit')
            names[ref['symbol']] = ref['target_rva']


def command(_args):
    from homm1.build import image
    claims, references = resolve(image())
    print(json.dumps(dict(scope='sparse code claims; no data matching',
                          claims=[asdict(c) for c in claims], references=references), indent=2))

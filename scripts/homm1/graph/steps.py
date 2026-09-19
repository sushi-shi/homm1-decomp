"""CLI adapters for the Gruntz graph edges and HoMM1's current data contracts.

The producers remain labels/model/delink/build. These adapters serialize their
outputs, separating source fingerprints from retail binding identity so a body
edit does not invalidate independent target extraction.
"""
import argparse
import csv
import io
from dataclasses import asdict, replace
import json
import sys

from homm1 import analysis, build, labels, model, delink, publication, toolchain
from homm1.core.inputs import REPO
from homm1.core.matching import Claim
from homm1.graph.cc import install


def write(path, value):
    publication.atomic_write(REPO / path, json.dumps(value, indent=2, sort_keys=True) + '\n')


def unit_config(name):
    config, units = build.units()
    unit = next((u for u in units if u['unit'] == name), None)
    if unit is None:
        raise ValueError(f'unknown graph unit {name}')
    return config, unit


def extract(name):
    config, unit = unit_config(name)
    source = REPO / unit['source']
    compiler, flags = config['build']['compiler'], config['flags'][unit['flags']]
    declarations = {}
    claims = [replace(c, unit=name) for c in labels.definitions(source, compiler, flags, declarations)]
    analysis.run(source, compiler, strict=True, flags=flags)
    dependencies = {str(p.relative_to(REPO)) if p.is_relative_to(REPO) else str(p): toolchain.digest(p)
                    for p in analysis.dependencies(source, compiler, flags)}
    write(f'build/gen/claims/{name}.json', dict(claims=[asdict(c) for c in claims],
                                              declarations=declarations, dependencies=dependencies))


def read_claims():
    _, units = build.units()
    claims, declarations, dependencies = [], {}, {}
    for unit in units:
        value = json.loads((REPO / f'build/gen/claims/{unit["unit"]}.json').read_text())
        claims += [Claim(**c) for c in value['claims']]
        for name, signature in value['declarations'].items():
            if name in declarations and declarations[name] != signature:
                raise ValueError(f'conflicting source declarations for {name}')
            declarations[name] = signature
        dependencies.update(value['dependencies'])
    return claims, declarations, dependencies


def bind():
    config, units = build.units()
    claims, _, _ = read_claims()
    claims, references = model.resolve(build.image(), config, units, source_claims=claims)
    internal = {c.rva for c in claims if c.linkage == 'internal'}
    namespace = {c.symbol: c.rva for c in claims if c.linkage != 'internal'}
    for refs in references.values():
        for ref in refs:
            name, rva = ref['symbol'], ref['target_rva']
            if rva in internal:
                continue
            if name in namespace and namespace[name] != rva:
                raise ValueError(f'conflicting relocation namespace for {name}')
            namespace[name] = rva
    for unit in units:
        owned = [c for c in claims if c.unit == unit['unit']]
        structural = [{key: value for key, value in asdict(c).items()
                       if key not in ('source', 'src_hash', 'context_hash')} for c in owned]
        write(f'build/gen/bindings/{unit["unit"]}.json',
              dict(claims=structural, references={c.rva: references[c.rva] for c in owned}))
    write('build/gen/namespace.json', namespace)
    inventory = io.StringIO()
    writer = csv.writer(inventory, lineterminator='\n')
    writer.writerow(('rva', 'name', 'unit', 'size', 'kind', 'provenance'))
    for claim in sorted(claims, key=lambda c: c.rva):
        writer.writerow((hex(claim.rva), claim.symbol, claim.unit, hex(claim.size), 'func', 'source RVA annotation'))
    publication.atomic_write(REPO / 'build/gen/symbol_names.csv', inventory.getvalue())


def bindings(name):
    value = json.loads((REPO / f'build/gen/bindings/{name}.json').read_text())
    return [Claim(**c) for c in value['claims']], {int(k): v for k, v in value['references'].items()}


def target(name):
    claims, refs = bindings(name)
    payload = delink.generate(build.image(), claims, refs)[name]
    path = REPO / f'build/objdiff/target/{name}.obj'
    path.parent.mkdir(parents=True, exist_ok=True)
    install(payload, path)
    delink.stamp_target(name, path)


def report(name):
    _, unit = unit_config(name)
    data = json.loads((REPO / f'build/gen/claims/{name}.json').read_text())
    claims = [Claim(**c) for c in data['claims']]
    _, refs = bindings(name)
    namespace = json.loads((REPO / 'build/gen/namespace.json').read_text())
    results = build.measure_unit(unit, claims, refs, build.image(), namespace)
    write(f'build/gen/reports/{name}.json', results)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('stage', choices=['labels', 'model', 'delink', 'report'])
    parser.add_argument('unit', nargs='?')
    args = parser.parse_args()
    try:
        if args.stage == 'model':
            bind()
        elif not args.unit:
            parser.error('this stage requires a unit')
        else:
            {'labels': extract, 'delink': target, 'report': report}[args.stage](args.unit)
    except (ValueError, OSError) as exc:
        print(f'[graph] {exc}', file=sys.stderr)
        return 1
    return 0


if __name__ == '__main__':
    raise SystemExit(main())

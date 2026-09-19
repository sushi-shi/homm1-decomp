"""Incremental code reconstruction with strict integrity and observational scores."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import shlex
import shutil
import subprocess
import sys

from homm1 import toolchain, verify, analysis, model, delink, checkpoint, publication, reporting
from homm1.core import manifest
from homm1.core.coff import CoffObject
from homm1.core.compiler import compile_source
from homm1.core.image import Image
from homm1.core.inputs import REPO, read_verified, targets
from homm1.core.matching import compare, confirm_object, retail_relocations, source_claim, target_object


def units():
    config = manifest.load()
    result = config.get('unit', [])
    seen = set()
    for unit in result:
        if set(unit) != {'unit', 'source', 'flags'} or not re.fullmatch(r'[a-z0-9_]+', unit['unit']):
            raise ValueError('units require only unit/source/flags and a simple unique unit name')
        source = (REPO / unit['source']).resolve()
        if not source.is_relative_to(REPO / 'src') or not source.is_file():
            raise ValueError('unit source must be an existing file under src/')
        if unit['unit'] in seen or unit['flags'] not in config['flags']:
            raise ValueError('duplicate unit or unknown compiler profile')
        flags = config['flags'][unit['flags']]
        if not isinstance(flags, list) or any(not isinstance(f, str) for f in flags) or '/c' not in flags:
            raise ValueError('compiler profile must be a list of flags including /c')
        seen.add(unit['unit'])
    enrolled = {u['source'] for u in result}
    physical = {str(p.relative_to(REPO)) for p in (REPO / 'src').rglob('*') if p.suffix in ('.cpp', '.cc', '.cxx')}
    if enrolled != physical or len(enrolled) != len(result):
        raise ValueError(f'source enrollment differs from physical source files: {sorted(enrolled ^ physical)}')
    return config, result


def image():
    game = targets()['game']
    return Image(read_verified(game, game.destination))


def validate_claims(retail):
    claims, _ = model.resolve(retail)
    checkpoint.check_claims(claims, checkpoint.read())
    verify.check_reviews(claims)
    return claims


def compile_unit(name):
    config, entries = units()
    unit = next((u for u in entries if u['unit'] == name), None)
    if unit is None:
        raise ValueError(f'unknown unit {name}')
    from homm1.graph.cc import compile_unit as stable_compile
    stable_compile(REPO / unit['source'], REPO / f'build/objdiff/base/{name}.obj',
                   config['flags'][unit['flags']], config['build']['compiler'])


def configure():
    from homm1.graph.emit import emit
    return emit()


def write_report(path, report):
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(json.dumps(report, indent=2) + '\n')


def run(args):
    with publication.locked():
        return _run(args)


def _run(args):
    verify.check()
    retail = image()
    config, all_entries = units()
    selected = getattr(args, 'unit', None)
    entries = [u for u in all_entries if not selected or u['unit'] == selected]
    if not entries:
        raise ValueError(f'unknown unit {selected}')
    compiler = config['build']['compiler']
    toolchain.verify(compiler)
    generation = checkpoint.fingerprint()
    configure()
    semantic = verify.check_semantic(require_complete=not bool(selected))
    environment = {**os.environ, 'PYTHONPATH': str(REPO / 'scripts')}
    build_targets = [f'build/gen/reports/{u["unit"]}.json' for u in entries]
    subprocess.run(['ninja', '-f', 'build/build.ninja', *build_targets], cwd=REPO, env=environment, check=True)
    from homm1.graph.steps import read_claims
    claims, declarations, dependencies = read_claims()
    checkpoint.check_claims(claims, checkpoint.read())
    results, objdiff_units = [], []
    for unit in entries:
        results.extend(json.loads((REPO / f'build/gen/reports/{unit["unit"]}.json').read_text()))
        objdiff_units.append(dict(name=unit['unit'], target_path=f'target/{unit["unit"]}.obj',
                                  base_path=f'base/{unit["unit"]}.obj'))
    if not results or checkpoint.fingerprint() != generation:
        raise ValueError('empty campaign or inputs changed during the build; no checkpoint published')
    if any(not (REPO / path).exists() or toolchain.digest(REPO / path) != digest for path, digest in dependencies.items()):
        raise ValueError('include dependencies changed during the build; no checkpoint published')
    from homm1.normalized_freshness import freshness_problems
    for fn in results:
        for side, field in (('base', 'object_sha256'), ('target', 'target_object_sha256')):
            artifact = REPO / f'build/objdiff/{side}/{fn["unit"]}.obj'
            if toolchain.digest(artifact) != fn[field]:
                raise ValueError(f'unmeasured {side} object for {fn["unit"]}; rebuild its graph report')
            if side == 'target' and (problems := freshness_problems(artifact)):
                raise ValueError('\n'.join(problems))
    report = dict(target_sha256=hashlib.sha256(retail.data).hexdigest(), toolchain=compiler,
                  compiler_files=toolchain.pins()[compiler]['files'], flags=config['flags'],
                  scope='admitted fragments only; not whole-game coverage', functions=results,
                  fingerprint=generation, dependencies=dependencies, complete=not bool(selected), cleanliness=verify.check())
    report['cleanliness']['readability'] = verify.check_reviews(claims, require_complete=not bool(selected))
    report['cleanliness']['semantic_checks'] = semantic
    if selected:
        write_report(REPO / f'build/unit-reports/{selected}.json', report)
        print('Unit report only; full checkpoint unchanged.')
    else:
        previous = checkpoint.read()
        next_rows = checkpoint.advance(previous, results)
        for fn in results:
            old = previous.get(fn['rva'])
            if old and fn['score'] < old['cur']:
                print(f'{fn["symbol"]}: score decreased {old["cur"]:.2f} -> {fn["score"]:.2f} (observational)')
        ledger = checkpoint.serialize(next_rows)
        report['ledger_sha256'] = hashlib.sha256(ledger.encode()).hexdigest()
        readme = reporting.render((REPO / 'README.md').read_text(), report, next_rows)
        publication.publish({
            'build/objdiff/objdiff.json': json.dumps(dict(units=objdiff_units), indent=2) + '\n',
            'build/match-report.json': json.dumps(report, indent=2) + '\n',
            'config/match_baseline.tsv': ledger,
            'README.md': readme,
        })
    return 0


def probe(args):
    retail = image()
    validate_claims(retail)
    _, entries = units()
    # Historical /Od-vs-/O2 control is established only for this callback.
    entries = [u for u in entries if u['unit'] == 'app_about']
    reports = []
    for compiler in args.ids:
        toolchain.verify(compiler)
        for optimization in ('/Od', '/O2'):
            for unit in entries:
                output = REPO / f'build/probes/{compiler}/{optimization[1:]}/{unit["unit"]}.obj'
                source = REPO / unit['source']
                compile_source(source, output, ['/nologo', '/c', optimization], compiler)
                claim = source_claim(source, retail)
                result = compare(CoffObject(output.read_bytes()), retail, claim, retail_relocations(retail, claim))
                result.update(compiler=compiler, flags=['/nologo', '/c', optimization],
                              source_sha256=toolchain.digest(source), object_sha256=toolchain.digest(output))
                reports.append(result)
                print(f'{compiler} {optimization}: {"EXACT" if result["exact"] else "DIFF"}, '
                      f'{result["compiled_size"]} bytes', flush=True)
    write_report(REPO / 'build/probes/report.json', dict(
        target_sha256=hashlib.sha256(retail.data).hexdigest(), candidates=reports,
        compiler_files={key: toolchain.pins()[key]['files'] for key in args.ids}))
    # These are compiler controls: /Od must match and /O2 must not.
    return 0 if all(r['exact'] == ('/Od' in r['flags']) for r in reports) else 1


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('--compile', required=True)
    try:
        compile_unit(parser.parse_args().compile)
    except (ValueError, OSError, subprocess.SubprocessError) as exc:
        print(f'[homm1] ERROR: {exc}', file=sys.stderr)
        raise SystemExit(1)


def measure_unit(unit, owned, references, retail, namespace):
    """The existing retail/object integrity checks, as one graph report edge."""
    results = []
    base = REPO / f'build/objdiff/base/{unit["unit"]}.obj'
    target = REPO / f'build/objdiff/target/{unit["unit"]}.obj'
    candidate = CoffObject(base.read_bytes())
    independent = CoffObject(target.read_bytes())
    confirm_object(candidate, owned)
    confirm_object(independent, owned)
    for claim in owned:
        refs = references[claim.rva]
        oracle = compare(independent, retail, claim, refs, owned, namespace)
        if not oracle['exact']:
            raise ValueError(f'delinked target fails original-retail validation: {claim.symbol}: {oracle}')
        result = compare(candidate, retail, claim, refs, owned, namespace)
        result.update(unit=unit['unit'], source=unit['source'], src_hash=claim.src_hash,
                      source_sha256=toolchain.digest(REPO / unit['source']),
                      object_sha256=toolchain.digest(base), target_object_sha256=toolchain.digest(target))
        results.append(result)
        print(f'{unit["unit"]}/{claim.symbol}: {"EXACT" if result["exact"] else "DIFF"} '
              f'{result["matched_bytes"]}/{claim.size} bytes; '
              f'relocations {"exact" if result["relocations_exact"] else "differ"}', flush=True)
    report_path = REPO / f'build/objdiff/{unit["unit"]}.diff.json'
    subprocess.run(['objdiff-cli', 'diff', '-1', str(target), '-2', str(base), '-o', str(report_path)], check=True)
    diff = json.loads(report_path.read_text())
    scores = {s['name']: float(s.get('match_percent', 0)) for s in diff['right']['symbols']
              if s.get('kind') == 'SYMBOL_FUNCTION' and 'size' in s}
    for fn in results:
        if fn['unit'] == unit['unit']:
            if fn['symbol'] not in scores:
                raise ValueError(f'objdiff did not measure {fn["symbol"]}')
            fn['score'] = scores[fn['symbol']]
    return results

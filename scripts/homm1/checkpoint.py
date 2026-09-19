"""Observational CUR/MAX/HIST checkpoints, adapted from HoMM3 match/status.

Stable RVAs preserve history across name promotion. Function tokens distinguish
implementation changes from collateral movement. Only a full green run banks.
"""
import csv
import hashlib
import io
import json
import math
from pathlib import Path
import shutil
import sys

from homm1.core.inputs import REPO
from homm1 import toolchain

BASELINE = REPO / 'config/match_baseline.tsv'


def read(path=BASELINE):
    with path.open() as handle:
        rows = list(csv.DictReader((line for line in handle if not line.startswith('#')), delimiter='\t'))
    result = {}
    for row in rows:
        rva = int(row['rva'], 0)
        if rva in result:
            raise ValueError('duplicate checkpoint RVA')
        legacy = 'cur' not in row
        scores = [float(row[key]) for key in ('cur', 'max', 'hist')] if not legacy else [100.0] * 3
        if not all(math.isfinite(s) for s in scores) or not 0 <= scores[0] <= scores[1] <= scores[2] <= 100:
            raise ValueError('invalid CUR <= MAX <= HIST checkpoint')
        result[rva] = dict(row, rva=rva, size=int(row['size'], 0),
                           cur=scores[0], max=scores[1], hist=scores[2],
                           src_hash=row.get('src_hash', ''))
    return result


def check_claims(claims, previous):
    current = {c.rva: c for c in claims}
    for rva, row in previous.items():
        if rva not in current or current[rva].size != row['size']:
            raise ValueError(f'lost or resized checkpoint claim {rva:#x}; review retail evidence before changing the ledger')


def advance(previous, functions):
    result = {}
    for fn in functions:
        rva, cur, digest = fn['rva'], fn['score'], fn['src_hash']
        if rva in result or not digest or cur is None or not math.isfinite(cur) or not 0 <= cur <= 100:
            raise ValueError('duplicate or unmeasured function cannot enter checkpoint')
        old = previous.get(rva)
        if old and fn['retail_size'] != old['size']:
            raise ValueError('checkpoint extent changed')
        maximum = max(cur, old['max']) if old and old['src_hash'] == digest else cur
        historical = max(cur, old['hist']) if old else cur
        result[rva] = dict(rva=rva, size=fn['retail_size'], symbol=fn['symbol'], unit=fn['unit'],
                           cur=cur, max=maximum, hist=historical, src_hash=digest)
    if previous.keys() - result.keys():
        raise ValueError('partial run cannot update the full checkpoint')
    return result


def serialize(rows):
    stream = io.StringIO()
    stream.write('# Observational scores: CUR <= MAX(current source) <= HIST(all source). Full green builds only.\n')
    columns = ('rva', 'size', 'symbol', 'unit', 'cur', 'max', 'hist', 'src_hash')
    writer = csv.DictWriter(stream, fieldnames=columns, delimiter='\t', lineterminator='\n')
    writer.writeheader()
    for rva, row in sorted(rows.items()):
        writer.writerow({**row, 'rva': f'0x{rva:08X}', 'size': f'0x{row["size"]:X}'})
    return stream.getvalue()


def write(rows, path=BASELINE):
    from homm1.publication import atomic_write
    atomic_write(path, serialize(rows))


def fingerprint(root=REPO, tools=True):
    digest = hashlib.sha256()
    files = sorted(p for directory in ('src', 'include', 'scripts/homm1', 'config')
                   for p in (root / directory).rglob('*')
                   if p.is_file() and '__pycache__' not in p.parts and p.suffix != '.pyc'
                   and p.name != 'match_baseline.tsv')
    files += [root / name for name in ('flake.nix', 'flake.lock', 'homm1') if (root / name).exists()]
    for path in files:
        digest.update(str(path.relative_to(root)).encode() + b'\0' + path.read_bytes() + b'\0')
    if tools:
        for name in ('clang++', 'llvm-pdbutil', 'vostok-delinker', 'objdiff-cli', 'wine', 'winepath', 'ninja', sys.executable):
            path = shutil.which(name)
            if not path:
                raise ValueError(f'{name} required for a measured fresh report')
            digest.update(str(Path(path).resolve()).encode())
            digest.update(toolchain.digest(path).encode())
    return digest.hexdigest()


def fresh_report(path=None):
    from homm1.publication import locked
    with locked():
        return _fresh_report(path)


def _fresh_report(path=None):
    path = path or REPO / 'build/match-report.json'
    if not path.exists():
        raise ValueError('no measured report; run homm1 build')
    report = json.loads(path.read_text())
    if report.get('fingerprint') != fingerprint():
        raise ValueError('stale report: source, metadata or tools changed; run homm1 build')
    toolchain.verify(report['toolchain'])
    from homm1.build import image
    if hashlib.sha256(image().data).hexdigest() != report['target_sha256']:
        raise ValueError('report target changed')
    for name, expected in report.get('dependencies', {}).items():
        dependency = REPO / name
        if not dependency.exists() or toolchain.digest(dependency) != expected:
            raise ValueError(f'stale report dependency: {name}')
    for fn in report['functions']:
        for side, field in (('base', 'object_sha256'), ('target', 'target_object_sha256')):
            obj = REPO / f'build/objdiff/{side}/{fn["unit"]}.obj'
            if not obj.exists() or toolchain.digest(obj) != fn[field]:
                raise ValueError(f'stale or missing {side} object for {fn["unit"]}; run homm1 build')
            if side == 'target':
                from homm1.normalized_freshness import freshness_problems
                problems = freshness_problems(obj)
                if problems:
                    raise ValueError('\n'.join(problems))
    if report.get('complete'):
        rows = read()
        check_consistency(report, rows)
        from homm1.reporting import render
        readme = (REPO / 'README.md').read_text()
        if render(readme, report, rows) != readme:
            raise ValueError('README campaign block disagrees with the checkpoint; run homm1 build')
    return report


def check_consistency(report, rows):
    functions = report['functions']
    if len({f['rva'] for f in functions}) != len(functions) or {f['rva'] for f in functions} != rows.keys():
        raise ValueError('checkpoint/report claims differ')
    for fn in functions:
        row = rows[fn['rva']]
        if (row['size'], row['symbol'], row['unit'], row['cur'], row['src_hash']) != (fn['retail_size'], fn['symbol'], fn['unit'], fn['score'], fn['src_hash']):
            raise ValueError(f'checkpoint/report measurement differs at {fn["rva"]:#x}')
    if report.get('ledger_sha256') != hashlib.sha256(serialize(rows).encode()).hexdigest():
        raise ValueError('checkpoint/report history differs')


def command(args):
    report = fresh_report()
    rows = read()
    for fn in report['functions']:
        if args.view == 'queue' and fn['exact']:
            continue
        row = rows.get(fn['rva'])
        print(f'{fn["rva"]:#010x} {fn["unit"]}/{fn["symbol"]} '
              f'CUR {fn["score"]:.2f} MAX {row["max"] if row else "unbanked"} '
              f'HIST {row["hist"] if row else "unbanked"} '
              f'{"EXACT" if fn["exact"] else "DIFF"}')

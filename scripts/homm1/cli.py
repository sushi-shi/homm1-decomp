"""Small, target-specific front door to the reconstruction workspace."""
from __future__ import annotations

import argparse
from dataclasses import asdict
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys

from homm1.core.coff import CoffObject
from homm1.core.image import Image
from homm1.core import manifest
from homm1.core.inputs import REPO, read_verified, stage_executable, targets
from homm1 import build, toolchain, verify, probes, labels, model, delink, checkpoint, sema, publication


def verified_image(target):
    executable = targets()[target]
    return Image(read_verified(executable, executable.destination))


def emit(value):
    print(json.dumps(value, indent=2))


def initialize(args):
    pins = targets()
    stage_executable(pins['game'], args.exe)
    selected = ['game']
    editor = pins['editor']
    if args.editor_exe is not None or os.environ.get(editor.env_var) or editor.destination.exists():
        stage_executable(editor, args.editor_exe)
        selected.append('editor')
    output = REPO / 'build/analysis'
    output.mkdir(parents=True, exist_ok=True)
    for key in selected:
        report = verified_image(key).report()
        path = output / f'{key}.json'
        path.write_text(json.dumps(report, indent=2) + '\n')
        print(f'{key}: verified {pins[key].name}; report: {path.relative_to(REPO)}')
    manifest.check_retail(verified_image('game'))
    print('Analysis workspace ready. Use homm1 toolchain install and homm1 build for matching.')


def status(args):
    if args.view:
        return checkpoint.command(args)
    pins = targets()
    states = {}
    for key, pin in pins.items():
        if not pin.destination.exists():
            states[key] = 'not staged'
        else:
            read_verified(pin, pin.destination)
            states[key] = 'verified'
    config = manifest.load()
    functions = manifest.table('functions.tsv', ('rva', 'kind'))
    value = dict(inputs=states, located_functions=len(functions),
                 census_status='sparse; extents unknown',
                 admitted_units=len(config.get('unit', [])),
                 compiler=config['build']['compiler_status'],
                 matching_build='homm1 build (admitted fragments only)',
                 last_build_report='build/match-report.json' if (REPO / 'build/match-report.json').exists() else None)
    if args.json:
        emit(value)
    else:
        for key, val in value.items():
            print(f'{key}: {val}')


def inspect(args):
    report = verified_image(args.target).report()
    if args.json:
        emit(report)
        return
    print(f'{args.target}: sha256 {report["sha256"]}')
    print(f'base 0x{report["image_base"]:08X}, entry 0x{report["entry_va"]:08X}, '
          f'linker {report["linker"][0]}.{report["linker"][1]:02d}')
    for s in report['sections']:
        print(f'{s["name"]:8} RVA 0x{s["rva"]:08X} virtual {s["virtual_size"]:7} raw {s["raw_size"]:7}')
    for row in report['exports']:
        print(f'export {row["ordinal"]}: 0x{row["va"]:08X} {", ".join(row["names"])}')
    print('imports: ' + ', '.join(row['dll'] for row in report['imports']))
    print(f'base relocations: {len(report["relocations"])}')
    for name in sorted({row['path'] for row in report['source_paths']}):
        print(f'source: {name}')


def disasm(args):
    image = verified_image(args.target)
    if args.size <= 0:
        raise ValueError('--size must be positive')
    rva = args.va - image.image_base
    section = image.section_of(rva)
    if section is None or not section.executable or rva + args.size > section.rva + section.size:
        raise ValueError('requested range must lie within file-backed executable code')
    tool = shutil.which('objdump')
    if tool is None:
        raise ValueError('objdump is missing; enter nix develop')
    subprocess.run([tool, '-D', '-Mintel', f'--section={section.name}',
                    f'--start-address={args.va}', f'--stop-address={args.va + args.size}',
                    str(targets()[args.target].destination)], check=True)


def check(_args):
    image = verified_image('game')
    image.report()
    rows = manifest.check_retail(image)
    editor = targets()['editor']
    if editor.destination.exists():
        verified_image('editor').report()
    claims = build.validate_claims(image)
    print(f'Input hashes, PE reports, and {len(rows)} located functions validated.')
    print(f'{len(claims)} source claims and their retail relocation fields validated; run homm1 build to compile/compare.')


def object_info(args):
    obj = CoffObject(args.path.read_bytes())
    emit(dict(sections=[asdict(s) for s in obj.sections],
              symbols=[asdict(s) for s in obj.symbols.values()],
              relocations=[asdict(r) for r in obj.relocations]))


def main(argv=None):
    parser = argparse.ArgumentParser(prog='homm1', description=__doc__)
    commands = parser.add_subparsers(dest='command', required=True)
    p = commands.add_parser('init', help='verify inputs and generate binary evidence')
    p.add_argument('--exe', type=Path)
    p.add_argument('--editor-exe', type=Path)
    p.set_defaults(run=initialize)
    p = commands.add_parser('status', help='show inputs and reconstruction readiness')
    p.add_argument('view', nargs='?', choices=['functions', 'queue', 'check'])
    p.add_argument('--json', action='store_true')
    p.set_defaults(run=status)
    p = commands.add_parser('inspect', help='inspect the pinned PE image')
    p.add_argument('--target', choices=['game', 'editor'], default='game')
    p.add_argument('--json', action='store_true')
    p.set_defaults(run=inspect)
    p = commands.add_parser('disasm', help='disassemble a retail VA range')
    p.add_argument('va', type=lambda value: int(value, 0))
    p.add_argument('--size', type=lambda value: int(value, 0), default=64)
    p.add_argument('--target', choices=['game', 'editor'], default='game')
    p.set_defaults(run=disasm)
    p = commands.add_parser('check', help='validate inputs and the function catalogue')
    p.set_defaults(run=check)
    p = commands.add_parser('object', help='inspect an i386 COFF object as JSON')
    p.add_argument('path', type=Path)
    p.set_defaults(run=object_info)
    p = commands.add_parser('build', help='compile, carve retail targets, and compare bytes and relocations')
    p.add_argument('--unit', help='compile/compare a single unit without banking a checkpoint')
    p.set_defaults(run=build.run)
    commands.add_parser('configure', help='emit the incremental Ninja graph').set_defaults(run=lambda _: build.configure() and 0)
    from homm1 import correspondence
    p = commands.add_parser('reference', help='look up pinned Buka source correspondences')
    p.add_argument('query', nargs='?', help='HoMM1 RVA, VA, or recovered name; omit to list all')
    p.add_argument('--checkout', type=Path, help='verify source pins against this donor Git repository')
    p.add_argument('--json', action='store_true')
    p.set_defaults(run=correspondence.command)
    from homm1 import audit
    p = commands.add_parser('audit', help='Buka semantic casts and physical source inventory')
    p.add_argument('audit', choices=['casts', 'readability'])
    p.add_argument('arguments', nargs=argparse.REMAINDER)
    p.set_defaults(run=audit.command)
    p = commands.add_parser('verify', help='source cleanliness and evidence gates')
    p.add_argument('action', choices=['check', 'board'])
    p.add_argument('--tier', choices=['fast', 'normal', 'full'], default='fast')
    p.set_defaults(run=verify.command)
    commands.add_parser('compare', help='verify freshness and show current comparisons').set_defaults(run=lambda _: emit(checkpoint.fresh_report()))
    commands.add_parser('delink', help='generate independent retail objects').set_defaults(run=delink.command)
    commands.add_parser('labels', help='AST-bound source identities').set_defaults(run=labels.command)
    commands.add_parser('model', help='joined retail claims and reference model').set_defaults(run=model.command)
    p = commands.add_parser('sema', help='inspect matching evidence and first divergence')
    p.add_argument('action', choices=['rva', 'disasm', 'diff', 'xref', 'strings', 'source', 'frame', 'blocks', 'branches', 'callers', 'callees', 'find-string', 'reference'])
    p.add_argument('address', help='RVA, VA, or unambiguous source symbol')
    p.add_argument('--side', choices=['retail', 'compiled'], default='retail')
    p.add_argument('--size', type=lambda v: int(v, 0))
    p.add_argument('--diff', action='store_true', help='compare compiled/retail blocks or branches')
    p.add_argument('--lite', action='store_true', help='show block structure without instruction details')
    p.set_defaults(run=sema.command)
    p = commands.add_parser('probe', help='compare pinned compiler candidates with /Od and /O2 controls')
    p.add_argument('--ids', nargs='+', choices=sorted(toolchain.pins()), default=['vc40'])
    p.add_argument('--contracts', action='store_true', help='measure calling conventions, domains, classes and EH')
    p.set_defaults(run=lambda args: probes.run(args) if args.contracts else build.probe(args))
    p = commands.add_parser('toolchain', help='install or verify pinned compiler components')
    p.add_argument('action', choices=['install', 'check', 'symbols'])
    p.add_argument('--id', choices=sorted(toolchain.pins()), default='vc40')
    p.add_argument('--media', type=Path)
    p.set_defaults(run=toolchain.command)
    p = commands.add_parser('test', help='run portable tooling tests without retail files')
    p.set_defaults(run=lambda _: subprocess.run(
        [sys.executable, '-m', 'unittest', 'discover', '-s', str(REPO / 'tests'), '-v'],
        cwd=REPO, env={**os.environ, 'PYTHONPATH': str(REPO / 'scripts')}).returncode)
    args = parser.parse_args(argv)
    try:
        with publication.locked():
            return args.run(args) or 0
    except (OSError, ValueError, subprocess.SubprocessError) as exc:
        print(f'[homm1] ERROR: {exc}', file=sys.stderr)
        return 1

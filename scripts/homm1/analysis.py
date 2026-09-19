"""Separate retail-language and strict-domain Clang analysis; no source lowering.

The two language modes follow HoMM2's clang_options/clang_cxx11 convention.
Only the pinned MSVC compiler produces the objects that are scored.
"""
import json
from pathlib import Path
import shutil
import subprocess

from homm1.core.inputs import REPO
from homm1 import toolchain


def arguments(source, compiler='vc40', strict=False, flags=()):
    clang = shutil.which('clang++')
    if not clang:
        raise ValueError('Clang is required for source analysis; enter nix develop')
    derived = []
    iterator = iter(flags)
    for flag in iterator:
        if flag[:2] in ('/D', '/U', '/I'):
            value = flag[2:] or next(iterator, '')
            if not value:
                raise ValueError(f'missing compiler argument for {flag}')
            derived.append('-' + flag[1] + value)
        elif flag.startswith('/Zp'):
            derived.append('-fpack-struct=' + (flag[3:] or '8'))
        elif flag == '/J':
            derived.append('-funsigned-char')
        elif flag in ('/Gr', '/Gz', '/Gd'):
            derived += ['-Xclang', '-fdefault-calling-conv=' + {'/Gr': 'fastcall', '/Gz': 'stdcall', '/Gd': 'cdecl'}[flag]]
        elif flag in ('/MT', '/MTd', '/MD', '/MDd'):
            derived.append('-D_MT')
            if flag.startswith('/MD'):
                derived.append('-D_DLL')
    return [clang, '--target=i386-pc-windows-msvc', '-fms-extensions',
            '-fms-compatibility', '-fms-compatibility-version=10.00',
            '-std=c++20' if strict else '-std=c++98',
            '-Wno-ignored-attributes', '-Wno-writable-strings',
            '-Werror=enum-conversion', '-Werror=enum-compare',
            '-Werror=deprecated-enum-enum-conversion',
            '-I', str(REPO / 'include'), '-isystem', str(toolchain.root(compiler) / 'include'),
            *derived, str(Path(source).resolve())]


def run(source, compiler='vc40', strict=False, ast=False, flags=()):
    command = arguments(source, compiler, strict, flags) + ['-fsyntax-only']
    if ast:
        command += ['-Xclang', '-ast-dump=json']
    result = subprocess.run(command, cwd=REPO, capture_output=True, text=True, timeout=120)
    if result.returncode:
        raise ValueError(f'Clang {"strict" if strict else "retail"} analysis failed for {source}:\n{result.stderr}')
    return json.loads(result.stdout) if ast else result.stderr


def compilation_databases(config, units):
    for strict, directory in ((False, 'retail'), (True, 'strict')):
        path = REPO / 'build/analysis' / directory / 'compile_commands.json'
        path.parent.mkdir(parents=True, exist_ok=True)
        rows = [dict(directory=str(REPO), file=str(REPO / u['source']),
                     arguments=arguments(REPO / u['source'], config['build']['compiler'], strict, config['flags'][u['flags']])
                     + ['-fsyntax-only']) for u in units]
        path.write_text(json.dumps(rows, indent=2) + '\n')


def check(config, units):
    compilation_databases(config, units)
    for unit in units:
        flags = config['flags'][unit['flags']]
        run(REPO / unit['source'], config['build']['compiler'], flags=flags)
        run(REPO / unit['source'], config['build']['compiler'], strict=True, flags=flags)
    return dict(retail_units=len(units), strict_units=len(units))

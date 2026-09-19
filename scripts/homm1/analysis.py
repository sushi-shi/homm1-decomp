"""Separate retail-language and strict-domain Clang analysis; no source lowering.

The two language modes follow HoMM2's clang_options/clang_cxx11 convention.
Only the pinned MSVC compiler produces the objects that are scored.
"""
import json
from pathlib import Path
import shutil
import shlex
import subprocess

from homm1.core.inputs import REPO
from homm1 import toolchain
from homm1.core.profile import clang_flags, VERSIONS


def arguments(source, compiler='vc40', strict=False, flags=()):
    clang = shutil.which('clang++')
    if not clang:
        raise ValueError('Clang is required for source analysis; enter nix develop')
    if compiler not in VERSIONS:
        raise ValueError(f'unsupported analysis compiler {compiler}')
    derived = clang_flags(flags)
    return [clang, '--target=i386-pc-windows-msvc', '-fms-extensions',
            '-fms-compatibility', '-fms-compatibility-version=' + VERSIONS[compiler],
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


def dependencies(source, compiler='vc40', flags=()):
    """Compiler-discovered includes, including forced and system headers."""
    command = arguments(source, compiler, flags=flags) + ['-M', '-MT', 'homm1']
    result = subprocess.run(command, cwd=REPO, capture_output=True, text=True, timeout=120)
    if result.returncode:
        raise ValueError(f'include discovery failed for {source}:\n{result.stderr}')
    payload = result.stdout.replace('\\\n', '').partition(':')[2]
    return sorted({Path(p).resolve() for p in shlex.split(payload)})


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

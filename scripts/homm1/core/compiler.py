"""Stateless Wine invocation, with verified inputs and no stale-object success."""
import os
from pathlib import Path
import shutil
import struct
import subprocess

from homm1.core.inputs import REPO
from homm1 import toolchain
from homm1.core.profile import parse
from homm1.core.wine import run_hang_proof


def wine_env():
    environment = dict(os.environ)
    environment.update(WINEPREFIX=str(REPO / 'build/wineprefix'), WINEDEBUG='-all',
                       WINEDLLOVERRIDES='mscoree,mshtml=;msvcrt20,msvcrt40=n', DISPLAY='')
    # CL and _CL_ can inject options before/after the declared profile.
    for name in ('CL', '_CL_', 'INCLUDE', 'LIB'):
        environment.pop(name, None)
    return environment


def windows_path(path, environment):
    return subprocess.check_output(['winepath', '-w', str(Path(path).resolve())],
                                   env=environment, text=True, stderr=subprocess.PIPE,
                                   timeout=60).strip()


def compile_source(source, output, flags, name):
    profile = parse(flags)
    compiler_root = toolchain.verify(name)
    if not shutil.which('wine') or not shutil.which('winepath'):
        raise ValueError('Wine is required; enter nix develop .#build')
    output = Path(output).resolve()
    output.parent.mkdir(parents=True, exist_ok=True)
    output.unlink(missing_ok=True)
    environment = wine_env()
    include = [REPO / 'include']
    if (compiler_root / 'include').is_dir():
        include.append(compiler_root / 'include')
    environment['INCLUDE'] = ';'.join(windows_path(p, environment) for p in include)
    if (compiler_root / 'lib').is_dir():
        environment['LIB'] = windows_path(compiler_root / 'lib', environment)
    native_flags = [flag + (windows_path(value, environment) if flag in ('/I', '/FI') else value or '')
                    for flag, value in profile]
    command = ['wine', str(compiler_root / 'bin/CL.EXE'), *native_flags,
               '/Fo' + windows_path(output, environment), windows_path(source, environment)]
    log = output.with_suffix('.compile.log')
    message, status, timed_out = run_hang_proof(command, output, cwd=output.parent,
                                              timeout=120, environment=environment)
    log.write_text(message)
    if timed_out:
        output.unlink(missing_ok=True)
        raise ValueError(f'compiler timed out; see {log}')
    if status or not output.is_file():
        output.unlink(missing_ok=True)
        raise ValueError(f'compiler failed; see {log}:\n{log.read_text(errors="replace")[-3000:]}')
    # COFF timestamp is not code or debugging evidence; make repeated builds stable.
    payload = bytearray(output.read_bytes())
    struct.pack_into('<I', payload, 4, 0)
    output.write_bytes(payload)
    return output

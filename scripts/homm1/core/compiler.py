"""Stateless Wine invocation, with verified inputs and no stale-object success."""
import os
from pathlib import Path
import shutil
import subprocess

from homm1.core.inputs import REPO
from homm1 import toolchain


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
    compiler_root = toolchain.verify(name)
    if not shutil.which('wine') or not shutil.which('winepath'):
        raise ValueError('Wine is required; enter nix develop .#build')
    output = Path(output).resolve()
    output.parent.mkdir(parents=True, exist_ok=True)
    output.unlink(missing_ok=True)
    environment = wine_env()
    environment['INCLUDE'] = windows_path(REPO / 'include', environment)
    command = ['wine', str(compiler_root / 'bin/CL.EXE'), *flags,
               '/Fo' + windows_path(output, environment), windows_path(source, environment)]
    log = output.with_suffix('.compile.log')
    try:
        with log.open('wb') as handle:
            result = subprocess.run(command, cwd=output.parent, env=environment,
                                    stdin=subprocess.DEVNULL, stdout=handle,
                                    stderr=subprocess.STDOUT, timeout=120)
    except subprocess.TimeoutExpired:
        output.unlink(missing_ok=True)
        raise ValueError(f'compiler timed out; see {log}') from None
    if result.returncode or not output.is_file():
        output.unlink(missing_ok=True)
        raise ValueError(f'compiler failed; see {log}:\n{log.read_text(errors="replace")[-3000:]}')
    return output

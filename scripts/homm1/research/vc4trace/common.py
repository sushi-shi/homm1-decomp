"""Shared paths and the separate C1XX/C2 invocation used by every vc4trace tool."""
from __future__ import annotations

import os
from pathlib import Path
import shutil
import subprocess

from homm1 import toolchain
from homm1.core.compiler import wine_env, windows_path
from homm1.core.inputs import REPO

HERE = Path(__file__).resolve().parent
WORK = REPO / 'build/research/vc4trace'
M32 = 0xffffffff

# The C1XX/C2 command lines `CL /Bd` prints for the unit profiles in config/units.toml.
C1_FLAGS = {
    'od': ['-Gs', '-Ot', '-Z7', '-D_M_IX86=500', '-G4', '-Ob1'],
    'odeh': ['-Gs', '-Ot', '-Z7', '-D_M_IX86=500', '-G4', '-Ob1', '-GX'],
    'o2': ['-Gs', '-Gf', '-Og', '-Oi', '-Ot', '-Oy', '-Ob1', '-Z7', '-D_M_IX86=500', '-G4'],
    'o2inline': ['-Gs', '-Gf', '-Og', '-Oi', '-Ot', '-Oy', '-Ob2', '-Z7', '-D_M_IX86=500', '-G4'],
}
C2_FLAGS = {
    'od': ['-Gs4096', '-Zi', '-G5', '-noblend'],
    'odeh': ['-Gs4096', '-Zi', '-G5', '-noblend'],
    'o2': ['-Gs4096', '-Gy', '-Zi', '-G5', '-noblend'],
    'o2inline': ['-Gs4096', '-Gy', '-Zi', '-G5', '-noblend'],
}
PROFILE_MODE = {'cpp_od': 'od', 'cpp_carcass': 'od', 'cpp_carcass_gy': 'od', 'cpp_carcass_oi': 'od',
                'cpp_carcass_oi_gy': 'od', 'cpp_carcass_eh': 'odeh', 'cpp_o2': 'o2',
                'cpp_o2_inline': 'o2inline', 'cpp_codec': 'o2inline'}


def vc_bin() -> Path:
    return toolchain.verify('vc40') / 'bin'


def c2_exe() -> Path:
    return vc_bin() / 'C2.EXE'


def k4(v: int) -> int:
    """C2 sortnode's 4-bit symbol hash (0x408588) of a C1 handle."""
    v &= M32
    a = v >> 8
    e = ((a - v) & M32) >> 4
    return ((e - a + v) & M32) & 0xf


def unit_info(unit: str) -> tuple[Path, str]:
    """(source path, vc4trace mode) of a config/units.toml unit such as SOURCE/ARMY."""
    import tomllib
    cfg = tomllib.loads((REPO / 'config/units.toml').read_text())
    for row in cfg.get('unit', []):
        if row.get('unit') == unit:
            return REPO / row['source'], PROFILE_MODE[row.get('flags', 'cpp_carcass')]
    raise SystemExit(f'unknown unit {unit}')


def compile_separately(source: Path, mode: str, *, c2: Path | None = None,
                       out_dir: Path | None = None, log: Path | None = None) -> tuple[Path, Path, str]:
    """Run C1XX then C2 (optionally a patched C2) on SOURCE; keep the IL.

    Returns (object path, IL prefix, C2 stdout+stderr). The IL prefix plus
    sy/gl/ex/db/in names C1's streams."""
    source = Path(source).resolve()
    out_dir = Path(out_dir or WORK / 'tu').resolve()
    (out_dir / 'il').mkdir(parents=True, exist_ok=True)
    env = wine_env()
    env['INCLUDE'] = ';'.join(windows_path(p, env) for p in (REPO / 'include', vc_bin().parent / 'include'))
    stem = source.stem
    il = out_dir / 'il' / stem
    obj = out_dir / f'{stem}.obj'
    obj.unlink(missing_ok=True)
    vb = vc_bin()
    common = ['-il', windows_path(il, env), '-f', windows_path(source, env), '-W', '1']
    c1 = ['wine', str(vb / 'C1XX.EXE'), '-ef', windows_path(vb / 'C1.ERR', env), *common,
          '-Ze', '-Zp8', '-ZB64', '-D_INTEGRAL_MAX_BITS=64', '-Fo' + windows_path(obj, env),
          '-pc', '\\:/', '-D_MSC_VER=1000', '-D_WIN32', '-nologo', *C1_FLAGS[mode],
          '-I', windows_path(REPO / 'include', env), '-I', windows_path(vb.parent / 'include', env)]
    r = subprocess.run(c1, cwd=out_dir, env=env, capture_output=True, text=True, errors='replace', timeout=300)
    if r.returncode:
        raise SystemExit(f'C1XX failed on {source.name}:\n{(r.stdout + r.stderr)[-2000:]}')
    exe = Path(c2 or vb / 'C2.EXE')
    if exe.parent != vb:   # a patched copy needs the runtime DLLs and message file beside it
        for name in ('MSVCRT40.DLL', 'MSPDB40.DLL', 'C23.ERR'):
            if (vb / name).exists() and not (exe.parent / name).exists():
                shutil.copy2(vb / name, exe.parent / name)
    c2cmd = ['wine', str(exe), '-ef', windows_path(vb / 'C23.ERR', env), *common, '-dos',
             '-Fo' + windows_path(obj, env), '-ML', *C2_FLAGS[mode]]
    with (open(log, 'w') if log else open(os.devnull, 'w')) as handle:
        r = subprocess.run(c2cmd, cwd=out_dir, env=env, stdout=handle, stderr=subprocess.STDOUT, timeout=600)
    if r.returncode or not obj.is_file():
        raise SystemExit(f'C2 failed on {source.name} (rc {r.returncode}); log {log}')
    return obj, il, ''

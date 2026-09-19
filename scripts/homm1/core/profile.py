"""Explicit VC2/VC4 source/analysis flag contract.

Unknown flags fail before either compiler runs. Paths are relative to the
repository, not to the output directory. Code generation stays in MSVC.
"""
import re
from pathlib import Path

from homm1.core.inputs import REPO

VERSIONS = {'vc20': '9.00', 'vc22': '9.00', 'vc40': '10.00'}
CODEGEN = re.compile(r'/(?:nologo|c|Od|O1|O2|Ox|Os|Ot|Oi-?|Oy-?|Ob[012]|Op-?|Og-?|Gs\d*|G[3456]|Gy-?|GF|Z7|Zi|W[0-4]|WX-?|Y-)\Z')


def parse(flags, root=REPO):
    result = []
    iterator = iter(flags)
    for flag in iterator:
        prefix = next((p for p in ('/FI', '/D', '/U', '/I') if flag.startswith(p)), None)
        if prefix:
            value = flag[len(prefix):] or next(iterator, '')
            if not value or value.startswith('/') and prefix in ('/D', '/U'):
                raise ValueError(f'missing compiler argument for {prefix}')
            if prefix in ('/I', '/FI'):
                value = str((Path(root) / value).resolve())
            result.append((prefix, value))
        elif CODEGEN.fullmatch(flag) or flag in ('/J', '/Gr', '/Gz', '/Gd', '/ML', '/MLd', '/MT', '/MTd', '/MD', '/MDd', '/GX', '/GX-') or re.fullmatch(r'/Zp(?:1|2|4|8|16)?', flag):
            result.append((flag, None))
        else:
            raise ValueError(f'unsupported compiler flag {flag!r}; extend and test the analysis/MSVC contract first')
    return result


def clang_flags(flags, root=REPO):
    result = []
    for flag, value in parse(flags, root):
        if flag in ('/D', '/U'):
            result.append('-' + flag[1] + value)
        elif flag in ('/I', '/FI'):
            result += ['-I' if flag == '/I' else '-include', value]
        elif flag.startswith('/Zp'):
            result.append('-fpack-struct=' + (flag[3:] or '8'))
        elif flag == '/J':
            result.append('-funsigned-char')
        elif flag in ('/Gr', '/Gz', '/Gd'):
            result += ['-Xclang', '-fdefault-calling-conv=' + {'/Gr': 'fastcall', '/Gz': 'stdcall', '/Gd': 'cdecl'}[flag]]
        elif flag in ('/MLd', '/MTd', '/MDd'):
            result.append('-D_DEBUG')
        if flag in ('/MT', '/MTd', '/MD', '/MDd'):
            result.append('-D_MT')
        if flag in ('/MD', '/MDd'):
            result.append('-D_DLL')
        if flag in ('/GX', '/GX-'):
            result += ['-fexceptions', '-fcxx-exceptions', '-D_CPPUNWIND'] if flag == '/GX' else ['-fno-exceptions', '-U_CPPUNWIND']
    return result

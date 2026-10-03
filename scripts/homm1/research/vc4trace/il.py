"""C1 IL symbol streams: function handles (gl) and per-function parameter/local handles (sy)."""
from __future__ import annotations

import re
from pathlib import Path

_GL = re.compile(rb'(..)\x00(\?[\x21-\x7e]+)\x00', re.S)
_REC = re.compile(rb'\x01([\x01\x02])(..)\x00([A-Za-z_][A-Za-z0-9_]*)\x00', re.S)
_FN = re.compile(rb'\x03\x01(..)\x1f\x00', re.S)


def globals_by_handle(prefix: Path | str) -> dict[int, str]:
    """{C1 handle: decorated name} from the gl stream."""
    out = {}
    for m in _GL.finditer(Path(str(prefix) + 'gl').read_bytes()):
        out.setdefault(int.from_bytes(m.group(1), 'little'), m.group(2).decode())
    return out


def function_handle(prefix: Path | str, name: str) -> tuple[int, str]:
    """(handle, decorated name) of the function whose decorated name starts with ?NAME@."""
    for h, dec in globals_by_handle(prefix).items():
        if dec.startswith('?' + name + '@') or dec == name:
            return h, dec
    raise SystemExit(f'no function {name} in {prefix}gl')


def blocks(prefix: Path | str) -> list[list[tuple[int, str, str]]]:
    """Per-function [(handle, name, 'param'|'local')] in sy-stream (= /Od slot) order."""
    sy = Path(str(prefix) + 'sy').read_bytes()
    starts = [m.start() for m in _FN.finditer(sy)] + [len(sy)]
    return [[(int.from_bytes(m.group(2), 'little'), m.group(3).decode(),
              'param' if m.group(1) == b'\x01' else 'local') for m in _REC.finditer(sy, a, b)]
            for a, b in zip(starts, starts[1:])]


def locals_of(prefix: Path | str, leaf_handles: set[int]) -> list[tuple[int, str, str]]:
    """The sy block sharing most handles with LEAF_HANDLES (a function's traced leaves)."""
    best = None
    for recs in blocks(prefix):
        score = len(leaf_handles & {h for h, _n, _k in recs})
        if recs and (best is None or score > best[0]):
            best = (score, recs)
    return best[1] if best else []

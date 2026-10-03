"""Compile one source with a traced C2.EXE.

    python3 -m homm1.research.vc4trace.trace (UNIT | --source SRC --mode MODE) [--spec SPEC] [--out DIR]

UNIT is a config/units.toml unit (SOURCE/ARMY); its profile picks the C1/C2
flags.  SPEC defaults to sort.spec (the sortnode tracer).  Writes
<out>/<stem>.trace, <stem>.obj and the IL streams under <out>/il/.
"""
from __future__ import annotations

import argparse
from pathlib import Path

from homm1.core.usage import logged
from homm1.research.vc4trace import common, tracer


def run(source: Path, mode: str, spec: Path | None = None, out_dir: Path | None = None) -> Path:
    out_dir = Path(out_dir or common.WORK / 'tu')
    out_dir.mkdir(parents=True, exist_ok=True)
    spec = Path(spec or common.HERE / 'sort.spec')
    exe = common.WORK / f'C2T-{spec.stem}.EXE'
    if not exe.exists() or exe.stat().st_mtime < spec.stat().st_mtime:
        exe.parent.mkdir(parents=True, exist_ok=True)
        tracer.patch(spec, exe)
    log = out_dir / f'{Path(source).stem}.trace'
    common.compile_separately(source, mode, c2=exe, out_dir=out_dir, log=log)
    return log


@logged
def main(argv: list[str] | None = None) -> int:
    ap = argparse.ArgumentParser(prog='python3 -m homm1.research.vc4trace.trace', description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument('unit', nargs='?')
    ap.add_argument('--source'); ap.add_argument('--mode', choices=sorted(common.C1_FLAGS))
    ap.add_argument('--spec'); ap.add_argument('--out')
    a = ap.parse_args(argv)
    if a.unit:
        src, mode = common.unit_info(a.unit)
    else:
        src, mode = Path(a.source), a.mode or 'od'
    log = run(src, a.mode or mode, a.spec, a.out)
    print(f'{log} ({sum(1 for _ in open(log, errors="replace"))} lines)')
    return 0


if __name__ == '__main__':
    raise SystemExit(main())

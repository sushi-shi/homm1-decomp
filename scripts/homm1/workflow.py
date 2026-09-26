"""Repository workflow integration: Git hooks and staged source formatting."""
from __future__ import annotations

import argparse
from pathlib import Path
import shutil
import subprocess
import sys

from homm1.core.paths import REPO
from homm1.core.usage import logged


def format_staged(root: Path = REPO) -> int:
    formatter = shutil.which('clang-format')
    if formatter is None:
        print('clang-format unavailable; enter nix develop .#build', file=sys.stderr)
        return 1
    staged = subprocess.check_output(
        ['git', 'diff', '--cached', '--name-only', '--diff-filter=ACMR', '-z'], cwd=root)
    paths = [p.decode() for p in staged.split(b'\0') if p and
             Path(p.decode()).parts[0] in ('src', 'include') and
             Path(p.decode()).suffix in ('.c', '.cpp', '.h', '.hpp', '.cc', '.hh', '.cxx')]
    for path in paths:
        if subprocess.run(['git', 'diff', '--quiet', '--', path], cwd=root).returncode:
            print(f'pre-commit: {path} is partially staged; format and stage it explicitly',
                  file=sys.stderr)
            return 1
    if paths:
        subprocess.run([formatter, '--style=file', '-i', '--', *paths], cwd=root, check=True)
        subprocess.run(['git', 'add', '--', *paths], cwd=root, check=True)
    return 0


def setup(root: Path = REPO) -> int:
    settings = {
        'core.hooksPath': '.githooks',
        'merge.units.name': 'units.toml [[unit]] merge',
        'merge.units.driver': 'sh scripts/merge-units.sh %O %A %B',
    }
    old_hook = subprocess.run(['git', 'config', '--local', '--get', 'core.hooksPath'],
                              cwd=root, capture_output=True, text=True).stdout.strip()
    if old_hook and old_hook != '.githooks':
        print(f'existing hooksPath {old_hook!r}: integrate .githooks/pre-commit there first',
              file=sys.stderr)
        return 1
    for key, value in settings.items():
        subprocess.run(['git', 'config', '--local', key, value], cwd=root, check=True)
    print('Repository hooks and unit-manifest merge driver installed.')
    return 0


@logged
def main(argv=None) -> int:
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('action', choices=('setup', 'format-staged'))
    args = ap.parse_args(argv)
    return setup() if args.action == 'setup' else format_staged()


if __name__ == '__main__':
    raise SystemExit(main())

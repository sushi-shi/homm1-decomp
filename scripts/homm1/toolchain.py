"""Provision and verify compiler files from hash-pinned original media."""
import hashlib
import json
import os
from pathlib import Path
import shutil
import subprocess
import tempfile

from homm1.core.inputs import REPO


def pins():
    return json.loads((REPO / 'config/toolchains.json').read_text())


def digest(path):
    with Path(path).open('rb') as handle:
        return hashlib.file_digest(handle, 'sha256').hexdigest()


def root(name):
    return REPO / 'build/toolchains' / name


def verify(name, directory=None):
    config = pins()[name]
    directory = directory or root(name)
    for relative, expected in config['files'].items():
        path = directory / relative
        if not path.is_file() or digest(path) != expected['sha256']:
            raise ValueError(f'{name}: missing or changed {relative}; run homm1 toolchain install --id {name} --media PATH')
    return directory


def install(name, media):
    config = pins()[name]
    if digest(media) != config['media']['sha256']:
        raise ValueError(f'{name}: media SHA-256 differs from the pin')
    sevenzip = shutil.which('7z') or shutil.which('7zz')
    if not sevenzip:
        raise ValueError('7z is required to extract compiler media; enter nix develop .#build')
    destination = root(name)
    destination.parent.mkdir(parents=True, exist_ok=True)
    # Extract only known compiler components; installation media is never executed.
    with tempfile.TemporaryDirectory(prefix=f'.{name}-', dir=destination.parent) as scratch:
        scratch = Path(scratch)
        extraction = scratch / 'media'
        subprocess.run([sevenzip, 'x', '-y', f'-o{extraction}', str(media.resolve()),
                        *[entry['media_path'] for entry in config['files'].values()]],
                       check=True, stdout=subprocess.DEVNULL)
        staged = scratch / 'toolchain'
        for relative, entry in config['files'].items():
            target = staged / relative
            target.parent.mkdir(parents=True, exist_ok=True)
            shutil.copyfile(extraction / entry['media_path'], target)
        verify(name, staged)
        if destination.exists():
            # Repair individual files atomically; never remove unrelated files.
            for relative in config['files']:
                target = destination / relative
                target.parent.mkdir(parents=True, exist_ok=True)
                os.replace(staged / relative, target)
        else:
            staged.rename(destination)
    print(f'{name}: compiler files verified and installed in {destination.relative_to(REPO)}')


def command(args):
    if args.action == 'install':
        if args.media is None:
            raise ValueError('toolchain install requires --media PATH')
        install(args.id, args.media)
    else:
        verify(args.id)
        print(f'{args.id}: all pinned compiler files verified')

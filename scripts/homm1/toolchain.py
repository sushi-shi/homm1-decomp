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
    elif args.action == 'symbols':
        index = library_symbols(args.id)
        print(f'{args.id}: indexed {len(index)} external symbols from verified libraries')
    else:
        verify(args.id)
        print(f'{args.id}: all pinned compiler files verified')


def library_symbols(name):
    """Library membership is evidence of provenance, never a guessed retail RVA."""
    directory = verify(name)
    libraries = {key: value for key, value in pins()[name]['files'].items()
                 if key.startswith('lib/') and key.endswith('.lib') and key.count('/') == 1}
    identity = hashlib.sha256(json.dumps(libraries, sort_keys=True).encode()).hexdigest()
    output = REPO / f'build/analysis/{name}-library-symbols.json'
    if output.exists():
        cached = json.loads(output.read_text())
        if cached['fingerprint'] == identity:
            return cached['symbols']
    if not shutil.which('llvm-nm'):
        raise ValueError('llvm-nm required to classify verified SDK/CRT symbols')
    symbols = {}
    for relative in sorted(libraries):
        # Some VC4 ar headers NUL-pad numeric fields. Modern LLVM requires
        # spaces; normalize that metadata in an ignored analysis copy only.
        archive = bytearray((directory / relative).read_bytes())
        if not archive.startswith(b'!<arch>\n'):
            raise ValueError(f'unsupported pinned library container: {relative}')
        position = 8
        while position < len(archive):
            if position + 60 > len(archive) or archive[position + 58:position + 60] != b'`\n':
                raise ValueError(f'invalid archive header: {relative}')
            size = int(archive[position + 48:position + 58].strip(b' \0'))
            archive[position + 16:position + 58] = archive[position + 16:position + 58].replace(b'\0', b' ')
            position += 60 + size + size % 2
        normalized = REPO / 'build/analysis/libraries' / name / Path(relative).name
        normalized.parent.mkdir(parents=True, exist_ok=True)
        normalized.write_bytes(archive)
        result = subprocess.run(['llvm-nm', '--defined-only', '--extern-only', '--format=posix',
                                 str(normalized)], capture_output=True, text=True, encoding="latin-1", timeout=120)
        if result.returncode:
            raise ValueError(f'cannot index pinned library {relative}: {result.stderr[-1000:]}')
        member = ''
        for line in result.stdout.splitlines():
            if line.endswith(':'):
                member = line[:-1]
            else:
                fields = line.split()
                if len(fields) >= 2 and len(fields[1]) == 1:
                    symbols.setdefault(fields[0], []).append(dict(library=relative, member=member))
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text(json.dumps(dict(fingerprint=identity, symbols=symbols), indent=2) + '\n')
    return symbols

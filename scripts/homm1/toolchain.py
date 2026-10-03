"""Provision and verify compiler and vendor-SDK files from hash-pinned original media."""
import hashlib
import json
import os
from pathlib import Path
import shutil
import subprocess
import tempfile
import urllib.request

from homm1.core.inputs import REPO


RELEASE_REPOSITORY = "sushi-shi/homm1-decomp"
RELEASE_TAG = "toolchain-vc40-masm611-sdk1"
RELEASE_ASSET = "homm1-toolchain-vc40-masm611-sdk1.tar.xz"
RELEASE_SHA256 = "eb582d9a293cd0d666ea56eb937b6b8c0891da231bce1c5566e6450f15c4e9a5"
#: VC4 + MASM, and the vendor SDK files pinned in config/toolchains.json
#: (each extracted from its original media; `install --id <sdk> --media`
#: rebuilds any of them from archive.org).
RELEASE_COMPONENTS = ("vc40", "wing10", "dx1")


def pins():
    return json.loads((REPO / 'config/toolchains.json').read_text())


def digest(path):
    with Path(path).open('rb') as handle:
        return hashlib.file_digest(handle, 'sha256').hexdigest()


def root(name):
    return REPO / 'build/toolchains' / name


def _entries(config, *, release=True):
    entries = dict(config['files'])
    if release:
        entries.update(config.get('release_files', {}))
    return entries


def _verify_entries(name, directory, entries):
    for relative, expected in entries.items():
        path = directory / relative
        if not path.is_file() or digest(path) != expected['sha256']:
            raise ValueError(f'{name}: missing or changed {relative}; run homm1 toolchain install')
    return directory


def verify(name, directory=None):
    config = pins()[name]
    directory = directory or root(name)
    return _verify_entries(name, directory, _entries(config))


def resource_entries(name):
    """The resource compiler pair (RC.EXE + RCDLL.DLL) and CVTRES.EXE, which
    LINK runs on a .res input. They come from the same pinned media but are
    not part of the compiler release bundle, so the compiler verification
    does not require them."""
    return dict(pins()[name].get('resource_files', {}))


def verify_resources(name, directory=None):
    entries = resource_entries(name)
    if not entries:
        raise ValueError(f'{name}: no resource compiler is pinned')
    return _verify_entries(name, directory or root(name), entries)


def resources_installed(name, directory=None):
    """True when every pinned resource tool is present with its pinned hash."""
    try:
        verify_resources(name, directory)
    except (KeyError, ValueError):
        return False
    return True


def install(name, media):
    config = pins()[name]
    if digest(media) != config['media']['sha256']:
        raise ValueError(f'{name}: media SHA-256 differs from the pin')
    sevenzip = shutil.which('7z') or shutil.which('7zz')
    if not sevenzip:
        raise ValueError('7z is required to extract compiler media; enter nix develop .#build')
    destination = root(name)
    destination.parent.mkdir(parents=True, exist_ok=True)
    files = {**config['files'], **config.get('resource_files', {})}
    # Extract only known compiler components; installation media is never executed.
    with tempfile.TemporaryDirectory(prefix=f'.{name}-', dir=destination.parent) as scratch:
        scratch = Path(scratch)
        extraction = scratch / 'media'
        subprocess.run([sevenzip, 'x', '-y', f'-o{extraction}', str(media.resolve()),
                        *[entry['media_path'] for entry in files.values()]],
                       check=True, stdout=subprocess.DEVNULL)
        staged = scratch / 'toolchain'
        for relative, entry in files.items():
            target = staged / relative
            target.parent.mkdir(parents=True, exist_ok=True)
            source = extraction / entry['media_path']
            if entry.get('expand') == 'szdd':
                # Setup disks ship MS COMPRESS (SZDD) members (`WING.H_`);
                # 7z expands them to the file the vendor's SETUP installed.
                expanded = scratch / 'expanded' / relative
                subprocess.run([sevenzip, 'x', '-y', f'-o{expanded}', str(source)],
                               check=True, stdout=subprocess.DEVNULL)
                (source,) = [p for p in expanded.iterdir() if p.is_file()]
            elif 'expand' in entry:
                raise ValueError(f"{name}: unknown expansion {entry['expand']!r} for {relative}")
            shutil.copyfile(source, target)
        _verify_entries(name, staged, files)
        if destination.exists():
            # Repair individual files atomically; never remove unrelated files.
            for relative in files:
                target = destination / relative
                target.parent.mkdir(parents=True, exist_ok=True)
                os.replace(staged / relative, target)
        else:
            staged.rename(destination)
    suffix = ("; the release bundle also supplies MASM"
              if config.get('release_files') else "")
    print(f'{name}: media files verified and installed in '
          f'{destination.relative_to(REPO)}{suffix}')


def _release_url():
    return (f"https://github.com/{RELEASE_REPOSITORY}/releases/download/"
            f"{RELEASE_TAG}/{RELEASE_ASSET}")


def _verify_archive(path):
    if len(RELEASE_SHA256) != 64:
        raise ValueError('toolchain release hash is not pinned yet')
    actual = digest(path)
    if actual != RELEASE_SHA256:
        raise ValueError(f'toolchain release SHA-256 {actual} differs from the pin')


def _download_release(directory):
    archive = directory / RELEASE_ASSET
    try:
        with urllib.request.urlopen(_release_url()) as response, archive.open('wb') as output:
            shutil.copyfileobj(response, output)
    except Exception as error:
        archive.unlink(missing_ok=True)
        raise ValueError(f'cannot download {_release_url()}: {error}') from error
    _verify_archive(archive)
    return archive


def install_release(archive=None):
    """Install the hash-pinned VC4 + MASM release atomically."""
    parent = REPO / 'build/toolchains'
    parent.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(prefix='.release-', dir=parent) as scratch_name:
        scratch = Path(scratch_name)
        if archive is None:
            archive = _download_release(scratch)
        else:
            archive = Path(archive).resolve()
            _verify_archive(archive)
        extraction = scratch / 'extract'
        extraction.mkdir()
        subprocess.run(['tar', 'xf', str(archive), '-C', str(extraction)], check=True)
        staged_root = extraction / 'toolchains'
        for name in RELEASE_COMPONENTS:
            verify(name, staged_root / name)
        for name in RELEASE_COMPONENTS:
            source = staged_root / name
            destination = root(name)
            previous = parent / f'.{name}.previous'
            if previous.exists():
                shutil.rmtree(previous)
            if destination.exists():
                os.replace(destination, previous)
            os.replace(source, destination)
            if previous.exists():
                shutil.rmtree(previous)
    print(f'toolchain release {RELEASE_TAG} verified and installed')
    for name in RELEASE_COMPONENTS:
        if resource_entries(name) and not resources_installed(name):
            print(f'{name}: the release carries no resource compiler; '
                  f'`homm1 toolchain install {name} --media <iso>` adds the '
                  'pinned RC.EXE/CVTRES.EXE for the candidate .rsrc')


def command(args):
    if args.action == 'install':
        if args.media is not None:
            install(args.id, args.media)
        else:
            install_release(args.archive)
    elif args.action == 'symbols':
        index = library_symbols(args.id)
        print(f'{args.id}: indexed {len(index)} external symbols from verified libraries')
    else:
        verify(args.id)
        print(f'{args.id}: all pinned files verified')
        if resource_entries(args.id):
            state = ('verified' if resources_installed(args.id) else
                     'not installed (install from the media to link .rsrc)')
            print(f'{args.id}: resource compiler and CVTRES {state}')


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

"""Serialized, recoverable publication of a validated campaign generation.

The journal precedes every replacement. Readers and writers acquire the same
lock and finish committed publication first, including after a process crash.
"""
from contextlib import contextmanager
import fcntl
import json
import os
from pathlib import Path
import tempfile

from homm1.core.inputs import REPO

_held = set()


def atomic_write(path, text):
    path = Path(path)
    path.parent.mkdir(parents=True, exist_ok=True)
    if path.exists() and path.read_text() == text:
        return
    fd, pending = tempfile.mkstemp(prefix=path.name + '.', dir=path.parent)
    try:
        with os.fdopen(fd, 'w') as handle:
            handle.write(text)
            handle.flush()
            os.fsync(handle.fileno())
        os.replace(pending, path)
        directory = os.open(path.parent, os.O_DIRECTORY)
        try:
            os.fsync(directory)
        finally:
            os.close(directory)
    finally:
        Path(pending).unlink(missing_ok=True)


def recover(root=REPO):
    journal = root / 'build/publication.json'
    if journal.exists():
        files = json.loads(journal.read_text())
        for relative, content in files.items():
            path = (root / relative).resolve()
            if not path.is_relative_to(root.resolve()) or path == journal.resolve():
                raise ValueError('invalid publication journal destination')
            atomic_write(path, content)
        journal.unlink()


@contextmanager
def locked(root=REPO):
    root = Path(root).resolve()
    if root in _held:
        yield
        return
    path = root / 'build/campaign.lock'
    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open('a') as handle:
        try:
            fcntl.flock(handle, fcntl.LOCK_EX | fcntl.LOCK_NB)
        except BlockingIOError:
            raise ValueError('another campaign command is running; retry after it finishes') from None
        _held.add(root)
        try:
            recover(root)
            yield
        finally:
            _held.remove(root)
            fcntl.flock(handle, fcntl.LOCK_UN)


def publish(files, root=REPO):
    with locked(root):
        atomic_write(root / 'build/publication.json', json.dumps(files))
        recover(root)

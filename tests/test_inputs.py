import hashlib
import os
from pathlib import Path
import tempfile
import unittest
from unittest.mock import patch

from homm1.core.inputs import Executable, InputError, read_verified, stage_executable


class InputTests(unittest.TestCase):
    def setUp(self):
        self.scratch = tempfile.TemporaryDirectory()
        self.addCleanup(self.scratch.cleanup)
        self.root = Path(self.scratch.name)
        self.data = b'pinned test executable'
        self.pin = Executable('test', self.root / 'build/orig/test.exe', len(self.data),
                              hashlib.sha256(self.data).hexdigest(), 'HOMM1_TEST_EXE', '--exe')
        self.source = self.root / 'source.exe'
        self.source.write_bytes(self.data)
        self.environment = patch.dict(os.environ, {}, clear=True)
        self.environment.start()
        self.addCleanup(self.environment.stop)

    def test_init_is_idempotent_and_does_not_need_original_again(self):
        result = stage_executable(self.pin, self.source)
        stamp = result.stat().st_mtime_ns
        self.source.unlink()
        self.assertEqual(stage_executable(self.pin), result)
        self.assertEqual(result.stat().st_mtime_ns, stamp)
        self.assertEqual(read_verified(self.pin, result), self.data)

    def test_bad_explicit_input_does_not_replace_good_staged_copy(self):
        stage_executable(self.pin, self.source)
        self.source.write_bytes(b'x' * len(self.data))
        with self.assertRaisesRegex(InputError, 'sha256'):
            stage_executable(self.pin, self.source)
        self.assertEqual(self.pin.destination.read_bytes(), self.data)

    def test_cli_overrides_environment_but_bad_environment_is_not_ignored(self):
        os.environ['HOMM1_TEST_EXE'] = str(self.root / 'missing.exe')
        stage_executable(self.pin, self.source)
        with self.assertRaises(InputError):
            stage_executable(self.pin)

    def test_corrupt_staged_copy_requires_valid_source(self):
        stage_executable(self.pin, self.source)
        self.pin.destination.write_bytes(b'bad')
        with self.assertRaisesRegex(InputError, 'size'):
            stage_executable(self.pin)
        stage_executable(self.pin, self.source)
        self.assertEqual(self.pin.destination.read_bytes(), self.data)

    def test_missing_input_has_actionable_error(self):
        with self.assertRaisesRegex(InputError, 'homm1 init --exe'):
            stage_executable(self.pin)

import hashlib
from pathlib import Path
import tempfile
import unittest
from unittest.mock import patch

from homm1 import toolchain
from homm1.core.compiler import wine_env


class ToolchainTests(unittest.TestCase):
    def test_verification_rejects_changed_or_missing_component(self):
        with tempfile.TemporaryDirectory() as scratch:
            root = Path(scratch)
            (root / 'bin').mkdir()
            component = root / 'bin/CL.EXE'
            component.write_bytes(b'compiler')
            config = {'test': {'files': {'bin/CL.EXE': {'sha256': hashlib.sha256(b'compiler').hexdigest()}}}}
            with patch.object(toolchain, 'pins', return_value=config):
                self.assertEqual(toolchain.verify('test', root), root)
                component.write_bytes(b'changed!')
                with self.assertRaisesRegex(ValueError, 'missing or changed'):
                    toolchain.verify('test', root)
                component.unlink()
                with self.assertRaises(ValueError):
                    toolchain.verify('test', root)

    def test_release_only_component_is_part_of_verification(self):
        with tempfile.TemporaryDirectory() as scratch:
            root = Path(scratch)
            (root / 'bin').mkdir()
            (root / 'bin/CL.EXE').write_bytes(b'compiler')
            config = {'test': {
                'files': {'bin/CL.EXE': {
                    'sha256': hashlib.sha256(b'compiler').hexdigest()}},
                'release_files': {'bin/ML.EXE': {
                    'sha256': hashlib.sha256(b'assembler').hexdigest()}},
            }}
            with patch.object(toolchain, 'pins', return_value=config):
                with self.assertRaisesRegex(ValueError, 'bin/ML.EXE'):
                    toolchain.verify('test', root)
                (root / 'bin/ML.EXE').write_bytes(b'assembler')
                self.assertEqual(toolchain.verify('test', root), root)

    def test_wrong_media_is_rejected_before_extraction(self):
        with tempfile.TemporaryDirectory() as scratch:
            media = Path(scratch) / 'wrong.iso'
            media.write_bytes(b'wrong')
            config = {'test': {'media': {'sha256': '0' * 64}}}
            with patch.object(toolchain, 'pins', return_value=config), patch.object(toolchain.subprocess, 'run') as run:
                with self.assertRaisesRegex(ValueError, 'media SHA-256'):
                    toolchain.install('test', media)
                run.assert_not_called()

    def test_inherited_flags_and_include_paths_cannot_change_profile(self):
        with patch.dict('os.environ', {'CL': '/O2', '_CL_': '/GX', 'INCLUDE': 'bad', 'LIB': 'bad',
                                      'WINEPREFIX': '/some/unrelated/prefix'}):
            environment = wine_env()
            for key in ('CL', '_CL_', 'INCLUDE', 'LIB'):
                self.assertNotIn(key, environment)
            self.assertEqual(environment['WINEPREFIX'], str(toolchain.REPO / 'build/wineprefix'))

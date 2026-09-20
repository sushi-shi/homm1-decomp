import hashlib
from pathlib import Path
import tempfile
import unittest
from unittest.mock import patch

from homm1 import toolchain
from homm1.core.compiler import wine_env
from homm1.tool import objconv_omf


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

    def test_watcom_comparison_object_renames_text_section(self):
        with tempfile.TemporaryDirectory() as scratch:
            root = Path(scratch)
            source = root / 'input.omf'
            output = root / 'output.obj'
            source.write_bytes(b'\x80omf')

            def convert(argv, **kwargs):
                output.write_bytes(b'coff')
                return type('Result', (), {
                    'returncode': 0, 'stdout': '', 'stderr': ''})()

            with patch.object(objconv_omf, 'require',
                              return_value='objconv-omf'), \
                    patch.object(objconv_omf.subprocess, 'run',
                                 side_effect=convert) as run:
                objconv_omf.convert(source, output)

            argv = run.call_args.args[0]
            self.assertIn('-nr:_TEXT:.text', argv)

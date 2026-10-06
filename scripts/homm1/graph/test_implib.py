"""Import archive checks cover old COFF and VC6 short member encodings."""
from pathlib import Path
import struct
import unittest
from unittest import mock

from homm1.graph import implib
from homm1.tool import ToolError


def archive(*members, name='example.dll'):
    result = b'!<arch>\n'
    for member in members:
        result += (f'{name}/'.ljust(16).encode() + b'0           ' + b'0     ' + b'0     '
                   + b'100644  ' + f'{len(member):<10}'.encode() + b'`\n'
                   + member + (b'\n' if len(member) & 1 else b''))
    return result


def short(name='_Example@4', value=7, kind=1, export=None):
    payload = name.encode() + b'\0example.dll\0'
    if export is not None:
        payload += export.encode() + b'\0'
    return struct.pack('<HHHHIIHH', 0, 0xffff, 0, 0x14c, 0,
                       len(payload), value, kind << 2) + payload


def long_hint(name, hint):
    blob = struct.pack('<H', hint) + name.encode() + b'\0'
    return (struct.pack('<HHIIIHH', 0x14c, 1, 0, 0, 0, 0, 0)
            + struct.pack('<8sIIIIIIHHI', b'.idata$6', 0, 0, len(blob), 60,
                          0, 0, 0, 0, 0) + blob)


class ImportVerificationTests(unittest.TestCase):
    def verify(self, member, hints=None, ordinals=None):
        with mock.patch.object(Path, 'read_bytes', return_value=archive(member)):
            implib._verify_hints(Path('example.lib'), hints or {})
            implib._verify_ordinals(Path('example.lib'), ordinals or {})

    def test_short_name_transformations(self):
        for kind, export, expected in ((1, None, '_Example@4'),
                                       (2, None, 'Example@4'),
                                       (3, None, 'Example'),
                                       (4, 'Alias', 'Alias')):
            with self.subTest(kind=kind):
                self.verify(short(kind=kind, export=export), {expected: 7})

    def test_long_format_still_checks_hint(self):
        self.verify(long_hint('Example', 7), {'Example': 7})
        with self.assertRaisesRegex(ToolError, 'hint mismatch'):
            self.verify(long_hint('Example', 8), {'Example': 7})

    def test_short_wrong_hint_and_ordinal_fail(self):
        with self.assertRaisesRegex(ToolError, 'hint mismatch'):
            self.verify(short(value=8), {'_Example@4': 7})
        self.verify(short(kind=0), ordinals={7: '_Example@4'})
        with self.assertRaisesRegex(ToolError, 'ordinal mismatch'):
            self.verify(short(kind=0), ordinals={8: '_Example@4'})
        with self.assertRaisesRegex(ToolError, 'ordinal mismatch'):
            self.verify(short(kind=1), ordinals={7: '_Example@4'})

    def test_malformed_short_records_fail_even_with_no_expected_hints(self):
        for member in (short()[:-1], short()[:10], short(kind=5),
                       short(kind=4), short() + b'garbage'):
            with self.subTest(member=member), self.assertRaises(ToolError):
                self.verify(member)

    def test_members_must_carry_the_dll_name(self):
        lib = Path('example.lib')
        with mock.patch.object(Path, 'read_bytes', return_value=archive(short())):
            implib._verify_members(lib, 'example.dll')
            with self.assertRaisesRegex(ToolError, 'not EXAMPLE.DLL/'):
                implib._verify_members(lib, 'EXAMPLE.DLL')
        mixed = archive(short()) + archive(short(), name='other.dll')[8:]
        with mock.patch.object(Path, 'read_bytes', return_value=mixed):
            with self.assertRaisesRegex(ToolError, 'other.dll/'):
                implib._verify_members(lib, 'example.dll')

    def test_truncated_archive_fails(self):
        with mock.patch.object(Path, 'read_bytes', return_value=archive(short())[:-2]):
            with self.assertRaisesRegex(ToolError, 'truncated'):
                implib._verify_hints(Path('example.lib'), {})

    def test_conflicting_duplicate_does_not_mask_bad_hint(self):
        data = archive(short(value=8), short(value=7))
        with mock.patch.object(Path, 'read_bytes', return_value=data):
            with self.assertRaisesRegex(ToolError, 'conflicting'):
                implib._verify_hints(Path('example.lib'), {'_Example@4': 7})

    def test_long_ordinal_keeps_the_symbol_binding(self):
        name = b'__imp__Example@4\0'
        symbol = struct.pack('<IIIhHBB', 0, 4, 0, 1, 0, 2, 0)
        member = (struct.pack('<HHIIIHH', 0x14c, 1, 0, 64, 1, 0, 0)
                  + struct.pack('<8sIIIIIIHHI', b'.idata$4', 0, 0, 4, 60,
                                0, 0, 0, 0, 0)
                  + struct.pack('<I', 0x80000007) + symbol
                  + struct.pack('<I', 4 + len(name)) + name)
        self.verify(member, ordinals={7: '_Example@4'})
        with self.assertRaisesRegex(ToolError, 'ordinal mismatch'):
            self.verify(member, ordinals={8: '_Example@4'})

    def test_direct_iat_facts_are_bound_to_image_and_slot(self):
        import hashlib
        import json
        import tempfile
        pe = mock.Mock(data=b'test-image')
        row = dict(slot_rva='0x1000', dll='test.dll', import_key=7, symbol='_Example@4')
        facts = dict(image_sha256=hashlib.sha256(pe.data).hexdigest(), imports=[row])
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / 'imports.json'
            path.write_text(json.dumps(facts))
            slots = {0x1000: ('test.dll', 7)}
            self.assertEqual(implib._reviewed_import_symbols(pe, slots, path),
                             {'test.dll': {7: '_Example@4'}})
            with self.assertRaisesRegex(ToolError, 'IAT identity'):
                implib._reviewed_import_symbols(pe, {0x1000: ('test.dll', 8)}, path)
            facts['imports'].append(row)
            path.write_text(json.dumps(facts))
            with self.assertRaisesRegex(ToolError, 'duplicate'):
                implib._reviewed_import_symbols(pe, slots, path)
            facts['image_sha256'] = 'wrong'
            path.write_text(json.dumps(facts))
            with self.assertRaisesRegex(ToolError, 'different image'):
                implib._reviewed_import_symbols(pe, slots, path)

    def test_failed_verification_does_not_publish(self):
        import tempfile
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            lib = root / 'example.lib'
            lib.write_bytes(b'previous-good-library')
            def link(*args, **kwargs):
                kwargs['expect'][0].write_bytes(archive(short(value=8)))
            with mock.patch('homm1.tool.wine.era_tool'), \
                 mock.patch('homm1.tool.wine.winepath', side_effect=str), \
                 mock.patch('homm1.tool.cl.compile'), \
                 mock.patch('homm1.tool.link.link', side_effect=link):
                with self.assertRaisesRegex(ToolError, 'hint mismatch'):
                    implib.synthesize('example.dll', {'_Example@4': 7}, root, False)
            self.assertEqual(lib.read_bytes(), b'previous-good-library')


if __name__ == '__main__':
    unittest.main()


class ShapedLibraryTests(unittest.TestCase):
    def test_shape_toolchain_library_preferred(self):
        import tempfile
        from homm1 import toolchain
        with tempfile.TemporaryDirectory() as tmp:
            lib = Path(tmp) / 'vc41' / 'lib'
            lib.mkdir(parents=True)
            (lib / 'NETAPI32.LIB').write_bytes(b'!<arch>\n')
            shapes = {'NETAPI32.dll': {'format': 'vc41'},
                      'mss32.dll': {'format': 'vc41'}}
            with mock.patch.object(toolchain, 'verify'), \
                    mock.patch.object(toolchain, 'root',
                                      side_effect=lambda name: Path(tmp) / name):
                self.assertEqual(implib.shaped_lib('NETAPI32.dll', shapes).name.lower(),
                                 'netapi32.lib')
                # no SDK copy in the shape toolchain: synthesised instead
                self.assertIsNone(implib.shaped_lib('mss32.dll', shapes))
                self.assertIsNone(implib.shaped_lib('KERNEL32.dll', shapes))
            with mock.patch.object(toolchain, 'verify', side_effect=ValueError('absent')):
                self.assertIsNone(implib.shaped_lib('NETAPI32.dll', shapes))

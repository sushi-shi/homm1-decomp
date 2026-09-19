import struct
import unittest

from homm1.core.coff import CoffObject, REL32
from homm1.core.image import Image
from homm1.core.matching import Claim, compare, confirm_object, function_extent
from homm1.delink import sparse_relocation_view, yaml_text
from test_image import fixture


def two_functions():
    # first: mov eax,1; ret. second: call first; ret.
    code = b'\xb8\1\0\0\0\xc3\xe8\0\0\0\0\xc3'
    reloc = struct.pack('<IIH', 7, 0, REL32)
    header = struct.pack('<HHIIIHH', 0x14c, 1, 0, 60 + len(code) + len(reloc), 2, 0, 0)
    section = b'.text\0\0\0' + struct.pack('<IIIIIIHHI', 0, 0, len(code), 60, 60 + len(code), 0, 1, 0, 0x60000020)
    symbols = b''.join(name.ljust(8, b'\0') + struct.pack('<IhHBB', value, 1, 32, 2, 0)
                       for name, value in ((b'_first', 0), (b'_second', 6)))
    return CoffObject(header + section + code + reloc + symbols + struct.pack('<I', 4))


class MultiFunctionTests(unittest.TestCase):
    def test_definition_and_intra_object_reference_are_independently_bound(self):
        obj = two_functions()
        claims = [Claim(0x1000, 6, '_first'), Claim(0x1100, 6, '_second')]
        class Retail:
            image_base = 0x400000
            def read(self, rva, size):
                return {0x1000: b'\xb8\1\0\0\0\xc3',
                        0x1100: b'\xe8' + struct.pack('<i', 0x1000 - 0x1105) + b'\xc3'}[rva]
        confirm_object(obj, claims)
        refs = [dict(site=1, typ=REL32, symbol='_first', target_rva=0x1000, addend=0)]
        self.assertTrue(compare(obj, Retail(), claims[0], [], claims)['exact'])
        self.assertTrue(compare(obj, Retail(), claims[1], refs, claims)['exact'])
        self.assertEqual(function_extent(obj, '_second')[1:], (6, 12))

    def test_unclaimed_emitted_body_fails(self):
        with self.assertRaisesRegex(ValueError, 'lack source/generated ownership'):
            confirm_object(two_functions(), [Claim(0x1000, 6, '_first')])

    def test_sparse_view_only_selects_relocation_directory_records(self):
        data = fixture()
        struct.pack_into('<II', data, 0x98 + 96 + 5 * 8, 0x1100, 12)
        struct.pack_into('<IIHH', data, 0x300, 0x1000, 12, 0x3002, 0x3022)
        original = Image(bytes(data))
        view = sparse_relocation_view(original, [Claim(0x1000, 16, '_first')])
        self.assertEqual(Image(view).relocations(), [dict(rva=0x1002, type=3)])
        self.assertEqual(view[:0x30a], data[:0x30a])
        self.assertEqual(view[0x30c:], data[0x30c:])
        self.assertEqual(original.relocations(), [dict(rva=0x1002, type=3), dict(rva=0x1022, type=3)])

    def test_pdb_unknown_target_is_zero_extent(self):
        image = Image(fixture())
        claim = Claim(0x1000, 6, '_first', unit='test')
        text = yaml_text(image, [claim], {claim.rva: [dict(symbol='_external', target_rva=0x1100)]})
        self.assertIn('CodeSize: 0', text)
        self.assertNotIn('CodeSize: 256', text)

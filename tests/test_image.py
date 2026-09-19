import struct
import unittest

from homm1.core.image import Image


def fixture():
    data = bytearray(0x400)
    data[:2] = b'MZ'
    struct.pack_into('<I', data, 0x3c, 0x80)
    data[0x80:0x84] = b'PE\0\0'
    struct.pack_into('<HHIIIHH', data, 0x84, 0x14c, 1, 0, 0, 0, 224, 0)
    opt = 0x98
    struct.pack_into('<HBB', data, opt, 0x10b, 3, 0)
    struct.pack_into('<I', data, opt + 16, 0x1000)
    struct.pack_into('<I', data, opt + 28, 0x400000)
    struct.pack_into('<II', data, opt + 56, 0x2000, 0x200)
    struct.pack_into('<I', data, opt + 92, 16)
    section = opt + 224
    data[section:section + 8] = b'.text\0\0\0'
    struct.pack_into('<IIII', data, section + 8, 0x300, 0x1000, 0x200, 0x200)
    struct.pack_into('<I', data, section + 36, 0x60000020)
    data[0x200] = 0xc3
    return data


class ImageTests(unittest.TestCase):
    def test_virtual_tail_is_mapped_but_not_readable(self):
        image = Image(fixture())
        self.assertEqual(image.read(0x1000, 1), b'\xc3')
        self.assertEqual(image.section_of(0x1250).name, '.text')
        with self.assertRaisesRegex(ValueError, 'not backed'):
            image.read(0x1250, 4)
        with self.assertRaises(ValueError):
            image.read(0x11ff, 2)

    def test_zero_fill_only_section_is_preserved(self):
        data = fixture()
        struct.pack_into('<II', data, 0x98 + 224 + 16, 0, 0)
        image = Image(data)
        self.assertIsNotNone(image.section_of(0x1000))
        with self.assertRaises(ValueError):
            image.read(0x1000, 1)

    def test_relocations_exclude_padding_and_preserve_highlow(self):
        data = fixture()
        struct.pack_into('<II', data, 0x98 + 96 + 5 * 8, 0x1100, 12)
        struct.pack_into('<IIHH', data, 0x300, 0x1000, 12, 0x3004, 0)
        self.assertEqual(Image(data).relocations(), [{'rva': 0x1004, 'type': 3}])
        struct.pack_into('<I', data, 0x304, 0)
        with self.assertRaisesRegex(ValueError, 'block size'):
            Image(data).relocations()

    def test_source_paths_retain_duplicate_addresses(self):
        data = fixture()
        path = b'D:\\Heroes\\Base\\INPUTMGR.CPP\0'
        data[0x220:0x220 + len(path)] = path
        data[0x280:0x280 + len(path)] = path
        rows = Image(data).source_paths()
        self.assertEqual([r['rva'] for r in rows], [0x1020, 0x1080])

    def test_truncation_and_wrong_architecture_are_rejected(self):
        for data in [b'MZ', fixture()[:0x190], fixture()[:0x300]]:
            with self.assertRaises(ValueError):
                Image(data)
        data = fixture()
        struct.pack_into('<H', data, 0x84, 0x8664)
        with self.assertRaisesRegex(ValueError, 'i386'):
            Image(data)

    def test_no_directories_is_valid(self):
        report = Image(fixture()).report()
        for key in ('imports', 'exports', 'relocations', 'source_paths'):
            self.assertEqual(report[key], [])

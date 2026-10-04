"""Boundary checks for real PE relocations and explicit /FIXED site manifests."""
import hashlib
from pathlib import Path
import struct
import tempfile
import unittest

from homm1.core.pe import Pe


class RelocationTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name)
        self.path = self.root / 'test.exe'

    def image(self, reloc=True):
        raw = bytearray(0x600)
        raw[:2] = b'MZ'
        struct.pack_into('<I', raw, 0x3c, 0x80)
        raw[0x80:0x84] = b'PE\0\0'
        struct.pack_into('<HH', raw, 0x84, 0x14c, 2)
        struct.pack_into('<H', raw, 0x94, 224)
        opt = 0x98
        struct.pack_into('<H', raw, opt, 0x10b)
        struct.pack_into('<I', raw, opt + 28, 0x400000)
        struct.pack_into('<I', raw, opt + 92, 16)
        for index, (name, rva, size, off) in enumerate([
                (b'.rdata', 0x1000, 0x200, 0x200), (b'.data', 0x2000, 0x200, 0x400)]):
            base = opt + 224 + index * 40
            raw[base:base + len(name)] = name
            struct.pack_into('<IIII', raw, base + 8, size, rva, size, off)
        struct.pack_into('<I', raw, 0x400, 0x402004)
        if reloc:
            struct.pack_into('<II', raw, opt + 96 + 5 * 8, 0x1000, 12)
            struct.pack_into('<IIHH', raw, 0x200, 0x2000, 12, 0x3000, 0)
        self.path.write_bytes(raw)
        return Pe(self.path)

    def manifest(self, pe, rows='0x2000\tdir32\n'):
        path = self.root / 'sites.tsv'
        path.write_text('# image-sha256: ' + hashlib.sha256(pe.data).hexdigest()
                        + '\nsite_rva\tkind\n' + rows)
        return path

    def test_directory_need_not_have_reloc_section(self):
        pe = self.image()
        self.assertEqual(pe.highlow_sites(), [0x2000])
        self.assertEqual(pe.data_regions()['idata'], (0, 0))

    def test_fixed_manifest_is_pinned_to_exact_image(self):
        pe = self.image(False)
        manifest = self.manifest(pe)
        self.assertEqual(pe.highlow_sites(manifest), [0x2000])
        raw = bytearray(pe.data)
        raw[0x405] ^= 1
        self.path.write_bytes(raw)
        with self.assertRaisesRegex(ValueError, 'not pinned'):
            Pe(self.path).highlow_sites(manifest)

    def test_missing_manifest_does_not_mean_no_references(self):
        with self.assertRaises(FileNotFoundError):
            self.image(False).highlow_sites(self.root / 'missing.tsv')

    def test_duplicate_and_unmapped_sites_are_rejected(self):
        pe = self.image(False)
        for rows in ['0x2000\tdir32\n0x2000\tdir32\n', '0x9999\tdir32\n', '0x2000\trel32\n']:
            with self.subTest(rows=rows), self.assertRaises(ValueError):
                pe.highlow_sites(self.manifest(pe, rows))

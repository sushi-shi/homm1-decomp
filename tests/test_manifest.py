from pathlib import Path
import struct
import tempfile
import unittest

from homm1.core.image import Image
from homm1.core.manifest import check_retail
from test_image import fixture


class RetailChannelsTests(unittest.TestCase):
    def setUp(self):
        self.scratch = tempfile.TemporaryDirectory()
        self.addCleanup(self.scratch.cleanup)
        self.root = Path(self.scratch.name)
        self.functions = self.root / 'functions.tsv'
        self.functions.write_text('# Sparse base\nrva\tkind\n0x1000\t\n')
        (self.root / 'data.tsv').write_text('rva\tkind\n')
        self.provider = self.root / 'functions_exports.tsv'
        self.provider.write_text('rva\tname\tordinal\tprovenance\n0x1000\tTest\t1\tPE export\n')
        data = fixture()
        struct.pack_into('<II', data, 0x98 + 96, 0x1100, 0x60)
        struct.pack_into('<IIHHIIIIIII', data, 0x300,
                         0, 0, 0, 0, 0x1150, 1, 1, 1, 0x1130, 0x1140, 0x1148)
        struct.pack_into('<I', data, 0x330, 0x1000)
        struct.pack_into('<I', data, 0x340, 0x1170)
        struct.pack_into('<H', data, 0x348, 0)
        data[0x350:0x359] = b'TEST.EXE\0'
        data[0x370:0x375] = b'Test\0'
        self.image = Image(data)

    def test_provider_matches_real_export_and_admitted_body(self):
        self.assertEqual(self.image.exports()[0]['va'], 0x401000)
        self.assertEqual(check_retail(self.image, self.root), {0x1000: ''})

    def test_orphan_and_wrong_kind_provider_are_rejected(self):
        for rows in ('', '0x1000\tpad\n'):
            self.functions.write_text('rva\tkind\n' + rows)
            with self.assertRaisesRegex(ValueError, 'admitted function body'):
                check_retail(self.image, self.root)

    def test_wrong_export_name_is_rejected(self):
        self.provider.write_text(self.provider.read_text().replace('Test', 'Wrong'))
        with self.assertRaisesRegex(ValueError, 'disagrees'):
            check_retail(self.image, self.root)

    def test_duplicate_and_unsorted_starts_are_rejected(self):
        for rows in ('0x1000\t\n0x1000\t\n', '0x1001\t\n0x1000\t\n'):
            self.functions.write_text('rva\tkind\n' + rows)
            with self.assertRaisesRegex(ValueError, 'unique and ascending'):
                check_retail(self.image, self.root)

    def test_data_census_cannot_admit_code(self):
        (self.root / 'data.tsv').write_text('rva\tkind\n0x1000\tstring\n')
        with self.assertRaisesRegex(ValueError, 'address space'):
            check_retail(self.image, self.root)

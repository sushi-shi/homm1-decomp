import struct
import unittest

from homm1.core.coff import CoffObject, DIR32, REL32
from homm1.core.matching import Claim, compare, target_object


class SyntheticImage:
    image_base = 0x400000

    def __init__(self):
        # call [IAT]; call function; ret. Independent resolved retail fixture.
        self.code = b'\xff\x15' + struct.pack('<I', 0x402000) + b'\xe8' + struct.pack('<i', 0x3000 - 0x100b) + b'\xc3'

    def read(self, rva, size):
        assert rva == 0x1000 and size == 12
        return self.code


class ExactComparisonTests(unittest.TestCase):
    def setUp(self):
        self.image = SyntheticImage()
        self.claim = Claim(0x1000, 12, '_Test@0')
        self.refs = [dict(site=2, typ=DIR32, symbol='__imp__Test@0', target_rva=0x2000, addend=0),
                     dict(site=7, typ=REL32, symbol='_Helper', target_rva=0x3000, addend=0)]
        self.payload = target_object(self.image, self.claim, self.refs)

    def compare(self, payload):
        return compare(CoffObject(payload), self.image, self.claim, self.refs)

    def test_independent_carve_restores_absolute_and_relative_relocations(self):
        result = self.compare(self.payload)
        self.assertTrue(result['exact'])
        self.assertEqual(result['matched_bytes'], 12)
        obj = CoffObject(self.payload)
        self.assertEqual(obj.section_bytes(obj.section(1))[2:6], bytes(4))
        self.assertEqual(obj.section_bytes(obj.section(1))[7:11], bytes(4))

    def test_changed_instruction_cannot_match(self):
        broken = bytearray(self.payload)
        broken[60 + 11] = 0x90
        result = self.compare(broken)
        self.assertFalse(result['exact'])
        self.assertEqual(result['differing_offsets'], [11])

    def test_changed_relocation_addend_is_not_masked(self):
        broken = bytearray(self.payload)
        struct.pack_into('<I', broken, 60 + 2, 4)
        result = self.compare(broken)
        self.assertFalse(result['exact'])
        self.assertFalse(result['relocations_exact'])

    def test_wrong_referent_is_rejected(self):
        broken = self.payload.replace(b'_Helper\0', b'_Wrong_\0')
        with self.assertRaisesRegex(ValueError, 'unresolved'):
            self.compare(broken)

    def test_missing_relocation_cannot_match(self):
        broken = bytearray(self.payload)
        struct.pack_into('<H', broken, 20 + 32, 1)
        result = self.compare(broken)
        self.assertFalse(result['exact'])
        self.assertFalse(result['relocations_exact'])

    def test_unsupported_relocation_is_rejected(self):
        broken = bytearray(self.payload)
        struct.pack_into('<H', broken, 60 + 12 + 8, 0x1234)
        with self.assertRaisesRegex(ValueError, 'unsupported'):
            self.compare(broken)

    def test_size_mismatch_cannot_match(self):
        broken = bytearray(self.payload)
        struct.pack_into('<I', broken, 20 + 16, 11)
        result = self.compare(broken)
        self.assertFalse(result['exact'])
        self.assertEqual(result['compiled_size'], 11)

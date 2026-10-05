"""VC6 /Od slot order, checked against measured Buka probe compiles."""

import unittest

from homm1.core.od_slots import bucket, slot_order


class OdSlotsTest(unittest.TestCase):
    def test_bucket_uses_folded_vc6_hash(self):
        self.assertEqual([bucket(n) for n in ("attackMask", "oldSide", "oldIndex", "j")],
                         [12, 9, 5, 10])

    def test_probe_layouts(self):
        # /Z7 S_BPREL32 order of VC6 probe compiles, first slot first.
        self.assertEqual(slot_order(["attackMask", "oldSide", "oldIndex", "j"]),
                         ["oldIndex", "oldSide", "j", "attackMask"])
        self.assertEqual(slot_order(["fileId", "highByte", "size", "buffer", "i"]),
                         ["size", "fileId", "highByte", "i", "buffer"])


if __name__ == "__main__":
    unittest.main()

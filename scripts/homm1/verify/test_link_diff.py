"""The linked-candidate diff counts bytes per retail region."""
import struct
import unittest

from homm1.verify import link_diff


def image(text: bytes, tail: bytes = b"") -> bytes:
    head = bytearray(0x200)
    struct.pack_into("<I", head, 0x3C, 0x40)
    struct.pack_into("<H", head, 0x46, 1)       # one section
    struct.pack_into("<H", head, 0x54, 0)       # no optional header
    o = 0x40 + 24
    head[o:o + 8] = b".text\0\0\0"
    struct.pack_into("<II", head, o + 16, len(text), 0x200)
    return bytes(head) + text + tail


class RegionTests(unittest.TestCase):
    def test_counts_each_region(self):
        retail = image(b"abcd", b"NB")
        cand = image(b"abXd", b"NB!")
        self.assertEqual(link_diff.regions(retail, cand),
                         {"size": 1, "headers": 0, ".text": 1, "overlay": 1})


if __name__ == "__main__":
    unittest.main()

import struct
import unittest

from homm1.core.od_slots import bucket, predict_offsets, slot_order
from homm1.sema.frame import frame_names


def _record(kind, body):
    return struct.pack("<HH", len(body) + 2, kind) + body


def _coff_with_debug_s(payload):
    section = b".debug$S"
    raw_pointer = 20 + 40
    symbol_pointer = raw_pointer + len(payload)
    header = struct.pack("<HHIIIHH", 0x14C, 1, 0, symbol_pointer, 0, 0, 0)
    section_header = section + struct.pack(
        "<IIIIIIHHI", 0, 0, len(payload), raw_pointer, 0, 0, 0, 0, 0)
    return header + section_header + payload + struct.pack("<I", 4)


class OdSlotTests(unittest.TestCase):
    def test_known_identifier_buckets_control_slot_order(self):
        names = ["i", "j", "k", "length"]
        expected = sorted(names, key=lambda name: (bucket(name), -names.index(name)))
        self.assertEqual(slot_order(names), expected)
        offsets = predict_offsets(names)
        self.assertEqual(sorted(offsets.values()), [-16, -12, -8, -4])

    def test_cv4_bprel_names_are_read_from_candidate_object(self):
        proc = bytearray(33)
        proc.extend(b"\x06Decode")
        local = struct.pack("<iH", -4, 0x74) + b"\x05count"
        argument = struct.pack("<iH", 8, 0x470) + b"\x06source"
        debug = struct.pack("<I", 1) + _record(0x0205, proc) \
            + _record(0x0200, local) + _record(0x0200, argument) \
            + _record(0x0006, b"")
        frames = frame_names(_coff_with_debug_s(debug))
        self.assertEqual(frames["Decode"], [(-4, "count"), (8, "source")])


if __name__ == "__main__":
    unittest.main()

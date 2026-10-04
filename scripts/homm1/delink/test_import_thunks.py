"""Indirect jumps inside a real function must not become import functions."""
import struct
import unittest
from types import SimpleNamespace
from unittest.mock import patch

from homm1.delink.pdb_synth import import_thunk_names


class ImportThunkTests(unittest.TestCase):
    def test_only_admitted_entries_receive_import_names(self):
        jump = b"\xff\x25" + struct.pack("<I", 0x408000)
        # Same valid IAT jump at a thunk, inside a function, and inside data.
        blob = jump + b"\x90\x55\x8b\xec" + jump + b"\x00" + jump
        image = SimpleNamespace(
            data=blob, image_base=0x400000,
            pe=SimpleNamespace(section=lambda name: {
                "va": 0x1000, "rsize": len(blob), "rptr": 0}))
        slots = [(0x8000, "__imp__GetACP@0")]
        with patch("homm1.delink.pdb_synth.retail", return_value=image):
            self.assertEqual(import_thunk_names(slots, {}, {0x1000, 0x1007}),
                             {0x1000: "_GetACP@0"})
            self.assertEqual(import_thunk_names(slots, {0x1000: "claimed"},
                                                {0x1000, 0x1007}), {})


if __name__ == "__main__":
    unittest.main()

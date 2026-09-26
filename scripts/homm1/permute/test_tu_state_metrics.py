from __future__ import annotations

import struct
import tempfile
import unittest
from pathlib import Path

from homm1.permute.tu_state_metrics import read_coff


def make_object() -> bytes:
    """One .text function with one DIR32 relocation carrying addend 4."""
    header_size = 20
    section_size = 40
    text = b"\xb8\x04\x00\x00\x00\xc3"
    raw_pointer = header_size + section_size
    relocation_pointer = raw_pointer + len(text)
    symbol_pointer = relocation_pointer + 10
    names = b"?Probe@@YAHXZ\0_target\0"
    strings = struct.pack("<L", 4 + len(names)) + names

    header = struct.pack("<HHLLLHH", 0x14C, 1, 0, symbol_pointer, 2, 0, 0)
    section = b".text\0\0\0" + struct.pack(
        "<LLLLLLHHL", 0, 0, len(text), raw_pointer, relocation_pointer,
        0, 1, 0, 0x60000020,
    )
    relocation = struct.pack("<LLH", 1, 1, 0x0006)
    function = struct.pack("<LLLhHBB", 0, 4, 0, 1, 0x20, 2, 0)
    target = struct.pack("<LLLhHBB", 0, 4 + len(b"?Probe@@YAHXZ\0"), 0, 0, 0, 2, 0)
    return header + section + text + relocation + function + target + strings


class TuStateMetricsTests(unittest.TestCase):
    def test_reads_function_bytes_and_ordered_relocation_addend(self):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "probe.obj"
            path.write_bytes(make_object())
            _object_hash, rows = read_coff(path)

        self.assertEqual(len(rows), 1)
        row = rows[0]
        self.assertEqual(row["function"], "?Probe@@YAHXZ")
        self.assertEqual(row["bytes"], b"\xb8\x04\x00\x00\x00\xc3")
        self.assertEqual(row["size"], 6)
        self.assertEqual(row["relocs"], 1)
        self.assertEqual(
            row["reloc_stream"],
            ["00000001:0006:_target:04000000"],
        )
        self.assertTrue(row["reloc_stream_complete"])

    def test_rejects_a_truncated_header(self):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "bad.obj"
            path.write_bytes(b"short")
            with self.assertRaisesRegex(ValueError, "truncated COFF header"):
                read_coff(path)


if __name__ == "__main__":
    unittest.main()

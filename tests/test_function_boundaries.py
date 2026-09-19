import struct
import unittest

from homm1.compare.canonicalize import (
    CoffObject,
    add_function_padding_boundaries,
)


def coff_with_padded_function(body=b"\xc3", padding=b"\xcc\xcc\xcc"):
    text = body + padding
    header_size = 20 + 40
    symbol_offset = header_size + len(text)
    name = b"function"
    header = struct.pack("<HHIIIHH", 0x14C, 1, 0, symbol_offset, 1, 0, 0)
    section = struct.pack(
        "<8sIIIIIIHHI", b".text\0\0\0", 0, 0, len(text), header_size,
        0, 0, 0, 0, 0x60000020)
    symbol = name + struct.pack("<IhHBB", 0, 1, 0x20, 2, 0)
    return header + section + text + symbol + struct.pack("<I", 4)


class FunctionBoundaryTests(unittest.TestCase):
    def test_reviewed_body_excludes_existing_linker_fill(self):
        original = coff_with_padded_function()
        result = add_function_padding_boundaries(
            original, (("function", 1),))

        before = CoffObject(original)
        after = CoffObject(result)
        self.assertEqual(before.section_bytes(before.sections[0]),
                         after.section_bytes(after.sections[0]))
        self.assertIn(
            ("$fnpad@1", 1, 1),
            [(s.name, s.value, s.section) for s in after.symbols.values()])

    def test_non_padding_tail_is_not_bounded(self):
        original = coff_with_padded_function(padding=b"\x04\xcc")
        self.assertEqual(
            original,
            add_function_padding_boundaries(original, (("function", 1),)))


if __name__ == "__main__":
    unittest.main()

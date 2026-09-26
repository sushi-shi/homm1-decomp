"""The data_matching = false relaxation step (homm1.compare.normalize)."""

from __future__ import annotations

import struct
import unittest
from unittest import mock

from homm1.compare import canonicalize as canon
from homm1.compare import normalize

DIR32, REL32 = 0x0006, 0x0014
TEXT = 0x60000020     # CNT_CODE | MEM_EXECUTE | MEM_READ
DATA = 0xC0000040     # CNT_INITIALIZED_DATA | MEM_READ | MEM_WRITE


def coff(text: bytes, text_relocs, data: bytes, data_relocs, symbols) -> bytes:
    """A two-section (.text, .data) i386 COFF.

    `symbols` are (name, value, section, type, storage); relocs are
    (site, symbol index, type). Long names go to the string table."""
    header_end = 20 + 2 * 40
    text_ptr = header_end
    text_rel = text_ptr + len(text)
    data_ptr = text_rel + 10 * len(text_relocs)
    data_rel = data_ptr + len(data)
    symptr = data_rel + 10 * len(data_relocs)
    out = bytearray(struct.pack("<HHIIIHH", 0x14C, 2, 0, symptr, len(symbols), 0, 0))
    for name, raw, ptr, rel, nrel, chars in (
            (b".text", text, text_ptr, text_rel, len(text_relocs), TEXT),
            (b".data", data, data_ptr, data_rel, len(data_relocs), DATA)):
        out += struct.pack("<8sIIIIIIHHI", name, 0, 0, len(raw), ptr,
                           rel if nrel else 0, 0, nrel, 0, chars)
    for raw, relocs in ((text, text_relocs), (data, data_relocs)):
        out += raw
        for site, index, typ in relocs:
            out += struct.pack("<IIH", site, index, typ)
    strings = bytearray()
    for name, value, section, typ, storage in symbols:
        raw = name.encode("latin-1")
        if len(raw) <= 8:
            field = raw.ljust(8, b"\0")
        else:
            field = struct.pack("<II", 0, 4 + len(strings))
            strings += raw + b"\0"
        out += struct.pack("<8sIhHBB", field, value, section, typ, storage, 0)
    out += struct.pack("<I", 4 + len(strings)) + strings
    return bytes(out)


#: symbol indices of the fixture below
FUNC, G_DATA, CALLEE, IMP, LOCAL, LABEL, FNPTR = range(7)
SYMBOLS = [
    ("_Func", 0, 1, 0x20, 2),            # 0 the function (defined, .text)
    ("_g_data", 0, 0, 0x00, 2),          # 1 undefined data extern
    ("_Callee", 0, 0, 0x20, 2),          # 2 undefined function
    ("__imp__Foo@4", 0, 0, 0x00, 2),     # 3 IAT slot
    ("_s_local", 4, 2, 0x00, 3),         # 4 static datum in .data
    ("$L100", 24, 1, 0x00, 6),           # 5 label in .text (jump table)
    ("_Handler", 0, 0, 0x20, 2),         # 6 function pointer target
]


def fixture(data_addend: int = 8, local_addend: int = 0) -> bytes:
    text = bytearray(32)
    text[0:1] = b"\xa1"                                  # mov eax,[g_data+A]
    struct.pack_into("<I", text, 1, data_addend)
    text[5:6] = b"\xe8"                                  # call Callee
    text[10:12] = b"\xff\x15"                            # call [__imp__Foo@4]
    text[16:17] = b"\xb8"                                # mov eax,s_local+B
    struct.pack_into("<I", text, 17, local_addend)
    text[21:22] = b"\x68"                                # push Handler
    struct.pack_into("<I", text, 28, 0)                  # jump-table slot
    text_relocs = [(1, G_DATA, DIR32), (6, CALLEE, REL32), (12, IMP, DIR32),
                   (17, LOCAL, DIR32), (22, FNPTR, DIR32), (28, LABEL, DIR32)]
    data = bytearray(8)
    struct.pack_into("<I", data, 0, 4)                   # .data pointer
    return coff(bytes(text), text_relocs, bytes(data),
                [(0, G_DATA, DIR32)], SYMBOLS)


def targets(payload: bytes) -> dict[tuple[int, int], tuple[str, int]]:
    """{(section, site): (target name, inline addend)}."""
    obj = canon.CoffObject(payload)
    out = {}
    for r in obj.relocations:
        section = obj.sections[r.section - 1]
        addend = struct.unpack_from("<I", payload, section.raw_offset + r.site)[0]
        out[(r.section, r.site)] = (obj.symbols[r.symbol_index].name, addend)
    return out


class RelaxDataRelocationsTest(unittest.TestCase):
    def test_only_data_targets_are_retargeted_and_zeroed(self):
        payload = fixture()
        out, relaxed = normalize.relax_data_relocations(payload)
        self.assertEqual({r.original_symbol for r in relaxed},
                         {"_g_data", "_s_local"})
        after = targets(out)
        self.assertEqual(after[(1, 1)], ("$data", 0))
        self.assertEqual(after[(1, 17)], ("$data", 0))
        before = targets(payload)
        for site in ((1, 6), (1, 12), (1, 22), (1, 28), (2, 0)):
            # calls, the IAT slot, a function pointer, a jump-table label and
            # a DATA-section relocation all stay exactly as they were
            self.assertEqual(after[site], before[site], site)

    def test_symbol_indices_are_kept_and_the_sink_is_appended(self):
        payload = fixture()
        out, _relaxed = normalize.relax_data_relocations(payload)
        old, new = canon.CoffObject(payload), canon.CoffObject(out)
        for index, symbol in old.symbols.items():
            self.assertEqual(new.symbols[index], symbol)
        sink = new.symbols[old.symbol_count]
        self.assertEqual((sink.name, sink.section, sink.typ, sink.storage_class),
                         ("$data", 0, 0, 2))

    def test_two_sides_differing_only_in_data_identity_become_identical(self):
        base = fixture(data_addend=8, local_addend=0)
        other = bytearray(fixture(data_addend=12, local_addend=4))
        # the other side names a different datum for the first site
        obj = canon.CoffObject(bytes(other))
        first = obj.relocations[0]
        struct.pack_into("<I", other, first.offset + 4, LOCAL)
        self.assertNotEqual(base, bytes(other))
        self.assertEqual(normalize.relax_data_relocations(base)[0],
                         normalize.relax_data_relocations(bytes(other))[0])

    def test_a_function_on_one_side_still_differs_from_data(self):
        base = fixture()
        other = bytearray(base)
        obj = canon.CoffObject(base)
        struct.pack_into("<I", other, obj.relocations[0].offset + 4, FNPTR)
        self.assertNotEqual(normalize.relax_data_relocations(base)[0],
                            normalize.relax_data_relocations(bytes(other))[0])

    def test_no_data_relocation_leaves_the_object_untouched(self):
        payload = coff(b"\xe8" + bytes(7), [(1, 1, REL32)], bytes(4), [],
                       [("_Func", 0, 1, 0x20, 2), ("_Callee", 0, 0, 0x20, 2)])
        self.assertEqual(normalize.relax_data_relocations(payload),
                         (payload, ()))

    def test_the_postcondition_refuses_a_strict_relocation_moving(self):
        payload = fixture()
        out, relaxed = normalize.relax_data_relocations(payload)
        obj = canon.CoffObject(payload)
        call = next(r for r in obj.relocations if r.typ == REL32)
        broken = bytearray(out)
        struct.pack_into("<I", broken, call.offset + 4, obj.symbol_count)
        with self.assertRaises(RuntimeError):
            normalize._assert_only_data_relaxation(obj, payload, bytes(broken),
                                                   relaxed)

    def test_the_postcondition_refuses_an_instruction_byte_moving(self):
        payload = fixture()
        out, relaxed = normalize.relax_data_relocations(payload)
        obj = canon.CoffObject(payload)
        broken = bytearray(out)
        broken[obj.sections[0].raw_offset] ^= 0xFF          # the opcode byte
        with self.assertRaises(RuntimeError):
            normalize._assert_only_data_relaxation(obj, payload, bytes(broken),
                                                   relaxed)

    def test_comparison_copy_follows_the_switch(self):
        payload = fixture()
        strict, strict_rows = normalize.comparison_copy(payload,
                                                        data_matching_on=True)
        self.assertEqual(strict, canon.canonicalize_coff(payload).data)
        self.assertFalse(any(r.family == "data-relax" for r in strict_rows))
        relaxed, rows = normalize.comparison_copy(payload,
                                                  data_matching_on=False)
        self.assertNotEqual(relaxed, strict)
        self.assertTrue(any(r.family == "data-relax" for r in rows))
        with mock.patch("homm1.core.data_matching.enabled", return_value=True):
            self.assertEqual(normalize.comparison_copy(payload)[0], strict)


if __name__ == "__main__":
    unittest.main()

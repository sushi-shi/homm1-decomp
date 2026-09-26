"""The data-identity gate (homm1.verify.data_identity) on synthetic objects."""

from __future__ import annotations

import struct
import unittest
from collections import Counter

from homm1.compare.test_normalize import DIR32, coff
from homm1.verify import data_identity as di

IMAGE_BASE = 0x400000
#: Model rva of each fixture function, by name.
FUNCTION_RVAS = {"_GetA": 0x1000, "_GetB": 0x1010, "_Other": 0x2000}


def unit(functions, *, statics=(), opcode=b"\xa1"):
    """A one-.text unit: each function is `mov eax,[sym+addend]` per ref,
    then `ret`, padded to 16 bytes. `functions` is [(name, [(sym, addend)])];
    `statics` are [(name, bytes)] defined in .data as file statics."""
    text, relocs, symbols, index = bytearray(), [], [], {}
    data = b"".join(payload for _n, payload in statics)
    offset = 0
    for name, payload in statics:
        index[name] = len(symbols)
        symbols.append((name, offset, 2, 0x00, 3))
        offset += len(payload)
    for name, _refs in functions:
        symbols.append((name, 0, 1, 0x20, 2))       # value patched below
    for fn_index, (name, refs) in enumerate(functions):
        start = len(text)
        position = len(statics) + fn_index
        symbols[position] = (name, start, 1, 0x20, 2)
        for symbol, addend in refs:
            if symbol not in index:
                index[symbol] = len(symbols)
                symbols.append((symbol, 0, 0, 0x00, 2))
            text += opcode
            relocs.append((len(text), index[symbol], DIR32))
            text += struct.pack("<I", addend)
        text += b"\xc3"
        text += b"\xcc" * (-len(text) % 16)
    return coff(bytes(text), relocs, data or bytes(4), [], symbols)


def words(pairs):
    """retail_word over {(function, offset): rva}."""
    table = {FUNCTION_RVAS[f] + o: IMAGE_BASE + rva for (f, o), rva in pairs.items()}
    return table.get


def sites(name, base, target, table):
    return di.unit_sites(name, base, target,
                         lambda _u, fn: FUNCTION_RVAS.get(fn),
                         words(table), IMAGE_BASE)


class DataIdentityTests(unittest.TestCase):
    def test_duplicate_strings_require_compiler_identity_and_equal_bytes(self):
        for name in ("??_C@_07CBFD@?5notify?$AA@", "$SG10", "_namedArray"):
            for second in (b" notify\0", b" wrong!\0", None):
                with self.subTest(name=name, second=second):
                    base = unit([("_GetA", [(name, 0)]),
                                 ("_GetB", [(name, 0)])],
                                statics=[(name, b" notify\0")])
                    target = unit([("_GetA", [("_copy1", 0)]),
                                   ("_GetB", [("_copy2", 0)])])
                    copies = {0x91000: b" notify\0", 0x91100: second}
                    got = di.unit_sites(
                        "u", base, target, lambda _u, fn: FUNCTION_RVAS.get(fn),
                        words({("_GetA", 1): 0x91000, ("_GetB", 1): 0x91100}),
                        IMAGE_BASE, retail_read=lambda rva, size: copies.get(rva))
                    allowed = name != "_namedArray" and second == b" notify\0"
                    self.assertEqual(not di.conflicts(got, {}), allowed)
                    if allowed:
                        self.assertTrue(di.conflicts(got, {name: {("u", 0x91000)}}))

    def test_consistent_placeholders_across_units_are_clean(self):
        got = (sites("mouse", unit([("_GetA", [("_g_hovered", 0)])]),
                     unit([("_GetA", [("DAT_004919e0", 0)])]),
                     {("_GetA", 1): 0x919e0})
               + sites("menu", unit([("_GetB", [("_g_hovered", 0)])]),
                       unit([("_GetB", [("DAT_004919e0", 0)])]),
                       {("_GetB", 1): 0x919e0}))
        self.assertEqual(di.conflicts(got, {}), [])
        rows = di.identity_rows(got, {"_g_hovered": "placeholder"})
        self.assertEqual(rows, [("_g_hovered", 0x919e0, 2, "menu,mouse",
                                 "placeholder")])

    def test_two_names_for_one_retail_address_fail(self):
        got = (sites("mouse", unit([("_GetA", [("_g_hovered", 0)])]),
                     unit([("_GetA", [("DAT_004919e0", 0)])]),
                     {("_GetA", 1): 0x919e0})
               + sites("menu", unit([("_GetB", [("_g_lastMenuItem", 0)])]),
                       unit([("_GetB", [("DAT_004919e0", 0)])]),
                       {("_GetB", 1): 0x919e0}))
        found = di.conflicts(got, {})
        self.assertEqual(len(found), 1)
        self.assertIn("0x0919e0", found[0])
        self.assertIn("_g_hovered", found[0])
        self.assertIn("_g_lastMenuItem", found[0])

    def test_one_name_for_two_retail_addresses_fails(self):
        got = sites("menu",
                    unit([("_GetA", [("_g_item", 0)]), ("_GetB", [("_g_item", 0)])]),
                    unit([("_GetA", [("DAT_004919e0", 0)]),
                          ("_GetB", [("$gap_0919ee", 0)])]),
                    {("_GetA", 1): 0x919e0, ("_GetB", 1): 0x919ee})
        found = di.conflicts(got, {})
        self.assertEqual(len(found), 1)
        self.assertIn("reaches 2 retail bases", found[0])

    def test_the_addend_is_subtracted_to_the_object_base(self):
        """`g_arr[1]` and `g_arr[0]` are one object; a separate `g_elem` at
        the element's address is a different object base, not a conflict."""
        got = sites("u",
                    unit([("_GetA", [("_g_arr", 0), ("_g_arr", 4)]),
                          ("_GetB", [("_g_elem", 0)])]),
                    unit([("_GetA", [("DAT_00491000", 0), ("DAT_00491000", 4)]),
                          ("_GetB", [("DAT_00491004", 0)])]),
                    {("_GetA", 1): 0x91000, ("_GetA", 6): 0x91004,
                     ("_GetB", 1): 0x91004})
        self.assertEqual({(s.key, s.base) for s in got},
                         {("_g_arr", 0x91000), ("_g_elem", 0x91004)})
        self.assertEqual(di.conflicts(got, {}), [])

    def test_a_claim_its_references_contradict_fails(self):
        got = sites("u", unit([("_GetA", [("_g_claimed", 0)])]),
                    unit([("_GetA", [("_g_claimed", 0)])]),
                    {("_GetA", 1): 0x91100})
        found = di.conflicts(got, {"_g_claimed": {("u", 0x91200)}})
        self.assertEqual(len(found), 1)
        self.assertIn("claimed at 0x091200", found[0])
        self.assertEqual(di.conflicts(got, {"_g_claimed": {("u", 0x91100)}}), [])

    def test_statics_are_per_unit(self):
        got = (sites("a", unit([("_GetA", [("_s_x", 0)])],
                               statics=[("_s_x", bytes(4))]),
                     unit([("_GetA", [("DAT_00491000", 0)])]),
                     {("_GetA", 1): 0x91000})
               + sites("b", unit([("_GetB", [("_s_x", 0)])],
                                 statics=[("_s_x", bytes(4))]),
                       unit([("_GetB", [("DAT_00491100", 0)])]),
                       {("_GetB", 1): 0x91100}))
        self.assertEqual(di.conflicts(got, {"_s_x": {("a", 0x91000)}}), [])
        # the claim is unit a's; unit b's static reaching elsewhere is fine,
        # but unit a's reaching elsewhere is not
        found = di.conflicts(got, {"_s_x": {("a", 0x91200)}})
        self.assertEqual(len(found), 1)
        self.assertIn("a:_s_x", found[0])

    def test_pooled_identical_literals_are_one_identity(self):
        text = b"hello\0\0\0"
        got = (sites("a", unit([("_GetA", [("$SG10", 0)])],
                               statics=[("$SG10", text)]),
                     unit([("_GetA", [("DAT_00468000", 0)])]),
                     {("_GetA", 1): 0x68000})
               + sites("b", unit([("_GetB", [("$SG99", 0)])],
                                 statics=[("$SG99", text)]),
                       unit([("_GetB", [("DAT_00468000", 0)])]),
                       {("_GetB", 1): 0x68000}))
        self.assertTrue(all(s.literal for s in got))
        self.assertEqual(di.conflicts(got, {}), [])

    def test_a_different_relocation_layout_pairs_nothing(self):
        stats = Counter()
        got = di.unit_sites(
            "u", unit([("_GetA", [("_g_a", 0), ("_g_b", 0)])]),
            unit([("_GetA", [("DAT_00491000", 0)])]),
            lambda _u, fn: FUNCTION_RVAS.get(fn),
            words({("_GetA", 1): 0x91000}), IMAGE_BASE, stats)
        self.assertEqual(got, [])
        self.assertEqual(stats["functions with a different relocation layout"], 1)

    def test_a_different_instruction_ahead_of_the_operand_is_skipped(self):
        stats = Counter()
        got = di.unit_sites(
            "u", unit([("_GetA", [("_g_a", 0)])]),
            unit([("_GetA", [("DAT_00491000", 0)])], opcode=b"\xa3"),
            lambda _u, fn: FUNCTION_RVAS.get(fn),
            words({("_GetA", 1): 0x91000}), IMAGE_BASE, stats)
        self.assertEqual(got, [])
        self.assertEqual(stats["sites under a different instruction"], 1)

    def test_function_references_are_not_data(self):
        """A `push offset Handler` stays a strict function reference."""
        base = coff(b"\x68\0\0\0\0\xc3" + b"\xcc" * 10, [(1, 1, DIR32)], bytes(4),
                    [], [("_GetA", 0, 1, 0x20, 2), ("_Handler", 0, 0, 0x20, 2)])
        got = sites("u", base, base, {("_GetA", 1): 0x5000})
        self.assertEqual(got, [])


if __name__ == "__main__":
    unittest.main()


class OperandSwaps(unittest.TestCase):
    """An operand-order swap is TU state; a real identity split still fails."""

    def _site(self, key, base, offset, function="_F"):
        from homm1.verify.data_identity import Site
        return Site("u", function, offset, key, key, False, 0, base, "t")

    def test_mutual_swap_is_excluded(self):
        from homm1.verify.data_identity import conflicts
        sites = [self._site("_a", 0x100, 0x10, "_G"), self._site("_b", 0x200, 0x20, "_G"),
                 self._site("_a", 0x100, 0x30, "_H"), self._site("_b", 0x200, 0x40, "_H"),
                 # in _F the two loads of `a + b` came out the other way round
                 self._site("_a", 0x200, 0x08), self._site("_b", 0x100, 0x0f)]
        self.assertEqual(conflicts(sites, {}), [])

    def test_members_of_one_claimed_object_can_swap(self):
        from homm1.verify.data_identity import Site, conflicts
        sites = [Site("u", "_F", 0x10, "_image", "_image", False,
                      8, 0x1004, "image+4"),
                 Site("u", "_F", 0x17, "_image", "_image", False,
                      4, 0x1008, "image+8")]
        self.assertEqual(conflicts(sites, {"_image": {("u", 0x1000)}}), [])

    def test_member_swap_does_not_hide_wrong_or_duplicate_member(self):
        from homm1.verify.data_identity import Site, conflicts
        for second in (0x1004, 0x100c):
            sites = [Site("u", "_F", 0x10, "_image", "_image", False,
                          8, 0x1004, "image+4"),
                     Site("u", "_F", 0x17, "_image", "_image", False,
                          4, second, "wrong")]
            self.assertTrue(conflicts(sites, {"_image": {("u", 0x1000)}}))

    def test_member_swap_cannot_cross_functions(self):
        from homm1.verify.data_identity import Site, conflicts
        sites = [Site("u", "_F", 0x10, "_image", "_image", False,
                      8, 0x1004, "image+4"),
                 Site("u", "_G", 0x17, "_image", "_image", False,
                      4, 0x1008, "image+8")]
        self.assertTrue(conflicts(sites, {"_image": {("u", 0x1000)}}))

    def test_two_names_one_datum_still_fails(self):
        from homm1.verify.data_identity import conflicts
        sites = [self._site("_a", 0x100, 0x10, "_G"), self._site("_b", 0x100, 0x10, "_H")]
        self.assertTrue(any("2 source symbols" in f for f in conflicts(sites, {})))

    def test_one_sided_mismatch_still_fails(self):
        from homm1.verify.data_identity import conflicts
        sites = [self._site("_a", 0x100, 0x10, "_G"), self._site("_a", 0x100, 0x30, "_H"),
                 self._site("_a", 0x300, 0x08)]
        self.assertTrue(any("reaches 2 retail bases" in f for f in conflicts(sites, {})))

    def test_rotation_of_hoisted_loads_is_excluded(self):
        from homm1.verify.data_identity import conflicts
        # three invariants loaded in a different order: each name lands on
        # another's claimed base, and together they cover exactly those bases
        sites = [self._site("_r", 0x200, 0x08), self._site("_g", 0x300, 0x10),
                 self._site("_b", 0x100, 0x18)]
        claims = {"_r": {("u", 0x100)}, "_g": {("u", 0x200)}, "_b": {("u", 0x300)}}
        self.assertEqual(conflicts(sites, claims), [])

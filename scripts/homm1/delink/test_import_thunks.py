"""Indirect jumps inside a real function must not become import functions."""
import io
import struct
import unittest
from types import SimpleNamespace
from unittest.mock import patch

from homm1.delink.pdb_synth import emit_yaml, game_site_test, import_thunk_names, reloc_target_refs


class ImportThunkTests(unittest.TestCase):
    def test_unclassified_code_does_not_gain_library_data_exemption(self):
        image = SimpleNamespace(pe=SimpleNamespace(text_span=lambda: (0x1000, 0x4000)))
        model = SimpleNamespace(functions=[SimpleNamespace(
            rva=0x2000, size=0x20, channel="functions_static_libs")])
        with patch("homm1.delink.pdb_synth.retail", return_value=image), \
                patch("homm1.delink.pdb_synth.band_lookup", return_value=
                      lambda site: "crt" if 0x3000 <= site < 0x3800 else ""):
            is_game = game_site_test(model)
            self.assertTrue(is_game(0x1100))
            self.assertFalse(is_game(0x2004))
            self.assertFalse(is_game(0x3100))
            self.assertFalse(is_game(0x5000))

    def test_import_pdb_coordinates_follow_real_pe_section(self):
        for separate in (False, True):
            sections = [dict(name=name, va=va, vsize=0x1000) for name, va in
                        [(".text", 0x1000), (".rdata", 0x3000), (".data", 0x5000)]]
            if separate:
                sections.append(dict(name=".idata", va=0x7000, vsize=0x1000))
            slot = 0x7104 if separate else 0x3104
            expected_segment = 4 if separate else 2
            image = SimpleNamespace(pe=SimpleNamespace(sections=sections))
            bounds = {x["name"]: (x["va"], x["va"] + x["vsize"]) for x in sections}
            output = io.StringIO()
            with self.subTest(separate=separate), \
                    patch("homm1.delink.pdb_synth.retail", return_value=image), \
                    patch("homm1.delink.pdb_synth.sections_of", return_value=bounds):
                emit_yaml([], [], [], [(slot, "__imp__GetACP@0")], {}, output)
                self.assertIn("Offset:          260", output.getvalue())
                self.assertIn(f"Segment:         {expected_segment}", output.getvalue())
                self.assertIn("DisplayName:     '__imp__GetACP@0'", output.getvalue())

    def test_embedded_iat_does_not_receive_ordinary_data_fences(self):
        directories = [(0, 0)] * 16
        directories[12] = (0x3100, 0x20)
        image = SimpleNamespace(pe=SimpleNamespace(directories=directories),
                                reloc_sites=[0x1001, 0x1008], image_base=0x400000,
                                u32=lambda site: {0x1001: 0x403104, 0x1008: 0x403120}[site])
        with patch("homm1.delink.pdb_synth.retail", return_value=image), \
                patch("homm1.delink.pdb_synth.sections_of", return_value={
                    ".rdata": (0x3000, 0x4000), ".data": (0x5000, 0x6000)}):
            self.assertEqual(reloc_target_refs(), {0x3120: [0x1008]})

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

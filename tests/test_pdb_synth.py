"""Structural checks for the synthetic delinker PDB."""

import io
import unittest
from unittest import mock

from homm1.delink import pdb_synth


class PdbSynthTests(unittest.TestCase):
    def test_each_source_owns_a_separate_dbi_module(self):
        bounds = {
            ".text": (0x1000, 0x2000),
            ".rdata": (0x3000, 0x4000),
            ".data": (0x4000, 0x5000),
            ".idata": (0x5000, 0x6000),
        }
        functions = [
            (0x1010, 7, "First"),
            (0x1020, 9, "Second"),
            (0x1030, 5, "Unclaimed"),
        ]
        names = {
            0x1010: ("First", "BASE/BITS", 7),
            0x1020: ("Second", "SOURCE/HERO", 9),
        }
        output = io.StringIO()

        with mock.patch.object(pdb_synth, "sections_of", return_value=bounds):
            pdb_synth.emit_yaml(
                functions,
                [(0x3010, "ReadOnly")],
                [(0x4010, "Writable")],
                [(0x5010, "__imp_Imported")],
                names,
                output,
            )

        yaml = output.getvalue()
        bits = yaml.index("Module:          'c:\\proj\\BASE\\BITS'")
        hero = yaml.index("Module:          'c:\\proj\\SOURCE\\HERO'")
        bucket = yaml.index("Module:          'c:\\proj\\seg_0000.cpp'")
        data = yaml.index("Module:          'c:\\proj\\_data'")
        modules = (yaml[bits:hero], yaml[hero:bucket], yaml[bucket:data], yaml[data:])

        self.assertIn("DisplayName:     'First'", modules[0])
        self.assertNotIn("DisplayName:     'Second'", modules[0])
        self.assertIn("DisplayName:     'Second'", modules[1])
        self.assertIn("DisplayName:     'Unclaimed'", modules[2])
        self.assertNotIn("Kind:            S_LDATA32", "".join(modules[:3]))
        self.assertEqual(modules[3].count("Kind:            S_LDATA32"), 3)


if __name__ == "__main__":
    unittest.main()

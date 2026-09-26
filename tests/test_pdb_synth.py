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


# Unchanged donor controls for the adopted data-fence relaxation and debt writer.
import tempfile
from pathlib import Path
from homm1.core.tsv import read as read_tsv

def fences():
    rdata = [(0x1000, "UNPROVISIONED_00401000"), (0x1010, "DAT_00401010"),
             (0x1020, "??_C@_03ABCD@abc?$AA@")]
    data = [(0x2000, "_g_named"), (0x2004, "UNPROVISIONED_00402004")]
    return rdata, data


class FenceSpellingTest(unittest.TestCase):
    def test_strict_keeps_the_refused_spelling(self):
        rdata, data = fences()
        self.assertEqual(pdb_synth.relax_fences(rdata, data, True), 0)
        self.assertEqual((rdata, data), fences())

    def test_relaxed_respells_only_unprovisioned_fences(self):
        rdata, data = fences()
        self.assertEqual(pdb_synth.relax_fences(rdata, data, False), 2)
        self.assertEqual(rdata[0], (0x1000, "DAT_00401000"))
        self.assertEqual(data[1], (0x2004, "DAT_00402004"))
        self.assertEqual(rdata[1:], fences()[0][1:])      # others untouched
        self.assertEqual(data[0], fences()[1][0])
        self.assertFalse(any(name.startswith("UNPROVISIONED_")
                             for name in (n for _r, n in rdata + data)))


class DataDebtTest(unittest.TestCase):
    ROWS = [{"rva": 0x2004, "sites": [0x1234, 0x1300], "bands": ["cmdline"],
             "units": ["party"], "census": None}]

    def _write(self, rows, on):
        with tempfile.TemporaryDirectory() as td:
            path = Path(td) / "data_debt.tsv"
            with mock.patch("homm1.core.data_matching.enabled",
                            return_value=on):
                pdb_synth.write_data_debt(rows, path)
            return read_tsv(path)

    def test_the_worklist_is_written_in_both_modes(self):
        for on in (True, False):
            banner, header, rows = self._write(self.ROWS, on)
            self.assertEqual(header, ["rva", "units", "bands", "sites", "census"])
            self.assertEqual(rows, [{"rva": "0x002004", "units": "party",
                                     "bands": "cmdline",
                                     "sites": "0x001234,0x001300",
                                     "census": "census=?"}])
            self.assertTrue(any(("true" if on else "false") in line
                                for line in banner))

    def test_an_empty_worklist_is_still_written(self):
        _banner, header, rows = self._write([], False)
        self.assertEqual((header[0], rows), ("rva", []))


if __name__ == "__main__":
    unittest.main()

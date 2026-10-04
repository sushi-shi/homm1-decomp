"""Assembly metadata must retain ownership and participate in the normal model."""
import tempfile
import unittest
from pathlib import Path
from homm1.graph.fixed_asm import _read_units


class FixedAsmTests(unittest.TestCase):
    def read(self, rows):
        with tempfile.TemporaryDirectory() as tmp:
            p = Path(tmp) / "asm.tsv"
            p.write_text("unit\tsource\trva\tsize\tname\tkind\n" + rows)
            return _read_units(p)

    def test_retail_table_preserves_per_unit_claims(self):
        units = self.read("BASE/A\tsrc/A.asm\t0x1234\t0x20\t_f\tfunc\n"
                          "BASE/B\tsrc/B.asm\t0x5678\t0x04\t_g\tdata\n")
        self.assertEqual(units["BASE/A"].claims[0].va, 0x401234)
        self.assertEqual(units["BASE/B"].claims[0].kind, "data")
        self.assertEqual(units["BASE/B"].source, "src/B.asm")

    def test_conflicting_source_is_rejected(self):
        with self.assertRaisesRegex(ValueError, "inconsistent assembly source"):
            self.read("BASE/A\ta.asm\t0x1000\t0x10\t_f\tfunc\n"
                      "BASE/A\tb.asm\t0x2000\t0x10\t_g\tfunc\n")

    def test_duplicate_name_and_empty_extent_are_rejected(self):
        first = "BASE/A\ta.asm\t0x1000\t0x10\t_f\tfunc\n"
        for second in ("BASE/A\ta.asm\t0x2000\t0x10\t_f\tfunc\n",
                       "BASE/A\ta.asm\t0x2000\t0x00\t_g\tfunc\n"):
            with self.assertRaisesRegex(ValueError, "invalid or duplicate"):
                self.read(first + second)


if __name__ == "__main__":
    unittest.main()

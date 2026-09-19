# Ported from HoMM2 Gold 2.1 Buka at 299514f88900c0cf30ba03422c72830a38fc1cb7.
import unittest

from homm1.match.residual_queue import residual_rows, campaign_rows


class ResidualQueueTest(unittest.TestCase):
    def test_homm1_relocation_failure_remains_queued_at_full_fuzzy_score(self):
        rows = campaign_rows(dict(functions=[dict(unit='a', symbol='_f', rva=0x1000,
                                                retail_size=20, score=100, exact=False)]))
        self.assertEqual([row['rva'] for row in rows], [0x1000])
    def test_orders_every_nonexact_function_highest_fuzzy_then_rva(self):
        report = {"units": [
            {"name": "SOURCE/A", "functions": [
                {"name": "a", "size": "10", "fuzzy_match_percent": 98.0,
                 "metadata": {"demangled_name": "A"}},
                {"name": "b", "size": "20", "fuzzy_match_percent": 100.0},
            ]},
            {"name": "BASE/B", "functions": [
                {"name": "c", "size": "30", "fuzzy_match_percent": 99.5},
            ]},
        ]}
        symbols = {
            ("SOURCE/A", "a"): {"rva": "0x200"},
            ("SOURCE/A", "b"): {"rva": "0x210"},
            ("BASE/B", "c"): {"rva": "0x100"},
        }
        rows = residual_rows(report, symbols)
        self.assertEqual([row["name"] for row in rows], ["c", "a"])
        self.assertEqual([row["rank"] for row in rows], [1, 2])

    def test_missing_rva_inventory_fails_closed(self):
        report = {"units": [{"name": "SOURCE/A", "functions": [
            {"name": "a", "size": "10", "fuzzy_match_percent": 99.0},
        ]}]}
        with self.assertRaisesRegex(ValueError, "lack an RVA inventory"):
            residual_rows(report, {})


if __name__ == "__main__":
    unittest.main()

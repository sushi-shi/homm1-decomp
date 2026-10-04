"""Negative controls for conservative baseline reporting."""
import json
import tempfile
import unittest
from pathlib import Path
from unittest.mock import patch

from homm1.compare.baseline import evidence, reference_reason, totals
from homm1.verify.scores import load


class BaselineTests(unittest.TestCase):
    def test_unscored_and_unmapped_stay_in_denominator(self):
        rows = [dict(census_size=100, size=80, score=100., status="scored"),
                dict(census_size=300, size=300, score=100., status="unmapped")]
        result = totals(rows)
        self.assertEqual(result["functions"], 2)
        self.assertEqual(result["exact_functions"], 1)
        self.assertEqual(result["exact_percent"], 50.)
        self.assertEqual(result["fuzzy_percent"], 20.)
        self.assertEqual(result["unscored_reasons"], {"unmapped": 1})

    def test_unknown_name_never_becomes_proven_even_if_listed(self):
        for name in ("UNPROVISIONED_00412345", "DAT_00412345", "FUN_00412345"):
            self.assertEqual(reference_reason(name, 1, 0x12345, 0, 10,
                                              {(1, 0x12345): {name}},
                                              {0x12345: {name}}),
                             "unprovided reference")

    def test_conflicting_and_wrong_site_proofs_are_withheld(self):
        for proofs in ({(1, 100): {"a", "b"}}, {(2, 100): {"a"}},
                       {(1, 101): {"a"}}):
            self.assertTrue(reference_reason("a", 1, 100, 0, 10, proofs, {}))
        self.assertEqual(reference_reason("a", 1, 100, 0, 10,
                                          {(1, 100): {"a"}}, {}), "")

    def test_other_image_evidence_is_not_imported(self):
        with tempfile.TemporaryDirectory() as tmp:
            p = Path(tmp) / "facts.json"
            p.write_text(json.dumps(dict(image_sha256="old", functions=[
                dict(name="f", rva="0x10", size=4, unit="SOURCE/F")],
                refs=[dict(site="0x11", target="0x100", name="g", kind=6)])))
            self.assertEqual(evidence([p], "new"), (set(), {}))
            functions, refs = evidence([p], "old")
            self.assertIn((0x10, "f"), functions)
            self.assertEqual(refs[(0x11, 0x100)], {"g"})

    def test_readme_rejects_a_stale_baseline(self):
        from homm1.compare import baseline
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            (root / "baseline.json").write_text(json.dumps(dict(input_digest="old")))
            with patch.object(baseline, "ROOT", root), patch.object(
                    baseline, "input_digest", return_value="new"):
                with self.assertRaisesRegex(ValueError, "baseline is stale"):
                    baseline.readme()

    def test_diagnostic_report_cannot_enter_verified_score_loader(self):
        with tempfile.TemporaryDirectory() as tmp:
            p = Path(tmp) / "report.json"
            p.write_text(json.dumps(dict(units=[], homm1_diagnostic=True)))
            with self.assertRaisesRegex(SystemExit, "cannot be banked"):
                load(p)


if __name__ == "__main__":
    unittest.main()

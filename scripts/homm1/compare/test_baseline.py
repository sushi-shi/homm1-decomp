"""Controls for diagnostic score reporting and independent reference review."""
import json
import tempfile
import unittest
from pathlib import Path
from types import SimpleNamespace
from unittest.mock import patch
from contextlib import ExitStack

from homm1.compare.baseline import comparison_rows, evidence, module_table, reference_reason, reconstruction_census, totals
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

    def test_module_table_keeps_unscored_and_unknown_owners(self):
        rows = [dict(unit="SOURCE/A", census_size=100, size=100,
                     score=100., status="scored"),
                dict(unit="SOURCE/A", census_size=300, size=300,
                     score=100., status="missing source comparison"),
                dict(unit="BASE/LZHUF", census_size=40, size=40,
                     score=100., status="scored"),
                dict(unit="", census_size=60, size=60,
                     score=0., status="unmapped function")]
        lines = module_table(rows, {"SOURCE/A": "src/SOURCE/A.cpp",
                                   "BASE/LZHUF": "vendor/lzhuf/lzhuf.asm"})
        cells = [[c.strip() for c in line.strip("|").split("|")]
                 for line in lines[2:]]
        self.assertIn(["`SOURCE`", "1", "1 / 2 (50.0%)", "25.0%"], cells)
        self.assertIn(["`lzhuf`", "1", "1 / 1 (100.0%)", "100.0%"], cells)
        self.assertIn(["`(unmapped)`", "—", "0 / 1 (0.0%)", "0.0%"], cells)

    def test_only_source_bodies_enter_matching_without_address_exclusions(self):
        channels = ["src", "src_compgen", "functions_referents",
                    "functions_static_libs", "", "src_decl", "src_dyninit"]
        bindings = [SimpleNamespace(rva=i + 1, channel=channel)
                    for i, channel in enumerate(channels)]
        census = [dict(rva=b.rva, kind="", size=17) for b in bindings]
        selected = reconstruction_census(census, bindings)
        self.assertEqual([r["rva"] for r in selected], [1, 2])
        # A missing comparison still counts as zero once a body is annotated.
        rows = [dict(unit="SOURCE/A", census_size=r["size"], size=r["size"],
                     score=0., status="missing source comparison")
                for r in selected]
        self.assertEqual(totals(rows)["functions"], 2)
        self.assertEqual(totals(rows)["scored_functions"], 0)
        lines = module_table(rows, {"SOURCE/A": "src/SOURCE/A.cpp"})
        self.assertFalse(any("CRT" in line for line in lines))
        # Selection does not erase identities from the structural inventory.
        self.assertEqual(len(census), len(channels))

    def test_measured_partial_and_zero_scores_survive_without_review_credit(self):
        bindings = [SimpleNamespace(rva=i, name=name, unit="SOURCE/A", size=100)
                    for i, name in enumerate(("partial", "zero", "missing"))]
        census = [dict(rva=b.rva, size=100) for b in bindings]
        rows = comparison_rows(census, bindings,
                               {("A", "partial"): 97.5, ("A", "zero"): 0.0})
        self.assertEqual([r["score"] for r in rows], [97.5, 0.0, 0.0])
        self.assertEqual([r["status"] for r in rows],
                         ["scored", "scored", "missing source comparison"])
        self.assertEqual(totals(rows)["scored_functions"], 2)
        self.assertEqual(totals(rows)["fuzzy_percent"], 32.5)

    def test_reference_audit_collects_later_failures_and_missing_absolute_sites(self):
        from homm1.compare import baseline
        from homm1.delink import pdb_synth
        binding = SimpleNamespace(rva=0x100, name="body", unit="SOURCE/A", size=32)
        obj = SimpleNamespace(
            symbols={0: SimpleNamespace(name="body", section=1, value=0),
                     1: SimpleNamespace(name="UNPROVISIONED_00402000", section=0),
                     2: SimpleNamespace(name="unreviewed", section=0)},
            relocations=[SimpleNamespace(section=1, site=4, typ=6, symbol_index=1),
                         SimpleNamespace(section=1, site=8, typ=6, symbol_index=2)])
        pe = SimpleNamespace(image_base=0x400000,
                             read=lambda site, size: (0x402000).to_bytes(4, "little"),
                             highlow_sites=lambda path: {0x104, 0x108, 0x10c})
        with tempfile.TemporaryDirectory() as tmp, ExitStack() as stack:
            root = Path(tmp)
            (root / "SOURCE").mkdir()
            (root / "SOURCE/A.c.obj").write_bytes(b"mock object")
            stack.enter_context(patch.object(baseline, "CoffObject", return_value=obj))
            stack.enter_context(patch.object(baseline, "evidence", return_value=(set(), {})))
            stack.enter_context(patch.object(baseline, "read_tsv", return_value=([], [], [])))
            stack.enter_context(patch.object(pdb_synth, "referent_function_names", return_value={}))
            stack.enter_context(patch.object(pdb_synth.implib, "resolve_iat", return_value=([], [])))
            stack.enter_context(patch.object(pdb_synth, "retail",
                                            return_value=SimpleNamespace(import_slots=lambda: [])))
            stack.enter_context(patch.object(pdb_synth, "import_thunk_names", return_value={}))
            audit = baseline.audit_references([dict(rva=0x100)],
                    SimpleNamespace(functions=[binding]), root, pe, "image")
        self.assertEqual(audit["summary"]["reference_sites"], 2)
        self.assertEqual(audit["summary"]["functions_with_issues"], 1)
        self.assertEqual(audit["summary"]["reasons"], {
            "unreviewed function identity": 1, "unprovided reference": 1,
            "unreviewed or conflicting reference": 1, "missing absolute relocation": 1})

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

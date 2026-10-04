"""Reviewed ownership must not manufacture source bodies or extents."""
import tempfile
import unittest
from pathlib import Path
from unittest.mock import patch

from homm1 import model
from homm1.retail_labels import Claim, providers


class OwnershipTests(unittest.TestCase):
    def resolve(self, claims):
        rows = [dict(rva=rva, size=16, kind="") for rva in (0x100, 0x110, 0x120)]
        with patch.object(model.censuses, "functions", return_value=rows), patch.object(
                model.censuses, "data", return_value=[]), patch.object(
                model.censuses, "link_order_bands", return_value=[(0x100, 0x120, "BASE/A")]), patch.object(
                model.providers, "all_claims", return_value=claims), patch.object(
                model.src_claims, "all_claims", return_value=[]), patch.object(
                model, "_emitted", return_value=lambda unit, name: None):
            return model.resolve()

    def test_ownership_keeps_anonymous_bodies_anonymous_and_gap_unowned(self):
        result = self.resolve([])
        self.assertEqual(result.violations, [])
        self.assertEqual([b.unit for b in result.functions], ["BASE/A", "BASE/A", ""])
        self.assertTrue(all(not b.name and not b.channel for b in result.functions))
        self.assertEqual(result.claimed("func"), [])

    def test_referent_gets_owner_but_no_source_channel_or_exact_extent(self):
        result = self.resolve([Claim(0x100, "f", "func", "functions_referents", 4, "", {})])
        b = result.functions[0]
        self.assertEqual((b.name, b.unit, b.size, b.channel), ("f", "BASE/A", 16, "functions_referents"))
        self.assertEqual(result.violations, [])

    def test_source_body_wins_over_referent_and_band_owner(self):
        result = self.resolve([
            Claim(0x100, "f", "func", "functions_referents", None, "", {}),
            Claim(0x100, "f", "func", "src", 7, "BASE/B", {}),
        ])
        b = result.functions[0]
        self.assertEqual((b.unit, b.size, b.channel), ("BASE/B", 7, "src"))
        self.assertEqual(b.aliases[0].channel, "functions_referents")

    def test_referent_must_hit_an_admitted_start(self):
        result = self.resolve([Claim(0x105, "f", "func", "functions_referents", None, "", {})])
        self.assertTrue(any("not an admitted census row" in v for v in result.violations))

    def test_provider_does_not_turn_a_linker_name_into_a_source_claim(self):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "referents.tsv"
            path.write_text("rva\tname\tprovenance\n0x100\tf\treviewed call\n")
            claim, = providers.functions_referents(path)
        self.assertEqual((claim.rva, claim.name, claim.unit, claim.size), (0x100, "f", "", None))
        self.assertEqual(claim.channel, "functions_referents")

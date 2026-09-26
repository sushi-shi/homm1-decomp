from __future__ import annotations

import unittest
from pathlib import Path
from unittest import mock

from homm1 import cli
from homm1.permute import campaign


class CampaignRankingTests(unittest.TestCase):
    def row(self, *, classification="regalloc", cur=90.0, proven=False, rva="0x10"):
        return {
            "classification": classification,
            "cur": cur,
            "proven": proven,
            "hist_max": cur,
            "rva": rva,
        }

    def test_proven_dip_precedes_high_current_unproven(self):
        proven = self.row(classification="cfg", cur=80.0, proven=True)
        high = self.row(classification="regalloc", cur=99.0)
        self.assertLess(campaign.campaign_priority(proven), campaign.campaign_priority(high))

    def test_same_score_prefers_regalloc_then_cfg_then_identity(self):
        ordered = sorted([
            self.row(classification="referent"),
            self.row(classification="cfg"),
            self.row(classification="regalloc"),
        ], key=campaign.campaign_priority)
        self.assertEqual(
            [row["classification"] for row in ordered],
            ["regalloc", "cfg", "referent"],
        )

    def test_default_depth_routes_structural_classes_to_source_shapes(self):
        self.assertEqual(campaign.default_depth("regalloc", False), 0)
        self.assertEqual(campaign.default_depth("cfg", False), 2)
        self.assertEqual(campaign.default_depth("inline", False), 2)
        self.assertEqual(campaign.default_depth("cfg", True), 0)

    def test_explicit_campaign_rvas_are_normalized_before_classification(self):
        args = mock.Mock(
            rva=[0x560450], unit=None, below=100.0, output=None, targets=1,
        )
        with mock.patch.object(campaign, "classified_candidates", return_value=[]) as classify:
            with self.assertRaises(ValueError):
                campaign.run_campaign(args)
        classify.assert_called_once_with(unit=None, below=100.0, rvas={0x160450})

    def test_whole_object_disassembly_is_split_by_symbol(self):
        output = """\
00000000 <first>:
       0: 90                           \tnop
00000001 <$L1>:
       1: 90                           \tnop
00000002 <$dispatch$1>:
       2: 90                           \tnop
00000000 <second>:
       0: c3                           \tret
"""
        completed = mock.Mock(returncode=0, stdout=output, stderr="")
        with mock.patch.object(campaign.subprocess, "run", return_value=completed):
            assemblies = campaign.object_assemblies(Path("unit.obj"), {"first", "second"})
        self.assertEqual(set(assemblies), {"first", "second"})
        self.assertIn("nop", assemblies["first"])
        self.assertIn("1: 90", assemblies["first"])
        self.assertNotIn("ret", assemblies["first"])

    def test_precomputed_skeleton_trims_padding_at_function_extent(self):
        assembly = """\
00000010 <fn>:
      10: e8 00 00 00 00             \tcall\t0x15
      15: 74 01                       \tje\t0x18
      17: c3                          \tret
      18: 90                          \tnop
"""
        masked, calls, branches, returns, instructions, _text = \
            campaign.assembly_skeleton(b"\xe8\0\0\0\0\x74\x01\xc3", {}, assembly, set())
        self.assertEqual(masked, b"\xe8\0\0\0\0\x74\x01\xc3")
        self.assertEqual((calls, branches, returns, instructions), (1, 1, 1, 3))
        self.assertIn("0:\tcall 0x15", _text)

    def test_completion_routes_single_island_targets_to_structural_search(self):
        results = [{
            "candidate": {"rva": "0x182610"},
            "exact": False,
            "baseline_score": 99.98,
            "best_score": 99.98,
            "search_route": "structural",
        }]
        message = campaign.completion_message(results, Path("/tmp/frontier"))
        self.assertIn("only a single compiler island was found", message)
        self.assertIn("next search should be structural: 0x182610", message)

    def test_completion_keeps_frontier_route_for_multiple_islands(self):
        results = [{
            "candidate": {"rva": "0x160450"},
            "exact": True,
            "baseline_score": 99.98,
            "best_score": 100.0,
            "search_route": "inspect-frontier",
        }]
        message = campaign.completion_message(results, Path("/tmp/frontier"))
        self.assertIn("inspect M-frontiers under /tmp/frontier", message)
        self.assertNotIn("structural", message)


class CampaignCliTests(unittest.TestCase):
    def test_public_candidates_verb_bypasses_exact_wall_gate(self):
        with mock.patch("homm1.permute.campaign.main", return_value=23) as run:
            result = cli.main(["permute", "candidates", "--limit", "5"])
        self.assertEqual(result, 23)
        run.assert_called_once_with(["candidates", "--limit", "5"])


if __name__ == "__main__":
    unittest.main()

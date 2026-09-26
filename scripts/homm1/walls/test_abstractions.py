from __future__ import annotations

import unittest
from unittest.mock import patch

from homm1.walls import abstractions


class SourceAbstractionTests(unittest.TestCase):
    def test_function_body_is_brace_balanced_and_ignores_literal_braces(self):
        source = '''
RVA(0x00123450, 0x20)
int F() {
    const char* text = "}"; // }
    if (text) { return 1; }
    return 0;
}
RVA(0x00123470, 0x10)
int G() { return 2; }
'''
        body = abstractions.function_body(source, 0x123450)
        self.assertIn("if (text)", body)
        self.assertNotIn("int G", body)

    def test_hidden_inline_or_macro_origin_precedes_regalloc(self):
        origin = {"promote": True, "inlines": [
            {"name": "Pack", "count": 2, "origin": "local"}], "macros": []}
        level, evidence = abstractions.choose_level("regalloc", [], origin, False)
        self.assertEqual(level, "textual")
        self.assertTrue(any("inline Pack x2" in item for item in evidence))

    def test_aggregate_lead_precedes_call_set_classification(self):
        level, evidence = abstractions.choose_level(
            "inline", ["aggregate-read:under@+0x20"], {"promote": False}, False)
        self.assertEqual(level, "object")
        self.assertEqual(evidence, ["aggregate-read:under@+0x20"])

    def test_argument_copy_shape_does_not_route_as_an_object_lead(self):
        from homm1.walls import aggregate_copies, aggdecl, aggscan, valuetemp

        arg_row = (88.65, "rezsync", "Run", 0x83450, 0x108,
                   ["SEP"], ["ARG"])
        empty_decl = ([], [], {}, 1, 0, 0)
        with patch.object(aggdecl, "scan", side_effect=[
                ([], [arg_row], {}, 1, 1, 0), empty_decl]), \
             patch.object(aggregate_copies, "scan", return_value=[]), \
             patch.object(valuetemp, "scan", return_value=([], [], [], [], [])), \
             patch.object(aggscan, "sweep", return_value={
                 "ours": [], "both": [], "retail": []}), \
             patch.object(aggscan, "perfunction", return_value={}):
            leads = abstractions.aggregate_leads()
        self.assertNotIn(("rezsync", "Run"), leads)

    def test_historical_max_remains_primary_queue_order(self):
        low_expression = {"hist_max": 70.0, "level": "expression",
                          "cur": 70.0, "rva": "0x20"}
        high_identity = {"hist_max": 80.0, "level": "identity",
                         "cur": 80.0, "rva": "0x10"}
        self.assertLess(abstractions.queue_priority(low_expression),
                        abstractions.queue_priority(high_identity))

    def test_generated_funclet_is_never_a_source_abstraction_claim(self):
        level, evidence = abstractions.choose_level(
            "regalloc", ["aggregate-copy-count"], {"promote": True}, True)
        self.assertEqual(level, "generated")
        self.assertEqual(evidence, ["EH-band funclet"])

    def test_state_is_a_terminal_routing_level(self):
        self.assertIn("state", abstractions.LEVEL_ORDER)
        self.assertIn("do not rewrite proven source", abstractions.NEXT_ACTION["state"])


if __name__ == "__main__":
    unittest.main()

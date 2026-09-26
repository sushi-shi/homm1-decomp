from __future__ import annotations

import json
import tempfile
import unittest
from pathlib import Path
from unittest import mock

import clang.cindex as ci

from homm1.permute import batch_source_variants as batch
from homm1.permute.generate_ast_variants import (
    AstEdit,
    AstMutation,
    candidate_payloads,
    clang_args,
    configure_libclang,
    crossed_candidate_payloads,
    declaration_hoist_edits,
    marker_span,
    non_overlapping,
    target_function,
)
from homm1.permute.topology import compare_topology, topology_rank


class BatchSourceVariantTests(unittest.TestCase):
    def test_exact_axes_form_a_cartesian_product(self):
        original = b"left + right\n"
        left = batch.Axis("left", 0, 4, b"left", (
            batch.AxisOption("keep", b"left"),
            batch.AxisOption("rename", b"first"),
        ))
        right = batch.Axis("right", 7, 12, b"right", (
            batch.AxisOption("keep", b"right"),
            batch.AxisOption("rename", b"second"),
        ))
        variants = list(batch.iter_variants(original, (left, right), ()))
        self.assertEqual(len(variants), 4)
        self.assertEqual(variants[-1][0], b"first + second\n")
        self.assertEqual(
            variants[-1][1], {"left": "rename", "right": "rename"}
        )

    def test_manifest_requires_unique_exact_spans_and_nonoverlap(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            source = root / "src/unit.cpp"
            source.parent.mkdir(parents=True)
            source.write_text("int value = left + right;\n")
            manifest = root / "axes.json"
            manifest.write_text(json.dumps({
                "schema": 1,
                "source": "src/unit.cpp",
                "rva": "0x1234",
                "axes": [{
                    "name": "order",
                    "find": "left + right",
                    "options": [
                        {"name": "keep"},
                        {"name": "swap", "replace": "right + left"},
                    ],
                }],
            }))
            _payload, _source, _original, axes, candidates, rva = \
                batch.load_manifest(manifest, root)
            self.assertEqual(rva, 0x1234)
            self.assertEqual(len(axes), 1)
            self.assertEqual(candidates, ())

            payload = json.loads(manifest.read_text())
            payload["axes"].append(dict(payload["axes"][0], name="overlap"))
            manifest.write_text(json.dumps(payload))
            with self.assertRaisesRegex(ValueError, "axes overlap"):
                batch.load_manifest(manifest, root)

    def test_axis_option_extra_edit_is_atomic(self):
        original = b"int helper;\nint result = old_call;\n"
        axis = batch.Axis("call", 25, 33, b"old_call", (
            batch.AxisOption("keep", b"old_call"),
            batch.AxisOption(
                "helper", b"new_call",
                (batch.Edit(0, 0, b"", b"static int new_call;\n"),),
            ),
        ))
        variants = list(batch.iter_variants(original, (axis,), ()))
        self.assertEqual(variants[1][1], {"call": "helper"})
        self.assertEqual(
            variants[1][0],
            b"static int new_call;\nint helper;\nint result = new_call;\n",
        )

    def test_disposable_sibling_is_removed_after_compile(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            source = root / "src" / "unit.cpp"
            source.parent.mkdir()
            source.write_bytes(b"original\n")
            scratch = root / "scratch"
            scratch.mkdir()

            def inspect_source(_root, probe, output, _flags, _timeout):
                self.assertEqual(probe.read_bytes(), b"candidate\n")
                self.assertEqual(output, scratch / "trial-0007.obj")
                return True, "", False

            with mock.patch.object(batch, "compile_object", side_effect=inspect_source):
                result = batch.compile_disposable_sibling(
                    root, source, scratch, 7, b"candidate\n", [], 12.0
                )
            self.assertEqual(result, (7, (True, "", False)))
            self.assertFalse((source.parent / ".unit.sourcevariant0007.cpp").exists())

    def test_parallel_precompile_deduplicates_identical_sources(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            source = root / "unit.cpp"
            source.write_bytes(b"original\n")
            scratch = root / "scratch"
            scratch.mkdir()
            variants = [
                (b"first\n", {"shape": "a"}),
                (b"first\n", {"shape": "duplicate"}),
                (b"second\n", {"shape": "b"}),
            ]

            def compiled(_root, _source, _scratch, index, candidate, _flags, _timeout):
                return index, (True, candidate.decode().strip(), False)

            with mock.patch.object(batch, "compile_disposable_sibling", side_effect=compiled) as call:
                results = batch.precompile_variants(
                    root, source, scratch, variants, [], 12.0, 2
                )
            self.assertEqual(
                results,
                {0: (True, "first", False), 2: (True, "second", False)},
            )
            self.assertEqual(call.call_count, 2)

    def test_result_rank_prefers_score_then_size_then_relocations(self):
        best = {"score": 99.0, "candidate_size": 10, "candidate_relocs": 2, "trial": 2}
        lower = {"score": 98.0, "candidate_size": 10, "candidate_relocs": 2, "trial": 1}
        self.assertLess(batch.result_rank(best, 10, 2), batch.result_rank(lower, 10, 2))

    def test_frontier_keeps_highest_distinct_states(self):
        with tempfile.TemporaryDirectory() as directory:
            scratch = Path(directory)
            candidate = scratch / "candidate.obj"
            frontier = {}
            for state, score in (("a", 90), ("b", 95), ("a", 92), ("c", 91)):
                candidate.write_bytes(state.encode())
                row = {"score": score}
                batch.retain_frontier_candidate(
                    frontier, 2, state, (-score,), row, state.encode(),
                    candidate, scratch,
                )
            self.assertEqual(set(frontier), {"a", "b"})
            self.assertEqual(frontier["a"]["row"]["score"], 92)

    def test_a_flat_multi_variant_census_routes_to_structural_search(self):
        self.assertEqual(batch.search_route(1, 129), "structural")

    def test_multiple_islands_route_to_frontier_inspection(self):
        self.assertEqual(batch.search_route(4, 129), "inspect-frontier")

    def test_one_executed_variant_does_not_claim_a_flat_island(self):
        self.assertEqual(batch.search_route(1, 1), "expand-campaign")


class AstVariantTests(unittest.TestCase):
    def test_clang_args_accept_msvc_address_of_temporary(self):
        configure_libclang()
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            source = root / "src/unit.cpp"
            source.parent.mkdir(parents=True)
            source.write_text("struct Value {}; void Use(Value*); void F() { Use(&Value()); }\n")
            tu = ci.Index.create().parse(str(source), args=clang_args(root, source))
            errors = [
                str(diagnostic) for diagnostic in tu.diagnostics
                if diagnostic.severity >= ci.Diagnostic.Error
            ]
            self.assertEqual(errors, [])

    def test_declaration_hoist_after_same_line_open_brace_is_valid_cpp(self):
        blob = (
            b"#define RVA(rva, size)\n"
            b"RVA(0x00123456, 0x1)\n"
            b"int Target() {\n"
            b"    int value = 7;\n"
            b"    return value;\n"
            b"}\n"
        )
        configure_libclang()
        with tempfile.TemporaryDirectory() as directory:
            source = Path(directory) / "unit.cpp"
            source.write_bytes(blob)
            index = ci.Index.create()
            tu = index.parse(str(source), args=["-x", "c++", "-std=c++14"])
            fn = target_function(tu, source, blob, 0x123456)
            mutations = declaration_hoist_edits(fn, blob)
            self.assertEqual(len(mutations), 1)
            modified = blob
            for edit in sorted(mutations[0].edits, key=lambda item: item.start, reverse=True):
                modified = modified[:edit.start] + edit.replacement + modified[edit.end:]
            self.assertIn(b"int Target() {\n    int value;\n", modified)
            self.assertNotIn(b"int Target()     int value;", modified)
            reparsed = index.parse(
                str(source), args=["-x", "c++", "-std=c++14"],
                unsaved_files=[(str(source), modified.decode("utf-8"))],
            )
            errors = [
                str(diagnostic) for diagnostic in reparsed.diagnostics
                if diagnostic.severity >= ci.Diagnostic.Error
            ]
            self.assertEqual(errors, [])

    def test_marker_span_uses_real_rva_markers(self):
        blob = (
            b"// RVA(0x00123456, in a comment\n"
            b"RVA(0x00123456, 0x1)\nint Target() { return 1; }\n"
            b"RVA(0x00123460, 0x1)\nint Next() { return 2; }\n"
        )
        start, end = marker_span(blob, 0x123456)
        self.assertEqual(blob[start:start + 4], b"RVA(")
        self.assertTrue(blob[end:].lstrip().startswith(b"RVA("))

    def test_target_function_is_the_first_definition_after_its_marker(self):
        blob = (
            b"#define RVA(rva, size)\n"
            b"RVA(0x00123456, 0x1)\n"
            b"int Target() { return 1; }\n"
            b"static inline int Helper() { return 2; }\n"
            b"RVA(0x00123460, 0x1)\n"
            b"int Next() { return 3; }\n"
        )
        configure_libclang()
        with tempfile.TemporaryDirectory() as directory:
            source = Path(directory) / "unit.cpp"
            source.write_bytes(blob)
            index = ci.Index.create()
            tu = index.parse(str(source), args=["-x", "c++", "-std=c++14"])
            fn = target_function(tu, source, blob, 0x123456)
            self.assertEqual(fn.spelling, "Target")

    def test_candidate_generation_rejects_overlapping_edits(self):
        blob = b"abcdef"
        mutations = [
            AstMutation("a", "first", (AstEdit(1, 3, b"XX"),)),
            AstMutation("b", "second", (AstEdit(2, 4, b"YY"),)),
        ]
        candidates, truncated = candidate_payloads(blob, mutations, 2, 20)
        self.assertFalse(truncated)
        self.assertEqual(len(candidates), 2)
        self.assertTrue(all(len(candidate["edits"]) == 1 for candidate in candidates))
        self.assertFalse(non_overlapping((mutations[0].edits[0], mutations[1].edits[0])))

    def test_source_shapes_are_crossed_with_all_tu_states(self):
        blob = b"abcdef"
        source = [AstMutation("source", "swap", (AstEdit(0, 1, b"A"),))]
        states = [
            AstMutation("tu_state_forest", "one", (AstEdit(6, 6, b"X"),)),
            AstMutation("tu_state_forest", "two", (AstEdit(6, 6, b"Y"),)),
        ]
        candidates, truncated, source_count, state_count = crossed_candidate_payloads(
            blob, source, states, max_depth=1, limit=6,
        )
        self.assertFalse(truncated)
        self.assertEqual((source_count, state_count, len(candidates)), (2, 3, 6))
        self.assertEqual(candidates[0]["name"], "baseline")
        self.assertIn("source:swap", candidates[-1]["name"])
        self.assertIn("tu_state_forest:two", candidates[-1]["name"])


class TopologyRankTests(unittest.TestCase):
    def test_structural_match_outranks_a_higher_fuzzy_wrong_shape(self):
        retail = {
            "instructions": 4, "branches": 1, "returns": 1,
            "flow": [["jne", 3], ["ret", None]],
        }
        exact_shape = compare_topology(dict(retail), retail)
        wrong_shape = compare_topology({
            "instructions": 5, "branches": 2, "returns": 1,
            "flow": [["jne", 3], ["jmp", 4], ["ret", None]],
        }, retail)
        self.assertLess(
            topology_rank(exact_shape, 95.0),
            topology_rank(wrong_shape, 99.0),
        )


if __name__ == "__main__":
    unittest.main()

from __future__ import annotations

import tempfile
import unittest
from pathlib import Path
from types import SimpleNamespace
from unittest import mock

from homm1 import cli
from homm1.permute import tu_state_noise as noise


class TuStateNoiseTests(unittest.TestCase):
    def target(self, text: str) -> noise.Target:
        marker = "RVA(0x00123456,"
        marker_offset = text.index(marker)
        insertion = noise._leading_metadata_offset(text, marker_offset)
        return noise.Target(
            Path("unit.cpp"), "unit", 0x123456, 0x523456,
            "?Target@@YAHXZ", 1, marker_offset, insertion,
            noise.logical_line_at(text, insertion),
        )

    def test_target_and_top_insertions_restore_logical_line(self):
        original = (
            "#include <a.h>\n\nint predecessor;\n\n// evidence\n"
            "RVA(0x00123456, 0x1)\nint Target() { return 1; }\n"
        )
        target = self.target(original)
        variant = noise.Variant(1, "typedef", "tag", "typedef int PROBE;\n")
        beside = noise.insert_variant(original, target, variant, "target")
        top = noise.insert_variant(original, target, variant, "top")
        self.assertLess(beside.index("PROBE"), beside.index("// evidence"))
        self.assertLess(top.index("PROBE"), top.index("int predecessor"))
        self.assertIn("#line 3\nint predecessor", top)

    def test_forest_is_deterministic_broad_and_replayable(self):
        left = noise.make_variants(12, noise.DEFAULT_FAMILIES, 123)
        right = noise.make_variants(12, noise.DEFAULT_FAMILIES, 123)
        self.assertEqual(left, right)
        self.assertTrue(all(variant.family == "forest" for variant in left))
        self.assertGreaterEqual(left[0].body.count("typedef "), 10)
        self.assertGreaterEqual(left[0].body.count("class "), 10)
        self.assertGreaterEqual(left[0].body.count("PROTOTYPE_"), 10)
        self.assertGreaterEqual(left[0].body.count("FUNCTION_"), 10)
        self.assertEqual(noise.select_variants(left, (4, 11), 12), [left[3], left[10]])
        with self.assertRaisesRegex(ValueError, "exceeds --trials"):
            noise.select_variants(left, (13,), 12)

    def test_typedef_count_is_an_exact_stride_one_prefix_sweep(self):
        variants = noise.make_variants(3, ("typedef-count",), 123)
        self.assertEqual(
            [variant.body.count("typedef int HOMM1_TU_STATE_COUNT_TYPEDEF_")
             for variant in variants],
            [1, 2, 3],
        )
        self.assertTrue(variants[1].body.startswith(variants[0].body))
        self.assertTrue(variants[2].body.startswith(variants[1].body))

    def test_state_identity_folds_nonsemantic_compiler_scaffolding(self):
        base = {
            "objdiff_size": 4,
            "text_sha": "same",
            "reloc_stream": ["00000000:0006:$SG123:00000000"],
        }
        renumbered = dict(base)
        renumbered["reloc_stream"] = ["00000000:0006:$SG999:00000000"]
        self.assertEqual(
            noise.target_state_identity(base),
            noise.target_state_identity(renumbered),
        )
        local_label = dict(base)
        local_label["reloc_stream"] = ["00000000:0006:$L456:00000000"]
        other_local_label = dict(base)
        other_local_label["reloc_stream"] = ["00000000:0006:$L999:00000000"]
        self.assertNotEqual(
            noise.target_state_identity(local_label),
            noise.target_state_identity(other_local_label),
        )
        changed = dict(base, text_sha="different")
        self.assertNotEqual(
            noise.target_state_identity(base),
            noise.target_state_identity(changed),
        )

    def test_single_island_census_prints_structural_route(self):
        metrics = {
            "objdiff_size": 4,
            "text_sha": "same",
            "reloc_stream": ["00000000:0006:_target:00000000"],
        }
        trial = {
            "trial": 1,
            "family": "forest",
            "tag": "probe",
            "body": "typedef int PROBE;\n",
            "candidate": dict(metrics),
            "score": 66.75,
            "topology": {"instruction_delta": 2},
        }
        census = noise.compiler_state_census(metrics, 66.75, [trial])
        self.assertEqual(census["island_count"], 1)
        self.assertEqual(census["executed_trials"], 1)
        self.assertEqual(census["search_route"], "structural")
        with mock.patch("builtins.print") as output:
            noise.report_state_search_route(census)
        output.assert_called_once_with(
            "only a single compiler island was found across 1 executed trials; "
            "compiler-state search is flat and the next search should be structural"
        )

    def test_multiple_islands_do_not_print_structural_route(self):
        baseline = {
            "objdiff_size": 4,
            "text_sha": "base",
            "reloc_stream": [],
        }
        trial = {
            "trial": 1,
            "family": "forest",
            "tag": "probe",
            "body": "typedef int PROBE;\n",
            "candidate": dict(baseline, text_sha="different"),
            "score": 70.0,
        }
        census = noise.compiler_state_census(baseline, 66.75, [trial])
        self.assertEqual(census["island_count"], 2)
        self.assertEqual(census["search_route"], "inspect-frontier")
        with mock.patch("builtins.print") as output:
            noise.report_state_search_route(census)
        output.assert_not_called()

    def test_exact_closure_requires_score_size_and_ordered_relocations(self):
        metrics = {
            "reloc_stream_complete": True,
            "reloc_stream": ["00000001:0006:_target:04000000"],
        }
        self.assertEqual(
            noise.exact_closure_rejections(100.0, 6, 6, metrics, metrics), []
        )
        self.assertIn(
            "unrounded objdiff score is not exactly 100.0",
            noise.exact_closure_rejections(99.999, 6, 6, metrics, metrics),
        )
        other = dict(metrics, reloc_stream=["00000001:0006:_other:04000000"])
        self.assertIn(
            "ordered relocation offsets/types/identities/addends differ from retail",
            noise.exact_closure_rejections(100.0, 6, 6, metrics, other),
        )

    def test_disposable_objects_use_the_authoritative_canonical_view(self):
        with tempfile.TemporaryDirectory() as directory:
            source = Path(directory) / "raw.obj"
            output = Path(directory) / "normalized.obj"
            source.write_bytes(b"raw-coff")
            canonical = SimpleNamespace(data=b"canonical-coff")
            with mock.patch.object(
                noise, "canonicalize_coff", return_value=canonical
            ) as transform:
                result = noise.canonicalize_disposable_object(source, output)
            self.assertEqual(result, output)
            self.assertEqual(output.read_bytes(), b"canonical-coff")
            transform.assert_called_once_with(b"raw-coff")

    def test_resolve_target_reads_current_model_serialization(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            source = root / "src/unit.cpp"
            source.parent.mkdir(parents=True)
            source.write_text("RVA(0x00123456, 0x1)\nint Target() { return 1; }\n")
            (root / "config").mkdir()
            (root / "config/units.toml").write_text(
                '[flags]\ncpp = ["/c"]\n\n[[unit]]\nunit = "unit"\n'
                'source = "src/unit.cpp"\nflags = "cpp"\n'
            )
            (root / "build/gen").mkdir(parents=True)
            (root / "build/gen/bindings.tsv").write_text(
                "# generated\n"
                "rva\tsize\tkind\tspace\tname\tunit\tchannel\talso_units\taliases\n"
                "0x00123456\t0x1\t\ttext\t?Target@@YAHXZ\tunit\tsrc\t\t\n"
            )
            target, flags = noise.resolve_target(root, Path("src/unit.cpp"), 0x123456)
        self.assertEqual(target.symbol, "?Target@@YAHXZ")
        self.assertEqual(target.retail_size, 1)
        self.assertEqual(flags, ["/c"])


class PermuteCliGateTests(unittest.TestCase):
    def run_cli(self, diagnosis: str, hist: float = 99.0):
        binding = SimpleNamespace(rva=0x123456, unit="unit", name="?Target@@YAHXZ")

        def diagnose(_token):
            print(diagnosis)
            return 0

        with mock.patch("homm1.walls.diagnose.diagnose", side_effect=diagnose), \
             mock.patch("homm1.model.resolve", return_value=SimpleNamespace(
                 functions=[binding]
             )), \
             mock.patch("homm1.verify.baseline.load", return_value={
                 (binding.unit, binding.name): {"hist": hist}
             }), \
             mock.patch("homm1.permute.tu_state_noise.main", return_value=17) as run:
            result = cli.main([
                "permute", "state", "--source", "src/unit.cpp",
                "--rva", "0x123456",
            ])
        return result, run

    def test_public_command_forwards_only_regalloc_wall_below_max(self):
        result, run = self.run_cli("class: REGALLOC/SCHEDULING")
        self.assertEqual(result, 17)
        run.assert_called_once()

    def test_public_command_refuses_cfg_and_historical_exact(self):
        result, run = self.run_cli("class: CFG")
        self.assertEqual(result, 2)
        run.assert_not_called()
        result, run = self.run_cli("class: REGALLOC/SCHEDULING", hist=100.0)
        self.assertEqual(result, 2)
        run.assert_not_called()

    def test_public_variants_command_uses_the_same_gate(self):
        binding = SimpleNamespace(rva=0x123456, unit="unit", name="?Target@@YAHXZ")

        def diagnose(_token):
            print("class: REGALLOC/SCHEDULING")
            return 0

        with mock.patch("homm1.walls.diagnose.diagnose", side_effect=diagnose), \
             mock.patch("homm1.model.resolve", return_value=SimpleNamespace(
                 functions=[binding]
             )), \
             mock.patch("homm1.verify.baseline.load", return_value={
                 (binding.unit, binding.name): {"hist": 99.0}
             }), \
             mock.patch("homm1.permute.match_variants.main", return_value=19) as run:
            result = cli.main([
                "permute", "variants", "src/unit.cpp", "0x123456",
                "--max-depth", "1", "-o", "/tmp/manifest.json",
            ])
        self.assertEqual(result, 19)
        run.assert_called_once()


if __name__ == "__main__":
    unittest.main()

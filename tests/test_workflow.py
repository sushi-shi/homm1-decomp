"""Controls for inherited workflow integration and generated status."""
import tempfile
import unittest
from pathlib import Path
from unittest.mock import patch
import subprocess
import sys

from homm1 import workflow, test_model
from homm1.core import test_data_matching
from homm1.retail_labels import test_decl_claims
from homm1.tool import test_merge_units
from homm1.verify import readme


class WorkflowTests(unittest.TestCase):
    def test_campaign_can_write_manifest_for_nested_unit(self):
        from homm1.permute import campaign
        row = dict(unit='BASE/WINDOW', source='src/BASE/WINDOW.cpp',
                   rva='0x74b30', symbol='ctor', classification='regalloc', proven=False)
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            def write_manifest(args, **kwargs):
                path = Path(args[args.index('-o') + 1])
                path.write_text('{}')
                self.assertEqual(path.parent, root / 'campaign')
                return 0
            with patch.object(campaign, 'project_root', return_value=root), \
                 patch.object(campaign, 'classified_candidates', return_value=[row]), \
                 patch.object(campaign, 'variants_main', side_effect=write_manifest):
                self.assertEqual(campaign.main(['campaign', '--rva', '0x74b30',
                                               '--output', 'campaign']), 0)
            self.assertTrue((root / 'campaign/001-BASE-WINDOW-0x074b30.manifest.json').is_file())

    def test_partially_staged_source_is_never_formatted_or_restaged(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            subprocess.run(['git', 'init', '-q', str(root)], check=True)
            source = root / 'src' / 'file with spaces.cpp'
            source.parent.mkdir()
            source.write_text('int staged;\n')
            subprocess.run(['git', 'add', '--', str(source)], cwd=root, check=True)
            source.write_text('int unstaged;\n')
            with patch.object(workflow.shutil, 'which', return_value=sys.executable):
                self.assertEqual(workflow.format_staged(root), 1)
            self.assertEqual(source.read_text(), 'int unstaged;\n')
            staged = subprocess.check_output(['git', 'show', ':src/file with spaces.cpp'], cwd=root)
            self.assertEqual(staged, b'int staged;\n')

    def test_early_stop_markers_join_absolute_va_to_rva(self):
        from homm1.walls import stale_markers
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            (root / 'src').mkdir()
            (root / 'include').mkdir()
            (root / 'src/a.cpp').write_text(
                '// @early-stop supported residue\n'
                'extern "C" VA(0x0044f640, 0x72)\nvoid PollSound() {}\n')
            with patch.object(stale_markers, 'REPO', root):
                self.assertEqual(stale_markers.marker_rvas(), {0x4f640})

    def test_conflicting_hooks_are_not_overwritten(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            subprocess.run(['git', 'init', '-q', str(root)], check=True)
            subprocess.run(['git', 'config', '--local', 'core.hooksPath', 'existing'], cwd=root, check=True)
            self.assertEqual(workflow.setup(root), 1)
            self.assertEqual(subprocess.check_output(['git', 'config', '--local', 'core.hooksPath'], cwd=root), b'existing\n')

    def test_readme_refresh_is_idempotent_and_preserves_authored_text(self):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / 'README.md'
            path.write_text('Intro\n' + readme.RM_START + '\nold\n' + readme.RM_END + '\nOutro\n')
            block = readme.RM_START + '\ncurrent\n' + readme.RM_END
            with patch.object(readme, 'README', path):
                self.assertTrue(readme.write_block(block))
                self.assertFalse(readme.write_block(block))
            self.assertEqual(path.read_text(), 'Intro\n' + block + '\nOutro\n')

    def test_readme_score_is_weighted_against_whole_engine_and_labels_mode(self):
        mods = {'BASE': {'tc': 100, 'mc': 100, 'fzw': 10000, 'tf': 1, 'mf': 1, 'units': 1, 'cw': 0, 'mx': 1}}
        engine = {'real_fn': 2, 'real_code': 200, 'unmatched_fn': 1, 'categories': []}
        with patch('homm1.core.data_matching.enabled', return_value=False):
            block = readme.render_block(mods, 10000, engine, {'cur': 1, 'max': 1, 'hist': 1, 'cw': 0, 'hw': 0})
        self.assertIn('1 / 2 functions exact (50.00%)', block)
        self.assertIn('50.00% fuzzy', block)
        self.assertIn('data-reference identities and addends are deferred', block)


def load_tests(loader, tests, pattern):
    for module in (test_merge_units, test_model, test_data_matching, test_decl_claims):
        tests.addTests(loader.loadTestsFromModule(module))
    return tests

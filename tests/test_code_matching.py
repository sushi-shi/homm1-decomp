"""Code-first comparison controls, including the unmodified donor fixtures."""
import tempfile
import unittest
from pathlib import Path
from unittest.mock import patch

from homm1.compare import normalize, test_normalize
from homm1.verify import test_data_identity
from homm1.verify.selftest import InlineEHControls


class ModeTransitionTests(unittest.TestCase):
    def test_selected_build_refreshes_all_pairs_when_mode_changes(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            base, target, output = (root / name for name in ('base', 'target', 'out'))
            for path in (base, target, output):
                path.mkdir()
            for name in ('A', 'B'):
                (base / f'{name}.obj').write_bytes(test_normalize.fixture())
                (target / f'{name}.c.obj').write_bytes(test_normalize.fixture())
            with patch.object(normalize.data_matching, 'enabled', return_value=True):
                normalize.normalize(base, target, output, ['A', 'B'], quiet=True)
            strict_b = (output / 'base/B.obj').read_bytes()
            with patch.object(normalize.data_matching, 'enabled', return_value=False):
                result = normalize.normalize(base, target, output, ['A'], quiet=True)
            self.assertEqual(result['base_objects'], 2)
            self.assertEqual(result['target_objects'], 2)
            self.assertNotEqual((output / 'base/B.obj').read_bytes(), strict_b)
            self.assertEqual((output / 'data-matching.mode').read_text(), 'false\n')


def load_tests(loader, tests, pattern):
    tests.addTests(loader.loadTestsFromModule(test_normalize))
    tests.addTests(loader.loadTestsFromModule(test_data_identity))
    return tests

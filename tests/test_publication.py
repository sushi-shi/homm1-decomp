import json
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest
from unittest.mock import patch

from homm1 import publication, reporting


class PublicationTests(unittest.TestCase):
    def test_interrupted_publication_is_completed_before_a_reader_proceeds(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            files = {'README.md': 'new readme', 'config/ledger.tsv': 'new ledger', 'build/report.json': '{}'}
            atomic = publication.atomic_write
            def interrupted(path, content):
                if path.name == 'ledger.tsv':
                    raise OSError('simulated interrupted publication')
                atomic(path, content)
            with patch.object(publication, 'atomic_write', side_effect=interrupted):
                with self.assertRaises(OSError):
                    publication.publish(files, root)
            self.assertTrue((root / 'build/publication.json').exists())
            with publication.locked(root):
                self.assertEqual({name: (root / name).read_text() for name in files}, files)
                self.assertFalse((root / 'build/publication.json').exists())

    def test_concurrent_writer_is_rejected(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            with publication.locked(root):
                result = subprocess.run([sys.executable, '-c',
                    'from homm1.publication import locked; from pathlib import Path; import sys\nwith locked(Path(sys.argv[1])): pass', str(root)],
                    capture_output=True, text=True)
                self.assertNotEqual(result.returncode, 0)
                self.assertIn('another campaign command', result.stderr)


class ReportingTests(unittest.TestCase):
    def report(self):
        return dict(functions=[dict(rva=0x1000, unit='one', retail_size=10, exact=False),
                               dict(rva=0x2000, unit='two', retail_size=30, exact=True)],
                    cleanliness=dict(findings=[], debt_sites=[{}], readability=dict(reviewed=[0x1000, 0x2000], pending=[])))

    def test_render_preserves_survey_is_idempotent_and_distinguishes_max_hist(self):
        source = f'target survey\n{reporting.START}\nstale\n{reporting.END}\nfooter'
        report = self.report()
        rows = {0x1000: dict(cur=50, max=70, hist=100), 0x2000: dict(cur=100, max=100, hist=100)}
        rendered = reporting.render(source, report, rows)
        self.assertTrue(rendered.startswith('target survey\n'))
        self.assertTrue(rendered.endswith('\nfooter'))
        self.assertIn('30/40 claimed code bytes', rendered)
        self.assertIn('| one | 0/1 | 10 | 50.00% | 70.00% | 100.00% |', rendered)
        self.assertEqual(rendering := reporting.render(rendered, report, rows), rendered)
        report['functions'][0]['exact'] = True
        rows[0x1000].update(cur=100, max=100)
        self.assertIn('40/40 claimed code bytes', reporting.render(rendering, report, rows))

    def test_malformed_markers_cannot_overwrite_manual_documentation(self):
        for source in ('survey', reporting.START + reporting.START + reporting.END, reporting.END + reporting.START):
            with self.assertRaisesRegex(ValueError, 'marker pair'):
                reporting.render(source, self.report(), {})

import csv
import hashlib
from pathlib import Path
import tempfile
import unittest
from unittest.mock import patch

from homm1 import correspondence as ref


class CorrespondenceTests(unittest.TestCase):
    def setUp(self):
        temporary = tempfile.TemporaryDirectory()
        self.addCleanup(temporary.cleanup)
        self.root = Path(temporary.name)
        (self.root / 'config/retail').mkdir(parents=True)
        (self.root / 'evidence').mkdir()
        (self.root / 'evidence/source.md').write_text('Reviewed behavioral evidence.')
        (self.root / 'config/retail/functions.tsv').write_text('rva\tkind\n0x1000\t\n')
        self.config = self.root / 'config/references.toml'
        self.config.write_text('[target]\nimage_base=0x400000\n'
            '[reference.homm2_buka_21]\nname="Buka 2.1"\nbranch="buka"\n'
            'revision="' + '1'*40 + '"\npriority=0\n'
            '[reference.homm2_20]\nname="HoMM2 2.0"\nbranch="2.0"\n'
            'revision="' + '2'*40 + '"\npriority=1\n')
        self.blob = b'VA(0x00402000, 0x10)\nvoid Poll() {}\n'
        self.row = dict(rva='0x1000', reference='homm2_buka_21', symbol='Poll',
                        donor_va='0x402000', source='src/Poll.cpp', line='1',
                        blob=hashlib.sha1(b'blob '+str(len(self.blob)).encode()+b'\0'+self.blob).hexdigest(),
                        confidence='hypothesis', evidence='evidence/source.md', differences='Body remains unclaimed.')
        self.write([self.row])

    def write(self, rows):
        with (self.root / 'config/retail/functions_correspondence.tsv').open('w') as stream:
            writer = csv.DictWriter(stream, ref.COLUMNS, delimiter='\t')
            writer.writeheader()
            writer.writerows(rows)

    def test_priority_survives_secondary_name_lookup_and_sparse_gaps_stay_unknown(self):
        self.write([dict(self.row, reference='homm2_20', symbol='OldPoll'), self.row])
        rows = ref.lookup('OldPoll', self.root)
        self.assertEqual([r['reference'] for r in rows], ['homm2_buka_21', 'homm2_20'])
        self.assertEqual([r['preferred'] for r in rows], [True, False])
        self.assertEqual(ref.lookup('0x401000', self.root), ref.lookup('0x1000', self.root))
        self.assertEqual(ref.lookup('0x1001', self.root), [])
        self.assertNotIn('size', rows[0])
        self.assertEqual(rows[0]['confidence'], 'hypothesis')

    def test_orphans_duplicates_paths_and_priority_drift_are_rejected(self):
        for rows in ([dict(self.row, rva='0x1001')], [self.row, self.row],
                     [dict(self.row, source='../outside.cpp')], [dict(self.row, blob='wrong')]):
            self.write(rows)
            with self.assertRaises(ValueError):
                ref.load(self.root)
        self.write([self.row])
        self.config.write_text(self.config.read_text().replace('priority=0', 'priority=2'))
        with self.assertRaisesRegex(ValueError, 'primary'):
            ref.load(self.root)

    def test_git_verification_uses_pinned_blob_and_checks_marker_and_declaration(self):
        rows = ref.lookup(root=self.root)
        with patch.object(ref.subprocess, 'check_output', return_value=self.blob) as git:
            self.assertEqual(ref.verify_checkout(rows, '/donor'), 1)
            self.assertEqual(git.call_args.args[0][-1], '1'*40 + ':src/Poll.cpp')
            for bad in (dict(rows[0], blob='0'*40), dict(rows[0], donor_va=0x403000),
                        dict(rows[0], symbol='Pol'), dict(rows[0], line=2)):
                with self.assertRaises(ValueError):
                    ref.verify_checkout([bad], '/donor')

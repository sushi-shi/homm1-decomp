from pathlib import Path
import tempfile
import unittest

from homm1.checkpoint import advance, read, write, fingerprint


def measurement(score, digest='first', rva=0x1000, symbol='_f'):
    return dict(rva=rva, score=score, src_hash=digest, retail_size=16, symbol=symbol, unit='unit')


class CheckpointTests(unittest.TestCase):
    def test_source_edit_resets_max_but_not_history(self):
        old = advance({}, [measurement(100)])
        changed = advance(old, [measurement(60, 'second')])[0x1000]
        self.assertEqual((changed['cur'], changed['max'], changed['hist']), (60, 60, 100))

    def test_collateral_dip_keeps_max(self):
        old = advance({}, [measurement(100)])
        changed = advance(old, [measurement(60)])[0x1000]
        self.assertEqual((changed['cur'], changed['max'], changed['hist']), (60, 100, 100))

    def test_name_promotion_preserves_history(self):
        old = advance({}, [measurement(100)])
        self.assertEqual(advance(old, [measurement(80, 'second', symbol='_real')])[0x1000]['hist'], 100)

    def test_partial_and_unmeasured_results_cannot_bank(self):
        old = advance({}, [measurement(100), measurement(50, rva=0x2000)])
        with self.assertRaisesRegex(ValueError, 'partial'):
            advance(old, [measurement(100)])
        with self.assertRaisesRegex(ValueError, 'unmeasured'):
            advance({}, [measurement(None)])

    def test_legacy_migration_and_roundtrip(self):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / 'ledger.tsv'
            path.write_text('rva\tsize\tsymbol\n0x1000\t0x10\t_f\n')
            old = read(path)
            current = advance(old, [measurement(70)])
            write(current, path)
            self.assertEqual(read(path)[0x1000]['hist'], 100)

    def test_dependency_change_invalidates_report_but_checkpoint_does_not(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            (root / 'include').mkdir()
            (root / 'config').mkdir()
            header = root / 'include/a.h'
            header.write_text('int f();')
            before = fingerprint(root, tools=False)
            (root / 'config/match_baseline.tsv').write_text('checkpoint')
            self.assertEqual(before, fingerprint(root, tools=False))
            header.write_text('long f();')
            self.assertNotEqual(before, fingerprint(root, tools=False))

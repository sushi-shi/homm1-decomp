"""Giten banking controls and HoMM1 MAX report integration."""
from contextlib import redirect_stdout, redirect_stderr
import io
from types import SimpleNamespace
import unittest
from unittest.mock import patch

from homm1.verify import readme, verbs
from homm1.verify.selftest import (
    BankRatchetControls, BankPreconditionControls, LedgerPrecisionControls,
    MaxGateClassificationControls, BankDataMatchingModeControls,
)


class MaxReadmeTests(unittest.TestCase):
    def test_refresh_previews_source_reset_and_keeps_unedited_peak_without_banking(self):
        a, b = ('u', 'edited'), ('u', 'dip')
        base = {k: dict(best=100., cur=100., fp='old', hist=100., tries=1,
                       state='', addr=i) for i, k in enumerate((a, b), 1)}
        cur = {a: 50., b: 75.}
        model = SimpleNamespace(functions=[SimpleNamespace(unit='BASE/u', name=k[1], size=100) for k in (a,b)])
        measures = {'u': dict(total_code=200, fuzzy_match_percent=62.5, total_functions=2, matched_functions=0)}
        eng = dict(real_fn=4, real_code=400, unmatched_fn=2)
        with patch.object(verbs, 'load_state', return_value=({}, cur, base, lambda u,f: 'new' if f=='edited' else 'old', set(), {a:1,b:2})), \
             patch.object(verbs, 'library_rvas', return_value=set()), \
             patch('homm1.model.resolve', return_value=model), \
             patch('homm1.verify.universe.engine_universe', return_value=eng), \
             patch.object(verbs.scores, 'unit_measures', return_value=measures), \
             patch.object(readme, 'unit_modules', return_value={'u':'BASE'}), \
             patch.object(readme, 'write_block', return_value=True) as write, \
             patch.object(verbs.bl, 'write') as bank:
            self.assertTrue(verbs.refresh_readme_block())
        block = write.call_args.args[0]
        self.assertIn('1 / 4 functions exact (25.00%)', block)
        self.assertIn('37.50% fuzzy', block)
        self.assertIn('CUR / MAX / HIST: 0 / 1 / 2 exact', block)
        self.assertIn('31.25% / 37.50% / 50.00% fuzzy', block)
        self.assertEqual(base[a]['best'], 100.)
        bank.assert_not_called()

    def test_dip_details_require_all(self):
        base = {('u','f'): dict(best=100.,cur=100.,fp='same',hist=100.,tries=1,addr=None,state='')}
        state = ({}, {('u','f'):90.}, base, lambda *_:'same', set(), {})
        with patch.object(verbs, 'load_state', return_value=state), \
             patch.object(verbs.bl, 'mode_mismatch', return_value=None), \
             patch.object(verbs, '_warn_stale_report'), \
             patch.object(verbs.scores, 'hard_failures', return_value=[]):
            out = io.StringIO()
            with redirect_stdout(out):
                self.assertEqual(verbs._report(SimpleNamespace(report=None, strict=False, all=False), True), 0)
            self.assertIn('DIP: 1 row(s)', out.getvalue())
            self.assertIn('--all lists them', out.getvalue())


class MaxGateVerdictTests(unittest.TestCase):
    def verdict(self, *, prev_cur=100., pct=90., edited=True, strict=False, gate=True, hard=()):
        base = {('u','f'): dict(best=100.,cur=prev_cur,fp='old',hist=100.,tries=1,addr=1,state='')}
        state = ({}, {('u','f'):pct}, base, lambda *_:'new' if edited else 'old', set(), {('u','f'):1})
        with patch.object(verbs, 'load_state', return_value=state), \
             patch.object(verbs.bl, 'mode_mismatch', return_value=None), \
             patch.object(verbs, '_warn_stale_report'), \
             patch.object(verbs.scores, 'hard_failures', return_value=list(hard)), \
             redirect_stdout(io.StringIO()):
            return verbs._report(SimpleNamespace(report=None, strict=strict, all=False), gate)

    def test_status_reports_fresh_failure_without_failing(self):
        self.assertEqual(self.verdict(gate=False), 0)
        self.assertEqual(self.verdict(), 1)

    def test_carried_regression_fails_only_strict_check(self):
        self.assertEqual(self.verdict(prev_cur=95.), 0)
        self.assertEqual(self.verdict(prev_cur=95., strict=True), 1)
        self.assertEqual(self.verdict(edited=False, strict=True), 0)

    def test_hard_failure_cannot_be_banked_even_with_dirty_override(self):
        with patch.object(verbs.bl, 'mode_mismatch', return_value=None), \
             patch.object(verbs, 'require_bankable_tree'), \
             patch.object(verbs, 'load_state', return_value=({}, {}, {}, lambda *_:'a', set(), {})), \
             patch.object(verbs, '_warn_stale_report'), \
             patch.object(verbs.scores, 'hard_failures', return_value=['zero-total pairing']), \
             patch.object(verbs.bl, 'write') as write, redirect_stderr(io.StringIO()):
            with self.assertRaisesRegex(SystemExit, 'defective report'):
                verbs.cmd_bank(['--dirty', '--no-refresh'])
        write.assert_not_called()


class StateMaxBankTests(unittest.TestCase):
    def test_only_max_hist_change_after_exact_closure(self):
        import tempfile
        from pathlib import Path
        from homm1.permute import tu_state_noise as noise
        row = dict(best=90.,cur=88.,fp='hash',hist=95.,tries=3,addr=0x1000,state='')
        original = verbs.bl.render({('u','f'):row, ('v','g'):dict(row)})
        with tempfile.TemporaryDirectory() as td:
            path = Path(td) / 'baseline.tsv'
            path.write_text(original)
            result = noise.record_target_max(path, 'BASE/u', 'f', 'hash', 100.)
            self.assertTrue(result['updated'])
            after = verbs.bl.load(path.read_text())
            self.assertEqual(after[('u','f')], dict(row, best=100., hist=100.))
            self.assertEqual(after[('v','g')], row)
            self.assertFalse(noise.record_target_max(path, 'BASE/u', 'f', 'hash', 100.)['updated'])

    def test_inexact_stale_duplicate_and_cross_mode_never_write(self):
        import tempfile
        from pathlib import Path
        from homm1.permute import tu_state_noise as noise
        row = dict(best=90.,cur=88.,fp='hash',hist=95.,tries=3,addr=0x1000,state='')
        original = verbs.bl.render({('u','f'):row})
        with tempfile.TemporaryDirectory() as td:
            path = Path(td) / 'baseline.tsv'
            path.write_text(original)
            for score in (None, 99.999999):
                self.assertFalse(noise.record_target_max(path, 'u', 'f', 'hash', score)['updated'])
                self.assertEqual(path.read_text(), original)
            for unit, name, fingerprint in [('u','f','wrong'), ('u','absent','hash')]:
                with self.assertRaises(noise.BaselineUpdateError):
                    noise.record_target_max(path, unit, name, fingerprint, 100.)
                self.assertEqual(path.read_text(), original)
            with patch.object(verbs.bl, 'mode_mismatch', return_value='other mode'):
                with self.assertRaisesRegex(noise.BaselineUpdateError, 'other mode'):
                    noise.record_target_max(path, 'u', 'f', 'hash', 100.)
            self.assertEqual(path.read_text(), original)
            target_line = next(line for line in original.splitlines() if line.startswith('u\tf\t'))
            duplicate = original + target_line + '\n'
            path.write_text(duplicate)
            with self.assertRaisesRegex(noise.BaselineUpdateError, 'duplicate'):
                noise.record_target_max(path, 'u', 'f', 'hash', 100.)
            self.assertEqual(path.read_text(), duplicate)

    def test_exact_score_alone_does_not_pass_state_closure(self):
        from homm1.permute import tu_state_noise as noise
        metrics = dict(reloc_stream_complete=True, reloc_stream=['0:REL32:f:0'])
        self.assertTrue(noise.exact_closure_rejections(100., 4, 5, metrics, metrics))
        self.assertTrue(noise.exact_closure_rejections(99.99999, 4, 4, metrics, metrics))
        incomplete = dict(metrics, reloc_stream_complete=False)
        self.assertTrue(noise.exact_closure_rejections(100., 4, 4, incomplete, metrics))

from pathlib import Path
import tempfile
import unittest

from homm1 import verify
from homm1.core.matching import Claim


class VerifyTests(unittest.TestCase):
    def scan(self, text):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            (root / 'src').mkdir()
            (root / 'src/a.cpp').write_text(text)
            return verify.board(root)

    def test_comments_and_literals_are_not_code(self):
        self.assertFalse(self.scan('// __asm\nconst char *s = "m_vptr";')['findings'])

    def test_inactive_assembly_is_forbidden(self):
        self.assertTrue(self.scan('#if 0\n__asm { nop }\n#endif')['findings'])

    def test_debt_is_not_silently_accepted(self):
        self.assertTrue(self.scan('void f() { RetailService_0044F640(); }')['findings'])

    def test_review_fingerprint_changes_with_code_only(self):
        self.assertEqual(verify.fingerprint('foo(); // one'), verify.fingerprint('foo(); // two'))
        self.assertNotEqual(verify.fingerprint('foo();'), verify.fingerprint('bar();'))

    def test_compiler_specific_source_fork_is_forbidden(self):
        self.assertTrue(self.scan('#ifdef __clang__\nint f();\n#endif')['findings'])

    def test_review_is_invalidated_by_source_change(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            path = root / 'config/cleanliness/reviews.toml'
            path.parent.mkdir(parents=True)
            path.write_text('[[review]]\nrva="0x1000"\nsrc_hash="old"\nreviewer="reviewer"\nevidence="read"\n')
            with self.assertRaisesRegex(ValueError, 'stale'):
                verify.check_reviews([Claim(0x1000, 4, '_a', src_hash='new')], root)

    def test_pending_review_prevents_full_publication(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            claims = [Claim(0x1000, 4, '_a', src_hash='new')]
            self.assertEqual(verify.check_reviews(claims, root)['pending'], [0x1000])
            with self.assertRaisesRegex(ValueError, 'requires source review'):
                verify.check_reviews(claims, root, require_complete=True)

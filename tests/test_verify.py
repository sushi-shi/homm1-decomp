from pathlib import Path
import tempfile
import unittest

from homm1 import verify


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

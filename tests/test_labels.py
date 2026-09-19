from pathlib import Path
import shutil
import tempfile
import unittest

from homm1.labels import definitions, source_hash
from homm1.model import validate
from homm1.core.matching import Claim
from test_image import fixture
from homm1.core.image import Image


class HashTests(unittest.TestCase):
    def test_tokens_ignore_comments_but_keep_literals(self):
        self.assertEqual(source_hash('return 1; // a'), source_hash('return 1; /* b */'))
        self.assertNotEqual(source_hash('return "a";'), source_hash('return "b";'))

    def test_overlapping_and_wrong_kind_claims_fail(self):
        image = Image(fixture())
        with self.assertRaisesRegex(ValueError, 'overlapping'):
            validate([Claim(0x1000, 0x20, '_a'), Claim(0x1010, 0x10, '_b')], image,
                     {0x1000: '', 0x1010: ''})
        with self.assertRaisesRegex(ValueError, 'correct kind'):
            validate([Claim(0x1000, 0x10, '_a')], image, {0x1000: 'eh'})


@unittest.skipUnless(shutil.which('clang++'), 'Clang is required; run in nix develop')
class LabelTests(unittest.TestCase):
    def claims(self, source):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / 'claims.cpp'
            path.write_text('#include "match.h"\n' + source)
            return definitions(path)

    def test_real_ast_binds_multiple_functions_and_overloads(self):
        claims = self.claims('''
extern "C" RVA(0x1000, 0x10) int __stdcall first(int a) { return a; }
RVA(0x1010, 0x10) static int local(int a) { return a; }
class Counter { public: int value(int); int value(short); };
RVA(0x1020, 0x10) int Counter::value(int a) { return a; }
RVA(0x1030, 0x10) int Counter::value(short a) { return a; }
''')
        self.assertEqual([c.rva for c in claims], [0x1000, 0x1010, 0x1020, 0x1030])
        self.assertEqual(claims[0].symbol, '_first@4')
        self.assertEqual(len({c.symbol for c in claims}), 4)

    def test_annotation_in_comment_cannot_claim_definition(self):
        with self.assertRaisesRegex(ValueError, 'no RVA identity'):
            self.claims('// RVA(0x1000, 0x10)\nint function() { return 1; }')

    def test_cstyle_cast_fails_semantic_gate(self):
        with self.assertRaisesRegex(ValueError, 'C-style cast'):
            self.claims('RVA(0x1000, 0x10) int function(long a) { return (int)a; }')

    def test_generated_body_requires_owner(self):
        with self.assertRaisesRegex(ValueError, 'lacks its source owner'):
            self.claims('RVA_COMPGEN(0x1020, 0x10, "generated", 0x1000)')

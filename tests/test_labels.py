from pathlib import Path
from dataclasses import replace
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

    def test_operator_boundaries_and_line_splicing(self):
        self.assertNotEqual(source_hash('return a + ++b;'), source_hash('return a++ + b;'))
        self.assertNotEqual(source_hash('a += b;'), source_hash('a + = b;'))
        self.assertEqual(source_hash('ret\\\nurn 1;'), source_hash('return 1;'))
        self.assertEqual(source_hash('// continued\\\nignored\nreturn 1;'), source_hash('return 1;'))
        self.assertNotEqual(source_hash('return L"x";'), source_hash('return L "x";'))

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

    def test_constructors_destructors_and_generated_ownership(self):
        claims = self.claims('''
class Item { public: Item(); ~Item(); };
RVA(0x1000, 0x10) Item::Item() {}
RVA(0x1010, 0x10) Item::~Item() {}
RVA_COMPGEN(0x1020, 0x10, "helper", 0x1000)
''')
        self.assertEqual(claims[0].symbol, '??0Item@@QAE@XZ')
        self.assertEqual(claims[1].symbol, '??1Item@@QAE@XZ')
        self.assertEqual(claims[2].parent, 0x1000)
        self.assertEqual(claims[2].src_hash, claims[0].src_hash)

    def test_unknown_layout_cannot_be_used_for_sizeof(self):
        with self.assertRaisesRegex(ValueError, 'unrecovered class layout'):
            self.claims('#include "SOURCE/KB.h"\nRVA(0x1000, 0x10) int size() { return sizeof(soundManager); }')

    def test_unknown_layout_arithmetic_and_inheritance_fail(self):
        cases = [
            'RVA(0x1000,16) int f(soundManager *a, soundManager *b) { return a-b; }',
            'RVA(0x1000,16) void f(soundManager *a) { a += 1; }',
            'RVA(0x1000,16) void f(soundManager *a) { ++a; }',
            'class Derived : public soundManager {};',
            'soundManager by_value();',
        ]
        for source in cases:
            with self.subTest(source=source), self.assertRaisesRegex(ValueError, 'recovered layout'):
                self.claims('#include "SOURCE/KB.h"\n' + source)
        self.assertEqual(len(self.claims('#include "SOURCE/KB.h"\nRVA(0x1000,16) bool f(soundManager *a, soundManager *b) { return a == b; }')), 1)

    def test_internal_names_are_scoped_to_source_unit(self):
        first = self.claims('RVA(0x1000,16) static int helper(int a) { return a; }')[0]
        second = self.claims('RVA(0x1010,16) static int helper(int a) { return a; }')[0]
        self.assertEqual(first.linkage, 'internal')
        validate([replace(first, unit='one'), replace(second, unit='two')], Image(fixture()), {0x1000: '', 0x1010: ''})
        with self.assertRaisesRegex(ValueError, 'duplicate source symbol'):
            validate([replace(first, unit='one'), replace(second, unit='one')], Image(fixture()), {0x1000: '', 0x1010: ''})

    def test_header_change_invalidates_review_context_without_resetting_source_max(self):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / 'claims.cpp'
            header = Path(directory) / 'context.h'
            path.write_text('#include "match.h"\n#include "context.h"\nRVA(0x1000,16) int f() { return VALUE; }')
            header.write_text('#define VALUE 1\n')
            old = definitions(path)[0]
            header.write_text('#define VALUE 2\n')
            new = definitions(path)[0]
            self.assertEqual(old.src_hash, new.src_hash)
            self.assertNotEqual(old.context_hash, new.context_hash)

    def test_conflicting_external_declarations_across_units_fail(self):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / 'declaration.cpp'
            declarations = {}
            path.write_text('extern "C" int shared(int);')
            definitions(path, declarations=declarations)
            path.write_text('extern "C" long shared(long);')
            with self.assertRaisesRegex(ValueError, 'conflicting source declarations'):
                definitions(path, declarations=declarations)

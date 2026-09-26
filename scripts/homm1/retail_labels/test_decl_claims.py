"""Giten declaration controls adapted to HoMM1 absolute VAs and VC4."""
from pathlib import Path
import tempfile
import unittest
from unittest.mock import patch
from types import SimpleNamespace

from homm1.retail_labels.source import decl_claims


def decl(name, ann, kind='func', defined=False):
    return {'kind': kind, 'name': name, 'annotations': [ann], 'defined': defined,
            'file': '/repo/include/X.h', 'internal': False}


class DeclClaims(unittest.TestCase):
    def setUp(self):
        p = patch('homm1.retail_labels.source.image', return_value=SimpleNamespace(image_base=0x400000))
        p.start()
        self.addCleanup(p.stop)

    def test_prototype_binds_and_redeclarations_coalesce(self):
        claims, problems = decl_claims([
            decl('_Foo', 'decl-va:0x00412340'), decl('_Foo', 'decl-va:0x00412340'),
            decl('_Foo', 'decl-va:0x00412340', defined=True)])
        self.assertEqual((claims, problems), ([(0x12340, '_Foo')], []))

    def test_other_annotations_are_not_declaration_claims(self):
        self.assertEqual(decl_claims([decl('_g', 'data-va:0x00491540', kind='var'),
            decl('_Bar', 'va:0x00412350 size:0x10', defined=True)]), ([], []))

    def test_decl_on_variable_is_a_problem(self):
        claims, problems = decl_claims([decl('_g', 'decl-va:0x00491540', kind='var')])
        self.assertEqual(claims, [])
        self.assertEqual(len(problems), 1)
        self.assertIn('not a function', problems[0])

    def test_real_header_prototype_reaches_including_translation_unit(self):
        from homm1.core.paths import BUILD
        from homm1.tool import clang
        BUILD.mkdir(exist_ok=True)
        with tempfile.TemporaryDirectory(dir=BUILD) as directory:
            root = Path(directory)
            (root / 'Decl.h').write_text('#include <match.h>\n'
                'VA_DECL(0x00412340)\nvoid Foo(short a);\n'
                'VA_DECL(0x00412350)\nshort Bar(void);\n')
            tu = root / 'decl.c'
            tu.write_text('#include "Decl.h"\n'
                'VA(0x00412350, 0x6)\nshort Bar(void) { return 1; }\n'
                'int Use(void) { Foo(2); return Bar(); }\n')
            declarations = clang.annotated_decls(str(tu), None)
        self.assertIsNotNone(declarations)
        self.assertEqual(decl_claims(declarations), ([(0x12340, '_Foo'), (0x12350, '_Bar')], []))

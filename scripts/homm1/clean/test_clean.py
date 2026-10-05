"""The clean generator's variants, classic rendering and verification helpers."""
import unittest

from homm1.clean import classic, verify
from homm1.graph.catalog import Catalog, literal, resource_literal

REGISTRY = ('HOMM1_MESSAGE("ui.gold", "Gold")\n'
            'HOMM1_MESSAGE("ui.quote", "Say \\"hi\\"\\n")\n')
PO = ('msgid ""\nmsgstr ""\n"Language: ru\\n"\n'
      '"Content-Type: text/plain; charset=UTF-8\\n"\n\n'
      'msgctxt "ui.gold"\nmsgid "Gold"\nmsgstr "Золото"\n\n'
      'msgctxt "ui.quote"\nmsgid "Say \\"hi\\"\\n"\nmsgstr "Скажи \\"да\\"\\n"\n')


def catalog():
    return Catalog.parse(REGISTRY, PO)


class ClassicRenderingTests(unittest.TestCase):
    def test_literal_is_readable_and_controls_stay_escaped(self):
        self.assertEqual(classic.readable_literal('Золото\n\t"\\\x01' + '7'),
                         '"Золото\\n\\t\\"\\\\\\0017"')

    def test_resource_literal_doubles_quotes(self):
        self.assertEqual(classic.resource_literal('a "b"\nc'), '"a ""b""\\nc"')

    def test_cpp_references_become_russian_literals(self):
        text = 'const char *g = localization::Tr("ui.gold");\n'
        self.assertEqual(classic.render_cpp(text, catalog()),
                         'const char *g = "Золото";\n')

    def test_rc_strings_language_and_code_page(self):
        text = ('LANGUAGE HOMM1_RESOURCE_LANGUAGE, 1\n'
                'STRINGTABLE { 1, localization::Tr("ui.quote") }\n')
        rendered = classic.render_rc(text, catalog())
        self.assertTrue(rendered.startswith('#pragma code_page(65001)\n'))
        self.assertIn('LANGUAGE 0x19, 1', rendered)
        self.assertIn('"Скажи ""да""\\n"', rendered)

    def test_rc_rejects_character_arrays(self):
        with self.assertRaises(ValueError):
            classic.render_rc('localization::Chars("ui.gold")', catalog())

    def test_russian_branches_are_kept(self):
        text = ('a\n#if HOMM1_RUSSIAN\nru\n#ifdef X\nx\n#else\ny\n#endif\n#else\nen\n#endif\n'
                '#if !HOMM1_RUSSIAN\nen2\n#else\nru2\n#endif\n#if Y\ny\n#endif\nb')
        self.assertEqual(classic.resolve_conditionals(text),
                         'a\nru\n#ifdef X\nx\n#else\ny\n#endif\nru2\n#if Y\ny\n#endif\nb')

    def test_unsupported_conditionals_fail(self):
        for text in ('#if HOMM1_RUSSIAN\n#elif X\n#endif', '#if HOMM1_RUSSIAN\nx',
                     'int x = HOMM1_RUSSIAN;', '#endif'):
            with self.subTest(text=text), self.assertRaises(ValueError):
                classic.resolve_conditionals(text)


class EquivalenceTests(unittest.TestCase):
    def test_literal_bytes(self):
        self.assertEqual(verify.literal_bytes('"Я\\n\\0017\\x41"'), b'\xdf\n\x017A')

    def test_classic_literal_equals_escaped_reference(self):
        reference = 'const char *g = ' + literal('Золото') + ';'
        self.assertIsNone(verify._equivalent_cpp('const char *g = "Золото";', reference))
        self.assertIsNotNone(verify._equivalent_cpp('const char *g = "Злато";', reference))

    def test_classic_literal_equals_character_initializer(self):
        self.assertIsNone(verify._equivalent_cpp('char g[2] = "Да";',
                                                 "char g[2] = {'\\xc4', '\\xe0'};"))
        self.assertIsNotNone(verify._equivalent_cpp('char g[2] = "Да";',
                                                    "char g[2] = {'\\xc4', '\\xe1'};"))

    def test_rc_classic_against_rendered_resource(self):
        text = ('LANGUAGE HOMM1_RESOURCE_LANGUAGE, 1\n'
                'STRINGTABLE { 1, localization::Tr("ui.quote") }\n')
        reference = catalog().render_resource(text, locale='ru')
        self.assertIn('L"', reference)
        self.assertIsNone(verify._equivalent_rc(classic.render_rc(text, catalog()), reference))
        changed = classic.render_rc(text, catalog()).replace('да', 'нет')
        self.assertIsNotNone(verify._equivalent_rc(changed, reference))
        self.assertTrue(resource_literal('x').startswith('L"'))


class CanonicalNameTests(unittest.TestCase):
    def test_local_counters_renumber_by_first_appearance(self):
        def section(*names):
            return [{"relocations": [(i, 6, n) for i, n in enumerate(names)], "defines": []}]
        left = verify._canonical(section("$L100", "_f", "$SG7", "$normalEvent$26311"),
                                 [("$L100", 1), ("$SG7", 2), ("$normalEvent$26311", 1)])
        right = verify._canonical(section("$L4", "_f", "$SG9", "$normalEvent$12"),
                                  [("$L4", 1), ("$SG9", 2), ("$normalEvent$12", 1)])
        self.assertEqual(left, right)
        self.assertEqual(left[0]["relocations"][1][2], "_f")


if __name__ == '__main__':
    unittest.main()

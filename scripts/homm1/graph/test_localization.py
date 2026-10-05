"""Readable authored text, exact compiler bytes, and real Clang format checking."""
import ast
import json
from pathlib import Path
import tempfile
import unittest
from unittest import mock

from homm1.graph import localization as loc


class LocalizationTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name)
        (self.root / 'locales').mkdir()
        (self.root / 'src').mkdir()
        (self.root / 'include').mkdir()
        self.write_catalog()

    def write_catalog(self, english='Gold: %s %d', russian='Золото: %s %d'):
        (self.root / 'locales/messages.def').write_text(
            f'HOMM1_MESSAGE("resource.gold", {loc.quoted(english)})\n', encoding='utf-8')
        (self.root / 'locales/ru.po').write_text(
            'msgid ""\nmsgstr ""\n"Language: ru\\n"\n'
            '"Content-Type: text/plain; charset=UTF-8\\n"\n\n'
            f'msgctxt "resource.gold"\nmsgid {loc.quoted(english)}\n'
            f'msgstr {loc.quoted(russian)}\n', encoding='utf-8')

    def test_cp1251_bytes_and_escape_boundaries(self):
        text = 'ЁёЯя A9"\\\n\t\0'
        encoded = loc.literal(text)
        self.assertTrue(encoded.isascii())
        self.assertEqual(ast.literal_eval(encoded).encode('latin1'), text.encode('cp1251'))

    def test_unrepresentable_translation_is_fatal(self):
        self.write_catalog('Gold', 'Золото 🪙')
        with self.assertRaises(UnicodeEncodeError):
            loc.Catalog.load(self.root)

    def test_unknown_id_is_fatal(self):
        with self.assertRaisesRegex(ValueError, 'unknown localization ID'):
            loc.Catalog.load(self.root).render('localization::Tr("resource.unknown")')

    def test_dynamic_id_is_fatal(self):
        for value in ('id', '"resource.gold", count', '"resource." "gold"'):
            with self.subTest(value=value), self.assertRaisesRegex(ValueError, 'one literal'):
                loc.Catalog.load(self.root).render('localization::Tr(' + value + ')')

    def test_comments_and_quoted_code_are_not_calls(self):
        text = '// localization::Tr(id)\n/* localization::Tr(id) */\n"localization::Tr(id)"'
        self.assertEqual(loc.Catalog.load(self.root).render(text), text)

    def test_generated_view_preserves_utf8_offsets_and_lines(self):
        catalog = loc.Catalog.load(self.root)
        text = '// Русский комментарий\na=localization :: Tr(\n "resource.gold"\n);\nDATA(123) int n;'
        generated = catalog.render(text)
        self.assertEqual(len(text.encode()), len(generated.encode()))
        self.assertEqual([i for i, b in enumerate(text.encode()) if b == 10],
                         [i for i, b in enumerate(generated.encode()) if b == 10])
        self.assertEqual(text.encode().index(b'DATA'), generated.encode().index(b'DATA'))
        self.assertIn(catalog.macro('resource.gold'), generated)

    def test_expansion_preserves_packed_storage_nuls(self):
        self.write_catalog('Gold', 'Золото')
        text = 'localization::Tr("resource.gold") "\\0\\0" localization::Tr("resource.gold")'
        rendered = loc.Catalog.load(self.root).render(text, expanded=True)
        value = ''.join(ast.literal_eval(t.group()) for t in loc.tokens(rendered))
        self.assertEqual(value.encode('latin1'), 'Золото\0\0Золото'.encode('cp1251'))

    def test_fixed_array_has_exact_characters_without_added_terminator(self):
        self.write_catalog("A'\\ Z", "Я'\\ Ю")
        catalog = loc.Catalog.load(self.root)
        source = self.root / 'src/test.cpp'
        text = ('struct Record { char name[5]; };\n'
                'Record value = { localization::Chars("resource.gold") };\n')
        source.write_text(text)
        for locale in ('en', 'ru'):
            expanded = catalog.render(text, expanded=True, locale=locale)
            chars = [ast.literal_eval(t.group()) for t in loc.tokens(expanded)
                     if t.lastgroup == 'char']
            self.assertEqual(''.join(chars).encode('latin1'),
                             catalog.messages(locale)['resource.gold'].encode('cp1251'))
            self.assertEqual(len(chars), 5)
            compiled, header, _, _ = loc.prepare(self.root, source, locale=locale)
            self.assertIn(catalog.macro('resource.gold', chars=True),
                          compiled.read_text())
            self.assertIn('#define ' + catalog.macro('resource.gold', chars=True),
                          header.read_text())
            self.assertEqual(loc.check_formats(self.root, source, locale=locale), [])
        self.assertEqual(list(catalog.calls(text))[0][2], 'resource.gold')
        self.assertNotIn('H1C', catalog.header())

    def test_fixed_array_preserves_offsets_and_compiler_checks_its_extent(self):
        self.write_catalog('ABCD', 'АБВГ')
        catalog = loc.Catalog.load(self.root)
        text = ('char value[3] = localization :: Chars(\n "resource.gold"\n);\n'
                'int following;\n')
        generated = catalog.render(text)
        self.assertEqual(len(text.encode()), len(generated.encode()))
        self.assertEqual(text.index('following'), generated.index('following'))
        self.assertEqual([i for i, c in enumerate(text) if c == '\n'],
                         [i for i, c in enumerate(generated) if c == '\n'])
        source = self.root / 'src/test.cpp'
        source.write_text(text)
        for locale in ('en', 'ru'):
            self.assertTrue(loc.check_formats(self.root, source, locale=locale))
        with self.assertRaisesRegex(ValueError, 'one literal'):
            catalog.render('localization::Chars(dynamic_id)')
        with self.assertRaisesRegex(ValueError, 'resource strings'):
            catalog.render_resource('localization::Chars("resource.gold")')
        quoted = '// localization::Chars(dynamic_id)\n"localization::Chars(dynamic_id)"'
        self.assertEqual(catalog.render(quoted), quoted)

    def test_stale_po_source_is_fatal(self):
        path = self.root / 'locales/ru.po'
        path.write_text(path.read_text().replace('Gold: %s %d', 'Changed: %s %d'))
        with self.assertRaisesRegex(ValueError, 'stale'):
            loc.Catalog.load(self.root)

    def test_catalog_cannot_hide_unicode_in_escapes(self):
        path = self.root / 'locales/ru.po'
        path.write_text(path.read_text().replace('З', r'\u0417'))
        with self.assertRaisesRegex(ValueError, 'numeric text escape'):
            loc.Catalog.load(self.root)

    def test_header_localization_reaches_both_compilers(self):
        source = self.root / 'src/test.cpp'
        source.write_text('#include <outer.h>\n')
        (self.root / 'include/outer.h').write_text('#include "inner.h"\n')
        (self.root / 'include/inner.h').write_text('char* text = localization::Tr("resource.gold");\n')
        compiled, header, overlay, _ = loc.prepare(self.root, source)
        self.assertNotEqual(compiled, source)
        self.assertIn('H1L', (header.parent / 'include/inner.h').read_text())
        self.assertTrue((header.parent / 'include/outer.h').is_file())
        self.assertTrue(any(e['name'].endswith('/inner.h') for e in json.loads(overlay.read_text())['roots']))

    def test_missing_translation_is_fatal(self):
        self.write_catalog(russian='')
        with self.assertRaisesRegex(ValueError, 'missing Russian translation'):
            loc.Catalog.load(self.root)

    def test_placeholder_mismatch_is_fatal(self):
        for ru in ('Золото: %s', 'Золото: %d %s', 'Золото: %s %ld', 'Золото: %s %d %d'):
            with self.subTest(ru=ru):
                self.write_catalog(russian=ru)
                with self.assertRaisesRegex(ValueError, 'placeholders differ'):
                    loc.Catalog.load(self.root)

    def write_variant(self, value=None):
        path = self.root / 'locales/format-variants.json'
        path.write_text(json.dumps(value if value is not None else {
            'resource.gold': {'en': ['s', 'd', 's'], 'ru': ['s', 'd']}}))
        return path

    def test_format_variants_require_exact_declared_signatures(self):
        self.write_catalog('Shoot %s(%d shot%s left)', 'Стрелять %s (%d)')
        with self.assertRaisesRegex(ValueError, 'placeholders differ'):
            loc.Catalog.load(self.root)
        self.write_variant()
        self.assertEqual(loc.Catalog.load(self.root).english['resource.gold'],
                         'Shoot %s(%d shot%s left)')
        for value in ({'unknown.id': {'en': ['s'], 'ru': ['d']}},
                      {'resource.gold': {'en': ['s'], 'ru': ['d']}},
                      {'resource.gold': {'en': ['s'], 'ru': ['s']}},
                      {'resource.gold': {'en': 's', 'ru': ['s']}},
                      {'resource.gold': {'en': ['s'], 'ru': ['d'], 'other': []}}):
            with self.subTest(value=value), self.assertRaises(ValueError):
                self.write_variant(value)
                loc.Catalog.load(self.root)

    def test_locale_branch_preserves_offsets_and_ignores_quoted_identifiers(self):
        text = ('// HOMM1_RUSSIAN\n"HOMM1_RUSSIAN";\n#if HOMM1_RUSSIAN\n'
                'localization::Tr("resource.gold")\n#endif\nDATA(123) int n;')
        catalog = loc.Catalog.load(self.root)
        for locale, value in [('en', '0'), ('ru', '1')]:
            rendered = catalog.render(text, locale=locale)
            self.assertIn('#if ' + value, rendered)
            self.assertIn('// HOMM1_RUSSIAN\n"HOMM1_RUSSIAN";', rendered)
            self.assertEqual(len(text.encode()), len(rendered.encode()))
            self.assertEqual(text.index('DATA'), rendered.index('DATA'))
            self.assertEqual([i for i, b in enumerate(text) if b == '\n'],
                             [i for i, b in enumerate(rendered) if b == '\n'])
            self.assertIn('#if ' + value + '\n', catalog.render(text, expanded=True, locale=locale))

    def test_real_compiler_checks_both_format_variant_calls(self):
        self.write_catalog('Shoot %s(%d shot%s left)', 'Стрелять %s (%d)')
        variant = self.write_variant()
        source = self.root / 'src/test.cpp'
        text = ('extern "C" int printf(const char*, ...);\nvoid f() {\n'
                '#if HOMM1_RUSSIAN\n'
                'printf(localization::Tr("resource.gold"), "name", 3);\n'
                '#else\n'
                'printf(localization::Tr("resource.gold"), "name", 3, "s");\n'
                '#endif\n}\n')
        source.write_text(text)
        for locale in ('en', 'ru'):
            self.assertEqual(loc.check_formats(self.root, source, locale=locale), [])
            _, _, _, deps = loc.prepare(self.root, source, locale=locale)
            self.assertIn(variant, deps)
        source.write_text(text.replace('3, "s"', '3'))
        self.assertTrue(loc.check_formats(self.root, source, locale='en'))
        self.assertEqual(loc.check_formats(self.root, source, locale='ru'), [])
        source.write_text(text.replace('"name", 3);', '"name", "bad");'))
        self.assertTrue(loc.check_formats(self.root, source, locale='ru'))

    def test_stars_and_length_modifiers(self):
        self.assertEqual(loc.format_signature('%*.*s %ld %%'), ['*', '*', 's', 'ld'])
        self.assertEqual(loc.format_signature('50% protection and 10% of the cost'), [])

    def test_duplicate_ids_and_fuzzy_translations_are_fatal(self):
        path = self.root / 'locales/messages.def'
        path.write_text(path.read_text() * 2)
        with self.assertRaisesRegex(ValueError, 'duplicate'):
            loc.Catalog.load(self.root)
        self.write_catalog()
        po = self.root / 'locales/ru.po'
        po.write_text(po.read_text().replace('msgctxt', '#, fuzzy\nmsgctxt'))
        with self.assertRaisesRegex(ValueError, 'fuzzy'):
            loc.Catalog.load(self.root)

    def test_all_hidden_text_spellings_are_rejected(self):
        for source in (r'"\xcf\xf0"', r'"\317\360"', r'"\u041f"',
                       r'"\U0000041f"', r'"\x41"', r'"\x0a"', r"'\xcf'", '"Привет"'):
            with self.subTest(source=source):
                self.assertTrue(loc.hidden_text_errors(source))

    def test_controls_comments_paths_and_literal_backslashes_are_allowed(self):
        for source in (r'"\n\r\t\b\0"', r'"C:\\Users\\xfile"',
                       r'"\\xCF"', r'// "\xcf"', r'/* "Привет" */', r"'\0'"):
            with self.subTest(source=source):
                self.assertEqual(loc.hidden_text_errors(source), [])

    def test_tree_gate_checks_headers_and_character_literals(self):
        (self.root / 'include/bad.h').write_text(r"char bad = '\317';")
        errors, _ = loc.check_tree(self.root)
        self.assertTrue(any('bad.h:1' in e for e in errors))

    def test_prepare_tracks_catalog_and_source_edits(self):
        source = self.root / 'src/test.cpp'
        source.write_text('char* text = localization::Tr("resource.gold");\n')
        compiled, header, overlay, deps = loc.prepare(self.root, source)
        self.assertEqual(compiled.name, source.name)
        self.assertNotEqual(compiled, source)
        self.assertEqual(len(compiled.read_bytes()), len(source.read_bytes()))
        self.assertEqual(json.loads(overlay.read_text())['roots'][0]['name'], str(source))
        self.assertIn(self.root / 'locales/ru.po', deps)
        before = header.read_bytes()
        self.write_catalog(russian='Монеты: %s %d')
        _, changed, _, _ = loc.prepare(self.root, source)
        self.assertNotEqual(changed.read_bytes(), before)

    def test_effective_hash_input_changes_with_translation(self):
        text = 'localization::Tr("resource.gold")'
        before = loc.Catalog.load(self.root).render(text, expanded=True)
        self.write_catalog(russian='Монеты: %s %d')
        after = loc.Catalog.load(self.root).render(text, expanded=True)
        self.assertNotEqual(before, after)

    def test_english_correction_preserves_compiler_view(self):
        source = 'char* text = localization::Tr("resource.gold");'
        before = loc.Catalog.load(self.root)
        self.write_catalog(english='Original gold: %s %d')
        after = loc.Catalog.load(self.root)
        self.assertNotEqual(before.english, after.english)
        self.assertEqual(before.russian, after.russian)
        self.assertEqual(before.header(), after.header())
        for expanded in (False, True):
            self.assertEqual(before.render(source, expanded=expanded),
                             after.render(source, expanded=expanded))

    def test_locale_selects_header_and_expanded_literals(self):
        catalog = loc.Catalog.load(self.root)
        text = 'localization::Tr("resource.gold")'
        self.assertEqual(catalog.render(text, expanded=True, locale='en'), '"Gold: %s %d"')
        self.assertIn('"Gold: %s %d"', catalog.header('en'))
        self.assertEqual(catalog.header(), catalog.header('ru'))
        self.assertEqual(catalog.render(text, locale='en'), catalog.render(text))
        with self.assertRaisesRegex(ValueError, 'unsupported locale'):
            catalog.header('pl')

    def test_english_prepare_cannot_replace_russian_compiler_view(self):
        source = self.root / 'src/test.cpp'
        source.write_text('char* text = localization::Tr("resource.gold");')
        _, ru_header, _, _ = loc.prepare(self.root, source)
        before = ru_header.read_bytes()
        _, en_header, _, _ = loc.prepare(self.root, source, locale='en')
        self.assertTrue(en_header.is_relative_to(self.root / 'build/ordinary/en'))
        self.assertNotEqual(en_header.read_bytes(), before)
        self.assertEqual(ru_header.read_bytes(), before)


    def test_clang_checks_real_printf_arguments(self):
        source = self.root / 'src/test.cpp'
        prefix = 'extern "C" int printf(const char*, ...);\n'
        source.write_text(prefix + 'void f() { printf(localization::Tr("resource.gold"), "gold", 3); }')
        self.assertEqual(loc.check_formats(self.root, source), [])
        source.write_text(prefix + 'void f() { printf(localization::Tr("resource.gold"), 3, "gold"); }')
        errors = loc.check_formats(self.root, source)
        self.assertTrue(errors)
        self.assertIn('format', errors[0])

    def test_english_output_guard_precedes_compilation(self):
        from homm1.tool import cl, ToolError
        with mock.patch('homm1.graph.localization.matching_locale', return_value='ru'), \
                self.assertRaisesRegex(ToolError, 'build/ordinary/en'):
            cl.compile(self.root / 'src/missing.cpp', self.root / 'bad.obj', [], locale='en')

    def test_registry_source_values_match_retained_snapshot(self):
        # Pointer/literal provenance is independently recorded for every ID.
        import csv
        root = Path(__file__).resolve().parents[3]
        catalog = loc.Catalog.load(root)
        def table(name):
            with (root / 'config/retail' / name).open() as stream:
                return list(csv.DictReader(stream, delimiter='\t'))
        rows = table('localization.tsv')
        resource_rows = table('localization_resources.tsv')
        fixed_rows = table('localization_fixed_width.tsv')
        self.assertEqual(set(catalog.english),
                         {row['id'] for row in rows} | {row['id'] for row in resource_rows}
                         | {row['id'] for row in fixed_rows})
        self.assertEqual(catalog.english['table.gResourceNames.0'], 'Wood')
        import hashlib
        for row in rows:
            payload = catalog.russian[row['id']].encode('cp1251') + b'\0'
            self.assertEqual(hashlib.sha256(payload).hexdigest(), row['russian_sha256'])

        for row in fixed_rows:
            self.assertEqual(catalog.english[row['id']], row['english'])
            payload = catalog.russian[row['id']].encode('cp1251')
            self.assertEqual(len(payload), int(row['size']))
            self.assertEqual(hashlib.sha256(payload).hexdigest(), row['payload_sha256'])

        for row in resource_rows:
            for name, messages in [('english', catalog.english), ('russian', catalog.russian)]:
                payload = messages[row['id']].encode('utf-16le')
                self.assertEqual(hashlib.sha256(payload).hexdigest(), row[name + '_sha256'])

    def test_resource_render_selects_language_and_preserves_english(self):
        self.write_catalog('Gold', 'Золото')
        source = 'LANGUAGE HOMM1_RESOURCE_LANGUAGE, 1\nMENUITEM localization::Tr("resource.gold"), 42\n'
        catalog = loc.Catalog.load(self.root)
        russian = catalog.render_resource(source)
        english = catalog.render_resource(source, locale='en')
        self.assertTrue(russian.isascii())
        self.assertIn('HOMM1_RESOURCE_LANGUAGE 0x19', russian)
        self.assertIn('HOMM1_RESOURCE_LANGUAGE 0x09', english)
        from homm1.graph.catalog import resource_literal
        self.assertIn('MENUITEM ' + resource_literal('Gold') + ', 42', english)
        self.assertIn('MENUITEM ' + resource_literal('Золото') + ', 42', russian)
        self.assertIn(r'\x0417\x043e', russian)

    def test_resource_inline_russian_is_rejected(self):
        (self.root / 'src/menu.rc').write_text('MENUITEM "Золото", 42')
        errors, _used = loc.check_tree(self.root)
        self.assertTrue(any('inline non-ASCII' in error for error in errors))

    def test_nonmatching_resource_output_guard_precedes_compilation(self):
        from homm1.tool import rc, ToolError
        with mock.patch('homm1.graph.localization.matching_locale', return_value='ru'), \
                self.assertRaisesRegex(ToolError, 'build/ordinary/en'):
            rc.compile(self.root / 'src/missing.rc', self.root / 'bad.res', locale='en')

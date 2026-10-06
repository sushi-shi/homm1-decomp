"""Readable authored text, exact compiler bytes, and real Clang format checking."""
import ast
import json
from pathlib import Path
import tempfile
import unittest
from unittest import mock

from homm1.graph import catalog as cat
from homm1.graph import localization as loc

RU = {'name': 'Russian', 'codepage': 1251, 'resource_language': '0x0419',
      'system_locale': 'ru_RU.UTF-8', 'glyphs': 'cyrillic',
      'keyboard': {'keys': 'qQ', 'typed': 'йЙ'}}
EN = {'name': 'English', 'codepage': 1252, 'resource_language': '0x0409',
      'system_locale': 'en_US.UTF-8', 'glyphs': 'ascii', 'keyboard': {'keys': '', 'typed': ''}}


def po(code, messages, flags=()):
    return cat.write_po([('Language', code)], (), [
        cat.Entry(key, value, flags=list(flags)) for key, value in messages.items()])


class LocalizationTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name)
        for name in ('locales', 'src', 'include'):
            (self.root / name).mkdir()
        (self.root / 'locales/ru.json').write_text(json.dumps(RU), encoding='utf-8')
        (self.root / 'locales/en.json').write_text(json.dumps(EN), encoding='utf-8')
        self.write_catalog()

    def write_catalog(self, english='Gold: %s %d', russian='Золото: %s %d', *,
                      chars=False, files=('src/test.cpp',)):
        found = {'resource.gold': {'files': list(files), 'chars': chars}}
        (self.root / 'locales/messages.pot').write_text(cat.template_text(found), encoding='utf-8')
        (self.root / 'locales/en.po').write_text(po('en', {'resource.gold': english}), encoding='utf-8')
        (self.root / 'locales/ru.po').write_text(po('ru', {'resource.gold': russian}), encoding='utf-8')

    def load(self):
        return loc.Catalog.load(self.root)

    def errors(self):
        return cat.Catalog.validate(cat.Catalog.files_of(self.root))[1]

    def assertInvalid(self, pattern):
        with self.assertRaisesRegex(ValueError, pattern):
            self.load()

    # -- encoding -----------------------------------------------------------

    def test_code_page_bytes_and_escape_boundaries(self):
        text = 'ЁёЯя A9"\\\n\t\0'
        encoded = loc.literal(text)
        self.assertTrue(encoded.isascii())
        self.assertEqual(ast.literal_eval(encoded).encode('latin1'), text.encode('cp1251'))
        self.assertEqual(ast.literal_eval(loc.literal('é', 1252)).encode('latin1'), b'\xe9')

    def test_unrepresentable_translation_is_fatal(self):
        self.write_catalog('Gold', 'Золото 🪙')
        self.assertInvalid('not in code page 1251')
        self.write_catalog('Gold Ж', 'Золото')
        self.assertInvalid('en.po: resource.gold: .* not in code page 1252')

    def test_text_needs_game_font_glyphs_outside_resources(self):
        self.write_catalog('Café', 'Золото')
        self.assertInvalid('en.po: resource.gold: no game-font glyph')
        self.write_catalog('Café', 'Золото', files=('src/menu.rc',))
        self.assertEqual(self.load().messages('en')['resource.gold'], 'Café')

    # -- source calls -------------------------------------------------------

    def test_unknown_id_is_fatal(self):
        with self.assertRaisesRegex(ValueError, 'unknown localization ID'):
            self.load().render('localization::Tr("resource.unknown")', locale='ru')

    def test_dynamic_id_is_fatal(self):
        for value in ('id', '"resource.gold", count', '"resource." "gold"'):
            with self.subTest(value=value), self.assertRaisesRegex(ValueError, 'one literal'):
                self.load().render('localization::Tr(' + value + ')', locale='ru')

    def test_comments_and_quoted_code_are_not_calls(self):
        text = '// localization::Tr(id)\n/* localization::Tr(id) */\n"localization::Tr(id)"'
        self.assertEqual(self.load().render(text, locale='ru'), text)

    def test_generated_view_preserves_utf8_offsets_and_lines(self):
        catalog = self.load()
        text = '// Русский комментарий\na=localization :: Tr(\n "resource.gold"\n);\nDATA(123) int n;'
        generated = catalog.render(text, locale='ru')
        self.assertEqual(len(text.encode()), len(generated.encode()))
        self.assertEqual([i for i, b in enumerate(text.encode()) if b == 10],
                         [i for i, b in enumerate(generated.encode()) if b == 10])
        self.assertEqual(text.encode().index(b'DATA'), generated.encode().index(b'DATA'))
        self.assertIn(catalog.macro('resource.gold'), generated)
        self.assertEqual(generated, catalog.render(text, locale='en'))

    def test_expansion_preserves_packed_storage_nuls(self):
        self.write_catalog('Gold', 'Золото')
        text = 'localization::Tr("resource.gold") "\\0\\0" localization::Tr("resource.gold")'
        rendered = self.load().render(text, expanded=True, locale='ru')
        value = ''.join(ast.literal_eval(t.group()) for t in loc.tokens(rendered))
        self.assertEqual(value.encode('latin1'), 'Золото\0\0Золото'.encode('cp1251'))

    # -- fixed-width fields -------------------------------------------------

    def test_fixed_array_has_exact_characters_without_added_terminator(self):
        self.write_catalog("A'\\ Z", "Я'\\ Ю", chars=True)
        catalog = self.load()
        source = self.root / 'src/test.cpp'
        text = ('struct Record { char name[5]; };\n'
                'Record value = { localization::Chars("resource.gold") };\n')
        source.write_text(text)
        for locale in ('en', 'ru'):
            current = catalog.locale(locale)
            expanded = catalog.render(text, expanded=True, locale=locale)
            chars = [ast.literal_eval(t.group()) for t in loc.tokens(expanded)
                     if t.lastgroup == 'char']
            self.assertEqual(''.join(chars).encode('latin1'),
                             current.messages['resource.gold'].encode(f'cp{current.codepage}'))
            self.assertEqual(len(chars), 5)
            compiled, header, _, _ = loc.prepare(self.root, source, locale=locale)
            self.assertIn(catalog.macro('resource.gold', chars=True), compiled.read_text())
            self.assertIn('#define ' + catalog.macro('resource.gold', chars=True),
                          header.read_text())
            self.assertEqual(loc.check_formats(self.root, source, locale=locale), [])
        self.assertEqual(list(catalog.calls(text))[0][2], 'resource.gold')
        self.assertNotIn('H1C', catalog.header('ru'))

    def test_fixed_width_text_has_one_byte_length(self):
        self.write_catalog('ABC', 'АБВГ', chars=True)
        self.assertInvalid('fixed-width text differs in byte length: en 3; ru 4')
        self.write_catalog('ABC', 'АБВГ')
        self.load()

    def test_fixed_array_preserves_offsets_and_compiler_checks_its_extent(self):
        self.write_catalog('ABCD', 'АБВГ', chars=True)
        catalog = self.load()
        text = ('char value[3] = localization :: Chars(\n "resource.gold"\n);\n'
                'int following;\n')
        generated = catalog.render(text, locale='ru')
        self.assertEqual(len(text.encode()), len(generated.encode()))
        self.assertEqual(text.index('following'), generated.index('following'))
        self.assertEqual([i for i, c in enumerate(text) if c == '\n'],
                         [i for i, c in enumerate(generated) if c == '\n'])
        source = self.root / 'src/test.cpp'
        source.write_text(text)
        for locale in ('en', 'ru'):
            self.assertTrue(loc.check_formats(self.root, source, locale=locale))
        with self.assertRaisesRegex(ValueError, 'one literal'):
            catalog.render('localization::Chars(dynamic_id)', locale='ru')
        with self.assertRaisesRegex(ValueError, 'resource strings'):
            catalog.render_resource('localization::Chars("resource.gold")', locale='ru')
        quoted = '// localization::Chars(dynamic_id)\n"localization::Chars(dynamic_id)"'
        self.assertEqual(catalog.render(quoted, locale='ru'), quoted)

    # -- descriptors --------------------------------------------------------

    def test_keyboard_table_is_descriptor_data(self):
        catalog = self.load()
        text = 'unsigned char map[0x80] = localization::Chars("locale.keyboard");'
        for locale, typed in (('ru', 'йЙ'), ('en', 'qQ')):
            expanded = catalog.render(text, expanded=True, locale=locale)
            chars = ''.join(ast.literal_eval(t.group()) for t in loc.tokens(expanded)
                            if t.lastgroup == 'char').encode('latin1')
            expected = bytearray(range(128))
            expected[ord('q')], expected[ord('Q')] = typed[0].encode('cp1251')[0], \
                typed[1].encode('cp1251')[0]
            self.assertEqual(chars, bytes(expected))
        source = self.root / 'src/test.cpp'
        source.write_text(text + '\n')
        self.assertEqual(loc.check_formats(self.root, source, locale='ru'), [])

    def test_descriptor_fields_are_checked(self):
        for change, pattern in ((dict(codepage=932), 'single-byte'),
                                (dict(codepage=99999), 'single-byte'),
                                (dict(resource_language='0x0400'), 'LANGID'),
                                (dict(glyphs='greek'), 'glyphs must be'),
                                (dict(keyboard={'keys': 'q', 'typed': 'йй'}), 'equal length'),
                                (dict(keyboard={'keys': 'é', 'typed': 'й'}), 'printable ASCII'),
                                (dict(keyboard={'keys': 'q', 'typed': '🪙'}), 'not in code page'),
                                (dict(extra=1), 'unknown field')):
            with self.subTest(change=change):
                (self.root / 'locales/ru.json').write_text(json.dumps({**RU, **change}))
                self.assertInvalid(pattern)

    def test_language_needs_po_and_descriptor(self):
        (self.root / 'locales/de.json').write_text(json.dumps(EN))
        self.assertInvalid('de: a language needs both de.po and de.json')

    # -- catalog coverage ---------------------------------------------------

    def test_stale_and_missing_entries_are_fatal(self):
        (self.root / 'locales/ru.po').write_text(po('ru', {'resource.gold': 'Золото: %s %d',
                                                           'resource.old': 'Старое'}))
        self.assertInvalid('ru.po: resource.old: stale ID')
        (self.root / 'locales/ru.po').write_text(po('ru', {}))
        self.assertInvalid(r'ru.po: 1 ID\(s\) without an entry: resource.gold')
        (self.root / 'locales/ru.po').write_text(po('ru', {'resource.gold': ''}))
        self.assertInvalid('ru.po: resource.gold: missing translation')

    def test_header_language_is_checked(self):
        (self.root / 'locales/ru.po').write_text(po('en', {'resource.gold': 'Золото: %s %d'}))
        self.assertInvalid('ru.po: the header Language must be ru')

    def test_catalog_cannot_hide_unicode_in_escapes(self):
        path = self.root / 'locales/ru.po'
        path.write_text(path.read_text().replace('З', r'\u0417'))
        self.assertInvalid('numeric text escape')

    def test_placeholder_mismatch_between_languages_is_fatal(self):
        for ru in ('Золото: %s', 'Золото: %d %s', 'Золото: %s %ld', 'Золото: %s %d %d'):
            with self.subTest(ru=ru):
                self.write_catalog(russian=ru)
                self.assertInvalid('resource.gold: printf arguments differ')

    def test_stars_and_length_modifiers(self):
        self.assertEqual(loc.format_signature('%*.*s %ld %%'), ['*', '*', 's', 'ld'])
        self.assertEqual(loc.format_signature('50% protection and 10% of the cost'), [])

    def test_duplicate_ids_and_fuzzy_translations_are_fatal(self):
        path = self.root / 'locales/ru.po'
        path.write_text(path.read_text() + '\nmsgid "resource.gold"\nmsgstr "Золото: %s %d"\n')
        self.assertInvalid('duplicate entry')
        self.write_catalog()
        path.write_text(po('ru', {'resource.gold': 'Золото: %s %d'}, flags=('fuzzy',)))
        self.assertInvalid('fuzzy')

    def test_all_errors_are_reported_together(self):
        (self.root / 'locales/ru.po').write_text(po('ru', {'resource.gold': ''}))
        (self.root / 'locales/en.json').write_text(json.dumps({**EN, 'glyphs': 'x'}))
        self.assertEqual(len(self.errors()), 2)

    # -- template maintenance -----------------------------------------------

    def test_update_regenerates_template_and_rewrites_languages(self):
        (self.root / 'src/a.cpp').write_text(
            'f(localization::Tr("menu.item.10"), localization::Tr("menu.item.2"));\n'
            'g(localization::Tr("resource.gold"));\n')
        (self.root / 'src/b.rc').write_text('MENUITEM localization::Tr("menu.item.2"), 1\n')
        (self.root / 'locales/en.po').write_text(po('en', {'resource.gold': 'Gold: %s %d',
                                                           'resource.old': 'Old'}))
        changed, stale = cat.update(self.root, 'ru')
        self.assertEqual(sorted(changed), ['en.po', 'messages.pot', 'ru.po'])
        self.assertEqual(stale, {'en': ['resource.old']})
        _, template = cat.parse_po((self.root / 'locales/messages.pot').read_text())
        self.assertEqual([e.msgid for e in template], ['menu.item.2', 'menu.item.10', 'resource.gold'])
        self.assertEqual(template[0].references, ['src/a.cpp', 'src/b.rc'])
        self.assertEqual(template[2].flags, ['c-format'])
        _, english = cat.parse_po((self.root / 'locales/en.po').read_text())
        self.assertEqual([(e.msgid, e.msgstr) for e in english],
                         [('menu.item.2', ''), ('menu.item.10', ''), ('resource.gold', 'Gold: %s %d')])
        self.assertEqual(cat.update(self.root, 'ru', check=True), ([], {}))
        self.assertInvalid('menu.item.2: missing translation')

    def test_update_creates_a_new_language(self):
        (self.root / 'src/test.cpp').write_text('localization::Tr("resource.gold");\n')
        cat.update(self.root, 'ru')
        (self.root / 'locales/de.json').write_text(json.dumps(EN))
        changed, _ = cat.update(self.root, 'ru')
        self.assertEqual(changed, ['de.po'])
        self.assertIn('msgid "resource.gold"\nmsgstr ""',
                      (self.root / 'locales/de.po').read_text())

    def test_check_reports_stale_template_and_inline_text(self):
        (self.root / 'src/test.cpp').write_text('localization::Tr("resource.gold");\n')
        cat.update(self.root, 'ru')
        self.assertEqual(cat.check(self.root, 'ru'), [])
        (self.root / 'include/bad.h').write_text("char bad = '\\317'; localization::Tr(\"x.y\");")
        errors = cat.check(self.root, 'ru')
        self.assertTrue(any('bad.h:1: numeric text escape' in e for e in errors))
        self.assertTrue(any('messages.pot is out of date' in e for e in errors))
        (self.root / 'include/bad.h').write_text('char* x = localization::Tr("locale.keyboard");')
        self.assertTrue(any('descriptors supply only' in e for e in cat.check(self.root, 'ru')))

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

    def test_resource_inline_russian_is_rejected(self):
        (self.root / 'src/menu.rc').write_text('MENUITEM "Золото", 42')
        self.assertTrue(any('inline non-ASCII' in e for e in cat.check(self.root, 'ru')))

    # -- compiler views -----------------------------------------------------

    def test_header_localization_reaches_both_compilers(self):
        source = self.root / 'src/test.cpp'
        source.write_text('#include <outer.h>\n')
        (self.root / 'include/outer.h').write_text('#include "inner.h"\n')
        (self.root / 'include/inner.h').write_text('char* text = localization::Tr("resource.gold");\n')
        with mock.patch('homm1.graph.localization.matching_locale', return_value='ru'):
            compiled, header, overlay, _ = loc.prepare(self.root, source)
        self.assertNotEqual(compiled, source)
        self.assertIn('H1L', (header.parent / 'include/inner.h').read_text())
        self.assertTrue((header.parent / 'include/outer.h').is_file())
        self.assertTrue(any(e['name'].endswith('/inner.h') for e in json.loads(overlay.read_text())['roots']))

    def test_prepare_tracks_catalog_and_source_edits(self):
        source = self.root / 'src/test.cpp'
        source.write_text('char* text = localization::Tr("resource.gold");\n')
        compiled, header, overlay, deps = loc.prepare(self.root, source, locale='ru')
        self.assertEqual(compiled.name, source.name)
        self.assertNotEqual(compiled, source)
        self.assertEqual(len(compiled.read_bytes()), len(source.read_bytes()))
        self.assertEqual(json.loads(overlay.read_text())['roots'][0]['name'], str(source))
        for name in ('messages.pot', 'ru.po', 'en.po', 'ru.json', 'en.json'):
            self.assertIn(self.root / 'locales' / name, deps)
        before = header.read_bytes()
        self.write_catalog(russian='Монеты: %s %d')
        _, changed, _, _ = loc.prepare(self.root, source, locale='ru')
        self.assertNotEqual(changed.read_bytes(), before)

    def test_one_language_edit_leaves_the_others_compiler_view(self):
        source = 'char* text = localization::Tr("resource.gold");'
        before = self.load()
        self.write_catalog(english='Coins: %s %d')
        after = self.load()
        self.assertNotEqual(before.messages('en'), after.messages('en'))
        self.assertEqual(before.header('ru'), after.header('ru'))
        self.assertEqual(before.render(source, expanded=True, locale='ru'),
                         after.render(source, expanded=True, locale='ru'))
        self.assertNotEqual(before.render(source, expanded=True, locale='en'),
                            after.render(source, expanded=True, locale='en'))

    def test_locale_selects_header_and_expanded_literals(self):
        catalog = self.load()
        text = 'localization::Tr("resource.gold")'
        self.assertEqual(catalog.render(text, expanded=True, locale='en'), '"Gold: %s %d"')
        self.assertIn('"Gold: %s %d"', catalog.header('en'))
        with self.assertRaisesRegex(ValueError, 'unsupported locale'):
            catalog.header('pl')

    def test_nonmatching_prepare_cannot_replace_matching_compiler_view(self):
        source = self.root / 'src/test.cpp'
        source.write_text('char* text = localization::Tr("resource.gold");')
        with mock.patch('homm1.graph.localization.matching_locale', return_value='ru'):
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
        for locale in ('ru', 'en'):
            self.assertEqual(loc.check_formats(self.root, source, locale=locale), [])
        source.write_text(prefix + 'void f() { printf(localization::Tr("resource.gold"), 3, "gold"); }')
        errors = loc.check_formats(self.root, source, locale='ru')
        self.assertTrue(errors)
        self.assertIn('format', errors[0])

    def test_nonmatching_output_guard_precedes_compilation(self):
        from homm1.tool import cl, rc, ToolError
        with mock.patch('homm1.graph.localization.matching_locale', return_value='ru'):
            with self.assertRaisesRegex(ToolError, 'build/ordinary/en'):
                cl.compile(self.root / 'src/missing.cpp', self.root / 'bad.obj', [], locale='en')
            with self.assertRaisesRegex(ToolError, 'build/ordinary/en'):
                rc.compile(self.root / 'src/missing.rc', self.root / 'bad.res', locale='en')
            with self.assertRaisesRegex(ToolError, 'unsupported locale: pl'):
                cl.compile(self.root / 'src/missing.cpp', self.root / 'bad.obj', [], locale='pl')

    # -- resources ----------------------------------------------------------

    def test_resource_render_selects_language_and_text(self):
        self.write_catalog('Gold', 'Золото', files=('src/menu.rc',))
        source = ('LANGUAGE HOMM1_RESOURCE_LANGUAGE, HOMM1_RESOURCE_SUBLANGUAGE\n'
                  'MENUITEM localization::Tr("resource.gold"), 42\n')
        catalog = self.load()
        russian = catalog.render_resource(source, locale='ru')
        english = catalog.render_resource(source, locale='en')
        self.assertTrue(russian.isascii())
        self.assertTrue(russian.startswith('#define HOMM1_RESOURCE_LANGUAGE 0x19\n'
                                           '#define HOMM1_RESOURCE_SUBLANGUAGE 0x1\n'))
        self.assertIn('HOMM1_RESOURCE_LANGUAGE 0x09', english)
        self.assertIn('MENUITEM ' + cat.resource_literal('Gold') + ', 42', english)
        self.assertIn('MENUITEM ' + cat.resource_literal('Золото') + ', 42', russian)
        self.assertIn(r'\x0417\x043e', russian)

    # -- the repository's catalogs ------------------------------------------

    def test_repository_catalogs_and_retail_provenance(self):
        # Pointer/literal provenance of the retail Russian is recorded for every ID.
        import csv
        import hashlib
        root = Path(__file__).resolve().parents[3]
        catalog = loc.Catalog.load(root)
        self.assertEqual(cat.check(root, loc.matching_locale(root)), [])

        def table(name):
            with (root / 'config/retail' / name).open() as stream:
                return list(csv.DictReader(stream, delimiter='\t'))
        rows = table('localization.tsv')
        # Another image's own catalog entries keep their provenance beside
        # that image's retail facts.
        for image_table in sorted((root / 'config/retail').glob('*/localization.tsv')):
            rows += table(image_table.relative_to(root / 'config/retail').as_posix())
        resource_rows = table('localization_resources.tsv')
        fixed_rows = table('localization_fixed_width.tsv')
        # The fixed-width rows include INPUTMGR's key table, which the Russian
        # descriptor supplies (Buka's ЙЦУКЕН layout on US keys).
        self.assertEqual({*catalog.ids, cat.KEYBOARD_ID},
                         {row['id'] for row in rows} | {row['id'] for row in resource_rows}
                         | {row['id'] for row in fixed_rows})
        russian = catalog.messages('ru')
        for row in rows:
            payload = russian[row['id']].encode('cp1251') + b'\0'
            self.assertEqual(hashlib.sha256(payload).hexdigest(), row['russian_sha256'], row['id'])
        for row in fixed_rows:
            payload = russian[row['id']].encode('cp1251')
            self.assertEqual(len(payload), int(row['size']))
            self.assertEqual(hashlib.sha256(payload).hexdigest(), row['payload_sha256'])
        for row in resource_rows:
            payload = russian[row['id']].encode('utf-16le')
            self.assertEqual(hashlib.sha256(payload).hexdigest(), row['russian_sha256'])


if __name__ == '__main__':
    unittest.main()

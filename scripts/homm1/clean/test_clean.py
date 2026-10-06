"""The clean generator's variants, classic rendering and verification helpers."""
import json
import unittest

from homm1.clean import classic, source, verify
from homm1.graph.catalog import Catalog, Entry, literal, resource_literal, template_text, write_po

MESSAGES = {'ru': {'ui.gold': 'Золото', 'ui.quote': 'Скажи "да"\n'},
            'en': {'ui.gold': 'Gold', 'ui.quote': 'Say "hi"\n'}}
DESCRIPTORS = {'ru': {'name': 'Russian', 'codepage': 1251, 'resource_language': '0x0419',
                      'system_locale': 'ru_RU.UTF-8', 'glyphs': 'cyrillic',
                      'keyboard': {'keys': '', 'typed': ''}},
               'en': {'name': 'English', 'codepage': 1252, 'resource_language': '0x0409',
                      'system_locale': 'en_US.UTF-8', 'glyphs': 'ascii',
                      'keyboard': {'keys': '', 'typed': ''}}}


def catalog():
    files = {'messages.pot': template_text({key: {'files': ['src/x.cpp'], 'chars': False}
                                            for key in MESSAGES['ru']})}
    for code, messages in MESSAGES.items():
        files[f'{code}.po'] = write_po([('Language', code)], (),
                                       [Entry(k, v) for k, v in messages.items()])
        files[f'{code}.json'] = json.dumps(DESCRIPTORS[code])
    return Catalog.parse(files)


class ClassicRenderingTests(unittest.TestCase):
    def test_literal_is_readable_and_controls_stay_escaped(self):
        self.assertEqual(classic.readable_literal('Золото\n\t"\\\x01' + '7'),
                         '"Золото\\n\\t\\"\\\\\\0017"')

    def test_resource_literal_doubles_quotes(self):
        self.assertEqual(classic.resource_literal('a "b"\nc'), '"a ""b""\\nc"')

    def test_cpp_references_become_literals_of_the_language(self):
        text = 'const char *g = localization::Tr("ui.gold");\n'
        self.assertEqual(classic.render_cpp(text, catalog(), 'ru'),
                         'const char *g = "Золото";\n')
        self.assertEqual(classic.render_cpp(text, catalog(), 'en'),
                         'const char *g = "Gold";\n')

    def test_rc_strings_language_and_code_page(self):
        text = ('LANGUAGE HOMM1_RESOURCE_LANGUAGE, HOMM1_RESOURCE_SUBLANGUAGE\n'
                'STRINGTABLE { 1, localization::Tr("ui.quote") }\n')
        rendered = classic.render_rc(text, catalog(), 'ru')
        self.assertTrue(rendered.startswith('#pragma code_page(65001)\n'))
        self.assertIn('LANGUAGE 0x19, 0x1', rendered)
        self.assertIn('"Скажи ""да""\\n"', rendered)

    def test_rc_rejects_character_arrays(self):
        with self.assertRaises(ValueError):
            classic.render_rc('localization::Chars("ui.gold")', catalog(), 'ru')


class EquivalenceTests(unittest.TestCase):
    def test_literal_bytes(self):
        self.assertEqual(verify.literal_bytes('"Я\\n\\0017\\x41"'), b'\xdf\n\x017A')

    def test_classic_literal_equals_escaped_reference(self):
        reference = 'const char *g = ' + literal('Золото') + ';'
        self.assertIsNone(verify._equivalent_cpp('const char *g = "Золото";', reference))
        self.assertIsNotNone(verify._equivalent_cpp('const char *g = "Злато";', reference))

    def test_classic_literal_in_another_code_page(self):
        reference = 'const char *g = ' + literal('Café', 1252) + ';'
        self.assertIsNone(verify._equivalent_cpp('const char *g = "Café";', reference, 1252))

    def test_classic_literal_equals_character_initializer(self):
        self.assertIsNone(verify._equivalent_cpp('char g[2] = "Да";',
                                                 "char g[2] = {'\\xc4', '\\xe0'};"))
        self.assertIsNotNone(verify._equivalent_cpp('char g[2] = "Да";',
                                                    "char g[2] = {'\\xc4', '\\xe1'};"))

    def test_rc_classic_against_rendered_resource(self):
        text = ('LANGUAGE HOMM1_RESOURCE_LANGUAGE, HOMM1_RESOURCE_SUBLANGUAGE\n'
                'STRINGTABLE { 1, localization::Tr("ui.quote") }\n')
        for locale in ('ru', 'en'):
            reference = catalog().render_resource(text, locale=locale)
            self.assertIn('L"', reference)
            rendered = classic.render_rc(text, catalog(), locale)
            self.assertIsNone(verify._equivalent_rc(rendered, reference, catalog(), locale))
        changed = classic.render_rc(text, catalog(), 'ru').replace('да', 'нет')
        reference = catalog().render_resource(text, locale='ru')
        self.assertIsNotNone(verify._equivalent_rc(changed, reference, catalog(), 'ru'))
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


class LayoutAliasTests(unittest.TestCase):
    HEADER = ("#define gGame gpGame // spelling fixes .bss order\n"
              "extern class game* gGame;\n")

    def test_the_source_tree_drops_the_define_and_keeps_the_readable_name(self):
        cleaned = source.clean_cpp(self.HEADER)
        self.assertNotIn("define", cleaned)
        self.assertIn("extern class game* gGame;", cleaned)

    def test_the_control_tree_keeps_the_define(self):
        self.assertIn("#define gGame gpGame", source.clean_cpp(self.HEADER, keep_lines=True))

    def test_assembly_references_take_the_readable_name(self):
        self.assertEqual(source.aliases([self.HEADER]), {"gpGame": "gGame"})
        cleaned = source.clean_asm("EXTERN gpGame:DWORD ; gpGame\n",
                                   renames={"gpGame": "gGame"})
        self.assertEqual(cleaned.strip(), "EXTERN gGame:DWORD")

    def test_matching_symbols_compare_under_the_readable_name(self):
        sections = [{"relocations": [(0, 6, "?gpGame@@3PAVgame@@A")],
                     "defines": ["_?s_x_4@?1??f@@YAXXZ@4HA"]}]
        renamed, symbols = verify._renamed(sections, [("?gpGame@@3PAVgame@@A", 3)],
                                           {"gpGame": "gGame", "s_x_4": "s_x"})
        self.assertEqual(renamed[0]["relocations"][0][2], "?gGame@@3PAVgame@@A")
        self.assertEqual(renamed[0]["defines"], ["_?s_x@?1??f@@YAXXZ@4HA"])
        self.assertEqual(symbols, [("?gGame@@3PAVgame@@A", 3)])

    def test_c_linkage_symbols_take_the_readable_name(self):
        sections = [{"relocations": [(8, 6, "_decodeSkip")], "defines": ["_decodeSkip"]}]
        renamed, _ = verify._renamed(sections, [], {"decodeSkip": "textsize"})
        self.assertEqual(renamed[0]["relocations"][0][2], "_textsize")
        self.assertEqual(renamed[0]["defines"], ["_textsize"])

    @staticmethod
    def _bss(size, defines):
        return {"name": ".bss", "flags": 0xC0300080, "data": size,
                "relocations": [], "defines": defines}

    def test_relaid_bss_differs_by_padding_alone(self):
        readable = {"putbuf"}
        theirs = self._bss(33683, ["_putlen", "_putbuf", "_text_buf"])
        self.assertTrue(verify._relaid_bss(
            self._bss(33680, ["_putbuf", "_putlen", "_text_buf"]), theirs, readable))
        self.assertFalse(verify._relaid_bss(
            self._bss(33680, ["_putbuf", "_text_buf"]), theirs, readable))
        self.assertFalse(verify._relaid_bss(
            self._bss(33580, ["_putbuf", "_putlen", "_text_buf"]), theirs, readable))
        self.assertFalse(verify._relaid_bss(
            self._bss(33680, ["_putbuf", "_putlen", "_text_buf"]), theirs, {"other"}))


class FrameSlotAliasTests(unittest.TestCase):
    UNIT = ("i32 minX;\n"
            "#define minX ourFirstX // frame-slot spelling\n"
            "#define maxX curEndX // frame-slot spelling\n"
            "void advManager::UpdateRadar(i32 force) {\n"
            "    i32 minX = force;\n"
            "    i32 maxX = minX + 1;\n"
            "    Draw(minX, maxX);\n"
            "}\n"
            "#undef minX\n"
            "#undef maxX\n"
            "\n"
            "void advManager::Other() { i32 ourFirstX = 0; }\n")

    @staticmethod
    def _bracket(body: str) -> str:
        return "#define minX ourFirstX // frame-slot spelling\n" + body + "#undef minX\n"

    def test_groups_cover_defines_through_undefs(self):
        self.assertEqual(source.local_aliases(self.UNIT),
                         [(1, 9, {"minX": "ourFirstX", "maxX": "curEndX"})])

    def test_the_source_tree_drops_the_pair_and_keeps_the_readable_names(self):
        cleaned = source.clean_cpp(self.UNIT)
        self.assertNotIn("#define", cleaned)
        self.assertNotIn("#undef", cleaned)
        self.assertIn("i32 minX = force;\n    i32 maxX = minX + 1;", cleaned)
        # Outside its function the storage spelling is an ordinary name.
        self.assertIn("i32 ourFirstX = 0;", cleaned)

    def test_the_control_tree_keeps_the_pair_and_every_line(self):
        cleaned = source.clean_cpp(self.UNIT, keep_lines=True)
        self.assertIn("#define minX ourFirstX", cleaned)
        self.assertIn("#undef maxX", cleaned)
        self.assertEqual(cleaned.count("\n"), self.UNIT.count("\n"))

    def test_a_define_without_its_undef_fails(self):
        with self.assertRaisesRegex(ValueError, "not #undef'd"):
            source.clean_cpp("#define minX ourFirstX // frame-slot spelling\n"
                             "void f() { i32 minX; }\n")
        with self.assertRaisesRegex(ValueError, "not #undef'd"):
            source.clean_cpp(self.UNIT.replace("#undef maxX\n", ""))

    def test_a_stray_undef_fails(self):
        with self.assertRaisesRegex(ValueError, "outside"):
            source.clean_cpp(self.UNIT + "#undef minX\n")

    def test_the_pair_brackets_exactly_one_function(self):
        with self.assertRaisesRegex(ValueError, "more than one"):
            source.clean_cpp(self._bracket("void f() { i32 minX; }\nvoid g() { }\n"))
        with self.assertRaisesRegex(ValueError, "exactly one"):
            source.clean_cpp(self._bracket("void f() { i32 minX; }\ni32 g;\n"))

    def test_the_storage_spelling_inside_the_function_fails(self):
        with self.assertRaisesRegex(ValueError, "split"):
            source.clean_cpp(self._bracket("void f() { i32 minX; ourFirstX = 1; }\n"))

    def test_the_storage_spelling_may_still_name_a_type(self):
        cleaned = source.clean_cpp(
            "#define moraleSound sample // frame-slot spelling\n"
            "void f() { class sample* moraleSound; Wait(moraleSound); }\n"
            "#undef moraleSound\n")
        self.assertIn("class sample* moraleSound;", cleaned)

    def test_captured_parameters_members_and_qualified_names_fail(self):
        for body in ("void f(i32 minX) { i32 y = minX; }\n",
                     "void f() { i32 minX; minX = box.minX; }\n",
                     "void f() { i32 minX; minX = box->minX; }\n",
                     "void f() { i32 minX; minX = limits::minX; }\n",
                     "void f() { class minX* minX; }\n"):
            with self.subTest(body=body), self.assertRaisesRegex(ValueError, "capture"):
                source.clean_cpp(self._bracket(body))

    def test_an_unused_alias_fails(self):
        with self.assertRaisesRegex(ValueError, "never used"):
            source.clean_cpp(self._bracket("void f() { i32 x; }\n"))

    def test_bss_aliases_are_not_frame_slot_aliases(self):
        self.assertEqual(source.local_aliases(LayoutAliasTests.HEADER), [])

    def test_aliased_units_explain_their_source_differences(self):
        from pathlib import Path
        from tempfile import TemporaryDirectory
        from unittest import mock
        with TemporaryDirectory() as tree:
            Path(tree, "a.cpp").write_text("void f() {}\n")
            with mock.patch("homm1.manifest.all_units",
                            return_value=[{"unit": "A", "source": "a.cpp"}]):
                self.assertEqual(verify.unexplained_differences(Path(tree), {"A": [".text"]}),
                                 ["A"])
                self.assertEqual(verify.unexplained_differences(
                    Path(tree), {"A": [".text"]}, {"a.cpp"}), [])


class EditorTargetTests(unittest.TestCase):
    UNITS = """
[build]
compiler = "vc6"
[flags]
cpp = ["/c", "/Od"]
cpp_oi = ["/c", "/Od", "/Oi"]
[images.editor]
defines = ["HOMM1_EDITOR"]
[[unit]]
unit = "SOURCE/GAME"
source = "src/SOURCE/GAME.cpp"
flags = "cpp"
[[unit]]
unit = "SOURCE/kbwin"
source = "src/SOURCE/kbwin.cpp"
flags = "cpp"
images = ["game", "editor"]
image_flags = { editor = "cpp_oi" }
[[unit]]
unit = "BASE/WINDOW"
source = "src/BASE/WINDOW.cpp"
flags = "cpp"
images = ["game", "editor"]
[[unit]]
unit = "EDITOR/EDITOR"
source = "src/EDITOR/EDITOR.cpp"
flags = "cpp_oi"
images = ["editor"]
"""
    ORDER = ("# reviewed spans\nindex\tunit\tlo\thi\tclass\n"
             "0\tEDITOR/EDITOR\t0x00001000\t0x00002000\tcode\n"
             "1\tSOURCE/kbwin\t0x00002000\t0x00003000\tcode\n"
             "2\tBASE/WINDOW\t0x0001b500\t0x0001c000\tcode\n"
             "3\tSOURCE/kbwin\t0x00001800\t0x00001900\tcode\n")

    def files(self):
        return {"config/units.toml": self.UNITS.encode(),
                "config/retail/targets.json": json.dumps(
                    {"game": {"name": "HEROES.EXE", "locale": "ru"},
                     "editor": {"name": "EDITOR.EXE"}}).encode(),
                "config/retail/editor/link_order.tsv": self.ORDER.encode(),
                "src/SOURCE/GAME.cpp": b"", "src/SOURCE/kbwin.cpp": b"",
                "src/BASE/WINDOW.cpp": b"", "src/EDITOR/EDITOR.cpp": b"",
                "src/SOURCE/Heroes.rc": b"", "src/EDITOR/Editor.rc": b"",
                "include/EDITOR/EDITOR.h": b""}

    def test_the_trees_carry_the_editor_sources_and_resources(self):
        from homm1.clean import run
        chosen = run.selected(self.files())
        for name in ("src/EDITOR/EDITOR.cpp", "src/EDITOR/Editor.rc", "include/EDITOR/EDITOR.h",
                     "src/SOURCE/GAME.cpp"):
            self.assertIn(name, chosen)

    def test_the_editor_target_orders_its_link_and_defines_its_image(self):
        from homm1.clean import project
        files = self.files()
        self.assertEqual(project.images(files), ["game", "editor"])
        self.assertEqual(project.image_starts(files, "editor"),
                         {"EDITOR/EDITOR": 0x1000, "SOURCE/kbwin": 0x1800, "BASE/WINDOW": 0x1b500})
        editor = project.target(files, "editor")
        self.assertEqual(editor["executable"], "EDITOR.EXE")
        self.assertEqual(editor["resources"], "src/EDITOR/Editor.rc")
        self.assertEqual(editor["link"]["objects"], ["EDITOR/EDITOR", "SOURCE/kbwin"])
        self.assertEqual(editor["link"]["members"], ["BASE/WINDOW"])
        self.assertEqual(editor["link"]["libraries"][-1], "libcmt.lib")
        self.assertNotIn("winmm.lib", editor["link"]["libraries"])
        flags = {u["unit"]: u["flags"] for u in editor["units"]}
        self.assertEqual(flags["SOURCE/kbwin"], ["/c", "/Od", "/Oi", "/DHOMM1_EDITOR"])
        self.assertEqual(flags["BASE/WINDOW"], ["/c", "/Od", "/DHOMM1_EDITOR"])
        self.assertNotIn("SOURCE/GAME", flags)
        self.assertFalse(any(f.startswith("/STACK") for f in editor["link"]["flags"]))


class DomainArrayTests(unittest.TestCase):
    def test_domain_arrays_become_plain_arrays(self):
        cleaned = source.clean_cpp(
            "extern H1_ENUM_ARRAY(i32, gTimers, TimerSlot, GLOBAL_TIMER_COUNT);\n"
            "H1_ENUM_ARRAY2(short, gGrid, Slot, SLOT_COUNT, Row, ROW_COUNT);\n"
            "H1_ENUM_STEPPED(Slot)\n")
        self.assertIn("extern i32 gTimers[GLOBAL_TIMER_COUNT];", cleaned)
        self.assertIn("short gGrid[SLOT_COUNT][ROW_COUNT];", cleaned)
        self.assertNotIn("H1_ENUM", cleaned)

    def test_a_continued_argument_drops_its_backslash(self):
        cleaned = source.clean_cpp(
            "#define FINISH(message) \\\n"
            "    ((message).command = H1_ENUM_DECODE( \\\n"
            "         Command, \\\n"
            "         (message).id = H1_ENUM_ENCODE(Command, SELECT)    \\\n"
            "     ))\n")
        self.assertIn("((message).id = SELECT)", cleaned)
        self.assertNotIn("SELECT    \\", cleaned)
        self.assertNotIn("H1_ENUM", cleaned)


if __name__ == '__main__':
    unittest.main()

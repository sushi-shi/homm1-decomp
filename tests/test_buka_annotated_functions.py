import unittest
from pathlib import Path
from tempfile import TemporaryDirectory

from homm1.symbols.annotated_functions import (
    definitions_for_file,
    source_function_spans,
)


class AnnotatedFunctionsTest(unittest.TestCase):
    def test_homm1_extern_c_rva_span_uses_configured_fragment_name(self):
        with TemporaryDirectory() as temporary:
            repo = Path(temporary)
            source_root = repo / 'src'
            source_root.mkdir()
            (source_root / 'dialog.cpp').write_text('extern "C" RVA(0x1230, 0x20)\nvoid callback() {}\n')
            (repo / 'config').mkdir()
            (repo / 'config/units.toml').write_text('[[unit]]\nunit="fragment"\nsource="src/dialog.cpp"\n')
            spans = source_function_spans(source_root, repo)
        self.assertEqual([(s.unit, s.rva, s.size) for s in spans], [('fragment', 0x1230, 0x20)])

    def test_external_source_root_does_not_inherit_repository_masm_claims(self):
        with TemporaryDirectory() as temporary:
            repo = Path(temporary) / "repo"
            (repo / "src/BASE").mkdir(parents=True)
            (repo / "src/BASE/BITS.asm").touch()
            source_root = Path(temporary) / "fixture"
            source_root.mkdir()

            spans = source_function_spans(source_root, repo)

        self.assertEqual(spans, [])

    def test_static_va_function_uses_microsoft_fastcall_mangling(self):
        with TemporaryDirectory() as temporary:
            repo = Path(temporary)
            source_root = repo / "src"
            path = source_root / "SOURCE/example.cpp"
            path.parent.mkdir(parents=True)
            path.write_text(
                '#define RVA(addr, size) __attribute__((annotate('
                '"rva:" #addr " size:" #size)))\n'
                "RVA(0x00001230, 0x20)\n"
                "static void __fastcall helper(int value) { (void)value; }\n"
                "struct Control {};\n"
                "RVA(0x00001250, 0x18)\n"
                "static void __stdcall callback(Control* value) { (void)value; }\n"
                "RVA(0x00001268, 0x10)\n"
                "void visible(int value) { (void)value; }\n",
                encoding="utf-8")

            rows = definitions_for_file(path, source_root, repo)
            spans = source_function_spans(source_root, repo)

        self.assertEqual(len(rows), 2)
        self.assertEqual(rows[0].rva, 0x1230)
        self.assertEqual(rows[0].size, 0x20)
        self.assertEqual(rows[0].name, "helper")
        self.assertEqual(rows[0].mangled_name, "?helper@@YIXH@Z")
        self.assertEqual(rows[1].mangled_name, "?callback@@YGXPAUControl@@@Z")
        self.assertEqual([(row.rva, row.size) for row in spans], [
            (0x1230, 0x20), (0x1250, 0x18), (0x1268, 0x10)])


if __name__ == "__main__":
    unittest.main()

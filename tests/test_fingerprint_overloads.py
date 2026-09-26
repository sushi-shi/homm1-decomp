"""Overload fingerprints are tied to compiler identities, never name unions."""
import shutil
import tempfile
import unittest
from pathlib import Path
from unittest import mock

from homm1.tool import clang
from homm1.verify import fingerprints as fp


@unittest.skipUnless(shutil.which('clang') and shutil.which('clangd')
                     and shutil.which('llvm-undname'), 'native clang tools required')
class OverloadFingerprintTests(unittest.TestCase):
    SHORT = '?Run@Probe@@QAEHF@Z'
    INT = '?Run@Probe@@QAEHH@Z'
    CONST = '?Run@Probe@@QBEHH@Z'

    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.path = Path(self.temp.name) / 'overloads.cpp'

    def hashes(self, source, names):
        self.path.write_text(source)
        ast = clang.ast_dump(str(self.path), None)
        self.assertIsNotNone(ast)
        lsp = fp.Clangd()
        try:
            symbols = lsp.document_symbols(self.path)
        finally:
            lsp.close()
            lsp.proc.wait(timeout=5)
            lsp.proc.stdin.close()
            lsp.proc.stdout.close()
        return fp.function_hashes(
            source.splitlines(), fp.body_ranges(symbols), names,
            fp.demangle_map(names), fp.exact_body_ranges(ast, str(self.path)))

    @staticmethod
    def source(short_body='return value;', extra=''):
        # A typedef and one-line definition defeat signature spelling/line heuristics.
        return '''typedef short coordinate;
class Probe { public: int Run(coordinate); int Run(int); int Run(int) const; };
int Probe::Run(coordinate value) { %s }
int Probe::Run(int value)
{
    return value + 1;
}
%s''' % (short_body, extra)

    def test_body_edit_changes_only_the_exact_overload(self):
        names = {self.SHORT, self.INT}
        before = self.hashes(self.source(), names)
        after = self.hashes(self.source('return value + 2;'), names)
        self.assertEqual(set(before), names)
        self.assertNotEqual(before[self.SHORT], before[self.INT])
        self.assertTrue(fp.real_edit(before[self.SHORT], after[self.SHORT]))
        self.assertEqual(before[self.INT], after[self.INT])

    def test_adding_const_sibling_leaves_both_existing_bodies_unchanged(self):
        before = self.hashes(self.source(), {self.SHORT, self.INT})
        after = self.hashes(
            self.source(extra='int Probe::Run(int value) const { return value + 3; }\n'),
            {self.SHORT, self.INT, self.CONST})
        self.assertEqual(before[self.SHORT], after[self.SHORT])
        self.assertEqual(before[self.INT], after[self.INT])
        self.assertNotEqual(after[self.INT], after[self.CONST])

    def test_declared_only_sibling_cannot_steal_a_definition(self):
        # The const declaration has no body. It must stay unresolved, not share Run(int).
        hashes = self.hashes(self.source(), {self.SHORT, self.INT, self.CONST})
        self.assertNotIn(self.CONST, hashes)

    def test_inline_header_body_is_not_owned_by_the_translation_unit(self):
        header = self.path.with_name('helper.h')
        header.write_text('inline int HeaderOnly(int value) { return value; }\n')
        self.path.write_text('#include "helper.h"\n' + self.source())
        ast = clang.ast_dump(str(self.path), None)
        ranges = fp.exact_body_ranges(ast, str(self.path))
        self.assertIn(self.SHORT, ranges)
        self.assertNotIn('?HeaderOnly@@YAHH@Z', ranges)


class FingerprintDomainMigrationTests(unittest.TestCase):
    def test_legacy_union_transition_is_unknown_not_a_body_edit(self):
        self.assertFalse(fp.real_edit('old-union', fp.OVERLOAD + 'individual-body'))
        self.assertFalse(fp.real_edit(fp.OVERLOAD + 'individual-body', 'legacy-body'))
        self.assertTrue(fp.real_edit(fp.OVERLOAD + 'old', fp.OVERLOAD + 'new'))
        self.assertTrue(fp.real_edit('old', 'new'))
        self.assertFalse(fp.real_edit('cpp:unknown', fp.OVERLOAD + 'body'))

    def test_old_cache_and_seed_are_invalidated_without_ledger_changes(self):
        with tempfile.TemporaryDirectory() as directory:
            cache = Path(directory) / 'cache.tsv'
            seed = Path(directory) / 'seed.tsv'
            legacy = '# [units]\nU\thash\tU.cpp\n# [functions]\nU\tf\tunion\n'
            seed.write_text(legacy)
            with mock.patch.object(fp, 'CACHE', cache), mock.patch.object(fp, 'SEED', seed):
                self.assertEqual(fp.load_cache(), ({}, {}))
                cache.write_text(legacy)
                self.assertEqual(fp.load_cache(), ({}, {}))
                units, funcs = {'U': {'cpp_hash': 'hash', 'source': 'U.cpp'}}, {('U','f'): fp.OVERLOAD+'body'}
                fp.write_cache(units, funcs)
                self.assertEqual(fp.load_cache(), (units, funcs))

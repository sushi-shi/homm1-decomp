"""HoMM1 boundary controls for the copied Buka audits (no retail inputs)."""
import json
from pathlib import Path
import tempfile
import unittest

from homm1 import verify
from homm1.audit import casts, readability


class AuditIntegrationTests(unittest.TestCase):
    def setUp(self):
        temporary = tempfile.TemporaryDirectory()
        self.addCleanup(temporary.cleanup)
        self.root = Path(temporary.name)
        (self.root / 'include').mkdir()
        (self.root / 'src').mkdir()
        (self.root / 'config/cleanliness').mkdir(parents=True)
        self.source = self.root / 'src/pilot.cpp'
        self.source.write_text('''#define RVA(address, size)
extern "C" RVA(0x1000, 1)
int pilot() { return 0; }
''')
        database = self.root / 'build/analysis/strict/compile_commands.json'
        database.parent.mkdir(parents=True)
        database.write_text(json.dumps([dict(
            directory=str(self.root), file=str(self.source), arguments=[
                'clang++', '--target=i386-pc-windows-msvc', '-std=c++20',
                '-I', str(self.root / 'include'), str(self.source), '-fsyntax-only',
            ])]))

    def test_full_gate_requires_physical_review_and_keeps_source_markers(self):
        normal = verify.check_semantic(self.root)
        self.assertEqual(normal['physical_source']['pending'], ['src/pilot.cpp'])
        products = readability.generate(self.root, 'ctags')
        self.assertIn('0x1000', products['functions.tsv'])
        with self.assertRaisesRegex(ValueError, 'physical source review'):
            verify.check_semantic(self.root, require_complete=True)
        review = self.root / 'config/cleanliness/file_reviews.json'
        review.write_text(json.dumps({'src/pilot.cpp': dict(
            sha256=readability.digest(self.source.read_bytes()), note='Read fixture body.')}))
        self.assertFalse(verify.check_semantic(self.root, require_complete=True)['findings'])
        self.source.write_text(self.source.read_text() + '// edit invalidates file review\n')
        with self.assertRaisesRegex(ValueError, 'physical source review'):
            verify.check_semantic(self.root, require_complete=True)

    def test_unclaimed_header_cast_is_a_build_gate(self):
        (self.root / 'include/helper.hpp').write_text(
            'inline int helper(int value) { return static_cast<int>(value); }\n')
        self.source.write_text('#include "helper.hpp"\n' + self.source.read_text())
        with self.assertRaisesRegex(ValueError, 'unreviewed same-type in helper'):
            verify.check_semantic(self.root)

    def test_external_include_failure_cannot_produce_clean_ast(self):
        (self.root / 'sdk').mkdir()
        (self.root / 'sdk/broken.h').write_text('#error broken SDK configuration\n')
        self.source.write_text('#include "../sdk/broken.h"\n' + self.source.read_text())
        report = casts.scan(self.root, jobs=1)
        self.assertFalse(report['strict_parse_clean'])
        self.assertTrue(any('sdk/broken.h' in d['file'] for d in report['strict_diagnostics']))
        with self.assertRaisesRegex(ValueError, 'broken SDK configuration'):
            verify.check_semantic(self.root)

    def test_inventory_includes_untracked_inactive_bodies_and_inl(self):
        (self.root / 'include/.gitkeep').touch()
        (self.root / 'include/helper.inl').write_text(
            '#if 0\ninline int inactive() { return 1; }\n#endif\n')
        products = readability.generate(self.root, 'ctags')
        self.assertIn('inactive', products['functions.tsv'])
        self.assertIn('helper.inl', products['files.tsv'])
        self.assertNotIn('.gitkeep', products['files.tsv'])
        (self.root / 'include/forgotten.unknown').write_text('code')
        with self.assertRaisesRegex(ValueError, 'Uninventoried source file types'):
            readability.generate(self.root, 'ctags')

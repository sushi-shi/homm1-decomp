from pathlib import Path
import shutil
import tempfile
import unittest

from homm1 import analysis
from homm1.core.profile import parse


@unittest.skipUnless(shutil.which('clang++'), 'Clang is required; run in nix develop')
class DomainTests(unittest.TestCase):
    def test_forced_include_changes_the_analyzed_source(self):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / 'profile.cpp'
            header = Path(directory) / 'forced.h'
            header.write_text('#define FORCED_VALUE 7\n')
            path.write_text('typedef char Assert[FORCED_VALUE == 7 ? 1 : -1];')
            analysis.run(path, flags=['/FI', str(header)])
            with self.assertRaises(ValueError):
                analysis.run(path)

    def test_unsupported_flags_fail_and_relative_paths_share_a_root(self):
        with self.assertRaisesRegex(ValueError, 'unsupported compiler flag'):
            analysis.arguments('file.cpp', flags=['/Za'])
        self.assertEqual(parse(['/Ilocal', '/FI', 'forced.h'], Path('/tmp/project')),
                         [('/I', '/tmp/project/local'), ('/FI', '/tmp/project/forced.h')])

    def test_compiler_profile_defines_reach_analysis(self):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / 'profile.cpp'
            path.write_text('#if MODE != 7\n#error profile mismatch\n#endif\n')
            analysis.run(path, flags=['/DMODE=7'])
            with self.assertRaises(ValueError):
                analysis.run(path)

    def test_domain_crossing_rejected_only_by_strict_view(self):
        source = '''#include "Domains.h"
H1_ENUM_BEGIN(First) FIRST = 1 H1_ENUM_END(First)
H1_ENUM_BEGIN(Second) SECOND = 1 H1_ENUM_END(Second)
int consume(H1_ENUM_PARAM(First, int) value) { return 0; }
int use() { return consume(SECOND); }
'''
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / 'domains.cpp'
            path.write_text(source)
            analysis.run(path)
            with self.assertRaisesRegex(ValueError, 'strict analysis failed'):
                analysis.run(path, strict=True)

    def test_narrow_domain_storage_keeps_layout(self):
        source = '''#include "Domains.h"
H1_ENUM_BEGIN(First) FIRST = 1 H1_ENUM_END(First)
struct Record { H1_ENUM_STORAGE(First, short) field; };
typedef char Size[sizeof(Record) == 2 ? 1 : -1];
'''
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / 'domains.cpp'
            path.write_text(source)
            analysis.run(path)
            analysis.run(path, strict=True)

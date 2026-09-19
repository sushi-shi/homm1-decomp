from pathlib import Path
import shutil
import tempfile
import unittest

from homm1 import analysis


@unittest.skipUnless(shutil.which('clang++'), 'Clang is required; run in nix develop')
class DomainTests(unittest.TestCase):
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

    def test_narrow_domain_storage_is_accepted(self):
        source = '''#include "Domains.h"
H1_ENUM_BEGIN(First) FIRST = 1 H1_ENUM_END(First)
struct Record { H1_ENUM_STORAGE(First, short) field; };
'''
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / 'domains.cpp'
            path.write_text(source)
            analysis.run(path)
            analysis.run(path, strict=True)

from pathlib import Path
import shutil
import tempfile
import unittest

from homm1 import analysis


@unittest.skipUnless(shutil.which('clang++'), 'Clang is required; run in nix develop')
class DomainTests(unittest.TestCase):
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

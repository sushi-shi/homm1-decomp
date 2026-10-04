"""Source-derived data identities must agree with the selected compiler ABI."""
import unittest
from homm1.core import msvc_names as names


class StaticNameTests(unittest.TestCase):
    def test_vc6_file_static_has_no_vc4_ordinal(self):
        self.assertEqual(names.data('_fileValue', internal=True, decorated=True,
                                    compiler='vc6'), '_fileValue')
        self.assertEqual(names.data('_fileValue', internal=True, decorated=True,
                                    compiler='vc41'), '_fileValue$S')

    def test_vc6_function_static_gets_underscore_prefix(self):
        name = '?localValue@?1??Pick@@YAPAHH@Z@4HA'
        self.assertEqual(names.data(name, internal=True, decorated=True,
                                    compiler='vc6'), '_' + name)
        self.assertEqual(names.data(name, internal=True, decorated=True,
                                    compiler='vc41'), name)

    def test_external_spelling_is_unchanged(self):
        name = '?externalValue@@3HA'
        for compiler in ('vc6', 'vc41'):
            with self.subTest(compiler=compiler):
                self.assertEqual(names.data(name, internal=False, decorated=True,
                                            compiler=compiler), name)

    def test_vc6_array_qualifiers_match_measured_coff_names(self):
        # Pinned VC6 SP5 /Od /Ob1 /GX /MT /G5 and Clang -fms-compatibility-version=12.
        # See docs/patterns/vc6-array-data-names.md for declarations and controls.
        measured = (
            '?writableInts@@3PAHA', '?writableChars@@3PADA',
            '?constantInts@@3QBHB', '?constantChars@@3QBDB',
            '?pointersToConst@@3PAPBHA', '?constantPointers@@3QBQAHB',
            '?writableMatrix@@3PAY02HA', '?constantMatrix@@3QAY02$$CBHA',
            '?volatileInts@@3RCHC', '?constantVolatileInts@@3SDHD',
            '?constantFloats@@3QBMB',
        )
        for name in measured:
            with self.subTest(name=name):
                self.assertEqual(names.data(name, internal=False, decorated=True,
                                            compiler='vc6'), name)

    def test_vc4_array_rewrites_remain_separate(self):
        self.assertEqual(names.data('?constantInts@@3QBHB', internal=False,
                                    decorated=True, compiler='vc41'),
                         '?constantInts@@3PBHB')
        self.assertEqual(names.data('?constantMatrix@@3QAY02$$CBHA', internal=False,
                                    decorated=True, compiler='vc41'),
                         '?constantMatrix@@3PAY02HA')

    def test_scope_ordinal_is_still_canonicalized(self):
        name = '?localValue@?9??Pick@@YAPAHH@Z@4HA'
        self.assertEqual(names.data(name, internal=True, decorated=True,
                                    compiler='vc6'),
                         '_?localValue@?1??Pick@@YAPAHH@Z@4HA')


if __name__ == '__main__':
    unittest.main()

import unittest

from homm1.retail_labels.source import generated_function_claims


class GeneratedFunctionClaimTests(unittest.TestCase):
    def test_absolute_body_and_owner_addresses_become_rvas(self):
        claims, problems = generated_function_claims(
            'VA_COMPGEN(0x00401230, 0x1e, "??_GWidget@@UAEPAXI@Z", '
            '0x00401000)',
            {0x1000},
            0x400000,
        )

        self.assertEqual(problems, [])
        self.assertEqual(claims, [(0x1230, "??_GWidget@@UAEPAXI@Z", 0x1e)])

    def test_owner_must_be_a_va_definition_in_the_same_unit(self):
        claims, problems = generated_function_claims(
            'VA_COMPGEN(0x00401230, 0x1e, "??_GWidget@@UAEPAXI@Z", '
            '0x00401000)',
            set(),
            0x400000,
        )

        self.assertEqual(claims, [])
        self.assertEqual(len(problems), 1)
        self.assertIn('lacks its source owner VA(0x00401000)', problems[0])

    def test_comments_do_not_create_generated_claims(self):
        claims, problems = generated_function_claims(
            '// VA_COMPGEN(0x00401230, 0x1e, "generated", 0x00401000)',
            {0x1000},
            0x400000,
        )

        self.assertEqual(claims, [])
        self.assertEqual(problems, [])


if __name__ == '__main__':
    unittest.main()

"""OLDNAMES spellings of one runtime body compare as one function."""
import unittest

from homm1.compare import runtime_aliases


class RuntimeAliasTests(unittest.TestCase):
    def setUp(self):
        try:
            self.table = runtime_aliases.aliases()
        except (FileNotFoundError, RuntimeError) as error:
            self.skipTest(f"pinned runtime libraries unavailable: {error}")

    def test_stricmp_and_strcmpi_reach_one_name(self):
        self.assertEqual(self.table["_stricmp"], self.table["_strcmpi"])


if __name__ == "__main__":
    unittest.main()

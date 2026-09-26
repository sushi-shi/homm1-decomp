"""The data-matching switch reader (homm1.core.data_matching)."""

from __future__ import annotations

import tempfile
import unittest
from pathlib import Path

from homm1.core import data_matching


class DataMatchingSwitchTest(unittest.TestCase):
    def _read(self, text: str | None) -> bool:
        with tempfile.TemporaryDirectory() as td:
            path = Path(td) / "compare.toml"
            if text is not None:
                path.write_text(text)
            return data_matching.enabled(path)

    def test_both_values_read(self):
        self.assertTrue(self._read("[compare]\ndata_matching = true\n"))
        self.assertFalse(self._read("[compare]\ndata_matching = false\n"))

    def test_a_missing_file_is_strict(self):
        self.assertTrue(self._read(None))

    def test_a_missing_or_non_boolean_key_is_refused(self):
        for text in ("[compare]\n", '[compare]\ndata_matching = "false"\n',
                     "[compare]\ndata_matching = 0\n"):
            with self.assertRaises(SystemExit):
                self._read(text)

    def test_the_committed_switch_parses(self):
        self.assertIsInstance(data_matching.enabled(), bool)
        self.assertIn(data_matching.label(),
                      ("data_matching = true", "data_matching = false"))


if __name__ == "__main__":
    unittest.main()

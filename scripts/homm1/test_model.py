"""Model policy for label-only declaration claims (src_decl)."""

from __future__ import annotations

import unittest

from homm1.model import decl_conflicts
from homm1.retail_labels import Claim


def _claim(rva, name, channel, unit="u"):
    return Claim(rva, name, "func", channel, None, unit, {})


class DeclConflicts(unittest.TestCase):
    def test_agreeing_definition_supersedes_quietly(self):
        self.assertEqual(decl_conflicts([
            _claim(0x1000, "_Foo", "src_decl"),
            _claim(0x1000, "_Foo", "src"),
        ]), [])

    def test_a_different_name_is_a_violation(self):
        out = decl_conflicts([
            _claim(0x1000, "_Foo", "src_decl"),
            _claim(0x1000, "_Bar", "src"),
        ])
        self.assertEqual(len(out), 1)
        self.assertIn("0x001000", out[0])

    def test_two_declarations_disagreeing_is_a_violation(self):
        self.assertEqual(len(decl_conflicts([
            _claim(0x1000, "_Foo", "src_decl", unit="a"),
            _claim(0x1000, "_Baz", "src_decl", unit="b"),
        ])), 1)

    def test_definitions_alone_are_not_this_policy(self):
        self.assertEqual(decl_conflicts([
            _claim(0x1000, "_Foo", "src"),
            _claim(0x1000, "_Bar", "functions_static_libs"),
        ]), [])


if __name__ == "__main__":
    unittest.main()

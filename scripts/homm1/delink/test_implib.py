"""Keep reviewed ordinal imports identical in linking and comparison."""
import unittest
from unittest.mock import patch
from contextlib import ExitStack

from homm1.delink import implib
from homm1.graph import implib as link_implib
from homm1.core import pe


class ReviewedImportTests(unittest.TestCase):
    def resolve(self, reviewed, ordinal_names):
        slots = [(0x1000, None, "smackw32.DLL", 18),
                 (0x1004, None, "unknown.dll", 9)]
        with ExitStack() as stack:
            stack.enter_context(patch.object(implib, "era_import_libs", return_value=[]))
            stack.enter_context(patch.object(implib, "collect_imp_decorations", return_value=(set(), {})))
            stack.enter_context(patch.object(implib, "collect_ordinal_decorations", return_value=ordinal_names))
            validate = stack.enter_context(patch.object(link_implib, "_reviewed_import_symbols", return_value=reviewed))
            stack.enter_context(patch.object(pe, "image", return_value=object()))
            result = implib.resolve_iat(iter(slots), None)
            self.assertEqual(validate.call_args.args[1],
                             {0x1000: ("smackw32.DLL", 18), 0x1004: ("unknown.dll", 9)})
            return result

    def test_reviewed_ordinal_uses_link_symbol_unknown_ordinal_stays_distinct(self):
        names, unresolved = self.resolve({"smackw32.DLL": {18: "_SmackClose@4"}}, {})
        self.assertEqual(names, [(0x1000, "__imp__SmackClose@4"),
                                (0x1004, "__imp_HOMM1_ORD_unknown_dll_9")])
        self.assertEqual(unresolved, [])

    def test_agreeing_library_and_review_are_accepted(self):
        names, _ = self.resolve({"smackw32.DLL": {18: "_SmackClose@4"}},
                                {("smackw32.dll", 18): "__imp__SmackClose@4"})
        self.assertEqual(names[0][1], "__imp__SmackClose@4")

    def test_conflicting_library_identity_is_rejected(self):
        with self.assertRaisesRegex(ValueError, "reviewed as"):
            self.resolve({"smackw32.DLL": {18: "_SmackClose@4"}},
                         {("smackw32.dll", 18): "__imp__Wrong@8"})


if __name__ == "__main__":
    unittest.main()

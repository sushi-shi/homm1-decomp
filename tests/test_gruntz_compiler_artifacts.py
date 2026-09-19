"""Copied Gruntz compiler-artifact rejection controls."""
import tempfile
import unittest
from pathlib import Path
from collections import Counter

class CompilerArtifactControls(unittest.TestCase):
    def _scan(self, text):
        from homm1.audit import compiler_artifacts as ca
        with tempfile.TemporaryDirectory() as td:
            path = Path(td) / "Probe.cpp"
            path.write_text(text)
            return ca.source_findings(
                [path], placement_allow=Counter(), dtor_allow=Counter(),
                low_level_allow=Counter()
            )

    def test_allocator_calls_and_realizers_fail(self):
        findings = self._scan(
            "void* F(unsigned n) { return ::operator new(n); }\n"
            "CThing* RealizeCThing() { return new CThing(); }\n"
        )
        self.assertTrue(any("compiler allocation call" in row for row in findings))
        self.assertTrue(any("forced-emission helper" in row for row in findings))

    def test_comments_and_normal_new_expressions_pass(self):
        findings = self._scan(
            "// ::operator delete(p); CThing* RealizeCThing() {}\n"
            "CThing* F() { return new CThing(); }\n"
        )
        self.assertEqual(findings, [])

    def test_quoted_examples_are_not_code_but_inactive_hooks_are(self):
        self.assertEqual(self._scan('const char *text = "operator new(x); atexit(f);";'), [])
        self.assertTrue(self._scan('#if 0\nvoid f() { atexit(cleanup); }\n#endif'))

    def test_unreviewed_explicit_destructor_call_fails(self):
        findings = self._scan("void F(CThing* p) { p->~CThing(); }\n")
        self.assertTrue(any("explicit destructor call" in row for row in findings))

    def test_reviewed_typed_teardown_callback_passes(self):
        from homm1.audit import compiler_artifacts as ca
        with tempfile.TemporaryDirectory() as td:
            path = Path(td) / "Probe.h"
            path.write_text("template<class T> void dtf(T* p) { p->~T(); }\n")
            allowed = Counter({(str(path), "T"): 1})
            findings = ca.source_findings(
                [path], placement_allow=Counter(), dtor_allow=allowed,
                low_level_allow=Counter()
            )
        self.assertEqual(findings, [])

    def test_base_only_realizer_is_fatal(self):
        from homm1.audit import compiler_artifacts as ca
        rows = [("probe", "?RealizeCThing@@YAPAVCThing@@XZ"),
                ("probe", "??_GCThing@@UAEPAXI@Z")]
        self.assertEqual(len(ca.base_only_suspicious(rows)), 1)

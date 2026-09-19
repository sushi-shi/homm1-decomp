"""Negative controls ported from Gruntz b1de0e555576a215898907b8ec8ed5423368883e.

These cover the restored compiler-artifact gate and existing source gates.
The remaining donor controls are tracked in docs/tooling-inheritance.md.
"""
import tempfile
import unittest
from collections import Counter
from pathlib import Path
from unittest import mock


class BoardControls(unittest.TestCase):
    def test_ratcheted_metric_cannot_creep_up(self):
        from homm1.verify import board
        base = board.load_baseline()
        floor = base.get("unexplained casts", 0)
        rows = [("unexplained casts", floor + 1)]
        self.assertTrue(board.gate(rows))            # a rise FAILS
        self.assertFalse(board.gate([("unexplained casts", floor)]))

    def test_reviewed_reinterpret_cast_inventory_only_tracks(self):
        from homm1.verify import board
        self.assertFalse(board.gate([("reinterpret_casts", 10_000)]))

    def test_non_ratcheted_metric_only_tracks(self):
        from homm1.verify import board
        self.assertFalse(board.gate([("Unknown ids", 10_000)]))

    def test_address_derived_identifiers_are_counted(self):
        from homm1.verify.board import _count_address_derived_identifiers
        src = ("i32 m_8; CMapStringToOb m_10map; CString local_14; "
               "DWORD g_ratingRaw_64da84;")
        self.assertEqual(_count_address_derived_identifiers(src), 4)
        clean = "i32 m_reserved; CMapStringToOb m_workersByName;"
        self.assertEqual(_count_address_derived_identifiers(clean), 0)

    def test_unmeasured_semantic_floor_survives_a_bless(self):
        from homm1.verify import board
        with tempfile.TemporaryDirectory() as td:
            sem = Path(td) / "sem.tsv"
            sem.write_text("caller-callee FAKE-VIEW\t0\ntruncated masks\t0\n")
            with mock.patch.object(board, "SEMANTIC_BASELINE", sem), \
                 mock.patch.object(board, "TEXT_BASELINE", Path(td) / "t.tsv"):
                board.save_baseline([("nested static_casts", 25)],
                                    include_semantic=True)
                kept = board.load_baseline()
        self.assertEqual(kept.get("truncated masks"), 0)   # floor NOT dropped
        self.assertEqual(kept.get("nested static_casts"), 25)


class BansControls(unittest.TestCase):
    def _scan(self, text):
        from homm1.verify import bans
        with tempfile.TemporaryDirectory() as td:
            f = Path(td) / "probe.h"
            f.write_text(text)
            with mock.patch("homm1.verify.bans.source_files",
                            return_value=[f]):
                return list(bans.scan())

    def test_manual_vtable_idiom_fails(self):
        hits = self._scan("struct CFooVtbl { void* slots[4]; };\n"
                          "int use(CFoo* p) { return p->vtbl != 0; }\n")
        self.assertGreaterEqual(len(hits), 2)

    def test_prose_does_not_trip_the_ban(self):
        hits = self._scan("// the old struct CFooVtbl idiom is banned\n"
                          "class CFoo { virtual void Render(); };\n")
        self.assertEqual(hits, [])


class CompilerArtifactControls(unittest.TestCase):
    def _scan(self, text):
        from homm1.verify import compiler_artifacts as ca
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

    def test_unreviewed_explicit_destructor_call_fails(self):
        findings = self._scan("void F(CThing* p) { p->~CThing(); }\n")
        self.assertTrue(any("explicit destructor call" in row for row in findings))

    def test_reviewed_typed_teardown_callback_passes(self):
        from homm1.verify import compiler_artifacts as ca
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
        from homm1.verify import compiler_artifacts as ca
        rows = [("probe", "?RealizeCThing@@YAPAVCThing@@XZ"),
                ("probe", "??_GCThing@@UAEPAXI@Z")]
        self.assertEqual(len(ca.base_only_suspicious(rows)), 1)


class CastControls(unittest.TestCase):
    def test_seam_self_recursion_is_caught(self):
        from homm1.verify import casts
        with tempfile.TemporaryDirectory() as td:
            (Path(td) / "src").mkdir()
            (Path(td) / "include").mkdir()
            f = Path(td) / "src/Probe.cpp"
            f.write_text("inline u16* Scratch16() { return Scratch16(); }\n")
            with mock.patch.object(casts, "REPO", Path(td)):
                self.assertEqual(len(casts.self_recursion()), 1)
                f.write_text("inline u16* Scratch16() { "
                             "return reinterpret_cast<u16*>(g_scratch); }\n")
                self.assertEqual(casts.self_recursion(), [])

    def test_different_arity_overload_forwarding_is_not_self_recursion(self):
        from homm1.verify import casts
        with tempfile.TemporaryDirectory() as td:
            (Path(td) / "src").mkdir()
            (Path(td) / "include").mkdir()
            f = Path(td) / "include/Probe.h"
            f.write_text(
                "inline int Blt(HDC dc) { return Blt(dc, 0, 0); }\n"
            )
            with mock.patch.object(casts, "REPO", Path(td)):
                self.assertEqual(casts.self_recursion(), [])

    def test_unreasoned_cast_is_open_and_reasoned_is_parked(self):
        from homm1.verify import casts
        with tempfile.TemporaryDirectory() as td:
            (Path(td) / "src").mkdir()
            (Path(td) / "include").mkdir()
            f = Path(td) / "src/Probe.cpp"
            with mock.patch.object(casts, "REPO", Path(td)):
                f.write_text("u16* p = reinterpret_cast<u16*>(g_x);\n")
                _forced, openv = casts.scan_ledger()
                self.assertEqual(sum(len(v) for v in openv.values()), 1)
                f.write_text("u16* p = reinterpret_cast<u16*>(g_x); "
                             "// byte-forced: no reloc, bare imm\n")
                _forced, openv = casts.scan_ledger()
                self.assertEqual(sum(len(v) for v in openv.values()), 0)

    def test_a_cast_in_prose_is_not_a_site(self):
        from homm1.verify import casts
        with tempfile.TemporaryDirectory() as td:
            (Path(td) / "src").mkdir()
            (Path(td) / "include").mkdir()
            (Path(td) / "src/P.cpp").write_text(
                "// reinterpret_cast<u16*>(g_x) would be wrong here\n")
            with mock.patch.object(casts, "REPO", Path(td)):
                forced, openv = casts.scan_ledger()
                self.assertEqual((sum(forced.values()),
                                  sum(len(v) for v in openv.values())), (0, 0))


class EnumDomainControls(unittest.TestCase):
    def _audit(self, files):
        from homm1.verify import enum_domains
        with tempfile.TemporaryDirectory() as td:
            paths = []
            for name, text in files.items():
                p = Path(td) / name
                p.write_text(text)
                paths.append(p)
            real = Path(td)
            with mock.patch.object(enum_domains, "source_files",
                                   lambda: iter(paths)), \
                 mock.patch.object(enum_domains, "REPO", real):
                return enum_domains.audit()

    def test_split_width_disagreement_is_fatal(self):
        fatal, _w, _d = self._audit({
            "A.h": "H1_ENUM_BEGIN_SPLIT(Tool, u8)\nTOOL_GAUNTLETZ = 0,\n"
                   "H1_ENUM_END_SPLIT(Tool)\n",
            "B.h": "H1_ENUM_STORAGE(Tool, i32) m_tool;\n"})
        self.assertTrue(any("two beliefs" in f for f in fatal))

    def test_matching_split_width_passes(self):
        fatal, _w, _d = self._audit({
            "A.h": "H1_ENUM_BEGIN_SPLIT(Tool, u8)\nTOOL_GAUNTLETZ = 0,\n"
                   "H1_ENUM_END_SPLIT(Tool)\n",
            "B.h": "H1_ENUM_STORAGE(Tool, u8) m_tool;\n"})
        self.assertEqual(fatal, [])

    def test_bare_header_enum_is_fatal_and_tag_type_exempt(self):
        fatal, _w, _d = self._audit({
            "A.h": "enum Tool { TOOL_A = 1, TOOL_B = 2 };\n"})
        self.assertTrue(any("bare `enum Tool`" in f for f in fatal))
        fatal, _w, _d = self._audit({
            "A.h": "enum ENoSeed { NO_SEED };\n"})
        self.assertEqual(fatal, [])

    def test_range_test_against_a_member_is_fatal(self):
        fatal, _w, _d = self._audit({
            "A.h": "H1_ENUM_BEGIN(Pickup)\nPICKUP_WINGZ = 22,\n"
                   "H1_ENUM_END(Pickup)\n",
            "B.cpp": "if (n > PICKUP_WINGZ) return 0;\n"})
        self.assertTrue(any("names a MEMBER" in f for f in fatal))


def _binding(rva, name, unit="probe", channel="src", kind="", size=0x10,
             space="text", aliases=()):
    from homm1.model import Binding
    return Binding(rva, size, kind, space, name, unit, channel,
                   tuple(aliases), ())


def _model(functions=(), data=(), violations=()):
    from homm1.model import Model
    return Model(list(functions), list(data), list(violations))


class UniqueNamesControls(unittest.TestCase):
    def test_one_name_at_two_rvas_is_fatal(self):
        from homm1.verify import unique_names
        m = _model([_binding(0x1000, "?F@@YAXXZ"),
                    _binding(0x2000, "?F@@YAXXZ")])
        with mock.patch("homm1.model.resolve", return_value=m):
            bad, _n = unique_names.findings()
        self.assertTrue(any("name-injectivity" in b for b in bad))

    def test_model_violation_is_fatal_and_clean_passes(self):
        from homm1.verify import unique_names
        m = _model([_binding(0x1000, "?F@@YAXXZ")], violations=["boom"])
        with mock.patch("homm1.model.resolve", return_value=m):
            bad, _n = unique_names.findings()
        self.assertTrue(any("model violation" in b for b in bad))
        m = _model([_binding(0x1000, "?F@@YAXXZ"),
                    _binding(0x2000, "?G@@YAXXZ")])
        with mock.patch("homm1.model.resolve", return_value=m):
            bad, _n = unique_names.findings()
        self.assertEqual(bad, [])

    def test_zero_claims_never_pass_vacuously(self):
        from homm1.verify import unique_names
        with mock.patch("homm1.model.resolve", return_value=_model()):
            bad, _n = unique_names.findings()
        self.assertTrue(any("vacuously" in b for b in bad))

    def test_a_fid_label_repeat_is_not_a_finding(self):
        from homm1.verify import unique_names
        m = _model([_binding(0x1000, "??_G__non_rtti_object@@UAEPAXI@Z",
                             channel="functions_static_libs"),
                    _binding(0x2000, "??_G__non_rtti_object@@UAEPAXI@Z",
                             channel="functions_static_libs"),
                    _binding(0x3000, "?F@@YAXXZ")])
        with mock.patch("homm1.model.resolve", return_value=m):
            bad, _n = unique_names.findings()
        self.assertEqual(bad, [])


class LibraryOverlapControls(unittest.TestCase):
    def test_src_claim_on_a_static_libs_row_is_loud(self):
        from homm1.retail_labels import Claim
        from homm1.verify import library_overlap
        alias = Claim(0x1000, "??0CFile@@QAE@XZ", "func",
                      "functions_static_libs", None, "", {})
        m = _model([_binding(0x1000, "?Open@CFileIO@@QAEHXZ",
                             aliases=[alias])])
        with mock.patch("homm1.model.resolve", return_value=m):
            bad, _n = library_overlap.findings()
        self.assertEqual(len(bad), 1)
        m = _model([_binding(0x1000, "?Open@CFileIO@@QAEHXZ")])
        with mock.patch("homm1.model.resolve", return_value=m):
            bad, _n = library_overlap.findings()
        self.assertEqual(bad, [])


class UndefinedClosureControls(unittest.TestCase):
    def _run(self, bdef, bund, tnames, baseline=frozenset()):
        from homm1.verify import undefined_closure as uc
        with mock.patch.object(uc, "live_base_objs", return_value=["x"]), \
             mock.patch.object(uc, "_sym_sets",
                               side_effect=[(bdef, bund), (tnames, set())]), \
             mock.patch.object(uc, "lib_symbols", return_value=set()), \
             mock.patch.object(uc, "_rtti_classes", return_value=set()), \
             mock.patch.object(uc, "source_library_shadows",
                               return_value=[]), \
             mock.patch.object(uc, "_read_baseline",
                               return_value=set(baseline)):
            return uc.analyse()

    def test_pure_phantom_class_is_a_fake_view(self):
        phantom, _sh, _dec = self._run(
            bdef={"?Run@CReal@@QAEXXZ"},
            bund={"?M@CPhantomView@@QAEXXZ"},
            tnames=set())
        self.assertIn("CPhantomView", phantom)

    def test_a_real_class_with_bodies_is_not_a_phantom(self):
        phantom, _sh, _dec = self._run(
            bdef={"?Run@CGrunt@@QAEXXZ"},
            bund={"?Walk@CGrunt@@QAEXXZ"},     # unreconstructed, not fake
            tnames=set())
        self.assertEqual(dict(phantom), {})

    def test_declared_only_alias_is_debt_unless_retail_names_it(self):
        _p, _s, declared = self._run(
            bdef=set(), bund={"?Check4_2ce8@@YGHH@Z"}, tnames=set())
        self.assertIn("?Check4_2ce8@@YGHH@Z", declared)
        _p, _s, declared = self._run(
            bdef=set(), bund={"?Check4_2ce8@@YGHH@Z"},
            tnames={"?Check4_2ce8@@YGHH@Z"})   # retail namespace has it
        self.assertEqual(declared, set())


class AssemblyFingerprintControls(unittest.TestCase):
    def test_assembly_uses_unknown_fallback_without_clangd(self):
        import contextlib
        import io
        from homm1.verify import fingerprints as fp
        with mock.patch.object(fp, "unit_sources", return_value={"BITS": "BITS.asm"}), \
                mock.patch.object(fp, "unit_mangled", return_value={"BITS": {"_Probe"}}), \
                mock.patch.object(fp, "load_cache", return_value=({}, {})), \
                mock.patch.object(fp, "cpp_hash", return_value="source-hash"), \
                mock.patch.object(fp, "write_cache") as write, \
                mock.patch.object(fp, "Clangd") as clangd, \
                contextlib.redirect_stdout(io.StringIO()):
            self.assertEqual(fp.regenerate(), 0)
        clangd.assert_not_called()
        write.assert_called_once_with(
            {"BITS": {"cpp_hash": "source-hash", "source": "BITS.asm"}}, {})
        self.assertFalse(fp.real_edit("old-body", "cpp:source-hash"))


class TierRunnerControls(unittest.TestCase):
    def test_a_crashing_gate_is_a_failure_not_a_skip(self):
        import contextlib
        import io

        from homm1.verify import tiers

        def boom():
            raise RuntimeError("tool broke")
        with mock.patch.dict(tiers.TIERS, {"fast": [("probe", boom)]}):
            with contextlib.redirect_stdout(io.StringIO()) as out:
                failed = tiers.run(["fast"])
        self.assertEqual(failed, 1)
        self.assertIn("gate crashed", out.getvalue())

    def test_a_clean_gate_passes(self):
        import contextlib
        import io

        from homm1.verify import tiers
        with mock.patch.dict(tiers.TIERS, {"fast": [("probe", lambda: [])]}):
            with contextlib.redirect_stdout(io.StringIO()):
                self.assertEqual(tiers.run(["fast"]), 0)

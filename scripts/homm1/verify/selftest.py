"""homm1.verify.selftest - NEGATIVE CONTROLS for the ported gates.

A gate nobody has seen FAIL is a green light, not a check (the ported
gate_selftest doctrine: the slot-binding gate once parsed its own baseline's
banner as the header row and passed everything, forever). Every ported gate
gets at least one demonstrated failure case AND a clean pass; the tests are
hermetic (tmpdir trees / synthetic models) except the DATA_COMPGEN control
set, which deliberately runs against real base objs (skipped loudly when the
tree is unbuilt).

    homm1 verify selftest             # every control, one dot each
    homm1 verify selftest -v          # name each control as it runs
    homm1 verify selftest -k Ledger   # only the controls matching a name
"""

from __future__ import annotations

from homm1.core.usage import logged

import os
import shutil
import struct
import sys
import tempfile
import unittest
from collections import Counter
from pathlib import Path
from unittest import mock

from homm1.core.paths import REPO as REPO_ROOT



# --------------------------------------------------------------------------- #
# fast tier                                                                   #
# --------------------------------------------------------------------------- #
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


class StaticDestructorAttributionControls(unittest.TestCase):
    """The real COFF/retail pair must reach the PDB record consumer."""

    def test_pinned_callbacks_reach_function_records(self):
        from homm1.delink import pdb_synth, static_dtors
        from homm1.delink.image import retail
        from homm1.model import resolve

        model = resolve()
        names = pdb_synth.unit_names(model)
        if not pdb_synth.BASE_DIR.joinpath("butemgr.obj").is_file():
            self.skipTest("base objects absent (unbuilt tree)")
        derived = static_dtors.provision(model, names, pdb_synth.BASE_DIR, retail())
        # 0x159C70/0x159C80: one owner (TickKillCues) with two local statics.
        expected = (0x153800, 0x1538B0, 0x159C70, 0x159C80, 0x173290, 0x173840,
                    0x173DC0, 0x174330, 0x174890)
        for rva in expected:
            self.assertIn(rva, derived)
            self.assertRegex(derived[rva][0], r"^_\$E[0-9]+$")

        names.update(derived)
        records = pdb_synth.function_records(
            model, names, {}, {}, [], lambda _message: None)
        by_rva = {rva: name for rva, _size, name in records}
        for rva in expected:
            self.assertEqual(by_rva[rva], derived[rva][0])

    def test_missing_retail_relocation_cannot_be_attributed(self):
        from types import SimpleNamespace
        from homm1.delink import pdb_synth, static_dtors
        from homm1.delink.image import retail
        from homm1.model import resolve

        model = resolve()
        if not pdb_synth.BASE_DIR.joinpath("butemgr.obj").is_file():
            self.skipTest("base objects absent (unbuilt tree)")
        real = retail()
        no_relocs = SimpleNamespace(
            pe=real.pe, image_base=real.image_base, reloc_sites=[])
        derived = static_dtors.provision(
            model, pdb_synth.unit_names(model), pdb_synth.BASE_DIR, no_relocs)
        self.assertEqual(derived, {})


class CompilerArtifactControls(unittest.TestCase):
    def _scan(self, text):
        from homm1.verify import compiler_artifacts as ca
        with tempfile.TemporaryDirectory() as td:
            path = Path(td) / "Probe.cpp"
            path.write_text(text)
            return ca.source_findings(
                [path], placement_allow=Counter(), dtor_allow=Counter(),
                low_level_allow=Counter(), allocation_definition_allow=Counter(),
                allocation_call_allow=Counter()
            )

    def test_allocator_calls_and_realizers_fail(self):
        findings = self._scan(
            "void* F(unsigned n) { return ::operator new(n); }\n"
            "CThing* RealizeCThing() { return new CThing(); }\n"
        )
        self.assertTrue(any("compiler allocation call" in row for row in findings))
        self.assertTrue(any("forced-emission helper" in row for row in findings))

    def test_allocation_call_allow_is_exact(self):
        from homm1.verify import compiler_artifacts as ca
        with tempfile.TemporaryDirectory() as td:
            path = Path(td) / "Probe.cpp"
            one = "void F(T* p) { ::operator delete(p); }\n"
            path.write_text(one)
            site = ca.rel(path)
            allow = Counter({(site, "::operator delete("): 1})

            def scan():
                return ca.source_findings(
                    [path], placement_allow=Counter(), dtor_allow=Counter(),
                    low_level_allow=Counter(), allocation_definition_allow=Counter(),
                    allocation_call_allow=allow)

            self.assertEqual(scan(), [])
            path.write_text(one + "void G(T* p) { ::operator delete(p); }\n")
            self.assertTrue(any("compiler allocation call" in row for row in scan()))
            path.write_text("void F() {}\n")
            self.assertTrue(any("found 0, expected 1" in row for row in scan()))

    def test_instantiation_only_unit_reaches_gate(self):
        from homm1.verify import compiler_artifacts as ca
        source = ('#include <Array.h>\n'
                  '// Former class implementation, now only an emitter.\n'
                  'RVA_COMPGEN(0x8710, 0x2b, ??0?$Array@H@@QAE@XZ)\n'
                  'template class Array<int>;\n')
        with tempfile.TemporaryDirectory() as td:
            path = Path(td) / 'Probe.cpp'
            path.write_text(source)
            with mock.patch.object(ca, 'base_only_suspicious', return_value=[]):
                findings = ca.gate_findings([path])
        self.assertTrue(any('instantiation-only translation unit' in row
                            for row in findings))

    def test_instantiations_beside_real_code_or_storage_are_not_empty_units(self):
        from homm1.verify.compiler_artifacts import instantiation_only
        instantiation = 'template class Array<int>;\n'
        for source in (instantiation + 'Array<int> g_values;\n',
                       instantiation + 'void Owner::Clear() { values.Clear(); }\n',
                       'template<> Array<int> Registry<Tag>::values;\n',
                       '#include <Array.h>\n'):
            with self.subTest(source=source):
                self.assertFalse(instantiation_only(source))

    def test_comments_and_normal_new_expressions_pass(self):
        findings = self._scan(
            "// ::operator delete(p); CThing* RealizeCThing() {}\n"
            "CThing* F() { return new CThing(); }\n"
        )
        self.assertEqual(findings, [])

    def test_unreviewed_explicit_destructor_call_fails(self):
        findings = self._scan("void F(CThing* p) { p->~CThing(); }\n")
        self.assertTrue(any("explicit destructor call" in row for row in findings))

    def test_explicit_constructor_expression_reaches_gate(self):
        from homm1.verify import compiler_artifacts as ca
        with tempfile.TemporaryDirectory() as td:
            path = Path(td) / "Probe.cpp"
            path.write_text("void F(CString* p) { p->CString::CString(); }\n")
            with mock.patch.object(ca, "base_only_suspicious", return_value=[]):
                findings = ca.gate_findings([path])
        self.assertTrue(any("explicit constructor call" in row for row in findings))
        self.assertEqual(self._scan("void F(CThing* p) { p->CThing::Reset(); }\n"), [])

    def test_reviewed_typed_teardown_callback_passes(self):
        from homm1.verify import compiler_artifacts as ca
        with tempfile.TemporaryDirectory() as td:
            path = Path(td) / "Probe.h"
            path.write_text("template<class T> void dtf(T* p) { p->~T(); }\n")
            allowed = Counter({(str(path), "T"): 1})
            findings = ca.source_findings(
                [path], placement_allow=Counter(), dtor_allow=allowed,
                low_level_allow=Counter(), allocation_definition_allow=Counter(),
                allocation_call_allow=Counter()
            )
        self.assertEqual(findings, [])

    def test_base_only_realizer_is_fatal(self):
        from homm1.verify import compiler_artifacts as ca
        rows = [("probe", "?RealizeCThing@@YAPAVCThing@@XZ"),
                ("probe", "??_GCThing@@UAEPAXI@Z")]
        self.assertEqual(len(ca.base_only_suspicious(rows)), 1)

    @unittest.skipUnless((REPO_ROOT / "include/ZTools").is_dir(),
                         "fixture needs the ZTools placement-new headers (gruntz-only)")
    def test_authored_placement_definition_and_negative_controls_reach_gate(self):
        from homm1.verify import compiler_artifacts as ca
        from homm1.core.paths import REPO

        owner = REPO / 'include/ZTools/PlacementNew.h'
        original = owner.read_text()
        read_text = Path.read_text
        cases = (
            ('original', original, False),
            ('explicit call', original + '\nvoid* F(void* p) { '
             'return ::operator new(4, p, 0, 0); }\n', True),
            ('duplicate', original + original, True),
            ('altered body', original.replace('return ptr;', 'return 0;'), True),
            ('missing', '', True),
        )
        for name, source, rejected in cases:
            with self.subTest(case=name):
                def substituted(path, *args, **kwargs):
                    return source if path == owner else read_text(path, *args, **kwargs)
                with mock.patch.object(Path, 'read_text', substituted):
                    findings = ca.gate_findings()
                self.assertEqual(bool(findings), rejected, findings)

        definition = ('inline void* operator new(size_t size, void* ptr, '
                      'int dummy1, int dummy2) { return ptr; }')
        self.assertTrue(any('allocation definition' in row
                            for row in self._scan(definition)))

    @unittest.skipUnless((REPO_ROOT / "include/ZTools").is_dir(),
                         "fixture needs the ZTools placement-new headers (gruntz-only)")
    def test_complete_array_placement_population_reaches_gate(self):
        from homm1.verify import compiler_artifacts as ca
        from homm1.core.paths import REPO

        owner = REPO / 'include/ZTools/ZDArray.h'
        original = owner.read_text()
        read_text = Path.read_text
        self.assertEqual(len(ca.PLACEMENT_RE.findall(original)), 4)
        for name, source, rejected in (
            ('complete family', original, False),
            ('missing construction', original.replace('new (p, 0, 0) T', 'p', 1), True),
            ('extra construction', original + '\ntemplate<class T> void extra(T* p) '
             '{ new (p, 0, 0) T; }\n', True),
            ('wrong element', original.replace('new (p, 0, 0) T', 'new (p, 0, 0) Item', 1), True),
        ):
            with self.subTest(case=name):
                def substituted(path, *args, **kwargs):
                    return source if path == owner else read_text(path, *args, **kwargs)
                with mock.patch.object(Path, 'read_text', substituted):
                    findings = ca.gate_findings()
                self.assertEqual(bool(findings), rejected, findings)


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
            "A.h": "GZ_ENUM_BEGIN_SPLIT(Tool, u8)\nTOOL_GAUNTLETZ = 0,\n"
                   "GZ_ENUM_END_SPLIT(Tool)\n",
            "B.h": "GZ_ENUM_STORAGE(Tool, i32) m_tool;\n"})
        self.assertTrue(any("two beliefs" in f for f in fatal))

    def test_matching_split_width_passes(self):
        fatal, _w, _d = self._audit({
            "A.h": "GZ_ENUM_BEGIN_SPLIT(Tool, u8)\nTOOL_GAUNTLETZ = 0,\n"
                   "GZ_ENUM_END_SPLIT(Tool)\n",
            "B.h": "GZ_ENUM_STORAGE(Tool, u8) m_tool;\n"})
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
            "A.h": "GZ_ENUM_BEGIN(Pickup)\nPICKUP_WINGZ = 22,\n"
                   "GZ_ENUM_END(Pickup)\n",
            "B.cpp": "if (n > PICKUP_WINGZ) return 0;\n"})
        self.assertTrue(any("names a MEMBER" in f for f in fatal))


class EnumReuseControls(unittest.TestCase):
    def _scan(self, files):
        from homm1.verify import enum_reuse
        with tempfile.TemporaryDirectory() as td:
            root = Path(td)
            (root / "src").mkdir()
            paths = []
            entries = []
            for name, source in files.items():
                path = root / name
                path.parent.mkdir(parents=True, exist_ok=True)
                path.write_text(source)
                paths.append(path)
                if path.suffix == ".cpp":
                    entries.append({
                        "directory": str(root),
                        "file": name,
                        "arguments": ["clang-cl", "/c", name, "/TP"],
                    })
            blocks = enum_reuse.scan_blocks(repo=root, paths=paths)
            raw, contexts, errors = enum_reuse.scan_entries(
                entries, repo=root, jobs=1)
            constants, uncovered_source, uncovered_ast = enum_reuse._join(
                raw, contexts, blocks)
            return constants, blocks, uncovered_source, uncovered_ast, errors

    def test_evaluates_aliases_expressions_and_implicit_members(self):
        constants, blocks, missing_source, missing_ast, errors = self._scan({
            "src/Probe.cpp": (
                "enum First { FIRST_ZERO = 0, FIRST_TEN = 5 * 2 };\n"
                "enum Second { SECOND_TEN = FIRST_TEN, SECOND_NEXT };\n"
            ),
        })
        self.assertEqual(errors, [])
        self.assertEqual(missing_source, [])
        self.assertEqual(missing_ast, [])
        self.assertEqual(len(blocks), 2)
        self.assertEqual(
            [(row.name, row.value) for row in constants],
            [("FIRST_ZERO", 0), ("FIRST_TEN", 10),
             ("SECOND_TEN", 10), ("SECOND_NEXT", 11)],
        )

    def test_shared_header_declaration_is_deduplicated_with_both_contexts(self):
        header = "enum Shared { SHARED_TEN = 10 };\n"
        constants, _blocks, missing_source, missing_ast, errors = self._scan({
            "include/Shared.h": header,
            "src/A.cpp": '#include "../include/Shared.h"\n',
            "src/B.cpp": '#include "../include/Shared.h"\n',
        })
        self.assertEqual(errors, [])
        self.assertEqual(missing_source, [])
        self.assertEqual(missing_ast, [])
        self.assertEqual(len(constants), 1)
        self.assertEqual(constants[0].contexts, ("src/A.cpp", "src/B.cpp"))

    def test_member_comments_are_not_mistaken_for_enumerator_names(self):
        constants, blocks, missing_source, missing_ast, errors = self._scan({
            "src/Probe.cpp": (
                "enum First { /* explanatory words */ FIRST = 10,\n"
                "// another explanation\nSECOND = 11 };\n"
            ),
        })
        self.assertEqual(errors, [])
        self.assertEqual(missing_source, [])
        self.assertEqual(missing_ast, [])
        self.assertEqual(
            [member.name for member in blocks[0].members],
            ["FIRST", "SECOND"],
        )
        self.assertEqual(
            [(row.name, row.value) for row in constants],
            [("FIRST", 10), ("SECOND", 11)],
        )

    def test_ledger_rejects_pending_and_unclaimed_members(self):
        from homm1.verify import enum_reuse
        constants, blocks, _missing_source, _missing_ast, _errors = self._scan({
            "src/Probe.cpp": "enum First { FIRST = 10 };\n",
        })
        with tempfile.TemporaryDirectory() as td:
            ledger = Path(td) / "review.tsv"
            enum_reuse.init_ledger(ledger, constants, blocks)
            findings = enum_reuse.check_ledger(ledger, constants)
        self.assertTrue(any("review is pending" in row for row in findings))

    def test_ledger_proves_removed_members_reuse_a_current_canonical(self):
        from homm1.verify import enum_reuse
        constants, _blocks, _missing_source, _missing_ast, _errors = self._scan({
            "src/Probe.cpp": "enum Canonical { CANONICAL_TEN = 10 };\n",
        })
        canonical = "src/Probe.cpp:Canonical"
        removed = "src/Old.h:OldDomain"
        with tempfile.TemporaryDirectory() as td:
            ledger = Path(td) / "review.tsv"
            ledger.write_text(
                "\t".join(enum_reuse.LEDGER_FIELDS) + "\n"
                f"{canonical}\tCANONICAL_TEN=10\tcanonical\t{canonical}\t\t"
                "Owns the shared quantity.\n"
                f"{removed}\tOLD_TEN=10\treuse\t{canonical}\t"
                f"OLD_TEN={canonical}::CANONICAL_TEN\t"
                "The old producer and canonical consumer carry one value.\n"
            )
            findings = enum_reuse.check_ledger(ledger, constants)
        self.assertEqual(findings, [])

    def test_ledger_rejects_a_current_member_without_starting_provenance(self):
        from homm1.verify import enum_reuse
        constants, _blocks, _missing_source, _missing_ast, _errors = self._scan({
            "src/Probe.cpp": (
                "enum Canonical { CANONICAL_TEN = 10, CANONICAL_NEW = 11 };\n"
            ),
        })
        canonical = "src/Probe.cpp:Canonical"
        with tempfile.TemporaryDirectory() as td:
            ledger = Path(td) / "review.tsv"
            ledger.write_text(
                "\t".join(enum_reuse.LEDGER_FIELDS) + "\n"
                f"{canonical}\tCANONICAL_TEN=10\tcanonical\t{canonical}\t\t"
                "Owns the shared quantity.\n"
            )
            findings = enum_reuse.check_ledger(ledger, constants)
        self.assertTrue(any("no starting-ledger provenance" in row
                            for row in findings))


class ConstantControls(unittest.TestCase):
    def _scan(self, source, *, flags=None):
        from homm1.verify import constants
        with tempfile.TemporaryDirectory() as td:
            root = Path(td)
            (root / "src").mkdir()
            path = root / "src/Probe.cpp"
            path.write_text(source)
            entry = {"directory": str(root), "file": "src/Probe.cpp",
                     "arguments": ["clang-cl", "/c", "src/Probe.cpp", "/TP"]}
            patch = (mock.patch.object(constants, "_flags", return_value=flags)
                     if flags is not None else mock.patch.object(
                         constants, "_flags", wraps=constants._flags))
            with patch:
                return constants.scan_entries([entry], repo=root, jobs=1)

    def test_typed_pointer_bool_and_enum_sites_are_proven(self):
        source = ("#define NULL 0\n"
                  "enum Kind { KIND_NONE = 0, KIND_ONE = 1 };\n"
                  "bool BoolReturn() { return 0; }\n"
                  "int* PtrReturn() { return 0; }\n"
                  "void Takes(bool b, int* p);\n"
                  "void Probe(Kind kind) { bool b = 1; int* p = 0; "
                  "Takes(0, 0); if (p == 0) {} if (b == 0) {} "
                  "if (kind == 0) {} }\n"
                  "int Arithmetic(int n) { return n + 0; }\n")
        sites, errors = self._scan(source)
        self.assertEqual(errors, [])
        replacements = [s.replacement for s in sites if s.proven]
        self.assertEqual(replacements.count("NULL"), 4)
        self.assertEqual(replacements.count("false"), 3)
        self.assertEqual(replacements.count("true"), 1)
        self.assertEqual(replacements.count("KIND_NONE"), 1)
        arithmetic = [s for s in sites
                      if s.function.startswith("Arithmetic") and s.spelling == "0"]
        self.assertEqual(len(arithmetic), 1)
        self.assertFalse(arithmetic[0].proven)

    def test_named_spellings_and_explicit_ingest_cast_pass(self):
        source = ("enum Kind { KIND_NONE = 0 };\n"
                  "#define NULL 0\n"
                  "bool B() { return false; }\n"
                  "int* P() { return NULL; }\n"
                  "Kind K(int n) { return static_cast<Kind>(0); }\n"
                  "Kind KN(int n) { return static_cast<Kind>(-1); }\n")
        sites, errors = self._scan(source)
        self.assertEqual(errors, [])
        self.assertEqual([s for s in sites if s.proven], [])

    def test_outer_result_cast_does_not_hide_pointer_call_argument(self):
        source = ("#define NULL 0\n"
                  "struct Node {};\n"
                  "void* Create(int* context);\n"
                  "Node* Probe() {\n"
                  "  return static_cast<Node*>(Create(0));\n"
                  "}\n")
        sites, errors = self._scan(source)
        self.assertEqual(errors, [])
        pointer_zeroes = [s for s in sites
                          if s.function.startswith("Probe") and s.value == 0]
        self.assertEqual(len(pointer_zeroes), 1)
        self.assertEqual(pointer_zeroes[0].replacement, "NULL")

    def test_call_arguments_do_not_supply_comparison_operand_type(self):
        source = ("enum Kind { KIND_NONE = 0 };\n"
                  "int IntResult(Kind kind);\n"
                  "int IntWithBool(bool value);\n"
                  "void Probe(Kind kind, bool value) {\n"
                  "  if (IntResult(kind) == 0) {}\n"
                  "  if (IntWithBool(value) != 1) {}\n"
                  "}\n")
        sites, errors = self._scan(source)
        self.assertEqual(errors, [])
        comparisons = [s for s in sites
                       if s.function.startswith("Probe") and s.value in (0, 1)]
        self.assertEqual(len(comparisons), 2)
        self.assertEqual([s for s in comparisons if s.proven], [])

    def test_direct_enum_and_bool_operands_survive_expression_wrappers(self):
        source = ("enum Kind { KIND_NONE = 0, KIND_ONE = 1 };\n"
                  "Kind KindResult();\n"
                  "bool BoolResult();\n"
                  "void Probe(Kind kind, int value) {\n"
                  "  if (KindResult() == 0) {}\n"
                  "  if ((kind) != 1) {}\n"
                  "  if (static_cast<Kind>(value) == 0) {}\n"
                  "  if (BoolResult() == 0) {}\n"
                  "}\n")
        sites, errors = self._scan(source)
        self.assertEqual(errors, [])
        replacements = [s.replacement for s in sites
                        if s.function.startswith("Probe") and s.proven]
        self.assertEqual(replacements.count("KIND_NONE"), 2)
        self.assertEqual(replacements.count("KIND_ONE"), 1)
        self.assertEqual(replacements.count("false"), 1)

    def test_pointer_zero_without_visible_null_is_not_a_fix(self):
        sites, errors = self._scan("int* P() { return 0; }\n")
        self.assertEqual(errors, [])
        self.assertEqual([s for s in sites if s.proven], [])
        self.assertTrue(any("NULL is not visible" in s.reason for s in sites))

    def test_conditional_pointer_and_integer_boolean_aliases_are_proven(self):
        source = ("#define NULL 0\n"
                  "typedef int BOOL;\n"
                  "typedef int b32;\n"
                  "int* Pick(bool use, int* value) { return use ? value : 0; }\n"
                  "BOOL WinBool() { return 1; }\n"
                  "void Takes(BOOL win, b32 project);\n"
                  "void Probe(BOOL win) { b32 project = 0; Takes(0, 1); "
                  "if (win == 0) {} project = 1; }\n")
        sites, errors = self._scan(source)
        self.assertEqual(errors, [])
        replacements = [s.replacement for s in sites if s.proven]
        self.assertEqual(replacements.count("NULL"), 1)
        self.assertEqual(replacements.count("false"), 3)
        self.assertEqual(replacements.count("true"), 3)

    def test_expected_type_does_not_retype_nested_numeric_expressions(self):
        source = ("typedef int BOOL;\n"
                  "BOOL ReturnArithmetic(int n) { return n + 0; }\n"
                  "void Takes(BOOL value);\n"
                  "void Probe(int n) { Takes(n + 0); }\n")
        sites, errors = self._scan(source)
        self.assertEqual(errors, [])
        self.assertEqual([s for s in sites if s.proven], [])

    def test_proven_fixer_uses_offsets_and_rejects_stale_source(self):
        from homm1.verify import constants
        with tempfile.TemporaryDirectory() as td:
            root = Path(td)
            source = "typedef int BOOL; BOOL F() { return 0; }\n"
            sites, errors = self._scan(source)
            self.assertEqual(errors, [])
            path = root / "src/Probe.cpp"
            path.parent.mkdir(parents=True)
            path.write_text(source)
            applied = constants.apply_proven(sites, repo=root)
            self.assertEqual(applied, 1)
            self.assertEqual(path.read_text(),
                             "typedef int BOOL; BOOL F() { return false; }\n")
            with self.assertRaises(RuntimeError):
                constants.apply_proven(sites, repo=root)

    def test_legacy_boolean_macro_gate_ignores_comments_and_strings(self):
        from homm1.verify import constants
        with tempfile.TemporaryDirectory() as td:
            root = Path(td)
            path = root / "src/Probe.cpp"
            path.parent.mkdir(parents=True)
            path.write_text("int a = TRUE; // FALSE\nconst char* s = \"TRUE\";\n")
            self.assertEqual(
                constants.legacy_boolean_spellings(repo=root),
                ["src/Probe.cpp:1:9: TRUE -> true"])

    def test_numeric_review_groups_preserve_context_without_claiming_semantics(self):
        source = ("void Sink(int value);\n"
                  "int Probe(int value) {\n"
                  "  int payload[] = { 10, 20 };\n"
                  "  Sink(30);\n"
                  "  Sink(0);\n"
                  "  if (value < 40) return (value & 0xff) + 50;\n"
                  "  return payload[value];\n"
                  "}\n")
        sites, errors = self._scan(source)
        self.assertEqual(errors, [])
        groups = {s.value: s.review_group for s in sites
                  if s.function.startswith("Probe")}
        self.assertEqual(groups[10], "initializer-payload")
        self.assertEqual(groups[20], "initializer-payload")
        self.assertEqual(groups[30], "call-argument")
        self.assertEqual(groups[0], "call-argument")
        self.assertEqual(groups[40], "comparison-or-bound")
        self.assertEqual(groups[0xff], "bitwise-or-packing")
        self.assertEqual(groups[50], "arithmetic")
        self.assertEqual([s for s in sites if s.proven], [])

    def test_consumer_rejects_missing_cl_driver_mode(self):
        _sites, errors = self._scan("int F() { return 0; }\n", flags=["/TP"])
        self.assertEqual(len(errors), 1)
        self.assertIn("requires --driver-mode=cl", errors[0])


class LabelStyleControls(unittest.TestCase):
    def _scan(self, text):
        from homm1.verify import label_style
        with tempfile.TemporaryDirectory() as td:
            p = Path(td) / "Probe.cpp"
            p.write_text(text)
            return label_style.scan(p), label_style.compgen_order(p)

    def test_off_canon_and_volatile_ordinal_fail(self):
        hits, _o = self._scan("RVA(0x12345, 0x10)\n"           # 5-digit addr
                              "RVA_COMPGEN(0x00012345, 0x10, _$E123)\n")
        self.assertTrue(any("off-canon" in why for _l, why, _t in hits))
        self.assertTrue(any("volatile" in why for _l, why, _t in hits))
        hits, _o = self._scan("RVA_DECL(0x00012345, 0x10)\n")  # a declaration has no size
        self.assertTrue(any("RVA_DECL(..) off-canon" in why for _l, why, _t in hits))

    def test_unblessed_marker_fails_and_blessed_passes(self):
        hits, _o = self._scan("// @fold-TODO: merge with CFoo\n")
        self.assertEqual(len(hits), 1)
        hits, _o = self._scan("// @early-stop cfg wall, branch counts differ\n")
        self.assertEqual(hits, [])

    def test_compgen_out_of_rva_order_fails(self):
        _h, order = self._scan("RVA(0x00001000, 0x10)\n"
                               "RVA_COMPGEN(0x00003000, 0x10, ??_GX@@UAEPAXI@Z)\n"
                               "RVA(0x00002000, 0x10)\n")
        self.assertEqual(len(order), 1)

    def test_canonical_labels_pass(self):
        hits, order = self._scan("RVA(0x00001000, 0x10)\n"
                                 "RVA_COMPGEN(0x00001400, 0x0, ??_GX@@UAEPAXI@Z)\n"
                                 "RVA(0x00002000, 0x1f)\n"
                                 "RVA_DYNINIT(0x00003000, 0x1f, "
                                 "CActRegPool<CGrunt>::s_table)\n"
                                 "RVA_DECL(0x00004000)\n")
        self.assertEqual((hits, order), ([], []))


class IncludeOrderControls(unittest.TestCase):
    def test_duplicate_and_disorder_are_caught_conservingly(self):
        from homm1.verify import include_order as io
        with tempfile.TemporaryDirectory() as td:
            p = Path(td) / "Probe.cpp"
            p.write_text("#include <rva.h>\n#include <Zed.h>\n"
                         "#include <Abc.h>\n#include <Zed.h>\n\nint x;\n")
            head, entries, tail = io.parse(p)
            headers = [h for _c, h in entries]
            dropped = [h for i, h in enumerate(headers) if h in headers[:i]]
            self.assertEqual(dropped, ["Zed.h"])
            want = io.render(head, entries, tail, None)
            self.assertNotEqual(want,
                                p.read_text().splitlines())   # out of order
            io.assert_conserved(p, p.read_text().splitlines(), want, dropped)

    def test_a_lost_line_aborts_the_rewrite(self):
        from homm1.verify import include_order as io
        with tempfile.TemporaryDirectory() as td:
            p = Path(td) / "Probe.cpp"
            p.write_text("#include <A.h>\n// @early-stop\nint x;\n")
            before = p.read_text().splitlines()
            butchered = ["#include <A.h>", "int x;"]     # marker vanished
            with self.assertRaises(SystemExit):
                io.assert_conserved(p, before, butchered, [])

    def test_a_library_header_reaching_a_consumer_is_caught(self):
        from homm1.verify import include_order as io
        with tempfile.TemporaryDirectory() as td:
            root = Path(td)
            (root / "ZTools").mkdir()
            (root / "Utils").mkdir()
            (root / "ZTools/List.h").write_text("#include <Utils/Pool.h>\n")
            (root / "Utils/Pool.h").write_text("#include <Giten/Grunt.h>\n")
            self.assertEqual(io.layering_violations(root),
                             ["Utils/Pool.h -> Giten/Grunt.h"])
            (root / "Utils/Pool.h").write_text("#include <Utils/Other.h>\n")
            self.assertEqual(io.layering_violations(root), [])


# --------------------------------------------------------------------------- #
# normal tier (synthetic Models)                                              #
# --------------------------------------------------------------------------- #
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


class TuOrderControls(unittest.TestCase):
    def test_descending_file_order_is_a_violation(self):
        from homm1.verify.tu_order import Entry, check_intra
        seq = [Entry(0x2000, 0x10, 1, "A::f", "t"),
               Entry(0x1000, 0x10, 2, "A::g", "t")]
        self.assertTrue(check_intra({"t": seq}))
        seq = [Entry(0x1000, 0x10, 1, "A::f", "t"),
               Entry(0x2000, 0x10, 2, "A::g", "t")]
        self.assertEqual(check_intra({"t": seq}), {})

    def test_overlapping_spans_are_a_violation(self):
        from homm1.verify.tu_order import Entry, check_intra
        seq = [Entry(0x1000, 0x200, 1, "A::f", "t"),
               Entry(0x1100, 0x10, 2, "A::g", "t")]
        self.assertTrue(check_intra({"t": seq}))

    def test_interleaving_tus_are_a_violation(self):
        from homm1.verify.tu_order import Entry, check_inter
        a = [Entry(0x1000, 0x10, 1, "A::f", "a"),
             Entry(0x3000, 0x10, 2, "A::g", "a")]
        b = [Entry(0x2000, 0x10, 1, "B::f", "b"),
             Entry(0x4000, 0x10, 2, "B::g", "b")]
        self.assertTrue(check_inter({"a": a, "b": b}))

    def test_the_tu_key_accepts_the_project_unit_spelling(self):
        """`--tu` keys on the .cpp STEM (`Grunt`) while every other --unit
        flag takes the unit (`grunt`); the correct unit name answered "no
        such TU"."""
        import contextlib
        import io

        from homm1.verify import tu_order as to
        seq = [to.Entry(0x1000, 0x10, 1, "CGrunt::f", "src/Grunt.cpp")]
        with mock.patch.object(to, "load_in_file_order",
                               return_value={"Grunt": seq}), \
             mock.patch.object(to, "load_exiles", return_value={}):
            with contextlib.redirect_stdout(io.StringIO()) as out:
                self.assertEqual(to.main(["--tu", "grunt"]), 0)
            self.assertIn("Grunt  (1 functions", out.getvalue())
            with contextlib.redirect_stderr(io.StringIO()) as err:
                self.assertEqual(to.main(["--tu", "zzz"]), 2)
            self.assertIn("no such TU: zzz", err.getvalue())
            with contextlib.redirect_stderr(io.StringIO()) as err:
                self.assertEqual(to.main(["--tu", "grun"]), 2)
            self.assertIn("did you mean: Grunt", err.getvalue())

    def test_a_stale_exile_row_fails(self):
        from homm1.verify.tu_order import verify_exiles
        exiles = {0x184610: ("MenuItem", "MenuPage", "CMenuItem::GetUpName")}
        bad = verify_exiles(exiles, {}, {}, {})     # nothing pins/emits/hosts
        self.assertEqual(len(bad), 2)               # no owner pin + no host span
        bad = verify_exiles(exiles, {0x184610: "MenuItem"},
                            {"MenuPage": (0x184000, 0x185000)}, {})
        self.assertEqual(bad, [])


class DataTuOrderControls(unittest.TestCase):
    def test_a_def_inside_a_foreign_band_is_a_crossing(self):
        from homm1.verify import data_tu_order as dto
        m = _model(data=[
            _binding(0x1000, "?g_a@@3HA", unit="alpha", space="data"),
            _binding(0x3000, "?g_b@@3HA", unit="alpha", space="data"),
            _binding(0x2000, "?g_x@@3HA", unit="beta", space="data")])
        with mock.patch("homm1.model.resolve", return_value=m), \
             mock.patch.object(dto, "_unit_basename",
                               return_value={"alpha": "Alpha.cpp",
                                             "beta": "Beta.cpp"}), \
             mock.patch.object(dto, "load_baseline", return_value=set()):
            self.assertTrue(dto.gate_findings())
        m = _model(data=[
            _binding(0x1000, "?g_a@@3HA", unit="alpha", space="data"),
            _binding(0x1100, "?g_b@@3HA", unit="alpha", space="data"),
            _binding(0x2000, "?g_x@@3HA", unit="beta", space="data")])
        with mock.patch("homm1.model.resolve", return_value=m), \
             mock.patch.object(dto, "_unit_basename",
                               return_value={"alpha": "Alpha.cpp",
                                             "beta": "Beta.cpp"}), \
             mock.patch.object(dto, "load_baseline", return_value=set()):
            self.assertEqual(dto.gate_findings(), [])

    def test_different_storage_bands_do_not_cross(self):
        from homm1.verify import data_tu_order as dto
        m = _model(data=[
            _binding(0x1000, "?g_a@@3HA", unit="alpha", space="data"),
            _binding(0x3000, "?g_b@@3HA", unit="alpha", space="data"),
            _binding(0x2000, "?g_x@@3HA", unit="beta", space="bss")])
        with mock.patch("homm1.model.resolve", return_value=m), \
             mock.patch.object(dto, "_unit_basename", return_value={}), \
             mock.patch.object(dto, "load_baseline", return_value=set()):
            self.assertEqual(dto.gate_findings(), [])


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


# --------------------------------------------------------------------------- #
# full tier                                                                   #
# --------------------------------------------------------------------------- #
class VtableTierControls(unittest.TestCase):
    def test_virtuality_is_read_off_the_mangled_name(self):
        from homm1.verify.vtables import classify_storage, split_mangled
        nm, quals, st = split_mangled("?RenderFrame@CFaderFlat@@UAEXH@Z")
        self.assertEqual((nm, quals[0], classify_storage(st)),
                         ("RenderFrame", "CFaderFlat", "virtual"))
        nm, quals, st = split_mangled("?Gap_17f660@@YAXXZ")
        self.assertEqual(classify_storage(st), "free function")
        nm, quals, st = split_mangled("??1CGrunt@@UAE@XZ")
        self.assertEqual((nm, quals[0], classify_storage(st)),
                         ("??1", "CGrunt", "virtual"))
        self.assertIsNone(split_mangled("_memcpy"))

    def test_wiring_defect_fails_and_inherited_virtual_passes(self):
        from homm1.verify.vtables import (base_closure, classify_storage,
                                           split_mangled)
        classes = {"CFaderFlat": (2, ["CFader"]), "CFader": (2, [])}
        allowed = base_closure("CFaderFlat", classes)
        # inherited slot bound to the base's virtual: allowed
        _n, quals, st = split_mangled("?OnInit@CFader@@UAEXXZ")
        self.assertIn(quals[0], allowed)
        self.assertEqual(classify_storage(st), "virtual")
        # a free function at the slot: the WIRING defect
        _n, _q, st = split_mangled("?Gap_17f660@@YAXXZ")
        self.assertNotEqual(classify_storage(st), "virtual")
        # a virtual of a FOREIGN class: MISBOUND
        _n, quals, st = split_mangled("?Render@CImage@@UAEXXZ")
        self.assertNotIn(quals[0], allowed)

    def test_under_virtualized_class_is_counted(self):
        from homm1.verify.vtables import resolved_virtuals
        classes = {"CShell": (1, [])}
        self.assertLess(resolved_virtuals("CShell", classes), 5)
        classes = {"CReal": (3, ["CCmdTarget"])}   # 3 own + 8 credited
        self.assertGreaterEqual(resolved_virtuals("CReal", classes), 11)

    def test_slot_resolution_honours_the_raw_target_first(self):
        from homm1.verify.vtables import resolve_slot
        syms = {0x1000: ("?Unload@CPool@@UAEXXZ", "pool"),
                0x2000: ("?FreeAll@CPool@@QAEXXZ", "pool")}
        # raw = a real 5-byte `jmp FreeAll` override: the override IS Unload
        self.assertEqual(resolve_slot(syms, 0x1000, 0x2000)[1],
                         "?Unload@CPool@@UAEXXZ")
        # anonymous ILT thunk at raw: chase to the body
        self.assertEqual(resolve_slot(syms, 0x5555, 0x2000)[1],
                         "?FreeAll@CPool@@QAEXXZ")
        self.assertIsNone(resolve_slot(syms, 0x5555, 0x6666))


class AssertRelocsControls(unittest.TestCase):
    def test_a_misdirected_referent_is_caught(self):
        """Poison ONE symbol's rva resolution and the multiset audit must
        report a WRONG row where the clean run reports none."""
        from homm1.core.paths import BUILD
        from homm1.verify import assert_relocs as ar
        if not (BUILD / "objdiff/base/butemgr.obj").is_file():
            self.skipTest("base objs absent (unbuilt tree)")
        clean, _seen = ar.audit(unit_filter="butemgr")
        if clean:
            self.skipTest("butemgr not clean on this tree state - control "
                          "needs a green substrate")
        real = ar.Resolver.resolve_base

        def poisoned(self, name, typ, addend):
            out = real(self, name, typ, addend)
            if "CButeMgr" in name and out:
                return {v + 0x10 for v in out}     # shift every CButeMgr ref
            return out
        with mock.patch.object(ar.Resolver, "resolve_base", poisoned):
            bad, _seen = ar.audit(unit_filter="butemgr")
        self.assertTrue(any("WRONG" in p for _u, _n, p in bad))

    def test_a_fabricated_symbol_is_fake(self):
        from homm1.verify import assert_relocs as ar
        resolver = mock.Mock()
        resolver.rva_of = lambda name: set()
        self.assertTrue(ar._is_fake("?M@CPhantom@@QAEXXZ", set(), resolver,
                                    set()))
        self.assertFalse(ar._is_fake("?M@CPhantom@@QAEXXZ",
                                     {"?M@CPhantom@@QAEXXZ"}, resolver,
                                     set()))
        self.assertFalse(ar._is_fake("??_C@_01ABC@x?$AA@", set(), resolver,
                                     set()))


class DataRelocsControls(unittest.TestCase):
    def test_compiler_literal_does_not_borrow_named_static_relocations(self):
        """Exercise Obj -> canon -> Resolver -> retail/paired oracle routing."""
        from types import SimpleNamespace

        from homm1.verify import data_relocs as dr

        def data_object(name):
            payload = b"literal\0"
            strings = name.encode("ascii") + b"\0"
            header = struct.pack("<HHIIIHH", 0x14c, 1, 0,
                                 60 + len(payload), 1, 0, 0)
            section = struct.pack("<8sIIIIIIHHI", b".data", 0, 0,
                                  len(payload), 60, 0, 0, 0, 0, 0xc0300040)
            symbol = struct.pack("<IIIhHBB", 0, 4, 0, 1, 0, 3, 0)
            return (header + section + payload + symbol
                    + struct.pack("<I", 4 + len(strings)) + strings)

        model = SimpleNamespace(functions=[], data=[SimpleNamespace(
            name="__$S", aliases=[], rva=0x1000, size=24)])
        image = SimpleNamespace(
            jmp_target=mock.Mock(return_value=None),
            relocs_in=mock.Mock(return_value=[(0x1000, 0x2000)]))
        with tempfile.TemporaryDirectory() as td:
            base, target = Path(td, "base.obj"), Path(td, "target.obj")
            with mock.patch("homm1.model.resolve", return_value=model), \
                 mock.patch("homm1.sema.image.retail", return_value=image), \
                 mock.patch.object(dr, "clean_units", return_value=set()), \
                 mock.patch.object(dr.pairscan, "pairs",
                                   return_value={"probe": (base, target)}):
                for name in ("_$S56", "$S56"):
                    with self.subTest(name=name):
                        base.write_bytes(data_object(name))
                        target.write_bytes(data_object(name))
                        image.relocs_in.reset_mock()
                        rows, unpaired, unresolved, stats, _dropped, eh = dr.scan()
                        self.assertEqual((rows, unpaired, unresolved, eh),
                                         ([], [], [], []))
                        self.assertEqual(stats["data symbols paired"], 1)
                        self.assertEqual(stats["data symbols pinned"], 0)
                        image.relocs_in.assert_not_called()

                # A genuinely named static must still use the retail oracle,
                # which catches its missing relocation in this negative control.
                base.write_bytes(data_object("__$S123"))
                target.write_bytes(data_object("__$S123"))
                rows, _unpaired, _unresolved, stats, _dropped, _eh = dr.scan()
                self.assertEqual([(r.verdict, r.oracle) for r in rows],
                                 [("MISSING", "retail")])
                self.assertEqual(stats["data symbols pinned"], 1)

    def test_an_any_comdat_number_is_not_an_associative_ordinal(self):
        """Integration control for the section-manifest consumer.

        cl 5 writes an `Any` COMDAT's own section number into the aux Number
        field.  Passing it through as an association produces an invalid
        `selection=2, associative_ordinal=N` manifest row which the delinker
        must reject.
        """
        import struct

        from homm1.delink import coffx, data_manifest

        header_size = 20
        section_size = 40
        raw_offset = header_size + section_size
        symbol_offset = raw_offset + 4
        header = struct.pack(
            "<HHIIIHH",
            0x14C,
            1,
            0,
            symbol_offset,
            2,
            0,
            0,
        )
        section = struct.pack(
            "<8sIIIIIIHHI",
            b".data\0\0\0",
            0,
            0,
            4,
            raw_offset,
            0,
            0,
            0,
            0,
            0xC0301040,
        )
        symbol = struct.pack("<8sIhHBB", b".data\0\0\0", 0, 1, 0, 3, 1)
        # Length=4, Number=1, Selection=Any.  Number is not an association.
        aux = struct.pack("<IHHIHB3x", 4, 0, 0, 0, 1, 2)
        payload = header + section + b"\0" * 4 + symbol + aux + struct.pack("<I", 4)

        with tempfile.TemporaryDirectory() as td:
            path = Path(td) / "probe.obj"
            path.write_bytes(payload)
            parsed = coffx.Obj(path).section_table[0]

        self.assertEqual(parsed["comdat"], 2)
        self.assertEqual(parsed["assoc"], 0)
        row = dict(parsed, object="probe.c", ordinal=1, rva=0x1000,
                   storage="data", provenance="selftest")
        line = data_manifest.section_manifest_bytes([row]).decode().splitlines()[1]
        self.assertEqual(line.split("\t")[7:10], ["2", "-", "data"])

    def test_an_injected_wrong_vtable_slot_is_caught(self):
        """The ported negative control: redirect one vtable slot's relocation
        in a copy of a real normalized base obj - the sieve must return WRONG
        (and a deleted record must return MISSING)."""
        import shutil
        import struct as st

        from homm1.delink.coffx import Obj
        from homm1.verify import data_relocs as dr
        from homm1.walls import pairscan
        src_pair = pairscan.pairs({"projectile"}).get("projectile")
        if not src_pair:
            self.skipTest("normalized projectile pair absent (unbuilt tree)")
        base_rows, *_rest = dr.scan("projectile")
        if base_rows:
            self.skipTest("projectile not clean on this tree state")
        with tempfile.TemporaryDirectory() as td:
            b = Path(td) / "base" / "projectile.obj"
            t = Path(td) / "target" / "projectile.c.obj"
            b.parent.mkdir()
            t.parent.mkdir()
            shutil.copy(src_pair[0], b)
            shutil.copy(src_pair[1], t)
            # find a vtable COMDAT DIR32 in the base copy and retarget it at
            # a different symbol index
            obj = Obj(b)
            edited = False
            for secnum in range(1, obj.nsec + 1):
                members = obj.section_members(secnum)
                if not any(n.startswith("??_7") for _v, n, _s in members):
                    continue
                sec = obj.section_table[secnum - 1]
                ptr, count = sec["reloc_offset"], sec["reloc_count"]
                if not ptr or count < 2:
                    continue
                buf = bytearray(obj.buf)
                # swap the symbol indices of the first two relocations
                s0 = st.unpack_from("<I", buf, ptr + 4)[0]
                s1 = st.unpack_from("<I", buf, ptr + 10 + 4)[0]
                if s0 == s1:
                    continue
                st.pack_into("<I", buf, ptr + 4, s1)
                st.pack_into("<I", buf, ptr + 10 + 4, s0)
                b.write_bytes(bytes(buf))
                edited = True
                break
            if not edited:
                self.skipTest("no editable vtable COMDAT found")
            with mock.patch.object(dr.pairscan, "pairs",
                                   lambda units=None: {"projectile": (b, t)}):
                rows, *_r = dr.scan("projectile")
        self.assertTrue(any(r.verdict in ("WRONG", "MISSING", "EXTRA")
                            for r in rows))


class AllocSizeControls(unittest.TestCase):
    def test_disp0_store_covers_the_ebp_form(self):
        from homm1.verify.alloc_size import _disp0_store
        # C7 06 imm32  -> mov [esi], imm32 (mod=00)
        self.assertEqual(_disp0_store(b"\xc7\x06AAAA", 0), (True, 6))
        # C7 45 00 imm32 -> mov [ebp+0x0], imm32 (mod=01 disp8=0) - the form
        # whose omission mis-attributed three CSBI_ImageSet sites
        self.assertEqual(_disp0_store(b"\xc7\x45\x00AAAA", 0), (True, 7))
        # SIB and non-zero disp are NOT plain [reg]
        self.assertEqual(_disp0_store(b"\xc7\x04\x24AAAA", 0)[0], False)
        self.assertEqual(_disp0_store(b"\xc7\x45\x04AAAA", 0)[0], False)

    def test_most_derived_uses_the_rtti_spine(self):
        from homm1.verify import alloc_size
        sw = object.__new__(alloc_size.Sweep)
        with mock.patch.object(alloc_size, "rtti_ancestors",
                               return_value={"CProjectile": {"CMovingLogic",
                                                             "CGameObject"},
                                             "CMovingLogic": {"CGameObject"}}):
            got = alloc_size.Sweep._most_derived(
                sw, {"CProjectile", "CMovingLogic"})
            self.assertEqual(got, "CProjectile")
            self.assertIsNone(alloc_size.Sweep._most_derived(
                sw, {"CProjectile", "CUnrelated"}))


class LinkTierControls(unittest.TestCase):
    def test_linked_image_only_admits_literal_100_percent_objects(self):
        """The 99.995 navigation threshold rounds a tiny object-local residue
        to EXACT, but the link audit must not relabel it as a link defect."""
        from homm1.verify import link_tier as lt
        pct = {
            ("play", "?DrawDebugStatsFull@CPlay@@QAEXXZ"): 99.99791,
            ("play", "?DrawDebugStats@CPlay@@QAEXXZ"): 100.0,
        }
        self.assertEqual(
            lt._byte_exact_symbols(pct),
            {"?DrawDebugStats@CPlay@@QAEXXZ"},
        )

    def test_unresolved_txt_content_fails(self):
        from homm1.verify import link_tier as lt
        with tempfile.TemporaryDirectory() as td:
            cand = Path(td) / "DDS.candidate.EXE"
            cand.write_bytes(b"MZ")
            unres = Path(td) / "u.txt"
            unres.write_text("?Lost@CFoo@@QAEXXZ\n")
            with mock.patch.object(lt, "CAND", cand), \
                 mock.patch.object(lt, "UNRESOLVED", unres), \
                 mock.patch("homm1.verify.undefined_closure._sym_sets",
                            return_value=(set(), set())), \
                 mock.patch("homm1.verify.undefined_closure.live_base_objs",
                            return_value=[]), \
                 mock.patch("homm1.verify.undefined_closure.lib_symbols",
                            return_value=set()):
                bad = lt.link_defect_findings()
        self.assertTrue(any("unresolved external" in b for b in bad))

    def test_a_stale_candidate_never_misattributes(self):
        import time as _t

        from homm1.verify import link_tier as lt
        with tempfile.TemporaryDirectory() as td:
            cand = Path(td) / "c.exe"
            cand.write_bytes(b"MZ")
            cmap = Path(td) / "c.map"
            cmap.write_text("x")
            objdir = Path(td) / "objdiff/base"
            objdir.mkdir(parents=True)
            newer = objdir / "a.obj"
            _t.sleep(0.01)
            newer.write_bytes(b"x")
            with mock.patch.object(lt, "CAND", cand), \
                 mock.patch.object(lt, "CMAP", cmap), \
                 mock.patch.object(lt, "BUILD", Path(td)):
                out = lt.image_diff_findings()
        self.assertTrue(out and "STALE" in out[0])


# --------------------------------------------------------------------------- #
# walls sieves                                                                #
# --------------------------------------------------------------------------- #


class DiagnosePriorClassControls(unittest.TestCase):
    """`walls diagnose` names the FIRST divergence; a review names the CAUSE.
    On 8 of the queue's 53 review-only rows those disagree - the review traced
    a branch delta to register allocation, which diagnose can only ever read as
    CFG (`CGruntPuddle`'s own review says so: "the automatic CFG label is
    downstream codegen").  Nothing told the matcher, so diagnose says it."""

    @staticmethod
    def _row(cls):
        return {"status": "bounded", "wall_class": cls, "evidence": "x"}

    def test_a_disagreeing_review_is_surfaced_with_both_labels(self):
        from homm1.walls.diagnose import prior_class_note
        note = prior_class_note(self._row("regalloc"), False, "cfg")
        self.assertIn("bounded/regalloc", note)
        self.assertIn("not cfg", note)
        self.assertIn("STALE", note)

    def test_an_agreeing_review_is_still_pointed_at(self):
        from homm1.walls.diagnose import prior_class_note
        note = prior_class_note(self._row("cfg"), True, "cfg")
        self.assertIn("agrees", note)
        self.assertIn("current", note)

    def test_no_review_prints_nothing(self):
        from homm1.walls.diagnose import prior_class_note
        self.assertIsNone(prior_class_note(None, False, "cfg"))


class PriorsBlockControls(unittest.TestCase):
    """`walls priors` reads the verdict block above a pin.  Reading only the
    CONTIGUOUS block made one blank line hide a whole verdict, so a row with a
    33-line `@early-stop` on it reported `no comment above the pin` and reached
    a worklist labelled unadjudicated.  The gap rule that fixes it can also
    MANUFACTURE a verdict out of a formatter directive, so both directions get a
    control, and both are taken from the tree rather than invented."""

    def test_one_blank_line_does_not_hide_the_verdict(self):
        """POSITIVE control, carrying the property under test: the real
        GitenMgrCmd.cpp spelling - block, blank line, pin."""
        from homm1.walls.priors import block_above
        src = ["// @early-stop", "// 313 branches on both sides.", "",
               "RVA(0x000862f0, 0x4369)"]
        self.assertEqual(block_above(src, 4),
                         ["// @early-stop", "// 313 branches on both sides."])

    def test_two_blank_lines_are_still_a_section_break(self):
        from homm1.walls.priors import block_above
        src = ["// a note about something else", "", "",
               "RVA(0x000862f0, 0x4369)"]
        self.assertEqual(block_above(src, 4), [])

    def test_a_formatter_directive_alone_is_not_a_verdict(self):
        """NEGATIVE control.  MultiStartDlg.cpp:119 is the tree's only pin whose
        gap-1 block is `// clang-format on` closing the table above it - the one
        row the gap rule would otherwise turn into a written verdict."""
        from homm1.walls.priors import block_above
        src = ["};", "// clang-format on", "", "RVA(0x000c1750, 0x88)"]
        self.assertEqual(block_above(src, 4), [])

    def test_a_directive_does_not_suppress_the_prose_beside_it(self):
        from homm1.walls.priors import block_above
        src = ["// clang-format off", "// @early-stop", "RVA(0x1, 0x2)"]
        self.assertEqual(block_above(src, 3), ["// @early-stop"])

    def test_the_contiguous_case_is_unchanged(self):
        from homm1.walls.priors import block_above
        src = ["// @early-stop", "RVA(0x1, 0x2)"]
        self.assertEqual(block_above(src, 2), ["// @early-stop"])

    def test_a_gapped_block_is_not_read_across_code(self):
        from homm1.walls.priors import block_above
        src = ["// the previous function's note", "}", "", "RVA(0x1, 0x2)"]
        self.assertEqual(block_above(src, 4), [])

    def test_a_compgen_pin_resolves_to_its_definition_name(self):
        """`RVA_COMPGEN` pins a COMDAT where it is EMITTED; the body, and any
        verdict beside it, is at the definition.  CPlay::CPlay 0x8c9d0 is pinned
        in GitenMgr.cpp and defined in Play.h under a 12-line `@early-stop`."""
        from homm1.walls.priors import source_name
        self.assertEqual(source_name("??0CPlay@@QAE@XZ"), "CPlay::CPlay")
        self.assertEqual(source_name("??1CState@@UAE@XZ"), "CState::~CState")
        self.assertEqual(source_name("?Render@CImage@@UAEHXZ"), "CImage::Render")

    def test_a_compiler_generated_thunk_resolves_to_nothing(self):
        """NEGATIVE control.  `??_G` scalar deleting destructors, `??_E` and the
        `??_L` vector ctor iterator have no source spelling at all, so guessing a
        name would attach whatever comment sits near a same-named symbol."""
        from homm1.walls.priors import source_name
        for m in ("??_GCRgn@@UAEPAXI@Z", "??_ECPlay@@UAEPAXI@Z",
                  "??_L@YGXPAXIHP6EX0@Z1@Z", "__ehreg$?Foo@@YAXXZ"):
            self.assertIsNone(source_name(m), m)

    def test_a_qualified_CALL_is_not_mistaken_for_a_definition(self):
        """NEGATIVE control, taken from the tree: `~CPlay`'s body is the single
        line `CPlay::ReleaseResources();`, so a name-only search would hand back
        the destructor's comment as that method's verdict."""
        from homm1.walls.priors import is_definition
        self.assertFalse(is_definition("    CPlay::ReleaseResources();",
                                       "CPlay::ReleaseResources"))
        self.assertFalse(is_definition("    return CPlay::Render();",
                                       "CPlay::Render"))
        self.assertTrue(is_definition("inline CPlay::CPlay() {", "CPlay::CPlay"))
        self.assertTrue(is_definition("i32 CImage::Render(CDDrawWorker* w)",
                                      "CImage::Render"))


class InlineModelGapArgumentControls(unittest.TestCase):
    """AGENTS.md points every inline/call-set wall at `walls inline-model
    --gap`, but its only form took a spec JSON of front-end `cb` estimates, so
    `--gap 0x08b960` answered `spec JSON missing` and the documented lever could
    not be invoked at an address at all.  The address form has to be reachable,
    and a mistyped spec path must NOT silently become an address."""

    @staticmethod
    def _err(argv):
        import contextlib
        import io
        from homm1.walls import inline_model
        buf = io.StringIO()
        with contextlib.redirect_stderr(buf), contextlib.suppress(SystemExit):
            inline_model.main(argv)
        return buf.getvalue()

    def test_a_missing_spec_path_still_says_spec_json_missing(self):
        """NEGATIVE control: a typo'd `.json` must not be read as an rva."""
        self.assertIn("spec JSON missing", self._err(["--gap", "nope.json"]))

    def test_an_unresolvable_address_reports_the_address_not_the_spec(self):
        """POSITIVE control carrying the property: a non-`.json` argument takes
        the address path, so its failure names the address."""
        err = self._err(["--gap", "0xdeadbee"])
        self.assertNotIn("spec JSON missing", err)

    @staticmethod
    def _repeated_site_gap(symbol_state):
        import contextlib
        import io
        from types import SimpleNamespace

        from homm1.delink import coffx
        from homm1.walls import diagnose as D
        from homm1.walls import inline_model

        caller = "?DeactivateOutside@CWwdSpatialMgr@@QAEHHH@Z"
        callee = "?RemoveAll@CDDrawChildGroup@@QAEXPAU__POSITION@@PAUCGameObject@@@Z"
        binding = SimpleNamespace(unit="u", name=caller, rva=0x168500)

        class FakeObj:
            def __init__(self, path):
                self.path = path

            def iter_symbols(self):
                if symbol_state == "absent":
                    return []
                return [(0, 0, 1 if symbol_state == "defined" else 0)]

            def sym_name(self, _index):
                return callee

        with tempfile.TemporaryDirectory() as td:
            norm = Path(td)
            (norm / "base").mkdir()
            (norm / "target").mkdir()
            (norm / "base/u.obj").touch()
            (norm / "target/u.c.obj").touch()
            with mock.patch.object(D, "NORM", norm), \
                 mock.patch.object(D, "_locate", return_value=(binding, "")), \
                 mock.patch.object(D, "_find_function", side_effect=[
                     (b"base", {}, 4), (b"target", {}, 4)]), \
                 mock.patch.object(D, "_jump_table_bytes", return_value=set()), \
                 mock.patch.object(D, "_skeleton", side_effect=[
                     (b"base", 2, 0, 0, 1, ""),
                     (b"target", 4, 0, 0, 1, "")]), \
                 mock.patch.object(D, "_call_targets", side_effect=[
                     [(callee, 0)] * 2, [(callee, 0)] * 4]), \
                 mock.patch.object(coffx, "Obj", FakeObj), \
                 contextlib.redirect_stdout(io.StringIO()) as out:
                if inline_model.gap_from_rva("0x168500") != 0:
                    raise AssertionError("gap_from_rva failed")
        return out.getvalue()

    def test_an_undefined_symbol_does_not_prove_tail_sharing(self):
        """NEGATIVE control: a declined header inline can remain undefined,
        so the symbol table alone cannot turn a call delta into tail sharing."""
        text = self._repeated_site_gap("undefined")
        self.assertIn("AMBIGUOUS: UNDEFINED external", text)
        self.assertNotIn("NOT A CANDIDATE", text)
        self.assertNotIn("tail-merged in base", text)

    def test_an_absent_symbol_allows_a_fully_expanded_header_inline(self):
        """Integration control for CGitenMgr::Run's pcount/out_waiting shape:
        base can expand every local site and therefore have no symbol row."""
        text = self._repeated_site_gap("absent")
        self.assertIn("symbol absent from the base obj", text)
        self.assertIn("consistent with full", text)
        self.assertNotIn("NOT A CANDIDATE", text)

    def test_a_fewer_call_delta_to_a_defined_comdat_can_be_expansion(self):
        """Negative control: keep the old diagnosis for an inline-visible
        definition emitted by the caller's own object."""
        text = self._repeated_site_gap("defined")
        self.assertIn("INLINE-VISIBLE", text)
        self.assertIn("Expansion is possible", text)
        self.assertNotIn("tail-merged in base", text)


class ReviewCountClaimControls(unittest.TestCase):
    """`walls recheck` re-measures the COUNT certifications a review states.  A
    review that says "base/retail agree on 24 calls" is an assertion and nothing
    re-ran it: `PlaceObjectFull` carried exactly that while the base had drifted
    to 25.  Every sentence here is taken verbatim from
    config/codex_wall_reviews.tsv, and the negatives are the two the first pass
    got wrong in each direction."""

    @staticmethod
    def _units(evidence):
        from homm1.walls.recheck import claims
        stated, _skipped = claims(evidence)
        return {(u, n) for u, n, _s in stated}

    def test_the_known_positive_is_read_through_its_semicolon(self):
        """POSITIVE control carrying the property under test: the certification
        that actually drifted.  Its agreement and its divergence share ONE
        sentence, so splitting on `.` alone reads the `vs` and throws the whole
        assertion away - which is how it escaped the first pass."""
        self.assertEqual(
            self._units(
                "Base/retail now agree at 24 calls, 96 branches, and 64 relocs; "
                "base has 15 vs retail 16 returns because the grouped preview "
                "keeps world in EBP on one arm."
            ),
            {("calls", 24), ("branches", 96), ("relocs", 64)},
        )

    def test_a_connective_is_not_a_unit_qualifier(self):
        """NEGATIVE control from 0x065e80: `returns 2/2 and relocs 249/249`
        otherwise reads as "relocs 2" and invents a failure out of a hold."""
        self.assertEqual(
            self._units("stores 90/90, returns 2/2 and relocs 249/249 exact."),
            {("returns", 2), ("relocs", 249)},
        )

    def test_the_unit_may_precede_the_pair(self):
        """0x0f42f0 writes it the other way round."""
        self.assertEqual(
            self._units("Calls 31/31 and ordered relocs 312/312."),
            {("calls", 31), ("relocs", 312)},
        )

    def test_a_two_spelling_measurement_is_not_a_certification(self):
        """NEGATIVE control from 0x163db0, whose own review states retail has 3
        returns to the base's 2: here `both` is two SOURCE spellings compared
        against each other, not base against retail."""
        self.assertEqual(
            self._units(
                "An explicit forward null-success goto and the exact authored "
                "shape suggested by retail both compile byte-identically at "
                "77.50 with 30 instructions, 3 branches and 2 returns."
            ),
            set(),
        )

    def test_a_rejected_frontier_is_not_a_certification(self):
        """NEGATIVE control from 0x13f020: a candidate that was REJECTED."""
        self.assertEqual(
            self._units(
                "Best structural frontier had 320/319 instructions, 32/32 "
                "branches and 21/21 relocs but emitted bank arithmetic before "
                "both Locks, so it was rejected."
            ),
            set(),
        )

    def test_a_disposable_probe_is_not_a_certification(self):
        """NEGATIVE control from 0x17a460: measured under `inline_depth(0)`,
        which the committed tree does not carry."""
        self.assertEqual(
            self._units(
                "Disposable inline_depth(0) at those expressions proves 59/59 "
                "calls, 43/43 branches, 65/65 relocs."
            ),
            set(),
        )

    def test_a_one_sided_count_is_never_read_as_agreement(self):
        """0x02a570 states its branch delta; asserting it back would test the
        parser's word order, not the tree."""
        self.assertEqual(
            self._units(
                "Retail homes coordList at entry and reloads it on both sides "
                "of RemoveAll, requiring the sole merge jump (38 vs 37 branches)."
            ),
            set(),
        )


class SourceClaimGateControls(unittest.TestCase):
    """The SECOND written-verdict store: the `//` block above an `RVA()` pin.

    `walls priors` has always read both stores and only the ledger was ever
    re-measured, which is how nine notes came to state something the pair no
    longer held - four of them a FRAME difference that later work had closed,
    so a reader would have gone hunting for a frame that already matches.

    The three-way verdict is the point of these controls.  BROKEN (the two
    sides stopped agreeing) is a defect and reaches the gate; DRIFT (they still
    agree, but not at the stated number) is a RETIRED RULER and must NOT, because
    several of these notes were written against a tool that counted CONDITIONAL
    branches where the pair reader counts every branch.  Collapsing the two
    would fail builds on arithmetic nobody got wrong.
    """

    RVA = 0x078A50
    SITE = ("src/Giten/TriggerMgr.cpp", 427)
    NAME = "?PlaceObjectFull@CTriggerMgr@@QAEHPAVCGrunt@@HH@Z"

    def _run(self, note, counts):
        import types as _types

        from homm1.walls import recheck
        b = _types.SimpleNamespace(rva=self.RVA, unit="triggermgrplace",
                                   name=self.NAME)
        with mock.patch("homm1.walls.priors._pin_sites",
                        return_value={self.RVA: [self.SITE]}), \
             mock.patch("homm1.walls.priors._comment_above",
                        return_value=[f"// {note}"]), \
             mock.patch("homm1.walls.diagnose.named_functions",
                        return_value={}), \
             mock.patch("homm1.walls.diagnose._locate", return_value=(b, "")), \
             mock.patch.object(recheck, "measure",
                               return_value=(counts, "inline")):
            return recheck.source_gate_findings(), recheck.source_sweep()

    AGREE = "Block topology and branch sequence agree exactly, 4 rets on both sides."
    HOLDING = {"calls": (24, 24), "branches": (96, 96), "returns": (4, 4),
               "relocs": (64, 64), "insns": (300, 300), "bytes": (0x400, 0x400)}
    SPLIT = dict(HOLDING, returns=(3, 4))
    DRIFTED = dict(HOLDING, returns=(5, 5))

    def test_a_note_whose_sides_still_agree_is_silent(self):
        """NEGATIVE control: the claim measures exactly, so nothing is a finding."""
        out, sweep = self._run(self.AGREE, self.HOLDING)
        self.assertEqual(out, [])
        self.assertEqual([v[0] for v in sweep[0]["verdicts"]], ["HOLD"])

    def test_a_note_whose_sides_stopped_agreeing_reaches_the_gate(self):
        """POSITIVE control carrying the property under test end to end."""
        out, sweep = self._run(self.AGREE, self.SPLIT)
        self.assertEqual([v[0] for v in sweep[0]["verdicts"]], ["BROKEN"])
        self.assertEqual(len(out), 1, out)
        self.assertIn("states both sides at 4 returns", out[0])
        self.assertIn("base 3 / target 4", out[0])
        self.assertIn(f"{self.SITE[0]}:{self.SITE[1]}", out[0])

    def test_a_retired_ruler_is_DRIFT_and_never_a_finding(self):
        """NEGATIVE control for the distinction the gate is built on: the sides
        agree at 5 where the note says 4, so the CLAIM holds and only the number
        is from another instrument.  It must be visible in the sweep and absent
        from the gate."""
        out, sweep = self._run(self.AGREE, self.DRIFTED)
        self.assertEqual([v[0] for v in sweep[0]["verdicts"]], ["DRIFT"])
        self.assertEqual(out, [])

    def test_a_residue_sentence_is_not_an_agreement(self):
        """`residue is 2 insns: retail loads BOTH operands ...` states a
        DIVERGENCE and happens to contain an agreement trigger.  Reading it as a
        claim invented three failures on the first sweep."""
        note = ("residue is 2 insns: retail loads BOTH operands of the second "
                "difference where cl folds one into a single fsubr.")
        out, sweep = self._run(note, self.HOLDING)
        self.assertEqual(sweep, [])
        self.assertEqual(out, [])

    def test_an_unmeasurable_pair_is_not_a_pass(self):
        """A note whose pair cannot be read has not been re-measured, and a gate
        that stays silent about that reports a green it did not verify."""
        import types as _types

        from homm1.walls import recheck
        b = _types.SimpleNamespace(rva=self.RVA, unit="triggermgrplace",
                                   name=self.NAME)
        with mock.patch("homm1.walls.priors._pin_sites",
                        return_value={self.RVA: [self.SITE]}), \
             mock.patch("homm1.walls.priors._comment_above",
                        return_value=[f"// {self.AGREE}"]), \
             mock.patch("homm1.walls.diagnose.named_functions",
                        return_value={}), \
             mock.patch("homm1.walls.diagnose._locate", return_value=(b, "")), \
             mock.patch.object(recheck, "measure",
                               return_value="normalized pair missing"):
            out = recheck.source_gate_findings()
        self.assertEqual(len(out), 1, out)
        self.assertIn("unmeasured", out[0])


class ReviewClaimGateControls(unittest.TestCase):
    """The `review-claims` tier row - the certifications re-measured on EVERY
    build.  It exists because the MAX gate structurally cannot see this class
    of drift: c1c1616a gave `PlaceObjectFull` the 16th `ret` retail has and
    silently lost a cross-jump elsewhere (calls +1, branches -1, relocs +1),
    the SCORE ROSE, and the source hash did not move, so neither the ledger nor
    a source reader had anything to look at.  The recognizer controls above
    prove the parser; these drive `gate_findings`, which is what the tier
    calls."""

    RVA = 0x078A50

    def _findings(self, evidence, counts, *, resolves=True, measurable=True):
        import types as _types

        from homm1.walls import recheck
        row = {"src_hash": "f96c455cab0e", "status": "open",
               "wall_class": "inline", "evidence": evidence}
        b = _types.SimpleNamespace(
            rva=self.RVA, unit="triggermgrplace",
            name="?PlaceObjectFull@CTriggerMgr@@QAEHPAVCGrunt@@HH@Z")
        located = (b, "") if resolves else (
            None, f"no CLAIMED function starts at 0x{self.RVA:x}")
        measured = ((counts, "inline") if measurable
                    else "normalized pair missing for triggermgrplace")
        with mock.patch("homm1.walls.reviews.load",
                        return_value={self.RVA: row}), \
             mock.patch("homm1.walls.reviews.current", return_value={}), \
             mock.patch("homm1.walls.diagnose._locate", return_value=located), \
             mock.patch.object(recheck, "source_gate_findings", return_value=[]), \
             mock.patch.object(recheck, "measure", return_value=measured):
            return recheck.gate_findings()

    #: the drifted pair c1c1616a actually produced, against the certification
    #: the review still carried
    DRIFTED = {"calls": (25, 24), "branches": (95, 96), "returns": (16, 16),
               "relocs": (65, 64), "insns": (300, 300), "bytes": (0x400, 0x400)}
    HOLDING = {"calls": (24, 24), "branches": (96, 96), "returns": (15, 16),
               "relocs": (64, 64), "insns": (300, 300), "bytes": (0x400, 0x400)}
    CERT = ("Base/retail now agree at 24 calls, 96 branches, and 64 relocs; "
            "base has 15 vs retail 16 returns because the grouped preview "
            "keeps world in EBP on one arm.")

    def test_the_known_positive_reaches_the_gate(self):
        """POSITIVE control carrying the property under test end to end: the
        certification that drifted, measured against the pair that drifted it.
        A parser control alone would not prove the GATE reads it."""
        out = self._findings(self.CERT, self.DRIFTED)
        self.assertEqual(len(out), 3, out)
        self.assertTrue(all(f"0x{self.RVA:06x}" in f for f in out))
        self.assertEqual(
            {f.split("both sides at ")[1].split(" - ")[0] for f in out},
            {"24 calls, now base 25 / target 24",
             "96 branches, now base 95 / target 96",
             "64 relocs, now base 65 / target 64"},
        )

    def test_a_holding_certification_is_silent(self):
        """The clean pass.  The stated `15 vs 16` returns divergence is NOT a
        claim, so a pair that still shows it must not fail the gate."""
        self.assertEqual(self._findings(self.CERT, self.HOLDING), [])

    def test_a_review_row_that_names_no_claimed_function_fails(self):
        """A review whose row no longer exists is a dangling ledger entry, not
        a hold: nothing was measured, so nothing may pass."""
        out = self._findings(self.CERT, self.HOLDING, resolves=False)
        self.assertEqual(len(out), 1, out)
        self.assertIn("names no claimed function", out[0])

    def test_an_unreadable_pair_is_not_a_pass(self):
        """A gate that SKIPS is not a gate that PASSES: a stated count whose
        normalized pair cannot be read is reported, not assumed green."""
        out = self._findings(self.CERT, None, measurable=False)
        self.assertEqual(len(out), 1, out)
        self.assertIn("unmeasurable", out[0])
        # ... but a review that certifies NO count has nothing to measure.
        self.assertEqual(
            self._findings("Regalloc wall: retail keeps world in EBP.",
                           None, measurable=False), [])

    def test_a_non_certification_sentence_cannot_fail_the_gate(self):
        """NEGATIVE control, the three sentence classes the parser must not
        read as a certification of the committed pair: two source spellings
        measured against each other, a REJECTED frontier, and a disposable
        `inline_depth(0)` probe.  Each is paired with counts that contradict
        its numbers - a gate that read them would fire here."""
        contradicting = {"calls": (1, 2), "branches": (1, 2),
                         "returns": (1, 2), "relocs": (1, 2),
                         "insns": (1, 2), "bytes": (1, 2)}
        for sentence in (
            "An explicit forward null-success goto and the exact authored "
            "shape suggested by retail both compile byte-identically at 77.50 "
            "with 30 instructions, 3 branches and 2 returns.",
            "Best structural frontier had 320/319 instructions, 32/32 "
            "branches and 21/21 relocs but emitted bank arithmetic before "
            "both Locks, so it was rejected.",
            "Disposable inline_depth(0) at those expressions proves 59/59 "
            "calls, 43/43 branches, 65/65 relocs.",
        ):
            self.assertEqual(self._findings(sentence, contradicting), [],
                             sentence)

    def test_the_tier_runs_it(self):
        """The row is REGISTERED, and it is this gate the runner calls - a
        gate nothing invokes is documentation."""
        import contextlib
        import io

        from homm1.verify import tiers
        self.assertIn("review-claims", dict(tiers.TIERS["normal"]))
        with mock.patch("homm1.walls.recheck.gate_findings",
                        return_value=["planted drift"]) as g:
            with contextlib.redirect_stdout(io.StringIO()) as out:
                failed = tiers.run(["normal"])
        g.assert_called_once()
        self.assertGreaterEqual(failed, 1)
        self.assertIn("review-claims: FAIL", out.getvalue())
        # ONE implementation: the registered gate module IS the walls tool the
        # campaign runs by hand, not a second copy that could drift from it.
        from homm1.verify import _GATES
        self.assertEqual(_GATES["review-claims"], "homm1.walls.recheck")
        self.assertEqual(tiers._rerun_command("review-claims"),
                         "homm1 verify review-claims")


class PairscanControls(unittest.TestCase):
    def test_rep_movsd_folds_and_movsx_does_not_count(self):
        from homm1.walls.aggregate_copies import REP_MOVS
        self.assertTrue(REP_MOVS.match("rep movs"))
        self.assertTrue(REP_MOVS.match("rep movsd"))
        self.assertFalse(REP_MOVS.match("movs"))       # bare table byte decode
        self.assertFalse(REP_MOVS.match("movsx"))      # sign-extend load
        # and the decoder really produces the folded spelling for F3 A5
        try:
            from homm1.delink.coffx import Obj  # noqa: F401 (env probe)
            from homm1.tool import objdump
            text = objdump.disassemble(b"\xf3\xa5\xc3", vma=0)
        except Exception as exc:  # noqa: BLE001
            self.skipTest(f"objdump unavailable: {exc}")
        self.assertIn("rep movs", text)

    def test_aggregate_copy_sieve_excludes_proven_current_dips(self):
        from homm1.walls import aggregate_copies
        row = {"unit": "u", "symbol": "?f@@YAXXZ", "cur": 75.0,
               "proven": True}
        with mock.patch("homm1.walls.pairscan.require_pairs"), \
             mock.patch("homm1.walls.inventory.build", return_value=[row]), \
             mock.patch("homm1.walls.pairscan.pairs") as pairs:
            self.assertEqual(aggregate_copies.scan(), [])
        pairs.assert_not_called()

    def test_aggregate_copy_cli_reports_a_source_cfg_lead(self):
        import contextlib
        import io

        from homm1.walls import aggregate_copies
        hit = (-1, "butemgr", 77.03, 2, 1,
               "?SetString@CButeMgr@@QAEXPBD0ABVCString@@@Z")
        out = io.StringIO()
        with mock.patch.object(aggregate_copies, "scan", return_value=[hit]), \
             mock.patch("homm1.walls.check_unit"), \
             contextlib.redirect_stdout(out):
            self.assertEqual(aggregate_copies.main([]), 0)
        text = out.getvalue()
        self.assertIn("base has more surviving copy blocks", text)
        self.assertIn("source/CFG leads: 1", text)
        self.assertNotIn("we copy an object retail does not", text)

    def test_local_labels_are_not_function_boundaries(self):
        from homm1.walls.pairscan import is_local_label
        self.assertTrue(is_local_label("$L27"))
        self.assertTrue(is_local_label("$loop_restart$32243"))
        self.assertFalse(is_local_label("?Advance@CAni@@QAEXXZ"))

    def test_canon_folds_static_suffix_and_vector_dtor(self):
        from homm1.walls.pairscan import canon
        self.assertEqual(canon("_s_QUESTZ$Sdata_data_87db2c_0"), "_s_QUESTZ")
        self.assertEqual(canon("__$S"), "__")
        self.assertEqual(canon("__$S123"), "__")
        for name in ("_$S56", "$S56", "_$S0", "$S0"):
            self.assertEqual(canon(name), name)
        self.assertEqual(canon("??_EzPTree@@UAEPAXI@Z"),
                         "??_GzPTree@@UAEPAXI@Z")

    def test_resolver_keeps_compiler_ordinals_distinct_from_named_statics(self):
        from types import SimpleNamespace

        from homm1.verify.assert_relocs import Resolver
        from homm1.walls.pairscan import DIR32

        model = SimpleNamespace(functions=[], data=[
            SimpleNamespace(name="__$S", aliases=[], rva=0x1000, size=24),
            SimpleNamespace(name="_named$S123", aliases=[], rva=0x2000, size=4),
        ])
        image = SimpleNamespace(jmp_target=lambda rva: None)
        with mock.patch("homm1.model.resolve", return_value=model), \
             mock.patch("homm1.sema.image.retail", return_value=image):
            resolver = Resolver()
        for name in ("_$S56", "$S56", "_$S0", "$S0"):
            with self.subTest(name=name):
                self.assertEqual(resolver.rva_of(name), set())
                self.assertEqual(resolver.resolve_base(name, DIR32, 4), set())
        for name in ("_", "__", "__$S", "__$S123", "__$Sdata_data_abcd_0"):
            self.assertEqual(resolver.rva_of(name), {0x1000})
        for name in ("named", "_named", "_named$S", "_named$S456"):
            self.assertEqual(resolver.rva_of(name), {0x2000})
        self.assertEqual(resolver.resolve_base("__$S123", DIR32, 4), {0x1004})


class EhFrameControls(unittest.TestCase):
    PROLOGUE = [
        (0, "push", "0xffffffff"),
        (2, "push", "0x0"),
        (7, "mov", "eax,DWORD PTR fs:0x0"),
        (13, "push", "eax"),
        (14, "mov", "DWORD PTR fs:0x0,esp"),
        (21, "sub", "esp,0x8"),
        (24, "push", "esi"),
    ]

    def test_the_full_sequence_is_required(self):
        from homm1.walls.eh_frame import has_eh
        self.assertTrue(has_eh(self.PROLOGUE))
        no_push = [i for i in self.PROLOGUE if i[2] != "0xffffffff"]
        self.assertFalse(has_eh(no_push))              # fs store alone != EH
        no_store = [i for i in self.PROLOGUE
                    if not i[2].startswith("DWORD PTR fs:0x0,")]
        self.assertFalse(has_eh(no_store))

    def test_presence_mismatch_classifies(self):
        from homm1.walls.eh_frame import classify
        plain = [(0, "push", "esi"), (1, "ret", "")]
        self.assertEqual(classify(plain, self.PROLOGUE), "TARGET_ONLY")
        self.assertEqual(classify(self.PROLOGUE, plain), "BASE_ONLY")
        self.assertEqual(classify(plain, plain), "NEITHER")

    def test_state_stores_are_counted_at_the_slot(self):
        from homm1.walls.eh_frame import eh_states
        insns = self.PROLOGUE + [
            (30, "mov", "DWORD PTR [esp+0x14],0x0"),
            (38, "call", "0x100"),
            (43, "mov", "BYTE PTR [esp+0x14],0x1"),
            (48, "mov", "DWORD PTR [esp+0x14],0xffffffff"),
        ]
        slot, states = eh_states(insns)
        self.assertEqual(slot, "[esp+0x14]")
        self.assertEqual(len(states), 3)
        self.assertEqual(sorted(s for _o, s in states if s is not None),
                         [-1, 0, 1])

    def test_inline_cut_vs_exit_merge_vs_object(self):
        from homm1.walls.eh_frame import cause, ctor_delta
        only_t, only_b, resited = ctor_delta(
            [(0, "??1CString@@QAE@XZ")],
            [(0, "??1CString@@QAE@XZ"), (5, "??0CRect@@QAE@XZ")])
        self.assertEqual(cause("TARGET_ONLY", 1, only_t, only_b, resited),
                         "INLINE_CUT")
        only_t, only_b, resited = ctor_delta(
            [(0, "??1CString@@QAE@XZ")] * 2,
            [(0, "??1CString@@QAE@XZ")] * 8)
        self.assertEqual(cause("TARGET_ONLY", 1, only_t, only_b, resited),
                         "EXIT_MERGE")
        self.assertEqual(cause("TARGET_ONLY", 1, [], [], []),
                         "MISSING_OBJECT")
        self.assertEqual(cause("BASE_ONLY", -1, [], [], []), "EXTRA_OBJECT")
        self.assertEqual(cause("BOTH", 1, [], [], []), "STATE_FLOW")

    def test_states_cli_does_not_call_state_flow_a_missing_object(self):
        import contextlib
        import io

        from homm1.walls import eh_frame
        row = dict(unit="grunt", name="?StepArrivalDrop@CGrunt@@QAEHXZ",
                   rva="0x04b370", fuzzy=0.0, size=2960, verdict="BOTH",
                   cause="STATE_FLOW", extra_ctors=[], our_ctors=[], resited=[],
                   base_insn=877, tgt_insn=853, slot="[esp+0x68]",
                   states=[-1, 0, 1], base_states=7, tgt_states=8,
                   first=0x2e1, last=0xa39, unwind=True)
        out = io.StringIO()
        with mock.patch.object(eh_frame.pairscan, "require_pairs"), \
             mock.patch.object(eh_frame, "scan", return_value=[row]), \
             mock.patch("homm1.walls.check_unit"), \
             contextlib.redirect_stdout(out):
            self.assertEqual(eh_frame.main(["--states"]), 0)
        text = out.getvalue()
        self.assertIn("STATE_FLOW", text)
        self.assertNotIn("MISSING_OBJECT", text)


class GlobalRefsControls(unittest.TestCase):
    def test_addend_past_the_symbol_is_dropped(self):
        import collections

        from homm1.walls import global_refs as gr
        relocs = [(0x10, "_g_dir", gr.DIR32, 0x2B0),   # unsized-datum fallback
                  (0x20, "_g_dir", gr.DIR32, 0x0)]     # a real read
        obj = mock.Mock()
        with mock.patch.object(gr.pairscan, "fn_relocs",
                               return_value=relocs):
            dropped = collections.Counter()
            c = gr._refs(obj, 1, 0, 0x100, gr.DIR32, dropped,
                         {"_g_dir": 12})
        self.assertEqual(c["_g_dir"], 1)
        self.assertEqual(sum(dropped.values()), 1)


class StaleMarkerControls(unittest.TestCase):
    def test_marker_resolution_prefers_plain_rva(self):
        from homm1.walls.stale_markers import RVA, RVA_COMPGEN
        self.assertTrue(RVA.search("RVA(0x00012340, 0x10)"))
        self.assertIsNone(RVA.search("RVA_COMPGEN(0x00012340, 0x10, X)"))
        self.assertTrue(RVA_COMPGEN.search("RVA_COMPGEN(0x00012340, 0x10, X)"))

    def test_header_inline_definition_has_a_qualified_owner(self):
        from homm1.walls.stale_markers import QUALIFIED_DEF
        m = QUALIFIED_DEF.search("inline CPlay::CPlay() {")
        self.assertIsNotNone(m)
        self.assertEqual(m.group(1), "CPlay::CPlay")


class WallReviewControls(unittest.TestCase):
    def test_exact_requires_a_hash_valid_100_percent_bank(self):
        from homm1.walls import reviews

        rows = {0x1000: (100.0, 100.0, "same")}
        with mock.patch("homm1.walls.inventory.baseline_rows", return_value=rows):
            self.assertTrue(reviews.exact_is_banked(0x1000, "same"))
            self.assertFalse(reviews.exact_is_banked(0x1000, "different"))
            self.assertFalse(reviews.exact_is_banked(0x2000, "same"))

    def test_source_edit_invalidates_a_personal_review(self):
        from types import SimpleNamespace

        from homm1.walls import reviews
        saved = {
            0x1000: {
                "src_hash": "old", "status": "bounded",
                "wall_class": "cfg", "evidence": "checked",
            },
            0x2000: {
                "src_hash": "same", "status": "open",
                "wall_class": "inline", "evidence": "inspect site",
            },
        }
        funcs = [
            SimpleNamespace(rva=0x1000, unit="u", name="changed"),
            SimpleNamespace(rva=0x2000, unit="u", name="unchanged"),
        ]
        hashes = {("u", "changed"): "new", ("u", "unchanged"): "same"}

        def fingerprinter():
            return lambda unit, name: hashes[(unit, name)], None, set()

        with mock.patch.object(reviews, "load", return_value=saved), \
             mock.patch("homm1.model.resolve",
                        return_value=SimpleNamespace(functions=funcs)), \
             mock.patch("homm1.verify.fingerprints.fingerprinter",
                        side_effect=fingerprinter):
            self.assertEqual(set(reviews.current()), {0x2000})

    def test_todo_excludes_only_hash_valid_terminal_reviews(self):
        from types import SimpleNamespace
        from homm1.walls import inventory

        names = {
            0x1000: "?Bounded@CThing@@QAEHXZ",
            0x2000: "?Open@CThing@@QAEHXZ",
            0x3000: "?New@CThing@@QAEHXZ",
            0x4000: "?Proven@CThing@@QAEHXZ",
            0x5000: "?Exact@CThing@@QAEHXZ",
        }
        funcs = [
            SimpleNamespace(unit="u", name=name, rva=rva, size=0x10)
            for rva, name in names.items()
        ]
        scores = {("u", name): 75.0 for name in names.values()}
        scores[("u", "__ehunwind$?Open@CThing@@QAEHXZ$0")] = 50.0
        baseline = {
            rva: (75.0, 100.0 if rva == 0x4000 else 75.0, f"h{rva:x}")
            for rva in names
        }
        reviewed = {
            0x1000: {
                "status": "bounded", "wall_class": "cfg", "evidence": "done",
            },
            0x2000: {
                "status": "open", "wall_class": "inline",
                "evidence": "inspect site",
            },
            0x5000: {
                "status": "exact", "wall_class": "cfg", "evidence": "100%",
            },
        }
        with mock.patch.object(inventory, "report_scores",
                               return_value=("report", scores)), \
             mock.patch.object(inventory, "baseline_rows", return_value=baseline), \
             mock.patch("homm1.model.resolve",
                        return_value=SimpleNamespace(functions=funcs)), \
             mock.patch("homm1.walls.reviews.current", return_value=reviewed):
            rows = inventory.build(todo=True)
        self.assertEqual([row["rva"] for row in rows], ["0x002000", "0x003000"])
        self.assertEqual(rows[0]["review_status"], "open")
        self.assertEqual(rows[1]["review_status"], "")

    def test_headroom_is_hist_minus_the_BANK_not_hist_minus_cur(self):
        """THE distinction that decides whether a row is a question at all.

        `cur` below the BANK is this exact source scoring lower than it
        already has - TU composition, no action. The BANK below `hist` is a
        source EDIT that gave the peak up. Read the wrong way,
        `CGruntSelectedSprite::Update` (cur 84.85, bank 99.24, hist 99.24)
        is the biggest opportunity in the queue; read the right way there is
        nothing to do, and 20 of the 66 rows whose hist beats their cur are
        that shape.
        """
        from types import SimpleNamespace
        from homm1.walls import inventory

        names = {0x1000: "?TuNoise@C@@QAEHXZ", 0x2000: "?EditGaveItUp@C@@QAEHXZ"}
        funcs = [SimpleNamespace(unit="u", name=n, rva=r, size=0x10)
                 for r, n in names.items()]
        scores = {("u", "?TuNoise@C@@QAEHXZ"): 84.85,
                  ("u", "?EditGaveItUp@C@@QAEHXZ"): 70.12}
        baseline = {0x1000: (99.24, 99.24, "h1"),     # bank == hist
                    0x2000: (70.12, 77.52, "h2")}     # bank below hist
        with mock.patch.object(inventory, "report_scores",
                               return_value=("report", scores)), \
             mock.patch.object(inventory, "baseline_rows", return_value=baseline), \
             mock.patch("homm1.model.resolve",
                        return_value=SimpleNamespace(functions=funcs)):
            rows = {r["symbol"]: r for r in inventory.build()}
        self.assertEqual(rows["?TuNoise@C@@QAEHXZ"]["lost"], 0.0)
        self.assertTrue(rows["?TuNoise@C@@QAEHXZ"]["regressed"])
        self.assertAlmostEqual(rows["?EditGaveItUp@C@@QAEHXZ"]["lost"], 7.40, 2)
        self.assertFalse(rows["?EditGaveItUp@C@@QAEHXZ"]["regressed"])


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


# --------------------------------------------------------------------------- #
# the data-access map: layout oracle, every category, every suppression       #
# --------------------------------------------------------------------------- #
def _layout(types=()):
    from homm1.verify.layout import Layout
    return Layout({"types": list(types), "units": {}, "tree_hash": "ctrl"})


def _prim(t, sz):
    return {"k": "prim", "t": t, "sz": sz}


def _arr(el, n, t="T[]"):
    return {"k": "arr", "t": t, "sz": el["sz"] * n, "n": n, "el": el}


def _claim(rva, node, name="?g_probe@@3HA", extent=None, channel="src",
           unit="probe"):
    from homm1.verify.access_map import Claim
    return Claim(rva=rva, name=name, unit=unit, channel=channel, kind="",
                 section=".data", space="data",
                 extent=extent if extent is not None else (node or {}).get("sz")
                 or 4, node=node, pct=100.0)


def _access(target, width=4, form="direct", rw="r", mnemonic="mov", text="",
            insn=0x1000, fpu="", ext="", scale=0, base_reg="", disp=0):
    from homm1.verify.access_map import Access
    return Access(insn_rva=insn, insn_len=6, mnemonic=mnemonic, site_rva=insn,
                  target_rva=target, width=width, rw=rw, form=form,
                  base_reg=base_reg, index_reg="eax" if scale else "",
                  scale=scale, disp=disp, fpu=fpu, ext=ext, origin="reloc",
                  text=text or f"{mnemonic} probe", owner=None)


def _findings(claims, accesses, rows=(), layout=None, cells=(), image=None):
    """Categories fired by one synthetic claim set - hermetic, no image."""
    from homm1.verify import data_access as da
    img = image if image is not None else mock.Mock()
    img.pe.data_regions.return_value = {"rdata": (0, 0), "data": (0, 1 << 30),
                                        "bss": (0, 0),
                                        "idata": (0x2C3000, 0x2C6C00)}
    img.section_name.return_value = ".data"
    img.u32.return_value = 0x2C4CCC
    model = mock.Mock()
    model.functions = []
    spine = da.Spine(img, model, layout or _layout(), sorted(claims,
                                                             key=lambda c: c.rva),
                     list(rows))
    owners = mock.Mock()
    owners.at.return_value = None
    found, _st = da.derive_findings(spine, list(accesses), list(cells), owners)
    return found


def _cats(rows):
    return {r[0] for r in rows}


class LayoutOracleControls(unittest.TestCase):
    def test_an_array_element_offset_is_absolute(self):
        """The measured own-goal: resolving +0x4 of `int[32]` returned the
        ELEMENT-relative 0, so every array offset past the first read as
        'lands INSIDE field [1]' - 42 fabricated width findings."""
        lay = _layout()
        node = _arr(_prim("int", 4), 32, "int[32]")
        f = lay.field_at(node, 4)
        self.assertEqual((f.off, f.size, f.path, f.tag), (4, 4, "[1]", ""))
        self.assertEqual(lay.field_at(node, 0x7C).off, 0x7C)
        self.assertEqual(lay.field_at(node, 0x80).tag, "out")

    def test_a_union_is_laid_out_but_never_adjudicated(self):
        lay = _layout()
        u = {"k": "rec", "t": "U", "sz": 8, "u": 1,
             "m": [[0, ".a", _prim("int", 4)], [0, ".b", _prim("float", 4)]]}
        self.assertFalse(lay.field_at(u, 0).resolved)

    def test_the_vptr_slot_is_tagged_not_reported_as_a_hole(self):
        lay = _layout()
        poly = {"k": "rec", "t": "CFoo", "sz": 8, "poly": 1,
                "m": [[0, ".__vfptr", _prim("void *", 4)],
                      [4, ".m_x", _prim("int", 4)]]}
        self.assertEqual(lay.field_at(poly, 0).path, ".__vfptr")
        plain = {"k": "rec", "t": "CBar", "sz": 8,
                 "m": [[4, ".m_x", _prim("int", 4)]]}
        self.assertEqual(lay.field_at(plain, 0).tag, "hole")

    def test_the_harvest_joins_every_src_claim_on_this_tree(self):
        """The oracle is only worth its verdicts if it actually reaches the
        claims: a silent join failure would look exactly like a clean tree."""
        from homm1.core.paths import BUILD
        from homm1.model import resolve
        from homm1.verify.layout import CACHE, harvest
        if not CACHE.is_file() or not (BUILD / "objdiff/base").is_dir():
            self.skipTest("layout cache absent (unbuilt tree)")
        lay, _p = harvest()
        miss = [b.name for b in resolve().data
                if b.channel == "src" and not lay.var(b.unit, b.name)]
        self.assertEqual(miss[:5], [])


class RecordCopyWidthControls(unittest.TestCase):
    def fixture(self, *, last_store=True, wrong_destination=False, altered_value=False,
                extra_read=False, partial_register=False, overwrite_store=False):
        import struct
        node = {'k': 'rec', 't': 'SDKRecord', 'sz': 16, 'm': [
            [0, '.Data1', _prim('unsigned long', 4)],
            [4, '.Data2', _prim('unsigned short', 2)],
            [6, '.Data3', _prim('unsigned short', 2)],
            [8, '.Data4', _arr(_prim('unsigned char', 1), 8)],
        ]}
        blob = bytearray()
        accesses = []
        for offset in range(0, 16, 4):
            site = 0x1000 + len(blob)
            blob += b'\xa1' + struct.pack('<I', 0x402000 + offset)
            access = _access(0x2000 + offset, insn=site)
            access.insn_len = 5
            accesses.append(access)
            if offset == 0:
                blob += bytes.fromhex('83ec10 8bd4 51')
            if altered_value and offset == 4:
                blob += b'\x40'  # inc eax: value no longer a copy
            if partial_register and offset == 4:
                blob += bytes.fromhex('b000')  # mov al,0 also clobbers eax
            if offset != 12 or last_store:
                dest = 0x10 if wrong_destination and offset == 12 else offset
                blob += b'\x89\x02' if dest == 0 else b'\x89\x42' + bytes([dest])
            if overwrite_store and offset == 4:
                blob += bytes.fromhex('c7420400000000')  # mov DWORD PTR [edx+4],0
        blob += b'\xc3'
        image = mock.Mock()
        image.read.side_effect = lambda rva, size: bytes(blob[rva - 0x1000:rva - 0x1000 + size])
        if extra_read:
            accesses.append(_access(0x2004, insn=0x2000))
        return _claim(0x2000, node), accesses, image

    def test_complete_decoded_copy_passes_the_full_width_consumer(self):
        claim, accesses, image = self.fixture()
        self.assertNotIn('width', _cats(_findings([claim], accesses, image=image)))

    def test_missing_store_does_not_suppress_the_wide_read(self):
        claim, accesses, image = self.fixture(last_store=False)
        self.assertIn('width', _cats(_findings([claim], accesses, image=image)))

    def test_wrong_destination_stride_is_not_a_record_copy(self):
        claim, accesses, image = self.fixture(wrong_destination=True)
        self.assertIn('width', _cats(_findings([claim], accesses, image=image)))

    def test_modified_register_is_not_a_copy(self):
        claim, accesses, image = self.fixture(altered_value=True)
        self.assertIn('width', _cats(_findings([claim], accesses, image=image)))

    def test_one_valid_copy_does_not_exempt_other_accesses(self):
        claim, accesses, image = self.fixture(extra_read=True)
        self.assertIn('width', _cats(_findings([claim], accesses, image=image)))

    def test_partial_register_write_is_not_a_copy(self):
        claim, accesses, image = self.fixture(partial_register=True)
        self.assertIn('width', _cats(_findings([claim], accesses, image=image)))

    def test_overwritten_destination_is_not_a_copy(self):
        claim, accesses, image = self.fixture(overwrite_store=True)
        self.assertIn('width', _cats(_findings([claim], accesses, image=image)))


class DataAccessCategoryControls(unittest.TestCase):
    def test_a_narrow_declaration_is_caught(self):
        c = _claim(0x1000, _prim("unsigned char", 1), extent=1)
        self.assertIn("width", _cats(_findings([c], [_access(0x1000, 4)])))

    def test_the_and_mask_narrow_load_is_suppressed(self):
        """cl 5.0 reads a u16 global 4 bytes wide and masks the register; the
        2-byte STORE is what proves the field is 2 bytes (g_sfDeviceIndex)."""
        c = _claim(0x1000, _prim("unsigned short", 2), extent=2)
        acc = [_access(0x1000, 4, insn=0x1000),
               _access(0x1000, 2, rw="w", insn=0x1010)]
        self.assertNotIn("width", _cats(_findings([c], acc)))
        wide_store = [_access(0x1000, 4, insn=0x1000),
                      _access(0x1000, 4, rw="w", insn=0x1010)]
        self.assertIn("width", _cats(_findings([c], wide_store)))

    def test_a_double_moved_as_two_dwords_is_suppressed(self):
        c = _claim(0x1000, _prim("double", 8), extent=8)
        acc = [_access(0x1000, 4), _access(0x1004, 4, insn=0x1010)]
        self.assertNotIn("width", _cats(_findings([c], acc)))

    def test_a_string_op_width_is_not_a_field_width(self):
        c = _claim(0x1000, _prim("unsigned short", 2), extent=2)
        acc = [_access(0x1000, 4, mnemonic="stosd", rw="w",
                       text="rep stos DWORD PTR es:[edi],eax")]
        self.assertNotIn("width", _cats(_findings([c], acc)))

    def test_a_block_move_over_a_byte_ARRAY_is_suppressed_a_scalar_is_not(self):
        arr = _claim(0x1000, _arr(_prim("char", 1), 8, "char[8]"), extent=8)
        self.assertNotIn("width", _cats(_findings([arr], [_access(0x1000, 4)])))
        scalar = _claim(0x1000, _prim("char", 1), extent=1)
        self.assertIn("width", _cats(_findings([scalar], [_access(0x1000, 4)])))

    def test_an_x87_access_on_an_integer_field_is_caught(self):
        c = _claim(0x1000, _prim("int", 4), extent=4)
        acc = [_access(0x1000, 4, mnemonic="fld", fpu="f32")]
        self.assertIn("width", _cats(_findings([c], acc)))

    def test_an_index_into_a_single_element_array_is_an_undercount(self):
        one = _claim(0x1000, _arr(_prim("int", 4), 1, "int[1]"), extent=4)
        acc = [_access(0x1000, 4, form="indexed", scale=4,
                       text="mov eax,DWORD PTR [eax*4+0x401000]")]
        self.assertIn("undercount", _cats(_findings([one], acc)))
        many = _claim(0x1000, _arr(_prim("int", 4), 8, "int[8]"), extent=32)
        self.assertNotIn("undercount", _cats(_findings([many], acc)))

    def test_a_pair_array_declared_as_scalars_is_a_stride_finding(self):
        c = _claim(0x1000, _arr(_prim("int", 4), 32, "int[32]"), extent=128)
        acc = [_access(0x1000, 4, form="indexed", scale=8,
                       text="mov edx,DWORD PTR [edx*8+0x401000]")]
        self.assertIn("stride", _cats(_findings([c], acc)))

    def test_an_indexed_base_just_before_a_claim_is_not_unmodelled_data(self):
        """`[ecx + &g_buf - 4]` is g_buf's own 1-based spelling; the same
        address touched DIRECTLY is a real unclaimed byte."""
        c = _claim(0x1010, _arr(_prim("char", 1), 16, "char[16]"), extent=16)
        idx = [_access(0x100C, 1, form="indexed", rw="w", base_reg="ecx",
                       text="mov BYTE PTR [ecx+0x40100c],al")]
        self.assertNotIn("unclaimed", _cats(_findings([c], idx)))
        direct = [_access(0x100C, 1, rw="w", text="mov BYTE PTR ds:0x40100c,al")]
        self.assertIn("unclaimed", _cats(_findings([c], direct)))

    def test_a_source_claim_on_an_iat_slot_is_an_import(self):
        c = _claim(0x2C43C0, _prim("int", 4), name="?g_val_2c43c0@@3HA")
        acc = [_access(0x2C43C0, 4, form="iat", mnemonic="call",
                       text="call DWORD PTR ds:0x6c43c0")]
        self.assertIn("import-slot", _cats(_findings([c], acc)))
        lib = _claim(0x2C43C0, None, name="??_7CFoo@@6B@",
                     channel="data_vtables")
        self.assertNotIn("import-slot", _cats(_findings([lib], acc)))

    def test_a_claim_nothing_names_is_a_phantom_candidate(self):
        c = _claim(0x1000, _prim("int", 4))
        self.assertIn("unaccessed", _cats(_findings([c], [])))
        self.assertNotIn("unaccessed", _cats(_findings([c], [_access(0x1000)])))

    def test_one_access_crossing_two_claims_is_one_object(self):
        a = _claim(0x1000, _prim("int", 4), name="?g_a@@3HA")
        b = _claim(0x1004, _prim("int", 4), name="?g_b@@3HA")
        acc = [_access(0x1000, 8, mnemonic="fld", fpu="f64")]
        self.assertIn("adjacent", _cats(_findings([a, b], acc)))

    def test_the_gate_reports_gated_rows_and_honours_the_accept_list(self):
        """A report-only category must not break the build, a gated one must,
        and a documented exception must stay silent while its evidence holds
        (a STALE accept is itself reported)."""
        from homm1.verify import data_access as da
        fired = [("shortfall", "high", 0x1000, "?g_x@@3HA", 0x1004, "d", "e"),
                 ("undercount", "high", 0x3000, "?g_z@@3PAHA",
                  0x3000, "d", "e")]
        with mock.patch.object(da, "ACCEPTED_MISMODELS",
                               frozenset({("undercount", 0x3000)})), \
                mock.patch.object(da, "analysis",
                                  return_value=(None, None, None, None, fired,
                                                None, None)):
            out = da.gate_findings()
            self.assertEqual(len(out), 1)
            self.assertIn("shortfall", out[0])
        with mock.patch.object(da, "ACCEPTED_MISMODELS",
                               frozenset({("undercount", 0x3000)})), \
                mock.patch.object(da, "analysis",
                                  return_value=(None, None, None, None, [],
                                                None, None)):
            stale = da.gate_findings()
        self.assertTrue(any("STALE accept" in s for s in stale))

    def test_the_injected_defects_are_all_caught_on_the_real_tree(self):
        """The whole-tree harness: nine planted defects, each a class this
        campaign has shipped. A sieve returning 0 rows because it is BLIND is
        indistinguishable from a clean tree - this is what tells them apart."""
        from homm1.core.paths import BUILD
        from homm1.verify import data_access as da
        if not (BUILD / "gen/bindings.tsv").is_file():
            self.skipTest("no bindings (unbuilt tree)")
        missed = [f"{tag}->{want}" for tag, want, _label, caught
                  in da.run_selftest() if not caught]
        self.assertEqual(missed, [])


class DataCoverageControls(unittest.TestCase):
    def _row(self, **kw):
        row = {"rva": 0x1000, "length": 16, "section": ".data",
               "verdict": "NONZERO", "addressed": 1, "touched": 8, "sites": 2,
               "payload_nonzero": 8, "relocs": 0, "prev_object": "probe",
               "prev_name": "?g_a@@3HA", "next_object": "probe",
               "next_name": "?g_b@@3HA", "first_bytes": "01"}
        row.update(kw)
        return row

    def test_a_touched_nonzero_gap_inside_one_unit_fails(self):
        from homm1.verify import data_coverage as dc
        self.assertEqual(len(dc.gate_rows([self._row()])), 1)

    def test_the_library_frontier_and_the_iat_do_not(self):
        from homm1.verify import data_coverage as dc
        self.assertEqual(dc.gate_rows([self._row(prev_object="library_data",
                                                 next_object="library_data")]),
                         [])
        self.assertEqual(dc.gate_rows([self._row(prev_object="probe",
                                                 next_object="other")]), [])
        self.assertEqual(dc.gate_rows([self._row(section=".idata")]), [])
        self.assertEqual(dc.gate_rows([self._row(touched=0, sites=0)]), [])
        self.assertEqual(dc.gate_rows([self._row(verdict="ZERO-GAP",
                                                 payload_nonzero=0)]), [])

    def test_a_folded_comdat_is_not_an_overlap_but_two_extents_are(self):
        from homm1.verify import data_coverage as dc
        folded = [{"rva": 0x1000, "size": 8, "name": "??_7C@@6B@",
                   "object": "a", "storage": "rdata"},
                  {"rva": 0x1000, "size": 8, "name": "??_7C@@6B@",
                   "object": "b", "storage": "rdata"}]
        self.assertEqual(dc.overlaps(folded), [])
        clash = folded + [{"rva": 0x1004, "size": 8, "name": "?g_x@@3HA",
                           "object": "c", "storage": "data"}]
        self.assertEqual(len(dc.overlaps(clash)), 1)


# --------------------------------------------------------------------------- #
# the DATA_COMPGEN negative-control set (real base objs)                      #
# --------------------------------------------------------------------------- #
class DataCompgenControls(unittest.TestCase):
    """The 16 controls from the extraction campaign: every malformed /
    unverifiable pin is REFUSED (never silently dropped, never admitted)."""

    def _cases(self):
        from homm1.core.paths import BUILD
        from homm1.model import resolve

        def owner_obj(rva, fallback):
            unit = next((b.unit for b in resolve().data
                         if b.rva == rva and b.unit), fallback)
            return BUILD / f"objdiff/base/{unit}.obj"

        # the owning TU is derived from the Model, not hardcoded - the pins
        # legitimately migrate when a fold re-homes their use sites
        obj = owner_obj(0x1E9A40, "grunt")
        str_obj = owner_obj(0x20D168, "fortconquered")
        return obj, str_obj, [
            ("positive f64", 'x = DATA_COMPGEN(0x001e9a40, 750.0);', obj, 1),
            ("positive str", 'x = DATA_COMPGEN(0x0020d168, "!");', str_obj, 1),
            ("wrong rva (right value, other slot)",
             'x = DATA_COMPGEN(0x001e9a48, 750.0);', obj, 0),
            ("value this TU never emitted",
             'x = DATA_COMPGEN(0x001e9a40, 751.0);', obj, 0),
            ("string this TU never emitted",
             'x = DATA_COMPGEN(0x0020d168, "nowhere");', str_obj, 0),
            ("string at the wrong rva",
             'x = DATA_COMPGEN(0x0020d16a, "!");', str_obj, 0),
            ("identifier value",
             'x = DATA_COMPGEN(0x001e9a40, g_thing);', obj, 0),
            ("integer value", 'x = DATA_COMPGEN(0x001e9a40, 750);', obj, 0),
            ("wide string", 'x = DATA_COMPGEN(0x0020d168, L"!");', str_obj, 0),
            ("non-canonical address",
             'x = DATA_COMPGEN(0x1e9a40, 750.0);', obj, 0),
            ("one argument", 'x = DATA_COMPGEN(0x001e9a40);', obj, 0),
            ("same rva, two values",
             'x = DATA_COMPGEN(0x001e9a40, 750.0) '
             '+ DATA_COMPGEN(0x001e9a40, 1.0);', obj, 1),
            ("f32 spelling of an f64 slot",
             'x = DATA_COMPGEN(0x001e9a40, 750.0f);', obj, 0),
            ("two pins on one line",
             'x = DATA_COMPGEN(0x001e9a40, 750.0) '
             '/ DATA_COMPGEN(0x001e9a58, 0.01);', obj, 2),
            ("wrapped over lines",
             'x = DATA_COMPGEN(0x001e9a58,\n        0.01);', obj, 1),
            ("commented out",
             '// x = DATA_COMPGEN(0x001e9a40, 750.0);', obj, 0),
        ]

    def test_the_sixteen_controls(self):
        from homm1.retail_labels.source import blank_comments, compgen_claims
        obj, str_obj, cases = self._cases()
        if not obj.is_file() or not str_obj.is_file():
            self.skipTest("base objs absent (unbuilt tree) - the "
                          "DATA_COMPGEN controls need grunt.obj + "
                          "fortconquered.obj")
        bad = []
        for label, text, o, want in cases:
            claims, problems = compgen_claims(blank_comments(text), "ctrl", o)
            ok = len(claims) == want and (want
                                          or all("FATAL" in p
                                                 for p in problems))
            if not ok:
                bad.append(f"{label}: claims={len(claims)} want={want} "
                           f"problems={problems[:1]}")
        self.assertEqual(bad, [])


class LedgerPrecisionControls(unittest.TestCase):
    """Below-best must read the same on a live float and on its banked copy.

    The bank stores cur_pct to 4 decimals. While the regress test compared
    the raw float and the carried test the stored one, a row whose dip is
    exactly EPS was regressed AND never-carried at once - banking could not
    clear it and the build gate stayed red forever (seen live on
    StepDiggerBehavior, best 68.1947 vs cur 68.18469).
    """

    def test_raw_and_banked_readings_agree(self):
        from homm1.verify.baseline import at_ledger_precision, below_best
        best = 68.1947
        for raw in (68.18469, 68.1847, 68.18471, 68.0, 68.1947, 70.0):
            self.assertEqual(
                below_best(raw, best),
                below_best(at_ledger_precision(raw), best),
                f"{raw} reads differently once banked - unclearable gate")

    def test_a_dip_the_ledger_cannot_see_is_not_a_regression(self):
        from homm1.verify.baseline import below_best
        self.assertFalse(below_best(68.18469, 68.1947))   # rounds to the edge
        self.assertTrue(below_best(68.1840, 68.1947))     # genuinely below

    def test_a_real_dip_is_still_fresh_after_banking(self):
        from homm1.verify.classify import currency
        base = {("u", "f"): {"best": 90.0, "cur": 90.0, "fp": "h", "tries": 1}}
        got = currency({("u", "f"): 80.0}, base, [("u", "f", 80.0, 90.0)])
        self.assertEqual((got["regress_fresh"], got["regress_carried"]), (1, 0))

    def test_a_banked_dip_is_carried(self):
        from homm1.verify.classify import currency
        base = {("u", "f"): {"best": 90.0, "cur": 80.0, "fp": "h", "tries": 1}}
        got = currency({("u", "f"): 80.0}, base, [("u", "f", 80.0, 90.0)])
        self.assertEqual((got["regress_fresh"], got["regress_carried"]), (0, 1))


# --------------------------------------------------------------------------- #
# investigation surfaces (sema / walls / lsp / ghidra): a navigator that
# answers CONFIDENTLY and WRONGLY is worse than one that fails, so each of
# these is the control for a defect that shipped.
# --------------------------------------------------------------------------- #
class SemaReportSourceControls(unittest.TestCase):
    """sema read build/objdiff/report.json - the BANKED reference copy, not
    the report `homm1 compare` writes. `sema match 0x153810` answered 99.57%
    while walls/verify read 99.89% off the same build."""

    def test_sema_reads_the_same_reports_in_the_same_order_as_verify(self):
        from homm1.sema.report import REPORTS as sema_reports
        from homm1.verify.scores import REPORTS as verify_reports
        self.assertEqual(tuple(sema_reports), tuple(verify_reports))

    def test_the_current_report_wins_over_the_banked_one(self):
        from homm1.sema import report as rep
        with tempfile.TemporaryDirectory() as td:
            new, old = Path(td) / "new.json", Path(td) / "old.json"
            old.write_text('{"units": [], "measures": {}}')
            with mock.patch.object(rep, "REPORTS", (new, old)):
                self.assertEqual(rep.report_path(), old)   # only the bank
                new.write_text('{"units": [], "measures": {}}')
                self.assertEqual(rep.report_path(), new)   # current wins


class SemaPipeControls(unittest.TestCase):
    """`homm1 sema vtable --list | head` exited 120 with 'Exception ignored
    on flushing sys.stdout' - the BrokenPipeError fires at interpreter exit,
    after run()'s handler could ever see it."""

    def test_a_closed_reader_is_a_clean_exit(self):
        import os

        from homm1 import sema
        r, w = os.pipe()
        os.close(r)                                  # the reader is gone
        saved = os.dup(1)
        try:
            os.dup2(w, 1)
            os.close(w)
            mod = mock.Mock()
            mod.main.side_effect = lambda argv: (print("x" * 200_000), 0)[1]
            with mock.patch("importlib.import_module", return_value=mod):
                rc = sema.run("homm1.sema.rva", [])
        finally:
            os.dup2(saved, 1)
            os.close(saved)
        self.assertEqual(rc, 0)


class SemaMapControls(unittest.TestCase):
    """`sema map at <unmapped>` printed 'no admitted row covers this address'
    and exited 0, against sema's own rc convention (1 = answered-NO)."""

    def test_an_uncovered_address_is_answered_no(self):
        from homm1.sema import map as smap
        idx = mock.Mock()
        idx.covering.return_value = None
        idx.preceding_func.return_value = None
        with mock.patch.object(smap, "index", return_value=idx), \
             mock.patch("homm1.sema.image.retail") as retail:
            retail.return_value.section_of.return_value = None
            lines, rc = smap.at(0xDEADBEEF)
        self.assertEqual(rc, 1)
        self.assertIn("outside every section", "\n".join(lines))


class SemaXrefControls(unittest.TestCase):
    """A relocated vtable/callback reference to a linker-thunk entry must
    keep the final body live.  The old tree followed only rel32 edges after
    entering the thunk and confidently printed ``no caller``."""

    def test_a_reference_to_a_thunk_entry_reaches_the_forwarded_body(self):
        from homm1.sema import xref

        idx = mock.Mock()
        thunk = mock.Mock(rva=0x2000, size=0x10, kind="thunk")
        idx.owner.side_effect = lambda site: thunk if site == 0x2005 else None

        img = mock.Mock()
        img.call_index = {0x1000: [(0x2005, 0xE9)]}
        img.jmp_target.side_effect = lambda site: 0x1000 if site == 0x2005 else None
        img.refs_to_range.side_effect = lambda lo, hi: (
            [(0x3000, 0x2005)] if (lo, hi) == (0x2005, 0x2006) else [])

        with mock.patch.object(xref, "index", return_value=idx), \
             mock.patch.object(xref, "retail", return_value=img), \
             mock.patch.object(xref, "site_where", return_value="in vtable"):
            lines = xref.caller_tree(0x1000)
            self.assertTrue(xref.is_effectively_reached(0x1000, 0x20))

        text = "\n".join(lines)
        self.assertIn("<- ref", text)
        self.assertIn("via 1 thunk", text)

    def test_an_unreferenced_thunk_does_not_make_the_body_live(self):
        from homm1.sema import xref

        idx = mock.Mock()
        thunk = mock.Mock(rva=0x2000, size=0x10, kind="thunk")
        idx.owner.return_value = thunk
        img = mock.Mock()
        img.call_index = {0x1000: [(0x2005, 0xE9)]}
        img.jmp_target.return_value = 0x1000
        img.refs_to_range.return_value = []

        with mock.patch.object(xref, "index", return_value=idx), \
             mock.patch.object(xref, "retail", return_value=img):
            self.assertFalse(xref.is_effectively_reached(0x1000, 0x20))


class DeadCodeControls(unittest.TestCase):
    def test_missing_and_stale_markers_both_fail(self):
        from homm1.verify import dead_code

        sites = {0x1000: ("a.cpp", 4), 0x2000: ("b.cpp", 8)}
        marked = {0x2000: [("b.cpp", 6)]}
        findings = dead_code.compare(marked, sites, {0x1000, 0x2000}, {0x1000})
        self.assertTrue(any("missing" in f for f in findings))
        self.assertTrue(any("stale" in f for f in findings))

    def test_a_proven_marker_is_clean_and_requires_its_proof_line(self):
        from homm1.verify import dead_code

        with tempfile.TemporaryDirectory() as td:
            path = Path(td) / "probe.cpp"
            path.write_text("// @dead-code\n"
                            "// Zero-ref: no retail reachability.\n"
                            "RVA(0x00001000, 0x1)\n"
                            "void Probe() {}\n")
            marked, sites, problems = dead_code.source_markers([path])
            self.assertEqual(problems, [])
            self.assertEqual(dead_code.compare(marked, sites, {0x1000},
                                               {0x1000}), [])
            path.write_text("// @dead-code\n"
                            "RVA(0x00001000, 0x1)\n"
                            "void Probe() {}\n")
            _marked, _sites, problems = dead_code.source_markers([path])
            self.assertTrue(any("proof" in f for f in problems))


class SemaGapControls(unittest.TestCase):
    """The same-file gap view must trim only edge padding and keep executable
    categories separate; otherwise the derived reconstruction queue can lose a
    tiny body or bury one under the repeated compiler/runtime bands."""

    def test_edge_padding_is_trimmed_without_eating_body_bytes(self):
        from homm1.sema import gaps
        rva, body = gaps._trim(0x1000, b"\x90\xcc\xc3\x90")
        self.assertEqual((rva, body), (0x1002, b"\xc3"))

    def test_gap_kinds_preserve_tiny_and_substantive_bodies(self):
        from homm1.sema import gaps
        with mock.patch.object(gaps, "_switch_table", return_value=False):
            self.assertEqual(gaps._kind(b"\xe9\x00\x00\x00\x00", None), "thunk")
            self.assertEqual(gaps._kind(b"\x33\xc0\xc3", None), "trivial")
            self.assertEqual(gaps._kind(b"\x55" * 9, None), "substantive")
            self.assertEqual(gaps._kind(b"\x55" * 0x100, None), "band")

    def test_aligned_padding_separates_multiple_missing_functions(self):
        from homm1.sema import gaps
        payload = b"\xc3" + b"\x90" * 15 + b"\xcc" * 16 + b"\xc2\x04\x00\x90"
        self.assertEqual(
            gaps._split(0x17400, payload),
            [(0x17400, b"\xc3"), (0x17420, b"\xc2\x04\x00")],
        )

    def test_xc_table_distinguishes_initializer_thunks_from_bodies(self):
        from homm1.sema import gaps

        class FakePe:
            image_base = 0x400000

            def read(self, rva, size):
                if rva == gaps.XC_START:
                    slots = [0x410000, 0x420000]
                    return struct.pack("<2I", *slots) + bytes(size - 8)
                if rva == 0x10000:
                    return b"\xe9\xfb\x00\x00\x00"
                if rva == 0x20000:
                    return b"\xc3\x90\x90\x90\x90"
                return None

        self.assertEqual(
            gaps._dyninit_roles(FakePe()),
            {0x10000: "dyninit-thunk", 0x10100: "dyninit-body", 0x20000: "dyninit-body"},
        )

    def test_initializer_owner_follows_relocated_data_not_neighbours(self):
        from homm1.sema import gaps
        img = mock.Mock()
        img.relocs_in.return_value = [(0x17D86, 0x229E18), (0x17D90, 0x229E1C)]
        idx = mock.Mock()
        idx.data_owner.side_effect = [mock.Mock(unit="customleveldlg"),
                                      mock.Mock(unit="customleveldlg")]
        self.assertEqual(
            gaps._dyninit_owner(0x17D80, 0x1A, img, idx),
            "customleveldlg",
        )

    def test_existing_non_source_claim_is_not_reported_as_a_gap(self):
        from homm1.sema import gaps
        binding = mock.Mock(rva=0x21280, size=0x10, channel="functions_static_libs")
        self.assertTrue(gaps._covered(0x21280, 0x10, [binding]))
        self.assertFalse(gaps._covered(0x21260, 0x8, [binding]))


class WallsUnitFilterControls(unittest.TestCase):
    """A misspelt --unit answered '0 mismatches' / '0 function(s) below 100%'
    - a typo that reads as a clean sieve."""

    def test_an_unknown_unit_is_refused(self):
        from homm1 import walls
        with mock.patch("homm1.manifest.units",
                        return_value=[{"unit": "cimage"}]):
            with self.assertRaises(SystemExit) as cm:
                walls.check_unit("cimag")
            self.assertEqual(cm.exception.code, 2)
            self.assertEqual(walls.check_unit("cimage"), "cimage")
            self.assertIsNone(walls.check_unit(None))


class WallsEmptyBuildControls(unittest.TestCase):
    """With no normalized pair on disk the sieves printed 'aggregate-copy
    mismatches: 0' / 'paired functions: 0' and exited 0 - an unbuilt tree
    reading as a clean sieve."""

    def test_no_pair_is_refused_not_answered(self):
        from homm1.walls import pairscan
        with tempfile.TemporaryDirectory() as td:
            with mock.patch.object(pairscan, "NORM", Path(td)):
                with self.assertRaises(SystemExit) as cm:
                    pairscan.require_pairs()
                self.assertEqual(cm.exception.code, 2)

    def test_a_real_pair_passes_through(self):
        from homm1.walls import pairscan
        with tempfile.TemporaryDirectory() as td:
            root = Path(td)
            (root / "base").mkdir()
            (root / "target").mkdir()
            (root / "base/cimage.obj").write_bytes(b"")
            (root / "target/cimage.c.obj").write_bytes(b"")
            with mock.patch.object(pairscan, "NORM", root):
                self.assertEqual(list(pairscan.require_pairs()), ["cimage"])


class EhFrameTsvControls(unittest.TestCase):
    """`--calibrate` returned before the `--tsv` writer, so asking for both
    silently produced no file."""

    ROW = dict(unit="u", name="?f@@YAXXZ", rva="0x001000", fuzzy=100.0,
               size=16, verdict="BOTH", cause="EXTRA_OBJECT", extra_ctors=[],
               our_ctors=[], resited=[], base_insn=4, tgt_insn=4,
               slot="[esp+0x4]", states=[0], base_states=1, tgt_states=1,
               first=0, last=4, unwind=True)

    def test_calibrate_still_writes_the_tsv(self):
        import contextlib
        import io

        from homm1.walls import eh_frame
        with tempfile.TemporaryDirectory() as td:
            out = Path(td) / "eh.tsv"
            with mock.patch.object(eh_frame, "scan", return_value=[self.ROW]), \
                 mock.patch.object(eh_frame.pairscan, "require_pairs"), \
                 contextlib.redirect_stdout(io.StringIO()):
                eh_frame.main(["--calibrate", "--tsv", str(out)])
            self.assertTrue(out.is_file(), "--tsv was swallowed by --calibrate")
            self.assertIn("verdict", out.read_text().splitlines()[0])


class WallsDiagnoseTargetControls(unittest.TestCase):
    """diagnose took only the rva or the exact mangled name, and answered
    every miss with the same 'no claimed function for X'."""

    def _model(self, *names):
        fns = [_binding(0x153810 + i * 0x10, n) for i, n in enumerate(names)]
        return mock.Mock(functions=fns)

    def test_a_readable_spelling_resolves(self):
        from homm1.walls import diagnose as D
        name = "?RenderFrameClipped@CImage@@QAEXH@Z"
        with mock.patch("homm1.model.resolve", return_value=self._model(name)):
            b, why = D._locate("CImage::RenderFrameClipped")
            self.assertIsNotNone(b, why)
            self.assertEqual(b.name, name)

    def test_a_miss_names_the_accepted_spellings(self):
        from homm1.walls import diagnose as D
        with mock.patch("homm1.model.resolve", return_value=self._model()):
            b, why = D._locate("NoSuchThing")
            self.assertIsNone(b)
            self.assertIn("CClass::Member", why)
            b, why = D._locate("0xdeadbeef")
            self.assertIsNone(b)
            self.assertIn("sema rva", why)

    def test_a_relocation_free_call_to_function_start_is_self_recursion(self):
        from homm1.walls import diagnose as D
        asm = "   2ba:\te8 00 00 00 00\tcall 0x0\n"
        self.assertEqual(D._call_targets({}, asm, "?f@@YAXXZ"), [("?f@@YAXXZ", 0)])

    def test_a_relocated_call_is_not_counted_twice_as_self_recursion(self):
        from homm1.walls import diagnose as D
        asm = "   2ba:\te8 00 00 00 00\tcall 0x0\n"
        rel = {0x2bb: ("?f@@YAXXZ", 0)}
        self.assertEqual(D._call_targets(rel, asm, "?f@@YAXXZ"), [("?f@@YAXXZ", 0)])

    def test_diagnose_does_not_route_relocation_free_self_recursion_to_inline(self):
        import contextlib
        import io
        from types import SimpleNamespace

        from homm1.walls import diagnose as D
        name = "?ImportDirectoryTree@CRezArchive@@QAEHXZ"
        binding = SimpleNamespace(unit="u", name=name, rva=0x13B300)
        asm = "   0:\te8 fb ff ff ff\tcall 0x0\n"
        with tempfile.TemporaryDirectory() as td:
            norm = Path(td)
            (norm / "base").mkdir()
            (norm / "target").mkdir()
            (norm / "base/u.obj").touch()
            (norm / "target/u.c.obj").touch()
            found = [
                (b"base", {1: (name, 0)}, 4),
                (b"target", {}, 4),
            ]
            skeletons = [
                (b"base", 1, 0, 0, 1, asm),
                (b"target", 1, 0, 0, 1, asm),
            ]
            with mock.patch.object(D, "NORM", norm), \
                 mock.patch.object(D, "_locate", return_value=(binding, "")), \
                 mock.patch.object(D, "Obj", side_effect=lambda path: path), \
                 mock.patch.object(D, "_find_function", side_effect=found), \
                 mock.patch.object(D, "_jump_table_bytes", return_value=set()), \
                 mock.patch.object(D, "_skeleton", side_effect=skeletons), \
                 contextlib.redirect_stdout(io.StringIO()) as out:
                self.assertEqual(D.diagnose("0x13b300"), 0)
        self.assertIn("REGALLOC/SCHEDULING", out.getvalue())
        self.assertNotIn("INLINE/CALL-SET", out.getvalue())

    def test_diagnose_names_a_repeated_call_site_delta(self):
        import contextlib
        import io
        from types import SimpleNamespace

        from homm1.walls import diagnose as D
        name = "?StepArrivalDrop@CGrunt@@QAEHHHHHHH@Z"
        callee = "?RemoveHead@CPtrList@@QAEPAXXZ"
        binding = SimpleNamespace(unit="u", name=name, rva=0x4B370)
        run = [f"{i:x}:\t90\tmov eax,0x{i:x}" for i in range(1, 11)]
        base_asm = "\n".join(run + ["b:\t74 00\tje 0xd"]
                             + run + ["17:\t75 00\tjne 0x19"])
        skeletons = [
            (b"base", 3, 0, 0, 1, base_asm),
            (b"target", 4, 0, 0, 1, ""),
        ]
        with tempfile.TemporaryDirectory() as td:
            norm = Path(td)
            (norm / "base").mkdir()
            (norm / "target").mkdir()
            (norm / "base/u.obj").touch()
            (norm / "target/u.c.obj").touch()
            with mock.patch.object(D, "NORM", norm), \
                 mock.patch.object(D, "_locate", return_value=(binding, "")), \
                 mock.patch.object(D, "Obj", side_effect=lambda path: path), \
                 mock.patch.object(D, "_find_function", side_effect=[
                     (b"base", {}, 4), (b"target", {}, 4)]), \
                 mock.patch.object(D, "_jump_table_bytes", return_value=set()), \
                 mock.patch.object(D, "_skeleton", side_effect=skeletons), \
                 mock.patch.object(D, "_call_targets", side_effect=[
                     [(callee, 0)] * 3, [(callee, 0)] * 4]), \
                 contextlib.redirect_stdout(io.StringIO()) as out:
                self.assertEqual(D.diagnose("0x4b370"), 0)
        text = out.getvalue()
        self.assertIn("INLINE/CALL-SET", text)
        self.assertIn("REPEATED-SITE DELTA: target 4, base 3", text)
        self.assertIn(callee, text)
        self.assertIn("site-positioned inline-budget residue", text)
        self.assertNotIn("is a CFG reconstruction question", text)


class InlineModelFlagControls(unittest.TestCase):
    """--spec was documented, parsed, and then fell through to
    `error: need --selftest, --spec/--gap FILE, or --measure-cb TU`."""

    def test_spec_predicts_instead_of_erroring(self):
        import contextlib
        import io

        from homm1.walls import inline_model
        with tempfile.TemporaryDirectory() as td:
            spec = Path(td) / "s.json"
            spec.write_text('{"caller_cb": 120, "sites": ['
                            + ",".join(['{"name": "fill", "cb": 150}'] * 9)
                            + "]}")
            with contextlib.redirect_stdout(io.StringIO()) as out:
                rc = inline_model.main(["--spec", str(spec)])
        text = out.getvalue()
        self.assertEqual(rc, 0)
        self.assertEqual(text.count("EXPAND fill"), 6)     # the oracle shape
        self.assertEqual(text.count("call   fill"), 3)

    def test_a_malformed_spec_is_explained(self):
        from homm1.walls import inline_model
        with tempfile.TemporaryDirectory() as td:
            bad = Path(td) / "b.json"
            bad.write_text("{}")
            with self.assertRaises(SystemExit):
                inline_model.main(["--spec", str(bad)])
            with self.assertRaises(SystemExit):
                inline_model.main(["--gap", str(Path(td) / "absent.json")])

    def test_unmarked_template_candidate_reaches_cli_prediction(self):
        import contextlib
        import io
        import json
        from homm1.walls import inline_model
        with tempfile.TemporaryDirectory() as td:
            spec = Path(td) / "template.json"
            spec.write_text(json.dumps({"caller_cb": 120, "sites": [
                {"name": "Array<int>::operator[]", "cb": 20,
                 "marked": False, "candidate": True},
                {"name": "Plain::At", "cb": 20,
                 "marked": False, "candidate": False},
            ]}))
            with contextlib.redirect_stdout(io.StringIO()) as out:
                rc = inline_model.main(["--spec", str(spec)])
        self.assertEqual(rc, 0)
        self.assertIn("EXPAND Array<int>::operator[]", out.getvalue())
        self.assertIn("call   Plain::At", out.getvalue())


class ExeMapWriteControls(unittest.TestCase):
    """`python3 -m homm1.sema.exe_map --help` ignored the flag and rewrote
    the tracked docs tree. Generated maps now default to build/exe-map/."""

    def test_help_does_not_write(self):
        import contextlib
        import io

        from homm1.sema import exe_map
        with contextlib.redirect_stdout(io.StringIO()):
            with self.assertRaises(SystemExit) as cm:
                exe_map.main(["--help"])
        self.assertEqual(cm.exception.code, 0)

    def test_check_writes_nothing(self):
        import contextlib
        import io

        from homm1.sema import exe_map
        with tempfile.TemporaryDirectory() as td:
            with mock.patch.object(exe_map, "core_rows",
                                   return_value=[{"unit": "u", "name": "n",
                                                  "rva": 0x1000, "size": 4}]), \
                 mock.patch.object(exe_map, "unit_sources", return_value={}), \
                 mock.patch.object(exe_map, "OUT_DIR", Path(td) / "out"), \
                 contextlib.redirect_stdout(io.StringIO()):
                exe_map.main(["--check"])
            self.assertFalse((Path(td) / "out").exists())


class ToolDriverMessageControls(unittest.TestCase):
    """A missing file / unparsable address reached the user as a traceback."""

    def test_objdump_refuses_a_missing_blob_and_a_bad_vma(self):
        from homm1.tool import objdump
        with tempfile.TemporaryDirectory() as td:
            blob = Path(td) / "b.bin"
            blob.write_bytes(b"\x90\xc3")
            self.assertEqual(objdump.main([str(Path(td) / "absent.bin")]), 2)
            self.assertEqual(objdump.main([str(blob), "--vma", "zz"]), 2)

    def test_ghidra_verify_refuses_a_non_address(self):
        from homm1.ghidra import project
        self.assertEqual(project.main(["verify", "zzz"]), 2)

    def test_lsp_point_names_the_missing_file(self):
        from homm1.lsp.query import parse_point
        with self.assertRaises(SystemExit) as cm:
            parse_point("include/NoSuchHeader.h:12")
        self.assertIn("no such file", str(cm.exception))
        self.assertIsNone(parse_point("CGrunt::GetAI"))   # still a symbol


# --------------------------------------------------------------------------- #
# the runner and the CLI surface (2026-08-16 review)                          #
# --------------------------------------------------------------------------- #
class TierRunnerExitControls(unittest.TestCase):
    """SystemExit is a BaseException: a gate reporting a missing input by
    raising it used to abort the whole tier, silently skipping every gate
    after it - the run looked short, not failed."""

    def _run(self, gates):
        import contextlib
        import io

        from homm1.verify import tiers
        with mock.patch.dict(tiers.TIERS, {"fast": gates}):
            with contextlib.redirect_stdout(io.StringIO()) as out:
                failed = tiers.run(["fast"])
        return failed, out.getvalue()

    def test_a_gate_that_raises_systemexit_is_a_failure_not_an_abort(self):
        def bail():
            raise SystemExit("no report.json - run `homm1 compare` first")
        seen = []
        failed, text = self._run([("probe", bail),
                                  ("after", lambda: seen.append(1) or [])])
        self.assertEqual(failed, 1)
        self.assertIn("could not run", text)
        self.assertIn("no report.json", text)
        self.assertEqual(seen, [1])          # the NEXT gate still ran

    def test_the_rerun_hint_names_a_command_that_exists(self):
        from homm1.verify import _ALIASES, _GATES, tiers
        for tier, rows in tiers.TIERS.items():
            for name, _fn in rows:
                hint = tiers._rerun_command(name)
                gate = hint.rsplit(" ", 1)[-1]
                self.assertIn(gate, _GATES,
                              f"[{tier}] {name}: hint {hint!r} names no gate")
        # the row this got wrong: the `vtable-bans` label runs verify.BANS, so
        # the mechanical name.replace('-','_') named a module that does not
        # exist (`python3 -m homm1.verify.vtable_bans`).
        self.assertEqual(_ALIASES["vtable-bans"], "bans")
        self.assertEqual(tiers._rerun_command("vtable-bans"),
                         "homm1 verify bans")

    def test_every_tier_label_is_runnable_as_a_verb(self):
        """A tier label nobody can type is a dead end at the exact moment the
        gate fails."""
        from homm1.verify import (_ALIASES, _GATES, _QUERY_ONLY, _STANDALONE,
                                   tiers)
        for tier, rows in tiers.TIERS.items():
            for name, _fn in rows:
                verb = _ALIASES.get(name, name)
                self.assertIn(verb, _GATES,
                              f"[{tier}] {name} runs in a tier but "
                              f"`homm1 verify {name}` reaches nothing")
        # and the help listing's own claim: every non-oracle gate IS tier-run
        labels = {n for rows in tiers.TIERS.values() for n, _f in rows}
        in_a_tier = {_ALIASES.get(n, n) for n in labels}
        for gate in _GATES:
            if gate in _QUERY_ONLY or gate in _STANDALONE:
                continue
            self.assertIn(gate, in_a_tier,
                          f"{gate} is advertised as tier-run but no tier "
                          f"lists it (add it to a tier, or declare it in "
                          f"_QUERY_ONLY or _STANDALONE)")

    def test_the_alias_verb_actually_dispatches(self):
        import contextlib
        import io

        from homm1 import verify
        with mock.patch("homm1.verify.bans.main", return_value=0) as m:
            with contextlib.redirect_stdout(io.StringIO()):
                self.assertEqual(verify.main(["vtable-bans"]), 0)
        m.assert_called_once()


class ReportInputControls(unittest.TestCase):
    """A bad --report is an operator error, not a traceback."""

    def test_missing_malformed_and_foreign_json_all_say_what_to_do(self):
        from homm1.verify import scores
        with tempfile.TemporaryDirectory() as td:
            gone = Path(td) / "gone.json"
            with self.assertRaises(SystemExit) as e:
                scores.load(gone)
            self.assertIn("gone.json", str(e.exception))

            trunc = Path(td) / "trunc.json"
            trunc.write_text('{"units": [')
            with self.assertRaises(SystemExit) as e:
                scores.load(trunc)
            self.assertIn("not valid JSON", str(e.exception))
            self.assertIn("homm1 compare", str(e.exception))

            foreign = Path(td) / "other.json"
            foreign.write_text('{"hello": 1}')
            with self.assertRaises(SystemExit) as e:
                scores.load(foreign)
            self.assertIn("not an objdiff report", str(e.exception))

    def test_an_unknown_tier_is_rejected_before_any_work(self):
        """`check --tier bogus` used to run the whole MAX gate first, so the
        typo surfaced behind a wall of regression output."""
        from homm1.verify import verbs
        with mock.patch.object(verbs, "_report",
                               side_effect=AssertionError("ran the gate")):
            with self.assertRaises(SystemExit) as e:
                verbs.cmd_check(["--tier", "bogus"])
        self.assertIn("unknown tier", str(e.exception))

    def test_a_real_report_still_loads(self):
        from homm1.verify import scores
        with tempfile.TemporaryDirectory() as td:
            p = Path(td) / "r.json"
            p.write_text('{"units": [], "measures": {}}')
            self.assertEqual(scores.load(p)["units"], [])


class GateCliSurfaceControls(unittest.TestCase):
    """Every `homm1 verify <gate>` answers --help and REJECTS a typo; none
    of them treats an unknown flag as 'run anyway'."""

    def test_every_gate_module_parses_its_argv(self):
        import contextlib
        import importlib
        import io

        from homm1.verify import _GATES
        for gate, module in sorted(_GATES.items()):
            mod = importlib.import_module(module)
            with contextlib.redirect_stdout(io.StringIO()) as out:
                with self.assertRaises(SystemExit) as e:
                    mod.main(["--help"])
            self.assertEqual(e.exception.code, 0, f"{gate} --help")
            # RENDERING it is the check: argparse %-expands help strings, so
            # an unescaped `%` in one (`100%-clean`) raises inside --help.
            self.assertIn("usage:", out.getvalue(), f"{gate} --help")
            self.assertIn("options:", out.getvalue(), f"{gate} --help")
            with contextlib.redirect_stderr(io.StringIO()):
                with self.assertRaises(SystemExit) as e:
                    mod.main(["--definitely-not-a-flag"])
            self.assertEqual(e.exception.code, 2, f"{gate} unknown flag")

    def test_no_gate_flag_is_a_silent_no_op(self):
        """A flag argparse accepts and nothing ever READS is worse than no
        flag: it looks like it worked. (`verify vtables --list` was one.)"""
        import ast
        import importlib
        import re

        from homm1.verify import _GATES

        def unread(src):
            """dests argparse defines that no `<ns>.<dest>` ever reads."""
            tree = ast.parse(src)
            dests, namespaces = [], set()
            for node in ast.walk(tree):
                if isinstance(node, ast.Call) \
                        and isinstance(node.func, ast.Attribute) \
                        and node.func.attr == "add_argument":
                    opts = [a.value for a in node.args
                            if isinstance(a, ast.Constant)]
                    dest = next((kw.value.value for kw in node.keywords
                                 if kw.arg == "dest"
                                 and isinstance(kw.value, ast.Constant)), None)
                    if dest is None and opts:
                        longs = [o for o in opts if o.startswith("--")] or opts
                        dest = longs[0].lstrip("-").replace("-", "_")
                    if dest and dest != "help":
                        dests.append(dest)
                if isinstance(node, ast.Assign) \
                        and isinstance(node.value, ast.Call) \
                        and isinstance(node.value.func, ast.Attribute) \
                        and node.value.func.attr == "parse_args":
                    namespaces |= {t.id for t in node.targets
                                   if isinstance(t, ast.Name)}
            # `args` covers the do_*(args) handlers data_access hands off to
            names = namespaces | {"args"}
            return dests, [d for d in dests
                           if not any(re.search(rf"\b{n}\.{d}\b", src)
                                      for n in names)]

        # the detector must actually SEE flags, or an empty `dead` proves
        # nothing (this scan found `vtables --list` before it was removed)
        probe = ('import argparse\nap = argparse.ArgumentParser()\n'
                 'ap.add_argument("--used", action="store_true")\n'
                 'ap.add_argument("--dead", action="store_true")\n'
                 'a = ap.parse_args()\nprint(a.used)\n')
        self.assertEqual(unread(probe), (["used", "dead"], ["dead"]))

        seen, dead = 0, []
        for gate, module in sorted(_GATES.items()):
            src = Path(importlib.import_module(module).__file__).read_text()
            dests, bad = unread(src)
            seen += len(dests)
            dead += [f"{gate} --{d.replace('_', '-')}" for d in bad]
        self.assertGreater(seen, 30, "the flag scan found almost nothing")
        self.assertEqual(dead, [])

    def test_every_gate_flag_documents_itself(self):
        """`--help` listing a bare flag name tells the reader nothing; the
        one place a flag's meaning is guaranteed to be found is `help=`."""
        import ast
        import importlib

        from homm1.verify import _GATES
        bare = []
        for gate, module in sorted(_GATES.items()):
            src = Path(importlib.import_module(module).__file__).read_text()
            for node in ast.walk(ast.parse(src)):
                if isinstance(node, ast.Call) \
                        and isinstance(node.func, ast.Attribute) \
                        and node.func.attr == "add_argument" \
                        and not any(kw.arg == "help" for kw in node.keywords):
                    opts = [a.value for a in node.args
                            if isinstance(a, ast.Constant)]
                    bare.append(f"{gate} {'/'.join(opts)}")
        self.assertEqual(bare, [])

    def test_model_help_does_not_run_the_join(self):
        """`homm1 model --help` used to resolve the whole Model and REWRITE
        build/gen/bindings.tsv as a side effect of asking for help."""
        import contextlib
        import io

        import homm1.model as model
        with mock.patch.object(model, "resolve") as res, \
             mock.patch.object(model, "serialize") as ser:
            with contextlib.redirect_stdout(io.StringIO()):
                with self.assertRaises(SystemExit) as e:
                    model.main(["--help"])
        self.assertEqual(e.exception.code, 0)
        res.assert_not_called()
        ser.assert_not_called()


# --------------------------------------------------------------------------- #
# never vacuous: a gate that measured NOTHING may not report OK               #
# --------------------------------------------------------------------------- #
class VacuityControls(unittest.TestCase):
    def test_tu_order_refuses_an_empty_scan(self):
        from homm1.verify import tu_order as to
        with mock.patch.object(to, "load_in_file_order", return_value={}), \
             mock.patch.object(to, "load_exiles", return_value={}), \
             mock.patch.object(to, "load_emitted_claims", return_value={}), \
             mock.patch.object(to, "_load_baseline", return_value=({}, 0)):
            findings, _s = to.gate_findings()
        self.assertTrue(any("vacuous" in f for f in findings))

    def test_data_tu_order_refuses_zero_defs(self):
        from homm1.verify import data_tu_order as dto
        with mock.patch.object(dto, "crossings", return_value=(set(), 0, [])), \
             mock.patch.object(dto, "load_baseline", return_value=set()), \
             mock.patch.object(dto, "_src_declares_data", return_value=True):
            self.assertTrue(any("vacuous" in f for f in dto.gate_findings()))
        # No DATA() label anywhere in src/: nothing to order, not vacuous.
        with mock.patch.object(dto, "crossings", return_value=(set(), 0, [])), \
             mock.patch.object(dto, "load_baseline", return_value=set()), \
             mock.patch.object(dto, "_src_declares_data", return_value=False):
            self.assertEqual(dto.gate_findings(), [])
        with mock.patch.object(dto, "crossings", return_value=(set(), 42, [])), \
             mock.patch.object(dto, "load_baseline", return_value=set()):
            self.assertEqual(dto.gate_findings(), [])

    def test_caller_callee_refuses_an_empty_call_graph(self):
        from homm1.verify import caller_callee as cc
        rc = mock.Mock()
        rc.tgt = set()
        with mock.patch.object(cc, "_summary", return_value=(rc, [], {})):
            self.assertTrue(any("vacuous" in f for f in cc.gate_findings()))

    def test_caller_callee_follows_inline_forwarding_members(self):
        from homm1.verify.caller_callee import _resolve_source_calls
        wrapper = "?ApplyName@CWapX@@QAEXPBD@Z"
        real = "?ApplyName@CWwdGameObjectA@@QAEXPBD@Z"
        rvas, leaves = _resolve_source_calls(
            wrapper, {wrapper: {real}}, {real: 0x150540})
        self.assertEqual(rvas, {0x150540})
        self.assertEqual(leaves, set())

    def test_caller_callee_keeps_unresolved_forwarding_leaves(self):
        from homm1.verify.caller_callee import _resolve_source_calls
        wrapper = "?Run@CRealInline@@QAEXXZ"
        phantom = "?Run@CPhantomView@@QAEXXZ"
        rvas, leaves = _resolve_source_calls(
            wrapper, {wrapper: {phantom}}, {})
        self.assertEqual(rvas, set())
        self.assertEqual(leaves, {phantom})

    def test_caller_callee_consumer_reconciles_an_inline_forwarder(self):
        from homm1.verify import caller_callee as cc
        caller = "?Ctor@COwner@@QAEXXZ"
        wrapper = "?ApplyName@CWapX@@QAEXPBD@Z"
        real = "?ApplyName@CWwdGameObjectA@@QAEXPBD@Z"
        ir = {caller: {wrapper}, wrapper: {real}, real: set()}
        rc = cc.Recon.__new__(cc.Recon)
        rc.m2rva = {caller: 0x1000, real: 0x150540}
        with mock.patch("homm1.tool.clang.compdb",
                        return_value={"/probe/src/Probe.cpp": []}), \
             mock.patch.object(cc, "_tu_edges",
                               return_value=("/probe/src/Probe.cpp", ir)):
            edges, defined, unresolved, failed = rc._base_graph(jobs=1)
        self.assertEqual(edges, {(0x1000, 0x150540)})
        self.assertEqual(defined, {0x1000, 0x150540})
        self.assertEqual(unresolved, {})
        self.assertEqual(failed, [])

    def test_assert_relocs_refuses_zero_audited_functions(self):
        from homm1.verify import assert_relocs as ar
        with mock.patch.object(ar, "audit", return_value=([], 0)):
            self.assertTrue(any("NOTHING was audited" in f
                                for f in ar.gate_findings()))
        with mock.patch.object(ar, "audit", return_value=([], 1200)):
            self.assertEqual(ar.gate_findings(), [])

    def test_data_relocs_refuses_zero_scanned_pairs(self):
        from collections import Counter

        from homm1.verify import data_relocs as dr
        with mock.patch.object(dr, "scan",
                               return_value=([], [], [], Counter(), {}, [])), \
             mock.patch.object(dr, "units_without_a_target", return_value=[]), \
             mock.patch.object(dr, "orphan_payloads", return_value=[]):
            self.assertTrue(any("0 base/target pairs" in f
                                for f in dr.gate_findings()))

    def test_alloc_size_refuses_an_empty_layout_harvest(self):
        from homm1.verify import alloc_size as az
        sw = mock.Mock()
        sw.rows.return_value = []
        with mock.patch.object(az, "Sweep", return_value=sw), \
             mock.patch.object(az, "computed_sizes", return_value=({}, set())), \
             mock.patch.object(az, "def_counts", return_value={}), \
             mock.patch.object(az, "classify_rows",
                               return_value=([], [], [], [], [], [])):
            self.assertTrue(any("0 class sizes" in f
                                for f in az.gate_findings()))
        with mock.patch.object(az, "Sweep", return_value=sw), \
             mock.patch.object(az, "computed_sizes",
                               return_value=({"CFoo": 8}, set())), \
             mock.patch.object(az, "def_counts", return_value={}), \
             mock.patch.object(az, "classify_rows",
                               return_value=([], [], [], [], [], [])):
            self.assertEqual(az.gate_findings(), [])

    def test_library_overlap_does_not_call_its_vacuity_guard_a_double_claim(self):
        import contextlib
        import io

        from homm1.verify import library_overlap as lo
        with mock.patch.object(lo, "findings",
                               return_value=(["parsed 0 src claims"], 0)):
            with contextlib.redirect_stderr(io.StringIO()) as err:
                self.assertEqual(lo.main([]), 1)
        self.assertIn("0 src claims", err.getvalue())
        self.assertNotIn("double-claim(s). Each", err.getvalue())


class LibSymbolCacheControls(unittest.TestCase):
    """The toolchain .LIB symbol cache is keyed on the ARCHIVE SET.

    Measured 2026-08-16: an unkeyed cache written under toolchain r2 was
    still answering under r3 (46,866 live symbols vs 56,474 cached), so
    `verify link-tier` called 42 resolvable Win32 imports "a guaranteed
    unresolved external" while the candidate link reported ZERO unresolved.
    """

    def _archive(self, path: Path, names: list[str]) -> None:
        import struct as st
        body = st.pack(">I", len(names)) + b"\0" * (4 * len(names)) \
            + b"".join(n.encode() + b"\0" for n in names)
        head = b"/" + b" " * 15 + b"0" * 12 + b" " * 20 \
            + f"{len(body):<10d}".encode() + b"`\n"
        path.write_bytes(b"!<arch>\n" + head + body)

    def test_a_toolchain_change_invalidates_the_cache(self):
        from homm1.verify import undefined_closure as uc
        with tempfile.TemporaryDirectory() as td:
            r2, r3 = Path(td) / "r2/lib", Path(td) / "r3/lib"
            r2.mkdir(parents=True)
            r3.mkdir(parents=True)
            self._archive(r2 / "OLD.LIB", ["_OnlyInR2@4"])
            self._archive(r3 / "NEW.LIB", ["_OnlyInR3@4"])
            cache = Path(td) / "lib_symbols.txt"
            with mock.patch.object(uc, "LIB_CACHE", cache):
                with mock.patch.dict("os.environ",
                                     {"MSVC_DIR": str(r2.parent)},
                                     clear=False):
                    os.environ.pop("DXSDK_DIR", None)
                    first = uc.lib_symbols()
                self.assertIn("_OnlyInR2@4", first)
                with mock.patch.dict("os.environ",
                                     {"MSVC_DIR": str(r3.parent)},
                                     clear=False):
                    os.environ.pop("DXSDK_DIR", None)
                    second = uc.lib_symbols()
        self.assertIn("_OnlyInR3@4", second)
        self.assertNotIn("_OnlyInR2@4", second)   # the r2 answer is retired

    def test_an_unreachable_toolchain_keeps_the_last_answer(self):
        """Outside the dev shell $MSVC_DIR is unset; returning an empty set
        would make every consumer call the whole CRT unresolvable."""
        from homm1.verify import undefined_closure as uc
        with tempfile.TemporaryDirectory() as td:
            cache = Path(td) / "lib_symbols.txt"
            cache.write_text("# libs deadbeef\n_CloseHandle@4\n")
            with mock.patch.object(uc, "LIB_CACHE", cache), \
                 mock.patch.object(uc, "_toolchain_libs", return_value=[]):
                self.assertEqual(uc.lib_symbols(), {"_CloseHandle@4"})


class LinkTierRunnabilityControls(unittest.TestCase):
    def test_a_missing_map_is_a_finding_not_a_silent_skip(self):
        """With no .map the image diff cannot run at all - and main()'s
        success line claims every exact body is byte-identical in the linked
        image, which would then be a claim about a check that never ran."""
        from homm1.verify import link_tier as lt
        with tempfile.TemporaryDirectory() as td:
            cand = Path(td) / "c.exe"
            cand.write_bytes(b"MZ")
            with mock.patch.object(lt, "CAND", cand), \
                 mock.patch.object(lt, "CMAP", Path(td) / "absent.map"):
                out = lt.image_diff_findings()
        self.assertTrue(out and "could not run" in out[0])

    def test_a_missing_retail_section_is_fatal_and_reloc_is_exempt(self):
        """The third link-tier check had no control at all."""
        from homm1.verify import link_tier as lt
        with tempfile.TemporaryDirectory() as td:
            cand = Path(td) / "c.exe"
            cand.write_bytes(b"MZ")
            with mock.patch.object(lt, "CAND", cand), \
                 mock.patch.object(lt, "census",
                                   return_value=[(".text", 0x1000, 0x1000),
                                                 (".rsrc", 0x2000, 0),
                                                 (".reloc", 0x400, 0)]):
                bad = lt.census_findings()
        self.assertEqual(len(bad), 1)
        self.assertIn(".rsrc", bad[0])       # .reloc is deliberately exempt

    def test_no_candidate_at_all_stays_one_finding(self):
        from homm1.verify import link_tier as lt
        with tempfile.TemporaryDirectory() as td:
            with mock.patch.object(lt, "CAND", Path(td) / "none.exe"), \
                 mock.patch.object(lt, "CMAP", Path(td) / "none.map"):
                self.assertEqual(lt.image_diff_findings(), [])
                self.assertEqual(lt.census_findings(), [])


# --------------------------------------------------------------------------- #
# function source fingerprint name bridge                                     #
# --------------------------------------------------------------------------- #
class FingerprintNameControls(unittest.TestCase):
    def test_function_pointer_return_keeps_the_declared_method_name(self):
        from homm1.verify.fingerprints import _qualified_of
        demangled = ("public: void (__cdecl * __thiscall "
                     "CVariantSlot::Add(class zErrHandling *, "
                     "void (__cdecl *)(char *, int)))(char *, int)")
        self.assertEqual(_qualified_of(demangled), "CVariantSlot::Add")

    def test_ordinary_method_name_is_unchanged(self):
        from homm1.verify.fingerprints import _qualified_of
        self.assertEqual(
            _qualified_of("public: int __thiscall CFileIO::Open(char const *)"),
            "CFileIO::Open")


# --------------------------------------------------------------------------- #
# the MAX ledger: the bank rules themselves (the file is project state)       #
# --------------------------------------------------------------------------- #
class BankRatchetControls(unittest.TestCase):
    """bank_rows is what edits config/match_baseline.tsv. Every rule that
    protects a banked MAX gets a control here; nothing writes the ledger."""

    def _bank(self, cur, base, fps=None, rvas=None, library=()):
        from homm1.verify import verbs
        fps = fps or {}
        with mock.patch.object(verbs, "library_rvas", return_value=set(library)):
            return verbs.bank_rows(cur, base,
                                   lambda u, f: fps.get((u, f), "h1"),
                                   rvas or {})

    def _row(self, best=90.0, cur=90.0, fp="h1", addr=0x1000, hist=None,
             state="", tries=1):
        return {"best": best, "cur": cur, "tries": tries, "fp": fp,
                "addr": addr, "hist": best if hist is None else hist,
                "state": state}

    def test_a_dip_never_lowers_a_best_while_the_source_is_unchanged(self):
        key = ("u", "f")
        new, stats, reset, _drop = self._bank(
            {key: 70.0}, {key: self._row(best=90.0, cur=90.0)},
            rvas={key: 0x1000})
        self.assertEqual(new[key]["best"], 90.0)     # MAX held
        self.assertEqual(new[key]["cur"], 70.0)
        self.assertEqual(new[key]["hist"], 90.0)
        self.assertEqual(reset, [])

    def test_a_real_source_edit_resets_best_but_never_hist(self):
        key = ("u", "f")
        new, _s, reset, _d = self._bank(
            {key: 70.0}, {key: self._row(best=90.0, fp="old")},
            fps={key: "new"}, rvas={key: 0x1000})
        self.assertEqual(new[key]["best"], 70.0)
        self.assertEqual(new[key]["hist"], 90.0)     # the all-time peak holds
        self.assertEqual(len(reset), 1)

    def test_a_fallback_fingerprint_is_not_an_edit(self):
        from homm1.verify.fingerprints import FALLBACK
        key = ("u", "f")
        new, _s, reset, _d = self._bank(
            {key: 70.0}, {key: self._row(best=90.0, fp="real")},
            fps={key: FALLBACK + "abc"}, rvas={key: 0x1000})
        self.assertEqual(new[key]["best"], 90.0)
        self.assertEqual(new[key]["fp"], "real")     # the real hash is kept
        self.assertEqual(reset, [])

    def test_the_rva_moving_is_the_only_rva_keyed_reset(self):
        key = ("u", "f")
        new, stats, _r, _d = self._bank(
            {key: 55.0}, {key: self._row(best=90.0, addr=0x1000)},
            rvas={key: 0x2000})
        self.assertEqual(stats["rebounds"], 1)
        self.assertEqual(new[key]["best"], 55.0)     # a different BODY
        self.assertEqual(new[key]["hist"], 90.0)

    def test_a_unit_move_migrates_the_high_water_by_rva(self):
        old, new_key = ("olda", "f"), ("newb", "f")
        new, stats, _r, _d = self._bank(
            {new_key: 80.0}, {old: self._row(best=95.0, addr=0x1000)},
            rvas={new_key: 0x1000})
        self.assertEqual(stats["moved"], 1)
        self.assertEqual(new[new_key]["best"], 95.0)
        self.assertNotIn(old, new)

    def test_an_unscored_body_is_preserved_absent_and_round_trips(self):
        from homm1.verify import baseline as bl
        key = ("u", "f")
        new, stats, _r, dropped = self._bank(
            {}, {key: self._row(best=100.0, addr=0x1000)}, rvas={})
        self.assertEqual(stats["preserved_absent"], 1)
        self.assertEqual(new[key]["state"], "absent")
        self.assertEqual(new[key]["best"], 100.0)
        self.assertEqual(dropped, [])
        self.assertEqual(bl.load(bl.render(new)), new)     # survives the file

    def test_an_absent_row_is_dropped_once_its_rva_is_claimed_elsewhere(self):
        key, other = ("u", "f"), ("u", "g")
        new, stats, _r, dropped = self._bank(
            {other: 100.0}, {key: self._row(best=100.0, addr=0x1000)},
            rvas={other: 0x1000})
        # 0x1000 is now claimed under another name: keeping the row would pin
        # a phantom, and the high-water travelled with the body (moved).
        self.assertEqual(stats["moved"] + len(dropped), 1)
        self.assertNotIn("absent", {r.get("state") for r in new.values()})

    def test_banking_twice_changes_nothing(self):
        key = ("u", "f")
        base = {key: self._row(best=90.0, cur=90.0)}
        first, _s, _r, _d = self._bank({key: 95.0}, base, rvas={key: 0x1000})
        second, stats, _r, _d = self._bank({key: 95.0}, first,
                                           rvas={key: 0x1000})
        self.assertEqual(first, second)
        self.assertEqual(stats["raised"], 0)

    def test_the_ledger_round_trips_through_render_and_load(self):
        from homm1.verify import baseline as bl
        rows = {("u", "f"): self._row(best=99.1234, cur=98.7654, hist=100.0),
                ("u", "g"): self._row(best=100.0, cur=100.0, addr=None,
                                      state="absent")}
        self.assertEqual(bl.load(bl.render(rows)), rows)

    def test_render_keeps_the_explicit_state_field(self):
        from homm1.verify import baseline as bl
        rows = {("u", "scored"): self._row(),
                ("u", "gone"): self._row(state="absent")}
        function_lines = [line for line in bl.render(rows).splitlines()
                          if line.startswith("u\t") and line.count("\t") > 2]
        scored = next(line for line in function_lines if "\tscored\t" in line)
        absent = next(line for line in function_lines if "\tgone\t" in line)
        self.assertTrue(scored.endswith("\t"))
        self.assertTrue(absent.endswith("\tabsent"))


class BankPreconditionControls(unittest.TestCase):
    def test_an_unstaged_build_input_refuses_and_names_the_paths(self):
        from homm1.verify import verbs
        with mock.patch.object(verbs, "unstaged_bank_inputs",
                               return_value=["src/Giten/Grunt.cpp"]):
            with self.assertRaises(SystemExit) as e:
                verbs.require_bankable_tree("write the baseline")
        msg = str(e.exception)
        self.assertIn("src/Giten/Grunt.cpp", msg)
        self.assertIn("--dirty", msg)

    def test_dirty_warns_loudly_and_proceeds(self):
        import contextlib
        import io

        from homm1.verify import verbs
        with mock.patch.object(verbs, "unstaged_bank_inputs",
                               return_value=["include/Giten/Grunt.h"]):
            with contextlib.redirect_stderr(io.StringIO()) as err:
                verbs.require_bankable_tree("write the baseline",
                                            allow_dirty=True)
        self.assertIn("WARNING", err.getvalue())
        self.assertIn("include/Giten/Grunt.h", err.getvalue())

    def test_a_clean_tree_is_silent(self):
        from homm1.verify import verbs
        with mock.patch.object(verbs, "unstaged_bank_inputs", return_value=[]):
            verbs.require_bankable_tree("write the baseline")

    def test_a_stale_report_is_called_out(self):
        import contextlib
        import io

        from homm1.verify import verbs
        with tempfile.TemporaryDirectory() as td:
            report = Path(td) / "report.json"
            report.write_text("{}")
            objs = Path(td) / "build/objdiff/base"
            objs.mkdir(parents=True)
            obj = objs / "a.obj"
            obj.write_bytes(b"x")
            os.utime(obj, (report.stat().st_mtime + 600,) * 2)
            with mock.patch.object(verbs, "REPO", Path(td)):
                with contextlib.redirect_stderr(io.StringIO()) as err:
                    verbs._warn_stale_report(report)
        self.assertIn("STALE", err.getvalue())
        self.assertIn("homm1 build", err.getvalue())


# --------------------------------------------------------------------------- #
# the CONSUMER, not the recognizer: what the tier actually calls              #
# --------------------------------------------------------------------------- #
class GateConsumerControls(unittest.TestCase):
    """A recognizer control proves a helper; the tier calls the GATE. These
    drive the exact entry point tiers.TIERS holds."""

    def test_include_order_the_tier_calls_audit_not_parse(self):
        from homm1.verify import include_order as io
        from homm1.verify import tiers

        def with_file(text):
            with tempfile.TemporaryDirectory() as td:
                root = Path(td)
                (root / "src").mkdir()
                p = root / "src/Probe.cpp"
                p.write_text(text)
                with mock.patch.object(io, "repo_files", return_value=[p]), \
                     mock.patch.object(io, "REPO", root):
                    return tiers._include_order()
        dirty = with_file("#include <rva.h>\n#include <Zed.h>\n"
                          "#include <Abc.h>\n#include <Zed.h>\n\nint x;\n")
        self.assertTrue(any("duplicate include" in f for f in dirty))
        clean = with_file("#include <rva.h>\n\n#include <Abc.h>\n"
                          "#include <Zed.h>\n\nint x;\n")
        self.assertEqual(clean, [])

    def test_caller_callee_fires_above_the_floor_and_is_silent_at_it(self):
        from homm1.verify import caller_callee as cc
        rc = mock.Mock()
        rc.tgt = {(1, 2)}
        rc.name = lambda r: "?F@@YAXXZ"
        rc.unit = lambda r: "u"
        miss = [(0x1000 + i, 0x2000, "FAKE-VIEW", "CView") for i in range(5)]
        summary = (rc, miss, {"FAKE-VIEW": 5})
        with mock.patch.object(cc, "_summary", return_value=summary), \
             mock.patch("homm1.verify.board.load_baseline",
                        return_value={"caller-callee FAKE-VIEW": 4}):
            over = cc.gate_findings()
        self.assertTrue(over and "exceeds the committed floor 4" in over[0])
        with mock.patch.object(cc, "_summary", return_value=summary), \
             mock.patch("homm1.verify.board.load_baseline",
                        return_value={"caller-callee FAKE-VIEW": 5}):
            self.assertEqual(cc.gate_findings(), [])

    def test_vtables_gate_renders_every_defect_class(self):
        from homm1.verify import vtables as vt
        wiring = [("WIRING", 0x100, "CFader", 3, 0x2000,
                   "?Gap_17f660@@YAXXZ", "u", "not a virtual")]
        with mock.patch.object(vt, "analyse",
                               return_value=([], [], [], wiring, [], 1, 1)):
            self.assertTrue(any("vtable-slot-binding [WIRING]" in f
                                for f in vt.gate_findings()))
        gaps = [(0x1234, 16, "rtti", "CGhost", 0)]
        with mock.patch.object(vt, "analyse",
                               return_value=(gaps, [], [], [], [], 1, 1)):
            self.assertTrue(any("vtable-coverage" in f
                                for f in vt.gate_findings()))
        virt = [("CShell", 0x300, 9, 2, "under-virtualized")]
        with mock.patch.object(vt, "analyse",
                               return_value=([], virt, [], [], [], 1, 1)):
            self.assertTrue(any("vtable-virtuality" in f
                                for f in vt.gate_findings()))
        with mock.patch.object(vt, "analyse",
                               return_value=([], [], [], [], [], 1, 1)):
            self.assertEqual(vt.gate_findings(), [])

    def test_alloc_size_catches_a_planted_sizeof_error_on_the_real_tree(self):
        """The whole-tree control: take a class whose retail `push <n>`
        immediate ALREADY agrees with clang's sizeof, shift the computed side
        by 4, and require the live gate to name it. A gate that is blind
        returns the same 0 rows as a clean tree."""
        from homm1.core.paths import BUILD
        from homm1.verify import alloc_size as az
        if not (BUILD / "gen/class_sizes.json").is_file():
            self.skipTest("class_sizes cache absent (unbuilt tree)")
        comp, conflicts = az.computed_sizes()
        if not comp:
            self.skipTest("libclang harvest empty")
        rows = az.Sweep().rows()
        _b, _s, _m, _u, _un, ok = az.classify_rows(rows, comp, conflicts,
                                                   az.def_counts())
        if not ok:
            self.skipTest("no agreeing class to poison on this tree state")
        victim = sorted(o[0] for o in ok)[0]
        poisoned = dict(comp)
        poisoned[victim] += 4
        with mock.patch.object(az, "computed_sizes",
                               return_value=(poisoned, conflicts)):
            out = az.gate_findings()
        self.assertTrue(any(victim in f for f in out),
                        f"a 4-byte sizeof error on {victim} was not caught")


class PipelineErrorControls(unittest.TestCase):
    """The pipeline verbs answer a broken environment with a MESSAGE.

    `homm1 delink` used to let the delinker's ToolError escape as a
    traceback, and the tool's own words name a symptom ("relocation alias
    owner is absent: _length_code$S") whose real cause is a stale
    vostok-delinker on $PATH.
    """

    def _delink(self, exc):
        import contextlib
        import io

        from homm1.delink import run as dr
        with mock.patch.object(dr, "run", side_effect=exc):
            with contextlib.redirect_stderr(io.StringIO()) as err:
                rc = dr.main([])
        return rc, err.getvalue()

    def test_a_stale_delinker_is_named_not_just_echoed(self):
        from homm1.tool import ToolError
        rc, text = self._delink(ToolError(
            "vostok-delinker failed (rc=1):\nrelocation alias owner is "
            "absent: _length_code$S"))
        self.assertEqual(rc, 1)
        self.assertIn("_length_code$S", text)      # the tool's own words kept
        self.assertIn("STALE vostok-delinker", text)
        self.assertIn("which vostok-delinker", text)

    def test_a_missing_delinker_binary_is_a_message_not_a_traceback(self):
        rc, text = self._delink(
            FileNotFoundError(2, "No such file or directory",
                              "vostok-delinker"))
        self.assertEqual(rc, 1)
        self.assertIn("nix develop", text)

    def test_an_unhinted_failure_still_prints_the_tools_words(self):
        from homm1.tool import ToolError
        rc, text = self._delink(ToolError("vostok-delinker failed (rc=9)"))
        self.assertEqual(rc, 1)
        self.assertIn("rc=9", text)


class FloorAbsenceControls(unittest.TestCase):
    """A ratchet with no floor must FAIL, never pass.

    board and casts used to fail OPEN: deleting the two files in
    config/cleanliness/ made the whole fast tier permanently green, while
    caller-callee and undefined-closure already refused to pass vacuously.
    """

    def test_board_reports_a_ratcheted_metric_with_no_floor(self):
        from homm1.verify import board
        with mock.patch.object(board, "load_baseline", return_value={}):
            found = board.gate([("unexplained casts", 9999)])
        self.assertTrue(found, "a ratcheted metric with no floor passed")
        self.assertIn("no committed floor", found[0])

    def test_board_still_ratchets_when_the_floor_exists(self):
        from homm1.verify import board
        with mock.patch.object(board, "load_baseline",
                               return_value={"unexplained casts": 10}):
            self.assertTrue(board.gate([("unexplained casts", 11)]))
            self.assertEqual(board.gate([("unexplained casts", 10)]), [])

    def test_casts_reports_a_missing_floor(self):
        from homm1.verify import board, casts
        with mock.patch.object(board, "load_baseline", return_value={}), \
             mock.patch.object(casts, "self_recursion", return_value=[]), \
             mock.patch.object(casts, "scan_ledger", return_value=({}, {"a.cpp": [1]})):
            found = casts.gate_findings()
        self.assertTrue(found, "an absent cast floor passed the gate")
        self.assertIn("no committed floor", found[0])


class LinkClosureScanSetControls(unittest.TestCase):
    """The closure check must scan the link line we actually use.

    graph.link substitutes our synthesized import libs into LINK_LIBS, so
    mss32/smackw32 imports resolve at link time; scanning only the
    toolchain reported 26 of them as guaranteed-unresolved.
    """

    def test_synthesized_import_libs_are_in_the_scan_set(self):
        from homm1.graph import implib
        from homm1.verify import undefined_closure as uc
        made = [p for p in implib.on_disk() if p.is_file()]
        if not made:
            self.skipTest("no synthesized import libs on disk")
        scanned = set(uc._toolchain_libs())
        for p in made:
            self.assertIn(p, scanned, f"{p.name} is on the link line but unscanned")


class TsvAtomicWriteControls(unittest.TestCase):
    """A concurrent reader never sees a half-written table.

    Observed live: a gate crashed with `no header row` while a build edge
    rewrote build/gen/claims/grunt.tsv in place.
    """

    def test_write_replaces_atomically(self):
        import os
        from homm1.core import tsv
        with tempfile.TemporaryDirectory() as d:
            path = Path(d) / "t.tsv"
            tsv.write(path, ["# b"], ["a", "b"], [["1", "2"]])
            before = path.read_text()
            seen = []
            real_replace = os.replace

            def spy(src, dst):
                # the destination must still hold the OLD table right up to
                # the instant of replacement - never a truncated one
                seen.append(Path(dst).read_text())
                return real_replace(src, dst)

            with mock.patch("os.replace", spy):
                tsv.write(path, ["# b"], ["a", "b"], [["3", "4"]])
            self.assertEqual(seen, [before])
            self.assertIn("3\t4", path.read_text())

    def test_no_temp_file_survives(self):
        from homm1.core import tsv
        with tempfile.TemporaryDirectory() as d:
            path = Path(d) / "t.tsv"
            tsv.write(path, ["# b"], ["a"], [["1"]])
            self.assertEqual([p.name for p in Path(d).iterdir()], ["t.tsv"])


class SourceNameRewriteControls(unittest.TestCase):
    """The rewrite rules are COMPLETE, proven once per build over the corpus.

    Labelling spells every claim from source (core.msvc_names), so a rule gap
    can no longer hide as a silent per-claim drop - it has to fail here. The
    control is the same assertion the dropped per-claim authority check made,
    lifted to the whole claim set: for EVERY extracted source claim, the name
    equals the emitting base object's own symbol modulo the volatile ordinals
    both sides mask. Reading the objects is fine HERE; it is a test, not the
    extraction path.
    """

    @staticmethod
    def _corpus():
        from homm1.core.paths import BUILD
        from homm1.retail_labels import fragments
        base = BUILD / "objdiff/base"
        claims = [c for c in fragments.all_claims() if c.channel == "src"]
        return base, claims

    #: every decoration cl 5.0 could have chosen instead - if one of THESE is
    #: in the object, the claim named the right body and spelled it wrong.
    @staticmethod
    def _alternate_spellings(name: str) -> set[str]:
        import re
        out = set()
        for n in {name, re.sub(r"@@([0-9])P", r"@@\1Q", name)}:
            for m in {n, n.removesuffix("$S"), n + "$S"}:
                out |= {m, "_" + m, m.removeprefix("_")}
        return out - {name}

    def test_every_source_claim_is_cls_own_spelling(self):
        from homm1.core.coff import Coff
        from homm1.core.msvc_names import mask
        from homm1.model import unmaterialized
        base, claims = self._corpus()
        if not claims:
            self.skipTest("no extracted claims - run `homm1 labels --all`")
        objs: dict[str, tuple[set[str], set[str]] | None] = {}
        for unit in {c.unit for c in claims}:
            path = base / f"{unit}.obj"
            if not path.is_file():
                objs[unit] = None
                continue
            coff = Coff(path)
            objs[unit] = ({mask(n) for n in coff.code_names()},
                          {mask(n) for n in coff.all_names()})
        absent = sorted(u for u, v in objs.items() if v is None)
        if absent:
            self.skipTest(f"{len(absent)} unit(s) have no base obj "
                          f"(e.g. {absent[0]}) - run `homm1 build`")
        # A header inline's macro reaches every including TU, but cl
        # materializes the COMDAT only where it is odr-used - so a claim with
        # no symbol in ITS OWN unit is expected. The rewrite is in question
        # only when NO unit claiming that (kind, rva, name) carries it.
        claimed, proven = {}, set()
        for c in claims:
            code, every = objs[c.unit]
            key = (c.kind, c.rva, mask(c.name))
            claimed.setdefault(key, []).append(c)
            if key[2] in (code if c.kind == "func" else every):
                proven.add(key)
        # ... and a gap is a SPELLING defect only if some other decoration of
        # the same claim IS in one of those objects. A gap with no spelling at
        # all is a missing body - a modelling question the Model reports.
        misspelled, bodiless = [], []
        for key in sorted(set(claimed) - proven):
            cs = claimed[key]
            alts = self._alternate_spellings(cs[0].name)
            hit = next((a for c in cs for a in sorted(alts)
                        if a in objs[c.unit][1]), None)
            row = f"{key[0]} 0x{key[1]:06x} {cs[0].name}"
            (misspelled if hit else bodiless).append(
                f"{row} -> cl spells it {hit}" if hit else row)
        self.assertFalse(
            misspelled,
            f"{len(misspelled)} of {len(claimed)} source claim(s) are spelled "
            f"differently by cl - the rewrite rules are incomplete "
            f"(first: {misspelled[0] if misspelled else ''})")
        # the bodiless class must stay LOUD somewhere: a claim with no other
        # spelling at its rva is a Model violation, one WITH another spelling
        # is recorded as that binding's alias. Nothing may be silent.
        gaps = {(c.kind, c.rva, mask(c.name)) for c in unmaterialized(
            [c for cs in claimed.values() for c in cs])}
        aliased = {rva for kind, rva, _n in proven if kind == "func"}
        unreported = sorted(
            k for k in set(claimed) - proven - gaps
            if k[0] == "func" and k[1] not in aliased)
        self.assertFalse(
            unreported,
            f"{len(unreported)} claim(s) match no object symbol and are "
            f"reported by nothing (first: {unreported[0] if unreported else ''})")

    def test_a_missing_rewrite_rule_fails_that_control(self):
        """The negative control: undo two rules, the corpus control must fail.

        A gate that would pass an incomplete rewrite is not a gate."""
        import io
        import re
        from homm1.retail_labels import fragments

        _base, claims = self._corpus()
        if not claims:
            self.skipTest("no extracted claims - run `homm1 labels --all`")
        if not any(re.search(r"@@[0-9]P", c.name) or c.name.endswith("$S")
                   for c in claims):
            self.skipTest("the corpus has no claim the poisoned rules rewrite "
                          "(no `@@nP` / `$S` decoration) - the control would be vacuous")

        def poisoned():
            out = []
            for c in claims:
                name = re.sub(r"@@([0-9])P", r"@@\1Q", c.name)   # undo Q -> P
                if name.endswith("$S"):                          # undo _x$S
                    name = (name[1:] if name.startswith("_") else name)[:-2]
                out.append(c._replace(name=name))
            return out

        with mock.patch.object(fragments, "all_claims", poisoned):
            case = SourceNameRewriteControls(
                "test_every_source_claim_is_cls_own_spelling")
            result = unittest.TextTestRunner(stream=io.StringIO()).run(
                unittest.TestSuite([case]))
        self.assertEqual(len(result.failures), 1,
                         "a broken rewrite rule did not fail the corpus control")
        self.assertIn("rewrite rules are incomplete", result.failures[0][1])

    def test_masking_never_merges_two_object_symbols(self):
        """The mask is only sound while it is injective per object."""
        from homm1.core.coff import Coff
        from homm1.core.msvc_names import mask
        base, _claims = self._corpus()
        objs = sorted(base.glob("*.obj"))
        if not objs:
            self.skipTest("no base objs")
        collisions = []
        for path in objs:
            try:
                names = Coff(path).all_names()
            except ValueError:
                continue
            seen: dict[str, str] = {}
            for name in sorted(names):
                other = seen.setdefault(mask(name), name)
                if other != name:
                    collisions.append(f"{path.stem}: {other} / {name}")
        self.assertFalse(collisions,
                         f"{len(collisions)} object symbol pair(s) mask "
                         f"together (first: {collisions[0] if collisions else ''})")

    def test_the_rewrite_rules_are_the_measured_ones(self):
        from homm1.core import msvc_names as m
        # the i386 COFF global prefix, applied to what LLVM did not mangle
        self.assertEqual(m.func("?Foo@C@@QAEXXZ"), "?Foo@C@@QAEXXZ")
        self.assertEqual(m.func("_stdcall_thing@8", decorated=True),
                         "_stdcall_thing@8")
        self.assertEqual(m.func("ordinary"), "_ordinary")
        # clang's array storage class
        self.assertEqual(m.data("?g_cmdBitTable@@3QBGB", internal=False),
                         "?g_cmdBitTable@@3PBGB")
        # TU-local storage: `_` and `$S` arrive together, whatever the mangling
        self.assertEqual(m.data("s_MAIN", internal=True), "_s_MAIN$S")
        self.assertEqual(m.data("_kDegToRad", internal=True, decorated=True),
                         "_kDegToRad$S")
        self.assertEqual(m.data("?s_x@?1??F@@QAEHXZ@4HA", internal=True),
                         "_?s_x@?1??F@@QAEHXZ@4HA$S")
        # the mask meets cl's own object on both ordinals
        self.assertEqual(m.mask("_?s_x@?BA@??F@@QAEHXZ@4HA$S35536"),
                         "_?s_x@?1??F@@QAEHXZ@4HA$S")
        self.assertEqual(m.mask("_?$S47@?1??G@@QAEHXZ@4EA$S20267"),
                         "_?$S@?1??G@@QAEHXZ@4EA$S")
        # an rva-keyed name is NOT an ordinal: it must survive masking
        self.assertEqual(m.mask("$S2277272"), "$S2277272")
        self.assertEqual(m.mask(m.discriminate("_s_x$S", 0x244970)), "_s_x$S")


class DataAlignmentPaddingControls(unittest.TestCase):
    """A named static's identity survives either allocator's alignment gap."""

    @staticmethod
    def obj(payload: bytes, successor: int | None, name="_s_msToSeconds$S7"):
        import struct
        rawptr = 60
        symptr = rawptr + len(payload)
        strings = bytearray(bytes(4))
        symbols = bytearray()
        rows = [(name, 0)] + ([("_next$S9", successor)] if successor else [])
        for symbol, value in rows:
            symbols += struct.pack("<II", 0, len(strings))
            strings += symbol.encode("latin1") + b"\0"
            symbols += struct.pack("<IhHBB", value, 1, 0, 3, 0)
        struct.pack_into("<I", strings, 0, len(strings))
        header = struct.pack("<HHIIIHH", 0x14c, 1, 0, symptr, len(rows), 0, 0)
        section = struct.pack("<8sIIIIIIHHI", b".rdata", 0, 0, len(payload),
                              rawptr, 0, 0, 0, 0, 0x40400040)
        return header + section + payload + symbols + strings

    @staticmethod
    def canonical(data: bytes) -> str:
        from homm1.compare.canonicalize import canonicalize_coff
        rows = canonicalize_coff(data).rows
        return next(row.canonical_name for row in rows
                    if row.original_name.startswith("_s_msToSeconds"))

    def test_padded_and_packed_spans_share_one_identity(self):
        value = bytes.fromhex("6f12833a")
        packed = self.obj(value + bytes.fromhex("0000803f"), 4)
        padded = self.obj(value + bytes(4) + bytes(8), 8)
        self.assertEqual(self.canonical(packed), self.canonical(padded))

    def test_content_beyond_the_gap_still_decides_identity(self):
        value = bytes.fromhex("6f12833a")
        self.assertNotEqual(self.canonical(self.obj(value + bytes(4), None)),
                            self.canonical(self.obj(value + bytes([0, 0, 0, 1]), None)))
        self.assertNotEqual(self.canonical(self.obj(value + bytes(12), None)),
                            self.canonical(self.obj(value, None)))


class AnonymousNamespaceControls(unittest.TestCase):
    """Anonymous .cpp COMMON identities survive both consumers and rebuilds."""

    @staticmethod
    def names(path=r"Z:\checkout\src\Wwd\WwdFactoryObject.cpp", nonce="1234"):
        scope = "?%" + path + nonce + "@"
        return ("?s_holdrand@?1??GetRandomNumber@" + scope + "@YAHXZ@4JA",
                "??_B?1??GetRandomNumber@" + scope + "@YAHXZ@51")

    @staticmethod
    def obj(names):
        import struct
        code = b"\xa1" + bytes(4) + b"\x8a\x15" + bytes(4) + b"\xc3"
        rawptr = 60
        relptr = rawptr + len(code)
        symptr = relptr + 20
        strings = bytearray(bytes(4))
        symbols = bytearray()
        for name, value, sec, typ in [
                (names[0], 4, 0, 0), (names[1], 1, 0, 0),
                ("_entry", 0, 1, 0x20)]:
            symbols += struct.pack("<II", 0, len(strings))
            strings += name.encode("latin1") + b"\0"
            symbols += struct.pack("<IhHBB", value, sec, typ, 2, 0)
        struct.pack_into("<I", strings, 0, len(strings))
        header = struct.pack("<HHIIIHH", 0x14c, 1, 0, symptr, 3, 0, 0)
        section = struct.pack("<8sIIIIIIHHI", b".text", 0, 0, len(code),
                              rawptr, relptr, 0, 2, 0, 0x60500020)
        relocs = struct.pack("<IIHIIH", 1, 0, 6, 7, 1, 6)
        return header + section + code + relocs + symbols + strings

    def test_actual_normalizer_removes_only_cpp_namespace_build_identity(self):
        from homm1.compare.canonicalize import canonicalize_coff
        from homm1.core.msvc_names import anonymous_namespaces
        a = self.obj(self.names())
        b = self.obj(self.names(r"Z:\another\worktree\src\Wwd\WwdFactoryObject.cpp", "987654"))
        self.assertNotEqual(a, b)
        self.assertEqual(canonicalize_coff(a).data, canonicalize_coff(b).data)
        # A recognizer-only test would pass if canonicalize_coff forgot to use it.
        with mock.patch("homm1.compare.canonicalize.msvc_names.anonymous_namespaces",
                        side_effect=lambda name: name):
            self.assertNotEqual(canonicalize_coff(a).data, canonicalize_coff(b).data)
        self.assertNotEqual(
            canonicalize_coff(a).data,
            canonicalize_coff(self.obj(self.names(r"Z:\checkout\src\DDrawMgr\FaderEffects.cpp"))).data)
        header = self.names(r"Z:\checkout\include\Shared.h")[0]
        self.assertEqual(anonymous_namespaces(header), header)

    def test_common_owner_and_raw_referent_resolver_use_same_identity(self):
        from homm1.core.msvc_names import mask
        from homm1.delink.data_manifest import _common_owner
        from homm1.verify.assert_relocs import Resolver
        names = self.names()
        with tempfile.TemporaryDirectory() as td:
            Path(td, "wwdfactoryobject.obj").write_bytes(self.obj(names))
            owners = _common_owner(td)
        self.assertEqual(owners[mask(names[0])], "wwdfactoryobject")
        self.assertEqual(owners[mask(names[1])], "wwdfactoryobject")
        resolver = Resolver.__new__(Resolver)
        resolver.names = {mask(names[0]): {0x2c2798}, mask(names[1]): {0x2c278c}}
        self.assertEqual(resolver.rva_of(names[0]), {0x2c2798})
        self.assertEqual(resolver.rva_of(names[1]), {0x2c278c})
        other = self.names(r"Z:\checkout\src\DDrawMgr\FaderEffects.cpp")[0]
        self.assertEqual(resolver.rva_of(other), set())

    def test_namespace_identity_collision_is_an_error(self):
        from homm1.compare.canonicalize import canonicalize_coff
        a = self.names()[0]
        b = self.names(nonce="9876")[0]
        with self.assertRaisesRegex(ValueError, "namespace identities collide"):
            canonicalize_coff(self.obj((a, b)))

    def test_same_source_compiled_into_multiple_objects_is_not_coalesced(self):
        from homm1.delink.data_manifest import _common_owner
        with tempfile.TemporaryDirectory() as td:
            Path(td, "first.obj").write_bytes(self.obj(self.names()))
            Path(td, "second.obj").write_bytes(self.obj(self.names(nonce="9876")))
            with self.assertRaisesRegex(ValueError, "emitted by multiple units"):
                _common_owner(td)


class ReadmeFreshnessControls(unittest.TestCase):
    """README's derived block must not be able to go stale.

    It is a pure function of the current report + the banked ledger, but it
    used to move only at `bank` (a deliberate manual act), so ordinary builds
    left it describing an older tree and readers quoted numbers that were no
    longer true - three times in one session. `check` (merge preparation)
    re-renders it write-if-changed; the ledger stays manual.
    """

    def test_check_rewrites_a_stale_block(self):
        from homm1.verify import readme as rm, verbs
        if not rm.README.is_file():
            self.skipTest("no README")
        before = rm.README.read_text()
        # Anchor on a token the block ALWAYS carries. The original anchor was
        # the `(unmatched)` row, which stopped existing the day the last
        # unclaimed reconstruction target got modelled - a freshness control
        # must not assert a row that only appears while work is outstanding.
        anchor = "CUR / MAX / HIST"
        self.assertIn(anchor, before, "the block lost its headline line")
        try:
            rm.README.write_text(before.replace(anchor, anchor + "-STALE", 1))
            verbs.refresh_readme_block()
            fresh = rm.README.read_text()
            self.assertIn(anchor, fresh)
            self.assertNotIn(anchor + "-STALE", fresh)   # the stale text is GONE
        finally:
            rm.README.write_text(before)

    def test_refresh_is_idempotent(self):
        from homm1.verify import readme as rm, verbs
        if not rm.README.is_file():
            self.skipTest("no README")
        before = rm.README.read_text()
        try:
            verbs.refresh_readme_block()
            self.assertFalse(verbs.refresh_readme_block(),
                             "a second refresh reported a change")
        finally:
            rm.README.write_text(before)

    def test_readme_is_not_a_bank_input(self):
        # writing it must never be able to block `bank`
        from homm1.verify.verbs import BANK_INPUT_PATHS
        self.assertNotIn("README.md", BANK_INPUT_PATHS)


# --------------------------------------------------------------------------- #
# the build loop (2026-08-16 review)                                          #
# --------------------------------------------------------------------------- #
def _coff(nsec: int = 0, symptr: int = 0, nsym: int = 0, machine: int = 0x14C,
          sections: list[tuple[int, int]] = (), tail: bytes = b"") -> bytes:
    """A hand-built COFF header (+ `sections` as (size, ptr) pairs)."""
    import struct
    out = bytearray(struct.pack("<HHIIIHH", machine, nsec, 0, symptr, nsym, 0, 0))
    for size, ptr in sections:
        raw = bytearray(40)
        struct.pack_into("<II", raw, 16, size, ptr)
        out += raw
    return bytes(out) + tail


class ClEdgeObjectIntegrityControls(unittest.TestCase):
    """The `cl` edge published an incomplete object and called the edge built.

    Two `homm1 build` runs in one tree are not serialised by anything, and
    the driver staged every unit at the SAME `.tmp/<unit>.obj` and installed
    through the same `<unit>.obj.install`. Measured 3/3: one run's cl deleted
    the other's staged object between its `compile()` and its `install()` ("cl
    produced no object (rc=0)", with an EMPTY diagnostic - the compiler blamed
    for a collision) and the shared `.install` rename raised FileNotFoundError
    as a traceback. The shared temp also makes `os.replace` non-atomic across
    processes, and a real build did fail in homm1.compare.normalize with
    "COFF string table is not final" over an object that a census then found
    invalid. Nothing checked the payload before installing it either.
    """

    def test_a_truncated_object_is_refused_not_installed(self):
        from homm1.graph import cc
        from homm1.tool import ToolError
        # one section header promising 0x1000 bytes at 0x40 in a 60-byte file
        torn = _coff(nsec=1, sections=[(0x1000, 0x40)])
        self.assertIsNotNone(cc.coff_defect(torn))
        with tempfile.TemporaryDirectory() as td:
            out = Path(td) / "unit.obj"
            with self.assertRaises(ToolError):
                cc.install(torn, out)
            self.assertFalse(out.exists(), "a torn object was installed anyway")

    def test_a_truncated_string_table_is_refused(self):
        from homm1.graph import cc
        import struct
        # symbol table present, string-table length dword promises past EOF
        body = _coff(nsec=0, symptr=20, nsym=1,
                     tail=b"\0" * 18 + struct.pack("<I", 0x1000))
        self.assertIn("string table", cc.coff_defect(body) or "")

    def test_a_complete_object_passes(self):
        from homm1.graph import cc
        self.assertIsNone(cc.coff_defect(_coff()))
        with tempfile.TemporaryDirectory() as td:
            out = Path(td) / "unit.obj"
            self.assertTrue(cc.install(_coff(), out))
            self.assertFalse(cc.install(_coff(), out), "rewrote unchanged bytes")
            self.assertEqual(list(Path(td).iterdir()), [out])   # no temp left

    def test_install_does_not_use_the_shared_temp_name(self):
        """A sibling build holding `<unit>.obj.install` must not break us."""
        from homm1.graph import cc
        with tempfile.TemporaryDirectory() as td:
            out = Path(td) / "unit.obj"
            (Path(td) / "unit.obj.install").mkdir()       # the old, shared name
            self.assertTrue(cc.install(_coff(), out))
            self.assertTrue(out.is_file())

    def test_the_staging_path_is_per_process(self):
        from homm1.graph import cc
        seen = {}

        def fake(src, staged, flags):
            seen["staged"] = Path(staged)
            Path(staged).write_bytes(_coff())
            return ""

        with tempfile.TemporaryDirectory() as td:
            src = Path(td) / "u.c"
            src.write_text("int main(void){return 0;}\n")
            with mock.patch("homm1.tool.cl.compile", fake):
                cc.compile_unit(src, Path(td) / "base" / "u.obj", [])
        parts = seen["staged"].parts
        self.assertIn(str(os.getpid()), parts, f"shared staging dir: {parts}")
        self.assertIn(".tmp", parts)

    def test_an_unwritable_object_tree_is_a_message(self):
        import contextlib
        import io
        from homm1.graph import cc
        with tempfile.TemporaryDirectory() as td:
            ro = Path(td) / "ro"
            ro.mkdir()
            ro.chmod(0o500)
            try:
                with contextlib.redirect_stderr(io.StringIO()) as err:
                    rc = cc.main(["--out", str(ro / "u.obj"), "--src", str(ro),
                                  "--unit", "u", "--", "/c"])
            finally:
                ro.chmod(0o700)
        self.assertEqual(rc, 1)          # a message + rc 1, never a traceback
        self.assertIn("cannot write", err.getvalue())


class ToolDriverEnvironmentControls(unittest.TestCase):
    """Outside `nix develop` the drivers raised bare Python exceptions.

    `$MSVC_DIR` unset reached the user as a RuntimeError traceback from
    homm1.core.paths (no tool main() catches it), and an absent
    wine/winepath/llvm-pdbutil/vostok-delinker as a FileNotFoundError out of
    subprocess - including from `homm1 tool wine --verify`, whose whole job
    is to answer "is wine set up?".
    """

    def _no_path(self):
        return mock.patch("shutil.which", lambda _p: None)

    def test_clang_ir_accepts_vc5_unsigned_long_case_label(self):
        """Exercise the extraction parser, not merely the flag list.

        VC5 accepts DirectX SDK constants such as 0x80040200L in a signed-int
        switch. Clang treats that conversion as a hard C++11 narrowing error
        unless the shared compatibility flag reaches emit_ir().
        """
        from homm1.tool import clang

        if not shutil.which(os.environ.get("GITEN_CLANG") or "clang"):
            self.skipTest("clang unavailable")
        with tempfile.TemporaryDirectory() as td:
            src = Path(td) / "case_probe.cpp"
            src.write_text(
                "int probe(int value) {\n"
                "    switch (value) { case 0x80040200L: return 1; }\n"
                "    return 0;\n"
                "}\n"
            )
            ir = clang.emit_ir(str(src), clang.MS_FLAGS)
        self.assertIsNotNone(ir)
        self.assertIn("probe", ir)

    def test_missing_msvc_dir_is_a_toolerror(self):
        from homm1.tool import ToolError, wine
        with mock.patch.dict(os.environ, {}, clear=False):
            os.environ.pop("MSVC_DIR", None)
            with self.assertRaises(ToolError) as cm:
                wine.era_tool("cl.exe")
        self.assertIn("MSVC_DIR", str(cm.exception))

    def test_the_rc_release_hint_is_only_on_rc(self):
        from homm1.tool import ToolError, wine
        with tempfile.TemporaryDirectory() as td:
            with mock.patch.object(wine, "toolchain_root", lambda: Path(td)):
                with self.assertRaises(ToolError) as rc_exc:
                    wine.era_tool("rc.exe")
                with self.assertRaises(ToolError) as cl_exc:
                    wine.era_tool("cl.exe")
        self.assertIn("r3", str(rc_exc.exception))
        self.assertNotIn("rc.exe", str(cl_exc.exception))

    def test_absent_wine_is_named_not_a_traceback(self):
        from homm1.tool import ToolError, wine
        with self._no_path():
            for call in (lambda: wine.winepath("/tmp"),
                         lambda: wine.require("wine")):
                with self.assertRaises(ToolError) as cm:
                    call()
                self.assertIn("nix develop", str(cm.exception))

    def test_wine_verify_without_wine_returns_a_message(self):
        import contextlib
        import io
        from homm1.tool import wine
        with self._no_path(), mock.patch.object(sys, "argv",
                                                ["homm1 tool wine", "--verify"]):
            with contextlib.redirect_stderr(io.StringIO()) as err:
                rc = wine.main()
        self.assertEqual(rc, 1)
        self.assertIn("not found on PATH", err.getvalue())

    def test_wine_with_no_action_is_not_a_silent_success(self):
        import contextlib
        import io
        from homm1.tool import wine
        for argv in (["homm1 tool wine"], ["homm1 tool wine", "--force"]):
            with mock.patch.object(sys, "argv", argv):
                with contextlib.redirect_stderr(io.StringIO()) as err:
                    rc = wine.main()
            self.assertEqual(rc, 2, argv)
            self.assertIn("--init", err.getvalue())

    def test_absent_native_tools_are_named(self):
        from homm1.tool import ToolError, delinker, pdbutil
        with self._no_path():
            with self.assertRaises(ToolError) as cm:
                delinker.delink("a.pdb", "b.exe", "/tmp/out")
            self.assertIn("vostok-delinker", str(cm.exception))
            with self.assertRaises(ToolError) as cm:
                pdbutil.dump("a.pdb")
            self.assertIn("llvm-pdbutil", str(cm.exception))

    def test_a_stalled_native_tool_is_a_message(self):
        import subprocess
        from homm1.tool import ToolError, pdbutil
        with mock.patch("shutil.which", lambda p: "/bin/" + p), \
                mock.patch("subprocess.run",
                           side_effect=subprocess.TimeoutExpired("x", 1)):
            with self.assertRaises(ToolError) as cm:
                pdbutil.dump("a.pdb")
        self.assertIn("did not finish", str(cm.exception))

    def test_implib_without_the_retail_image_is_a_message(self):
        import contextlib
        import io
        from homm1.graph import implib
        with mock.patch.object(sys, "argv", ["implib", "--list"]), \
                mock.patch.object(implib, "import_table",
                                  side_effect=FileNotFoundError(
                                      2, "No such file or directory",
                                      "DDS.EXE")):
            with contextlib.redirect_stderr(io.StringIO()) as err:
                rc = implib.main()
        self.assertEqual(rc, 1)
        self.assertIn("retail image", err.getvalue())


class LabelGraphDependencyControls(unittest.TestCase):
    def test_a_header_is_an_input_of_both_compile_and_label_edges(self):
        """An inline RVA name can change while its TU emits no changed bytes.

        The cl edge then restats its object, so only a direct header dependency
        can make the label extractor refresh the retail claim.
        """
        from homm1.graph import emit

        class OneHeader:
            def headers(self, _source):
                return ["include/Test/InlineClaim.h"]

            def scanned(self):
                return {"src/Test/Owner.cpp", "include/Test/InlineClaim.h"}

        unit = {"unit": "owner", "source": "src/Test/Owner.cpp",
                "flags": "retail", "cflags": ["/nologo", "/c"]}
        manifest = {"flags": {"retail": ["/nologo", "/c"]}}
        with tempfile.TemporaryDirectory() as td, \
                mock.patch.object(emit, "load_units",
                                  return_value=(manifest, [unit])), \
                mock.patch.object(emit, "prune_orphan_artifacts",
                                  return_value=0), \
                mock.patch.object(emit, "write_toolchain_id",
                                  return_value=False), \
                mock.patch.object(emit, "Scanner", return_value=OneHeader()), \
                mock.patch.object(emit, "era_rc_available", return_value=False):
            out = Path(td) / "build.ninja"
            emit.emit(out)
            graph_text = out.read_text()

        def edge(prefix):
            start = graph_text.index(prefix)
            end = graph_text.index("\n\n", start)
            return graph_text[start:end]

        compile_edge = edge("build build/objdiff/base/owner.obj: cl")
        label_edge = edge("build build/gen/claims/owner.tsv: labels")
        self.assertIn("include/Test/InlineClaim.h", compile_edge)
        self.assertIn("include/Test/InlineClaim.h", label_edge)


class LinkVerbTargetControls(unittest.TestCase):
    """`homm1 link <anything>` died on `unknown target build/gen/homm1.res`.

    The `.res` edge exists only when the toolchain carried rc.exe at CONFIGURE
    time; the verb asked ninja for it unconditionally, so on a pre-r3 pin every
    flagged form - `--help` included - failed before reaching the parser, and
    `--help` also ran a full build first.
    """

    def test_manifest_targets_reads_the_emitted_edges(self):
        from homm1.graph import verbs
        with tempfile.TemporaryDirectory() as td:
            man = Path(td) / "build.ninja"
            man.write_text("rule cl\n  command = x\n"
                           "build a/b.obj: cl src.c | dep.h\n"
                           "build one two: phony $\n    three\n")
            with mock.patch.object(verbs.graph, "NINJA", str(man)), \
                    mock.patch.object(verbs, "REPO", Path("/")):
                got = verbs.manifest_targets()
        self.assertEqual(got, {"a/b.obj", "one", "two"})
        self.assertNotIn("build/gen/homm1.res", got)

    def test_a_missing_manifest_is_an_empty_set_not_a_traceback(self):
        from homm1.graph import verbs
        with mock.patch.object(verbs.graph, "NINJA", "no/such/build.ninja"):
            self.assertEqual(verbs.manifest_targets(), set())

    def test_help_answers_the_parser_without_building(self):
        import contextlib
        import io
        from homm1.graph import verbs
        with mock.patch.object(verbs, "ninja",
                               side_effect=AssertionError("built for --help")), \
                mock.patch.object(verbs, "configure_if_needed",
                                  side_effect=AssertionError("configured")):
            with contextlib.redirect_stdout(io.StringIO()) as out:
                with self.assertRaises(SystemExit) as cm:
                    verbs.link_main(["--help"])
        self.assertEqual(cm.exception.code, 0)
        self.assertIn("--engine-lib", out.getvalue())

    def test_the_res_target_is_only_requested_when_it_exists(self):
        import contextlib
        import io
        from homm1 import graph
        from homm1.graph import verbs
        asked = []
        with mock.patch.object(verbs, "configure_if_needed", lambda *a, **k: None), \
                mock.patch.object(verbs, "ninja",
                                  lambda t, **k: asked.append(list(t)) or 1):
            with mock.patch.object(verbs, "manifest_targets", lambda: {"base"}):
                with contextlib.redirect_stderr(io.StringIO()) as err:
                    verbs.link_main(["--dry-run"])
            self.assertEqual(asked[-1], ["base"])
            self.assertIn("no build/gen/homm1.res edge", err.getvalue())
            with mock.patch.object(verbs, "manifest_targets",
                                   lambda: {"base", graph.RESOURCE_RES}):
                verbs.link_main(["--dry-run"])
            self.assertEqual(asked[-1], ["base", graph.RESOURCE_RES])

    def test_a_resourceless_candidate_says_so(self):
        """A candidate with no .rsrc has no MFC dialogs; the only note used to
        live in a generated manifest comment nobody reads."""
        import contextlib
        import io
        from homm1.graph import implib, link as gl
        with tempfile.TemporaryDirectory() as td:
            out = Path(td) / "cand.EXE"
            obj = Path(td) / "u.obj"
            obj.write_bytes(_coff())

            def fake_link(args, **kw):
                out.write_bytes(b"MZ")
                out.with_suffix(".map").write_text("")
                return ""

            with mock.patch.object(gl, "winepath", str), \
                    mock.patch.object(implib, "ensure_all", lambda *a, **k: []), \
                    mock.patch("homm1.tool.link.link", fake_link):
                with contextlib.redirect_stdout(io.StringIO()) as sout:
                    gl.candidate(out, Path(td), explicit=[str(obj)])
        self.assertIn("NO .rsrc", sout.getvalue())


class ImplibScaffoldControls(unittest.TestCase):
    """The synthesis cleaned up `<stem>.exp`, a name link.exe never writes.

    The .exp is named after the /IMPLIB path, and synthesis links through
    `<stem>.lib.tmp`, so the real leftovers were `mss32.lib.exp` and
    `smackw32.lib.exp` - both still sitting in build/lib/ weeks later.
    """

    def test_the_cleanup_names_the_exp_link_actually_writes(self):
        import inspect
        from homm1.graph import implib
        src = inspect.getsource(implib.synthesize)
        self.assertIn('tmp_lib.with_suffix(".exp")', src)

    def test_the_temp_lib_derives_the_observed_exp_name(self):
        from homm1.graph import implib
        lib = Path(implib.OUT_DIR) / "mss32.lib"
        tmp_lib = lib.with_suffix(".lib.tmp")
        self.assertEqual(tmp_lib.with_suffix(".exp").name, "mss32.lib.exp")


class ConfigureWriteControls(unittest.TestCase):
    """`homm1 configure --out <unwritable>` raised FileNotFoundError from the
    manifest mkdir instead of naming the path it could not write."""

    def test_an_unwritable_manifest_path_is_a_message(self):
        import contextlib
        import io
        from homm1.graph import emit
        with contextlib.redirect_stderr(io.StringIO()) as err:
            rc = emit.main(["--out", "/proc/no/such/dir/build.ninja"])
        self.assertEqual(rc, 1)
        self.assertIn("cannot write", err.getvalue())


class CompdbStalenessControls(unittest.TestCase):
    """`--check` answered "coverage: 300/300" for a database whose toolchain
    include dirs no longer exist. A toolchain re-pin moves $MSVC_DIR and no
    ninja edge can see it, so the stored /nix/store path outlives its store
    entry; every clang consumer then silently loses its headers."""

    def test_a_dead_include_dir_is_reported(self):
        from homm1.graph import compdb
        db = {"/x/a.cpp": ["/imsvc", "/nix/store/gone-toolchain/msvc/include",
                           "/I", "/nix/store/gone-toolchain/dx/Include"]}
        self.assertEqual(
            compdb.dead_include_dirs(db),
            ["/nix/store/gone-toolchain/dx/Include",
             "/nix/store/gone-toolchain/msvc/include"])

    def test_live_dirs_are_not_reported(self):
        from homm1.graph import compdb
        with tempfile.TemporaryDirectory() as td:
            self.assertEqual(compdb.dead_include_dirs({"x": ["/imsvc", td]}), [])

    def test_a_flag_without_its_operand_is_not_a_crash(self):
        from homm1.graph import compdb
        self.assertEqual(compdb.dead_include_dirs({"x": ["/imsvc"]}), [])


class DecodeNormalizationControls(unittest.TestCase):
    """`semdiff._decode` is the substrate under every paired sieve, and it owns
    the two normalizations none of them can do afterwards: the function's own
    jump/index TABLE is data, and a relocation naming the function ITSELF on a
    real instruction is a self-transfer the delinked side does not relocate at
    all.  The second one was found by the exact-row reflexivity control, where
    two byte-identical recursive functions read as a `selection` residual."""

    #: `call rel32` (self-relocated at +1), `ret`, two `nop`, then two
    #: self-relocated dwords - a jump table objdump decodes as instructions
    BODY = bytes([0xe8, 0, 0, 0, 0, 0xc3, 0x90, 0x90,
                  0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88])
    REL = {1: ("Self", 0), 8: ("Self", 0), 12: ("Self", 0)}

    def test_without_the_name_the_table_decodes_as_three_instructions(self):
        """The negative control: one of them is a `ja`, which is exactly the
        phantom condition code a branch census would count."""
        from homm1.walls.semdiff import _decode
        lines = _decode(self.BODY, self.REL)
        self.assertEqual([x.addr for x in lines], [0, 5, 6, 7, 8, 0xa, 0xe])
        self.assertTrue(any(x.asm.startswith("ja ") for x in lines))
        self.assertEqual(lines[0].ref, "Self")

    def test_the_table_is_dropped_and_the_self_call_keeps_its_instruction(self):
        from homm1.walls.semdiff import _decode
        lines = _decode(self.BODY, self.REL, "Self")
        self.assertEqual([x.addr for x in lines], [0, 5, 6, 7])
        self.assertTrue(lines[0].asm.startswith("call "))
        # the referent means "here" and only our own object spells it
        self.assertIsNone(lines[0].ref)


class SemDiffControls(unittest.TestCase):
    """`walls semdiff` screens a pair at OPERAND level, so its filters ARE the
    tool: a filter that stops working turns every scheduling artifact into a
    reported bug and the sweep becomes unreadable.  A screen nobody has seen
    both PASS a known-equal pair and FLAG a real swap is not a screen."""

    @staticmethod
    def _lines(asms, ref=None):
        from homm1.walls.semdiff import Line
        return [Line(i * 4, a, ref) for i, a in enumerate(asms)]

    def test_a_known_equal_pair_screens_clean(self):
        """Register mirror, frame size, cross-jump duplication and a byte-
        continuation line - four observed false-positive classes at once."""
        from homm1.walls.semdiff import exclusive, features
        base = self._lines([
            "sub esp,0xc", "mov eax,DWORD PTR [esi+0x1ec]",
            "and ecx,0xffffffe0", "mov DWORD PTR [esi+0x2f0],eax",
            "call 0x1234", "ret 0x10",
        ])
        target = self._lines([
            "sub esp,0x10", "mov edx,DWORD PTR [esi+0x1ec]",
            "and al,0xffffffe0", "mov DWORD PTR [esi+0x2f0],edx",
            "call 0x1234", "83 c4 10", "call 0x1234", "ret 0x10",
        ])
        fb, ft = features(base), features(target)
        self.assertEqual(exclusive(fb, ft), [])          # nothing reported
        self.assertEqual(fb["disp"]["+0x1ec"], ft["disp"]["+0x1ec"])

    def test_a_swapped_member_is_flagged(self):
        from homm1.walls.semdiff import exclusive, features
        base = self._lines(["mov eax,DWORD PTR [esi+0x1f0]"])
        target = self._lines(["mov eax,DWORD PTR [esi+0x1ec]"])
        keys = {(k, key) for k, key, _u, _v in exclusive(features(base),
                                                         features(target))}
        self.assertIn(("disp", "+0x1f0"), keys)
        self.assertIn(("disp", "+0x1ec"), keys)

    def test_a_dropped_conversion_is_flagged(self):
        """cl never adds or drops an fild to schedule - the projectile
        spawn-coordinate bug was exactly this delta."""
        from homm1.walls.semdiff import features
        fb = features(self._lines(["fld QWORD PTR [esi+0x8]"]))
        ft = features(self._lines(["fild DWORD PTR [esi+0x5c]",
                                   "fld QWORD PTR [esi+0x8]"]))
        self.assertEqual(fb["fp"]["fild"], 0)
        self.assertEqual(ft["fp"]["fild"], 1)

    def test_a_jump_table_never_reaches_the_multisets(self):
        """A function's own index table decodes as junk with huge
        displacements; counting it swamps every real key."""
        from homm1.walls.semdiff import features
        me = "?Fn@C@@QAEHXZ"
        junk = self._lines(["mov cl,BYTE PTR [eax+0xd1c]",
                            "add BYTE PTR [edx-0x70ffffff],bl"], ref=me)
        self.assertEqual(sum(features(junk, me)["disp"].values()), 0)
        self.assertEqual(sum(features(junk, "?Other@@QAEXXZ")["disp"].values()),
                         2)

    def test_the_referent_sequence_sees_a_masked_swap(self):
        """Relocated operands are masked in the scored bytes, so a swapped
        pair of string keys is invisible to every value-level multiset."""
        from homm1.walls.semdiff import Line, referent_runs
        b = [Line(0, "push 0x0", "??_C@_0A@AAA@KEY_A"),
             Line(4, "push 0x0", "??_C@_0A@BBB@KEY_B")]
        t = [Line(0, "push 0x0", "??_C@_0A@BBB@KEY_B"),
             Line(4, "push 0x0", "??_C@_0A@AAA@KEY_A")]
        self.assertNotEqual(referent_runs(b), referent_runs(t))


class EhRegistrationRenameControls(unittest.TestCase):
    """`_eh_funclet_owners` renames a delinked `push <undefined FUN_<rva>>` to
    `__ehreg$<owner>` so the two sides co-name their EH machinery.  On the
    delinked side that is the ONLY structure available, and a
    `push <$E atexit thunk>; call _atexit` has exactly the same shape - it was
    renamed on 12 sites in the tree, asserting a registration stub that does
    not exist (`0x153800` is `mov ecx,&clip; jmp ~CResolveNode`).  The
    discriminator is the instruction that makes a pushed record ACTIVE."""

    def test_the_registration_prologue_is_recognized(self):
        from homm1.compare.canonicalize import _installs_seh_frame
        # push <stub> / mov eax,fs:[0] / push eax / mov fs:[0],esp
        body = bytes.fromhex("64a10000000050" "64892500000000")
        self.assertTrue(_installs_seh_frame(body, 0))

    def test_the_interleaved_registration_prologue_is_recognized(self):
        """363 of the 911 real sites load `fs:[0]` BEFORE the push, so the
        window has to start at `push eax`, not at the `fs` prefix."""
        from homm1.compare.canonicalize import _installs_seh_frame
        self.assertTrue(_installs_seh_frame(
            bytes.fromhex("50" "64892500000000"), 0))

    def test_an_atexit_thunk_push_is_rejected(self):
        from homm1.compare.canonicalize import _installs_seh_frame
        # push <$E thunk> / call _atexit / add esp,4 / mov edx,[..]
        self.assertFalse(_installs_seh_frame(
            bytes.fromhex("e80000000083c4048b150000000000000000000000000000"), 0))

    def test_a_static_guard_push_is_rejected(self):
        from homm1.compare.canonicalize import _installs_seh_frame
        # the CButeMgr shape: or dl,al / mov [guard],... then the atexit call
        self.assertFalse(_installs_seh_frame(
            bytes.fromhex("0ad0c705000000000000000000000000000000000000"), 0))


class ValueTempLivenessControls(unittest.TestCase):
    """`walls valuetemp` finds an inlined accessor's by-value struct temp by the
    DEAD stores it leaves, so its whole result rests on the liveness rule.  A
    whole-function read SET gets that wrong in both directions, and both errors
    are silent: an address-taken aggregate names only its BASE, so its interior
    fields read as dead (a fabricated hit), while a slot overwritten before its
    address is taken reads as live (a missed hit).  Only the EVENT ORDER on the
    slot separates them, and both directions are controlled here."""

    @staticmethod
    def _ins(*asm):
        """One instruction per 4 bytes; `temps` reads offsets only for order."""
        out = []
        for i, text in enumerate(asm):
            mn, _, ops = text.partition(" ")
            out.append((i * 4, mn, ops))
        return out

    def test_the_overwritten_temp_is_dead_even_though_the_slot_escapes(self):
        """The known positive (CGrunt::RectContains).  Retail materialises the
        by-value Coord, then overwrites BOTH halves with the real value and
        takes the slot's address - so the temp is dead and the `lea` observes
        only its successor."""
        from homm1.walls.valuetemp import temps
        self.assertEqual(temps(self._ins(
            "mov edx,DWORD PTR [ecx+0x180]", "mov DWORD PTR [esp+0x14],edx",
            "mov edx,DWORD PTR [ecx+0x17c]", "mov DWORD PTR [esp+0x10],edx",
            "mov eax,DWORD PTR [ecx+0x184]", "mov DWORD PTR [esp+0x10],eax",
            "mov eax,DWORD PTR [ecx+0x188]", "mov DWORD PTR [esp+0x14],eax",
            "lea edx,[esp+0x10]", "push edx")), {("mem", 0x17C)})

    def test_an_address_taken_aggregates_interior_fields_are_not_dead(self):
        """The known negative (CBattlezMapConfig::ScanRegion).  Two adjacent
        RECTs are built and one is pushed by address; the second RECT's
        right/bottom stores are fed by an adjacent member pair and are never
        named again, which is exactly the temp's signature - but the `lea`
        covering their object observes them."""
        from homm1.walls.valuetemp import temps
        self.assertEqual(temps(self._ins(
            "mov ecx,DWORD PTR [eax+0x10]", "mov edx,DWORD PTR [eax+0xc]",
            "mov DWORD PTR [esp+0x64],ecx", "mov DWORD PTR [esp+0x60],edx",
            "lea eax,[esp+0x58]", "push eax")), set())

    def test_a_pair_nothing_ever_reads_is_dead(self):
        """The second form the mechanism produces: no killing store, no read."""
        from homm1.walls.valuetemp import temps
        self.assertEqual(temps(self._ins(
            "mov eax,DWORD PTR [esi+0x38]", "mov DWORD PTR [esp+0x20],eax",
            "mov ecx,DWORD PTR [esi+0x3c]", "mov DWORD PTR [esp+0x24],ecx",
            "ret")), {("mem", 0x38)})

    def test_a_push_between_the_store_and_its_read_is_not_a_second_slot(self):
        """ESP tracking.  After a `push`, the SAME slot is spelled +4 higher;
        an untracked scan reads that as a different slot and calls the store
        dead."""
        from homm1.walls.valuetemp import temps
        self.assertEqual(temps(self._ins(
            "mov eax,DWORD PTR [esi+0x8]", "mov DWORD PTR [esp+0x10],eax",
            "mov ecx,DWORD PTR [esi+0xc]", "mov DWORD PTR [esp+0x14],ecx",
            "push ebx", "mov edx,DWORD PTR [esp+0x14]")), set())

    def test_a_call_restores_the_frame_level(self):
        """The argument pushes are gone once the call returns - the callee pops
        them under `__thiscall`/`__stdcall`.  Carrying them forward drifts the
        delta upward for the rest of the body, so the SAME slot read after the
        call scores as a different one and the pair reads dead."""
        from homm1.walls.valuetemp import temps
        self.assertEqual(temps(self._ins(
            "sub esp,0x10", "push ebx",
            "mov eax,DWORD PTR [esi+0x8]", "mov DWORD PTR [esp+0x4],eax",
            "mov ecx,DWORD PTR [esi+0xc]", "mov DWORD PTR [esp+0x8],ecx",
            "push 0x1", "push 0x2", "call 0x0",
            "mov edx,DWORD PTR [esp+0x4]")), set())

    def test_the_frame_level_counts_saves_that_follow_sub_esp(self):
        """cl 5.0 puts `sub esp,N` on either side of the callee-save pushes, so
        the prologue cannot be cut at the first non-push; an ARGUMENT push ends
        it, and only ebx/ebp/esi/edi are saved, each once."""
        from homm1.walls.valuetemp import _frame_level
        self.assertEqual(_frame_level(self._ins(
            "sub esp,0x14", "mov eax,DWORD PTR [esp+0x18]", "push ebx",
            "push ebp", "lea eax,[eax+eax*2]", "push esi", "push edi",
            "mov DWORD PTR [esp+0x14],ecx", "je 0x0")), 0x24)

    def test_an_argument_push_ends_the_prologue(self):
        from homm1.walls.valuetemp import _frame_level
        self.assertEqual(_frame_level(self._ins(
            "sub esp,0x10", "push ebx", "push 0x1", "push 0x2",
            "call 0x0")), 0x14)

    def test_a_reused_slot_does_not_lend_its_source_to_an_earlier_store(self):
        """Deadness belongs to ONE STORE.  cl reuses a slot, so taking the
        slot's first store for the verdict and its first MEMBER-sourced store
        for the identity reports a member pair that was never dead - the
        surviving stores here are the ones the `lea` keeps live."""
        from homm1.walls.valuetemp import temps
        self.assertEqual(temps(self._ins(
            "xor eax,eax", "mov DWORD PTR [esp+0x20],eax",
            "xor ecx,ecx", "mov DWORD PTR [esp+0x24],ecx",
            "mov edx,DWORD PTR [esi+0x8]", "mov DWORD PTR [esp+0x20],edx",
            "mov ebx,DWORD PTR [esi+0xc]", "mov DWORD PTR [esp+0x24],ebx",
            "lea eax,[esp+0x20]", "push eax", "call 0x0")), set())

    def test_a_local_aggregate_copy_is_counted_not_compared(self):
        """A pair read from another FRAME slot is keyed on a frame offset, which
        the two sides do not agree on.  The identical copy at two different
        frame offsets read as an asymmetry in BOTH directions at once, so these
        never enter the comparison."""
        from homm1.walls.valuetemp import _pairs
        out, prov, local = _pairs(self._ins(
            "mov ecx,DWORD PTR [esp+0x40]", "mov edx,DWORD PTR [esp+0x44]",
            "mov DWORD PTR [esp+0x20],ecx", "mov DWORD PTR [esp+0x24],edx",
            "ret"))
        self.assertEqual((out, prov, local), (set(), {}, 1))

    def test_a_pointer_read_from_a_global_is_tagged_as_one(self):
        """The second mechanism the sieve reports: a load through a pointer read
        from a global blocks the dead-store elimination the same load through a
        pointer parameter allows, so the row has to say which it was."""
        from homm1.walls.valuetemp import _pairs
        _out, prov, _local = _pairs(self._ins(
            "mov eax,ds:0x0", "mov esi,DWORD PTR [eax+0x4]",
            "mov ecx,DWORD PTR [esi+0x8]", "mov DWORD PTR [esp+0x20],ecx",
            "mov edx,DWORD PTR [esi+0xc]", "mov DWORD PTR [esp+0x24],edx",
            "ret"))
        self.assertEqual(prov, {("mem", 0x8): "glob"})

    def test_a_pointer_taken_from_an_argument_is_not_a_global(self):
        from homm1.walls.valuetemp import _pairs
        _out, prov, _local = _pairs(self._ins(
            "sub esp,0x10", "mov esi,DWORD PTR [esp+0x14]",
            "mov ecx,DWORD PTR [esi+0x8]", "mov DWORD PTR [esp+0x0],ecx",
            "mov edx,DWORD PTR [esi+0xc]", "mov DWORD PTR [esp+0x4],edx",
            "ret"))
        self.assertEqual(prov, {("mem", 0x8): "param"})

    def test_the_gx_preamble_pushes_are_frame(self):
        """A /GX function pushes -1, the handler and the old fs:0 chain before
        anything else, and cl 5.0 gives it no ebp frame - so the callee-save
        rule alone cuts its prologue at the first instruction and reports a
        level of zero.  The registration install ends the preamble."""
        from homm1.walls.valuetemp import _frame_level
        self.assertEqual(_frame_level(self._ins(
            "push 0xffffffff", "push 0x0", "mov eax,fs:0x0", "push eax",
            "mov DWORD PTR fs:0x0,esp", "sub esp,0x5c", "push ebx", "push ebp",
            "push esi", "mov esi,ecx", "je 0x0")), 0x74)


class MatchReferenceControls(unittest.TestCase):
    """`homm1 match --reference <bad path>` raised FileNotFoundError AFTER a
    full build - the work was done and the run ended in a traceback."""

    def test_an_unreadable_reference_is_a_message(self):
        import contextlib
        import io
        from homm1.graph import verbs
        with tempfile.TemporaryDirectory() as td:
            missing = Path(td) / "nope.json"
            with mock.patch.object(verbs, "configure_if_needed", lambda *a, **k: None), \
                    mock.patch.object(verbs, "ninja", lambda *a, **k: 0), \
                    mock.patch.object(verbs, "object_census", dict), \
                    mock.patch("homm1.tool.objdiff.load",
                               lambda p: (_ for _ in ()).throw(FileNotFoundError(p))
                               if str(p) == str(missing) else {"measures": {},
                                                               "units": []}), \
                    mock.patch("homm1.compare.run.print_summary",
                               lambda *a, **k: None), \
                    mock.patch.object(Path, "exists", lambda self: True):
                with contextlib.redirect_stderr(io.StringIO()) as err:
                    with contextlib.redirect_stdout(io.StringIO()):
                        rc = verbs.match_main(["--reference", str(missing)])
        self.assertEqual(rc, 2)
        self.assertIn("--reference", err.getvalue())


class ToolchainIsADeclaredInput(unittest.TestCase):
    """The era toolchain and the delinker used to be pure environment.

    Consequence chain, all silent: re-pinning $MSVC_DIR recompiled only the
    units that happened to be dirty and left the rest built by the previous
    cl (a mixed object set, the worst failure a byte-matching project has),
    swapping the delinker gave `ninja: no work to do`, and because the `rc`
    edge only exists when rc.exe was present at CONFIGURE time, a pre-r3 pin
    silently produced a candidate image with no `.rsrc` at all.
    """

    def test_the_identity_names_all_three_inputs(self):
        from homm1.graph.emit import toolchain_id
        with mock.patch.dict(os.environ, {"MSVC_DIR": "/m", "DXSDK_DIR": "/d"}):
            got = toolchain_id()
        self.assertIn("MSVC_DIR=/m", got)
        self.assertIn("DXSDK_DIR=/d", got)
        self.assertIn("delinker=", got)

    def test_an_unset_variable_is_recorded_not_skipped(self):
        """Unset -> set is itself a change the edges must see."""
        from homm1.graph.emit import toolchain_id
        with mock.patch.dict(os.environ, {"DXSDK_DIR": "/d"}, clear=True):
            got = toolchain_id()
        self.assertIn("MSVC_DIR=-", got)

    def test_the_id_is_written_if_changed(self):
        """It is an input of all 300 cl edges: an unconditional rewrite would
        recompile the tree every time anything else re-ran configure."""
        from homm1.graph.emit import write_toolchain_id
        with tempfile.TemporaryDirectory() as td:
            p = Path(td) / "toolchain.id"
            self.assertTrue(write_toolchain_id(p))     # created
            before = p.stat().st_mtime_ns
            self.assertFalse(write_toolchain_id(p))    # unchanged
            self.assertEqual(p.stat().st_mtime_ns, before)
            p.write_text("MSVC_DIR=/elsewhere\n")
            self.assertTrue(write_toolchain_id(p))     # moved

    def test_a_repin_is_detected(self):
        from homm1.graph import verbs
        from homm1.graph.emit import toolchain_id
        with tempfile.TemporaryDirectory() as td:
            p = Path(td) / "toolchain.id"
            with mock.patch.dict(os.environ, {"MSVC_DIR": "/one"}):
                p.write_text(toolchain_id())
                with mock.patch.object(verbs, "REPO", Path(td)), \
                        mock.patch.object(verbs.graph, "TOOLCHAIN_ID", "toolchain.id"):
                    self.assertFalse(verbs.toolchain_repinned())
                    with mock.patch.dict(os.environ, {"MSVC_DIR": "/two"}):
                        self.assertTrue(verbs.toolchain_repinned())

    def test_the_emitted_manifest_declares_it_on_the_cl_edges(self):
        """The integration control. Recognising the file is not the claim -
        the claim is that the COMPILE edges depend on it, which is what makes
        a re-pin invalidate the object set."""
        from homm1 import graph
        from homm1.core.paths import REPO
        ninja = (REPO / graph.NINJA)
        if not ninja.exists():
            self.skipTest("no emitted manifest (run `homm1 configure`)")
        text = ninja.read_text()
        self.assertIn(graph.TOOLCHAIN_ID, text)
        cl_edges = [ln for ln in text.splitlines()
                    if ln.startswith("build build/objdiff/base/") and ": cl " in ln]
        self.assertTrue(cl_edges, "no cl edges in the manifest")
        # Edge lines are wrapped by the writer, so join the continuations.
        joined, buf = [], ""
        for ln in text.splitlines():
            buf += ln.rstrip("$")
            if not ln.endswith("$"):
                joined.append(buf); buf = ""
        declaring = [ln for ln in joined
                     if ln.startswith("build build/objdiff/base/")
                     and ": cl " in ln and graph.TOOLCHAIN_ID in ln]
        self.assertEqual(len(declaring), len(cl_edges),
                         "every cl edge must declare the toolchain identity")


def _asm_lines(asms, refs=None):
    """One instruction per 4 bytes, so a branch operand `0xN` names the
    instruction at index N/4."""
    from homm1.walls.semdiff import Line
    refs = refs or {}
    return [Line(i * 4, a, refs.get(i)) for i, a in enumerate(asms)]


class ByValueAggregateControls(unittest.TestCase):
    """`walls aggscan` says a callee's parameter is ONE object where we
    declared scalars.  cl 5.0 hands an aggregate wider than a register by
    opening a hole and copying into it, and four `push`es is a different
    shape - which is how `AddToList3` was caught taking a level record's rect
    as four `i32`."""

    @staticmethod
    def _holes(hexbytes, rel=None):
        from homm1.walls.aggscan import holes
        payload = bytes.fromhex(hexbytes)
        return holes(payload, 0, len(payload), rel or {})

    #: sub esp,0x10 / mov ecx,esp / mov [ecx],0 / mov [ecx+4],eax
    #: / mov [ecx+8],eax / mov [ecx+0xc],eax / call / ret
    BLOCK = "83ec108bccc70100000000894104894108894 10ce800000000c3".replace(" ", "")

    def test_a_by_value_block_fires(self):
        self.assertEqual([n for _o, n, _r, _c in self._holes(self.BLOCK)], [16])

    def test_four_pushes_of_four_scalars_are_silent(self):
        """The whole signature question: this is the shape a wrong declaration
        produces, and it must not read as a block."""
        self.assertEqual(self._holes("6a006a006a006a00e800000000c3"), [])

    def test_a_reservation_nothing_is_copied_into_is_the_frame(self):
        """THE headline control: the prologue reservation is the frame.
        Testing for the COPIES rather than for position is what let the sweep
        keep six callees whose only block sits before the first call."""
        self.assertEqual(self._holes("83ec108bcc33c0e800000000c3"), [])

    def test_a_non_dword_reservation_is_a_byte_misalignment(self):
        """`83 EC 02` occurs inside the displacement of
        `mov DWORD PTR [ebx+0x2ec],0x1f401`.  cl never moves ESP by a
        non-dword amount, so the filter costs nothing."""
        self.assertEqual(self._holes("c783ec02000001f40100c3"), [])

    def test_the_callee_is_read_through_an_unconditional_jmp(self):
        """cl TAIL-MERGES the argument build: predecessors fill the hole and
        jump to one shared call.  Stopping at the first `call` byte named
        `CTriggerMgr::CellDispatch` for two blocks that jump to
        `CGrunt::PlaySound`."""
        from homm1.walls.aggscan import callee
        # jmp +5 over a junk `call` byte, landing on the real call
        payload = bytes.fromhex("eb05") + b"\xe8\0\0\0\0" + b"\xe8\0\0\0\0\xc3"
        self.assertEqual(callee(payload, 0, len(payload), {8: ("?Real@@YAXXZ", 20)}),
                         "?Real@@YAXXZ")


class MaxGateClassificationControls(unittest.TestCase):
    """MAX belongs to a function's own source hash, so only an edit can lower
    it. An unedited CUR dip must never fail the gate: it used to, and that
    pushed workers away from correct header changes."""

    def _kinds(self, pct, prev_cur, prev_fp, cur_fp):
        from homm1.verify import classify as cl
        base = {("u", "f"): {"best": 100.0, "cur": prev_cur, "fp": prev_fp,
                             "addr": None, "hist": 100.0, "tries": 1,
                             "state": ""}}
        return [k for k, *_ in cl.classify({("u", "f"): pct}, base,
                                           lambda *_: cur_fp, {})]

    def test_an_unedited_dip_is_not_a_regression(self):
        self.assertEqual(self._kinds(89.5, 100.0, "aaaa", "aaaa"), ["DIP"])

    def test_an_edit_that_lowers_cur_is_a_regression(self):
        self.assertEqual(self._kinds(89.5, 100.0, "aaaa", "bbbb"), ["REGRESS"])

    def test_an_edit_that_keeps_cur_is_a_reset(self):
        self.assertEqual(self._kinds(97.4, 97.4, "aaaa", "bbbb"), ["RESET"])

    def test_a_changed_fallback_fingerprint_counts_as_an_edit(self):
        from homm1.verify.fingerprints import FALLBACK
        self.assertEqual(
            self._kinds(89.5, 100.0, FALLBACK + "1", FALLBACK + "2"), ["REGRESS"])

    def test_only_regress_fails_the_gate(self):
        from homm1.verify import classify as cl
        base = {("u", k): {"best": 100.0, "cur": c, "fp": "a", "addr": None,
                           "hist": 100.0, "tries": 1, "state": ""}
                for k, c in (("dip", 100.0), ("reset", 97.0), ("drop", 100.0))}
        cur = {("u", "dip"): 90.0, ("u", "reset"): 97.0, ("u", "drop"): 90.0}
        fps = {("u", "dip"): "a", ("u", "reset"): "b", ("u", "drop"): "b"}
        buckets = cl.buckets_of(cur, base, lambda *k: fps[k], {})
        regress = buckets.get("REGRESS", [])
        self.assertEqual([r[1] for r in regress], ["drop"])
        self.assertEqual([r[1] for r in cl.fresh_regressions(cur, base, regress)],
                         ["drop"])


# --------------------------------------------------------------------------- #
# data_matching (config/compare.toml): gate verdicts and ledger modes        #
# --------------------------------------------------------------------------- #
def _relaxed(on: bool):
    """Pin the data-matching switch for one control."""
    return mock.patch("homm1.core.data_matching.enabled", return_value=on)


class UndefinedClosureDataMatchingControls(unittest.TestCase):
    """A placeholder `extern` is listed debt while data matching is off and a
    FAILURE while it is on - the switch itself is the re-enable worklist."""

    DEBT = {"_g_owned": {"units": ["party"], "reason": "undefined"}}

    def _verdict(self, on, debt=None, hard=()):
        from homm1.verify import undefined_closure as uc
        with tempfile.TemporaryDirectory() as td:
            Path(td, "probe.obj").write_bytes(b"")
            with _relaxed(on), mock.patch.object(uc, "BASE", Path(td)), \
                 mock.patch.object(uc, "placeholder_externs",
                                   return_value=dict(self.DEBT if debt is None
                                                     else debt)), \
                 mock.patch.object(uc, "_closure_findings",
                                   return_value=list(hard)):
                return uc.gate_verdict(), uc.gate_findings()

    def test_undefined_closure_placeholder_extern_fails_when_strict(self):
        (hard, advisory), findings = self._verdict(True)
        self.assertEqual(len(hard), 1)
        self.assertIn("_g_owned", hard[0])
        self.assertEqual(advisory, [])
        self.assertEqual(findings, hard)

    def test_undefined_closure_placeholder_extern_is_listed_when_relaxed(self):
        (hard, advisory), findings = self._verdict(False)
        self.assertEqual(hard, [])
        self.assertEqual(findings, [])
        self.assertEqual(len(advisory), 1)
        self.assertIn("_g_owned", advisory[0])

    def test_undefined_closure_other_findings_fail_in_both_modes(self):
        for on in (True, False):
            (hard, _adv), _f = self._verdict(on, debt={},
                                             hard=["pure-phantom class CX"])
            self.assertEqual(hard, ["pure-phantom class CX"])

    def test_undefined_closure_classifies_placeholder_externs(self):
        from homm1.verify import undefined_closure as uc
        refs = {"_g_placeholder": {"a"}, "_g_owned": {"a"},
                "_g_unclaimed": {"b"}, "_g_provided": {"a"},
                "__imp__GetTickCount@4": {"a"}, "__iob": {"b"}}
        defined = {"_g_owned", "_g_unclaimed"}
        with mock.patch.object(uc, "_data_externs",
                               return_value=(refs, defined)), \
             mock.patch.object(uc, "_data_claims",
                               return_value=({"_g_owned"}, {"_g_provided"})):
            debt = uc.placeholder_externs(paths=[], libs={"__iob"})
        self.assertEqual(debt, {
            "_g_placeholder": {"units": ["a"], "reason": "undefined"},
            "_g_unclaimed": {"units": ["b"], "reason": "unclaimed"}})

    def test_undefined_closure_reads_only_data_externs_from_objects(self):
        from homm1.compare.test_normalize import coff
        from homm1.verify import undefined_closure as uc
        symbols = [("_Func", 0, 1, 0x20, 2), ("_g_data", 0, 0, 0x00, 2),
                   ("_Callee", 0, 0, 0x20, 2), ("_g_common", 4, 0, 0x00, 2),
                   ("_g_defined", 0, 2, 0x00, 2)]
        obj = coff(bytes(8), [], bytes(4), [], symbols)
        with tempfile.TemporaryDirectory() as td:
            path = Path(td, "unit.obj")
            path.write_bytes(obj)
            refs, defined = uc._data_externs([path])
        self.assertEqual(dict(refs), {"_g_data": {"unit"}})
        self.assertTrue({"_Func", "_g_common", "_g_defined"} <= defined)

    def test_undefined_closure_leaves_data_externs_out_of_declared_only(self):
        from homm1.verify import undefined_closure as uc
        with mock.patch.object(uc, "live_base_objs", return_value=["x"]), \
             mock.patch.object(uc, "_sym_sets",
                               side_effect=[(set(), {"?g_x@@3HA"}),
                                            (set(), set())]), \
             mock.patch.object(uc, "_data_externs",
                               return_value=({"?g_x@@3HA": {"x"}}, set())), \
             mock.patch.object(uc, "lib_symbols", return_value=set()), \
             mock.patch.object(uc, "_rtti_classes", return_value=set()), \
             mock.patch.object(uc, "source_library_shadows", return_value=[]):
            _p, _s, declared = uc.analyse()
        self.assertEqual(declared, set())


class DataIdentityControls(unittest.TestCase):
    """Two externs naming one retail global (0x919e0 was `g_hoveredObjectId`
    in one lane and `g_lastMenuItem` in another) are invisible to objdiff
    while data relocations are relaxed. The gate reads them pre-relaxation
    and fails in BOTH modes."""

    def _gate(self, sites, *, paired=1, base_objs=True):
        from homm1.verify import data_identity as di
        from homm1.verify import tiers

        def scan(stats):
            stats["functions paired"] += paired
            return sites, {}, {}
        gate = dict(tiers.TIERS["normal"])["data-identity"]
        verdicts = {}
        with tempfile.TemporaryDirectory() as td:
            if base_objs:
                Path(td, "probe.obj").write_bytes(b"")
            with mock.patch("homm1.verify.undefined_closure.BASE", Path(td)), \
                 mock.patch.object(di, "scan", side_effect=scan), \
                 mock.patch.object(di, "TSV", Path(td, "data_identity.tsv")):
                for on in (True, False):
                    with _relaxed(on):
                        verdicts[on] = gate()
        return verdicts

    @staticmethod
    def _sites(menu_symbol):
        from homm1.verify.test_data_identity import sites, unit
        return (sites("mouse", unit([("_GetA", [("_g_hoveredObjectId", 0)])]),
                      unit([("_GetA", [("DAT_004919e0", 0)])]),
                      {("_GetA", 1): 0x919e0})
                + sites("menuresult", unit([("_GetB", [(menu_symbol, 0)])]),
                        unit([("_GetB", [("DAT_004919e0", 0)])]),
                        {("_GetB", 1): 0x919e0}))

    def test_data_identity_duplicate_extern_fails_in_both_modes(self):
        for on, findings in self._gate(self._sites("_g_lastMenuItem")).items():
            self.assertEqual(len(findings), 1, f"data_matching={on}")
            self.assertIn("_g_lastMenuItem", findings[0])

    def test_data_identity_one_extern_passes(self):
        for findings in self._gate(self._sites("_g_hoveredObjectId")).values():
            self.assertEqual(findings, [])

    def test_data_identity_never_passes_vacuously(self):
        for findings in self._gate([], base_objs=False).values():
            self.assertIn("no base objs", findings[0])
        for findings in self._gate([], paired=0).values():
            self.assertIn("no function paired", findings[0])

    def test_data_identity_is_not_a_data_matching_gate(self):
        from homm1.verify import tiers
        self.assertIn("data-identity", dict(tiers.TIERS["normal"]))
        self.assertNotIn("data-identity", tiers.DATA_MATCHING_GATES)


class DataPlacementDataMatchingControls(unittest.TestCase):
    """The data identity/placement gates report without failing only while
    data matching is off; the tier runner prints their count either way."""

    GATES = (("data-tu-order", "homm1.verify.data_tu_order.gate_findings"),
             ("data-coverage", "homm1.verify.data_coverage.gate_findings"))

    def _tier_fn(self, label):
        from homm1.verify import tiers
        return dict(tiers.TIERS["normal"])[label]

    def test_data_tu_order_and_data_coverage_follow_the_switch(self):
        from homm1.verify import tiers
        for label, target in self.GATES:
            with mock.patch(target, return_value=["finding"]):
                with _relaxed(True):
                    self.assertEqual(self._tier_fn(label)(), ["finding"])
                with _relaxed(False):
                    self.assertEqual(self._tier_fn(label)(),
                                     tiers.Verdict([], ["finding"]))

    def test_data_relocs_keeps_its_integrity_rows_failing(self):
        from homm1.verify import tiers
        parts = (["WRONG referent"], ["live unit 'x' has no scored target"])
        with mock.patch("homm1.verify.data_relocs.gate_parts",
                        return_value=parts):
            with _relaxed(True):
                self.assertEqual(self._tier_fn("data-relocs")(),
                                 parts[0] + parts[1])
            with _relaxed(False):
                self.assertEqual(self._tier_fn("data-relocs")(),
                                 tiers.Verdict(parts[1], parts[0]))

    def test_data_access_stays_failing_when_relaxed(self):
        with mock.patch("homm1.verify.data_access.gate_findings",
                        return_value=["data-access: [width] probe"]), \
             _relaxed(False):
            self.assertEqual(self._tier_fn("data-access")(),
                             ["data-access: [width] probe"])

    def test_every_data_matching_gate_is_a_tier_member(self):
        from homm1.verify import tiers
        labels = {n for rows in tiers.TIERS.values() for n, _f in rows}
        self.assertTrue(set(tiers.DATA_MATCHING_GATES) <= labels)

    def test_the_runner_lists_advisory_findings_without_failing(self):
        import contextlib
        import io

        from homm1.verify import tiers
        gates = [("probe", lambda: tiers.Verdict([], ["debt one", "debt two"]))]
        with mock.patch.dict(tiers.TIERS, {"fast": gates}):
            with contextlib.redirect_stdout(io.StringIO()) as out:
                self.assertEqual(tiers.run(["fast"]), 0)
        text = out.getvalue()
        self.assertIn("ADVISORY (2 finding(s)", text)
        self.assertIn("debt two", text)
        gates = [("probe", lambda: tiers.Verdict(["hard"], ["debt"]))]
        with mock.patch.dict(tiers.TIERS, {"fast": gates}):
            with contextlib.redirect_stdout(io.StringIO()) as out:
                self.assertEqual(tiers.run(["fast"]), 1)
        self.assertIn("advisory (1, not failing)", out.getvalue())


class BankDataMatchingModeControls(unittest.TestCase):
    """Scores from the two modes never compare: check and bank refuse a
    mismatch, and the explicit re-base is the only way across."""

    def _row(self, best, cur, hist, state=""):
        return {"best": best, "cur": cur, "tries": 1, "fp": "a",
                "addr": 0x1000, "hist": hist, "state": state}

    def test_the_ledger_records_its_mode_and_a_legacy_ledger_is_strict(self):
        from homm1.verify import baseline as bl
        rows = {("u", "f"): self._row(90.0, 90.0, 95.0)}
        self.assertTrue(bl.load_mode("# old banner\n# [functions]\tunit\n"))
        for on in (True, False):
            text = bl.render(rows, on)
            self.assertEqual(bl.load_mode(text), on)
            self.assertEqual(bl.load(text), rows)      # rows still round-trip

    def test_a_mode_change_is_a_mismatch(self):
        from homm1.verify import baseline as bl
        strict = bl.render({}, True)
        with _relaxed(False):
            self.assertIn("--rebase-data-matching", bl.mode_mismatch(strict))
            self.assertIsNone(bl.mode_mismatch(bl.render({}, False)))
        with _relaxed(True):
            self.assertIsNone(bl.mode_mismatch(strict))

    def test_the_max_gate_refuses_to_compare_across_modes(self):
        import argparse
        import contextlib
        import io

        from homm1.verify import verbs
        args = argparse.Namespace(report=None, strict=False, all=False)
        with mock.patch("homm1.verify.baseline.mode_mismatch",
                        return_value="banked under the other mode"), \
             mock.patch.object(verbs, "load_state") as load:
            with contextlib.redirect_stdout(io.StringIO()) as out:
                self.assertEqual(verbs._report(args, gate=True), 1)
                self.assertEqual(verbs._report(args, gate=False), 0)
        load.assert_not_called()          # no row was compared at all
        self.assertIn("banked under the other mode", out.getvalue())

    def test_bank_refuses_a_mode_change_without_the_rebase_flag(self):
        from homm1.verify import verbs
        with mock.patch("homm1.verify.baseline.mode_mismatch",
                        return_value="banked under the other mode"), \
             mock.patch.object(verbs, "require_bankable_tree") as tree:
            with self.assertRaises(SystemExit) as e:
                verbs.cmd_bank([])
        self.assertIn("refusing to bank", str(e.exception))
        tree.assert_not_called()

    def test_rebase_is_refused_when_the_mode_did_not_change(self):
        from homm1.verify import verbs
        with mock.patch("homm1.verify.baseline.mode_mismatch",
                        return_value=None):
            with self.assertRaises(SystemExit) as e:
                verbs.cmd_bank(["--rebase-data-matching"])
        self.assertIn("refusing to re-base", str(e.exception))

    def test_rebase_sets_max_and_hist_to_cur_and_drops_absent_rows(self):
        from homm1.verify import verbs
        rows = {("u", "rose"): self._row(90.0, 100.0, 95.0),
                ("u", "fell"): self._row(100.0, 97.5, 100.0),
                ("u", "gone"): self._row(100.0, 100.0, 100.0, state="absent")}
        rebased, dropped = verbs.rebase_rows(rows)
        self.assertEqual(dropped, [("u", "gone")])
        for key in (("u", "rose"), ("u", "fell")):
            row = rebased[key]
            self.assertEqual((row["best"], row["hist"]), (row["cur"], row["cur"]))
        # nothing reads as a lost match (HIST > MAX) after the re-base
        self.assertFalse(any(r["hist"] > r["best"] for r in rebased.values()))


class InlineEHControls(unittest.TestCase):
    """VC4 inline cleanup/stub/epilog and the separate packed-band control."""

    @staticmethod
    def fixture():
        from homm1.compare.test_normalize import coff, DIR32, REL32
        text = bytearray.fromhex('558bec6aff680000000064a1000000005064892500000000')
        text += b'\xe9' + bytes(4)
        action = len(text)
        text += bytes.fromhex('8b45fc50e80000000083c404c3')
        stub = len(text)
        text += bytes.fromhex('b800000000e900000000')
        text += bytes.fromhex('8b4df464890d000000005f5e5bc9c20400')
        data = struct.pack('<IiIiIiIIiI', 0x19930520, 1, 0, 0, 0, 0, 0, 0, -1, 0)
        symbols = [('_Owner', 0, 1, 0x20, 2), ('$LReg', stub, 1, 0, 6),
                   ('$LAction', action, 1, 0, 6), ('$TInfo', 0, 2, 0, 3),
                   ('$TMap', 32, 2, 0, 3), ('___CxxFrameHandler', 0, 0, 0x20, 2),
                   ('??3@YAXPAX@Z', 0, 0, 0x20, 2)]
        payload = coff(bytes(text), [(6, 1, DIR32), (action + 5, 6, REL32),
                                    (stub + 1, 3, DIR32), (stub + 6, 5, REL32)],
                       data, [(8, 4, DIR32), (36, 2, DIR32)], symbols)
        return payload, action, stub

    def test_inline_copy_keeps_full_epilog_and_protected_identities(self):
        from homm1.compare import canonicalize as canon, normalize
        payload, action, stub = self.fixture()
        original = canon.CoffObject(payload)
        copied, _ = normalize.comparison_copy(payload, data_matching_on=False)
        result = canon.CoffObject(copied)
        self.assertEqual(result.sections[0].raw_size, original.sections[0].raw_size)
        self.assertEqual(result.section_bytes(result.sections[0])[stub + 10:],
                         original.section_bytes(original.sections[0])[stub + 10:])
        by_site = {(r.section, r.site): r for r in result.relocations}
        self.assertEqual(result.symbols[by_site[1, stub + 1].symbol_index].name,
                         '__ehfuncinfo$_Owner')
        self.assertEqual(result.symbols[by_site[2, 8].symbol_index].name,
                         '__ehunwindmap$_Owner')
        self.assertEqual(result.symbols[by_site[2, 36].symbol_index].name, '_Owner')
        self.assertEqual(struct.unpack_from('<I', result.section_bytes(result.sections[1]), 36)[0], action)
        self.assertEqual(result.symbols[by_site[1, stub + 6].symbol_index].name,
                         '___CxxFrameHandler')

    def test_malformed_funcinfo_is_rejected(self):
        from homm1.compare import canonicalize as canon
        payload, _, _ = self.fixture()
        obj = canon.CoffObject(payload)
        broken = bytearray(payload)
        struct.pack_into('<I', broken, obj.sections[1].raw_offset, 0)
        with self.assertRaisesRegex(ValueError, 'FuncInfo'):
            canon.canonicalize_coff(bytes(broken))

    def test_missing_restoring_return_is_rejected(self):
        from homm1.compare import canonicalize as canon
        payload, _, _ = self.fixture()
        obj = canon.CoffObject(payload)
        broken = bytearray(payload)
        section = obj.sections[0]
        broken[section.raw_offset + section.raw_size - 3:
               section.raw_offset + section.raw_size] = b'\x90' * 3
        with self.assertRaisesRegex(ValueError, 'continuation'):
            canon.canonicalize_coff(bytes(broken))

    def test_epilog_pairs_the_loaded_register_with_fs_restore(self):
        from homm1.delink.eh_band import inline_epilog
        self.assertTrue(inline_epilog(bytes.fromhex('8b4df464890d000000005f5e5bc9c20400')))
        self.assertTrue(inline_epilog(bytes.fromhex('8b45f464a3000000005f5e5bc9c3cc')))
        self.assertFalse(inline_epilog(bytes.fromhex('8b55f464890d000000005f5e5bc9c20400')))
        self.assertFalse(inline_epilog(bytes.fromhex('8b4df464890d000000005f5e5bc9')))

    def test_inline_records_preserve_owner_and_packed_records_remain(self):
        from homm1.delink import eh_band, pdb_synth
        from types import SimpleNamespace
        inline = eh_band.Group(0x1000, '_Owner', 'u', (0x1040,), 0x1050, inline=True)
        packed = eh_band.Group(0x2000, '_Packed', 'u', (0x2080,), 0x2090)
        self.assertEqual(eh_band.records([inline]), [])
        self.assertEqual(len(eh_band.records([packed])), 2)
        model = SimpleNamespace(functions=[SimpleNamespace(rva=0x1000, size=0x80, kind='')])
        with mock.patch.object(pdb_synth, 'retail', return_value=SimpleNamespace(image_base=0x400000)):
            rows = pdb_synth.function_records(model, {0x1000: ('_Owner', 'u', 0x80)},
                                              {}, {}, [], {}, lambda _: None)
        self.assertEqual(rows, [(0x1000, 0x80, '_Owner')])
        self.assertEqual(eh_band.records([packed])[0][1], '__ehunwind$_Packed$0')

    def test_retail_groups_classify_inline_and_packed_from_continuation(self):
        from homm1.compare import canonicalize as canon
        from homm1.delink import eh_band
        from types import SimpleNamespace
        payload, action, stub = self.fixture()
        obj = canon.CoffObject(payload)
        sections = {i + 1: bytearray(obj.section_bytes(section))
                    for i, section in enumerate(obj.sections)}
        bases = {1: 0x1000, 2: 0x3000}
        for relocation in obj.relocations:
            if relocation.typ != canon.DIR32:
                continue
            symbol = obj.symbols[relocation.symbol_index]
            struct.pack_into('<I', sections[relocation.section], relocation.site,
                             0x400000 + bases[symbol.section] + symbol.value)
        def read(rva, size):
            for section, raw in sections.items():
                offset = rva - bases[section]
                if 0 <= offset and offset + size <= len(raw):
                    return bytes(raw[offset:offset + size])
            return None
        image = SimpleNamespace(image_base=0x400000, text_lo=0x1000,
                                text_hi=0x2000, read=read)
        with mock.patch.object(eh_band, '_Image', return_value=image):
            inline = eh_band.groups(Path('unused'), {0x1000: ('_Owner', 'u', len(sections[1]))})
            packed = eh_band.groups(Path('unused'), {0x1000: ('_Owner', 'u', action)})
            self.assertEqual(len(inline), 1)
            self.assertTrue(inline[0].inline)
            self.assertFalse(packed[0].inline)
            self.assertEqual(eh_band.records(inline), [])
            self.assertEqual(len(eh_band.records(packed)), 2)
            sections[1][-3:] = b'\x90' * 3
            with self.assertRaisesRegex(ValueError, 'continuation'):
                eh_band.groups(Path('unused'), {0x1000: ('_Owner', 'u', len(sections[1]))})

    def test_packed_registration_still_uses_separate_owner_name(self):
        from homm1.compare import canonicalize as canon
        from homm1.compare.test_normalize import coff, DIR32
        text = bytes.fromhex('680000000064a1000000005064892500000000c3')
        payload = bytearray(coff(text, [(1, 1, DIR32)], bytes(16), [],
                                 [('_Packed', 0, 1, 0x20, 2), ('$LReg', 6, 2, 0, 6),
                                  ('$LAction', 0, 2, 0, 6)]))
        # Separate executable COMDAT, the donor's existing VC5 representation.
        struct.pack_into('<I', payload, 20 + 40 + 36, 0x60000020)
        obj = canon.CoffObject(bytes(payload))
        names = canon._eh_funclet_owners(obj)
        self.assertEqual(names[1], '__ehreg$_Packed')
        self.assertEqual(names[2], '__ehunwind$_Packed$0')



@logged
def main(argv=None) -> int:
    import argparse
    ap = argparse.ArgumentParser(
        prog="homm1 verify selftest", description=__doc__,
        formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("-v", "--verbose", action="store_true",
                    help="name every control as it runs")
    ap.add_argument("-k", dest="patterns", action="append", default=[],
                    help="run only controls whose name contains this "
                         "substring (repeatable; comma-separated also works)")
    a = ap.parse_args(list(argv or []))
    verbosity = 2 if a.verbose else 1
    loader = unittest.TestLoader()
    wanted = [p.strip() for spec in a.patterns for p in spec.split(",")
              if p.strip()]
    if wanted:
        loader.testNamePatterns = [f"*{p}*" for p in wanted]
    suite = loader.loadTestsFromModule(sys.modules[__name__])
    runner = unittest.TextTestRunner(verbosity=verbosity)
    result = runner.run(suite)
    return 0 if result.wasSuccessful() else 1


if __name__ == "__main__":
    raise SystemExit(main(sys.argv[1:]))

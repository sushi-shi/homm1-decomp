# Tooling inheritance review — 2026-09-19

The original port preserved a selected Gruntz pipeline, not the combined
Gruntz/HoMM2 tooling contract. The prior documentation's claim that only
target inputs changed was too strong. This review separates measured module
inheritance from capabilities that still need restoration and validation.

## Evidence and repeatability

- Gruntz donor: `b1de0e555576a215898907b8ec8ed5423368883e`, the originally
  documented pin. The audit reads committed blobs, not that checkout's dirty
  source tree.
- HoMM2 comparison: Buka checkout at
  `299514f88900c0cf30ba03422c72830a38fc1cb7`; `scripts/homm2/cli.py` and
  `scripts/homm2/analysis/sema.py` had no local changes when reviewed.
- Run `homm1 audit tooling --json` for all 172 Python modules in the Gruntz
  package. The checked-in [inventory](../evidence/tooling-inheritance.json)
  records 74 equivalent ASTs after package/macro renaming, 37 adapted modules,
  and 61 absent paths after the repairs below. Logging decorators and
  docstrings are ignored by this structural comparison. Equivalent ASTs can
  still depend on missing target data; adapted modules require behavioral
  review. Neither category is a correctness certificate.

The HoMM2 review compares the command surface and the specific capabilities
below. It is not a line-by-line audit of the entire HoMM2 implementation.

## Confirmed findings and repairs

| Capability | Donor evidence | HoMM1 result |
| --- | --- | --- |
| Invocation logging | HoMM2 `analysis/sema.py::_sema_log`; Gruntz `docs/tooling-map.md` explicitly says sema logging was removed | Restored more broadly: every Python `main`, individual batch query, and subprocess launch is recorded in `build/homm1_usage.jsonl`. HoMM2's implementation also missed parser failures because parsing happened before its logging handler. |
| Public tool access | Gruntz CLI registers `link` and `rc`; both implementations were copied | Restored `homm1 tool link`, `rc`, and the locally added `ml`; a regression test checks every tool module's entry point is reachable. |
| Ghidra command dispatch | Gruntz CLI exposes `ghidra`; HoMM1 copied the whole package | Restored `homm1 ghidra ...` dispatch. Actual Ghidra project operation still requires its external installation. |
| Compiler-artifact source gate | Gruntz `verify/compiler_artifacts.py`, registered in its fast tier | Restored the donor implementation and fast-tier registration. Gruntz's source-specific exception counters are empty for HoMM1; nested HoMM1 object directories are scanned. |
| Gate negative controls | Gruntz `verify/selftest.py` contains 427 test methods | The whole module was omitted. Thirty controls covering nine existing/restored gate groups are now ported into `tests/test_donor_gates.py`; the remaining controls are still outstanding. |
| Documented semantic diff | HoMM1 docs advertised `homm1 sema diff`; the dispatcher has no such view | Corrected the example to the implemented retail disassembly command. This fixes documentation, not the missing comparison capability. |
| Assembly fingerprinting | Existing fingerprint path assumed every source was C++ | Build exposed `clangd: invalid AST` on the workspace's MASM units. Assembly now keeps the explicit unknown whole-file fallback and never reaches clangd; it does not masquerade as a per-function fingerprint. A regression test protects that behavior. |

Usage logging preserves arguments, UTC times, working directories, process and
nested invocation identities, elapsed time, exit status and escaping exceptions.
Subprocess records describe launch attempts, not child completion. No terminal
output, environment dump or unrelated shell activity is recorded. See
[the usage contract](tooling.md#usage-history).

## Outstanding capabilities

These are open restoration work, not approved permanent exclusions.

| Priority | Gap | Evidence and required adaptation |
| --- | --- | --- |
| 1 | Candidate/retail instruction and block differences | HoMM2 `sema disasm --base/--target/--diff/--rich/--blocks/--branches/--dot`; Gruntz's `walls` package contains pair/semantic/residue diagnostics. HoMM1 retained retail-only `sema disasm`, including retail `--blocks`, but no candidate comparison. Port against HoMM1 object paths, identities, relocations and VC4 CodeView. |
| 1 | Remaining gate negative controls | Port the controls applicable to the retained model, delinker, normalizer, MAX gate, data gates and graph. Tests tied to omitted features must travel with those features. Thirty restored tests are not equivalence with the 427-test donor suite. |
| 2 | Source navigation and safe symbol rename | HoMM2 exposes symbol/definition/references/hover/rename; Gruntz has `lsp/`. HoMM1 has the low-level clangd client but omitted its user-facing consumers. This is not a Gruntz-only feature. |
| 2 | Residual diagnosis and review revalidation | All 33 Gruntz `walls/` files are absent. This includes reusable instruction/relocation/stack diagnostics as well as campaign-specific ledgers. Review per component; ledger-specific inputs do not justify dropping all diagnostics. |
| 3 | Reviewed source variants and compiler-state search | All 12 Gruntz `permute/` files are absent; HoMM2 has its own public permuter. Requires the VC4 compile profile and verified object comparison, source restoration, timeout and relocation-identity contracts. Do not transplant VC6 assumptions. |
| 3 | Resource build/check support | Four `rsrc/` modules are absent, while `tool.rc` was copied. The parser and compiler workflow are reusable; resource identities, script and byte comparison must be derived for HoMM1. |

The 61 absent Gruntz paths are: 33 `walls`, 12 `permute`, four `lsp`, four
`lineage`, four `rsrc`, `graph/play.py`, `graph/test_play.py`, `tool/rez.py`, and
`verify/selftest.py`. Test and package-marker files are included in those
counts. `lineage` is specifically LithTech source-lineage discovery; `tool/rez`
and the play/install setup depend on Gruntz formats and deployment. They need
target-specific applicability decisions, not blind copying.

The original port also moved data-TU order, data relocations, data access and
data coverage out of the normal tier, and vtable/allocation checks out of the
full tier, into an explicit `data` tier. That follows HoMM1's documented
data-last campaign policy. Their implementations remain present; they are
deferred checks, not silently missing modules. `review-claims` is absent with
the wall-review system and remains part of that restoration work.

## Preventing another silent reduction

New command entry points must be usage-logged and new copied tool CLIs must be
registered; `homm1 test` enforces both. Changes to donor-derived tooling must
record their donor revision and classify capabilities as retained, adapted,
deferred with a concrete prerequisite, or inapplicable with target evidence.
Presence in one donor does not establish parity with the other. Repeat the
module inventory and update this capability review when accepting a port or
removing a command; do not relabel reusable tooling as game-specific.

## Validation of this repair

`homm1 test` passes the ported donor controls and
the logging, surface, fingerprint, and MASM dead-code
regressions. `homm1 build` passes the full verification gate and fingerprints
411 functions across the 50-unit manifest. `homm1 link --dry-run` resolves the
complete object and library line; the real link reaches VC4 LINK.EXE and
reports the remaining reconstruction closure without `/FORCE`.

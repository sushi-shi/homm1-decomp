# Tooling inheritance review — 2026-09-26

The active donor is Giten `39384dc6726478357b5efd42c66522781e8310fe`.
The original Gruntz pin is `b1de0e555576a215898907b8ec8ed5423368883e`;
HoMM2 remains the comparison donor at
`299514f88900c0cf30ba03422c72830a38fc1cb7`.
The historical review below records the earlier state, not current parity.

## Whole-workflow audit and port

The [whole-tree inventory](../evidence/tooling-giten-whole-tree.json) covers all
526 tracked paths at the pinned Giten revision, including symlinks, skills,
documentation, editor files, hooks, patches, release scripts and game facts.
Repeat it with `homm1 audit tooling --giten PATH --whole-tree --json`.
Every path has a content hash and a local counterpart or explicit disposition;
this is coverage of the audit, not proof of behavioral parity.

| Area | Result |
| --- | --- |
| Skills | All four installed: matcher, holista, wall-identifier, permute; metadata and lever reference retained, adapted to VA/VC4/code-first policy; skill validator passes. |
| Agent instructions | Root guidance expanded; CLAUDE.md and .claude/skills use canonical symlinks; bounded matcher worker profile installed. |
| README generation | Ported MAX headline/module scores and CUR/MAX/HIST rollup, using the would-be banked ledger so edited source resets MAX before banking. Default build refreshes README, explicit `build verify` runs all existing fatal gates; recursive unit-size joins are adapted; code mode never reports data coverage. Repeated generation leaves the ledger unchanged. |
| Editor | Neovim plugin ported to live bindings.tsv, VA conversion, recursive unit names, normalized pairs and matching project settings. VA hints and real objdiff assembly smoke-tested; class navigation uses the supported HoMM1 interface. Shell wrapper loads it without global editor changes. |
| Formatting/hooks | Donor style adapted to existing macros; structural brace/comma insertion disabled. Pre-commit protects partial staging and excludes vendor files; repository-local hook installed. Real formatting tested in an isolated Git repository. |
| Manifest merge | Donor three-way unit-block merge and controls ported; duplicate unit keys rejected; merge driver installed locally. |
| Reference docs | Build, scoring, markers, data contracts, compiler/linker guidance, clangd, permutation and workflow docs supplied; compiler-pattern observations retain donor provenance and explicitly require VC4 validation. Broken donor-local links are identified as historical references. |
| Declaration/model contract | Mining the missing tests exposed omitted src_decl precedence, conflict checks and ownership clearing; these are now integrated and tested with real clang declaration extraction. |
| Marker joins | Early-stop scans now convert absolute VA annotations to model RVAs. |
| Usage | Giten diagnostic categorization, bounded error tails, streamed Ninja output, query difference outcomes and copyable completion log are adapted into HoMM1's entry-point/start/finish/subprocess logger. Existing entry-point coverage remains enforced; the JSON event schema stays HoMM1's. |
| Donor task and exception ledgers | Not imported as HoMM1 facts or authorizations. Worklists stay derived under build; retail and source facts remain target-owned. |

The ordinary tooling suite now contains 117 controls. The larger imported donor
selftest still has compiler/project-specific failures; it is not advertised as
passing or replaced by this suite. Full build verification still exposes the
existing board, include-order and widget-method closure findings.

Remaining explicitly deferred donor capabilities are the VC5 inline-budget
measurement controls, executable-section data support and its census controls,
and the placement audit for the later data campaign. Giten's relocation-image
synthesis, disc extraction and IAT-in-rdata assumptions do not apply to the
current HoMM1 input contract. Retained/adapted module counts below are earlier
snapshots; the repeatable audit reports the current state.

The surrounding workflow was also checked against the pinned HoMM2 Buka tree:
its CLI refreshes README after building; its Neovim root test emphasizes
worktree-local paths; and its matcher uses absolute VA annotations. Those
contracts informed the local adaptations. HoMM2's pre-commit C++11 strict-enum
compiler check and VC6 worker profile are target-specific and are not substituted
for HoMM1's VC4 build or source gates. The pinned Gruntz module audit was rerun.

## Deep pipeline port

| Capability | Disposition and evidence |
| --- | --- |
| Claim extraction | Adapted Giten declaration claims to absolute `VA_DECL`, subtracting the image base. Clang declarations join the existing model; they do not claim bodies. |
| Delinking | Adapted static destructor recognition, code-first data fences and debt output. Preserved HoMM1 reviewed referents, VC4 EH handling and per-TU ownership. |
| Comparison | Retained Giten data-reference relaxation and anonymous namespace normalization. Default is code first (`data_matching=false`), by campaign decision. Function, IAT and EH identities remain checked. |
| Selected-unit loop | Adapted incremental compilation, claim refresh, binding invalidation and comparison to recursive BASE/SOURCE paths and MASM units. Global binding changes trigger full delinking. |
| Score history | Retained mode-marked ledgers, explicit rebasing and DIP/RESET classifications. No mixed-mode MAX comparisons. |
| Experiment engine | Ported source variants and state trials. A real VC4 PollSound typedef trial scored 100%, retained evidence and restored source. Restored `state --record-max`: audited exact compiler-state peaks may raise MAX/HIST for the same fingerprint; CUR/source stay unchanged. Added comparison-mode and nested-unit ledger checks. |
| Diagnostics | Ported walls pair/semantic/stack/relocation analysis. Corrected relocation type/addend interpretation and HoMM1 function padding bounds. Heuristic control tables require VC4 calibration. |
| Navigation | Ported clangd consumers; hover smoke-tested on a HoMM1 header. Rename behavior still needs end-to-end validation. |
| Code identity gates | Added data-identity and review-claims to normal checks, preserved existing source gates. Contradictory data identities are checked even during the code-only campaign, as in Giten. |
| Negative controls | Imported donor controls without treating their presence as parity. Existing HoMM1 tests pass; the donor selftest still has target/compiler-specific failures and missing APIs. It is not yet part of `homm1 test`. |
| Inline prediction | Deferred VC5 model: no evidence that its thresholds predict VC4. The local gap command reports definitions/calls only. |
| Data placement/coverage | Deferred to the data campaign. Existing explicit data tier is retained. No initializer coverage admitted by this port. |
| Vostok game patches | Giten's extra .idata-in-.rdata and text-data patches are not ported without HoMM1 applicability evidence. Existing patched Vostok/objdiff pins remain. |
| Resources/runtime deployment | Deferred pending target resource and deployment evidence; not required for the code matching loop. |
| LithTech lineage/REZ | Inapplicable to HoMM1's engine and formats. |

This is an implementation port with partial behavioral validation, not a claim
that all Giten capabilities or its overnight throughput have been reproduced.
`homm1 verify selftest` exposes the remaining donor control failures. They must
be resolved with target-specific fixtures and evidence, not by suppressing tests.
The full build currently also encounters pre-existing board, include-order and
undefined widget-method findings. These remain visible.

## Validation and remaining gaps

The current `homm1 test` suite runs 90 passing controls, including unchanged
Giten normalization/data-identity fixtures and a full-pair mode-transition test.
The donor-wide selftest remains separate and failing; these 90 tests are not
its replacement. Selected-unit matching and a real VC4 state trial were exercised.
The code-mode ledger was explicitly rebased after a full comparison rebuild;
the previous strict ledger is retained in `build/port-validation`.

The [Giten inventory](../evidence/tooling-giten-2026-09-26.json) has 87
structurally equivalent modules, 68 adaptations and 12 absent paths. The
[Gruntz inventory](../evidence/tooling-gruntz-2026-09-26.json) has 90 equivalent,
56 adapted and 26 absent. These counts include test and package files.

Of Giten's missing production modules, `delink/reloc_image.py` synthesizes a
relocation section for its /FIXED executable; HoMM1 uses its retained retail
relocations. `tool/cdfs.py` is disc extraction, outside the code loop.
`tool/merge_units.py` is deferred until a reviewed TU regrouping needs its
transactional rewrite; `verify/placement.py` includes data-claim extent checks
for the later data campaign. Missing test files and VC5 inline measurement
controls remain explicit validation debt. Gruntz's additional scanner modules
are not all present in the chosen Giten donor; their absence is reported rather
than treated as Giten parity failure.

The full build compiles, delinks and compares successfully, then fails the
three documented source gates. Candidate linking reaches VC4 LINK.EXE and
reports 61 unresolved references across 11 symbols. Nothing forces the link.

## Historical review (2026-09-19)

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
Subprocess records describe launch attempts, not child completion. Failure tails
now retain bounded command diagnostics; successful routine output, environment
dumps and unrelated shell activity are not recorded. See
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

## Earlier validation before the Giten workflow port

`homm1 test` passes the ported donor controls and
the logging, surface, fingerprint, and MASM dead-code
regressions. `homm1 build` passes the full verification gate and fingerprints
411 functions across the 50-unit manifest. `homm1 link --dry-run` resolves the
complete object and library line; the real link reaches VC4 LINK.EXE and
reports the remaining reconstruction closure without `/FORCE`.

## MAX, diagnostics and banking follow-through

The first README port still used the older CUR headline and best-ever churn
column. The current port uses Giten's MAX headline and module table, with a
separate CUR/MAX/HIST line. Refresh builds a would-be banked ledger, applying
source resets and RVA migrations, without writing the bank. Unedited DIP details
are hidden unless `--all` is requested. Ordinary builds update comparisons,
fingerprints and README; `homm1 build verify` runs the existing fatal gates.
Data debt is printed after successful builds in either comparison mode.

[Behavior comparison](../evidence/tooling-giten-max-behavior.json) records ten
functions whose syntax trees match the pinned Giten donor after package renaming,
including the bank update, gate verdict, score weights and error categorization.
The standard suite now includes Giten's unchanged banking, mode, precision and
MAX-classification controls, plus README no-write and command verdict tests.
The exact-state recorder is enabled with its original source/score/size/relocation
checks and HoMM1 unit/mode adaptations. It retains MAX/HIST only; disposable
source is removed and sub-100 results cannot update the ledger.

Usage retains start/finish/subprocess events and adds streamed Ninja diagnostics,
a bounded failure tail, donor error categories, query difference outcomes and a
copyable text completion log. Successful routine output is not persisted.

Latest validation: `homm1 test` passes 157 tests. A real one-trial VC4 state
campaign with `--record-max` audited PollSound at 100%, 114 bytes, 11 ordered
relocations, then reported `already_exact`; SHA-256 checks confirmed unchanged
source and ledger. The combined worker build succeeds. Explicit verification
still reports the existing local enum, include ordering and missing widget Read
closure findings; the MAX gate passes. Candidate linking remains incomplete,
with undefined genuine reconstruction symbols rather than forced linkage.

The sustained worker campaign exposed an additional nested-unit artifact-path
bug: `BASE/WINDOW` created an unintended subdirectory in permutation manifest
filenames. Campaign output now flattens the unit path; a real manifest-write
control runs in `homm1 test`. Donor semantics are unchanged.

The next integrated worker checkpoint passes all fast and normal build gates,
including the previously reported board, include-order, and widget Read closure
findings. `homm1 test` passes 158 tests. Both pinned donor audits were repeated;
the Giten whole-tree inventory still accounts for all 526 paths, without claiming
behavioral parity for the 225 entries marked adapted-review-required. Evidence
and the measured checkpoint ledger/report are saved under ignored
`build/sol-integration/checkpoint-150/`. Candidate linking remains incomplete.

The WinG initializer exposed a same-object member-order case in the inherited
data-identity permutation check. It now compares complete address multisets
within a function, including addends; wrong/duplicated members and swaps across
functions still fail. This adapts the existing donor check rather than adding
a second comparison path. The positive and negative controls run in `homm1 test`.

CDPlay exposed another pooling distinction: VC4 coalesces two identical narrow
string literals that retail keeps at separate addresses. The identity gate now
allows that split only for compiler-generated strings whose terminated source
bytes equal every referenced retail copy. Named arrays, mismatched or unreadable
copies, and contradictory explicit claims still fail. This is an adaptation of
the inherited gate; data scoring remains deferred. Synthetic positive and
negative controls pass alongside the full tooling suite, and the integrated
build passes all normal gates. The pinned Giten whole-tree and Gruntz module
audits were repeated. Candidate linking still reports genuine unresolved
definitions without forced linkage.

The recovered normal and dim icon callers establish six-argument renderer
ABIs in HoMM1, unlike the later donors' eleven-argument forms. The existing
fixed-ASM declarations and export names now use those retail-backed signatures;
the assembly instructions and comparison rules are unchanged. This is a target
fact adaptation within the inherited build pipeline. The full tooling suite
passes after these corrections.

## Exact overload fingerprints

Reviewed Giten `39384dc6726478357b5efd42c66522781e8310fe` and Gruntz
`b1de0e555576a215898907b8ec8ed5423368883e`: both deliberately join genuine
overloads into one qualified-name range hash. This behavior changes a mapped
function's fingerprint when a sibling overload is added, despite its own
body being unchanged; it also drops a one-line overload when another overload
has a multiline body. HoMM2 Buka `e0689d3f71b2942b544fd677cb54085a13503d7b`
uses VA-owned lexical function blocks and keeps unrelated bodies independent.
Its offset/argument normalization and inline-helper hash propagation remain
deferred, rather than copied into HoMM1's hash contract.

HoMM1 retains the donor document-symbol ranges and hashes for unambiguous
names. Repeated names now use exact mangled AST definition identities through
existing source extraction's clang/location/MSVC-spelling mechanisms; no
signature-spelling heuristics or second adapter is introduced. Body-less
siblings and inline definitions in headers cannot supply an overload body.
These exact-overload hashes use the `overload1:` provenance domain. A legacy
union-to-individual transition is unknown provenance, not proof of a source
edit; subsequent edits within the exact-overload domain remain detectable.
Cache version 2 invalidates old derived caches/seeds without changing the
score ledger. Native clang/clangd controls cover typedefs, one-line bodies,
const overloads, isolated edits, sibling additions and header ownership; cache
and domain migration controls protect conservative provenance handling.

## VC4 inline EH continuation adaptation

Reviewed against pinned Giten `39384dc6726478357b5efd42c66522781e8310fe` and Gruntz `b1de0e555576a215898907b8ec8ed5423368883e`: their funclet-owner canonicalization assumes separate executable sections. HoMM2 Buka's `scripts/homm2/build/canonicalize_data_symbols.py` retains the same general owner-relative relocation checks, but does not provide this HoMM1 VC4 inline-registration classifier. HoMM1 adapts the existing canonicalization and delinker mechanisms: certified inline groups stay within the full owner; packed groups retain separate records. Exact resolved offsets, handler references and FuncInfo/map identities remain protected. Unsupported continuations are rejected; no alternate adapter or comparison masking was added. Broader compiler-profile parity remains deferred. See [the retail controls](../evidence/vc4-inline-eh.md).

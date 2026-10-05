# Tooling inheritance

| Donor | Reviewed revision | Role |
| --- | --- | --- |
| Giten | `39384dc6726478357b5efd42c66522781e8310fe` | Active workflow and comparison donor |
| Giten (giten-enums) | `94435eff949d6190fa0b1b697caae9f21c53bb6b` | Constants work list and enum-domain/reuse review |
| Gruntz | `b1de0e555576a215898907b8ec8ed5423368883e` | Original pipeline ancestry and cross-review |
| Gruntz | `7d4bd55b9` | Constant destination keys and enum use-context ranking |
| Gruntz | `38fd8e9f7` (main) | `gruntz play`, game prefix and clean-export runner |
| Gruntz | `f272a806b3084d721f8f8304705413279b8ef273` | 1.1 fingerprint review against `verify/link_tier.py` relocation masking |
| HoMM2 Buka | `299514f88900c0cf30ba03422c72830a38fc1cb7` | Initial capability review |
| HoMM2 Buka | `e0689d3f71b2942b544fd677cb54085a13503d7b` | Exact-overload fingerprint review; `homm2 clean` |
| kf1 | `62870641`, `1a1e594e`, `904687dd`, `5ad5875f`, `655774b2`, `53cc73af` | `kf clean` export, verification and snapshot branches |

These are implementation reviews, not a claim of complete behavioral parity.
The executable, source ownership, types, VC4 profiles and data policy stay
HoMM1-owned. PoL 2.0 supplies secondary source correspondence.

## Capabilities

| Area | Disposition |
| --- | --- |
| Claims/model/delinking | Adapted: absolute VA conversion, declaration precedence/conflict checks, recursive units, VC4 EH and reviewed referents in the existing pipeline. |
| Comparison | Retained code-first default; call/IAT/EH identities remain checked. Mode-marked ledgers require explicit rebasing. |
| MAX and README | Retained fingerprint resets, held MAX for unchanged TU-state dips, HIST, RVA migration, absent rows and explicit banking. README uses a would-be bank without writing it. |
| Errors/warnings | Adapted streamed Ninja diagnostics, bounded failure tails, error categories, query difference outcomes and completion logs to HoMM1's invocation logger. |
| State/source experiments | Adapted complete-TU trials, restoration, timeout and exact-score/extent/ordered-relocation audits. `state --record-max` retains exact peaks only for unchanged fingerprints and matching ledger mode. Nested unit paths are supported. |
| Walls/navigation | Ported semantic, stack and relocation diagnostics and clangd consumers. Hover was smoke-tested; rename still needs end-to-end validation. Donor heuristic tables need VC4 calibration. |
| /Od slot model | Adapted from HoMM2 Buka `scripts/homm2/core/od_slots.py` (reviewed at `e0689d3`): `homm1.core.od_slots` uses the VC6 folded hash measured by Buka probe compiles and a `/Z7` census of the tree, replacing the inherited VC4.0 variant. See [VC6 frame slots](patterns/vc6-od-frame-slots.md). |
| Source/identity gates | Retained normal source gates, review-claims and contradictory-data-identity checks even in code mode. Data coverage/placement checks remain in the explicit later data tier. |
| Constants/enum review | Ported from giten-enums into the existing `verify` modules: `constants` gains the `config/constants.tsv` glob work list (first match wins, stale rows fail), committed floor (`--update-floor`), `--list`, `build/gen/constants_open.tsv`, strict-domain parse with retail fallback, switch-subject/store-target review details and float literals; `enum-reuse` gains the role-pair report and the `config/reviews/enum-reuse.tsv` ledger; `enum-domains` gains constant groups as non-storage and LOCAL/PARAM/RETURN width exemption; `board` reads `H1_ENUM_*` blocks and declarators. Adapted: `H1_ENUM_*`/`include/Domains.h` instead of `GZ_ENUM_*`/`EnumDomain.h`, strict view via `/std:c++20 /Zc:__cplusplus` instead of `GZ_STRICT_ENUMS`, and VC4 booleans: only Win32 `BOOL` is a boolean domain, its proven spelling is `TRUE`/`FALSE`, and `true`/`false` spellings fail (C2065; Giten's TRUE->true check is inverted). From Gruntz `7d4bd55b9`: `verify/constant_context.py` (declaration-identity destination keys), the `context_key`/`context_label` census columns and `build/gen/constant_contexts.tsv`, enum member use contexts with shared-context ranking in the collision/pair reports, and `enum-reuse --extend-ledger`. Adapted: the VC6 (Buka) target has `bool`, so a `bool` destination proves `true`/`false` and the `true`/`false` spelling gate applies only to VC4 targets (`config/units.toml` `compiler`). Deferred: Gruntz's `#define`/const-integral inventory and `replace` ledger decision. See [constants](constants.md) and [enum reuse](enum-reuse.md). Deferred: Giten's handoff evidence notes are game-specific. |
| Skills/workflow | Adapted all four skills, canonical instruction symlinks, safe staged formatting and unit-block merge driver. See [workflow](workflow.md). |
| Negative controls | Inapplicable: HoMM1 keeps no test suite or self-test verb; `homm1 audit usage` checks entry-point logging and the build graph runs the gates. |
| Gruntz-only scanners | Deferred: `walls/calibrate`, `ehactions`, `escapescan`, `framescan`, `jccscan`, `loopscan`, `offsetscan`, `reloadscan`, `residue`, `retscan`, `signscan`, `storescan`, `thisscan`, `uninitscan`, `vptrscan` need separate applicability review and VC4 controls; the Giten diagnostic port does not establish their parity. |
| Inline-budget prediction | Deferred: VC5 thresholds need measured VC4 controls. The local gap command reports definitions/calls only. |
| Executable-section data/placement | Deferred: requires HoMM1 fixtures and the later data campaign. No initializer coverage is admitted. |
| Resources | Adapted from HoMM2 Buka `rc_res.py` (reviewed at `e0689d3`): `homm1.tool.rc` stages the retail icon in a temporary directory and gates every compiled payload against retail in both directions. RC/RCDLL/CVTRES are the VC4 media tools pinned as vc40 `resource_files`. See [candidate linking](linker-flags.md). |
| Clean source branch | Adapted from HoMM2 `scripts/homm2/clean/clean_source.py` and kf1 `scripts/kf/clean.py`/`clean_lexer.py` as `homm1 clean` (`scripts/homm1/clean`). Retained: the literal-aware lexer, innermost-first macro rules, comment removal without token joining, residue and stranded-punctuation self-checks, marked output replacement, kf1's committed-`HEAD` snapshot input and single-root-commit branch with `Source-Commit` provenance. Adapted: VC4's production `H1_ENUM_*` branch instead of HoMM2's strict typed branch (VC4 cannot compile it), and verification with the pinned VC4 through fixedroot plus a line-preserving control that must reproduce the candidate EXE, in place of kf1's CPE comparison; publication uses a private index, never a worktree. The standalone tree builds with VC4 under Wine from the pinned toolchain release. Deferred: HoMM2's `--classic-from`/C++20 typed view and `GENERATED_PATCHES` (no portable target yet). Inapplicable: HoMM2 allocation-wrapper, locale and `--publish-parent` rules. See [clean source](clean-source.md). |
| Runtime deployment | Adapted from Gruntz `scripts/gruntz/graph/play.py`, `play_main`, `scripts/create-wine-prefix.py`, the flake `play` shell and the clean export's `play.py`: `homm1 play` and the clean tree's `nix run path:. -- --data DIR` share `homm1.graph.play` (separate game prefix, 640x480 virtual desktop, CD-ROM drive, gamescope integer scaling, wineserver cleanup, remembered data path, saves preserved). Adapted: no provisioning script (the runner prepares the prefix on each launch), HoMM1's registry key and CD layout, copied rather than linked writable folders. Inapplicable: `nix/runtime.nix` pins - HoMM1's Smacker/Miles DLLs come from the user's installation and Wine's built-in `wing32` replaces the 16-bit-thunking WinG runtime. See [playing](play.md). |
| Relocation synthesis/disc/IAT patches | Inapplicable to current inputs: HoMM1 retains retail relocations and its own `.idata`; Giten's fixed-image and disc-layout assumptions differ. |
| LithTech lineage/REZ and donor ledgers | Inapplicable to HoMM1. Foreign task rows and exceptions are not evidence or authorization. |

## HoMM1 adaptations

- Data identity compares complete within-function address multisets, including
  addends, for same-object operand permutations. Wrong/duplicated members and
  cross-function swaps still fail. Compiler-generated pooled strings may refer
  to separate retail copies only when every terminated byte string agrees;
  named arrays and contradictory explicit identities remain checked.
- Overloaded definitions use exact mangled AST identities through the existing
  extractor. Giten/Gruntz union sibling ranges; Buka's VA-owned blocks preserve
  unrelated bodies. HoMM1 keeps unambiguous hashes and uses `overload1:` for
  exact overloads. Buka's argument normalization and inline-helper hash
  propagation remain deferred.
- VC4 inline EH groups stay within their full owner; packed groups retain
  separate records. Resolved offsets, handler and FuncInfo/map identities remain
  protected. Unsupported continuations fail closed.
- Comparison resolves OLDNAMES references as LINK does. `runtime_aliases`
  reads the pinned OLDNAMES.LIB weak externals and LIBCMT label aliases, and
  `canonicalize_coff` names each undefined alias reference by its runtime
  function. `normalize` proves that no compared object defines an alias.
  Reviewed referents use the runtime symbols. HoMM1 adds this capability; it
  does not come from a donor.
- For reviewed fixed-asm units, `relocate_in_object_calls` gives each call
  that MASM resolved inside one module the REL32 relocation that the delinked
  target carries. A postcondition proves that the call target is unchanged.
  This lets one retail module stay a single object (`BASE/LZHUFDEC`).
- The 1.1 fork retains HoMM2 Buka `e0689d3`'s fixed-MASM claim mechanism;
  only target VAs change. The new `sema fid` is a HoMM1 discovery command,
  reviewed against Gruntz's `verify/link_tier.py` masking and HoMM2's
  `build/fixed_asm.py` and `analysis/disasm.py`. It reuses the existing PE,
  relocation and usage-log modules. Retained: decoded relative operands and
  HIGHLOW site masks. Adapted: whole-image anchor search and monotonic-order
  disambiguation. Deferred: changed-function similarity scoring and automatic
  claim admission; neither is authorized by a fingerprint. Inapplicable:
  interpreting a discovery match as the strict comparison score. The same-image
  control resolves only identity mappings; the 1.0-to-1.1 report preserves
  ambiguous candidates for call/vtable review. Clean-export target labels and
  the default source branch now identify Windows 95 1.1; publication was not run.
- HoMM2's VC6 worker/compiler settings and C++11 strict-enum hook are not
  HoMM1 build controls. Giten's `editor/nvim` integration and research
  solvers are `inapplicable`.

## Repeat the review

Run inside `nix develop .#build`; reports belong in ignored `build/`:

```sh
homm1 audit tooling --giten /path/to/giten --whole-tree --json > build/tooling-giten.json
homm1 audit tooling --gruntz /path/to/gruntz --json > build/tooling-gruntz.json
homm1 audit usage
homm1 build
homm1 build verify
homm1 link --dry-run
```

The audit compares committed donor blobs, including skills, hooks, docs and
editor files (editor paths are inapplicable here). Presence, content hashes and AST equality establish inventory
coverage only. New ports or removals must update the dispositions above, review
both Gruntz and HoMM2, preserve usage logging and run applicable controls.
The 1.1 candidate links without unresolved definitions. Future unresolved
definitions remain reconstruction findings, not grounds for forced linking.

## Win95 1.2 toolchain and target port

Reviewed Gruntz `ee6365395c443019e3c0b8d82f54642f9621bbd2`
(`scripts/create-toolchain-release.py` and `.nix`) and HoMM2 Buka
`e0689d3f71b2942b544fd677cb54085a13503d7b`
(`scripts/homm2/init/toolchain.py` and toolchain release scripts), retaining
original-media hashes, per-file verification, deterministic archives and
separate tool/runtime provisioning. Adapted: VC4.1 with MASM/WinG/DirectX,
resource tools included and checked at release installation, and an explicit
verified-installed-tree packaging route. Donor VC5/VC6 compiler patches and
flags are inapplicable; no patch was imported. Existing usage-logged CLI
entry points are retained; the release builder retains its existing logging.

The fixed compiler path view reads `build.source_roots` rather than assuming
1.0/1.1's D: layout. Both ordinary and clean builds use HEROESW and the new
runtime imports, omit exports, and retain the stack contract. Clean publication
remains opt-in. DNA classification uses the reviewed retail CRT band and
requires exact library controls within it: the last vendor import thunk is
insufficient because 1.2 puts the whole BASE library after those thunks.
This is a target-fact adaptation within the existing census pipeline.
Giten remains pinned at `39384dc6726478357b5efd42c66522781e8310fe`.

## Buka 2003 migration infrastructure

Reviewed HoMM2 Buka `e0689d3f71b2942b544fd677cb54085a13503d7b`
(`build/catalog.py`, `build/localization.py`, localization tests,
`build/reloc_owners.py`, `audit/reloc_sweep.py`) and Gruntz
`ee6365395c443019e3c0b8d82f54642f9621bbd2` (existing graph compiler,
link and fingerprint contracts). Gruntz has no equivalent localization catalog;
its single build graph and comparison boundaries remain the integration model.

Retained: HoMM2's contextual IDs, original-English registry, PO validation,
CP1251 literal generation, source-offset-preserving views, and locale-separated
generated files. Adapted: HoMM1's existing include scanner, compiler/Clang
entry points, Ninja dependencies, fingerprint implementation and usage logging.
The 29 tests cover catalog rejection cases, exact encoding, mirrors,
source offsets, English/Russian output isolation, actual Clang argument checks,
retail catalog provenance and relocation-directory/manifest rejection cases.
The existing clean exporter now carries the portable catalogs/renderer and
locale-specific standalone outputs. Its line-preserving control passes all 69
object comparisons and produces an identical candidate apart from timestamps.
Adapted VC4 support keeps the generated catalog at a stable compiler-visible
path and declares fixedroot changes as compiler dependencies; this avoids
host-path-dependent `/Gi` output. The standalone build also localizes vendored
translation units, declares include paths explicitly and uses its own Wine
prefix so a reconstruction prefix cannot replace its library environment.

Adapted for `/FIXED`: one PE reader supplies both sema and delink absolute
sites; a reviewed TSV must carry the exact image SHA-256. The Python adapter
passes the same explicit manifest; the delinker package adaptation and reviewed
site enrollment are recorded below. Retained: existing PE relocation records
take precedence. Source claims and comparison activation remain incomplete.
The relocation sweep was measured against 1.2, not assumed correct from the
donor's different target.

Migrated: 565 verified table entries in authored source, preserving original
English. Still pending: the remaining text/call-site rewrites, Buka target
activation, resources and reconstructed runtime integration. The unmodified
retail executable reaches its Russian menu with the verified Archive.org assets;
that baseline is not a runtime test of the port.
Earlier statements that locale rules are inapplicable describe the NWC branches;
they do not apply to the Buka branch. No command or behavioral parity is claimed
for these pending features.


## Buka compiler media and release enrollment

Reviewed HoMM2 Buka `e0689d3f71b2942b544fd677cb54085a13503d7b`,
`scripts/toolchain/create-toolchain-release.py` and `.nix`, alongside the
existing Gruntz `ee6365395c443019e3c0b8d82f54642f9621bbd2` compiler selection
and toolchain contracts. Retained: verified original VC6 media, SP5 cabinet
chaining from volume 1, Enterprise backend alias, six restored standard-header
names, the separately pinned MASM disk and normalized archive metadata.
Adapted: per-file media membership and installed names live in HoMM1’s existing
`config/toolchains.json`; media install and release generation share one
extractor. The existing compiler contract chooses resource pins, Nix paths,
release installation and clean-export URLs. There is no second adapter graph.

Validation: all 1,297 bundle files verified, two identical archive hashes,
local archive installation, `@comp.id = 0x000b2306`, and a real BASEMGR object
identical to the preceding verified VC6 control. Deferred: selecting the Buka
compiler in `units.toml` alongside the reviewed source and retail claims;
matching and runtime validation of the full Buka candidate are still required.

VC6 library indexing accepts uppercase `.LIB` names. Its separately shipped
legacy `MAPI.LIB` is a pinned OMF archive, not a Win32 COFF provider; the index
records that explicit format exclusion instead of treating it as corrupt COFF.
The Win32 `MAPI32.LIB` remains indexed. No retail identity is inferred from a
library name alone.

## Buka target activation and CFG review

Reviewed Gruntz `d1cdb537caa6142849c7345eedc306dbb5af3763`
(`core/paths.py`, `graph/compdb.py`) and HoMM2 Buka
`e0689d3f71b2942b544fd677cb54085a13503d7b`
(`core/paths.py`, `init/clangd.py`, `analysis/disasm.py`). Retained: the single
repository/path resolver, existing target-input contract, lowercase SDK mirror
and assembly-only CFG analysis. Adapted: HoMM1's `core.paths.retail_exe` and
Ninja retail input now read the selected game destination instead of assuming
HEROESW. The Buka target, locale and VC6 profile are selected together; stale
NWC reports and scores were retired.

The existing donor CFG generator is used in temporary review scripts; this
adds no parallel pipeline or decompiler dependency. No decompiler output is
permitted. Deferred: migration of the address census, fixed-image references,
Clang parsing of the backend SDK's legacy STL and candidate/runtime/clean
validation. The small path adaptation does not establish those capabilities.
Earlier references above to Buka compiler activation being deferred describe
the previous migration stage; compiler/target activation has now happened.

Validation for this stage: `homm1 audit usage` passes; the catalog/PE suite
passes all 29 existing tests. The pinned-Giten whole-tree audit completed and
still reports missing `sema/exe_map.py` and `verify/selftest.py`, plus adaptations
requiring broader review. These are recorded limitations, not silently treated
as parity. `homm1 build verify` stops at the unmigrated census; `homm1 link`
rejects inherited resource identities before linking.


## Buka resources and VC6 import archives

Reviewed HoMM2 Buka `e0689d3f71b2942b544fd677cb54085a13503d7b`
(`build/import_lib.py`, `build/test_import_lib.py`,
`build/symbol_providers.py`, `BASE/MiscRuntime.cpp`) and Gruntz
`d1cdb537caa6142849c7345eedc306dbb5af3763` (`graph/implib.py`,
`delink/implib.py`). Retained: the single resource gate and catalog, retail
hint/ordinal/public-symbol validation, old COFF import support, and exact
DLL-backed symbol identities. Adapted: shared catalog rendering for authored
RC, generated UTF-16 literals for both RC.EXE and llvm-rc, and VC6 short-import
verification using the existing delinker decoder. Failed validation no longer
publishes a generated library. Direct-IAT ordinal symbol facts live in the
hash-bound `config/retail/import_symbols.json`, validated against PE slots;
these supplement the existing jump-thunk lookup rather than bypass it.
Audiere is now an explicit link input.

Measured: seven Russian resource payloads exact through both compilers;
seven English payloads preserved through the portable path; 41 relevant tests
pass inside `nix develop .#build`; usage audit passes. The whole-tree Giten audit
still reports missing `sema/exe_map.py` and `verify/selftest.py`, plus broader
adaptation reviews. The candidate links without unresolved symbols or duplicate
warnings. Deferred: full clean/runtime validation, strict census and fixed-image
referents, and complete startup/preferences migration. The build and final
verification still stop at the inherited NWC census; these results do not
establish full command or behavioral parity.

## Buka census and VC6 data names

Reviewed Gruntz `d1cdb537caa6142849c7345eedc306dbb5af3763`
(`graph/implib.py`, `core/msvc_names.py`) and HoMM2 Buka
`e0689d3f71b2942b544fd677cb54085a13503d7b`
(`audit/unmatched_census.py`, `audit/data_claims.py`). Retained: the existing
compiler contract, source-derived identities, scope canonicalization and
strict referent checks. Adapted: runtime-alias and DNA-band library discovery
uses the existing case-insensitive lookup for VC6's uppercase archives;
data-name derivation selects measured VC6 static spelling while retaining
VC4 rules. The HoMM2 internal/external distinction corroborates the controls;
Gruntz's historical `$S` rule is not copied into the VC6 ABI.

Import-thunk naming now requires an admitted function entry as well as an IAT
operand: the VC6 CRT contains interior FF25 jumps that are not functions.
The delinker hint for a missing executable no longer mislabels a missing
retail manifest as a PATH problem. No new adapter pipeline was introduced.

All 46 focused tests and the usage audit pass. The candidate links with
zero unresolved symbols and duplicate warnings. At that census checkpoint, build/final verification stopped
at the missing fixed-image manifest; the selected-unit match report also
remains unavailable. The whole-tree pinned-Giten audit still reports missing
`sema/exe_map.py` and `verify/selftest.py`, and adaptations needing broader
review. That checkpoint deferred manifest enrollment (completed below), source,
data and library identities, unique representation of colliding VC6 internal
names if encountered, and full runtime/clean validation. The structural census
and diagnostic instruction agreement do not establish strict matching parity.

## Buka reviewed references and embedded IAT

Reviewed HoMM2 Buka `e0689d3f71b2942b544fd677cb54085a13503d7b` and its
delinker pin `1393e24b4804cb357fdac147c68013f0aa5a9d95`, Gruntz
`d1cdb537caa6142849c7345eedc306dbb5af3763` and its retained
`81d34b204a0384a92cf3b4c641a8430256b2922e` delinker pin, plus pinned Giten's
`delink/reloc_image.py` and `vostok-iat-in-rdata.patch`.

Retained: the existing package pin, strict referent classifier, canonical alias
ownership, unprovided-identity refusal, switch-table handling and usage-logged
Python entry points. Adapted from HoMM2: the reviewed `site_rva/kind` manifest
schema/parser and real-section IAT range semantics. The parser uses the pinned
package's existing dependency set. HoMM1 reads the PE relocation directory
first; otherwise it uses the explicit hash-validated manifest. No pointer
rediscovery fallback is enabled. The synthetic PDB now emits imports relative
to their real PE section and excludes IAT targets from ordinary data fences.
Unclassified code keeps the game-data identity requirement until reviewed
library evidence says otherwise.

Inapplicable: Giten's rewritten image and invented IAT segment, because the
existing HoMM1 reader and manifest-aware delinker consume the pristine image
directly. Deferred: Buka source/data/alias ownership migration, complete strict
comparison, clean exports and runtime validation. The archived NWC alias rows
are historical evidence only. The Buka site manifest records reviewed fields,
not a claim of donor command parity or source matching.

Validation: 68 Rust tests pass, including manifest parsing/recovery, retained
directory precedence and IAT bounds. Old/new delinkers produce byte-identical
sets of 72 NWC 1.2 objects from the same PDB/manifests. All 49 focused Python tests pass, covering separate and embedded IATs and
conservative unknown-band handling. Usage logging passes. Candidate linking
has zero unresolved symbols and duplicate warnings.
The build reaches strict unprovided-data refusal at Buka RVA `0x8a38c`.
The whole-tree audit dispositions now describe the actual /FIXED and IAT
adaptations; remaining missing capabilities are still recorded separately.


## Buka VC6 array identities

Reviewed Giten's pinned `39384dc6726478357b5efd42c66522781e8310fe`
`core/msvc_names.py`, Gruntz `7d7e44b78aa15b543d92bfc4876b84cded474002`
`core/msvc_names.py`, and HoMM2 Buka
`e0689d3f71b2942b544fd677cb54085a13503d7b` `audit/data_claims.py`.
Retained: the shared source-derived claim path, linkage/scope handling, unit
ownership, and strict reference identities. Adapted: array spelling now uses
the measured selected-compiler ABI; VC6-compatible Clang already agrees with
VC6 on all eleven controlled declarations. Giten/Gruntz's earlier-compiler
array rewrites remain available to VC4 and are inapplicable to VC6. HoMM2's
unit-aware object-symbol checks corroborate the validation approach; its
majority address-voting mechanism was not imported. No adapter pipeline,
aliases, masks or entry points were added.

The whole generated claim corpus now has 698 data names resolving to their
own objects, repairing the two constant-array joins without regressions.
This is naming coverage, not address or behavior parity. The remaining Buka
source/claim migration and the donor-wide missing/deferred capabilities are
still unfinished. See the measured
[array-name pattern](patterns/vc6-array-data-names.md).


## Reporting during the Buka identity migration

Reviewed Giten `39384dc6726478357b5efd42c66522781e8310fe` and Gruntz
`7d7e44b78aa15b543d92bfc4876b84cded474002` unprovisioned-identity refusal patches,
and HoMM2 `e0689d3f71b2942b544fd677cb54085a13503d7b` `redelink.py`.
Retained: whole-image delinking, strict default refusal, candidate normalization,
objdiff name/addend checks, and the separate manual score bank. Adapted:
`--report-unprovided` preserves unresolved names only in marked diagnostic
objects and writes every unresolved reference site. `homm1 compare --baseline`
uses those objects with the existing comparison pipeline. Initially, unreviewed
annotated bodies/references and missing absolute relocations withheld the whole
function score; that temporary rule was retired after the annotated-body sweep
(see below). Diagnostic reports are rejected
by the verified score loader; the README baseline is separately generated and
checks its input fingerprint. Deferred: completing the Buka identity migration
and the existing full verification gates. No donor command parity is claimed.


The Buka baseline README rollup reuses HoMM1's module classifier and Markdown
table formatter. Reviewed Gruntz `5287280c97453c356a8ec835676ece8b498ca953`
`verify/readme.py` and HoMM2 `e0689d3f71b2942b544fd677cb54085a13503d7b`
`match/status.py`: retained generated blocks, module ownership and byte-weighted
fuzzy reporting; adapted the rollup to explicit Buka source-body annotations, counting
unscored annotated bodies as zero. The full census remains structural evidence.
MAX/HIST banking remains deferred until strict verification is complete; no
score policy or normalization changed. Existing logged command entry points
are retained.


## Buka assembly claim transport

Reviewed HoMM2 `e0689d3f71b2942b544fd677cb54085a13503d7b`
`build/fixed_asm.py` against the existing Giten/Gruntz-derived graph.
Retained: fixed unit/source ownership checks, period MASM, comparison COFF,
link OMF, and the ordinary source-claim fragment path. Adapted: assembly retail
addresses and sizes move from Python literals to `config/retail/asm_claims.tsv`;
configuration and assembly label edges explicitly depend on the table. The
reader rejects contradictory source ownership, duplicate names and invalid
extents. No new assembly implementation or independent adapter was added.
Remaining inherited rows are explicitly marked for migration.


## VC6 empty string storage

Reviewed the pinned Giten `39384dc6726478357b5efd42c66522781e8310fe`
and current Giten `d675d472ff0f350a7e27bcf3b9a2c7548e2bb77c`, Gruntz
`ae5226ce34959b98f1eed2e58e437638273f2f84` literal-pool enrollment,
and HoMM2 Buka `e0689d3f71b2942b544fd677cb54085a13503d7b`
`build/candidate_data_manifest.py`. Retained: the existing relocation-paired
literal path, exact payload and storage checks, strict identities and addends.
Adapted: VC6's empty `$SG` string may occupy one byte of `.bss`, whose payload
comes from PE loader-zero semantics. Its address requires one corroborated
relocation result; unreferenced neighbors and conflicting addresses remain
withheld. HoMM2 likewise excludes BSS from payload-occurrence searches.
Giten/Gruntz's initialized-pool restriction does not cover this VC6 case.
Deferred: broader BSS placement and the existing whole-image migration gates.
No parallel exporter or new tooling entry point was introduced. Real-COFF
controls cover the positive case, absent references, adjacent zeros, nonzero or
unmapped payloads, incompatible storage, and compiler-proven BSS inside PE
FileAlignment slack. See [the compiler observation](patterns/vc6-empty-string-bss.md).

The Buka RVA pass also exercises Clang extraction of the Audiere units for
which VA annotations were previously absent. The Giten/Gruntz lowercase SDK
mirror and HoMM2 `init/clangd.py` at the revisions recorded above have no VC6
STL syntax repair. The existing compdb generator now adds a generated,
Clang-only overlay: explicit template specializations, defaults retained at
first declarations, and qualified iterator/ios flag names. `/EHsc` permits
parsing SDK throw expressions. The original VC6 headers, compiler profiles,
matching objects and source bodies are unchanged. This adapts metadata
extraction without substituting a different STL or bypassing failed units.

The full RVA pass exposed one baseline eligibility omission: the ordinary
Giten-derived model materializes both `src` and `src_compgen` from real COFF
bodies, while the temporary lower-bound reporter admitted only `src`.
`VA_COMPGEN` bodies now pass through the same reviewed-identity, report-body,
reference-site and absolute-relocation checks. Declarations and unbound
initializers remain ineligible. No byte normalization, reference rule,
denominator or MAX policy changes. The previously reviewed complete scalar
destructor controls provide the end-to-end positive controls; the existing
unknown/conflicting-reference tests remain negative controls.


## Locale-specific format arguments

Reviewed HoMM2 Buka `e0689d3f71b2942b544fd677cb54085a13503d7b`
`build/catalog.py` and `build/localization.py`, and Gruntz
`0e590d1189058c534523ba0c4324c08122ab2ee4` `clean/__init__.py`.
Retained: HoMM2's portable catalog, literal expansion, offset-preserving Clang
view, separate ordinary English objects, and actual compiler format checks;
Gruntz's explicit clean-export input enumeration. Adapted: a per-ID declaration
of both exact format signatures for retail-proven argument-list differences,
and the compile-time `HOMM1_RUSSIAN` selector. All undeclared entries retain the
original equal-signature rule. Catalog dependencies and clean exports include
the variant manifest. Negative controls reject unknown, stale and unnecessary
variants; actual Clang controls check each language's argument list. No runtime
translation layer or separate build pipeline was introduced. Gruntz has no
bilingual catalog to port. The clean-export link-order gate exposed the obsolete, unclaimed NWC CPUSPEED
unit. Its source and unused declarations were removed using the existing Buka
absence evidence; no ordering fallback or invented function claim was added.

The resulting full clean tree generates successfully and both locale branches
render from it. The final ordinary English DRAWING and KB objects compile with
the pinned VC6 compiler; the table's complete strings equal NWC retail. The
Russian build retains its literal, call and jump-table controls. Full clean
executable equivalence is not claimed by these focused controls.


## Candidate and retail literal-referrer extents

Reviewed HoMM2 `e0689d3f71b2942b544fd677cb54085a13503d7b`
`build/candidate_data_manifest.py::_function_dir32`, Gruntz
`0e590d1189058c534523ba0c4324c08122ab2ee4`
`delink/data_manifest.py`, and Giten
`d675d472ff0f350a7e27bcf3b9a2c7548e2bb77c` at the same module path, alongside
the pinned Giten ancestry already recorded above. Retained: the existing
literal enrollment path, equal relocation-sequence length, public-anchor
corroboration and complete payload checks. Adapted: HoMM2's independent COFF
function boundary and retail claimed extent. Gruntz caps candidate operands
at the smaller of its next external symbol and the retail size; Giten's
version uses the retail size. Neither covers a larger candidate like Buka's
`DoEvent`. HoMM1 now stops at the next candidate definition or typed static
function, even when that next function has no retail claim. A real-COFF
positive control covers a longer candidate; negative controls reject consuming
references from following external and static functions. Comparison byte and
reference rules are unchanged. No new tooling entry point or adapter is added.


## Fixed-width localized character fields

Reviewed HoMM2 Buka `e0689d3f71b2942b544fd677cb54085a13503d7b`
`build/catalog.py` and `build/localization.py`, with Gruntz
`0e590d1189058c534523ba0c4324c08122ab2ee4` and Giten
`d675d472ff0f350a7e27bcf3b9a2c7548e2bb77c` export/tooling trees.
Retained: HoMM2's ID/PO catalog, exact CP1251 compiler literals, preserved
source offsets, reachable-header mirrors, and isolated English objects.
Adapted: `localization::Chars` supplies ordinary character-array initializers
for non-terminated fixed-width fields. Only character macros used by the
translation unit or its headers enter its generated header. The existing
portable renderer also carries this form through clean exports. Both methods
participate in the same catalog usage check. Gruntz and Giten have no bilingual
catalog or fixed-field renderer to inherit; those donor capabilities are
inapplicable here. No additional entry point, adapter or runtime layer is added.

Controls check quotes/backslashes, exact byte counts, source offsets, both
compiler locales, too-small array rejection, and rejection in RC strings.
Whole-table VC6 controls verify both the Russian and original English campaign
initializers. Clean-export controls verify that the copied renderer handles
these fields and retains both catalog versions. Matching rules are unchanged.


## Reviewed function ownership before body reconstruction

Reviewed Giten `d675d472ff0f350a7e27bcf3b9a2c7548e2bb77c` and Gruntz
`0e590d1189058c534523ba0c4324c08122ab2ee4` retail-label providers and model
joins, and HoMM2 Buka `e0689d3f71b2942b544fd677cb54085a13503d7b`
`build/symbol_providers.py`. Retained: source annotations take precedence,
provider labels do not establish source bodies, and census starts bound extents.
Adapted: the existing reviewed function-referent table now supplies fallback
names to HoMM1's canonical model, with an explicit build dependency. Reviewed
link-order contribution spans also attribute unnamed bodies to their TU.
Generated ordinals and unresolved original names remain unnamed. The ordinary
delinker keeps its existing referent validation and anonymous address buckets;
no separate mapping pipeline or source body is introduced. Baseline enrollment
requires an emitted source or source-compiler-generated body; reference evidence
is checked separately after retirement of the temporary score rule. Referent-only identities do not enroll matching targets.
Tests cover source precedence, unknown gaps, missing census starts, label-only
extent handling, and annotation-based matching eligibility. HoMM2's unrelated import
provider mechanisms are inapplicable to this function-ownership change.


## Buka README target enrollment correction

Reviewed Giten `d675d472ff0f350a7e27bcf3b9a2c7548e2bb77c` and Gruntz
`0e590d1189058c534523ba0c4324c08122ab2ee4` `verify/readme.py`, and HoMM2
`e0689d3f71b2942b544fd677cb54085a13503d7b` `match/status.py`. Retained:
generated module tables, strict comparison and separate score banking. Adapted:
HoMM1's diagnostic baseline now enrolls only `VA`/`VA_COMPGEN` source bodies;
retail labels and inferred TU ownership cannot create README matching targets.
This corrects the former whole-census denominator that accidentally enrolled
unannotated library/header bodies. The complete census and referent identities
remain available for structural recovery and reference checks. Annotated bodies
without usable comparisons still count as zero. Deferred: full strict build
verification. Donor-specific report schemas and carve-out tables are inapplicable;
no new command or logging path is introduced.


## Retirement of the temporary Buka reference score rule

Reviewed Giten `d675d472ff0f350a7e27bcf3b9a2c7548e2bb77c` and Gruntz
`0e590d1189058c534523ba0c4324c08122ab2ee4` `verify/scores.py::functions`,
and HoMM2 `e0689d3f71b2942b544fd677cb54085a13503d7b`
`match/status.py::_fn_fuzzy`: each reads measured per-function fuzzy scores
without a separate reference-review penalty.

Retained: strict objdiff reference/addend comparison, source-body enrollment,
missing-comparison accounting, diagnostic report isolation, stale-input refusal,
and separate MAX/HIST banking. Adapted: removed the temporary rule that replaced
an entire function's measured score with zero after one unreviewed reference.
The all-function reference sweep passed before this change. The same review
logic now emits a separate complete `reference-audit.json` on every baseline
comparison, including all failures and omitted absolute relocations rather than
stopping at the first. No comparison identities, bytes, or addends are masked.
Regression controls retain partial scores and distinguish a measured zero from
an absent comparison. Existing logged command entry points are unchanged.

Deferred: whole-image provision, placement/initializer gates and full build
verification, including dependencies outside annotated bodies. Inapplicable:
donor-specific report schemas and target carve-out tables. This scoped change
does not establish donor-wide command or behavioral parity.


## Shared reviewed ordinal-import identities

Reviewed Giten `d675d472ff0f350a7e27bcf3b9a2c7548e2bb77c` and Gruntz
`0e590d1189058c534523ba0c4324c08122ab2ee4` `delink/implib.py::resolve_iat`,
and HoMM2 Buka `e0689d3f71b2942b544fd677cb54085a13503d7b`
`imports/smackw32.def`. Retained: SDK import-library decorations, exact named
DLL exports, distinct unknown-ordinal identities, strict reference/addend checks,
and the existing logged commands. Adapted: the delinker reuses HoMM1's existing
linker validator for the selected image's reviewed DLL/slot/ordinal facts before
falling back to an anonymous ordinal. Conflicting SDK and reviewed spellings are
rejected. HoMM2's Smacker ordinal/name pairs corroborate the existing facts; no
unreviewed ordinal inference or comparison alias is added.

Controls cover an unknown ordinal, agreeing evidence, conflicting identities,
and the existing malformed/archive/image validation cases. Deferred: full-image
provisioning and final build verification. Inapplicable: donor-specific target
layouts and unrelated ABI rules. This does not establish donor-wide parity.

## Unpadded delinked function extents

Reviewed the retained Gruntz delinker pin
`81d34b204a0384a92cf3b4c641a8430256b2922e` and HoMM2 Buka's pin
`1393e24b4804cb357fdac147c68013f0aa5a9d95`. Both pad every delinked function
to a four-byte boundary with `0x90` (`append_with_padding`). Buka's unoptimized
VC6 functions are packed back to back, so a body ending at an unaligned address
gained one to three bytes absent from retail. After a switch byte-index table
objdiff decodes that fill as data and scored it as deleted bytes: 15 complete
bodies, including `GetObjectFamily` (`0x2f621`) and `PerWeek` (`0x33b9c`).

Adapted: `vostok-unpadded-function-extents.patch` appends each function at its
census extent with no fill; data sections keep the existing padding. A Rust test
checks two unaligned functions are packed contiguously. The target now carries
exactly retail's function bytes; no comparison byte is masked. The Python
fill-boundary canonicalization is retained for reviewed retail fill. The
baseline rose from 932 to 947 exact bodies with no regressed function.

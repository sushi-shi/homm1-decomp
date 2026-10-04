# Tooling inheritance

| Donor | Reviewed revision | Role |
| --- | --- | --- |
| Giten | `39384dc6726478357b5efd42c66522781e8310fe` | Active workflow and comparison donor |
| Giten (giten-enums) | `94435eff949d6190fa0b1b697caae9f21c53bb6b` | Constants work list and enum-domain/reuse review |
| Gruntz | `b1de0e555576a215898907b8ec8ed5423368883e` | Original pipeline ancestry and cross-review |
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
| Source/identity gates | Retained normal source gates, review-claims and contradictory-data-identity checks even in code mode. Data coverage/placement checks remain in the explicit later data tier. |
| Constants/enum review | Ported from giten-enums into the existing `verify` modules: `constants` gains the `config/constants.tsv` glob work list (first match wins, stale rows fail), committed floor (`--update-floor`), `--list`, `build/gen/constants_open.tsv`, strict-domain parse with retail fallback, switch-subject/store-target review details and float literals; `enum-reuse` gains the role-pair report and the `config/reviews/enum-reuse.tsv` ledger; `enum-domains` gains constant groups as non-storage and LOCAL/PARAM/RETURN width exemption; `board` reads `H1_ENUM_*` blocks and declarators. Adapted: `H1_ENUM_*`/`include/Domains.h` instead of `GZ_ENUM_*`/`EnumDomain.h`, strict view via `/std:c++20 /Zc:__cplusplus` instead of `GZ_STRICT_ENUMS`, and VC4 booleans: only Win32 `BOOL` is a boolean domain, its proven spelling is `TRUE`/`FALSE`, and `true`/`false` spellings fail (C2065; Giten's TRUE->true check is inverted). See [constants](constants.md) and [enum reuse](enum-reuse.md). Deferred: Giten's handoff evidence notes are game-specific. |
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
uses those objects with the existing comparison pipeline and counts unreviewed
functions/references as zero over the current-image game census. Missing
absolute relocations also withhold the function. Diagnostic reports are rejected
by the verified score loader; the README baseline is separately generated and
checks its input fingerprint. Deferred: completing the Buka identity migration
and the existing full verification gates. No donor command parity is claimed.


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

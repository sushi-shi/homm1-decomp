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

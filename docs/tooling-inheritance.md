# Tooling inheritance

| Donor | Reviewed revision | Role |
| --- | --- | --- |
| Giten | `39384dc6726478357b5efd42c66522781e8310fe` | Active workflow and comparison donor |
| Giten (giten-enums) | `94435eff949d6190fa0b1b697caae9f21c53bb6b` | Constants work list and enum-domain/reuse review |
| Gruntz | `b1de0e555576a215898907b8ec8ed5423368883e` | Original pipeline ancestry and cross-review |
| HoMM2 Buka | `299514f88900c0cf30ba03422c72830a38fc1cb7` | Initial capability review |
| HoMM2 Buka | `e0689d3f71b2942b544fd677cb54085a13503d7b` | Exact-overload fingerprint review |

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
| Skills/editor/workflow | Adapted all four skills, canonical instruction symlinks, Neovim VA/binding navigation, safe staged formatting and unit-block merge driver. See [workflow](workflow.md). |
| Negative controls | Retained applicable tests in `homm1 test`. The larger imported `verify selftest` has compiler/project-specific failures and missing APIs; it is a separate validation backlog. |
| Gruntz-only scanners | Deferred: `walls/calibrate`, `ehactions`, `escapescan`, `framescan`, `jccscan`, `loopscan`, `offsetscan`, `reloadscan`, `residue`, `retscan`, `signscan`, `storescan`, `thisscan`, `uninitscan`, `vptrscan` need separate applicability review and VC4 controls; the Giten diagnostic port does not establish their parity. |
| Inline-budget prediction | Deferred: VC5 thresholds need measured VC4 controls. The local gap command reports definitions/calls only. |
| Executable-section data/placement | Deferred: requires HoMM1 fixtures and the later data campaign. No initializer coverage is admitted. |
| Resources/runtime deployment | Deferred pending HoMM1 resource and deployment evidence. |
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
  exact overloads; cache v2 invalidates stale derived seeds. Legacy hash-domain
  migration is unknown provenance, not proof of an edit. Buka's argument
  normalization and inline-helper hash propagation remain deferred.
- VC4 inline EH groups stay within their full owner; packed groups retain
  separate records. Resolved offsets, handler and FuncInfo/map identities remain
  protected. Unsupported continuations fail closed. See [retail controls](../evidence/vc4-inline-eh.md).
- HoMM2's build-time README refresh, worktree-local editor roots and VA workflow
  informed the port. Its VC6 worker/compiler settings and C++11 strict-enum
  hook are not HoMM1 build controls.

## Repeat the review

Run inside `nix develop .#build`; reports belong in ignored `build/`:

```sh
homm1 audit tooling --giten /path/to/giten --whole-tree --json > build/tooling-giten.json
homm1 audit tooling --gruntz /path/to/gruntz --json > build/tooling-gruntz.json
homm1 test
homm1 build
homm1 build verify
homm1 link --dry-run
```

The audit compares committed donor blobs, including skills, hooks, docs and
editor files. Presence, content hashes and AST equality establish inventory
coverage only. New ports or removals must update the dispositions above, review
both Gruntz and HoMM2, preserve usage logging and run applicable controls.
Candidate linking remains incomplete; unresolved definitions are reconstruction
findings, not grounds for forced linking.

## Repository cleanup

HoMM3's documentation/evidence/experiment cleanups (`401806759`, `02474f5d1`,
`dce7317cc`) informed this layout. Generated audit and donor-candidate snapshots
are retired; reproducible commands remain. The completed HoMM2 stub materializer
and embedded bootstrap bodies are retired, while donor alignment remains a
research tool. Reviewed DNA/link-band/TU-segment tables remain because live
census consumers use them. Reconstruction evidence keeps retail facts and
source hypotheses; current scores come from the generated ledger/report.
Historical reports and retired scripts remain recoverable from Git history.

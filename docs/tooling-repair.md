# Campaign tooling repair checklist

Scope: all fourteen findings in the campaign tooling audit. Data byte matching,
whole-data census and executable reconstruction remain deferred. A passing pilot
does not establish completion of the general campaign tooling.

Reference priority: HoMM2 **Gold 2.1 Buka**, local branch
`decomp-gold-2.1-buka`, snapshot `299514f88900c0cf30ba03422c72830a38fc1cb7`.
HoMM2 2.0 is supplementary when relevant; 2.1 takes precedence. Use Gruntz for
configuration/dependency structure and HoMM3 for observational reporting.

Implementation policy: copy working donor implementations and their dependency
closures before considering new code. Preserve their structure and existing
tests. Document target-specific adaptations; absence of debug symbols is not
a reason to rewrite the pipeline (Buka is stripped too). The initial miniature
substitutes are not the architecture to extend.

## Donor map

| Capability | Working implementation to port |
| --- | --- |
| Token identity | HoMM3 `core/cpp_tokens.py` (copied unchanged; replaces the local lexer) |
| Source ownership, static functions, generated code | Buka `build/source_symbols.py`, `annotated_functions.py`, `symbol_providers.py` and their dependencies |
| Compiler invocation and analysis | Buka `build/cc_wrap.py`, `clang_options.py`, `build/annotated_data.py`; adapt the compiler profile to measured VC4 behavior |
| Build dependency graph | Gruntz `graph/{emit,scan,cc,ninja_syntax}.py`; use its existing stage CLIs and data contracts |
| Sparse bindings and delinking | Gruntz `retail_labels`, `model`, `delink`; Buka's source-owned stripped-image identities and relocation canonicalizers |
| Comparison freshness | Buka `build/normalized_freshness.py` (copied, command names adapted, donor tests retained) |
| Scoreboard and README | HoMM3 `match/status.py` and build reporting integration |
| Cleanliness | Buka `audit/{casts,readability,readability_contracts}.py`; Gruntz `verify` modules for gates and board |
| Code prerequisites | Gruntz `verify/{caller_callee,layout,alloc_size,vtables,library_overlap,undefined_closure}.py` |
| Navigation and queue | Buka `match/residual_queue.py`, Buka analysis commands, HoMM3 `sema` |

HoMM1 differences to keep explicit: pinned PE/VC4 inputs, sparse reviewed
extents and RVA annotations, `config/retail` providers, and data-last scope.
Do not import Buka-specific absolute addresses, complete-census assumptions,
localization, or VC6 optimizer assumptions as HoMM1 evidence.

- [x] 1. Generated README reporting and freshness tests: full exact/nonexact/
      restored builds update the block; focused builds preserve it.
- [x] 2. Correct C++ token hashes, including compound operators: copied HoMM3
      `core/cpp_tokens.py`; regression checks preserve operator/literal boundaries
      and line splicing. Native rebuild remains exact for all three pilot functions.
- [ ] 3. Complete unrecovered-layout restrictions.
- [ ] 4. Unit-scoped internal symbol ownership and relocation resolution.
- [ ] 5. Explicit compiler/analysis flag contract and consistent paths.
- [ ] 6. Semantic cleanliness gates and auditable quality board.
- [ ] 7. Dependency-aware source review freshness and publication policy.
- [ ] 8. Code prerequisites: ABI/layout/allocation/virtual-call/library closure.
- [x] 9. Incremental dependency graph across analysis, model, compilation,
      delinking and comparison; focused unit builds. Gruntz graph port retains
      include scanning, generator timestamps, per-edge dependencies and stable
      object installation. Exported-worktree acceptance proves no-op behavior
      and target/other-unit isolation after a single-body edit.
- [ ] 10. Complete freshness, checkpoint consistency and recoverable publication.
- [ ] 11. General independent delinking and relocation acceptance fixtures.
- [ ] 12. Discovery, source context, mismatch diagnostics and campaign queue.
- [ ] 13. Structured Buka correspondence with provenance and CLI access.
- [ ] 14. End-to-end acceptance coverage, clean-checkout verification and docs.

Record evidence here as each item is completed. Do not check off a capability
based only on a unit test of one internal helper or a compiler-only probe.

Current intermediate validation: 190 tooling tests pass, including copied Buka
provenance-chain, Wine, residual-queue and disassembly tests and Gruntz object
installation checks. The native VC4 build matches all three pilot functions
(290 bytes); full verification accepts generated README/ledger/report
consistency and the Buka target-object provenance chain. The exported-worktree
campaign additionally checks no-op and focused rebuild behavior and nonexact
full publication. This does not close broader ownership, cleanliness, ABI,
general-delinking or source-recovery requirements.

Direct ports now in use: Buka `core/wine.py`, `build/normalized_freshness.py`,
`match/residual_queue.py`, `analysis/{disasm,xref,string_xref}.py`; Gruntz
`graph/{cc,scan,ninja_syntax}.py` and its graph wiring; HoMM3
`core/cpp_tokens.py`. The graph's stage adapters translate the current HoMM1
claim/report schema. They do not constitute a completed port of Buka's richer
source-symbol/provider and ABI/audit modules.

Semantic-audit port: Buka `audit/casts.py` and `audit/readability.py`, unchanged
`clang_options.py`, and extracted shared helpers now run in the build. Existing
donor tests were retained. The native pilot remains 3/3 exact. The physical
inventory covers six files, seven bodies (including four header bodies), and
nineteen macro definitions; every file was reread before recording its current
review. Full publication requires those reviews. The cast audit sees four
explicit sites and no high-priority sites; macro-only casts remain explicitly
unmapped lexical evidence. Item 6 remains open for the other semantic gates.
See [the port/adaptation and replacement list](tooling-ports.md).
Validation for this port: all 145 tests and `nix flake check` pass. The exported
worktree campaign passes exact/nonexact/restored publication, no-op and focused
artifact isolation, and a new check that a stale physical review blocks full
publication without changing the ledger, README or complete report.

Source-binding port: copied Buka `source_symbols.py`, `annotated_functions.py`
and their data/vtable/provider dependencies. `labels` now uses the donor's
libclang annotations, decorated identities and internal linkage; the graph uses
its inventory renderer. Donor tests are retained with target fixtures adapted.
Native VC4 contract probes confirm all twelve fixture source bindings; the
pilot retains its exact 290 bytes and unchanged function/context hashes.
Generated-code/provider canonicalization and general independent relocation
fixtures are still outstanding, so item 4 is not yet closed.
Validation: all 190 tests and the clean Nix check pass. The exported-worktree
campaign passes the complete existing acceptance sequence with Buka supplying
source identities, including focused/no-op behavior and exact/nonexact/restored
publication. The standalone donor scanner reports exactly the three admitted
source functions; its span inventory agrees with the graph's joined manifest.

# Direct tooling ports and remaining replacements

Primary reference: HoMM2 Gold **2.1 Buka**, revision
`299514f88900c0cf30ba03422c72830a38fc1cb7`. HoMM2 2.0 is supplementary.
Buka already solves source identity for a stripped image; missing HoMM1 debug
symbols does not justify a replacement architecture.

The initial implementation wrote small substitutes for established mechanisms.
Those substitutes account for the incomplete behavior tracked in
[the repair checklist](tooling-repair.md). The correction is to copy donor
modules with their existing tests, adapt inputs at the boundary, and run the
native matching loop. New tests should cover actual HoMM1 differences and
integration failures, not recreate the donor's whole test suite.

## Ports currently used

| HoMM1 implementation | Donor | Adaptations |
| --- | --- | --- |
| `core/cpp_tokens.py` | HoMM3 `core/cpp_tokens.py` | Copied unchanged; replaces the original local tokenizer. |
| `core/wine.py` | Buka `core/wine.py` | Verified VC4 compiler root and explicit environment; donor process cleanup retained. |
| `normalized_freshness.py` | Buka `build/normalized_freshness.py` | Command names and local artifact paths. |
| `graph/{cc,scan,ninja_syntax}.py` | Gruntz equivalents | VC4 invocation adapter, local exceptions; scanner and object installation retained. |
| `graph/emit.py`, `graph/steps.py` | Gruntz graph | HoMM1 claim/report schemas, sparse per-unit targets, split tool identities. These adapters still call the older local model/delinker. |
| `match/residual_queue.py` | Buka equivalent | Flat report adapter; explicit nonexact status cannot disappear at a rounded 100% score. |
| `navigation/{disasm,xref,string_xref}.py` | Buka `analysis` equivalents | Verified target path, zero-size sparse entries, local object names, diagnostic-only removal of VC4 debug labels. |
| `clang_options.py` | Buka equivalent | Copied unchanged. |
| `audit/casts.py` | Buka equivalent | Imports, H1 prefix, configuration path, additional source suffixes; reject errors outside project headers too. Classifier, cursor traversal, deduplication and review-key logic retained. |
| `audit/readability.py` | Buka equivalent | RVA spelling (including `extern "C"`), H1 macros, configuration path, source exports/untracked files and additional suffixes. Physical Ctags/macro indexing and reading-credit logic retained. |
| `audit/common.py` | Buka `build/annotated_data.py`, `audit/bool_fields.py` | Extracted the actual helper functions needed by the cast audit; no data-matching callers. Retail arguments come directly from HoMM1's generated VC4 analysis database. Prefer the libclang library paired with its Nix Python binding. |

Buka's copied cast and readability tests remain in `tests/test_buka_*.py`.
Four integration controls cover unclaimed header casts, external parse failures,
physical review freshness, and inventory of inactive/untracked source. Nix
supplies Buka's libclang binding and Universal Ctags. There are no runtime reads
from sibling checkouts.

Physical file review is distinct from token-based matching identity. A comment
edit need not reset a function's MAX score, but it invalidates the exact-file
reading record. A focused build reports that pending review; a full build
cannot bank it. Neither cast success nor Ctags success awards reading credit.
Macro casts absent from the active AST remain visible as an unmapped lexical
count, not a claim of semantic coverage.

## What still needs replacing or completing

| Area | Current limitation | Next donor work |
| --- | --- | --- |
| Source identities | Handwritten JSON-AST binding and a limited generated-code contract | Port Buka `source_symbols`, `annotated_functions` and provider dependencies; preserve scoped static ownership and emitted-symbol checks. |
| Compiler contract | Explicit local VC4 flag adapter; broad profile acceptance still incomplete | Reuse Buka wrapper/analysis contracts and validate only the VC4 differences. |
| Semantic cleanliness | Cast audit and physical inventory now copied; older text checks and layout guard still limited | Port Gruntz semantic gates and Buka readability contracts with their dependencies. |
| Code prerequisites | No complete ABI/layout/allocation/virtual/library closure | Port Gruntz's existing auditors for admitted code. Data initializer scoring remains deferred. |
| Delinking and relocation ownership | Pilot works; general source-owned symbols and independent relocation fixtures remain incomplete | Connect donor identity/provider and canonicalization modules, then exercise the independent PDB/delinker route. |
| Freshness/publication | Shared CLI lock, journal and input fingerprints implemented | Finish direct-stage/concurrent-writer audit and complete acceptance evidence. |
| Navigation and correspondence | Buka discovery tools copied; structured correspondence is still prose | Add the reviewed Buka correspondence provider and CLI; finish source context and discovery integration. |

This is an implementation gap list, not a claim that these capabilities are
unavailable in the donors. The complete fourteen-item checklist remains open
until the general tooling, rather than only the three-function pilot, passes.

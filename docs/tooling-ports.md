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
| `symbols/source_symbols.py`, `annotated_functions.py` | Buka equivalents | RVA annotation adapter, configured fragment names, VC4 arguments, libclang linkage and cursor offsets. Used by `labels` and the graph inventory writer. |
| `symbols/annotated_{data,vtables,compgen_data}.py`, `symbol_providers.py`, `fixed_asm.py` | Buka dependency closure | Copied identity scanners and provider tests. Buka fixed assembly addresses and vendor aliases excluded. Data scoring remains off; provider/canonicalizer integration is still pending. |
| `clang_options.py` | Buka equivalent | Copied unchanged. |
| `audit/casts.py` | Buka equivalent | Imports, H1 prefix, configuration path, additional source suffixes; reject errors outside project headers too. Classifier, cursor traversal, deduplication and review-key logic retained. |
| `audit/readability.py` | Buka equivalent | RVA spelling (including `extern "C"`), H1 macros, configuration path, source exports/untracked files and additional suffixes. Physical Ctags/macro indexing and reading-credit logic retained. |
| `audit/common.py` | Buka `build/annotated_data.py`, `audit/bool_fields.py` | Extracted the actual helper functions needed by the cast audit; no data-matching callers. Retail arguments come directly from HoMM1's generated VC4 analysis database. Prefer the libclang library paired with its Nix Python binding. |
| `audit/compiler_artifacts.py`, `audit/srcscan.py` | Gruntz `verify` equivalents at `b1de0e555576a215898907b8ec8ed5423368883e` | Copied guard/scanner and five donor tests. HoMM1 paths, COFF/report adapters and source suffixes; no donor-specific lifetime exceptions. Buka's lexical masking excludes quoted examples. The object report requires a fresh complete build. |

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
| Source identities | Buka now owns function annotations, decorated names and linkage; the generated-code/provider-to-delinker connection remains limited | Integrate the copied provider and generated-code contracts with relocation canonicalization. |
| Compiler contract | Explicit local VC4 flag adapter; broad profile acceptance still incomplete | Reuse Buka wrapper/analysis contracts and validate only the VC4 differences. |
| Semantic cleanliness | Cast audit and physical inventory now copied; older text checks and layout guard still limited | Port Gruntz semantic gates and Buka readability contracts with their dependencies. |
| Code prerequisites | No complete ABI/layout/allocation/virtual/library closure | Port Gruntz's existing auditors for admitted code. Data initializer scoring remains deferred. |
| Delinking and relocation ownership | Pilot works; general source-owned symbols and independent relocation fixtures remain incomplete | Connect donor identity/provider and canonicalization modules, then exercise the independent PDB/delinker route. |
| Freshness/publication | Shared CLI lock, journal and input fingerprints implemented | Finish direct-stage/concurrent-writer audit and complete acceptance evidence. |
| Navigation and correspondence | Buka discovery tools copied; structured correspondence provider and CLI implemented | Finish source context and discovery integration. |

This is an implementation gap list, not a claim that these capabilities are
unavailable in the donors. The complete fourteen-item checklist remains open
until the general tooling, rather than only the three-function pilot, passes.

The source-symbol port replaces the local literal-annotation parser and manual
JSON-AST destructor-name recovery. HoMM1 retains its JSON-AST review spans,
declaration checks and incomplete-layout guard; these are not claimed as donor
ports. A small cursor-offset adapter joins Buka identities to those review
spans. Source offsets are treated as UTF-8 byte offsets. Existing private,
protected and virtual destructor access spellings remain supported.

`symbols/profile.py` supplies manifest flags and configured fragment names;
standalone donor fixtures use an isolated VC4-compatible Clang profile. The
native VC4 contract fixture confirms all twelve source identities, including
static/overloaded/member functions and destructor spelling. The three-function
campaign retains identical source/context hashes and exact bytes/relocations.

The copied data/vtable/generated-data modules are dependency closure and future
code-reference machinery, not an enabled data matching campaign. Buka's
semantic `VA_COMPGEN` scanner and binary providers are not yet wired into the
HoMM1 legacy `RVA_COMPGEN`/manual-reference join. Consequently item 4 remains
open. The standalone scanner writes `source_symbols.csv`, never overwriting the
graph's joined `symbol_names.csv`.

The correspondence provider is target-specific mapping data, not a replacement
for a donor algorithm. It reuses the existing Gruntz-style TSV reader. Six
function correspondences record Buka Git provenance, known differences and
reviewed/hypothesis confidence. CLI lookup works offline; optional donor
verification reads pinned Git objects and never depends on the sibling at runtime.

The compiler-artifact source guard runs in the fast board and before building;
its candidate-object check also runs in full verification. The existing HoMM1
unrecovered-layout policy now uses canonical types from the Buka scanner's
already-parsed translation unit to catch aliases and object-pointer operations.
That admission policy is a HoMM1 adaptation for method-only shells, not another
source-symbol implementation. Remaining semantic and ABI gates are still open.

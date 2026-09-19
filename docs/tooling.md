# Tooling and provenance

The repository is independent of sibling checkouts at runtime. Its starting
points are local snapshots of:

- `homm3-decomp` at `d2bd86aa746da522f8d1af2c77fb657233fc4a2e`: verified input
  staging in `scripts/homm3/core/inputs.py`, the readable/mapped PE section
  distinction in `core/image.py`, CLI layout, and the Nix package definitions
  and lock for objdiff 3.7.3 and vostok-delinker.
- `homm2-decomp` at `299514f88900c0cf30ba03422c72830a38fc1cb7`: the COFF/RES
  reader and its portable tests (`scripts/homm2/core/{coff,test_coff}.py`).

The configuration layout follows `gruntz` at
`b1de0e555576a215898907b8ec8ed5423368883e`: structural RVA/kind base censuses,
separate provider tables, and a root per-TU manifest with `[build]`, `[flags]`
and `[[unit]]`. Only the channels with evidence are populated here.

Both HoMM projects dedicate their own tooling to CC0. The copied reader retains
its design notes; imports and module names have been adapted to HoMM1.
The PE report parser is new and handles this target's imports, exports and
base-relocation table. It does not guess function boundaries from prologues.

## Campaign loop

```sh
nix develop .#build
homm1 check
homm1 build
homm1 build --unit kb_poll_sound
homm1 status functions
homm1 status queue
homm1 sema diff 0x4F640
homm1 sema frame 0x4F640
homm1 sema xref 0x4F640
homm1 sema callers 0x4F640
homm1 sema blocks 0x4F640 --diff --lite
homm1 sema branches 0x4F640 --diff
homm1 verify check --tier full
homm1 audit casts --check --all
homm1 audit readability --check
homm1 test
```

`labels` shows AST-bound definitions; `model` joins those claims to sparse
retail evidence; `delink` generates independent target objects; `compare`
reads a fresh validated comparison report. `sema rva`, `disasm`, `source`,
`strings`, `xref`, `diff`, and `frame` expose the matching evidence. An
unclaimed function requires an explicit disassembly size. Xrefs describe
reviewed references in admitted code, not a complete executable call graph.

Buka 2.1's `callers`, `callees`, and `find-string` tools provide discovery
outside reviewed relocation rows. Call/jump hits are raw opcode candidates,
not admitted instruction boundaries. Unknown function sizes never confer
ownership of a census gap. Buka's `blocks` and `branches` compare the candidate
and independent target objects; a diagnostic difference returns status 1.
VC4's `.lf/.bf/.ef` debug symbols are removed only in a temporary disassembly
view, with unchanged section bytes and relocation identities verified.

`homm1 configure` emits the Gruntz-derived Ninja graph. It has separate
per-unit compiler, labels/strict-analysis, target extraction and comparison
edges, with a shared sparse model join. A source-body edit can update the
candidate and report without extracting the retail target again. Compiler
objects are validated, timestamp-stabilized and installed only when changed.
The Wine runner is copied from Buka, including process-group timeout cleanup.
Whole-image linking and data matching remain deferred.

`probe --contracts` exercises the native compiler's calling conventions,
constructor/destructor names, member/static/overloaded functions, virtual
calls, enum storage, class layout, switch and exception code. It confirms
source bindings against emitted symbols and checks that `/Z7` leaves code
unchanged. `probe --ids vc20 vc22 vc40` remains the AppAbout-only historical
optimization control. `toolchain symbols --id vc40` indexes verified SDK/CRT
library membership; membership never supplies a guessed retail address.

## Source and quality contracts

Source compiles directly under the selected MSVC profile. Clang has separate
retail-language and strict-domain compilation databases under `build/analysis`.
The period compiler's objects are the only candidate code that is scored.
HoMM2-style enum helpers expose strict domains to analysis and the explicit
integer representation to MSVC. There is no source transpilation or rewriting.

`RVA(rva, size)` attaches to an actual definition in the Clang AST. Place it
inside an `extern "C"` declaration, before its return type. Claims can include
multiple functions, static/member functions, overloads, constructors and
destructors. Period-compiler COFF must confirm every claimed symbol; extra
emitted bodies fail ownership checks. `RVA_COMPGEN(rva, size, "symbol", owner_rva)`
attributes generated code to an existing source definition. Unsupported or
ambiguous bindings fail explicitly. Retail extents never come from sparse gaps.

Cleanliness runs from the beginning:

- Fast checks cover source/header inventory, banned assembly/vtable/codegen
  idioms, compiler-specific behavior forks, declaration placement, and scoped
  debt for provisional names, reinterpret casts and volatile use.
- Normal checks add source bindings, retail identity/fixup integrity,
  C-style-cast rejection, strict domains, unresolved-layout restrictions and
  source-review freshness. The copied Buka cast audit classifies explicit casts
  in active project source and headers and rejects unreviewed high-priority
  sites, stale exceptions, and parse errors (including external SDK errors).
- Full checks additionally require a fresh complete binary comparison and
  current physical file reviews. The copied Buka Ctags inventory includes
  header bodies, inactive bodies, macros and conditional variants. Normal and
  focused builds report unread files; full publication requires review.

`verify board --tier normal` measures the semantic audits too. Generated audit
reports are under `build/audit` and `build/readability/inventory`; their presence
does not grant review credit. Builds regenerate them from current source.
The lexical cast census explicitly reports casts outside the active AST (for
example macro definitions); those are not advertised as semantically checked.
Broader ABI and ownership gates remain on the repair checklist.

Human review records are distinct from automated checks. A changed function
invalidates its recorded review; a function without a record is reported as
pending, never automatically credited as read. Unknown layouts permit pointer
and method declarations, not fabricated field layouts, allocation or sizeof.
New RVAs may introduce explicit, evidenced debt; they do not get blanket waivers.

## Delinking and exactness

The synthetic PDB describes claimed function extents and reference identities.
Unclaimed code identities carry zero extents. The pinned delinker attempts to
process unrelated data relocations, so it receives an ignored **metadata view**
of the executable with unrelated relocation-directory records disabled. No
code/data operand is altered in that view. The hash-pinned original executable
remains the sole comparison oracle.

Delinker output can add alignment bytes, attach zero-size external definitions,
and synthesize data ownership. Target normalization first resolves every
retained code byte against the original executable, then emits canonical code
sections with reviewed fixups and undefined data references. Unknown code or
unreviewed fixups fail. Candidate objects never receive retail-size trimming.
The bootstrap one-function carver remains covered as a separate regression
oracle in the portable tests.

VC4 `/Z7` COFF auxiliaries supply compiled function extents. The comparison
resolves addresses and checks the full site/type/symbol/addend stream; address
fields are never masked. Retail HIGHLOW fields and outgoing direct calls/jumps
must be accounted for. Explicit embedded table ranges support code/data
boundaries; missing information fails instead of inventing a matching target.

## Checkpoints and freshness

A full successful build records **CUR / MAX / HIST**, keyed by retail RVA:
current objdiff score, best for the current function token hash, and historical
best. Source edits reset MAX to CUR while preserving HIST. Collateral changes
preserve MAX. Scores are observational; a non-exact but structurally valid,
clean implementation is a successful build. Exactness is reported separately
and requires resolved bytes plus relocation equality.

Dropped/resized claims and integrity/cleanliness failures remain fatal. Missing
measurements cannot enter the ledger. `build --unit` writes an isolated unit
report and never banks a repository checkpoint. Reports reject changed source,
headers, configuration, tools, targets or object content. Compiler failures
remove stale output; only the COFF timestamp is normalized in candidate objects.

## Scope and provenance

The campaign currently includes AppAbout and the PollSound/ForcePollSound
fragment; see `evidence/poll-sound.md`. Neither source filename proves original
HoMM1 TU ownership. Four referenced storage identities are sufficient for this
slice; their initializers and enclosing ownership remain unscored.

Additional ports/adaptations use the sibling snapshots named above: Gruntz's
verification tiers, source bindings, unified model and PDB stream repair;
HoMM2's direct-source enum modes, destructor spelling correction, stack-frame
comparison and KB/kbwin source evidence; HoMM3's observational checkpoints and
semantic inspection interface. Modules retain local provenance notes. There
are no runtime imports or dependencies on sibling checkouts. The HoMM2 global
`AUDITS = False` switch and complete-census assumptions were not copied.

Whole-data census/ownership, initializer scoring, vtable/data-table byte
matching, complete source-recovery certification, and executable linking are
later work. Minimal references and code prerequisites do not claim data matching.

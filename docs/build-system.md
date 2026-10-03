# Build system

The manifest `config/units.toml` selects source owners and complete compiler
profiles. `homm1 configure` emits `build/build.ninja`; the normal entry point is
`homm1 build` inside `nix develop .#build`. It compiles enrolled C++ and MASM
units, extracts claims, joins the retail model, delinks, normalizes, compares,
refreshes README, and runs fast/normal verification. `homm1 link` is opt-in.

The compiler is pinned VC4 under Wine. Clang reads source and ABI declarations;
it does not determine code matching. The generated clang-cl database includes
per-unit ABI-relevant flags and lowercase mirrors of the era headers. Toolchain
identity is a declared graph input. Inherited CL/INCLUDE/LIB state cannot override
the build profile. Compiler outputs, Wine state and retail inputs stay in `build`.

Source annotations from `include/match.h` carry absolute VAs. Extraction converts
them to RVAs and emits `build/gen/claims/<unit>.tsv`. `VA_DECL` identifies an
external function without assigning its body to the declaring unit. The model
joins source, reviewed retail tables and library providers into
`build/gen/bindings.tsv`; ownership must not be inferred from declaration use.

The model supplies a synthetic PDB to Vostok. Target objects live under
`build/objdiff/target-new`; candidate objects under `build/objdiff/base`.
VC4 EH records, compiler-generated code and reviewed referents retain their
identity paths. Unrecognized destructor/static-init instruction forms fail
closed rather than creating guessed claims. MASM units use explicit claims and
candidate linking preserves their OMF conversion path.

Disposable comparison copies live under `build/objdiff/compare-new`. They
normalize compiler-private names, weak externals and supported jump-table labels
without changing the raw objects. Pairing is based on actual target objects;
missing objects are not silently replaced by an empty comparison.

## Incremental loop

`homm1 match BASE/MOUSEMGR` or a source path compiles selected units and claims.
Model binding changes invalidate all delinked targets; otherwise existing targets
are reused. Selected comparisons are refreshed and objdiff generates the report.
This loop does not run unrelated source gates. Use a full build for cross-unit
changes and `homm1 build verify` for final gate verification. Ordinary builds
refresh fingerprints and the MAX-based README without banking. Outputs that are unchanged preserve their content
and timestamps where supported, stopping downstream work.

## Data matching

Current policy is code first: `config/compare.toml` sets `data_matching=false`.
Eligible code references to data normalize to `$data+0`; function calls, IAT and
EH identities remain strict. A 100% score in this mode is not full data-reference
or executable identity. The raw objects preserve the deferred evidence.
Identity contradictions remain checked by `verify data-identity`; initializer
and placement coverage belong to the later data phase.

The ledger records its mode. On a deliberate mode change, rebuild comparisons,
then use `homm1 verify bank --rebase-data-matching` to establish that mode's
scores. The normalizer refreshes all pairs even if the triggering build selected
one unit. MAX/HIST from different modes are never compared. Re-enabling strict
mode also requires resolving deferred data ownership, definitions and placement;
it is not part of the current campaign.

## Verification and integration

`homm1 verify readme` refreshes generated status independently of banking.
`homm1 verify check` runs MAX plus fast/normal gates; `--tier full`, `data`, or
`link` opt into further checks. The imported
whole-donor selftest remains a separate validation backlog, documented in the
inheritance review. No failing gate authorizes weakening it or fabricating source.

To enroll a TU, add its evidence-backed source/owner and full flag profile to the
manifest, use ordinary types and VA annotations, configure and build. Do not use
TU ownership changes merely to shift a score. See [workflow](workflow.md) for
formatting, the manifest merge driver and skills.

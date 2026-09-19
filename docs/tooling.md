# Matching tooling

The campaign tooling is a direct port of the local Gruntz repository at
`b1de0e555576a215898907b8ec8ed5423368883e`. The package was copied as
`scripts/homm1`, mechanically renamed, and then changed only where the target
requires different inputs: `HEROES.EXE`, VC4, HoMM1 section/import layout and
the smaller source manifest. There is no compatibility adapter between HoMM1
and the donor pipeline.

The copied graph is the campaign loop:

```text
source -> VC4 objects -> source claims -> unified model
       -> synthetic PDB -> Vostok delinked objects
       -> normalized object pairs -> objdiff report
       -> MAX regression gate + source cleanliness gates
```

`flake.nix` pins the same patched Vostok and objdiff revisions as that donor.
The Vostok and objdiff patches are kept under `nix/patches/` so a clean checkout
reproduces the tools used to make the report.

## Normal loop

Run the campaign inside the build shell:

```sh
nix develop .#build
homm1 configure
homm1 build
homm1 match
homm1 sema diff 0x0004f640
homm1 verify status
homm1 test
```

`homm1 build` is incremental and runs the default fast and normal verification
tiers. `homm1 match` performs the same build and prints the units whose object
content changed. `homm1 verify check --tier full` enables the slower code
evidence gates. The separate `data` tier is intentionally opt-in until data
matching begins.

The first labeling pass contains 386 functions in 31 inferred donor TUs. It
accepts 383 functions only where HoMM2 Buka 2.1 and PoL 2.0 independently name
the same logical function. Buka 2.1 takes precedence for names and module
order; PoL 2.0 supplies declarations known to compile with VC4. `AppAbout`,
`PollSound`, and `ForcePollSound` are reconstructed and byte-exact. The other
functions deliberately have empty carcass bodies so the matching campaign can
start from named, compilable TUs.

Source annotations use `VA(0x004xxxxx, size)`. Claim extraction subtracts the
PE image base and keeps the model, comparisons, and evidence tables in RVA
space. The hand-owned `functions.tsv` partitions all 1,250 `.text` function
starts; it was admitted from a one-time Ghidra analysis and is never regenerated
by the build. Data names needed by code relocations come from reviewed evidence,
but all four data bodies remain unclaimed and unscored.

The alignment evidence is committed under `evidence/homm2-label-*.tsv`.
`scripts/homm1/labels/donor_align.py` compares instruction shapes, strings,
calls, and function order against both donor builds. Its materializer places
accepted declarations in `src/carcass/{BASE,SOURCE}` and keeps the normal source
claim path; there is no label-provider adapter.

## Candidate linking

Linking is preserved as the donor's opt-in second phase:

```sh
homm1 link --dry-run
homm1 link
```

It runs the pinned VC4 linker without `/FORCE`, emits a map when the link is
complete, and therefore treats unresolved or duplicate symbols as findings.
The tooling synthesizes WinG and WAIL import libraries from the retail import
table. `smkwai32.dll` is ordinal-only, so its import library remains deferred
until matching code needs it.

The real link reaches LINK.EXE. With this batch it has only four unresolved
symbols: `gbInPollSound`, `gbForegroundApp`, `gNextSoundPollTick`, and
`gpSoundManager`. They are the deliberately deferred data definitions; code
closure, `_WinMain@16`, and the previously external helper calls now resolve.

## Target-specific evidence

HoMM1 has no debug symbols. The synthetic PDB assigns anonymous functions to
address buckets and names only source claims, exact import identities and
reviewed referents. `config/retail/function_referents.tsv` carries three exact
external function names proven by the pilot objects and decoded retail calls;
it does not claim those bodies for a source unit. Missing vendor import-library
names use stable `(DLL, ordinal)` identities until the original library or a
reviewed symbol map is available.

The default cleanliness tiers keep the donor checks that apply from the first
function: score regression, cast debt, enum domains, label spelling, include
order, unique names, library overlap, TU order, dead code and undefined-symbol
closure. Gruntz fixtures tied to its source tree, mature data census and review
walls are not in the default HoMM1 tier. This keeps the active gates strict
without pretending that deferred data work is complete.

The port deliberately omits Gruntz-only gameplay, resource, LSP, permutation,
wall-ledger and play-launcher features. Candidate linking, delinking, semantic
inspection, comparison, score banking and README status refresh are retained.

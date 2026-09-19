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

The current pilot contains two units and three exact functions: `AppAbout`,
`PollSound`, and `ForcePollSound`. The report scores 290/290 claimed code bytes
exactly. The hand-owned `functions.tsv` partitions all 1,250 `.text` function
starts; it was admitted from a one-time Ghidra analysis and is never regenerated
by the build. The data census contains only the four identities required by the
pilot code. Initializers, ownership and data bytes remain outside the score.

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

With the current three-function pilot, the real link reaches LINK.EXE and stops
on eight expected reconstruction gaps: `KBTickCount`, `PollRemote`,
`soundManager::PollSound`, four global storage definitions, and `_WinMain@16`.
These are source backlog, not a missing link stage.

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

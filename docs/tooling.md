# Matching tooling

The campaign tooling began as a partial port of the local Gruntz repository at
`b1de0e555576a215898907b8ec8ed5423368883e`, renamed into `scripts/homm1` and
adapted for `HEROES.EXE`, VC4, and HoMM1 inputs. Copying that package did not
preserve all useful Gruntz or HoMM2 behavior. The two-donor capability review,
restored features, and outstanding omissions are in
[tooling-inheritance.md](tooling-inheritance.md). There is no compatibility
adapter between HoMM1 and the donor pipeline.

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
homm1 sema disasm 0x0004f640
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
functions deliberately have empty bodies so the matching campaign can
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
accepted declarations in `src/{BASE,SOURCE}` and keeps the normal source
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

The real link reaches LINK.EXE and currently stops on the reconstruction
backlog, as intended. The current log reports 11 distinct missing game symbols
(41 unresolved references): seven deferred globals and four functions that
have not yet been reconstructed. `_WinMain@16` and the vendor import libraries
resolve. No `/FORCE` or placeholder definitions hide the remaining closure.

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

The original port omitted gameplay, resource, LSP, permutation, wall-ledger and
play-launcher features. LSP, permutation, and code-difference diagnostics are
not Gruntz-only requirements; their absence is tracked as tooling debt in the
inheritance review. Candidate linking, delinking, semantic inspection,
comparison, score banking and README status refresh are retained.

## Usage history

All Python tooling entry points append events to `build/homm1_usage.jsonl`.
This includes `homm1 ...`, `python3 -m homm1...`, Ninja's module commands,
help/invalid arguments, and each command in `homm1 sema -` batch mode.
Each invocation has start/finish records with UTC timestamps, module, original
arguments, copyable command, working directory, PID, parent invocation ID,
elapsed time, exit code, and an exception when one escapes the entry point.
Nested dispatches have separate IDs; count root starts for top-level usage.
Cross-process relationships can be inspected using PID/PPID.

Python subprocess launches made during those commands are recorded too,
including arguments and working directory. A subprocess event records an
attempted launch, not its eventual exit status; the enclosing tool's finish
records its result. A start without a finish may be running or terminated.
Stdout, stdin, environment variables and arbitrary shell commands outside
HoMM1 are not captured. This is an invocation history, not a terminal recorder.
Logs are local, ignored build artifacts and are not rotated or truncated by
the tooling. Concurrent writers lock the append; a logging failure warns once
per process and preserves the tool's normal result.

`homm1 test` checks every Python `main` remains instrumented, direct module
execution and concurrent writes, batch queries, errors, and unavailable logs.
It also checks that every copied `tool.*` CLI remains publicly reachable.

Run `homm1 audit tooling --json` to repeat the complete pinned Gruntz module
inventory (use `--gruntz /path/to/checkout` if needed). This reports missing and
adapted modules; it does not certify behavioral parity.

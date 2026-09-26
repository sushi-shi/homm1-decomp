# Matching tooling

The pipeline is adapted from Giten at
`39384dc6726478357b5efd42c66522781e8310fe`, following the original Gruntz port.
It uses HoMM1's pinned February 1996 Windows executable and VC4 toolchain.
The capability dispositions and validation limits are recorded in
[tooling-inheritance.md](tooling-inheritance.md).

```text
ordinary C++ -> VC4 objects + source claims -> unified retail model
            -> synthetic PDB -> Vostok delinked objects
            -> normalized object pairs -> objdiff -> score ledger
```

## Code first, data later

`config/compare.toml` selects `data_matching = false`, matching the pinned
Giten checkout's campaign mode. There is one active score. Eligible data
references in code are normalized to a shared symbol and zero addend;
function calls, import identities, and EH identities remain checked.
A 100% score in this mode means a code-mode match, not complete data-reference
or executable byte identity. Data initializer coverage is not claimed.
The identity audit still detects contradictory names for the same retail datum.

The score ledger records its comparison mode. After changing the mode, rebuild
all comparisons and explicitly rebase with
`homm1 verify bank --rebase-data-matching`. Scores from different modes are
not comparable. Strict data matching is the later campaign phase.

## Working loop

```sh
nix develop .#build
homm1 build
homm1 match BASE/MOUSEMGR
homm1 walls diagnose 0x0044f640
homm1 sema disasm 0x0004f640
homm1 verify status
homm1 test
```

`build` runs the incremental graph and source gates. `match UNIT` compiles that
unit and its claims, updates model bindings and delinking as needed, and scores
the resulting object pairs without running unrelated source gates. A mode
change refreshes every existing comparison pair, including in a selected-unit
build. Full delinking is retained when global bindings change.

Use absolute `VA(0x004xxxxx, size)` annotations in source; the model and evidence
use RVAs. `VA_DECL(address)` gives an existing external function declaration a
retail identity without claiming its body. Retail claims and relocation identities
remain evidence, not inferred source ownership. Static initializer/destructor
attribution accepts only recognized instruction forms.

`homm1 walls` exposes pair, semantic, relocation, and stack diagnostics.
`homm1 lsp` exposes clangd navigation and rename operations.
`homm1 permute` exposes reviewed variants and disposable compiler-state trials.
Trials retain evidence under ignored `build/`, use the normal comparison
transform, and restore source. Historical-100 checks prevent routine state
search on already-matched functions; state trials cannot bank a score.
VC5-specific inline predictions and uncalibrated donor control tables are not
validated VC4 models. Consult command help before selecting an experiment.

## Candidate linking

```sh
homm1 link --dry-run
homm1 link
```

The pinned VC4 linker receives the real object and library list. Unresolved
symbols remain reconstruction findings; `/FORCE` and placeholder definitions
must not hide them. Import libraries are generated from reviewed retail imports.

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
Finish records include `outcome` (success/difference/error), an error category,
and a bounded 16 KiB diagnostic tail on failure. Ninja output is streamed through
the logger so compiler failures are captured. Successful routine output, stdin,
environment variables and unrelated shell commands are not stored. A copyable
completion line is also appended to `build/homm1_usage.log`. Query families
`sema` and `walls` treat exit 1 as a difference, matching Giten.
Logs are local, ignored build artifacts and are not rotated or truncated by
the tooling. Concurrent writers lock the append; a logging failure warns once
per process and preserves the tool's normal result.

`homm1 test` checks every Python `main` remains instrumented, direct module
execution and concurrent writes, batch queries, errors, and unavailable logs.
It also checks that every copied `tool.*` CLI remains publicly reachable.

Run `homm1 audit tooling --json` for the pinned Gruntz inventory, or
`homm1 audit tooling --giten /path/to/giten --json` for Giten. Both compare
committed donor blobs. Presence and AST equality do not certify behavioral parity.

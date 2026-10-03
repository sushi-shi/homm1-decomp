# Matching tooling

The [Giten-derived pipeline](tooling-inheritance.md) uses HoMM1's pinned retail
image and VC4 profiles:

```text
C++ + VA claims -> VC4 objects + retail model -> PDB/Vostok delinking
               -> normalized object pairs -> objdiff -> score ledger
```

## Working loop

```sh
nix develop .#build
homm1 walls inventory --todo --limit 10
homm1 walls priors 0x0044f640
homm1 match BASE/MOUSEMGR
homm1 walls diagnose 0x0044f640
homm1 sema disasm 0x0004f640
homm1 verify status
homm1 build
homm1 build verify
```

`match UNIT` is the selected-unit edit loop. Use `build` for cross-unit changes,
and `build verify` for final gates. Source uses
absolute VAs; the model uses RVAs. `VA_DECL` identifies a declaration without
claiming a body. See [build details](build-system.md) and the [command map](tooling-map.md).

`config/compare.toml` selects strict comparison (`data_matching = true`):
data-reference identities and addends are checked together with calls, imports
and EH identities. Exact code scores do not establish executable identity.

[CUR/MAX/HIST and banking](match-status.md) have one contract. README generation
previews the bank rules without writing the ledger. `verify bank` explicitly
updates it. [Permutation trials](permuter.md) restore source; only an audited
exact `state --record-max` result can retain MAX/HIST for the unchanged function.
Sub-100 probes remain diagnostic. VC5 compiler patterns require VC4 validation.

`homm1 link` uses the real objects and pinned linker. Unresolved definitions
remain reconstruction findings; never hide them with `/FORCE` or dummy bodies.

## Usage history

Every Python entry point, direct module invocation and `sema -` batch query
logs to `build/homm1_usage.jsonl`. Start/finish events carry invocation and parent
IDs, UTC time, arguments, copyable command, cwd, PID, duration and exit code.
Subprocess events record launch attempts, not child completion. A start without
a finish may be running or terminated.

Finishes distinguish success, difference and error, with an error category and
a bounded 16 KiB failure tail. Ninja output streams through the logger;
`sema`/`walls` exit 1 is a difference. `build/homm1_usage.log` contains copyable
completion lines. Successful routine output, stdin and environment are not
stored. Concurrent appends lock; logging failures warn once without changing
the command result. Logs are ignored and are not automatically rotated.

`homm1 audit usage` checks that every entry point keeps usage logging.

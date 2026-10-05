---
name: wall-identifier
description: Classify a HoMM1 matching WALL before spending effort on it. When a reconstruction plateaus below 100% and no spelling obviously closes it, name WHICH VC4 decision diverged - inline/call-set, control flow, register/schedule, or masked/referent - and route to the lever for that class. Start with `homm1 walls diagnose ADDRESS`. Use when a function is stuck, when triaging plateaus, when asked "why won't this match" or "what kind of wall is this". Complements `matcher` (reconstructs) and `permute` (breaks proven codegen residue); this one DIAGNOSES.
---

Use HoMM1's pinned VC4 profile and absolute `VA(...)` source annotations.
The active score is strict (`data_matching=true`): data-reference identities
and addends count. `AGENTS.md` and the user's instructions take precedence.


# wall-identifier — classify the wall before fighting it

The pinned VC4 is deterministic: `bytes = f(preprocessed TU, flags)`.
A confirmed compiler profile permits controlled source comparisons; a
persistent discrepancy may still require checking the profile itself. The unit of
reproduction is the whole TU: some residue is front-end state no local body
edit can reach. Each class below has a different lever, and two of the four are
not permute problems at all.

## Start here

- `homm1 walls diagnose <rva> --asm` — classifies the residual from the same
  normalized base/target pair objdiff scores (no recompile): first divergence
  class, call/branch/return counts, and both sides' leading instructions.
- `homm1 walls semdiff <rva>` — operand, FP-opcode, constant, and ordered
  referent comparison over that pair.
- `homm1 sema match <unit|rva>` — current % vs best-ever (proven headroom?).
- `homm1 sema disasm <rva> --blocks` — retail-only basic-block view.
- `homm1 verify assert-relocs <rva>` — the actual referent set, unmasked.

## The four classes, in routing order

Do not call a wall class N while class N-1 still diverges.

| class | deciding signal | lever |
|---|---|---|
| **inline / call-set** | out-of-line CALL multiset differs | body completeness, inline boundary, or duplicated call tail |
| **control flow** | block, branch, or ret COUNTS differ | source construct — structural matcher work |
| **register / schedule** | counts and branch sequence agree; operand order, spills, coloring differ | source-shape checklist, then classified `homm1 permute state\|variants` |
| **masked / referent** | masked diff identical but score < 100 | referent identity — labeling work, not codegen |

### inline / call-set

Read the unit's flags from `config/units.toml`. Inline thresholds and template
behavior observed with other compilers are hypotheses, not HoMM1 facts.
Prove each missing expansion with the complete caller and a real VC4 control.

- The class is **inline / call-set**, not "inline budget". A call-count delta
  can also be a duplicated or merged call-carrying exit tail, and an
  equal-count callee substitution can be a wrong identity. Name the differing
  sites before choosing a lever.
- `REPEATED-SITE DELTA` (a direct callee present on both sides with different
  counts) does not distinguish a per-site inline decision from a cross-jumped
  call tail. Locate the sites and check the retail jumps first.
- A missing expansion usually means the CALLER's body is incomplete — budget
  follows statement mass. Finish the caller before touching the callee.
- `homm1 walls inline-model --gap <rva>` reports candidacy evidence;
  VC5 budget measurement is not a supported VC4 prediction.
  `llvm-nm build/objdiff/base/*.obj | grep <mangled>` screens which TUs emit a
  COMDAT.
- Never land a forcing device (PMF ref, dllexport, artificial caller) to
  materialize a COMDAT.

### control flow

A count mismatch is a reconstruction problem. Candidate source regimes include — separate returns, `goto fail` to one exit, and a
total `||`/`&&` collapse — plus loop-form effects (`while` versus
`do/while`, backward gotos). See the matcher lever catalog, sections 6-7.

One narrow exception is proven: when the first real divergence is an earlier
register rotation and every extra edge is confined to the returns of an
inlined value-only accessor, register availability can decide whether global
optimization factors the caller tail. That is a branch- or return-count delta
downstream of coloring with no authored CFG difference. Require the complete
signature — same guards, call set, constants, and ordered referents; only the
accessor-return tail differs; source-shaped result/receiver/scope controls are
byte-flat. It is not permission to relabel an ordinary branch mismatch from
counts alone: follow the first divergence.

### register / schedule

Reached by elimination only. First exhaust the source-shape checklist
(matcher `references/levers.md`): widths, cv/ref boundaries,
local census and lifetimes, helper boundaries, statement grouping. One
misplaced register op can mean the TYPE is wrong (a member array modeled as
scalars, a lost aggregate). Then use classified `homm1 permute state|variants`
(the `permute` skill). TU-global effects exist
(`docs/patterns/tu-state-probe-family-decides-reachability.md`): a flat probe
sweep is evidence about the probe, not the function. Probes are diagnostics:
retain diagnostic evidence and delete them. Only an audited exact state
for the unchanged function may raise MAX/HIST through `permute state --record-max`;
sub-100 probe scores are never banked.

### masked / referent

The active comparison mode is code first: data symbol names and addends
are relaxed. Function, import and EH identities still matter. Inspect raw
objects when investigating deferred data identities; a normalized equality
does not prove those identities. Fix wrong code referents before permutation.

## What does NOT transfer from other compilers

Do not transfer VC5 or VC6 allocator models, inline thresholds, `/Ob2`
semantics, template quirks or IL capture switches without real VC4 controls.
The pattern reference labels its unmeasured hypotheses explicitly.

A reproducibly bounded residue stays visible through the derived inventory,
the MAX ledger, and a valid `@early-stop` marker; never a hand-kept wall
ledger. Consolidate a reusable lever under `docs/patterns/README.md` only with
bounded A/B evidence.

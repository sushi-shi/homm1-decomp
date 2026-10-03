---
name: matcher
description: Reconstruct and byte-match HoMM1 C++ functions, translation units, classes, globals, and referents against retail HEROES.EXE with VC4. Use for function matching, low historical-MAX work, TU reconstruction, class/type recovery, vtable or calling-convention recovery, relocation/referent correction, data modeling, and diagnosing a plateau before declaring it bounded or using the permuter.
---

Adapted from Giten `39384dc6726478357b5efd42c66522781e8310fe`.
Use HoMM1's pinned VC4 profile and absolute `VA(...)` source annotations.
The active score is strict (`data_matching=true`): data-reference identities
and addends count. `AGENTS.md` and the user's instructions take precedence.


# HoMM1 matcher

Recover the source structure that explains retail bytes. `AGENTS.md` holds the
authority, modeling rules, and validation cadence; this skill is the working
loop. Use `wall-identifier` to classify a plateau and `permute` only for a
diagnosed register/schedule residue.

Do not write per-function plan files or formal matching plans unless the user
asks. A brief note of target, hypothesis, and next compiler control is enough.

## Choose work

- Work the lowest HIST rows first from `homm1 walls inventory --todo`
  (default output is truncated; pass `--limit N`), or recover `HIST > MAX`
  rows (CUR/MAX/HIST are defined in `AGENTS.md`). A bounded `@early-stop`
  stays in that derived queue; there is no hand-kept exclusion ledger.
- Do not revisit a function whose MAX is 100%. Do not investigate an
  unrelated current-score dip; the MAX gate decides regressions at merge
  preparation (`homm1 verify check`).
- Before inventing a spelling, run `homm1 walls priors <rva>`: many rows
  already carry a written verdict in the source comment or review ledger.

## Evidence pass (before editing)

```sh
homm1 walls diagnose <rva> --asm   # first divergence class, counts, both sides
homm1 walls semdiff <rva>          # operands, FP opcodes, constants, ordered referents
homm1 sema disasm <rva>            # retail assembly (never a decompile)
homm1 sema xref <rva>              # callers/callees, identity
homm1 sema class <Class>           # vtable slots, hierarchy
```

Also read the whole source function, declaration, callers, callees, adjacent
family members, class layout, and any lineage candidate
(HoMM2 Buka 2.1 first; PoL 2.0 second). Resolve identity or layout doubts before calling
a row codegen residue. Mine history every time:

```sh
git log -S'<function-name>' -- src include config/match_baseline.tsv
git log -G'<mangled-name>.*100\.0000' -- config/match_baseline.tsv
```

For `HIST > MAX`, recover the exact source-hash transition before
inventing new forms.

## Reconstruct, then compare

Prioritize hypotheses by evidence:

1. surviving/source-oracle body and complete family;
2. identity, ABI, ownership, type, layout, referent, or missing-body defects;
3. authentic inline/helper/macro/operator/constructor boundaries;
4. local census, scope, lifetime, initialization, parameter reuse, and
   statement/control-flow shape;
5. expression, loop, library/MFC idiom, and evaluation order;
6. classified compiler-state experiments, only after semantics, call set,
   CFG, constants, and referents are credible.

[references/levers.md](references/levers.md) catalogs the levers that have
produced exact closures; scan it so the search is not limited to the first
familiar explanation. `docs/patterns/INDEX.md` lists compiler mechanisms.

For each candidate, name the source change and the expected emitted delta,
run `homm1 match <unit>`, and compare from the first
real divergence: instructions, constants, call/CFG structure, and ordered
relocations. Fuzzy alone is insufficient. A single dip does not reject a
sourced or structurally convergent base, but confirm the
feature you are chasing was absent from the baseline first.

"Checked" means a real-TU A/B was compiled, or retail/source evidence proves
the form inapplicable. Do not mix several independent levers in one
experiment. Do not retain probes, unused declarations, artificial locals, or
distorted source.

## Classify the plateau

Route in this order; do not call a wall class N while class N-1 still diverges:

1. **Referent:** masked bytes identical, relocation targets differ — fix the
   claim or identity (`homm1 verify assert-relocs <rva>`).
2. **Inline/call set:** out-of-line callee multiset or ordered relocations
   differ — incomplete body, inline boundary, or duplicated call tail.
3. **CFG:** block, branch, or return counts differ — structural source work.
4. **Register/schedule:** same calls and skeleton — widths, lifetimes, helper
   boundaries, then classified `homm1 permute state|variants`.

Details and proven exceptions: the `wall-identifier` skill.

## Model real entities

`AGENTS.md` "Source rules" governs. In practice:

- Casts are symptoms: retype the member or canonical class until placeholder
  casts disappear. Raw offsets, casts of `this`, and `.cpp`-local views are
  defects.
- Recover vtables mechanically from `homm1 sema class`: `inherited` declares
  nothing, `override` follows the existing VC4 header convention, `new` is plain `virtual`. Never add
  dummy virtuals (placeholder slots once shipped a live crash by truncating a
  vtable); one class has one real `??_7`. Use the actual per-unit flags in `config/units.toml`; missing RTTI does
  not prove a class non-polymorphic.
- Pin `__thiscall`/`__stdcall`/`__cdecl` from the disassembly (callee `ret N`
  versus caller `add esp,N`). A destructible stack local forces the `/GX` EH
  frame; unwind states are evidence of the local census.
- External engine, DirectX, Win32, and COM callees are modeled as declarations
  with no body; their `rel32`/`DIR32` referents must still be the right names.
- Code comes first. Use owner-header declarations for globals needed by code;
  admit only reviewed identity/layout evidence. `DATA(va)` is an identity claim,
  not permission to reconstruct initializers in this phase. Use the macros
  actually defined in `include/match.h`; never pin volatile compiler ordinals.
- Never model an interior address as independent storage, and never add
  source padding to fit a final-image gap.

## Stop and hand off

- Claim exact closure only from an actual compile of the intended
  function fingerprint. Remove disposable TU probes. As in Giten,
  `permute state --record-max` may retain a proven exact compiler-state peak
  after its source-hash, size and ordered-relocation checks; it changes only
  MAX/HIST, never CUR or the restored source. Sub-100 states are diagnostic only.
- Claim a bounded residue only after the applicable lever families have
  evidence-backed dispositions; then mark the complete body `@early-stop`.
  It never excuses missing logic, wrong referents, or an unresolved identity.
- Exactness does not authorize fake source, storage or weakened gates.
  Resolve a rule conflict from retail evidence and the user's instructions.
- A short or user-directed batch may stop sooner: mark remaining hypotheses
  open in the handoff.

Commit the focused source, reusable pattern docs, and baseline rows once
`homm1 build` is clean; `homm1 verify check` runs the gates at merge
preparation. Tooling changes run `homm1 test`; source changes run `homm1 build`.

Report the MAX change, the structural correction, its evidence and
compiler controls, the referent verdict, and any remaining wall.

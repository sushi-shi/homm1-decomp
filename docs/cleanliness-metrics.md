# Cleanliness checks

`homm1 verify check` runs the MAX score gate and fast/normal source/model checks.
The registry in `scripts/homm1/verify/tiers.py` is authoritative. Committed debt
floors live under `config/cleanliness`; generated findings belong in `build`.

Direct gate commands expose their evidence. Updating a debt floor is an explicit
reviewed operation, not a routine response to a failing build. No gate failure
licenses false code, wrong callees, contradictory identities or silent gate
exceptions. Run `homm1 build` for tooling changes.

Counters identify candidates, not source truth. Preserve authentic SDK/ABI types,
source-backed ownership and necessary reviewed casts. Do not replace a correct
type with a layout view, hide a cast in a helper, or invent names to lower a
count. Equal numeric values alone do not establish one enum domain
(`homm1 verify enum-reuse` compares declarations with the review ledger).
Every cleanup edit must leave the edited function exact in `homm1 match`.

## Measuring

Run inside `nix develop .#build`:

```sh
homm1 verify board               # text rows, delta vs config/cleanliness floors
homm1 verify board --semantic    # adds the build-derived rows
homm1 verify casts               # reinterpret_cast ledger + self-recursion
homm1 verify casts --nested      # libclang nested static_cast scan
homm1 verify dead-code           # @dead-code markers vs retail reachability
homm1 verify enum-domains        # range tests and enum-domain defects
homm1 verify enum-reuse          # named constants vs the reuse review ledger
homm1 verify constants           # bare numeric spellings work list
homm1 verify label-style | line-directives | include-order | localization | unique-names | source-encoding | review-claims | bans
```

Board floors (`cleanliness-{text,semantic}-baseline.tsv`) are ratchets:
`board --gate` reports every row above its floor. Re-blessing them with
`--update` is a reviewed, manual act.

## Review items

The commands above give current counts; this list records what each item
asks and the facts that bound it.

- **reinterpret_casts**: each needs a Win32-ABI type or a reason comment
  (`verify casts`); nested `static_cast` chains should not exist.
- **C-style casts**: `philAI::QuickCombat`'s `(float)` stays C-style: VC4
  narrows its double through a stack temporary only for that form
  ([float expression shape](patterns/vc4-float-expression-shape.md)). The
  `combatRemoteMessage`/`heroRemoteMessage` views over `char*` packets in
  `EVENTS` and the `game` save-transfer writes are the remaining pointer casts.
- **Unions** model record variants (DirectDraw SDK, `tag_message` word views,
  remote payloads, NetBIOS node payload), not layout workarounds.
- **gotos** in exact functions are byte evidence: replacing one of
  `PollRemote`'s `goto done` jumps with `return` drops it to 98.97.
- **Address arithmetic, owner recovery, manual varargs**: none; keep it so.
- **Out-of-object pointers and buffer bounds**: the `game` save-transfer writes
  and `tag_Node`'s one-element trailing arrays.
- **Unused stack slots**: `junk*`, `unused*`, `spare*`, `gap*`, `dummy*`, `pad*`
  locals hold retail stack slots; whether each is a real variable is open.
- **Inline helpers and macros**: the `ProcessAssert(cond, file, line + n)`
  sites are an `assert` macro candidate whose per-function line base is data.
- **Unknown identifiers and slot-tuned names**: a rename must keep the name's
  `/Od` identifier-hash bucket (`h = (h << 2) + (h >> 7) + c`, bucket `h & 15`)
  so stack slots and C1 handles do not move.
- **include-order**: `Misc`, `KB`, `REQUEST`, `comwin` and `kbwin` define
  `WIN32_LEAN_AND_MEAN` inside the include block and stay manual.
- **Board structural rows**: externs and prototypes live in the owner unit's
  header (Buka's header where Buka declares the symbol, else the retail data
  band's owner; KB-band tables in `X_GLOBAL.h`).
- **`verify casts` self-recursion**: the `(void)` overloads of `WalkTo`,
  `AttackTo` and `FlyTo` forward to the one-argument overload; the gate counts
  `void` as an argument (3 false positives).

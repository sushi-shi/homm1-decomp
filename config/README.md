# Configuration and retail evidence

This layout follows the local Gruntz repository's census/provider model.
Group configuration by what it describes. Keep retail observations separate
from build enrollment, matching scores, and source-quality policy.

## Root build contracts

`units.toml` is the sole per-TU build manifest: `[build]` identifies the
platform/toolchain, `[flags]` defines complete named compiler profiles, and
each `[[unit]]` will select `unit`, `source`, and `flags`. No units or flags
are admitted until a compiler probe and end-to-end matching build validate them.

Future measured match baselines belong here. Future source-quality audit
baselines belong in `cleanliness/`. Neither is fabricated at bootstrap.

## `retail/`: facts about the executable

- `targets.json`: size/hash pins for the game and optional editor input.
- `functions.tsv`: structural function-start census, columns `rva`, `kind`.
  Kinds follow Gruntz: empty = body, or `thunk`, `eh`, `helper`, `pad`.
- `data.tsv`: structural datum-start census with `rva`, `kind`. Empty = datum;
  other kinds are `string`, `fppool`, `vtable`, `rtti`, `ehtable`, `guard`,
  `common`, `copy`, `pad`. No data boundaries have been admitted yet.
- `functions_exports.tsv`: naming/provenance provider for PE exports, with
  `rva`, `name`, `ordinal`, `provenance`. Each row must refer to an admitted
  body in `functions.tsv` and match the pinned PE export table.

All census/provider RVAs currently refer to **HEROES.EXE**. Do not mix editor
addresses into those tables; future editor censuses need a separate namespace.

Base tables contain starts/kinds, not names, ownership, or matched sizes.
Providers may supply names and evidence; exact matched code sizes will belong
to reconstruction claims. `homm1 check` gates address spaces, ordering,
uniqueness, kinds, and the provider-to-base relationship.

The initial censuses are **sparse**, not full partitions. Unlike the mature
Gruntz census, the next admitted row does not yet determine a contribution's
extent. There is no coverage denominator or inferred size until intervening
boundaries and section tiling are established.

The build must never regenerate hand-owned base tables. `homm1 init` writes
raw observations under ignored `build/analysis/`; promote reviewed facts into
these tables deliberately. Add library/vtable/compiler-data providers,
derived link bands/order, and relocation referents when there is evidence to
populate them. Merge reviewed changes per row, preserving both sides' facts.

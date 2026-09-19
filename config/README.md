# Configuration and retail evidence

This layout follows the local Gruntz repository's census/provider model.
Group configuration by what it describes. Keep retail observations separate
from build enrollment, matching scores, and source-quality policy.

## Root build contracts

`units.toml` is the sole per-TU build manifest: `[build]` identifies the
platform/toolchain, `[flags]` defines complete named compiler profiles, and
each `[[unit]]` selects `unit`, `source`, and `flags`. `app_about` is an
admitted function fragment, not a claim of original TU ownership. Its `/Od`
profile has passed compiler probes and the full matching loop.

`toolchains.json` pins original media and each provisioned compiler/SDK/CRT component.
`match_baseline.tsv` records stable RVA/extent identity and observational
CUR/MAX/HIST scores plus per-function source-token hashes. Full green builds
update it; unit builds do not. Dropped/resized claims fail; score dips do not.

`cleanliness/debt.toml` holds occurrence-scoped, fingerprinted debt and evidence.
`cleanliness/types.toml` records unresolved layouts whose size-dependent use is
forbidden. `cleanliness/reviews.toml` records human source review by function
hash; stale reviews fail and missing reviews remain explicitly pending.
`cleanliness/file_reviews.json` is Buka's physical file-review ledger: exact
SHA-256 and a substantive reading note for every source/header file, including
inactive bodies and macros. Full publication requires current records. The
generator never updates review records. `cleanliness/cast_exceptions.tsv` holds
exact semantic identities and evidence for high-priority cast exceptions; stale
or duplicate rows fail the copied Buka audit. It is initially empty.

## `retail/`: facts about the executable

- `targets.json`: size/hash pins for the game and optional editor input.
- `functions.tsv`: structural function-start census, columns `rva`, `kind`.
  Kinds follow Gruntz: empty = body, or `thunk`, `eh`, `helper`, `pad`.
- `data.tsv`: structural datum-start census with `rva`, `kind`. Empty = datum;
  other kinds are `string`, `fppool`, `vtable`, `rtti`, `ehtable`, `guard`,
  `common`, `copy`, `pad`. Only code-required datum identities are admitted; no initializer matching.
- `functions_exports.tsv`: naming/provenance provider for PE exports, with
  `rva`, `name`, `ordinal`, `provenance`. Each row must refer to an admitted
  body in `functions.tsv` and match the pinned PE export table.
- `data_symbols.tsv`: minimal referenced storage identities, explicit extents and
  provenance. Every row must join to the sparse data census.
- `code_data.tsv`: explicit embedded jump/EH table extents inside code claims.
- `reloc_referents.tsv`: reviewed function-relative relocation ownership,
  expressed as image RVAs, kind, symbol, target RVA, addend and provenance.
  `check` verifies encoded operands, IAT identity, admitted callee starts, and
  complete HIGHLOW and outgoing direct-call/jump coverage inside each claim.

All census/provider RVAs currently refer to **HEROES.EXE**. Do not mix editor
addresses into those tables; future editor censuses need a separate namespace.

Base tables contain starts/kinds, not names, ownership, or matched sizes.
Providers may supply names and evidence; exact matched code sizes belong
to `RVA(rva, size)` reconstruction claims in source. `homm1 check` gates address spaces, ordering,
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

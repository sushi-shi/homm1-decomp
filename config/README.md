# Configuration and retail evidence

The layout follows the Gruntz census/provider model. Retail observations stay
separate from build enrollment, matching scores and cleanliness policy.

`units.toml` is the per-TU build manifest. Named `[flags]` profiles contain the
complete VC4 command line, and each `[[unit]]` selects one source and profile.
`match_baseline.tsv` is the hand-banked RVA-keyed MAX ledger used by the score
regression gate. Files under `cleanliness/` are committed ratchet floors and
small evidence-backed exceptions.

`retail/` contains hand-owned facts:

- `targets.json` pins the game and optional editor executables.
- `functions.tsv` is the complete 1,250-start `.text` partition. Its initial
  starts came from a one-time Ghidra 12 analysis of the pinned executable; the
  build never regenerates it. Extents are derived to the next start.
- `data.tsv` currently lists only the four storage identities required by the
  admitted code. Data matching and complete data partitioning are deferred.
- `function_referents.tsv` names unclaimed function targets proven by admitted
  object relocations and decoded retail operands. These names do not create
  source claims or matching-denominator entries.
- `reloc_referents.tsv` uses the donor schema for exceptional relocation
  aliases that containment cannot infer. It is currently empty.
- `link_order.tsv` and `link_bands.tsv` are empty until link-layout evidence is
  admitted.
- `functions_static_libs.tsv`, `functions_zlib.tsv`, `data_zlib.tsv`,
  `data_vtables.tsv`, `data_static_libs.tsv`, and `data_compgen.tsv` retain the
  donor provider channels and are currently empty.

Base censuses supply structure. Source `RVA`/`DATA` annotations and provider
tables supply identity. Exact function sizes come from `RVA(rva, size)` claims;
data identities do not imply that initializers or bytes are matched.

All RVAs in these tables refer to `HEROES.EXE`. Editor evidence needs a separate
namespace. Promote generated observations from ignored `build/analysis/` only
after review, and merge hand-owned tables per row.

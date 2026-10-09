# Configuration

Build contracts live here; retail facts live under `retail/`. Every file is
read by the named tooling. Generated state belongs in `build/`.

## Build and scoring

- `units.toml`: per-TU manifest. `[flags]` profiles hold the complete VC4.1
  command line; each `[[unit]]` selects one source and profile.
  `[build.source_roots]` pins the retail compiler paths on drive `F:`.
- `compare.toml`: comparison mode (`data_matching`). The ledger records its mode.
- `match_baseline.tsv`: RVA-keyed CUR/MAX/HIST ledger, written only by
  `homm1 verify bank` (`match_baseline.<image>.tsv` for another image).
- `toolchains.json`: pinned compiler media, components and release hashes
  (`homm1 toolchain`).
- `heroes.def`: historical stack contract; 1.2 has no exports. The candidate
  link passes stack sizes explicitly and omits `/DEF` (`homm1 link`).
- `constants.tsv`: numeric spellings kept on purpose (`homm1 verify constants`).
- `reviews/enum-reuse.tsv`: enum-reuse review ledger (`homm1 verify enum-reuse`).
- `reviews/bool_exceptions.tsv`: proven 0/1 contracts kept as integers and
  truth values that are not 0/1, each with its reason (`homm1 audit bool-fields`).

## Cleanliness floors (`cleanliness/`)

- `cleanliness-text-baseline.tsv`, `cleanliness-semantic-baseline.tsv`:
  `verify board` floors.
- `tu-order-baseline.tsv`, `kept-comdat-exiles.tsv`: `verify tu-order`.
- `data-tu-order-baseline.tsv`: `verify data-tu-order`.
- `types.toml`: admitted incomplete types (`verify undefined-closure`).

## Retail facts (`retail/`)

Files at the top of `retail/` describe the pinned Buka 2003 `HEROES.EXE`; a
second image keeps the same kinds of files in its own subdirectory (for
example `retail/editor/`). Addresses are image RVAs. An image that shares
units with the game also keeps `placements.tsv`, the game identities its
shared source spells joined to its own addresses (`homm1 --image editor audit
placements`); its `link_order.tsv` rows for shared units are derived there and
its image-only rows are reviewed by hand ([editor](../docs/editor.md)).

- `targets.json`: hashes of the game and editor executables.
- `functions.tsv`, `data.tsv`: hand-owned `.text`/data start censuses. They
  supply structure only; names and sizes come from source and provider claims.
- `function_referents.tsv`, `reloc_referents.tsv`, `data_symbols.tsv`: reviewed
  identities of referenced functions, relocation targets and data that the
  delinker cannot infer.
- `functions_static_libs.tsv`, `data_vtables.tsv`, `data_static_libs.tsv`,
  `data_compgen.tsv`: provider claim channels (`homm1 model`).
- `asm_claims.tsv`: MASM unit claims (`homm1.graph.fixed_asm`).
- `absolute_relocations.tsv`, `absolute_reference_evidence.tsv`: the reviewed
  absolute-reference manifest of the `/FIXED` image and its per-site evidence.
- `import_libraries.tsv`, `import_symbols.json`: import libraries whose format
  differs from the pinned LINK's output, and import names (`homm1.graph.implib`).
- `link_order.tsv`, `link_bands.tsv`: link-layout channels; admitting rows
  changes delinker ownership.
- `dna_bands.tsv`: executable DNA census against the VC6 libraries
  (`homm1 audit dna-bands`, `verify.universe`).
- `function_identities.tsv`, `reference_proofs.tsv`: reviewed function
  identities and (site, target, symbol) reference proofs for the
  `homm1 compare --baseline` reference audit.
- `localization.tsv`, `localization_resources.tsv`,
  `localization_fixed_width.tsv`: provenance (pointer slots, literal and
  resource payload hashes) of every catalog entry, checked by the
  localization tests.
- `assets.json`: hashes of the retail game data files.
- `editor/link_diff.tsv`: the editor candidate's per-region ceiling
  (`homm1 --image editor verify link-diff`; the game's is `config/link_diff.tsv`).

### Release lineage (`retail/versions/`)

Correspondence between the game's releases, described in
[version lineage](../docs/versions/README.md):

- `win95-1.0-to-1.1.tsv`, `win95-1.0-to-1.1-review.tsv`: 1.0 → 1.1 address
  correspondence and changed-function review.
- `win95-1.1-to-1.2.tsv`, `win95-1.1-to-1.2-review.tsv`: 1.1 → 1.2
  correspondence and complete old-function inventory.
- `win95-1.2-to-buka-2003.tsv`: 1.2 → Buka function and data correspondence;
  rows without a new RVA are 1.2 functions with no standalone Buka body.
- `win95-1.2-reloc-referents.tsv`: the 1.2 image's relocation referents.
- `images.json`: PE inventories of the 1.2 and Buka executables and DLLs.

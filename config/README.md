# Configuration

Build contracts live here; retail facts live under `retail/`. Every file is
read by the named tooling. Generated state belongs in `build/`.

## Build and scoring

- `units.toml`: per-TU manifest. `[flags]` profiles hold the complete VC4.1
  command line; each `[[unit]]` selects one source and profile.
  `[build.source_roots]` pins the retail compiler paths on drive `F:`.
- `compare.toml`: comparison mode (`data_matching`). The ledger records its mode.
- `match_baseline.tsv`: RVA-keyed CUR/MAX/HIST ledger, written only by
  `homm1 verify bank`.
- `toolchains.json`: pinned compiler media, components and release hashes
  (`homm1 toolchain`).
- `heroes.def`: historical stack contract; 1.2 has no exports. The candidate
  link passes stack sizes explicitly and omits `/DEF` (`homm1 link`).
- `constants.tsv`: numeric spellings kept on purpose (`homm1 verify constants`).
- `reviews/enum-reuse.tsv`: enum-reuse review ledger (`homm1 verify enum-reuse`).

## Cleanliness floors (`cleanliness/`)

- `cleanliness-text-baseline.tsv`, `cleanliness-semantic-baseline.tsv`:
  `verify board` floors.
- `tu-order-baseline.tsv`, `kept-comdat-exiles.tsv`: `verify tu-order`.
- `data-tu-order-baseline.tsv`: `verify data-tu-order`.
- `types.toml`: admitted incomplete types (`verify undefined-closure`).

## Retail facts (`retail/`)

Current census and claim RVAs refer to the pinned 1.2 `HEROESW.EXE`.
Migration tables identify both their source and destination versions.

- `win95-1.1-to-1.2.tsv`, `win95-1.1-to-1.2-review.tsv`: current port
  correspondence, mapping evidence and complete old-function inventory.
- `win95-1.0-to-1.1.tsv`, `win95-1.0-to-1.1-review.tsv`: retained historical
  correspondence and review for the earlier port.
- `targets.json`: hashes of the game and optional editor executables.
- `functions.tsv`, `data.tsv`: hand-owned `.text`/data start censuses. They
  supply structure only; names and sizes come from source and provider claims.
- `function_referents.tsv`, `reloc_referents.tsv`, `data_symbols.tsv`: reviewed
  identities of referenced functions, relocation targets and data that the
  delinker cannot infer.
- `functions_static_libs.tsv`, `data_vtables.tsv`, `data_static_libs.tsv`,
  `data_compgen.tsv`: provider claim channels (`homm1 model`).
- `import_libraries.tsv`: import libraries whose format differs from the
  pinned LINK's output (`homm1.graph.implib`).
- `link_order.tsv`, `link_bands.tsv`: link-layout channels; admitting rows
  changes delinker ownership.
- `dna_bands.tsv`: executable DNA census against VC4 LIBCMT/OLDNAMES
  (`homm1 audit dna-bands`, `verify.universe`).
- `homm2_tu_segments.tsv`: retail RVA ranges assigned to source units, read by
  the DNA census.

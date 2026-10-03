# Configuration

Build contracts live here; retail facts live under `retail/`. Every file is
read by the named tooling. Generated state belongs in `build/`.

## Build and scoring

- `units.toml`: per-TU manifest. `[flags]` profiles hold the complete VC4
  command line; each `[[unit]]` selects one source and profile.
- `compare.toml`: comparison mode (`data_matching`). The ledger records its mode.
- `match_baseline.tsv`: RVA-keyed CUR/MAX/HIST ledger, written only by
  `homm1 verify bank`.
- `toolchains.json`: pinned compiler media, components and release hashes
  (`homm1 toolchain`).
- `heroes.def`: module name and exports for the candidate link (`homm1 link`).
- `probes/compiler.cpp`: ABI fixture for the compiler-contract probe
  (`homm1.probes`).
- `constants.tsv`: numeric spellings kept on purpose (`homm1 verify constants`).
- `reviews/enum-reuse.tsv`: enum-reuse review ledger (`homm1 verify enum-reuse`).

## Cleanliness floors (`cleanliness/`)

- `cleanliness-text-baseline.tsv`, `cleanliness-semantic-baseline.tsv`:
  `verify board` floors.
- `tu-order-baseline.tsv`, `kept-comdat-exiles.tsv`: `verify tu-order`.
- `data-tu-order-baseline.tsv`: `verify data-tu-order`.
- `types.toml`: admitted incomplete types (`verify undefined-closure`).

## Retail facts (`retail/`)

All RVAs refer to the pinned `HEROES.EXE`.

- `targets.json`: hashes of the game and optional editor executables.
- `functions.tsv`, `data.tsv`: hand-owned `.text`/data start censuses. They
  supply structure only; names and sizes come from source and provider claims.
- `function_referents.tsv`, `reloc_referents.tsv`, `data_symbols.tsv`: reviewed
  identities of referenced functions, relocation targets and data that the
  delinker cannot infer.
- `functions_static_libs.tsv`, `functions_zlib.tsv`, `data_zlib.tsv`,
  `data_vtables.tsv`, `data_static_libs.tsv`, `data_compgen.tsv`: provider
  claim channels (`homm1 model`).
- `import_libraries.tsv`: vendor import libraries whose format differs from
  VC4 LINK output (`homm1.graph.implib`).
- `link_order.tsv`, `link_bands.tsv`: link-layout channels; admitting rows
  changes delinker ownership.
- `dna_bands.tsv`: executable DNA census against VC4 LIBCMT/OLDNAMES
  (`homm1 audit dna-bands`, `verify.universe`).
- `homm2_tu_segments.tsv`: HoMM2 TU segment correspondence read by the DNA census.

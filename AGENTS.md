# HoMM1 reconstruction

Use the pinned February 1996 Windows `HEROES.EXE` from
`config/retail/targets.json`. Retail bytes, RVAs and relocations are the
authority. The editor and other releases are secondary evidence.

- Enter `nix develop .#build` and run `homm1 build` after source, claim,
  compiler, delinker or comparison changes. Run `homm1 test` after tooling
  changes.
- Use `homm1 match` for the normal edit loop and `homm1 sema` for detailed
  object/retail inspection. Exactness includes relocation symbol and addend.
- Keep candidate linking working through `homm1 link`; unresolved definitions
  are reconstruction findings. Never add `/FORCE` to hide them.
- Recover ordinary C++ and real types. Do not use byte arrays, naked assembly,
  dummy bodies or address masking to manufacture a score.
- Keep retail facts under `config/retail`, build contracts under `config`,
  tooling under `scripts/homm1`, research under `evidence`, headers under
  `include`, and reconstruction source under `src/BASE` or `src/SOURCE`.
- Retail executables, compiler media/toolchains, Wine prefixes and generated
  artifacts belong in ignored `build/`.
- Data matching is last. Until then, admit only identities and layout evidence
  required by code; do not claim initializer or data-byte coverage.
- VC4 `/Od` is validated for the current three functions. Do not reuse HoMM3
  VC6 flags. HoMM2 Buka 2.1 is the preferred source-correspondence donor, with
  2.0 as secondary evidence.
- The tooling design comes directly from the pinned Gruntz donor documented in
  `docs/tooling.md`. Adapt target facts in the existing modules; do not add a
  parallel adapter pipeline.

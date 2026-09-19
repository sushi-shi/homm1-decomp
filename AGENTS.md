# HoMM1 reconstruction

Work from the pinned Windows 95 February 1996 `HEROES.EXE` in
`config/retail/targets.json`. Preserve the target survey in README.md. The editor is
secondary evidence, and the DOS and 1997 builds are different targets.

- Run `./homm1 check` after changing binary metadata and `./homm1 test` after
  changing tooling. Enter `nix develop` if Python/tools are missing.
- Retail bytes, addresses and relocations are authoritative. Label guesses.
  Located/exported functions are not matching claims. Sparse census gaps are not known extents.
- Recover ordinary C++ and real types. Do not substitute byte arrays, naked
  assembly or dummy source just to produce a score.
- Keep tools under `scripts/homm1`, retail facts under `config/retail`, build contracts under `config`, research
  under `evidence`, headers under `include`, and code under `src/BASE` or
  `src/SOURCE` once ownership is evidenced.
- Retail inputs, extracted assets, compilers, Wine prefixes and generated
  reports belong in ignored `build/`. Do not commit them.
- Compiler identity and flags remain unconfirmed. Do not reuse HoMM3 VC6
  settings as if validated. See docs/tooling.md for the compiler bring-up path.
- Use sibling repositories as references without modifying or depending on
  them. Preserve provenance for borrowed tooling.

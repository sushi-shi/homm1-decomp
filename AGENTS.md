# HoMM1 reconstruction

Work from the pinned Windows 95 February 1996 `HEROES.EXE` in
`config/retail/targets.json`. Preserve the target survey in README.md. The editor is
secondary evidence, and the DOS and 1997 builds are different targets.

- Run `./homm1 check` after changing binary metadata and `./homm1 test` after
  changing tooling. Enter `nix develop` if Python/tools are missing.
- Run `homm1 build` in `nix develop .#build` after source, compiler-wrapper,
  claim or relocation changes. Exactness includes resolved relocation bytes
  and the site/type/symbol/addend stream; never mask address fields.
- Retail bytes, addresses and relocations are authoritative. Label guesses.
  Located/exported functions are not matching claims. Sparse census gaps are not known extents.
- Recover ordinary C++ and real types. Do not substitute byte arrays, naked
  assembly or dummy source just to produce a score.
- Keep tools under `scripts/homm1`, retail facts under `config/retail`, build contracts under `config`, research
  under `evidence`, headers under `include`, and code under `src/BASE` or
  `src/SOURCE`. Mark function fragments whose original TU ownership is unknown.
- Retail inputs, extracted assets, compilers, Wine prefixes and generated
  reports belong in ignored `build/`. Do not commit them.
- Run `homm1 verify check --tier full` before banking/committing campaign work.
  Scores are observational; source/identity/relocation integrity remains strict.
  Refresh a human source review only after actually re-reading the changed body.
- Data matching is last. Admit only code-required identities/layout evidence;
  never use sparse census gaps as boundaries or a coverage denominator.
- VC4.0 `/Od` is validated for AppAbout and the small PollSound fragment only. VC2.0 and VC2.2 emit the same
  callback; historical compiler identity and engine-wide flags remain open.
  Do not reuse HoMM3 VC6 settings. See docs/compiler.md for measured evidence.
- Use sibling repositories as references without modifying or depending on
  them. Preserve provenance for borrowed tooling.

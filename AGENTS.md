# HoMM1 Buka 2003 reconstruction

Byte-matching C++ reconstruction of the Buka 2003 Windows `HEROES.EXE`
(`config/retail/targets.json`), built with the pinned Visual C++ 6.0 SP5
toolchain. Retail bytes are the authority. Every annotated function matches;
the remaining goals are a byte-identical linked executable and the scenario
editor (`EDITOR.EXE`) as a second target.

## Build and gates

- Work inside `nix develop .#build`. Run `homm1 build` after any source,
  claim, config or tooling change.
- Every commit keeps `homm1 compare --baseline` at 100% and `homm1 build
  verify` passing. `homm1 link` builds the candidate executable; never use
  `/FORCE`.
- `homm1 build verify` includes `homm1 verify behaviour`, the game-behaviour
  tests ([workflow](docs/workflow.md#game-behaviour-gate)). Their expected
  outputs are snapshots of retail behaviour, so change one only on purpose.
- The README status block is generated (`homm1 verify readme`); never edit
  it by hand. `homm1 verify bank` updates the score ledger.

## Source rules

- Ordinary C++ with real types. No decompiler output, byte blobs, naked
  assembly, dummy bodies, address masking, pragmas, or helpers that only hide
  a cast (`IDX`-style); enum-indexed data uses the typed enum wrappers.
- Data identities, types and initializers come from retail bytes and their
  code users.
- Compiler profiles live in `config/units.toml`; changing one needs
  retail-backed evidence.
- Comments describe behaviour, not how a match was achieved. Keep cast
  reasons, `VA`/`DATA` annotations and `#line` directives (they pin retail
  assertion line numbers).
- Layout-fitted spellings (globals' .bss order, locals' /Od slots) use
  `#define clean storage` aliases; generated branches resolve them.
- Repository text is self-contained: no references to other projects.

## Layout

| Path | Contents |
| --- | --- |
| `src/BASE`, `src/SOURCE`, `include/` | reconstructed source and headers |
| `vendor/` | third-party code (Audiere, LZHUF) |
| `config/`, `config/retail/` | build contracts; retail facts |
| `scripts/homm1/` | tooling; keep `homm1.core.usage.logged` on entry points (`homm1 audit usage`) |
| `tools/` | Rust tools |
| `docs/`, `docs/patterns/` | reference docs; measured compiler mechanisms |
| `build/` (ignored) | retail images, toolchains, Wine prefixes, generated files |

## Branches

`decomp-buka-2003` (this branch) generates `source-buka-2003` and
`classic-buka-2003` with `homm1 clean`; never edit generated branches. The
cross-platform `port` and the `source-te` edition build on `source-buka-2003`.

## Skills

`.agents/skills/` (linked from `.claude/skills`): `matcher` for
reconstruction, `wall-identifier` for diagnosing a non-matching function,
`holista` for helper recovery, `permute` for compiler-state experiments.

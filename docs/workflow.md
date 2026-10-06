# Repository workflow

The four project skills live under `.agents/skills`: `matcher`,
`wall-identifier`, `holista`, and `permute`. They follow one evidence loop
with HoMM1's VC4 profiles, VA annotations and strict score.
Their reference catalog and `docs/patterns` keep compiler observations that
were not measured on HoMM1 as hypotheses to test, not established behavior.
`CLAUDE.md` and `.claude/skills` link to the canonical instructions and skills;
`.claude/agents/matcher.md` supplies the bounded worker profile.

```sh
nix develop .#build
homm1 workflow setup
homm1 match BASE/MOUSEMGR
homm1 verify readme
homm1 build
```

`workflow setup` installs repository-local Git settings for `.githooks` and
its unit-manifest merge driver. It refuses to replace an unrelated hooks path.
The pre-commit hook formats fully staged C/C++ files under `src` and `include`.
It refuses partially staged files before changing anything, so unstaged edits
cannot enter a commit accidentally. Vendor files are excluded. The configuration does not insert braces or
trailing commas.
Formatting still requires the normal comparison validation before committing.

The unit merge driver preserves independent additions/deletions and refuses
conflicting changes to one unit. It does not decide compiler profiles or
reconstruction ownership. Git history and review still decide those facts.

Root `.clangd` reads the generated compilation database; no global editor
settings are changed.

README status is a build product. `homm1 verify readme` updates only the marked
block, does not bank scores, and refuses comparison-mode ledger mismatches.
`verify check` also refreshes it when run directly. Counts outside that block
must not duplicate generated status. Generated data lives in ignored `build/`.

`homm1 build verify` also links the candidate (`homm1 link`) and runs
`link-diff`: the bytes in which the candidate differs from the retail image,
per region (headers, each section, the trailing overlay, the file size), may
not exceed the ceiling in `config/link_diff.tsv`. Lower counts pass and are
blessed with `homm1 verify link-diff --update`. The editor's candidate
(`homm1 --image editor link`) is checked the same way against
`config/retail/editor/link_diff.tsv`.

## Game-behaviour gate

`homm1 verify behaviour` (also run by `homm1 build verify`, in the normal
tier) tests what the reconstructed game does rather than how it matches. The
byte gates already prove this branch reproduces retail; this gate catches a
behaviour change that a cleanup, a review mishap or a deliberate code change
lets through. Its sources live in `scripts/homm1/verify/behaviour/`:

- `game_contracts.cpp` is compiled with the SOURCE/KB profile and linked by
  the pinned VC6 linker against a static library of the game objects that
  `homm1 link` uses, so only what the contracts reach is pulled in. The
  vendor DLLs are delay-loaded and never called. The program runs under Wine
  headless: there is no display, the prefix's crash dialog is off, and an
  unhandled exception exits with code 3. It prints the creature, town,
  building, hero-class, spell and artifact tables (through `GetMonsterCost`,
  `GetBuildingCost` and `GetBuildingBaseResourceValue` where the game reads
  them that way). It also prints terrain and movement costs, experience
  levels, luck, morale and the seeded `SRandom` and `Random` rolls, damage
  through `army::DamageEnemy` on combat stacks with the screen closed,
  `CanBuild`, `FightValueOfStack` and `ProbableOutcomeOfBattle` on fixed
  armies, and `FindNearestObject` on a map fixture. Finally it covers a
  synthetic save that must reload and resave byte for byte, and a synthetic
  map written in the order `game::LoadMap` reads it.
- With a game copy imported by `homm1 play`, the same program also reloads
  and resaves every save present (only the record's name field may change),
  and loads `ORIGDATA.BIN` and each shipped map. Without a copy that case is
  skipped, and it is the only skip the gate accepts.
- `test_catalog_text.py` renders the combat sentences that `DoAttack`,
  `KeepAttack` and the luck and morale checks compose from catalog
  fragments, in every language. Catalog completeness, argument signatures
  and fixed-width fits belong to `homm1 verify localization`.

`game_contracts.expected` and `installed_game.expected` are behaviour
snapshots taken once from the retail-exact build, not values worked out by
hand. A failure names the first line that differs and leaves the run's output
in `build/behaviour/`. Change a snapshot only for an intended behaviour change
(copy that output over it) and say why in the commit. The gate takes about
fifteen seconds.

The contracts program is verification tooling, not game source. It stays out
of the generated `source-buka-2003` and `classic-buka-2003` trees, which carry
only the units in `config/units.toml`, their headers, resources and catalog.

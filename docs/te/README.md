# Tournament Edition on source-te

`source-te` is the Tournament Edition (TE 1.05 f3) of the Buka 2003
`HEROES.EXE`, expressed as ordinary source edits on top of the generated
`source-buka-2003` tree. `catalogue.md` describes every change of the
edition and the decisions this branch took; `changes.tsv` records, per row,
how it is implemented (`status`, `status_note`).

## Layout of the edition's changes

- Source edits live in the function bodies and headers they change; new
  helpers sit next to the code that uses them. The code reads as the game's
  own: comments describe behaviour, and nothing is marked as the edition's.
- Text: every new string has an id in the game's scheme
  (`combat.forecast.kills`, `table.gHeroNamesAccusative.3`,
  `modem.port_error.read`, ...) in `locales/messages.pot`, `locales/en.po`
  and `locales/ru.po`. Revised wording of existing strings changes their
  entries in place.
- Locale data: the Russian keyboard mapping (ё, «», №) and the per-language
  registry key (`registry_key`, read as `localization::Tr("locale.registry_key")`)
  are in `locales/<lang>.json`; the extra glyphs (« » — №) are listed in
  the Russian glyph set in `catalog.py`. These descriptor fields are the
  branch's tooling change.
- Options are ordinary `configStruct` preferences, read and written by
  `ReadPrefs`/`WritePrefs` with their defaults in `SetGameDefaults`;
  `ReadPrefs` starts from those defaults, so values missing from older
  preferences keep them.
- The editor (`--target editor`) shares kbwin, REQUEST, Audio and the BASE
  library with the game, and with them the new behaviour and settings: the
  registry key, windowed default, message pump, music path and file-name
  punctuation (catalogue section 8). Assertions stay as in the original
  source; the edition's disabled `ProcessAssert` (X03) is a build choice,
  not a source change.

## Building and playing

```sh
nix develop -c python3 build.py --target all --locale ru --icon-from /path/to/HEROES.EXE
nix develop -c python3 build.py --target all --locale en --icon-from /path/to/HEROES.EXE
nix run .#play -- --game /path/to/game-or-cd.iso --state /scratch/te-state --window
```

`--state` keeps the edition's saves and Wine prefix apart from a retail
setup. On a headless X server (Xvfb), `xdotool key F5` arrives as Alt+F5;
send function keys as raw key codes (XTest) instead.

## Resynchronising with a regenerated source-buka-2003

`source-buka-2003` is regenerated from `decomp-buka-2003` (`homm1 clean`),
often with whole-program renames, as a new root commit unrelated to the last.
Merge it with the export merged last as the merge base, so the merge brings
in exactly what the regeneration changed:

```sh
old=$(git log -1 --format=%H --grep='^Generated-By: homm1 clean$' source-te)
git merge-recursive "$old" -- source-te origin/source-buka-2003
```

`git merge-recursive` leaves the result in the working tree; commit it with
the new export as the second parent.

1. Resolve the conflicts token by token: the export's renames and constants,
   the edition's change. Keep this branch's `README.md` and `AGENTS.md`.
2. Rename what the edition's own code and `docs/te` still call by an old
   name: list the identifiers the old export has and the new one lacks, and
   look for them in the merged tree. The decomp's `config/reviews/naming-*.tsv`
   lists the parameter and local renames scoped to their function; the build
   finds the rest.
3. `python3 catalog.py check` and `update --check` must pass.
4. Build both locales.
5. Smoke-test with `nix run .#play`: main menu, a new game, F5 and F9; and
   the editor (`--editor`) loading a shipped map with the shared settings.

The first exports were carried forward by replaying the TE commits one by
one onto each new export; since `a5004479` (from `327da239`) the branch
merges them.

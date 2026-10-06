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
often with whole-program renames. Replay the TE commits one by one:

1. Branch from the new export: `git checkout -b te-replay source-buka-2003`.
   When the export changed little, cherry-pick each commit with diff3
   conflicts and resolve them token by token (the export's renames and
   constants on the base side, the commit's change on the other), then go
   to step 5.
2. Build an old-to-new identifier map: diff the token streams of every
   source file between the old and the new export; a token that no longer
   exists anywhere in the new tree and is consistently replaced by one name
   is renamed (a per-file map adds locals that vanished from that file).
   The decomp's `config/reviews/naming-*.tsv` adds parameter and local
   renames scoped to their function.
3. For each TE commit, rewrite the identifiers of its source diff (code
   only, never strings or comments) and apply it with `git apply -3`,
   falling back to `--reject`. Apply rejected hunks with whitespace- and
   line-break-insensitive matching that must cover whole lines (the export
   reflows code after renames); resolve what is left by hand.
4. Reformat only the changed lines with the decomp's `.clang-format`
   (`git clang-format --style=file:... HEAD`).
5. Move the commit's catalog changes into the new `.po` files: new and
   changed entries for `en` and `ru`, removed entries dropped; then run
   `python3 catalog.py update`. `catalog.py check` and `update --check`
   must pass.
6. Build both locales after every commit; compare each replayed commit with
   the original by the multiset of tokens it adds and removes per file.
   Every difference must be explained by the new base (renamed names,
   removed casts) or by a deliberate adaptation.
7. Smoke-test with `nix run .#play`: main menu, a new game, F5 and F9; and
   the editor (`--editor`) loading a shipped map with the shared settings.

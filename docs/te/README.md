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

## The edition on the native port (port-te)

`port-te` is the native port (`port`: SDL3, Linux, Windows, the browser,
the editor, multiplayer over TCP, the help viewer) with the edition on top:
the source-te commits replayed one by one, each followed in its own commit
message by a "Native port:" paragraph naming what the port needed, then the
port-te commits of its own and merges of `port`. Build, run and test it as
`port` (`docs/port/README.md`); the game and the editor share the edition's
behaviour and settings, as on source-te.

### Installing it with the flake

port-te's flake installs the edition the way the port's installs the game
(README, "Install with a NixOS flake"): `heroes-te` runs the Tournament
Edition and `heroes-te-editor` its scenario editor. Its saved games, maps and
high scores live in `~/.local/share/homm1-te/game` and its settings in
`~/.config/homm1/heroes-te-LANG.cfg`, apart from a plain installation's
`homm1` folder and `heroes.cfg`.

```nix
inputs.homm1-te.url = "github:sushi-shi/homm1-decomp/port-te";
inputs.homm1-game = {
  url = "path:/path/to/folder-with-the-iso";
  flake = false;
};

# in a NixOS configuration's modules (homeManagerModules.default alike):
homm1-te.nixosModules.default
{
  programs.homm1-te = {
    enable = true;
    game = "${homm1-game}/heroes.iso";   # your Buka 2003 image
    locale = "ru";                       # or "en"
  };
}
```

The port branch's flake declares `programs.homm1` and installs `heroes` and
`heroes-editor`; port-te's options, launchers, menu entries and icons carry
`-te`, so one system can install both editions.
Without installing: `HOMM1_GAME=/path/to/heroes.iso nix run
github:sushi-shi/homm1-decomp/port-te`. The package is
`packages.x86_64-linux.default`, overridable like the port's (`game`,
`locale`, `editor`; `stateName` is `homm1-te`, `edition` `te`, which gives
the launchers their `-te` names).

### Platform rows on the port's layer

The port replaces the Windows units (`kbwin`, `wingraph`, `Audio`,
`netwin`, `comwin`) with `src/PORT` on the SDL3 layer (`src/PLATFORM`). The
Windows units keep the edition's edits for the Visual C++ build; the
native counterparts carry the same effect:

| Row | Edition | Native port |
| --- | --- | --- |
| X02 | No CD check in `EarlySetup`. | The same shared `EarlySetup`; the port's `SetupCDDrive` is no longer called (the game folder is found before start-up, a missing CD folder falls back per track). |
| X03 | The edition's build left `ProcessAssert` empty (a build choice). | Assertions stay as the source has them, natively too. |
| X04, PL-CPU-1 | `Sleep(1)` per message-pump pass, 1 ms timer period, blocking `GetMessage` every 127 ms. | `Process1WindowsMessage` sleeps 1 ms per pass (not in the browser, which already yields in the event poll). SDL3 sets Windows' timer to 1 ms itself (`SDL_HINT_TIMER_RESOLUTION`). The port's pump never blocks in `GetMessage`, so the 127 ms interval has no counterpart; frames are paced by the display (`kPresentInterval`). |
| X05, PL-FS-1 | Windowed by default, menu bar forced on, full-screen preference read; preferences under `…\HeroesWorld TE\EN` or `\RU`. | Same defaults (`KBCOMMON.cpp`); the settings file is `heroes-te-en.cfg` or `heroes-te-ru.cfg` (the locale descriptor's `settings_file`, read as `localization::Tr("locale.settings_file")`, beside `registry_key`) in the port's settings folder, apart from the Buka port's `heroes.cfg`. Both programs keep their options there, by their registry value names. |
| X06, PL-OFF-2 | Videos only with `PlayVideos` (off). | The same shared `PlaySmacker`. |
| D04, R01 | Loading banner and About box name the edition. | Same catalog texts; the port's About message box shows them, and the game's window takes the catalog title (`window.gTitle`, `platform::SetWindowTitle`). |
| TE-OPT-6, D03 | Music from the game folder: `Tracks\NN-AudioTrack NN.ogg`, or `Audio\Track NN.flac` with `LosslessAudio`. | `PORT/BASE/Audio.cpp` looks there first (stb_vorbis decodes the Ogg tracks, dr_flac the FLAC ones, `vendor/dr_flac`), then in the port's CD folder, then plays `SOUND`. The browser page puts a chosen `Track NN.flac` into `Audio`. |
| X01, RT-* | Plugin loader, wrapper, proxies. | Out of scope, as on source-te. |

### Save format and protocol

- The save header's reserved block goes through the port's record codecs:
  `WriteSaveHeaderReserved`/`ReadSaveHeaderReserved` (`saveRecords.h`)
  write the 44-byte block that starts with `SaveFormatTag` ("H1TE",
  version 2), with static assertions on its layout. The fled-state byte
  (`hero::m_fledState`) is the hero record's former `m_unused38`; reserved
  tavern heroes are availability bytes `0x40`; neither changes a record's
  size. Version 2 stores the towns' "built today" flags in five bytes
  (BUG-TWN-3); `LoadGame` reads four from older saves.
- The protocol (`REMOTE_PROTOCOL_VERSION` 1) lives in the shared codecs
  (`REMOTEREC.cpp`): the packet checksum seed, the serial identification tag
  (`TE`) and the NetBIOS group name (`Empire TE1 `). The native TCP
  transport's session frame (`WriteNetbiosSession`, `remoteRecords.h`)
  carries the protocol version (frame version 2): a native program refuses
  a peer of another version, including a native program of the original
  game, whose frames lack it, and stops calling it.
- Tests: `records_test` (the block's codec, the tag's bytes, every saved
  game with the tag, the flag bytes by format), `remote_records_test` (golden packets under the
  edition's seed; the original game's packets, identification, host
  announcement and session frames refused), `net_session_test` (the
  handshake over localhost sockets, both directions), `save_roundtrip` (an
  original save saved by the edition carries the tag, then saves again
  unchanged).

### The port's other adaptations

- `_access` (quick load) is `FileExists` (`PLATFORM/File.h`).
- The final campaign battle's ground is chosen with an `if`; GCC types a
  conditional of a literal and a `char*` as `const char*`.
- Where the port fixes a defect the edition also fixes (TE-FIX-1, TE-FIX-2,
  X17, X21, X28, TE-FIX-8), port-te keeps the port's form; the edition's
  gameplay applies on top (the per-cell skeleton artifact with the port's
  gold fallback, the "(Visited)" text with the port's bounded object
  names, the edition's weekly tavern draw).

### Retail gameplay bugs the port keeps and port-te fixes

`docs/port/divergences.md` lists retail gameplay bugs the port keeps; plain
`port` stays faithful to the original game, and port-te fixes them, except
the Thieves' Guild grouping, kept as the original has it (BUG-TWN-1,
reverted), and the campaign's crest order (BUG-CAM-1 guards only the read
past the table, as the port already did).
The edition itself fixes ghost retaliation (X10), bad luck (X11),
auto-resolved losses (X12), duplicate stacks (X13), the wandering monster
count (X14), experience of grown stacks (X15), map-placed heroes' movement
(X27), the tavern (X30), obelisks (X32), the ultimate artifact hint (X33),
the shipwreck (X34), the computer's army value (X36), the Elves' second
shot (TE-FIX-3), the computer's auto-resolve (TE-FIX-4), the recruit
maximum (TE-FIX-7), the town footprint (TE-MAP-1) and the computer's Bless
and Curse evaluation (TE-UNR-1). The others are fixed on source-te as the
`BUG-*` rows of `changes.tsv` (catalogue section 9) and replayed here like
the edition's commits: the computer player (BUG-AI-*), combat (BUG-CMB-*),
the adventure map (BUG-ADV-*), the campaign's crest table (BUG-CAM-1), towns
(BUG-TWN-*), the random map generator (BUG-GEN-*) and the editor
(BUG-EDT-*). The "weekly monster growth" row is not a defect: a site's stock
grows only while below 100, by at most 10. `game_regressions_test` and
`editor_regressions_test` drive the reproductions the harness can reach
(not the recruit window, the generator, ground painting or the scroll
knob, which need their screens).

The `BUG-*` rows of the 2026-10-07 review (the computer's reads, the
adventure screens, files and text, network and modem, the engine and the
editor; catalogue section 9) are replayed the same way. Where the port
already corrects a defect in its shared units (most of the out-of-bounds
reads, the text layout, the high score and map list reads, the save and
network decoders), source-te took the port's form and port-te keeps it.
The tests add the first player's ultimate artifact hint, the "built
today" flags of town 35, the obelisk value without obelisks and a
full-width bitmap copy (`game_regressions_test`), and a map that cannot
be opened and the knobs' drag range (`editor_regressions_test`).

Where a port fix and one of these meet, the edition's form keeps the port's
guard: `BuildPath` keeps its bounds check and emptied route, the skeleton
pays through `GiveRandomArtifact` in place of the port's folded test (which
still paid nothing to a hero with 14 artifacts), the crest table's bound is
that of the corrected read, and the editor's full-table refusal follows the
release of unused records.

### Keeping port-te in step

When `port` moves on the same source (a bug pass, new platform work), merge
it: `git merge port`. Where both sides fix the same defect, keep one
implementation, the port's when it is equivalent, and keep the edition's
gameplay on top of it; record the case in this section. Then build and test
as below.

When `port` and `source-te` merge a regenerated `source-buka-2003`, merge
the new `port` the same way. Its conflicts are where the edition's changes
meet the regeneration's renames: resolve them as the source-te merge did
(the same hunks, in the same files), keeping the port's guards where a port
fix and an edition change meet. Then rename what the edition's own code, the
port-te tests and `docs/te` still call by an old name (step 2 of the resync
above). Until `a5004479` port-te was re-derived as a line on each new port,
its replayed TE commits cherry-picked again.

Checks: the native game and editor with GCC and Clang, `ctest` (with
`HOMM1_DATA`) in a normal and a `-DHOMM1_SANITIZERS=ON` build,
`nix build .#native .#sanitized .#windows .#wasm`, `python3 build.py
--target all`, and a headless smoke run: the main menu with the edition's
title, a new game to the adventure map, F5 (`GAMES/QUICKSAVE.GM1` begins its
header block with `H1TE` and version 2), F9 back to the saved position.

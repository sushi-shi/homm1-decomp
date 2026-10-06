# Tournament Edition on source-te

`source-te` is the Tournament Edition (TE 1.05 f3) of the Buka 2003
`HEROES.EXE`, expressed as ordinary source edits on top of the generated
`source-buka-2003` tree. `catalogue.md` describes every change of the
edition and the decisions this branch took; `changes.tsv` records, per row,
how it is implemented (`status`, `status_note`).

## Layout of the edition's changes

- Source edits live in the function bodies and headers they change; new
  helpers sit next to the code that uses them. Comments describe behaviour.
- Text: every string the edition adds has a `te.*` id in
  `locales/messages.pot`, `locales/en.po` and `locales/ru.po`. Revised
  wording of existing strings changes their entries in place.
- Locale data: the Russian keyboard mapping (ё, «», №) is in
  `locales/ru.json`; the extra glyphs (« » — №) are listed in the Russian
  glyph set in `catalog.py`, which also keeps the editor's catalog IDs.
- Options are registry preferences (`ReadPrefs`/`WritePrefs`) under the
  edition's own key, with the edition's defaults (`SetEditionDefaults`).

## Building and playing

```sh
nix develop -c python3 build.py --locale ru --icon-from /path/to/HEROES.EXE
nix develop -c python3 build.py --locale en --icon-from /path/to/HEROES.EXE
nix run .#play -- --game /path/to/game-or-cd.iso --state /scratch/te-state --window
```

`--state` keeps the edition's saves and Wine prefix apart from a retail
setup. On a headless X server (Xvfb), `xdotool key F5` arrives as Alt+F5;
send function keys as raw key codes (XTest) instead.

## Resynchronising with a regenerated source-buka-2003

`source-buka-2003` is regenerated from `decomp-buka-2003` (`homm1 clean`),
often with whole-program renames. Replay the TE commits one by one:

1. Branch from the new export: `git checkout -b te-replay source-buka-2003`.
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
   changed entries for `en` and `ru`, removed entries dropped; then
   regenerate the template, keeping the editor's IDs whose sources are not
   in this tree.
6. Build both locales after every commit; compare each replayed commit with
   the original by the multiset of tokens it adds and removes per file.
   Every difference must be explained by the new base (renamed names,
   removed casts) or by a deliberate adaptation.
7. Smoke-test with `nix run .#play`: main menu, a new game, F5 and F9.

## The edition on the native port (port-te)

`port-te` is the native port (`port`: SDL3, Linux, Windows, the browser,
the editor, multiplayer over TCP, the help viewer) with the edition on top:
the source-te commits replayed one by one, each followed in its own commit
message by a "Native port:" paragraph naming what the port needed, then the
port-te commits of its own and merges of `port`. Build, run and test it as
`port` (`docs/port/README.md`); the game is the edition, the editor stays
the Buka editor.

### Platform rows on the port's layer

The port replaces the Windows units (`kbwin`, `wingraph`, `Audio`,
`netwin`, `comwin`) with `src/PORT` on the SDL3 layer (`src/PLATFORM`). The
Windows units keep the edition's edits for the Visual C++ build; the
native counterparts carry the same effect:

| Row | Edition | Native port |
| --- | --- | --- |
| X02 | No CD check in `EarlySetup`. | The same shared `EarlySetup`; the port's `SetupCDDrive` is no longer called (the game folder is found before start-up, a missing CD folder falls back per track). |
| X03 | Assertions compiled out (`H1_ASSERT`, kept by the editor). | The same `kbwin.h`; every native build of the game compiles them out. |
| X04, PL-CPU-1 | `Sleep(1)` per message-pump pass, 1 ms timer period, blocking `GetMessage` every 127 ms. | `Process1WindowsMessage` sleeps 1 ms per pass (not in the browser, which already yields in the event poll, and not in the editor). SDL3 sets Windows' timer to 1 ms itself (`SDL_HINT_TIMER_RESOLUTION`). The port's pump never blocks in `GetMessage`, so the 127 ms interval has no counterpart; frames are paced by the display (`kPresentInterval`). |
| X05, PL-FS-1 | Windowed by default, menu bar forced on, full-screen preference read; preferences under `…\HeroesWorld TE\EN` or `\RU`. | Same defaults (`KBCOMMON.cpp`); the settings file is `heroes-te-en.cfg` or `heroes-te-ru.cfg` (catalog entry `te.prefs.settings_file`) in the port's settings folder, apart from the original's `heroes.cfg`, which the editor keeps. The edition's options are stored there by their registry value names. |
| X06, PL-OFF-2 | Videos only with `PlayVideos` (off). | The same shared `PlaySmacker`. |
| D04, R01 | Loading banner and About box name the edition. | Same catalog texts; the port's About message box shows them, and the game's window takes the catalog title (`window.gTitle`, `platform::SetWindowTitle`). |
| TE-OPT-6, D03 | Music from the game folder: `Tracks\NN-AudioTrack NN.ogg`, or `Audio\Track NN.flac` with `LosslessAudio`. | `PORT/BASE/Audio.cpp` looks there first (FFmpeg decodes both; the minimal FFmpeg of the Windows and browser builds gains FLAC), then in the port's CD folder, then plays `SOUND`. The browser page puts a chosen `Track NN.flac` into `Audio`. The editor does not look in the game folder. |
| X01, RT-* | Plugin loader, wrapper, proxies. | Out of scope, as on source-te. |

### Save format and protocol

- The save header's reserved block goes through the port's record codecs:
  `WriteSaveHeaderReserved`/`ReadSaveHeaderReserved` (`saveRecords.h`)
  write the 44-byte block that starts with `SaveFormatTag` ("H1TE",
  version 1), with static assertions on its layout. The fled-state byte
  (`hero::m_fledState`) is the hero record's former `m_unused38`; reserved
  tavern heroes are availability bytes `0x40`; neither changes a record's
  size.
- The protocol (`REMOTE_PROTOCOL_VERSION` 1) lives in the shared codecs
  (`REMOTEREC.cpp`): the packet checksum seed, the serial identification tag
  (`TE`) and the NetBIOS group name (`Empire TE1 `). The native TCP
  transport's session frame (`WriteNetbiosSession`, `remoteRecords.h`)
  carries the protocol version (frame version 2): a native program refuses
  a peer of another version, including a native program of the original
  game, whose frames lack it, and stops calling it.
- Tests: `records_test` (the block's codec, the tag's bytes, every saved
  game with the tag), `remote_records_test` (golden packets under the
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

`docs/port/divergences.md` lists retail gameplay bugs the port keeps. On
port-te the edition fixes: ghost retaliation (X10), bad luck (X11),
auto-resolved losses (X12), duplicate stacks (X13), the wandering monster
count (X14), experience of grown stacks (X15), map-placed heroes' movement
(X27), the tavern (X30), obelisks (X32), the ultimate artifact hint (X33),
the shipwreck (X34), the computer's army value (X36), the Elves' second
shot (TE-FIX-3), the computer's auto-resolve (TE-FIX-4), the recruit
maximum (TE-FIX-7), the town footprint (TE-MAP-1) and the computer's Bless
and Curse evaluation (TE-UNR-1). The others remain on port-te too.

### Keeping port-te in step

When `port` moves on the same source (a bug pass, new platform work), merge
it: `git merge port`. Where both sides fix the same defect, keep one
implementation, the port's when it is equivalent, and keep the edition's
gameplay on top of it; record the case in this section. Then build and test
as below.

When `port` and `source-te` move onto a regenerated `source-buka-2003`,
re-derive port-te as a line again:

1. `git checkout -b port-te-new port` (the new port).
2. List port-te's own commits: `git rev-list --reverse --first-parent
   --no-merges OLD_PORT..port-te`, where `OLD_PORT` is the port commit
   port-te last branched from or merged (the replayed TE commits and the
   port-te commits; the port's commits come with the new port).
3. Cherry-pick them in order (`git cherry-pick -x`). A replayed TE commit
   is the source-te commit of the same subject plus its "Native port:"
   adaptation. Where it conflicts with the new base, take the new
   source-te commit (`git log NEW_SOURCE..source-te`, same subject), apply
   it (`git cherry-pick`), and redo the adaptation its old message names.
   `git range-diff OLD_SOURCE..OLD_SOURCE_TE NEW_SOURCE..source-te` shows
   what changed in the edition itself; every difference between the old
   and the new port-te commit must be one of those or come from the new
   port.
4. Rename catalog ids and identifiers as the source-te resync did (its
   steps 2 and 5 above); keep `te.prefs.settings_file` beside
   `te.prefs.registry_key`.
5. After every commit: `nix develop .#port -c ninja -C build/port`. At the
   end, the checks below; then move `port-te` to the new line.

Checks: the native game and editor with GCC and Clang, `ctest` (with
`HOMM1_DATA`) in a normal and a `-DHOMM1_SANITIZERS=ON` build,
`nix build .#native .#sanitized .#windows .#wasm`, `python3 build.py
--target all`, and a headless smoke run: the main menu with the edition's
title, a new game to the adventure map, F5 (`GAMES/QUICKSAVE.GM1` begins its
header block with `H1TE` and version 1), F9 back to the saved position.

# Native port

This branch builds Heroes of Might and Magic (the Buka 2003 edition) and its
scenario editor as native programs for current systems, from the same
reconstructed sources as the Windows build. They run on 64-bit Linux, on
64-bit Windows (cross-compiled with MinGW-w64) and in a web browser
(WebAssembly); the platform layer is SDL3 with FFmpeg, so a macOS build needs
only its toolchain and packages. The Visual C++ 6 build of `HEROES.EXE` and
`EDITOR.EXE` (`build.py --target all`) keeps working on this branch.

- [Build and run](#build-and-run): [Linux](#build-and-run),
  [Windows](#windows), [in a browser](#in-a-browser), [macOS](#macos),
  [the help book](#help)
- [Architecture](#architecture)
- [Status](#status)
- [Keeping up with the source branch](#keeping-up-with-the-source-branch)
- [Plans: network play](#plans)
- [Porting lessons](lessons.md): the defect classes a port of this code runs
  into and the guard for each
- [Differences from the original](divergences.md)

## Build and run

With Nix (flakes enabled), from the repository root:

```sh
nix develop .#port
cmake -S . -B build/port -G Ninja          # -DHOMM1_LOCALE=en for English
ninja -C build/port
build/port/heroes --data ~/.local/share/homm1-buka/game          # the game
build/port/heroes-editor --data ~/.local/share/homm1-buka/game   # the editor
```

`nix run .#native` and `nix run .#native-editor` build and run them from the
flake (pass `-- --data DIR`).

Without Nix: CMake 3.20, Ninja or Make, a C++20 compiler (GCC 12+ or Clang
15+), Python 3, pkg-config, SDL 3.2+ and the FFmpeg libraries `libavformat`,
`libavcodec`, `libavutil` and `libswresample`.

The programs need the game data of the Buka edition: a folder with `DATA`,
`MAPS`, `GAMES`, `SOUND` and `ANIM` (the CD's game folder, or what
`nix run .#play` installs to `~/.local/share/homm1-buka/game`). File names may
be in any case. The folder is, in order: `--data DIR`, `$HOMM1_DATA`, the
executable's folder, the current folder, `$XDG_DATA_HOME/homm1-buka/game`,
`~/.local/share/homm1-buka/game`. Saved games and the editor's maps are
written to its `GAMES` and `MAPS` folders, so it must be writable.

The CD's music (`Tracks/NN-AudioTrack NN.ogg`) is found in `$HOMM1_CD`, in a
`cd` folder beside the game folder (where `nix run .#play` puts it) or in the
game folder; without it the game plays the installed music in `SOUND`.

Options: `--window`, `--fullscreen` and the original's switches (`/I0` skips
the intro movies, `/C1` uses colour pointers instead of the default
monochrome ones). F4 switches full screen. Settings are kept in
`$XDG_CONFIG_HOME/homm1/heroes.cfg` (both programs, as both shared the
original's registry key).

In a window both programs show their menu bar above the picture, as the
original did; at full screen it is hidden, as it was. The game's options,
the editor's window sizes, full screen, About and Help are there.

`$HOMM1_CONFIG` names another settings folder on every system.

### Windows

```sh
nix build .#windows        # result/bin: heroes.exe, heroes-editor.exe and their DLLs
```

`nix/windows.nix` cross-compiles both programs for x86-64 Windows with
nixpkgs' MinGW-w64 GCC, SDL3 built for the target and the minimal FFmpeg of
`nix/ffmpeg-minimal.nix` (Smacker and Ogg Vorbis only), and ships the DLLs
they import (SDL3, the four FFmpeg libraries, `libmcfgthread-2.dll`); the
build fails if a program imports any other DLL that is not part of Windows.
Copy `result/bin` to Windows and run `heroes.exe --data C:\Games\Heroes` or
`heroes-editor.exe`. The game folder is searched as on Linux, with
`%LOCALAPPDATA%\homm1-buka\game` in place of the XDG folders; settings and
the converted help are in `%APPDATA%\homm1\`. The programs are GUI programs:
their log goes to a redirected stderr, else to the console that started
them, else to `%APPDATA%\homm1\homm1.log`.

Under Wine (the flake's default shell has it):

```sh
xvfb-run wine result/bin/heroes.exe --data 'Z:\home\me\.local\share\homm1-buka\game' /I0
```

`HOMM1_INPUT_REPLAY` takes a Windows path there, and `shot` paths may be
`Z:\...`. The unit tests cross-build and pass under Wine with
`-DCMAKE_CROSSCOMPILING_EMULATOR=wine` (all but `save_roundtrip`, whose
driver is a Linux script); the flake does not run them.

### In a browser

```sh
nix build .#wasm           # result/share/homm1-web: the page and both programs
nix run .#web              # serves it on http://127.0.0.1:8000/
```

`nix/wasm.nix` builds SDL3, the minimal FFmpeg and both programs with
Emscripten. The page (`src/PLATFORM/Web/`) asks for the game folder once
(**Choose a folder**: the folder with `DATA`, `MAPS`, ..., or one above it
that also holds the CD's `Tracks` and `HEROES.HLP`; **Add single files**
takes music tracks or the help file on their own), copies it into the
browser's storage for the site (IndexedDB) and then starts the game, or the
editor with `?program=editor`, on it. Nothing is uploaded; the build output
holds no game data. **Remove the stored files** deletes everything,
saved games included, and the page lists the saved games and maps for
download. Any static web server works as long as it serves `.wasm` as
`application/wasm`.

- **The main loop.** The game never returns to a main loop: menus, dialogs,
  combat and the adventure map each run their own loop that polls for
  input, as the original's did around `GetMessage`. The browser build uses
  Emscripten's ASYNCIFY, which lets those loops give control back to the
  browser where they already wait and resumes them afterwards: SDL does so
  when it presents a frame and in `SDL_Delay`, and the event poll does so at
  least once per 16 ms (`YieldToBrowser` in `src/PLATFORM/SDL3/Host.cpp`).
  The game code is unchanged. JSPI would do the same with smaller code,
  but is not yet available in every browser (Firefox and Safari); an
  explicit main-loop rewrite would mean restructuring every modal loop of the
  game. ASYNCIFY costs code size (the game's `.wasm` is 3.5 MB) and some
  speed, which this game does not notice.
- **Files.** Everything lives under `/homm1` (`src/PLATFORM/Web/pre.js`):
  `game` (the game folder), `cd` (`Tracks`) and `config` (settings and the
  converted help), an IDBFS mount loaded before the program starts and
  written back to IndexedDB shortly after each file the program writes is
  closed (`autoPersist`), so saved games, maps and settings survive reloads.
- **Audio.** Browsers keep audio suspended until the page is used. The
  **Play** click creates the audio context that SDL then takes over, and any
  later click or key resumes a suspended context.
- **Pointer and keyboard.** SDL maps the canvas's events: pointer positions
  through the canvas's shown size (the page scales the 640x500 canvas to the
  window, pixelated), keys by their physical code, so the Set 1 scan codes
  are the same as natively. The context menu is suppressed on the canvas.
- **Movies and music.** The same FFmpeg code as natively, with FFmpeg built
  for WebAssembly with only the Smacker and Ogg demuxers and the `smacker`,
  `smackaud` and `vorbis` decoders.
- **Help** opens in a new tab (a Blob URL of the converted page); when the
  browser blocks the tab, the page shows a link instead.

Page parameters: `program=editor`, `args=/I0` (the original's switches),
`quiet=1` (message boxes to the log only).

`nix run .#web-smoke -- --data DIR [--cd DIR] [--help-file HEROES.HLP]
[--browser firefox] [--out DIR]` (`tests/port/web_smoke.py`) drives the
built page in headless Chromium or Firefox through Playwright, the way a
player would: it hands the page the folder through its file input, starts
the game, opens the help from the menu, starts a new game to the adventure
map, reloads (the files must still be there), checks that the intro movie
runs and audio is unlocked by the click, loads the shipped saved game, saves
it, reloads and checks that the save reached IndexedDB, and opens the editor,
with a screenshot of each step. Clicks are made at the game's coordinates
scaled through the canvas's size on the page. Headless Firefox has no audio
device, which the script detects and reports.

### macOS

Not built. The code has nothing Windows- or Linux-only outside the platform
layer: `File.cpp`'s POSIX half, `Host.cpp`'s XDG settings folder (which
should become `SDL_GetPrefPath` there, as on Windows) and the system browser
through `SDL_OpenURL` all apply. A build needs SDL3 and FFmpeg for macOS
(`nix/ffmpeg-minimal.nix` builds natively too) and a `.app` bundle.

### Help

The game's help is a WinHelp 4.0 book, `HELP\HEROES.HLP` with its contents
file `HEROES.CNT`, which no current system shows. The Help item of the game
and of the editor converts it, on first use and again when the file or the
converter changes, into one self-contained HTML page in the settings folder
(`help/heroes.html`), and opens it in the system's browser, or in a new tab
in the browser build. Without the file the item is greyed, as before. The
book ships in the CD's installer cabinet, not in the game folder, so
`nix run .#play`'s installation now copies it to the game folder's `Help`;
for an existing installation copy `Help_Files/Help` from the cabinet there.

The converter (`src/PLATFORM/Help.cpp`, no SDL) reads WinHelp 3.1 and 4.0:
the internal file system and its B+ trees, LZ77 topic blocks, phrase and Hall
compression, topic and paragraph records with their fonts, jumps and pop-ups
(through the `|CONTEXT` hashes of the contents file's topic names), the
keyword index, browse sequences and `|bmN` bitmaps (embedded as images). The
contents and the index come first; each topic is a section of the page.
Not supported: WinHelp 3.0 files, metafile pictures, hotspots, embedded
windows, macros (shown as text) and jumps into other help files.
`homm1-hlp2html IN.HLP [IN.CNT] OUT.html` runs the same converter (installed
by `nix build .#native`). The converted text is the game's; neither the
converter's tests nor the documentation contain any of it.

### Sanitizers and tests

```sh
cmake -S . -B build/port-asan -G Ninja -DHOMM1_SANITIZERS=ON
ninja -C build/port-asan
HOMM1_DATA=~/.local/share/homm1-buka/game ctest --test-dir build/port-asan --output-on-failure
```

`records_test` checks the file record codecs and, with `HOMM1_DATA`, parses
and re-encodes every shipped map, campaign map, saved game, high score table
and the archive directory. `file_test` covers the game path resolver,
`blit_test` the drawing routines, `lzhuf_test` the network save compressor,
`help_test` the help converter on a synthetic book and thousands of
damaged copies of it, `help_game_test` (with `$HOMM1_HELP` or
`HELP/HEROES.HLP` under `HOMM1_DATA`) on the real book,
`save_roundtrip` (with `HOMM1_DATA` and `xvfb-run`) loads the shipped
saved game in the program and saves it again, comparing the bytes, and
`editor_maps_test` (with `HOMM1_DATA`, headless) loads every shipped map with
the editor's own reader and saves it with its writer, comparing the bytes
region by region: the header text, the cells' terrain, object and overlay
bytes and the extra records must be unchanged; what the writer derives (the
format word, trigger bytes, the town, mine, artifact, obelisk and sound tables,
the cells above random towns that the editor clears on saving, and the layout
of older-format maps) is accounted for in its header comment; saving the saved
map again must reproduce it.
The `*_replay` tests replay the fuzz harnesses' regression inputs (see
[Fuzzing the file parsers](#fuzzing-the-file-parsers)).
`nix flake check` builds the native and sanitizer builds and runs their tests
(without game data), and builds the Windows programs.
`-DHOMM1_SANITIZERS_RECOVER=ON` keeps going after undefined behaviour, to
survey a whole session.

### Fuzzing the file parsers

`tests/port/fuzz` holds libFuzzer harnesses for every parser of file data
that a player can be handed: saved games (`fuzz_savegame`, through
`game::LoadGame` after the load requester lists the folder), maps and
campaign maps (`fuzz_map`, through the scenario list and `game::NewMap`),
the editor's map reader with its checks and writer (`fuzz_editor_map`),
high score tables (`fuzz_highscore`, through the high score screen), the
resource archive and every decoder that reads a resource in place, drawing
what it decodes into exactly-sized buffers (`fuzz_resources`), the WinHelp
converter on a help file and its contents file (`fuzz_help`), the record
codecs (`fuzz_records`) and the network save compressor (`fuzz_lzhuf`). The
game's own units run headless; its error exits (`FileError`, `ShutDown`)
are wrapped to throw, so a file the game refuses is an ordinary outcome.

```sh
cmake -S . -B build/fuzz -G Ninja -DCMAKE_CXX_COMPILER=clang++ -DHOMM1_FUZZERS=ON
ninja -C build/fuzz
HOMM1_HELP=path/to/HEROES.HLP python3 tests/port/fuzz/seeds.py ~/.local/share/homm1-buka/game /tmp/seeds
mkdir -p /tmp/corpus/fuzz_map
HOMM1_DATA=~/.local/share/homm1-buka/game build/fuzz/tests/port/fuzz/fuzz_map \
    -rss_limit_mb=2048 -max_total_time=1800 /tmp/corpus/fuzz_map /tmp/seeds/fuzz_map
```

`HOMM1_FUZZERS` needs Clang and turns the sanitizers on; the fuzzers are not
part of the default build. The harnesses that run the game's units need
`HOMM1_DATA` (they start the game on a scratch folder that links the data).
`fuzz_map` and `fuzz_savegame` read the new game dialog's choices from a
byte after the file, `fuzz_resources` the kind of resource and `fuzz_help`
the length of the contents file that follows the help file; the
comments at the top of each harness give the format, and `seeds.py` writes
seeds in it. `-fork=1 -ignore_crashes=1` keeps a campaign going past
findings; `HOMM1_FUZZ_LOAD_ONLY=1` stops `fuzz_map` after `game::LoadMap`.

Every harness is also built, in every configuration, as `<name>_replay`,
which ctest runs over the regression inputs in
`tests/port/fuzz/regressions/<name>` and, with `HOMM1_DATA`, over the
shipped files, which must all be accepted (each map under every standard
game set-up, the whole resource archive). A regression input is either the
input itself or a `.patch` recipe that builds it from a shipped file or from
bytes and says whether the game must accept or refuse it (the format is at
the top of `ReplayMain.cpp`), so no game data is checked in.

### Unattended runs

`HOMM1_INPUT_REPLAY=FILE` drives either program from a script, for tests and
screenshots without a person at the keyboard (under `xvfb-run`, with
`SDL_AUDIO_DRIVER=dummy` when there is no sound device). Each line is a time
in milliseconds from the first event poll (or `+N` after the previous line),
an action and its arguments; `#` starts a comment:

```text
3000 click 497 104          # New Game
+1500 click 497 104         # Standard Game
+2000 click 382 437         # OK
+9000 shot newgame.bmp      # the display image as a BMP
+100 key e                  # a key: a letter, digit, name or scan code
+100 exit
```

Actions: `move X Y`, `click X Y`, `left-down`, `left-up`, `right-down`,
`right-up` (display coordinates; the menu bar has negative `y`),
`key K`, `key-down K`, `key-up K`,
`shot PATH`, `quit` (the window's close button), `exit` (end at once). While
a replay runs the real mouse and keyboard are ignored.
`HOMM1_NO_DIALOGS=1` sends message boxes to the log only.

## Architecture

```text
src/BASE, src/SOURCE    the game: shared by both builds
src/EDITOR              the scenario editor (with BASE, SOURCE units built
                        with HOMM1_EDITOR)
include/BASE, SOURCE    its headers; *Host.h are the Windows host's alone
include/PLATFORM        File.h, Records.h: shared, plain C++98
                        Platform.h: the native platform interface
src/PLATFORM            File.cpp, Records.cpp (both builds); Text.cpp,
                        MsvcRuntime.cpp, Help.cpp (the WinHelp converter)
                        (native)
src/PLATFORM/SDL3       the SDL3 + FFmpeg backend
src/PLATFORM/Web        the browser build's page and its file system setup
src/PORT                native replacements of the Windows-bound units
tools/port              localize.py (game text), units.py (each program's
                        units from build.json), menus.py (menus from the
                        .rc scripts), sync.sh
tools/help              homm1-hlp2html, the converter on the command line
nix                     the Windows and browser builds, the minimal FFmpeg
tests/port              ctest programs; web_smoke.py (the browser build)
```

**The boundary.** The Windows original talks to its host in a few units:
`kbwin.cpp` (window, message loop, menus, registry, CD probe), `wingraph.cpp`
(WinG and DirectDraw), `Audio.cpp` (Audiere and Miles), `netwin.cpp` and
`comwin.cpp` (NetBIOS and serial), the Smacker DLL, and six assembly routines.
The rest of the game reaches the host only through the functions those units
export (`kbwin.h`, `wingraph.h`, `audio.h`, `smack.h`, `mouseManager.h`,
`netwin.h`, `comwin.h`). The Visual C++ build compiles the Windows units; the
native build compiles their counterparts in `src/PORT` instead, which
implement the same functions on `include/PLATFORM/Platform.h`. Game logic
stays in the shared units; the port units hold only translation.

| Windows unit | Native counterpart | Platform service |
| --- | --- | --- |
| `SOURCE/kbwin.cpp` | `PORT/SOURCE/kbwin.cpp`, `Main.cpp`, `Menu.cpp` | entry point, event pump, timers, settings file, menu bar, CD folder |
| `SOURCE/wingraph.cpp` | `PORT/SOURCE/wingraph.cpp` | 8-bit display with palette, cursors |
| `BASE/Audio.cpp` | `PORT/BASE/Audio.cpp` | sound samples, Ogg music, movie sound |
| SMACKW32.DLL | `PORT/SOURCE/Smacker.cpp` | Smacker decoding (FFmpeg) |
| `SOURCE/netwin.cpp`, `comwin.cpp` | `PORT/SOURCE/netwin.cpp`, `comwin.cpp` | none yet: reported unavailable |
| `BASE/*.asm`, LZHUF decoder | `PORT/BASE/*.cpp` | portable C++ of the same routines |

Both programs are built from the units `build.json` lists for their Visual
C++ build (`tools/port/units.py`), with these substitutions, so the two builds
cannot drift apart. Each program's units are an object library; the
executables add `Main.cpp`, and tests link the units with their own `main`.

**Menus.** The original loaded its menus from the executable's resources and
let Windows draw them. `tools/port/menus.py` turns the menus and About box of
`Heroes.rc` and `Editor.rc` into tables with the selected language's text;
`Menu.cpp` keeps the menus' state as the game's calls change it (check marks,
greyed items, the enable table) and draws the bar and its pop-ups with the
game's small font in the display's reserved Windows colours on the platform's
chrome layer, a strip above the game image and panels over it. An open menu
runs a modal loop, as Windows' did, so the game does not run underneath it;
the chosen command then runs through `AppMenuCommand` as `WM_COMMAND` did.
Message boxes are the host's (`SDL_ShowSimpleMessageBox`).

**The platform interface** (`include/PLATFORM/Platform.h`) is free functions
in `namespace platform`: start-up, time, text conversion, display, cursor,
input events, audio and movies. A backend is a set of files implementing them;
the shipped one is `src/PLATFORM/SDL3`. The display holds a 640x480 8-bit
image and a 256-colour palette, as the original's primary surface did, and
draws the game's pointer over it; scaling and letterboxing are the renderer's.

**Shared portable pieces.** `File.h` resolves the game's backslash, any-case
paths under the game folder and lists folders; `Records.h` and `saveRecords.h`
encode every file record field by field. Both compile under Visual C++ 6 too,
so the two builds read and write the same files the same way.

**Game text.** As in the Visual C++ build, `localization::Tr("id")` is
resolved at build time: `tools/port/localize.py` writes a copy of the sources
with each message as a literal in the language's code page into
`build/.../localized/`, and the compiler reads that copy. The game's fonts
index those code-page bytes; `platform::ToUtf8` converts text that goes to the
host (window title, message boxes).

**Compiler settings.** `-fsigned-char -fwrapv -fno-strict-aliasing` keep the
original compiler's semantics on every target. Port and platform code build
with strict warnings as errors; see [lessons.md](lessons.md).

## Status

| Area | State |
| --- | --- |
| Build | Game and editor on Linux x86-64 with GCC and Clang; sanitizer configuration; Windows x86-64 with MinGW-w64; WebAssembly with Emscripten. The Visual C++ 6 build of both is unaffected. |
| Start-up, main menu, intro movies | Works. |
| New game to the adventure map | Works (scripted under Xvfb). |
| Load a saved game, shipped or new | Works; every shipped save parses and re-encodes byte for byte. |
| Save a game | Works; files are replaced atomically. |
| Maps | All shipped maps and campaign maps parse and re-encode byte for byte. |
| Display | 640x480 8-bit with palette, palette cycling and fades, scaled window or full screen. |
| Pointer | Colour and monochrome game pointers, drawn by the port. |
| Keyboard and mouse | Set 1 scan codes as the original read them; the language's keyboard table applies. |
| Music | CD tracks or installed Ogg music, with the original's repeat and resume rules. |
| Sound effects | Works. |
| Movies | Smacker through FFmpeg, with sound. |
| Menu bars | Drawn for the game (default, adventure, combat, town) and the editor, with check marks, greyed items, submenus and a modal loop. |
| Help file | Converted from `HELP\HEROES.HLP` to HTML on first use and shown in the browser (a new tab in the browser build); greyed without the file. |
| Network, modem, direct cable | Not available; the game reports it. |
| Scenario editor | Works: opens shipped maps, edits, saves; every shipped map round-trips through its reader and writer; maps it saves load in the native game and in the Visual C++ game and editor. |
| Windows | Main menu, new game to the adventure map, intro movie, loading the shipped save and saving it again (byte for byte outside its name field), the editor, the help conversion: checked under Wine. |
| Browser | Chromium and Firefox (headless): the page stores the player's files, the game reaches the main menu, a new game the adventure map, the intro movie plays, audio starts on the click, the shipped save loads and saves back to IndexedDB, the help opens in a new tab, the editor opens. |
| macOS | Not built yet. |

## Keeping up with the source branch

`source-buka-2003` is generated from `decomp-buka-2003` and is replaced by a
new single root commit each time it is regenerated. This branch is that root
commit plus the port's commits, so carrying the port forward is a replay of
those commits onto the new root:

```sh
git fetch origin source-buka-2003
old=$(git rev-list --max-parents=0 port)         # the source commit the port sits on
git rebase --onto origin/source-buka-2003 "$old" port
```

Then rebuild both programs and run the tests:

```sh
nix develop -c python3 build.py --target all      # Visual C++ 6 HEROES.EXE, EDITOR.EXE
nix develop .#port -c cmake -S . -B build/port -G Ninja
nix develop .#port -c ninja -C build/port
HOMM1_DATA=... nix develop .#port -c ctest --test-dir build/port
```

`tools/port/sync.sh` runs the replay and both builds. Conflicts arise where
the regenerated source changed the lines a port commit edits; resolve them in
favour of the new source and re-apply the port's intent. New Windows calls in
shared units show up as native compile errors (the shared units no longer see
`windows.h`); renamed host functions show up as link errors. Port changes that
leave the Visual C++ output byte-identical (the header splits, for example)
can be made on `decomp-buka-2003` instead, which shrinks the replay.

The first regeneration replayed this way (`b0488801` to `52922394`, which
brought the editor, `build.json` targets and a naming and comment pass)
conflicted in thirteen files (twelve sources and `build.json` in the first
port commit, `flake.nix` in the second), all where a port commit edited lines the
regeneration renamed (`screenImage` to `gScreenImage`, `findHandleWork` to
`findHandle`, the high-score locals, `SmackOptions` to `gSmackOptions`) or
where a split header lost declarations the regeneration had re-spelled. The
host headers (`*Host.h`) were refreshed from the new headers' Windows part,
the editor's own copies of the Windows calls got the same host functions, and
the four shared portable units (`PLATFORM/File`, `PLATFORM/Records`,
`SOURCE/SAVEREC`, `SOURCE/KBCOMMON`) joined both `build.json` targets.

## Plans

**Network play.** The transports sit behind `netwin.h` and `comwin.h`
(`nb_init`, `nb_snd`, `nb_rcv`, `com_init`, ...), which the game's `REMOTE.cpp`
drives with its own protocol. The plan is a TCP transport that implements the
NetBIOS session calls (named sessions become host:port endpoints, datagrams
become length-prefixed frames), keeping `REMOTE.cpp` unchanged. Before that,
the network messages that copy structures whole (`combatRemoteData`,
`heroRemoteMessage`, the save transfer) need record codecs like the save
files, so that different builds can talk to each other.

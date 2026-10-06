# Native port

This branch builds Heroes of Might and Magic (the Buka 2003 edition) and its
scenario editor as native programs for current systems, from the same
reconstructed sources as the Windows build. They run on 64-bit Linux; the
platform layer is SDL3 with FFmpeg, so Windows and macOS builds need only their
own toolchain and packages. The Visual C++ 6 build of `HEROES.EXE` and
`EDITOR.EXE` (`build.py --target all`) keeps working on this branch.

- [Build and run](#build-and-run)
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
the editor's window sizes, full screen and About are there.

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
`nix flake check` builds the native and sanitizer builds and runs their tests
(without game data).
`-DHOMM1_SANITIZERS_RECOVER=ON` keeps going after undefined behaviour, to
survey a whole session.

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
                        MsvcRuntime.cpp (native)
src/PLATFORM/SDL3       the SDL3 + FFmpeg backend
src/PORT                native replacements of the Windows-bound units
tools/port              localize.py (game text), units.py (each program's
                        units from build.json), menus.py (menus from the
                        .rc scripts), sync.sh
tests/port              ctest programs
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
| Build | Game and editor on Linux x86-64 with GCC and Clang; sanitizer configuration. The Visual C++ 6 build of both is unaffected. |
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
| Help file | Not available (WinHelp); the item is greyed. |
| Network, modem, direct cable | Not available; the game reports it. |
| Scenario editor | Works: opens shipped maps, edits, saves; every shipped map round-trips through its reader and writer; maps it saves load in the native game and in the Visual C++ game and editor. |
| Windows, macOS | Not built yet; nothing in the code is Linux-only besides `File.cpp`'s POSIX half. |

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

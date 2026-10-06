# Native port

This branch builds Heroes of Might and Magic (the Buka 2003 edition) as a
native program for current systems, from the same reconstructed sources as the
Windows build. It runs on 64-bit Linux; the platform layer is SDL3 with FFmpeg,
so Windows and macOS builds need only their own toolchain and packages. The
Visual C++ 6 build of `HEROES.EXE` (`build.py`) keeps working on this branch.

- [Build and run](#build-and-run)
- [Architecture](#architecture)
- [Status](#status)
- [Keeping up with the source branch](#keeping-up-with-the-source-branch)
- [Plans: scenario editor, network play](#plans)
- [Porting lessons](lessons.md): the defect classes a port of this code runs
  into and the guard for each
- [Differences from the original](divergences.md)

## Build and run

With Nix (flakes enabled), from the repository root:

```sh
nix develop .#port
cmake -S . -B build/port -G Ninja          # -DHOMM1_LOCALE=en for English
ninja -C build/port
build/port/heroes --data ~/.local/share/homm1-buka/game
```

Without Nix: CMake 3.20, Ninja or Make, a C++20 compiler (GCC 12+ or Clang
15+), Python 3, pkg-config, SDL 3.2+ and the FFmpeg libraries `libavformat`,
`libavcodec`, `libavutil` and `libswresample`.

The program needs the game data of the Buka edition: a folder with `DATA`,
`MAPS`, `GAMES`, `SOUND` and `ANIM` (the CD's game folder, or what
`nix run .#play` installs to `~/.local/share/homm1-buka/game`). File names may
be in any case. The folder is, in order: `--data DIR`, `$HOMM1_DATA`, the
executable's folder, the current folder, `$XDG_DATA_HOME/homm1-buka/game`,
`~/.local/share/homm1-buka/game`. Saved games are written to its `GAMES`
folder, so it must be writable.

The CD's music (`Tracks/NN-AudioTrack NN.ogg`) is found in `$HOMM1_CD`, in a
`cd` folder beside the game folder (where `nix run .#play` puts it) or in the
game folder; without it the game plays the installed music in `SOUND`.

Options: `--window`, `--fullscreen` and the original's switches (`/I0` skips
the intro movies, `/C1` uses colour pointers instead of the default
monochrome ones). F4 switches full screen. Settings are kept in
`$XDG_CONFIG_HOME/homm1/heroes.cfg`.

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
and `save_roundtrip` (with `HOMM1_DATA` and `xvfb-run`) loads the shipped
saved game in the program and saves it again, comparing the bytes.
`nix flake check` builds the native and sanitizer builds and runs their tests
(without game data).
`-DHOMM1_SANITIZERS_RECOVER=ON` keeps going after undefined behaviour, to
survey a whole session.

### Unattended runs

`HOMM1_INPUT_REPLAY=FILE` drives the game from a script, for tests and
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
`right-up` (display coordinates), `key K`, `key-down K`, `key-up K`,
`shot PATH`, `quit` (the window's close button), `exit` (end at once). While
a replay runs the real mouse and keyboard are ignored.
`HOMM1_NO_DIALOGS=1` sends message boxes to the log only.

## Architecture

```text
src/BASE, src/SOURCE    the game: shared by both builds
include/BASE, SOURCE    its headers; *Host.h are the Windows host's alone
include/PLATFORM        File.h, Records.h: shared, plain C++98
                        Platform.h: the native platform interface
src/PLATFORM            File.cpp, Records.cpp (both builds); Text.cpp,
                        MsvcRuntime.cpp (native)
src/PLATFORM/SDL3       the SDL3 + FFmpeg backend
src/PORT                native replacements of the Windows-bound units
tools/port              localize.py (game text for the native build)
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
| `SOURCE/kbwin.cpp` | `PORT/SOURCE/kbwin.cpp` | entry point, event pump, timers, settings file, menus (model only), CD folder |
| `SOURCE/wingraph.cpp` | `PORT/SOURCE/wingraph.cpp` | 8-bit display with palette, cursors |
| `BASE/Audio.cpp` | `PORT/BASE/Audio.cpp` | sound samples, Ogg music, movie sound |
| SMACKW32.DLL | `PORT/SOURCE/Smacker.cpp` | Smacker decoding (FFmpeg) |
| `SOURCE/netwin.cpp`, `comwin.cpp` | `PORT/SOURCE/netwin.cpp`, `comwin.cpp` | none yet: reported unavailable |
| `BASE/*.asm`, LZHUF decoder | `PORT/BASE/*.cpp` | portable C++ of the same routines |

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
| Build | Linux x86-64 with GCC and Clang; sanitizer configuration. The Visual C++ 6 build is unaffected. |
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
| Menu bar | The game's menu model runs; the bar is not drawn (all commands also exist in the game's screens). |
| Help file | Not available (WinHelp). |
| Network, modem, direct cable | Not available; the game reports it. |
| Scenario editor | Not on this branch yet; see the plan below. |
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
nix develop -c python3 build.py                   # Visual C++ 6 HEROES.EXE
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

## Plans

**Scenario editor.** The editor (`EDITOR.EXE`) shares the game's units, built
with `HOMM1_EDITOR`, plus `src/EDITOR`. Its interface is drawn by the game's
own window and widget code (map view, radar, palettes, the file requester for
opening and saving maps); its only Windows features are the default menu
(`mnuDflt`: window size, full screen, help, about), message boxes, the
single-instance check and the shared host units. The native editor therefore
needs no native toolkit: it builds from the same `src/PORT` units, which
already carry the `HOMM1_EDITOR` variants of `kbwin.cpp` and `wingraph.cpp`,
as a second CMake executable. `EDITOR.CPP`'s own start-up copies the game's
`MessageBoxA` and `LoadMenuA` calls and gets the same `KBErrorBox` and
`KBLoadMenu` treatment. Its `.MAP` writer (`editManager::SaveMap`) writes the
header, cells, town/mine/artifact/obelisk tables, sounds, extra records and,
in the new format, the object owner table with raw `write` calls; it moves to
the same codecs (`WriteMapHeader`, `WriteMapCell`, ...) and `FileReplace`,
and `records_test` already checks the owner table that maps saved this way
carry. If the menu bar is wanted later, the port host can draw it above the
640x480 image with the game's own small font, keeping the menu model it
already maintains.

**Network play.** The transports sit behind `netwin.h` and `comwin.h`
(`nb_init`, `nb_snd`, `nb_rcv`, `com_init`, ...), which the game's `REMOTE.cpp`
drives with its own protocol. The plan is a TCP transport that implements the
NetBIOS session calls (named sessions become host:port endpoints, datagrams
become length-prefixed frames), keeping `REMOTE.cpp` unchanged. Before that,
the network messages that copy structures whole (`combatRemoteData`,
`heroRemoteMessage`, the save transfer) need record codecs like the save
files, so that different builds can talk to each other.

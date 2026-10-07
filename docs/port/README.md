# Native port

This branch builds Heroes of Might and Magic (the Buka 2003 edition) and its
scenario editor as native programs for current systems, from the same
reconstructed sources as the Windows build. They run on 64-bit Linux, on
64-bit Windows (cross-compiled with MinGW-w64) and in a web browser
(WebAssembly); the platform layer is SDL3 and the port decodes the movies and
music itself (Smacker, and Ogg Vorbis with stb_vorbis), so a macOS build needs
only its toolchain and packages. The Visual C++ 6 build of `HEROES.EXE` and
`EDITOR.EXE` (`build.py --target all`) keeps working on this branch.

- [Build and run](#build-and-run): [Linux](#build-and-run),
  [Windows](#windows), [in a browser](#in-a-browser), [macOS](#macos),
  [the help book](#help), [multiplayer](#multiplayer)
- [Installing with Nix](#installing-with-nix)
- [The Visual C++ 6 build](#the-visual-c-6-build) and the
  [text catalog](#the-text-catalog)
- [Architecture](#architecture)
- [Status](#status)
- [Keeping up with the source branch](#keeping-up-with-the-source-branch)
- [Porting lessons](lessons.md): the defect classes a port of this code runs
  into and the guard for each
- [Differences from the original](divergences.md)

## Build and run

With Nix (flakes enabled), from the repository root:

```sh
nix develop                                # or .#port: the native build alone
cmake --preset linux                       # -DHOMM1_LOCALE=en for English
cmake --build --preset linux
build/linux/heroes --data ~/.local/share/homm1-buka/game          # the game
build/linux/heroes-editor --data ~/.local/share/homm1-buka/game   # the editor
ctest --preset linux                       # with HOMM1_DATA=... for the data tests
```

The presets (`CMakePresets.json`) are `linux`, `wasm` (configured with
`emcmake`) and `windows` (MinGW-w64), building in `build/<preset>`; plain
`cmake -S . -B build/port -G Ninja` works as before.

`nix run .#native` and `nix run .#native-editor` build and run them from the
flake (pass `-- --data DIR`). To install them with your game data as
`heroes-te` and `heroes-te-editor`, with a NixOS or home-manager module, see
[Install with a NixOS flake](../../README.md#install-with-a-nixos-flake);
the launchers and the data import are `nix/game.nix`, `nix/launch.sh` and
`nix/game-data.py`.

Without Nix: CMake 3.20, Ninja or Make, a C++20 compiler (GCC 12+ or Clang
15+), Python 3 and SDL 3.2+ (without an installed SDL, CMake 3.25 downloads
SDL 3.4.8, pinned by SHA-256, and links it into the programs).

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

`$HOMM1_CONFIG` names another settings folder on every system. On
`port-te` the game is the Tournament Edition and keeps its settings in
`heroes-te-en.cfg` or `heroes-te-ru.cfg` instead (`docs/te/README.md`); the
editor keeps `heroes.cfg`.

### Windows

```sh
nix build .#windows        # result/bin: heroes.exe, heroes-editor.exe
```

`nix/windows.nix` cross-compiles both programs for x86-64 Windows with
nixpkgs' MinGW-w64 GCC and a static SDL3 built for the target, and links
SDL, the C++ runtime and the thread library into each program (`-static`):
both are single files with no DLL beside them, and the build fails if a
program imports any DLL that is not part of Windows. Copy the two programs
anywhere on Windows and run `heroes.exe --data C:\Games\Heroes` or
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
driver is a Linux script); the flake runs `file_test` and `data_root_test`
from the package's `tests` output under Wine (`windows-tests`), and
`HOMM1_DATA=DIR nix run .#windows-smoke` starts `heroes.exe` from a player's
game folder three ways, and `heroes-editor.exe` once.

On Windows itself (MinGW-w64, CMake 3.25+, Ninja, Python 3), `cmake --preset
windows` and `cmake --build --preset windows` build both programs in
`build\windows`; without an installed static SDL, CMake fetches SDL and
links it, the C++ runtime and the thread library in (`-static`), so each
program is a single `.exe` that imports only Windows' own DLLs. That path
is checked by cross-building the preset with nixpkgs' MinGW-w64 from a
fetched SDL and starting the result under Wine.

The game folder search takes the paths a player gives (`--data`,
`HOMM1_DATA`, `HOMM1_CD`, `HOMM1_CONFIG`) without surrounding blanks, one pair
of surrounding quotes and trailing separators (`platform::ConfiguredDirectory`);
when no folder holds the data, the message lists every folder tried.

### In a browser

```sh
nix build .#wasm           # result/share/homm1-web: the page and both programs
nix run .#web              # serves it on http://127.0.0.1:8000/
```

`nix/wasm.nix` builds SDL3 and both programs with Emscripten. Without Nix,
`emcmake cmake --preset wasm` and `cmake --build --preset wasm` (emsdk 5.0.6,
CMake 3.25+, Ninja, Python 3) build the same with an SDL that CMake fetches,
and copy the page beside the programs: `build/wasm` is the site. The page (`src/PLATFORM/Web/`) asks for the game folder once
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
- **Movies and music.** The same decoders as natively (Smacker, and Ogg
  Vorbis with stb_vorbis); nothing beyond SDL is built for WebAssembly.
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
through `SDL_OpenURL` all apply. A build needs only SDL3 for macOS (CMake
builds it when none is installed) and a `.app` bundle.

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

### Multiplayer

Network, modem and direct-connection games are played over TCP, through the
game's own menus and protocol. One side hosts, the other joins:

```sh
build/port/heroes --data DIR --port 1995                      # the host
build/port/heroes --data DIR --join host.example.org:1995     # the guest
```

`--port N` (default 1995; or `$HOMM1_NET_PORT`) is where a host listens, and
`--join ADDRESS[:PORT]` (or `$HOMM1_NET_JOIN`) is whom a guest calls; IPv4,
IPv6 (`[::1]:1995`) and host names work. The host's port must be reachable
(open it in a firewall or forward it on a router); the guest needs nothing.
Both then choose the game in the menus as in the original:

- **Network** (*New Game*, *Multi-player*, *Network*): the host chooses
  *Host*, the guest *Guest*; the host then picks the scenario and the game
  starts on both. A guest without `--join` finds a host on the same local
  network by the original's broadcast (UDP, the same port).
- **Direct connection**: the host chooses *Host* and listens, the guest
  *Guest* and connects, retrying every second until the host is there. The
  COM port and speed asked for are kept in the settings but do not matter.
- **Modem**: the port answers the game's modem commands as a Hayes modem
  would. The answering side (*Guest*) listens and rings when called; the
  dialling side (*Host*) types the address as the telephone number
  (`192.168.1.20:1995`), or any number of digits to dial `--join`.

Loading a saved multi-player game works the same way (*Load Game*,
*Multi-player*, ...). Chat (F2) and everything else the original sent work
as they did. Native programs on any system play each other; the Windows
build and the retail `HEROES.EXE` play a native program over a serial line
(see [the interoperability run](#interoperability)), not over NetBIOS.

The browser build compiles the same transports on Emscripten's socket
emulation, which carries TCP over a WebSocket and so needs a relay that
turns it back into TCP (websockify or Emscripten's own proxy); this is not
set up or tested.

#### Interoperability

`tools/port/serial_interop.py --heroes build/port/heroes [--retail]` plays a
direct-connection game between the native program and the Visual C++ build
(or the retail `HEROES.EXE`) under Wine: Wine's COM1 is a pseudo-terminal
that socat joins to the native program's TCP line. The native side hosts,
the Windows side joins, and both play a full turn cycle; every saved game
sent must arrive with the same hash. Against the Visual C++ build all
transfers agree byte for byte. Against the retail program they agree
except where the retail program's own defects strike: it decodes a save
with a window it did not reset (two padding bytes of the map name arrive as
zeros) and sends its own saves without the compressed stream's last four
bytes (the last bytes of the map visit flags arrive as whatever the
receiver's buffer held); see [divergences.md](divergences.md).

NetBIOS play between the native program and a Windows one is out of reach:
Wine's NetBIOS (`netapi32`, version 11.8) implements neither `NCBLISTEN`,
which the host waits with, nor the broadcast datagrams the guest finds the
host by, and the TCP framing here is the port's own. A real NetBIOS over
TCP/IP stack would also need the privileged ports 137 to 139.

### Sanitizers and tests

```sh
cmake -S . -B build/port-asan -G Ninja -DHOMM1_SANITIZERS=ON
ninja -C build/port-asan
HOMM1_DATA=~/.local/share/homm1-buka/game ctest --test-dir build/port-asan --output-on-failure
```

`records_test` checks the file record codecs and, with `HOMM1_DATA`, parses
and re-encodes every shipped map, campaign map, saved game, high score table
and the archive directory. `file_test` covers the game path resolver, `data_root_test` the search for
the game folder (quoted and trailing-separator `HOMM1_DATA`, the program's
folder from elsewhere, non-ASCII folders, file names in another case),
`media_test` (with `HOMM1_DATA`) decodes every shipped movie to the hashes
of what FFmpeg decoded and every music file to its length,
`catalog_check` and `catalog_update_check` run `catalog.py`,
`blit_test` the drawing routines, `lzhuf_test` the network save compressor,
`remote_records_test` the network message codecs (sizes, round trips,
refusal of short input, golden packets with their CRC and refusal of every
single-byte corruption),
`help_test` the help converter on a synthetic book and thousands of
damaged copies of it, `help_game_test` (with `$HOMM1_HELP` or
`HELP/HEROES.HLP` under `HOMM1_DATA`) on the real book,
`save_roundtrip` (with `HOMM1_DATA`, headless) loads the shipped
saved game in the program and saves it again, comparing the bytes, and
`editor_maps_test` (with `HOMM1_DATA`, headless) loads every shipped map with
the editor's own reader and saves it with its writer, comparing the bytes
region by region: the header text, the cells' terrain, object and overlay
bytes and the extra records must be unchanged; what the writer derives (the
format word, trigger bytes, the town, mine, artifact, obelisk and sound tables,
the cells above random towns that the editor clears on saving, and the layout
of older-format maps) is accounted for in its header comment; saving the saved
map again must reproduce it.
`save_diskfull` saves on a full disk (the save's temporary file is a link to
`/dev/full`) and checks that the game reports it and goes on.
`game_regressions_test` and `editor_regressions_test` (with `HOMM1_DATA`,
headless) check the retail out-of-bounds defects fixed in the shared units;
they are most useful in the sanitizer build.
`net_game_test` (with `HOMM1_DATA` and `xvfb-run`) runs two instances on
localhost with scripted input: a new network game through a full turn cycle;
a network battle, from a copy of that game's first save with the guest's
hero beside the host's, fought on auto combat, then a week of turns; and
the start of a direct-connection and of a modem game. With
`HOMM1_NET_TRACE=FILE` each instance writes a hash of every saved game it
sends, receives and loads, and of every battle's outcome (result, armies as
fought, heroes); the hashes must agree at every hand-off.
The `*_replay` tests replay the fuzz harnesses' regression inputs (see
[Fuzzing the file parsers](#fuzzing-the-file-parsers)).
`nix flake check` builds the native and sanitizer builds and runs their tests
(without game data), builds the Windows programs and runs `file_test` and
`data_root_test` under Wine (`windows-tests`), and builds the `heroes`
launchers (`nix/game.nix`) without game data. `HOMM1_DATA=DIR nix run
.#windows-smoke` starts the Windows build under Wine and Xvfb from a
player's game folder three ways (from it, from another folder, with a quoted
`HOMM1_DATA`) and the scenario editor from it, and requires the main menu
(the editor's map) each time.
`-DHOMM1_SANITIZERS_RECOVER=ON` keeps going after undefined behaviour, to
survey a whole session.

### Sanitizer survey

The long-running survey is opt-in (`-DHOMM1_SURVEY=ON`, not part of ctest).
It builds `homm1_survey` and `homm1_editor_survey`, which link the programs'
units with their own entry points and play them headless, and
`tools/port/survey.py` runs them in parallel and groups what the sanitizers
report:

```sh
cmake -S . -B build/port-asan -G Ninja -DHOMM1_SANITIZERS=ON \
      -DHOMM1_SANITIZERS_RECOVER=ON -DHOMM1_SURVEY=ON
ninja -C build/port-asan
tools/port/survey.py --build build/port-asan --data ~/.local/share/homm1-buka/game \
      --parts ai,campaign,load,combat,editor,monkey,monkey-editor --jobs 8
```

- `ai`: every shipped map with every player a computer player for `--days`
  days; each week the game is saved, loaded and saved again, and the two
  files must match.
- `campaign`: every campaign scenario's opening; `load`: the shipped saved
  games, continued by the computer.
- `combat`: random battles on the combat screen between computer players,
  with random heroes, armies, spells and artifacts, against heroes, monsters
  and towns (with and without castles).
- `editor`: random maps from the editor's generator with random settings,
  each saved, read and saved again, then played by the computer.
- `monkey`, `monkey-editor`: the game and the editor under random clicks and
  keys (including the menu bar and F4) under `xvfb-run`; after every 25
  actions and a pause, the display must equal the game's own picture
  (a difference is a region drawn but never copied to the screen).

Each finding is printed once with how often it was seen and a command that
reproduces it; a run that stops making progress is stopped
(`HOMM1_SURVEY_WATCHDOG`) and its stack printed. `--keep-logs` keeps every
run's output. The survey programs of a build without sanitizers also run
under valgrind (`nix shell nixpkgs#valgrind`; `valgrind --track-origins=yes
build/port/tests/port/homm1_survey combat MAP 5 1`), which finds the reads
of uninitialised memory the sanitizers do not.

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
codecs (`fuzz_records`), the network and serial message codecs, packets and
payloads (`fuzz_remote`), the network save compressor (`fuzz_lzhuf`) and the
Smacker movie decoder (`fuzz_smacker`). The
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
`shot PATH`, `check [PATH]` (compare the display with the game's own
picture and log the differing region; with a path, save a screenshot when
they differ), `quit` (the window's close button), `exit` (end at once). While
a replay runs the real mouse and keyboard are ignored.
`HOMM1_NO_DIALOGS=1` sends message boxes to the log only.
The programs take SIGTERM as a request to close the window, which a hung
program never reads; for unattended runs use `timeout -k 5 SECONDS`, or set
`SDL_NO_SIGNAL_HANDLERS=1` (the headless test programs, harnesses and survey
programs set it) so that SIGTERM ends them.
`HOMM1_TIME_SCALE=N` runs the game's clock N times faster than real time
(animations, delays and the replay's times alike); `HOMM1_TICK_START=N`
starts the clock at N instead of 1,000,000 (for example just below 2^31, to
see the tick count wrap).

## Installing with Nix

The installable package (`packages.x86_64-linux.default`, `nix/game.nix`)
puts `heroes-te` and `heroes-te-editor` launchers (`nix/launch.sh`) on the native
programs, with the menu entries and the icons of your copy's programs. Its
`game` may be the CD image, the CD's files, an installed game folder or a
`.zip`/`.7z`/`.rar` of one, or a folder holding only the image (`game =
homm1-game;`). `nix/game-data.py` unpacks a `.rar` with unar, checks the copy (the resource archive by
SHA-256, the other files by name and size), unpacks the CD's installer when
it is a CD and lays the game out in the store; nothing is fetched from a
binary cache. Without `game` the programs are installed alone, and the first
start imports your copy from `HOMM1_GAME` into
`$XDG_DATA_HOME/homm1-te/data` (`~/.local/share/homm1-te/data`).

The programs run on `$XDG_DATA_HOME/homm1-te/game`, where `ANIM`, `SOUND`,
`HELP` and the resource archive are links into the data, and the files the
programs write (saved games in `GAMES`, maps in `MAPS`, high scores and the
network save in `DATA`) are copied from it once and never overwritten or
brought back after being deleted. `--data DIR` or `HOMM1_DATA` bypasses the
launcher's folder and runs on `DIR` as it is. The package overrides as
`.override { game = ...; locale = "en"; editor = false; }`; the module's
options (`programs.homm1-te.game`, `.locale`, `.editor.enable`, `.package`) do
the same. The launchers, menu entries, icons, per-user folder and module
options all carry `-te`, apart from the port branch's (`heroes`,
`heroes-editor`, `homm1`, `programs.homm1`), so both editions install side by
side.

## The Visual C++ 6 build

The branch still builds the original programs, `HEROES.EXE` and
`EDITOR.EXE`, with the Visual C++ 6.0 SP5 toolchain under Wine:

```sh
nix develop -c python3 build.py --icon-from /path/to/HEROES.EXE              # Russian
nix develop -c python3 build.py --locale en --icon-from /path/to/HEROES.EXE  # English
nix develop -c python3 build.py --target all \
    --icon-from /path/to/HEROES.EXE --icon-from /path/to/EDITOR.EXE
```

This writes `build/ru/HEROES.EXE` or `build/en/HEROES.EXE`, and with
`--target editor` (or `all`) `build/<locale>/EDITOR.EXE`. The flake fetches
the hash-pinned Visual C++ 6.0 SP5, WinG and DirectX 1 files and supplies
Wine and LLVM's resource tools. Each unit compiles with its own retail
optimization profile (`build.json`) and links in retail object order. The
editor reuses the game's BASE library and its `kbwin`, `REQUEST` and
`wingraph` sources, compiled a second time with the editor's own profiles and
`HOMM1_EDITOR` defined. `--icon-from` takes each program's icon from your own
executable of the same name. Game data and the Smacker, Miles and Audiere
runtime DLLs are not included.

`nix run .#play` builds `HEROES.EXE` and runs it on your copy under Wine:

```sh
nix run .#play -- --game /path/to/game-or-cd.iso   # first run
nix run .#play                                     # later runs
nix run .#play -- --locale en --window             # English build, in a 640x480 window
nix run .#editor -- --window                       # the scenario editor
```

The first run checks the game files, copies them into
`~/.local/share/homm1-buka/` (`$XDG_DATA_HOME`), builds the program with the
game's icon, creates a Wine prefix with the CD as drive `D:` and the game's
registry key, and starts the game full screen at 640x480 (`--window` runs it
in a 640x480 Wine desktop window). Later runs rebuild only when the sources
changed and remember the language. Saved games and high scores stay in
`~/.local/share/homm1-buka/game/`. The editor uses the same game copy, Wine
prefix, CD drive and registry key. Other options: `--rebuild`,
`--prefix-reset`, `--dry-run` and `-- ARGS` for the program;
`nix run .#play -- --help` lists them.

### The text catalog

The source keeps every piece of game text as `localization::Tr("id")`. Both
builds resolve each ID to the selected language as literals in its Windows
code page, in a copy of the sources (`build/<locale>/localized/`,
`build/<preset>/localized/`); nothing is looked up at run time. Edit the
catalogs, not the copies. The English build changes only the program's own
text and menus; the game's data files stay as installed.

`locales/messages.pot` lists every ID the source uses. Each language is a
`locales/<lang>.po` translation plus a `locales/<lang>.json` descriptor: its
Windows code page, resource language, system locale, glyph set and keyboard
table. To add a language, write its descriptor, run `python3 catalog.py
update` to create its `.po`, translate every entry, check it with
`python3 catalog.py check` and build with `--locale <lang>` (natively
`-DHOMM1_LOCALE=<lang>`). Source text outside the catalog is ASCII without
numeric escapes; ctest runs `catalog.py check` and `update --check`
(`catalog_check`, `catalog_update_check`). The game draws text with the
fonts in its data files, which have glyphs for ASCII and, in Buka's edition,
Cyrillic; a language needing other letters also needs new fonts.

## Architecture

```text
src/BASE, src/SOURCE    the game: shared by both builds
src/EDITOR              the scenario editor (with BASE, SOURCE units built
                        with HOMM1_EDITOR)
include/BASE, SOURCE    its headers; *Host.h are the Windows host's alone
include/PLATFORM        File.h, Records.h: shared, plain C++98
                        Platform.h: the native platform interface
                        Net.h: sockets for the network and serial transports
src/PLATFORM            File.cpp, Records.cpp (both builds); Text.cpp,
                        MsvcRuntime.cpp, Help.cpp (the WinHelp converter),
                        Net.cpp (native)
src/PLATFORM/SDL3       the SDL3 backend; its Media.cpp decodes movies and music
src/PLATFORM/Web        the browser build's page and its file system setup
src/PORT                native replacements of the Windows-bound units
tools/port              localize.py (game text), units.py (each program's
                        units from build.json), menus.py (menus from the
                        .rc scripts), sync.sh, serial_interop.py
tools/help              homm1-hlp2html, the converter on the command line
nix                     the Windows and browser builds and checks, the installer
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
| SMACKW32.DLL | `PORT/SOURCE/Smacker.cpp` | Smacker decoding (`PLATFORM/SmackerDecoder`) |
| `SOURCE/netwin.cpp`, `comwin.cpp` | `PORT/SOURCE/netwin.cpp`, `comwin.cpp` | TCP sessions and streams (`PLATFORM/Net.h`) |
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
encode every file record field by field; `remoteRecords.h` (`REMOTEREC.cpp`)
does the same for every network and serial message. They compile under
Visual C++ 6 too, so the two builds read and write the same files and speak
the same protocol, the original's, byte for byte.

**Network play.** `REMOTE.cpp`, the game's protocol (packets with a CRC,
confirmations and retries, heartbeats, the compressed save transfer at each
change of turn, the battle hand-off and the battle's actions), is the same in
both builds. Below it the Visual C++ build calls Windows' NetBIOS and serial
ports; the native `netwin.cpp` keeps the NetBIOS session table, its status
bits, queues and retries and carries each session over a TCP connection
(`PLATFORM/Net.h`), and `comwin.cpp` carries the serial line's bytes over a
TCP stream, with a small Hayes modem for modem play. Between turns the
peers exchange the whole saved game; battles run on both sides in lockstep
from one random seed, which is why every build draws random numbers with the
Microsoft runtime's generator.

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
| Movies | Smacker, with sound, by the port's decoder (frames, palettes and sound identical to FFmpeg's for every shipped movie: `media_test`). |
| Menu bars | Drawn for the game (default, adventure, combat, town) and the editor, with check marks, greyed items, submenus and a modal loop. |
| Help file | Converted from `HELP\HEROES.HLP` to HTML on first use and shown in the browser (a new tab in the browser build); greyed without the file. |
| Network, modem, direct cable | Over TCP (see [multiplayer](#multiplayer)): new and loaded games, battles, chat; two instances agree at every hand-off (`net_game_test`). Plays the Visual C++ build and the retail program over a serial line under Wine. |
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
git rebase --rebase-merges --onto origin/source-buka-2003 "$old" port
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

The second (`52922394` to `d59c9910`: named constants, `b8`/`b32` flags with
`true` and `false`, helper families such as `WRITE_FILE_VALUE`, renamed
unknown members, inlined strings) was replayed with `--rebase-merges`, which
keeps the side branches' merges; a merge's own resolutions (its remerge diff)
were carried over by hand. Conflicts were where a port commit rewrote lines
the regeneration had re-spelled: the record codecs replacing `read`/`write`
and `WRITE_FILE_VALUE` calls, the bug pass's fixes in conditions that gained
named constants (`COMBAT_OPPOSING_SIDE`, `MAP_EVENT_TRIGGER`,
`TOWN_GATE_NO_TOWN`), and code the port had moved out of a unit (the movie
sound setup into `Audio.cpp`, `SetGameDefaults` into `KBCOMMON.cpp`, the
`*Host.h` declarations), which took the regeneration's changes in its new
place. Windows spellings the regeneration added to shared units (`BOOL`,
`TRUE`, `FALSE`) became `b32`, `true` and `false` there; the record codecs
follow the renamed members (`m_unused00`, `m_unused9a`, `m_unused19`) and
the network codec's entry points take `b8` flags like `TransmitRemoteData`.

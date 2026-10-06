# Heroes of Might and Magic — Tournament Edition, native port

The Tournament Edition (TE 1.05 f3) of the 2003 Buka Heroes of Might and
Magic and its scenario editor, as source changes on the reconstructed C++
tree, ported to Linux, Windows and the browser. Supply your own copy of the
Buka game (its CD image, the CD or an installed game folder); game data is
not bundled.

## Branches

```text
decomp-win95-1.0 -------------------> decomp-win95-1.1
    |                                    |
    v                                    v
source-win95-1.0                     decomp-win95-1.2
                                         |
                                         v
                                     decomp-buka-2003
                                         |
                 +-----------------------+---------+
                 |                                 |
                 v                                 v
         source-buka-2003                  classic-buka-2003
                 |
      +----------+------------+
      |                       |
      v                       v
  source-te                  port ------------------> port-te (you are here)
```

- [`decomp-win95-1.0`](https://github.com/sushi-shi/homm1-decomp/tree/decomp-win95-1.0#branches) — Win95 1.0 `HEROES.EXE` (Feb 1996)
- [`decomp-win95-1.1`](https://github.com/sushi-shi/homm1-decomp/tree/decomp-win95-1.1#branches) — Win95 1.1 `HEROES.EXE` (May 1996)
- [`decomp-win95-1.2`](https://github.com/sushi-shi/homm1-decomp/tree/decomp-win95-1.2#branches) — Win95 1.2 `HEROESW.EXE` (Aug 1997), VC4.1
- [`source-win95-1.0`](https://github.com/sushi-shi/homm1-decomp/tree/source-win95-1.0#branches) — Clean source, Win95 1.0
- [`decomp-buka-2003`](https://github.com/sushi-shi/homm1-decomp/tree/decomp-buka-2003#branches) — Buka 2003 game and editor, byte-identical
- [`source-buka-2003`](https://github.com/sushi-shi/homm1-decomp/tree/source-buka-2003#branches) — Clean source, Buka 2003 (ru/en)
- [`classic-buka-2003`](https://github.com/sushi-shi/homm1-decomp/tree/classic-buka-2003#branches) — Reading view, UTF-8 Russian
- [`source-te`](https://github.com/sushi-shi/homm1-decomp/tree/source-te#branches) — Tournament Edition on the source
- [`port`](https://github.com/sushi-shi/homm1-decomp/tree/port#branches) — Native port: Linux, Windows, browser
- [`port-te`](https://github.com/sushi-shi/homm1-decomp/tree/port-te#branches) — Tournament Edition on the port

## Play on Linux

On x86_64 Linux with Nix flakes enabled:

```sh
HOMM1_GAME=/path/to/heroes.iso nix run github:sushi-shi/homm1-decomp/port-te
nix run github:sushi-shi/homm1-decomp/port-te#heroes-editor     # the scenario editor
```

Supply your own copy of the Buka 2003 edition: its CD image, the CD's files
or an installed game folder (a `.zip`, `.7z` or `.rar` of one works too).
Tested image: `Герои. Платиновая версия [Бука].iso`, SHA-256:

```text
cd9f410094783e40c537bbca20990c0e9128e6fe71fecff5fd54ccc8bdd99c72
```

archive.org keeps it as `***REMOVED***` in
[***REMOVED***](https://archive.org/details/***REMOVED***);
`HOMM1_GAME` may name the downloaded `.rar`.

The first launch checks the copy (the resource archive by SHA-256, the other
files by name and size) and imports its data into
`~/.local/share/homm1-te/data`; later launches need no `HOMM1_GAME`. Saved
games (with the edition's `H1TE` header), the editor's maps and the high
scores are written to `~/.local/share/homm1-te/game`, apart from the Buka
port's `homm1` folder. The settings of both programs are kept in
`~/.config/homm1/heroes-te-ru.cfg` (or `heroes-te-en.cfg`), with the edition's
options (`HMM1 ShowEnemyMobility`, `SoftRetreatSurrender`, `SlightlyHarderAI`,
`CheatMode`, `OriginalCheatKeys`, `LosslessAudio`, `PlayVideos`,
`BattleMessageFormat`; [docs/te](docs/te/catalogue.md)). The game starts in a
window; the intro movies play only with `PlayVideos=1`.

Pass the game's options after `--`, for example
`nix run github:sushi-shi/homm1-decomp/port-te -- --fullscreen`:

| Option | Purpose |
| --- | --- |
| `--window`, `--fullscreen` | Start in a window or at full screen (F4 switches while playing) |
| `/C1` | Colour pointers instead of the monochrome ones |
| `--port N`, `--join ADDRESS[:PORT]` | Network, modem and direct-connection games over TCP ([multiplayer](docs/port/README.md#multiplayer)) |
| `--data DIR` | Run on an existing game folder as it is |

`HOMM1_DATA` does what `--data` does, `HOMM1_CD` names a folder with the CD's
music tracks and `HOMM1_CONFIG` another settings folder. `heroes --help`
lists the options.

## Install with a NixOS flake

Add the port and a local folder holding your copy of the game to your flake
inputs:

```nix
inputs.homm1.url = "github:sushi-shi/homm1-decomp/port-te";
inputs.homm1-game = {
  url = "path:/path/to/folder-with-the-iso";
  flake = false;
};
```

Import the module and name your copy:

```nix
outputs = { nixpkgs, homm1, homm1-game, ... }: {
  nixosConfigurations."<host>" = nixpkgs.lib.nixosSystem {
    modules = [
      ./configuration.nix
      homm1.nixosModules.default
      {
        programs.homm1 = {
          enable = true;
          edition = "te";                      # the default on this branch
          game = "${homm1-game}/heroes.iso";   # your image's file name
        };
      }
    ];
  };
};
```

Nix checks the copy and lays its data out in its store when the
configuration is built. Rebuild, replacing `<host>` with your host's name,
then launch:

```sh
sudo nixos-rebuild switch --flake '.#<host>'
heroes             # the game
heroes-editor      # the scenario editor
```

Instead of a local copy, a module of the configuration can fetch the one on archive.org:

```nix
{ pkgs, ... }:
let
  heroes-buka = pkgs.fetchurl {
    name = "heroes-platinum-buka.rar";   # the importer goes by the extension
    url = "https://archive.org/download/***REMOVED***/%D0%93%D0%B5%D1%80%D0%BE%D0%B8.%20%D0%9F%D0%BB%D0%B0%D1%82%D0%B8%D0%BD%D0%BE%D0%B2%D0%B0%D1%8F%20%D0%B2%D0%B5%D1%80%D1%81%D0%B8%D1%8F%20%5B%D0%91%D1%83%D0%BA%D0%B0%5D.rar";
    hash = "sha256-YWAmitzQ5TozJxS8Jph6ATp7KB0BRjJFS7+JOjnQBPk=";
  };
in {
  programs.homm1 = { enable = true; game = heroes-buka; };
  system.extraDependencies = [ heroes-buka ];
}
```

The 1 GB archive is downloaded into the store once. `system.extraDependencies`
(`home.extraDependencies` with home-manager) keeps it through garbage
collection, so a rebuild that imports the game again does not download it
again.

With home-manager, the same options install the game for one user:

```nix
homeConfigurations."<user>" = home-manager.lib.homeManagerConfiguration {
  pkgs = nixpkgs.legacyPackages.x86_64-linux;
  modules = [
    homm1.homeManagerModules.default
    { programs.homm1 = { enable = true; game = "${homm1-game}/heroes.iso"; }; }
  ];
};
```

`programs.homm1.edition` is `"te"` on this branch; `"buka"` is the port
branch's flake, which builds the Buka edition. `programs.homm1.locale = "en"`
builds the programs with English text and menus, and
`programs.homm1.editor.enable = false` leaves the editor out;
[more about the install](docs/te/README.md#installing-it-with-the-flake).

## Controls

The game is played with the mouse, as the original was; the keyboard has the
original's shortcuts.

| Action | Keyboard / mouse |
| --- | --- |
| Select, move, act | Left mouse button |
| Information about what is under the pointer | Right mouse button (hold) |
| Move the hero one cell | Arrow keys or keypad, diagonals on 7, 9, 1 and 3 |
| Next hero / next town | H / T |
| Open the hero's or the town's screen | Enter |
| Cast a spell / dig / view the world / puzzle map | C / D / V / P |
| Scenario information | I |
| Save / load / new game / quit | S / L / N / Q |
| Quick save / quick load | F5 / F9 |
| Network chat | F2 |
| Full screen on or off | F4 |
| Combat: skip the stack's turn, cast, view the hero, view the stack | Space, C, H, T |
| Close a dialog | Escape |

## Build from source

From the `port-te` branch:

```sh
nix develop
cmake --preset linux
cmake --build --preset linux
build/linux/heroes --data /path/to/game           # the game
build/linux/heroes-editor --data /path/to/game    # the editor
```

The game folder is the one holding `DATA`, `MAPS`, `GAMES`, `SOUND` and
`ANIM` (in any case): an installed game, or `~/.local/share/homm1-te/data/game`
after a first `nix run`. Without `--data` the programs also look beside
themselves and in the current folder. `-DHOMM1_LOCALE=en` builds the English
text. Without Nix: CMake 3.25, Ninja, a C++20 compiler (GCC 12+ or Clang
15+) and Python 3; CMake downloads and builds SDL 3 when it is not installed.
Details, tests and the Visual C++ 6 build: [docs/port](docs/port/README.md);
the edition's changes and how each is implemented: [docs/te](docs/te/README.md).

## Browser

### Linux

Inside `nix develop`:

```sh
emcmake cmake --preset wasm
cmake --build --preset wasm
python3 -m http.server --directory build/wasm
```

### Windows

The browser version can be built directly on Windows. Install
[Git](https://git-scm.com/downloads/win), [Python 3](https://www.python.org/downloads/windows/),
[CMake 3.25+](https://cmake.org/download/) and [Ninja](https://ninja-build.org/),
with their commands available on `PATH`. In PowerShell, from your `port-te`
checkout:

```powershell
git clone --depth 1 --branch 5.0.6 https://github.com/emscripten-core/emsdk.git build/emsdk
.\build\emsdk\emsdk.bat install 5.0.6
.\build\emsdk\emsdk.bat activate 5.0.6
Set-ExecutionPolicy -Scope Process RemoteSigned
.\build\emsdk\emsdk_env.ps1
emcmake cmake --preset wasm
cmake --build --preset wasm
python -m http.server --directory build/wasm
```

CMake downloads SDL automatically. For later builds, repeat from
`Set-ExecutionPolicy`.

### Play

Open [the game](http://localhost:8000/), choose your game folder and press
**Play**; `?program=editor` opens the scenario editor. The files are copied
into the browser's storage for the page and nothing is uploaded; saved games
stay there, and clearing the site's data removes them.

## Windows (native)

With Nix, on Linux:

```sh
nix build .#windows
```

`result/bin` holds the edition's `heroes.exe`, `heroes-editor.exe` and every
DLL they need
(`SDL3.dll`, `libmcfgthread-2.dll`). Copy these files into your installed
game folder (the one with `DATA`, `MAPS`, `GAMES`, `SOUND` and `ANIM`) and
start `heroes.exe` or `heroes-editor.exe` there; the edition's lossless music
goes in an `Audio` folder there (`Track NN.flac`). Elsewhere, start them with
`--data C:\Games\Heroes` or set `HOMM1_DATA` to that folder (the quotes cmd's
`set HOMM1_DATA="C:\Games\Heroes"` keeps are fine).

On Windows itself, with [MinGW-w64](https://www.mingw-w64.org/) (for example
MSYS2's UCRT64 GCC), CMake 3.25+, Ninja, Python 3 and Git on `PATH`:

```powershell
cmake --preset windows
cmake --build --preset windows
```

CMake downloads SDL and links it in: `build\windows\heroes.exe` and
`heroes-editor.exe` need no DLL beside them. Settings and the converted help
are kept in `%APPDATA%\homm1`.

## Multiplayer

- Network, modem and direct-connection games run over TCP between the
  native programs, the Windows build and, over a serial line under Wine, the
  retail program: [multiplayer](docs/port/README.md#multiplayer).
- Network games need the same version on both sides: the edition's
  protocol refuses the Buka game's.

## Documentation

- [Port guide](docs/port/README.md): details, tests and the original's Visual
  C++ 6 build
- [Help](docs/port/README.md#help): the game's own help book
  (`HELP\HEROES.HLP`), converted to a page in your browser
- [Divergences](docs/port/divergences.md): where the port behaves differently
  from the original, and why
- [Lessons](docs/port/lessons.md): what porting this code takes
- [Tournament Edition](docs/te/README.md): the edition's changes, how each is
  implemented, and [the edition on the native port](docs/te/README.md#the-edition-on-the-native-port-port-te)

## License

Original project contributions are dedicated to the public domain under
[CC0 1.0](LICENSE), to the extent of the contributors' rights. This does not
grant rights to New World Computing's or Buka's game or to the Microsoft, RAD
Game Tools or other third-party material it uses. Game assets are not included.

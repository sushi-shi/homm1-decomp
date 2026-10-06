# Heroes of Might and Magic — Buka 2003 source

C++ source for the 2003 Buka edition of Heroes of Might and Magic (`HEROES.EXE`,
Windows) and its scenario editor (`EDITOR.EXE`), built with the original Visual
C++ 6.0 SP5 toolchain. The text lives in a catalog with one translation per
language: `locales/ru.po` (the retail Russian) and `locales/en.po` (English).
Building selects one of them.

## Build and play

On x86-64 Linux with Nix flakes enabled, from this directory, with your copy of
the Buka 2003 game (an installed game folder, the CD or its `.iso` image):

```sh
nix run .#play -- --game /path/to/game-or-cd.iso   # first run
nix run .#play                                     # later runs
nix run .#play -- --locale en --window             # English build, in a 640x480 window
```

The first run checks the game files, copies them into
`~/.local/share/homm1-buka/` (`$XDG_DATA_HOME`), builds `build/ru/HEROES.EXE`
with the game's icon, creates a Wine prefix with the CD as drive `D:` and the
game's registry key, and starts the game full screen at 640x480. Later runs
rebuild only when the sources changed and remember the language. Saved games
and high scores stay in `~/.local/share/homm1-buka/game/`. If the screen cannot
switch to 640x480 the game stops at start-up; `--window` runs it in a 640x480
Wine desktop window instead. Other options: `--rebuild`, `--prefix-reset`,
`--dry-run` (print the steps, change nothing) and `-- ARGS` for the game;
`nix run .#play -- --help` lists them.

## Scenario editor

```sh
nix run .#editor -- --game /path/to/game-or-cd.iso   # first run, if the game was never imported
nix run .#editor -- --window                         # later runs, in a 640x480 window
```

`nix run .#editor` builds `build/<locale>/EDITOR.EXE` and runs it with the same
game copy, Wine prefix, CD drive and registry key as the game. The editor
needs the same installation: it opens `DATA/HEROES.AGG` from the game folder,
finds the CD by its first music track, reads the game's registry key (its
window settings are the `HMM1 Editor...` values) and loads and saves maps in
`~/.local/share/homm1-buka/game/MAPS/`. It takes the same options as the game's
runner.

## Branches

```text
decomp-win95-1.0 ---> decomp-win95-1.1 ---> decomp-win95-1.2 ---> decomp-buka-2003
        |                                                                 |
        v                                                    +------------+------------+
source-win95-1.0                                             |                         |
                                                             v                         v
                                                     source-buka-2003         classic-buka-2003
                                                             |
                                                    +--------+--------+
                                                    |                 |
                                                    v                 v
                                                  port            source-te
                                                    |
                                                    v
                                                 port-te
```

| Branch | Purpose |
| --- | --- |
| [decomp-win95-1.0](https://github.com/sushi-shi/homm1-decomp/tree/decomp-win95-1.0) | Reconstruction of the February 1996 Win95 1.0 `HEROES.EXE` |
| [decomp-win95-1.1](https://github.com/sushi-shi/homm1-decomp/tree/decomp-win95-1.1) | Reconstruction of the May 1996 Win95 1.1 `HEROES.EXE` |
| [decomp-win95-1.2](https://github.com/sushi-shi/homm1-decomp/tree/decomp-win95-1.2) | Maintained reconstruction of the August 1997 Win95 1.2 `HEROESW.EXE`, using VC4.1 |
| [decomp-buka-2003](https://github.com/sushi-shi/homm1-decomp/tree/decomp-buka-2003) | Reconstruction of the 2003 Buka `HEROES.EXE`, using VC6 SP5 |
| [source-win95-1.0](https://github.com/sushi-shi/homm1-decomp/tree/source-win95-1.0) | Generated clean source for Win95 1.0 |
| [source-buka-2003](https://github.com/sushi-shi/homm1-decomp/tree/source-buka-2003) | Generated clean source for Buka 2003: the primary C++ tree, with its Russian and English text catalog |
| [classic-buka-2003](https://github.com/sushi-shi/homm1-decomp/tree/classic-buka-2003) | The same generated tree as a reading view, its text spelled out as UTF-8 Russian |
| [port](https://github.com/sushi-shi/homm1-decomp/tree/port) | Native port of the game and editor on `source-buka-2003`: SDL3 on Linux, Windows and the browser, multiplayer over TCP, the help book: [docs/port](docs/port/README.md) |
| [source-te](https://github.com/sushi-shi/homm1-decomp/tree/source-te) | The Tournament Edition (TE 1.05 f3) as source changes on `source-buka-2003` |
| [port-te](https://github.com/sushi-shi/homm1-decomp/tree/port-te) | The Tournament Edition on the native port |

This branch is `port`: the native build is described in [docs/port/README.md](docs/port/README.md).

## Install with a NixOS flake

The native game and its scenario editor install as `heroes` and
`heroes-editor` on x86_64 Linux. Add this flake and a local folder holding
your copy of the Buka 2003 game (its CD image, the CD's files or an installed
game folder; a `.zip` or `.7z` of one works too) to your flake inputs:

```nix
inputs.homm1.url = "github:sushi-shi/homm1-decomp/port";
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
          game = "${homm1-game}/heroes.iso";   # your image's file name
        };
      }
    ];
  };
};
```

`game` may also be the input itself (`game = homm1-game;`) when the folder
holds only the image, or the folder of an installed game. Nix checks the copy
(the resource archive by SHA-256, the other files by name and size) and lays
its data out in its store when the configuration is built; nothing is fetched
from a binary cache. Rebuild, replacing `<host>` with your host's name, then
launch:

```sh
sudo nixos-rebuild switch --flake '.#<host>'
heroes             # the game
heroes-editor      # the scenario editor
```

Both are in the desktop's application menu too, with the icons of your copy's
programs. Saved games, the editor's maps and the high scores are written to
`~/.local/share/homm1/game` (`$XDG_DATA_HOME`), where the game's read-only
files are links into the store; settings are kept in `~/.config/homm1`.

With home-manager, the same options install the game for one user:

```nix
homeConfigurations."<user>" = home-manager.lib.homeManagerConfiguration {
  pkgs = nixpkgs.legacyPackages.x86_64-linux;
  modules = [
    homm1.homeManagerModules.default
    {
      programs.homm1 = {
        enable = true;
        game = "${homm1-game}/heroes.iso";
      };
    }
  ];
};
```

Other options: `programs.homm1.locale = "en"` builds the programs with English
text and menus (the game's pictures stay as installed; default `"ru"`), and
`programs.homm1.editor.enable = false` leaves the editor out. Without `game`
the programs are installed alone, and the first start imports your copy from
`HOMM1_GAME` into `~/.local/share/homm1/data`:

```sh
HOMM1_GAME=/path/to/heroes.iso heroes
```

To try it without installing:

```sh
HOMM1_GAME=/path/to/heroes.iso nix run github:sushi-shi/homm1-decomp/port
nix run github:sushi-shi/homm1-decomp/port#heroes-editor
nix run github:sushi-shi/homm1-decomp/port -- --window /I0   # options go after --
```

`heroes --help` lists the options. The package is also
`packages.x86_64-linux.default`, overridable with
`.override { game = ...; locale = "en"; editor = false; }`.

## Build

On x86-64 Linux with Nix flakes enabled, from this directory:

```sh
nix develop -c python3 build.py --icon-from /path/to/HEROES.EXE              # Russian
nix develop -c python3 build.py --locale en --icon-from /path/to/HEROES.EXE  # English
nix develop -c python3 build.py --target editor --icon-from /path/to/EDITOR.EXE
nix develop -c python3 build.py --target all \
    --icon-from /path/to/HEROES.EXE --icon-from /path/to/EDITOR.EXE
```

This writes `build/ru/HEROES.EXE` or `build/en/HEROES.EXE`, and with
`--target editor` (or `all`) `build/<locale>/EDITOR.EXE`. The flake fetches the
hash-pinned Visual C++ 6.0 SP5, WinG and DirectX 1 files and supplies Wine and
LLVM's resource tools. Each unit compiles with its own retail optimization
profile (`build.json`) and links in retail object order. The editor reuses the
game's BASE library and its `kbwin`, `REQUEST` and `wingraph` sources, compiled
a second time with the editor's own profiles and `HOMM1_EDITOR` defined;
`src/EDITOR/` holds the editor's own units and `src/EDITOR/Editor.rc` its menu,
About box and icon. `--icon-from` takes each program's icon from your own
executable of the same name. Game data and the Smacker, Miles and
Audiere runtime DLLs are not included.

The source keeps every piece of game text as `localization::Tr("id")`. The build
resolves each ID to the selected language as literals in its Windows code page
in a copy of the sources under `build/<locale>/localized/`; nothing is looked up
at run time. Edit the catalogs, not the copies. The English build changes only
the program's own text and menus; the game's data files stay as installed.

`locales/messages.pot` lists every ID the source uses. Each language is a
`locales/<lang>.po` translation plus a `locales/<lang>.json` descriptor: its
Windows code page, resource language, system locale, glyph set and keyboard
table. To add a language, write its descriptor, run `python3 catalog.py update`
to create its `.po`, translate every entry, check it with
`python3 catalog.py check` and build with `--locale <lang>`. The game draws text
with the fonts in its data files, which have glyphs for ASCII and, in Buka's
edition, Cyrillic; a language needing other letters also needs new fonts.

`nix run .#play` and `nix run .#editor` run the results with your game data; see
[Build and play](#build-and-play) and [Scenario editor](#scenario-editor).

## Regeneration

`decomp-buka-2003` generates `source-buka-2003` with `homm1 clean`. This branch
is that generated source plus the port's commits, replayed onto each
regeneration ([sync procedure](docs/port/README.md#keeping-up-with-the-source-branch)).
Reconstruction changes belong on `decomp-buka-2003`; portability changes
belong here.

## License

Original project contributions are dedicated to the public domain under
[CC0 1.0](LICENSE), to the extent of the contributors' rights. This does not
grant rights to New World Computing's or Buka's game or to the Microsoft, RAD
Game Tools or other third-party material it uses. Game assets are not included.

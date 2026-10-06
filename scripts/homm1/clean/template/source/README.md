# Heroes of Might and Magic — Buka 2003 source

C++ source for the 2003 Buka edition of Heroes of Might and Magic (`HEROES.EXE`,
Windows), built with the original Visual C++ 6.0 SP5 toolchain. The game text
lives in a catalog with one translation per language: `locales/ru.po` (the
retail Russian) and `locales/en.po` (English). Building selects one of them.

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
| port | Cross-platform port based on `source-buka-2003` (planned) |
| source-te | Branch based on `source-buka-2003` (planned) |

This branch is `source-buka-2003`.

## Build

On x86-64 Linux with Nix flakes enabled, from this directory:

```sh
nix develop -c python3 build.py --icon-from /path/to/HEROES.EXE              # Russian
nix develop -c python3 build.py --locale en --icon-from /path/to/HEROES.EXE  # English
```

This writes `build/ru/HEROES.EXE` or `build/en/HEROES.EXE`. The flake fetches the
hash-pinned Visual C++ 6.0 SP5, WinG and DirectX 1 files and supplies Wine and
LLVM's resource tools. Each unit compiles with its own retail optimization
profile (`build.json`) and links in retail object order. `--icon-from` takes the
program icon from your own executable. Game data and the Smacker, Miles and
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

`nix run .#play` runs the result with your game data; see [Build and play](#build-and-play).

## Regeneration

`decomp-buka-2003` generates this branch with `homm1 clean`. Make source changes
there and regenerate; do not edit this branch by hand.

## License

Original project contributions are dedicated to the public domain under
[CC0 1.0](LICENSE), to the extent of the contributors' rights. This does not
grant rights to New World Computing's or Buka's game or to the Microsoft, RAD
Game Tools or other third-party material it uses. Game assets are not included.

# Heroes of Might and Magic — Tournament Edition source

The Tournament Edition (TE 1.05 f3) of the 2003 Buka Heroes of Might and Magic
(`HEROES.EXE`, Windows) and its scenario editor (`EDITOR.EXE`), as ordinary
source changes on the generated `source-buka-2003` tree, built with the
original Visual C++ 6.0 SP5 toolchain. The text lives in a catalog with one
translation per language: `locales/ru.po` (Russian) and `locales/en.po`
(English). Building selects one of them.

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
  source-te (you are here)   port ------------------> port-te
```

- [`decomp-win95-1.0`](https://github.com/sushi-shi/homm1-decomp/tree/decomp-win95-1.0) — Win95 1.0 `HEROES.EXE` (Feb 1996)
- [`decomp-win95-1.1`](https://github.com/sushi-shi/homm1-decomp/tree/decomp-win95-1.1) — Win95 1.1 `HEROES.EXE` (May 1996)
- [`decomp-win95-1.2`](https://github.com/sushi-shi/homm1-decomp/tree/decomp-win95-1.2) — Win95 1.2 `HEROESW.EXE` (Aug 1997), VC4.1
- [`source-win95-1.0`](https://github.com/sushi-shi/homm1-decomp/tree/source-win95-1.0) — Clean source, Win95 1.0
- [`decomp-buka-2003`](https://github.com/sushi-shi/homm1-decomp/tree/decomp-buka-2003) — Buka 2003 game and editor, byte-identical
- [`source-buka-2003`](https://github.com/sushi-shi/homm1-decomp/tree/source-buka-2003) — Clean source, Buka 2003 (ru/en)
- [`classic-buka-2003`](https://github.com/sushi-shi/homm1-decomp/tree/classic-buka-2003) — Reading view, UTF-8 Russian
- [`source-te`](https://github.com/sushi-shi/homm1-decomp/tree/source-te) — Tournament Edition on the source
- [`port`](https://github.com/sushi-shi/homm1-decomp/tree/port) — Native port: Linux, Windows, browser
- [`port-te`](https://github.com/sushi-shi/homm1-decomp/tree/port-te) — Tournament Edition on the port

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

## Edition changes

[docs/te](docs/te/README.md) describes how the edition's changes are laid out
in the source; [the catalogue](docs/te/catalogue.md) lists every change of the
edition and the decisions this branch took, and
[changes.tsv](docs/te/changes.tsv) records how each row is implemented.

## License

Original project contributions are dedicated to the public domain under
[CC0 1.0](LICENSE), to the extent of the contributors' rights. This does not
grant rights to New World Computing's or Buka's game or to the Microsoft, RAD
Game Tools or other third-party material it uses. Game assets are not included.

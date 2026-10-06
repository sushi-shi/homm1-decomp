# Heroes of Might and Magic — Win95 1.0 source

C++ source for the Windows 95 release of Heroes of Might and Magic
(New World Computing, February 1996 `HEROES.EXE`), built with the original
Visual C++ 4.0 toolchain.

## Branches

```text
decomp-win95-1.0 -------------------> decomp-win95-1.1
    |                                    |
    v                                    v
source-win95-1.0 (you are here)      decomp-win95-1.2
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
  source-te                  port ------------------> port-te
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

## Build and play

On x86-64 Linux with Nix flakes enabled, from this directory:

```sh
nix run path:. -- --data "/path/to/HEROES"
nix run path:.                      # later launches reuse the remembered folder
```

`--data` is your installed Windows 95 game folder with `DATA/HEROES.AGG`,
`SMKWAI32.DLL` and `WAIL32.DLL`. Pass `--cd /path/to/cd` with the CD's contents
for sound effects and videos. Each launch builds the game, installs it in
`build/game/game/` (saves and high scores stay there; your folder is never
written) and runs it in its own Wine prefix with the CD mapped as drive `D:`.
gamescope integer-scales the 640x480 desktop; `PLAY_GAMESCOPE=0` runs plain
Wine. `--dry-run` prepares everything without starting the game.

## Build

```sh
nix develop -c python3 build.py --icon-from /path/to/HEROES.EXE
```

This writes `build/HEROES.EXE`. The flake fetches the hash-pinned Visual C++
4.0, MASM 6.11, WinG and DirectX 1 files and supplies Wine and LLVM's resource
tools. `--icon-from` takes the program icon from your own executable. Game data
and the Smacker and Miles runtime DLLs are not included.

## Regeneration

`decomp-win95-1.0` generates this branch with `homm1 clean`. Make source changes on
`decomp-win95-1.0` and regenerate; do not edit this branch by hand.

## License

Original project contributions are dedicated to the public domain under
[CC0 1.0](LICENSE), to the extent of the contributors' rights. This does not
grant rights to New World Computing's game or to the Microsoft, RAD Game Tools
or other third-party material it uses. Game assets are not included.

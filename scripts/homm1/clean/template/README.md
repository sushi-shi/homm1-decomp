# Heroes of Might and Magic — source

C++ source for the Windows 95 release of Heroes of Might and Magic
(New World Computing, May 1996 (Windows 95 1.1) `HEROES.EXE`), built with the original
Visual C++ 4.0 toolchain.

## Branches

```text
              master
                 |
                 v
  source-win95-1.1-1996 (you are here)
```

| Branch | Purpose |
| --- | --- |
| `master` | Reconstruction and matching against the Win95 1.1 `HEROES.EXE` |
| `source-win95-1.1-1996` | Generated clean source (`homm1 clean`): no matching annotations or comments; builds the game with the pinned toolchain |

## Play

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

`master` generates this branch with `homm1 clean`. Make source changes on
`master` and regenerate; do not edit this branch by hand.

## License

Original project contributions are dedicated to the public domain under
[CC0 1.0](LICENSE), to the extent of the contributors' rights. This does not
grant rights to New World Computing's game or to the Microsoft, RAD Game Tools
or other third-party material it uses. Game assets are not included.

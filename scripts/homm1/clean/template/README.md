# Heroes of Might and Magic — source

C++ source for the Windows 95 release of Heroes of Might and Magic
(New World Computing, February 1996 `HEROES.EXE`), built with the original
Visual C++ 4.0 toolchain.

```text
          master
             |
             v
  source-win95-1996 (you are here)
```

| Branch | Purpose |
| --- | --- |
| `master` | Reconstruction and matching |
| `source-win95-1996` | Clean, buildable source |

## Build

On x86-64 Linux with Nix flakes enabled:

```sh
nix develop -c python3 build.py --icon-from /path/to/HEROES.EXE
```

This writes `build/HEROES.EXE`. Copy it into your installed game directory and
run it with Wine. The flake fetches the hash-pinned Visual C++ 4.0, MASM 6.11,
WinG and DirectX 1 files and supplies Wine and LLVM's resource tools.
`--icon-from` takes the program icon from your own executable; without it the
build has no icon. Game data is not included.

## Regeneration

`master` generates this branch with `homm1 clean`. Matching annotations,
retail line pins, reconstruction comments and tooling are absent. Make source
changes on `master` and regenerate; do not edit this branch by hand.

## License

Original project contributions are dedicated to the public domain under
[CC0 1.0](LICENSE), to the extent of the contributors' rights. This does not
grant rights to New World Computing's game or to the Microsoft, RAD Game Tools
or other third-party material it uses. Game assets are not included.

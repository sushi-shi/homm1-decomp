# Heroes of Might and Magic: `port`

This branch ports the 2003 Buka Heroes of Might and Magic and its scenario
editor to Linux, Windows and the browser. It is built from `source-buka-2003`,
the edition's clean C++ source, with SDL3 in place of Windows.

## Build, run and test

```sh
nix develop                     # .#port: the native build alone
cmake --preset linux            # presets: linux, wasm (emcmake), windows (MinGW)
cmake --build --preset linux
build/linux/heroes --data DIR   # build/linux/heroes-editor: the editor
ctest --preset linux            # HOMM1_DATA=DIR adds the tests on game data
nix flake check                 # native, sanitized, windows, windows-tests, launcher
nix build .#windows             # also .#native, .#wasm; nix run .#web serves it
HOMM1_GAME=PATH nix run         # the launcher; .#heroes-editor; .#native -- --data DIR
```

## Layout

| Path | Contents |
| --- | --- |
| `src/SOURCE`, `src/BASE`, `src/EDITOR` | the game, its engine library, the editor (headers: `include/`) |
| `src/PORT`, `src/PLATFORM` | native units for the Windows-bound ones; SDL3, web |
| `tests/port`, `tools/port` | ctest programs, fuzzers, survey; its runner, interop |
| `nix/`, `locales/` | launcher, Windows and browser builds; text catalogs |

## Headless testing hooks

- `HOMM1_INPUT_REPLAY=FILE` scripts either program, one `MS ACTION ARGS` a line
  (`+MS` after the previous): `click X Y`, `key K`, `shot PATH` (a BMP
  screenshot), `check [PATH]` (display against the game's picture), `exit`.
- `HOMM1_NO_DIALOGS=1` logs message boxes; use `xvfb-run`, `SDL_AUDIO_DRIVER=dummy`.
- `HOMM1_TIME_SCALE=N` speeds the clock up; `HOMM1_TICK_START=N` sets its start.
- `HOMM1_SURVEY_WATCHDOG=SECONDS` stops a stalled survey run (`-DHOMM1_SURVEY=ON`).
- `HOMM1_NET_TRACE=FILE` hashes every save and battle a network game exchanges.

## Branch rules

- `port` and the edition sources (`source-buka-2003`, `source-win95-1.0`) stay
  faithful to retail behaviour; `docs/port/divergences.md` lists what must differ.
- `source-te` and `port-te` fix bugs as normal code.
- Windows builds stay fully static: each `.exe` imports only Windows' own DLLs.
- Merge only changes that add no new complexity.
- Don't commit game data or retail binaries.

## Documentation

`README.md` (playing, installing) and `docs/port/` (builds, tests, lessons).

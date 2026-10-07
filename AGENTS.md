# Heroes of Might and Magic: `source-te`

This branch is the Tournament Edition (TE 1.05 f3) of the 2003 Buka Heroes of
Might and Magic and its scenario editor, as ordinary source changes. It is built
from `source-buka-2003` and compiles the Windows programs with the original
Visual C++ 6.0 SP5 toolchain under Wine.

## Build, run and test

```sh
nix develop                     # Wine, LLVM, Python and the pinned VC6 toolchain
python3 build.py                # build/ru/HEROES.EXE; --locale en, --icon-from EXE
python3 build.py --target all   # also build/ru/EDITOR.EXE
nix run .#play -- --game PATH   # build and run the game; later runs drop --game
nix run .#editor -- --window    # the editor, in a 640x480 Wine desktop
python3 catalog.py check        # every language's catalog
```

There is no test suite here: the port branches carry the tests.

## Layout

| Path | Contents |
| --- | --- |
| `src/SOURCE`, `src/BASE`, `src/EDITOR` | the game, its engine library, the editor (headers: `include/`) |
| `build.json`, `build.py`, `play.py` | per-unit compiler profiles; the build; the Wine runner |
| `locales/`, `catalog.py` | text catalogs (`ru.po`, `en.po`) and their tool |
| `imports/`, `vendor/` | import stubs and headers of the runtime DLLs; lzhuf |

## Headless testing hooks

- None in the programs: they are the original Windows programs under Wine.
- `nix run .#play -- --dry-run` prints the runner's steps and changes nothing.
- `--window` keeps the game in a 640x480 Wine desktop instead of full screen.
- `--state DIR` moves the game copy and Wine prefix (default `homm1-buka`).
- `HOMM1_INPUT_REPLAY` and the other hooks are on `port` and `port-te`.

## Branch rules

- `port` and the edition sources (`source-buka-2003`, `source-win95-1.0`) stay
  faithful to retail behaviour.
- `source-te` and `port-te` fix bugs as normal code.
- Windows builds stay fully static on the port branches; these programs need
  the original's Smacker, Miles and Audiere DLLs, as retail did.
- Merge only changes that add no new complexity.
- Don't commit game data or retail binaries.

## Documentation

`README.md` (building, playing) and `docs/te/` (`catalogue.md`, `changes.tsv`).

# Generated source branches

`homm1 clean` derives two trees from committed `HEAD`, with no matching
machinery, and can publish each one as a local single-commit branch:

| Variant | Branch | Contents |
| --- | --- | --- |
| `source` | `source-buka-2003` | The primary C++ tree of the game and the scenario editor ([editor](editor.md)). Text stays as catalog references (`localization::Tr("id")`) with one catalog per language ([localization](localization.md)). Builds either program in any of them with the pinned VC6. |
| `classic` | `classic-buka-2003` | The same tree as a reading view. Every reference is spelled out as a readable UTF-8 Russian literal. The integer-enum and name-mangling model is unchanged. |

```sh
homm1 clean                                        # build/source
homm1 clean --verify                               # build and compare
homm1 clean --verify --publish                     # branch source-buka-2003
homm1 clean --variant classic --verify --publish   # branch classic-buka-2003
```

`--out` defaults to `build/<variant>`, and `--ref REVISION` exports another
commit. `--working-tree` previews tracked and staged files, but it cannot
publish. Output inside the checkout must be a child of `build/`. An existing
output is replaced only when it carries the generator's marker.

`source-buka-2003` is the base for later branches (a cross-platform port and
`source-te`). Change the source on `decomp-buka-2003`, then regenerate.

The trees carry their own build, not this branch's tooling. The
game-behaviour gate (`homm1 verify behaviour`,
[workflow](workflow.md#game-behaviour-gate)) and its contracts program stay on
this branch: they check the reconstruction before it is exported.

## What is removed

| Construct | Becomes |
| --- | --- |
| `VA`, `VA_DECL`, `DATA`, `VA_COMPGEN`, `RVA_DYNINIT` | deleted |
| `H1_ENUM_BEGIN`/`_SPLIT`/`FLAGS_BEGIN`/`CONST_BEGIN` ... `_END` | `enum Name { ... };` |
| `H1_ENUM_PARAM`/`RETURN`/`LOCAL`/`STORAGE(Name, type)` | `type` |
| `H1_ENUM_CAST(Name, type, value)` | `static_cast<type>(value)` |
| `OVERRIDE` | deleted |
| `H1_C_LINKAGE` | `extern "C"` |
| `#include <match.h>` | `#include <H1/Ints.h>` (match.h also defines the integer aliases) |
| `#include <Domains.h>`, `<H1/Macros.h>` | deleted with the headers |
| `#line N "..."` | deleted; the compiler supplies `__FILE__`/`__LINE__` |
| `#define name storage // spelling fixes .bss order` | deleted; the readable name stays, and MASM references to the storage spelling take it |
| `#define name storage // frame-slot spelling` before a function, `#undef name` after it | deleted; the function's readable local name stays |
| `//`, `/* */` and MASM/RC/DEF `;` comments | deleted |

Each rule is the expansion that VC6 already compiles in the matching build.
The `H1_ENUM_*` rules select that production branch, not Domains.h's C++20
strict view. VC6 cannot compile `enum class`, `using enum` or
`H1EnumStorage`. The strict view is not buildable as a tree either: a clang-cl
`/std:c++20` syntax check of the strict headers and units reports 5,534
errors. The enum declarations stay named.

The lexer never enters string or character literals. It removes comments
without joining tokens and expands macro arguments innermost first.
Generation fails in any of these cases:

- a comment, scaffolding name or `#line` survives;
- a construct leaves a stranded `;` or `,`;
- a frame-slot alias is not `#undef`'d right after its function, brackets
  more than one definition, or its readable name also spells a parameter,
  member or qualified name there (or its storage spelling appears there);
- `src/` holds a file that no rule covers;
- in the classic view, a catalog reference survives.

Both trees carry the unit sources of both programs (`src/EDITOR/` with the
shared BASE and SOURCE units), every header, `Heroes.rc` and `Editor.rc`, the
module definition and import stubs for `mss32.dll`, `smackw32.dll` and
`audiere.dll`. The stubs come from the game's retail import table through
`homm1.graph.implib`; the editor imports two of the game's Audiere functions
and nothing else without an SDK library. Shared sources keep their
`#ifdef HOMM1_EDITOR` branches: `HOMM1_EDITOR` is a real build define that the
editor's compile of every unit passes.

The source tree also carries:

- `locales/` (`messages.pot`, each language's `.po` and `.json` descriptor)
  and `catalog.py`;
- a `build.json` link contract and `build.py`;
- `play.py`, the game runner shared with `homm1 play` ([playing](play.md)),
  behind the flake's `nix run .#play` and, with `--target editor`,
  `nix run .#editor`;
- a flake and a README.

`build.json` has one entry per program under `targets` (`game`, the default,
and `editor`). Each lists its units with their full VC6 flags (the per-image
profile of `config/units.toml` and the image's defines), its resource script,
and its link: the retail object order, the BASE library and the retail
`/OPT:NOREF`. The game's order is computed from the annotations before they are
removed; the editor's comes from `config/retail/editor/link_order.tsv`. Each
program's library line, BASE library start and stack are its
`homm1.graph.link` profile (the editor's: the game's libraries without WINMM,
mss32, smackw32 and NETAPI32, and LINK's default stack). No address reaches
the tree.

The classic tree drops the build files and the catalog. It keeps a README that
points to `source-buka-2003` for building. Research notes and all tooling stay
on `decomp-buka-2003`.

## Localization

The source tree names each piece of text by catalog ID. Its `build.py
--locale LANG` writes a copy of the sources to `build/<LANG>/localized/` with
each reference resolved to the selected language's literal: bytes in its
Windows code page, or a brace-enclosed character initializer for `Chars`.
`Heroes.rc` gets wide Unicode string literals and the descriptor's resource
language. Nothing is looked up at run time.

The classic view resolves the same references once, for Russian:

- C++ literals are UTF-8 text, with controls escaped (`\n`, `\t`, `\r` or
  three-digit octal);
- `Heroes.rc` starts with `#pragma code_page(65001)`, uses `""`-doubled RC
  literals and declares language `0x19`.

The view is for reading. VC6 copies literal bytes as written, so compiling the
UTF-8 files would not reproduce the Windows-1251 strings.

## Verification

`--verify` first runs `homm1 link` so that the matching objects and the
candidate are current. It then compiles the trees with the pinned VC6 and each
unit's `config/units.toml` profile. MASM units are assembled as the retail
link's OMF and as comparison COFF. Every tree links through `homm1.graph.link`
without `/FORCE`, and an unresolved external fails verification.

Both programs are compiled: the game's 68 units against `build/objdiff/base`,
and the editor's 39 (its own units and the shared ones again, with the
editor's profiles and `HOMM1_EDITOR`) against `build/editor/objdiff/base`.

- **Control.** The tree applies the same expansions and comment removal, but
  keeps every line, the `#line` pins and the scaffolding headers and includes.
  Each unit compiles in the matching build's localization view (the
  length-padded catalog macros through the forced include). The control must
  reproduce every non-debug section of all 68 game and 39 editor objects
  (bytes, relocations and symbol names) and both candidates, `HEROES.EXE` and
  `EDITOR.EXE` (linked with the editor's `homm1.graph.link` profile), byte for
  byte. The only exception is LINK's TimeDateStamps (PE header, export,
  resource and debug directories, and the CodeView signature). This proves
  that the transforms change no code.
- **Source.** The generated tree is compiled from its Russian localized copy,
  as its own `build.py` does, and must link. Its objects are compared after
  VC6's compiler-local names (`$L`, `$T`, `$SG`, `$E`, `$S`, `$label$N`) are
  renumbered by first appearance, and the matching objects' `.bss` storage
  spellings are read as their readable names. These counters advance with every macro a
  compilation defines, so they shift once the scaffolding headers are gone.
  Every remaining difference is listed, and the command fails unless the
  differing unit's source uses `H1_ASSERT`, `__FILE__` or `__LINE__` or
  defines a function behind frame-slot aliases. Without the `#line` pins,
  those assertions carry their own line numbers and file names; without the
  aliases, the locals take their readable spellings' `/Od` slots. Currently
  36 of 68 game units and 20 of 39 editor units are identical
  (`differences.tsv` lists the rest).
- **Resources.** `Heroes.rc` and `Editor.rc` of the source tree must compile
  to their retail programs' payloads (`homm1.tool.rc`).
- **Standalone** (source variant). The tree's `build.py --target all` runs
  through its own flake for every language of its catalog (`--locale en`,
  `--locale ru`). Each run must produce `build/<locale>/HEROES.EXE` and
  `build/<locale>/EDITOR.EXE`.
- **Classic equivalence** (classic variant). The classic files must equal the
  source tree's Russian compiler input token for token. Each UTF-8 literal is
  read back as the Windows-1251 bytes it shows, and it may stand for a byte
  literal or for a `Chars` brace initializer. `Heroes.rc` strings must carry
  the same text as the rendered wide literals. The classic tree itself is not
  compiled.

The per-section differences are written to
`build/<variant>-verify/differences.tsv`. Nothing is banked, and this is not a
retail match claim. The game has not been run from a generated executable here.

## Publication

`--publish [BRANCH]` writes the tree as one root commit through a private Git
index, so no worktree changes. The default branch is `<variant>-buka-2003`. The
commit message records `Generated-By: homm1 clean` and `Source-Commit:`.

- Regeneration replaces the snapshot.
- An identical regeneration from the same commit is a no-op.
- The command refuses a branch that is checked out in any worktree, and a
  branch whose tip it did not generate.
- It never pushes. Update a remote with an explicit
  `--force-with-lease=<branch>:<expected tip>`.

## Standalone build

```sh
nix develop -c python3 build.py --icon-from /path/to/HEROES.EXE              # Russian
nix develop -c python3 build.py --locale en --icon-from /path/to/HEROES.EXE  # English
nix develop -c python3 build.py --target editor --icon-from /path/to/EDITOR.EXE
nix develop -c python3 build.py --target all --icon-from HEROES.EXE --icon-from EDITOR.EXE
```

The flake fetches the hash-pinned Buka toolchain release (VC6 SP5 and the
vendor SDK files) and supplies Wine and LLVM. `build.py` works as follows:

- `--target` selects `game` (the default), `editor` or `all`. Each program's
  objects, BASE library and resources go to `build/<locale>/<target>/`, its
  executable and map to `build/<locale>/`.
- It compiles each unit with its retail profile in a fresh directory, using a
  Wine prefix under `build/wineprefix`.
- It builds the vendor import libraries from the stubs.
- It compiles resources with `llvm-rc`/`llvm-cvtres`.
- It links with the retail library line and object order.

The program icons are retail assets, so `--icon-from` extracts each from the
user's executable of the same name (`HEROES.EXE`, `EDITOR.EXE`; repeatable);
`nix run .#play` and `nix run .#editor` pass the ones from the imported game
copy.
`--out` moves `build/` elsewhere: the runner builds into its state directory
when it runs from the flake's read-only copy. The generated README carries the
branch diagram and these instructions. `homm1 clean --verify` also runs
`nix run .#play -- --dry-run` and `nix run .#editor -- --dry-run` against a
fresh state directory (and checks the game copy named by `HOMM1_GAME`, when
set).

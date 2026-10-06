# Generated source branches

`homm1 clean` derives two trees from committed `HEAD`, with no matching
machinery, and can publish each one as a local single-commit branch:

| Variant | Branch | Contents |
| --- | --- | --- |
| `source` | `source-buka-2003` | The primary C++ tree. Game text stays as catalog references (`localization::Tr("id")`) with an English and a Russian catalog. Builds either language with the pinned VC6. |
| `classic` | `classic-buka-2003` | The same tree as a reading view. Every reference is spelled out as a readable UTF-8 Russian literal. The `HOMM1_RUSSIAN` conditionals keep their Russian branch. The integer-enum and name-mangling model is unchanged. |

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
- `src/` holds a file that no rule covers;
- in the classic view, a catalog reference or `HOMM1_RUSSIAN` survives, or an
  `#elif` follows a `HOMM1_RUSSIAN` conditional.

Both trees carry the unit sources, every header, `Heroes.rc`, the module
definition and import stubs for `mss32.dll`, `smackw32.dll` and `audiere.dll`.
The stubs come from the retail import table through `homm1.graph.implib`.

The source tree also carries:

- `locales/` (`messages.def`, `ru.po`, `format-variants.json`) and
  `catalog.py`;
- a `build.json` link contract and `build.py`;
- `play.py`, the game runner shared with `homm1 play` ([playing](play.md)),
  behind the flake's `nix run .#play`;
- a flake and a README.

`build.json` keeps the retail object order, the BASE library and the retail
`/OPT:NOREF`, computed from the annotations before they are removed. No address
reaches the tree.

The classic tree drops the build files and the catalog. It keeps a README that
points to `source-buka-2003` for building. Research notes and all tooling stay
on `decomp-buka-2003`.

## Localization

The source tree names each piece of text by catalog ID. Its `build.py
--locale ru|en` writes a copy of the sources to `build/<locale>/localized/`
with each reference resolved to the selected language's literal: Windows-1251
bytes, or a brace-enclosed character initializer for `Chars`. In the same copy,
`HOMM1_RUSSIAN` becomes `1` or `0`. `Heroes.rc` gets wide Unicode string
literals and `LANG_RUSSIAN` or `LANG_ENGLISH`. Nothing is looked up at run
time.

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

- **Control.** The tree applies the same expansions and comment removal, but
  keeps every line, the `#line` pins and the scaffolding headers and includes.
  Each unit compiles in the matching build's localization view (the
  length-padded catalog macros through the forced include). The control must
  reproduce every non-debug section of all 68 objects (bytes, relocations and
  symbol names) and the candidate `HEROES.EXE` byte for byte. The only
  exception is LINK's TimeDateStamps (PE header, export, resource and debug
  directories, and the CodeView signature). This proves that the transforms
  change no code.
- **Source.** The generated tree is compiled from its Russian localized copy,
  as its own `build.py` does, and must link. Its objects are compared after
  VC6's compiler-local names (`$L`, `$T`, `$SG`, `$E`, `$S`, `$label$N`) are
  renumbered by first appearance. These counters advance with every macro a
  compilation defines, so they shift once the scaffolding headers are gone.
  Every remaining difference is listed, and the command fails unless the
  differing unit's source uses `H1_ASSERT`, `__FILE__` or `__LINE__`. Without
  the `#line` pins, those assertions carry their own line numbers and file
  names. Currently 56 of 68 units are identical. The 12 that differ are the
  assertion units INPUTMGR, MOUSEMGR, RESMGR, WINMGR, miscwin, EVENTS, NOOPT,
  PATH, SMACKMGR, TOWNMGR, netwin and wingraph.
- **Standalone** (source variant). The tree's `build.py` runs through its own
  flake for `--locale ru` and `--locale en`. Each run must produce
  `build/<locale>/HEROES.EXE`.
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
```

The flake fetches the hash-pinned Buka toolchain release (VC6 SP5 and the
vendor SDK files) and supplies Wine and LLVM. `build.py` works as follows:

- It compiles each unit with its retail profile in a fresh directory, using a
  Wine prefix under `build/wineprefix`.
- It builds the vendor import libraries from the stubs.
- It compiles resources with `llvm-rc`/`llvm-cvtres`.
- It links with the retail library line and object order.

The program icon is a retail asset, so `--icon-from` extracts it from the
user's executable; `nix run .#play` passes the one from the imported game copy.
`--out` moves `build/` elsewhere: the runner builds into its state directory
when it runs from the flake's read-only copy. The generated README carries the
branch diagram and these instructions. `homm1 clean --verify` also runs
`nix run .#play -- --dry-run` against a fresh state directory (and checks the
game copy named by `HOMM1_GAME`, when set).

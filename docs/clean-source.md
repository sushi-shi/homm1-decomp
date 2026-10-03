# Generated source branch

`homm1 clean` derives a clean, buildable source tree from committed `HEAD` and
can publish it as a local single-commit branch. The port follows HoMM2 Buka's
`homm2 clean` and kf1's `kf clean`; see [tooling inheritance](tooling-inheritance.md).

```sh
homm1 clean --out build/clean                        # generate
homm1 clean --out build/clean --verify               # build and compare
homm1 clean --out build/clean --verify --publish     # branch source-win95-1.1-1996
```

`--ref REVISION` exports another commit. `--working-tree` previews tracked and
staged files but cannot publish. Output inside the checkout must be a child of
`build/`, and an existing output is replaced only when it carries the
generator's marker.

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
| `#line N "D:\\Heroes\\..."` | deleted; the compiler supplies `__FILE__`/`__LINE__` |
| `//`, `/* */` and MASM/RC/DEF `;` comments | deleted |

Each rule is the expansion VC4 already compiles in the matching build. The
`H1_ENUM_*` rules select that production branch, not Domains.h's C++20 strict
view: VC4 cannot compile `enum class`, `using enum` or `H1EnumStorage`, and
substituting the domain type even where its storage is `int` fails six units
with C2446/C2664 and moves code in two more. The enum declarations stay named.

The lexer never enters string or character literals, removes comments without
joining tokens and expands macro arguments innermost first. Generation fails
if a comment, scaffolding name or `#line` survives, if a construct leaves
stranded `;` or `,`, or if `src/` holds a file no rule covers.

The tree carries the unit sources, every header, `Heroes.rc`, the module
definition, a `build.json` link contract, import stubs for `wail32.dll` and
`smkwai32.dll` (from the retail import table through `homm1.graph.implib`), a
`build.py`, the `play.py` runner, a flake and a short README. LZHUF's reference sources, research
notes and all tooling stay on `master`. `build.json` keeps retail object order
and the BASE library, computed from the annotations before removal; no address
reaches the tree.

## Verification

`--verify` first runs `homm1 link` so the matching objects and candidate are
current. It then compiles two trees through `homm1.tool.cl`/fixedroot with each
unit's profile and links both through `homm1.graph.link` without `/FORCE`.

- The control tree applies the same expansions and comment removal but keeps
  every line, the `#line` pins and the scaffolding headers and their includes.
  It must reproduce every non-debug object section and the candidate
  `HEROES.EXE` byte for byte, apart from LINK's timestamps.
- The clean tree must compile and link. Its code differs: under retail's `/Gi`,
  VC4's symbol handles follow the path strings of the opened files and the
  source line numbers ([handle paths](patterns/vc4-gi-handles-follow-path-lengths.md),
  [line statics](patterns/vc4-gi-line-var.md)). Removing the scaffolding includes
  alone leaves 16 of 62 C++ units identical; the `#line` pins change assertion
  line words and file literals.
- Finally the tree's `build.py` runs through its own flake and must produce
  `HEROES.EXE`.

The per-section differences of both trees are written to
`build/clean-verify/differences.tsv`. Nothing is banked; this is not a retail
match claim. The game has not been run from the generated executable here.

## Publication

`--publish [BRANCH]` (default `source-win95-1.1-1996`) writes the tree as one root
commit through a private Git index, so no worktree changes. The message records
`Generated-By: homm1 clean` and `Source-Commit:`. Regeneration replaces the
snapshot. An identical regeneration from the same commit is a no-op. The command
refuses a branch checked out in any worktree and a branch whose tip it did not
generate. It never pushes; update a remote with an explicit
`--force-with-lease=<branch>:<expected tip>`.

## Standalone build

```sh
nix develop -c python3 build.py --icon-from /path/to/HEROES.EXE
```

The flake fetches the hash-pinned toolchain release (VC4, MASM 6.11, WinG,
DirectX 1) and supplies Wine and LLVM. `build.py` compiles each unit with its
retail profile in a fresh directory, builds the vendor import libraries from
the stubs, compiles resources with `llvm-rc`/`llvm-cvtres` and links with the
retail library line and object order. The program icon is a retail asset, so
`--icon-from` extracts it from the user's executable. Without the fixedroot
view the code is equivalent but not identical to the matching build.

## Playing the generated tree

```sh
nix run path:. -- --data "/path/to/HEROES"    # remembered; later: nix run path:.
```

The flake's app runs `play.py`, a copy of `homm1.graph.play` (see
[playing the build](play.md)): it builds with `build.py`, taking the icon from the
`HEROES.EXE` in `--data`, installs the result in `build/game/game/` beside copies
of the user's `DATA`, `MAPS` and `GAMES`, maps `--cd` or a stand-in as `D:`
and starts the game through gamescope in its own Wine prefix. `--dry-run`
stops before launching. The generated README carries the branch diagram and
these instructions.

# The scenario editor (`EDITOR.EXE`)

The Buka 2003 release ships a scenario editor, `EDITOR.EXE` (340,031 bytes,
linked 2003-02-26 by LINK 6.00, pinned as `editor` in
`config/retail/targets.json`). It is a second linked program of the same
reconstruction: its own link graph, its own retail facts and its own scores,
sharing the compiler, the headers and every source file proven common to
both programs.

## Images

Every authoritative fact is keyed by `(image, rva)`. `homm1 --image editor
<command>` (exported to child processes as `$HOMM1_IMAGE`) selects the
editor for any command; the default image is the game.

| Kind | Game | Editor |
| --- | --- | --- |
| Retail facts | `config/retail/` | `config/retail/editor/` |
| Objects, claims, model, delink, comparison | `build/` | `build/editor/` |
| MAX ledger | `config/match_baseline.tsv` | `config/match_baseline.editor.tsv` |

A `[[unit]]` lists the images it links into (`images = ["game", "editor"]`;
the default is the game alone). A unit compiles once per image: the editor's
BASE objects were compiled in another checkout (`U:\HMM\VSS\HMM1\Source\Base`
against the game's `E:\Users\igorl\VSS\HMM\HMM1\Source\Base`), and those
assertion paths are part of each object's data. The five BASE sources that
spell a path select it with `HOMM1_EDITOR`, the define
`[images.editor]` in `config/units.toml` adds to every editor compile.

`homm1 build` builds both images. The README block reports each image
separately.

## Claims

A claim spells the address of the program its source belongs to. Sources and
headers under `src/EDITOR/` and `include/EDITOR/` spell `EDITOR.EXE`
addresses; every other tree spells `HEROES.EXE` addresses
(`retail_labels.source.claim_space`). A shared unit therefore keeps its game
claims. The editor reads them through `config/retail/editor/placements.tsv`,
which joins each game identity (kind, game rva) to the editor address where
the same entity is proven to lie; the name stays the source's. A body the
shared source compiles differently for the editor names its editor address
with `VA_AT(editor, address, size)` beside its `VA`; each program reads only
the claims of its own address space.

`homm1 --image editor audit placements --write-config` derives that table and
the editor's provider tables from the game's resolved bindings and the two
retail images:

- A function is placed where its game body, with every absolute field and
  every rel32 operand masked, occurs at an editor census start. A body that
  occurs more than once is placed at the offset its unit's other bodies share
  (one object's code moves as a block). Two game bodies at one editor address
  are byte-identical and stay unnamed.
- A datum is placed by its code users: each absolute field of a placed body
  pairs the game target with the editor target at the same body offset, and
  every pair inside the datum must agree. A datum with no code user is placed
  only when its complete initialized bytes occur exactly once. Literal-pool
  names derived from a game address (`$SG<rva>`, `$T<rva>`) never carry over.
- An unplaced callee of a placed shared body takes the game callee's name: one
  source call site names one function (`PollSound`, `operator delete`).
- A shared function the editor compiles with another profile is placed where
  the editor's own compile of it, relocations masked, occurs once at a census
  start (inside the unit's reviewed span when it occurs more than once); its
  calls name their callees.
- Import thunks pair by their IAT import.
- Data referenced only by a `VA_AT` body is named from the editor's own compile
  of that body, where it equals the retail body with relocations masked.
- The vtable of a class only the editor defines is named where the editor's own
  constructor, relocations masked, equals retail and stores it, and every slot
  of the compiled vtable holds the address of the method the editor claims. A
  floating constant an editor-only body reads is named where retail's bytes
  equal its pool entry.

`--check` fails when the committed tables differ from a fresh derivation (for
example after a game rename).

## Census

`homm1 --image editor audit census --write-config` writes the editor's
`functions.tsv`, `absolute_relocations.tsv` and
`absolute_reference_evidence.tsv` from retail instructions alone: recursive
descent from the entry point, call targets, code addresses named by
instruction immediates and data words, import thunks, the unwind and catch
entries of every C++ `FuncInfo`, switch dispatch and index tables, and
unreached bodies at a `/Od` frame prologue or a 16-byte object boundary after
fill. Whole LIBCMT/OLDNAMES code and data contributions, matched with their
relocations masked, fix the runtime's starts and pointer fields. Data words
naming `.rdata`/`.data` are admitted with a typed corroboration (a
neighbouring pointer, an instruction user of the target, or a string start).
Run on the game, the same pass recovers 2,405 of the 2,420 reviewed starts and
17,499 of the 17,553 reviewed absolute fields with four extra fields.

| Editor census | Count |
| --- | ---: |
| Function starts | 1,365 |
| Functions (excluding the rows below) | 951 |
| EH registration stubs and funclets | 147 |
| Import thunks | 167 |
| Alignment fill | 100 |
| Absolute fields | 7,414 |

`homm1 --image editor audit dna-bands --write-config` classifies 373 runtime
bodies (349 masked-exact LIBCMT functions), 97 compiler helpers and the
remaining 481 reconstruction targets.

## Translation units

The editor's Rich header lists 35 C++ objects (C/C++ 12.00.8966), 109 LIBCMT
C objects and 9 MASM 6.13 objects. Each C++ object ends with the
`std::ctype<wchar_t>::id` initializer/registration pair; the editor has 35.

- 25 C++ objects and 4 MASM objects are the shared BASE library: all 277 of
  their source-claimed functions place byte-identically. BITS, LZHUF and
  LZHUFDEC are not linked (no editor code references them; 11 − 2 = 9 MASM
  objects).
- 10 C++ objects are editor-only (`config/retail/editor/link_order.tsv`).
  Retail assertion paths name `Editor\EDITMGR.CPP`, `Editor\EDITOR.CPP`,
  `Editor\OVERLAY.CPP` and `Editor\wingraph.cpp`; the other names are
  descriptive (the class names the managers store).
- `Editor\wingraph.cpp` is the game's `wingraph.cpp` compiled with the
  editor's profile: the editor's own objects expand string and memory
  intrinsics inline (`/Oi`, `cpp_editor_oi_g5`; its `strcpy`/`memset`/`memcpy`
  calls are `rep` sequences), which alone accounts for three of the four
  bodies that differ from the game's. The fourth, `WGUpdatePalette`, has no
  partial combat-screen redraw. `src/SOURCE/wingraph.cpp` is therefore one
  source for both programs (unit `SOURCE/wingraph`, `image_flags`), with the
  editor's path strings and that branch selected by `HOMM1_EDITOR`; all 33
  editor bodies are exact.
- `kbwin.cpp` and `REQUEST.cpp` are shared the same way (`REQUEST` with
  `/Ob2`, as in the game). The editor's `AppWndProc` and the requester's
  `ShowThisMap`, `Open`, `Main`, `Update` and `ShowMapInfo` are editor
  variants selected by `HOMM1_EDITOR`; `SetWinText` scans a 70-row table in
  the editor. The editor's window class, title and instance strings are its
  own catalog entries.
- `EDITOR.CPP` carries copies of six `KB`/`NOOPT` bodies inside an editor
  unit with its own functions and data; it stays an editor unit.

[`shared-function-accounting.tsv`](shared-function-accounting.tsv) lists every
editor reconstruction target with its shared unit, its identical game body or
none (`homm1 --image editor audit placements --accounting PATH`).

## Comparison

The editor compares under the same strict data matching. A data target with no
provided identity is fenced as an anonymous datum rather than refused: no
reconstructed name matches it, so the referencing function scores below 100%,
and `build/editor/gen/data_debt.tsv` lists the targets a claimed body
references. Reviewed data names carry the extent of the game datum they were
placed from, so interior references resolve to the owner and an addend.

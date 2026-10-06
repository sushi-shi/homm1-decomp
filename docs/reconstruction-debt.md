# Reconstruction debt review

The README checklist counts source constructs that a reconstruction tends to
introduce and the original developers probably did not write. Counts cover
`src` and `include`; vendor code is excluded. A count is a review input, not a
defect total: a Win32 boundary cast or a `goto` that retail's control flow
requires is correct source. Every cleanup preserves banked exact matches
(`homm1 compare --baseline`); where a change legitimately moves an `/Od` frame,
the local spellings are re-fitted with the
[VC6 slot rule](patterns/vc6-od-frame-slots.md).

## How the counts are measured

```sh
rg -o 'reinterpret_cast<[^>]+>' src include      # split: Win32 types (LP*, H*, P*, *PROC) vs game types
rg -c 'static_cast<' src include
rg -w goto src include                           # two hits are comments in Icon2bc.asm
rg -c '\bunion\b' src include
clang-cl /Zs -Wno-everything -Wunused-variable ...  # dead locals (#line blanked)
rg '\bm_(unknown|unk|pad|field)[A-Za-z0-9_]*' include
rg '\[\s*-\s*[0-9]' src include                    # negative indexing
rg 'CONTAINING_RECORD|container_of|\bthis\)\s*-' src include
rg 'va_start' src
```

## Categories

**Game-type `reinterpret_cast`.** Casts to Win32 handle, procedure and buffer
types at API boundaries are expected. Game-type casts mark data whose type is
not yet modelled:

- the icon frame directory (resolved): `icon` holds its loaded ICN resource in
  a union of the raw bytes (`m_data`), the leading `IconEntry` directory
  (`m_frames`) and the font code's word view (`m_frameWords`). Its 26 casts are
  gone and the code is unchanged;
- network packets (resolved): the remote queue hands out typed records.
  `GetRemoteData` returns `RemoteMessage*` (its output buffer `rcvBufOut` is a
  `RemoteMessage`), `CheckHandleNet`, `DoNetCombat`, `ReceiveHeroTownData` and
  `TransmitAndWait`'s response carry `RemoteMessage*`, and `SendRemoteData`,
  `ReceiveRemoteData`, `EncodePacket` and `DecodePacket` take the
  `RemoteMessage` they copy to or from the wire. `TransmitRemoteData` and
  `TransmitAndWait` take `void*` payloads, because they only copy the caller's
  bytes into the record. The relayed combat action is a `CombatRemoteAction`
  member of the `RemotePayload` union (it replaced the duplicate
  `CombatRemotePacket` record), and `ProcessNextAction` builds it in a typed
  local of the same 16 bytes. 17 casts are gone and every object is identical
  apart from the changed names. `SendHeroTownData` uses one allocation for the
  combat record and both hero fragments, through a local union of the two typed
  pointers (one frame slot, as in retail);
- resource reads and pixel buffers (resolved): `resourceManager::ReadBlock`
  takes `void*` (it only forwards to `_read`) and `Read13` takes `char*`. The
  widget and font name buffers are `char`. `bitmap::m_pixels` and
  `tileset::m_data` are `u8*`, because they hold palette indices.
  This removed 15 casts, including the fizzle loop's byte views, and the code is
  unchanged;
- the adventure search's occupancy flags (resolved): `TestPossibleDirections`
  only clears and sets them, and `SeedPosition` reads them zero-extended, so
  the parameter and both callers' arrays are `u8`;
- remaining views, each a different typed read of the same storage (10
  sites, each with its reason at the cast):
  - `EVENTS_REMOTE_MESSAGE`/`EVENTS_REMOTE_HERO` overlay a received record's
    payload as the combat record or a hero fragment. `combatRemoteData` holds
    `armyGroup` and `town` objects, whose constructors keep it out of the
    `RemotePayload` union (VC6 rejects union members with constructors);
  - `PollRemote` receives into `rcvBufIn` and reads it through
    `REMOTE_MESSAGE`: retail places that buffer 4-byte aligned, which a
    `RemoteMessage` object (8-byte aligned by VC6) cannot be, so it stays a
    `char` array;
  - the wire layer frames the `char` packet buffers with `RemotePacketHeader`
    (`REMOTE_PACKET`) and walks them as unsigned bytes for the CRC. The receive
    buffer also carries the direct-connect `ID` text, so it stays `char`;
  - palette channels: `palette::m_data` is `i8` because the fades compare its
    channels signed; `PostprocessPalette` moves `PaletteColor` triples,
    `CreateFizzleTables` reads the channels zero-extended as RGB
    rows, and `ConvertSmackerPalette` scales Smacker's 8-bit channels as `u8`;
  - `DoAdvance`'s win-text assertion passes the pointer to `ProcessAssert`'s
    `i32` condition, as retail pushes the pointer itself.

Casts whose operand or target is a Win32 type are API boundaries (77 sites):
`LPBYTE`/`PUCHAR`/`LPBITMAPINFO` buffer arguments, window and dialog
procedures, `GetProcAddress` results, the handle assertions and the
`HINSTANCE`/`WPARAM` comparisons (the value itself is the integer retail
tests), and the NetBIOS name bytes.

The fix is the real type at its owner (a typed member, a packet struct or
union), not an inline accessor: under `/Od /Ob1` an inlined accessor adds frame
slots and changes code.

**Manual byte layouts.** Indexing a packed record through a byte or word view
with hand-computed strides. The font glyph widths are read as
`widths[glyph * FONT_GLYPH_ENTRY_WORDS + FONT_GLYPH_WIDTH_WORD]`; retail's
`imul 6` with a scaled word index is only produced by an `i16` index, so the
arithmetic is evidenced, but the layout still needs a named type. Packet bytes
written or read by literal index belong to the same class.

**Negative offsets.** Pointer arithmetic that steps back from a member or
object (`[-N]`, `this - N`, container-of). None remain; the count guards
against new ones.

**Unknown members.** `m_unknownNN` and `m_field_0xNNN` placeholders keep a
class layout without a recovered name or type.

Names come from a code user:

- mouseManager's `m_savedLeft`/`m_savedTop` are the cursor area ComboDraw
  marks; nothing updates them after the constructor.
- playerData's `m_unusedSaveData` is the span Write zeroes and Read skips.

Spans that no code reads or writes are spelled `m_unused<offset>`, or
`m_padding<offset>` in recruitUnit. Nineteen
placeholders remain:

- inputManager (seven): written by its constructor, the event queue, the two
  option setters and advManager's context changes; nothing reads them.
- army (two), combatManager (four), heroWindowManager (two), town (one) and
  mouseManager (one): set only by constructors or Init.
- playerData `m_unknown00`/`m_unknown99`: only copied raw by Write/Read;
  `m_unknown99[1]` is written twice.

The editor image compiles the same BASE managers, so it was searched too:
no instruction in `EDITOR.EXE` or `HEROES.EXE` loads `gpInputManager`,
`gpMouseManager` or `gpWindowManager` and then reads one of these offsets.
Inventing a meaning for them is not
evidence, so they stay placeholders until a reader is found.

**`goto`.** A `goto` stays when retail's block layout requires it; it is
replaced only when a structured form compiles to identical bytes.

The 204 C++ statements (the other two `rg` hits are comments in
`src/BASE/Icon2bc.asm`) fall into these classes. The counts are from a source
scan, and each class was checked by compiling a structured replacement:

| Class | Count | Structured form tried | Result |
| --- | ---: | --- | --- |
| Forward skip to a shared tail outside a `switch` | 76 | `combatManager::GetControl` early exit as `else if` | 1056/1057. The nested `else` exits become a chain of `jmp`s (`jmp +2; jmp back`), while retail jumps once, directly to the tail. |
| Shared tail at the end of `switch` cases | 73 | (not tried) | Each case jumps to one common tail. Duplicating the tail adds code, and moving it after the `switch` changes the `break` targets. |
| Backward (retry or re-entry) | 24 | (not tried) | These re-enter the middle of a block, and no loop gives the same entry point. |
| Jump to the next statement | 17 | `KB.cpp` `goto L; L:` removed | 1056/1057. VC6 `/Od` emits a `jmp` for every `goto`, even one to the next instruction. |
| Single-loop exit | 12 | `fileRequester::fileRequester` `goto insert` and `ReceiveRemoteData` loop `goto done` as `break` | 1056/1057 for each. `break` emits a different jump than the retail `goto`. |
| Multi-level loop exit | 2 | (no structured form) | `break` leaves only the inner loop. |

Every site was also tried in place as `break;` and as `continue;`, one
compile per trial against the unchanged object (408 trials). 161 do not
compile, because the `goto` is not inside a loop, and none of the 247 that
compile leaves the object identical. The loop exits show why: VC6 `/Od`
compiles `if (c) break;` to one conditional jump to the loop exit, while
`if (c) goto L;` keeps a conditional jump around an unconditional `jmp L`.
`advManager::DoAdvCommand`'s route loop shrinks from 1345 to 1340 bytes with
`break`; retail has the `jmp` at every site.

Every goto is kept because retail's block layout requires it.

**Dead locals.** Every never-referenced local must correspond to an
unreferenced slot in retail's `/Od` frame (a hole between referenced slots or a
larger frame).

The audit lists locals with `clang-cl /Zs -Wunused-variable` after replacing
each `#line` with an empty line, so diagnostics keep the file's own numbering.
It found 116 such locals. The 41 with an initializer emit retail stores. A
control removed the 75 initializer-free declarations together: every function
that contained one lost its exact frame, and the others were unchanged. Each
of the 75 was then removed alone (its line blanked, so `#line` pins hold), and
every single removal changes its function. Under `/Od`, VC6 gives each
declared local its own slot, so each of these maps to an unread slot in
retail's frame. The 101 locals that are only written (`-Wunused-but-set-variable`)
are retail stores.

**Dead declarations.** A function or method declared in a header but never
defined, called or linked is a name with no body in either image. A libclang
pass over every game unit and the editor's `EDITOR.cpp` lists the
non-virtual declarations that no unit defines or references, that no object
(assembly units included) defines, and that carry no `VA_DECL` claim. The
adventure, game, hero, town and BASE headers lost 109 such declarations, and
the combat headers 98; two remain in `combatManager.h` for that lane.
Constructors, destructors and virtual methods stay, because the vtables and
object lifetimes evidence them.

**`static_cast`.** Narrowing and signedness conversions are often required for
retail's widths; the review removes the ones that only paper over a wrong
declared type. A libclang scan compared each cast's operand type with its target
type. The review removed 108 lines of casts, and each removal kept all 1057
bodies exact:

- Win32 handles. The menu, instance, window and DC handles were declared
  `void*`, so every API call cast them back. The owners are now typed `HMENU`,
  `HINSTANCE`, `HWND`, `HDC` and `HANDLE` under VC6's default `STRICT` handle
  types, and 60 casts are gone. Handle types only change mangling, so the
  claimed names of the retyped globals and of the functions that take them
  (`AppInit`, `AppWndProc`, `AppCommand`, the menu and paint functions) now use
  the `STRICT` spelling, matching `gAppWindow` and the other handle globals.
- Casts to the operand's own type: `u8` map-cell payloads, `u8` hit points and
  a `float` difference.
- `CONST` enum values converted to `int` or a narrower integer. These enums are
  unscoped in both views, so the cast does nothing.

A second libclang pass over every unit listed each `static_cast` whose operand and target are arithmetic types (or a
`CONST`/`FLAGS` enum, unscoped in both views) and whose value is consumed where
C++ applies the same conversion implicitly: the right side of `=` with a
left side of the target type, a variable initializer, a non-variadic argument,
a `return`, a compound assignment, or an arithmetic operand whose other
operand already has the target type. Removing those 167 casts left every
object byte-identical except one: `ScaleSampleVolume`'s
`GetEffectsVolume() * static_cast<float>(volume)` becomes an `fimul` from
memory without the cast, so it stays. Casts of names declared through the
`H1_ENUM_*` storage macros are excluded, because the strict view types them
as enums. `highScoreManager::Main`'s one cast of a `MessageModifier` mask is
gone too: every other modifier test in the tree is written without it.

The remaining casts are:

- `void*` results of `malloc`, `GlobalAlloc` and the resource cache, and the
  resource-to-subclass downcasts. C++ requires these casts.
- Strict-domain values converted to their storage width (`enum class` in the
  Clang view), including enum-indexed subscripts.
- `i8` ternary arms, which give the conditional its byte type.
- Conversions whose result feeds a wider operation (`static_cast<u8>(c) >=
  'a'`, `static_cast<i8>(Random(0, 3)) + 4`, integer-to-floating divisions):
  dropping these changes the value.
- `ScaleSampleVolume`'s `float` operand (above).

Retyping the owner was measured for the remaining narrowing casts on locals
and failed. For example, declaring `CheckEndGame`'s player index `i8` instead
of casting its seven `m_players` subscripts loses the function's exact match,
because the loop and the other subscripts use the full `int`.

**Verify-board text debt.** `homm1 verify board` ratchets several textual
metrics, and their committed floors are 0:

- Switch case labels and equality tests use named values: domain members,
  `_FIRST`/`_LAST` range markers, or `CONST` groups in the owning header
  (stage machines, campaign scenario rows, volume levels, ten-way rolls,
  layout counts).
- File-scope enums, views and `extern` declarations live in their owning
  headers.
- Each remaining `reinterpret_cast` states its reason within the ledger's
  window.

These are renames and moves, so the generated code is unchanged. Comments
next to an assertion must not move its `#line` source line.

**Helpers, accessors and macros.** A reconstruction transcribes the expanded
body of a helper the developers called. The common-code review reads each
function, records its candidate families with a verdict, and restores a helper
only where every affected object stays identical. Which forms survive VC6 is
recorded in [VC6 helper forms](patterns/vc6-helper-forms.md). Helpers whose
only purpose is to hide an enum-to-index or flag conversion are not introduced;
typed indexing belongs to the enum domains.

The combat and AI units (AI, ARMY, ARMYGRP, CMBTMGR, COMMAND, DRAWING,
FINDPATH, FLY, HEXCELL, PATH, PHILAI, SEARCH, SPELLAI, SPELLS, VIEW) are read
in full: [function checklist](common-code-combat-functions.tsv),
[candidate catalogue](common-code-combat.tsv).

The adventure, town, hero, network, Windows and BASE units (ADVMGR, CURSOR,
EVENTS, GAME, HERO, KB, kbwin, RECRUIT, REQUEST, SETUP, SMACKMGR, STRIP,
SWAPMGR, TOWN, TOWNMGR, HISCORE, netwin, REMOTE, comwin, wingraph and every
C++ BASE unit) are read in full: [function checklist](common-code-adventure-functions.tsv),
[candidate catalogue](common-code-adventure.tsv). Compiler-generated bodies,
the assembly units (BITS, BMAP2, Icon2b, Icon2bc, TILE) and the vendored LZHUF
code are outside the checklist. Declared default arguments (`NormalDialog`,
`TransmitRemoteData`) count as recovered source conveniences: they shorten calls
without changing code.

**Unions and varargs.** Alternate views and manual argument access are kept only
where retail evidence requires them. Eight unions remain; the other two `rg`
hits are comments in `ARMY.cpp` and `EVENTS.h`. Each one gives two or more readers of the same storage
their own types:

| Union | Readers |
| --- | --- |
| `tag_message`, three anonymous words | Each message type reads the same word under its own name and type (command or key code or x, id or y, value or text). Retail reads every word directly off the message, so a named payload level would change the operand order. |
| `icon` resource data | Raw bytes, the `IconEntry` directory and Buka's font word reads. |
| `searchNode` tail | The adventure search reads adjacent-monster bytes, and the value search reads signed coordinates. |
| `tag_Node` payload | The serial payload at +0xa, and the NetBIOS session byte then payload at +0xb. |
| `RemotePayload` | The remote message payload layouts: text, the save transfer header and segments, and the relayed combat action. |
| `advManager::SendHeroTownData` buffer (`EVENTS.cpp`) | The single allocation is filled as the combat record, then as each hero fragment. |

`nb_sess` is the only `va_start` user. It is a standard variadic function:
the `REMOTE.cpp` callers pass between one and three trailing arguments
depending on the operation. Its leading unused `i32` keeps the `com_sess(i32, i32, ...)`
calling shape, because retail pushes a zero in front of the operation. No
function walks its arguments by address.

## Editor-only units

The scenario editor's own units (`src/EDITOR`: EDITOR, EDITMGR, CLEARMGR,
TERRMGR, EVENTMGR, OVERLAY, MAPOBJ, and `include/EDITOR`) were read in full
with the same passes as the game. Every editor function stays exact.

- Constants: `homm1 verify constants` reads these units with the editor's
  compile commands, so the one floor covers both programs. None of their
  1,270 starting literals is open: 32-bit flags are `b32` (8-bit `b8`), and
  `config/constants.tsv` keeps the flags only `i16` or `u8` storage carries
  (`b8` is signed, so a `u8` flag would widen differently), counters, unit
  steps, unread frame locals and AppCommand's window-procedure result.
  Duplicates moved into shared domains: the five
  managers' dispatch masks, the four tool-panel rectangles and sentinels, the
  terrain counts (`EDITOR_TERRAIN_COUNT`), the radar geometry, shade and
  viewport colour (the adventure screen's `AdventureRadarConstant`), the
  animation cycle, the event-bit trigger copies (`MAP_TRIGGER_EVENT |` the
  game's codes), the generator's site kinds (`ResourceType`) and the castle
  frames (`EDIT_CASTLE_FRAME`). The editor keeps its own domains for its
  ICN frames, pointers, help tables, object classes (`OverlayKind`), tile
  runs, clear layers, view colours and window controls
  ([ledger](../config/reviews/enum-reuse.tsv)).
- Naming: the [editor naming ledger](../config/reviews/naming-editor.tsv)
  records each rename and its evidence. Locals whose readable names would move
  the `/Od` frame keep their slots through 62 frame-slot aliases; globals keep
  their storage spellings.
- `goto`: **3**, all kept. Rewriting BlendTerrain's skip as `if (!skipFill)
  { ... }` drops it to 99.51%; duplicating the object tool's shared
  category tail drops overlayManager::Main to 97.78% and PickOverlay to
  97.68%.
- `static_cast`: **30** (from 38). The `double`-to-`i32` argument and store
  casts are gone. The rest are the `void*` map-extra record and tool-manager
  downcasts (9), `malloc` results (6), the map-code field's CP1251 character
  tests (13, as `REQUEST.cpp`'s) and PlaceTowns' float shares (2), whose
  parenthesized divisor is the [parenthesized-cast pattern](patterns/vc6-parenthesized-cast-operand.md).
- Unknown members and unread tails: `overlayManager::m_unused16ca` (no
  instruction of the editor image touches the offset) and the town and hero
  records' `unused14`/`unused19` blocks, which only whole-record copies cover.
  The map header lives in a 2000-byte character buffer that its six users
  view as `SMapHeader` (retail places it 4-byte aligned, which VC6 gives no
  record), and its last 636 bytes are unread; the linked image's other unread
  `.bss` placeholders are listed in [the editor's data debt](editor.md#data-debt).
- Dead declarations: none in the editor headers.
- Byte layouts: the map file's town and mine records are an `editMapRecord`,
  SaveMap and LoadMap name the header range with `offsetof`, the generator's
  cell grids go through `MAP_GRID_CELL` and the object footprints through
  `OVERLAY_FOOTPRINT_CELL`/`_BIT`.
- Helpers: the editor reuses `CELL_TERRAIN` (31 neighbour reads) and
  `SET_WIDGET_MESSAGE` (4); `EDIT_CASTLE_FRAME`/`EDIT_TOWN_FRAME`,
  `MAP_GRID_CELL`, `OVERLAY_FOOTPRINT_CELL`/`_BIT` and `OVERLAY_TERRAIN_BIT`
  are recovered at every site. `MAP_CELL_IN_BOUNDS` does not apply: the
  editor's bounds tests compare `> MAP_CELL_GRID_SIZE - 1`. The generator's
  `x ± 1` neighbour reads stay open-coded (the macro groups the offset and
  the functions drop to 95-96%), and the zoom-dependent cell sizes stay
  explicit, because the matching source spells them three ways.

## Resolved

Categories closed during the game cleanup. Each count is final; a regression
shows up in the measuring commands above.

- Review game-type `reinterpret_cast`: **10 sites** (from 36): two
  combat-transfer payload overlays, the 4-byte-aligned receive buffer's record
  view, the wire packet header and its two CRC byte walks, three palette
  channel views and one pointer assertion. The icon
  frame directory, the remote message queue (typed `RemoteMessage` records and
  payloads), the combat and save transfer buffers, the search occupancy flags,
  resource reads and pixel buffers are typed. 77 further casts are
  Win32 API boundaries, including the handle assertions and comparisons.
  Every remaining cast carries its reason (cast ledger OPEN = 0).
- Replace manual byte layouts with named types: the font reads
  `icon::m_frameWords` (**6 sites**). `widths[g * 6 + 2]` is retained because
  only an `i16` index yields retail's `imul 6`. No literal-index packet stores
  remain.
- Negative offsets: **0 sites** (no negative indexing, `this` arithmetic or
  container-of recovery).
- Verify-board text debt at **0**: magic case labels, unnamed domain
  compares, `.cpp`-local enums and views, `.cpp` extern declarations, C-style
  casts and unexplained casts.
- Review gotos: **204 statements**, all kept because retail's block layout
  requires them. Replacing them with `break`, `else if` or nothing breaks an
  exact match, because VC6 `/Od` emits a `jmp` for every `goto`; each site was
  also tried as `break` and `continue`, and none compiles identically
  ([classes](docs/reconstruction-debt.md)).
- Review dead locals: **116** never-read locals. Each of the 75 without
  an initializer was removed alone, and every removal changes its function's
  frame. The 41 with an initializer emit retail stores.
- Remove dead declarations: **109** declared methods and functions with no
  definition, call or object symbol in either image are gone (98 more in the
  combat headers); **2** remain in `combatManager.h`.
- Review `static_cast`: **245 lines** (from 410). Casts that only hid a
  wrong declared type are gone, including the `void*` Win32 handles, which are
  now `STRICT`, and so are the 168 that restated the conversion an assignment,
  initialization, argument, return or arithmetic operand already performs; the
  remaining classes are listed in the [debt notes](docs/reconstruction-debt.md).
- Review unions: **8 definitions**, each one shared storage with typed
  readers; varargs: **1 function** (`nb_sess`), standard `va_arg` with no
  argument-address walking.
- Enum-domain review ([ledger](config/reviews/enum-reuse.tsv),
  [notes](docs/enum-reuse.md)): **403** starting blocks reviewed (67
  canonical, 288 retained, 48 merged); **98** members merged into shared
  domains and **43** unused members retired; **168** cross-domain value
  collisions remain, each with a reviewed reason.
- Strict enum view (`/std:c++20`, `homm1 verify strict-view`, floor 0): every
  game and editor unit parses with typed domains, `H1_ENUM_ARRAY` indices and
  `b8`/`b32` boolean storage (about 470 declarations retyped); the gate runs
  in `homm1 build verify`.

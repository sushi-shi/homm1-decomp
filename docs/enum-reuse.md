# Enum and constant reuse review

`homm1 verify enum-reuse` evaluates every named integer constant of both
programs (`HEROES.EXE` and `EDITOR.EXE`) and groups them by value. Equal
numbers are review leads, not evidence that two domains are one type.

## What it reads

A key is one named constant:

- an enum member: every `H1_ENUM_*` block (`BEGIN`, `BEGIN_SPLIT`,
  `FLAGS_BEGIN`, `CONST_BEGIN`, `ID_BEGIN`) and every raw `enum` block under
  `include/` and `src/` (the macro machinery in `include/Domains.h` is
  skipped);
- an object-like `#define` whose body is an integer constant expression; and
- a `const` or `static const` integer (or enum-typed) variable with a constant
  initializer, at namespace, class or function scope. Function-local consts
  that hold a retail stack slot are keys like any other.

A lexical inventory finds every enum block and `#define` without
preprocessing. libclang then parses every unit of every image with that
image's compile database (`build/clangd` for the game, `build/editor/clangd`
for the editor): an editor-only unit is read as the editor, and a unit both
programs link is read once as the game and once as the editor (with
`HOMM1_EDITOR`), so `#ifdef HOMM1_EDITOR` code is evaluated. Enum members and
`const` values are evaluated by Clang; each macro is evaluated by an
enumerator appended after the unit's last line (`enum : __int64 { probe =
(NAME) }`), and a macro the unit `#undef`s before its end by its definition's
body. Probes that do not compile (string, type or storage-alias macros such as
the `#define gName storageName` spellings) are not constants.

The two views must cover each other. An enum member or a numeric `#define`
(a body with an integer literal) that no unit evaluates is fatal, as is an
evaluated enum member or macro the inventory does not know. Two exceptions
are not holes: a block under `#if H1_STRICT_DOMAINS` exists only for the
strict view the retail compiler never sees (reported as "strict-view only"),
and one macro name of one file is one key, so `H1_STRICT_DOMAINS 1` is
covered by its evaluated `0` alternative.

## Reports

The command writes derived reports to ignored `build/gen/`:

- `constant_values.tsv` and `constant_values.json`: the value map, every
  evaluated value with every key that has it. Each key carries its qualified
  name, category (`enum`, `macro`, `const`), domain (the enum, or the file's
  `<macros>`/`<const>` group, or a class's or function's `<const>`), file and
  line, the images that compile it, and its use contexts. Values held by two
  or more keys come first, ordered by how many distinct domains share them.
- `enum_reuse.tsv`: every key with its block kind, storage, expression, the
  images and the units that evaluated it.
- `enum_value_collisions.tsv`: keys grouped by value across domains, joined
  with bare function literals of the same value from `homm1 verify
  constants`.
- `enum_domain_pairs.tsv`: enum pairs with overlapping value sets.
- `enum_role_pairs.tsv`: enum pairs where at least two equal values also have
  equal member-name suffixes after each enum's common prefix. A search aid
  only.

```sh
homm1 verify enum-reuse                  # reports, then the ledger check
homm1 verify enum-reuse --by-value       # print the value map
homm1 verify enum-reuse --value 232      # one value (repeatable)
homm1 verify enum-reuse --duplicates     # values two or more domains declare
homm1 verify enum-reuse --json           # the selection as JSON
homm1 verify enum-reuse --extend-ledger  # append new domains as pending
```

Every key records its use contexts: the declaration identity (field,
parameter, comparison operand, switch subject, array, return) that receives
each reference to it (`scripts/homm1/verify/constant_context.py`). A macro's
uses are its expansion sites. `enum_value_collisions.tsv` lists contexts
shared by two domains of one value (`shared_named_contexts`) or by a key and
a bare literal of that value (`shared_literal_contexts`, read from
`build/gen/bare_constants.tsv`, so run `homm1 verify constants` first);
`enum_domain_pairs.tsv` ranks pairs by shared direct contexts before numeric
overlap. A shared destination is a lead for one domain; a transport that
carries several domains (the `tag_message::id` widget id, each window's own
controls) is not.

## Decisions

`config/reviews/enum-reuse.tsv` snapshots each starting domain (each enum
block, and each file's macro and const groups) with its evaluated
`name=value` members and one decision:

- `retain`: the enum keeps its domain; its values select a distinct quantity,
  table, operation, representation or state machine.
- `canonical`: this enum owns values that another reviewed block reuses.
- `reuse`: the members moved to the canonical enum; `member_reuse` maps every
  moved member to `source-enum::MEMBER`.
  A member that no code names any more maps to `-` (retired); the check
  requires its identifier to be absent from every file under `include/` and
  `src/`.
- `pending`: the producers, consumers and encodings still need review, or the
  reuse is decided but the source move has not landed. Pending rows keep the
  command nonzero.

Follow both value paths through their fields, callers and tables. Direct
transport of one quantity supports reuse (HoMM1's `combatManager::GetPointer`
returns its command as the pointer code). Two zero-based tables that only share
an order support retention (`MoraleInfoText` rows and `ArtifactType` medals).
`BaseManagerMessageMask` stays apart from `MessageType`, and per-window dialog
roles are spelled as aliases of the reserved slots (`FILE_REQUESTER_OK = DIALOG_BUTTON_2`), so role names stay with their
window while slot-named copies move to `DialogButtonId`. Constant groups
(`H1_ENUM_CONST`) are not value domains, but a member that repeats a domain's
quantity (the 640x480 logical screen, `RESOURCE_GOLD`, a widget command) is a
reuse lead like any other.

Every starting member needs a current home with the same value. New, removed
or changed members require a new decision: add the row when an enum is added.
Moving members changes C1 symbol numbering in the TU, so do source moves as a
reviewed batch and re-check edited functions with `homm1 match`.

## Buka ledger

The Buka branch inherited the NWC ledger. Its starting snapshot was rebased on
the Buka tree: a row whose current members and values are unchanged kept its
reviewed `retain`/`canonical` decision; every other current block started
`pending` (with the NWC reason quoted as a lead), and NWC rows for blocks that
do not exist in Buka were dropped.

## Review result

All 403 starting blocks are reviewed: 67 `canonical`, 288 `retain`, 48
`reuse`. 98 members moved into a canonical domain and 43 members no Buka
code names were retired; the census now holds 397 blocks, 2,872 members and
168 cross-domain collision values, each covered by a reviewed row. The merges
follow shared producers and consumers:

| Canonical owner | Merged copies | Shared use |
| --- | --- | --- |
| `KB.h` `TimerSlot` | eleven per-owner glTimers slot constants | every one indexes `glTimers`, now `H1_ENUM_ARRAY(i32, glTimers, TimerSlot, ...)` |
| `display.h` `LogicalScreenConstant` | miscwin, wingraph, bitmap, icon, inputManager, resourceManager and kbwin 640x480 copies | m_screen size/stride, DirectDraw mode, full-screen regions |
| `KB.h` `DebugLevel` | Misc, philAI, fileRequester and kbwin levels | every value is stored to or compared with `giDebugLevel` |
| `palette.h` / `display.h` palette sizes | the 768-byte, 256-entry and 3-byte copies | `palette::m_data` copies and the LOGPALETTE |
| `dialog.h` `DialogButtonId` | NormalDialog, setup and panel slot literals | `m_dialogResult` and the window records |
| `cursorTypes.h` `MapDirectionMask` | cursor and path-search direction masks | the same north/south object test |
| `advManager.h` view geometry | quick-view, summon-boat, combo-draw and update copies of the inner map box; the radar corner of the world and puzzle windows; the view centre of CURSOR and EVENTS | clamps and regions of the one adventure view |
| `REMOTE.h` | combat hand-off commands, three reply timeouts | `RemoteMessage::command` and the same wait loop |
| `soundmgr.h` `ConfigVolumeLevel` | volume OFF/FIRST/LAST | `gConfig.musicVolume/soundVolume` |
| `hero.h` `HERO_ID_NONE`, `gameTypes.h` `GAME_PLAYER_NONE` | GAME_HERO_NONE, INVALID_HERO, HERO_OWNER_NONE | `m_currentHero`, `m_owner` |
| `baseManager.h` `BaseManagerMessageMask` | mouse, town and combat manager masks | `baseManager::m_messageMask` |
| `inputManager.h` `InputScanCode` | textEntryWidget's key switch | the scan code before `AsciiConvert` |
| smaller owners | sample volume, music tracks, keep-current-frame, boat flag, map grid, Dragon City row, last filename size | one field or argument each |

Per-window control ids, help-text rows and screen geometry that only share
numbers through the `tag_message::id` transport or by coincidence are
retained with that reason.


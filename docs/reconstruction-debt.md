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
- network packets: `char` buffers viewed as `RemoteMessage`,
  `combatRemoteMessage`, `heroRemoteMessage` and fragment records.
  `SendHeroTownData` uses one allocation for the combat record and both hero
  fragments. It is now a local union of the three typed pointers (combat
  record, hero fragment, wire bytes), so the frame slot is unchanged.
  `ReceiveHeroTownData` and `DoCombat` read received records through
  `EVENTS_REMOTE_MESSAGE`/`EVENTS_REMOTE_HERO`, the Buka 2.1 donor's view
  macros. The remaining nine casts convert the `char*` that `GetRemoteData`,
  `CheckHandleNet` and the transmit functions use for queue records; the
  donor-derived symbols (`?GetRemoteData@@YIPADC@Z`) fix that type, and the
  donor casts at the same call sites. A union cannot hold the combat payload
  because `armyGroup`/`town` members have constructors. `PacketSend` keeps its
  `char[]` data identity. `TransmitSaveGame` builds its packets in a named
  `RemotePayload`, the `RemoteMessage` payload union;
- remaining byte, word and integer views.

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

Names come from a code user or the Buka 2.1 donor:

- mouseManager's `m_savedLeft`/`m_savedTop` are the cursor area ComboDraw
  marks; Buka never updates them after the constructor.
- playerData's `m_unusedSaveData` is the span Write zeroes and Read skips.

Spans that no code reads or writes take the donor's spelling for such
members: `m_unused<offset>`, or `m_padding<offset>` in recruitUnit. Nineteen
placeholders remain:

- inputManager (seven): written by its constructor, the event queue, the two
  option setters and advManager's context changes; nothing reads them.
- army (two), combatManager (four), heroWindowManager (two), town (one) and
  mouseManager (one): set only by constructors or Init.
- playerData `m_unknown00`/`m_unknown99`: only copied raw by Write/Read. Like
  HoMM2's `m_barrierTents`, `m_unknown99[1]` is written twice.

The HoMM2 donor leaves the corresponding fields unnamed as well
(`field_0x742`, `m_unknownF373`, ...). Inventing a meaning for them is not
evidence, so they stay placeholders until a reader is found.

**`goto`.** The HoMM2 Buka donor contains several hundred, so the original code
used them. A `goto` stays when retail's block layout requires it; it is
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

Every goto is kept because retail's block layout requires it.

**Dead locals.** Every never-referenced local must correspond to an
unreferenced slot in retail's `/Od` frame (a hole between referenced slots or a
larger frame). Names chosen only to fill frames are reviewed against donor
spellings.

The audit lists locals with `clang-cl /Zs -Wunused-variable` after replacing
each `#line` with an empty line, so diagnostics keep the file's own numbering.
It found 116 such locals. The 41 with an initializer emit retail stores. A
control removed the 75 initializer-free declarations together: every function
that contained one lost its exact frame, and the others were unchanged. Under
`/Od`, VC6 gives each declared local its own slot, so each of these maps to an
unread slot in retail's frame.

**`static_cast`.** Narrowing and signedness conversions are often required for
retail's widths; the review removes the ones that only paper over a wrong
declared type. A libclang scan compared each cast's operand type with its target
type. The review removed 108 lines of casts, and each removal kept all 1057
bodies exact:

- Win32 handles. The menu, instance, window and DC handles were declared
  `void*`, so every API call cast them back. All units now build with
  `/DNO_STRICT` (`config/units.toml`), which makes VC6's handles `void*` as in
  the HoMM2 Buka lineage. The owners are typed `HMENU`, `HINSTANCE`, `HWND`,
  `HDC` and `HANDLE`, the HoMM2 donor's spellings, and the 61 casts are gone.
  Mangled names keep the `PAX` handles the claims already used. The retail
  data identities that had recorded the STRICT spelling (`hwndApp`, `hpalApp`,
  `hdcImage`, the mouse cursor and bitmap tables) were renamed to match.
- Casts to the operand's own type: `u8` map-cell payloads, `u8` hit points and
  a `float` difference.
- `CONST` enum values converted to `int` or a narrower integer. These enums are
  unscoped in both views, so the cast does nothing.

The remaining casts are:

- Float-to-integer conversions. The HoMM2 donor spells these the same way.
- `void*` results of `malloc`, `GlobalAlloc` and the resource cache. C++
  requires these casts.
- Narrowing stores and `i8` ternary arms whose byte width retail shows.
- `char` to `u8` code-page comparisons.
- Casts of strict-domain values, which are `enum class` in the Clang view.

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

**Unions and varargs.** Alternate views and manual argument access are kept only
where retail evidence requires them.

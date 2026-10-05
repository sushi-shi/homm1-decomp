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
rg -w goto src include
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
  `char[]` data identity;
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

**`goto`.** The HoMM2 Buka donor contains several hundred, so the original code
used them. A `goto` stays when retail's block layout requires it; it is
replaced only when a structured form compiles to identical bytes.

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
declared type.

**Unions and varargs.** Alternate views and manual argument access are kept only
where retail evidence requires them.

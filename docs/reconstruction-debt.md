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
rg '\bunused[A-Z0-9][A-Za-z0-9_]*\b\s*(\[|;|=)' src  # dead locals
rg '\bm_(unknown|unk|pad|field)[A-Za-z0-9_]*' include
rg '\[\s*-\s*[0-9]' src include                    # negative indexing
rg 'CONTAINING_RECORD|container_of|\bthis\)\s*-' src include
rg 'va_start' src
```

## Categories

**Game-type `reinterpret_cast`.** Casts to Win32 handle, procedure and buffer
types at API boundaries are expected. Game-type casts mark data whose type is
not yet modelled:

- the icon frame directory: `IconEntry*` views of `icon::m_data` resource bytes;
- network packets: `char` buffers viewed as `RemoteMessage`,
  `combatRemoteMessage`, `heroRemoteMessage` and fragment records;
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

**`static_cast`.** Narrowing and signedness conversions are often required for
retail's widths; the review removes the ones that only paper over a wrong
declared type.

**Unions and varargs.** Alternate views and manual argument access are kept only
where retail evidence requires them.

# WGInitializePalette (retail RVA 0x4faf)

Buka 2.1 `src/SOURCE/wingraph.cpp` at `VA(0x004b1434, 0x1a2)` supplies the
palette initialization source and its two locals (`dc0`, `entry0` in the
retail-compiler naming branch). HoMM1 retail at RVA 0x4faf confirms the
`hpalApp` early return, two `GetSystemPaletteEntries` calls for [0, 10) and
[246, 256), `ReleaseDC`, the two palette-copy loops, and `CreatePalette`.
The pinned bytes read `LogicalPalette` at VA 0x48e198 and `screenImage.colors`
at VA 0x4a4768. Those DATA identities and layouts are documented in
`wingraph-paint.md`; this is code-use evidence, not initializer coverage.

The retail interior-color loop stores zero to each `LogicalPalette` channel,
reloads the channel, then stores that value to `screenImage.colors`. Chained
assignment expresses this source-level behavior under VC4. The candidate is
0x1d5 bytes with 116 instructions, five calls, nine branches, and 37
relocations, equal to retail; code-mode score is 99.6542%. The remaining
register/stack allocation difference begins at +0x25: retail puts `dc0` at
`ebp-4` and `entry0` at `ebp-8`, while this candidate reverses those slots.

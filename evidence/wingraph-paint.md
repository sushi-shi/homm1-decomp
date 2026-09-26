# WinG paint code-use identities

The pinned retail `WGAppPaint` at RVA `0x5184` compares the pointer at VA
`0x004A4B68` with zero, then paints through WinG. HoMM2 Buka 2.1 has the same
`WGAppPaint` control flow and identifies that pointer as `screenImage.bits`.
Its `_IMAGE` record consists of a 40-byte `BITMAPINFOHEADER`, 256 four-byte
`RGBQUAD` entries, and the four-byte bits pointer. This places the record at
retail VA `0x004A4740` and the pointer at offset `0x428`, exactly as read by
retail. The local `WingImage` definition preserves that ordinary record layout.

The retail paint body reads `giScrollX` at VA `0x00492E08` and `giScrollY` at
`0x00492E0C`, then increments `giTtlBlts` at `0x0048E5A4`. The Buka body uses
those same names, types, and operations. The direct calls at RVAs `0x52D1`
and `0x5322` target the `WING32.dll` import thunks identified in
`evidence/homm1-dna-bands.tsv` as `WinGBitBlt` and `WinGStretchBlt`.

These admitted DATA rows are code-required identities and sizes only. They do
not assert initializer or data-byte coverage.

At RVA `0x4DD2`, the pinned palette body addresses `LogicalPalette` at VA
`0x0048E198` with a two-word header and 256 four-byte entries (size `0x404`).
The loop writes red, green and blue bytes into its entries and the matching
`screenImage.colors` RGBQUAD entries. It compares the dword video depth at
`0x0048E184` with 8, then reads combat repaint flags at `0x00494134` and
`0x00494130`. Buka 2.1 supplies these names and record types; the pinned
retail accesses and branch order confirm them. HoMM1 omits Buka's extra
`gpWindowManager->m_screen != NULL` guard, so the source follows retail.

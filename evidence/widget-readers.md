# HoMM1 widget resource readers

The seven previously unclaimed `Read` bodies called by `heroWindow::Open` at
RVA `0x74B30` are `border` `0x79850`, `iconWidget` `0x7AAD0`, `textWidget`
`0x7AED0`, `backdropWidget` `0x7D070`, `textEntryWidget` `0x7E170`,
`dimmerWidget` `0x7EE70`, and `button` `0x7EF70`. Each starts by reading four
16-bit geometry words into inherited widget offsets `+0x18` through `+0x1E`.
Backdrop and dimmer then read ID and kind into `+0x10` and `+0x14`. Icon and
button read a 13-byte resource name, save the aggregate position, load an
icon by name, restore the position, and read their class-specific fields.
Text reads its length and bytes, loads a font with the same save/restore
pattern, reads color and alignment, then fixes its kind to `0x200`. All six
patterns correspond to Buka 2.1's reader family and match the retail code
exactly.

HoMM1 border's `Read` body at `0x79850` has only a bitmap branch when kind
is `0x800`; unlike Buka 2.1, there is no icon branch. On the fill branch,
retail calls `ReadWord` before clearing the background pointer, then masks
the returned color to eight bits. Keeping that observed evaluation order
matches the final six bytes exactly.

The `textEntryWidget` allocation in `heroWindow::Open` is `0x45` bytes. Its
reader confirms icon at `+0x2B`, icon frame at `+0x2F`, cursor position at
`+0x31`, maximum text length at `+0x33`, rectangle at `+0x35` through
`+0x3B`, line count and focus-preservation fields at `+0x3D` and `+0x3F`,
read mode at `+0x41`, and display offset at `+0x43`. This corrects the older
donor-derived header layout. The reader allocates `maxLength + 5`, reads
text, font and icon resources, sets rectangle fields from the stream for
mode 2 or from widget geometry otherwise, then reads icon frame and ID and
fixes widget kind to `0x4000`. It matches exactly. The coarse retail census
span starting at `0x7E170` also contained separate `Main` and `Draw` entries
at `0x7E360` and `0x7EB60`; their independent prologues and the `Read`
`ret 4` at `0x7E355` justify the two new function boundaries. No code claim
or data initializer was made for those other entries.

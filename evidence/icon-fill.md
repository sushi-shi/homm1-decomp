# Mono-icon fill ABI and extent checks

The anonymous retail method at RVA `0x79e60` is source-correspondent to Buka 2.1 `icon::FillToBuffer`. HoMM1 uses its own four word extent members, not the donor's later supplied limit struct. It optionally clips against global army extents, then calls MonoIconToBitmap (`0x79422`) or FlipMonoIconToBitmap (`0x794f8`) according to the orientation byte.

All callers and the callee use six stack slots: four signed word arguments x/y/frame/color and two signed byte arguments orientation/mode. Retail callee word-loads slots 1–4, byte-loads slots 5–6, and returns with `ret 0x18`. Font DrawString (`0x7b3f0`) pushes a word glyph index and three other arguments plus zero orientation/mode. The corrected source signature replaces the later donor's int coordinates and SLimitData pointer.

The mono renderers consume seven cdecl arguments (icon, screen bitmap, x, y, frame, mapped color, mode). FillToBuffer pushes exactly seven slots and cleans 0x1c bytes after either call. The existing renderer ASM bodies consume those same slots; their eleven-argument donor export names were therefore corrected to the seven-argument identities. No renderer instructions were altered.

The extent globals are direct dword operands in FillToBuffer: limit flag `0x94158`, current-army flag `0x9415c`, maximum X/Y `0xc5148`/`0xc514c`, minimum X/Y `0xc6738`/`0xc673c`. These names correspond to the donor's extent check. The mapped color is a byte read from `0x92d00 + color`; an unsized unsigned-byte table declaration models that code identity. Only its accessed base identity is admitted (one-byte manifest row), not a guessed bound or initializer.

The glyph directory uses twelve-byte entries, x at offset 0, y at offset 2, width at offset 4 and height at offset 6, independently confirming the shared IconEntry type used by font measurement. HoMM1 uses exclusive right/bottom extents without the donor's later minus-one adjustment.

## Neighboring font line count

RVA `0x7b7a0` is LineLength, ending at `0x7b9b1` before fifteen padding bytes. The ordinary donor loop survives but HoMM1 uses signed words for length, positions and accumulated width, a signed glyph byte, and twelve-byte glyph directory entries instead of the later GetCharacterWidth helper. Its sole dword counter is returned. Retail also retains the older shared text layout census: initialized blank byte and unused word t, plus a saved byte around a line-end NUL/restore pair. These stores are actual instructions, not invented compiler padding. Width is a signed word argument, proved by its word loads; callee pops eight bytes.

The Buka LineLength/DrawBoundedString family and PoL2.0 LineLength establish correspondence; HoMM1 bytes supply the earlier direct loop. PoL2.0 names s/q/u/aa/z/t/r/y/p/x/w/v explain the current local census. The authored body has exactly the retail 147 instructions, 17 branches and no calls; remaining mismatch is local-slot allocation. No arbitrary local-name or storage campaign was retained.

## Bounded drawing

RVA `0x7b4d0`, length `0x2d0`, shares the direct line-break loop and temporary NUL with LineLength; it stops when font height plus the current vertical offset exceeds the supplied height. It selects left/center/right alignment 0/1/2, calls DrawString on the current line-start pointer, restores the saved byte, and advances by font height. The later donors' vertical centering, final-line exception, overlong-word fallback and ExtractLine calls are absent from this retail body and were not copied.

All six scalar parameters are signed words. Color is copied with word instructions into a local and that local is pushed as an unextended dword to DrawString. This proves DrawString's color parameter is also a word (VC4 does not sign-extend a word argument passed to a word parameter). DrawString's own raw dword parameter push to FillToBuffer, whose color is independently word-proven, agrees. The former int-color declaration could not distinguish these when measuring DrawString alone; the new real caller resolves its export identity. Left/center/right values come from both donors and the retail switch.

## Dim icon extent family

RVA `0x79fa0` ends at `0x7a162`, before fourteen padding bytes. It is DimToBuffer with three signed-word coordinates/frame and two signed-byte orientation/mode arguments (ret 0x14). Buka/PoL retain the same renderer selection, and their combat clipping method retains the mirrored extent arithmetic now residing in HoMM1's icon owner. Retail proves the quarter-offset mode by word arithmetic right shifts of the frame x offset by two. Left/right and top/bottom are exclusive member extents.

Dword flags at `0x94150` and `0x94154` control computation and collection of these extents. The former corresponds to donor gbComputeExtent; the latter unions the member extents into the existing four global min/max values. The descriptive gbSaveBiggestExtent name corresponds to both donors. Neither flag receives invented storage or initialization.

DimToBuffer calls DimIconToBitmap (`0x795cc`) or FlipDimIconToBitmap (`0x796a6`) with six cdecl arguments and cleans 0x18. Both authored ASM bodies independently consume six slots. Their labels and fixed-ASM target facts therefore replace the donor eleven-argument identities; instructions remain untouched. No data initializer claim is made.

## Normal drawing and clipped renderer selection

DrawToBuffer at `0x79bd0` ends at `0x79dfa`, excluding six padding bytes. It uses the same three-word/two-byte ABI, computation/collection flags, mirrored extents and army clipping checks as DimToBuffer. Byte `0xc74a8` selects a separate normal/flipped renderer pair at `0x7ca44`/`0x7cca4` instead of IconToBitmap/FlipIconToBitmap (`0x79280`/`0x79356`). All four calls push six cdecl slots and clean 0x18. The existing normal/flip ASM instructions consume those slots, so only their export labels and fixed target names change to the six-argument identities.

The anonymous alternate pair clips RLE drawing against the bitmap dimensions: negative x/y produce run/line skipping, and bitmap width/height limit writes. The literal-run loop copies full runs with rep movsd/movsb; these are not shrinking renderers. ClippedIconToBitmap/FlipClippedIconToBitmap and gbUseClippedIconRenderer are descriptive retail-backed identities, not assertions of original names. The two routines remain unclaimed, identity-only referents without supplied dummy definitions. The mode byte independently quarters the glyph x origin in the extent calculations, exactly as word sar-by-two instructions show.

## Mono clipping wrapper

ClipFillToBuffer at `0x79e00` ends at `0x79e51`, excluding fifteen padding bytes. It takes ten method stack slots: x/y/frame/color signed words, unused orientation byte, mode byte, and four dword clip rectangle arguments. It maps color through the same table and calls the mono clipping kernel at `0x738e0` with eleven cdecl arguments (icon, bitmap, x, y, frame, mapped color, mode, clip x/y/w/h), cleaning 0x2c before ret 0x28. The method corresponds to both donors' ClipFillToBuffer, whose later implementation unified the renderer entry points. ClippedMonoIconToBitmap is a descriptive identity for the independently identified HoMM1 kernel; no original spelling or body coverage is claimed yet.

## Authored mono clipping kernel

The C++ kernel at `0x738e0` ends at `0x73ac6`, before ten padding bytes. Buka/PoL's MonoIconToBitmap supplies source correspondence for the RLE protocol and four clipping fill cases. HoMM1 uses actual locals rather than the donor file statics, always checks the supplied clipping rectangle, and uses a fixed 640-byte screen stride rather than destination width. The signed-byte command has three cases: negative skip masked by 0x7f (zero terminates), positive opaque run, and zero newline. Newline resets x from the frame origin and advances y and the row offset. The mode slot is unused in the retail kernel.

The left intersection test is inclusive (`position + run >= clipX`), independently proved by the retail jl rejection, while the later donor uses strict greater-than. Four rep stosd/stosb fill sites prove intrinsic memset lowering; source uses the pinned optimized ICON profile with a local intrinsic declaration, and no hand-written fill bytecode. No donor static storage or initializer was imported. Clipped-left length is the ordinary position+run-clipX expression; a diagnostic equivalent sequential arithmetic spelling emitted identical bytes and was restored.

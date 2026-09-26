# WGInitGraphics (retail RVA 0x4c81)

Buka 2.1 `SOURCE/wingraph.cpp` supplies the WinG DIB setup, one `HBITMAP`
local, and the call sequence. HoMM1's pinned 0x151-byte body confirms the
early `hdcImage` guard, recommended-format branch, 640 by negative-480 DIB,
palette initialization, WinG DC/bitmap creation, selected old bitmap, and
final `PatBlt`. All 83 instructions, six calls, five branches, and 40
relocations match exactly in code mode after naming the three WING32.dll
import thunks at RVAs 0x801ca, 0x801d0, and 0x801d6.

Retail reads and writes `Orientation` at VA 0x48e190 and `lpInitWin` at
VA 0x48e59c. These are code-use DATA identities with four-byte storage;
their initializer bytes remain unclaimed. Retail loads `biWidth` before
`biHeight` when multiplying the dimensions; candidate VC4 loads them in the
opposite order for either source spelling. This is a same-object, commutative
operand swap. Code-mode bytes normalize identically. The identity gate now compares the
complete address multiset for a proven within-function permutation, including
member addends, so this swap does not invent two object bases. Negative
controls retain failures for wrong, duplicated, or cross-function members.

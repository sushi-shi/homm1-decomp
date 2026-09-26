# Mouse cursor selection, teardown and delay helpers

Retail SetPointer at RVA 0x768F0 reads the first character of the resource
name. A/a selects adventure cursor family 0, S/s selects spell family 2,
and other characters select combat family 1. It forwards the frame DWORD
home to the mapped short-frame overload at 0x76940 and returns with RET 8.
The body ends at 0x76934 (0x45 bytes); eleven following INT3 bytes are padding.
This disproves the three-argument HoMM2 declaration. The two-argument source
keeps donor int for the forwarding frame because this retail wrapper does
not constrain its width further. combatManager::Open pushes frame 6 at
0x4BC5E and the name at 0x4BC60 before calling at 0x4BC6B. SetupTown pushes
frame 0 at 0x884D and the name at 0x884F before calling at 0x885A.

Close at RVA 0x767E0 releases the saved bitmap, restores the default Windows
cursor, waits fifty milliseconds, destroys and clears each of seventy-five
cursor handles, AND bits, color bits, AND-mask bitmaps and color bitmaps, then
waits another fifty milliseconds. Both bitmap-handle arrays are present in
retail; Buka only clears its AND-mask array. The function ends at 0x768D1
(0xF2 bytes), before fourteen padding bytes and the separate short-return
method at 0x768E0. Its complete source has 72 instructions, ten calls, eight
branches and eighteen relocations, equal to retail. The residual is a cursor
index / cached DestroyIcon register swap, ESI/EDI against retail EDI/ESI.
Loop scope, initialization lifetime and Buka early-return controls were flat;
a thirty-two-state forest/enum/struct/prototype control found one compiler
island. No disposable source was retained.

The two teardown waits call NOOPT DelayMilli at RVA 0x644C4. Buka NOOPT
supplies the complete three-helper family. DelayMilli adds KBTickCount to
its long delay then calls DelayTilMilli. DelayTilMilli at 0x644E7 processes
one Windows message and polls sound while the signed target time exceeds
the tick count. Its body ends at 0x64513 (0x2D bytes), followed by twelve
padding bytes. DelayTil at 0x64470 re-reads an int target through a pointer
and has a HoMM1 assertion that the target exceeds 10000, absent in Buka.
Its assertion reads a signed short line at 0xA0800, adds one and uses the
source-file string beginning 0xA0804. These are code-required identities
without initializer or data-byte coverage. DelayTil and DelayMilli have
0x54- and 0x23-byte bodies without trailing padding. All three helpers match
the existing cpp_carcass profile exactly.

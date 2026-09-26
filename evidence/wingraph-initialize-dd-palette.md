# DDInitializePalette (retail RVA 0x3f68)

Buka 2.1 and PoL 2.0 SOURCE/wingraph.cpp supply the three locals, busy guard,
system-reserved color bands, mutable-entry initialization, DirectDraw
CreatePalette and final SetPalette call. Retail retains the same behavior,
but its DDSD error arm adds 63 to the signed word at VA 0x48e800 and uses
the HoMM1 source filename. The word identity is code-use evidence only.

The candidate and retail both have 0x140 bytes, 91 instructions, seven calls,
ten branches, and 21 relocations. Code-mode score is 99.92%; the remaining
compiler allocation difference starts at +0x25. The real SDK DDPCAPS_8BIT
flag is 4; IDirectDraw::CreatePalette is interface slot 5. No new interface
storage or initializer coverage is claimed.

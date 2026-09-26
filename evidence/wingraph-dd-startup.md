# DirectDraw startup family

Existing mapped empty bodies reconstructed within the graphics-family lane: CreatePrimary RVA0x36df size0x9b baseline24.11%; SetupClipper0x377a size0xeb baseline15.18%; DDInitGraphics0x3865 size0x171 baseline10.51%. All three100% in the actual intended TU on first compile.

Preferred Buka SOURCE/wingraph.cpp60-153 and PoL61-129 preserve complete bodies. The only release adaptation is HoMM1 retail filename and signed word-based diagnostic lines. HoMM1 wordsVA0x48e5e8(CreatePrimary,+10),0x48e60c(SetupClipper,+8/+13/+18),0x48e670(DDInit,+8/+20/+24/+31) are independently observed by movsx and addition in retail. Admit identity/layout only, no data initializer coverage.

Creation calls existing DDCreateSurface with640x480 and primary1. Clip setup uses real config.gfx[giCurExe].fullScreen(+0x2c, stride24), COM CreateClipper, SetHWnd, SetClipper. Init calls real loaded DirectDrawCreate pointer, selects normal or exclusive/fullscreen/allowreboot mode19, sets640x480x8, creates both surfaces, and initializes palette. SDK cooperative bits are original DDRAW.H2588-2609. Calls use existing strict identities; no speculative callee bodies added. SetMenuStatus(int) receives only a declaration in its KB owner header.

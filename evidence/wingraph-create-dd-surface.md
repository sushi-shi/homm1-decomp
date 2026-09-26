# DirectDraw surface creation

RVA0x415b, size0x12a, existing empty mapped body baseline12.88%. Genuine body preserved by Buka SOURCE/wingraph.cpp378-421 and PoL289-322. Donors retain descriptor, surface pointer, unused cnt/unused locals, HRESULT rv; Buka explicitly aliases count->cnt and result->rv. The unused locals are donor census, not manufactured frame padding. Both donors show the same branch logic.

DDSURFACEDESC is the real108-byte Microsoft SDK record, including actual DDPIXELFORMAT unions, DDCOLORKEY and DDSCAPS records. Transcribed Buka build/toolchain/msvc/include/DDRAW.H171-178,240-244,387-423,1178-1205. Retail memset size108 and member loads/stores validate descriptor. Original flag values come from same SDK. Lock returns lpSurface+36; the resulting pointer is assigned to bitmap::m_pixels(+0x14) through heroWindowManager::m_screen(+0x42) and lpInitWin, with real existing owner layouts.

Signed diagnostic wordVA0x48e848, +28/+36, independently observed in retail. Identity/layout admitted only; no initializer coverage. Root already identifies retail0x80820 as exact VC4 _memset in functions_static_libs.tsv; mirrored that existing root row privately for strict call identity, excluded from patch.

Actual intended source99.70%,298bytes/88instructions/5calls/8branches/13relocations agree. First difference+0xf is local descriptor allocation; ordinary PoL description/surface and preferred Buka ddsd/lpSurface spellings both measured99.65% before restoring the existing root memset identity; preferred spelling then measured99.70% with correct referents and is retained. Residue remains open, no artificial source or tooling exceptions.

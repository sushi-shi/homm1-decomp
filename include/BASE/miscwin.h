#ifndef HOMM1_BASE_MISCWIN_H
#define HOMM1_BASE_MISCWIN_H

#include <Domains.h>

// miscwin.cpp: Windows-side screen, palette and clipped-icon helpers.
void ClippedMonoIconToBitmap(
    class icon*,
    class bitmap*,
    i32,
    i32,
    i32,
    i32,
    i32,
    i32,
    i32,
    i32,
    i32
);
void ClipIconToBitmap(class icon*, class bitmap*, i32, i32, i32, i32, i32, i32, i32, i32);
i32 Random(i32, i32);
void FadeIn(i32);
void FadeOut(i32);
void PrintMemoryLeaks(void);
void PostprocessPalette(i8*);
void BlitBitmapToScreen(class bitmap*, i32, i32, i32, i32, i32, i32);
void PostprocessBitmap(i8*, i32, i32);
void GrabScreenBitmap(class bitmap*, i32, i32);
void PostprocessIcon(class icon*);

H1_ENUM_CONST_BEGIN(ScreenBlitConstant)
    SCREEN_BLIT_WIDTH = 640,
    SCREEN_BLIT_HEIGHT = 480,
    SCREEN_BLIT_WIDTH_END = 640,
    SCREEN_BLIT_ENLARGE_END = 637,
    SCREEN_BLIT_ENLARGE_PIXELS = 4
H1_ENUM_CONST_END(ScreenBlitConstant)

#endif // HOMM1_BASE_MISCWIN_H

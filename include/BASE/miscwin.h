#ifndef HOMM1_BASE_MISCWIN_H
#define HOMM1_BASE_MISCWIN_H

#include <Domains.h>

// miscwin.cpp: Windows-side screen, palette and clipped-icon helpers.
void ClippedMonoIconToBitmap(
    class icon* sourceIcon,
    class bitmap* destination,
    i32 x,
    i32 y,
    i32 frame,
    i32 color,
    i32 mode,
    i32 clipX,
    i32 clipY,
    i32 clipW,
    i32 clipH
);
void ClipIconToBitmap(
    class icon* sourceIcon,
    class bitmap* destination,
    i32 x,
    i32 y,
    i32 frame,
    i32 mode,
    i32 clipX,
    i32 clipY,
    i32 clipW,
    i32 clipH
);
i32 Random(i32 low, i32 high);
void FadeIn(i32 increment);
void FadeOut(i32 increment);
void PrintMemoryLeaks(void);
void PostprocessPalette(i8* data);
void BlitBitmapToScreen(
    class bitmap* sourceBitmap,
    i32 sourceX,
    i32 sourceY,
    i32 width,
    i32 height,
    i32 destinationX,
    i32 destinationY
);
void PostprocessBitmap(i8*, i32, i32);
void GrabScreenBitmap(class bitmap* destination, i32 x, i32 y);
void PostprocessIcon(class icon*);

H1_ENUM_CONST_BEGIN(ScreenBlitConstant)
    SCREEN_BLIT_WIDTH = 640,
    SCREEN_BLIT_HEIGHT = 480,
    SCREEN_BLIT_WIDTH_END = 640,
    SCREEN_BLIT_ENLARGE_END = 637,
    SCREEN_BLIT_ENLARGE_PIXELS = 4
H1_ENUM_CONST_END(ScreenBlitConstant)

#endif // HOMM1_BASE_MISCWIN_H

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

// Outcomes of a ten-way roll, Random(0, 9) or Random(0, 99) % 10, as the
// event setup switches share them out in tenths.
H1_ENUM_CONST_BEGIN(RandomDecile)
    RANDOM_DECILE_0 = 0,
    RANDOM_DECILE_1 = 1,
    RANDOM_DECILE_2 = 2,
    RANDOM_DECILE_3 = 3,
    RANDOM_DECILE_4 = 4,
    RANDOM_DECILE_5 = 5,
    RANDOM_DECILE_6 = 6,
    RANDOM_DECILE_7 = 7,
    RANDOM_DECILE_8 = 8,
    RANDOM_DECILE_9 = 9
H1_ENUM_CONST_END(RandomDecile)
void FadeIn(i32 increment) throw();
void FadeOut(i32 increment) throw();
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
void PostprocessBitmap(u8*, i32, i32);
void GrabScreenBitmap(class bitmap* destination, i32 x, i32 y);
void BitmapToScreen(class bitmap* image);
i16 AutoInitSVGA(void);
void PostprocessIcon(class icon*);

H1_ENUM_CONST_BEGIN(ScreenBlitConstant)
    SCREEN_BLIT_WIDTH = 640,
    SCREEN_BLIT_HEIGHT = 480,
    SCREEN_BLIT_WIDTH_END = 640,
    SCREEN_BLIT_ENLARGE_END = 637,
    SCREEN_BLIT_ENLARGE_PIXELS = 4
H1_ENUM_CONST_END(ScreenBlitConstant)

struct PaletteColor {
    u8 red;
    u8 green;
    u8 blue;
};

#endif // HOMM1_BASE_MISCWIN_H

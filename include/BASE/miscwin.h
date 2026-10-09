#ifndef HOMM1_BASE_MISCWIN_H
#define HOMM1_BASE_MISCWIN_H

#include <BASE/display.h>
#include <BASE/icon.h>

void ClippedMonoIconToBitmap(
    class icon* sourceIcon,
    class bitmap* destination,
    i32 x,
    i32 y,
    i32 frame,
    i32 color,
    i32 offsetMode,
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
    i32 offsetMode,
    i32 clipX,
    i32 clipY,
    i32 clipW,
    i32 clipH
);
i32 Random(i32 low, i32 high);

enum RandomDecile {
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
};
void FadeIn(i32 increment) throw();
void FadeOut(i32 increment) throw();
void PostprocessPalette(i8* paletteData);
void BlitBitmapToScreen(
    class bitmap* sourceBitmap,
    i32 sourceX,
    i32 sourceY,
    i32 width,
    i32 height,
    i32 destinationX,
    i32 destinationY
);
void PostprocessBitmap(u8* pixels, i32 width, i32 height);
void GrabScreenBitmap(class bitmap* destination, i32 x, i32 y);
void BitmapToScreen(class bitmap* image);
i16 AutoInitSVGA(void);
void PostprocessIcon(class icon* loadedIcon);

enum ScreenBlitConstant {
    SCREEN_BLIT_ENLARGE_END = LOGICAL_SCREEN_WIDTH - 3,
    SCREEN_BLIT_ENLARGE_PIXELS = 4
};

struct PaletteColor {
    u8 red;
    u8 green;
    u8 blue;
};

#endif

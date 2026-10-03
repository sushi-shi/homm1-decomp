#ifndef HOMM1_BASE_MISCWIN_H
#define HOMM1_BASE_MISCWIN_H

#include <Domains.h>

// miscwin.cpp: Windows-side screen, palette and clipped-icon helpers.
void ClippedMonoIconToBitmap(class icon*, class bitmap*, int, int, int, int, int, int, int, int, int);
void ClipIconToBitmap(class icon*, class bitmap*, int, int, int, int, int, int, int, int);
int Random(int, int);
void FadeIn(int);
void FadeOut(int);
void PrintMemoryLeaks(void);
void PostprocessPalette(signed char*);
void BlitBitmapToScreen(class bitmap*, int, int, int, int, int, int);
void PostprocessBitmap(signed char*, int, int);
void GrabScreenBitmap(class bitmap*, int, int);
void PostprocessIcon(class icon*);

H1_ENUM_CONST_BEGIN(ScreenBlitConstant)
    SCREEN_BLIT_WIDTH = 640,
    SCREEN_BLIT_HEIGHT = 480,
    SCREEN_BLIT_WIDTH_END = 640,
    SCREEN_BLIT_ENLARGE_END = 637,
    SCREEN_BLIT_ENLARGE_PIXELS = 4
H1_ENUM_CONST_END(ScreenBlitConstant)

#endif // HOMM1_BASE_MISCWIN_H

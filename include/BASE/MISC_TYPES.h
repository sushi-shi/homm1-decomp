#ifndef HOMM1_BASE_MISC_TYPES_H
#define HOMM1_BASE_MISC_TYPES_H

#include <Domains.h>

// clang-format off
H1_ENUM_CONST_BEGIN(MiscLogConstant)
    MISC_FILE_DEBUG_BEGIN = 2,
    MISC_DEBUGGER_OUTPUT_LEVEL = 3,
    MISC_LOG_TEXT_CAPACITY = 500,
    MISC_LOG_VALUE_TEXT_CAPACITY = 100,
    MISC_LOG_VALUES_TEXT_CAPACITY = 130,
    MISC_FORCED_DEBUG_LEVEL = 9
H1_ENUM_CONST_END(MiscLogConstant)

H1_ENUM_CONST_BEGIN(ScreenBlitConstant)
    SCREEN_BLIT_WIDTH = 640,
    SCREEN_BLIT_HEIGHT = 480,
    SCREEN_BLIT_WIDTH_END = 640,
    SCREEN_BLIT_ENLARGE_END = 637,
    SCREEN_BLIT_ENLARGE_PIXELS = 4
H1_ENUM_CONST_END(ScreenBlitConstant)

H1_ENUM_CONST_BEGIN(PaletteGraphicsConstant)
    PALETTE_GRAPHICS_BYTES = 768,
    PALETTE_GRAPHICS_END = PALETTE_GRAPHICS_BYTES,
    PALETTE_GRAPHICS_CHANNELS = 3,
    PALETTE_CYCLE_FIRST = 214,
    PALETTE_CYCLE_COLOR_COUNT = 32,
    PALETTE_FADE_LEVEL_END = 64,
    PALETTE_FADE_LEVEL_LAST = 63,
    PALETTE_WINDOWED_FADE_SCALE = 2,
    PALETTE_CYCLE_BYTES = PALETTE_CYCLE_COLOR_COUNT * PALETTE_GRAPHICS_CHANNELS
H1_ENUM_CONST_END(PaletteGraphicsConstant)
                     // clang-format on

                     class palette;
extern palette* gpBufferPalette;
extern signed char gCyclePal[PALETTE_CYCLE_BYTES];

extern int giCurExe;
extern void* hwndApp;
extern int iMainWinScreenWidth;
extern int iMainWinScreenHeight;
extern int giDebugLevel;

#endif

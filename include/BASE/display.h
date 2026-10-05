#ifndef HOMM1_BASE_DISPLAY_H
#define HOMM1_BASE_DISPLAY_H

#include <Domains.h>

// The 640x480 logical screen that heroWindowManager draws and updates.
H1_ENUM_CONST_BEGIN(LogicalScreenConstant)
    LOGICAL_SCREEN_WIDTH = 640,
    LOGICAL_SCREEN_HEIGHT = 480
H1_ENUM_CONST_END(LogicalScreenConstant)

// The 256-entry palette and the byte mask that keeps a value a palette index.
H1_ENUM_CONST_BEGIN(PaletteFormatConstant)
    PALETTE_COLOR_COUNT = 256,
    COLOR_INDEX_MASK = 0xff
H1_ENUM_CONST_END(PaletteFormatConstant)

// A client-area coordinate scaled to the logical screen.
// iMainWinScreenWidth/Height are kbwin's client extents.
#define CLIENT_TO_GAME_X(x) (((x) * LOGICAL_SCREEN_WIDTH) / iMainWinScreenWidth)
#define CLIENT_TO_GAME_Y(y) (((y) * LOGICAL_SCREEN_HEIGHT) / iMainWinScreenHeight)

#endif // HOMM1_BASE_DISPLAY_H

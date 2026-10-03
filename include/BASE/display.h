#ifndef HOMM1_BASE_DISPLAY_H
#define HOMM1_BASE_DISPLAY_H

#include <Domains.h>

// The 640x480 logical screen that heroWindowManager draws and updates,
// as in HoMM2 Buka's BASE/display.h.
H1_ENUM_CONST_BEGIN(LogicalScreenConstant)
    LOGICAL_SCREEN_WIDTH = 640,
    LOGICAL_SCREEN_HEIGHT = 480
H1_ENUM_CONST_END(LogicalScreenConstant)

// The 256-entry palette and the byte mask that keeps a value a palette index
// (Buka display.h PALETTE_COLOR_COUNT; BORDER/ICONWDGT COLOR_INDEX_MASK).
H1_ENUM_CONST_BEGIN(PaletteFormatConstant)
    PALETTE_COLOR_COUNT = 256,
    COLOR_INDEX_MASK = 0xff
H1_ENUM_CONST_END(PaletteFormatConstant)

#endif // HOMM1_BASE_DISPLAY_H

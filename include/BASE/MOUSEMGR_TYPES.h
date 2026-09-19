#ifndef HOMM1_BASE_MOUSEMGR_TYPES_H
#define HOMM1_BASE_MOUSEMGR_TYPES_H

#define WIN32_LEAN_AND_MEAN

#include <windows.h>

#include <BASE/mouseManager.h>

class resourceManager;

H1_ENUM_BEGIN(MouseManagerConstant)
    MOUSE_CURSOR_COUNT = 75,
    MOUSE_CURSOR_AXIS_COUNT = 2,
    MOUSE_CURSOR_HORIZONTAL = 0,
    MOUSE_CURSOR_VERTICAL = 1,
    MOUSE_CURSOR_BITMAP_BEGIN = 0,
    MOUSE_CURSOR_BITMAP_END = 32,
    MOUSE_CURSOR_BITMAP_WIDTH = MOUSE_CURSOR_BITMAP_END,
    MOUSE_CURSOR_MASK_HEIGHT = 64,
    MOUSE_CURSOR_MASK_ROW_BYTES = 4,
    MOUSE_CURSOR_COLOR_BYTES = 0x400,
    MOUSE_CURSOR_AND_BYTES = 0x100,
    MOUSE_CURSOR_MASK_PLANE_BYTES = 0x80,
    MOUSE_CURSOR_BITMAP_HEADER_BYTES = 6,
    MOUSE_CURSOR_BITMAP_PLANES = 1,
    MOUSE_CURSOR_BITMAP_BITS_PER_PIXEL = 1,
    MOUSE_CURSOR_MASK_SHIFT = 3,
    MOUSE_CURSOR_MASK_HIGH_BIT = 7,
    MOUSE_CURSOR_ADVENTURE = 0,
    MOUSE_CURSOR_COMBAT = 1,
    MOUSE_CURSOR_SPELL = 2,
    MOUSE_KEEP_CURRENT_FRAME = 1000,
    CONFIG_EXECUTABLE_EDITOR = 1
H1_ENUM_END(MouseManagerConstant)

extern void *hwndApp;
extern int iMainWinScreenWidth;
extern int iMainWinScreenHeight;
extern int giCurExe;
extern int gbColorMice;
extern int gbSpecialMouseMasks;
extern int gMouseCursorType;
extern int iMouseOffset[3];
extern unsigned char iHotSpot[MOUSE_CURSOR_COUNT][MOUSE_CURSOR_AXIS_COUNT];
extern int gbInSetPointer;
extern HCURSOR hMouseCursor[MOUSE_CURSOR_COUNT];
extern signed char *cColorBits[MOUSE_CURSOR_COUNT];
extern unsigned char *cAndBits[MOUSE_CURSOR_COUNT];
extern BITMAP bmpAndMask[MOUSE_CURSOR_COUNT];
extern BITMAP bmpColor[MOUSE_CURSOR_COUNT];
extern HBITMAP hbmpAndMask[MOUSE_CURSOR_COUNT];
extern HBITMAP hbmpColor[MOUSE_CURSOR_COUNT];
extern ICONINFO mouseIconInfo[MOUSE_CURSOR_COUNT];
extern resourceManager *gpResourceManager;

#endif

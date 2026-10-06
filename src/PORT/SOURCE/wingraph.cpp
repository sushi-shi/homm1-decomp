// The native display host: the counterpart of src/SOURCE/wingraph.cpp, which
// drew through WinG in a window and DirectDraw at full screen. The game draws
// into a 640x480 8-bit buffer; finished rectangles and palette changes go to
// the platform display, which shows them as DirectDraw's palettized primary
// surface did.

#include <H1/Ints.h>

#include <SOURCE/wingraph.h>

#include <BASE/bitmap.h>
#include <BASE/heroWindowManager.h>
#include <BASE/Misc.h>
#include <BASE/miscwin.h>
#include <BASE/mouseManager.h>
#include <BASE/palette.h>
#include <SOURCE/KB.h>
#include <SOURCE/kbwin.h>

#include <PLATFORM/Platform.h>

#include "../PortHost.h"

#include <array>
#include <cstring>
#include <vector>

i32 gWinGAttached = 0;
i32 gDDrawAttached = 1;
i32 gGraphicsType = WINGRAPH_GRAPHICS_DIRECT_DRAW;
i32 gMainVideoModeColorDepth = WINGRAPH_COLOR_DEPTH;
i32 gMainVideoModeWidth = 1024;
i32 gMainVideoModeHeight = 768;
void* gInitWin = NULL;
i32 gTtlBlts = 0;
i32 gWinGraphBusy = 0;

namespace {

// Windows reserves the first and last ten palette entries for its static
// colors; the game sets only the 236 between them.
constexpr platform::Color kSystemColors[WINGRAPH_SYSTEM_PALETTE_SIZE * 2] = {
    {0, 0, 0},       {128, 0, 0},     {0, 128, 0},     {128, 128, 0},   {0, 0, 128},
    {128, 0, 128},   {0, 128, 128},   {192, 192, 192}, {192, 220, 192}, {166, 202, 240},
    {255, 251, 240}, {160, 160, 164}, {128, 128, 128}, {255, 0, 0},     {0, 255, 0},
    {255, 255, 0},   {0, 0, 255},     {255, 0, 255},   {0, 255, 255},   {255, 255, 255},
};

std::vector<u8> gScreen;

std::array<platform::CursorImage, MOUSE_CURSOR_COUNT> gCursors;
std::array<bool, MOUSE_CURSOR_COUNT> gCursorReady{};

}  // namespace

void GetGraphicsInfo(void) {
    gMainVideoModeColorDepth = WINGRAPH_COLOR_DEPTH;
}

void InitializePalette() {
    platform::Color colors[PALETTE_COLOR_COUNT] = {};
    for (i32 i = 0; i < WINGRAPH_SYSTEM_PALETTE_SIZE; i++) {
        colors[i] = kSystemColors[i];
        colors[WINGRAPH_MUTABLE_PALETTE_END + i] = kSystemColors[WINGRAPH_SYSTEM_PALETTE_SIZE + i];
    }
    platform::SetPalette(colors, 0, PALETTE_COLOR_COUNT);
}

void InitGraphics() {
    if (!gScreen.empty())
        return;
    if (!platform::OpenDisplay())
        ShutDown(const_cast<char*>("The display could not be opened."));
    gScreen.assign(LOGICAL_SCREEN_WIDTH * LOGICAL_SCREEN_HEIGHT, 0);
    gInitWin = gScreen.data();
    platform::SetReferenceImage(gScreen.data());
    InitializePalette();
}

void UpdatePalette(i8* paletteData) {
    platform::Color colors[WINGRAPH_MUTABLE_PALETTE_END - WINGRAPH_SYSTEM_PALETTE_SIZE];
    for (i32 entry = WINGRAPH_SYSTEM_PALETTE_SIZE; entry < WINGRAPH_MUTABLE_PALETTE_END; entry++) {
        platform::Color& color = colors[entry - WINGRAPH_SYSTEM_PALETTE_SIZE];
        color.r = static_cast<u8>(paletteData[entry * PALETTE_GRAPHICS_CHANNELS]
                                  << WINGRAPH_PALETTE_VALUE_SHIFT);
        color.g = static_cast<u8>(paletteData[entry * PALETTE_GRAPHICS_CHANNELS + 1]
                                  << WINGRAPH_PALETTE_VALUE_SHIFT);
        color.b = static_cast<u8>(paletteData[entry * PALETTE_GRAPHICS_CHANNELS + 2]
                                  << WINGRAPH_PALETTE_VALUE_SHIFT);
    }
    platform::SetPalette(colors, WINGRAPH_SYSTEM_PALETTE_SIZE,
                         WINGRAPH_MUTABLE_PALETTE_END - WINGRAPH_SYSTEM_PALETTE_SIZE);
}

void CleanUpWinGraphics() {
    platform::CloseDisplay();
}

void RestoreDisplayMode() {}

i32 QueryNewPalette() {
    return 0;
}

i32 SetGraphicsType(i32 graphicsType) {
    return graphicsType == gGraphicsType;
}

void SetFullScreenStatus(i32 fullScreen) {
    if (gInSmacker != 0)
        return;
    if (fullScreen == CURRENT_GRAPHICS_CONFIG.fullScreen)
        return;
    CURRENT_GRAPHICS_CONFIG.fullScreen = fullScreen;
    platform::SetFullscreen(fullScreen != 0);
    MenuRefresh();
    WritePrefs();
}

// The original invalidated this rectangle and painted it from WM_PAINT. While
// the adventure map scrolls (gScrollX or gScrollY set), the paint takes its
// source from the scrolled position inside the 16-pixel margin around the
// 448x448 view, as DDAppPaint did.
void KBPaintScreen(i32 left, i32 top, i32 right, i32 bottom) {
    if (gInitWin == NULL)
        return;
    // The original invalidated [left, right) and DDAppPaint then copied one
    // more column and row than the rectangle it was given (right++ below the
    // screen's edge, and a width of right - left + 1): the game's blits rely
    // on it, drawing up to two pixels past what they name.
    if (right < LOGICAL_SCREEN_WIDTH)
        right++;
    if (bottom < LOGICAL_SCREEN_HEIGHT)
        bottom++;
    i32 width = right - left + 1;
    i32 height = bottom - top + 1;
    i32 sourceX = left;
    i32 sourceY = top;
    if (gScrollX != 0) {
        sourceX = gScrollX + WINGRAPH_SCROLL_MARGIN;
        width = WINGRAPH_SCROLL_SIZE;
    }
    if (gScrollY != 0) {
        sourceY = gScrollY + WINGRAPH_SCROLL_MARGIN;
        height = WINGRAPH_SCROLL_SIZE;
    }
    gTtlBlts++;
    platform::UpdateDisplay(static_cast<const u8*>(gInitWin), LOGICAL_SCREEN_WIDTH, sourceX,
                            sourceY, width, height, left, top);
}

// ---------------------------------------------------------------- cursors

i32 KBCursorReady(i32 cursorIndex) {
    return gCursorReady[static_cast<size_t>(cursorIndex)];
}

void KBCreateCursor(
    i32 cursorIndex,
    const u8* colorBits,
    const u8* maskBits,
    i32 colorCursor,
    i32 hotX,
    i32 hotY
) {
    platform::CursorImage& image = gCursors[static_cast<size_t>(cursorIndex)];
    image = platform::CursorImage();
    image.colorCursor = colorCursor != 0;
    image.hotX = hotX;
    image.hotY = hotY;
    std::memcpy(image.color, colorBits, sizeof(image.color));
    std::memcpy(image.andMask, maskBits, MOUSE_CURSOR_MASK_PLANE_BYTES);
    if (!colorCursor)
        std::memcpy(image.xorMask, maskBits + MOUSE_CURSOR_MASK_PLANE_BYTES,
                    MOUSE_CURSOR_MASK_PLANE_BYTES);
    gCursorReady[static_cast<size_t>(cursorIndex)] = true;
}

void KBSelectCursor(i32 cursorIndex) {
    platform::SetCursorImage(&gCursors[static_cast<size_t>(cursorIndex)]);
}

void KBSelectArrowCursor(void) {
    platform::SetCursorImage(nullptr);
}

void KBDestroyCursors(void) {
    gCursorReady.fill(false);
}

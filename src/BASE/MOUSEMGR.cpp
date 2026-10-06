#include <H1/Ints.h>

#include <BASE/baseManager.h>
#include <BASE/bitmap.h>
#include <BASE/display.h>
#include <BASE/message.h>
#include <BASE/Misc.h>
#include <BASE/mouseManager.h>
#include <BASE/resourceManager.h>
#include <SOURCE/KB.h>
#include <SOURCE/kbwin.h>
#include <SOURCE/NOOPT.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef HOMM1_EDITOR
#define MOUSEMGR_CPP_PATH "U:\\HMM\\VSS\\HMM1\\Source\\Base\\MOUSEMGR.CPP"
#else
#define MOUSEMGR_CPP_PATH "E:\\Users\\igorl\\VSS\\HMM\\HMM1\\Source\\Base\\MOUSEMGR.CPP"
#endif

mouseManager::mouseManager(void) {
    m_savedUnderlying = NULL;
    m_cursorImage = NULL;
    m_mouseX = MOUSE_INITIAL_X;
    m_mouseY = MOUSE_INITIAL_Y;
    m_pointerFlags = MOUSE_INITIAL_POINTER_FLAGS;
    m_cursorReady = 1;
    m_active = 0;
    strcpy(m_name, "mouseManager");
    m_savedLeft = 0;
    m_savedTop = 0;
    m_drawIntoScreen = 0;
    m_savedUnderlying = NULL;
    m_cursorFrame = 0;
    for (i32 cursorIndex = 0; cursorIndex < MOUSE_CURSOR_COUNT; cursorIndex++) {
        gMouseCursors[cursorIndex] = NULL;
        gAndBits[cursorIndex] = NULL;
        gColorBits[cursorIndex] = NULL;
        gAndMaskBitmaps[cursorIndex] = NULL;
        gColorBitmaps[cursorIndex] = NULL;
    }
}

i16 mouseManager::Open(i16 priority) {
    m_savedUnderlying =
        new bitmap(BITMAP_TYPE_MEMORY, MOUSE_SAVED_BITMAP_SIZE, MOUSE_SAVED_BITMAP_SIZE);
    m_messageMask = BASE_MANAGER_ACCEPT_RIGHT_BUTTON_UP;
    m_priority = priority;
    m_active = 1;
    return BASE_MANAGER_SUCCESS;
}

void mouseManager::Close(void) {
    i32 cursorIndex;
    if (m_active != 1)
        return;
    m_active = 0;
    if (m_savedUnderlying != NULL)
        delete m_savedUnderlying;
    m_savedUnderlying = NULL;
    SetCursor(LoadCursorA(NULL, IDC_ARROW));
    DelayMilli(50);
    for (cursorIndex = 0; cursorIndex < MOUSE_CURSOR_COUNT; cursorIndex++) {
        if (gMouseCursors[cursorIndex] != NULL)
            DestroyIcon(gMouseCursors[cursorIndex]);
        gMouseCursors[cursorIndex] = NULL;
        if (gAndBits[cursorIndex] != NULL)
            free(gAndBits[cursorIndex]);
        gAndBits[cursorIndex] = NULL;
        if (gColorBits[cursorIndex] != NULL)
            free(gColorBits[cursorIndex]);
        gColorBits[cursorIndex] = NULL;
        if (gAndMaskBitmaps[cursorIndex] != NULL)
            DeleteObject(gAndMaskBitmaps[cursorIndex]);
        gAndMaskBitmaps[cursorIndex] = NULL;
        if (gColorBitmaps[cursorIndex] != NULL)
            DeleteObject(gColorBitmaps[cursorIndex]);
        gColorBitmaps[cursorIndex] = NULL;
    }
    DelayMilli(50);
}

i16 mouseManager::Main(tag_message& message) {
    return MESSAGE_DISPATCH_CONTINUE;
}

void mouseManager::SetPointer(char* name, i16 frame) {
    if (*name == 'a' || *name == 'A')
        gMouseCursorType = MOUSE_CURSOR_ADVENTURE;
    else if (*name == 's' || *name == 'S')
        gMouseCursorType = MOUSE_CURSOR_SPELL;
    else
        gMouseCursorType = MOUSE_CURSOR_COMBAT;
    SetPointer(frame);
}

i32 gMouseCursorType = 0;

void mouseManager::SetPointer(i16 frame) {
    static BOOL gInSetPointer = FALSE;
    i32 cursorIndex;
    i32 x;
    i32 y;
    char filename[MOUSE_CURSOR_FILENAME_CAPACITY];

    if (frame < 0)
        return;
    if (m_active != 1)
        return;

    if (gCurExe == CONFIG_EXECUTABLE_EDITOR) {
        gMouseCursorType = MOUSE_CURSOR_ADVENTURE;
        if (frame > 0)
            frame = 0;
    }

    if (gInSetPointer)
        return;
    gInSetPointer = TRUE;

    if (frame == MOUSE_KEEP_CURRENT_FRAME)
        frame = m_cursorFrame;
    else
        m_cursorFrame = frame;

    cursorIndex = frame + gMouseOffset[gMouseCursorType];
    H1_ASSERT(cursorIndex >= 0 && cursorIndex < MOUSE_CURSOR_COUNT);

    if (gMouseCursors[cursorIndex] == NULL) {
        gColorBits[cursorIndex] = static_cast<u8*>(malloc(MOUSE_CURSOR_COLOR_BYTES));
        if (gColorMice)
            gAndBits[cursorIndex] = static_cast<u8*>(malloc(MOUSE_CURSOR_MASK_PLANE_BYTES));
        else
            gAndBits[cursorIndex] = static_cast<u8*>(malloc(MOUSE_CURSOR_AND_BYTES));

        if (gMouseCursorType == MOUSE_CURSOR_ADVENTURE)
            sprintf(filename, "ADVM%s%02d.BMP", gColorMice ? "CO" : "BW", frame + 1);
        else if (gMouseCursorType == MOUSE_CURSOR_SPELL)
            sprintf(filename, "SPEL%s%02d.BMP", gColorMice ? "CO" : "BW", frame + 1);
        else
            sprintf(filename, "CMSE%s%02d.BMP", gColorMice ? "CO" : "BW", frame + 1);

        gResourceManager->PointToFile(gResourceManager->MakeId(filename));
        gResourceManager->ReadBlock(gColorBits[cursorIndex], MOUSE_CURSOR_BITMAP_HEADER_BYTES);
        gResourceManager->ReadBlock(gColorBits[cursorIndex], MOUSE_CURSOR_COLOR_BYTES);
        memset(
            gAndBits[cursorIndex],
            0,
            gColorMice ? MOUSE_CURSOR_MASK_PLANE_BYTES : MOUSE_CURSOR_AND_BYTES
        );

        for (y = MOUSE_CURSOR_BITMAP_BEGIN; y < MOUSE_CURSOR_BITMAP_END; y++) {
            for (x = MOUSE_CURSOR_BITMAP_BEGIN; x < MOUSE_CURSOR_BITMAP_END; x++) {
                if (gSpecialMouseMasks && !gColorMice) {
                    if (*(gColorBits[cursorIndex] + x + y * MOUSE_CURSOR_BITMAP_WIDTH)
                        == MOUSE_CURSOR_PIXEL_TRANSPARENT)
                        *(gAndBits[cursorIndex] + y * MOUSE_CURSOR_MASK_ROW_BYTES
                          + (x >> MOUSE_CURSOR_MASK_SHIFT)) |=
                            1 << (MOUSE_CURSOR_MASK_HIGH_BIT - (x & MOUSE_CURSOR_MASK_HIGH_BIT));
                    else if (*(gColorBits[cursorIndex] + x + y * MOUSE_CURSOR_BITMAP_WIDTH)
                             == MOUSE_CURSOR_PIXEL_OUTLINE)
                        *(gAndBits[cursorIndex] + MOUSE_CURSOR_MASK_PLANE_BYTES
                          + y * MOUSE_CURSOR_MASK_ROW_BYTES + (x >> MOUSE_CURSOR_MASK_SHIFT)) |=
                            1 << (MOUSE_CURSOR_MASK_HIGH_BIT - (x & MOUSE_CURSOR_MASK_HIGH_BIT));
                } else {
                    if (*(gColorBits[cursorIndex] + x + y * MOUSE_CURSOR_BITMAP_WIDTH)
                        == MOUSE_CURSOR_PIXEL_TRANSPARENT)
                        *(gAndBits[cursorIndex] + y * MOUSE_CURSOR_MASK_ROW_BYTES
                          + (x >> MOUSE_CURSOR_MASK_SHIFT)) |=
                            1 << (MOUSE_CURSOR_MASK_HIGH_BIT - (x & MOUSE_CURSOR_MASK_HIGH_BIT));
                    else if (!gColorMice
                             && *(gColorBits[cursorIndex] + x + y * MOUSE_CURSOR_BITMAP_WIDTH)
                                    != MOUSE_CURSOR_PIXEL_OUTLINE)
                        *(gAndBits[cursorIndex] + MOUSE_CURSOR_MASK_PLANE_BYTES
                          + y * MOUSE_CURSOR_MASK_ROW_BYTES + (x >> MOUSE_CURSOR_MASK_SHIFT)) |=
                            1 << (MOUSE_CURSOR_MASK_HIGH_BIT - (x & MOUSE_CURSOR_MASK_HIGH_BIT));
                }
            }
        }

        gAndMaskBitmapInfo[cursorIndex].bmType = 0;
        gAndMaskBitmapInfo[cursorIndex].bmWidth = MOUSE_CURSOR_BITMAP_WIDTH;
        gAndMaskBitmapInfo[cursorIndex].bmHeight =
            gColorMice ? MOUSE_CURSOR_BITMAP_WIDTH : MOUSE_CURSOR_MASK_HEIGHT;
        gAndMaskBitmapInfo[cursorIndex].bmWidthBytes = MOUSE_CURSOR_MASK_ROW_BYTES;
        gAndMaskBitmapInfo[cursorIndex].bmPlanes = MOUSE_CURSOR_BITMAP_PLANES;
        gAndMaskBitmapInfo[cursorIndex].bmBitsPixel = MOUSE_CURSOR_BITMAP_BITS_PER_PIXEL;
        gAndMaskBitmapInfo[cursorIndex].bmWidthBytes = MOUSE_CURSOR_MASK_ROW_BYTES;
        gAndMaskBitmapInfo[cursorIndex].bmBits = gAndBits[cursorIndex];
        gAndMaskBitmaps[cursorIndex] = CreateBitmapIndirect(&gAndMaskBitmapInfo[cursorIndex]);
        H1_ASSERT(reinterpret_cast<i32>(gAndMaskBitmaps[cursorIndex]));

        if (gColorMice) {
            gColorBitmapInfo[cursorIndex].bmType = 0;
            gColorBitmapInfo[cursorIndex].bmWidth = MOUSE_CURSOR_BITMAP_WIDTH;
            gColorBitmapInfo[cursorIndex].bmHeight = MOUSE_CURSOR_BITMAP_WIDTH;
            gColorBitmapInfo[cursorIndex].bmPlanes = MOUSE_CURSOR_BITMAP_PLANES;
            gColorBitmapInfo[cursorIndex].bmBitsPixel = MOUSE_CURSOR_COLOR_BITS_PER_PIXEL;
            gColorBitmapInfo[cursorIndex].bmWidthBytes = MOUSE_CURSOR_BITMAP_WIDTH;
            gColorBitmapInfo[cursorIndex].bmBits = gColorBits[cursorIndex];
            gColorBitmaps[cursorIndex] = CreateBitmapIndirect(&gColorBitmapInfo[cursorIndex]);
        }

        gMouseIconInfo[cursorIndex].fIcon = FALSE;
        gMouseIconInfo[cursorIndex].xHotspot = gHotSpot[cursorIndex][MOUSE_CURSOR_HORIZONTAL];
        gMouseIconInfo[cursorIndex].yHotspot = gHotSpot[cursorIndex][MOUSE_CURSOR_VERTICAL];
        gMouseIconInfo[cursorIndex].hbmMask = gAndMaskBitmaps[cursorIndex];
        gMouseIconInfo[cursorIndex].hbmColor = gColorMice ? gColorBitmaps[cursorIndex] : NULL;
        gMouseCursors[cursorIndex] = CreateIconIndirect(&gMouseIconInfo[cursorIndex]);
        H1_ASSERT(reinterpret_cast<i32>(gMouseCursors[cursorIndex]));
    }

    SetCursor(gMouseCursors[cursorIndex]);
    gInSetPointer = FALSE;
}

void mouseManager::ReallyShowPointer(void) {}

void mouseManager::UnusedTwoArgumentHook1(i16, i16) {}

void mouseManager::ReallyHidePointer(void) {}

void mouseManager::HideColorPointer(void) {}

void mouseManager::RestoreUnderlying(void) {}

void mouseManager::SaveAndDraw(void) {}

void mouseManager::UnusedTwoArgumentHook2(i16, i16) {}

void mouseManager::SaveAndDraw(bitmap* buffer, i16 x, i16 y, i16 cursorUpdate) {}

void mouseManager::MovePointer(i16 x, i16 y) {}

void mouseManager::ShowColorPointer(void) {}

void mouseManager::NewUpdate(b32 force) {}

void mouseManager::WarpPointer(i16 x, i16 y) {}

void mouseManager::UnusedOneArgumentHook(i32) {}

void mouseManager::MouseCoords(i16& x, i16& y) {
    POINT point;

    GetCursorPos(&point);
    ScreenToClient(gAppWindow, &point);
    x = CLIENT_TO_GAME_X(point.x);
    y = CLIENT_TO_GAME_Y(point.y);
}

void mouseManager::SetCursorShape(i32 shape) {}

void mouseManager::SetColorMice(b32 enabled) {}

void mouseManager::HideSystemCursor(void) {
    ShowCursor(FALSE);
}

void mouseManager::ShowSystemCursor(void) {
    ShowCursor(TRUE);
}

i32 gMouseOffset[3] = {0, 40, 55};
u8 gHotSpot[MOUSE_CURSOR_COUNT][MOUSE_CURSOR_AXIS_COUNT] = {
    {2, 3},   {2, 3},   {12, 11}, {12, 13}, {15, 11}, {10, 10}, {12, 13}, {9, 12},  {7, 9},
    {15, 15}, {15, 11}, {10, 10}, {12, 13}, {9, 12},  {7, 9},   {15, 15}, {15, 11}, {10, 10},
    {12, 13}, {9, 12},  {7, 9},   {15, 15}, {15, 11}, {10, 10}, {12, 13}, {9, 12},  {7, 9},
    {15, 15}, {12, 12}, {12, 12}, {12, 12}, {12, 12}, {3, 0},   {23, 0},  {31, 4},  {23, 23},
    {3, 31},  {0, 24},  {0, 5},   {0, 0},   {10, 9},  {9, 11},  {10, 11}, {12, 12}, {10, 12},
    {5, 8},   {1, 1},   {21, 1},  {30, 7},  {21, 21}, {1, 21},  {1, 7},   {1, 1},   {7, 1},
    {7, 30},  {22, 23}, {22, 23}, {22, 23}, {22, 23}, {22, 23}, {22, 23}, {22, 23}, {22, 23},
    {22, 23}, {22, 23}, {22, 23}, {22, 23}, {22, 23}, {22, 23}, {22, 23}, {22, 23}, {22, 23},
    {22, 23}, {22, 23}, {22, 23}
};
HBITMAP gColorBitmaps[MOUSE_CURSOR_COUNT];
BITMAP gAndMaskBitmapInfo[MOUSE_CURSOR_COUNT];
HCURSOR gMouseCursors[MOUSE_CURSOR_COUNT];
u8* gAndBits[MOUSE_CURSOR_COUNT];
BITMAP gColorBitmapInfo[MOUSE_CURSOR_COUNT];
u8* gColorBits[MOUSE_CURSOR_COUNT];
ICONINFO gMouseIconInfo[MOUSE_CURSOR_COUNT];
HBITMAP gAndMaskBitmaps[MOUSE_CURSOR_COUNT];

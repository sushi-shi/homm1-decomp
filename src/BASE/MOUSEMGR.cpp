// Located from HoMM2 Buka 2.1; PoL 2.0 supplies the VC4 declaration.

#include <match.h>

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

// Retail assertion paths: each program's BASE objects were compiled in its own
// checkout (HEROES.EXE and EDITOR.EXE assertion strings).
#ifdef HOMM1_EDITOR
#define MOUSEMGR_CPP_PATH "U:\\HMM\\VSS\\HMM1\\Source\\Base\\MOUSEMGR.CPP"
#else
#define MOUSEMGR_CPP_PATH "E:\\Users\\igorl\\VSS\\HMM\\HMM1\\Source\\Base\\MOUSEMGR.CPP"
#endif

DATA(0x004a10e4)
char gAdventureColor[] = "CO";
DATA(0x004a10e8)
char gAdventureMonochrome[] = "BW";
DATA(0x004a10ec)
char gAdventureBitmapFormat[] = "ADVM%s%02d.BMP";
DATA(0x004a10fc)
char gSpellColor[] = "CO";
DATA(0x004a1100)
char gSpellMonochrome[] = "BW";
DATA(0x004a1104)
char gSpellBitmapFormat[] = "SPEL%s%02d.BMP";
DATA(0x004a1114)
char gCombatColor[] = "CO";
DATA(0x004a1118)
char gCombatMonochrome[] = "BW";
DATA(0x004a111c)
char gCombatBitmapFormat[] = "CMSE%s%02d.BMP";

VA(0x0046b510, 0x102)
mouseManager::mouseManager(void) {
    m_savedUnderlying = NULL;
    m_cursorImage = NULL;
    m_mouseX = MOUSE_INITIAL_X;
    m_mouseY = MOUSE_INITIAL_Y;
    m_pointerFlags = MOUSE_INITIAL_POINTER_FLAGS;
    m_cursorReady = 1;
    m_active = 0;
    strcpy(m_name, "mouseManager");
    m_unknown49 = 0;
    m_unknown4d = 0;
    m_unknown51 = 0;
    m_savedUnderlying = NULL;
    m_cursorFrame = 0;
    for (i32 cursorIndex = 0; cursorIndex < MOUSE_CURSOR_COUNT; cursorIndex++) {
        hMouseCursor[cursorIndex] = NULL;
        cAndBits[cursorIndex] = NULL;
        gColorBits[cursorIndex] = NULL;
        hbmpAndMask[cursorIndex] = NULL;
        hbmpColor[cursorIndex] = NULL;
    }
}

VA(0x0046b612, 0x98)
i16 mouseManager::Open(i16 priority) {
    m_savedUnderlying =
        new bitmap(BITMAP_TYPE_MEMORY, MOUSE_SAVED_BITMAP_SIZE, MOUSE_SAVED_BITMAP_SIZE);
    m_messageMask = MOUSE_MANAGER_MESSAGE_MASK;
    m_priority = priority;
    m_active = 1;
    return BASE_MANAGER_SUCCESS;
}

// Retail releases both monochrome/color masks and pauses around cursor teardown.
VA(0x0046b6aa, 0x190)
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
        if (hMouseCursor[cursorIndex] != NULL)
            DestroyIcon(hMouseCursor[cursorIndex]);
        hMouseCursor[cursorIndex] = NULL;
        if (cAndBits[cursorIndex] != NULL)
            free(cAndBits[cursorIndex]);
        cAndBits[cursorIndex] = NULL;
        if (gColorBits[cursorIndex] != NULL)
            free(gColorBits[cursorIndex]);
        gColorBits[cursorIndex] = NULL;
        if (hbmpAndMask[cursorIndex] != NULL)
            DeleteObject(hbmpAndMask[cursorIndex]);
        hbmpAndMask[cursorIndex] = NULL;
        if (hbmpColor[cursorIndex] != NULL)
            DeleteObject(hbmpColor[cursorIndex]);
        hbmpColor[cursorIndex] = NULL;
    }
    DelayMilli(50);
}

VA(0x0046b83a, 0x10)
i16 mouseManager::Main(tag_message&) {
    return 0;
}

// HoMM1 selects the cursor family by name and forwards the requested frame.
VA(0x0046b84a, 0x68)
void mouseManager::SetPointer(char* name, i16 frame) {
    if (*name == 'a' || *name == 'A')
        gMouseCursorType = MOUSE_CURSOR_ADVENTURE;
    else if (*name == 's' || *name == 'S')
        gMouseCursorType = MOUSE_CURSOR_SPELL;
    else
        gMouseCursorType = MOUSE_CURSOR_COMBAT;
    SetPointer(frame);
}

// donor PoL RVA 0x000c9630; preferred Buka symbol ?SetPointer@mouseManager@@QAEXH@Z
// donor Buka TU BASE/MOUSEMGR; HoMM1 owner inferred from contiguous order
// evidence: graph:7;base=0.417606;margin=1.286688;shape=0.189;size=0.849;calls=0.737;alternate=pol20:void mouseManager::SetPointer(int)@0x000c9630
VA(0x0046b8b2, 0x6bd)
#line 232 MOUSEMGR_CPP_PATH
void mouseManager::SetPointer(i16 frame) {
    DATA(0x004cfb44)
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
#line 266
    H1_ASSERT(cursorIndex >= 0 && cursorIndex < MOUSE_CURSOR_COUNT);

    if (hMouseCursor[cursorIndex] == NULL) {
        gColorBits[cursorIndex] = static_cast<u8*>(malloc(MOUSE_CURSOR_COLOR_BYTES));
        if (gColorMice)
            cAndBits[cursorIndex] = static_cast<u8*>(malloc(MOUSE_CURSOR_MASK_PLANE_BYTES));
        else
            cAndBits[cursorIndex] = static_cast<u8*>(malloc(MOUSE_CURSOR_AND_BYTES));

        if (gMouseCursorType == MOUSE_CURSOR_ADVENTURE)
            sprintf(
                filename,
                gAdventureBitmapFormat,
                gColorMice ? gAdventureColor : gAdventureMonochrome,
                frame + 1
            );
        else if (gMouseCursorType == MOUSE_CURSOR_SPELL)
            sprintf(
                filename,
                gSpellBitmapFormat,
                gColorMice ? gSpellColor : gSpellMonochrome,
                frame + 1
            );
        else
            sprintf(
                filename,
                gCombatBitmapFormat,
                gColorMice ? gCombatColor : gCombatMonochrome,
                frame + 1
            );

        gpResourceManager->PointToFile(gpResourceManager->MakeId(filename));
        // API-forced: ReadBlock takes i8*.
        gpResourceManager->ReadBlock(
            reinterpret_cast<i8*>(gColorBits[cursorIndex]),
            MOUSE_CURSOR_BITMAP_HEADER_BYTES
        );
        // API-forced: ReadBlock takes i8*.
        gpResourceManager->ReadBlock(
            reinterpret_cast<i8*>(gColorBits[cursorIndex]),
            MOUSE_CURSOR_COLOR_BYTES
        );
        memset(
            cAndBits[cursorIndex],
            0,
            gColorMice ? MOUSE_CURSOR_MASK_PLANE_BYTES : MOUSE_CURSOR_AND_BYTES
        );

        for (y = MOUSE_CURSOR_BITMAP_BEGIN; y < MOUSE_CURSOR_BITMAP_END; y++) {
            for (x = MOUSE_CURSOR_BITMAP_BEGIN; x < MOUSE_CURSOR_BITMAP_END; x++) {
                if (gSpecialMouseMasks && !gColorMice) {
                    if (*(gColorBits[cursorIndex] + x + y * MOUSE_CURSOR_BITMAP_WIDTH) == 0)
                        *(cAndBits[cursorIndex] + y * MOUSE_CURSOR_MASK_ROW_BYTES
                          + (x >> MOUSE_CURSOR_MASK_SHIFT)) |=
                            1 << (MOUSE_CURSOR_MASK_HIGH_BIT - (x & MOUSE_CURSOR_MASK_HIGH_BIT));
                    else if (*(gColorBits[cursorIndex] + x + y * MOUSE_CURSOR_BITMAP_WIDTH) == 1)
                        *(cAndBits[cursorIndex] + MOUSE_CURSOR_MASK_PLANE_BYTES
                          + y * MOUSE_CURSOR_MASK_ROW_BYTES + (x >> MOUSE_CURSOR_MASK_SHIFT)) |=
                            1 << (MOUSE_CURSOR_MASK_HIGH_BIT - (x & MOUSE_CURSOR_MASK_HIGH_BIT));
                } else {
                    if (*(gColorBits[cursorIndex] + x + y * MOUSE_CURSOR_BITMAP_WIDTH) == 0)
                        *(cAndBits[cursorIndex] + y * MOUSE_CURSOR_MASK_ROW_BYTES
                          + (x >> MOUSE_CURSOR_MASK_SHIFT)) |=
                            1 << (MOUSE_CURSOR_MASK_HIGH_BIT - (x & MOUSE_CURSOR_MASK_HIGH_BIT));
                    else if (!gColorMice
                             && *(gColorBits[cursorIndex] + x + y * MOUSE_CURSOR_BITMAP_WIDTH) != 1)
                        *(cAndBits[cursorIndex] + MOUSE_CURSOR_MASK_PLANE_BYTES
                          + y * MOUSE_CURSOR_MASK_ROW_BYTES + (x >> MOUSE_CURSOR_MASK_SHIFT)) |=
                            1 << (MOUSE_CURSOR_MASK_HIGH_BIT - (x & MOUSE_CURSOR_MASK_HIGH_BIT));
                }
            }
        }

        bmpAndMask[cursorIndex].bmType = 0;
        bmpAndMask[cursorIndex].bmWidth = MOUSE_CURSOR_BITMAP_WIDTH;
        bmpAndMask[cursorIndex].bmHeight =
            gColorMice ? MOUSE_CURSOR_BITMAP_WIDTH : MOUSE_CURSOR_MASK_HEIGHT;
        bmpAndMask[cursorIndex].bmWidthBytes = MOUSE_CURSOR_MASK_ROW_BYTES;
        bmpAndMask[cursorIndex].bmPlanes = MOUSE_CURSOR_BITMAP_PLANES;
        bmpAndMask[cursorIndex].bmBitsPixel = MOUSE_CURSOR_BITMAP_BITS_PER_PIXEL;
        bmpAndMask[cursorIndex].bmWidthBytes = MOUSE_CURSOR_MASK_ROW_BYTES;
        bmpAndMask[cursorIndex].bmBits = cAndBits[cursorIndex];
        hbmpAndMask[cursorIndex] = CreateBitmapIndirect(&bmpAndMask[cursorIndex]);
        // API-forced handle value.
#line 322
        H1_ASSERT(reinterpret_cast<i32>(hbmpAndMask[cursorIndex]));

        if (gColorMice) {
            bmpColor[cursorIndex].bmType = 0;
            bmpColor[cursorIndex].bmWidth = MOUSE_CURSOR_BITMAP_WIDTH;
            bmpColor[cursorIndex].bmHeight = MOUSE_CURSOR_BITMAP_WIDTH;
            bmpColor[cursorIndex].bmPlanes = MOUSE_CURSOR_BITMAP_PLANES;
            bmpColor[cursorIndex].bmBitsPixel = MOUSE_CURSOR_COLOR_BITS_PER_PIXEL;
            bmpColor[cursorIndex].bmWidthBytes = MOUSE_CURSOR_BITMAP_WIDTH;
            bmpColor[cursorIndex].bmBits = gColorBits[cursorIndex];
            hbmpColor[cursorIndex] = CreateBitmapIndirect(&bmpColor[cursorIndex]);
        }

        mouseIconInfo[cursorIndex].fIcon = FALSE;
        mouseIconInfo[cursorIndex].xHotspot = gHotSpot[cursorIndex][MOUSE_CURSOR_HORIZONTAL];
        mouseIconInfo[cursorIndex].yHotspot = gHotSpot[cursorIndex][MOUSE_CURSOR_VERTICAL];
        mouseIconInfo[cursorIndex].hbmMask = hbmpAndMask[cursorIndex];
        mouseIconInfo[cursorIndex].hbmColor = gColorMice ? hbmpColor[cursorIndex] : NULL;
        hMouseCursor[cursorIndex] = CreateIconIndirect(&mouseIconInfo[cursorIndex]);
        // API-forced handle value.
#line 343
        H1_ASSERT(reinterpret_cast<i32>(hMouseCursor[cursorIndex]));
    }

    SetCursor(hMouseCursor[cursorIndex]);
    gInSetPointer = FALSE;
}

// The Windows build leaves the software-pointer hooks empty; these names
// follow the HoMM2 mouseManager methods with the same call arity.
VA(0x0046bf6f, 0xb)
void mouseManager::ReallyShowPointer(void) {}

// Descriptive name: an empty two-argument hook (`ret 8`).
// @dead-code
// Zero-ref: no incoming call, jump or relocated reference in retail.
VA(0x0046bf7a, 0xd)
void mouseManager::UnusedTwoArgumentHook1(i16, i16) {}

VA(0x0046bf87, 0xb)
void mouseManager::ReallyHidePointer(void) {}

VA(0x0046bf92, 0xb)
void mouseManager::HideColorPointer(void) {}

// townManager::DrawTown and advManager::UpdateScreen bracket a screen blit
// under the pointer with these hooks (Buka MiscRuntime's SaveAndDraw /
// RestoreUnderlying pair); retail keeps only the returns.
VA(0x0046bf9d, 0xb)
void mouseManager::RestoreUnderlying(void) {}

// HoMM2 Buka's SaveAndDraw(void), empty in the Windows build.
// @dead-code
// Zero-ref: no incoming call, jump or relocated reference in retail.
VA(0x0046bfa8, 0xb)
void mouseManager::SaveAndDraw(void) {}

// Descriptive name: an empty two-argument hook (`ret 8`).
// @dead-code
// Zero-ref: no incoming call, jump or relocated reference in retail.
VA(0x0046bfb3, 0xd)
void mouseManager::UnusedTwoArgumentHook2(i16, i16) {}

// advManager::UpdateScreen pushes the two origin words and a sign-extended
// cursor flag word.
VA(0x0046bfc0, 0xd)
void mouseManager::SaveAndDraw(bitmap*, i16, i16, i16) {}

// philAI's CheckDoMain still asks for a software pointer move; the Windows
// build ignores it (`ret 8`).
VA(0x0046bfcd, 0xd)
void mouseManager::MovePointer(i16, i16) {}

VA(0x0046bfda, 0xb)
void mouseManager::ShowColorPointer(void) {}

// townManager::Open forces a pointer refresh here; the Windows build keeps
// only the one-argument return.
VA(0x0046bfe5, 0xd)
void mouseManager::NewUpdate(i32) {}

VA(0x0046bff2, 0xd)
void mouseManager::WarpPointer(i16, i16) {}

// Descriptive name: an empty one-argument hook (`ret 4`).
// @dead-code
// Zero-ref: no incoming call, jump or relocated reference in retail.
VA(0x0046bfff, 0xd)
void mouseManager::UnusedOneArgumentHook(i32) {}

// donor PoL RVA 0x000c9ec0; preferred Buka symbol ?MouseCoords@mouseManager@@QAEXAAH0@Z
// donor Buka TU BASE/MOUSEMGR; HoMM1 owner inferred from contiguous order
// evidence: graph:4;base=0.387847;margin=0.576156;shape=0.081;size=0.906;calls=1.000;alternate=pol20:void mouseManager::MouseCoords(int &, int &)@0x000c9ec0
VA(0x0046c00c, 0x56)
void mouseManager::MouseCoords(i16& x, i16& y) {
    POINT point;

    GetCursorPos(&point);
    ScreenToClient(hwndApp, &point);
    x = CLIENT_TO_GAME_X(point.x);
    y = CLIENT_TO_GAME_Y(point.y);
}

VA(0x0046c062, 0xd)
void mouseManager::SetCursorShape(i32) {}

// advManager::Open passes the colour-pointer preference; Windows ignores it.
VA(0x0046c06f, 0xd)
void mouseManager::SetColorMice(i32) {}

VA(0x0046c07c, 0x13)
void mouseManager::HideSystemCursor(void) {
    ShowCursor(FALSE);
}

VA(0x0046c08f, 0x13)
void mouseManager::ShowSystemCursor(void) {
    ShowCursor(TRUE);
}

// Mouse-manager data, initialized from retail .data (0x004a0e70..) and
// zero-filled cursor tables (0x004cac88..).
DATA(0x004a0ff8)
i32 gMouseOffset[3] = {0, 40, 55};
DATA(0x004cfb40)
i32 gMouseCursorType = 0;
DATA(0x004a1004)
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
DATA(0x004cf30c)
HBITMAP hbmpColor[MOUSE_CURSOR_COUNT];
DATA(0x004ce178)
BITMAP bmpAndMask[MOUSE_CURSOR_COUNT];
DATA(0x004cef88)
HCURSOR hMouseCursor[MOUSE_CURSOR_COUNT];
DATA(0x004cfa14)
u8* cAndBits[MOUSE_CURSOR_COUNT];
DATA(0x004ce880)
BITMAP bmpColor[MOUSE_CURSOR_COUNT];
DATA(0x004cf1e0)
u8* gColorBits[MOUSE_CURSOR_COUNT];
DATA(0x004cf438)
ICONINFO mouseIconInfo[MOUSE_CURSOR_COUNT];
DATA(0x004cf0b4)
HBITMAP hbmpAndMask[MOUSE_CURSOR_COUNT];

// Located from HoMM2 Buka 2.1; PoL 2.0 supplies the VC4 declaration.

#include <match.h>

#include <BASE/Misc.h>
#include <BASE/MOUSEMGR_TYPES.h>
#include <H1/All.h>
#include <H1/KB.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

short gMouseManagerAssertLine = 232;
char gMouseManagerAssertFile1[] = "D:\\Heroes\\Base\\MOUSEMGR.CPP";
char gAdventureColor[] = "CO";
char gAdventureMonochrome[] = "BW";
char gAdventureBitmapFormat[] = "ADVM%s%02d.BMP";
char gSpellColor[] = "CO";
char gSpellMonochrome[] = "BW";
char gSpellBitmapFormat[] = "SPEL%s%02d.BMP";
char gCombatColor[] = "CO";
char gCombatMonochrome[] = "BW";
char gCombatBitmapFormat[] = "CMSE%s%02d.BMP";
char gMouseManagerAssertFile2[] = "D:\\Heroes\\Base\\MOUSEMGR.CPP";
char gMouseManagerAssertFile3[] = "D:\\Heroes\\Base\\MOUSEMGR.CPP";

// donor PoL RVA 0x000c9630; preferred Buka symbol ?SetPointer@mouseManager@@QAEXH@Z
// donor Buka TU BASE/MOUSEMGR; HoMM1 owner inferred from contiguous order
// evidence: graph:7;base=0.417606;margin=1.286688;shape=0.189;size=0.849;calls=0.737;alternate=pol20:void mouseManager::SetPointer(int)@0x000c9630
VA(0x00476940, 0x489)
void mouseManager::SetPointer(short frame)
{
    int cursorIndex;
    int x;
    int y;
    char filename[16];

    if (frame < 0 || m_active != 1)
        return;

    if (giCurExe == CONFIG_EXECUTABLE_EDITOR) {
        gMouseCursorType = MOUSE_CURSOR_ADVENTURE;
        if (frame > 0)
            frame = 0;
    }

    if (gbInSetPointer)
        return;
    gbInSetPointer = 1;

    if (frame == MOUSE_KEEP_CURRENT_FRAME)
        frame = m_cursorFrame;
    else
        m_cursorFrame = frame;

    cursorIndex = iMouseOffset[gMouseCursorType] + frame;
    ProcessAssert(
        cursorIndex >= 0 && cursorIndex < MOUSE_CURSOR_COUNT,
        gMouseManagerAssertFile1,
        gMouseManagerAssertLine + 34);

    if (hMouseCursor[cursorIndex] == 0) {
        cColorBits[cursorIndex] =
            static_cast<signed char *>(malloc(MOUSE_CURSOR_COLOR_BYTES));
        if (gbColorMice)
            cAndBits[cursorIndex] =
                static_cast<unsigned char *>(malloc(MOUSE_CURSOR_MASK_PLANE_BYTES));
        else
            cAndBits[cursorIndex] =
                static_cast<unsigned char *>(malloc(MOUSE_CURSOR_AND_BYTES));

        if (gMouseCursorType == MOUSE_CURSOR_ADVENTURE)
            sprintf(
                filename,
                gAdventureBitmapFormat,
                gbColorMice ? gAdventureColor : gAdventureMonochrome,
                frame + 1);
        else if (gMouseCursorType == MOUSE_CURSOR_SPELL)
            sprintf(
                filename,
                gSpellBitmapFormat,
                gbColorMice ? gSpellColor : gSpellMonochrome,
                frame + 1);
        else
            sprintf(
                filename,
                gCombatBitmapFormat,
                gbColorMice ? gCombatColor : gCombatMonochrome,
                frame + 1);

        gpResourceManager->PointToFile(gpResourceManager->MakeId(filename));
        gpResourceManager->ReadBlock(
            cColorBits[cursorIndex], MOUSE_CURSOR_BITMAP_HEADER_BYTES);
        gpResourceManager->ReadBlock(cColorBits[cursorIndex], MOUSE_CURSOR_COLOR_BYTES);
        memset(
            cAndBits[cursorIndex],
            0,
            gbColorMice ? MOUSE_CURSOR_MASK_PLANE_BYTES : MOUSE_CURSOR_AND_BYTES);

        for (y = MOUSE_CURSOR_BITMAP_BEGIN; y < MOUSE_CURSOR_BITMAP_END; y++) {
            for (x = MOUSE_CURSOR_BITMAP_BEGIN; x < MOUSE_CURSOR_BITMAP_END; x++) {
                if (gbSpecialMouseMasks && !gbColorMice) {
                    if (*(cColorBits[cursorIndex] + x
                          + y * MOUSE_CURSOR_BITMAP_WIDTH)
                        == 0)
                        *(cAndBits[cursorIndex] + y * MOUSE_CURSOR_MASK_ROW_BYTES
                          + (x >> MOUSE_CURSOR_MASK_SHIFT)) |=
                            1 << (MOUSE_CURSOR_MASK_HIGH_BIT
                                  - (x & MOUSE_CURSOR_MASK_HIGH_BIT));
                    else if (*(cColorBits[cursorIndex] + x
                               + y * MOUSE_CURSOR_BITMAP_WIDTH)
                             == 1)
                        *(cAndBits[cursorIndex] + MOUSE_CURSOR_MASK_PLANE_BYTES
                          + y * MOUSE_CURSOR_MASK_ROW_BYTES
                          + (x >> MOUSE_CURSOR_MASK_SHIFT)) |=
                            1 << (MOUSE_CURSOR_MASK_HIGH_BIT
                                  - (x & MOUSE_CURSOR_MASK_HIGH_BIT));
                } else {
                    if (*(cColorBits[cursorIndex] + x
                          + y * MOUSE_CURSOR_BITMAP_WIDTH)
                        == 0)
                        *(cAndBits[cursorIndex] + y * MOUSE_CURSOR_MASK_ROW_BYTES
                          + (x >> MOUSE_CURSOR_MASK_SHIFT)) |=
                            1 << (MOUSE_CURSOR_MASK_HIGH_BIT
                                  - (x & MOUSE_CURSOR_MASK_HIGH_BIT));
                    else if (!gbColorMice
                             && *(cColorBits[cursorIndex] + x
                                  + y * MOUSE_CURSOR_BITMAP_WIDTH)
                                    != 1)
                        *(cAndBits[cursorIndex] + MOUSE_CURSOR_MASK_PLANE_BYTES
                          + y * MOUSE_CURSOR_MASK_ROW_BYTES
                          + (x >> MOUSE_CURSOR_MASK_SHIFT)) |=
                            1 << (MOUSE_CURSOR_MASK_HIGH_BIT
                                  - (x & MOUSE_CURSOR_MASK_HIGH_BIT));
                }
            }
        }

        bmpAndMask[cursorIndex].bmType = 0;
        bmpAndMask[cursorIndex].bmWidth = MOUSE_CURSOR_BITMAP_WIDTH;
        bmpAndMask[cursorIndex].bmHeight =
            gbColorMice ? MOUSE_CURSOR_BITMAP_WIDTH : MOUSE_CURSOR_MASK_HEIGHT;
        bmpAndMask[cursorIndex].bmWidthBytes = MOUSE_CURSOR_MASK_ROW_BYTES;
        bmpAndMask[cursorIndex].bmPlanes = MOUSE_CURSOR_BITMAP_PLANES;
        bmpAndMask[cursorIndex].bmBitsPixel = MOUSE_CURSOR_BITMAP_BITS_PER_PIXEL;
        bmpAndMask[cursorIndex].bmWidthBytes = MOUSE_CURSOR_MASK_ROW_BYTES;
        bmpAndMask[cursorIndex].bmBits = cAndBits[cursorIndex];
        hbmpAndMask[cursorIndex] = CreateBitmapIndirect(&bmpAndMask[cursorIndex]);
        ProcessAssert(
            reinterpret_cast<int>(hbmpAndMask[cursorIndex]), // API-forced handle value.
            gMouseManagerAssertFile2,
            gMouseManagerAssertLine + 106);

        if (gbColorMice) {
            bmpColor[cursorIndex].bmType = 0;
            bmpColor[cursorIndex].bmWidth = MOUSE_CURSOR_BITMAP_WIDTH;
            bmpColor[cursorIndex].bmHeight = MOUSE_CURSOR_BITMAP_WIDTH;
            bmpColor[cursorIndex].bmPlanes = MOUSE_CURSOR_BITMAP_PLANES;
            bmpColor[cursorIndex].bmBitsPixel = 8;
            bmpColor[cursorIndex].bmWidthBytes = MOUSE_CURSOR_BITMAP_WIDTH;
            bmpColor[cursorIndex].bmBits = cColorBits[cursorIndex];
            hbmpColor[cursorIndex] = CreateBitmapIndirect(&bmpColor[cursorIndex]);
        }

        mouseIconInfo[cursorIndex].fIcon = 0;
        mouseIconInfo[cursorIndex].xHotspot =
            iHotSpot[cursorIndex][MOUSE_CURSOR_HORIZONTAL];
        mouseIconInfo[cursorIndex].yHotspot =
            iHotSpot[cursorIndex][MOUSE_CURSOR_VERTICAL];
        mouseIconInfo[cursorIndex].hbmMask = hbmpAndMask[cursorIndex];
        mouseIconInfo[cursorIndex].hbmColor =
            gbColorMice ? hbmpColor[cursorIndex] : 0;
        hMouseCursor[cursorIndex] = CreateIconIndirect(&mouseIconInfo[cursorIndex]);
        ProcessAssert(
            reinterpret_cast<int>(hMouseCursor[cursorIndex]), // API-forced handle value.
            gMouseManagerAssertFile3,
            gMouseManagerAssertLine + 127);
    }

    SetCursor(hMouseCursor[cursorIndex]);
    gbInSetPointer = 0;
}

// donor PoL RVA 0x000c9ec0; preferred Buka symbol ?MouseCoords@mouseManager@@QAEXAAH0@Z
// donor Buka TU BASE/MOUSEMGR; HoMM1 owner inferred from contiguous order
// evidence: graph:4;base=0.387847;margin=0.576156;shape=0.081;size=0.906;calls=1.000;alternate=pol20:void mouseManager::MouseCoords(int &, int &)@0x000c9ec0
VA(0x00476e60, 0x5a)
void mouseManager::MouseCoords(short &x, short &y)
{
    POINT point;

    GetCursorPos(&point);
    ScreenToClient(hwndApp, &point);
    x = point.x * 640 / iMainWinScreenWidth;
    y = point.y * 480 / iMainWinScreenHeight;
}

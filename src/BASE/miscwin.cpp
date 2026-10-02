// Retail screen blitting, corresponding to Buka/PoL miscwin.

#define WIN32_LEAN_AND_MEAN

#include <match.h>

#include <windows.h>

#include <BASE/bitmap.h>
#include <BASE/bmap2.h>
#include <BASE/heroWindowManager.h>
#include <BASE/Misc.h>
#include <BASE/MISC_TYPES.h>
#include <BASE/palette.h>
#include <H1/KB.h>
#include <H1/Types.h>
#include <SOURCE/wingraph.h>
#include <SOURCE/X_GLOBAL.h>

#include <string.h>

#pragma intrinsic(memcpy, memset)

VA(0x00473450, 0x199)
void BlitBitmapToScreen(bitmap *sourceBitmap, int sourceX, int sourceY, int width, int height, int destinationX, int destinationY)
{
    if (gpWindowManager->m_screen != sourceBitmap) {
        for (int row = 0; row < height; row++)
            memcpy(gpWindowManager->m_screen->m_pixels + (destinationY + row) * SCREEN_BLIT_WIDTH + destinationX,
                sourceBitmap->m_pixels + (row + sourceY) * sourceBitmap->m_width + sourceX, width);
    }
    if (gbEnlargeScreenBlit != 0) {
        if (iMainWinScreenWidth == SCREEN_BLIT_WIDTH && iMainWinScreenHeight == SCREEN_BLIT_HEIGHT) {
            if (width < SCREEN_BLIT_WIDTH_END) width++;
            if (height < SCREEN_BLIT_WIDTH_END) height++;
        } else {
            if (destinationX > 0) destinationX--;
            if (destinationY > 0) destinationY--;
            if (width < SCREEN_BLIT_ENLARGE_END) width += SCREEN_BLIT_ENLARGE_PIXELS;
            if (height < SCREEN_BLIT_ENLARGE_END) height += SCREEN_BLIT_ENLARGE_PIXELS;
        }
    }
    RECT invalidRectangle;
    invalidRectangle.left = destinationX * iMainWinScreenWidth / SCREEN_BLIT_WIDTH;
    invalidRectangle.top = destinationY * iMainWinScreenHeight / SCREEN_BLIT_HEIGHT;
    invalidRectangle.right = (destinationX + width) * iMainWinScreenWidth / SCREEN_BLIT_WIDTH - 1;
    invalidRectangle.bottom = (destinationY + height) * iMainWinScreenHeight / SCREEN_BLIT_HEIGHT - 1;
    if (InvalidateRect(hwndApp, &invalidRectangle, FALSE) == FALSE)
        LogStr("InvalidateRect Failed");
    if (UpdateWindow(hwndApp) == FALSE)
        LogStr("UpdateWindow Failed");
}

VA(0x004735f0, 0x30)
void GrabScreenBitmap(bitmap *destination, int x, int y)
{
    BlitBitmap(gpWindowManager->m_screen, x, y, destination->m_width, destination->m_height, destination, 0, 0);
}


VA(0x00473620, 0x45)
void SetPalette(signed char *paletteData, int updateDisplay)
{
    memcpy(gpBufferPalette->m_data, paletteData, PALETTE_GRAPHICS_BYTES);
    memcpy(gCyclePal, paletteData + PALETTE_CYCLE_FIRST * PALETTE_GRAPHICS_CHANNELS, sizeof(gCyclePal));
    if (updateDisplay != 0)
        UpdatePalette(gpBufferPalette->m_data);
}

VA(0x00473670, 0xdd)
void FadeIn(int increment)
{
    signed char done;
    int i, j, threshold;
    palette *currentPalette = new palette;
    if (currentPalette == NULL)
        MemError();
    done = 0;
    memset(currentPalette->m_data, 0, PALETTE_GRAPHICS_BYTES);
    if (gConfig.gfx[giCurExe].fullScreen == 0)
        increment *= PALETTE_WINDOWED_FADE_SCALE;
    for (i = 0; i < PALETTE_FADE_LEVEL_END; i += increment) {
    fadeStep:
        PollSound();
        if (i == PALETTE_FADE_LEVEL_LAST) {
            done = 1;
            UpdatePalette(gpBufferPalette->m_data);
        } else {
            threshold = PALETTE_FADE_LEVEL_LAST - i;
            for (j = 0; j < PALETTE_GRAPHICS_END; j++) {
                if (gpBufferPalette->m_data[j] > threshold)
                    currentPalette->m_data[j] = gpBufferPalette->m_data[j] - threshold;
            }
            UpdatePalette(currentPalette->m_data);
        }
    }
    if (done == 0) {
        i = PALETTE_FADE_LEVEL_LAST;
        goto fadeStep;
    }
    delete currentPalette;
}

VA(0x00473750, 0xcd)
void FadeOut(int increment)
{
    signed char done;
    int i, j;
    palette *currentPalette = new palette;
    if (currentPalette == NULL)
        MemError();
    done = 0;
    if (gConfig.gfx[giCurExe].fullScreen == 0)
        increment *= PALETTE_WINDOWED_FADE_SCALE;
    memcpy(currentPalette->m_data, gpBufferPalette->m_data, PALETTE_GRAPHICS_BYTES);
    for (i = 0; i < PALETTE_FADE_LEVEL_END; i += increment) {
    fadeStep:
        PollSound();
        if (i == PALETTE_FADE_LEVEL_LAST)
            done = 1;
        for (j = 0; j < PALETTE_GRAPHICS_END; j++) {
            if (currentPalette->m_data[j] > 0) {
                if (currentPalette->m_data[j] > increment)
                    currentPalette->m_data[j] -= increment;
                else
                    currentPalette->m_data[j] = 0;
            }
        }
        UpdatePalette(currentPalette->m_data);
    }
    if (done == 0) {
        i = PALETTE_FADE_LEVEL_LAST;
        goto fadeStep;
    }
    delete currentPalette;
}

// ---------------------------------------------------------------------------
// The rest of this object. Retail places OLDASM's helpers, the clipped icon
// renderers and PrintMemoryLeaks in the same object as the blitters: VC4
// LINK pulls a library member once per object in first-reference order, and
// only a shared object reproduces retail's BASE order (this group first,
// although the first symbol SOURCE references is Random). The OLDASM assert
// keeps its own file name (retail 0x004a0c44).
// ---------------------------------------------------------------------------

// HoMM1 OLDASM.CPP helpers; the assert literal names the retail source file.


#include <BASE/bitmap.h>
#include <BASE/icon.h>
#include <BASE/Misc.h>
#include <BASE/MISC_TYPES.h>

#include <stdlib.h>
#include <string.h>

#pragma intrinsic(memcpy, memset)

struct PaletteColor {
    unsigned char red;
    unsigned char green;
    unsigned char blue;
};

short gOldAsmAssertLine = 207;
char gOldAsmAssertFile[] = "D:\\Heroes\\Base\\OLDASM.CPP";

VA(0x00473820, 0x3a)
int Random(int low, int high) {
    ProcessAssert(high > low, gOldAsmAssertFile, gOldAsmAssertLine + 1);
    return rand() % (high - low + 1) + low;
}

// Called on the loaded kb.pal data before SetPalette.
VA(0x00473860, 0x60)
void PostprocessPalette(signed char* data) {
    PaletteColor* remapped = static_cast<PaletteColor*>(malloc(PALETTE_GRAPHICS_BYTES));
    memset(remapped, 0, PALETTE_GRAPHICS_BYTES);
    for (int index = 0; index < 256; index++)
        remapped[gMonoColorMap[index]] =
            reinterpret_cast<PaletteColor*>(data)[index]; // byte-evidenced: 3-byte colour copies
    memcpy(data, remapped, PALETTE_GRAPHICS_BYTES);
    free(remapped);
}

VA(0x004738c0, 0x1)
void PostprocessBitmap(signed char*, int, int) {}

VA(0x004738d0, 0x1)
void PostprocessIcon(icon*) {}

// HoMM1's C++ mono clipping path, corresponding to donor Iconm2b.cpp.


#include <BASE/bitmap.h>
#include <BASE/icon.h>
#include <BASE/IconEntry.h>
#include <BASE/Iconm2b.h>

#include <string.h>

#pragma intrinsic(memcpy, memset)

VA(0x004738e0, 0x1e6)
void ClippedMonoIconToBitmap(icon *sourceIcon, bitmap *destination, int x, int y, int frame, int color, int mode, int clipX, int clipY, int clipW, int clipH)
{
    int clipRight = clipX + clipW - 1;
    int clipBottom = clipY + clipH - 1;
    IconEntry *entry = reinterpret_cast<IconEntry *>(sourceIcon->m_data) + frame; // byte-evidenced: packed frame directory decoded from resource bytes.
    unsigned char *source = sourceIcon->m_data + entry->srcOffset;
    int position = x + entry->x;
    int drawing = 1;
    int row = y + entry->y;
    int rowOffset = row * ICON_SCREEN_ROW_BYTES;
    while (drawing != 0) {
        unsigned char run = *source;
        if (static_cast<signed char>(run) < 0) {
            run &= ICON_MONO_SKIP_MASK;
            if (run != 0) {
                position += run;
                source++;
            } else
                drawing = 0;
        } else if (run != ICON_MONO_NEWLINE_COMMAND) {
            if (row >= clipY && row <= clipBottom && position + run >= clipX && position <= clipRight) {
                if (position >= clipX) {
                    if (position + run <= clipRight)
                        memset(destination->m_pixels + rowOffset + position, color, run);
                    else
                        memset(destination->m_pixels + rowOffset + position, color, clipRight - position + 1);
                } else {
                    if (position + run <= clipRight)
                        memset(destination->m_pixels + rowOffset + clipX, color, position + run - clipX);
                    else
                        memset(destination->m_pixels + rowOffset + clipX, color, clipW);
                }
            }
            position += *source;
            source++;
        } else {
            position = x + entry->x;
            rowOffset += ICON_SCREEN_ROW_BYTES;
            row++;
            source++;
        }
    }
}

// Clipped colour icon blit kept beside the mono path. Retail keeps every
// working value in file statics, as in the assembly renderers. Their
// declaration order sets the compare operand sort keys; the .bss layout
// follows the names, not this order.
static int sClipY;
static int sClipBottom;
static int sClipX;
static unsigned int sClipRun;
static int sClipRowStart;
static signed char *sClipRow;
static IconEntry *sClipEntry;
static unsigned char *sClipSource;
static int sClipRight;
static int sClipInside;

VA(0x00473ad0, 0x2ad)
void ClipIconToBitmap(icon *sourceIcon, bitmap *destination, int x, int y, int frame, int mode, int clipX, int clipY, int clipW, int clipH)
{
    sClipEntry = reinterpret_cast<IconEntry *>(sourceIcon->m_data) + frame; // byte-evidenced: packed frame directory decoded from resource bytes.
    sClipSource = sourceIcon->m_data + sClipEntry->srcOffset;
    sClipX = sClipRowStart = x + sClipEntry->x;
    sClipY = y + sClipEntry->y;
    if (sClipRowStart < clipX || sClipRowStart + sClipEntry->w > clipX + clipW
        || sClipY < clipY || sClipY + sClipEntry->h > clipY + clipH) {
        sClipInside = 0;
        sClipRight = clipX + clipW - 1;
        sClipBottom = clipY + clipH - 1;
    } else {
        sClipInside = 1;
    }
    sClipRow = destination->m_pixels + destination->m_width * sClipY;
    for (;;) {
        sClipRun = *sClipSource++;
        if (static_cast<signed char>(sClipRun) < 0) {
            if ((sClipRun & ICON_MONO_SKIP_MASK) == 0)
                return;
            sClipX += sClipRun & ICON_MONO_SKIP_MASK;
            continue;
        }
        if (sClipRun != 0) {
            if (sClipInside) {
                memcpy(sClipRow + sClipX, sClipSource, sClipRun);
            } else if (sClipY >= clipY && sClipBottom >= sClipY && sClipRun + sClipX >= clipX
                       && sClipX <= sClipRight) {
                if (sClipX >= clipX) {
                    if (sClipRight >= sClipX + sClipRun)
                        memcpy(sClipRow + sClipX, sClipSource, sClipRun);
                    else
                        memcpy(sClipRow + sClipX, sClipSource, sClipRight - sClipX + 1);
                } else {
                    if (*sClipSource + sClipX <= sClipRight)
                        memcpy(sClipRow + sClipX, sClipSource, sClipRun - clipX + sClipX);
                    else
                        memcpy(sClipRow + sClipX, sClipSource, clipW);
                }
            }
            sClipX += sClipRun;
            sClipSource += sClipRun;
        } else {
            sClipX = sClipRowStart;
            sClipY++;
            sClipRow += destination->m_width;
        }
    }
}

// int3-delimited single-function TU between Iconm2bClip and BASEMGR; KB's
// ShutDown calls it. Buka keeps a debug-heap report here; HoMM1 retail ships
// the empty release body.


VA(0x00473d80, 0x1)
void PrintMemoryLeaks(void) {}

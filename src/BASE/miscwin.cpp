// Retail screen blitting, corresponding to Buka/PoL miscwin.

#define WIN32_LEAN_AND_MEAN

#include <match.h>

#include <BASE/miscwin.h>

#include <windows.h>

#include <BASE/bitmap.h>
#include <BASE/bmap2.h>
#include <BASE/display.h>
#include <BASE/heroWindowManager.h>
#include <BASE/Misc.h>
#include <BASE/palette.h>
#include <SOURCE/KB.h>
#include <SOURCE/kbwin.h>
#include <SOURCE/wingraph.h>

#include <string.h>

VA(0x00475c40, 0x199)
void BlitBitmapToScreen(
    bitmap* sourceBitmap,
    i32 sourceX,
    i32 sourceY,
    i32 width,
    i32 height,
    i32 destinationX,
    i32 destinationY
) {
    if (gpWindowManager->m_screen != sourceBitmap) {
        for (i32 row = 0; row < height; row++)
            memcpy(
                gpWindowManager->m_screen->m_pixels + (destinationY + row) * SCREEN_BLIT_WIDTH
                    + destinationX,
                sourceBitmap->m_pixels + (row + sourceY) * sourceBitmap->m_width + sourceX,
                width
            );
    }
    if (gEnlargeScreenBlit != 0) {
        if (iMainWinScreenWidth == SCREEN_BLIT_WIDTH
            && gMainWinScreenHeight == SCREEN_BLIT_HEIGHT) {
            if (width < SCREEN_BLIT_WIDTH_END)
                width++;
            if (height < SCREEN_BLIT_WIDTH_END)
                height++;
        } else {
            if (destinationX > 0)
                destinationX--;
            if (destinationY > 0)
                destinationY--;
            if (width < SCREEN_BLIT_ENLARGE_END)
                width += SCREEN_BLIT_ENLARGE_PIXELS;
            if (height < SCREEN_BLIT_ENLARGE_END)
                height += SCREEN_BLIT_ENLARGE_PIXELS;
        }
    }
    RECT invalidRectangle;
    invalidRectangle.left = destinationX * iMainWinScreenWidth / SCREEN_BLIT_WIDTH;
    invalidRectangle.top = destinationY * gMainWinScreenHeight / SCREEN_BLIT_HEIGHT;
    invalidRectangle.right = (destinationX + width) * iMainWinScreenWidth / SCREEN_BLIT_WIDTH - 1;
    invalidRectangle.bottom =
        (destinationY + height) * gMainWinScreenHeight / SCREEN_BLIT_HEIGHT - 1;
    if (InvalidateRect(hwndApp, &invalidRectangle, FALSE) == FALSE)
        LogStr("InvalidateRect Failed");
    if (UpdateWindow(hwndApp) == FALSE)
        LogStr("UpdateWindow Failed");
}

VA(0x00475de0, 0x30)
void GrabScreenBitmap(bitmap* destination, i32 x, i32 y) {
    BlitBitmap(
        gpWindowManager->m_screen,
        x,
        y,
        destination->m_width,
        destination->m_height,
        destination,
        0,
        0
    );
}

VA(0x00475e10, 0x45)
void SetPalette(i8* paletteData, i32 updateDisplay) {
    memcpy(gpBufferPalette->m_data, paletteData, PALETTE_GRAPHICS_BYTES);
    memcpy(
        gCyclePal,
        paletteData + PALETTE_CYCLE_FIRST * PALETTE_GRAPHICS_CHANNELS,
        sizeof(gCyclePal)
    );
    if (updateDisplay != 0)
        UpdatePalette(gpBufferPalette->m_data);
}

VA(0x00475e60, 0xdd)
void FadeIn(i32 increment) {
    i8 done;
    i32 i, j, threshold;
    palette* currentPalette = new palette;
    if (currentPalette == NULL)
        MemError();
    done = 0;
    memset(currentPalette->m_data, 0, PALETTE_GRAPHICS_BYTES);
    if (gConfig.gfx[gCurExe].fullScreen == 0)
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

VA(0x00475f40, 0xcd)
void FadeOut(i32 increment) {
    i8 done;
    i32 i, j;
    palette* currentPalette = new palette;
    if (currentPalette == NULL)
        MemError();
    done = 0;
    if (gConfig.gfx[gCurExe].fullScreen == 0)
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
// keeps its own file name (retail 0x004a0838).
// ---------------------------------------------------------------------------

// HoMM1 OLDASM.CPP helpers; the assert literal names the retail source file.

#include <BASE/bitmap.h>
#include <BASE/icon.h>
#include <BASE/Misc.h>

#include <stdlib.h>
#include <string.h>

struct PaletteColor {
    u8 red;
    u8 green;
    u8 blue;
};

VA(0x00476010, 0x3a)
#line 207 "F:\\H1w95src\\Base\\OLDASM.CPP"
i32 Random(i32 low, i32 high) {
#line 208
    H1_ASSERT(high > low);
    return rand() % (high - low + 1) + low;
}

// Called on the loaded kb.pal data before SetPalette.
VA(0x00476050, 0x60)
void PostprocessPalette(i8* data) {
    PaletteColor* remapped = static_cast<PaletteColor*>(malloc(PALETTE_GRAPHICS_BYTES));
    memset(remapped, 0, PALETTE_GRAPHICS_BYTES);
    for (i32 index = 0; index < PALETTE_COLOR_COUNT; index++)
        remapped[gMonoColorMap[index]] =
            reinterpret_cast<PaletteColor*>(data)[index]; // byte-evidenced: 3-byte colour copies
    memcpy(data, remapped, PALETTE_GRAPHICS_BYTES);
    free(remapped);
}

VA(0x004760b0, 0x1)
void PostprocessBitmap(i8*, i32, i32) {}

VA(0x004760c0, 0x1)
void PostprocessIcon(icon*) {}

// HoMM1's C++ mono clipping path, corresponding to donor Iconm2b.cpp.

#include <BASE/bitmap.h>
#include <BASE/icon.h>
#include <BASE/IconEntry.h>
#include <BASE/Iconm2b.h>

#include <string.h>

VA(0x004760d0, 0x1e6)
void ClippedMonoIconToBitmap(
    icon* sourceIcon,
    bitmap* destination,
    i32 x,
    i32 y,
    i32 frame,
    i32 color,
    i32 mode,
    i32 clipX,
    i32 clipY,
    i32 clipW,
    i32 clipH
) {
    i32 clipRight = clipX + clipW - 1;
    i32 clipBottom = clipY + clipH - 1;
    IconEntry* entry =
        reinterpret_cast<IconEntry*>(sourceIcon->m_data)
        + frame; // byte-evidenced: packed frame directory decoded from resource bytes.
    u8* source = sourceIcon->m_data + entry->srcOffset;
    i32 position = x + entry->x;
    BOOL drawing = TRUE;
    i32 row = y + entry->y;
    i32 rowOffset = row * ICON_SCREEN_ROW_BYTES;
    while (drawing) {
        u8 run = *source;
        if (static_cast<i8>(run) < 0) {
            run &= ICON_MONO_SKIP_MASK;
            if (run != 0) {
                position += run;
                source++;
            } else
                drawing = FALSE;
        } else if (run != ICON_MONO_NEWLINE_COMMAND) {
            if (row >= clipY && row <= clipBottom && position + run >= clipX
                && position <= clipRight) {
                if (position >= clipX) {
                    if (position + run <= clipRight)
                        memset(destination->m_pixels + rowOffset + position, color, run);
                    else
                        memset(
                            destination->m_pixels + rowOffset + position,
                            color,
                            clipRight - position + 1
                        );
                } else {
                    if (position + run <= clipRight)
                        memset(
                            destination->m_pixels + rowOffset + clipX,
                            color,
                            position + run - clipX
                        );
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
DATA(0x004cf52c)
static i32 sClipY;
DATA(0x004cf530)
static i32 sClipBottom;
DATA(0x004cf520)
static i32 sClipRowStart;
DATA(0x004cf518)
static i8* sClipRow;
DATA(0x004cf4c8)
static IconEntry* sClipEntry;
DATA(0x004cf51c)
static u8* sClipSource;
DATA(0x004cf524)
static i32 sClipRight;
DATA(0x004cf4d0)
static i32 sClipX;
DATA(0x004cf4cc)
static u32 sClipRun;
DATA(0x004cf528)
static BOOL sClipInside;

VA(0x004762c0, 0x2a8)
void ClipIconToBitmap(
    icon* sourceIcon,
    bitmap* destination,
    i32 x,
    i32 y,
    i32 frame,
    i32 mode,
    i32 clipX,
    i32 clipY,
    i32 clipW,
    i32 clipH
) {
    sClipEntry = reinterpret_cast<IconEntry*>(sourceIcon->m_data)
                 + frame; // byte-evidenced: packed frame directory decoded from resource bytes.
    sClipSource = sourceIcon->m_data + sClipEntry->srcOffset;
    sClipX = sClipRowStart = x + sClipEntry->x;
    sClipY = y + sClipEntry->y;
    if (sClipRowStart < clipX || sClipRowStart + sClipEntry->w > clipX + clipW || sClipY < clipY
        || sClipY + sClipEntry->h > clipY + clipH) {
        sClipInside = FALSE;
        sClipRight = clipX + clipW - 1;
        sClipBottom = clipY + clipH - 1;
    } else {
        sClipInside = TRUE;
    }
    sClipRow = destination->m_pixels + destination->m_width * sClipY;
    for (;;) {
        sClipRun = *sClipSource++;
        if (static_cast<i8>(sClipRun) < 0) {
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

VA(0x00476570, 0x1)
void PrintMemoryLeaks(void) {}

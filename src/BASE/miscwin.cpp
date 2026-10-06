// Retail screen blitting.

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

// Retail assertion paths: each program's BASE objects were compiled in its own
// checkout (HEROES.EXE and EDITOR.EXE assertion strings).
#ifdef HOMM1_EDITOR
#define OLDASM_CPP_PATH "U:\\HMM\\VSS\\HMM1\\Source\\Base\\OLDASM.CPP"
#else
#define OLDASM_CPP_PATH "E:\\Users\\igorl\\VSS\\HMM\\HMM1\\Source\\Base\\OLDASM.CPP"
#endif

VA(0x0046f870, 0x185)
void BlitBitmapToScreen(
    bitmap* sourceBitmap,
    i32 sourceX,
    i32 sourceY,
    i32 width,
    i32 height,
    i32 destinationX,
    i32 destinationY
) {
    if (sourceBitmap != gWindowManager->m_screen) {
        for (i32 row = 0; row < height; row++)
            memcpy(
                gWindowManager->m_screen->m_pixels + (destinationY + row) * LOGICAL_SCREEN_WIDTH
                    + destinationX,
                sourceBitmap->m_pixels + (row + sourceY) * sourceBitmap->m_width + sourceX,
                width
            );
    }
    if (gEnlargeScreenBlit != 0) {
        if (gMainWinScreenWidth == LOGICAL_SCREEN_WIDTH
            && gMainWinScreenHeight == LOGICAL_SCREEN_HEIGHT) {
            if (width < LOGICAL_SCREEN_WIDTH)
                width++;
            if (height < LOGICAL_SCREEN_WIDTH)
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
    invalidRectangle.left = destinationX * gMainWinScreenWidth / LOGICAL_SCREEN_WIDTH;
    invalidRectangle.top = destinationY * gMainWinScreenHeight / LOGICAL_SCREEN_HEIGHT;
    invalidRectangle.right =
        (destinationX + width) * gMainWinScreenWidth / LOGICAL_SCREEN_WIDTH - 1;
    invalidRectangle.bottom =
        (destinationY + height) * gMainWinScreenHeight / LOGICAL_SCREEN_HEIGHT - 1;
    InvalidateRect(gAppWindow, &invalidRectangle, FALSE);
    UpdateWindow(gAppWindow);
}

VA(0x0046f9f5, 0x37)
void GrabScreenBitmap(bitmap* destination, i32 x, i32 y) {
    BlitBitmap(
        gWindowManager->m_screen,
        x,
        y,
        destination->m_width,
        destination->m_height,
        destination,
        0,
        0
    );
}

// Blit a whole bitmap to the screen origin.
VA(0x0046fa2c, 0x29)
void BitmapToScreen(bitmap* image) {
    BlitBitmapToScreen(image, 0, 0, image->m_width, image->m_height, 0, 0);
}

VA(0x0046fa55, 0x50)
void SetPalette(i8* paletteData, i32 updateDisplay) {
    memcpy(gBufferPalette->m_data, paletteData, PALETTE_DATA_SIZE);
    memcpy(
        gCyclePal,
        paletteData + PALETTE_CYCLE_FIRST * PALETTE_GRAPHICS_CHANNELS,
        sizeof(gCyclePal)
    );
    if (updateDisplay != 0)
        UpdatePalette(gBufferPalette->m_data);
}

VA(0x0046faa5, 0x16d)
void FadeIn(i32 increment) throw() {
    bool done;
    i32 i, j, threshold;
    palette* pal = new palette;
    if (pal == NULL)
        MemError();
    done = false;
    memset(pal->m_data, 0, PALETTE_DATA_SIZE);
    if (CURRENT_GRAPHICS_CONFIG.fullScreen == 0)
        increment *= PALETTE_WINDOWED_FADE_SCALE;
    for (i = 0; i < PALETTE_FADE_LEVEL_END; i += increment) {
    fadeStep:
        PollSound();
        if (i == PALETTE_FADE_LEVEL_LAST) {
            done = true;
            UpdatePalette(gBufferPalette->m_data);
        } else {
            threshold = PALETTE_FADE_LEVEL_LAST - i;
            for (j = 0; j < PALETTE_DATA_SIZE; j++) {
                if (gBufferPalette->m_data[j] > threshold)
                    pal->m_data[j] = gBufferPalette->m_data[j] - threshold;
            }
            UpdatePalette(pal->m_data);
        }
    }
    if (done == false) {
        i = PALETTE_FADE_LEVEL_LAST;
        goto fadeStep;
    }
    delete pal;
}

VA(0x0046fc12, 0x170)
void FadeOut(i32 increment) throw() {
    bool done;
    i32 i, j;
    palette* pal = new palette;
    if (pal == NULL)
        MemError();
    done = false;
    if (CURRENT_GRAPHICS_CONFIG.fullScreen == 0)
        increment *= PALETTE_WINDOWED_FADE_SCALE;
    memcpy(pal->m_data, gBufferPalette->m_data, PALETTE_DATA_SIZE);
    for (i = 0; i < PALETTE_FADE_LEVEL_END; i += increment) {
    fadeStep:
        PollSound();
        if (i == PALETTE_FADE_LEVEL_LAST)
            done = true;
        for (j = 0; j < PALETTE_DATA_SIZE; j++) {
            if (pal->m_data[j] > 0) {
                if (pal->m_data[j] > increment)
                    pal->m_data[j] -= increment;
                else
                    pal->m_data[j] = 0;
            }
        }
        UpdatePalette(pal->m_data);
    }
    if (done == false) {
        i = PALETTE_FADE_LEVEL_LAST;
        goto fadeStep;
    }
    delete pal;
}

// ---------------------------------------------------------------------------
// The rest of this object. Retail places OLDASM's helpers and the clipped icon
// renderers in the same object as the blitters. The OLDASM assert keeps its
// own file name (retail 0x004a0838).
// ---------------------------------------------------------------------------

// HoMM1 OLDASM.CPP helpers; the assert literal names the retail source file.

#include <BASE/bitmap.h>
#include <BASE/icon.h>
#include <BASE/Misc.h>

#include <stdlib.h>
#include <string.h>

// The Windows build has no SVGA mode to set up.
// @dead-code
// Zero-ref: no incoming call, jump or relocated reference in retail.
VA(0x0046fd82, 0x8)
i16 AutoInitSVGA(void) {
    return 0;
}

VA(0x0046fd8a, 0x39)
#line 207 OLDASM_CPP_PATH
i32 Random(i32 low, i32 high) {
#line 191
    H1_ASSERT(high > low);
    return rand() % (high - low + 1) + low;
}

// Called on the loaded kb.pal data before SetPalette.
VA(0x0046fdc3, 0x95)
void PostprocessPalette(i8* paletteData) {
    PaletteColor* remapped = static_cast<PaletteColor*>(malloc(PALETTE_DATA_SIZE));
    memset(remapped, 0, PALETTE_DATA_SIZE);
    for (i32 index = 0; index < PALETTE_COLOR_COUNT; index++)
        memcpy(
            &remapped[gMonoColorMap[index]],
            // byte-evidenced: RGB triples of the raw palette.
            &reinterpret_cast<PaletteColor*>(paletteData)[index],
            sizeof(PaletteColor)
        );
    memcpy(paletteData, remapped, PALETTE_DATA_SIZE);
    free(remapped);
}

VA(0x0046fe58, 0x5)
void PostprocessBitmap(u8* pixels, i32 width, i32 height) {}

VA(0x0046fe5d, 0x5)
void PostprocessIcon(icon* loadedIcon) {}

// HoMM1's C++ mono clipping path.

#include <BASE/bitmap.h>
#include <BASE/icon.h>
#include <BASE/Iconm2b.h>

#include <string.h>

VA(0x0046fe62, 0x214)
void ClippedMonoIconToBitmap(
    icon* sourceIcon,
    bitmap* destination,
    i32 x,
    i32 y,
    i32 frame,
    i32 color,
    i32 offsetMode,
    i32 clipX,
    i32 clipY,
    i32 clipW,
    i32 clipH
) {
    i32 clipRight = clipX + clipW - 1;
    i32 clipLast = clipY + clipH - 1;
    IconEntry* entry = sourceIcon->m_frames + frame;
    u8* source = sourceIcon->m_data + entry->srcOffset;
    i32 curX = x + entry->x;
    i32 curY = y + entry->y;
    BOOL decoding = TRUE;
    while (decoding) {
        if (static_cast<i8>(*source) < 0) {
            if ((*source & ICON_MONO_SKIP_MASK) != 0) {
                curX += *source & ICON_MONO_SKIP_MASK;
                source++;
            } else
                decoding = FALSE;
        } else if (*source != ICON_MONO_NEWLINE_COMMAND) {
            if (curY >= clipY && curY <= clipLast && curX + *source >= clipX && curX <= clipRight) {
                if (curX >= clipX) {
                    if (curX + *source <= clipRight)
                        memset(
                            destination->m_pixels + curX + curY * LOGICAL_SCREEN_WIDTH,
                            color,
                            *source
                        );
                    else
                        memset(
                            destination->m_pixels + curX + curY * LOGICAL_SCREEN_WIDTH,
                            color,
                            clipRight - curX + 1
                        );
                } else {
                    if (curX + *source <= clipRight)
                        memset(
                            destination->m_pixels + clipX + curY * LOGICAL_SCREEN_WIDTH,
                            color,
                            curX + *source - clipX
                        );
                    else
                        memset(
                            destination->m_pixels + clipX + curY * LOGICAL_SCREEN_WIDTH,
                            color,
                            clipW
                        );
                }
            }
            curX += *source;
            source++;
        } else {
            curX = x + entry->x;
            curY++;
            source++;
        }
    }
}

// Clipped colour icon blit kept beside the mono path. Retail keeps every
// working value in file statics, as in the assembly renderers.
#define sClipY sClipPosY // spelling fixes .bss order
DATA(0x004cfb50)
static i32 sClipY;
DATA(0x004cfb58)
static i32 sClipLimitY;
#define sClipRowStart sClipLeft // spelling fixes .bss order
DATA(0x004cfbb4)
static i32 sClipRowStart;
DATA(0x004cfb64)
static u8* sClipRow;
DATA(0x004cfb68)
static IconEntry* sClipFrameEntry;
#define sClipSource sClipSrcPtr // spelling fixes .bss order
DATA(0x004cfb5c)
static u8* sClipSource;
DATA(0x004cfb54)
static i32 sClipLimitX;
DATA(0x004cfb60)
static i32 sClipX;
DATA(0x004cfb6c)
static u32 sClipRun;
DATA(0x004cfbb0)
static BOOL sClipInside;

VA(0x00470076, 0x307)
void ClipIconToBitmap(
    icon* sourceIcon,
    bitmap* destination,
    i32 x,
    i32 y,
    i32 frame,
    i32 offsetMode,
    i32 clipX,
    i32 clipY,
    i32 clipW,
    i32 clipH
) {
    sClipFrameEntry = sourceIcon->m_frames + frame;
    sClipSource = sourceIcon->m_data + sClipFrameEntry->srcOffset;
    sClipX = sClipRowStart = x + sClipFrameEntry->x;
    sClipY = y + sClipFrameEntry->y;
    if (ICON_FITS_CLIP(
            sClipRowStart,
            sClipY,
            sClipFrameEntry->w,
            sClipFrameEntry->h,
            clipX,
            clipY,
            clipW,
            clipH
        )) {
        sClipInside = TRUE;
    } else {
        sClipInside = FALSE;
        sClipLimitX = clipX + clipW - 1;
        sClipLimitY = clipY + clipH - 1;
    }
    sClipRow = destination->m_pixels + sClipY * destination->m_width;
    for (;;) {
        sClipRun = *sClipSource++;
        if (static_cast<i8>(sClipRun) < 0) {
            if (sClipRun & ICON_MONO_SKIP_MASK)
                sClipX += sClipRun & ICON_MONO_SKIP_MASK;
            else
                break;
        } else if (sClipRun != 0) {
            if (sClipInside) {
                memcpy(sClipRow + sClipX, sClipSource, sClipRun);
            } else if (sClipY >= clipY && sClipY <= sClipLimitY && sClipX + sClipRun >= clipX
                       && sClipX <= sClipLimitX) {
                if (sClipX >= clipX) {
                    if (sClipX + sClipRun <= sClipLimitX)
                        memcpy(sClipRow + sClipX, sClipSource, sClipRun);
                    else
                        memcpy(sClipRow + sClipX, sClipSource, sClipLimitX - sClipX + 1);
                } else {
                    if (sClipX + *sClipSource <= sClipLimitX)
                        memcpy(sClipRow + sClipX, sClipSource, sClipX + sClipRun - clipX);
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

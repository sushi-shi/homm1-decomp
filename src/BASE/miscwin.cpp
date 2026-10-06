#include <H1/Ints.h>

#include <BASE/miscwin.h>

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

#ifdef HOMM1_EDITOR
#define OLDASM_CPP_PATH "U:\\HMM\\VSS\\HMM1\\Source\\Base\\OLDASM.CPP"
#else
#define OLDASM_CPP_PATH "E:\\Users\\igorl\\VSS\\HMM\\HMM1\\Source\\Base\\OLDASM.CPP"
#endif

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
    if (gEnlargeScreenBlit != false) {
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
    KBPaintScreen(
        destinationX * gMainWinScreenWidth / LOGICAL_SCREEN_WIDTH,
        destinationY * gMainWinScreenHeight / LOGICAL_SCREEN_HEIGHT,
        (destinationX + width) * gMainWinScreenWidth / LOGICAL_SCREEN_WIDTH - 1,
        (destinationY + height) * gMainWinScreenHeight / LOGICAL_SCREEN_HEIGHT - 1
    );
}

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

void BitmapToScreen(bitmap* image) {
    BlitBitmapToScreen(image, 0, 0, image->m_width, image->m_height, 0, 0);
}

void SetPalette(i8* paletteData, b32 updateDisplay) {
    memcpy(gBufferPalette->m_data, paletteData, PALETTE_DATA_SIZE);
    memcpy(
        gCyclePal,
        paletteData + PALETTE_CYCLE_FIRST * PALETTE_GRAPHICS_CHANNELS,
        sizeof(gCyclePal)
    );
    if (updateDisplay != false)
        UpdatePalette(gBufferPalette->m_data);
}

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

#include <BASE/bitmap.h>
#include <BASE/icon.h>
#include <BASE/Misc.h>

#include <stdlib.h>
#include <string.h>

i16 AutoInitSVGA(void) {
    return 0;
}

i32 Random(i32 low, i32 high) {
    H1_ASSERT(high > low);
    return rand() % (high - low + 1) + low;
}

void PostprocessPalette(i8* paletteData) {
    PaletteColor* remapped = static_cast<PaletteColor*>(malloc(PALETTE_DATA_SIZE));
    memset(remapped, 0, PALETTE_DATA_SIZE);
    for (i32 index = 0; index < PALETTE_COLOR_COUNT; index++)
        memcpy(
            &remapped[gMonoColorMap[index]],
            &reinterpret_cast<PaletteColor*>(paletteData)[index],
            sizeof(PaletteColor)
        );
    memcpy(paletteData, remapped, PALETTE_DATA_SIZE);
    free(remapped);
}

void PostprocessBitmap(u8* pixels, i32 width, i32 height) {}

void PostprocessIcon(icon* loadedIcon) {}

#include <BASE/bitmap.h>
#include <BASE/icon.h>
#include <BASE/Iconm2b.h>

#include <string.h>

// The icon routines below read only inside the icon's data (m_dataSize
// bytes: a command outside it ends the frame, a pixel outside it is 0) and
// write only inside the destination's pixel block. The original trusted the
// frame's commands and the place it was drawn at.
static u8 IconDataByte(icon* sourceIcon, i32 at, u8 outside) {
    if (at < 0 || static_cast<u32>(at) >= sourceIcon->m_dataSize)
        return outside;
    return sourceIcon->m_data[at];
}

// Copies count pixel bytes of the icon's data from offset `from` to offset
// `to` of the destination's pixels.
static void CopyIconRun(bitmap* destination, i32 to, icon* sourceIcon, i32 from, i32 count) {
    i32 size = static_cast<u16>(destination->m_width) * static_cast<u16>(destination->m_height);
    i32 i;
    if (count <= 0)
        return;
    if (to >= 0 && count <= size - to && from >= 0
        && static_cast<u32>(from) + static_cast<u32>(count) <= sourceIcon->m_dataSize) {
        memcpy(destination->m_pixels + to, sourceIcon->m_data + from, count);
        return;
    }
    for (i = 0; i < count; i++) {
        if (to + i >= 0 && to + i < size)
            destination->m_pixels[to + i] = IconDataByte(sourceIcon, from + i, 0);
    }
}

// Fills count pixels from offset `to` of the destination's pixels.
static void FillIconRun(bitmap* destination, i32 to, i32 color, i32 count) {
    i32 size = static_cast<u16>(destination->m_width) * static_cast<u16>(destination->m_height);
    if (count <= 0)
        return;
    if (to < 0) {
        count += to;
        to = 0;
    }
    if (count > size - to)
        count = size - to;
    if (count > 0)
        memset(destination->m_pixels + to, color, count);
}

// Whether frame names a frame whose header lies in the icon's data; a frame
// outside the table draws nothing.
static i32 IconFrameKnown(icon* sourceIcon, i32 frame) {
    return frame >= 0 && frame < sourceIcon->m_frameCount
           && static_cast<u32>(frame + 1) * sizeof(IconEntry) <= sourceIcon->m_dataSize;
}

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
    i32 clipBottom = clipY + clipH - 1;
    if (!IconFrameKnown(sourceIcon, frame))
        return;
    IconEntry* entry = sourceIcon->m_frames + frame;
    i32 source = entry->srcOffset;
    i32 curX = x + entry->x;
    i32 curY = y + entry->y;
    i32 run;
    bool decoding = true;
    while (decoding) {
        run = IconDataByte(sourceIcon, source, ICON_MONO_END_COMMAND);
        if (static_cast<i8>(run) < 0) {
            if ((run & ICON_MONO_SKIP_MASK) != 0) {
                curX += run & ICON_MONO_SKIP_MASK;
                source++;
            } else
                decoding = false;
        } else if (run != ICON_MONO_NEWLINE_COMMAND) {
            if (curY >= clipY && curY <= clipBottom && curX + run >= clipX && curX <= clipRight) {
                if (curX >= clipX) {
                    if (curX + run <= clipRight)
                        FillIconRun(destination, curX + curY * LOGICAL_SCREEN_WIDTH, color, run);
                    else
                        FillIconRun(destination, curX + curY * LOGICAL_SCREEN_WIDTH, color,
                                    clipRight - curX + 1);
                } else {
                    if (curX + run <= clipRight)
                        FillIconRun(destination, clipX + curY * LOGICAL_SCREEN_WIDTH, color,
                                    curX + run - clipX);
                    else
                        FillIconRun(destination, clipX + curY * LOGICAL_SCREEN_WIDTH, color, clipW);
                }
            }
            curX += run;
            source++;
        } else {
            curX = x + entry->x;
            curY++;
            source++;
        }
    }
}

static i32 gMiscOldField;
static i32 gClipY;
static i32 gClipLimitY;
static i32 gClipRowStart;
static i32 gClipRow;
static IconEntry* gClipFrameEntry;
static i32 gClipSource;
static i32 gClipLimitX;
static i32 gClipX;
static u32 gClipRun;
static bool gClipInside;
static u8 gMiscScanTable[64];

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
    if (!IconFrameKnown(sourceIcon, frame))
        return;
    gClipFrameEntry = sourceIcon->m_frames + frame;
    gClipSource = gClipFrameEntry->srcOffset;
    gClipX = gClipRowStart = x + gClipFrameEntry->x;
    gClipY = y + gClipFrameEntry->y;
    if (ICON_FITS_CLIP(
            gClipRowStart,
            gClipY,
            gClipFrameEntry->w,
            gClipFrameEntry->h,
            clipX,
            clipY,
            clipW,
            clipH
        )) {
        gClipInside = true;
    } else {
        gClipInside = false;
        gClipLimitX = clipX + clipW - 1;
        gClipLimitY = clipY + clipH - 1;
    }
    gClipRow = gClipY * destination->m_width;
    for (;;) {
        gClipRun = IconDataByte(sourceIcon, gClipSource++, ICON_MONO_END_COMMAND);
        if (static_cast<i8>(gClipRun) < 0) {
            if (gClipRun & ICON_MONO_SKIP_MASK)
                gClipX += gClipRun & ICON_MONO_SKIP_MASK;
            else
                break;
        } else if (gClipRun != 0) {
            if (gClipInside) {
                CopyIconRun(destination, gClipRow + gClipX, sourceIcon, gClipSource, gClipRun);
            } else if (gClipY >= clipY && gClipY <= gClipLimitY && gClipX + gClipRun >= clipX
                       && gClipX <= gClipLimitX) {
                if (gClipX >= clipX) {
                    if (gClipX + gClipRun <= gClipLimitX)
                        CopyIconRun(destination, gClipRow + gClipX, sourceIcon, gClipSource,
                                    gClipRun);
                    else
                        CopyIconRun(destination, gClipRow + gClipX, sourceIcon, gClipSource,
                                    gClipLimitX - gClipX + 1);
                } else {
                    if (gClipX + IconDataByte(sourceIcon, gClipSource, 0) <= gClipLimitX)
                        CopyIconRun(destination, gClipRow + gClipX, sourceIcon, gClipSource,
                                    gClipX + gClipRun - clipX);
                    else
                        CopyIconRun(destination, gClipRow + gClipX, sourceIcon, gClipSource,
                                    clipW);
                }
            }
            gClipX += gClipRun;
            gClipSource += gClipRun;
        } else {
            gClipX = gClipRowStart;
            gClipY++;
            gClipRow += destination->m_width;
        }
    }
}

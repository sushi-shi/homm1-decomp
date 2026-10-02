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
    if (InvalidateRect(hwndApp, &invalidRectangle, 0) == 0)
        LogStr("InvalidateRect Failed");
    if (UpdateWindow(hwndApp) == 0)
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
    if (currentPalette == 0)
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

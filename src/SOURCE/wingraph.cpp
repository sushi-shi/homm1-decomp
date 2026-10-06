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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

BOOL gWinGAttached = TRUE;
BOOL gDDrawAttached = FALSE;
i32 gGraphicsType = WINGRAPH_GRAPHICS_WING;
i32 gMainVideoModeColorDepth = 16;
i32 gMainVideoModeWidth = 1024;
i32 gMainVideoModeHeight = 768;
i32 Orientation = 1;
WingPalette LogicalPalette = {0x300, WINGRAPH_PALETTE_SIZE};
void* gInitWin = NULL;
i32 gTtlBlts = 0;
BOOL gWinGraphBusy = FALSE;
DirectDrawCreateProc gDirectDrawCreate = NULL;
IDirectDraw* gDD = NULL;
IDirectDrawSurface* gDDSPrimary = NULL;
IDirectDrawSurface* gDDSOne = NULL;
IDirectDrawClipper* gClipper = NULL;
IDirectDrawPalette* gDDPal = NULL;
i32 gBusyRetry = 0;
BOOL gInDDSD = FALSE;
HDC hdcImage = NULL;
HBITMAP gbmOldMonoBitmap = NULL;
HPALETTE hpalApp = NULL;
HINSTANCE gDDrawLibrary = NULL;
RECT gDDClientRect;
RECT gDDSourceRect;
RECT gDDDestinationRect;
i32 gDDResult;
_DDSURFACEDESC gDDSurfaceDesc;
i32 gPaintStart;
WingImage screenImage;
extern i32 gScrollX;
extern i32 gScrollY;
extern i32 gFullCombatScreenDrawn;
extern i32 gLimitedCombatUpdatePalette;
extern configStruct gConfig;

void DDRestoreDisplayMode() {
    i32 result;
    if (gDD != NULL) {
        result = gDD->RestoreDisplayMode();
        if (result != DD_OK)
            DDSD(result, __FILE__, __LINE__);
    }
}

BOOL DDQueryNewPalette() {
    i32 unused;

    if (gWinGraphBusy)
        return TRUE;
    if (!gForegroundApp)
        return TRUE;
    return SetPalette();
}

void CreatePrimary(void) {
    i32 result;

    gDDSPrimary = DDCreateSurface(WINGRAPH_WIDTH, WINGRAPH_HEIGHT, 1);
    if (gClipper != NULL) {
        result = gDDSPrimary->SetClipper(NULL);
        if (result != DD_OK && result != DDERR_NOCLIPPERATTACHED)
            DDSD(result, __FILE__, __LINE__);
        gClipper->Release();
        gClipper = NULL;
    }
}

void SetupClipper(void) {
    i32 result;

    if (gConfig.gfx[gCurExe].fullScreen == 0) {
        result = gDD->CreateClipper(0, &gClipper, NULL);
        if (result != DD_OK)
            DDSD(result, __FILE__, __LINE__);
        result = gClipper->SetHWnd(0, hwndApp);
        if (result != DD_OK)
            DDSD(result, __FILE__, __LINE__);
        result = gDDSPrimary->SetClipper(gClipper);
        if (result != DD_OK)
            DDSD(result, __FILE__, __LINE__);
    }
}

void DDInitGraphics(void) {
    i32 result;

    if (gWinGraphBusy != FALSE)
        return;
    result = gDirectDrawCreate(NULL, &gDD, NULL);
    if (result != DD_OK)
        DDSD(result, __FILE__, __LINE__);
    if (gConfig.gfx[gCurExe].fullScreen != 0) {
        SetMenuStatus(0);
        result = gDD->SetCooperativeLevel(
            hwndApp,
            DDSCL_EXCLUSIVE | DDSCL_FULLSCREEN | DDSCL_ALLOWREBOOT
        );
        if (result != DD_OK)
            DDSD(result, __FILE__, __LINE__);
        result = gDD->SetDisplayMode(WINGRAPH_WIDTH, WINGRAPH_HEIGHT, WINGRAPH_COLOR_DEPTH);
        if (result != DD_OK)
            DDSD(result, __FILE__, __LINE__);
    } else {
        result = gDD->SetCooperativeLevel(hwndApp, DDSCL_NORMAL);
        if (result != DD_OK)
            DDSD(result, __FILE__, __LINE__);
    }
    CreatePrimary();
    SetupClipper();
    gDDSOne = DDCreateSurface(WINGRAPH_WIDTH, WINGRAPH_HEIGHT, 0);
    InitializePalette();
}

BOOL DDAppPaint(void* window, void* paintDC) {
    i32 ySrc;
    i32 height;
    i32 x;
    i32 width;
    PAINTSTRUCT ps;
    POINT pt;

    if (gWinGraphBusy != FALSE)
        return TRUE;
    if (gMinimized != 0)
        return TRUE;
    if (gDD == NULL)
        return TRUE;
    {
        gWinGraphBusy = TRUE;
        paintDC = BeginPaint(static_cast<HWND>(window), &ps);
        GetClientRect(static_cast<HWND>(window), &gDDClientRect);
        if (ps.rcPaint.right == 0 || ps.rcPaint.bottom == 0)
            ps.rcPaint = gDDClientRect;
        if (ps.rcPaint.right < WINGRAPH_PAINT_X_END)
            ps.rcPaint.right++;
        if (ps.rcPaint.bottom < WINGRAPH_PAINT_Y_END)
            ps.rcPaint.bottom++;

        gDDDestinationRect = ps.rcPaint;
        width = CLIENT_TO_GAME_X(gDDDestinationRect.right - gDDDestinationRect.left + 1);
        height = CLIENT_TO_GAME_Y(gDDDestinationRect.bottom - gDDDestinationRect.top + 1);
        x = CLIENT_TO_GAME_X(gDDDestinationRect.left);
        ySrc = CLIENT_TO_GAME_Y(gDDDestinationRect.top);
        if (gScrollX != 0) {
            x = gScrollX + WINGRAPH_SCROLL_MARGIN;
            width = WINGRAPH_SCROLL_SIZE;
        }
        if (gScrollY != 0) {
            ySrc = gScrollY + WINGRAPH_SCROLL_MARGIN;
            height = WINGRAPH_SCROLL_SIZE;
        }
        gDDSourceRect.left = x;
        gDDSourceRect.right = x + width - 1;
        gDDSourceRect.top = ySrc;
        gDDSourceRect.bottom = height + ySrc - 1;

        pt.y = 0;
        pt.x = pt.y;
        ClientToScreen(hwndApp, &pt);
        OffsetRect(&gDDDestinationRect, pt.x, pt.y);
        gDDResult = gDDSOne->Unlock(NULL);
        if (gDDResult != DD_OK)
            DDSD(gDDResult, __FILE__, __LINE__);

        if (gDDSourceRect.left < 0)
            gDDSourceRect.left = 0;
        if (gDDSourceRect.top < 0)
            gDDSourceRect.top = 0;
        if (gDDSourceRect.right > WINGRAPH_PAINT_X_END)
            gDDSourceRect.right = WINGRAPH_WIDTH;
        if (gDDSourceRect.bottom > WINGRAPH_PAINT_Y_END)
            gDDSourceRect.bottom = WINGRAPH_HEIGHT;

        gPaintStart = KBTickCount();
        for (;;) {
            LogStr(
                "BltD",
                gDDDestinationRect.left,
                gDDDestinationRect.right,
                gDDDestinationRect.top,
                gDDDestinationRect.bottom,
                0
            );
            LogStr(
                "BltS",
                gDDSourceRect.left,
                gDDSourceRect.right,
                gDDSourceRect.top,
                gDDSourceRect.bottom,
                0
            );
            gDDResult =
                gDDSPrimary->Blt(&gDDDestinationRect, gDDSOne, &gDDSourceRect, DDBLT_WAIT, NULL);
            if (gDDResult == DDERR_SURFACELOST) {
                gDDResult = gDDSPrimary->Restore();
                if (gDDResult == DDERR_WRONGMODE) {
                    LogStr("ResetDisplayMode");
                    gDDResult =
                        gDD->SetDisplayMode(WINGRAPH_WIDTH, WINGRAPH_HEIGHT, WINGRAPH_COLOR_DEPTH);
                    if (gDDResult != DD_OK)
                        DDSD(gDDResult, __FILE__, __LINE__);
                    gDDResult = gDDSPrimary->Restore();
                    if (gDDResult != DD_OK)
                        DDSD(gDDResult, __FILE__, __LINE__);
                    gDDDestinationRect = gDDSourceRect;
                }
                if (gDDResult != DD_OK)
                    DDSD(gDDResult, __FILE__, __LINE__);
            } else if (gDDResult == DDERR_SURFACEBUSY
                       && KBTickCount() < gPaintStart + WINGRAPH_PAINT_TIMEOUT) {
                gBusyRetry++;
            } else if (gDDResult != DD_OK) {
                DDSD(gDDResult, __FILE__, __LINE__);
            } else {
                break;
            }
        }

        memset(&gDDSurfaceDesc, 0, sizeof(gDDSurfaceDesc));
        gDDSurfaceDesc.dwSize = sizeof(gDDSurfaceDesc);
        gDDResult = gDDSOne->Lock(NULL, &gDDSurfaceDesc, DDLOCK_WAIT, NULL);
        if (gDDResult != DD_OK)
            DDSD(gDDResult, __FILE__, __LINE__);
        if (gpWindowManager->m_screen != NULL) {
            gpWindowManager->m_screen->m_pixels = static_cast<i8*>(gDDSurfaceDesc.lpSurface);
            gInitWin = gDDSurfaceDesc.lpSurface;
        } else {
            gInitWin = gDDSurfaceDesc.lpSurface;
        }
        if (gDDResult != DD_OK)
            DDSD(gDDResult, __FILE__, __LINE__);
        EndPaint(static_cast<HWND>(window), &ps);
        gWinGraphBusy = FALSE;
    }
    return TRUE;
}

void DDInitializePalette() {
    i32 ddrval;
    HDC hdc;
    i32 i;
    if (gWinGraphBusy != FALSE)
        return;
    {
        hdc = GetDC(NULL);
        GetSystemPaletteEntries(hdc, 0, WINGRAPH_SYSTEM_PALETTE_SIZE, LogicalPalette.entries);
        GetSystemPaletteEntries(
            hdc,
            WINGRAPH_MUTABLE_PALETTE_END,
            WINGRAPH_SYSTEM_PALETTE_SIZE,
            &LogicalPalette.entries[WINGRAPH_MUTABLE_PALETTE_END]
        );
        ReleaseDC(NULL, hdc);
        for (i = 0; i < WINGRAPH_SYSTEM_PALETTE_END; i++) {
            LogicalPalette.entries[i].peFlags = 0;
            LogicalPalette.entries[WINGRAPH_MUTABLE_PALETTE_END + i].peFlags = 0;
        }
        for (i = WINGRAPH_SYSTEM_PALETTE_SIZE; i < WINGRAPH_MUTABLE_PALETTE_END; i++) {
            LogicalPalette.entries[i].peRed = 0;
            LogicalPalette.entries[i].peGreen = 0;
            LogicalPalette.entries[i].peBlue = 0;
            LogicalPalette.entries[i].peFlags = PC_NOCOLLAPSE;
        }
        ddrval = gDD->CreatePalette(DDPCAPS_8BIT, LogicalPalette.entries, &gDDPal, NULL);
        if (ddrval != DD_OK)
            DDSD(ddrval, __FILE__, __LINE__);
        SetPalette();
    }
}

BOOL DDSetPalette() {
    i32 result;
    if (gWinGraphBusy != FALSE)
        return TRUE;
    if (gForegroundApp == 0)
        return TRUE;
    if (gDDPal == NULL || gDDSPrimary == NULL || gDD == NULL)
        return TRUE;
    result = gDDSPrimary->SetPalette(gDDPal);
    if (result != DD_OK)
        DDSD(result, __FILE__, __LINE__);
    return FALSE;
}

struct IDirectDrawSurface* DDCreateSurface(u32 width, u32 height, i32 primary) {
    _DDSURFACEDESC ddsd;
    IDirectDrawSurface* lpSurface;
    i32 i;
    i32 tmp;
    i32 ddrval;

    memset(&ddsd, 0, sizeof(ddsd));
    ddsd.dwSize = sizeof(ddsd);
    if (primary != 0) {
        ddsd.dwFlags = 0;
        ddsd.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE;
    } else {
        ddsd.dwFlags = DDSD_HEIGHT | DDSD_WIDTH;
        ddsd.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN;
        ddsd.ddsCaps.dwCaps |= DDSCAPS_SYSTEMMEMORY;
        ddsd.dwHeight = height;
        ddsd.dwWidth = width;
    }
    ddrval = gDD->CreateSurface(&ddsd, &lpSurface, NULL);
    if (ddrval != DD_OK)
        DDSD(ddrval, __FILE__, __LINE__);
    if (primary == 0) {
        ddrval = lpSurface->Lock(NULL, &ddsd, DDLOCK_WAIT, NULL);
        if (ddrval != DD_OK)
            DDSD(ddrval, __FILE__, __LINE__);
        if (gpWindowManager->m_screen != NULL) {
            gpWindowManager->m_screen->m_pixels = static_cast<i8*>(ddsd.lpSurface);
            gInitWin = ddsd.lpSurface;
        } else {
            gInitWin = ddsd.lpSurface;
        }
    }
    return lpSurface;
}

void DDSD(i32 error, char* file, i32 line) {
    i32 restoreResult;
    i32 unused;

    if (gInDDSD != FALSE)
        return;
    gInDDSD = TRUE;
    restoreResult = gDD->RestoreDisplayMode();
    unused = DDSD_REPORT_NONE;
    switch (error) {
        case DD_OK:
            return;
        case DDERR_GENERIC:
            unused = DDSD_REPORT_GENERIC;
            break;
        case DDERR_INVALIDCLIPLIST:
            unused = DDSD_REPORT_INVALIDCLIPLIST;
            break;
        case DDERR_INVALIDOBJECT:
            unused = DDSD_REPORT_INVALIDOBJECT;
            break;
        case DDERR_INVALIDPARAMS:
            unused = DDSD_REPORT_INVALIDPARAMS;
            break;
        case DDERR_INVALIDRECT:
            unused = DDSD_REPORT_INVALIDRECT;
            break;
        case DDERR_NOALPHAHW:
            unused = DDSD_REPORT_NOALPHAHW;
            break;
        case DDERR_NOBLTHW:
            unused = DDSD_REPORT_NOBLTHW;
            break;
        case DDERR_NOCLIPLIST:
            unused = DDSD_REPORT_NOCLIPLIST;
            break;
        case DDERR_NODDROPSHW:
            unused = DDSD_REPORT_NODDROPSHW;
            break;
        case DDERR_SURFACELOST:
            unused = DDSD_REPORT_SURFACELOST;
            break;
        case DDERR_UNSUPPORTED:
            unused = DDSD_REPORT_UNSUPPORTED;
            break;
        case DDERR_NOMIRRORHW:
            unused = DDSD_REPORT_NOMIRRORHW;
            break;
        case DDERR_NORASTEROPHW:
            unused = DDSD_REPORT_NORASTEROPHW;
            break;
        case DDERR_NOROTATIONHW:
            unused = DDSD_REPORT_NOROTATIONHW;
            break;
        case DDERR_NOSTRETCHHW:
            unused = DDSD_REPORT_NOSTRETCHHW;
            break;
        case DDERR_SURFACEBUSY:
            unused = DDSD_REPORT_SURFACEBUSY;
            break;
        case DDERR_NOZBUFFERHW:
            unused = DDSD_REPORT_NOZBUFFERHW;
            break;
        case DDERR_OUTOFMEMORY:
            unused = DDSD_REPORT_OUTOFMEMORY;
            break;
        case DDERR_CLIPPERISUSINGHWND:
            unused = DDSD_REPORT_CLIPPERISUSINGHWND;
            break;
        case DDERR_NOEXCLUSIVEMODE:
            unused = DDSD_REPORT_NOEXCLUSIVEMODE;
            break;
        case DDERR_NOT8BITCOLOR:
            unused = DDSD_REPORT_NOT8BITCOLOR;
            break;
        case DDERR_NOPALETTEATTACHED:
            unused = DDSD_REPORT_NOPALETTEATTACHED;
            break;
        case DDERR_NOPALETTEHW:
            unused = DDSD_REPORT_NOPALETTEHW;
            break;
        case DDERR_LOCKEDSURFACES:
            unused = DDSD_REPORT_LOCKEDSURFACES;
            break;
        case DDERR_IMPLICITLYCREATED:
            unused = DDSD_REPORT_IMPLICITLYCREATED;
            break;
        case DDERR_WRONGMODE:
            unused = DDSD_REPORT_WRONGMODE;
            break;
        case DDERR_INCOMPATIBLEPRIMARY:
            unused = DDSD_REPORT_INCOMPATIBLEPRIMARY;
            break;
        case DDERR_NOCLIPPERATTACHED:
            unused = DDSD_REPORT_NOCLIPPERATTACHED;
            break;
        default:
            unused = DDSD_REPORT_UNKNOWN;
            break;
    }
    MessageBeep(MB_OK);
    MessageBeep(MB_OK);
    MessageBeep(MB_OK);
    sprintf(gText, "Direct Draw Error #%d in file '%s' at Line #%d", unused, file, line);
    LogStr(gText);
    ShutDown(gText);
}

void DDUpdatePalette(i8* paletteData) {
    i32 entry;
    i32 res;

    if (gWinGraphBusy != FALSE)
        return;
    if (gForegroundApp == 0)
        return;
    for (entry = WINGRAPH_SYSTEM_PALETTE_SIZE; entry < WINGRAPH_MUTABLE_PALETTE_END; entry++) {
        LogicalPalette.entries[entry].peRed = paletteData[entry * WINGRAPH_PALETTE_COMPONENT_COUNT]
                                              << WINGRAPH_PALETTE_VALUE_SHIFT;
        LogicalPalette.entries[entry].peGreen =
            paletteData[entry * WINGRAPH_PALETTE_COMPONENT_COUNT + 1]
            << WINGRAPH_PALETTE_VALUE_SHIFT;
        LogicalPalette.entries[entry].peBlue =
            paletteData[entry * WINGRAPH_PALETTE_COMPONENT_COUNT + 2]
            << WINGRAPH_PALETTE_VALUE_SHIFT;
        LogicalPalette.entries[entry].peFlags = PC_NOCOLLAPSE;
    }
    H1_ASSERT(reinterpret_cast<i32>(gDDPal));
    res = gDDPal->SetEntries(
        0,
        WINGRAPH_SYSTEM_PALETTE_SIZE,
        WINGRAPH_PALETTE_SIZE - WINGRAPH_SYSTEM_PALETTE_SIZE * 2,
        &LogicalPalette.entries[WINGRAPH_SYSTEM_PALETTE_SIZE]
    );
    if (res != DD_OK)
        DDSD(res, __FILE__, __LINE__);
}

void DDCleanUpWinGraphics(void) {
    i32 restoreVal;
    i32 result;

    if (gDD != NULL) {
        restoreVal = gDD->RestoreDisplayMode();
        if (gClipper != NULL) {
            if (gDDSPrimary != NULL) {
                result = gDDSPrimary->SetClipper(NULL);
                if (result != DD_OK && result != DDERR_NOCLIPPERATTACHED)
                    DDSD(result, __FILE__, __LINE__);
            }
            gClipper->Release();
            gClipper = NULL;
        }
        if (gDDSPrimary != NULL) {
            gDDSPrimary->Release();
            gDDSPrimary = NULL;
        }
        if (gDDSOne != NULL) {
            gDDSOne->Release();
            gDDSOne = NULL;
        }
        if (gDDPal != NULL) {
            gDDPal->Release();
            gDDPal = NULL;
        }
        result = gDD->SetCooperativeLevel(hwndApp, DDSCL_NORMAL);
        if (result != DD_OK)
            DDSD(result, __FILE__, __LINE__);
        gDD->Release();
        gDD = NULL;
    }
}

void DDSetFullScreenStatus(i32 fullScreen) {
    i32 w;
    i32 x;
    i32 h;
    i32 y;
    i32 ddrval;

    if (gWinGraphBusy != FALSE)
        return;
    if (gConfig.gfx[gCurExe].fullScreen == fullScreen)
        return;
    {
        x = gConfig.gfx[gCurExe].x;
        y = gConfig.gfx[gCurExe].y;
        w = gConfig.gfx[gCurExe].width;
        h = gConfig.gfx[gCurExe].height;
        gWinGraphBusy = TRUE;
        gConfig.gfx[gCurExe].fullScreen = fullScreen;
        if (gConfig.gfx[gCurExe].fullScreen != 0)
            SetMenuStatus(0);

        ddrval = gDD->SetCooperativeLevel(
            hwndApp,
            DDSCL_EXCLUSIVE | DDSCL_FULLSCREEN | DDSCL_ALLOWREBOOT
        );
        if (ddrval != DD_OK)
            DDSD(ddrval, __FILE__, __LINE__);
        if (gConfig.gfx[gCurExe].fullScreen != 0) {
            ddrval = gDD->SetDisplayMode(WINGRAPH_WIDTH, WINGRAPH_HEIGHT, WINGRAPH_COLOR_DEPTH);
            if (ddrval != DD_OK)
                DDSD(ddrval, __FILE__, __LINE__);
        } else {
            ddrval = gDD->RestoreDisplayMode();
            if (ddrval != DD_OK)
                DDSD(ddrval, __FILE__, __LINE__);
            ddrval = gDD->SetCooperativeLevel(hwndApp, DDSCL_NORMAL);
            if (ddrval != DD_OK)
                DDSD(ddrval, __FILE__, __LINE__);
        }
        if (gDDSPrimary != NULL) {
            gDDSPrimary->Release();
            gDDSPrimary = NULL;
        }
        CreatePrimary();
        ddrval = gDDSPrimary->SetPalette(gDDPal);
        if (ddrval != DD_OK)
            DDSD(ddrval, __FILE__, __LINE__);
        WritePrefs();
        gWinGraphBusy = FALSE;
        if (gConfig.gfx[gCurExe].fullScreen == 0) {
            SetMenuStatus(1);
            ResizeWindow(x, y, w, h);
        } else {
            gConfig.gfx[gCurExe].x = x;
            gConfig.gfx[gCurExe].y = y;
            gConfig.gfx[gCurExe].width = w;
            gConfig.gfx[gCurExe].height = h;
        }
        SetupClipper();
    }
}

BOOL WGQueryNewPalette() {
    i32 paletteChanges;
    {
        HDC hdc;

        hdc = GetDC(hwndApp);
        if (hpalApp != NULL)
            SelectPalette(hdc, hpalApp, FALSE);
        paletteChanges = RealizePalette(hdc);
        ReleaseDC(hwndApp, hdc);
    }
    if (paletteChanges > 0) {
        InvalidateRect(hwndApp, NULL, TRUE);
        return TRUE;
    } else {
        return FALSE;
    }
}

void WGInitGraphics() {
    HBITMAP bitmap;

    if (hdcImage != NULL)
        return;
    if (WinGRecommendDIBFormat(reinterpret_cast<LPBITMAPINFO>(&screenImage))) {
        screenImage.header.biBitCount = WINGRAPH_COLOR_DEPTH;
        screenImage.header.biCompression = BI_RGB;
        Orientation = screenImage.header.biHeight;
    } else {
        screenImage.header.biSize = sizeof(BITMAPINFOHEADER);
        screenImage.header.biPlanes = 1;
        screenImage.header.biBitCount = WINGRAPH_COLOR_DEPTH;
        screenImage.header.biCompression = BI_RGB;
        screenImage.header.biSizeImage = 0;
        screenImage.header.biClrUsed = 0;
        screenImage.header.biClrImportant = 0;
    }
    screenImage.header.biWidth = WINGRAPH_WIDTH;
    screenImage.header.biHeight = -WINGRAPH_HEIGHT;
    InitializePalette();
    hdcImage = WinGCreateDC();
    screenImage.header.biWidth = WINGRAPH_WIDTH;
    screenImage.header.biHeight = -WINGRAPH_HEIGHT;
    bitmap =
        WinGCreateBitmap(hdcImage, reinterpret_cast<LPBITMAPINFO>(&screenImage), &screenImage.bits);
    screenImage.header.biSizeImage = screenImage.header.biHeight * screenImage.header.biWidth;
    screenImage.header.biSizeImage *= Orientation;
    gbmOldMonoBitmap = static_cast<HBITMAP>(SelectObject(hdcImage, bitmap));
    gInitWin = screenImage.bits;
    PatBlt(hdcImage, 0, 0, iMainWinScreenWidth, gMainWinScreenHeight, BLACKNESS);
}

void WGUpdatePalette(i8* paletteData) {
    HDC dc;
    i32 result;
    i32 index;

    for (index = WINGRAPH_SYSTEM_PALETTE_SIZE; index < WINGRAPH_MUTABLE_PALETTE_END; index++) {
        LogicalPalette.entries[index].peRed = paletteData[index * 3] << 2;
        screenImage.colors[index].rgbRed = LogicalPalette.entries[index].peRed;
        LogicalPalette.entries[index].peGreen = paletteData[index * 3 + 1] << 2;
        screenImage.colors[index].rgbGreen = LogicalPalette.entries[index].peGreen;
        LogicalPalette.entries[index].peBlue = paletteData[index * 3 + 2] << 2;
        screenImage.colors[index].rgbBlue = LogicalPalette.entries[index].peBlue;
    }
    AnimatePalette(
        hpalApp,
        WINGRAPH_SYSTEM_PALETTE_SIZE,
        WINGRAPH_PALETTE_SIZE - WINGRAPH_SYSTEM_PALETTE_SIZE * 2,
        &LogicalPalette.entries[WINGRAPH_SYSTEM_PALETTE_SIZE]
    );
    WinGSetDIBColorTable(
        hdcImage,
        WINGRAPH_SYSTEM_PALETTE_SIZE,
        WINGRAPH_PALETTE_SIZE - WINGRAPH_SYSTEM_PALETTE_SIZE * 2,
        &screenImage.colors[WINGRAPH_SYSTEM_PALETTE_SIZE]
    );
    if (hpalApp != NULL)
        DeleteObject(hpalApp);
    hpalApp = CreatePalette(reinterpret_cast<LPLOGPALETTE>(&LogicalPalette));
    dc = GetDC(hwndApp);
    if (hpalApp != NULL)
        SelectPalette(dc, hpalApp, FALSE);
    result = RealizePalette(dc);
    ReleaseDC(hwndApp, dc);
    if (gMainVideoModeColorDepth != WINGRAPH_COLOR_DEPTH) {
        if (gLimitedCombatUpdatePalette != 0) {
            if (gFullCombatScreenDrawn != 0)
                BlitBitmapToScreen(
                    gpWindowManager->m_screen,
                    0,
                    0,
                    WINGRAPH_WIDTH,
                    WINGRAPH_LIMITED_COMBAT_HEIGHT,
                    0,
                    0
                );
        } else {
            BlitBitmapToScreen(
                gpWindowManager->m_screen,
                0,
                0,
                WINGRAPH_WIDTH,
                WINGRAPH_HEIGHT,
                0,
                0
            );
        }
    }
}

void WGInitializePalette() {
    HDC hdc;
    i32 i;

    if (hpalApp != NULL)
        return;
    hdc = GetDC(NULL);
    GetSystemPaletteEntries(hdc, 0, WINGRAPH_SYSTEM_PALETTE_SIZE, LogicalPalette.entries);
    GetSystemPaletteEntries(
        hdc,
        WINGRAPH_MUTABLE_PALETTE_END,
        WINGRAPH_SYSTEM_PALETTE_SIZE,
        &LogicalPalette.entries[WINGRAPH_MUTABLE_PALETTE_END]
    );
    ReleaseDC(NULL, hdc);
    for (i = 0; i < WINGRAPH_SYSTEM_PALETTE_END; i++) {
        screenImage.colors[i].rgbRed = LogicalPalette.entries[i].peRed;
        screenImage.colors[i].rgbGreen = LogicalPalette.entries[i].peGreen;
        screenImage.colors[i].rgbBlue = LogicalPalette.entries[i].peBlue;
        screenImage.colors[i].rgbReserved = 0;
        LogicalPalette.entries[i].peFlags = 0;
        screenImage.colors[WINGRAPH_MUTABLE_PALETTE_END + i].rgbRed =
            LogicalPalette.entries[WINGRAPH_MUTABLE_PALETTE_END + i].peRed;
        screenImage.colors[WINGRAPH_MUTABLE_PALETTE_END + i].rgbGreen =
            LogicalPalette.entries[WINGRAPH_MUTABLE_PALETTE_END + i].peGreen;
        screenImage.colors[WINGRAPH_MUTABLE_PALETTE_END + i].rgbBlue =
            LogicalPalette.entries[WINGRAPH_MUTABLE_PALETTE_END + i].peBlue;
        screenImage.colors[WINGRAPH_MUTABLE_PALETTE_END + i].rgbReserved = 0;
        LogicalPalette.entries[WINGRAPH_MUTABLE_PALETTE_END + i].peFlags = 0;
    }
    for (i = WINGRAPH_SYSTEM_PALETTE_SIZE; i < WINGRAPH_MUTABLE_PALETTE_END; i++) {
        screenImage.colors[i].rgbRed = LogicalPalette.entries[i].peRed = 0;
        screenImage.colors[i].rgbGreen = LogicalPalette.entries[i].peGreen = 0;
        screenImage.colors[i].rgbBlue = LogicalPalette.entries[i].peBlue = 0;
        screenImage.colors[i].rgbReserved = 0;
        LogicalPalette.entries[i].peFlags = PC_NOCOLLAPSE;
    }
    hpalApp = CreatePalette(reinterpret_cast<LPLOGPALETTE>(&LogicalPalette));
}

BOOL WGAppPaint(void* window, void* paintDC) {
    i32 srcX;
    i32 iSrcY;
    i32 dstW;
    i32 destX;
    i32 nDestY;
    RECT rect;
    i32 destHeight;
    i32 padding;
    PAINTSTRUCT paintStruct;
    i8 unused;

    unused = 0;
    if (screenImage.bits != NULL) {
        paintDC = BeginPaint(static_cast<HWND>(window), &paintStruct);
        SelectPalette(static_cast<HDC>(paintDC), hpalApp, FALSE);
        RealizePalette(static_cast<HDC>(paintDC));
        GetClientRect(static_cast<HWND>(window), &rect);
        destX = 0;
        srcX = destX;
        nDestY = 0;
        iSrcY = nDestY;
        dstW = rect.right - rect.left;
        destHeight = rect.bottom - rect.top;
        srcX = CLIENT_TO_GAME_X(destX);
        iSrcY = CLIENT_TO_GAME_Y(nDestY);
        if (gScrollX != 0)
            srcX += gScrollX;
        if (gScrollY != 0)
            iSrcY += gScrollY;
        gTtlBlts++;
        if (iMainWinScreenWidth == WINGRAPH_WIDTH && gMainWinScreenHeight == WINGRAPH_HEIGHT) {
            destX = paintStruct.rcPaint.left & WINGRAPH_PAINT_ALIGN_MASK;
            dstW = paintStruct.rcPaint.right - destX + 1;
            nDestY = paintStruct.rcPaint.top;
            destHeight = paintStruct.rcPaint.bottom - nDestY + 1;
            WinGBitBlt(
                static_cast<HDC>(paintDC),
                destX,
                nDestY,
                dstW,
                destHeight,
                hdcImage,
                destX + gScrollX,
                nDestY + gScrollY
            );
        } else {
            WinGStretchBlt(
                static_cast<HDC>(paintDC),
                destX,
                nDestY,
                dstW,
                destHeight,
                hdcImage,
                srcX,
                iSrcY,
                CLIENT_TO_GAME_X(dstW),
                CLIENT_TO_GAME_Y(destHeight)
            );
        }
        EndPaint(static_cast<HWND>(window), &paintStruct);
    }
    return TRUE;
}

void WGCleanUpWinGraphics() {
    HGDIOBJ bitmap;

    if (hdcImage != NULL) {
        bitmap = SelectObject(hdcImage, gbmOldMonoBitmap);
        DeleteObject(bitmap);
        DeleteDC(hdcImage);
        hdcImage = NULL;
    }
    if (hpalApp != NULL) {
        DeleteObject(hpalApp);
        hpalApp = NULL;
    }
}

void ConnectToDLLs() {
    gDDrawLibrary = LoadLibraryA("DDRAW.DLL");
    if (reinterpret_cast<u32>(gDDrawLibrary)
        >= HINSTANCE_ERROR) {
        gDirectDrawCreate = reinterpret_cast<DirectDrawCreateProc>(
            GetProcAddress(gDDrawLibrary, "DirectDrawCreate")
        );
        if (gDirectDrawCreate != NULL)
            gDDrawAttached = TRUE;
        else
            ShutDown("Error loading DDRAW.DLL");
    }
}

void DisconnectDLLs() {
    if (reinterpret_cast<u32>(gDDrawLibrary)
        >= HINSTANCE_ERROR)
        FreeLibrary(gDDrawLibrary);
}

void RestoreDisplayMode() {
    if (gGraphicsType == WINGRAPH_GRAPHICS_WING)
        return;
    else
        DDRestoreDisplayMode();
}

BOOL SetPalette() {
    if (gGraphicsType == WINGRAPH_GRAPHICS_WING)
        return FALSE;
    else
        return DDSetPalette();
}

void GetGraphicsInfo(void) {
    HDC screenDC;
    screenDC = GetDC(NULL);
    if (screenDC != NULL) {
        gMainVideoModeColorDepth = GetDeviceCaps(screenDC, BITSPIXEL);
        gMainVideoModeWidth = GetDeviceCaps(screenDC, HORZRES);
        gMainVideoModeHeight = GetDeviceCaps(screenDC, VERTRES);
        ReleaseDC(NULL, screenDC);
        if (gMainVideoModeColorDepth < WINGRAPH_COLOR_DEPTH)
            ShutDown(
                "Heroes requires 256 color mode or higher.\n\nTo change color mode, right "
                "click in an open area on the Windows 95 background, choose 'Properties', "
                "then the 'Settings' tab, then change the entry in the 'Color Palette Box'."
            );
    }
}

void InitGraphics() {
    ConnectToDLLs();
    if (gConfig.gfx[gCurExe].fullScreen != 0)
        gGraphicsType = WINGRAPH_GRAPHICS_DIRECT_DRAW;
    else
        gGraphicsType = WINGRAPH_GRAPHICS_WING;
    if (gGraphicsType == WINGRAPH_GRAPHICS_WING)
        WGInitGraphics();
    else
        DDInitGraphics();
}

BOOL AppPaint(void* window, void* paintDC) {
    if (gGraphicsType == WINGRAPH_GRAPHICS_WING)
        return WGAppPaint(window, paintDC);
    else
        return DDAppPaint(window, paintDC);
}

void InitializePalette() {
    if (gGraphicsType == WINGRAPH_GRAPHICS_WING)
        WGInitializePalette();
    else
        DDInitializePalette();
}

void UpdatePalette(i8* paletteData) {
    if (gGraphicsType == WINGRAPH_GRAPHICS_WING)
        WGUpdatePalette(paletteData);
    else
        DDUpdatePalette(paletteData);
}

void CleanUpWinGraphics() {
    if (gGraphicsType == WINGRAPH_GRAPHICS_WING)
        WGCleanUpWinGraphics();
    else
        DDCleanUpWinGraphics();
    DisconnectDLLs();
}

void SetFullScreenStatus(i32 fullScreen) {
    if (gInSmacker != 0)
        return;
    if (gConfig.gfx[gCurExe].fullScreen == fullScreen)
        return;
    if (gGraphicsType == WINGRAPH_GRAPHICS_WING) {
        gConfig.gfx[gCurExe].fullScreen = 1;
        if (SetGraphicsType(WINGRAPH_GRAPHICS_DIRECT_DRAW) != FALSE)
            DDSetFullScreenStatus(fullScreen);
        return;
    } else if (fullScreen == 0) {
        if (gWinGAttached != FALSE)
            SetGraphicsType(WINGRAPH_GRAPHICS_WING);
    } else {
        DDSetFullScreenStatus(fullScreen);
    }
}

BOOL QueryNewPalette() {
    if (gGraphicsType == WINGRAPH_GRAPHICS_WING)
        return WGQueryNewPalette();
    else
        return DDQueryNewPalette();
}

BOOL SetGraphicsType(i32 graphicsType) {
    void* screenBuffer;
    i32 w;
    i32 fullScreen;
    i32 x;
    i32 h;
    i32 y;

    if (graphicsType == gGraphicsType)
        return TRUE;
    if (graphicsType == WINGRAPH_GRAPHICS_WING && gWinGAttached == FALSE)
        return FALSE;
    if (graphicsType == WINGRAPH_GRAPHICS_DIRECT_DRAW && gDDrawAttached == FALSE)
        return FALSE;

    fullScreen = gConfig.gfx[gCurExe].fullScreen;
    x = gConfig.gfx[gCurExe].x;
    y = gConfig.gfx[gCurExe].y;
    w = gConfig.gfx[gCurExe].width;
    h = gConfig.gfx[gCurExe].height;
    screenBuffer = malloc(WINGRAPH_WIDTH * WINGRAPH_HEIGHT);
    memcpy(screenBuffer, gpWindowManager->m_screen->m_pixels, WINGRAPH_WIDTH * WINGRAPH_HEIGHT);
    if (graphicsType == WINGRAPH_GRAPHICS_WING) {
        gConfig.gfx[gCurExe].fullScreen = 0;
        DDCleanUpWinGraphics();
        gGraphicsType = WINGRAPH_GRAPHICS_WING;
        WGInitGraphics();
        gpWindowManager->m_screen->m_pixels = static_cast<i8*>(gInitWin);
    } else {
        WGCleanUpWinGraphics();
        gGraphicsType = WINGRAPH_GRAPHICS_DIRECT_DRAW;
        DDInitGraphics();
        gpWindowManager->m_screen->m_pixels = static_cast<i8*>(gInitWin);
    }
    memcpy(gpWindowManager->m_screen->m_pixels, screenBuffer, WINGRAPH_WIDTH * WINGRAPH_HEIGHT);
    free(screenBuffer);
    if (fullScreen != 0 && graphicsType == WINGRAPH_GRAPHICS_WING) {
        SetMenuStatus(1);
        ResizeWindow(x, y, w, h);
    }
    BlitBitmapToScreen(gpWindowManager->m_screen, 0, 0, WINGRAPH_WIDTH, WINGRAPH_HEIGHT, 0, 0);
    UpdatePalette(gpBufferPalette->m_data);
    return TRUE;
}

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

#ifdef HOMM1_EDITOR
#define WINGRAPH_CPP_PATH "U:\\HMM\\VSS\\HMM1\\Source\\Editor\\wingraph.cpp"
#else
#define WINGRAPH_CPP_PATH "E:\\Users\\igorl\\VSS\\HMM\\HMM1\\Source\\Game\\wingraph.cpp"
#endif

BOOL gWinGAttached = TRUE;
BOOL gDDrawAttached = FALSE;
i32 gGraphicsType = WINGRAPH_GRAPHICS_WING;
i32 gMainVideoModeColorDepth = 16;
i32 gMainVideoModeWidth = 1024;
i32 gMainVideoModeHeight = 768;
i32 gOrientation = 1;
WingPalette gLogicalPalette = {0x300, PALETTE_COLOR_COUNT};
void* gInitWin = NULL;
i32 gUnusedPaintCount = 0;
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
HDC gImageDC = NULL;
HBITMAP gOldMonoBitmap = NULL;
HPALETTE gAppPalette = NULL;
HINSTANCE gDDrawLibrary = NULL;
#ifdef HOMM1_EDITOR
i32 gUnusedData453444 = 0;
#endif
RECT gDDClientRect;
RECT gDDSourceRect;
RECT gDDDestinationRect;
i32 gDDResult;
_DDSURFACEDESC gDDSurfaceDesc;
i32 gDDPaintStart;
WingImage gScreenImage;

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

    gDDSPrimary = DDCreateSurface(LOGICAL_SCREEN_WIDTH, LOGICAL_SCREEN_HEIGHT, true);
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

    if (CURRENT_GRAPHICS_CONFIG.fullScreen == 0) {
        result = gDD->CreateClipper(0, &gClipper, NULL);
        if (result != DD_OK)
            DDSD(result, __FILE__, __LINE__);
        result = gClipper->SetHWnd(0, gAppWindow);
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
    if (CURRENT_GRAPHICS_CONFIG.fullScreen != 0) {
        SetMenuStatus(0);
        result = gDD->SetCooperativeLevel(
            gAppWindow,
            DDSCL_EXCLUSIVE | DDSCL_FULLSCREEN | DDSCL_ALLOWREBOOT
        );
        if (result != DD_OK)
            DDSD(result, __FILE__, __LINE__);
        result =
            gDD->SetDisplayMode(LOGICAL_SCREEN_WIDTH, LOGICAL_SCREEN_HEIGHT, WINGRAPH_COLOR_DEPTH);
        if (result != DD_OK)
            DDSD(result, __FILE__, __LINE__);
    } else {
        result = gDD->SetCooperativeLevel(gAppWindow, DDSCL_NORMAL);
        if (result != DD_OK)
            DDSD(result, __FILE__, __LINE__);
    }
    CreatePrimary();
    SetupClipper();
    gDDSOne = DDCreateSurface(LOGICAL_SCREEN_WIDTH, LOGICAL_SCREEN_HEIGHT, false);
    InitializePalette();
}

BOOL DDAppPaint(HWND window, HDC paintDC) {
    i32 srcWidth;
    i32 srcHeight;
    i32 srcTop;
    i32 srcLeft;
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
        paintDC = BeginPaint(window, &ps);
        GetClientRect(window, &gDDClientRect);
        if (ps.rcPaint.right == 0 || ps.rcPaint.bottom == 0)
            ps.rcPaint = gDDClientRect;
        if (ps.rcPaint.right < LOGICAL_SCREEN_WIDTH)
            ps.rcPaint.right++;
        if (ps.rcPaint.bottom < LOGICAL_SCREEN_HEIGHT)
            ps.rcPaint.bottom++;

        gDDDestinationRect = ps.rcPaint;
        srcWidth = CLIENT_TO_GAME_X(gDDDestinationRect.right - gDDDestinationRect.left + 1);
        srcHeight = CLIENT_TO_GAME_Y(gDDDestinationRect.bottom - gDDDestinationRect.top + 1);
        srcLeft = CLIENT_TO_GAME_X(gDDDestinationRect.left);
        srcTop = CLIENT_TO_GAME_Y(gDDDestinationRect.top);
        if (gScrollX != 0) {
            srcLeft = gScrollX + WINGRAPH_SCROLL_MARGIN;
            srcWidth = WINGRAPH_SCROLL_SIZE;
        }
        if (gScrollY != 0) {
            srcTop = gScrollY + WINGRAPH_SCROLL_MARGIN;
            srcHeight = WINGRAPH_SCROLL_SIZE;
        }
        gDDSourceRect.left = srcLeft;
        gDDSourceRect.right = srcLeft + srcWidth - 1;
        gDDSourceRect.top = srcTop;
        gDDSourceRect.bottom = srcTop + srcHeight - 1;

        pt.y = 0;
        pt.x = pt.y;
        ClientToScreen(gAppWindow, &pt);
        OffsetRect(&gDDDestinationRect, pt.x, pt.y);
        gDDResult = gDDSOne->Unlock(NULL);
        if (gDDResult != DD_OK)
            DDSD(gDDResult, __FILE__, __LINE__);

        if (gDDSourceRect.left < 0)
            gDDSourceRect.left = 0;
        if (gDDSourceRect.top < 0)
            gDDSourceRect.top = 0;
        if (gDDSourceRect.right > LOGICAL_SCREEN_WIDTH)
            gDDSourceRect.right = LOGICAL_SCREEN_WIDTH;
        if (gDDSourceRect.bottom > LOGICAL_SCREEN_HEIGHT)
            gDDSourceRect.bottom = LOGICAL_SCREEN_HEIGHT;

        gDDPaintStart = KBTickCount();
        while (TRUE) {
            gDDResult =
                gDDSPrimary->Blt(&gDDDestinationRect, gDDSOne, &gDDSourceRect, DDBLT_WAIT, NULL);
            if (gDDResult == DDERR_SURFACELOST) {
                gDDResult = gDDSPrimary->Restore();
                if (gDDResult == DDERR_WRONGMODE) {
                    gDDResult = gDD->SetDisplayMode(
                        LOGICAL_SCREEN_WIDTH,
                        LOGICAL_SCREEN_HEIGHT,
                        WINGRAPH_COLOR_DEPTH
                    );
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
                       && KBTickCount() < gDDPaintStart + WINGRAPH_PAINT_TIMEOUT) {
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
        if (gWindowManager->m_screen != NULL) {
            gWindowManager->m_screen->m_pixels = static_cast<u8*>(gDDSurfaceDesc.lpSurface);
            gInitWin = gDDSurfaceDesc.lpSurface;
        } else {
            gInitWin = gDDSurfaceDesc.lpSurface;
        }
        if (gDDResult != DD_OK)
            DDSD(gDDResult, __FILE__, __LINE__);
        EndPaint(window, &ps);
        gWinGraphBusy = FALSE;
    }
    return TRUE;
}

void DDInitializePalette() {
    i32 status;
    HDC systemDeviceContext;
    i32 i;
    if (gWinGraphBusy != FALSE)
        return;
    {
        systemDeviceContext = GetDC(NULL);
        GetSystemPaletteEntries(
            systemDeviceContext,
            0,
            WINGRAPH_SYSTEM_PALETTE_SIZE,
            gLogicalPalette.entries
        );
        GetSystemPaletteEntries(
            systemDeviceContext,
            WINGRAPH_MUTABLE_PALETTE_END,
            WINGRAPH_SYSTEM_PALETTE_SIZE,
            &gLogicalPalette.entries[WINGRAPH_MUTABLE_PALETTE_END]
        );
        ReleaseDC(NULL, systemDeviceContext);
        for (i = 0; i < WINGRAPH_SYSTEM_PALETTE_END; i++) {
            gLogicalPalette.entries[i].peFlags = 0;
            gLogicalPalette.entries[WINGRAPH_MUTABLE_PALETTE_END + i].peFlags = 0;
        }
        for (i = WINGRAPH_SYSTEM_PALETTE_SIZE; i < WINGRAPH_MUTABLE_PALETTE_END; i++) {
            gLogicalPalette.entries[i].peRed = 0;
            gLogicalPalette.entries[i].peGreen = 0;
            gLogicalPalette.entries[i].peBlue = 0;
            gLogicalPalette.entries[i].peFlags = PC_NOCOLLAPSE;
        }
        status = gDD->CreatePalette(DDPCAPS_8BIT, gLogicalPalette.entries, &gDDPal, NULL);
        if (status != DD_OK)
            DDSD(status, __FILE__, __LINE__);
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

struct IDirectDrawSurface* DDCreateSurface(u32 width, u32 height, b32 primary) {
    _DDSURFACEDESC ddsd;
    IDirectDrawSurface* createdSurface;
    i32 i;
    i32 tmp;
    i32 status;

    memset(&ddsd, 0, sizeof(ddsd));
    ddsd.dwSize = sizeof(ddsd);
    if (primary != false) {
        ddsd.dwFlags = 0;
        ddsd.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE;
    } else {
        ddsd.dwFlags = DDSD_HEIGHT | DDSD_WIDTH;
        ddsd.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN;
        ddsd.ddsCaps.dwCaps |= DDSCAPS_SYSTEMMEMORY;
        ddsd.dwHeight = height;
        ddsd.dwWidth = width;
    }
    status = gDD->CreateSurface(&ddsd, &createdSurface, NULL);
    if (status != DD_OK)
        DDSD(status, __FILE__, __LINE__);
    if (primary == false) {
        status = createdSurface->Lock(NULL, &ddsd, DDLOCK_WAIT, NULL);
        if (status != DD_OK)
            DDSD(status, __FILE__, __LINE__);
        if (gWindowManager->m_screen != NULL) {
            gWindowManager->m_screen->m_pixels = static_cast<u8*>(ddsd.lpSurface);
            gInitWin = ddsd.lpSurface;
        } else {
            gInitWin = ddsd.lpSurface;
        }
    }
    return createdSurface;
}

void DDSD(i32 error, char* file, i32 line) {
    i32 restoreResult;
    i32 reportCode;

    if (gInDDSD != FALSE)
        return;
    gInDDSD = TRUE;
    restoreResult = gDD->RestoreDisplayMode();
    reportCode = DDSD_REPORT_NONE;
    switch (error) {
        case DD_OK:
            return;
        case DDERR_GENERIC:
            reportCode = DDSD_REPORT_GENERIC;
            break;
        case DDERR_INVALIDCLIPLIST:
            reportCode = DDSD_REPORT_INVALIDCLIPLIST;
            break;
        case DDERR_INVALIDOBJECT:
            reportCode = DDSD_REPORT_INVALIDOBJECT;
            break;
        case DDERR_INVALIDPARAMS:
            reportCode = DDSD_REPORT_INVALIDPARAMS;
            break;
        case DDERR_INVALIDRECT:
            reportCode = DDSD_REPORT_INVALIDRECT;
            break;
        case DDERR_NOALPHAHW:
            reportCode = DDSD_REPORT_NOALPHAHW;
            break;
        case DDERR_NOBLTHW:
            reportCode = DDSD_REPORT_NOBLTHW;
            break;
        case DDERR_NOCLIPLIST:
            reportCode = DDSD_REPORT_NOCLIPLIST;
            break;
        case DDERR_NODDROPSHW:
            reportCode = DDSD_REPORT_NODDROPSHW;
            break;
        case DDERR_SURFACELOST:
            reportCode = DDSD_REPORT_SURFACELOST;
            break;
        case DDERR_UNSUPPORTED:
            reportCode = DDSD_REPORT_UNSUPPORTED;
            break;
        case DDERR_NOMIRRORHW:
            reportCode = DDSD_REPORT_NOMIRRORHW;
            break;
        case DDERR_NORASTEROPHW:
            reportCode = DDSD_REPORT_NORASTEROPHW;
            break;
        case DDERR_NOROTATIONHW:
            reportCode = DDSD_REPORT_NOROTATIONHW;
            break;
        case DDERR_NOSTRETCHHW:
            reportCode = DDSD_REPORT_NOSTRETCHHW;
            break;
        case DDERR_SURFACEBUSY:
            reportCode = DDSD_REPORT_SURFACEBUSY;
            break;
        case DDERR_NOZBUFFERHW:
            reportCode = DDSD_REPORT_NOZBUFFERHW;
            break;
        case DDERR_OUTOFMEMORY:
            reportCode = DDSD_REPORT_OUTOFMEMORY;
            break;
        case DDERR_CLIPPERISUSINGHWND:
            reportCode = DDSD_REPORT_CLIPPERISUSINGHWND;
            break;
        case DDERR_NOEXCLUSIVEMODE:
            reportCode = DDSD_REPORT_NOEXCLUSIVEMODE;
            break;
        case DDERR_NOT8BITCOLOR:
            reportCode = DDSD_REPORT_NOT8BITCOLOR;
            break;
        case DDERR_NOPALETTEATTACHED:
            reportCode = DDSD_REPORT_NOPALETTEATTACHED;
            break;
        case DDERR_NOPALETTEHW:
            reportCode = DDSD_REPORT_NOPALETTEHW;
            break;
        case DDERR_LOCKEDSURFACES:
            reportCode = DDSD_REPORT_LOCKEDSURFACES;
            break;
        case DDERR_IMPLICITLYCREATED:
            reportCode = DDSD_REPORT_IMPLICITLYCREATED;
            break;
        case DDERR_WRONGMODE:
            reportCode = DDSD_REPORT_WRONGMODE;
            break;
        case DDERR_INCOMPATIBLEPRIMARY:
            reportCode = DDSD_REPORT_INCOMPATIBLEPRIMARY;
            break;
        case DDERR_NOCLIPPERATTACHED:
            reportCode = DDSD_REPORT_NOCLIPPERATTACHED;
            break;
        default:
            reportCode = DDSD_REPORT_UNKNOWN;
            break;
    }
    MessageBeep(MB_OK);
    MessageBeep(MB_OK);
    MessageBeep(MB_OK);
    sprintf(gText, "Direct Draw Error #%d in file '%s' at Line #%d", reportCode, file, line);
    ShutDown(gText);
}

void DDUpdatePalette(i8* paletteData) {
    i32 entry;
    i32 status;

    if (gWinGraphBusy != FALSE)
        return;
    if (gForegroundApp == 0)
        return;
    for (entry = WINGRAPH_SYSTEM_PALETTE_SIZE; entry < WINGRAPH_MUTABLE_PALETTE_END; entry++) {
        gLogicalPalette.entries[entry].peRed = paletteData[entry * PALETTE_GRAPHICS_CHANNELS]
                                               << WINGRAPH_PALETTE_VALUE_SHIFT;
        gLogicalPalette.entries[entry].peGreen =
            paletteData[entry * PALETTE_GRAPHICS_CHANNELS + PALETTE_CHANNEL_GREEN]
            << WINGRAPH_PALETTE_VALUE_SHIFT;
        gLogicalPalette.entries[entry].peBlue =
            paletteData[entry * PALETTE_GRAPHICS_CHANNELS + PALETTE_CHANNEL_BLUE]
            << WINGRAPH_PALETTE_VALUE_SHIFT;
        gLogicalPalette.entries[entry].peFlags = PC_NOCOLLAPSE;
    }
    H1_ASSERT(reinterpret_cast<i32>(gDDPal));
    status = gDDPal->SetEntries(
        0,
        WINGRAPH_SYSTEM_PALETTE_SIZE,
        WINGRAPH_MUTABLE_PALETTE_END - WINGRAPH_SYSTEM_PALETTE_END,
        &gLogicalPalette.entries[WINGRAPH_SYSTEM_PALETTE_SIZE]
    );
    if (status != DD_OK)
        DDSD(status, __FILE__, __LINE__);
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
        result = gDD->SetCooperativeLevel(gAppWindow, DDSCL_NORMAL);
        if (result != DD_OK)
            DDSD(result, __FILE__, __LINE__);
        gDD->Release();
        gDD = NULL;
    }
}

void DDSetFullScreenStatus(i32 fullScreen) {
    i32 width;
    i32 x;
    i32 windowHeight;
    i32 y;
    i32 hres;

    if (gWinGraphBusy != FALSE)
        return;
    if (CURRENT_GRAPHICS_CONFIG.fullScreen == fullScreen)
        return;
    {
        x = CURRENT_GRAPHICS_CONFIG.x;
        y = CURRENT_GRAPHICS_CONFIG.y;
        width = CURRENT_GRAPHICS_CONFIG.width;
        windowHeight = CURRENT_GRAPHICS_CONFIG.height;
        gWinGraphBusy = TRUE;
        CURRENT_GRAPHICS_CONFIG.fullScreen = fullScreen;
        if (CURRENT_GRAPHICS_CONFIG.fullScreen != 0)
            SetMenuStatus(0);

        hres = gDD->SetCooperativeLevel(
            gAppWindow,
            DDSCL_EXCLUSIVE | DDSCL_FULLSCREEN | DDSCL_ALLOWREBOOT
        );
        if (hres != DD_OK)
            DDSD(hres, __FILE__, __LINE__);
        if (CURRENT_GRAPHICS_CONFIG.fullScreen != 0) {
            hres = gDD->SetDisplayMode(
                LOGICAL_SCREEN_WIDTH,
                LOGICAL_SCREEN_HEIGHT,
                WINGRAPH_COLOR_DEPTH
            );
            if (hres != DD_OK)
                DDSD(hres, __FILE__, __LINE__);
        } else {
            hres = gDD->RestoreDisplayMode();
            if (hres != DD_OK)
                DDSD(hres, __FILE__, __LINE__);
            hres = gDD->SetCooperativeLevel(gAppWindow, DDSCL_NORMAL);
            if (hres != DD_OK)
                DDSD(hres, __FILE__, __LINE__);
        }
        if (gDDSPrimary != NULL) {
            gDDSPrimary->Release();
            gDDSPrimary = NULL;
        }
        CreatePrimary();
        hres = gDDSPrimary->SetPalette(gDDPal);
        if (hres != DD_OK)
            DDSD(hres, __FILE__, __LINE__);
        WritePrefs();
        gWinGraphBusy = FALSE;
        if (CURRENT_GRAPHICS_CONFIG.fullScreen == 0) {
            SetMenuStatus(1);
            ResizeWindow(x, y, width, windowHeight);
        } else {
            CURRENT_GRAPHICS_CONFIG.x = x;
            CURRENT_GRAPHICS_CONFIG.y = y;
            CURRENT_GRAPHICS_CONFIG.width = width;
            CURRENT_GRAPHICS_CONFIG.height = windowHeight;
        }
        SetupClipper();
    }
}

BOOL WGQueryNewPalette() {
    i32 paletteChanges;
    {
        HDC hdc;

        hdc = GetDC(gAppWindow);
        if (gAppPalette != NULL)
            SelectPalette(hdc, gAppPalette, FALSE);
        paletteChanges = RealizePalette(hdc);
        ReleaseDC(gAppWindow, hdc);
    }
    if (paletteChanges > 0) {
        InvalidateRect(gAppWindow, NULL, TRUE);
        return TRUE;
    } else {
        return FALSE;
    }
}

void WGInitGraphics() {
    HBITMAP bitmap;

    if (gImageDC != NULL)
        return;
    if (WinGRecommendDIBFormat(reinterpret_cast<LPBITMAPINFO>(&gScreenImage))) {
        gScreenImage.header.biBitCount = WINGRAPH_COLOR_DEPTH;
        gScreenImage.header.biCompression = BI_RGB;
        gOrientation = gScreenImage.header.biHeight;
    } else {
        gScreenImage.header.biSize = sizeof(BITMAPINFOHEADER);
        gScreenImage.header.biPlanes = 1;
        gScreenImage.header.biBitCount = WINGRAPH_COLOR_DEPTH;
        gScreenImage.header.biCompression = BI_RGB;
        gScreenImage.header.biSizeImage = 0;
        gScreenImage.header.biClrUsed = 0;
        gScreenImage.header.biClrImportant = 0;
    }
    gScreenImage.header.biWidth = LOGICAL_SCREEN_WIDTH;
    gScreenImage.header.biHeight = -LOGICAL_SCREEN_HEIGHT;
    InitializePalette();
    gImageDC = WinGCreateDC();
    gScreenImage.header.biWidth = LOGICAL_SCREEN_WIDTH;
    gScreenImage.header.biHeight = -LOGICAL_SCREEN_HEIGHT;
    bitmap = WinGCreateBitmap(
        gImageDC,
        reinterpret_cast<LPBITMAPINFO>(&gScreenImage),
        &gScreenImage.bits
    );
    gScreenImage.header.biSizeImage = gScreenImage.header.biWidth * gScreenImage.header.biHeight;
    gScreenImage.header.biSizeImage *= gOrientation;
    gOldMonoBitmap = static_cast<HBITMAP>(SelectObject(gImageDC, bitmap));
    gInitWin = gScreenImage.bits;
    PatBlt(gImageDC, 0, 0, gMainWinScreenWidth, gMainWinScreenHeight, BLACKNESS);
}

void WGUpdatePalette(i8* paletteData) {
    HDC deviceContext;
    i32 result;
    i32 idx;

    for (idx = WINGRAPH_SYSTEM_PALETTE_SIZE; idx < WINGRAPH_MUTABLE_PALETTE_END; idx++) {
        gLogicalPalette.entries[idx].peRed = paletteData[idx * PALETTE_GRAPHICS_CHANNELS]
                                             << WINGRAPH_PALETTE_VALUE_SHIFT;
        gScreenImage.colors[idx].rgbRed = gLogicalPalette.entries[idx].peRed;
        gLogicalPalette.entries[idx].peGreen =
            paletteData[idx * PALETTE_GRAPHICS_CHANNELS + PALETTE_CHANNEL_GREEN]
            << WINGRAPH_PALETTE_VALUE_SHIFT;
        gScreenImage.colors[idx].rgbGreen = gLogicalPalette.entries[idx].peGreen;
        gLogicalPalette.entries[idx].peBlue =
            paletteData[idx * PALETTE_GRAPHICS_CHANNELS + PALETTE_CHANNEL_BLUE]
            << WINGRAPH_PALETTE_VALUE_SHIFT;
        gScreenImage.colors[idx].rgbBlue = gLogicalPalette.entries[idx].peBlue;
    }
    AnimatePalette(
        gAppPalette,
        WINGRAPH_SYSTEM_PALETTE_SIZE,
        WINGRAPH_MUTABLE_PALETTE_END - WINGRAPH_SYSTEM_PALETTE_END,
        &gLogicalPalette.entries[WINGRAPH_SYSTEM_PALETTE_SIZE]
    );
    WinGSetDIBColorTable(
        gImageDC,
        WINGRAPH_SYSTEM_PALETTE_SIZE,
        WINGRAPH_MUTABLE_PALETTE_END - WINGRAPH_SYSTEM_PALETTE_END,
        &gScreenImage.colors[WINGRAPH_SYSTEM_PALETTE_SIZE]
    );
    if (gAppPalette != NULL)
        DeleteObject(gAppPalette);
    gAppPalette = CreatePalette(reinterpret_cast<LPLOGPALETTE>(&gLogicalPalette));
    deviceContext = GetDC(gAppWindow);
    if (gAppPalette != NULL)
        SelectPalette(deviceContext, gAppPalette, FALSE);
    result = RealizePalette(deviceContext);
    ReleaseDC(gAppWindow, deviceContext);
    if (gMainVideoModeColorDepth != WINGRAPH_COLOR_DEPTH) {
#ifdef HOMM1_EDITOR
        {
#else
        if (gLimitedCombatUpdatePalette != false) {
            if (gFullCombatScreenDrawn != false)
                BlitBitmapToScreen(
                    gWindowManager->m_screen,
                    0,
                    0,
                    LOGICAL_SCREEN_WIDTH,
                    WINGRAPH_LIMITED_COMBAT_HEIGHT,
                    0,
                    0
                );
        } else {
#endif
            BlitBitmapToScreen(
                gWindowManager->m_screen,
                0,
                0,
                LOGICAL_SCREEN_WIDTH,
                LOGICAL_SCREEN_HEIGHT,
                0,
                0
            );
        }
    }
}

void WGInitializePalette() {
    HDC screenDC;
    i32 i;

    if (gAppPalette != NULL)
        return;
    screenDC = GetDC(NULL);
    GetSystemPaletteEntries(screenDC, 0, WINGRAPH_SYSTEM_PALETTE_SIZE, gLogicalPalette.entries);
    GetSystemPaletteEntries(
        screenDC,
        WINGRAPH_MUTABLE_PALETTE_END,
        WINGRAPH_SYSTEM_PALETTE_SIZE,
        &gLogicalPalette.entries[WINGRAPH_MUTABLE_PALETTE_END]
    );
    ReleaseDC(NULL, screenDC);
    for (i = 0; i < WINGRAPH_SYSTEM_PALETTE_END; i++) {
        gScreenImage.colors[i].rgbRed = gLogicalPalette.entries[i].peRed;
        gScreenImage.colors[i].rgbGreen = gLogicalPalette.entries[i].peGreen;
        gScreenImage.colors[i].rgbBlue = gLogicalPalette.entries[i].peBlue;
        gScreenImage.colors[i].rgbReserved = 0;
        gLogicalPalette.entries[i].peFlags = 0;
        gScreenImage.colors[WINGRAPH_MUTABLE_PALETTE_END + i].rgbRed =
            gLogicalPalette.entries[WINGRAPH_MUTABLE_PALETTE_END + i].peRed;
        gScreenImage.colors[WINGRAPH_MUTABLE_PALETTE_END + i].rgbGreen =
            gLogicalPalette.entries[WINGRAPH_MUTABLE_PALETTE_END + i].peGreen;
        gScreenImage.colors[WINGRAPH_MUTABLE_PALETTE_END + i].rgbBlue =
            gLogicalPalette.entries[WINGRAPH_MUTABLE_PALETTE_END + i].peBlue;
        gScreenImage.colors[WINGRAPH_MUTABLE_PALETTE_END + i].rgbReserved = 0;
        gLogicalPalette.entries[WINGRAPH_MUTABLE_PALETTE_END + i].peFlags = 0;
    }
    for (i = WINGRAPH_SYSTEM_PALETTE_SIZE; i < WINGRAPH_MUTABLE_PALETTE_END; i++) {
        gScreenImage.colors[i].rgbRed = gLogicalPalette.entries[i].peRed = 0;
        gScreenImage.colors[i].rgbGreen = gLogicalPalette.entries[i].peGreen = 0;
        gScreenImage.colors[i].rgbBlue = gLogicalPalette.entries[i].peBlue = 0;
        gScreenImage.colors[i].rgbReserved = 0;
        gLogicalPalette.entries[i].peFlags = PC_NOCOLLAPSE;
    }
    gAppPalette = CreatePalette(reinterpret_cast<LPLOGPALETTE>(&gLogicalPalette));
}

BOOL WGAppPaint(HWND window, HDC paintDC) {
    RECT rect;
    i8 unusedChar;
    i32 spareDword;
    i32 sourceY;
    i32 blitWidth;
    i32 srcLeft;
    PAINTSTRUCT ps;
    i32 destTop;
    i32 blitX;
    i32 destHeight;

    unusedChar = 0;
    if (gScreenImage.bits != NULL) {
        paintDC = BeginPaint(window, &ps);
        SelectPalette(paintDC, gAppPalette, FALSE);
        RealizePalette(paintDC);
        GetClientRect(window, &rect);
        blitX = 0;
        srcLeft = blitX;
        destTop = 0;
        sourceY = destTop;
        blitWidth = rect.right - rect.left;
        destHeight = rect.bottom - rect.top;
        srcLeft = CLIENT_TO_GAME_X(blitX);
        sourceY = CLIENT_TO_GAME_Y(destTop);
        if (gScrollX != 0)
            srcLeft += gScrollX;
        if (gScrollY != 0)
            sourceY += gScrollY;
        gTtlBlts++;
        if (gMainWinScreenWidth == LOGICAL_SCREEN_WIDTH
            && gMainWinScreenHeight == LOGICAL_SCREEN_HEIGHT) {
            blitX = ps.rcPaint.left & WINGRAPH_PAINT_ALIGN_MASK;
            blitWidth = ps.rcPaint.right - blitX + 1;
            destTop = ps.rcPaint.top;
            destHeight = ps.rcPaint.bottom - destTop + 1;
            WinGBitBlt(
                paintDC,
                blitX,
                destTop,
                blitWidth,
                destHeight,
                gImageDC,
                blitX + gScrollX,
                destTop + gScrollY
            );
        } else {
            WinGStretchBlt(
                paintDC,
                blitX,
                destTop,
                blitWidth,
                destHeight,
                gImageDC,
                srcLeft,
                sourceY,
                CLIENT_TO_GAME_X(blitWidth),
                CLIENT_TO_GAME_Y(destHeight)
            );
        }
        EndPaint(window, &ps);
    }
    return TRUE;
}

void WGCleanUpWinGraphics() {
    HGDIOBJ bitmap;

    if (gImageDC != NULL) {
        bitmap = SelectObject(gImageDC, gOldMonoBitmap);
        DeleteObject(bitmap);
        DeleteDC(gImageDC);
        gImageDC = NULL;
    }
    if (gAppPalette != NULL) {
        DeleteObject(gAppPalette);
        gAppPalette = NULL;
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
            ShutDown(localization::Tr("display.color_mode.required"));
    }
}

void InitGraphics() {
    ConnectToDLLs();
    if (CURRENT_GRAPHICS_CONFIG.fullScreen != 0)
        gGraphicsType = WINGRAPH_GRAPHICS_DIRECT_DRAW;
    else
        gGraphicsType = WINGRAPH_GRAPHICS_WING;
    if (gGraphicsType == WINGRAPH_GRAPHICS_WING)
        WGInitGraphics();
    else
        DDInitGraphics();
}

BOOL AppPaint(HWND window, HDC paintDC) {
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
    if (gInSmacker != false)
        return;
    if (fullScreen == CURRENT_GRAPHICS_CONFIG.fullScreen)
        return;
    if (gGraphicsType == WINGRAPH_GRAPHICS_WING) {
        CURRENT_GRAPHICS_CONFIG.fullScreen = 1;
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
    void* buffer;
    i32 width;
    i32 fullState;
    i32 x;
    i32 hgt;
    i32 y;

    if (gGraphicsType == graphicsType)
        return TRUE;
    if (graphicsType == WINGRAPH_GRAPHICS_WING && gWinGAttached == FALSE)
        return FALSE;
    if (graphicsType == WINGRAPH_GRAPHICS_DIRECT_DRAW && gDDrawAttached == FALSE)
        return FALSE;

    fullState = CURRENT_GRAPHICS_CONFIG.fullScreen;
    x = CURRENT_GRAPHICS_CONFIG.x;
    y = CURRENT_GRAPHICS_CONFIG.y;
    width = CURRENT_GRAPHICS_CONFIG.width;
    hgt = CURRENT_GRAPHICS_CONFIG.height;
    buffer = malloc(LOGICAL_SCREEN_WIDTH * LOGICAL_SCREEN_HEIGHT);
    memcpy(
        buffer,
        gWindowManager->m_screen->m_pixels,
        LOGICAL_SCREEN_WIDTH * LOGICAL_SCREEN_HEIGHT
    );
    if (graphicsType == WINGRAPH_GRAPHICS_WING) {
        CURRENT_GRAPHICS_CONFIG.fullScreen = 0;
        DDCleanUpWinGraphics();
        gGraphicsType = WINGRAPH_GRAPHICS_WING;
        WGInitGraphics();
        gWindowManager->m_screen->m_pixels = static_cast<u8*>(gInitWin);
    } else {
        WGCleanUpWinGraphics();
        gGraphicsType = WINGRAPH_GRAPHICS_DIRECT_DRAW;
        DDInitGraphics();
        gWindowManager->m_screen->m_pixels = static_cast<u8*>(gInitWin);
    }
    memcpy(
        gWindowManager->m_screen->m_pixels,
        buffer,
        LOGICAL_SCREEN_WIDTH * LOGICAL_SCREEN_HEIGHT
    );
    free(buffer);
    if (fullState != 0 && graphicsType == WINGRAPH_GRAPHICS_WING) {
        SetMenuStatus(1);
        ResizeWindow(x, y, width, hgt);
    }
    BlitBitmapToScreen(
        gWindowManager->m_screen,
        0,
        0,
        LOGICAL_SCREEN_WIDTH,
        LOGICAL_SCREEN_HEIGHT,
        0,
        0
    );
    UpdatePalette(gBufferPalette->m_data);
    return TRUE;
}

#include <match.h>

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

// wingraph owns retail .data 0x0049fe60-0x0048eb17 and .bss
// 0x004a46a0-0x004a4b7f.
DATA(0x004a0190)
BOOL gWinGAttached = TRUE;
DATA(0x004cdda0)
BOOL gDDrawAttached = FALSE;
DATA(0x004a0194)
H1_ENUM_STORAGE(WingraphGraphicsType, i32) gGraphicsType = WINGRAPH_GRAPHICS_WING;
DATA(0x004a0198)
i32 gMainVideoModeColorDepth = 16;
DATA(0x004a019c)
i32 gMainVideoModeWidth = 1024;
DATA(0x004a01a0)
i32 gMainVideoModeHeight = 768;
DATA(0x004a01a4)
i32 Orientation = 1;
DATA(0x004a01a8)
WingPalette LogicalPalette = {0x300, WINGRAPH_PALETTE_SIZE};
DATA(0x004cdda4)
void* gInitWin = NULL;
// Image and scroll counters of the WinG paint path.
DATA(0x004cddac)
i32 gTtlBlts = 0;
DATA(0x004cddb0)
BOOL gWinGraphBusy = FALSE;
DATA(0x004cddb4)
DirectDrawCreateProc gDirectDrawCreate = NULL;
DATA(0x004cddb8)
IDirectDraw* gDD = NULL;
DATA(0x004cddbc)
IDirectDrawSurface* gDDSPrimary = NULL;
DATA(0x004cddc0)
IDirectDrawSurface* gDDSOne = NULL;
DATA(0x004cddc4)
IDirectDrawClipper* gClipper = NULL;
DATA(0x004cddc8)
IDirectDrawPalette* gDDPal = NULL;
DATA(0x004cddcc)
i32 gBusyRetry = 0;
DATA(0x004cddd0)
BOOL gInDDSD = FALSE;
DATA(0x004cddd4)
HDC hdcImage = NULL;
DATA(0x004cddd8)
HBITMAP gbmOldMonoBitmap = NULL;
DATA(0x004cdddc)
HPALETTE hpalApp = NULL;
DATA(0x004cdde0)
HINSTANCE gDDrawLibrary = NULL;
DATA(0x004cdd78)
RECT gDDClientRect;
DATA(0x004cd938)
RECT gDDSourceRect;
DATA(0x004cdd90)
RECT gDDDestinationRect;
DATA(0x004cd8c0)
i32 gDDResult;
DATA(0x004cd8c8)
_DDSURFACEDESC gDDSurfaceDesc;
DATA(0x004cdd88)
i32 gPaintStart;
DATA(0x004cd948)
WingImage screenImage;

VA(0x00466710, 0x3e)
#line 49 "E:\\Users\\igorl\\VSS\\HMM\\HMM1\\Source\\Game\\wingraph.cpp"
void DDRestoreDisplayMode() {
    i32 result;
    if (gDD != NULL) {
        result = gDD->RestoreDisplayMode();
        if (result != DD_OK)
#line 56
            DDSD(result, __FILE__, __LINE__);
    }
}

VA(0x0046674e, 0x2d)
BOOL DDQueryNewPalette() {
    i32 unused;

    if (gWinGraphBusy)
        return TRUE;
    if (!gForegroundApp)
        return TRUE;
    return SetPalette();
}

VA(0x0046677b, 0x7d)
#line 71 "E:\\Users\\igorl\\VSS\\HMM\\HMM1\\Source\\Game\\wingraph.cpp"
void CreatePrimary(void) {
    i32 result;

    gDDSPrimary = DDCreateSurface(WINGRAPH_WIDTH, WINGRAPH_HEIGHT, 1);
    if (gClipper != NULL) {
        result = gDDSPrimary->SetClipper(NULL);
        if (result != DD_OK && result != DDERR_NOCLIPPERATTACHED)
#line 81
            DDSD(result, __FILE__, __LINE__);
        gClipper->Release();
        gClipper = NULL;
    }
}

VA(0x004667f8, 0xbe)
#line 91 "E:\\Users\\igorl\\VSS\\HMM\\HMM1\\Source\\Game\\wingraph.cpp"
void SetupClipper(void) {
    i32 result;

    if (CURRENT_GRAPHICS_CONFIG.fullScreen == 0) {
        result = gDD->CreateClipper(0, &gClipper, NULL);
        if (result != DD_OK)
#line 99
            DDSD(result, __FILE__, __LINE__);
        result = gClipper->SetHWnd(0, hwndApp);
        if (result != DD_OK)
#line 104
            DDSD(result, __FILE__, __LINE__);
        result = gDDSPrimary->SetClipper(gClipper);
        if (result != DD_OK)
#line 109
            DDSD(result, __FILE__, __LINE__);
    }
}

VA(0x004668b6, 0x139)
#line 114 "E:\\Users\\igorl\\VSS\\HMM\\HMM1\\Source\\Game\\wingraph.cpp"
void DDInitGraphics(void) {
    i32 result;

    if (gWinGraphBusy != FALSE)
        return;
    result = gDirectDrawCreate(NULL, &gDD, NULL);
    if (result != DD_OK)
#line 122
        DDSD(result, __FILE__, __LINE__);
    if (CURRENT_GRAPHICS_CONFIG.fullScreen != 0) {
        SetMenuStatus(0);
        result = gDD->SetCooperativeLevel(
            hwndApp,
            DDSCL_EXCLUSIVE | DDSCL_FULLSCREEN | DDSCL_ALLOWREBOOT
        );
        if (result != DD_OK)
#line 134
            DDSD(result, __FILE__, __LINE__);
        result = gDD->SetDisplayMode(WINGRAPH_WIDTH, WINGRAPH_HEIGHT, WINGRAPH_COLOR_DEPTH);
        if (result != DD_OK)
#line 138
            DDSD(result, __FILE__, __LINE__);
    } else {
        result = gDD->SetCooperativeLevel(hwndApp, DDSCL_NORMAL);
        if (result != DD_OK)
#line 145
            DDSD(result, __FILE__, __LINE__);
    }
    CreatePrimary();
    SetupClipper();
    gDDSOne = DDCreateSurface(WINGRAPH_WIDTH, WINGRAPH_HEIGHT, 0);
    InitializePalette();
}

VA(0x004669ef, 0x4e4)
#line 161 "E:\\Users\\igorl\\VSS\\HMM\\HMM1\\Source\\Game\\wingraph.cpp"
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
        if (ps.rcPaint.right < WINGRAPH_PAINT_X_END)
            ps.rcPaint.right++;
        if (ps.rcPaint.bottom < WINGRAPH_PAINT_Y_END)
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
        ClientToScreen(hwndApp, &pt);
        OffsetRect(&gDDDestinationRect, pt.x, pt.y);
        gDDResult = gDDSOne->Unlock(NULL);
        if (gDDResult != DD_OK)
#line 233
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
        while (TRUE) {
            gDDResult =
                gDDSPrimary->Blt(&gDDDestinationRect, gDDSOne, &gDDSourceRect, DDBLT_WAIT, NULL);
            if (gDDResult == DDERR_SURFACELOST) {
                gDDResult = gDDSPrimary->Restore();
                if (gDDResult == DDERR_WRONGMODE) {
                    gDDResult =
                        gDD->SetDisplayMode(WINGRAPH_WIDTH, WINGRAPH_HEIGHT, WINGRAPH_COLOR_DEPTH);
                    if (gDDResult != DD_OK)
#line 252
                        DDSD(gDDResult, __FILE__, __LINE__);
                    gDDResult = gDDSPrimary->Restore();
                    if (gDDResult != DD_OK)
#line 256
                        DDSD(gDDResult, __FILE__, __LINE__);
                    gDDDestinationRect = gDDSourceRect;
                }
                if (gDDResult != DD_OK)
#line 261
                    DDSD(gDDResult, __FILE__, __LINE__);
            } else if (gDDResult == DDERR_SURFACEBUSY
                       && KBTickCount() < gPaintStart + WINGRAPH_PAINT_TIMEOUT) {
                gBusyRetry++;
            } else if (gDDResult != DD_OK) {
#line 266
                DDSD(gDDResult, __FILE__, __LINE__);
            } else {
                break;
            }
        }

        memset(&gDDSurfaceDesc, 0, sizeof(gDDSurfaceDesc));
        gDDSurfaceDesc.dwSize = sizeof(gDDSurfaceDesc);
        gDDResult = gDDSOne->Lock(NULL, &gDDSurfaceDesc, DDLOCK_WAIT, NULL);
        if (gDDResult != DD_OK)
#line 276
            DDSD(gDDResult, __FILE__, __LINE__);
        if (gpWindowManager->m_screen != NULL) {
            gpWindowManager->m_screen->m_pixels = static_cast<u8*>(gDDSurfaceDesc.lpSurface);
            gInitWin = gDDSurfaceDesc.lpSurface;
        } else {
            gInitWin = gDDSurfaceDesc.lpSurface;
        }
        if (gDDResult != DD_OK)
#line 287
            DDSD(gDDResult, __FILE__, __LINE__);
        EndPaint(window, &ps);
        gWinGraphBusy = FALSE;
    }
    return TRUE;
}

VA(0x00466ed3, 0x115)
#line 315 "E:\\Users\\igorl\\VSS\\HMM\\HMM1\\Source\\Game\\wingraph.cpp"
void DDInitializePalette() {
    i32 ddrval;
    HDC curHdc;
    i32 i;
    if (gWinGraphBusy != FALSE)
        return;
    {
        curHdc = GetDC(NULL);
        GetSystemPaletteEntries(curHdc, 0, WINGRAPH_SYSTEM_PALETTE_SIZE, LogicalPalette.entries);
        GetSystemPaletteEntries(
            curHdc,
            WINGRAPH_MUTABLE_PALETTE_END,
            WINGRAPH_SYSTEM_PALETTE_SIZE,
            &LogicalPalette.entries[WINGRAPH_MUTABLE_PALETTE_END]
        );
        ReleaseDC(NULL, curHdc);
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
#line 360
            DDSD(ddrval, __FILE__, __LINE__);
        SetPalette();
    }
}

VA(0x00466fe8, 0x82)
#line 387 "E:\\Users\\igorl\\VSS\\HMM\\HMM1\\Source\\Game\\wingraph.cpp"
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
#line 389
        DDSD(result, __FILE__, __LINE__);
    return FALSE;
}

VA(0x0046706a, 0x100)
#line 417 "E:\\Users\\igorl\\VSS\\HMM\\HMM1\\Source\\Game\\wingraph.cpp"
struct IDirectDrawSurface* DDCreateSurface(u32 width, u32 height, i32 primary) {
    _DDSURFACEDESC ddsd;
    IDirectDrawSurface* lpSurface;
    i32 i;
    i32 tmp;
    i32 ddrval;

    memset(&ddsd, 0, sizeof(ddsd));
    ddsd.dwSize = sizeof(ddsd);
    if (primary != 0) {
        // Retail stores no DDSD_CAPS bit for the primary surface.
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
#line 427
        DDSD(ddrval, __FILE__, __LINE__);
    if (primary == 0) {
        ddrval = lpSurface->Lock(NULL, &ddsd, DDLOCK_WAIT, NULL);
        if (ddrval != DD_OK)
#line 435
            DDSD(ddrval, __FILE__, __LINE__);
        if (gpWindowManager->m_screen != NULL) {
            gpWindowManager->m_screen->m_pixels = static_cast<u8*>(ddsd.lpSurface);
            gInitWin = ddsd.lpSurface;
        } else {
            gInitWin = ddsd.lpSurface;
        }
    }
    return lpSurface;
}

VA(0x0046716a, 0x3ed)
void DDSD(i32 error, char* file, i32 line) {
    i32 restoreResult;
    H1_ENUM_STORAGE(DirectDrawReportCode, i32) unused;

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
    ShutDown(gText);
}

VA(0x00467557, 0xf5)
#line 524 "E:\\Users\\igorl\\VSS\\HMM\\HMM1\\Source\\Game\\wingraph.cpp"
void DDUpdatePalette(i8* paletteData) {
    i32 entry;
    i32 curRes;

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
    // API-forced: ProcessAssert accepts the pointer assertion as a 32-bit int.
#line 521
    H1_ASSERT(reinterpret_cast<i32>(gDDPal));
    curRes = gDDPal->SetEntries(
        0,
        WINGRAPH_SYSTEM_PALETTE_SIZE,
        WINGRAPH_PALETTE_SIZE - WINGRAPH_SYSTEM_PALETTE_SIZE * 2,
        &LogicalPalette.entries[WINGRAPH_SYSTEM_PALETTE_SIZE]
    );
    if (curRes != DD_OK)
#line 525
        DDSD(curRes, __FILE__, __LINE__);
}

VA(0x0046764c, 0x154)
#line 550 "E:\\Users\\igorl\\VSS\\HMM\\HMM1\\Source\\Game\\wingraph.cpp"
void DDCleanUpWinGraphics(void) {
    i32 restoreVal;
    i32 result;

    if (gDD != NULL) {
        restoreVal = gDD->RestoreDisplayMode();
        if (gClipper != NULL) {
            if (gDDSPrimary != NULL) {
                result = gDDSPrimary->SetClipper(NULL);
                if (result != DD_OK && result != DDERR_NOCLIPPERATTACHED)
#line 543
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
#line 567
            DDSD(result, __FILE__, __LINE__);
        gDD->Release();
        gDD = NULL;
    }
}

VA(0x004677a0, 0x291)
#line 596 "E:\\Users\\igorl\\VSS\\HMM\\HMM1\\Source\\Game\\wingraph.cpp"
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
            hwndApp,
            DDSCL_EXCLUSIVE | DDSCL_FULLSCREEN | DDSCL_ALLOWREBOOT
        );
        if (hres != DD_OK)
#line 596
            DDSD(hres, __FILE__, __LINE__);
        if (CURRENT_GRAPHICS_CONFIG.fullScreen != 0) {
            hres = gDD->SetDisplayMode(WINGRAPH_WIDTH, WINGRAPH_HEIGHT, WINGRAPH_COLOR_DEPTH);
            if (hres != DD_OK)
#line 602
                DDSD(hres, __FILE__, __LINE__);
        } else {
            hres = gDD->RestoreDisplayMode();
            if (hres != DD_OK)
#line 609
                DDSD(hres, __FILE__, __LINE__);
            hres = gDD->SetCooperativeLevel(hwndApp, DDSCL_NORMAL);
            if (hres != DD_OK)
#line 614
                DDSD(hres, __FILE__, __LINE__);
        }
        if (gDDSPrimary != NULL) {
            gDDSPrimary->Release();
            gDDSPrimary = NULL;
        }
        CreatePrimary();
        hres = gDDSPrimary->SetPalette(gDDPal);
        if (hres != DD_OK)
#line 626
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

// The WinG palette path uses the application window and palette handles.
VA(0x00467a31, 0x72)
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

// WinG backend initializer: the logical-palette and DIB records; entries
// 10..245 are mutable.
VA(0x00467aa3, 0x13b)
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
    screenImage.header.biSizeImage = screenImage.header.biWidth * screenImage.header.biHeight;
    screenImage.header.biSizeImage *= Orientation;
    gbmOldMonoBitmap = static_cast<HBITMAP>(SelectObject(hdcImage, bitmap));
    gInitWin = screenImage.bits;
    PatBlt(hdcImage, 0, 0, iMainWinScreenWidth, gMainWinScreenHeight, BLACKNESS);
}

VA(0x00467bde, 0x1bd)
void WGUpdatePalette(i8* paletteData) {
    HDC deviceContext;
    i32 result;
    i32 idx;

    for (idx = WINGRAPH_SYSTEM_PALETTE_SIZE; idx < WINGRAPH_MUTABLE_PALETTE_END; idx++) {
        LogicalPalette.entries[idx].peRed = paletteData[idx * 3] << 2;
        screenImage.colors[idx].rgbRed = LogicalPalette.entries[idx].peRed;
        LogicalPalette.entries[idx].peGreen = paletteData[idx * 3 + 1] << 2;
        screenImage.colors[idx].rgbGreen = LogicalPalette.entries[idx].peGreen;
        LogicalPalette.entries[idx].peBlue = paletteData[idx * 3 + 2] << 2;
        screenImage.colors[idx].rgbBlue = LogicalPalette.entries[idx].peBlue;
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
    deviceContext = GetDC(hwndApp);
    if (hpalApp != NULL)
        SelectPalette(deviceContext, hpalApp, FALSE);
    result = RealizePalette(deviceContext);
    ReleaseDC(hwndApp, deviceContext);
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

// Initializes the system-reserved WinG colors and leaves the mutable
// interior flagged for palette animation.
VA(0x00467d9b, 0x1a2)
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

// The client-to-game transform uses the pinned 640x480 viewport.
VA(0x00467f3d, 0x1b7)
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
    if (screenImage.bits != NULL) {
        paintDC = BeginPaint(window, &ps);
        SelectPalette(paintDC, hpalApp, FALSE);
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
        if (iMainWinScreenWidth == WINGRAPH_WIDTH && gMainWinScreenHeight == WINGRAPH_HEIGHT) {
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
                hdcImage,
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
                hdcImage,
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

VA(0x004680f4, 0x67)
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

// The failed-factory arm invokes ShutDown with its own error string.
VA(0x0046815b, 0x56)
void ConnectToDLLs() {
    gDDrawLibrary = LoadLibraryA("DDRAW.DLL");
    if (reinterpret_cast<u32>(gDDrawLibrary)
        >= HINSTANCE_ERROR) { // API-forced: LoadLibrary returns an error code below HINSTANCE_ERROR
        // API-forced: GetProcAddress returns FARPROC for the typed DirectDraw factory.
        gDirectDrawCreate = reinterpret_cast<DirectDrawCreateProc>(
            GetProcAddress(gDDrawLibrary, "DirectDrawCreate")
        );
        if (gDirectDrawCreate != NULL)
            gDDrawAttached = TRUE;
        else
            ShutDown("Error loading DDRAW.DLL");
    }
}

// The DLL teardown checks the Win32 module handle before release.
VA(0x004681b1, 0x1a)
void DisconnectDLLs() {
    if (reinterpret_cast<u32>(gDDrawLibrary)
        >= HINSTANCE_ERROR) // API-forced: LoadLibrary returns an error code below HINSTANCE_ERROR
        FreeLibrary(gDDrawLibrary);
}

// @dead-code
// Zero-ref: pinned retail has no incoming direct call/jump or relocated reference.
VA(0x004681cb, 0x15)
void RestoreDisplayMode() {
    if (gGraphicsType == WINGRAPH_GRAPHICS_WING)
        return;
    else
        DDRestoreDisplayMode();
}

VA(0x004681e0, 0x17)
BOOL SetPalette() {
    if (gGraphicsType == WINGRAPH_GRAPHICS_WING)
        return FALSE;
    else
        return DDSetPalette();
}

VA(0x004681f7, 0x6e)
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

VA(0x00468265, 0x46)
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

// Returns the selected backend's paint result.
VA(0x004682ab, 0x30)
BOOL AppPaint(HWND window, HDC paintDC) {
    if (gGraphicsType == WINGRAPH_GRAPHICS_WING)
        return WGAppPaint(window, paintDC);
    else
        return DDAppPaint(window, paintDC);
}

VA(0x004682db, 0x1a)
void InitializePalette() {
    if (gGraphicsType == WINGRAPH_GRAPHICS_WING)
        WGInitializePalette();
    else
        DDInitializePalette();
}

// Dispatches the palette buffer to the selected backend.
VA(0x004682f5, 0x28)
void UpdatePalette(i8* paletteData) {
    if (gGraphicsType == WINGRAPH_GRAPHICS_WING)
        WGUpdatePalette(paletteData);
    else
        DDUpdatePalette(paletteData);
}

VA(0x0046831d, 0x1f)
void CleanUpWinGraphics() {
    if (gGraphicsType == WINGRAPH_GRAPHICS_WING)
        WGCleanUpWinGraphics();
    else
        DDCleanUpWinGraphics();
    DisconnectDLLs();
}

VA(0x0046833c, 0x84)
void SetFullScreenStatus(i32 fullScreen) {
    if (gInSmacker != 0)
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

VA(0x004683c0, 0x1a)
BOOL QueryNewPalette() {
    if (gGraphicsType == WINGRAPH_GRAPHICS_WING)
        return WGQueryNewPalette();
    else
        return DDQueryNewPalette();
}

VA(0x004683da, 0x1c6)
BOOL SetGraphicsType(H1_ENUM_PARAM(WingraphGraphicsType, i32) graphicsType) {
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
    buffer = malloc(WINGRAPH_WIDTH * WINGRAPH_HEIGHT);
    memcpy(buffer, gpWindowManager->m_screen->m_pixels, WINGRAPH_WIDTH * WINGRAPH_HEIGHT);
    if (graphicsType == WINGRAPH_GRAPHICS_WING) {
        CURRENT_GRAPHICS_CONFIG.fullScreen = 0;
        DDCleanUpWinGraphics();
        gGraphicsType = WINGRAPH_GRAPHICS_WING;
        WGInitGraphics();
        gpWindowManager->m_screen->m_pixels = static_cast<u8*>(gInitWin);
    } else {
        WGCleanUpWinGraphics();
        gGraphicsType = WINGRAPH_GRAPHICS_DIRECT_DRAW;
        DDInitGraphics();
        gpWindowManager->m_screen->m_pixels = static_cast<u8*>(gInitWin);
    }
    memcpy(gpWindowManager->m_screen->m_pixels, buffer, WINGRAPH_WIDTH * WINGRAPH_HEIGHT);
    free(buffer);
    if (fullState != 0 && graphicsType == WINGRAPH_GRAPHICS_WING) {
        SetMenuStatus(1);
        ResizeWindow(x, y, width, hgt);
    }
    BlitBitmapToScreen(gpWindowManager->m_screen, 0, 0, WINGRAPH_WIDTH, WINGRAPH_HEIGHT, 0, 0);
    UpdatePalette(gpBufferPalette->m_data);
    return TRUE;
}

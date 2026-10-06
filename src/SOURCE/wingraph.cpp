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

// Retail assertion paths: EDITOR.EXE links its own compile of this file
// (Source\Editor\wingraph.cpp, in another checkout).
#ifdef HOMM1_EDITOR
#define WINGRAPH_CPP_PATH "U:\\HMM\\VSS\\HMM1\\Source\\Editor\\wingraph.cpp"
#else
#define WINGRAPH_CPP_PATH "E:\\Users\\igorl\\VSS\\HMM\\HMM1\\Source\\Game\\wingraph.cpp"
#endif

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
i32 gOrientation = 1;
DATA(0x004a01a8)
WingPalette gLogicalPalette = {0x300, PALETTE_COLOR_COUNT};
DATA(0x004cdda4)
void* gInitWin = NULL;
// No retail code reads this; it holds its retail .bss place.
DATA(0x004cdda8)
i32 gUnusedPaintCount = 0;
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
HDC gImageDC = NULL;
DATA(0x004cddd8)
HBITMAP gOldMonoBitmap = NULL;
DATA(0x004cdddc)
HPALETTE gAppPalette = NULL;
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
i32 gDDPaintStart;
DATA(0x004cd948)
WingImage gScreenImage;

VA(0x00466710, 0x3e)
#line 49 WINGRAPH_CPP_PATH
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
#line 71 WINGRAPH_CPP_PATH
void CreatePrimary(void) {
    i32 result;

    gDDSPrimary = DDCreateSurface(LOGICAL_SCREEN_WIDTH, LOGICAL_SCREEN_HEIGHT, 1);
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
#line 91 WINGRAPH_CPP_PATH
void SetupClipper(void) {
    i32 result;

    if (CURRENT_GRAPHICS_CONFIG.fullScreen == 0) {
        result = gDD->CreateClipper(0, &gClipper, NULL);
        if (result != DD_OK)
#line 99
            DDSD(result, __FILE__, __LINE__);
        result = gClipper->SetHWnd(0, gAppWindow);
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
#line 114 WINGRAPH_CPP_PATH
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
            gAppWindow,
            DDSCL_EXCLUSIVE | DDSCL_FULLSCREEN | DDSCL_ALLOWREBOOT
        );
        if (result != DD_OK)
#line 134
            DDSD(result, __FILE__, __LINE__);
        result =
            gDD->SetDisplayMode(LOGICAL_SCREEN_WIDTH, LOGICAL_SCREEN_HEIGHT, WINGRAPH_COLOR_DEPTH);
        if (result != DD_OK)
#line 138
            DDSD(result, __FILE__, __LINE__);
    } else {
        result = gDD->SetCooperativeLevel(gAppWindow, DDSCL_NORMAL);
        if (result != DD_OK)
#line 145
            DDSD(result, __FILE__, __LINE__);
    }
    CreatePrimary();
    SetupClipper();
    gDDSOne = DDCreateSurface(LOGICAL_SCREEN_WIDTH, LOGICAL_SCREEN_HEIGHT, 0);
    InitializePalette();
}

VA(0x004669ef, 0x4e4)
VA_AT(editor, 0x0041976f, 0x4e3)
#line 161 WINGRAPH_CPP_PATH
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
#line 233
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
                       && KBTickCount() < gDDPaintStart + WINGRAPH_PAINT_TIMEOUT) {
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
        if (gWindowManager->m_screen != NULL) {
            gWindowManager->m_screen->m_pixels = static_cast<u8*>(gDDSurfaceDesc.lpSurface);
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
#line 315 WINGRAPH_CPP_PATH
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
#line 360
            DDSD(status, __FILE__, __LINE__);
        SetPalette();
    }
}

VA(0x00466fe8, 0x82)
#line 387 WINGRAPH_CPP_PATH
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
VA_AT(editor, 0x00419de9, 0xfe)
#line 417 WINGRAPH_CPP_PATH
struct IDirectDrawSurface* DDCreateSurface(u32 width, u32 height, i32 primary) {
    _DDSURFACEDESC ddsd;
    IDirectDrawSurface* createdSurface;
    i32 i;
    i32 tmp;
    i32 status;

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
    status = gDD->CreateSurface(&ddsd, &createdSurface, NULL);
    if (status != DD_OK)
#line 427
        DDSD(status, __FILE__, __LINE__);
    if (primary == 0) {
        status = createdSurface->Lock(NULL, &ddsd, DDLOCK_WAIT, NULL);
        if (status != DD_OK)
#line 435
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

VA(0x0046716a, 0x3ed)
void DDSD(i32 error, char* file, i32 line) {
    i32 restoreResult;
    H1_ENUM_STORAGE(DirectDrawReportCode, i32) reportCode;

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

VA(0x00467557, 0xf5)
#line 524 WINGRAPH_CPP_PATH
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
        gLogicalPalette.entries[entry].peGreen = paletteData[entry * PALETTE_GRAPHICS_CHANNELS + 1]
                                                 << WINGRAPH_PALETTE_VALUE_SHIFT;
        gLogicalPalette.entries[entry].peBlue = paletteData[entry * PALETTE_GRAPHICS_CHANNELS + 2]
                                                << WINGRAPH_PALETTE_VALUE_SHIFT;
        gLogicalPalette.entries[entry].peFlags = PC_NOCOLLAPSE;
    }
    // API-forced: ProcessAssert accepts the pointer assertion as a 32-bit int.
#line 521
    H1_ASSERT(reinterpret_cast<i32>(gDDPal));
    status = gDDPal->SetEntries(
        0,
        WINGRAPH_SYSTEM_PALETTE_SIZE,
        PALETTE_COLOR_COUNT - WINGRAPH_SYSTEM_PALETTE_SIZE * 2,
        &gLogicalPalette.entries[WINGRAPH_SYSTEM_PALETTE_SIZE]
    );
    if (status != DD_OK)
#line 525
        DDSD(status, __FILE__, __LINE__);
}

VA(0x0046764c, 0x154)
#line 550 WINGRAPH_CPP_PATH
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
        result = gDD->SetCooperativeLevel(gAppWindow, DDSCL_NORMAL);
        if (result != DD_OK)
#line 567
            DDSD(result, __FILE__, __LINE__);
        gDD->Release();
        gDD = NULL;
    }
}

VA(0x004677a0, 0x291)
#line 596 WINGRAPH_CPP_PATH
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
#line 596
            DDSD(hres, __FILE__, __LINE__);
        if (CURRENT_GRAPHICS_CONFIG.fullScreen != 0) {
            hres = gDD->SetDisplayMode(
                LOGICAL_SCREEN_WIDTH,
                LOGICAL_SCREEN_HEIGHT,
                WINGRAPH_COLOR_DEPTH
            );
            if (hres != DD_OK)
#line 602
                DDSD(hres, __FILE__, __LINE__);
        } else {
            hres = gDD->RestoreDisplayMode();
            if (hres != DD_OK)
#line 609
                DDSD(hres, __FILE__, __LINE__);
            hres = gDD->SetCooperativeLevel(gAppWindow, DDSCL_NORMAL);
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

// WinG backend initializer: the logical-palette and DIB records; entries
// 10..245 are mutable.
VA(0x00467aa3, 0x13b)
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

VA(0x00467bde, 0x1bd)
VA_AT(editor, 0x0041a95b, 0x186)
void WGUpdatePalette(i8* paletteData) {
    HDC deviceContext;
    i32 result;
    i32 idx;

    for (idx = WINGRAPH_SYSTEM_PALETTE_SIZE; idx < WINGRAPH_MUTABLE_PALETTE_END; idx++) {
        gLogicalPalette.entries[idx].peRed = paletteData[idx * 3] << 2;
        gScreenImage.colors[idx].rgbRed = gLogicalPalette.entries[idx].peRed;
        gLogicalPalette.entries[idx].peGreen = paletteData[idx * 3 + 1] << 2;
        gScreenImage.colors[idx].rgbGreen = gLogicalPalette.entries[idx].peGreen;
        gLogicalPalette.entries[idx].peBlue = paletteData[idx * 3 + 2] << 2;
        gScreenImage.colors[idx].rgbBlue = gLogicalPalette.entries[idx].peBlue;
    }
    AnimatePalette(
        gAppPalette,
        WINGRAPH_SYSTEM_PALETTE_SIZE,
        PALETTE_COLOR_COUNT - WINGRAPH_SYSTEM_PALETTE_SIZE * 2,
        &gLogicalPalette.entries[WINGRAPH_SYSTEM_PALETTE_SIZE]
    );
    WinGSetDIBColorTable(
        gImageDC,
        WINGRAPH_SYSTEM_PALETTE_SIZE,
        PALETTE_COLOR_COUNT - WINGRAPH_SYSTEM_PALETTE_SIZE * 2,
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
        // The editor has no combat screen to redraw partially.
        {
#else
        if (gLimitedCombatUpdatePalette != 0) {
            if (gFullCombatScreenDrawn != 0)
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

// Initializes the system-reserved WinG colors and leaves the mutable
// interior flagged for palette animation.
VA(0x00467d9b, 0x1a2)
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

VA(0x004680f4, 0x67)
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
VA_AT(editor, 0x0041b120, 0x1bd)
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

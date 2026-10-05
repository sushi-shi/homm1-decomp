// Located from HoMM2 Buka 2.1; PoL 2.0 supplies the VC4 declaration.

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

// wingraph owns retail .data 0x0049fe60-0x0048eb17 (definitions below in
// retail order; initializers are retail bytes) and .bss 0x004a46a0-0x004a4b7f.
// Its DDSD line arguments are /Gi compiler line statics (docs/patterns/vc4-gi-line-var.md).
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
// Buka's image/scroll counters are identified by the retail WinG paint path.
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
// KB owns these scroll, combat-palette and configuration globals.
extern i32 gScrollX;
extern i32 gScrollY;
extern i32 gFullCombatScreenDrawn;
extern i32 gLimitedCombatUpdatePalette;
extern configStruct gConfig;

// PoL retains the source-line-base expression, matching HoMM1's word load.
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
    // Buka 2.1 retains this unused local; retail's four-byte frame confirms it.
    i32 unused;

    if (gWinGraphBusy)
        return TRUE;
    if (!gForegroundApp)
        return TRUE;
    return SetPalette();
}

// donor PoL RVA 0x0003532b; preferred Buka symbol ?CreatePrimary@@YIXXZ
// donor Buka TU SOURCE/wingraph; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.466160;margin=0.260025;shape=0.308;size=0.761;calls=1.000;alternate=pol20:void CreatePrimary(void)@0x0003532b
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

// donor PoL RVA 0x000353bf; preferred Buka symbol ?SetupClipper@@YIXXZ
// donor Buka TU SOURCE/wingraph; HoMM1 owner inferred from contiguous order
// evidence: graph:3;base=0.462387;margin=0.608484;shape=0.302;size=0.757;calls=1.000;alternate=pol20:void SetupClipper(void)@0x000353bf
VA(0x004667f8, 0xbe)
#line 91 "E:\\Users\\igorl\\VSS\\HMM\\HMM1\\Source\\Game\\wingraph.cpp"
void SetupClipper(void) {
    i32 result;

    if (gConfig.gfx[gCurExe].fullScreen == 0) {
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

// donor PoL RVA 0x000354a2; preferred Buka symbol ?DDInitGraphics@@YIXXZ
// donor Buka TU SOURCE/wingraph; HoMM1 owner inferred from contiguous order
// evidence: graph:4;base=0.478180;margin=0.785524;shape=0.323;size=0.780;calls=1.000;alternate=pol20:void DDInitGraphics(void)@0x000354a2
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
    if (gConfig.gfx[gCurExe].fullScreen != 0) {
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

// donor PoL RVA 0x00035601; preferred Buka symbol ?DDAppPaint@@YIHPAX0@Z
// donor Buka TU SOURCE/wingraph; HoMM1 owner inferred from contiguous order
// evidence: graph:1;base=0.713701;margin=0.241032;shape=0.479;size=0.871;calls=0.917;strings=ResetDisplayMode;alternate=pol20:int DDAppPaint(void *, void *)@0x00035601
VA(0x004669ef, 0x4e4)
#line 161 "E:\\Users\\igorl\\VSS\\HMM\\HMM1\\Source\\Game\\wingraph.cpp"
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
            gpWindowManager->m_screen->m_pixels = static_cast<i8*>(gDDSurfaceDesc.lpSurface);
            gInitWin = gDDSurfaceDesc.lpSurface;
        } else {
            gInitWin = gDDSurfaceDesc.lpSurface;
        }
        if (gDDResult != DD_OK)
#line 287
            DDSD(gDDResult, __FILE__, __LINE__);
        EndPaint(static_cast<HWND>(window), &ps);
        gWinGraphBusy = FALSE;
    }
    return TRUE;
}

// Both donors retain the DirectDraw palette setup and its three locals.
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

// Buka's palette attachment; PoL retains the error line-base source form.
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

// donor PoL RVA 0x00035d1c; preferred Buka symbol ?DDCreateSurface@@YIPAUIDirectDrawSurface@@KKH@Z
// donor Buka TU SOURCE/wingraph; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.607502;margin=0.616633;shape=0.536;size=0.889;calls=1.000;alternate=pol20:struct IDirectDrawSurface * DDCreateSurface(unsigned long int, unsigned long int, int)@0x00035d1c
VA(0x0046706a, 0x100)
#line 417 "E:\\Users\\igorl\\VSS\\HMM\\HMM1\\Source\\Game\\wingraph.cpp"
struct IDirectDrawSurface* DDCreateSurface(u32 width, u32 height, i32 primary) {
    _DDSURFACEDESC ddsd;
    IDirectDrawSurface* lpSurface;
    // Donor unused locals (Buka count/unused, PoL cnt/unused); retail keeps
    // two unreferenced slots between lpSurface and the result.
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
            gpWindowManager->m_screen->m_pixels = static_cast<i8*>(ddsd.lpSurface);
            gInitWin = ddsd.lpSurface;
        } else {
            gInitWin = ddsd.lpSurface;
        }
    }
    return lpSurface;
}

// donor PoL RVA 0x00035e4f; preferred Buka symbol ?DDSD@@YIXHPADH@Z
// donor Buka TU SOURCE/wingraph; HoMM1 owner inferred from contiguous order
// evidence: graph:3;base=0.296609;margin=0.202074;shape=0.224;size=0.647;calls=0.194;alternate=pol20:void DDSD(int, char *, int)@0x00035e4f
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

// donor PoL RVA 0x00036421; preferred Buka symbol ?DDUpdatePalette@@YAXPAC@Z
// donor Buka TU SOURCE/wingraph; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.482125;margin=0.532523;shape=0.296;size=0.838;calls=1.000;alternate=pol20:void DDUpdatePalette(signed char *)@0x00036421
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
    // API-forced: ProcessAssert accepts the donor pointer assertion as a 32-bit int.
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

// donor PoL RVA 0x00036539; preferred Buka symbol ?DDCleanUpWinGraphics@@YIXXZ
// donor Buka TU SOURCE/wingraph; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.511358;margin=0.374406;shape=0.341;size=0.862;calls=1.000;alternate=pol20:void DDCleanUpWinGraphics(void)@0x00036539
VA(0x0046764c, 0x154)
#line 550 "E:\\Users\\igorl\\VSS\\HMM\\HMM1\\Source\\Game\\wingraph.cpp"
void DDCleanUpWinGraphics(void) {
    // Both locals survive in Buka591-632 and PoL481-519; restoreVal is written.
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

// donor PoL RVA 0x000366b0; preferred Buka symbol ?DDSetFullScreenStatus@@YIXH@Z
// donor Buka TU SOURCE/wingraph; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.474854;margin=0.471351;shape=0.269;size=0.849;calls=1.000;alternate=pol20:void DDSetFullScreenStatus(int)@0x000366b0
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
    if (gConfig.gfx[gCurExe].fullScreen == fullScreen)
        return;
    {
        x = gConfig.gfx[gCurExe].x;
        y = gConfig.gfx[gCurExe].y;
        width = gConfig.gfx[gCurExe].width;
        windowHeight = gConfig.gfx[gCurExe].height;
        gWinGraphBusy = TRUE;
        gConfig.gfx[gCurExe].fullScreen = fullScreen;
        if (gConfig.gfx[gCurExe].fullScreen != 0)
            SetMenuStatus(0);

        hres = gDD->SetCooperativeLevel(
            hwndApp,
            DDSCL_EXCLUSIVE | DDSCL_FULLSCREEN | DDSCL_ALLOWREBOOT
        );
        if (hres != DD_OK)
#line 596
            DDSD(hres, __FILE__, __LINE__);
        if (gConfig.gfx[gCurExe].fullScreen != 0) {
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
        if (gConfig.gfx[gCurExe].fullScreen == 0) {
            SetMenuStatus(1);
            ResizeWindow(x, y, width, windowHeight);
        } else {
            gConfig.gfx[gCurExe].x = x;
            gConfig.gfx[gCurExe].y = y;
            gConfig.gfx[gCurExe].width = width;
            gConfig.gfx[gCurExe].height = windowHeight;
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

// Buka 2.1's WinG palette update, retaining its logical-palette and DIB
// records; retail uses 10..245 for mutable entries.
// Donor Buka 2.1 supplies the DIB setup; retail's one-word frame and API
// call graph confirm this WinG backend initializer.
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

// Buka 2.1 initializes the system-reserved WinG colors and leaves the
// mutable interior flagged for palette animation; retail uses the same bands.
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

// Buka 2.1 supplies the WinG paint sequence and local lifetimes. HoMM1's
// client-to-game transform uses its pinned 640x480 viewport.
VA(0x00467f3d, 0x1b7)
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

// The Buka loader supplies the DLL and factory sequence; HoMM1 retail's
// failed-factory arm invokes ShutDown with its own error string.
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

// Buka's DLL teardown checks the Win32 module handle before release.
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

// donor PoL RVA 0x0003728a; preferred Buka symbol ?GetGraphicsInfo@@YIXXZ
// donor Buka TU SOURCE/wingraph; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.553360;margin=0.492664;shape=0.455;size=0.829;calls=1.000;alternate=pol20:void GetGraphicsInfo(void)@0x0003728a
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

// Buka's graphics startup sequence; HoMM1 has no intervening debug logs.
VA(0x00468265, 0x46)
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

// Buka's graphics dispatcher returns the selected backend's paint result.
VA(0x004682ab, 0x30)
BOOL AppPaint(void* window, void* paintDC) {
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

// Retail and Buka dispatch the same palette buffer to the selected backend.
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

// donor PoL RVA 0x00037483; preferred Buka symbol ?SetFullScreenStatus@@YIXH@Z
// donor Buka TU SOURCE/wingraph; HoMM1 owner inferred from contiguous order
// evidence: graph:3;base=0.430220;margin=0.650390;shape=0.175;size=0.870;calls=0.800;alternate=pol20:void SetFullScreenStatus(int)@0x00037483
VA(0x0046833c, 0x84)
void SetFullScreenStatus(i32 fullScreen) {
    if (gInSmacker != 0)
        return;
    if (fullScreen == gConfig.gfx[gCurExe].fullScreen)
        return;
    if (gGraphicsType == WINGRAPH_GRAPHICS_WING) {
        // HoMM1 has no DirectDraw-attached guard or cursor refresh here.
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

VA(0x004683c0, 0x1a)
BOOL QueryNewPalette() {
    if (gGraphicsType == WINGRAPH_GRAPHICS_WING)
        return WGQueryNewPalette();
    else
        return DDQueryNewPalette();
}

// donor PoL RVA 0x00037595; preferred Buka symbol ?SetGraphicsType@@YIHH@Z
// donor Buka TU SOURCE/wingraph; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.565182;margin=0.258783;shape=0.434;size=0.909;calls=1.000;alternate=pol20:int SetGraphicsType(int)@0x00037595
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

    fullState = gConfig.gfx[gCurExe].fullScreen;
    x = gConfig.gfx[gCurExe].x;
    y = gConfig.gfx[gCurExe].y;
    width = gConfig.gfx[gCurExe].width;
    hgt = gConfig.gfx[gCurExe].height;
    buffer = malloc(WINGRAPH_WIDTH * WINGRAPH_HEIGHT);
    memcpy(buffer, gpWindowManager->m_screen->m_pixels, WINGRAPH_WIDTH * WINGRAPH_HEIGHT);
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

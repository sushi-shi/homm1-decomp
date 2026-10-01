// Located from HoMM2 Buka 2.1; PoL 2.0 supplies the VC4 declaration.

#include <match.h>

#include <SOURCE/wingraph.h>

#include <BASE/Misc.h>
#include <BASE/MOUSEMGR_TYPES.h>
#include <H1/All.h>
#include <H1/KB.h>
#include <SOURCE/kbwin.h>

#include <stdlib.h>
#include <string.h>

// Code-required retail identities; initializer/data-byte matching is deferred.
DATA(0x0048e180)
int giGraphicsType;
DATA(0x0048e178)
int gbWinGAttached;
DATA(0x0048e17c)
int gbDDrawAttached;
DATA(0x0048e5a8)
int gbWinGraphBusy;
DATA(0x0048e5ac)
DirectDrawCreateProc lpDirectDrawCreate;
DATA(0x0048e5b0)
IDirectDraw* lpDD;
DATA(0x0048e5b4)
IDirectDrawSurface* lpDDSPrimary;
DATA(0x0048e5b8)
IDirectDrawSurface* lpDDSOne;
DATA(0x0048e5bc)
IDirectDrawClipper* lpClipper;
DATA(0x0048e5c0)
IDirectDrawPalette* lpDDPal;
DATA(0x0048e5c4)
short gDDRestoreLineBase;
DATA(0x0048e824)
short gDDSetPaletteLineBase;
DATA(0x0048e800)
short gDDInitializePaletteLineBase;
DATA(0x0048e8c0)
short gDDUpdatePaletteLineBase;
DATA(0x0048e904)
short gDDCleanUpLineBase;
DATA(0x0048e5e8)
short gCreatePrimaryLineBase;
DATA(0x0048e60c)
short gSetupClipperLineBase;
DATA(0x0048e670)
short gDDInitLineBase;
DATA(0x0048e848)
short gDDCreateSurfaceLineBase;
DATA(0x0048e84c)
int bInDDSD;
DATA(0x0048e948)
short gDDSetFullScreenLineBase;
DATA(0x0048e6f8)
short gDDPaintLineBase;
DATA(0x004a4b70)
RECT gDDClientRect;
DATA(0x004a46a8)
RECT gDDSourceRect;
DATA(0x004a46b8)
RECT gDDDestinationRect;
DATA(0x004a46a0)
long gDDResult;
DATA(0x004a46d0)
_DDSURFACEDESC gDDSurfaceDesc;
DATA(0x004a46c8)
long lPaintStart;
DATA(0x0048e674)
int iBusyRetry;
DATA(0x0048e94c)
HDC hdcImage;
DATA(0x0048e950)
HBITMAP gbmOldMonoBitmap;
DATA(0x0048e954)
HPALETTE hpalApp;
DATA(0x0048e9fc)
HINSTANCE hDDrawLibrary;
// Buka's image/scroll counters are identified by the retail WinG paint path.
DATA(0x0048e5a4)
int giTtlBlts;
DATA(0x0048e184)
int giMainVideoModeColorDepth;
DATA(0x0048e198)
WingPalette LogicalPalette;
DATA(0x0048e190)
int Orientation;
DATA(0x0048e59c)
void* lpInitWin;
DATA(0x00492e08)
int giScrollX;
DATA(0x00492e0c)
int giScrollY;
DATA(0x00494130)
int gbFullCombatScreenDrawn;
DATA(0x00494134)
int gbLimitedCombatUpdatePalette;
DATA(0x004a4740)
WingImage screenImage;
DATA(0x004c6aa8)
configStruct gConfig;

// donor PoL RVA 0x000a26a0; preferred Buka symbol ?SeedPosition@searchArray@@QAEXHHHHHHHHHHHH@Z
// donor Buka TU SOURCE/SEARCH; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.418459;margin=0.401354;shape=0.193;size=0.813;calls=0.875;alternate=pol20:void searchArray::SeedPosition(int, int, int, int, int, int, int, int, int, int, int, int)@0x000a26a0
VA(0x00402be0, 0xa60)
void searchArray::SeedPosition(short, short, short, int, int, int, int, int, int, int, int, int) {}

// PoL retains the source-line-base expression, matching HoMM1's word load.
VA(0x00403640, 0x59)
void DDRestoreDisplayMode() {
    long result;
    if (lpDD != 0) {
        result = lpDD->RestoreDisplayMode();
        if (result != 0)
            DDSD(result, "D:\\Heroes\\Source\\wingraph.cpp", gDDRestoreLineBase + 7);
    }
}

VA(0x00403699, 0x46)
int DDQueryNewPalette() {
    // Buka 2.1 retains this unused local; retail's four-byte frame confirms it.
    int unused;

    if (gbWinGraphBusy)
        return 1;
    if (!gbForegroundApp)
        return 1;
    return SetPalette();
}

// donor PoL RVA 0x0003532b; preferred Buka symbol ?CreatePrimary@@YIXXZ
// donor Buka TU SOURCE/wingraph; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.466160;margin=0.260025;shape=0.308;size=0.761;calls=1.000;alternate=pol20:void CreatePrimary(void)@0x0003532b
VA(0x004036df, 0x9b)
void CreatePrimary(void) {
    long result;

    lpDDSPrimary = DDCreateSurface(WINGRAPH_WIDTH, WINGRAPH_HEIGHT, 1);
    if (lpClipper != 0) {
        result = lpDDSPrimary->SetClipper(0);
        if (result != 0 && result != DDERR_NOCLIPPERATTACHED)
            DDSD(result, "D:\\Heroes\\Source\\wingraph.cpp", gCreatePrimaryLineBase + 10);
        lpClipper->Release();
        lpClipper = 0;
    }
}

// donor PoL RVA 0x000353bf; preferred Buka symbol ?SetupClipper@@YIXXZ
// donor Buka TU SOURCE/wingraph; HoMM1 owner inferred from contiguous order
// evidence: graph:3;base=0.462387;margin=0.608484;shape=0.302;size=0.757;calls=1.000;alternate=pol20:void SetupClipper(void)@0x000353bf
VA(0x0040377a, 0xeb)
void SetupClipper(void) {
    long result;

    if (gConfig.gfx[giCurExe].fullScreen == 0) {
        result = lpDD->CreateClipper(0, &lpClipper, 0);
        if (result != 0)
            DDSD(result, "D:\\Heroes\\Source\\wingraph.cpp", gSetupClipperLineBase + 8);
        result = lpClipper->SetHWnd(0, hwndApp);
        if (result != 0)
            DDSD(result, "D:\\Heroes\\Source\\wingraph.cpp", gSetupClipperLineBase + 13);
        result = lpDDSPrimary->SetClipper(lpClipper);
        if (result != 0)
            DDSD(result, "D:\\Heroes\\Source\\wingraph.cpp", gSetupClipperLineBase + 18);
    }
}

// donor PoL RVA 0x000354a2; preferred Buka symbol ?DDInitGraphics@@YIXXZ
// donor Buka TU SOURCE/wingraph; HoMM1 owner inferred from contiguous order
// evidence: graph:4;base=0.478180;margin=0.785524;shape=0.323;size=0.780;calls=1.000;alternate=pol20:void DDInitGraphics(void)@0x000354a2
VA(0x00403865, 0x171)
void DDInitGraphics(void) {
    long result;

    if (gbWinGraphBusy != 0)
        return;
    result = lpDirectDrawCreate(0, &lpDD, 0);
    if (result != 0)
        DDSD(result, "D:\\Heroes\\Source\\wingraph.cpp", gDDInitLineBase + 8);
    if (gConfig.gfx[giCurExe].fullScreen != 0) {
        SetMenuStatus(0);
        result = lpDD->SetCooperativeLevel(
            hwndApp,
            DDSCL_EXCLUSIVE | DDSCL_FULLSCREEN | DDSCL_ALLOWREBOOT
        );
        if (result != 0)
            DDSD(result, "D:\\Heroes\\Source\\wingraph.cpp", gDDInitLineBase + 20);
        result = lpDD->SetDisplayMode(WINGRAPH_WIDTH, WINGRAPH_HEIGHT, WINGRAPH_COLOR_DEPTH);
        if (result != 0)
            DDSD(result, "D:\\Heroes\\Source\\wingraph.cpp", gDDInitLineBase + 24);
    } else {
        result = lpDD->SetCooperativeLevel(hwndApp, DDSCL_NORMAL);
        if (result != 0)
            DDSD(result, "D:\\Heroes\\Source\\wingraph.cpp", gDDInitLineBase + 31);
    }
    CreatePrimary();
    SetupClipper();
    lpDDSOne = DDCreateSurface(WINGRAPH_WIDTH, WINGRAPH_HEIGHT, 0);
    InitializePalette();
}

// donor PoL RVA 0x00035601; preferred Buka symbol ?DDAppPaint@@YIHPAX0@Z
// donor Buka TU SOURCE/wingraph; HoMM1 owner inferred from contiguous order
// evidence: graph:1;base=0.713701;margin=0.241032;shape=0.479;size=0.871;calls=0.917;strings=ResetDisplayMode;alternate=pol20:int DDAppPaint(void *, void *)@0x00035601
VA(0x004039d6, 0x592)
int DDAppPaint(void* window, void* paintDC) {
    int width;
    int ySrc;
    int height;
    int x;
    POINT pt;
    PAINTSTRUCT ps;

    if (gbWinGraphBusy != 0)
        return 1;
    if (gbMinimized != 0)
        return 1;
    if (lpDD == 0)
        return 1;
    {
        gbWinGraphBusy = 1;
        paintDC = BeginPaint(static_cast<HWND>(window), &ps);
        GetClientRect(static_cast<HWND>(window), &gDDClientRect);
        if (ps.rcPaint.right == 0 || ps.rcPaint.bottom == 0)
            ps.rcPaint = gDDClientRect;
        if (ps.rcPaint.right < WINGRAPH_PAINT_X_END)
            ps.rcPaint.right++;
        if (ps.rcPaint.bottom < WINGRAPH_PAINT_Y_END)
            ps.rcPaint.bottom++;

        gDDDestinationRect = ps.rcPaint;
        width = ((gDDDestinationRect.right - gDDDestinationRect.left + 1) * WINGRAPH_WIDTH)
                / iMainWinScreenWidth;
        height = ((gDDDestinationRect.bottom - gDDDestinationRect.top + 1) * WINGRAPH_HEIGHT)
                 / iMainWinScreenHeight;
        x = (gDDDestinationRect.left * WINGRAPH_WIDTH) / iMainWinScreenWidth;
        ySrc = (gDDDestinationRect.top * WINGRAPH_HEIGHT) / iMainWinScreenHeight;
        if (giScrollX != 0) {
            x = giScrollX + WINGRAPH_SCROLL_MARGIN;
            width = WINGRAPH_SCROLL_SIZE;
        }
        if (giScrollY != 0) {
            ySrc = giScrollY + WINGRAPH_SCROLL_MARGIN;
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
        gDDResult = lpDDSOne->Unlock(0);
        if (gDDResult != DD_OK)
            DDSD(gDDResult, "D:\\Heroes\\Source\\wingraph.cpp", gDDPaintLineBase + 72);

        if (gDDSourceRect.left < 0)
            gDDSourceRect.left = 0;
        if (gDDSourceRect.top < 0)
            gDDSourceRect.top = 0;
        if (gDDSourceRect.right > WINGRAPH_PAINT_X_END)
            gDDSourceRect.right = WINGRAPH_WIDTH;
        if (gDDSourceRect.bottom > WINGRAPH_PAINT_Y_END)
            gDDSourceRect.bottom = WINGRAPH_HEIGHT;

        lPaintStart = KBTickCount();
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
                lpDDSPrimary->Blt(&gDDDestinationRect, lpDDSOne, &gDDSourceRect, DDBLT_WAIT, 0);
            if (gDDResult == DDERR_SURFACELOST) {
                gDDResult = lpDDSPrimary->Restore();
                if (gDDResult == DDERR_WRONGMODE) {
                    LogStr("ResetDisplayMode");
                    gDDResult =
                        lpDD->SetDisplayMode(WINGRAPH_WIDTH, WINGRAPH_HEIGHT, WINGRAPH_COLOR_DEPTH);
                    if (gDDResult != DD_OK)
                        DDSD(gDDResult, "D:\\Heroes\\Source\\wingraph.cpp", gDDPaintLineBase + 106);
                    gDDResult = lpDDSPrimary->Restore();
                    if (gDDResult != DD_OK)
                        DDSD(gDDResult, "D:\\Heroes\\Source\\wingraph.cpp", gDDPaintLineBase + 110);
                    gDDDestinationRect = gDDSourceRect;
                }
                if (gDDResult != DD_OK)
                    DDSD(gDDResult, "D:\\Heroes\\Source\\wingraph.cpp", gDDPaintLineBase + 118);
            } else if (gDDResult == DDERR_SURFACEBUSY
                       && KBTickCount() < lPaintStart + WINGRAPH_PAINT_TIMEOUT) {
                iBusyRetry++;
            } else if (gDDResult != DD_OK) {
                DDSD(gDDResult, "D:\\Heroes\\Source\\wingraph.cpp", gDDPaintLineBase + 123);
            } else {
                break;
            }
        }

        memset(&gDDSurfaceDesc, 0, sizeof(gDDSurfaceDesc));
        gDDSurfaceDesc.dwSize = sizeof(gDDSurfaceDesc);
        gDDResult = lpDDSOne->Lock(0, &gDDSurfaceDesc, DDLOCK_WAIT, 0);
        if (gDDResult != DD_OK)
            DDSD(gDDResult, "D:\\Heroes\\Source\\wingraph.cpp", gDDPaintLineBase + 133);
        if (gpWindowManager->m_screen != 0) {
            gpWindowManager->m_screen->m_pixels =
                static_cast<signed char*>(gDDSurfaceDesc.lpSurface);
            lpInitWin = gDDSurfaceDesc.lpSurface;
        } else {
            lpInitWin = gDDSurfaceDesc.lpSurface;
        }
        if (gDDResult != DD_OK)
            DDSD(gDDResult, "D:\\Heroes\\Source\\wingraph.cpp", gDDPaintLineBase + 144);
        EndPaint(static_cast<HWND>(window), &ps);
        gbWinGraphBusy = 0;
    }
    return 1;
}

// Both donors retain the DirectDraw palette setup and its three locals.
VA(0x00403f68, 0x140)
void DDInitializePalette() {
    long ddrval;
    HDC hdc;
    int i;
    if (gbWinGraphBusy != 0)
        return;
    {
        hdc = GetDC(0);
        GetSystemPaletteEntries(hdc, 0, WINGRAPH_SYSTEM_PALETTE_SIZE, LogicalPalette.entries);
        GetSystemPaletteEntries(
            hdc,
            WINGRAPH_MUTABLE_PALETTE_END,
            WINGRAPH_SYSTEM_PALETTE_SIZE,
            &LogicalPalette.entries[WINGRAPH_MUTABLE_PALETTE_END]
        );
        ReleaseDC(0, hdc);
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
        ddrval = lpDD->CreatePalette(DDPCAPS_8BIT, LogicalPalette.entries, &lpDDPal, 0);
        if (ddrval != 0)
            DDSD(ddrval, "D:\\Heroes\\Source\\wingraph.cpp", gDDInitializePaletteLineBase + 63);
        SetPalette();
    }
}

// Buka's palette attachment; PoL retains the error line-base source form.
VA(0x004040a8, 0xb3)
int DDSetPalette() {
    long result;
    if (gbWinGraphBusy != 0)
        return 1;
    if (gbForegroundApp == 0)
        return 1;
    if (lpDDPal == 0 || lpDDSPrimary == 0 || lpDD == 0)
        return 1;
    result = lpDDSPrimary->SetPalette(lpDDPal);
    if (result != 0)
        DDSD(result, "D:\\Heroes\\Source\\wingraph.cpp", gDDSetPaletteLineBase + 20);
    return 0;
}

// donor PoL RVA 0x00035d1c; preferred Buka symbol ?DDCreateSurface@@YIPAUIDirectDrawSurface@@KKH@Z
// donor Buka TU SOURCE/wingraph; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.607502;margin=0.616633;shape=0.536;size=0.889;calls=1.000;alternate=pol20:struct IDirectDrawSurface * DDCreateSurface(unsigned long int, unsigned long int, int)@0x00035d1c
VA(0x0040415b, 0x12a)
struct IDirectDrawSurface* DDCreateSurface(unsigned long width, unsigned long height, int primary) {
    _DDSURFACEDESC ddsd;
    IDirectDrawSurface* lpSurface;
    // Donor unused locals (Buka count/unused, PoL cnt/unused); retail keeps
    // two unreferenced slots between lpSurface and the result.
    int i;
    int tmp;
    long ddrval;

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
    ddrval = lpDD->CreateSurface(&ddsd, &lpSurface, 0);
    if (ddrval != 0)
        DDSD(ddrval, "D:\\Heroes\\Source\\wingraph.cpp", gDDCreateSurfaceLineBase + 28);
    if (primary == 0) {
        ddrval = lpSurface->Lock(0, &ddsd, DDLOCK_WAIT, 0);
        if (ddrval != 0)
            DDSD(ddrval, "D:\\Heroes\\Source\\wingraph.cpp", gDDCreateSurfaceLineBase + 36);
        if (gpWindowManager->m_screen != 0) {
            gpWindowManager->m_screen->m_pixels = static_cast<signed char*>(ddsd.lpSurface);
            lpInitWin = ddsd.lpSurface;
        } else {
            lpInitWin = ddsd.lpSurface;
        }
    }
    return lpSurface;
}

// donor PoL RVA 0x00035e4f; preferred Buka symbol ?DDSD@@YIXHPADH@Z
// donor Buka TU SOURCE/wingraph; HoMM1 owner inferred from contiguous order
// evidence: graph:3;base=0.296609;margin=0.202074;shape=0.224;size=0.647;calls=0.194;alternate=pol20:void DDSD(int, char *, int)@0x00035e4f
VA(0x00404285, 0x3ee)
void DDSD(int error, char* file, int line) {
    long restoreResult;
    H1_ENUM_STORAGE(DirectDrawReportCode, int) unused;

    if (bInDDSD != 0)
        return;
    bInDDSD = 1;
    restoreResult = lpDD->RestoreDisplayMode();
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
    MessageBeep(0);
    MessageBeep(0);
    MessageBeep(0);
    sprintf(gText, "Direct Draw Error #%d in file '%s' at Line #%d", unused, file, line);
    LogStr(gText);
    ShutDown(gText);
}

// donor PoL RVA 0x00036421; preferred Buka symbol ?DDUpdatePalette@@YAXPAC@Z
// donor Buka TU SOURCE/wingraph; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.482125;margin=0.532523;shape=0.296;size=0.838;calls=1.000;alternate=pol20:void DDUpdatePalette(signed char *)@0x00036421
VA(0x00404673, 0x11c)
void DDUpdatePalette(signed char* paletteData) {
    int entry;
    long result0;

    if (gbWinGraphBusy != 0)
        return;
    if (gbForegroundApp == 0)
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
    ProcessAssert(
        reinterpret_cast<int>(lpDDPal),
        "D:\\Heroes\\Source\\wingraph.cpp",
        gDDUpdatePaletteLineBase + 18
    );
    result0 = lpDDPal->SetEntries(
        0,
        WINGRAPH_SYSTEM_PALETTE_SIZE,
        WINGRAPH_PALETTE_SIZE - WINGRAPH_SYSTEM_PALETTE_SIZE * 2,
        &LogicalPalette.entries[WINGRAPH_SYSTEM_PALETTE_SIZE]
    );
    if (result0 != 0)
        DDSD(result0, "D:\\Heroes\\Source\\wingraph.cpp", gDDUpdatePaletteLineBase + 22);
}

// donor PoL RVA 0x00036539; preferred Buka symbol ?DDCleanUpWinGraphics@@YIXXZ
// donor Buka TU SOURCE/wingraph; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.511358;margin=0.374406;shape=0.341;size=0.862;calls=1.000;alternate=pol20:void DDCleanUpWinGraphics(void)@0x00036539
VA(0x0040478f, 0x17f)
void DDCleanUpWinGraphics(void) {
    // Both locals survive in Buka591-632 and PoL481-519; restoreVal is written.
    long restoreVal;
    long result;

    if (lpDD != 0) {
        restoreVal = lpDD->RestoreDisplayMode();
        if (lpClipper != 0) {
            if (lpDDSPrimary != 0) {
                result = lpDDSPrimary->SetClipper(0);
                if (result != 0 && result != DDERR_NOCLIPPERATTACHED)
                    DDSD(result, "D:\\Heroes\\Source\\wingraph.cpp", gDDCleanUpLineBase + 14);
            }
            lpClipper->Release();
            lpClipper = 0;
        }
        if (lpDDSPrimary != 0) {
            lpDDSPrimary->Release();
            lpDDSPrimary = 0;
        }
        if (lpDDSOne != 0) {
            lpDDSOne->Release();
            lpDDSOne = 0;
        }
        if (lpDDPal != 0) {
            lpDDPal->Release();
            lpDDPal = 0;
        }
        result = lpDD->SetCooperativeLevel(hwndApp, DDSCL_NORMAL);
        if (result != 0)
            DDSD(result, "D:\\Heroes\\Source\\wingraph.cpp", gDDCleanUpLineBase + 38);
        lpDD->Release();
        lpDD = 0;
    }
}

// donor PoL RVA 0x000366b0; preferred Buka symbol ?DDSetFullScreenStatus@@YIXH@Z
// donor Buka TU SOURCE/wingraph; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.474854;margin=0.471351;shape=0.269;size=0.849;calls=1.000;alternate=pol20:void DDSetFullScreenStatus(int)@0x000366b0
VA(0x0040490e, 0x2ea)
void DDSetFullScreenStatus(int fullScreen) {
    int w;
    int x;
    int h;
    int y;
    long ddrval;

    if (gbWinGraphBusy != 0)
        return;
    if (gConfig.gfx[giCurExe].fullScreen == fullScreen)
        return;
    {
        x = gConfig.gfx[giCurExe].x;
        y = gConfig.gfx[giCurExe].y;
        w = gConfig.gfx[giCurExe].width;
        h = gConfig.gfx[giCurExe].height;
        gbWinGraphBusy = 1;
        gConfig.gfx[giCurExe].fullScreen = fullScreen;
        if (gConfig.gfx[giCurExe].fullScreen != 0)
            SetMenuStatus(0);

        ddrval = lpDD->SetCooperativeLevel(
            hwndApp,
            DDSCL_EXCLUSIVE | DDSCL_FULLSCREEN | DDSCL_ALLOWREBOOT
        );
        if (ddrval != DD_OK)
            DDSD(ddrval, "D:\\Heroes\\Source\\wingraph.cpp", gDDSetFullScreenLineBase + 21);
        if (gConfig.gfx[giCurExe].fullScreen != 0) {
            ddrval = lpDD->SetDisplayMode(WINGRAPH_WIDTH, WINGRAPH_HEIGHT, WINGRAPH_COLOR_DEPTH);
            if (ddrval != DD_OK)
                DDSD(ddrval, "D:\\Heroes\\Source\\wingraph.cpp", gDDSetFullScreenLineBase + 27);
        } else {
            ddrval = lpDD->RestoreDisplayMode();
            if (ddrval != DD_OK)
                DDSD(ddrval, "D:\\Heroes\\Source\\wingraph.cpp", gDDSetFullScreenLineBase + 34);
            ddrval = lpDD->SetCooperativeLevel(hwndApp, DDSCL_NORMAL);
            if (ddrval != DD_OK)
                DDSD(ddrval, "D:\\Heroes\\Source\\wingraph.cpp", gDDSetFullScreenLineBase + 39);
        }
        if (lpDDSPrimary != 0) {
            lpDDSPrimary->Release();
            lpDDSPrimary = 0;
        }
        CreatePrimary();
        ddrval = lpDDSPrimary->SetPalette(lpDDPal);
        if (ddrval != DD_OK)
            DDSD(ddrval, "D:\\Heroes\\Source\\wingraph.cpp", gDDSetFullScreenLineBase + 51);
        WritePrefs();
        gbWinGraphBusy = 0;
        if (gConfig.gfx[giCurExe].fullScreen == 0) {
            SetMenuStatus(1);
            ResizeWindow(x, y, w, h);
        } else {
            gConfig.gfx[giCurExe].x = x;
            gConfig.gfx[giCurExe].y = y;
            gConfig.gfx[giCurExe].width = w;
            gConfig.gfx[giCurExe].height = h;
        }
        SetupClipper();
    }
}

// The WinG palette path uses the application window and palette handles.
VA(0x00404bf8, 0x89)
int WGQueryNewPalette() {
    int paletteChanges;
    {
        HDC hdc;

        hdc = GetDC(hwndApp);
        if (hpalApp != 0)
            SelectPalette(hdc, hpalApp, 0);
        paletteChanges = RealizePalette(hdc);
        ReleaseDC(hwndApp, hdc);
    }
    if (paletteChanges > 0) {
        InvalidateRect(hwndApp, 0, 1);
        return 1;
    } else {
        return 0;
    }
}

// Buka 2.1's WinG palette update, retaining its logical-palette and DIB
// records; retail uses 10..245 for mutable entries.
// Donor Buka 2.1 supplies the DIB setup; retail's one-word frame and API
// call graph confirm this WinG backend initializer.
VA(0x00404c81, 0x151)
void WGInitGraphics() {
    HBITMAP bitmap;

    if (hdcImage != 0)
        return;
    if (WinGRecommendDIBFormat(static_cast<BITMAPINFO*>(static_cast<void*>(&screenImage)))) {
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
    bitmap = WinGCreateBitmap(
        hdcImage,
        static_cast<BITMAPINFO*>(static_cast<void*>(&screenImage)),
        &screenImage.bits
    );
    screenImage.header.biSizeImage = screenImage.header.biHeight * screenImage.header.biWidth;
    screenImage.header.biSizeImage *= Orientation;
    gbmOldMonoBitmap = static_cast<HBITMAP>(SelectObject(hdcImage, bitmap));
    lpInitWin = screenImage.bits;
    PatBlt(hdcImage, 0, 0, iMainWinScreenWidth, iMainWinScreenHeight, BLACKNESS);
}

VA(0x00404dd2, 0x1dd)
void WGUpdatePalette(signed char* paletteData) {
    HDC dc;
    int result;
    int index;

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
    if (hpalApp != 0)
        DeleteObject(hpalApp);
    hpalApp = CreatePalette(static_cast<LOGPALETTE*>(static_cast<void*>(&LogicalPalette)));
    dc = GetDC(hwndApp);
    if (hpalApp != 0)
        SelectPalette(dc, hpalApp, 0);
    result = RealizePalette(dc);
    ReleaseDC(hwndApp, dc);
    if (giMainVideoModeColorDepth != WINGRAPH_COLOR_DEPTH) {
        if (gbLimitedCombatUpdatePalette != 0) {
            if (gbFullCombatScreenDrawn != 0)
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
VA(0x00404faf, 0x1d5)
void WGInitializePalette() {
    HDC hdc;
    int i;

    if (hpalApp != 0)
        return;
    hdc = GetDC(0);
    GetSystemPaletteEntries(hdc, 0, WINGRAPH_SYSTEM_PALETTE_SIZE, LogicalPalette.entries);
    GetSystemPaletteEntries(
        hdc,
        WINGRAPH_MUTABLE_PALETTE_END,
        WINGRAPH_SYSTEM_PALETTE_SIZE,
        &LogicalPalette.entries[WINGRAPH_MUTABLE_PALETTE_END]
    );
    ReleaseDC(0, hdc);
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
    hpalApp = CreatePalette(static_cast<LOGPALETTE*>(static_cast<void*>(&LogicalPalette)));
}

// Buka 2.1 supplies the WinG paint sequence and local lifetimes. HoMM1's
// client-to-game transform uses its pinned 640x480 viewport.
VA(0x00405184, 0x1c0)
int WGAppPaint(void* window, void* paintDC) {
    int nDestY;
    int destX;
    int iSrcY;
    int dstW;
    int destHeight;
    int srcX;
    PAINTSTRUCT paintStruct;
    int padding;
    RECT rect;
    char unused;

    unused = 0;
    if (screenImage.bits != 0) {
        paintDC = BeginPaint(static_cast<HWND>(window), &paintStruct);
        SelectPalette(static_cast<HDC>(paintDC), hpalApp, 0);
        RealizePalette(static_cast<HDC>(paintDC));
        GetClientRect(static_cast<HWND>(window), &rect);
        destX = 0;
        srcX = destX;
        nDestY = 0;
        iSrcY = nDestY;
        dstW = rect.right - rect.left;
        destHeight = rect.bottom - rect.top;
        srcX = destX * WINGRAPH_WIDTH / iMainWinScreenWidth;
        iSrcY = nDestY * WINGRAPH_HEIGHT / iMainWinScreenHeight;
        if (giScrollX != 0)
            srcX += giScrollX;
        if (giScrollY != 0)
            iSrcY += giScrollY;
        giTtlBlts++;
        if (iMainWinScreenWidth == WINGRAPH_WIDTH && iMainWinScreenHeight == WINGRAPH_HEIGHT) {
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
                destX + giScrollX,
                nDestY + giScrollY
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
                dstW * WINGRAPH_WIDTH / iMainWinScreenWidth,
                destHeight * WINGRAPH_HEIGHT / iMainWinScreenHeight
            );
        }
        EndPaint(static_cast<HWND>(window), &paintStruct);
    }
    return 1;
}

VA(0x00405344, 0x78)
void WGCleanUpWinGraphics() {
    HGDIOBJ bitmap;

    if (hdcImage != 0) {
        bitmap = SelectObject(hdcImage, gbmOldMonoBitmap);
        DeleteObject(bitmap);
        DeleteDC(hdcImage);
        hdcImage = 0;
    }
    if (hpalApp != 0) {
        DeleteObject(hpalApp);
        hpalApp = 0;
    }
}

// The Buka loader supplies the DLL and factory sequence; HoMM1 retail's
// failed-factory arm invokes ShutDown with its own error string.
VA(0x004053bc, 0x6c)
void ConnectToDLLs() {
    hDDrawLibrary = LoadLibraryA("DDRAW.DLL");
    if ((unsigned long)hDDrawLibrary >= 32) {
        // API-forced: GetProcAddress returns FARPROC for the typed DirectDraw factory.
        lpDirectDrawCreate = reinterpret_cast<DirectDrawCreateProc>(
            GetProcAddress(hDDrawLibrary, "DirectDrawCreate")
        );
        if (lpDirectDrawCreate != 0)
            gbDDrawAttached = 1;
        else
            ShutDown("Error loading DDRAW.DLL");
    }
}

// Buka's DLL teardown checks the Win32 module handle before release.
VA(0x00405428, 0x29)
void DisconnectDLLs() {
    if ((unsigned long)hDDrawLibrary >= 32)
        FreeLibrary(hDDrawLibrary);
}

// @dead-code
// Zero-ref: pinned retail has no incoming direct call/jump or relocated reference.
VA(0x00405451, 0x2c)
void RestoreDisplayMode() {
    if (giGraphicsType == 1)
        return;
    else
        DDRestoreDisplayMode();
}

VA(0x0040547d, 0x2e)
int SetPalette() {
    if (giGraphicsType == 1)
        return 0;
    else
        return DDSetPalette();
}

// donor PoL RVA 0x0003728a; preferred Buka symbol ?GetGraphicsInfo@@YIXXZ
// donor Buka TU SOURCE/wingraph; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.553360;margin=0.492664;shape=0.455;size=0.829;calls=1.000;alternate=pol20:void GetGraphicsInfo(void)@0x0003728a
VA(0x004054ab, 0x81)
void GetGraphicsInfo(void) {
    HDC screenDC;
    screenDC = GetDC(NULL);
    if (screenDC != NULL) {
        giMainVideoModeColorDepth = GetDeviceCaps(screenDC, BITSPIXEL);
        giMainVideoModeWidth = GetDeviceCaps(screenDC, HORZRES);
        giMainVideoModeHeight = GetDeviceCaps(screenDC, VERTRES);
        ReleaseDC(NULL, screenDC);
        if (giMainVideoModeColorDepth < 8)
            ShutDown("Heroes requires 256 color mode or higher.");
    }
}

// Buka's graphics startup sequence; HoMM1 has no intervening debug logs.
VA(0x0040552c, 0x60)
void InitGraphics() {
    ConnectToDLLs();
    if (gConfig.gfx[giCurExe].fullScreen != 0)
        giGraphicsType = 2;
    else
        giGraphicsType = 1;
    if (giGraphicsType == 1)
        WGInitGraphics();
    else
        DDInitGraphics();
}

// Buka's graphics dispatcher returns the selected backend's paint result.
VA(0x0040558c, 0x47)
int AppPaint(void* window, void* paintDC) {
    if (giGraphicsType == 1)
        return WGAppPaint(window, paintDC);
    else
        return DDAppPaint(window, paintDC);
}

VA(0x004055d3, 0x2c)
void InitializePalette() {
    if (giGraphicsType == 1)
        WGInitializePalette();
    else
        DDInitializePalette();
}

// Retail and Buka dispatch the same palette buffer to the selected backend.
VA(0x004055ff, 0x3a)
void UpdatePalette(signed char* paletteData) {
    if (giGraphicsType == 1)
        WGUpdatePalette(paletteData);
    else
        DDUpdatePalette(paletteData);
}

VA(0x00405639, 0x31)
void CleanUpWinGraphics() {
    if (giGraphicsType == 1)
        WGCleanUpWinGraphics();
    else
        DDCleanUpWinGraphics();
    DisconnectDLLs();
}

// donor PoL RVA 0x00037483; preferred Buka symbol ?SetFullScreenStatus@@YIXH@Z
// donor Buka TU SOURCE/wingraph; HoMM1 owner inferred from contiguous order
// evidence: graph:3;base=0.430220;margin=0.650390;shape=0.175;size=0.870;calls=0.800;alternate=pol20:void SetFullScreenStatus(int)@0x00037483
VA(0x0040566a, 0xb9)
void SetFullScreenStatus(int fullScreen) {
    if (gbInSmackMgr != 0)
        return;
    if (gConfig.gfx[giCurExe].fullScreen == fullScreen)
        return;
    if (giGraphicsType == 1) {
        // HoMM1 has no DirectDraw-attached guard or cursor refresh here.
        gConfig.gfx[giCurExe].fullScreen = 1;
        if (SetGraphicsType(2) != 0)
            DDSetFullScreenStatus(fullScreen);
        return;
    } else if (fullScreen == 0) {
        if (gbWinGAttached != 0)
            SetGraphicsType(1);
    } else {
        DDSetFullScreenStatus(fullScreen);
    }
}

VA(0x00405723, 0x31)
int QueryNewPalette() {
    if (giGraphicsType == 1)
        return WGQueryNewPalette();
    else
        return DDQueryNewPalette();
}

// donor PoL RVA 0x00037595; preferred Buka symbol ?SetGraphicsType@@YIHH@Z
// donor Buka TU SOURCE/wingraph; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.565182;margin=0.258783;shape=0.434;size=0.909;calls=1.000;alternate=pol20:int SetGraphicsType(int)@0x00037595
VA(0x00405754, 0x1f3)
int SetGraphicsType(int graphicsType) {
    void* screenBuffer;
    int w;
    int fullScreen;
    int x;
    int h;
    int y;

    if (graphicsType == giGraphicsType)
        return 1;
    if (graphicsType == 1 && gbWinGAttached == 0)
        return 0;
    if (graphicsType == 2 && gbDDrawAttached == 0)
        return 0;

    fullScreen = gConfig.gfx[giCurExe].fullScreen;
    x = gConfig.gfx[giCurExe].x;
    y = gConfig.gfx[giCurExe].y;
    w = gConfig.gfx[giCurExe].width;
    h = gConfig.gfx[giCurExe].height;
    screenBuffer = malloc(WINGRAPH_WIDTH * WINGRAPH_HEIGHT);
    memcpy(screenBuffer, gpWindowManager->m_screen->m_pixels, WINGRAPH_WIDTH * WINGRAPH_HEIGHT);
    if (graphicsType == 1) {
        gConfig.gfx[giCurExe].fullScreen = 0;
        DDCleanUpWinGraphics();
        giGraphicsType = 1;
        WGInitGraphics();
        gpWindowManager->m_screen->m_pixels = static_cast<signed char*>(lpInitWin);
    } else {
        WGCleanUpWinGraphics();
        giGraphicsType = 2;
        DDInitGraphics();
        gpWindowManager->m_screen->m_pixels = static_cast<signed char*>(lpInitWin);
    }
    memcpy(gpWindowManager->m_screen->m_pixels, screenBuffer, WINGRAPH_WIDTH * WINGRAPH_HEIGHT);
    free(screenBuffer);
    if (fullScreen != 0 && graphicsType == 1) {
        SetMenuStatus(1);
        ResizeWindow(x, y, w, h);
    }
    BlitBitmapToScreen(gpWindowManager->m_screen, 0, 0, WINGRAPH_WIDTH, WINGRAPH_HEIGHT, 0, 0);
    UpdatePalette(gpBufferPalette->m_data);
    return 1;
}

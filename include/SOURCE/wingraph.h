#ifndef HOMM1_SOURCE_WINGRAPH_H
#define HOMM1_SOURCE_WINGRAPH_H

#define WIN32_LEAN_AND_MEAN

#include <windows.h>

#include <Domains.h>
#include <SOURCE/DirectDraw.h>

struct IDirectDraw;
struct IUnknown;
typedef long(__stdcall* DirectDrawCreateProc)(GUID*, IDirectDraw**, IUnknown*);

H1_ENUM_CONST_BEGIN(WingraphPaintConstant)
    WINGRAPH_WIDTH = 640,
    WINGRAPH_HEIGHT = 480,
    WINGRAPH_PAINT_ALIGN_MASK = 0xfffc,
    WINGRAPH_PALETTE_SIZE = 256,
    WINGRAPH_SYSTEM_PALETTE_SIZE = 10,
    WINGRAPH_SYSTEM_PALETTE_END = WINGRAPH_SYSTEM_PALETTE_SIZE,
    WINGRAPH_MUTABLE_PALETTE_END = WINGRAPH_PALETTE_SIZE - WINGRAPH_SYSTEM_PALETTE_SIZE,
    WINGRAPH_LIMITED_COMBAT_HEIGHT = 458,
    WINGRAPH_COLOR_DEPTH = 8,
    WINGRAPH_PALETTE_COMPONENT_COUNT = 3,
    WINGRAPH_PALETTE_VALUE_SHIFT = 2,
    WINGRAPH_SCROLL_MARGIN = 16,
    WINGRAPH_SCROLL_SIZE = WINGRAPH_HEIGHT - WINGRAPH_SCROLL_MARGIN * 2,
    WINGRAPH_PAINT_TIMEOUT = 10000,
    WINGRAPH_PAINT_X_END = WINGRAPH_WIDTH,
    WINGRAPH_PAINT_Y_END = WINGRAPH_HEIGHT
H1_ENUM_CONST_END(WingraphPaintConstant)

// giGraphicsType: the WinG window backend or the DirectDraw full-screen one
// (InitGraphics picks DirectDraw for full screen; Buka WingraphGraphicsType,
// same numbering).
H1_ENUM_BEGIN(WingraphGraphicsType)
    WINGRAPH_GRAPHICS_WING = 1,
    WINGRAPH_GRAPHICS_DIRECT_DRAW = 2
H1_ENUM_END(WingraphGraphicsType)

H1_ENUM_BEGIN(DirectDrawReportCode)
    DDSD_REPORT_NONE = 0,
    DDSD_REPORT_GENERIC = 1,
    DDSD_REPORT_INVALIDCLIPLIST = 2,
    DDSD_REPORT_INVALIDOBJECT = 3,
    DDSD_REPORT_INVALIDPARAMS = 4,
    DDSD_REPORT_INVALIDRECT = 5,
    DDSD_REPORT_NOALPHAHW = 6,
    DDSD_REPORT_NOBLTHW = 7,
    DDSD_REPORT_NOCLIPLIST = 8,
    DDSD_REPORT_NODDROPSHW = 9,
    DDSD_REPORT_SURFACELOST = 10,
    DDSD_REPORT_UNSUPPORTED = 11,
    DDSD_REPORT_NOMIRRORHW = 12,
    DDSD_REPORT_NORASTEROPHW = 13,
    DDSD_REPORT_NOROTATIONHW = 14,
    DDSD_REPORT_NOSTRETCHHW = 15,
    DDSD_REPORT_SURFACEBUSY = 16,
    DDSD_REPORT_NOZBUFFERHW = 17,
    DDSD_REPORT_OUTOFMEMORY = 18,
    DDSD_REPORT_CLIPPERISUSINGHWND = 19,
    DDSD_REPORT_NOEXCLUSIVEMODE = 20,
    DDSD_REPORT_NOT8BITCOLOR = 21,
    DDSD_REPORT_NOPALETTEATTACHED = 22,
    DDSD_REPORT_NOPALETTEHW = 23,
    DDSD_REPORT_LOCKEDSURFACES = 24,
    DDSD_REPORT_IMPLICITLYCREATED = 25,
    DDSD_REPORT_WRONGMODE = 26,
    DDSD_REPORT_INCOMPATIBLEPRIMARY = 27,
    DDSD_REPORT_NOCLIPPERATTACHED = 28,
    DDSD_REPORT_UNKNOWN = 100
H1_ENUM_END(DirectDrawReportCode)

struct WingPalette {
    WORD version;
    WORD entryCount;
    PALETTEENTRY entries[WINGRAPH_PALETTE_SIZE];
};

// WinG's DIB record: BITMAPINFOHEADER, 256 palette entries, then image bits.
struct WingImage {
    BITMAPINFOHEADER header;
    RGBQUAD colors[256];
    void* bits;
};

extern "C" BOOL __stdcall WinGBitBlt(HDC, int, int, int, int, HDC, int, int);
extern "C" BOOL __stdcall WinGStretchBlt(HDC, int, int, int, int, HDC, int, int, int, int);
extern "C" UINT __stdcall WinGSetDIBColorTable(HDC, UINT, UINT, const RGBQUAD*);

extern "C" BOOL __stdcall WinGRecommendDIBFormat(BITMAPINFO*);
extern "C" HDC __stdcall WinGCreateDC();
extern "C" HBITMAP __stdcall WinGCreateBitmap(HDC, BITMAPINFO*, void**);

extern H1_ENUM_STORAGE(WingraphGraphicsType, int) giGraphicsType;
extern int giMainVideoModeHeight;
extern int giMainVideoModeWidth;
extern BOOL gbDDrawAttached;
extern BOOL gbWinGAttached;
// Smacker playback owner; SetFullScreenStatus ignores requests while it runs.
extern int gbInSmacker;
extern BOOL gbWinGraphBusy;
extern HPALETTE hpalApp;
extern HINSTANCE hDDrawLibrary;
extern DirectDrawCreateProc lpDirectDrawCreate;
extern IDirectDraw* lpDD;
extern IDirectDrawSurface* lpDDSPrimary;
extern IDirectDrawSurface* lpDDSOne;
extern IDirectDrawClipper* lpClipper;
extern IDirectDrawPalette* lpDDPal;
extern short gDDRestoreLineBase;
extern short gDDSetPaletteLineBase;
extern short gDDInitializePaletteLineBase;
extern short gDDUpdatePaletteLineBase;
extern short gDDCleanUpLineBase;
extern short gCreatePrimaryLineBase;
extern short gSetupClipperLineBase;
extern short gDDInitLineBase;
extern short gDDCreateSurfaceLineBase;
extern BOOL bInDDSD;
extern short gDDSetFullScreenLineBase;
extern short gDDPaintLineBase;
extern RECT gDDClientRect;
extern RECT gDDSourceRect;
extern RECT gDDDestinationRect;
extern long gDDResult;
extern _DDSURFACEDESC gDDSurfaceDesc;
extern long lPaintStart;
extern int iBusyRetry;
extern HDC hdcImage;
extern HBITMAP gbmOldMonoBitmap;
extern WingImage screenImage;
extern WingPalette LogicalPalette;
extern int Orientation;
extern void* lpInitWin;
extern int giScrollX;
extern int giScrollY;
extern int giTtlBlts;
extern int giMainVideoModeColorDepth;
extern int gbFullCombatScreenDrawn;
extern int gbLimitedCombatUpdatePalette;

void DDRestoreDisplayMode();
void SetFullScreenStatus(int);
void DDSD(int, char*, int);
BOOL DDSetPalette();
BOOL SetPalette();
void DDCleanUpWinGraphics();
void DDInitializePalette();
void WGInitializePalette();
void WGInitGraphics();
void WGCleanUpWinGraphics();
BOOL DDAppPaint(void*, void*);
BOOL WGAppPaint(void*, void*);
void DDUpdatePalette(signed char*);
void WGUpdatePalette(signed char*);
BOOL DDQueryNewPalette();
BOOL WGQueryNewPalette();
void DisconnectDLLs();
void ConnectToDLLs();
void InitGraphics();
void DDInitGraphics();
void CreatePrimary();
void SetupClipper();
IDirectDrawSurface* DDCreateSurface(unsigned long, unsigned long, int);
void RestoreDisplayMode();
void InitializePalette();
BOOL AppPaint(void*, void*);
void UpdatePalette(signed char*);
void CleanUpWinGraphics();
BOOL QueryNewPalette();
BOOL SetGraphicsType(H1_ENUM_PARAM(WingraphGraphicsType, int));

#endif

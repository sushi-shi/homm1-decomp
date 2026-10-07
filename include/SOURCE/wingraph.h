#ifndef HOMM1_SOURCE_WINGRAPH_H
#define HOMM1_SOURCE_WINGRAPH_H

#define WIN32_LEAN_AND_MEAN

#include <windows.h>

#include <BASE/display.h>
#include <Domains.h>

#include <ddraw.h>
#include <wing.h>

struct IDirectDraw;
struct IUnknown;
typedef long(__stdcall* DirectDrawCreateProc)(GUID*, IDirectDraw**, IUnknown*);

H1_ENUM_CONST_BEGIN(WingraphPaintConstant)
    WINGRAPH_PAINT_ALIGN_MASK = 0xfffc,
    WINGRAPH_SYSTEM_PALETTE_SIZE = 10,
    WINGRAPH_SYSTEM_PALETTE_END = WINGRAPH_SYSTEM_PALETTE_SIZE,
    WINGRAPH_MUTABLE_PALETTE_END = PALETTE_COLOR_COUNT - WINGRAPH_SYSTEM_PALETTE_SIZE,
    WINGRAPH_LIMITED_COMBAT_HEIGHT = 458,
    WINGRAPH_COLOR_DEPTH = 8,
    WINGRAPH_PALETTE_VALUE_SHIFT = 2,
    WINGRAPH_SCROLL_MARGIN = 16,
    WINGRAPH_SCROLL_SIZE = LOGICAL_SCREEN_HEIGHT - WINGRAPH_SCROLL_MARGIN * 2,
    WINGRAPH_PAINT_TIMEOUT = 10000
H1_ENUM_CONST_END(WingraphPaintConstant)

// gGraphicsType: the WinG window backend or the DirectDraw full-screen one
// (InitGraphics picks DirectDraw for full screen).
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
    PALETTEENTRY entries[PALETTE_COLOR_COUNT];
};

// WinG's DIB record: BITMAPINFOHEADER, 256 palette entries, then image bits.
struct WingImage {
    BITMAPINFOHEADER header;
    RGBQUAD colors[PALETTE_COLOR_COUNT];
    void* bits;
};

extern H1_ENUM_STORAGE(WingraphGraphicsType, i32) gGraphicsType;
extern i32 gMainVideoModeHeight;
extern i32 gMainVideoModeWidth;
extern BOOL gDDrawAttached;
extern BOOL gWinGAttached;
extern BOOL gWinGraphBusy;
extern HPALETTE gAppPalette;
extern HINSTANCE gDDrawLibrary;
extern DirectDrawCreateProc gDirectDrawCreate;
extern IDirectDraw* gDD;
extern IDirectDrawSurface* gDDSPrimary;
extern IDirectDrawSurface* gDDSOne;
extern IDirectDrawClipper* gClipper;
extern IDirectDrawPalette* gDDPal;
extern i16 gDDRestoreLineBase;
extern i16 gDDSetPaletteLineBase;
extern i16 gDDInitializePaletteLineBase;
extern i16 gDDUpdatePaletteLineBase;
extern i16 gDDCleanUpLineBase;
extern i16 gCreatePrimaryLineBase;
extern i16 gSetupClipperLineBase;
extern i16 gDDInitLineBase;
extern i16 gDDCreateSurfaceLineBase;
extern BOOL gInDDSD;
extern i16 gDDSetFullScreenLineBase;
extern i16 gDDPaintLineBase;
extern RECT gDDClientRect;
extern RECT gDDSourceRect;
#define gDDDestinationRect gDDDestRect // spelling fixes .bss order
extern RECT gDDDestinationRect;
#define gDDResult gDDrawStatus // spelling fixes .bss order
extern i32 gDDResult;
#define gDDSurfaceDesc gDDrawSurfaceDesc // spelling fixes .bss order
extern _DDSURFACEDESC gDDSurfaceDesc;
extern i32 gDDPaintStart;
extern i32 gBusyRetry;
extern HDC gImageDC;
extern HBITMAP gOldMonoBitmap;
#define gScreenImage screenImage // spelling fixes .bss order
extern WingImage gScreenImage;
extern WingPalette gLogicalPalette;
extern i32 gOrientation;
extern void* gInitWin;
extern i32 gTtlBlts;
extern i32 gMainVideoModeColorDepth;

void DDRestoreDisplayMode();
void SetFullScreenStatus(i32 fullScreen);
void DDSD(i32 error, char* file, i32 line);
BOOL DDSetPalette();
BOOL SetPalette();
void DDCleanUpWinGraphics();
void DDInitializePalette();
void WGInitializePalette();
void WGInitGraphics();
void WGCleanUpWinGraphics();
BOOL DDAppPaint(HWND window, HDC paintDC);
BOOL WGAppPaint(HWND window, HDC paintDC);
void DDUpdatePalette(i8* paletteData);
void WGUpdatePalette(i8* paletteData);
BOOL DDQueryNewPalette();
BOOL WGQueryNewPalette();
void DisconnectDLLs();
void ConnectToDLLs();
void InitGraphics();
void DDInitGraphics();
void CreatePrimary();
void SetupClipper();
IDirectDrawSurface* DDCreateSurface(u32 width, u32 height, b32 primary);
void RestoreDisplayMode();
void InitializePalette();
BOOL AppPaint(HWND window, HDC paintDC);
void UpdatePalette(i8* paletteData);
void CleanUpWinGraphics();
BOOL QueryNewPalette();
BOOL SetGraphicsType(H1_ENUM_PARAM(WingraphGraphicsType, i32) graphicsType);
void GetGraphicsInfo(void);

#endif

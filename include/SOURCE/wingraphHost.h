#ifndef HOMM1_SOURCE_WINGRAPHHOST_H
#define HOMM1_SOURCE_WINGRAPHHOST_H

// The Windows display host: WinG for windowed play, DirectDraw for full
// screen. Only the Windows units include this header; the game sees
// wingraph.h.

#define WIN32_LEAN_AND_MEAN

#include <windows.h>

#include <ddraw.h>
#include <wing.h>

#include <SOURCE/wingraph.h>

struct IDirectDraw;
struct IUnknown;
typedef long(__stdcall* DirectDrawCreateProc)(GUID*, IDirectDraw**, IUnknown*);

struct WingPalette {
    WORD version;
    WORD entryCount;
    PALETTEENTRY entries[PALETTE_COLOR_COUNT];
};

struct WingImage {
    BITMAPINFOHEADER header;
    RGBQUAD colors[PALETTE_COLOR_COUNT];
    void* bits;
};

extern i32 gGraphicsType;
extern i32 gMainVideoModeHeight;
extern i32 gMainVideoModeWidth;
extern b32 gDDrawAttached;
extern b32 gWinGAttached;
extern b32 gWinGraphBusy;
extern HPALETTE gAppPalette;
extern HINSTANCE gDDrawLibrary;
extern DirectDrawCreateProc gDirectDrawCreate;
extern IDirectDraw* gDD;
extern IDirectDrawSurface* gDDSPrimary;
extern IDirectDrawSurface* gDDSOne;
extern IDirectDrawClipper* gClipper;
extern IDirectDrawPalette* gDDPal;
extern b32 gInDDSD;
extern RECT gDDClientRect;
extern RECT gDDSourceRect;
extern RECT gDDDestinationRect;
extern i32 gDDResult;
extern _DDSURFACEDESC gDDSurfaceDesc;
extern i32 gDDPaintStart;
extern i32 gBusyRetry;
extern HDC gImageDC;
extern HBITMAP gOldMonoBitmap;
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
BOOL SetGraphicsType(i32 graphicsType);
void GetGraphicsInfo(void);

#endif

#ifndef HOMM1_SOURCE_DIRECTDRAW_H
#define HOMM1_SOURCE_DIRECTDRAW_H

#define WIN32_LEAN_AND_MEAN

#include <windows.h>

// Original DirectDraw palette capability bit.

// Original SDK DirectDraw error codes used by DDSD.
#define DD_OK 0
#define DDERR_GENERIC static_cast<long>(0x80004005L)
#define DDERR_INVALIDCLIPLIST static_cast<long>(0x8876006eL)
#define DDERR_INVALIDOBJECT static_cast<long>(0x88760082L)
#define DDERR_INVALIDPARAMS static_cast<long>(0x80070057L)
#define DDERR_INVALIDRECT static_cast<long>(0x88760096L)
#define DDERR_NOALPHAHW static_cast<long>(0x887600b4L)
#define DDERR_NOBLTHW static_cast<long>(0x8876023fL)
#define DDERR_NOCLIPLIST static_cast<long>(0x887600cdL)
#define DDERR_NODDROPSHW static_cast<long>(0x88760240L)
#define DDERR_SURFACELOST static_cast<long>(0x887601c2L)
#define DDERR_UNSUPPORTED static_cast<long>(0x80004001L)
#define DDERR_NOMIRRORHW static_cast<long>(0x887600faL)
#define DDERR_NORASTEROPHW static_cast<long>(0x88760118L)
#define DDERR_NOROTATIONHW static_cast<long>(0x88760122L)
#define DDERR_NOSTRETCHHW static_cast<long>(0x88760136L)
#define DDERR_SURFACEBUSY static_cast<long>(0x887601aeL)
#define DDERR_NOZBUFFERHW static_cast<long>(0x88760154L)
#define DDERR_OUTOFMEMORY static_cast<long>(0x8007000eL)
#define DDERR_CLIPPERISUSINGHWND static_cast<long>(0x88760237L)
#define DDERR_NOEXCLUSIVEMODE static_cast<long>(0x887600e1L)
#define DDERR_NOT8BITCOLOR static_cast<long>(0x88760140L)
#define DDERR_NOPALETTEATTACHED static_cast<long>(0x8876023cL)
#define DDERR_NOPALETTEHW static_cast<long>(0x8876023dL)
#define DDERR_LOCKEDSURFACES static_cast<long>(0x887600a0L)
#define DDERR_IMPLICITLYCREATED static_cast<long>(0x8876024cL)
#define DDERR_WRONGMODE static_cast<long>(0x8876024bL)
#define DDERR_INCOMPATIBLEPRIMARY static_cast<long>(0x8876005fL)

#define DDPCAPS_8BIT 0x00000004L
#define DDSCL_NORMAL 0x00000008L
#define DDSCL_FULLSCREEN 0x00000001L
#define DDSCL_ALLOWREBOOT 0x00000002L
#define DDSCL_EXCLUSIVE 0x00000010L
#define DDERR_NOCLIPPERATTACHED static_cast<long>(0x88760238L)

// Original IDirectDraw interface from the donor Microsoft DDRAW.H, methods
// in SDK order. These are external COM operations, never game-owned bodies.
struct IUnknown;
struct IDirectDrawClipper;
struct IDirectDrawPalette;
struct IDirectDrawSurface;
struct _DDSURFACEDESC;
struct _DDCAPS;
struct _DDSCAPS;
struct _DDCOLORKEY;
struct _DDPIXELFORMAT;
struct _DDBLTFX;
struct _DDBLTBATCH;
struct _DDOVERLAYFX;
typedef long (__stdcall *DDEnumModesCallback)(_DDSURFACEDESC *, void *);
typedef long (__stdcall *DDEnumSurfacesCallback)(IDirectDrawSurface *, _DDSURFACEDESC *, void *);

struct IDirectDraw {
    virtual long __stdcall QueryInterface(const GUID &, void **) = 0;
    virtual unsigned long __stdcall AddRef() = 0;
    virtual unsigned long __stdcall Release() = 0;
    virtual long __stdcall Compact() = 0;
    virtual long __stdcall CreateClipper(DWORD, IDirectDrawClipper **, IUnknown *) = 0;
    virtual long __stdcall CreatePalette(DWORD, PALETTEENTRY *, IDirectDrawPalette **, IUnknown *) = 0;
    virtual long __stdcall CreateSurface(_DDSURFACEDESC *, IDirectDrawSurface **, IUnknown *) = 0;
    virtual long __stdcall DuplicateSurface(IDirectDrawSurface *, IDirectDrawSurface **) = 0;
    virtual long __stdcall EnumDisplayModes(DWORD, _DDSURFACEDESC *, void *, DDEnumModesCallback) = 0;
    virtual long __stdcall EnumSurfaces(DWORD, _DDSURFACEDESC *, void *, DDEnumSurfacesCallback) = 0;
    virtual long __stdcall FlipToGDISurface() = 0;
    virtual long __stdcall GetCaps(_DDCAPS *, _DDCAPS *) = 0;
    virtual long __stdcall GetDisplayMode(_DDSURFACEDESC *) = 0;
    virtual long __stdcall GetFourCCCodes(DWORD *, DWORD *) = 0;
    virtual long __stdcall GetGDISurface(IDirectDrawSurface **) = 0;
    virtual long __stdcall GetMonitorFrequency(DWORD *) = 0;
    virtual long __stdcall GetScanLine(DWORD *) = 0;
    virtual long __stdcall GetVerticalBlankStatus(BOOL *) = 0;
    virtual long __stdcall Initialize(GUID *) = 0;
    virtual long __stdcall RestoreDisplayMode() = 0;
    virtual long __stdcall SetCooperativeLevel(HWND, DWORD) = 0;
    virtual long __stdcall SetDisplayMode(DWORD, DWORD, DWORD) = 0;
    virtual long __stdcall WaitForVerticalBlank(DWORD, HANDLE) = 0;
};

// Original SDK surface descriptor and its component records.
struct _DDCOLORKEY {
    DWORD dwColorSpaceLowValue;
    DWORD dwColorSpaceHighValue;
};
struct _DDSCAPS {
    DWORD dwCaps;
};
struct _DDPIXELFORMAT {
    DWORD dwSize;
    DWORD dwFlags;
    DWORD dwFourCC;
    union { DWORD dwRGBBitCount; DWORD dwYUVBitCount; DWORD dwZBufferBitDepth; DWORD dwAlphaBitDepth; };
    union { DWORD dwRBitMask; DWORD dwYBitMask; };
    union { DWORD dwGBitMask; DWORD dwUBitMask; };
    union { DWORD dwBBitMask; DWORD dwVBitMask; };
    union { DWORD dwRGBAlphaBitMask; DWORD dwYUVAlphaBitMask; DWORD dwRGBZBitMask; DWORD dwYUVZBitMask; };
};
struct _DDSURFACEDESC {
    DWORD dwSize;
    DWORD dwFlags;
    DWORD dwHeight;
    DWORD dwWidth;
    union { LONG lPitch; DWORD dwLinearSize; };
    DWORD dwBackBufferCount;
    union { DWORD dwMipMapCount; DWORD dwZBufferBitDepth; DWORD dwRefreshRate; };
    DWORD dwAlphaBitDepth;
    DWORD dwReserved;
    void *lpSurface;
    _DDCOLORKEY ddckCKDestOverlay;
    _DDCOLORKEY ddckCKDestBlt;
    _DDCOLORKEY ddckCKSrcOverlay;
    _DDCOLORKEY ddckCKSrcBlt;
    _DDPIXELFORMAT ddpfPixelFormat;
    _DDSCAPS ddsCaps;
};

#define DDSD_CAPS 0x00000001L
#define DDSD_HEIGHT 0x00000002L
#define DDSD_WIDTH 0x00000004L
#define DDSCAPS_OFFSCREENPLAIN 0x00000040L
#define DDSCAPS_PRIMARYSURFACE 0x00000200L
#define DDSCAPS_SYSTEMMEMORY 0x00000800L
#define DDLOCK_WAIT 0x00000001L
#define DDBLT_WAIT 0x01000000L

// Microsoft DDRAW.H clipper interface, donor SDK lines 704-718.
struct IDirectDrawClipper {
    virtual long __stdcall QueryInterface(const GUID &, void **) = 0;
    virtual unsigned long __stdcall AddRef() = 0;
    virtual unsigned long __stdcall Release() = 0;
    virtual long __stdcall GetClipList(RECT *, RGNDATA *, DWORD *) = 0;
    virtual long __stdcall GetHWnd(HWND *) = 0;
    virtual long __stdcall Initialize(IDirectDraw *, DWORD) = 0;
    virtual long __stdcall IsClipListChanged(BOOL *) = 0;
    virtual long __stdcall SetClipList(RGNDATA *, DWORD) = 0;
    virtual long __stdcall SetHWnd(DWORD, HWND) = 0;
};

// Microsoft DDRAW.H palette interface, donor SDK lines 665-677.
struct IDirectDrawPalette {
    virtual long __stdcall QueryInterface(const GUID &, void **) = 0;
    virtual unsigned long __stdcall AddRef() = 0;
    virtual unsigned long __stdcall Release() = 0;
    virtual long __stdcall GetCaps(DWORD *) = 0;
    virtual long __stdcall GetEntries(DWORD, DWORD, DWORD, PALETTEENTRY *) = 0;
    virtual long __stdcall Initialize(IDirectDraw *, DWORD, PALETTEENTRY *) = 0;
    virtual long __stdcall SetEntries(DWORD, DWORD, DWORD, PALETTEENTRY *) = 0;
};

// Original IDirectDrawSurface interface, donor SDK DDRAW.H lines 749–789.
struct IDirectDrawSurface {
    virtual long __stdcall QueryInterface(const GUID &, void **) = 0;
    virtual unsigned long __stdcall AddRef() = 0;
    virtual unsigned long __stdcall Release() = 0;
    virtual long __stdcall AddAttachedSurface(IDirectDrawSurface *) = 0;
    virtual long __stdcall AddOverlayDirtyRect(RECT *) = 0;
    virtual long __stdcall Blt(RECT *, IDirectDrawSurface *, RECT *, DWORD, _DDBLTFX *) = 0;
    virtual long __stdcall BltBatch(_DDBLTBATCH *, DWORD, DWORD) = 0;
    virtual long __stdcall BltFast(DWORD, DWORD, IDirectDrawSurface *, RECT *, DWORD) = 0;
    virtual long __stdcall DeleteAttachedSurface(DWORD, IDirectDrawSurface *) = 0;
    virtual long __stdcall EnumAttachedSurfaces(void *, DDEnumSurfacesCallback) = 0;
    virtual long __stdcall EnumOverlayZOrders(DWORD, void *, DDEnumSurfacesCallback) = 0;
    virtual long __stdcall Flip(IDirectDrawSurface *, DWORD) = 0;
    virtual long __stdcall GetAttachedSurface(_DDSCAPS *, IDirectDrawSurface **) = 0;
    virtual long __stdcall GetBltStatus(DWORD) = 0;
    virtual long __stdcall GetCaps(_DDSCAPS *) = 0;
    virtual long __stdcall GetClipper(IDirectDrawClipper **) = 0;
    virtual long __stdcall GetColorKey(DWORD, _DDCOLORKEY *) = 0;
    virtual long __stdcall GetDC(HDC *) = 0;
    virtual long __stdcall GetFlipStatus(DWORD) = 0;
    virtual long __stdcall GetOverlayPosition(LONG *, LONG *) = 0;
    virtual long __stdcall GetPalette(IDirectDrawPalette **) = 0;
    virtual long __stdcall GetPixelFormat(_DDPIXELFORMAT *) = 0;
    virtual long __stdcall GetSurfaceDesc(_DDSURFACEDESC *) = 0;
    virtual long __stdcall Initialize(IDirectDraw *, _DDSURFACEDESC *) = 0;
    virtual long __stdcall IsLost() = 0;
    virtual long __stdcall Lock(RECT *, _DDSURFACEDESC *, DWORD, HANDLE) = 0;
    virtual long __stdcall ReleaseDC(HDC) = 0;
    virtual long __stdcall Restore() = 0;
    virtual long __stdcall SetClipper(IDirectDrawClipper *) = 0;
    virtual long __stdcall SetColorKey(DWORD, _DDCOLORKEY *) = 0;
    virtual long __stdcall SetOverlayPosition(LONG, LONG) = 0;
    virtual long __stdcall SetPalette(IDirectDrawPalette *) = 0;
    virtual long __stdcall Unlock(void *) = 0;
    virtual long __stdcall UpdateOverlay(RECT *, IDirectDrawSurface *, RECT *, DWORD, _DDOVERLAYFX *) = 0;
    virtual long __stdcall UpdateOverlayDisplay(DWORD) = 0;
    virtual long __stdcall UpdateOverlayZOrder(DWORD, IDirectDrawSurface *) = 0;
};

#endif

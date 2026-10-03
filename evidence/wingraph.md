# Windows graphics evidence

Authority: the February 1996 image in `config/retail/targets.json`.
HoMM2 Buka 2.1 `SOURCE/wingraph.cpp` and `include/SOURCE/wingraph.h` supply
source correspondence; PoL 2.0 is secondary. Addresses below explicitly use
RVA or VA. Data rows establish code-required identities/layout only.
Current scores and compiler residues must be re-derived with
`homm1 match SOURCE/wingraph` and `homm1 walls diagnose ADDRESS`.

## Configuration and WinG

`InitGraphics` (RVA `0x552c`, size `0x60`) follows Buka VA `0x4b1911`:
connect DLLs, select graphics settings, initialize backend. HoMM1 omits its
debug logging. `ConnectToDLLs` (`0x53bc`, `0x6c`) calls LoadLibraryA and
GetProcAddress for DirectDrawCreate, or ShutDown with `Error loading DDRAW.DLL`.
It writes the attached flag at VA `0x48e17c` and factory pointer at `0x48e5ac`.
The factory's HRESULT is the VC4 SDK's underlying `long`.

ReadPrefsFromFile (RVA `0x5cba1`) passes VA `0x4c6aa8` and size `0x134` to
memset/fread, proving the gConfig owner. Graphics records contain six dwords:
showMenu, x, y, width, height, fullScreen. They start at owner+`0x18`, with
stride `0x18`, two elements and fullScreen at record+`0x14`. The selection at
VA `0x492e30` therefore indexes the fullScreen read at `0x4c6ad4`.
HoMM2's later colorMouseCursor member is absent. PollSound establishes
musicVolume at owner+4 and musicSource at +`0xb8`. Defaults (`0x5c9fe`) write
2/3 at offset zero; its name remains unknown. VA `0x4c6a94` is
glTimers[5] (the poll-sound deadline), not a config base.

| RVA | Identity | Retail evidence |
| --- | --- | --- |
| `0x8e180` | giGraphicsType | Compared with 1 by wrappers at `0x5451`, `0x547d`, `0x55d3`, `0x5639`, `0x5723`. |
| `0x8e5a8` | gbWinGraphBusy | Zero guard in DDQueryNewPalette at `0x36a2`. |
| `0x8e94c` | hdcImage | Selected/deleted/cleared by WGCleanUpWinGraphics at `0x5344`. |
| `0x8e950` | gbmOldMonoBitmap | Restored by SelectObject at `0x535a`. |
| `0x8e954` | hpalApp | Selected at `0x4bf8`, deleted/cleared at `0x5344`. |
| `0x8e9fc` | hDDrawLibrary | Unsigned comparison with 32 before FreeLibrary at `0x5428`. |

These are four-byte identities. WGInitGraphics (`0x4c81`, `0x151`) retains
the HBITMAP local, DC guard, recommended-format branch, 640 by negative-480
DIB, palette setup, WinG DC/bitmap creation, old bitmap selection and PatBlt.
Orientation at VA `0x48e190` and lpInitWin at `0x48e59c` are four bytes each.
WING32 import thunks are at RVAs `0x801ca`, `0x801d0`, `0x801d6`.
Retail and candidate multiply biWidth/biHeight in opposite load orders:
the identity gate checks the complete within-function address multiset,
including addends. Wrong, duplicated and cross-function members still fail.

WGAppPaint (`0x5184`) tests screenImage.bits at VA `0x4a4b68`. WingImage is
BITMAPINFOHEADER (40 bytes), 256 RGBQUADs, then a four-byte bits pointer at
+`0x428`, establishing owner VA `0x4a4740` and size `0x42c`. Scroll values
at `0x492e08`/`0x492e0c` affect source coordinates; `0x48e5a4` counts blits.
Calls at RVAs `0x52d1`/`0x5322` reach WinGBitBlt/WinGStretchBlt import thunks;
see [DNA evidence](homm1-dna-bands.tsv).

The palette body (`0x4dd2`) uses LogicalPalette at VA `0x48e198`: two-word
header plus 256 PALETTEENTRYs, size `0x404`. It updates matching RGBQUAD
channels in screenImage.colors at `0x4a4768`, checks video depth at `0x48e184`
and repaint flags at `0x494134`/`0x494130`. HoMM1 lacks Buka's extra screen
pointer guard. WGInitializePalette (`0x4faf`, `0x1d5`; Buka VA `0x4b1434`)
gets system colors [0,10) and [246,256), releases the DC and creates the
palette. Chained channel assignment expresses retail's store/reload/store
sequence. Donor locals dc0/entry0 are real; stack allocation depends on TU state.

## DirectDraw interfaces and diagnostics

The external interfaces come from Microsoft's DDRAW.H in Buka's toolchain:
IDirectDraw lines 491–518 (23 operations), IDirectDrawSurface 749–789 (36),
IDirectDrawPalette 665–677 (7), IDirectDrawClipper 704–718 (9). Preserve SDK
order. RestoreDisplayMode is slot 19, CreatePalette slot 5, surface SetPalette
slot 31 and palette SetEntries slot 6. Pointer-only SDK types remain forward
declarations; these interfaces do not introduce game implementations.

DDSURFACEDESC is 108 bytes, with genuine DDPIXELFORMAT unions, DDCOLORKEY and
DDSCAPS (SDK lines 171–178, 240–244, 387–423, 1178–1205). Retail memset size
and member accesses confirm it. Lock returns lpSurface at +36, assigned through
heroWindowManager::m_screen (+`0x42`) to bitmap::m_pixels (+`0x14`) and lpInitWin.
DDPCAPS_8BIT is 4, DDBLT_WAIT `0x01000000`, DDERR_NOCLIPPERATTACHED `0x88760238`.

Diagnostic sites load signed two-byte line bases and add these retail offsets;
the filename is `D:\Heroes\Source\wingraph.cpp`:

| Function RVA (size) | Line-base VA | Addends | Donor source lines (Buka / PoL) |
| --- | --- | --- | --- |
| DDRestoreDisplayMode `0x3640` (`0x59`) | `0x48e5c4` | 7 | Corresponding restore function |
| CreatePrimary `0x36df` (`0x9b`) | `0x48e5e8` | 10 | 60–153 / 61–129, startup family |
| SetupClipper `0x377a` (`0xeb`) | `0x48e60c` | 8, 13, 18 | Same startup family |
| DDInitGraphics `0x3865` (`0x171`) | `0x48e670` | 8, 20, 24, 31 | Same startup family |
| DDAppPaint `0x39d6` (`0x592`) | `0x48e6f8` | 72, 106, 110, 118, 123, 133, 144 | 158–295 / 126–228 |
| DDInitializePalette `0x3f68` (`0x140`) | `0x48e800` | 63 | Corresponding palette function |
| DDSetPalette `0x40a8` (`0xb3`) | `0x48e824` | 20 | Corresponding palette function |
| DDCreateSurface `0x415b` (`0x12a`) | `0x48e848` | 28, 36 | 378–421 / 289–322 |
| DDUpdatePalette `0x4673` (`0x11c`) | `0x48e8c0` | 18, 22 | 546–589 / 439–478 |
| DDCleanUp `0x478f` (`0x17f`) | `0x48e904` | 14, 38 | 591–632 / 481–519 |
| DDSetFullScreen `0x490e` (`0x2ea`) | `0x48e948` | 21, 27, 34, 39, 51 | 635–713 / 516–600 |

Four-byte COM owner pointers: lpDD VA `0x48e5b0`, primary surface `0x48e5b4`,
secondary surface `0x48e5b8`, clipper `0x48e5bc`, palette `0x48e5c0`.
Startup creates 640x480 surfaces, attaches clipper/window, sets cooperative
mode (normal or exclusive/fullscreen/allowreboot = 19), and selects 640x480x8.
Palette initialization retains system bands, mutable entries and busy guard;
SetPalette also checks foreground and object availability. DDUpdatePalette
converts entries 10–245 using signed component shifts by 2, PC_NOCOLLAPSE,
ProcessAssert and SetEntries. The pointer-to-int assertion is donor-supported.

DDSD (`0x4285`, `0x3ee`; Buka 427–541, PoL 323–437) guards reentry at VA
`0x48e84c`, restores mode, classifies 28 SDK errors, beeps three times, formats,
logs and shuts down. HoMM1 uses report IDs 1–28 and 100 for unknown errors.
Format: `Direct Draw Error #%d in file '%s' at Line #%d`. Locals restoreResult
and unused are donor-supported; the switch spill is compiler-generated.
HRESULT aliases use SDK E_FAIL/E_INVALIDARG/E_OUTOFMEMORY/E_NOTIMPL values.
The sprintf referent is RVA `0x806f0`; memset is `0x80820`.

Cleanup restores mode, detaches/releases clipper, releases surfaces/palette,
selects DDSCL_NORMAL and releases lpDD. Its stored-but-unobserved restoreResult
is present in both donors and retail (+`0x26`). Fullscreen switching saves or
restores geometry, changes COM mode, recreates surfaces/palette, writes prefs,
resizes the menu/window and resets clipping. Its width/windowHeight/x/y/hres
locals survive in the donor. WritePrefs is RVA `0x5d740` (Buka Misc.cpp:1486).

## DirectDraw paint

DDAppPaint retains busy/minimized/DD guards, paint/client rectangles, coordinate
conversion, scroll overrides, unlock/clamp, blit retry/restore, relock and EndPaint.
HoMM1 adds BltD/BltS rectangle logging inside the retry loop. PoL's six locals
are sourceTop4/sourceWidth5/sourceHeight3/sourceLeft3/point0/paint3; Buka supplies
alternate names/order. These are source clues, not artificial frame controls.

Retail accesses prove the independent owners below; none overlaps screenImage:

| VA | Object / layout |
| --- | --- |
| `0x48e674` | iBusyRetry, int |
| `0x4a46a0` | gDDResult, long |
| `0x4a46a8` / `0x4a46b8` | source / destination RECT, 16 bytes each |
| `0x4a46c8` | lPaintStart, long |
| `0x4a46d0` | gDDSurfaceDesc, 108 bytes |
| `0x4a4b70` | client RECT, 16 bytes |
| `0x493030` | external gbMinimized flag, four bytes |

Callee RVA `0x19bd4` takes a label and five 32-bit values, formats
`%s : % 8d  % 8d  % 8d  % 8d  % 8d`, and writes KB.LOG.
`LogStr(char*,long,long,long,long,long)` is a provisional family identity,
not a recovered original name. Its 132-byte buffer frame's original extent/
local census remains unresolved, so only the declaration/referent is admitted.

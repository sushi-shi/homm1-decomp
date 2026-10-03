#ifndef HOMM1_BASE_MOUSEMANAGER_H
#define HOMM1_BASE_MOUSEMANAGER_H
// Reconstructed class (BASE) from CodeView NB09 of HEROES2W.EXE — NOT original source.
// 17 methods, 3 own-virtual, 0 static data.

#include <BASE/baseManager.h>
#include <H1/Macros.h>
#define WIN32_LEAN_AND_MEAN

#include <windows.h>

// forward declarations:
struct tag_message;
class bitmap;

// SetPointer frame that leaves the current pointer alone: SetPointer returns
// for any negative frame (TOWNMGR Close and ADVMGR pass it; Buka
// MOUSE_INVALID_CURSOR_FRAME).
H1_ENUM_CONST_BEGIN(MouseCursorFrameConstant)
    MOUSE_INVALID_CURSOR_FRAME = -1
H1_ENUM_CONST_END(MouseCursorFrameConstant)

#pragma pack(push, 1)
                               class mouseManager : public baseManager {
public:
    void* m_cursorResource;
    bitmap* m_savedUnderlying;
    void* m_cursorImage;
    short m_cursorFrame;
    short m_cursorReady;
    // Constructor and UpdateScreenRegion establish the packed tail.
    signed char m_pointerFlags;
    // CheckDoMain compares the pointer position less this offset with the
    // last drawn position at +0x5b/+0x5d.
    short m_hotspotX;
    short m_hotspotY;
    short m_mouseX;
    short m_mouseY;
    int m_unknown49;
    int m_unknown4d;
    char m_unknown51;
    char m_unknown52[9];
    short m_drawnX;
    short m_drawnY;

    // --- constructors ---
    mouseManager(void);
    // --- virtual methods (vtable order) ---
    virtual short Open(short) OVERRIDE;
    virtual void Close(void) OVERRIDE;
    virtual short Main(struct tag_message&) OVERRIDE;
    // --- methods ---
    // CombatManager::ViewSpells passes a sign-extended word frame.
    void SetPointer(char*, short);
    void SetPointer(short);
    void NewUpdate(int);
    void MouseCoords(short&, short&);
    void SaveAndDraw(void);
    // HoMM1 Windows keeps the DOS buffer-pointer hooks as empty stubs.
    void SaveAndDraw(class bitmap*, short, short, short);
    void RestoreUnderlying(void);
    void ReallyHidePointer(void);
    void ReallyShowPointer(void);
    void HideColorPointer(void);
    void ShowColorPointer(void);
    int IsVis(void) {
        return m_pointerFlags & 1;
    }
    void CheckUpdateMousePos(void);
    // Empty in the Windows build (retail 0x00476e20, `ret 8`).
    void MovePointer(short, short);
    // Empty in the Windows build (retail 0x00476ec0, `ret 4`); the locator
    // knob drag passes 4 on entry and 6 on release.
    void SetCursorShape(int);
    // Empty in the Windows build (retail 0x00476e50, `ret 8`).
    void WarpPointer(short, short);
    void SetColorMice(int);
    // The quick views hide (retail 0x00476ee0, ShowCursor(0)) and restore
    // (0x00476ef0, ShowCursor(1)) the Windows cursor around QuickViewWait.
    void HideSystemCursor(void);
    void ShowSystemCursor(void);
};
#pragma pack(pop)

H1_ENUM_CONST_BEGIN(MouseManagerConstant)
    MOUSE_CURSOR_COUNT = 75,
    MOUSE_CURSOR_AXIS_COUNT = 2,
    MOUSE_CURSOR_HORIZONTAL = 0,
    MOUSE_CURSOR_VERTICAL = 1,
    MOUSE_CURSOR_BITMAP_BEGIN = 0,
    MOUSE_CURSOR_BITMAP_END = 32,
    MOUSE_CURSOR_BITMAP_WIDTH = MOUSE_CURSOR_BITMAP_END,
    MOUSE_CURSOR_MASK_HEIGHT = 64,
    MOUSE_CURSOR_MASK_ROW_BYTES = 4,
    MOUSE_CURSOR_COLOR_BYTES = 0x400,
    MOUSE_CURSOR_AND_BYTES = 0x100,
    MOUSE_CURSOR_MASK_PLANE_BYTES = 0x80,
    MOUSE_CURSOR_BITMAP_HEADER_BYTES = 6,
    MOUSE_CURSOR_BITMAP_PLANES = 1,
    MOUSE_CURSOR_BITMAP_BITS_PER_PIXEL = 1,
    MOUSE_CURSOR_COLOR_BITS_PER_PIXEL = 8,
    MOUSE_CURSOR_MASK_SHIFT = 3,
    MOUSE_CURSOR_MASK_HIGH_BIT = 7,
    MOUSE_CURSOR_ADVENTURE = 0,
    MOUSE_CURSOR_COMBAT = 1,
    MOUSE_CURSOR_SPELL = 2,
    MOUSE_KEEP_CURRENT_FRAME = 1000
H1_ENUM_CONST_END(MouseManagerConstant)

extern int gMouseCursorType;
extern int iMouseOffset[3];
extern unsigned char iHotSpot[MOUSE_CURSOR_COUNT][MOUSE_CURSOR_AXIS_COUNT];
extern HCURSOR hMouseCursor[MOUSE_CURSOR_COUNT];
extern signed char* cColorBits[MOUSE_CURSOR_COUNT];
extern unsigned char* cAndBits[MOUSE_CURSOR_COUNT];
extern BITMAP bmpAndMask[MOUSE_CURSOR_COUNT];
extern BITMAP bmpColor[MOUSE_CURSOR_COUNT];
extern HBITMAP hbmpAndMask[MOUSE_CURSOR_COUNT];
extern HBITMAP hbmpColor[MOUSE_CURSOR_COUNT];
extern ICONINFO mouseIconInfo[MOUSE_CURSOR_COUNT];

#endif // HOMM1_BASE_MOUSEMANAGER_H

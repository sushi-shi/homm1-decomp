#ifndef HOMM1_BASE_MOUSEMANAGER_H
#define HOMM1_BASE_MOUSEMANAGER_H

#include <Domains.h>

#include <BASE/baseManager.h>
#include <BASE/display.h>
#include <H1/Macros.h>
#include <BASE/message.h>
#define WIN32_LEAN_AND_MEAN

#include <windows.h>

// forward declarations:
struct tag_message;
class bitmap;

// SetPointer frame that leaves the current pointer alone: SetPointer returns
// for any negative frame (TOWNMGR Close and ADVMGR pass it).
H1_ENUM_CONST_BEGIN(MouseCursorFrameConstant)
    MOUSE_INVALID_CURSOR_FRAME = -1
H1_ENUM_CONST_END(MouseCursorFrameConstant)

#pragma pack(push, 1)
class mouseManager : public baseManager {
public:
    void* m_cursorResource;
    bitmap* m_savedUnderlying;
    void* m_cursorImage;
    i16 m_cursorFrame;
    i16 m_cursorReady;
    // Constructor and UpdateScreenRegion establish the packed tail.
    i8 m_pointerFlags;
    // CheckDoMain compares the pointer position less this offset with the
    // last drawn position at +0x5b/+0x5d.
    i16 m_hotspotX;
    i16 m_hotspotY;
    i16 m_mouseX;
    i16 m_mouseY;
    // The cursor's saved screen area: ComboDraw marks the map cells under it.
    // The mouse code never updates them after the constructor clears them.
    i32 m_savedLeft;
    i32 m_savedTop;
    i8 m_unknown51;
    char m_unused52[9];
    i16 m_drawnX;
    i16 m_drawnY;

    // --- constructors ---
    mouseManager(void);
    // --- virtual methods (vtable order) ---
    virtual H1_ENUM_RETURN(BaseManagerStatus, i16) Open(i16 priority) OVERRIDE;
    virtual void Close(void) OVERRIDE;
    virtual H1_ENUM_RETURN(MessageDispatchResult, i16) Main(struct tag_message& message) OVERRIDE;
    // --- methods ---
    // CombatManager::ViewSpells passes a sign-extended word frame.
    void SetPointer(char* name, i16 frame);
    void SetPointer(i16 frame);
    void NewUpdate(i32 force);
    void MouseCoords(i16& x, i16& y);
    void SaveAndDraw(void);
    // The buffer-pointer hooks are empty stubs.
    void SaveAndDraw(class bitmap* buffer, i16 x, i16 y, i16 cursorUpdate);
    void RestoreUnderlying(void);
    void ReallyHidePointer(void);
    void ReallyShowPointer(void);
    void HideColorPointer(void);
    void ShowColorPointer(void);
    i32 IsVis(void) {
        return m_pointerFlags & 1;
    }
    // Empty in the Windows build (retail 0x00476e20, `ret 8`).
    void MovePointer(i16 x, i16 y);
    // Empty in the Windows build (retail 0x00473410, `ret 4`); the locator
    // knob drag passes 4 on entry and 6 on release.
    void SetCursorShape(i32 shape);
    // Empty in the Windows build (retail 0x00476e50, `ret 8`).
    void WarpPointer(i16 x, i16 y);
    void SetColorMice(i32 enabled);
    // Empty unreferenced Windows-build hooks; original names unavailable.
    void UnusedTwoArgumentHook1(i16, i16);
    void UnusedTwoArgumentHook2(i16, i16);
    void UnusedOneArgumentHook(i32);
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

extern i32 gMouseCursorType;
extern i32 gMouseOffset[3];
extern u8 gHotSpot[MOUSE_CURSOR_COUNT][MOUSE_CURSOR_AXIS_COUNT];
#define gMouseCursors hMouseCursor // spelling fixes .bss order
extern HCURSOR gMouseCursors[MOUSE_CURSOR_COUNT];
#define gColorBits cColorBits // spelling fixes .bss order
extern u8* gColorBits[MOUSE_CURSOR_COUNT];
#define gAndBits cAndBits // spelling fixes .bss order
extern u8* gAndBits[MOUSE_CURSOR_COUNT];
extern BITMAP gAndMaskBitmapInfo[MOUSE_CURSOR_COUNT];
#define gColorBitmapInfo bmpColor // spelling fixes .bss order
extern BITMAP gColorBitmapInfo[MOUSE_CURSOR_COUNT];
#define gAndMaskBitmaps hbmpAndMask // spelling fixes .bss order
extern HBITMAP gAndMaskBitmaps[MOUSE_CURSOR_COUNT];
#define gColorBitmaps hbmpColor // spelling fixes .bss order
extern HBITMAP gColorBitmaps[MOUSE_CURSOR_COUNT];
#define gMouseIconInfo cursorIconInfo // spelling fixes .bss order
extern ICONINFO gMouseIconInfo[MOUSE_CURSOR_COUNT];

// Moved from MOUSEMGR.cpp.
H1_ENUM_CONST_BEGIN(MouseManagerStateConstant)
    MOUSE_INITIAL_POINTER_FLAGS = 6,
    // The pointer starts at the centre of the logical screen.
    MOUSE_INITIAL_X = LOGICAL_SCREEN_WIDTH / 2,
    MOUSE_INITIAL_Y = LOGICAL_SCREEN_HEIGHT / 2,
    MOUSE_SAVED_BITMAP_SIZE = 0x40,
    MOUSE_CURSOR_FILENAME_CAPACITY = 16
H1_ENUM_CONST_END(MouseManagerStateConstant)

#endif // HOMM1_BASE_MOUSEMANAGER_H

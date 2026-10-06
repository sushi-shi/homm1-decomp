#ifndef HOMM1_BASE_MOUSEMANAGER_H
#define HOMM1_BASE_MOUSEMANAGER_H

#include <BASE/baseManager.h>
#define WIN32_LEAN_AND_MEAN

#include <windows.h>

struct tag_message;
class bitmap;

enum MouseCursorFrameConstant {
    MOUSE_INVALID_CURSOR_FRAME = -1
};

#pragma pack(push, 1)
class mouseManager : public baseManager {
public:
    void* m_cursorResource;
    bitmap* m_savedUnderlying;
    void* m_cursorImage;
    i16 m_cursorFrame;
    i16 m_cursorReady;
    i8 m_pointerFlags;
    i16 m_hotspotX;
    i16 m_hotspotY;
    i16 m_mouseX;
    i16 m_mouseY;
    i32 m_unknown49;
    i32 m_unknown4d;
    i8 m_unknown51;
    char m_unknown52[9];
    i16 m_drawnX;
    i16 m_drawnY;

    mouseManager(void);
    virtual i16 Open(i16 priority) ;
    virtual void Close(void) ;
    virtual i16 Main(struct tag_message&) ;
    void SetPointer(char* name, i16 frame);
    void SetPointer(i16 frame);
    void NewUpdate(i32);
    void MouseCoords(i16& x, i16& y);
    void SaveAndDraw(void);
    void SaveAndDraw(class bitmap*, i16, i16, i16);
    void RestoreUnderlying(void);
    void ReallyHidePointer(void);
    void ReallyShowPointer(void);
    void HideColorPointer(void);
    void ShowColorPointer(void);
    i32 IsVis(void) {
        return m_pointerFlags & 1;
    }
    void CheckUpdateMousePos(void);
    void MovePointer(i16, i16);
    void SetCursorShape(i32);
    void WarpPointer(i16, i16);
    void SetColorMice(i32);
    void HideSystemCursor(void);
    void ShowSystemCursor(void);
};
#pragma pack(pop)

enum MouseManagerConstant {
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
};

extern i32 gMouseCursorType;
extern i32 gMouseOffset[3];
extern u8 gHotSpot[MOUSE_CURSOR_COUNT][MOUSE_CURSOR_AXIS_COUNT];
extern HCURSOR hMouseCursor[MOUSE_CURSOR_COUNT];
extern i8* gColorBits[MOUSE_CURSOR_COUNT];
extern u8* cAndBits[MOUSE_CURSOR_COUNT];
extern BITMAP bmpAndMask[MOUSE_CURSOR_COUNT];
extern BITMAP bmpColor[MOUSE_CURSOR_COUNT];
extern HBITMAP hbmpAndMask[MOUSE_CURSOR_COUNT];
extern HBITMAP hbmpColor[MOUSE_CURSOR_COUNT];
extern ICONINFO mouseIconInfo[MOUSE_CURSOR_COUNT];

#endif

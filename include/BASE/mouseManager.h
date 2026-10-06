#ifndef HOMM1_BASE_MOUSEMANAGER_H
#define HOMM1_BASE_MOUSEMANAGER_H

#include <BASE/baseManager.h>
#include <BASE/display.h>
#include <BASE/message.h>

struct tag_message;
class bitmap;

enum MouseCursorFrameConstant {
    MOUSE_INVALID_CURSOR_FRAME = -1
};

enum MousePointerFlag {
    MOUSE_POINTER_FLAG_VISIBLE = 1
};

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
    i32 m_savedLeft;
    i32 m_savedTop;
    i8 m_drawIntoScreen;
    i8 m_bandFlushed;
    i32 m_bandSplitY;
    i16 m_cursorWidth;
    i16 m_cursorHeight;
    i16 m_drawnX;
    i16 m_drawnY;

    mouseManager(void);
    virtual i16 Open(i16 priority) ;
    virtual void Close(void) ;
    virtual i16 Main(struct tag_message& message) ;
    void SetPointer(char* name, i16 frame);
    void SetPointer(i16 frame);
    void NewUpdate(b32 force);
    void MouseCoords(i16& x, i16& y);
    void SaveAndDraw(void);
    void SaveAndDraw(class bitmap* buffer, i16 x, i16 y, i16 cursorUpdate);
    void RestoreUnderlying(void);
    void ReallyHidePointer(void);
    void ReallyShowPointer(void);
    void HideColorPointer(void);
    void ShowColorPointer(void);
    i32 IsVis(void) {
        return m_pointerFlags & MOUSE_POINTER_FLAG_VISIBLE;
    }
    void MovePointer(i16 x, i16 y);
    void SetCursorShape(i32 shape);
    void WarpPointer(i16 x, i16 y);
    void SetColorMice(b32 enabled);
    void UnusedTwoArgumentHook1(i16, i16);
    void UnusedTwoArgumentHook2(i16, i16);
    void UnusedOneArgumentHook(i32);
    void HideSystemCursor(void);
    void ShowSystemCursor(void);
};

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
    MOUSE_KEEP_CURRENT_FRAME = 1000,
    MOUSE_CURSOR_PIXEL_TRANSPARENT = 0,
    MOUSE_CURSOR_PIXEL_OUTLINE = 1
};

extern i32 gMouseCursorType;
extern i32 gMouseOffset[3];
extern u8 gHotSpot[MOUSE_CURSOR_COUNT][MOUSE_CURSOR_AXIS_COUNT];
extern u8* gColorBits[MOUSE_CURSOR_COUNT];
extern u8* gAndBits[MOUSE_CURSOR_COUNT];

// The pointer shapes are host cursors, built from a 32x32 image: colorBits
// holds one palette index per pixel; maskBits the AND mask (and, for a
// monochrome cursor, the XOR mask after it), one bit per pixel, most
// significant bit first, MOUSE_CURSOR_MASK_ROW_BYTES per row. Windows builds
// an HCURSOR from them; the native port draws them over the game screen.
i32 KBCursorReady(i32 cursorIndex);
void KBCreateCursor(
    i32 cursorIndex,
    const u8* colorBits,
    const u8* maskBits,
    i32 colorCursor,
    i32 hotX,
    i32 hotY
);
void KBSelectCursor(i32 cursorIndex);
void KBSelectArrowCursor(void);
void KBDestroyCursors(void);

enum MouseManagerStateConstant {
    MOUSE_INITIAL_POINTER_FLAGS = 6,
    MOUSE_INITIAL_X = LOGICAL_SCREEN_WIDTH / 2,
    MOUSE_INITIAL_Y = LOGICAL_SCREEN_HEIGHT / 2,
    MOUSE_SAVED_BITMAP_SIZE = 0x40,
    MOUSE_CURSOR_FILENAME_CAPACITY = 16
};

#endif

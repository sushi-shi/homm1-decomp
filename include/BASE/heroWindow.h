#ifndef HOMM1_BASE_HEROWINDOW_H
#define HOMM1_BASE_HEROWINDOW_H

#include <Domains.h>
#include <H1/Macros.h>

// forward declarations:
class widget;
class bitmap;
struct tag_message;

H1_ENUM_BEGIN(WindowFlag)
    WINDOW_FLAG_NONE = 0,
    WINDOW_FLAG_FIXED_LAYER = 1,
    WINDOW_FLAG_SAVE_BACKGROUND = 2,
    WINDOW_FLAG_STRIP_WINDOW = 8,
    WINDOW_FLAG_OWNS_WIDGETS = 0x4000,
    WINDOW_UPDATE_SUPPRESS_MASK = 0x7fff
H1_ENUM_END(WindowFlag)

H1_ENUM_BEGIN(WindowState)
    WINDOW_STATE_CLOSED = 0,
    WINDOW_STATE_OPEN = 1
H1_ENUM_END(WindowState)

// heroWindow::Open status.
H1_ENUM_BEGIN(WindowOpenStatus)
    WINDOW_OPEN_SUCCESS = 0,
    WINDOW_OPEN_FAILURE = 3
H1_ENUM_END(WindowOpenStatus)

H1_ENUM_CONST_BEGIN(HeroWindowConstant)
    HERO_WINDOW_NAME_CAPACITY = 20,
    // AddWindow/AddWidget z-order meaning "one above the current top"; unlinked
    // windows and widgets keep it.
    WINDOW_Z_ORDER_APPEND = -1,
    WINDOW_ALL_WIDGETS_LOW = -65535,
    WINDOW_ALL_WIDGETS_HIGH = 65535
H1_ENUM_CONST_END(HeroWindowConstant)

#pragma pack(push, 1)
class heroWindow {
public:
    i16 m_zOrder;
    heroWindow* m_nextWindow;
    heroWindow* m_prevWindow;
    char m_name[HERO_WINDOW_NAME_CAPACITY];
    H1_ENUM_STORAGE(WindowFlag, i16) m_winFlags;
    H1_ENUM_STORAGE(WindowState, i16) m_winState;
    i16 m_posX;
    i16 m_posY;
    i16 m_winWidth;
    i16 m_winHeight;
    widget* m_widgetListTail;
    widget* m_widgetListHead;
    bitmap* m_savedBackground;

    // --- constructors ---
    heroWindow(void);
    heroWindow(i16 x, i16 y, i16 width, i16 height, i16 flags);
    heroWindow(i16 x, i16 y, char* resourceName);
    // --- methods ---
    i16 Open(i16 zOrder, i8 flags);
    void Close(void);
    void AddWidget(class widget* newWidget, i16 zOrder);
    void RemoveWidget(class widget* w);
    i16 BroadcastMessage(struct tag_message& message);
    void DrawWindow(void);
    void DrawWindow(i16 flags);
    void DrawWindow(i16 update, i32 firstId, i32 lastId);
    i16 SaveBackground(void);
    void RestoreBackground(void);
    void MoveWindow(i16 dx, i16 dy);
};
#pragma pack(pop)
// Moved from WINDOW.cpp.
H1_ENUM_BEGIN(WindowWidgetRecordType)
    WIDGET_RECORD_END = 0,
    WIDGET_RECORD_BORDER = 1,
    WIDGET_RECORD_BUTTON = 2,
    WIDGET_RECORD_TEXT = 8,
    WIDGET_RECORD_ICON = 0x10,
    WIDGET_RECORD_BACKDROP = 0x20,
    WIDGET_RECORD_DIMMER = 0x40,
    WIDGET_RECORD_TEXT_ENTRY = 0x100,
    WIDGET_RECORD_TEXT_ENTRY_RECT = 0x201,
    WIDGET_RECORD_TEXT_ENTRY_MULTILINE = 0x202
H1_ENUM_END(WindowWidgetRecordType)

#endif // HOMM1_BASE_HEROWINDOW_H

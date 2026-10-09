#ifndef HOMM1_BASE_HEROWINDOW_H
#define HOMM1_BASE_HEROWINDOW_H

#include <BASE/message.h>

class widget;
class bitmap;
struct tag_message;

enum WindowFlag {
    WINDOW_FLAG_NONE = 0,
    WINDOW_FLAG_FIXED_LAYER = 1,
    WINDOW_FLAG_SAVE_BACKGROUND = 2,
    WINDOW_FLAG_STRIP_WINDOW = 8,
    WINDOW_FLAG_OWNS_WIDGETS = 0x4000,
    WINDOW_UPDATE_SUPPRESS_MASK = 0x7fff
};

enum WindowState {
    WINDOW_STATE_CLOSED = 0,
    WINDOW_STATE_OPEN = 1
};

enum WindowOpenStatus {
    WINDOW_OPEN_SUCCESS = 0,
    WINDOW_OPEN_FAILURE = 3
};

enum HeroWindowConstant {
    HERO_WINDOW_NAME_CAPACITY = 20,
    WINDOW_Z_ORDER_APPEND = -1,
    WINDOW_Z_ORDER_BOTTOM = 0,
    WINDOW_ALL_WIDGETS_LOW = -65535,
    WINDOW_ALL_WIDGETS_HIGH = 65535
};

class heroWindow {
public:
    i16 m_zOrder;
    heroWindow* m_nextWindow;
    heroWindow* m_prevWindow;
    char m_name[HERO_WINDOW_NAME_CAPACITY];
    i16 m_winFlags;
    i16 m_winState;
    i16 m_posX;
    i16 m_posY;
    i16 m_winWidth;
    i16 m_winHeight;
    widget* m_widgetListTail;
    widget* m_widgetListHead;
    bitmap* m_savedBackground;

    heroWindow(void);
    heroWindow(i16 x, i16 y, i16 width, i16 height, i16 flags);
    heroWindow(i16 x, i16 y, char* resourceName);
    i16 Open(i16 zOrder, b8 updateScreen);
    void Close(void);
    void AddWidget(class widget* newWidget, i16 zOrder);
    void RemoveWidget(class widget* removedWidget);
    i16 BroadcastMessage(struct tag_message& message);
    void DrawWindow(void);
    void DrawWindow(i16 updateScreen);
    void DrawWindow(i16 updateScreen, i32 firstId, i32 lastId);
    i16 SaveBackground(void);
    void RestoreBackground(void);
    void MoveWindow(i16 dx, i16 dy);
};
enum WindowWidgetRecordType {
    WIDGET_RECORD_END = 0,
    WIDGET_RECORD_BORDER = 1,
    WIDGET_RECORD_BUTTON = 2,
    WIDGET_RECORD_TEXT = 8,
    WIDGET_RECORD_ICON = 0x10,
    WIDGET_RECORD_BACKDROP = 0x20,
    WIDGET_RECORD_DIMMER = 0x40,
    WIDGET_RECORD_TEXT_ENTRY = 0x100,
    WIDGET_RECORD_TEXT_ENTRY_RECT = 0x201,
    WIDGET_RECORD_TEXT_ENTRY_SCROLLING = 0x202
};

#endif

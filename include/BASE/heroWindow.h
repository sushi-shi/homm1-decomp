#ifndef HOMM1_BASE_HEROWINDOW_H
#define HOMM1_BASE_HEROWINDOW_H

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
    WINDOW_ALL_WIDGETS_LOW = -65535,
    WINDOW_ALL_WIDGETS_HIGH = 65535
};

#pragma pack(push, 1)
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
    i16 Open(i16 zOrder, i8 flags);
    void RemoveAndDeleteWidget(i32 id);
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
#endif

#ifndef HOMM1_BASE_HEROWINDOW_H
#define HOMM1_BASE_HEROWINDOW_H
// Reconstructed class (BASE) from CodeView NB09 of HEROES2W.EXE — NOT original source.
// 15 methods, 0 own-virtual, 0 static data.

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

// heroWindow::Open status (Buka WINDOW.cpp OPEN_FAILURE).
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
    heroWindow(i16, i16, i16, i16, i16);
    heroWindow(i16, i16, char*);
    // --- methods ---
    i16 Open(i16, i8);
    void RemoveAndDeleteWidget(i32 id);
    void Close(void);
    void AddWidget(class widget*, i16);
    void RemoveWidget(class widget*);
    i16 BroadcastMessage(struct tag_message&);
    void DrawWindow(void);
    void DrawWindow(i16);
    void DrawWindow(i16, i32, i32);
    i16 SaveBackground(void);
    void RestoreBackground(void);
    void MoveWindow(i16, i16);
};
#pragma pack(pop)
#endif // HOMM1_BASE_HEROWINDOW_H

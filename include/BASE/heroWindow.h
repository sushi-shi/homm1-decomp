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

H1_ENUM_BEGIN(HeroWindowConstant)
    HERO_WINDOW_NAME_CAPACITY = 20,
    WINDOW_ALL_WIDGETS_LOW = -65535,
    WINDOW_ALL_WIDGETS_HIGH = 65535
H1_ENUM_END(HeroWindowConstant)

#pragma pack(push, 1)
class heroWindow {
public:
    short m_zOrder;
    heroWindow *m_nextWindow;
    heroWindow *m_prevWindow;
    char m_name[HERO_WINDOW_NAME_CAPACITY];
    H1_ENUM_STORAGE(WindowFlag, short) m_winFlags;
    H1_ENUM_STORAGE(WindowState, short) m_winState;
    short m_posX;
    short m_posY;
    short m_winWidth;
    short m_winHeight;
    widget *m_widgetListTail;
    widget *m_widgetListHead;
    bitmap *m_savedBackground;

    // --- constructors ---
    heroWindow(void);
    heroWindow(short, short, short, short, short);
    heroWindow(int, int, char *);
    // --- methods ---
    short Open(short, signed char);
    void RemoveAndDeleteWidget(int);
    void Close(void);
    void AddWidget(class widget *, short);
    void RemoveWidget(class widget *);
    short BroadcastMessage(struct tag_message &);
    void DrawWindow(void);
    void DrawWindow(short);
    void DrawWindow(short, int, int);
    short SaveBackground(void);
    void RestoreBackground(void);
    void MoveWindow(short, short);
};
#pragma pack(pop)
#endif // HOMM1_BASE_HEROWINDOW_H

#ifndef HOMM1_BASE_HEROWINDOWMANAGER_H
#define HOMM1_BASE_HEROWINDOWMANAGER_H
// Reconstructed class (BASE) from CodeView NB09 of HEROES2W.EXE — NOT original source.
// 17 methods, 3 own-virtual, 0 static data.

#include <BASE/baseManager.h>
#include <H1/Macros.h>

// forward declarations:
class heroWindow;
class palette;
class bitmap;
struct tag_message;

#pragma pack(push, 1)
class heroWindowManager : public baseManager {
public:
    heroWindow *m_windowListHead;
    heroWindow *m_windowListTail;
    heroWindow *m_focusWindow;
    heroWindow *m_activeWindow;
    char m_unknown40;
    char m_unknown41;
    bitmap *m_screen;
    bitmap *m_fizzleSource;
    bitmap *m_fizzleWork;
    short m_screenshotIndex;
    short m_updateFlags;
    int m_dialogResult;
    signed char m_lastHoverId;

    // --- constructors ---
    heroWindowManager(void);
    // --- virtual methods (vtable order) ---
    virtual short Open(short) OVERRIDE;
    virtual void Close(void) OVERRIDE;
    virtual short Main(struct tag_message &) OVERRIDE;
    // --- methods ---
    short ConvertToHover(struct tag_message &);
    short BroadcastMessage(short, short, short, short);
    void AddWindow(class heroWindow *, int, int);
    void RemoveWindow(class heroWindow *);
    int DoDialog(class heroWindow *, int (*)(struct tag_message &), int);
    void UpdateScreen(void);
    void UpdateScreenRegion(short, short, short, short);
    void RedrawScreen(void);
    void FadeScreen(int, int, class palette *);
    void ScreenShot(void);
    void SaveFizzleSource(int, int, int, int);
    void FizzleForward(int, int, int, int, int, signed char *, signed char *);
    void ReleaseFizzleSource(void);
};
#pragma pack(pop)
#endif // HOMM1_BASE_HEROWINDOWMANAGER_H

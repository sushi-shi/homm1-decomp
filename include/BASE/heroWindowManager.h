#ifndef HOMM1_BASE_HEROWINDOWMANAGER_H
#define HOMM1_BASE_HEROWINDOWMANAGER_H
// Reconstructed class (BASE) from CodeView NB09 of HEROES2W.EXE — NOT original source.
// 17 methods, 3 own-virtual, 0 static data.

#include <BASE/baseManager.h>
#include <BASE/dialog.h>
#include <BASE/display.h>
#include <BASE/palette.h>
#include <Domains.h>
#include <H1/Macros.h>

// forward declarations:
class heroWindow;
class palette;
class bitmap;
struct tag_message;

// FizzleForward's delay argument asking for the manager's default transition
// delay (WINMGR.cpp FIZZLE_DEFAULT_DELAY).
H1_ENUM_CONST_BEGIN(FizzleDelayConstant)
    FIZZLE_USE_DEFAULT_DELAY = -1
H1_ENUM_CONST_END(FizzleDelayConstant)

#pragma pack(push, 1)
class heroWindowManager : public baseManager {
public:
    heroWindow* m_windowListHead;
    heroWindow* m_windowListTail;
    heroWindow* m_focusWindow;
    heroWindow* m_activeWindow;
    i8 m_unknown40;
    i8 m_unknown41;
    bitmap* m_screen;
    bitmap* m_fizzleSource;
    bitmap* m_fizzleWork;
    i16 m_screenshotIndex;
    i16 m_updateFlags;
    i32 m_dialogResult;
    i8 m_lastHoverId;

    // --- constructors ---
    heroWindowManager(void);
    // --- virtual methods (vtable order) ---
    virtual i16 Open(i16) OVERRIDE;
    virtual void Close(void) OVERRIDE;
    virtual i16 Main(struct tag_message&) OVERRIDE;
    // --- methods ---
    i16 ConvertToHover(struct tag_message& message);
    i16 BroadcastMessage(i16, i16, i16, i16);
    void AddWindow(class heroWindow*, i16, i8);
    void RemoveWindow(class heroWindow*);
    i16 DoDialog(class heroWindow*, i16 (*)(struct tag_message&), i32);
    void UpdateScreen(void);
    void UpdateScreenRegion(i16, i16, i16, i16);
    void RedrawScreen(void);
    void FadeScreen(i16, i16, class palette*);
    void ScreenShot(void);
    void SaveFizzleSource(i16, i16, i16, i16);
    void FizzleForward(i16, i16, i16, i16, i32);
    void ReleaseFizzleSource(void);
};
#pragma pack(pop)

// A dialog handler records the selected widget as the dialog result and turns
// the message into the dialog-select notification (Buka 2.1
// heroWindowManager.h; HoMM1 assigns id and command in one chain).
#define FINISH_DIALOG_MESSAGE(message)                                                             \
    (gpWindowManager->m_dialogResult = (message).id,                                               \
     (message).command = (message).id = WIDGET_COMMAND_DIALOG_SELECT)

// Redraw the inclusive screen rectangle left..right, top..bottom (Buka 2.1
// heroWindowManager.h).
#define UPDATE_INCLUSIVE_REGION(left, top, right, bottom)                                          \
    (gpWindowManager->UpdateScreenRegion((left), (top), (right) - (left) + 1, (bottom) - (top) + 1))
extern i8 gCyclePal[PALETTE_CYCLE_BYTES];
void CycleColors(void);
extern i8 gWindowFadeSavedUpdate;

H1_ENUM_BEGIN(WindowFadeMode)
    WINDOW_FADE_IN = 0,
    WINDOW_FADE_OUT = 1
H1_ENUM_END(WindowFadeMode)

// Palette fade lengths passed to FadeIn/FadeOut/FadeScreen (Buka SMACKMGR
// SHORT_FADE / NORMAL_FADE): the short fade of dialogs and screen changes and
// the long fade of the window manager's start-up.
H1_ENUM_BEGIN(WindowFadeSteps)
    WINDOW_FADE_STEPS_SHORT = 8,
    WINDOW_FADE_STEPS_NORMAL = 0x80
H1_ENUM_END(WindowFadeSteps)

H1_ENUM_CONST_BEGIN(WindowManagerConstant)
    WINDOW_MANAGER_NO_DIALOG_RESULT = -1,
    WINDOW_MANAGER_NO_HOVER_WIDGET = -1,
    // heroWindowManager::Open when the screen bitmap is missing.
    WINDOW_MANAGER_OPEN_FAILURE = 1
H1_ENUM_CONST_END(WindowManagerConstant)

class palette;

#endif // HOMM1_BASE_HEROWINDOWMANAGER_H

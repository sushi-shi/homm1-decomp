#ifndef HOMM1_BASE_HEROWINDOWMANAGER_H
#define HOMM1_BASE_HEROWINDOWMANAGER_H

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
    virtual i16 Open(i16 managerOrder) OVERRIDE;
    virtual void Close(void) OVERRIDE;
    virtual i16 Main(struct tag_message& message) OVERRIDE;
    // --- methods ---
    i16 UpdateHoverWindow(i16 x, i16 y);
    i16 ConvertToHover(struct tag_message& message);
    i16 BroadcastMessage(i16 type, i16 command, i16 widgetId, i16 value);
    void AddWindow(class heroWindow* window, i16 zOrder, i8 openFlags);
    void RemoveWindow(class heroWindow* window);
    i16 DoDialog(class heroWindow* window, i16 (*handler)(struct tag_message&), i32 fade);
    void UpdateScreen(void);
    void UpdateScreenRegion(i16 x, i16 y, i16 width, i16 height);
    void RedrawScreen(void);
    void Cleanup(void);
    void FadeScreen(i16 direction, i16 steps, class palette* currentPalette);
    void ScreenShot(void);
    void SaveFizzleSource(i16 x, i16 y, i16 width, i16 height);
    void FizzleForward(i16 x, i16 y, i16 width, i16 height, i32 delay);
    void ReleaseFizzleSource(void);
};
#pragma pack(pop)

// A dialog handler records the selected widget as the dialog result and turns
// the message into the dialog-select notification.
#define FINISH_DIALOG_MESSAGE(message)                                                             \
    (gWindowManager->m_dialogResult = (message).id,                                                \
     (message).command = (message).id = WIDGET_COMMAND_DIALOG_SELECT)

// Redraw the inclusive screen rectangle left..right, top..bottom.
#define UPDATE_INCLUSIVE_REGION(left, top, right, bottom)                                          \
    (gWindowManager->UpdateScreenRegion((left), (top), (right) - (left) + 1, (bottom) - (top) + 1))
extern i8 gCyclePal[PALETTE_CYCLE_BYTES];
void CycleColors(void);
extern i8 gFadeSavedUpdate;

H1_ENUM_BEGIN(WindowFadeMode)
    WINDOW_FADE_IN = 0,
    WINDOW_FADE_OUT = 1
H1_ENUM_END(WindowFadeMode)

// Palette fade lengths passed to FadeIn/FadeOut/FadeScreen: the short fade
// of dialogs and screen changes and the long fade of the window manager's
// start-up.
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

// Moved from WINMGR.cpp.
// FizzleForward's colour-cycle transition: eight CCYCLE tables of 64K
// word-indexed lookups.
H1_ENUM_CONST_BEGIN(WindowFizzleConstant)
    CYCLE_FRAME_COUNT = 8,
    FIZZLE_DEFAULT_DELAY = 150,
    FIZZLE_CYCLE_TABLE_BYTES = 0x10000,
    FIZZLE_LOOKUP_HIGH_BYTE_SHIFT = 8,
    PALETTE_CUBE_LEVELS = 64,
    PALETTE_NEAREST_DISTANCE_LIMIT = 1000,
    FIZZLE_COLOR_PAIR_FLOATS = 256 * 256 * 3,
    SCREENSHOT_FILENAME_CAPACITY = 16
H1_ENUM_CONST_END(WindowFizzleConstant)

#endif // HOMM1_BASE_HEROWINDOWMANAGER_H

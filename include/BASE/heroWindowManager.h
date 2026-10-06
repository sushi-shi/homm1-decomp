#ifndef HOMM1_BASE_HEROWINDOWMANAGER_H
#define HOMM1_BASE_HEROWINDOWMANAGER_H

#include <BASE/baseManager.h>
#include <BASE/dialog.h>
#include <BASE/display.h>
#include <BASE/palette.h>

class heroWindow;
class palette;
class bitmap;
struct tag_message;

enum FizzleDelayConstant {
    FIZZLE_USE_DEFAULT_DELAY = -1
};

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

    heroWindowManager(void);
    virtual i16 Open(i16 managerOrder) ;
    virtual void Close(void) ;
    virtual i16 Main(struct tag_message& message) ;
    i16 ConvertToHover(struct tag_message& message);
    i16 BroadcastMessage(i16 type, i16 command, i16 widgetId, i16 value);
    void AddWindow(class heroWindow* window, i16 zOrder, i8 openFlags);
    void RemoveWindow(class heroWindow* window);
    i16 DoDialog(class heroWindow* window, i16 (*handler)(struct tag_message&), i32 fade);
    void UpdateScreen(void);
    void UpdateScreenRegion(i16 x, i16 y, i16 width, i16 height);
    void RedrawScreen(void);
    void FadeScreen(i16 direction, i16 steps, class palette* currentPalette);
    void ScreenShot(void);
    void SaveFizzleSource(i16 x, i16 y, i16 width, i16 height);
    void FizzleForward(i16 x, i16 y, i16 width, i16 height, i32 delay);
    void ReleaseFizzleSource(void);
};
#pragma pack(pop)

#define FINISH_DIALOG_MESSAGE(message)                                                             \
    (gpWindowManager->m_dialogResult = (message).id,                                               \
     (message).command = (message).id = WIDGET_COMMAND_DIALOG_SELECT)

#define UPDATE_INCLUSIVE_REGION(left, top, right, bottom)                                          \
    (gpWindowManager->UpdateScreenRegion((left), (top), (right) - (left) + 1, (bottom) - (top) + 1))
extern i8 gCyclePal[PALETTE_CYCLE_BYTES];
void CycleColors(void);
extern i8 gWindowFadeSavedUpdate;

enum WindowFadeMode {
    WINDOW_FADE_IN = 0,
    WINDOW_FADE_OUT = 1
};

enum WindowFadeSteps {
    WINDOW_FADE_STEPS_SHORT = 8,
    WINDOW_FADE_STEPS_NORMAL = 0x80
};

enum WindowManagerConstant {
    WINDOW_MANAGER_NO_DIALOG_RESULT = -1,
    WINDOW_MANAGER_NO_HOVER_WIDGET = -1,
    WINDOW_MANAGER_OPEN_FAILURE = 1
};

class palette;

#endif

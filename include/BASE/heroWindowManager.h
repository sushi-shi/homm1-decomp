#ifndef HOMM1_BASE_HEROWINDOWMANAGER_H
#define HOMM1_BASE_HEROWINDOWMANAGER_H

#include <BASE/baseManager.h>
#include <BASE/dialog.h>
#include <BASE/display.h>
#include <BASE/message.h>
#include <BASE/palette.h>

class heroWindow;
class palette;
class bitmap;
struct tag_message;

enum FizzleDelayConstant {
    FIZZLE_USE_DEFAULT_DELAY = -1
};

enum WindowFadeMode {
    WINDOW_FADE_IN = 0,
    WINDOW_FADE_OUT = 1
};

enum WindowFadeIncrement {
    WINDOW_FADE_SHORT = 8,
    WINDOW_FADE_NORMAL = 0x80
};

enum WindowManagerConstant {
    WINDOW_MANAGER_NO_DIALOG_RESULT = -1,
    WINDOW_MANAGER_NO_HOVER_WIDGET = -1
};

#pragma pack(push, 1)
class heroWindowManager : public baseManager {
public:
    heroWindow* m_windowListHead;
    heroWindow* m_windowListTail;
    heroWindow* m_focusWindow;
    heroWindow* m_previousFocusWindow;
    i8 m_unused40;
    i8 m_unused41;
    bitmap* m_screen;
    bitmap* m_fizzleSource;
    bitmap* m_fizzleWork;
    i16 m_screenshotIndex;
    i16 m_colorCycling;
    i32 m_dialogResult;
    i8 m_lastHoverId;

    heroWindowManager(void);
    virtual i16 Open(i16 priority) ;
    virtual void Close(void) ;
    virtual i16 Main(struct tag_message& message) ;
    i16 UpdateHoverWindow(i16 x, i16 y);
    i16 BroadcastMessage(
        i16 type,
        i16 command,
        i16 widgetId,
        i16 value
    );
    void AddWindow(class heroWindow* window, i16 zOrder, b8 updateScreen);
    void RemoveWindow(class heroWindow* window);
    i16 DoDialog(
        class heroWindow* window,
        i16 (*handler)(struct tag_message&),
        b32 fade
    );
    void UpdateScreen(void);
    void UpdateScreenRegion(i16 x, i16 y, i16 width, i16 height);
    void RedrawScreen(void);
    void Cleanup(void);
    void FadeScreen(
        i16 direction,
        i16 increment,
        class palette* currentPalette
    );
    void ScreenShot(void);
    void SaveFizzleSource(i16 x, i16 y, i16 width, i16 height);
    void FizzleForward(i16 x, i16 y, i16 width, i16 height, i32 delay);
    void ReleaseFizzleSource(void);
};
#pragma pack(pop)

#define FINISH_DIALOG_MESSAGE(message)                                                             \
    (gWindowManager->m_dialogResult = (message).id,                                                \
     (message).command = ((message).id = WIDGET_COMMAND_DIALOG_SELECT))

#define UPDATE_INCLUSIVE_REGION(left, top, right, bottom)                                          \
    (gWindowManager->UpdateScreenRegion((left), (top), (right) - (left) + 1, (bottom) - (top) + 1))
extern i8 gCyclePal[PALETTE_CYCLE_BYTES];
void CycleColors(void);
extern i8 gFadeSavedColorCycling;

enum WindowFizzleConstant {
    CYCLE_FRAME_COUNT = 8,
    FIZZLE_DEFAULT_DELAY = 150,
    FIZZLE_CYCLE_TABLE_BYTES = 0x10000,
    FIZZLE_LOOKUP_HIGH_BYTE_SHIFT = 8,
    PALETTE_CUBE_LEVELS = 64,
    PALETTE_NEAREST_DISTANCE_LIMIT = 1000,
    FIZZLE_COLOR_PAIR_FLOATS = 256 * 256 * 3,
    SCREENSHOT_FILENAME_CAPACITY = 16
};

#endif

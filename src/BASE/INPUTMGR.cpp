// Located from HoMM2 Buka 2.1; PoL 2.0 supplies the VC4 declaration.

#define WIN32_LEAN_AND_MEAN

#include <match.h>

#include <windows.h>

#include <BASE/INPUTMGR_TYPES.h>
#include <BASE/Misc.h>
#include <BASE/mouseManager.h>
#include <H1/All.h>

short gInputManagerAssertLine = 137;
char gLeftReleaseCaptureFailure[] = "ReleaseCapture Failed";
char gRightReleaseCaptureFailure[] = "ReleaseCapture Failed";
char gInputManagerAssertFile[] = "D:\\Heroes\\Base\\INPUTMGR.CPP";

static inline void ResetEventQueue(inputManager *manager)
{
    manager->m_writeIndex = 0;
    manager->m_readIndex = 0;
}

// donor PoL RVA 0x000cde60; preferred Buka symbol ?MouseMessageHandler@@YIHPAXIIJ@Z
// donor Buka TU BASE/INPUTMGR; HoMM1 owner inferred from contiguous order
// evidence: graph:3;base=0.528083;margin=0.800438;shape=0.151;size=0.712;calls=0.800;strings=ReleaseCapture Failed;alternate=pol20:int MouseMessageHandler(void *, unsigned int, unsigned int, long int)@0x000cde60
VA(0x0047be30, 0x280)
int MouseMessageHandler(
    void *, unsigned int message, unsigned int, long messageData)
{
    if (gpInputManager == 0)
        return 1;
    if (gpInputManager->m_active != 1)
        return 1;
    if (gpInputManager->m_mouseMessageActive != 0)
        return 1;
    gpInputManager->m_mouseMessageActive = 1;

    tag_message *event =
        &gpInputManager->m_eventRing[gpInputManager->m_writeIndex];
    event->payload.mouse.modifiers = MESSAGE_MODIFIER_NONE;
    event->payload.mouse.y = 0;
    event->payload.mouse.x = 0;
    event->type = MESSAGE_NONE;

    switch (message) {
    case WM_MOUSEMOVE:
        event->type = MESSAGE_MOUSE_MOVE;
        goto mouseCoordinates;
    case WM_LBUTTONDOWN:
        event->type = MESSAGE_LEFT_BUTTON_DOWN;
        SetCapture(hwndApp);
        goto mouseCoordinates;
    case WM_LBUTTONUP:
        event->type = MESSAGE_LEFT_BUTTON_UP;
        if (ReleaseCapture() == 0)
            LogStr(gLeftReleaseCaptureFailure);
        goto mouseCoordinates;
    case WM_LBUTTONDBLCLK:
        event->type = MESSAGE_LEFT_BUTTON_DOWN;
        goto mouseCoordinates;
    case WM_RBUTTONDOWN:
        event->type = MESSAGE_RIGHT_BUTTON_DOWN;
        SetCapture(hwndApp);
        goto mouseCoordinates;
    case WM_RBUTTONUP:
        event->type = MESSAGE_RIGHT_BUTTON_UP;
        if (ReleaseCapture() == 0)
            LogStr(gRightReleaseCaptureFailure);
        goto mouseCoordinates;
    case WM_RBUTTONDBLCLK:
        event->type = MESSAGE_RIGHT_BUTTON_DOWN;
        goto mouseCoordinates;
    default:
        goto afterMouseCoordinates;
    }

mouseCoordinates:
    ProcessAssert(
        iMainWinScreenHeight > 0 && iMainWinScreenWidth > 0,
        gInputManagerAssertFile,
        gInputManagerAssertLine + 50);
    event->payload.mouse.x =
        static_cast<unsigned short>(messageData) * INPUT_GAME_WIDTH
        / iMainWinScreenWidth;
    event->payload.mouse.y =
        static_cast<unsigned short>(static_cast<unsigned long>(messageData) >> 16)
        * INPUT_GAME_HEIGHT / iMainWinScreenHeight;

    if (message == WM_MOUSEMOVE && gpMouseManager != 0
        && event->payload.mouse.x > INPUT_CURSOR_INTERIOR_X_MIN
        && event->payload.mouse.x < INPUT_CURSOR_INTERIOR_X_MAX
        && event->payload.mouse.y > INPUT_CURSOR_INTERIOR_Y_MIN
        && event->payload.mouse.y < INPUT_CURSOR_INTERIOR_Y_MAX)
        gpMouseManager->SetPointer(INPUT_KEEP_CURRENT_MOUSE_FRAME);

afterMouseCoordinates:
    event->payload.mouse.modifiers = MESSAGE_MODIFIER_NONE;
    if (event->type != MESSAGE_NONE) {
        event->payload.mouse.modifiers = gpInputManager->m_modifiers;
        gpInputManager->m_writeIndex++;
        gpInputManager->m_writeIndex %= INPUT_EVENT_RING_CAPACITY;
        if (gpInputManager->m_readIndex == gpInputManager->m_writeIndex) {
            gpInputManager->m_readIndex++;
            gpInputManager->m_readIndex %= INPUT_EVENT_RING_CAPACITY;
        }
    }

    gpInputManager->m_mouseMessageActive = 0;
    return event->type == MESSAGE_NONE;
}

// Buka 2.1 and PoL 2.0 both reset the two queue indices in this method.
// HoMM1's body confirms the same short fields at +0x230 and +0x232.
VA(0x0047c200, 0x11)
void inputManager::Flush(void)
{
    ResetEventQueue(this);
}

// The donor assigns the key-code mode and then flushes the event queue.
// HoMM1 inlines Flush here and stores the mode as a short at +0x340.
VA(0x0047c300, 0x1f)
void inputManager::SetKeyCodeType(short keyCodeType)
{
    m_keyCodeType = keyCodeType;
    ResetEventQueue(this);
}

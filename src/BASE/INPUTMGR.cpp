// Located from HoMM2 Buka 2.1; PoL 2.0 supplies the VC4 declaration.

#define WIN32_LEAN_AND_MEAN

#include <match.h>

#include <windows.h>

#include <BASE/heroWindowManager.h>
#include <BASE/inputManager.h>
#include <BASE/message.h>
#include <BASE/Misc.h>
#include <BASE/mouseManager.h>
#include <SOURCE/KB.h>
#include <SOURCE/kbwin.h>
#include <SOURCE/philAI.h>
#include <SOURCE/wingraph.h>

#include <io.h>
#include <string.h>

static inline void ResetEventQueue(inputManager* manager) {
    manager->m_writeIndex = 0;
    manager->m_readIndex = 0;
}

VA(0x0047bb40, 0x2e4)
int KeyboardMessageHandler(void*, unsigned int message, unsigned int, long messageData) {
    if (gpInputManager == NULL)
        return 1;
    if (gpInputManager->m_active != 1)
        return 1;

    tag_message* event = &gpInputManager->m_eventRing[gpInputManager->m_writeIndex];
    event->type = MESSAGE_NONE;
    event->modifiers = MESSAGE_MODIFIER_NONE;
    event->y = 0;
    event->keyCode = 0;

    switch (message) {
        case WM_KEYDOWN:
            event->type = MESSAGE_KEY_DOWN;
            event->keyCode = HIWORD(messageData) & INPUT_SCAN_CODE_MASK;
            event->y = 0;
            event->modifiers = MESSAGE_MODIFIER_NONE;
            switch (event->keyCode) {
                case INPUT_SCAN_CONTROL:
                    gpInputManager->m_modifiers |= MESSAGE_MODIFIER_CONTROL;
                    break;
                case INPUT_SCAN_LEFT_SHIFT:
                    gpInputManager->m_modifiers |= MESSAGE_MODIFIER_LEFT_SHIFT;
                    break;
                case INPUT_SCAN_RIGHT_SHIFT:
                    gpInputManager->m_modifiers |= MESSAGE_MODIFIER_RIGHT_SHIFT;
                    break;
                case INPUT_SCAN_ALT:
                    gpInputManager->m_modifiers |= MESSAGE_MODIFIER_ALT;
                    break;
            }
            break;
        case WM_KEYUP:
            event->type = MESSAGE_KEY_UP;
            event->keyCode = HIWORD(messageData) & INPUT_SCAN_CODE_MASK;
            event->y = 0;
            event->modifiers = MESSAGE_MODIFIER_NONE;
            switch (event->keyCode) {
                case INPUT_SCAN_CONTROL:
                    gpInputManager->m_modifiers &= ~MESSAGE_MODIFIER_CONTROL;
                    break;
                case INPUT_SCAN_LEFT_SHIFT:
                    gpInputManager->m_modifiers &= ~MESSAGE_MODIFIER_LEFT_SHIFT;
                    break;
                case INPUT_SCAN_RIGHT_SHIFT:
                    gpInputManager->m_modifiers &= ~MESSAGE_MODIFIER_RIGHT_SHIFT;
                    break;
                case INPUT_SCAN_ALT:
                    gpInputManager->m_modifiers &= ~MESSAGE_MODIFIER_ALT;
                    break;
            }
            break;
    }

    if (event->type != MESSAGE_NONE) {
        event->modifiers = gpInputManager->m_modifiers;
        gpInputManager->m_writeIndex++;
        gpInputManager->m_writeIndex %= INPUT_EVENT_RING_CAPACITY;
        if (gpInputManager->m_readIndex == gpInputManager->m_writeIndex) {
            gpInputManager->m_readIndex++;
            gpInputManager->m_readIndex %= INPUT_EVENT_RING_CAPACITY;
        }
        gpInputManager->m_field_0x342 = 0;
        if (gpWindowManager->m_active == 1) {
            if (event->type == MESSAGE_KEY_DOWN && event->keyCode == INPUT_SCAN_F12
                && (event->modifiers & MESSAGE_MODIFIER_SHIFT_KEYS))
                gpWindowManager->ScreenShot();
            if (event->type == MESSAGE_KEY_DOWN && event->keyCode == INPUT_SCAN_F1) {
                SetFullScreenStatus(FALSE);
                AppCommand(hwndApp, 0, KBWIN_MENU_HELP, 0);
            }
            if (event->type == MESSAGE_KEY_DOWN && event->keyCode == INPUT_SCAN_F4)
                SetFullScreenStatus(1 - gConfig.gfx[gCurExe].fullScreen);
        }
    }
    return event->type == MESSAGE_NONE;
}

// donor PoL RVA 0x000cde60; preferred Buka symbol ?MouseMessageHandler@@YIHPAXIIJ@Z
// donor Buka TU BASE/INPUTMGR; HoMM1 owner inferred from contiguous order
// evidence: graph:3;base=0.528083;margin=0.800438;shape=0.151;size=0.712;calls=0.800;strings=ReleaseCapture Failed;alternate=pol20:int MouseMessageHandler(void *, unsigned int, unsigned int, long int)@0x000cde60
VA(0x0047be30, 0x27c)
#line 137 "D:\\Heroes\\Base\\INPUTMGR.CPP"
int MouseMessageHandler(void*, unsigned int message, unsigned int, long messageData) {
    DATA(0x004a1a44)
    static char gLeftReleaseCaptureFailure[] = "ReleaseCapture Failed";
    DATA(0x004a1a5c)
    static char gRightReleaseCaptureFailure[] = "ReleaseCapture Failed";
    if (gpInputManager == NULL)
        return 1;
    if (gpInputManager->m_active != 1)
        return 1;
    if (gpInputManager->m_mouseMessageActive != 0)
        return 1;
    gpInputManager->m_mouseMessageActive = 1;

    tag_message* event = &gpInputManager->m_eventRing[gpInputManager->m_writeIndex];
    event->modifiers = MESSAGE_MODIFIER_NONE;
    event->y = 0;
    event->x = 0;
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
            if (ReleaseCapture() == FALSE)
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
            if (ReleaseCapture() == FALSE)
                LogStr(gRightReleaseCaptureFailure);
            goto mouseCoordinates;
        case WM_RBUTTONDBLCLK:
            event->type = MESSAGE_RIGHT_BUTTON_DOWN;
            goto mouseCoordinates;
        default:
            goto mouseMoveCursorCheck;
    }

mouseCoordinates:
#line 187
    ProcessAssert(gMainWinScreenHeight > 0 && iMainWinScreenWidth > 0, __FILE__, __LINE__);
    event->x = CLIENT_TO_GAME_X(LOWORD(messageData));
    event->y = CLIENT_TO_GAME_Y(HIWORD(messageData));

mouseMoveCursorCheck:
    if (message == WM_MOUSEMOVE && gpMouseManager != NULL) {
        if (event->x > INPUT_CURSOR_INTERIOR_X_MIN && event->x < INPUT_CURSOR_INTERIOR_X_MAX
            && event->y > INPUT_CURSOR_INTERIOR_Y_MIN && event->y < INPUT_CURSOR_INTERIOR_Y_MAX)
            gpMouseManager->SetPointer(INPUT_KEEP_CURRENT_MOUSE_FRAME);
    }

afterMouseCoordinates:
    event->modifiers = MESSAGE_MODIFIER_NONE;
    if (event->type != MESSAGE_NONE) {
        event->modifiers = gpInputManager->m_modifiers;
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

VA(0x0047c0b0, 0x64)
inputManager::inputManager(void) {
    m_active = 0;
    m_mouseMessageActive = 0;
    m_field_0x34a = 0;
    m_requestedPriority = 1;
    m_field_0x33c = 0;
    m_field_0x236 = 0;
    m_field_0x238 = 0;
    m_field_0x23a = 1;
    m_keyCodeType = INPUT_KEY_CODE_SCAN;
    m_recordFile = -1;
    m_field_0x34f = 0;
}

// @early-stop 99.75: the inline strcpy saves its length in edx where retail
// uses eax; nothing else differs. /O2 RA trace: the length is not a colouring
// node (priority, this, the 0/1 constants and the scan temporary colour as in
// retail). Probes: `return 1` moves it to ebp; an int priority, or swapping the
// two stores of 1, keeps edx. Retail's heroWindowManager::Open also uses edx,
// mouseManager's constructor eax.
VA(0x0047c120, 0x85)
short inputManager::Open(short priority) {
    memset(m_eventRing, 0, sizeof(m_eventRing));
    ResetEventQueue(this);
    m_requestedPriority = priority;
    m_modifiers = MESSAGE_MODIFIER_NONE;
    MakeScanCodeTable();
    m_messageMask = BASE_MANAGER_ACCEPT_MOUSE_MOVE;
    m_priority = BASE_MANAGER_PRIORITY_UNASSIGNED;
    m_field_0x23a = 1;
    m_active = 1;
    strcpy(m_name, "inputManager");
    return BASE_MANAGER_SUCCESS;
}

VA(0x0047c1b0, 0x3b)
void inputManager::Close(void) {
    if (m_active != 1)
        return;
    if (m_recordFile != -1)
        close(m_recordFile);
    ResetEventQueue(this);
    m_requestedPriority = 0;
    m_active = 0;
}

VA(0x0047c1f0, 0x6)
short inputManager::Main(tag_message&) {
    return 0;
}

// Buka 2.1 and PoL 2.0 both reset the two queue indices in this method.
// HoMM1's body confirms the same short fields at +0x230 and +0x232.
VA(0x0047c200, 0x11)
void inputManager::Flush(void) {
    ResetEventQueue(this);
}

VA(0x0047c220, 0xd1)
tag_message inputManager::GetEvent(void) {
    tag_message event;
    PollSound();
    if (gpInputManager->m_active != 1 || m_readIndex == m_writeIndex) {
        event.type = MESSAGE_NONE;
        event.id = 0;
        event.command = event.id;
        event.modifiers = event.command;
    } else {
        event = m_eventRing[m_readIndex];
        m_readIndex++;
        m_readIndex %= INPUT_EVENT_RING_CAPACITY;
        if (event.type == MESSAGE_KEY_DOWN && m_keyCodeType == INPUT_KEY_CODE_ASCII)
            AsciiConvert(event);
    }
    return event;
}

// The donor assigns the key-code mode and then flushes the event queue.
// HoMM1 inlines Flush here and stores the mode as a short at +0x340.
VA(0x0047c300, 0x1f)
void inputManager::SetKeyCodeType(short keyCodeType) {
    m_keyCodeType = keyCodeType;
    ResetEventQueue(this);
}

VA(0x0047c320, 0x1cb)
void inputManager::AsciiConvert(tag_message& event) {
    if ((event.keyCode >= INPUT_SCAN_FUNCTION_KEY_FIRST
         && event.keyCode <= INPUT_SCAN_FUNCTION_KEY_LAST)
        || event.keyCode == INPUT_SCAN_F11 || event.keyCode == INPUT_SCAN_F12)
        event.keyCode = m_keyState[event.keyCode];
    else
        event.keyCode = m_keyState[event.keyCode] & INPUT_SCAN_CODE_MASK;

    if ((event.modifiers & MESSAGE_MODIFIER_SHIFT_KEYS) == 0 && event.keyCode > 'A' - 1
        && event.keyCode < 'Z' + 1)
        event.keyCode += 'a' - 'A';

    if ((event.modifiers & MESSAGE_MODIFIER_SHIFT_KEYS) != 0) {
        switch (event.keyCode) {
            case '1':
                event.keyCode = '!';
                break;
            case '2':
                event.keyCode = '@';
                break;
            case '3':
                event.keyCode = '#';
                break;
            case '4':
                event.keyCode = '$';
                break;
            case '5':
                event.keyCode = '%';
                break;
            case '6':
                event.keyCode = '^';
                break;
            case '7':
                event.keyCode = '&';
                break;
            case '8':
                event.keyCode = '*';
                break;
            case '9':
                event.keyCode = '(';
                break;
            case '0':
                event.keyCode = ')';
                break;
            case '-':
                event.keyCode = '_';
                break;
            case '=':
                event.keyCode = '+';
                break;
            case '[':
                event.keyCode = '{';
                break;
            case ']':
                event.keyCode = '}';
                break;
            case '\\':
                event.keyCode = '|';
                break;
            case ';':
                event.keyCode = ':';
                break;
            case '\'':
                event.keyCode = '"';
                break;
            case ',':
                event.keyCode = '<';
                break;
            case '.':
                event.keyCode = '>';
                break;
            case '/':
                event.keyCode = '?';
                break;
        }
    }
}

VA(0x0047c4f0, 0x33c)
void inputManager::MakeScanCodeTable(void) {
    for (unsigned int scanCode = 0; scanCode < INPUT_SCAN_CODE_CAPACITY; scanCode++)
        m_keyState[scanCode] = scanCode << INPUT_KEY_SCAN_SHIFT;
    m_keyState[INPUT_SCAN_NONE] = 0;
    m_keyState[INPUT_SCAN_ESCAPE] = INPUT_ASCII_ESCAPE;
    m_keyState[INPUT_SCAN_1] = '1';
    m_keyState[INPUT_SCAN_2] = '2';
    m_keyState[INPUT_SCAN_3] = '3';
    m_keyState[INPUT_SCAN_4] = '4';
    m_keyState[INPUT_SCAN_5] = '5';
    m_keyState[INPUT_SCAN_6] = '6';
    m_keyState[INPUT_SCAN_7] = '7';
    m_keyState[INPUT_SCAN_8] = '8';
    m_keyState[INPUT_SCAN_9] = '9';
    m_keyState[INPUT_SCAN_0] = '0';
    m_keyState[INPUT_SCAN_MINUS] = '-';
    m_keyState[INPUT_SCAN_EQUALS] = '=';
    m_keyState[INPUT_SCAN_BACKSPACE] = INPUT_ASCII_DELETE;
    m_keyState[INPUT_SCAN_TAB] = '\t';
    m_keyState[INPUT_SCAN_Q] = 'Q';
    m_keyState[INPUT_SCAN_W] = 'W';
    m_keyState[INPUT_SCAN_E] = 'E';
    m_keyState[INPUT_SCAN_R] = 'R';
    m_keyState[INPUT_SCAN_T] = 'T';
    m_keyState[INPUT_SCAN_Y] = 'Y';
    m_keyState[INPUT_SCAN_U] = 'U';
    m_keyState[INPUT_SCAN_I] = 'I';
    m_keyState[INPUT_SCAN_O] = 'O';
    m_keyState[INPUT_SCAN_P] = 'P';
    m_keyState[INPUT_SCAN_LEFT_BRACKET] = '[';
    m_keyState[INPUT_SCAN_RIGHT_BRACKET] = ']';
    m_keyState[INPUT_SCAN_ENTER] = '\n';
    m_keyState[INPUT_SCAN_CONTROL] = INPUT_SCAN_CONTROL << INPUT_KEY_SCAN_SHIFT;
    m_keyState[INPUT_SCAN_A] = 'A';
    m_keyState[INPUT_SCAN_S] = 'S';
    m_keyState[INPUT_SCAN_D] = 'D';
    m_keyState[INPUT_SCAN_F] = 'F';
    m_keyState[INPUT_SCAN_G] = 'G';
    m_keyState[INPUT_SCAN_H] = 'H';
    m_keyState[INPUT_SCAN_J] = 'J';
    m_keyState[INPUT_SCAN_K] = 'K';
    m_keyState[INPUT_SCAN_L] = 'L';
    m_keyState[INPUT_SCAN_SEMICOLON] = '\'';
    m_keyState[INPUT_SCAN_APOSTROPHE] = '\'';
    m_keyState[INPUT_SCAN_GRAVE] = INPUT_SCAN_GRAVE << INPUT_KEY_SCAN_SHIFT;
    m_keyState[INPUT_SCAN_LEFT_SHIFT] = INPUT_SCAN_LEFT_SHIFT << INPUT_KEY_SCAN_SHIFT;
    m_keyState[INPUT_SCAN_BACKSLASH] = '\\';
    m_keyState[INPUT_SCAN_Z] = 'Z';
    m_keyState[INPUT_SCAN_X] = 'X';
    m_keyState[INPUT_SCAN_C] = 'C';
    m_keyState[INPUT_SCAN_V] = 'V';
    m_keyState[INPUT_SCAN_B] = 'B';
    m_keyState[INPUT_SCAN_N] = 'N';
    m_keyState[INPUT_SCAN_M] = 'M';
    m_keyState[INPUT_SCAN_COMMA] = ',';
    m_keyState[INPUT_SCAN_PERIOD] = '.';
    m_keyState[INPUT_SCAN_SLASH] = '/';
    m_keyState[INPUT_SCAN_RIGHT_SHIFT] = INPUT_SCAN_RIGHT_SHIFT << INPUT_KEY_SCAN_SHIFT;
    m_keyState[INPUT_SCAN_NUMPAD_MULTIPLY] = '*';
    m_keyState[INPUT_SCAN_ALT] = INPUT_SCAN_ALT << INPUT_KEY_SCAN_SHIFT;
    m_keyState[INPUT_SCAN_SPACE] = ' ';
    m_keyState[INPUT_SCAN_CAPS_LOCK] = INPUT_SCAN_CAPS_LOCK << INPUT_KEY_SCAN_SHIFT;
    m_keyState[INPUT_SCAN_F1] = INPUT_SCAN_F1 << INPUT_KEY_SCAN_SHIFT;
    m_keyState[INPUT_SCAN_F2] = INPUT_SCAN_F2 << INPUT_KEY_SCAN_SHIFT;
    m_keyState[INPUT_SCAN_F3] = INPUT_SCAN_F3 << INPUT_KEY_SCAN_SHIFT;
    m_keyState[INPUT_SCAN_F4] = INPUT_SCAN_F4 << INPUT_KEY_SCAN_SHIFT;
    m_keyState[INPUT_SCAN_F5] = INPUT_SCAN_F5 << INPUT_KEY_SCAN_SHIFT;
    m_keyState[INPUT_SCAN_F6] = INPUT_SCAN_F6 << INPUT_KEY_SCAN_SHIFT;
    m_keyState[INPUT_SCAN_F7] = INPUT_SCAN_F7 << INPUT_KEY_SCAN_SHIFT;
    m_keyState[INPUT_SCAN_F8] = INPUT_SCAN_F8 << INPUT_KEY_SCAN_SHIFT;
    m_keyState[INPUT_SCAN_F9] = INPUT_SCAN_F9 << INPUT_KEY_SCAN_SHIFT;
    m_keyState[INPUT_SCAN_F10] = INPUT_SCAN_F10 << INPUT_KEY_SCAN_SHIFT;
    m_keyState[INPUT_SCAN_NUM_LOCK] = INPUT_SCAN_NUM_LOCK << INPUT_KEY_SCAN_SHIFT;
    m_keyState[INPUT_SCAN_SCROLL_LOCK] = INPUT_SCAN_SCROLL_LOCK << INPUT_KEY_SCAN_SHIFT;
    m_keyState[INPUT_SCAN_NUMPAD_7] = INPUT_SCAN_NUMPAD_7 << INPUT_KEY_SCAN_SHIFT;
    m_keyState[INPUT_SCAN_NUMPAD_8] = INPUT_SCAN_NUMPAD_8 << INPUT_KEY_SCAN_SHIFT;
    m_keyState[INPUT_SCAN_NUMPAD_9] = INPUT_SCAN_NUMPAD_9 << INPUT_KEY_SCAN_SHIFT;
    m_keyState[INPUT_SCAN_NUMPAD_MINUS] = '-';
    m_keyState[INPUT_SCAN_NUMPAD_4] = INPUT_SCAN_NUMPAD_4 << INPUT_KEY_SCAN_SHIFT;
    m_keyState[INPUT_SCAN_NUMPAD_5] = INPUT_SCAN_NUMPAD_5 << INPUT_KEY_SCAN_SHIFT;
    m_keyState[INPUT_SCAN_NUMPAD_6] = INPUT_SCAN_NUMPAD_6 << INPUT_KEY_SCAN_SHIFT;
    m_keyState[INPUT_SCAN_NUMPAD_PLUS] = '+';
    m_keyState[INPUT_SCAN_NUMPAD_1] = INPUT_SCAN_NUMPAD_1 << INPUT_KEY_SCAN_SHIFT;
    m_keyState[INPUT_SCAN_NUMPAD_2] = INPUT_SCAN_NUMPAD_2 << INPUT_KEY_SCAN_SHIFT;
    m_keyState[INPUT_SCAN_NUMPAD_3] = INPUT_SCAN_NUMPAD_3 << INPUT_KEY_SCAN_SHIFT;
    m_keyState[INPUT_SCAN_NUMPAD_0] = INPUT_SCAN_NUMPAD_0 << INPUT_KEY_SCAN_SHIFT;
    m_keyState[INPUT_SCAN_NUMPAD_DELETE] = INPUT_SCAN_NUMPAD_DELETE << INPUT_KEY_SCAN_SHIFT;
    m_keyState[INPUT_SCAN_SYSREQ] = INPUT_SCAN_SYSREQ << INPUT_KEY_SCAN_SHIFT;
    m_keyState[INPUT_SCAN_RESERVED_55] = INPUT_SCAN_RESERVED_55 << INPUT_KEY_SCAN_SHIFT;
    m_keyState[INPUT_SCAN_ISO_BACKSLASH] = INPUT_SCAN_ISO_BACKSLASH << INPUT_KEY_SCAN_SHIFT;
    m_keyState[INPUT_SCAN_F11] = INPUT_SCAN_F11 << INPUT_KEY_SCAN_SHIFT;
    m_keyState[INPUT_SCAN_F12] = INPUT_SCAN_F12 << INPUT_KEY_SCAN_SHIFT;
}

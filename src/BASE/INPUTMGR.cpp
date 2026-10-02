// Located from HoMM2 Buka 2.1; PoL 2.0 supplies the VC4 declaration.

#define WIN32_LEAN_AND_MEAN

#include <match.h>

#include <windows.h>

#include <BASE/INPUTMGR_TYPES.h>
#include <BASE/Misc.h>
#include <BASE/MISC_TYPES.h>
#include <BASE/mouseManager.h>
#include <H1/All.h>
#include <H1/KB.h>
#include <SOURCE/kbwin.h>
#include <SOURCE/wingraph.h>

#include <io.h>
#include <string.h>

#pragma intrinsic(memset, strcpy)

short gInputManagerAssertLine = 137;
char gLeftReleaseCaptureFailure[] = "ReleaseCapture Failed";
char gRightReleaseCaptureFailure[] = "ReleaseCapture Failed";
char gInputManagerAssertFile[] = "D:\\Heroes\\Base\\INPUTMGR.CPP";

static inline void ResetEventQueue(inputManager* manager) {
    manager->m_writeIndex = 0;
    manager->m_readIndex = 0;
}

H1_ENUM_BEGIN(InputScanCode)
    INPUT_SCAN_CONTROL = 0x1d,
    INPUT_SCAN_LEFT_SHIFT = 0x2a,
    INPUT_SCAN_RIGHT_SHIFT = 0x36,
    INPUT_SCAN_ALT = 0x38,
    INPUT_SCAN_F1 = 0x3b,
    INPUT_SCAN_F4 = 0x3e,
    INPUT_SCAN_F10 = 0x44,
    INPUT_SCAN_F11 = 0x57,
    INPUT_SCAN_F12 = 0x58,
    INPUT_SCAN_CODE_MASK = 0xff
H1_ENUM_END(InputScanCode)

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
            if (event->type == MESSAGE_KEY_DOWN
                && event->keyCode == INPUT_SCAN_F1) {
                SetFullScreenStatus(0);
                AppCommand(hwndApp, 0, KBWIN_MENU_HELP, 0);
            }
            if (event->type == MESSAGE_KEY_DOWN && event->keyCode == INPUT_SCAN_F4)
                SetFullScreenStatus(1 - gConfig.gfx[giCurExe].fullScreen);
        }
    }
    return event->type == MESSAGE_NONE;
}

// donor PoL RVA 0x000cde60; preferred Buka symbol ?MouseMessageHandler@@YIHPAXIIJ@Z
// donor Buka TU BASE/INPUTMGR; HoMM1 owner inferred from contiguous order
// evidence: graph:3;base=0.528083;margin=0.800438;shape=0.151;size=0.712;calls=0.800;strings=ReleaseCapture Failed;alternate=pol20:int MouseMessageHandler(void *, unsigned int, unsigned int, long int)@0x000cde60
VA(0x0047be30, 0x27c)
int MouseMessageHandler(void*, unsigned int message, unsigned int, long messageData) {
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
            goto mouseMoveCursorCheck;
    }

mouseCoordinates:
    ProcessAssert(
        iMainWinScreenHeight > 0 && iMainWinScreenWidth > 0,
        gInputManagerAssertFile,
        gInputManagerAssertLine + 50
    );
    event->x = LOWORD(messageData) * INPUT_GAME_WIDTH / iMainWinScreenWidth;
    event->y = HIWORD(messageData) * INPUT_GAME_HEIGHT / iMainWinScreenHeight;

mouseMoveCursorCheck:
    if (message == WM_MOUSEMOVE && gpMouseManager != NULL) {
        if (event->x > INPUT_CURSOR_INTERIOR_X_MIN
            && event->x < INPUT_CURSOR_INTERIOR_X_MAX
            && event->y > INPUT_CURSOR_INTERIOR_Y_MIN
            && event->y < INPUT_CURSOR_INTERIOR_Y_MAX)
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

VA(0x0047c120, 0x85)
short inputManager::Open(short priority) {
    memset(m_eventRing, 0, sizeof(m_eventRing));
    ResetEventQueue(this);
    m_requestedPriority = priority;
    m_modifiers = MESSAGE_MODIFIER_NONE;
    MakeScanCodeTable();
    m_messageMask = BASE_MANAGER_ACCEPT_MOUSE_MOVE;
    m_priority = -1;
    m_field_0x23a = 1;
    m_active = 1;
    strcpy(m_name, "inputManager");
    return 0;
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
    if ((event.keyCode >= INPUT_SCAN_F1
         && event.keyCode <= INPUT_SCAN_F10)
        || event.keyCode == INPUT_SCAN_F11
        || event.keyCode == INPUT_SCAN_F12)
        event.keyCode = m_keyState[event.keyCode];
    else
        event.keyCode =
            m_keyState[event.keyCode] & INPUT_SCAN_CODE_MASK;

    if ((event.modifiers & MESSAGE_MODIFIER_SHIFT_KEYS) == 0
        && event.keyCode > 'A' - 1 && event.keyCode < 'Z' + 1)
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
        m_keyState[scanCode] = scanCode << 8;
    m_keyState[0] = 0;
    m_keyState[1] = 0x1b;
    m_keyState[2] = '1';
    m_keyState[3] = '2';
    m_keyState[4] = '3';
    m_keyState[5] = '4';
    m_keyState[6] = '5';
    m_keyState[7] = '6';
    m_keyState[8] = '7';
    m_keyState[9] = '8';
    m_keyState[10] = '9';
    m_keyState[11] = '0';
    m_keyState[12] = '-';
    m_keyState[13] = '=';
    m_keyState[14] = 0x7f;
    m_keyState[15] = '\t';
    m_keyState[16] = 'Q';
    m_keyState[17] = 'W';
    m_keyState[18] = 'E';
    m_keyState[19] = 'R';
    m_keyState[20] = 'T';
    m_keyState[21] = 'Y';
    m_keyState[22] = 'U';
    m_keyState[23] = 'I';
    m_keyState[24] = 'O';
    m_keyState[25] = 'P';
    m_keyState[26] = '[';
    m_keyState[27] = ']';
    m_keyState[28] = '\n';
    m_keyState[29] = 29 << 8;
    m_keyState[30] = 'A';
    m_keyState[31] = 'S';
    m_keyState[32] = 'D';
    m_keyState[33] = 'F';
    m_keyState[34] = 'G';
    m_keyState[35] = 'H';
    m_keyState[36] = 'J';
    m_keyState[37] = 'K';
    m_keyState[38] = 'L';
    m_keyState[39] = '\'';
    m_keyState[40] = '\'';
    m_keyState[41] = 41 << 8;
    m_keyState[42] = 42 << 8;
    m_keyState[43] = '\\';
    m_keyState[44] = 'Z';
    m_keyState[45] = 'X';
    m_keyState[46] = 'C';
    m_keyState[47] = 'V';
    m_keyState[48] = 'B';
    m_keyState[49] = 'N';
    m_keyState[50] = 'M';
    m_keyState[51] = ',';
    m_keyState[52] = '.';
    m_keyState[53] = '/';
    m_keyState[54] = 54 << 8;
    m_keyState[55] = '*';
    m_keyState[56] = 56 << 8;
    m_keyState[57] = ' ';
    m_keyState[58] = 58 << 8;
    m_keyState[59] = 59 << 8;
    m_keyState[60] = 60 << 8;
    m_keyState[61] = 61 << 8;
    m_keyState[62] = 62 << 8;
    m_keyState[63] = 63 << 8;
    m_keyState[64] = 64 << 8;
    m_keyState[65] = 65 << 8;
    m_keyState[66] = 66 << 8;
    m_keyState[67] = 67 << 8;
    m_keyState[68] = 68 << 8;
    m_keyState[69] = 69 << 8;
    m_keyState[70] = 70 << 8;
    m_keyState[71] = 71 << 8;
    m_keyState[72] = 72 << 8;
    m_keyState[73] = 73 << 8;
    m_keyState[74] = '-';
    m_keyState[75] = 75 << 8;
    m_keyState[76] = 76 << 8;
    m_keyState[77] = 77 << 8;
    m_keyState[78] = '+';
    m_keyState[79] = 79 << 8;
    m_keyState[80] = 80 << 8;
    m_keyState[81] = 81 << 8;
    m_keyState[82] = 82 << 8;
    m_keyState[83] = 83 << 8;
    m_keyState[84] = 84 << 8;
    m_keyState[85] = 85 << 8;
    m_keyState[86] = 86 << 8;
    m_keyState[87] = 87 << 8;
    m_keyState[88] = 88 << 8;
}

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

// Retail assertion paths: each program's BASE objects were compiled in its own
// checkout (HEROES.EXE and EDITOR.EXE assertion strings).
#ifdef HOMM1_EDITOR
#define INPUTMGR_CPP_PATH "U:\\HMM\\VSS\\HMM1\\Source\\Base\\INPUTMGR.CPP"
#else
#define INPUTMGR_CPP_PATH "E:\\Users\\igorl\\VSS\\HMM\\HMM1\\Source\\Base\\INPUTMGR.CPP"
#endif

// The character each key types, by its US-layout character: the selected
// language's keyboard table.
DATA(0x004a1388)
static u8 gInputCharacterMapCp1251[0x80] = localization::Chars("locale.keyboard");

static inline void ResetEventQueue(inputManager* manager) {
    manager->m_writeIndex = 0;
    manager->m_readIndex = 0;
}

VA(0x0046e560, 0x464)
i32 KeyboardMessageHandler(void*, u32 message, u32 virtualKey, i32 messageData) {
    if (gInputManager == NULL)
        return 1;
    if (gInputManager->m_active != 1)
        return 1;

    tag_message* event = &gInputManager->m_eventRing[gInputManager->m_writeIndex];
    event->type = MESSAGE_NONE;
    event->modifiers = MESSAGE_MODIFIER_NONE;
    event->y = 0;
    event->keyCode = 0;

    switch (message) {
        case WM_KEYDOWN:
            event->type = MESSAGE_KEY_DOWN;
            if (virtualKey == VK_RETURN)
                event->keyCode = INPUT_SCAN_ENTER;
            else
                event->keyCode = HIWORD(messageData) & INPUT_SCAN_CODE_MASK;
            event->y = 0;
            event->modifiers = MESSAGE_MODIFIER_NONE;
            switch (event->keyCode) {
                case INPUT_SCAN_CONTROL:
                    gInputManager->m_modifiers |= MESSAGE_MODIFIER_CONTROL;
                    break;
                case INPUT_SCAN_ALT:
                    gInputManager->m_modifiers |= MESSAGE_MODIFIER_ALT;
                    break;
                case INPUT_SCAN_LEFT_SHIFT:
                    gInputManager->m_modifiers |= MESSAGE_MODIFIER_LEFT_SHIFT;
                    break;
                case INPUT_SCAN_RIGHT_SHIFT:
                    gInputManager->m_modifiers |= MESSAGE_MODIFIER_RIGHT_SHIFT;
                    break;
            }
            break;
        case WM_KEYUP:
            event->type = MESSAGE_KEY_UP;
            if (virtualKey == VK_RETURN)
                event->keyCode = INPUT_SCAN_ENTER;
            else
                event->keyCode = HIWORD(messageData) & INPUT_SCAN_CODE_MASK;
            event->y = 0;
            event->modifiers = MESSAGE_MODIFIER_NONE;
            switch (event->keyCode) {
                case INPUT_SCAN_CONTROL:
                    gInputManager->m_modifiers &= ~MESSAGE_MODIFIER_CONTROL;
                    break;
                case INPUT_SCAN_ALT:
                    gInputManager->m_modifiers &= ~MESSAGE_MODIFIER_ALT;
                    break;
                case INPUT_SCAN_LEFT_SHIFT:
                    gInputManager->m_modifiers &= ~MESSAGE_MODIFIER_LEFT_SHIFT;
                    break;
                case INPUT_SCAN_RIGHT_SHIFT:
                    gInputManager->m_modifiers &= ~MESSAGE_MODIFIER_RIGHT_SHIFT;
                    break;
            }
            break;
    }

    if (event->type != MESSAGE_NONE) {
        event->modifiers = gInputManager->m_modifiers;
        gInputManager->m_writeIndex++;
        gInputManager->m_writeIndex %= INPUT_EVENT_RING_CAPACITY;
        if (gInputManager->m_readIndex == gInputManager->m_writeIndex) {
            gInputManager->m_readIndex++;
            gInputManager->m_readIndex %= INPUT_EVENT_RING_CAPACITY;
        }
        gInputManager->m_field_0x342 = 0;
        if (gWindowManager->m_active == 1) {
            if (event->type == MESSAGE_KEY_DOWN && event->keyCode == INPUT_SCAN_F12
                && (event->modifiers & MESSAGE_MODIFIER_SHIFT_KEYS))
                gWindowManager->ScreenShot();
            if (event->type == MESSAGE_KEY_DOWN && event->keyCode == INPUT_SCAN_F1) {
                SetFullScreenStatus(FALSE);
                AppCommand(gAppWindow, 0, KBWIN_MENU_HELP, 0);
            }
            if (event->type == MESSAGE_KEY_DOWN && event->keyCode == INPUT_SCAN_F4)
                SetFullScreenStatus(1 - CURRENT_GRAPHICS_CONFIG.fullScreen);
        }
    }
    return event->type == MESSAGE_NONE;
}

VA(0x0046e9c4, 0x33a)
#line 137 INPUTMGR_CPP_PATH
i32 MouseMessageHandler(void*, u32 message, u32, i32 messageData) {
    if (gInputManager == NULL)
        return 1;
    if (gInputManager->m_active != 1)
        return 1;
    if (gInputManager->m_mouseMessageActive != 0)
        return 1;
    gInputManager->m_mouseMessageActive = 1;

    i32 captureReleased;
    tag_message* event = &gInputManager->m_eventRing[gInputManager->m_writeIndex];
    event->modifiers = MESSAGE_MODIFIER_NONE;
    event->y = 0;
    event->x = 0;
    event->type = MESSAGE_NONE;

    switch (message) {
        case WM_MOUSEMOVE:
            event->type = MESSAGE_MOUSE_MOVE;
            goto mouseCoordinates;
        case WM_LBUTTONDBLCLK:
            event->type = MESSAGE_LEFT_BUTTON_DOWN;
            goto mouseCoordinates;
        case WM_LBUTTONDOWN:
            event->type = MESSAGE_LEFT_BUTTON_DOWN;
            SetCapture(gAppWindow);
            goto mouseCoordinates;
        case WM_RBUTTONDOWN:
            event->type = MESSAGE_RIGHT_BUTTON_DOWN;
            SetCapture(gAppWindow);
            goto mouseCoordinates;
        case WM_RBUTTONDBLCLK:
            event->type = MESSAGE_RIGHT_BUTTON_DOWN;
            goto mouseCoordinates;
        case WM_LBUTTONUP:
            event->type = MESSAGE_LEFT_BUTTON_UP;
            captureReleased = ReleaseCapture();
            goto mouseCoordinates;
        case WM_RBUTTONUP:
            event->type = MESSAGE_RIGHT_BUTTON_UP;
            captureReleased = ReleaseCapture();

        mouseCoordinates:
#line 191
            H1_ASSERT(gMainWinScreenHeight > 0 && gMainWinScreenWidth > 0);
            event->x = CLIENT_TO_GAME_X(LOWORD(messageData));
            event->y = CLIENT_TO_GAME_Y(HIWORD(messageData));
    }

mouseMoveCursorCheck:
    if (message == WM_MOUSEMOVE && gMouseManager != NULL) {
        if (event->x > INPUT_CURSOR_INTERIOR_X_MIN && event->x < INPUT_CURSOR_INTERIOR_X_MAX
            && event->y > INPUT_CURSOR_INTERIOR_Y_MIN && event->y < INPUT_CURSOR_INTERIOR_Y_MAX)
            gMouseManager->SetPointer(MOUSE_KEEP_CURRENT_FRAME);
    }

afterMouseCoordinates:
    event->modifiers = MESSAGE_MODIFIER_NONE;
    if (event->type != MESSAGE_NONE) {
        event->modifiers = gInputManager->m_modifiers;
        gInputManager->m_writeIndex++;
        gInputManager->m_writeIndex %= INPUT_EVENT_RING_CAPACITY;
        if (gInputManager->m_readIndex == gInputManager->m_writeIndex) {
            gInputManager->m_readIndex++;
            gInputManager->m_readIndex %= INPUT_EVENT_RING_CAPACITY;
        }
    }

    gInputManager->m_mouseMessageActive = 0;
    return event->type == MESSAGE_NONE;
}

VA(0x0046ecfe, 0xa0)
inputManager::inputManager(void) {
    m_active = 0;
    m_field_0x34a = 0;
    m_mouseMessageActive = 0;
    m_requestedPriority = 1;
    m_field_0x33c = 0;
    m_field_0x236 = 0;
    m_field_0x238 = 0;
    m_field_0x23a = 1;
    m_keyCodeType = INPUT_KEY_CODE_SCAN;
    m_recordFile = -1;
    m_field_0x34f = 0;
}

VA(0x0046ed9e, 0xa4)
H1_ENUM_RETURN(BaseManagerStatus, i16) inputManager::Open(i16 priority) {
    i16 positiveOption = 1;
    memset(m_eventRing, 0, sizeof(m_eventRing));
    ResetEventQueue(this);
    m_requestedPriority = priority;
    m_modifiers = MESSAGE_MODIFIER_NONE;
    MakeScanCodeTable();
    SetPositiveOption(positiveOption);
    m_messageMask = BASE_MANAGER_ACCEPT_MOUSE_MOVE;
    m_priority = BASE_MANAGER_PRIORITY_UNASSIGNED;
    m_active = 1;
    strcpy(m_name, "inputManager");
    return BASE_MANAGER_SUCCESS;
}

VA(0x0046ee42, 0x64)
void inputManager::Close(void) {
    if (m_active != 1)
        return;
    if (m_recordFile != -1)
        close(m_recordFile);
    ResetEventQueue(this);
    m_requestedPriority = 0;
    m_active = 0;
}

VA(0x0046eea6, 0x10)
H1_ENUM_RETURN(MessageDispatchResult, i16) inputManager::Main(tag_message&) {
    return MESSAGE_DISPATCH_CONTINUE;
}

VA(0x0046eeb6, 0x23)
void inputManager::Flush(void) {
    ResetEventQueue(this);
}

VA(0x0046eed9, 0xfe)
tag_message inputManager::GetEvent(void) {
    tag_message event;
    PollSound();
    if (gInputManager->m_active != 1 || m_readIndex == m_writeIndex) {
        event.type = MESSAGE_NONE;
        event.id = 0;
        event.command = H1_ENUM_DECODE(BaseWidgetCommand, event.id);
        event.modifiers =
            H1_ENUM_DECODE(MessageModifier, H1_ENUM_ENCODE(BaseWidgetCommand, event.command));
    } else {
        event = m_eventRing[m_readIndex];
        m_readIndex++;
        m_readIndex %= INPUT_EVENT_RING_CAPACITY;
        if (event.type == MESSAGE_KEY_DOWN && m_keyCodeType == INPUT_KEY_CODE_ASCII)
            AsciiConvert(event);
    }
    return event;
}

// Descriptive name: the 0/1 counterpart of SetPositiveOption; retail's
// original method name is not available.
// @dead-code
// Zero-ref: no incoming call, jump or relocated reference in retail.
VA(0x0046efd7, 0x31)
void inputManager::SetBooleanOption(i16 enabled) {
    tag_message unusedMessage;

    if (enabled)
        m_field_0x238 = 1;
    else
        m_field_0x238 = 0;
}

// Descriptive name: retail's original method name is not available.
VA(0x0046f008, 0x31)
void inputManager::SetPositiveOption(i16 value) {
    if (value > 0)
        m_field_0x23a = value;
    else
        m_field_0x23a = 1;
}

// @dead-code
// Zero-ref: no incoming call, jump or relocated reference in retail.
VA(0x0046f039, 0x29)
void inputManager::SetMouseCoords(i16 x, i16 y) {
    m_mouseX = x;
    m_mouseY = y;
}

VA(0x0046f062, 0x23)
void inputManager::SetKeyCodeType(i16 keyCodeType) {
    m_keyCodeType = keyCodeType;
    Flush();
}

VA(0x0046f085, 0x34)
void TranslateInputCharacterCp1251(tag_message& event) {
    if (event.keyCode >= 0 && event.keyCode < static_cast<i32>(sizeof(gInputCharacterMapCp1251)))
        event.keyCode = static_cast<i8>(gInputCharacterMapCp1251[event.keyCode]);
}

VA(0x0046f0b9, 0x312)
void inputManager::AsciiConvert(tag_message& event) {
    if ((event.keyCode >= INPUT_SCAN_FUNCTION_KEY_FIRST
         && event.keyCode <= INPUT_SCAN_FUNCTION_KEY_LAST)
        || event.keyCode == INPUT_SCAN_F11 || event.keyCode == INPUT_SCAN_F12)
        event.keyCode = m_keyState[event.keyCode];
    else
        event.keyCode = m_keyState[event.keyCode] & INPUT_SCAN_CODE_MASK;

    if ((event.modifiers & MESSAGE_MODIFIER_SHIFT_KEYS) == MESSAGE_MODIFIER_NONE
        && event.keyCode > 'A' - 1 && event.keyCode < 'Z' + 1)
        event.keyCode = static_cast<u8>(CyrillicToLower(event.keyCode));

    if ((event.modifiers & MESSAGE_MODIFIER_SHIFT_KEYS) != MESSAGE_MODIFIER_NONE) {
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
    if ((event.modifiers & MESSAGE_MODIFIER_CONTROL_KEYS) == MESSAGE_MODIFIER_NONE)
        TranslateInputCharacterCp1251(event);
}

VA(0x0046f3cb, 0x46a)
void inputManager::MakeScanCodeTable(void) {
    for (u32 scanCode = 0; scanCode < INPUT_SCAN_CODE_CAPACITY; scanCode++)
        m_keyState[scanCode] = EncodeScanCode(scanCode);
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
    m_keyState[INPUT_SCAN_CONTROL] = EncodeScanCode(INPUT_SCAN_CONTROL);
    m_keyState[INPUT_SCAN_A] = 'A';
    m_keyState[INPUT_SCAN_S] = 'S';
    m_keyState[INPUT_SCAN_D] = 'D';
    m_keyState[INPUT_SCAN_F] = 'F';
    m_keyState[INPUT_SCAN_G] = 'G';
    m_keyState[INPUT_SCAN_H] = 'H';
    m_keyState[INPUT_SCAN_J] = 'J';
    m_keyState[INPUT_SCAN_K] = 'K';
    m_keyState[INPUT_SCAN_L] = 'L';
    m_keyState[INPUT_SCAN_SEMICOLON] = ';';
    m_keyState[INPUT_SCAN_APOSTROPHE] = '\'';
    m_keyState[INPUT_SCAN_GRAVE] = EncodeScanCode(INPUT_SCAN_GRAVE);
    m_keyState[INPUT_SCAN_LEFT_SHIFT] = EncodeScanCode(INPUT_SCAN_LEFT_SHIFT);
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
    m_keyState[INPUT_SCAN_RIGHT_SHIFT] = EncodeScanCode(INPUT_SCAN_RIGHT_SHIFT);
    m_keyState[INPUT_SCAN_NUMPAD_MULTIPLY] = '*';
    m_keyState[INPUT_SCAN_ALT] = EncodeScanCode(INPUT_SCAN_ALT);
    m_keyState[INPUT_SCAN_SPACE] = ' ';
    m_keyState[INPUT_SCAN_CAPS_LOCK] = EncodeScanCode(INPUT_SCAN_CAPS_LOCK);
    m_keyState[INPUT_SCAN_F1] = EncodeScanCode(INPUT_SCAN_F1);
    m_keyState[INPUT_SCAN_F2] = EncodeScanCode(INPUT_SCAN_F2);
    m_keyState[INPUT_SCAN_F3] = EncodeScanCode(INPUT_SCAN_F3);
    m_keyState[INPUT_SCAN_F4] = EncodeScanCode(INPUT_SCAN_F4);
    m_keyState[INPUT_SCAN_F5] = EncodeScanCode(INPUT_SCAN_F5);
    m_keyState[INPUT_SCAN_F6] = EncodeScanCode(INPUT_SCAN_F6);
    m_keyState[INPUT_SCAN_F7] = EncodeScanCode(INPUT_SCAN_F7);
    m_keyState[INPUT_SCAN_F8] = EncodeScanCode(INPUT_SCAN_F8);
    m_keyState[INPUT_SCAN_F9] = EncodeScanCode(INPUT_SCAN_F9);
    m_keyState[INPUT_SCAN_F10] = EncodeScanCode(INPUT_SCAN_F10);
    m_keyState[INPUT_SCAN_NUM_LOCK] = EncodeScanCode(INPUT_SCAN_NUM_LOCK);
    m_keyState[INPUT_SCAN_SCROLL_LOCK] = EncodeScanCode(INPUT_SCAN_SCROLL_LOCK);
    m_keyState[INPUT_SCAN_NUMPAD_7] = EncodeScanCode(INPUT_SCAN_NUMPAD_7);
    m_keyState[INPUT_SCAN_NUMPAD_8] = EncodeScanCode(INPUT_SCAN_NUMPAD_8);
    m_keyState[INPUT_SCAN_NUMPAD_9] = EncodeScanCode(INPUT_SCAN_NUMPAD_9);
    m_keyState[INPUT_SCAN_NUMPAD_MINUS] = '-';
    m_keyState[INPUT_SCAN_NUMPAD_4] = EncodeScanCode(INPUT_SCAN_NUMPAD_4);
    m_keyState[INPUT_SCAN_NUMPAD_5] = EncodeScanCode(INPUT_SCAN_NUMPAD_5);
    m_keyState[INPUT_SCAN_NUMPAD_6] = EncodeScanCode(INPUT_SCAN_NUMPAD_6);
    m_keyState[INPUT_SCAN_NUMPAD_PLUS] = '+';
    m_keyState[INPUT_SCAN_NUMPAD_1] = EncodeScanCode(INPUT_SCAN_NUMPAD_1);
    m_keyState[INPUT_SCAN_NUMPAD_2] = EncodeScanCode(INPUT_SCAN_NUMPAD_2);
    m_keyState[INPUT_SCAN_NUMPAD_3] = EncodeScanCode(INPUT_SCAN_NUMPAD_3);
    m_keyState[INPUT_SCAN_NUMPAD_0] = EncodeScanCode(INPUT_SCAN_NUMPAD_0);
    m_keyState[INPUT_SCAN_NUMPAD_DELETE] = EncodeScanCode(INPUT_SCAN_NUMPAD_DELETE);
    m_keyState[INPUT_SCAN_SYSREQ] = EncodeScanCode(INPUT_SCAN_SYSREQ);
    m_keyState[INPUT_SCAN_RESERVED_55] = EncodeScanCode(INPUT_SCAN_RESERVED_55);
    m_keyState[INPUT_SCAN_ISO_BACKSLASH] = EncodeScanCode(INPUT_SCAN_ISO_BACKSLASH);
    m_keyState[INPUT_SCAN_F11] = EncodeScanCode(INPUT_SCAN_F11);
    m_keyState[INPUT_SCAN_F12] = EncodeScanCode(INPUT_SCAN_F12);
}

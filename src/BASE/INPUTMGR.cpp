#include <H1/Ints.h>

#include <BASE/heroWindowManager.h>
#include <BASE/inputManager.h>
#include <BASE/message.h>
#include <BASE/Misc.h>
#include <BASE/mouseManager.h>
#include <SOURCE/KB.h>
#include <SOURCE/kbwin.h>
#include <SOURCE/philAI.h>
#include <SOURCE/wingraph.h>

#include <PLATFORM/File.h>

#include <string.h>

#ifdef HOMM1_EDITOR
#define INPUTMGR_CPP_PATH "U:\\HMM\\VSS\\HMM1\\Source\\Base\\INPUTMGR.CPP"
#else
#define INPUTMGR_CPP_PATH "E:\\Users\\igorl\\VSS\\HMM\\HMM1\\Source\\Base\\INPUTMGR.CPP"
#endif

static u8 gInputCharacterMap[0x80] = localization::Chars("locale.keyboard");

static inline void ResetEventQueue(inputManager* manager) {
    manager->m_writeIndex = 0;
    manager->m_readIndex = 0;
}

b32 KeyboardMessageHandler(void* window, u32 message, u32 virtualKey, i32 messageData) {
    if (gInputManager == NULL)
        return true;
    if (gInputManager->m_active != 1)
        return true;

    tag_message* event = &gInputManager->m_eventRing[gInputManager->m_writeIndex];
    event->type = MESSAGE_NONE;
    event->modifiers = MESSAGE_MODIFIER_NONE;
    event->y = 0;
    event->keyCode = INPUT_SCAN_NONE;

    switch (message) {
        case INPUT_MESSAGE_KEY_DOWN:
            event->type = MESSAGE_KEY_DOWN;
            if (virtualKey == INPUT_VIRTUAL_KEY_RETURN)
                event->keyCode = INPUT_SCAN_ENTER;
            else
                event->keyCode = INPUT_MESSAGE_SCAN_CODE(messageData);
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
        case INPUT_MESSAGE_KEY_UP:
            event->type = MESSAGE_KEY_UP;
            if (virtualKey == INPUT_VIRTUAL_KEY_RETURN)
                event->keyCode = INPUT_SCAN_ENTER;
            else
                event->keyCode = INPUT_MESSAGE_SCAN_CODE(messageData);
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
        gInputManager->m_keyPrefixPending = 0;
        if (gWindowManager->m_active == 1) {
            if (event->type == MESSAGE_KEY_DOWN && event->keyCode == INPUT_SCAN_F12
                && (event->modifiers & MESSAGE_MODIFIER_SHIFT_KEYS))
                gWindowManager->ScreenShot();
            if (event->type == MESSAGE_KEY_DOWN && event->keyCode == INPUT_SCAN_F1) {
                SetFullScreenStatus(0);
                AppMenuCommand(KBWIN_MENU_HELP);
            }
            if (event->type == MESSAGE_KEY_DOWN && event->keyCode == INPUT_SCAN_F4)
                SetFullScreenStatus(1 - CURRENT_GRAPHICS_CONFIG.fullScreen);
        }
    }
    return event->type == MESSAGE_NONE;
}

b32 MouseMessageHandler(void* window, u32 message, u32 keyFlags, i32 messageData) {
    if (gInputManager == NULL)
        return true;
    if (gInputManager->m_active != 1)
        return true;
    if (gInputManager->m_mouseMessageActive != 0)
        return true;
    gInputManager->m_mouseMessageActive = 1;

    tag_message* event = &gInputManager->m_eventRing[gInputManager->m_writeIndex];
    event->modifiers = MESSAGE_MODIFIER_NONE;
    event->y = 0;
    event->x = 0;
    event->type = MESSAGE_NONE;

    switch (message) {
        case INPUT_MESSAGE_MOUSE_MOVE:
            event->type = MESSAGE_MOUSE_MOVE;
            goto mouseCoordinates;
        case INPUT_MESSAGE_LEFT_DOUBLE:
            event->type = MESSAGE_LEFT_BUTTON_DOWN;
            goto mouseCoordinates;
        case INPUT_MESSAGE_LEFT_DOWN:
            event->type = MESSAGE_LEFT_BUTTON_DOWN;
            KBCaptureMouse();
            goto mouseCoordinates;
        case INPUT_MESSAGE_RIGHT_DOWN:
            event->type = MESSAGE_RIGHT_BUTTON_DOWN;
            KBCaptureMouse();
            goto mouseCoordinates;
        case INPUT_MESSAGE_RIGHT_DOUBLE:
            event->type = MESSAGE_RIGHT_BUTTON_DOWN;
            goto mouseCoordinates;
        case INPUT_MESSAGE_LEFT_UP:
            event->type = MESSAGE_LEFT_BUTTON_UP;
            KBReleaseMouse();
            goto mouseCoordinates;
        case INPUT_MESSAGE_RIGHT_UP:
            event->type = MESSAGE_RIGHT_BUTTON_UP;
            KBReleaseMouse();

        mouseCoordinates:
            H1_ASSERT(gMainWinScreenHeight > 0 && gMainWinScreenWidth > 0);
            event->x = CLIENT_TO_GAME_X(INPUT_MESSAGE_X(messageData));
            event->y = CLIENT_TO_GAME_Y(INPUT_MESSAGE_Y(messageData));
    }

    if (message == INPUT_MESSAGE_MOUSE_MOVE && gMouseManager != NULL) {
        if (event->x > INPUT_CURSOR_INTERIOR_X_MIN && event->x < INPUT_CURSOR_INTERIOR_X_MAX
            && event->y > INPUT_CURSOR_INTERIOR_Y_MIN && event->y < INPUT_CURSOR_INTERIOR_Y_MAX)
            gMouseManager->SetPointer(MOUSE_KEEP_CURRENT_FRAME);
    }

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

inputManager::inputManager(void) {
    m_active = 0;
    m_forceMouseMove = false;
    m_mouseMessageActive = 0;
    m_requestedPriority = 1;
    m_keyboardHookInstalled = 0;
    m_mouseDriverReady = 0;
    m_relativeMouse = 0;
    m_mouseSpeedDivisor = 1;
    m_keyCodeType = INPUT_KEY_CODE_SCAN;
    m_recordFile = FILE_DESCRIPTOR_INVALID;
    m_unused34f = 0;
}

i16 inputManager::Open(i16 priority) {
    i16 mouseSpeedDivisor = 1;
    memset(m_eventRing, 0, sizeof(m_eventRing));
    ResetEventQueue(this);
    m_requestedPriority = priority;
    m_modifiers = MESSAGE_MODIFIER_NONE;
    MakeScanCodeTable();
    SetMouseSpeedDivisor(mouseSpeedDivisor);
    m_messageMask = BASE_MANAGER_ACCEPT_MOUSE_MOVE;
    m_priority = BASE_MANAGER_PRIORITY_UNASSIGNED;
    m_active = 1;
    strcpy(m_name, "inputManager");
    return BASE_MANAGER_SUCCESS;
}

void inputManager::Close(void) {
    if (m_active != 1)
        return;
    if (m_recordFile != FILE_DESCRIPTOR_INVALID)
        FileClose(m_recordFile);
    ResetEventQueue(this);
    m_requestedPriority = 0;
    m_active = 0;
}

i16 inputManager::Main(tag_message& message) {
    return MESSAGE_DISPATCH_CONTINUE;
}

void inputManager::Flush(void) {
    ResetEventQueue(this);
}

tag_message inputManager::GetEvent(void) {
    tag_message event;
    PollSound();
    if (gInputManager->m_active != 1 || m_readIndex == m_writeIndex) {
        event.type = MESSAGE_NONE;
        event.id = 0;
        event.command = event.id;
        event.modifiers =
            event.command;
    } else {
        event = m_eventRing[m_readIndex];
        m_readIndex++;
        m_readIndex %= INPUT_EVENT_RING_CAPACITY;
        if (event.type == MESSAGE_KEY_DOWN && m_keyCodeType == INPUT_KEY_CODE_ASCII)
            AsciiConvert(event);
    }
    return event;
}

void inputManager::SetRelativeMouse(i16 relative) {
    tag_message unusedMessage;

    if (relative)
        m_relativeMouse = 1;
    else
        m_relativeMouse = 0;
}

void inputManager::SetMouseSpeedDivisor(i16 divisor) {
    if (divisor > 0)
        m_mouseSpeedDivisor = divisor;
    else
        m_mouseSpeedDivisor = 1;
}

void inputManager::SetMouseCoords(i16 x, i16 y) {
    m_mouseX = x;
    m_mouseY = y;
}

void inputManager::SetKeyCodeType(i16 keyCodeType) {
    m_keyCodeType = keyCodeType;
    Flush();
}

void TranslateInputCharacter(tag_message& event) {
    if (event.keyCode >= 0 && event.keyCode < static_cast<i32>(sizeof(gInputCharacterMap)))
        event.keyCode = static_cast<i8>(gInputCharacterMap[event.keyCode]);
}

void inputManager::AsciiConvert(tag_message& event) {
    if ((event.keyCode >= INPUT_SCAN_FUNCTION_KEY_FIRST
         && event.keyCode <= INPUT_SCAN_FUNCTION_KEY_LAST)
        || event.keyCode == INPUT_SCAN_F11 || event.keyCode == INPUT_SCAN_F12)
        event.keyCode = m_scanCodeTable[event.keyCode];
    else
        event.keyCode = m_scanCodeTable[event.keyCode] & INPUT_SCAN_CODE_MASK;

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
        TranslateInputCharacter(event);
}

void inputManager::MakeScanCodeTable(void) {
    for (u32 scanCode = 0; scanCode < INPUT_SCAN_CODE_CAPACITY; scanCode++)
        m_scanCodeTable[scanCode] = EncodeScanCode(scanCode);
    m_scanCodeTable[INPUT_SCAN_NONE] = '\0';
    m_scanCodeTable[INPUT_SCAN_ESCAPE] = INPUT_ASCII_ESCAPE;
    m_scanCodeTable[INPUT_SCAN_1] = '1';
    m_scanCodeTable[INPUT_SCAN_2] = '2';
    m_scanCodeTable[INPUT_SCAN_3] = '3';
    m_scanCodeTable[INPUT_SCAN_4] = '4';
    m_scanCodeTable[INPUT_SCAN_5] = '5';
    m_scanCodeTable[INPUT_SCAN_6] = '6';
    m_scanCodeTable[INPUT_SCAN_7] = '7';
    m_scanCodeTable[INPUT_SCAN_8] = '8';
    m_scanCodeTable[INPUT_SCAN_9] = '9';
    m_scanCodeTable[INPUT_SCAN_0] = '0';
    m_scanCodeTable[INPUT_SCAN_MINUS] = '-';
    m_scanCodeTable[INPUT_SCAN_EQUALS] = '=';
    m_scanCodeTable[INPUT_SCAN_BACKSPACE] = INPUT_ASCII_DELETE;
    m_scanCodeTable[INPUT_SCAN_TAB] = '\t';
    m_scanCodeTable[INPUT_SCAN_Q] = 'Q';
    m_scanCodeTable[INPUT_SCAN_W] = 'W';
    m_scanCodeTable[INPUT_SCAN_E] = 'E';
    m_scanCodeTable[INPUT_SCAN_R] = 'R';
    m_scanCodeTable[INPUT_SCAN_T] = 'T';
    m_scanCodeTable[INPUT_SCAN_Y] = 'Y';
    m_scanCodeTable[INPUT_SCAN_U] = 'U';
    m_scanCodeTable[INPUT_SCAN_I] = 'I';
    m_scanCodeTable[INPUT_SCAN_O] = 'O';
    m_scanCodeTable[INPUT_SCAN_P] = 'P';
    m_scanCodeTable[INPUT_SCAN_LEFT_BRACKET] = '[';
    m_scanCodeTable[INPUT_SCAN_RIGHT_BRACKET] = ']';
    m_scanCodeTable[INPUT_SCAN_ENTER] = '\n';
    m_scanCodeTable[INPUT_SCAN_CONTROL] = EncodeScanCode(INPUT_SCAN_CONTROL);
    m_scanCodeTable[INPUT_SCAN_A] = 'A';
    m_scanCodeTable[INPUT_SCAN_S] = 'S';
    m_scanCodeTable[INPUT_SCAN_D] = 'D';
    m_scanCodeTable[INPUT_SCAN_F] = 'F';
    m_scanCodeTable[INPUT_SCAN_G] = 'G';
    m_scanCodeTable[INPUT_SCAN_H] = 'H';
    m_scanCodeTable[INPUT_SCAN_J] = 'J';
    m_scanCodeTable[INPUT_SCAN_K] = 'K';
    m_scanCodeTable[INPUT_SCAN_L] = 'L';
    m_scanCodeTable[INPUT_SCAN_SEMICOLON] = ';';
    m_scanCodeTable[INPUT_SCAN_APOSTROPHE] = '\'';
    m_scanCodeTable[INPUT_SCAN_GRAVE] = EncodeScanCode(INPUT_SCAN_GRAVE);
    m_scanCodeTable[INPUT_SCAN_LEFT_SHIFT] = EncodeScanCode(INPUT_SCAN_LEFT_SHIFT);
    m_scanCodeTable[INPUT_SCAN_BACKSLASH] = '\\';
    m_scanCodeTable[INPUT_SCAN_Z] = 'Z';
    m_scanCodeTable[INPUT_SCAN_X] = 'X';
    m_scanCodeTable[INPUT_SCAN_C] = 'C';
    m_scanCodeTable[INPUT_SCAN_V] = 'V';
    m_scanCodeTable[INPUT_SCAN_B] = 'B';
    m_scanCodeTable[INPUT_SCAN_N] = 'N';
    m_scanCodeTable[INPUT_SCAN_M] = 'M';
    m_scanCodeTable[INPUT_SCAN_COMMA] = ',';
    m_scanCodeTable[INPUT_SCAN_PERIOD] = '.';
    m_scanCodeTable[INPUT_SCAN_SLASH] = '/';
    m_scanCodeTable[INPUT_SCAN_RIGHT_SHIFT] = EncodeScanCode(INPUT_SCAN_RIGHT_SHIFT);
    m_scanCodeTable[INPUT_SCAN_NUMPAD_MULTIPLY] = '*';
    m_scanCodeTable[INPUT_SCAN_ALT] = EncodeScanCode(INPUT_SCAN_ALT);
    m_scanCodeTable[INPUT_SCAN_SPACE] = ' ';
    m_scanCodeTable[INPUT_SCAN_CAPS_LOCK] = EncodeScanCode(INPUT_SCAN_CAPS_LOCK);
    m_scanCodeTable[INPUT_SCAN_F1] = EncodeScanCode(INPUT_SCAN_F1);
    m_scanCodeTable[INPUT_SCAN_F2] = EncodeScanCode(INPUT_SCAN_F2);
    m_scanCodeTable[INPUT_SCAN_F3] = EncodeScanCode(INPUT_SCAN_F3);
    m_scanCodeTable[INPUT_SCAN_F4] = EncodeScanCode(INPUT_SCAN_F4);
    m_scanCodeTable[INPUT_SCAN_F5] = EncodeScanCode(INPUT_SCAN_F5);
    m_scanCodeTable[INPUT_SCAN_F6] = EncodeScanCode(INPUT_SCAN_F6);
    m_scanCodeTable[INPUT_SCAN_F7] = EncodeScanCode(INPUT_SCAN_F7);
    m_scanCodeTable[INPUT_SCAN_F8] = EncodeScanCode(INPUT_SCAN_F8);
    m_scanCodeTable[INPUT_SCAN_F9] = EncodeScanCode(INPUT_SCAN_F9);
    m_scanCodeTable[INPUT_SCAN_F10] = EncodeScanCode(INPUT_SCAN_F10);
    m_scanCodeTable[INPUT_SCAN_NUM_LOCK] = EncodeScanCode(INPUT_SCAN_NUM_LOCK);
    m_scanCodeTable[INPUT_SCAN_SCROLL_LOCK] = EncodeScanCode(INPUT_SCAN_SCROLL_LOCK);
    m_scanCodeTable[INPUT_SCAN_NUMPAD_7] = EncodeScanCode(INPUT_SCAN_NUMPAD_7);
    m_scanCodeTable[INPUT_SCAN_NUMPAD_8] = EncodeScanCode(INPUT_SCAN_NUMPAD_8);
    m_scanCodeTable[INPUT_SCAN_NUMPAD_9] = EncodeScanCode(INPUT_SCAN_NUMPAD_9);
    m_scanCodeTable[INPUT_SCAN_NUMPAD_MINUS] = '-';
    m_scanCodeTable[INPUT_SCAN_NUMPAD_4] = EncodeScanCode(INPUT_SCAN_NUMPAD_4);
    m_scanCodeTable[INPUT_SCAN_NUMPAD_5] = EncodeScanCode(INPUT_SCAN_NUMPAD_5);
    m_scanCodeTable[INPUT_SCAN_NUMPAD_6] = EncodeScanCode(INPUT_SCAN_NUMPAD_6);
    m_scanCodeTable[INPUT_SCAN_NUMPAD_PLUS] = '+';
    m_scanCodeTable[INPUT_SCAN_NUMPAD_1] = EncodeScanCode(INPUT_SCAN_NUMPAD_1);
    m_scanCodeTable[INPUT_SCAN_NUMPAD_2] = EncodeScanCode(INPUT_SCAN_NUMPAD_2);
    m_scanCodeTable[INPUT_SCAN_NUMPAD_3] = EncodeScanCode(INPUT_SCAN_NUMPAD_3);
    m_scanCodeTable[INPUT_SCAN_NUMPAD_0] = EncodeScanCode(INPUT_SCAN_NUMPAD_0);
    m_scanCodeTable[INPUT_SCAN_NUMPAD_DELETE] = EncodeScanCode(INPUT_SCAN_NUMPAD_DELETE);
    m_scanCodeTable[INPUT_SCAN_SYSREQ] = EncodeScanCode(INPUT_SCAN_SYSREQ);
    m_scanCodeTable[INPUT_SCAN_RESERVED_55] = EncodeScanCode(INPUT_SCAN_RESERVED_55);
    m_scanCodeTable[INPUT_SCAN_ISO_BACKSLASH] = EncodeScanCode(INPUT_SCAN_ISO_BACKSLASH);
    m_scanCodeTable[INPUT_SCAN_F11] = EncodeScanCode(INPUT_SCAN_F11);
    m_scanCodeTable[INPUT_SCAN_F12] = EncodeScanCode(INPUT_SCAN_F12);
}

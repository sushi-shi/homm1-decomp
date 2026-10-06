#ifndef HOMM1_BASE_INPUTMANAGER_H
#define HOMM1_BASE_INPUTMANAGER_H

#include <BASE/baseManager.h>
#include <BASE/message.h>

enum InputManagerConstant {
    INPUT_EVENT_RING_CAPACITY = 32,
    INPUT_SCAN_CODE_CAPACITY = 0x80,
    INPUT_KEY_CODE_ASCII = 0,
    INPUT_KEY_CODE_SCAN = 1,
    INPUT_CURSOR_INTERIOR_X_MIN = 3,
    INPUT_CURSOR_INTERIOR_Y_MIN = 3,
    INPUT_CURSOR_INTERIOR_X_MAX = 636,
    INPUT_CURSOR_INTERIOR_Y_MAX = 476
};

enum InputScanCode {
INPUT_SCAN_NONE = 0x00,
    INPUT_SCAN_ESCAPE = 0x01, INPUT_SCAN_1 = 0x02, INPUT_SCAN_2 = 0x03, INPUT_SCAN_3 = 0x04,
    INPUT_SCAN_4 = 0x05, INPUT_SCAN_5 = 0x06, INPUT_SCAN_6 = 0x07, INPUT_SCAN_7 = 0x08,
    INPUT_SCAN_8 = 0x09, INPUT_SCAN_9 = 0x0a, INPUT_SCAN_0 = 0x0b, INPUT_SCAN_MINUS = 0x0c,
    INPUT_SCAN_EQUALS = 0x0d, INPUT_SCAN_BACKSPACE = 0x0e, INPUT_SCAN_TAB = 0x0f,
    INPUT_SCAN_Q = 0x10, INPUT_SCAN_W = 0x11, INPUT_SCAN_E = 0x12, INPUT_SCAN_R = 0x13,
    INPUT_SCAN_T = 0x14, INPUT_SCAN_Y = 0x15, INPUT_SCAN_U = 0x16, INPUT_SCAN_I = 0x17,
    INPUT_SCAN_O = 0x18, INPUT_SCAN_P = 0x19, INPUT_SCAN_LEFT_BRACKET = 0x1a,
    INPUT_SCAN_RIGHT_BRACKET = 0x1b, INPUT_SCAN_ENTER = 0x1c, INPUT_SCAN_CONTROL = 0x1d,
    INPUT_SCAN_A = 0x1e, INPUT_SCAN_S = 0x1f, INPUT_SCAN_D = 0x20, INPUT_SCAN_F = 0x21,
    INPUT_SCAN_G = 0x22, INPUT_SCAN_H = 0x23, INPUT_SCAN_J = 0x24, INPUT_SCAN_K = 0x25,
    INPUT_SCAN_L = 0x26, INPUT_SCAN_SEMICOLON = 0x27, INPUT_SCAN_APOSTROPHE = 0x28,
    INPUT_SCAN_GRAVE = 0x29, INPUT_SCAN_LEFT_SHIFT = 0x2a, INPUT_SCAN_BACKSLASH = 0x2b,
    INPUT_SCAN_Z = 0x2c, INPUT_SCAN_X = 0x2d, INPUT_SCAN_C = 0x2e, INPUT_SCAN_V = 0x2f,
    INPUT_SCAN_B = 0x30, INPUT_SCAN_N = 0x31, INPUT_SCAN_M = 0x32, INPUT_SCAN_COMMA = 0x33,
    INPUT_SCAN_PERIOD = 0x34, INPUT_SCAN_SLASH = 0x35, INPUT_SCAN_RIGHT_SHIFT = 0x36,
    INPUT_SCAN_NUMPAD_MULTIPLY = 0x37, INPUT_SCAN_ALT = 0x38, INPUT_SCAN_SPACE = 0x39,
    INPUT_SCAN_CAPS_LOCK = 0x3a, INPUT_SCAN_F1 = 0x3b, INPUT_SCAN_F2 = 0x3c, INPUT_SCAN_F3 = 0x3d,
    INPUT_SCAN_F4 = 0x3e, INPUT_SCAN_F5 = 0x3f, INPUT_SCAN_F6 = 0x40, INPUT_SCAN_F7 = 0x41,
    INPUT_SCAN_F8 = 0x42, INPUT_SCAN_F9 = 0x43, INPUT_SCAN_F10 = 0x44,
    INPUT_SCAN_FUNCTION_KEY_FIRST = INPUT_SCAN_F1, INPUT_SCAN_FUNCTION_KEY_LAST = INPUT_SCAN_F10,
    INPUT_SCAN_NUM_LOCK = 0x45, INPUT_SCAN_SCROLL_LOCK = 0x46, INPUT_SCAN_NUMPAD_7 = 0x47,
    INPUT_SCAN_NUMPAD_8 = 0x48, INPUT_SCAN_NUMPAD_9 = 0x49, INPUT_SCAN_NUMPAD_MINUS = 0x4a,
    INPUT_SCAN_NUMPAD_4 = 0x4b, INPUT_SCAN_NUMPAD_5 = 0x4c, INPUT_SCAN_NUMPAD_6 = 0x4d,
    INPUT_SCAN_NUMPAD_PLUS = 0x4e, INPUT_SCAN_NUMPAD_1 = 0x4f, INPUT_SCAN_NUMPAD_2 = 0x50,
    INPUT_SCAN_NUMPAD_3 = 0x51, INPUT_SCAN_NUMPAD_0 = 0x52, INPUT_SCAN_NUMPAD_DELETE = 0x53,
    INPUT_SCAN_SYSREQ = 0x54, INPUT_SCAN_RESERVED_55 = 0x55, INPUT_SCAN_ISO_BACKSLASH = 0x56,
    INPUT_SCAN_F11 = 0x57, INPUT_SCAN_F12 = 0x58,
    INPUT_SCAN_CODE_MASK = 0xff };

    enum InputKeyCodeConstant {
    INPUT_KEY_SCAN_SHIFT = 8,
    INPUT_ASCII_ESCAPE = 0x1b,
    INPUT_ASCII_DELETE = 0x7f
};

// The window messages the host passes to KeyboardMessageHandler and
// MouseMessageHandler, with the Windows values and parameter packing: a key
// message carries its Set 1 scan code in bits 16-23 of messageData, a mouse
// message its client coordinates in the low and high words.
enum InputHostMessage {
    INPUT_MESSAGE_KEY_DOWN = 0x100,
    INPUT_MESSAGE_KEY_UP = 0x101,
    INPUT_MESSAGE_MOUSE_MOVE = 0x200,
    INPUT_MESSAGE_LEFT_DOWN = 0x201,
    INPUT_MESSAGE_LEFT_UP = 0x202,
    INPUT_MESSAGE_LEFT_DOUBLE = 0x203,
    INPUT_MESSAGE_RIGHT_DOWN = 0x204,
    INPUT_MESSAGE_RIGHT_UP = 0x205,
    INPUT_MESSAGE_RIGHT_DOUBLE = 0x206,
    INPUT_VIRTUAL_KEY_RETURN = 0x0d
};

#define INPUT_MESSAGE_SCAN_CODE(messageData) (((messageData) >> 16) & INPUT_SCAN_CODE_MASK)
#define INPUT_MESSAGE_X(messageData) ((messageData) & 0xffff)
#define INPUT_MESSAGE_Y(messageData) (((messageData) >> 16) & 0xffff)

#define EncodeScanCode(scanCode) ((scanCode) << INPUT_KEY_SCAN_SHIFT)

class inputManager : public baseManager {
public:
    tag_message m_eventRing[INPUT_EVENT_RING_CAPACITY];
    i16 m_readIndex;
    i16 m_writeIndex;
    i16 m_mouseMessageActive;
    i16 m_mouseDriverReady;
    i16 m_relativeMouse;
    i16 m_mouseSpeedDivisor;
    i16 m_scanCodeTable[INPUT_SCAN_CODE_CAPACITY];
    i16 m_keyboardHookInstalled;
    i16 m_requestedPriority;
    i16 m_keyCodeType;
    i16 m_keyPrefixPending;
    i16 m_modifiers;
    i16 m_mouseX;
    i16 m_mouseY;
    b8 m_forceMouseMove;
    i32 m_recordFile;
    i32 m_unused34f;

    inputManager(void);
    virtual i16 Open(i16 priority) ;
    virtual void Close(void) ;
    virtual i16 Main(tag_message& message) ;
    void Flush(void);
    tag_message GetEvent(void);
    void SetRelativeMouse(i16 relative);
    void SetMouseCoords(i16 x, i16 y);
    void SetMouseSpeedDivisor(i16 divisor);
    void SetKeyCodeType(i16 keyCodeType);
    void AsciiConvert(tag_message& event);
    void MakeScanCodeTable(void);
    i16 GetModifiers(void) {
        return m_modifiers;
    }
};
b32 KeyboardMessageHandler(void* window, u32 message, u32 virtualKey, i32 messageData);
b32 MouseMessageHandler(void* window, u32 message, u32 keyFlags, i32 messageData);

#endif

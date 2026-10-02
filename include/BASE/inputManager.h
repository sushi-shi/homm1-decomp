#ifndef HOMM1_BASE_INPUTMANAGER_H
#define HOMM1_BASE_INPUTMANAGER_H

#include <BASE/baseManager.h>
#include <BASE/message.h>
#include <H1/Macros.h>

H1_ENUM_CONST_BEGIN(InputManagerConstant)
    INPUT_GAME_WIDTH = 640,
    INPUT_GAME_HEIGHT = 480,
    INPUT_EVENT_RING_CAPACITY = 32,
    INPUT_SCAN_CODE_CAPACITY = 0x80,
    INPUT_KEY_CODE_ASCII = 0,
    INPUT_KEY_CODE_SCAN = 1,
    INPUT_CURSOR_INTERIOR_X_MIN = 3,
    INPUT_CURSOR_INTERIOR_Y_MIN = 3,
    INPUT_CURSOR_INTERIOR_X_MAX = 636,
    INPUT_CURSOR_INTERIOR_Y_MAX = 476,
    INPUT_KEEP_CURRENT_MOUSE_FRAME = 1000
H1_ENUM_CONST_END(InputManagerConstant)

// clang-format off
// PC set-1 scan codes: KeyboardMessageHandler stores bits 16..23 of the
// WM_KEYDOWN lParam, and scan-code-mode handlers switch on them. Names follow
// HoMM2 Buka's InputManagerScanCode; only codes that HoMM1 tests are listed.
H1_ENUM_BEGIN(InputScanCode)
    INPUT_SCAN_ESCAPE = 0x01,
    INPUT_SCAN_1 = 0x02,
    INPUT_SCAN_2 = 0x03,
    INPUT_SCAN_3 = 0x04,
    INPUT_SCAN_4 = 0x05,
    INPUT_SCAN_5 = 0x06,
    INPUT_SCAN_6 = 0x07,
    INPUT_SCAN_7 = 0x08,
    INPUT_SCAN_8 = 0x09,
    INPUT_SCAN_9 = 0x0a,
    INPUT_SCAN_0 = 0x0b,
    INPUT_SCAN_Q = 0x10,
    INPUT_SCAN_T = 0x14,
    INPUT_SCAN_I = 0x17,
    INPUT_SCAN_P = 0x19,
    INPUT_SCAN_ENTER = 0x1c,
    INPUT_SCAN_CONTROL = 0x1d,
    INPUT_SCAN_S = 0x1f,
    INPUT_SCAN_D = 0x20,
    INPUT_SCAN_H = 0x23,
    INPUT_SCAN_L = 0x26,
    INPUT_SCAN_LEFT_SHIFT = 0x2a,
    INPUT_SCAN_C = 0x2e,
    INPUT_SCAN_V = 0x2f,
    INPUT_SCAN_N = 0x31,
    INPUT_SCAN_RIGHT_SHIFT = 0x36,
    INPUT_SCAN_ALT = 0x38,
    INPUT_SCAN_SPACE = 0x39,
    INPUT_SCAN_F1 = 0x3b,
    INPUT_SCAN_F2 = 0x3c,
    INPUT_SCAN_F3 = 0x3d,
    INPUT_SCAN_F4 = 0x3e,
    INPUT_SCAN_F5 = 0x3f,
    INPUT_SCAN_F6 = 0x40,
    INPUT_SCAN_F7 = 0x41,
    INPUT_SCAN_F8 = 0x42,
    INPUT_SCAN_F9 = 0x43,
    INPUT_SCAN_F10 = 0x44,
    // AsciiConvert passes F1..F10 through as one contiguous range.
    INPUT_SCAN_FUNCTION_KEY_FIRST = INPUT_SCAN_F1,
    INPUT_SCAN_FUNCTION_KEY_LAST = INPUT_SCAN_F10,
    INPUT_SCAN_NUMPAD_7 = 0x47,
    INPUT_SCAN_NUMPAD_8 = 0x48,
    INPUT_SCAN_NUMPAD_9 = 0x49,
    INPUT_SCAN_NUMPAD_4 = 0x4b,
    INPUT_SCAN_NUMPAD_6 = 0x4d,
    INPUT_SCAN_NUMPAD_1 = 0x4f,
    INPUT_SCAN_NUMPAD_2 = 0x50,
    INPUT_SCAN_NUMPAD_3 = 0x51,
    INPUT_SCAN_F11 = 0x57,
    INPUT_SCAN_F12 = 0x58,
    INPUT_SCAN_CODE_MASK = 0xff
H1_ENUM_END(InputScanCode)
// clang-format on

#pragma pack(push, 1)
class inputManager : public baseManager {
public:
    tag_message m_eventRing[INPUT_EVENT_RING_CAPACITY];
    short m_readIndex;
    short m_writeIndex;
    short m_mouseMessageActive;
    short m_field_0x236;
    short m_field_0x238;
    short m_field_0x23a;
    short m_keyState[INPUT_SCAN_CODE_CAPACITY];
    short m_field_0x33c;
    short m_requestedPriority;
    short m_keyCodeType;
    short m_field_0x342;
    H1_ENUM_STORAGE(MessageModifier, short) m_modifiers;
    char m_unknownAfterModifiers[4];
    char m_field_0x34a;
    int m_recordFile;
    int m_field_0x34f;

    inputManager(void);
    virtual short Open(short) OVERRIDE;
    virtual void Close(void) OVERRIDE;
    virtual short Main(tag_message&) OVERRIDE;
    void Flush(void);
    tag_message GetEvent(void);
    tag_message PeekEvent(void);
    void SetMouseCoords(int, int);
    void SetKeyCodeType(short);
    void AsciiConvert(tag_message&);
    void MakeScanCodeTable(void);
    void ForceMouseMove(void);
    // Inline qualifier accessor; townManager::ShiftQualChange retains its jmp.
    short GetModifiers(void) { return m_modifiers; }
};
#pragma pack(pop)
#endif // HOMM1_BASE_INPUTMANAGER_H

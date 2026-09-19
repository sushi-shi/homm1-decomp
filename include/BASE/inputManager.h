#ifndef HOMM1_BASE_INPUTMANAGER_H
#define HOMM1_BASE_INPUTMANAGER_H

#include <BASE/baseManager.h>
#include <BASE/message.h>
#include <H1/Macros.h>

H1_ENUM_BEGIN(InputManagerConstant)
    INPUT_GAME_WIDTH = 640,
    INPUT_GAME_HEIGHT = 480,
    INPUT_EVENT_RING_CAPACITY = 32,
    INPUT_CURSOR_INTERIOR_X_MIN = 3,
    INPUT_CURSOR_INTERIOR_Y_MIN = 3,
    INPUT_CURSOR_INTERIOR_X_MAX = 636,
    INPUT_CURSOR_INTERIOR_Y_MAX = 476,
    INPUT_KEEP_CURRENT_MOUSE_FRAME = 1000
H1_ENUM_END(InputManagerConstant)

class inputManager : public baseManager {
public:
    tag_message m_eventRing[INPUT_EVENT_RING_CAPACITY];
    short m_readIndex;
    short m_writeIndex;
    short m_mouseMessageActive;
    char m_unknownAfterMouse[0x10A];
    short m_keyCodeType;
    char m_unknownBeforeModifiers[2];
    H1_ENUM_STORAGE(MessageModifier, short) m_modifiers;

    inputManager(void);
    virtual short Open(short) OVERRIDE;
    virtual void Close(void) OVERRIDE;
    virtual short Main(tag_message &) OVERRIDE;
    void Flush(void);
    tag_message GetEvent(void);
    tag_message PeekEvent(void);
    void SetMouseCoords(int, int);
    void SetKeyCodeType(short);
    void AsciiConvert(tag_message &);
    void MakeScanCodeTable(void);
    void ForceMouseMove(void);
};
#endif // HOMM1_BASE_INPUTMANAGER_H

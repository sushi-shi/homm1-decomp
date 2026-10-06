#ifndef HOMM1_BASE_BUTTON_H
#define HOMM1_BASE_BUTTON_H

#include <BASE/message.h>
#include <BASE/widget.h>

enum ButtonConstant {
    BUTTON_SELECT_NOTIFY = 0,
    BUTTON_SELECT_DIALOG_RESULT = 1,
    BUTTON_REPEAT_DELAY_TICKS = 60,
    BUTTON_NO_HOTKEY = -1
};

class icon;
struct tag_message;

#pragma pack(push, 1)
class button : public widget {
public:
    icon* m_icon;
    i16 m_normalFrame;
    i16 m_pressedFrame;
    i16 m_selectMode;
    i16 m_hotkey;
    button(void);
    button(
        i16 x,
        i16 y,
        i16 width,
        i16 height,
        i16 iconId,
        i16 normalFrame,
        i16 pressedFrame,
        i16 selectMode,
        i16 hotkey,
        i16 id,
        i16 kind
    );
    button(
        i16 x,
        i16 y,
        i16 width,
        i16 height,
        char* iconName,
        i16 normalFrame,
        i16 pressedFrame,
        i16 selectMode,
        i16 hotkey,
        i16 id,
        i16 kind
    );
    virtual ~button() ;
    virtual void Draw(void) ;
    virtual i16 Main(struct tag_message& message) ;
    void Read(void);
    i16 Select(struct tag_message& message);
    i16 Deselect(struct tag_message& message);
};
#pragma pack(pop)
#endif

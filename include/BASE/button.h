#ifndef HOMM1_BASE_BUTTON_H
#define HOMM1_BASE_BUTTON_H
// Reconstructed class (BASE) from CodeView NB09 of HEROES2W.EXE — NOT original source.
// 10 methods, 2 own-virtual, 0 static data.

#include <BASE/widget.h>
#include <H1/Macros.h>

H1_ENUM_CONST_BEGIN(ButtonConstant)
    BUTTON_SELECT_DIALOG_RESULT = 1,
    BUTTON_REPEAT_DELAY_TICKS = 60,
    BUTTON_NO_HOTKEY = -1
H1_ENUM_CONST_END(ButtonConstant)

// forward declarations:
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
    // --- constructors ---
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
        char* iconId,
        i16 normalFrame,
        i16 pressedFrame,
        i16 selectMode,
        i16 hotkey,
        i16 id,
        i16 kind
    );
    virtual ~button() OVERRIDE;
    // --- virtual methods (vtable order) ---
    virtual void Draw(void) OVERRIDE;
    virtual i16 Main(struct tag_message& message) OVERRIDE;
    // --- methods ---
    void Read(void);
    i16 Select(struct tag_message& message);
    i16 Deselect(struct tag_message& message);
};
#pragma pack(pop)
#endif // HOMM1_BASE_BUTTON_H

#ifndef HOMM1_BASE_BUTTON_H
#define HOMM1_BASE_BUTTON_H
// Reconstructed class (BASE) from CodeView NB09 of HEROES2W.EXE — NOT original source.
// 10 methods, 2 own-virtual, 0 static data.

#include <BASE/widget.h>
#include <H1/Macros.h>

H1_ENUM_BEGIN(ButtonConstant)
    BUTTON_SELECT_DIALOG_RESULT = 1,
    BUTTON_REPEAT_DELAY_TICKS = 60
H1_ENUM_END(ButtonConstant)

// forward declarations:
class icon;
struct tag_message;

#pragma pack(push, 1)
class button : public widget {
public:
    icon* m_icon;
    short m_normalFrame;
    short m_pressedFrame;
    short m_selectMode;
    short m_hotkey;
    // --- constructors ---
    button(void);
    button(
        short int,
        short int,
        short int,
        short int,
        unsigned long int,
        short int,
        short int,
        short int,
        short int,
        short int,
        short int
    );
    button(
        short int,
        short int,
        short int,
        short int,
        char*,
        short int,
        short int,
        short int,
        short int,
        short int,
        short int
    );
    virtual inline ~button() OVERRIDE;
    // --- virtual methods (vtable order) ---
    virtual void Draw(void) OVERRIDE;
    virtual short Main(struct tag_message&) OVERRIDE;
    // --- methods ---
    void Read(void);
    short int Select(struct tag_message&);
    short int Deselect(struct tag_message&);
};
#pragma pack(pop)
#endif // HOMM1_BASE_BUTTON_H

#ifndef HOMM1_BASE_ICONWIDGET_H
#define HOMM1_BASE_ICONWIDGET_H
// Reconstructed class (BASE) from CodeView NB09 of HEROES2W.EXE — NOT original source.
// 8 methods, 2 own-virtual, 0 static data.

#include <BASE/widget.h>
#include <H1/Macros.h>

H1_ENUM_BEGIN(IconWidgetKind)
    ICON_WIDGET_DRAW = 0x10,
    ICON_WIDGET_FILL = 0x80
H1_ENUM_END(IconWidgetKind)

// forward declarations:
class icon;
struct tag_message;

#pragma pack(push, 1)
class iconWidget : public widget {
public:
    icon* m_icon;
    short m_frame;
    signed char m_orientation;
    short m_fillColor;
    // --- constructors ---
    iconWidget(void);
    iconWidget(
        short int x,
        short int y,
        short int width,
        short int height,
        unsigned long int iconId,
        short int frame,
        signed char orientation,
        short int id,
        short int kind,
        short int fillColor
    );
    iconWidget(
        short int,
        short int,
        short int,
        short int,
        char*,
        signed char,
        signed char,
        short int,
        short int,
        short int
    );
    virtual inline ~iconWidget() OVERRIDE;
    // --- virtual methods (vtable order) ---
    virtual void Draw(void) OVERRIDE;
    virtual short Main(struct tag_message&) OVERRIDE;
    // --- methods ---
    void Read(void);
};
#pragma pack(pop)
#endif // HOMM1_BASE_ICONWIDGET_H

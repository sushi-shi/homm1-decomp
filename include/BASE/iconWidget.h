#ifndef HOMM1_BASE_ICONWIDGET_H
#define HOMM1_BASE_ICONWIDGET_H

#include <BASE/icon.h>
#include <BASE/message.h>
#include <BASE/widget.h>
#include <H1/Macros.h>

// iconWidget kinds, carried in widget::m_kind.
H1_ENUM_ID_BEGIN(IconWidgetKind)
    ICON_WIDGET_DRAW = 0x10,
    ICON_WIDGET_FILL = 0x80
H1_ENUM_ID_END(IconWidgetKind)

// Read keeps the low byte of the resource's orientation word.
H1_ENUM_CONST_BEGIN(IconWidgetConstant)
    ICON_WIDGET_ORIENTATION_MASK = 0xff
H1_ENUM_CONST_END(IconWidgetConstant)

// forward declarations:
class icon;
struct tag_message;

#pragma pack(push, 1)
class iconWidget : public widget {
public:
    icon* m_icon;
    i16 m_frame;
    H1_ENUM_STORAGE(IconDrawOrientation, i8) m_orientation;
    i16 m_fillColor;
    // --- constructors ---
    iconWidget(void);
    iconWidget(
        i16 x,
        i16 y,
        i16 width,
        i16 height,
        i16 iconId,
        i8 frame,
        H1_ENUM_PARAM(IconDrawOrientation, i8) orientation,
        i16 id,
        H1_ENUM_PARAM(IconWidgetKind, i16) kind,
        i16 fillColor
    );
    iconWidget(
        i16 x,
        i16 y,
        i16 width,
        i16 height,
        char* iconName,
        i8 frame,
        H1_ENUM_PARAM(IconDrawOrientation, i8) orientation,
        i16 id,
        H1_ENUM_PARAM(IconWidgetKind, i16) kind,
        i16 fillColor
    );
    virtual ~iconWidget() OVERRIDE;
    // --- virtual methods (vtable order) ---
    virtual void Draw(void) OVERRIDE;
    virtual H1_ENUM_RETURN(MessageDispatchResult, i16) Main(struct tag_message& message) OVERRIDE;
    // --- methods ---
    void Read(void);
};
#pragma pack(pop)
#endif // HOMM1_BASE_ICONWIDGET_H

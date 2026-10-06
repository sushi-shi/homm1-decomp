#ifndef HOMM1_BASE_ICONWIDGET_H
#define HOMM1_BASE_ICONWIDGET_H

#include <BASE/widget.h>

enum IconWidgetKind {
    ICON_WIDGET_DRAW = 0x10,
    ICON_WIDGET_FILL = 0x80
};

class icon;
struct tag_message;

#pragma pack(push, 1)
class iconWidget : public widget {
public:
    icon* m_icon;
    i16 m_frame;
    i8 m_orientation;
    i16 m_fillColor;
    iconWidget(void);
    iconWidget(
        i16 x,
        i16 y,
        i16 width,
        i16 height,
        u32 iconId,
        i16 frame,
        i8 orientation,
        i16 id,
        i16 kind,
        i16 fillColor
    );
    iconWidget(
        i16 x,
        i16 y,
        i16 width,
        i16 height,
        char* name,
        i8 frame,
        i8 orientation,
        i16 id,
        i16 kind,
        i16 fillColor
    );
    virtual inline ~iconWidget() ;
    virtual void Draw(void) ;
    virtual i16 Main(struct tag_message& message) ;
    void Read(void);
};
#pragma pack(pop)
#endif

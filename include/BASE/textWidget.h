#ifndef HOMM1_BASE_TEXTWIDGET_H
#define HOMM1_BASE_TEXTWIDGET_H

#include <BASE/font.h>
#include <BASE/message.h>
#include <BASE/widget.h>
#include <H1/Macros.h>

// forward declarations:
class font;
struct tag_message;

// textWidget::m_color (a WIDGET_COMMAND_SET_COLOR value): the plain text
// colour a textWidget starts with and a list restores to a row it no longer
// highlights (the file requester's and the high-score window's rows).
H1_ENUM_CONST_BEGIN(TextWidgetColor)
    TEXT_WIDGET_PLAIN_COLOR = 1
H1_ENUM_CONST_END(TextWidgetColor)

#pragma pack(push, 1)
class textWidget : public widget {
public:
    char* m_text;
    font* m_font;
    i16 m_color;
    H1_ENUM_STORAGE(FontAlignment, i8) m_alignment;
    // --- constructors ---
    textWidget(void);
    textWidget(
        i16 x,
        i16 y,
        i16 width,
        i16 height,
        char* text,
        char* fontName,
        i16 color,
        i16 id,
        i16 kind
    );
    virtual ~textWidget() OVERRIDE;
    // --- virtual methods (vtable order) ---
    virtual void Draw(void) OVERRIDE;
    virtual H1_ENUM_RETURN(MessageDispatchResult, i16) Main(struct tag_message& message) OVERRIDE;
    // --- methods ---
    void Read(void);
    void SetColorIndex(i16 color);
    void SetText(char* text);
};
#pragma pack(pop)
#endif // HOMM1_BASE_TEXTWIDGET_H

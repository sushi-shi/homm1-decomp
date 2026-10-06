#ifndef HOMM1_BASE_TEXTWIDGET_H
#define HOMM1_BASE_TEXTWIDGET_H

#include <BASE/font.h>
#include <BASE/message.h>
#include <BASE/widget.h>

class font;
struct tag_message;

class textWidget : public widget {
public:
    char* m_text;
    font* m_font;
    i16 m_color;
    i8 m_alignment;
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
    virtual ~textWidget() ;
    virtual void Draw(void) ;
    virtual i16 Main(struct tag_message& message) ;
    void Read(void);
    void SetColorIndex(i16 color);
    void SetText(char* text);
};
#endif

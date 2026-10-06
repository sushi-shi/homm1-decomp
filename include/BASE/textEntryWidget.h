#ifndef HOMM1_BASE_TEXTENTRYWIDGET_H
#define HOMM1_BASE_TEXTENTRYWIDGET_H

#include <BASE/message.h>
#include <BASE/textWidget.h>

enum TextEntryReadMode {
    TEXT_ENTRY_READ_DEFAULT = 1,
    TEXT_ENTRY_READ_RECT = 2,
    TEXT_ENTRY_READ_SCROLLING = 3
};

enum TextEntryConstant {
    TEXT_ENTRY_DISPLAY_CAPACITY = 300,
    TEXT_ENTRY_PRESERVE_TEXT = 1,
    TEXT_ENTRY_ALLOCATION_PADDING = 5,
    TEXT_ENTRY_KEY_ACCEPT = '\n',
    TEXT_ENTRY_EXTENDED_KEY_BASE = 0x100
};

class icon;
struct tag_message;

#pragma pack(push, 1)
class textEntryWidget : public textWidget {
public:
    icon* m_icon;
    i16 m_iconFrame;
    u16 m_cursorPosition;
    u16 m_maxLength;
    i16 m_rectX;
    i16 m_rectY;
    i16 m_rectW;
    i16 m_rectH;
    i16 m_maxLines;
    i16 m_preserveTextOnFocus;
    i16 m_entryType;
    i16 m_displayOffset;
    textEntryWidget(void);
    textEntryWidget(
        i16 x,
        i16 y,
        i16 width,
        i16 height,
        i16 maxLength,
        char* text,
        char* fontName,
        i16 color,
        char* iconName,
        i16 iconFrame,
        i16 id,
        i16 kind
    );
    virtual ~textEntryWidget() ;
    virtual void Draw(void) ;
    virtual i16 Main(struct tag_message& message) ;
    void Read(i32 type);
    void SetupDisplayString(char* source, u16 cursor);
};
#pragma pack(pop)
#endif

#ifndef HOMM1_BASE_TEXTENTRYWIDGET_H
#define HOMM1_BASE_TEXTENTRYWIDGET_H

#include <BASE/textWidget.h>

enum TextEntryReadMode {
    TEXT_ENTRY_READ_DEFAULT = 1,
    TEXT_ENTRY_READ_RECT = 2,
    TEXT_ENTRY_READ_MULTILINE = 3
};

enum TextEntryConstant {
    TEXT_ENTRY_DISPLAY_CAPACITY = 300,
    TEXT_ENTRY_PRESERVE_TEXT = 1,
    TEXT_ENTRY_ALLOCATION_PADDING = 5,
    TEXT_ENTRY_KEY_ESCAPE = 1,
    TEXT_ENTRY_KEY_LEFT = 0x4b,
    TEXT_ENTRY_KEY_RIGHT = 0x4d,
    TEXT_ENTRY_KEY_DELETE = 0x53,
    TEXT_ENTRY_KEY_ACCEPT = '\n',
    TEXT_ENTRY_KEY_BACKSPACE = 0x7f,
    TEXT_ENTRY_EXTENDED_KEY_BASE = 0x100,
    TEXT_ENTRY_KEYPAD_7 = 0x47,
    TEXT_ENTRY_KEYPAD_8 = 0x48,
    TEXT_ENTRY_KEYPAD_9 = 0x49,
    TEXT_ENTRY_KEYPAD_4 = 0x4b,
    TEXT_ENTRY_KEYPAD_5 = 0x4c,
    TEXT_ENTRY_KEYPAD_6 = 0x4d,
    TEXT_ENTRY_KEYPAD_1 = 0x4f,
    TEXT_ENTRY_KEYPAD_2 = 0x50,
    TEXT_ENTRY_KEYPAD_3 = 0x51,
    TEXT_ENTRY_KEYPAD_0 = 0x52
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
        i16 kind,
        i16 layout,
        i32 horizontalInset,
        i32 verticalInset
    );
    virtual inline ~textEntryWidget() ;
    virtual void Draw(void) ;
    virtual i16 Main(struct tag_message& message) ;
    void Read(i32 type);
    void SetupDisplayString(char* source, u16 cursor);
};
#pragma pack(pop)
#endif

#ifndef HOMM1_BASE_TEXTENTRYWIDGET_H
#define HOMM1_BASE_TEXTENTRYWIDGET_H

#include <BASE/textWidget.h>
#include <Domains.h>
#include <H1/Macros.h>

// DEFAULT and RECT entries wrap their text and refuse input past m_maxLines
// lines; a SCROLLING entry keeps one line and scrolls it horizontally to the
// cursor through m_displayOffset.
H1_ENUM_BEGIN(TextEntryReadMode)
    TEXT_ENTRY_READ_DEFAULT = 1,
    TEXT_ENTRY_READ_RECT = 2,
    TEXT_ENTRY_READ_SCROLLING = 3
H1_ENUM_END(TextEntryReadMode)

H1_ENUM_CONST_BEGIN(TextEntryConstant)
    TEXT_ENTRY_DISPLAY_CAPACITY = 300,
    TEXT_ENTRY_PRESERVE_TEXT = 1,
    TEXT_ENTRY_ALLOCATION_PADDING = 5,
    // AsciiConvert's code for the Enter key.
    TEXT_ENTRY_KEY_ACCEPT = '\n',
    TEXT_ENTRY_EXTENDED_KEY_BASE = 0x100
H1_ENUM_CONST_END(TextEntryConstant)

// forward declarations:
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
    H1_ENUM_STORAGE(TextEntryReadMode, i16) m_entryType;
    i16 m_displayOffset;
    // --- constructors ---
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
    virtual ~textEntryWidget() OVERRIDE;
    // --- virtual methods (vtable order) ---
    virtual void Draw(void) OVERRIDE;
    virtual i16 Main(struct tag_message& message) OVERRIDE;
    // --- methods ---
    void Read(H1_ENUM_PARAM(TextEntryReadMode, i32) type);
    void SetupDisplayString(char* source, u16 cursor);
};
#pragma pack(pop)
#endif // HOMM1_BASE_TEXTENTRYWIDGET_H

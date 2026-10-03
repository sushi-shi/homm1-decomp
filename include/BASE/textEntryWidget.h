#ifndef HOMM1_BASE_TEXTENTRYWIDGET_H
#define HOMM1_BASE_TEXTENTRYWIDGET_H
// Reconstructed class (BASE) from CodeView NB09 of HEROES2W.EXE — NOT original source.
// 8 methods, 2 own-virtual, 0 static data.

#include <BASE/textWidget.h>
#include <Domains.h>
#include <H1/Macros.h>

H1_ENUM_BEGIN(TextEntryReadMode)
    TEXT_ENTRY_READ_DEFAULT = 1,
    TEXT_ENTRY_READ_RECT = 2,
    TEXT_ENTRY_READ_MULTILINE = 3
H1_ENUM_END(TextEntryReadMode)

H1_ENUM_CONST_BEGIN(TextEntryConstant)
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
H1_ENUM_CONST_END(TextEntryConstant)

// forward declarations:
class icon;
struct tag_message;

#pragma pack(push, 1)
class textEntryWidget : public textWidget {
public:
    icon* m_icon;
    short m_iconFrame;
    unsigned short m_cursorPosition;
    unsigned short m_maxLength;
    short m_rectX;
    short m_rectY;
    short m_rectW;
    short m_rectH;
    short m_maxLines;
    short m_preserveTextOnFocus;
    H1_ENUM_STORAGE(TextEntryReadMode, short) m_entryType;
    short m_displayOffset;
    // --- constructors ---
    textEntryWidget(void);
    textEntryWidget(
        short int x,
        short int y,
        short int width,
        short int height,
        short int maxLength,
        char* text,
        char* fontName,
        short int color,
        char* iconName,
        short int iconFrame,
        short int id,
        short int kind,
        short int layout,
        int horizontalInset,
        int verticalInset
    );
    virtual inline ~textEntryWidget() OVERRIDE;
    // --- virtual methods (vtable order) ---
    virtual void Draw(void) OVERRIDE;
    virtual short Main(struct tag_message&) OVERRIDE;
    // --- methods ---
    void Read(H1_ENUM_PARAM(TextEntryReadMode, int));
    void SetupDisplayString(char*, unsigned short int);
};
#pragma pack(pop)
#endif // HOMM1_BASE_TEXTENTRYWIDGET_H

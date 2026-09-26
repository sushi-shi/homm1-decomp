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

H1_ENUM_BEGIN(TextEntryConstant)
    TEXT_ENTRY_DISPLAY_CAPACITY = 300
H1_ENUM_END(TextEntryConstant)

// forward declarations:
class icon;
struct tag_message;

#pragma pack(push, 1)
class textEntryWidget : public textWidget {
public:
    icon *m_icon;
    short m_iconFrame;
    unsigned short m_cursorPosition;
    unsigned short m_maxLength;
    short m_rectX;
    short m_rectY;
    short m_rectW;
    short m_rectH;
    short m_maxLines;
    short m_preserveTextOnFocus;
    short m_entryType;
    short m_displayOffset;
    // --- constructors ---
    textEntryWidget(void);
    textEntryWidget(short int, short int, short int, short int, short int, char *, char *, short int, char *, short int, short int, short int, short int, int, int);
    virtual inline ~textEntryWidget() OVERRIDE;
    // --- virtual methods (vtable order) ---
    virtual void Draw(void) OVERRIDE;
    virtual short Main(struct tag_message &) OVERRIDE;
    // --- methods ---
    void Read(int);
    void SetupDisplayString(char *, unsigned short int);
};
#pragma pack(pop)
#endif // HOMM1_BASE_TEXTENTRYWIDGET_H

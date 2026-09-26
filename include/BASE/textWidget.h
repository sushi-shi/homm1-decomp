#ifndef HOMM1_BASE_TEXTWIDGET_H
#define HOMM1_BASE_TEXTWIDGET_H
// Reconstructed class (BASE) from CodeView NB09 of HEROES2W.EXE — NOT original source.
// 9 methods, 2 own-virtual, 0 static data.

#include <BASE/widget.h>
#include <H1/Macros.h>

// forward declarations:
class font;
struct tag_message;

#pragma pack(push, 1)
class textWidget : public widget {
public:
    char *m_text;
    font *m_font;
    short m_color;
    char m_alignment;
    // --- constructors ---
    textWidget(void);
    textWidget(short int, short int, short int, short int, char *, char *, short int, short int, short int, short int);
    virtual inline ~textWidget() OVERRIDE;
    // --- virtual methods (vtable order) ---
    virtual void Draw(void) OVERRIDE;
    virtual short Main(struct tag_message &) OVERRIDE;
    // --- methods ---
    void Read(void);
    void SetColorIndex(short int);
    void SetText(char *);
};
#pragma pack(pop)
#endif // HOMM1_BASE_TEXTWIDGET_H

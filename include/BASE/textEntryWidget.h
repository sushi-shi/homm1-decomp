#ifndef HOMM1_BASE_TEXTENTRYWIDGET_H
#define HOMM1_BASE_TEXTENTRYWIDGET_H
// Reconstructed class (BASE) from CodeView NB09 of HEROES2W.EXE — NOT original source.
// 8 methods, 2 own-virtual, 0 static data.

#include <BASE/widget.h>
#include <H1/Macros.h>

// forward declarations:
struct tag_message;

class textEntryWidget {
public:
    // --- constructors ---
    textEntryWidget(void);
    textEntryWidget(short int, short int, short int, short int, short int, char *, char *, short int, char *, short int, short int, short int, short int, int, int);
    virtual ~textEntryWidget() OVERRIDE;
    // --- virtual methods (vtable order) ---
    virtual void Draw(void) OVERRIDE;
    virtual int Main(struct tag_message &) OVERRIDE;
    // --- methods ---
    void Read(int);
    void SetupDisplayString(char *, unsigned short int);
};
#endif // HOMM1_BASE_TEXTENTRYWIDGET_H

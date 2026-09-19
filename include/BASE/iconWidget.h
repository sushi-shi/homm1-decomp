#ifndef HOMM1_BASE_ICONWIDGET_H
#define HOMM1_BASE_ICONWIDGET_H
// Reconstructed class (BASE) from CodeView NB09 of HEROES2W.EXE — NOT original source.
// 8 methods, 2 own-virtual, 0 static data.

#include <BASE/widget.h>
#include <H1/Macros.h>

// forward declarations:
struct tag_message;

class iconWidget {
public:
    // --- constructors ---
    iconWidget(void);
    iconWidget(short int, short int, short int, short int, unsigned long int, short int, signed char, short int, short int, short int);
    iconWidget(short int, short int, short int, short int, char *, short int, signed char, short int, short int, short int);
    virtual ~iconWidget() OVERRIDE;
    // --- virtual methods (vtable order) ---
    virtual void Draw(void) OVERRIDE;
    virtual int Main(struct tag_message &) OVERRIDE;
    // --- methods ---
    void Read(void);
};
#endif // HOMM1_BASE_ICONWIDGET_H

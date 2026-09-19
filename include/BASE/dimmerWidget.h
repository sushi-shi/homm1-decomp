#ifndef HOMM1_BASE_DIMMERWIDGET_H
#define HOMM1_BASE_DIMMERWIDGET_H
// Reconstructed class (BASE) from CodeView NB09 of HEROES2W.EXE — NOT original source.
// 6 methods, 2 own-virtual, 0 static data.

#include <BASE/widget.h>
#include <H1/Macros.h>

// forward declarations:
struct tag_message;

class dimmerWidget : public widget {
public:
    // --- constructors ---
    dimmerWidget(void);
    dimmerWidget(short int, short int, short int, short int, short int, short int);
    virtual ~dimmerWidget() OVERRIDE {}   // EXPLICIT but inline: retail has ??_E/??_G (deleting
                                          // dtors) at 0x4dd410 with the base dtor folded in and
                                          // NO standalone ??1 — an out-of-line body would emit one.
    // --- virtual methods (vtable order) ---
    virtual void Draw(void) OVERRIDE;
    virtual int Main(struct tag_message &) OVERRIDE;
    // --- methods ---
    void Read(void);
};
#endif // HOMM1_BASE_DIMMERWIDGET_H

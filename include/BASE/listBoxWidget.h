#ifndef HOMM1_BASE_LISTBOXWIDGET_H
#define HOMM1_BASE_LISTBOXWIDGET_H
// Reconstructed class (BASE) from CodeView NB09 of HEROES2W.EXE — NOT original source.
// 9 methods, 2 own-virtual, 0 static data.

#include <BASE/widget.h>
#include <H1/Macros.h>

// forward declarations:
struct tag_message;

class listBoxWidget : public widget {
public:
    // --- constructors ---
    listBoxWidget(void);
    virtual ~listBoxWidget() OVERRIDE;
    // --- virtual methods (vtable order) ---
    virtual void Draw(void) OVERRIDE;
    virtual short Main(struct tag_message & message) OVERRIDE;
    // --- methods ---
    void Read(void);
    void DeleteItem(int index);
    void DrawLBStuff(int doUpdate);
    int ProcessMouseMessage(struct tag_message & message);
};
#endif // HOMM1_BASE_LISTBOXWIDGET_H

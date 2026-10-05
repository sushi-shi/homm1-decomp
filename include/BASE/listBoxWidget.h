#ifndef HOMM1_BASE_LISTBOXWIDGET_H
#define HOMM1_BASE_LISTBOXWIDGET_H

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
    virtual i16 Main(struct tag_message& message) OVERRIDE;
    // --- methods ---
    void Read(void);
    void DeleteItem(i32 index);
    void DrawLBStuff(i32 doUpdate);
    i32 ProcessMouseMessage(struct tag_message& message);
};
#endif // HOMM1_BASE_LISTBOXWIDGET_H

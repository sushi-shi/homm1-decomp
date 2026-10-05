#ifndef HOMM1_BASE_DROPLISTWIDGET_H
#define HOMM1_BASE_DROPLISTWIDGET_H

#include <BASE/widget.h>
#include <H1/Macros.h>

// forward declarations:
struct tag_message;

class dropListWidget : public widget {
public:
    // --- constructors ---
    dropListWidget(void);
    virtual ~dropListWidget() OVERRIDE;
    // --- virtual methods (vtable order) ---
    virtual void Draw(void) OVERRIDE;
    virtual i16 Main(struct tag_message& message) OVERRIDE;
    // --- methods ---
    void Read(void);
    void DeleteItem(i32 index);
    void DrawDropStuff(void);
    void SaveDropBackground(void);
    void RestoreDropBackground(void);
    void ProcessSelectDialog(void);
};
#endif // HOMM1_BASE_DROPLISTWIDGET_H

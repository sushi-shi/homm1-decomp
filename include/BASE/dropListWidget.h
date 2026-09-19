#ifndef HOMM1_BASE_DROPLISTWIDGET_H
#define HOMM1_BASE_DROPLISTWIDGET_H
// Reconstructed class (BASE) from CodeView NB09 of HEROES2W.EXE — NOT original source.
// 11 methods, 2 own-virtual, 0 static data.

#include <BASE/widget.h>
#include <H1/Macros.h>

// forward declarations:
struct tag_message;

class dropListWidget {
public:
    // --- constructors ---
    dropListWidget(void);
    virtual ~dropListWidget() OVERRIDE;
    // --- virtual methods (vtable order) ---
    virtual void Draw(void) OVERRIDE;
    virtual int Main(struct tag_message &) OVERRIDE;
    // --- methods ---
    void Read(void);
    void DeleteItem(int);
    void DrawDropStuff(void);
    void SaveDropBackground(void);
    void RestoreDropBackground(void);
    void ProcessSelectDialog(void);
};
#endif // HOMM1_BASE_DROPLISTWIDGET_H

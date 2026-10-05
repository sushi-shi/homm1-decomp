#ifndef HOMM1_BASE_DIMMERWIDGET_H
#define HOMM1_BASE_DIMMERWIDGET_H

#include <BASE/message.h>
#include <BASE/widget.h>
#include <H1/Macros.h>

// forward declarations:
struct tag_message;

class dimmerWidget : public widget {
public:
    // --- constructors ---
    dimmerWidget(void);
    dimmerWidget(i16 x, i16 y, i16 width, i16 height, i16 id, i16 kind);
    virtual inline ~dimmerWidget() OVERRIDE;
    // --- virtual methods (vtable order) ---
    virtual void Draw(void) OVERRIDE;
    virtual H1_ENUM_RETURN(MessageDispatchResult, i16) Main(struct tag_message& message) OVERRIDE;
    // --- methods ---
    void Read(void);
};
#endif // HOMM1_BASE_DIMMERWIDGET_H

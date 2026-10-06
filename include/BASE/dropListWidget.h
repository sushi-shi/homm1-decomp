#ifndef HOMM1_BASE_DROPLISTWIDGET_H
#define HOMM1_BASE_DROPLISTWIDGET_H

#include <BASE/message.h>
#include <BASE/widget.h>

struct tag_message;

class dropListWidget : public widget {
public:
    dropListWidget(void);
    virtual ~dropListWidget() ;
    virtual void Draw(void) ;
    virtual i16 Main(struct tag_message& message) ;
    void Read(void);
    void DeleteItem(i32 index);
    void DrawDropStuff(void);
    void SaveDropBackground(void);
    void RestoreDropBackground(void);
    void ProcessSelectDialog(void);
};
#endif

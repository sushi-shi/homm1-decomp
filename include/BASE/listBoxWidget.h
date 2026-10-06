#ifndef HOMM1_BASE_LISTBOXWIDGET_H
#define HOMM1_BASE_LISTBOXWIDGET_H

#include <BASE/widget.h>

struct tag_message;

class listBoxWidget : public widget {
public:
    listBoxWidget(void);
    virtual ~listBoxWidget() ;
    virtual void Draw(void) ;
    virtual i16 Main(struct tag_message& message) ;
    void Read(void);
    void DeleteItem(i32 index);
    void DrawLBStuff(i32 doUpdate);
    i32 ProcessMouseMessage(struct tag_message& message);
};
#endif

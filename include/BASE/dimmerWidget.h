#ifndef HOMM1_BASE_DIMMERWIDGET_H
#define HOMM1_BASE_DIMMERWIDGET_H

#include <BASE/message.h>
#include <BASE/widget.h>

struct tag_message;

class dimmerWidget : public widget {
public:
    dimmerWidget(void);
    dimmerWidget(i16 x, i16 y, i16 width, i16 height, i16 id, i16 kind);
    virtual inline ~dimmerWidget() ;
    virtual void Draw(void) ;
    virtual i16 Main(struct tag_message& message) ;
    void Read(void);
};
#endif

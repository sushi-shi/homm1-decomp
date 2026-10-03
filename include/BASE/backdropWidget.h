#ifndef HOMM1_BASE_BACKDROPWIDGET_H
#define HOMM1_BASE_BACKDROPWIDGET_H

#include <BASE/widget.h>
#include <H1/Macros.h>

struct tag_message;

class backdropWidget : public widget {
public:
    backdropWidget(void);
    backdropWidget(i16 x, i16 y, i16 width, i16 height, i16 id, i16 kind);
    virtual inline ~backdropWidget() OVERRIDE;
    virtual void Draw(void) OVERRIDE;
    virtual i16 Main(struct tag_message& message) OVERRIDE;
    void Read(void);
};

#endif // HOMM1_BASE_BACKDROPWIDGET_H

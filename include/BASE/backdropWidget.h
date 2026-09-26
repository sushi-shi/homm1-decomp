#ifndef HOMM1_BASE_BACKDROPWIDGET_H
#define HOMM1_BASE_BACKDROPWIDGET_H

#include <BASE/widget.h>
#include <H1/Macros.h>

struct tag_message;

class backdropWidget : public widget {
public:
    backdropWidget(void);
    virtual inline ~backdropWidget() OVERRIDE;
    virtual void Draw(void) OVERRIDE;
    virtual short Main(struct tag_message &) OVERRIDE;
    void Read(void);
};

#endif // HOMM1_BASE_BACKDROPWIDGET_H

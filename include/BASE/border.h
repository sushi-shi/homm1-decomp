#ifndef HOMM1_BASE_BORDER_H
#define HOMM1_BASE_BORDER_H

#include <BASE/message.h>
#include <BASE/widget.h>

enum BorderBackgroundKind {
    BORDER_BACKGROUND_SOLID = 0x400,
    BORDER_BACKGROUND_BITMAP = 0x800
};

class bitmap;
struct tag_message;

#pragma pack(push, 1)
class border : public widget {
public:
    bitmap* m_background;
    i16 m_fillColor;
    border(void);
    border(i16 x, i16 y, i16 width, i16 height, i16 id, i16 kind, i16 fillColor, char* bitmapName);
    virtual ~border() ;
    virtual void Draw(void) ;
    virtual i16 Main(struct tag_message& message) ;
    void Read(void);
};
#pragma pack(pop)
#endif

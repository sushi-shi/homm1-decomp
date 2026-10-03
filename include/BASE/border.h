#ifndef HOMM1_BASE_BORDER_H
#define HOMM1_BASE_BORDER_H
// Reconstructed class (BASE) from CodeView NB09 of HEROES2W.EXE — NOT original source.
// 7 methods, 2 own-virtual, 0 static data.

#include <BASE/widget.h>
#include <Domains.h>
#include <H1/Macros.h>

H1_ENUM_BEGIN(BorderBackgroundKind)
    BORDER_BACKGROUND_SOLID = 0x400,
    BORDER_BACKGROUND_BITMAP = 0x800
H1_ENUM_END(BorderBackgroundKind)

// forward declarations:
class bitmap;
struct tag_message;

#pragma pack(push, 1)
class border : public widget {
public:
    bitmap* m_background;
    i16 m_fillColor;
    // --- constructors ---
    border(void);
    border(i16 x, i16 y, i16 width, i16 height, i16 id, i16 kind, i16 fillColor, char* name);
    virtual inline ~border() OVERRIDE;
    // --- virtual methods (vtable order) ---
    virtual void Draw(void) OVERRIDE;
    virtual i16 Main(struct tag_message& message) OVERRIDE;
    // --- methods ---
    void Read(void);
};
#pragma pack(pop)
#endif // HOMM1_BASE_BORDER_H

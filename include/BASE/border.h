#ifndef HOMM1_BASE_BORDER_H
#define HOMM1_BASE_BORDER_H
// Reconstructed class (BASE) from CodeView NB09 of HEROES2W.EXE — NOT original source.
// 7 methods, 2 own-virtual, 0 static data.

#include <BASE/widget.h>
#include <Domains.h>
#include <H1/Macros.h>

// clang-format off
H1_ENUM_BEGIN(BorderBackgroundKind)
    BORDER_BACKGROUND_SOLID = 0x400,
    BORDER_BACKGROUND_BITMAP = 0x800
H1_ENUM_END(BorderBackgroundKind)
// clang-format on

// forward declarations:
class bitmap;
struct tag_message;

#pragma pack(push, 1)
class border : public widget {
public:
    bitmap* m_background;
    short m_fillColor;
    // --- constructors ---
    border(void);
    border(short int, short int, short int, short int, short int, short int, short int, char*);
    virtual inline ~border() OVERRIDE;
    // --- virtual methods (vtable order) ---
    virtual void Draw(void) OVERRIDE;
    virtual short Main(struct tag_message&) OVERRIDE;
    // --- methods ---
    void Read(void);
};
#pragma pack(pop)
#endif // HOMM1_BASE_BORDER_H

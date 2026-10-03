#ifndef HOMM1_BASE_PALETTE_H
#define HOMM1_BASE_PALETTE_H
// Reconstructed class (BASE) from CodeView NB09 of HEROES2W.EXE — NOT original source.
// 5 methods, 0 own-virtual, 0 static data.

#include <BASE/resource.h>

// The raw 256-colour, 3-byte palette block (Buka palette.h).
H1_ENUM_CONST_BEGIN(PaletteConstant)
    PALETTE_DATA_SIZE = 0x300
H1_ENUM_CONST_END(PaletteConstant)

#pragma pack(push, 1)
class palette : public resource {
public:
    signed char* m_data;
    // --- constructors ---
    palette(void);
    palette(short);
    virtual inline ~palette();
    // --- methods ---
    signed char* Data(void);
};
#pragma pack(pop)
#endif // HOMM1_BASE_PALETTE_H

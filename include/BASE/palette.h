#ifndef HOMM1_BASE_PALETTE_H
#define HOMM1_BASE_PALETTE_H
// Reconstructed class (BASE) from CodeView NB09 of HEROES2W.EXE — NOT original source.
// 5 methods, 0 own-virtual, 0 static data.

#include <BASE/resource.h>
#include <Domains.h>

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

H1_ENUM_CONST_BEGIN(PaletteGraphicsConstant)
    PALETTE_GRAPHICS_BYTES = 768,
    PALETTE_GRAPHICS_END = PALETTE_GRAPHICS_BYTES,
    PALETTE_GRAPHICS_CHANNELS = 3,
    PALETTE_CYCLE_FIRST = 214,
    PALETTE_CYCLE_COLOR_COUNT = 32,
    PALETTE_FADE_LEVEL_END = 64,
    PALETTE_FADE_LEVEL_LAST = 63,
    PALETTE_WINDOWED_FADE_SCALE = 2,
    PALETTE_CYCLE_BYTES = PALETTE_CYCLE_COLOR_COUNT * PALETTE_GRAPHICS_CHANNELS
H1_ENUM_CONST_END(PaletteGraphicsConstant)

#endif // HOMM1_BASE_PALETTE_H

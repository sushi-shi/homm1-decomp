#ifndef HOMM1_BASE_PALETTE_H
#define HOMM1_BASE_PALETTE_H

#include <BASE/resource.h>
#include <Domains.h>

// The raw 256-colour, 3-byte palette block.
H1_ENUM_CONST_BEGIN(PaletteConstant)
    PALETTE_DATA_SIZE = 0x300
H1_ENUM_CONST_END(PaletteConstant)

#pragma pack(push, 1)
class palette : public resource {
public:
    i8* m_data;
    // --- constructors ---
    palette(void);
    palette(i16 id);
    virtual ~palette();
    // --- methods ---
    i8* Data(void);
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

#ifndef HOMM1_BASE_PALETTE_H
#define HOMM1_BASE_PALETTE_H

#include <BASE/resource.h>

enum PaletteConstant {
    PALETTE_DATA_SIZE = 0x300
};

class palette : public resource {
public:
    i8* m_data;
    palette(void);
    palette(i16 id);
    virtual ~palette();
    i8* Data(void);
};

enum PaletteGraphicsConstant {
    PALETTE_GRAPHICS_CHANNELS = 3,
    PALETTE_CYCLE_FIRST = 214,
    PALETTE_CYCLE_COLOR_COUNT = 32,
    PALETTE_FADE_LEVEL_END = 64,
    PALETTE_FADE_LEVEL_LAST = 63,
    PALETTE_WINDOWED_FADE_SCALE = 2,
    PALETTE_CYCLE_BYTES = PALETTE_CYCLE_COLOR_COUNT * PALETTE_GRAPHICS_CHANNELS
};

enum PaletteChannel {
    PALETTE_CHANNEL_RED = 0,
    PALETTE_CHANNEL_GREEN = 1,
    PALETTE_CHANNEL_BLUE = 2
};

enum PaletteSelectionColor {
    PALETTE_SELECTION_COLOR = 232
};

#endif

#ifndef HOMM1_BASE_MISC_H
#define HOMM1_BASE_MISC_H

#include <Domains.h>

class bitmap;

// Map-grid (taxicab) distance of an offset (Buka 2.1 Misc.h).
#define MANHATTAN_LENGTH(dx, dy) (abs((dx)) + abs((dy)))

void SetPalette(i8* paletteData, i32 updateDisplay);

H1_ENUM_CONST_BEGIN(MiscLogConstant)
    MISC_FORCED_DEBUG_LEVEL = 9
H1_ENUM_CONST_END(MiscLogConstant)

#endif

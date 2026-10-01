// HoMM1 OLDASM.CPP helpers; the assert literal names the retail source file.

#include <match.h>

#include <BASE/bitmap.h>
#include <BASE/icon.h>
#include <BASE/Misc.h>
#include <BASE/MISC_TYPES.h>

#include <stdlib.h>
#include <string.h>

#pragma intrinsic(memcpy, memset)

struct PaletteColor {
    unsigned char red;
    unsigned char green;
    unsigned char blue;
};

short gOldAsmAssertLine = 207;
char gOldAsmAssertFile[] = "D:\\Heroes\\Base\\OLDASM.CPP";

VA(0x00473820, 0x3a)
int Random(int low, int high) {
    ProcessAssert(high > low, gOldAsmAssertFile, gOldAsmAssertLine + 1);
    return rand() % (high - low + 1) + low;
}

// Called on the loaded kb.pal data before SetPalette.
VA(0x00473860, 0x60)
void PostprocessPalette(signed char* data) {
    PaletteColor* remapped = static_cast<PaletteColor*>(malloc(PALETTE_GRAPHICS_BYTES));
    memset(remapped, 0, PALETTE_GRAPHICS_BYTES);
    for (int index = 0; index < 256; index++)
        remapped[gMonoColorMap[index]] =
            reinterpret_cast<PaletteColor*>(data)[index]; // byte-evidenced: 3-byte colour copies
    memcpy(data, remapped, PALETTE_GRAPHICS_BYTES);
    free(remapped);
}

VA(0x004738c0, 0x1)
void PostprocessBitmap(signed char*, int, int) {}

VA(0x004738d0, 0x1)
void PostprocessIcon(icon*) {}

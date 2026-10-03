// HoMM1's packed palette resource; Buka 2.1 supplies the cache contract.

#include <match.h>

#include <BASE/palette.h>
#include <BASE/resourceManager.h>
#include <SOURCE/KB.h>

#include <stdlib.h>

VA(0x0047d4d0, 0x2b)
palette::palette(void) : resource(RESOURCE_CATEGORY_PALETTE, -1, RESOURCE_REFERENCE_INITIAL, NULL) {
    m_data = static_cast<i8*>(malloc(PALETTE_DATA_SIZE));
}

// VC4 emits this virtual deleting destructor from the ordinary destructor below.
VA_COMPGEN(0x0047d500, 0x33, "??_Gpalette@@UAEPAXI@Z", 0x0047d540)
VA(0x0047d540, 0x53)
palette::palette(i16 id)
    : resource(RESOURCE_CATEGORY_PALETTE, id, RESOURCE_REFERENCE_INITIAL, NULL) {
    m_data = static_cast<i8*>(malloc(PALETTE_DATA_SIZE));
    gpResourceManager->PointToFile(id);
    gpResourceManager->ReadBlock(m_data, PALETTE_DATA_SIZE);
}

palette::~palette(void) {
    free(m_data);
}

VA(0x0047d5a0, 0x4)
i8* palette::Data(void) {
    return m_data;
}

// The packed palette resource.

#include <match.h>

#include <BASE/palette.h>
#include <BASE/resourceManager.h>
#include <SOURCE/KB.h>

#include <stdlib.h>

VA(0x004747d0, 0x3a)
palette::palette(void) : resource(RESOURCE_CATEGORY_PALETTE, -1, RESOURCE_REFERENCE_INITIAL, NULL) {
    m_data = static_cast<i8*>(malloc(PALETTE_DATA_SIZE));
}

VA(0x0047480a, 0x93)
palette::palette(i16 id)
    : resource(RESOURCE_CATEGORY_PALETTE, id, RESOURCE_REFERENCE_INITIAL, NULL) {
    m_data = static_cast<i8*>(malloc(PALETTE_DATA_SIZE));
    gpResourceManager->PointToFile(id);
    gpResourceManager->ReadBlock(m_data, PALETTE_DATA_SIZE);
}

VA(0x0047489d, 0x2b)
palette::~palette(void) {
    free(m_data);
}

VA(0x004748c8, 0x11)
i8* palette::Data(void) {
    return m_data;
}

VA_COMPGEN(0x00474920, 0x2e, "??_Gpalette@@UAEPAXI@Z", 0x0047480a)

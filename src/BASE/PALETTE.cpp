#include <H1/Ints.h>

#include <BASE/palette.h>
#include <BASE/resourceManager.h>
#include <SOURCE/KB.h>

#include <stdlib.h>

palette::palette(void) : resource(RESOURCE_CATEGORY_PALETTE, -1, RESOURCE_REFERENCE_INITIAL, NULL) {
    m_data = static_cast<i8*>(malloc(PALETTE_DATA_SIZE));
}

palette::palette(i16 id)
    : resource(RESOURCE_CATEGORY_PALETTE, id, RESOURCE_REFERENCE_INITIAL, NULL) {
    m_data = static_cast<i8*>(malloc(PALETTE_DATA_SIZE));
    gResourceManager->PointToFile(id);
    gResourceManager->ReadBlock(m_data, PALETTE_DATA_SIZE);
}

palette::~palette(void) {
    free(m_data);
}

i8* palette::Data(void) {
    return m_data;
}

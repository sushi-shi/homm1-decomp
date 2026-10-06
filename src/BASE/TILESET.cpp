// Packed tileset loader.

#include <match.h>

#include <BASE/bitmap.h>
#include <BASE/miscwin.h>
#include <BASE/resourceManager.h>
#include <BASE/tileset.h>
#include <SOURCE/KB.h>

#include <stdlib.h>

VA(0x00474ea0, 0x11c)
tileset::tileset(i16 id)
    : resource(RESOURCE_CATEGORY_TILESET, id, RESOURCE_REFERENCE_INITIAL, NULL) {
    gResourceManager->PointToFile(id);
    m_tileCount = gResourceManager->ReadWord();
    m_tileWidth = gResourceManager->ReadWord();
    m_tileHeight = gResourceManager->ReadWord();
    i32 size = m_tileCount * m_tileWidth * m_tileHeight;
    m_data = static_cast<u8*>(malloc(size));
    gResourceManager->ReadBlock(m_data, size);
    PostprocessBitmap(m_data, m_tileWidth, m_tileHeight * m_tileCount);
}

VA(0x00474fbc, 0x2b)
tileset::~tileset(void) {
    free(m_data);
}

VA_COMPGEN(0x00475020, 0x2e, "??_Gtileset@@UAEPAXI@Z", 0x00474ea0)

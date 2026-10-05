// HoMM1's packed tileset loader; Buka 2.1 supplies the resource contract.

#include <match.h>

#include <BASE/bitmap.h>
#include <BASE/miscwin.h>
#include <BASE/resourceManager.h>
#include <BASE/tileset.h>
#include <SOURCE/KB.h>

#include <stdlib.h>

VA_COMPGEN(0x00474fbc, 0x2b, "??1tileset@@UAE@XZ", 0x00474ea0)
VA(0x00474ea0, 0x11c)
tileset::tileset(i16 id)
    : resource(RESOURCE_CATEGORY_TILESET, id, RESOURCE_REFERENCE_INITIAL, NULL) {
    gpResourceManager->PointToFile(id);
    m_tileCount = gpResourceManager->ReadWord();
    m_tileWidth = gpResourceManager->ReadWord();
    m_tileHeight = gpResourceManager->ReadWord();
    i32 size = m_tileCount * m_tileWidth * m_tileHeight;
    m_data = static_cast<i8*>(malloc(size));
    gpResourceManager->ReadBlock(m_data, size);
    PostprocessBitmap(m_data, m_tileWidth, m_tileHeight * m_tileCount);
}

VA_COMPGEN(0x00475020, 0x2e, "??_Gtileset@@UAEPAXI@Z", 0x00474ea0)
tileset::~tileset(void) {
    free(m_data);
}

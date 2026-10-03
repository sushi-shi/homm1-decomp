// HoMM1's packed tileset loader; Buka 2.1 supplies the resource contract.

#include <match.h>

#include <BASE/bitmap.h>
#include <BASE/miscwin.h>
#include <BASE/resourceManager.h>
#include <BASE/tileset.h>
#include <SOURCE/KB.h>

#include <stdlib.h>

VA(0x0047ff40, 0xb0)
tileset::tileset(i16 id)
    : resource(RESOURCE_CATEGORY_TILESET, id, RESOURCE_REFERENCE_INITIAL, NULL) {
    gpResourceManager->PointToFile(id);
    m_tileCount = gpResourceManager->ReadWord();
    m_tileWidth = gpResourceManager->ReadWord();
    m_tileHeight = gpResourceManager->ReadWord();
    i32 size = m_tileCount * m_tileWidth * m_tileHeight;
    m_data = static_cast<i8*>(malloc(size));
    gpResourceManager->ReadBlock(m_data, size);
    PostprocessBitmap(m_data, m_tileWidth, m_tileCount * m_tileHeight);
}

VA_COMPGEN(0x0047fff0, 0x33, "??_Gtileset@@UAEPAXI@Z", 0x0047ff40)
tileset::~tileset(void) {
    free(m_data);
}

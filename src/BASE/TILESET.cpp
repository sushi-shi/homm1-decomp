#include <H1/Ints.h>

#include <BASE/bitmap.h>
#include <BASE/miscwin.h>
#include <BASE/resourceManager.h>
#include <BASE/tileset.h>
#include <SOURCE/KB.h>

#include <stdlib.h>

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

tileset::~tileset(void) {
    free(m_data);
}

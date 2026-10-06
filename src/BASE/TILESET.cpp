#include <H1/Ints.h>

#include <BASE/bitmap.h>
#include <BASE/miscwin.h>
#include <BASE/resourceManager.h>
#include <BASE/tileset.h>
#include <SOURCE/KB.h>

#include <stdlib.h>

tileset::tileset(i16 id)
    : resource(RESOURCE_CATEGORY_TILESET, id, RESOURCE_REFERENCE_INITIAL, NULL) {
    u32 entrySize = gResourceManager->GetFileSize(id);
    gResourceManager->PointToFile(id);
    m_tileCount = gResourceManager->ReadWord();
    m_tileWidth = gResourceManager->ReadWord();
    m_tileHeight = gResourceManager->ReadWord();
    // The tiles follow the count, width and height in the archive entry.
    if (entrySize < TILESET_HEADER_SIZE
        || (m_tileWidth != 0 && m_tileHeight != 0
            && m_tileCount > (entrySize - TILESET_HEADER_SIZE) / m_tileWidth / m_tileHeight))
        gResourceManager->InvalidResource(id);
    i32 size = m_tileCount * m_tileWidth * m_tileHeight;
    m_data = static_cast<u8*>(malloc(size));
    gResourceManager->ReadBlock(m_data, size);
    PostprocessBitmap(m_data, m_tileWidth, m_tileHeight * m_tileCount);
}

tileset::~tileset(void) {
    free(m_data);
}

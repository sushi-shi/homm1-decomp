#ifndef HOMM1_BASE_TILESET_H
#define HOMM1_BASE_TILESET_H

#include <BASE/resource.h>

enum TilesetFileConstant {
    // The tile count, width and height before a tileset's tiles.
    TILESET_HEADER_SIZE = 6
};

#pragma pack(push, 1)
class tileset : public resource {
public:
    u16 m_tileCount;
    u16 m_tileWidth;
    u16 m_tileHeight;
    u8* m_data;
    tileset(i16 id);
    virtual ~tileset();
};
#pragma pack(pop)
#endif

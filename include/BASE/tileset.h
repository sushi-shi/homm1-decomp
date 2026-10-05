#ifndef HOMM1_BASE_TILESET_H
#define HOMM1_BASE_TILESET_H

#include <BASE/resource.h>

#pragma pack(push, 1)
class tileset : public resource {
public:
    u16 m_tileCount;
    u16 m_tileWidth;
    u16 m_tileHeight;
    u8* m_data;
    // --- constructors ---
    tileset(i16 id);
    virtual ~tileset();
};
#pragma pack(pop)
#endif // HOMM1_BASE_TILESET_H

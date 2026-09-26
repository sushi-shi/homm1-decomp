#ifndef HOMM1_BASE_TILESET_H
#define HOMM1_BASE_TILESET_H
// Reconstructed class (BASE) from CodeView NB09 of HEROES2W.EXE — NOT original source.
// 3 methods, 0 own-virtual, 0 static data.

#include <BASE/resource.h>

#pragma pack(push, 1)
class tileset : public resource {
public:
    unsigned short m_tileCount;
    unsigned short m_tileWidth;
    unsigned short m_tileHeight;
    signed char *m_data;
    // --- constructors ---
    tileset(short);
    virtual inline ~tileset();
};
#pragma pack(pop)
#endif // HOMM1_BASE_TILESET_H

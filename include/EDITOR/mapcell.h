#ifndef HOMM1_EDITOR_MAPCELL_H
#define HOMM1_EDITOR_MAPCELL_H

#include <H1/Ints.h>

#pragma pack(push, 1)
struct mapCellExtra {
    u16 index;
    u8 objFlag : 1;
    u8 objTileset : 7;
    u8 objIndex;
    u8 f4a : 1;
    u8 f4b : 1;
    u8 f4c : 1;
    u8 f4hi : 5;
    u8 ovlFlag0 : 1;
    u8 ovlFlag1 : 1;
    u8 ovlTileset : 6;
    u8 ovlIndex;
};
#pragma pack(pop)

class mapCell {
public:
    u16 tile;
    u8 objFlag0 : 1;
    u8 objFlag1 : 1;
    u8 objTileset : 6;
    u8 objIndex;
    u16 w4a : 1;
    u16 w4b : 1;
    u16 w4c : 1;
    u16 w4hi : 13;
    u8 ovlFlag0 : 1;
    u8 ovlFlag1 : 1;
    u8 ovlTileset : 6;
    u8 ovlIndex;
    u16 unk8;
    u16 extra;
};

struct oldMapCell {
    u8 raw[20];
};
struct oldMapCellExtra {
    u8 raw[15];
};
#endif

#ifndef HOMM1_SOURCE_MAPCELL_H
#define HOMM1_SOURCE_MAPCELL_H
// HoMM1 adventure-map cell. advManager::GetCell's x*720 + y*10 addressing
// and game::GetWorldMapData's embedded 72x72 map prove a ten-byte record;
// the editor's twelve-byte mapCell (EDITOR/mapcell.h) adds an extra chain.

#include <Domains.h>

// clang-format off
H1_ENUM_BEGIN(MapCellConstant)
    MAP_CELL_GRID_SIZE = 72
H1_ENUM_END(MapCellConstant)
// clang-format on

#pragma pack(push, 1)
class mapCell {
public:
    // Tile index read zero-extended into the terrain lookup table.
    unsigned char m_tileIndex;
    char m_unknown01[7];
    // Object type in the low seven bits; bit seven marks an event cell.
    unsigned char m_objType : 7;
    unsigned char m_hasEvent : 1;
    unsigned char m_objExtra;
};
#pragma pack(pop)

#endif // HOMM1_SOURCE_MAPCELL_H

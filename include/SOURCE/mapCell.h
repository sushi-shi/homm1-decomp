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
    // PuzzleDraw masks the object and overlay tileset low nibbles and their
    // 0xff-terminated frame indices.
    unsigned char m_objectTileset;
    unsigned char m_objectIndex;
    unsigned char m_overlayTileset;
    unsigned char m_overlayIndex;
    // DrawCell draws this overlay frame zero-extended.
    unsigned char m_unknown05;
    // Bit 6 marks the hero cursor's cell; DemobilizeCurrHero clears it.
    unsigned char m_flags;
    unsigned char m_unknown07;
    // Whole-byte trigger: readers mask the low seven type bits and the
    // 0x80 event bit; DemobilizeCurrHero stores the hero trigger directly.
    unsigned char m_triggerType;
    // Object instance (town, hero, mine, guard index); 109 of 119 retail
    // readers zero-extend it.
    unsigned char m_objectMetadata;
};
#pragma pack(pop)

#endif // HOMM1_SOURCE_MAPCELL_H

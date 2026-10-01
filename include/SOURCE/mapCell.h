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
    char m_unknown01;
    // XformToCastle shifts the town's lower rows by twelve object frames
    // and its top row by twelve overlay frames.
    unsigned char m_objectIndex;
    char m_unknown03;
    unsigned char m_overlayIndex;
    char m_unknown05;
    // Bit 6 marks the hero cursor's cell; DemobilizeCurrHero clears it.
    unsigned char m_flags;
    unsigned char m_unknown07;
    // Whole-byte trigger: readers mask the low seven type bits and the
    // 0x80 event bit; DemobilizeCurrHero stores the hero trigger directly.
    unsigned char m_triggerType;
    unsigned char m_objectMetadata;
};
#pragma pack(pop)

#endif // HOMM1_SOURCE_MAPCELL_H

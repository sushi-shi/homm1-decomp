#ifndef HOMM1_SOURCE_MAPCELL_H
#define HOMM1_SOURCE_MAPCELL_H
// HoMM1 adventure-map cell. advManager::GetCell's x*720 + y*10 addressing
// and game::GetWorldMapData's embedded 72x72 map prove a ten-byte record;
// the editor's twelve-byte mapCell (EDITOR/mapcell.h) adds an extra chain.

#include <Domains.h>

// clang-format off
H1_ENUM_CONST_BEGIN(MapCellConstant)
    MAP_CELL_GRID_SIZE = 72,
    // An object or overlay index of 0xff draws no frame (PuzzleDraw,
    // SettleOverlay, ProcessOnMapHeroes).
    MAP_CELL_NO_FRAME = 0xff,
    // m_objectTileset/m_overlayTileset: the low nibble selects the frame's
    // tileset, the high nibble the extra sprite's (DrawCell, ViewWorld).
    MAP_CELL_TILESET_MASK = 0x0f,
    MAP_CELL_EXTRA_TILESET_SHIFT = 4,
    // m_secondaryTrigger bit 7: the cell blocks pathing (DoDimensionDoor and
    // ViewWorld's dimension-door preview skip it).
    MAP_CELL_SECONDARY_BLOCKED = 0x80
H1_ENUM_CONST_END(MapCellConstant)

// m_flags (DrawCell, ViewWorld, ComboDraw, FINDPATH, the hero cursor). The two
// low bits become the ground tile's flip bits 14/15 (TileToBitmap); ViewWorld
// mirrors bit 1 and switches to the pre-flipped row for bit 0. Bits 2/3
// animate the object/overlay frame, bits 4/5 add the high-nibble tileset's
// extra sprite (owner flags, a mine's resource), bit 6 marks the hero cursor
// cell and bit 7 draws the object with the ground (Buka's shadow-only bit;
// pathing ignores such objects).
H1_ENUM_BEGIN(MapCellFlag)
    MAP_CELL_GROUND_FLIP_VERTICAL = 0x01,
    MAP_CELL_GROUND_FLIP_HORIZONTAL = 0x02,
    MAP_CELL_OBJECT_ANIMATED = 0x04,
    MAP_CELL_OVERLAY_ANIMATED = 0x08,
    MAP_CELL_OBJECT_EXTRA = 0x10,
    MAP_CELL_OVERLAY_EXTRA = 0x20,
    MAP_CELL_HERO_CURSOR = 0x40,
    MAP_CELL_OBJECT_SHADOW_ONLY = 0x80
H1_ENUM_END(MapCellFlag)

// Adventure object tilesets: advManager's m_objectIcons slots, loaded from
// these ICN files in the constructor (Buka's TilesetId keeps 12..17, 19, 20).
H1_ENUM_BEGIN(MapTileset)
    TILESET_OBJ32_00 = 0,
    TILESET_OBJ32_01 = 1,
    TILESET_OBJ32_02 = 2,
    TILESET_OBJ32_03 = 3,
    TILESET_OBJ32_04 = 4,
    TILESET_OBJ32_05 = 5,
    TILESET_OBJ32_06 = 6,
    TILESET_OBJ32_07 = 7,
    TILESET_MTN32 = 8,
    TILESET_TREE32 = 9,
    TILESET_TOWN32 = 10,
    TILESET_RSRC32 = 11,
    TILESET_MONS32 = 12,
    TILESET_ART32 = 13,
    TILESET_FLAG32 = 14,
    TILESET_RESSMALL = 15,
    TILESET_HOURGLAS = 16,
    TILESET_ROUTE = 17,
    TILESET_SMCREST = 18,
    TILESET_STONBACK = 19,
    TILESET_MINIMON = 20
H1_ENUM_END(MapTileset)
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
    // Frame of the extra sprite DrawCell adds from the high-nibble tileset:
    // town and mine owner flags (flags 0x10/0x20) and a mine's resource icon.
    unsigned char m_extraFrame;
    // Bit 6 marks the hero cursor's cell; DemobilizeCurrHero clears it.
    unsigned char m_flags;
    // A second trigger sharing the cell (low seven bits): EraseObj promotes it
    // when the primary object goes. Bit 7 blocks pathing; map setup sets it
    // where an object continues into the next cell.
    unsigned char m_secondaryTrigger;
    // Whole-byte trigger: readers mask the low seven type bits and the
    // 0x80 event bit; DemobilizeCurrHero stores the hero trigger directly.
    unsigned char m_triggerType;
    // Object instance (town, hero, mine, guard index); 109 of 119 retail
    // readers zero-extend it.
    unsigned char m_objectMetadata;
};
#pragma pack(pop)

#endif // HOMM1_SOURCE_MAPCELL_H

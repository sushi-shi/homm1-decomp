#ifndef HOMM1_SOURCE_MAPCELL_H
#define HOMM1_SOURCE_MAPCELL_H

enum MapCellConstant {
    MAP_CELL_GRID_SIZE = 72,
    MAP_CELL_NO_FRAME = 0xff,
    MAP_CELL_TILESET_MASK = 0x0f,
    MAP_CELL_EXTRA_TILESET_SHIFT = 4,
    MAP_CELL_SECONDARY_BLOCKED = 0x80,
    MAP_CELL_TILES_PER_TERRAIN = 20,
    MAP_CELL_GROUND_TILE_COUNT = 140,
    MAP_CELL_GROUND_FLIP_SHIFT = 14
};

enum MapCellFlag {
    MAP_CELL_GROUND_FLIP_VERTICAL = 0x01,
    MAP_CELL_GROUND_FLIP_HORIZONTAL = 0x02,
    MAP_CELL_OBJECT_ANIMATED = 0x04,
    MAP_CELL_OVERLAY_ANIMATED = 0x08,
    MAP_CELL_OBJECT_EXTRA = 0x10,
    MAP_CELL_OVERLAY_EXTRA = 0x20,
    MAP_CELL_HERO_CURSOR = 0x40,
    MAP_CELL_OBJECT_SHADOW_ONLY = 0x80
};

enum MapTileset {
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
};

#pragma pack(push, 1)
class mapCell {
public:
    u8 m_tileIndex;
    u8 m_objectTileset;
    u8 m_objectIndex;
    u8 m_overlayTileset;
    u8 m_overlayIndex;
    u8 m_extraFrame;
    u8 m_flags;
    u8 m_secondaryTrigger;
    u8 m_triggerType;
    u8 m_objectMetadata;
};
#pragma pack(pop)

#endif

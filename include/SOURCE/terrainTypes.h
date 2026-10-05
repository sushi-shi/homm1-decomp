#ifndef HOMM1_SOURCE_TERRAINTYPES_H
#define HOMM1_SOURCE_TERRAINTYPES_H

#include <Domains.h>

// giGroundToTerrain's values: the order of retail gTerrainNames
// (0x00493240), which advManager::QuickInfo prints for a bare cell; the
// water terrain prints as "Ocean". combatManager::LoadIcons picks its
// ground and obstacle icons by this index. searchArray::TestPossibleDirections
// fills a direction it cannot step to with TERRAIN_INVALID, which
// FindNearestObject and SeedPosition skip.
H1_ENUM_BEGIN(TerrainType)
    TERRAIN_INVALID = -1,
    TERRAIN_WATER = 0,
    TERRAIN_GRASS = 1,
    TERRAIN_SNOW = 2,
    TERRAIN_SWAMP = 3,
    TERRAIN_LAVA = 4,
    TERRAIN_DESERT = 5,
    TERRAIN_DIRT = 6,
    // Terrains after WATER_LAST are land (philAI's embark/landing tests).
    TERRAIN_WATER_LAST = TERRAIN_WATER
H1_ENUM_END(TerrainType)

#endif // HOMM1_SOURCE_TERRAINTYPES_H

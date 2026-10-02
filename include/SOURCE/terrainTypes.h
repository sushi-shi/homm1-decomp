#ifndef HOMM1_SOURCE_TERRAINTYPES_H
#define HOMM1_SOURCE_TERRAINTYPES_H

#include <Domains.h>

// clang-format off
// giGroundToTerrain's values: the order of retail gTerrainNames
// (0x00493428), which advManager::QuickInfo prints for a bare cell. HoMM1
// prints the water terrain as "Ocean"; HoMM2 Buka's TerrainType keeps the
// same seven values and names it water. combatManager::LoadIcons picks its
// ground and obstacle icons by this index.
H1_ENUM_BEGIN(TerrainType)
    TERRAIN_WATER = 0,
    TERRAIN_GRASS = 1,
    TERRAIN_SNOW = 2,
    TERRAIN_SWAMP = 3,
    TERRAIN_LAVA = 4,
    TERRAIN_DESERT = 5,
    TERRAIN_DIRT = 6
H1_ENUM_END(TerrainType)
// clang-format on

#endif // HOMM1_SOURCE_TERRAINTYPES_H

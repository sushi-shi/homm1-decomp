#ifndef HOMM1_EDITOR_RANDOMMAP_H
#define HOMM1_EDITOR_RANDOMMAP_H

// The random map generator's own types (src/EDITOR/MAPOBJ.cpp); its steps
// are editManager methods. The names are descriptive.

#include <Domains.h>

// A cell offset.
struct mapStep {
    i32 x;
    i32 y;
};

// The chain tables keep their offsets as plain pairs: an array of 8-byte
// structs would take 8-byte alignment, and retail starts MAPOBJ's .data at a
// 4-byte boundary.
H1_ENUM_CONST_BEGIN(MapStepAxis)
    MAP_STEP_X = 0,
    MAP_STEP_Y = 1,
    MAP_STEP_AXES = 2
H1_ENUM_CONST_END(MapStepAxis)

// PlaceTowns: castle slots (one per player), the land regions it numbers and
// the object classes a road between castles erases.
H1_ENUM_CONST_BEGIN(TownPlacementConstant)
    RANDOM_MAP_CASTLE_SLOTS = 4,
    RANDOM_MAP_REGION_LIMIT = 255,
    RANDOM_MAP_ROAD_CLEAR_MASK = 0xfc7f
H1_ENUM_CONST_END(TownPlacementConstant)

H1_ENUM_CONST_BEGIN(RandomMapConstant)
// GenerateRandomMap retries a map without enough castles this often.
    RANDOM_MAP_ATTEMPTS = 5,
    // overlayType::terrainMask of an object placeable on every terrain.
    RANDOM_MAP_ANY_TERRAIN = 0xfe,
    // PaintRandomTerrain's percent that covers the whole map.
    RANDOM_MAP_FULL_PERCENT = 100,
    // PaintRandomTerrain drifts a seed every eighth step and its walk
    // weights every 64th.
    RANDOM_MAP_SEED_DRIFT_MASK = 7,
    RANDOM_MAP_WEIGHT_DRIFT_MASK = 0x3f,
    // HasEnoughCastles: the castle frames of the four town32.icn towns.
    RANDOM_MAP_KNIGHT_CASTLE_FRAME = 22,
    RANDOM_MAP_BARBARIAN_CASTLE_FRAME = 46,
    RANDOM_MAP_SORCERESS_CASTLE_FRAME = 70,
    RANDOM_MAP_WARLOCK_CASTLE_FRAME = 94,
    RANDOM_MAP_MIN_CASTLES = 4
H1_ENUM_CONST_END(RandomMapConstant)

// PlaceResourceSite's kinds (PlaceRandomObjects' Random(0, 6)): kinds from
// RANDOM_MAP_SITE_FIRST_MINE on are the mines of gMineSiteKinds' resources.
H1_ENUM_BEGIN(RandomMapSiteKind)
    RANDOM_MAP_SITE_SAWMILL = 0,
    RANDOM_MAP_SITE_ALCHEMIST_LAB = 1,
    RANDOM_MAP_SITE_FIRST_MINE = 2,
    RANDOM_MAP_SITE_KIND_COUNT = 7
H1_ENUM_END(RandomMapSiteKind)

// Where PlaceTreasures guards a treasure: the diagonal cell of a corner whose
// two sides are blocked.
H1_ENUM_BEGIN(TreasureGuard)
    TREASURE_UNGUARDED = 0,
    TREASURE_GUARD_NE = 1,
    TREASURE_GUARD_SE = 2,
    TREASURE_GUARD_SW = 3,
    TREASURE_GUARD_NW = 4,
    // Layouts above this one place a guard.
    TREASURE_UNGUARDED_LAST = TREASURE_UNGUARDED
H1_ENUM_END(TreasureGuard)

// The eight directions a mountain or tree chain runs (gChainSteps): even
// directions climb two rows per column, odd ones one.
H1_ENUM_BEGIN(ChainDirection)
    CHAIN_UP_RIGHT_STEEP = 0,
    CHAIN_UP_RIGHT = 1,
    CHAIN_DOWN_RIGHT_STEEP = 2,
    CHAIN_DOWN_RIGHT = 3,
    CHAIN_DOWN_LEFT_STEEP = 4,
    CHAIN_DOWN_LEFT = 5,
    CHAIN_UP_LEFT_STEEP = 6,
    CHAIN_UP_LEFT = 7,
    // Directions below this marker run rightwards.
    CHAIN_RIGHTWARD_END = CHAIN_DOWN_LEFT_STEEP,
    CHAIN_DIRECTION_COUNT = 8
H1_ENUM_END(ChainDirection)

// PlaceObstacleChains' roll for a tree chain's family (the first letter of
// its objects' names: autumn, pine or deciduous trees).
H1_ENUM_BEGIN(ChainTreeFamily)
    CHAIN_TREE_AUTUMN = 0,
    CHAIN_TREE_PINE = 1,
    CHAIN_TREE_DECIDUOUS = 2
H1_ENUM_END(ChainTreeFamily)

#endif // HOMM1_EDITOR_RANDOMMAP_H

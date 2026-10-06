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

H1_ENUM_CONST_BEGIN(RandomMapConstant)
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

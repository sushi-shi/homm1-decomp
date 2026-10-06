#ifndef HOMM1_EDITOR_RANDOMMAP_H
#define HOMM1_EDITOR_RANDOMMAP_H

// The random map generator's own types (src/EDITOR/MAPOBJ.cpp); its steps
// are editManager methods. The names are descriptive.

#include <Domains.h>
#include <SOURCE/mapCell.h>

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

// A cell of a malloc'ed MAP_CELL_GRID_SIZE x MAP_CELL_GRID_SIZE byte grid:
// RemoveSmallRegions' visited and region marks, PlaceTowns' region numbers
// and each castle's reach.
#define MAP_GRID_CELL(grid, x, y) (*((grid) + (x) + (y) * MAP_CELL_GRID_SIZE))

// PlaceTowns: castle slots (one per player) and the land regions it numbers.
H1_ENUM_CONST_BEGIN(TownPlacementConstant)
    RANDOM_MAP_CASTLE_SLOTS = 4,
    RANDOM_MAP_REGION_LIMIT = 255
H1_ENUM_CONST_END(TownPlacementConstant)

// The cells a placed town or castle clears and levels around its anchor: its
// TOWN_FOOTPRINT plus a column to the right and the row below the entrance.
H1_ENUM_CONST_BEGIN(TownSiteConstant)
    RANDOM_MAP_SITE_LEFT = 2,
    RANDOM_MAP_SITE_TOP = 2,
    RANDOM_MAP_SITE_RIGHT = 2,
    RANDOM_MAP_SITE_BOTTOM = 1,
    RANDOM_MAP_SITE_WIDTH = 5,
    RANDOM_MAP_SITE_HEIGHT = 4
H1_ENUM_CONST_END(TownSiteConstant)

H1_ENUM_CONST_BEGIN(RandomMapConstant)
// GenerateRandomMap retries a map without enough castles this often.
    RANDOM_MAP_ATTEMPTS = 5,
    // overlayType::terrainMask of an object placeable on every terrain.
    RANDOM_MAP_ANY_TERRAIN = 0xfe,
    // PaintRandomTerrain's percent that covers the whole map; densities and
    // land shares are percents.
    RANDOM_MAP_FULL_PERCENT = 100,
    // ScaleByDensity leaves a count unchanged at this density.
    RANDOM_MAP_NEUTRAL_DENSITY = 50,
    // PaintRandomTerrain drifts a seed every eighth step and its walk
    // weights every 64th.
    RANDOM_MAP_SEED_DRIFT_MASK = 7,
    RANDOM_MAP_WEIGHT_DRIFT_MASK = 0x3f,
    // PaintRandomTerrain gives up a seed search and a walk after these many
    // steps, and grows at most this many seeds.
    RANDOM_MAP_SEED_TRIES = 200,
    RANDOM_MAP_WALK_LIMIT = 1000,
    RANDOM_MAP_SEED_LIMIT = 20,
    // RemoveSmallRegions merges a region of at most this many cells.
    RANDOM_MAP_SMALL_REGION_SIZE = 15,
    // The placement loops budget this many tries per object (a mine gets
    // more) and spend an object's tries when it is placed.
    RANDOM_MAP_TRIES_PER_OBJECT = 100,
    RANDOM_MAP_TRIES_PER_MINE = 1000,
    // GenerateRandomMap's terrain index past TERRAIN_LAST once the base
    // terrain is painted.
    RANDOM_MAP_END_TERRAIN_SCAN = 99,
    // PlaceTowns: a castle on another continent than its peers walks its
    // approach towards water without a step limit.
    RANDOM_MAP_UNLIMITED_STEPS = 999,
    // PlaceChainLink: a tileset with no chain of the cell's terrain matches
    // no object's terrainMask.
    RANDOM_MAP_NO_CHAIN_TERRAIN = -1,
    // PlaceRandomObjects: the obelisk objects are named "obelisk<terrain>".
    RANDOM_MAP_OBELISK_NAME_LENGTH = 7,
    // gMineSiteKinds: the resources of the five mines' resource marker
    // frames.
    RANDOM_MAP_MINE_RESOURCE_COUNT = 5
H1_ENUM_CONST_END(RandomMapConstant)

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

// A chain turns a quarter (two directions) clockwise or counterclockwise:
// gChainTurns' second index (the turn's sideways shift), and what
// PlaceObstacleChains adds to the direction modulo CHAIN_DIRECTION_COUNT.
H1_ENUM_BEGIN(ChainTurn)
    CHAIN_TURN_CLOCKWISE = 0,
    CHAIN_TURN_COUNTERCLOCKWISE = 1,
    CHAIN_TURN_COUNT = 2
H1_ENUM_END(ChainTurn)

H1_ENUM_CONST_BEGIN(ChainTurnConstant)
    CHAIN_CLOCKWISE_STEP = 10,
    CHAIN_COUNTERCLOCKWISE_STEP = 6
H1_ENUM_CONST_END(ChainTurnConstant)

// The four objects of a mountain or tree chain, in the object table from the
// one of the cell's terrain: a direction and its opposite draw one slope.
H1_ENUM_BEGIN(ChainPiece)
    CHAIN_PIECE_STEEP_RISING = 0,
    CHAIN_PIECE_RISING = 1,
    CHAIN_PIECE_STEEP_FALLING = 2,
    CHAIN_PIECE_FALLING = 3
H1_ENUM_END(ChainPiece)

// PlaceObstacleChains' roll for a tree chain's family (the first letter of
// its objects' names: autumn, pine or deciduous trees).
H1_ENUM_BEGIN(ChainTreeFamily)
    CHAIN_TREE_AUTUMN = 0,
    CHAIN_TREE_PINE = 1,
    CHAIN_TREE_DECIDUOUS = 2
H1_ENUM_END(ChainTreeFamily)

#endif // HOMM1_EDITOR_RANDOMMAP_H

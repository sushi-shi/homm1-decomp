#ifndef HOMM1_SOURCE_SEARCHARRAY_H
#define HOMM1_SOURCE_SEARCHARRAY_H

#include <Domains.h>
#include <H1/Macros.h>
#include <SOURCE/cursorTypes.h>
#include <SOURCE/mapCell.h>
#include <SOURCE/PATH.h>

// forward declarations:
class army;

H1_ENUM_CONST_BEGIN(SearchStorageConstant)
    SEARCH_QUEUE_CAPACITY = 1024,
    SEARCH_CELL_CAPACITY = MAP_CELL_GRID_SIZE * MAP_CELL_GRID_SIZE,
    SEARCH_FLAG_BIT_COUNT = 1,
    SEARCH_DIRECTION_BIT_COUNT = 4,
    SEARCH_PATH_CAPACITY = 256
H1_ENUM_CONST_END(SearchStorageConstant)

// A step is diagonal when bit 0 of its direction kind is set
// (TerrainStepCost); which cell's object blocks a step is cursorTypes.h
// MapDirectionMask (TestPossibleDirections, SeedPosition).
H1_ENUM_CONST_BEGIN(SearchDirectionConstant)
    SEARCH_DIAGONAL_COST_MASK = 1
H1_ENUM_CONST_END(SearchDirectionConstant)

// SeedPosition's cost bookkeeping (on the 4/6/8 step-cost scale): MAX_COST
// starts the best cost to an explicit target, TARGET_COST_WINDOW (one straight
// step) stops the flood once nothing cheaper can reach it and lets a continued
// seed return early, MONSTER_RESEED_WINDOW (three straight steps) skips neighbours an
// adjacent-monster node already reached that cheaply. INVALID_COORDINATE is
// the no-target / no-monster coordinate (m_specialTargetX/Y, PushPoint's
// value/previous bytes, FindAdjacentMonster's excluded monster).
// FindNearestObject floods with a working mobility of NEAREST_OBJECT_MOBILITY.
// UNLIMITED_COST (999) is a cost cap or mobility no route reaches: the
// maximumCost advManager passes to SeedPosition/BuildPath for the cursor
// route, philAI's seed mobility, and FindNearestObject's per-step mobility
// (CalcTerrainCost then always charges the full diagonal cost).
H1_ENUM_CONST_BEGIN(SearchConstant)
    SEARCH_INVALID_COORDINATE = -1,
    // maximumCost <= 0 imposes no cost cap (FindNearestObject, SeedPosition
    // test maximumCost > 0); EVENTS' signpost search passes -1.
    SEARCH_NO_COST_LIMIT = -1,
    SEARCH_TARGET_COST_WINDOW = 4,
    SEARCH_MONSTER_RESEED_WINDOW = 12,
    SEARCH_NEAREST_OBJECT_MOBILITY = 500,
    SEARCH_UNLIMITED_COST = 999,
    SEARCH_MAX_COST = 9999
H1_ENUM_CONST_END(SearchConstant)

// Packs the direction nibble under a 12-bit distance in the word at +2
// (CheckReload shifts it right four; the path builder masks 0xf).
#pragma pack(push, 1)
struct searchNode {
    // BuildPath sign-extends both coordinates.
    i8 x;
    i8 y;
    u16 direction : SEARCH_DIRECTION_BIT_COUNT;
    u16 distance : 12;
    u8 visited : SEARCH_FLAG_BIT_COUNT;
    // TestPossibleDirections' occupancy for the step that pushed the node;
    // SeedPosition then inspects the cell's trigger.
    u8 occupied : SEARCH_FLAG_BIT_COUNT;
    // The route steps next to a wandering monster at adjacentMonsterX/Y.
    u8 hasAdjacentMonster : SEARCH_FLAG_BIT_COUNT;
    // The route needs more than this turn's mobility; turnEndX/Y is the last
    // cell it reaches this turn. DetermineTargetPosition passes bits 3..7 to
    // RVOfPosition as a byte.
    u8 beyondTurnMobility : 5;
    // SeedPosition sign-extends the adjacent monster coordinates.
    i8 adjacentMonsterX;
    i8 adjacentMonsterY;
    i8 turnEndX;
    i8 turnEndY;
};

class searchArray {
public:
    u32 m_queueCount;
    u32 m_maxQueueCount;
    i32 m_pathLength;
    i32 m_specialTargetX;
    i32 m_specialTargetY;
    searchNode m_queue[SEARCH_QUEUE_CAPACITY];
    // Retail indexes the [x][y] grid with a 648-byte row stride.
    searchNode m_cells[MAP_CELL_GRID_SIZE][MAP_CELL_GRID_SIZE];
    // Retail DoDimensionDoor reads the path directions at +0xda54.
    u8 m_directions[SEARCH_PATH_CAPACITY];
    // --- constructors ---
    searchArray(void);
    // --- methods ---
    i32 BuildPath(i16 startX, i16 startY, i16 destinationX, i16 destinationY, i16 maximumCost);
    void SeedPosition(
        i16 seedX,
        i16 seedY,
        i16 seedDirection,
        i16 maximumCost,
        i32 waterMode,
        b32 findAdjacentMonster,
        i32 mobility,
        i32 heroClass,
        i32 targetX,
        i32 targetY,
        b32 continueSeed,
        b32 scanMap
    );
    // Seeds from a hero and builds the path to the nearest cell carrying the
    // trigger type (EVENTS finds a town with 0xa8).
    i16 FindNearestObject(i16 startX, i16 startY, i16 direction, i16 maximumCost, u8 triggerType);
    void Clear(void);
    i16 QuickDistance(i16 x1, i16 y1, i16 x2, i16 y2);
    void PushPoint(
        i16 x,
        i16 y,
        u16 direction,
        u16 cost,
        u16 maximumCost,
        i8 occupied,
        b8 hasAdjacentMonster,
        i8 adjacentMonsterX,
        i8 adjacentMonsterY,
        i8 beyondTurnMobility,
        i8 turnEndX,
        i8 turnEndY
    );
    void TestPossibleDirections(
        i16 x,
        i16 y,
        i8* const terrain,
        u8* const occupied,
        i16 allowOccupied,
        i32 waterMode
    );
    // attackPath is an ArmyPathTarget (PATH.h).
    i16 FindCombatPath(
        i16 sourceHex,
        i16 targetHex,
        class army* unit,
        H1_ENUM_PARAM(ArmyPathTarget, i8) attackPath
    );
    void PushCombatPoint(i16 hex, i16 direction, u16 distance, u16 speed);
};
#pragma pack(pop)
#define gFullySeeded gSearchSeedingComplete // spelling fixes .bss order
extern b32 gFullySeeded;

#endif // HOMM1_SOURCE_SEARCHARRAY_H

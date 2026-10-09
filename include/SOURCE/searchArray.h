#ifndef HOMM1_SOURCE_SEARCHARRAY_H
#define HOMM1_SOURCE_SEARCHARRAY_H

#include <SOURCE/cursorTypes.h>
#include <SOURCE/mapCell.h>
#include <SOURCE/PATH.h>

class army;

enum SearchStorageConstant {
    SEARCH_QUEUE_CAPACITY = 1024,
    SEARCH_CELL_CAPACITY = MAP_CELL_GRID_SIZE * MAP_CELL_GRID_SIZE,
    SEARCH_FLAG_BIT_COUNT = 1,
    SEARCH_DIRECTION_BIT_COUNT = 4,
    SEARCH_PATH_CAPACITY = 256
};

enum SearchDirectionConstant {
    SEARCH_DIAGONAL_COST_MASK = 1
};

enum SearchConstant {
    SEARCH_INVALID_COORDINATE = -1,
    SEARCH_NO_COST_LIMIT = -1,
    SEARCH_TARGET_COST_WINDOW = 4,
    SEARCH_MONSTER_RESEED_WINDOW = 12,
    SEARCH_NEAREST_OBJECT_MOBILITY = 500,
    SEARCH_UNLIMITED_COST = 999,
    SEARCH_MAX_COST = 9999
};

#pragma pack(push, 1)
struct searchNode {
    i8 x;
    i8 y;
    u16 direction : SEARCH_DIRECTION_BIT_COUNT;
    u16 distance : 12;
    u8 visited : SEARCH_FLAG_BIT_COUNT;
    u8 occupied : SEARCH_FLAG_BIT_COUNT;
    u8 hasAdjacentMonster : SEARCH_FLAG_BIT_COUNT;
    u8 beyondTurnMobility : 5;
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
    searchNode m_cells[MAP_CELL_GRID_SIZE][MAP_CELL_GRID_SIZE];
    u8 m_directions[SEARCH_PATH_CAPACITY];
    searchArray(void);
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
    i16 FindCombatPath(
        i16 sourceHex,
        i16 targetHex,
        class army* unit,
        i8 attackPath
    );
    void PushCombatPoint(i16 hex, i16 direction, u16 distance, u16 speed);
};
#pragma pack(pop)
extern b32 gFullySeeded;

#endif

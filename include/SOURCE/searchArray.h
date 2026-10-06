#ifndef HOMM1_SOURCE_SEARCHARRAY_H
#define HOMM1_SOURCE_SEARCHARRAY_H

class army;

enum SearchStorageConstant {
    SEARCH_QUEUE_CAPACITY = 1024,
    SEARCH_CELL_CAPACITY = 5184,
    SEARCH_GRID_SIZE = 72,
    SEARCH_FLAG_BIT_COUNT = 1,
    SEARCH_DIRECTION_BIT_COUNT = 4,
    SEARCH_PATH_CAPACITY = 256
};

enum SearchDirectionConstant {
    SEARCH_DIAGONAL_COST_MASK = 1,
    SEARCH_DIRECTION_EDGE_OBJECT_MASK = 0x83,
    SEARCH_DIRECTION_OBJECT_MASK = 0x38
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
    u8 rvFlag1 : SEARCH_FLAG_BIT_COUNT;
    u8 rvFlag2 : 5;
    union {
        struct {
            i8 adjacentMonsterX;
            i8 adjacentMonsterY;
            u8 previousFlags;
            u8 terrain;
        };
        struct {
            i8 valueX;
            i8 valueY;
            i8 previousX;
            i8 previousY;
        };
    };
};

class searchArray {
public:
    u32 m_queueCount;
    u32 m_maxQueueCount;
    i32 m_pathLength;
    i32 m_specialTargetX;
    i32 m_specialTargetY;
    searchNode m_queue[SEARCH_QUEUE_CAPACITY];
    searchNode m_cells[SEARCH_GRID_SIZE][SEARCH_GRID_SIZE];
    u8 m_directions[SEARCH_PATH_CAPACITY];
    searchArray(void);
    i32 BuildPath(i16 startX, i16 startY, i16 destinationX, i16 destinationY, i16 maximumCost);
    void SeedPosition(
        i16 seedX,
        i16 seedY,
        i16 seedDirection,
        i16 maximumCost,
        i32 waterMode,
        i32 findAdjacentMonster,
        i32 mobility,
        i32 costMode,
        i32 targetX,
        i32 targetY,
        i32 continueSeed,
        i32 scanMap
    );
    i16 FindNearestObject(i16 startX, i16 startY, i16 direction, i16 maximumCost, u8 triggerType);
    void Init(void);
    void Close(void);
    void Clear(void);
    i16 QuickDistance(i16 x1, i16 y1, i16 x2, i16 y2);
    void PushPoint(
        i16 x,
        i16 y,
        u16 direction,
        u16 cost,
        u16 mobility,
        i8 occupied,
        i8 rvFlag1,
        i8 valueX,
        i8 valueY,
        i8 rvFlag2,
        i8 previousX,
        i8 previousY
    );
    void TestPossibleDirections(
        i16 x,
        i16 y,
        i8* const terrain,
        i8* const occupied,
        i16 allowOccupied,
        i32 waterMode
    );
    void SeedCombatPosition(class army* unit);
    i16 FindCombatPath(i16 sourceHex, i16 targetHex, class army* unit, i8 attackPath);
    void PushCombatPoint(i16 hex, i16 direction, u16 distance, u16 speed);
};
#pragma pack(pop)
extern i32 gFullySeeded;

#endif

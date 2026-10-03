#ifndef HOMM1_SOURCE_SEARCHARRAY_H
#define HOMM1_SOURCE_SEARCHARRAY_H
// Reconstructed class (SOURCE) from CodeView NB09 of HEROES2W.EXE — NOT original source.
// 13 methods, 0 own-virtual, 0 static data.

#include <Domains.h>
#include <H1/Macros.h>

// forward declarations:
class army;

H1_ENUM_CONST_BEGIN(SearchStorageConstant)
    SEARCH_QUEUE_CAPACITY = 1024,
    SEARCH_CELL_CAPACITY = 5184,
    SEARCH_GRID_SIZE = 72,
    SEARCH_FLAG_BIT_COUNT = 1,
    SEARCH_DIRECTION_BIT_COUNT = 4,
    SEARCH_PATH_CAPACITY = 256
H1_ENUM_CONST_END(SearchStorageConstant)

// Direction bit masks over MapDirection (1 << direction): north, north-east
// and north-west steps test the current cell's object, south-east, south and
// south-west the next cell's (TestPossibleDirections, SeedPosition; Buka
// searchArray.h numbering). A step is diagonal when bit 0 of its direction
// kind is set (TerrainStepCost).
H1_ENUM_CONST_BEGIN(SearchDirectionConstant)
    SEARCH_DIAGONAL_COST_MASK = 1,
    SEARCH_DIRECTION_EDGE_OBJECT_MASK = 0x83,
    SEARCH_DIRECTION_OBJECT_MASK = 0x38
H1_ENUM_CONST_END(SearchDirectionConstant)

// SeedPosition's cost bookkeeping (Buka searchArray.h SearchConstant names,
// HoMM1 values on its 4/6/8 step-cost scale): MAX_COST starts the best cost
// to an explicit target, TARGET_COST_WINDOW (one straight step) stops the
// flood once nothing cheaper can reach it and lets a continued seed return
// early, MONSTER_RESEED_WINDOW (three straight steps) skips neighbours an
// adjacent-monster node already reached that cheaply. INVALID_COORDINATE is
// the no-target / no-monster coordinate (m_specialTargetX/Y, PushPoint's
// value/previous bytes, FindAdjacentMonster's excluded monster).
// FindNearestObject floods with a working mobility of NEAREST_OBJECT_MOBILITY.
// UNLIMITED_COST (999) is a cost cap or mobility no route reaches: the
// maximumCost advManager passes to SeedPosition/BuildPath for the cursor
// route, philAI's seed mobility, and FindNearestObject's per-step mobility
// (CalcTerrainCost then always charges the full diagonal cost). Buka's
// ADVMGR ROUTE_PATH_COST_LIMIT (59999) plays the same role.
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

// Donor searchNode's real packed record; HoMM1 stores nodes inline.
// HoMM1 packs the direction nibble under a 12-bit distance in the word at +2
// (CheckReload shifts it right four; the path builder masks 0xf).
#pragma pack(push, 1)
struct searchNode {
    // BuildPath sign-extends both coordinates.
    i8 x;
    i8 y;
    u16 direction : SEARCH_DIRECTION_BIT_COUNT;
    u16 distance : 12;
    u8 visited : SEARCH_FLAG_BIT_COUNT;
    // Buka unknownFlag: TestPossibleDirections' occupancy for the step that
    // pushed the node; SeedPosition then inspects the cell's trigger.
    u8 occupied : SEARCH_FLAG_BIT_COUNT;
    u8 rvFlag1 : SEARCH_FLAG_BIT_COUNT;
    // DetermineTargetPosition passes bits 3..7 to RVOfPosition as a byte.
    u8 rvFlag2 : 5;
    union {
        // SeedPosition sign-extends the adjacent monster coordinates.
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
    // Retail indexes the [x][y] grid with a 648-byte row stride.
    searchNode m_cells[SEARCH_GRID_SIZE][SEARCH_GRID_SIZE];
    // Retail DoDimensionDoor reads the path directions at +0xda54.
    u8 m_directions[SEARCH_PATH_CAPACITY];
    // --- constructors ---
    searchArray(void);
    // --- methods ---
    // HoMM1 retail 0x00402af0: word coordinates and cost cap (ret 0x14).
    i32 BuildPath(i16, i16, i16, i16, i16);
    // HoMM1 retail 0x00402be0: word seed and cost cap (ret 0x30).
    void SeedPosition(i16, i16, i16, i16, i32, i32, i32, i32, i32, i32, i32, i32);
    // HoMM1 retail 0x004028b0: seeds from a hero and builds the path to the
    // nearest cell carrying the trigger type (EVENTS finds a town with 0xa8).
    i16 FindNearestObject(i16, i16, i16, i16, u8);
    void Init(void);
    void Close(void);
    void Clear(void);
    i16 QuickDistance(i16, i16, i16, i16);
    // HoMM1 retail 0x00424d90 (ret 0x30): word x/y, unsigned word
    // direction/cost/mobility and byte flags and coordinates.
    void PushPoint(
        i16,
        i16,
        u16,
        u16,
        u16,
        i8,
        i8,
        i8,
        i8,
        i8,
        i8,
        i8
    );
    // HoMM1 retail 0x00425040 (ret 0x18): word coordinates and occupancy flag.
    void TestPossibleDirections(i16, i16, i8* const, i8* const, i16, i32);
    void SeedCombatPosition(class army* unit);
    // HoMM1 retail 0x00424950 takes four arguments (ret 0x10).
    // attackPath is an ArmyPathTarget (PATH.h).
    i16 FindCombatPath(i16, i16, class army*, i8);
    // HoMM1 retail 0x00424c50 (ret 0x10): word hex/direction and unsigned
    // word distance/speed.
    void PushCombatPoint(i16, i16, u16, u16);
};
#pragma pack(pop)
extern i32 gFullySeeded;

#endif // HOMM1_SOURCE_SEARCHARRAY_H

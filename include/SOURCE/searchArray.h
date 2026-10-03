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
    signed char x;
    signed char y;
    unsigned short direction : SEARCH_DIRECTION_BIT_COUNT;
    unsigned short distance : 12;
    unsigned char visited : SEARCH_FLAG_BIT_COUNT;
    // Buka unknownFlag: TestPossibleDirections' occupancy for the step that
    // pushed the node; SeedPosition then inspects the cell's trigger.
    unsigned char occupied : SEARCH_FLAG_BIT_COUNT;
    unsigned char rvFlag1 : SEARCH_FLAG_BIT_COUNT;
    // DetermineTargetPosition passes bits 3..7 to RVOfPosition as a byte.
    unsigned char rvFlag2 : 5;
    union {
        // SeedPosition sign-extends the adjacent monster coordinates.
        struct {
            signed char adjacentMonsterX;
            signed char adjacentMonsterY;
            unsigned char previousFlags;
            unsigned char terrain;
        };
        struct {
            signed char valueX;
            signed char valueY;
            signed char previousX;
            signed char previousY;
        };
    };
};

class searchArray {
public:
    unsigned int m_queueCount;
    unsigned int m_maxQueueCount;
    int m_pathLength;
    int m_specialTargetX;
    int m_specialTargetY;
    searchNode m_queue[SEARCH_QUEUE_CAPACITY];
    // Retail indexes the [x][y] grid with a 648-byte row stride.
    searchNode m_cells[SEARCH_GRID_SIZE][SEARCH_GRID_SIZE];
    // Retail DoDimensionDoor reads the path directions at +0xda54.
    unsigned char m_directions[SEARCH_PATH_CAPACITY];
    // --- constructors ---
    searchArray(void);
    // --- methods ---
    // HoMM1 retail 0x00402af0: word coordinates and cost cap (ret 0x14).
    int BuildPath(short, short, short, short, short);
    // HoMM1 retail 0x00402be0: word seed and cost cap (ret 0x30).
    void SeedPosition(short, short, short, short, int, int, int, int, int, int, int, int);
    // HoMM1 retail 0x004028b0: seeds from a hero and builds the path to the
    // nearest cell carrying the trigger type (EVENTS finds a town with 0xa8).
    short FindNearestObject(short, short, short, short, unsigned char);
    void Init(void);
    void Close(void);
    void Clear(void);
    short QuickDistance(short, short, short, short);
    // HoMM1 retail 0x00424d90 (ret 0x30): word x/y, unsigned word
    // direction/cost/mobility and byte flags and coordinates.
    void PushPoint(
        short,
        short,
        unsigned short,
        unsigned short,
        unsigned short,
        char,
        char,
        signed char,
        signed char,
        char,
        signed char,
        signed char
    );
    // HoMM1 retail 0x00425040 (ret 0x18): word coordinates and occupancy flag.
    void TestPossibleDirections(short, short, signed char* const, signed char* const, short, int);
    void SeedCombatPosition(class army* unit);
    // HoMM1 retail 0x00424950 takes four arguments (ret 0x10).
    // attackPath is an ArmyPathTarget (PATH.h).
    short FindCombatPath(short, short, class army*, signed char);
    // HoMM1 retail 0x00424c50 (ret 0x10): word hex/direction and unsigned
    // word distance/speed.
    void PushCombatPoint(short, short, unsigned short, unsigned short);
};
#pragma pack(pop)

// SeedPosition's seeding state.
extern int giSeedingValid;
#endif // HOMM1_SOURCE_SEARCHARRAY_H

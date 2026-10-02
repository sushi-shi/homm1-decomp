#ifndef HOMM1_SOURCE_SEARCHARRAY_H
#define HOMM1_SOURCE_SEARCHARRAY_H
// Reconstructed class (SOURCE) from CodeView NB09 of HEROES2W.EXE — NOT original source.
// 13 methods, 0 own-virtual, 0 static data.

#include <Domains.h>
#include <H1/Macros.h>

// forward declarations:
class army;

H1_ENUM_BEGIN(SearchStorageConstant)
    SEARCH_QUEUE_CAPACITY = 1024,
    SEARCH_CELL_CAPACITY = 5184,
    SEARCH_GRID_SIZE = 72,
    SEARCH_FLAG_BIT_COUNT = 1,
    SEARCH_DIRECTION_BIT_COUNT = 4,
    SEARCH_PATH_CAPACITY = 256
H1_ENUM_END(SearchStorageConstant)

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
    void PushPoint(short, short, unsigned short, unsigned short, unsigned short, char, char, signed char,
                   signed char, char, signed char, signed char);
    // HoMM1 retail 0x00425040 (ret 0x18): word coordinates and occupancy flag.
    void TestPossibleDirections(short, short, signed char * const, signed char * const, short, int);
    void SeedCombatPosition(class army *);
    // HoMM1 retail 0x00424950 takes four arguments (ret 0x10).
    short FindCombatPath(short, short, class army *, signed char);
    // HoMM1 retail 0x00424c50 (ret 0x10): word hex/direction and unsigned
    // word distance/speed.
    void PushCombatPoint(short, short, unsigned short, unsigned short);
};
#pragma pack(pop)

// SeedPosition's working mobility (0x004c4efc) and seeding state.
extern short giCurTempMobility;
extern int giSeedingValid;
#endif // HOMM1_SOURCE_SEARCHARRAY_H

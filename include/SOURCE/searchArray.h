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
    SEARCH_FLAG_BIT_COUNT = 1,
    SEARCH_DIRECTION_BIT_COUNT = 4,
    SEARCH_PATH_CAPACITY = 256
H1_ENUM_END(SearchStorageConstant)

// HoMM1 search node, nine bytes. PushPoint packs the direction into the low
// four bits of the cost word and the reseed flags above the flag bits.
#pragma pack(push, 1)
struct searchNode {
    signed char x;
    signed char y;
    unsigned short direction : SEARCH_DIRECTION_BIT_COUNT;
    unsigned short distance : 12;
    unsigned char visited : SEARCH_FLAG_BIT_COUNT;
    unsigned char unknownFlag : SEARCH_FLAG_BIT_COUNT;
    unsigned char rvFlag1 : SEARCH_FLAG_BIT_COUNT;
    unsigned char rvFlag2 : 5;
    signed char adjacentMonsterX;
    signed char adjacentMonsterY;
    signed char previousX;
    signed char previousY;
};

class searchArray {
public:
    unsigned int m_queueCount;
    unsigned int m_maxQueueCount;
    int m_pathLength;
    int m_specialTargetX;
    int m_specialTargetY;
    searchNode m_queue[SEARCH_QUEUE_CAPACITY];
    searchNode m_cells[SEARCH_CELL_CAPACITY];
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
    // HoMM1 retail 0x00424d90: word point, direction and costs, byte flags
    // and neighbour coordinates (ret 0x30).
    void PushPoint(short, short, unsigned short, unsigned short, unsigned short, unsigned char, signed char,
                   signed char, signed char, unsigned char, signed char, signed char);
    // HoMM1 retail 0x00425040: word coordinates (ret 0x18).
    void TestPossibleDirections(short, short, signed char * const, signed char * const, int, int);
    void SeedCombatPosition(class army *);
    // HoMM1 retail 0x00424950 takes four arguments (ret 0x10).
    short FindCombatPath(short, short, class army *, signed char);
    void PushCombatPoint(int, int, int, int);
};
#pragma pack(pop)

// SeedPosition's working mobility (0x004c4efc) and seeding state.
extern short giCurTempMobility;
extern int giSeedingValid;
#endif // HOMM1_SOURCE_SEARCHARRAY_H

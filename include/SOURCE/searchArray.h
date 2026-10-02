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

// Donor searchNode's real packed record; HoMM1 stores nodes inline.
#pragma pack(push, 1)
struct searchNode {
    unsigned char x;
    unsigned char y;
    unsigned short distance;
    unsigned char visited : SEARCH_FLAG_BIT_COUNT;
    unsigned char unknownFlag : SEARCH_FLAG_BIT_COUNT;
    unsigned char rvFlag1 : SEARCH_FLAG_BIT_COUNT;
    unsigned char rvFlag2 : SEARCH_FLAG_BIT_COUNT;
    unsigned char direction : SEARCH_DIRECTION_BIT_COUNT;
    union {
        struct {
            unsigned char adjacentMonsterX;
            unsigned char adjacentMonsterY;
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
    searchNode m_cells[SEARCH_CELL_CAPACITY];
    // Retail DoDimensionDoor reads the path directions at +0xda54.
    unsigned char m_directions[SEARCH_PATH_CAPACITY];
    // --- constructors ---
    searchArray(void);
    // --- methods ---
    int BuildPath(short, short, short, short, int);
    void SeedPosition(short, short, short, int, int, int, int, int, int, int, int, int);
    void Init(void);
    void Close(void);
    void Clear(void);
    short QuickDistance(short, short, short, short);
    void PushPoint(int, int, int, int, int, int, int, int, int, int, int, int);
    void TestPossibleDirections(int, int, signed char * const, signed char * const, int, int);
    void SeedCombatPosition(class army *);
    // HoMM1 retail 0x00424950 takes four arguments (ret 0x10).
    short FindCombatPath(short, short, class army *, signed char);
    void PushCombatPoint(int, int, int, int);
};
#pragma pack(pop)
#endif // HOMM1_SOURCE_SEARCHARRAY_H

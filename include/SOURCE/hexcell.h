#ifndef HOMM1_SOURCE_HEXCELL_H
#define HOMM1_SOURCE_HEXCELL_H
// Reconstructed class (SOURCE) from CodeView NB09 of HEROES2W.EXE — NOT original source.
// 8 methods, 0 own-virtual, 0 static data.

#include <H1/Macros.h>

// 12-byte combat grid cell (retail constructor 0x0046e5b0); the combat
// manager embeds 45 of them (5 rows of 9) at +0x40.
#pragma pack(push, 1)
class hexcell {
public:
    // Screen position of the hex centre (Fireball draws from here).
    short m_x;
    short m_y;
    signed char m_unknown04;
    signed char m_unknown05;
    char m_unknown06;
    // ValidHexToStandOn: -1 when no obstacle stands on the hex.
    signed char m_obstacle;
    signed char m_occupantSide;
    signed char m_occupantIndex;
    signed char m_unknown0a;
    signed char m_unknown0b;
    // --- constructors ---
    hexcell(void);
    // --- methods ---
    void DrawGround(void);
    void DrawLowerDeadOccupants(void);
    void DrawUpperDeadOccupant(void);
    void DrawOccupant(int, int);
    void DrawTower(int);
    void DrawClouds(void);
    void DrawObstacle(void);
};
#pragma pack(pop)
#endif // HOMM1_SOURCE_HEXCELL_H

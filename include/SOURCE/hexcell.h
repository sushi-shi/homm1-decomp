#ifndef HOMM1_SOURCE_HEXCELL_H
#define HOMM1_SOURCE_HEXCELL_H
// Reconstructed class (SOURCE) from CodeView NB09 of HEROES2W.EXE — NOT original source.
// HoMM1 keeps 12-byte cells: combatManager strides its 45 hexes by twelve
// bytes from +0x40.

#include <H1/Macros.h>

#pragma pack(push, 1)
class hexcell {
public:
    short m_x;
    short m_y;
    // DrawGround draws combat icon (3 + this) at this frame.
    signed char m_groundIcon;
    signed char m_groundFrame;
    // Castle pieces (5) draw towers and walls; other obstacles use frame 7.
    signed char m_obstacleType;
    signed char m_obstacleIndex;
    signed char m_occupantSide;
    signed char m_occupantIndex;
    signed char m_occupantFrame;
    // army::ResetPath clears the per-cell path mark.
    signed char m_pathFlag;
    // --- constructors ---
    hexcell(void);
    // --- methods ---
    hexcell* TakeOccupant(hexcell*);
    void DrawGround(void);
    void DrawOccupant(void);
    void DrawTower(signed char);
    void DrawWall(void);
    void DrawObstacle(void);
};
#pragma pack(pop)
#endif // HOMM1_SOURCE_HEXCELL_H

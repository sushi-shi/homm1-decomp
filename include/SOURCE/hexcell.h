#ifndef HOMM1_SOURCE_HEXCELL_H
#define HOMM1_SOURCE_HEXCELL_H
// Reconstructed class (SOURCE) from CodeView NB09 of HEROES2W.EXE — NOT original source.
// HoMM1 keeps 12-byte cells: combatManager strides its 45 hexes by twelve
// bytes from +0x40.

#include <Domains.h>
#include <H1/Macros.h>
#include <SOURCE/combatTypes.h>

// m_occupantFrame: the facing DrawOccupant last drew the occupant with; the
// constructor and TakeOccupant reset it to NONE so the next frame redraws.
H1_ENUM_CONST_BEGIN(HexcellConstant)
    HEXCELL_OCCUPANT_FRAME_NONE = -1
H1_ENUM_CONST_END(HexcellConstant)

#pragma pack(push, 1)
class hexcell {
public:
    i16 m_x;
    i16 m_y;
    // DrawGround draws this combat icon at this frame.
    i8 m_groundIcon;
    i8 m_groundFrame;
    // Castle pieces (5) draw towers and walls; other obstacles use frame 7.
    i8 m_obstacleType;
    // -1 when no obstacle stands on the hex (ValidHexToStandOn).
    H1_ENUM_STORAGE(CombatObstacleIndex, i8) m_obstacleIndex;
    H1_ENUM_STORAGE(CombatSide, i8) m_occupantSide;
    i8 m_occupantIndex;
    i8 m_occupantFrame;
    // army::ResetPath clears the per-cell path mark.
    i8 m_pathFlag;
    // --- constructors ---
    hexcell(void);
    // --- methods ---
    hexcell* TakeOccupant(hexcell*);
    void DrawGround(void);
    void DrawOccupant(void);
    void DrawTower(i8);
    void DrawWall(void);
    void DrawObstacle(void);
};
#pragma pack(pop)
#endif // HOMM1_SOURCE_HEXCELL_H

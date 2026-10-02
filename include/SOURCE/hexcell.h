#ifndef HOMM1_SOURCE_HEXCELL_H
#define HOMM1_SOURCE_HEXCELL_H
// Reconstructed class (SOURCE) from CodeView NB09 of HEROES2W.EXE — NOT original source.
// HoMM1 keeps 12-byte cells: combatManager strides its 45 hexes by twelve
// bytes from +0x40.

#include <Domains.h>
#include <H1/Macros.h>
#include <SOURCE/combatTypes.h>

// clang-format off
// m_occupantFrame: the facing DrawOccupant last drew the occupant with; the
// constructor and TakeOccupant reset it to NONE so the next frame redraws.
H1_ENUM_CONST_BEGIN(HexcellConstant)
    HEXCELL_OCCUPANT_FRAME_NONE = -1
H1_ENUM_CONST_END(HexcellConstant)
// clang-format on

#pragma pack(push, 1)
class hexcell {
public:
    short m_x;
    short m_y;
    // DrawGround draws this combat icon at this frame.
    signed char m_groundIcon;
    signed char m_groundFrame;
    // Castle pieces (5) draw towers and walls; other obstacles use frame 7.
    signed char m_obstacleType;
    // -1 when no obstacle stands on the hex (ValidHexToStandOn).
    H1_ENUM_STORAGE(CombatObstacleIndex, signed char) m_obstacleIndex;
    H1_ENUM_STORAGE(CombatSide, signed char) m_occupantSide;
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

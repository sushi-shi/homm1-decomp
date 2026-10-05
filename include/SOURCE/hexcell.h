#ifndef HOMM1_SOURCE_HEXCELL_H
#define HOMM1_SOURCE_HEXCELL_H
// Cells are 12 bytes: combatManager strides its 45 hexes by twelve
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
    hexcell* TakeOccupant(hexcell* from);
    void DrawGround(void);
    void DrawOccupant(void);
    void DrawTower(i8 frame);
    void DrawWall(void);
    void DrawObstacle(void);
};
#pragma pack(pop)

// The cell holds the given stack: side first, then index, without narrowing
// the requested identity.
#define HEX_HAS_OCCUPANT(cell, side, index)                                                        \
    ((cell).m_occupantSide == (side) && (cell).m_occupantIndex == (index))
// Forget the live occupant, side then index; the frame stays.
#define CLEAR_HEX_OCCUPANT(cell)                                                                   \
    ((cell).m_occupantSide = COMBAT_SIDE_NONE, (cell).m_occupantIndex = COMBAT_ARMY_INDEX_NONE)
#endif // HOMM1_SOURCE_HEXCELL_H

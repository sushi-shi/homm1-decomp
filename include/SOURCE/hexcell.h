#ifndef HOMM1_SOURCE_HEXCELL_H
#define HOMM1_SOURCE_HEXCELL_H
// Cells are 12 bytes: combatManager strides its 45 hexes by twelve
// bytes from +0x40.

#include <Domains.h>
#include <H1/Macros.h>
#include <SOURCE/combatTypes.h>

#pragma pack(push, 1)
class hexcell {
public:
    i16 m_x;
    i16 m_y;
    // DrawGround draws this combat icon at this frame.
    H1_ENUM_STORAGE(CombatIconSlot, i8) m_groundIcon;
    i8 m_groundFrame;
    // The combat icon slot the obstacle draws from: COMBAT_ICON_CASTLE wall
    // pieces draw as towers and walls, COMBAT_ICON_OBSTACLES rocks draw frame
    // m_obstacleIndex.
    H1_ENUM_STORAGE(CombatIconSlot, i8) m_obstacleIcon;
    // -1 when no obstacle stands on the hex (ValidHexToStandOn).
    H1_ENUM_STORAGE(CombatObstacleIndex, i8) m_obstacleIndex;
    H1_ENUM_STORAGE(CombatSide, i8) m_occupantSide;
    i8 m_occupantIndex;
    // Which half of a two-hex stack the cell holds (ARMY_FACING_LEFT on the
    // left hex, ARMY_FACING_RIGHT on the right one); ARMY_FACING_NONE for a
    // one-hex stack or an empty cell. DrawOccupant draws the stack from the
    // cell whose half differs from its facing, so it draws once.
    H1_ENUM_STORAGE(ArmyFacing, i8) m_occupantFootprintHalf;
    // army::ResetPath clears the per-cell path mark.
    i8 m_pathFlag;
    // --- constructors ---
    hexcell(void);
    // --- methods ---
    hexcell* TakeOccupant(hexcell* from);
    void DrawGround(void);
    void DrawOccupant(void);
    void DrawTower(H1_ENUM_PARAM(CombatObstacleIndex, i8) frame);
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

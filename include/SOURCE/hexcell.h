#ifndef HOMM1_SOURCE_HEXCELL_H
#define HOMM1_SOURCE_HEXCELL_H

#include <SOURCE/combatTypes.h>

#pragma pack(push, 1)
class hexcell {
public:
    i16 m_x;
    i16 m_y;
    i8 m_groundIcon;
    i8 m_groundFrame;
    i8 m_obstacleIcon;
    i8 m_obstacleIndex;
    i8 m_occupantSide;
    i8 m_occupantIndex;
    i8 m_occupantFootprintHalf;
    b8 m_pathFlag;
    hexcell(void);
    hexcell* TakeOccupant(hexcell* from);
    void DrawGround(void);
    void DrawOccupant(void);
    void DrawTower(i8 frame);
    void DrawWall(void);
    void DrawObstacle(void);
};
#pragma pack(pop)

#define HEX_HAS_OCCUPANT(cell, side, index)                                                        \
    ((cell).m_occupantSide == (side) && (cell).m_occupantIndex == (index))
#define CLEAR_HEX_OCCUPANT(cell)                                                                   \
    ((cell).m_occupantSide = COMBAT_SIDE_NONE, (cell).m_occupantIndex = COMBAT_ARMY_INDEX_NONE)
#endif

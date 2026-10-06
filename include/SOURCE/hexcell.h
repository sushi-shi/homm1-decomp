#ifndef HOMM1_SOURCE_HEXCELL_H
#define HOMM1_SOURCE_HEXCELL_H

#include <SOURCE/combatTypes.h>

enum HexcellConstant {
    HEXCELL_OCCUPANT_FRAME_NONE = -1
};

#pragma pack(push, 1)
class hexcell {
public:
    i16 m_x;
    i16 m_y;
    i8 m_groundIcon;
    i8 m_groundFrame;
    i8 m_obstacleType;
    i8 m_obstacleIndex;
    i8 m_occupantSide;
    i8 m_occupantIndex;
    i8 m_occupantFrame;
    i8 m_pathFlag;
    hexcell(void);
    hexcell* TakeOccupant(hexcell* from);
    void DrawGround(void);
    void DrawOccupant(void);
    void DrawTower(i8 frame);
    void DrawWall(void);
    void DrawObstacle(void);
};
#pragma pack(pop)
#endif

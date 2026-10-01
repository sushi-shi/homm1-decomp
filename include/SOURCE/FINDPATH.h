#ifndef HOMM1_SOURCE_FINDPATH_H
#define HOMM1_SOURCE_FINDPATH_H

#include <Domains.h>

H1_ENUM_BEGIN(FindPathDistanceConstant)
    DISTANCE_MINOR_DIVISOR = 2
H1_ENUM_END(FindPathDistanceConstant)

H1_ENUM_BEGIN(FindPathTerrainConstant)
    FINDPATH_TERRAIN_COUNT = 7,
    FINDPATH_STEP_COST_COUNT = 2,
    FINDPATH_WATER_TERRAIN = 1,
    FINDPATH_WATER_MODE = 1
H1_ENUM_END(FindPathTerrainConstant)

int CalcTerrainCost(int, int, int, int);
short TerrainStepCost(signed char, char);

// PoL FINDPATH.cpp:32-36 retains this inline approximation helper.
inline short ApproximateGridDistance(short xDistance, short yDistance) {
    if (xDistance >= yDistance)
        return xDistance + yDistance / DISTANCE_MINOR_DIVISOR;
    return yDistance + xDistance / DISTANCE_MINOR_DIVISOR;
}

#endif

#ifndef HOMM1_SOURCE_FINDPATH_H
#define HOMM1_SOURCE_FINDPATH_H

#include <Domains.h>

// FindCombatPath and combatManager::GetClosestArmy start their best
// QuickDistance at 640 (Buka FINDPATH.cpp INITIAL_BEST_DISTANCE).
// clang-format off
H1_ENUM_CONST_BEGIN(FindPathDistanceConstant)
    DISTANCE_MINOR_DIVISOR = 2,
    FINDPATH_INITIAL_BEST_DISTANCE = 640
H1_ENUM_CONST_END(FindPathDistanceConstant)

H1_ENUM_CONST_BEGIN(FindPathTerrainConstant)
    FINDPATH_TERRAIN_COUNT = 7,
    FINDPATH_STEP_COST_COUNT = 2,
    FINDPATH_WATER_TERRAIN = 1,
    FINDPATH_WATER_MODE = 1
H1_ENUM_CONST_END(FindPathTerrainConstant)
// clang-format on

int CalcTerrainCost(int, int, int, int);
short TerrainStepCost(signed char, char);
// FindNearestObject seeds this word limit; PushPoint marks costlier nodes.
extern short giCurTempMobility;

// PoL FINDPATH.cpp:32-36 retains this inline approximation helper.
inline short ApproximateGridDistance(short xDistance, short yDistance) {
    if (xDistance >= yDistance)
        return xDistance + yDistance / DISTANCE_MINOR_DIVISOR;
    return yDistance + xDistance / DISTANCE_MINOR_DIVISOR;
}

#endif

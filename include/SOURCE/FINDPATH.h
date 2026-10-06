#ifndef HOMM1_SOURCE_FINDPATH_H
#define HOMM1_SOURCE_FINDPATH_H

enum FindPathDistanceConstant {
    DISTANCE_MINOR_DIVISOR = 2,
    FINDPATH_INITIAL_BEST_DISTANCE = 640
};

enum FindPathTerrainConstant {
    FINDPATH_TERRAIN_COUNT = 7,
    FINDPATH_STEP_STRAIGHT = 0,
    FINDPATH_STEP_DIAGONAL = 1,
    FINDPATH_STEP_COST_COUNT = 2,
    FINDPATH_WATER_TERRAIN = 1,
    FINDPATH_WATER_MODE = 1
};

i32 CalcTerrainCost(i32 terrain, i32 diagonal, i32 mobility, i32 waterMode);
i16 TerrainStepCost(i8 terrain, i8 diagonal);
extern i16 gCurTempMobility;

inline i16 ApproximateGridDistance(i16 xDistance, i16 yDistance) {
    if (xDistance >= yDistance)
        return xDistance + yDistance / DISTANCE_MINOR_DIVISOR;
    return yDistance + xDistance / DISTANCE_MINOR_DIVISOR;
}

#endif

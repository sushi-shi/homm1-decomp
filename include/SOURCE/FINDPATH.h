#ifndef HOMM1_SOURCE_FINDPATH_H
#define HOMM1_SOURCE_FINDPATH_H

#include <SOURCE/terrainTypes.h>

enum FindPathDistanceConstant {
    DISTANCE_MINOR_DIVISOR = 2,
    FINDPATH_INITIAL_BEST_DISTANCE = 640
};

enum FindPathTerrainConstant {
    FINDPATH_TERRAIN_COUNT = 7,
    FINDPATH_STEP_STRAIGHT = 0,
    FINDPATH_STEP_DIAGONAL = 1,
    FINDPATH_STEP_COST_COUNT = 2,
    FINDPATH_BARBARIAN_TERRAIN = 1,
    FINDPATH_BARBARIAN_CLASS = 1
};

i32 CalcTerrainCost(i32 terrain, i32 diagonal, i32 mobility, i32 heroClass);
i16 TerrainStepCost(i8 terrain, i8 diagonal);
extern i16 gCurTempMobility;

#endif

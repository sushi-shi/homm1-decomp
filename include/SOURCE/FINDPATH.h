#ifndef HOMM1_SOURCE_FINDPATH_H
#define HOMM1_SOURCE_FINDPATH_H

#include <Domains.h>
#include <SOURCE/terrainTypes.h>

// FindCombatPath and combatManager::GetClosestArmy start their best
// QuickDistance at 640.
// clang-format off
H1_ENUM_CONST_BEGIN(FindPathDistanceConstant)
    DISTANCE_MINOR_DIVISOR = 2,
    FINDPATH_INITIAL_BEST_DISTANCE = 640
H1_ENUM_CONST_END(FindPathDistanceConstant)

H1_ENUM_CONST_BEGIN(FindPathTerrainConstant)
    FINDPATH_TERRAIN_COUNT = 7,
    // gTerrainCost's second index and CalcTerrainCost's diagonal argument:
    // a straight or a diagonal step (SeedPosition's s_stepCost pair).
    FINDPATH_STEP_STRAIGHT = 0,
    FINDPATH_STEP_DIAGONAL = 1,
    FINDPATH_STEP_COST_COUNT = 2,
    // CalcTerrainCost's heroClass (every caller passes hero::m_heroClass):
    // a barbarian (class 1) pays every step at the grass row (TERRAIN_GRASS),
    // ignoring rough terrain.
    FINDPATH_BARBARIAN_TERRAIN = 1,
    FINDPATH_BARBARIAN_CLASS = 1
H1_ENUM_CONST_END(FindPathTerrainConstant)

i32 CalcTerrainCost(i32 terrain, i32 diagonal, i32 mobility, i32 heroClass);
// clang-format on
i16 TerrainStepCost(H1_ENUM_PARAM(TerrainType, i8) terrain, i8 diagonal);
// FindNearestObject seeds this mobility limit; PushPoint marks costlier nodes.
extern i16 gCurTempMobility;

#endif

#include <H1/Ints.h>

#include <SOURCE/advManager.h>
#include <SOURCE/FINDPATH.h>
#include <SOURCE/hero.h>
#include <SOURCE/KB.h>
#include <SOURCE/mapCell.h>
#include <SOURCE/mapObjectTypes.h>
#include <SOURCE/philAI.h>
#include <SOURCE/playerData.h>
#include <SOURCE/searchArray.h>

static i32 gSearchDeadInt;

i16 searchArray::FindNearestObject(
    i16 startX,
    i16 startY,
    i16 direction,
    i16 maximumCost,
    u8 triggerType
) {
    searchNode node;
    i8 directionTerrain[MAP_DIRECTION_COUNT];
    u8 directionOccupied[MAP_DIRECTION_COUNT];
    i16 i;
    i16 terrain;
    i16 cost;
    searchNode* pathNode;
    u8* pathDirection;
    i16 destinationY;
    i16 destinationX;
    i16 neighborX;
    i16 neighborY;

    gCurTempMobility = SEARCH_NEAREST_OBJECT_MOBILITY;
    m_specialTargetX = SEARCH_INVALID_COORDINATE;
    Clear();
    PushPoint(startX, startY, direction, 0, maximumCost, 0, false, 0, 0, 0, 0, 0);
    while (m_queueCount > 0) {
        node = m_queue[--m_queueCount];
        if (maximumCost > 0 && node.distance > maximumCost)
            continue;
        if (gGame->m_map[node.x][node.y].m_triggerType == triggerType)
            if (node.x != startX || node.y != startY) {
                m_specialTargetX = node.x;
                m_specialTargetY = node.y;
                break;
            }
        TestPossibleDirections(node.x, node.y, directionTerrain, directionOccupied, 1, 0);
        for (i = MAP_DIRECTION_FIRST; i < MAP_DIRECTION_COUNT; i++) {
            terrain = directionTerrain[i];
            if (terrain != TERRAIN_INVALID) {
                cost = CalcTerrainCost(
                    terrain,
                    i & SEARCH_DIAGONAL_COST_MASK,
                    SEARCH_UNLIMITED_COST,
                    0
                );
                neighborX = node.x + gNormalDirTable[i].x;
                neighborY = node.y + gNormalDirTable[i].y;
                PushPoint(
                    neighborX,
                    neighborY,
                    i,
                    node.distance + cost,
                    maximumCost,
                    0,
                    false,
                    0,
                    0,
                    node.beyondTurnMobility,
                    node.turnEndX,
                    node.turnEndY
                );
            }
        }
    }
    destinationX = m_specialTargetX;
    destinationY = m_specialTargetY;
    pathDirection = m_directions;
    if (destinationX < 0)
        return 0;
    while (destinationX != startX || destinationY != startY) {
        pathNode = &m_cells[destinationX][destinationY];
        *pathDirection++ = pathNode->direction;
        if (++m_pathLength >= SEARCH_PATH_CAPACITY)
            break;
        i16 backDirection = OppositeMapDirection(pathNode->direction);
        destinationX += gNormalDirTable[backDirection].x;
        destinationY += gNormalDirTable[backDirection].y;
    }
    return m_pathLength;
}

i32 searchArray::BuildPath(
    i16 startX,
    i16 startY,
    i16 destinationX,
    i16 destinationY,
    i16 maximumCost
) {
    u8* pathDirection = m_directions;
    m_pathLength = 0;
    while (destinationX != startX || destinationY != startY) {
        // A route traced back through a cell the search did not reach can
        // leave the grid; the original read the nodes beyond it.
        // A failed trace also forgets the steps it collected: the original
        // left them in m_pathLength, and ShowRoute drew them from a start
        // outside the grid, writing before the route map.
        if (destinationX < 0 || destinationX >= MAP_CELL_GRID_SIZE || destinationY < 0
            || destinationY >= MAP_CELL_GRID_SIZE) {
            m_pathLength = 0;
            return 0;
        }
        searchNode* node = &m_cells[destinationX][destinationY];
        if (node->x != destinationX || node->y != destinationY) {
            m_pathLength = 0;
            return 0;
        }
        if (node->distance <= maximumCost) {
            *pathDirection = node->direction;
            ++pathDirection;
            ++m_pathLength;
            if (m_pathLength >= SEARCH_PATH_CAPACITY) {
                m_pathLength = 0;
                break;
            }
        }
        i16 backDirection = OppositeMapDirection(node->direction);
        destinationX += gNormalDirTable[backDirection].x;
        destinationY += gNormalDirTable[backDirection].y;
    }
    return m_pathLength;
}

void searchArray::SeedPosition(
    i16 seedX,
    i16 seedY,
    i16 seedDirection,
    i16 maximumCost,
    i32 waterMode,
    b32 findAdjacentMonster,
    i32 mobility,
    i32 heroClass,
    i32 targetX,
    i32 targetY,
    b32 continueSeed,
    b32 scanMap
) {
    static i16 s_direction;
    static i32 s_terrain;
    static searchNode s_currentNode;
    static i32 s_mapX;
    static i32 s_mapY;
    static i32 s_adjacentMonsterX;
    static i32 s_adjacentMonsterY;
    static i32 s_stepCost[FINDPATH_STEP_COST_COUNT];
    static i8 s_directionTerrain[MAP_DIRECTION_COUNT];
    static i32 s_currentCost;
    static b32 s_hasTarget;
    static hero* s_currentHero;
    static i32 s_neighborX;
    static i32 s_neighborY;
    static u8 s_directionOccupied[MAP_DIRECTION_COUNT];
    static b32 s_directionBlocked;
    static mapCell* s_targetCell;
    static b8 s_hasAdjacentMonster;
    static i32 s_triggerType;
    static i32 s_adjacentX;
    static i32 s_adjacentY;
    static i32 s_adjacentCost;
    static i32 s_bestTargetCost;
    static i16 s_processedPointCount = 0;

    if (!continueSeed) {
        gFullySeeded = false;
        gCurTempMobility = mobility;
        Clear();
        m_specialTargetY = SEARCH_INVALID_COORDINATE;
        m_specialTargetX = SEARCH_INVALID_COORDINATE;
        s_currentCost = 0;
    }
    gSeedingValid = true;
    if (targetX >= 0) {
        if (!(gGame->m_mapExtra[targetX][targetY] & gCurPlayerBit))
            return;
        s_targetCell = gAdvManager->GetCell(targetX, targetY);
        if (s_targetCell->m_secondaryTrigger & MAP_CELL_SECONDARY_BLOCKED)
            return;
        if (!TERRAIN_IS_LAND(CELL_TERRAIN(s_targetCell))) {
            if (waterMode) {
                if (s_targetCell->m_triggerType == MAP_EVENT_TRIGGER(MAP_OBJECT_SHIPWRECK)
                    || s_targetCell->m_triggerType == MAP_EVENT_TRIGGER(MAP_OBJECT_SHIP))
                    return;
            } else {
                if (s_targetCell->m_triggerType != MAP_EVENT_TRIGGER(MAP_OBJECT_HERO)
                    && s_targetCell->m_triggerType != MAP_EVENT_TRIGGER(MAP_OBJECT_SHIP)
                    && s_targetCell->m_triggerType != MAP_EVENT_TRIGGER(MAP_OBJECT_SHIPWRECK))
                    return;
            }
        }
        s_hasTarget = true;
        s_bestTargetCost = SEARCH_MAX_COST;
    } else
        s_hasTarget = false;
    if (s_hasTarget && continueSeed) {
        s_currentNode = m_cells[targetX][targetY];
        if (s_currentNode.visited
            && s_currentNode.distance <= s_currentCost + SEARCH_TARGET_COST_WINDOW)
            return;
    }
    if (!continueSeed)
        PushPoint(seedX, seedY, seedDirection, 0, maximumCost, 0, false, 0, 0, 0, 0, 0);
    s_currentHero = gCurPlayerData->m_currentHero >= 0 ? gGame->GetHero(gCurPlayerData->m_currentHero)
                                                       : NULL;
    while (m_queueCount > 0) {
        --m_queueCount;
        s_currentNode = m_queue[m_queueCount];
        if (s_hasTarget && s_bestTargetCost < SEARCH_MAX_COST
            && s_currentNode.distance + SEARCH_TARGET_COST_WINDOW >= s_bestTargetCost) {
            s_currentCost = s_currentNode.distance;
            ++m_queueCount;
            return;
        }
        if (s_currentNode.distance > maximumCost && maximumCost > 0)
            goto point_complete;
        if (s_currentNode.hasAdjacentMonster) {
            s_hasAdjacentMonster = true;
            s_adjacentMonsterX = s_currentNode.adjacentMonsterX;
            s_adjacentMonsterY = s_currentNode.adjacentMonsterY;
        } else
            s_hasAdjacentMonster = false;
        if (s_currentNode.occupied) {
            s_triggerType = MAP_TRIGGER_OBJECT(
                gAdvManager->GetCell(s_currentNode.x, s_currentNode.y)->m_triggerType
            );
            if (s_triggerType == MAP_OBJECT_MONSTER || s_triggerType == MAP_OBJECT_STONE_LITHS
                || s_triggerType == MAP_OBJECT_HERO || s_triggerType == MAP_OBJECT_SHIP) {
                if (!findAdjacentMonster || s_currentNode.hasAdjacentMonster)
                    goto point_complete;
                s_hasAdjacentMonster = true;
                s_adjacentMonsterX = s_currentNode.x;
                s_adjacentMonsterY = s_currentNode.y;
                if (s_triggerType == MAP_OBJECT_HERO
                    && gGame->m_heroOwners[gAdvManager->GetCell(s_currentNode.x, s_currentNode.y)
                                               ->m_objectMetadata]
                           == gCurPlayer)
                    goto point_complete;
            } else {
                if (!findAdjacentMonster)
                    goto point_complete;
                if (s_triggerType == MAP_OBJECT_RESOURCE || s_triggerType == MAP_OBJECT_TOWN
                    || s_triggerType == MAP_OBJECT_TREASURE_CHEST
                    || s_triggerType == MAP_OBJECT_CAMPFIRE
                    || s_triggerType == MAP_OBJECT_ANCIENT_LAMP
                    || s_triggerType == MAP_OBJECT_ARTIFACT || s_triggerType == MAP_OBJECT_SIGNPOST
                    || s_triggerType == MAP_OBJECT_BUOY || s_triggerType == MAP_OBJECT_SKELETON
                    || s_triggerType == MAP_OBJECT_FOUNTAIN || s_triggerType == MAP_OBJECT_OBELISK
                    || s_triggerType == MAP_OBJECT_STATUE || s_triggerType == MAP_OBJECT_WELL)
                    goto point_complete;
            }
        }
        if (waterMode) {
            s_triggerType = MAP_PASSIVE_OBJECT(
                gAdvManager->GetCell(s_currentNode.x, s_currentNode.y)->m_triggerType
            );
            if (s_triggerType == MAP_OBJECT_COAST)
                goto point_complete;
        } else {
            if ((gMapExtra[s_currentNode.x][s_currentNode.y] & MAP_EXTRA_MONSTER_ADJACENT)
                && (s_currentNode.x != seedX || s_currentNode.y != seedY)) {
                if (!findAdjacentMonster)
                    goto point_complete;
                if (s_currentNode.hasAdjacentMonster) {
                    if (gAdvManager->FindAdjacentMonster(
                            s_currentNode.x,
                            s_currentNode.y,
                            &s_adjacentMonsterX,
                            &s_adjacentMonsterY,
                            s_currentNode.adjacentMonsterX,
                            s_currentNode.adjacentMonsterY
                        ))
                        goto point_complete;
                } else if (gAdvManager->FindAdjacentMonster(
                               s_currentNode.x,
                               s_currentNode.y,
                               &s_adjacentMonsterX,
                               &s_adjacentMonsterY,
                               SEARCH_INVALID_COORDINATE,
                               SEARCH_INVALID_COORDINATE
                           ))
                    s_hasAdjacentMonster = true;
            }
        }
        TestPossibleDirections(
            s_currentNode.x,
            s_currentNode.y,
            s_directionTerrain,
            s_directionOccupied,
            1,
            waterMode
        );
        s_terrain = CELL_TERRAIN(gAdvManager->GetCell(s_currentNode.x, s_currentNode.y));
        s_stepCost[FINDPATH_STEP_STRAIGHT] = s_currentNode.distance
                                             + CalcTerrainCost(
                                                 s_terrain,
                                                 FINDPATH_STEP_STRAIGHT,
                                                 gCurTempMobility - s_currentNode.distance,
                                                 heroClass
                                             );
        s_stepCost[FINDPATH_STEP_DIAGONAL] = s_currentNode.distance
                                             + CalcTerrainCost(
                                                 s_terrain,
                                                 FINDPATH_STEP_DIAGONAL,
                                                 gCurTempMobility - s_currentNode.distance,
                                                 heroClass
                                             );
        for (s_direction = MAP_DIRECTION_FIRST; s_direction < MAP_DIRECTION_COUNT; s_direction++) {
            if (s_directionTerrain[s_direction] == TERRAIN_INVALID)
                continue;
            s_neighborX =
                s_currentNode.x + gNormalDirTable[s_direction].x;
            s_neighborY =
                s_currentNode.y + gNormalDirTable[s_direction].y;
            if (findAdjacentMonster
                && (gMapExtra[s_neighborX][s_neighborY] & MAP_EXTRA_MONSTER_ADJACENT)
                && m_cells[s_neighborX][s_neighborY].visited
                && m_cells[s_neighborX][s_neighborY].hasAdjacentMonster
                && m_cells[s_neighborX][s_neighborY].distance
                       < s_currentNode.distance + SEARCH_MONSTER_RESEED_WINDOW
                && gAdvManager->FindAdjacentMonster(
                    s_neighborX,
                    s_neighborY,
                    &s_adjacentMonsterX,
                    &s_adjacentMonsterY,
                    SEARCH_INVALID_COORDINATE,
                    SEARCH_INVALID_COORDINATE
                )
                && m_cells[s_neighborX][s_neighborY].adjacentMonsterX == s_adjacentMonsterX
                && m_cells[s_neighborX][s_neighborY].adjacentMonsterY == s_adjacentMonsterY)
                continue;
            PushPoint(
                s_neighborX,
                s_neighborY,
                s_direction,
                s_stepCost[s_direction & SEARCH_DIAGONAL_COST_MASK],
                maximumCost,
                s_directionOccupied[s_direction],
                s_hasAdjacentMonster,
                s_adjacentMonsterX,
                s_adjacentMonsterY,
                s_currentNode.beyondTurnMobility,
                s_currentNode.turnEndX,
                s_currentNode.turnEndY
            );
            if (s_hasTarget
                && s_currentNode.x + gNormalDirTable[s_direction].x
                       == targetX
                && s_currentNode.y + gNormalDirTable[s_direction].y
                       == targetY
                && !s_currentNode.hasAdjacentMonster) {
                if (s_currentNode.distance
                        + CalcTerrainCost(
                            s_directionTerrain[s_direction],
                            s_direction & SEARCH_DIAGONAL_COST_MASK,
                            gCurTempMobility - s_currentNode.distance,
                            heroClass
                        )
                    < s_bestTargetCost)
                    s_bestTargetCost =
                        s_currentNode.distance
                        + CalcTerrainCost(
                            s_directionTerrain[s_direction],
                            s_direction & SEARCH_DIAGONAL_COST_MASK,
                            gCurTempMobility - s_currentNode.distance,
                            heroClass
                        );
            }
        }
    point_complete:
        s_processedPointCount++;
    }
    if (scanMap) {
        for (s_mapX = 0; s_mapX < MAP_CELL_GRID_SIZE; s_mapX++) {
            for (s_mapY = 0; s_mapY < MAP_CELL_GRID_SIZE; s_mapY++) {
                if (MAP_TRIGGER_OBJECT(gAdvManager->GetCell(s_mapX, s_mapY)->m_triggerType)
                    == MAP_OBJECT_MONSTER) {
                    for (s_direction = MAP_DIRECTION_FIRST; s_direction < MAP_DIRECTION_COUNT; s_direction++) {
                        s_adjacentX = s_mapX + gNormalDirTable[s_direction].x;
                        s_adjacentY = s_mapY + gNormalDirTable[s_direction].y;
                        // The original read search nodes outside the grid for
                        // monsters on the map's edge.
                        if (s_adjacentX < 0 || s_adjacentX >= MAP_CELL_GRID_SIZE || s_adjacentY < 0
                            || s_adjacentY >= MAP_CELL_GRID_SIZE)
                            continue;
                        s_targetCell = gAdvManager->GetCell(s_adjacentX, s_adjacentY);
                        s_directionBlocked = true;
                        if (((1 << s_direction) & MAP_DIRECTION_SOUTH_MASK)
                            && CELL_HAS_NON_SHADOW_OBJECT(s_targetCell))
                            s_directionBlocked = false;
                        if (s_directionBlocked && m_cells[s_adjacentX][s_adjacentY].visited
                            && !(s_targetCell->m_triggerType & MAP_TRIGGER_EVENT)) {
                            s_terrain = CELL_TERRAIN(s_targetCell);
                            s_adjacentCost = m_cells[s_adjacentX][s_adjacentY].distance;
                            s_stepCost[FINDPATH_STEP_STRAIGHT] =
                                s_adjacentCost
                                + CalcTerrainCost(
                                    s_terrain,
                                    FINDPATH_STEP_STRAIGHT,
                                    gCurTempMobility - s_adjacentCost,
                                    heroClass
                                );
                            s_stepCost[FINDPATH_STEP_DIAGONAL] =
                                s_adjacentCost
                                + CalcTerrainCost(
                                    s_terrain,
                                    FINDPATH_STEP_DIAGONAL,
                                    gCurTempMobility - s_adjacentCost,
                                    heroClass
                                );
                            PushPoint(
                                s_mapX,
                                s_mapY,
                                OppositeMapDirection(s_direction),
                                s_stepCost
                                    [s_direction
                                     & SEARCH_DIAGONAL_COST_MASK],
                                maximumCost,
                                1,
                                false,
                                SEARCH_INVALID_COORDINATE,
                                SEARCH_INVALID_COORDINATE,
                                0,
                                SEARCH_INVALID_COORDINATE,
                                SEARCH_INVALID_COORDINATE
                            );
                        }
                    }
                }
            }
        }
    }
    gFullySeeded = true;
}

b32 gFullySeeded;

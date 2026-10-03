// Located from HoMM2 Buka 2.1 SEARCH.cpp; HoMM1 builds this TU with /O2.

#include <match.h>

#include <H1/All.h>
#include <H1/KB.h>
#include <SOURCE/FINDPATH.h>
#include <SOURCE/mapObjectTypes.h>
#include <SOURCE/philAI.h>
#include <SOURCE/searchArray.h>

// SeedPosition's working mobility, read back by PushPoint (FINDPATH storage).
extern short giCurTempMobility;

// HoMM1: flood from the hero until a cell carrying the trigger type turns
// up, then walk the directions back into the path buffer.
VA(0x004028b0, 0x23a)
short searchArray::FindNearestObject(
    short startX,
    short startY,
    short direction,
    short maximumCost,
    unsigned char triggerType
) {
    // node.x/node.y are read in place: retail spills the coordinate as a CSE
    // temporary in the dword slot below node. The declaration order gives the
    // start-versus-destination compares their retail operand order.
    searchNode node;
    signed char possibleDirections[MAP_DIRECTION_COUNT];
    signed char directionCosts[MAP_DIRECTION_COUNT];
    short i;
    short terrain;
    short cost;
    searchNode* pathNode;
    unsigned char* pathDirection;
    short destinationY;
    short destinationX;
    short neighborX;
    short neighborY;

    giCurTempMobility = SEARCH_NEAREST_OBJECT_MOBILITY;
    m_specialTargetX = SEARCH_INVALID_COORDINATE;
    Clear();
    PushPoint(startX, startY, direction, 0, maximumCost, 0, 0, 0, 0, 0, 0, 0);
    while (m_queueCount) {
        node = m_queue[--m_queueCount];
        if (maximumCost > 0 && node.distance > maximumCost)
            continue;
        // Nested rather than &&: the code is the same, and the C1 labels it
        // allocates give SeedPosition retail's register allocation.
        if (gpGame->m_map[node.x][node.y].m_triggerType == triggerType)
            if (startX != node.x || startY != node.y) {
                m_specialTargetX = node.x;
                m_specialTargetY = node.y;
                break;
            }
        TestPossibleDirections(node.x, node.y, possibleDirections, directionCosts, 1, 0);
        for (i = 0; i < MAP_DIRECTION_COUNT; i++) {
            terrain = possibleDirections[i];
            if (terrain != TERRAIN_INVALID) {
                cost = CalcTerrainCost(
                    terrain,
                    i & SEARCH_DIAGONAL_COST_MASK,
                    SEARCH_UNLIMITED_COST,
                    0
                );
                neighborX = node.x + normalDirTable[i].x;
                neighborY = node.y + normalDirTable[i].y;
                PushPoint(
                    neighborX,
                    neighborY,
                    i,
                    node.distance + cost,
                    maximumCost,
                    0,
                    0,
                    0,
                    0,
                    node.rvFlag2,
                    node.previousX,
                    node.previousY
                );
            }
        }
    }
    destinationX = m_specialTargetX;
    destinationY = m_specialTargetY;
    pathDirection = m_directions;
    if (destinationX < 0)
        return 0;
    while (startX != destinationX || startY != destinationY) {
        pathNode = &m_cells[destinationX][destinationY];
        *pathDirection++ = pathNode->direction;
        if (++m_pathLength >= SEARCH_PATH_CAPACITY)
            break;
        destinationX += normalDirTable
                            [static_cast<short>(pathNode->direction + MAP_DIRECTION_OPPOSITE_OFFSET)
                             & MAP_DIRECTION_INDEX_MASK]
                                .x;
        destinationY += normalDirTable
                            [static_cast<short>(pathNode->direction + MAP_DIRECTION_OPPOSITE_OFFSET)
                             & MAP_DIRECTION_INDEX_MASK]
                                .y;
    }
    return m_pathLength;
}

// Buka SEARCH.cpp BuildPath over HoMM1's packed node word.
VA(0x00402af0, 0xe4)
int searchArray::BuildPath(
    short startX,
    short startY,
    short destinationX,
    short destinationY,
    short maximumCost
) {
    unsigned char* pathDirection = m_directions;
    m_pathLength = 0;
    while (startX != destinationX || startY != destinationY) {
        searchNode* node = &m_cells[destinationX][destinationY];
        if (node->x != destinationX && node->y != destinationY)
            return 0;
        if (node->distance <= maximumCost) {
            *pathDirection = node->direction;
            ++pathDirection;
            ++m_pathLength;
            if (m_pathLength >= SEARCH_PATH_CAPACITY) {
                m_pathLength = 0;
                break;
            }
        }
        destinationX += normalDirTable
                            [static_cast<short>(node->direction + MAP_DIRECTION_OPPOSITE_OFFSET)
                             & MAP_DIRECTION_INDEX_MASK]
                                .x;
        destinationY += normalDirTable
                            [static_cast<short>(node->direction + MAP_DIRECTION_OPPOSITE_OFFSET)
                             & MAP_DIRECTION_INDEX_MASK]
                                .y;
    }
    return m_pathLength;
}

// Buka SEARCH.cpp SeedPosition; HoMM1 has no roads or pathfinding skill and
// precomputes the straight and diagonal step costs once per node. Buka's
// file-scope scratch is function-static here: only that TU state gives
// retail's esi/edi/ebx allocation of this, continueSeed and zero.
VA(0x00402be0, 0xa53)
void searchArray::SeedPosition(
    short seedX,
    short seedY,
    short seedDirection,
    short maximumCost,
    int waterMode,
    int findAdjacentMonster,
    int mobility,
    int costMode,
    int targetX,
    int targetY,
    int continueSeed,
    int scanMap
) {
    static short s_direction;
    static int s_terrain;
    static searchNode s_currentNode;
    static int s_mapX;
    static int s_mapY;
    static int s_adjacentMonsterX;
    static int s_adjacentMonsterY;
    static int s_stepCost[FINDPATH_STEP_COST_COUNT];
    static signed char s_possibleDirections[MAP_DIRECTION_COUNT];
    static int s_currentCost;
    static int s_hasTarget;
    static hero* s_currentHero;
    static int s_neighborX;
    static int s_neighborY;
    static unsigned char s_directionOccupied[MAP_DIRECTION_COUNT];
    static int s_directionBlocked;
    static mapCell* s_targetCell;
    static signed char s_hasAdjacentMonster;
    static int s_triggerType;
    static int s_adjacentX;
    static int s_adjacentY;
    static int s_adjacentCost;
    static int s_bestTargetCost;
    static short s_processedPointCount;

    if (!continueSeed) {
        giFullySeeded = 0;
        giCurTempMobility = mobility;
        Clear();
        m_specialTargetY = SEARCH_INVALID_COORDINATE;
        m_specialTargetX = SEARCH_INVALID_COORDINATE;
        s_currentCost = 0;
    }
    giSeedingValid = 1;
    if (targetX >= 0) {
        if (!(gpGame->m_mapExtra[targetX][targetY] & giCurPlayerBit))
            return;
        s_targetCell = gpAdvManager->GetCell(targetX, targetY);
        if (s_targetCell->m_secondaryTrigger & MAP_CELL_SECONDARY_BLOCKED)
            return;
        if (!CELL_TERRAIN(s_targetCell)) {
            if (waterMode) {
                if (s_targetCell->m_triggerType == (MAP_TRIGGER_EVENT | MAP_OBJECT_SHIPWRECK)
                    || s_targetCell->m_triggerType == (MAP_TRIGGER_EVENT | MAP_OBJECT_SHIP))
                    return;
            } else {
                if (s_targetCell->m_triggerType != (MAP_TRIGGER_EVENT | MAP_OBJECT_HERO)
                    && s_targetCell->m_triggerType != (MAP_TRIGGER_EVENT | MAP_OBJECT_SHIP)
                    && s_targetCell->m_triggerType != (MAP_TRIGGER_EVENT | MAP_OBJECT_SHIPWRECK))
                    return;
            }
        }
        s_hasTarget = 1;
        s_bestTargetCost = SEARCH_MAX_COST;
    } else
        s_hasTarget = 0;
    if (s_hasTarget && continueSeed) {
        s_currentNode = m_cells[targetX][targetY];
        if (s_currentNode.visited
            && s_currentNode.distance <= s_currentCost + SEARCH_TARGET_COST_WINDOW)
            return;
    }
    if (!continueSeed)
        PushPoint(seedX, seedY, seedDirection, 0, maximumCost, 0, 0, 0, 0, 0, 0, 0);
    s_currentHero = gpGame->GetHero(gpCurPlayer->m_currentHero);
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
        if (s_currentNode.rvFlag1) {
            s_hasAdjacentMonster = 1;
            s_adjacentMonsterX = s_currentNode.adjacentMonsterX;
            s_adjacentMonsterY = s_currentNode.adjacentMonsterY;
        } else
            s_hasAdjacentMonster = 0;
        if (s_currentNode.occupied) {
            s_triggerType = gpAdvManager->GetCell(s_currentNode.x, s_currentNode.y)->m_triggerType
                            & MAP_TRIGGER_TYPE_MASK;
            if (s_triggerType == MAP_OBJECT_MONSTER || s_triggerType == MAP_OBJECT_STONE_LITHS
                || s_triggerType == MAP_OBJECT_HERO || s_triggerType == MAP_OBJECT_SHIP) {
                if (!findAdjacentMonster || s_currentNode.rvFlag1)
                    goto point_complete;
                s_hasAdjacentMonster = 1;
                s_adjacentMonsterX = s_currentNode.x;
                s_adjacentMonsterY = s_currentNode.y;
                if (s_triggerType == MAP_OBJECT_HERO
                    && gpGame->m_availableHeroes[static_cast<unsigned char>(
                           gpAdvManager->GetCell(s_currentNode.x, s_currentNode.y)->m_objectMetadata
                       )] == giCurPlayer)
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
            s_triggerType = gpAdvManager->GetCell(s_currentNode.x, s_currentNode.y)->m_triggerType;
            if (s_triggerType == MAP_OBJECT_COAST)
                goto point_complete;
        } else {
            if ((mapExtra[s_currentNode.x][s_currentNode.y] & MAP_EXTRA_MONSTER_ADJACENT)
                && (s_currentNode.x != seedX || s_currentNode.y != seedY)) {
                if (!findAdjacentMonster)
                    goto point_complete;
                if (s_currentNode.rvFlag1) {
                    if (gpAdvManager->FindAdjacentMonster(
                            s_currentNode.x,
                            s_currentNode.y,
                            &s_adjacentMonsterX,
                            &s_adjacentMonsterY,
                            s_currentNode.adjacentMonsterX,
                            s_currentNode.adjacentMonsterY
                        ))
                        goto point_complete;
                } else if (gpAdvManager->FindAdjacentMonster(
                               s_currentNode.x,
                               s_currentNode.y,
                               &s_adjacentMonsterX,
                               &s_adjacentMonsterY,
                               SEARCH_INVALID_COORDINATE,
                               SEARCH_INVALID_COORDINATE
                           ))
                    s_hasAdjacentMonster = 1;
            }
        }
        // byte-evidenced: read back zero-extended, filled as signed bytes.
        TestPossibleDirections(
            s_currentNode.x,
            s_currentNode.y,
            s_possibleDirections,
            reinterpret_cast<signed char*>(s_directionOccupied),
            1,
            waterMode
        );
        s_terrain =
            CELL_TERRAIN(gpAdvManager->GetCell(s_currentNode.x, s_currentNode.y));
        s_stepCost[FINDPATH_STEP_STRAIGHT] = s_currentNode.distance
                                             + CalcTerrainCost(
                                                 s_terrain,
                                                 FINDPATH_STEP_STRAIGHT,
                                                 giCurTempMobility - s_currentNode.distance,
                                                 costMode
                                             );
        s_stepCost[FINDPATH_STEP_DIAGONAL] = s_currentNode.distance
                                             + CalcTerrainCost(
                                                 s_terrain,
                                                 FINDPATH_STEP_DIAGONAL,
                                                 giCurTempMobility - s_currentNode.distance,
                                                 costMode
                                             );
        for (s_direction = 0; s_direction < MAP_DIRECTION_COUNT; s_direction++) {
            if (s_possibleDirections[s_direction] == TERRAIN_INVALID)
                continue;
            s_neighborX = s_currentNode.x + normalDirTable[s_direction].x;
            s_neighborY = s_currentNode.y + normalDirTable[s_direction].y;
            if (findAdjacentMonster
                && (mapExtra[s_neighborX][s_neighborY] & MAP_EXTRA_MONSTER_ADJACENT)
                && m_cells[s_neighborX][s_neighborY].visited
                && m_cells[s_neighborX][s_neighborY].rvFlag1
                && m_cells[s_neighborX][s_neighborY].distance
                       < s_currentNode.distance + SEARCH_MONSTER_RESEED_WINDOW
                && gpAdvManager->FindAdjacentMonster(
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
                s_currentNode.rvFlag2,
                s_currentNode.previousX,
                s_currentNode.previousY
            );
            if (s_hasTarget && s_currentNode.x + normalDirTable[s_direction].x == targetX
                && s_currentNode.y + normalDirTable[s_direction].y == targetY
                && !s_currentNode.rvFlag1) {
                if (s_currentNode.distance
                        + CalcTerrainCost(
                            s_possibleDirections[s_direction],
                            s_direction & SEARCH_DIAGONAL_COST_MASK,
                            giCurTempMobility - s_currentNode.distance,
                            costMode
                        )
                    < s_bestTargetCost)
                    s_bestTargetCost = s_currentNode.distance
                                       + CalcTerrainCost(
                                           s_possibleDirections[s_direction],
                                           s_direction & SEARCH_DIAGONAL_COST_MASK,
                                           giCurTempMobility - s_currentNode.distance,
                                           costMode
                                       );
            }
        }
    point_complete:
        s_processedPointCount++;
    }
    if (scanMap) {
        for (s_mapX = 0; s_mapX < MAP_CELL_GRID_SIZE; s_mapX++) {
            for (s_mapY = 0; s_mapY < MAP_CELL_GRID_SIZE; s_mapY++) {
                if ((gpAdvManager->GetCell(s_mapX, s_mapY)->m_triggerType & MAP_TRIGGER_TYPE_MASK)
                    == MAP_OBJECT_MONSTER) {
                    for (s_direction = 0; s_direction < MAP_DIRECTION_COUNT; s_direction++) {
                        s_adjacentX = s_mapX + normalDirTable[s_direction].x;
                        s_adjacentY = s_mapY + normalDirTable[s_direction].y;
                        s_targetCell = gpAdvManager->GetCell(s_adjacentX, s_adjacentY);
                        s_directionBlocked = 1;
                        if (((1 << s_direction) & SEARCH_DIRECTION_OBJECT_MASK)
                            && s_targetCell->m_objectIndex != MAP_CELL_NO_FRAME
                            && !(s_targetCell->m_flags & MAP_CELL_OBJECT_SHADOW_ONLY))
                            s_directionBlocked = 0;
                        if (s_directionBlocked && m_cells[s_adjacentX][s_adjacentY].visited
                            && !(s_targetCell->m_triggerType & MAP_TRIGGER_EVENT)) {
                            s_terrain = CELL_TERRAIN(s_targetCell);
                            s_adjacentCost = m_cells[s_adjacentX][s_adjacentY].distance;
                            s_stepCost[FINDPATH_STEP_STRAIGHT] =
                                s_adjacentCost
                                + CalcTerrainCost(
                                    s_terrain,
                                    FINDPATH_STEP_STRAIGHT,
                                    giCurTempMobility - s_adjacentCost,
                                    costMode
                                );
                            s_stepCost[FINDPATH_STEP_DIAGONAL] =
                                s_adjacentCost
                                + CalcTerrainCost(
                                    s_terrain,
                                    FINDPATH_STEP_DIAGONAL,
                                    giCurTempMobility - s_adjacentCost,
                                    costMode
                                );
                            PushPoint(
                                s_mapX,
                                s_mapY,
                                (s_direction + MAP_DIRECTION_OPPOSITE_OFFSET)
                                    & MAP_DIRECTION_INDEX_MASK,
                                s_stepCost[s_direction & SEARCH_DIAGONAL_COST_MASK],
                                maximumCost,
                                1,
                                0,
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
    giFullySeeded = 1;
}

// SEARCH owns retail .bss 0x004a4620-0x004a469f (SeedPosition's statics and
// this flag). Retail keeps SeedPosition's point counter initialized
// (.data 0x0048e170); that body edit waits until SeedPosition is exact.
DATA(0x004a4680)
int giFullySeeded;

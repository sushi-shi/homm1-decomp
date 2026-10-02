#include <match.h>

#include <SOURCE/FINDPATH.h>

#include <H1/KB.h>
#include <H1/Types.h>
#include <SOURCE/PATH.h>
#include <SOURCE/advManager.h>
#include <SOURCE/army.h>
#include <SOURCE/combatManager.h>
#include <SOURCE/game.h>
#include <SOURCE/mapCell.h>
#include <SOURCE/playerData.h>
#include <SOURCE/searchArray.h>

#include <stdlib.h>
#include <string.h>

// Pathfinder scratch state shared by PushPoint and TestPossibleDirections.
static int gSearchNextY;
static int gSearchNextX;
static short gSearchHigh;
static mapCell* gSearchCurrentCell;
static searchNode* gSearchQueueNode;
static short gSearchLow;
static searchNode* gSearchCell;
static int gSearchTriggerType;
static int gSearchTerrain;
static unsigned int gSearchMiddle;
static int gSearchDirection;
static mapCell* gSearchNextCell;

// Buka FINDPATH.cpp:48-51 without the heap cell pointer: HoMM1 cells are inline.
VA(0x00424810, 0xa)
searchArray::searchArray(void) {
    m_maxQueueCount = 0;
}

// Donor Clear; retail retains inline cells instead of HoMM2 heap storage.
VA(0x00424820, 0x23)
void searchArray::Clear(void) {
    memset(m_queue, 0, sizeof(m_queue));
    memset(m_cells, 0, sizeof(m_cells));
    m_pathLength = 0;
    m_queueCount = 0;
}

// Buka FINDPATH.cpp:83-88; retail retains short parameters/locals/return.
VA(0x00424850, 0x4c)
short searchArray::QuickDistance(short x1, short y1, short x2, short y2) {
    short yDistance;
    short xDistance;

    xDistance = abs(x1 - x2);
    yDistance = abs(y1 - y2);
    return ApproximateGridDistance(xDistance, yDistance);
}

// HoMM1-only per-terrain step cost that InitVars tabulates into giTerrainCost
// for both step kinds; a diagonal step costs half as much again.
VA(0x004248a0, 0x54)
short TerrainStepCost(signed char terrain, char diagonal) {
    short cost = 0;
    switch (terrain) {
    case 0:
    case 1:
    case 4:
    case 6:
        cost = 4;
        break;
    case 2:
    case 3:
        cost = 6;
        break;
    case 5:
        cost = 8;
        break;
    }
    if (diagonal & 1)
        cost += cost >> 1;
    return cost;
}

// Donor CalcTerrainCost family; HoMM1 lacks pathfinding-skill/road dimensions.
VA(0x00424900, 0x4f)
int CalcTerrainCost(int terrain, int diagonal, int mobility, int waterMode) {
    int baseCost;
    int diagonalCost;
    if (waterMode == FINDPATH_WATER_MODE)
        terrain = FINDPATH_WATER_TERRAIN;
    if (diagonal == 0)
        return giTerrainCost[terrain][diagonal];
    diagonalCost = giTerrainCost[terrain][1];
    if (diagonalCost <= mobility)
        return giTerrainCost[terrain][diagonal];
    baseCost = giTerrainCost[terrain][0];
    if (baseCost > mobility)
        baseCost = diagonalCost;
    return baseCost;
}

// Buka FINDPATH.cpp:405-582 without the moat slowdown: HoMM1 has no castle
// moat, and the retail body inlines Clear and QuickDistance.
VA(0x00424950, 0x2ff)
short searchArray::FindCombatPath(short sourceHex, short targetHex, army* unit, signed char attackPath)
{
    int bestHex;
    int direction;
    signed char attackTargetHex;
    unsigned char* path;
    searchNode node;
    int distance;
    short attackMask;
    int moveMask;
    int bestDistance;
    int opposite;

    bestDistance = 640;
    bestHex = -1;
    if (attackPath)
        attackTargetHex = (signed char)targetHex;
    else
        attackTargetHex = -1;
    Clear();
    if (!ValidHex(sourceHex) || !ValidHex(targetHex) || unit == NULL)
        return 0;
    path = m_directions;
    PushCombatPoint(sourceHex, (signed char)(unit->m_facing == 1 ? COMBAT_DIRECTION_WEST : COMBAT_DIRECTION_EAST), 0,
                    unit->m_stats.speed);
    while (m_queueCount > 0) {
        m_queueCount--;
        node = m_queue[m_queueCount];
        if (node.distance > unit->m_stats.speed)
            continue;
        distance = QuickDistance(gpCombatManager->m_hexCells[node.x].m_x, gpCombatManager->m_hexCells[node.x].m_y,
                                 gpCombatManager->m_hexCells[targetHex].m_x,
                                 gpCombatManager->m_hexCells[targetHex].m_y);
        if (unit->m_targetSide != -1) {
            attackMask = unit->GetAttackMask(node.x, 0, attackTargetHex);
            if (attackMask != 0xff) {
                for (direction = 0; direction < COMBAT_DIRECTION_COUNT; direction++) {
                    if (!(attackMask & (1 << direction))) {
                        *path++ = (unsigned char)direction;
                        m_pathLength++;
                        bestHex = node.x;
                        break;
                    }
                }
                break;
            }
        }
        if (distance < bestDistance) {
            bestHex = node.x;
            bestDistance = distance;
            if (distance == 0)
                break;
        }
        moveMask = unit->GetMoveMask(node.x);
        for (direction = 0; direction < COMBAT_DIRECTION_COUNT; direction++) {
            if (!(moveMask & (1 << direction)))
                PushCombatPoint(unit->GetAdjacentCellIndex(node.x, direction), direction, node.distance + 1,
                                unit->m_stats.speed);
        }
    }
    if (unit->m_targetSide != -1) {
        if (m_pathLength == 0)
            return 0;
    } else if (bestHex != targetHex) {
        return 0;
    }
    while (bestHex != sourceHex) {
        searchNode* cell = &m_cells[bestHex][0];

        *path++ = cell->direction;
        m_pathLength++;
        if (m_pathLength >= SEARCH_PATH_CAPACITY)
            break;
        opposite = OppositeDirection(cell->direction);
        bestHex = unit->GetAdjacentCellIndex(bestHex, opposite);
    }
    return m_pathLength;
}

// Buka FINDPATH.cpp:585-646; combat nodes use the first column of m_cells.
VA(0x00424c50, 0x13a)
void searchArray::PushCombatPoint(short hex, short direction, unsigned short distance, unsigned short speed)
{
    int low;
    int high;
    unsigned int middle;
    searchNode* node;
    searchNode* cell;

    if (!ValidHex(hex))
        return;
    high = m_queueCount;
    low = 0;
    if (speed != 0 && distance > speed)
        return;
    cell = &m_cells[hex][0];
    if (cell->visited && cell->distance <= distance)
        return;
    if (m_queueCount >= SEARCH_QUEUE_CAPACITY)
        return;
    for (;;) {
        middle = (low + high) / 2;
        node = &m_queue[middle];
        if (high <= low)
            break;
        if (distance < node->distance)
            low = middle + 1;
        else
            high = middle;
    }
    if (middle < m_queueCount)
        memmove(node + 1, node, (m_queueCount - middle) * sizeof(searchNode));
    m_queueCount++;
    if (m_queueCount > m_maxQueueCount)
        m_maxQueueCount = m_queueCount;
    node->x = (signed char)hex;
    node->y = 0;
    node->direction = direction;
    node->distance = distance;
    cell->visited = 1;
    cell->direction = direction;
    cell->distance = distance;
}

// Buka FINDPATH.cpp:113-183; HoMM1 keeps word binary-search bounds.
VA(0x00424d90, 0x2ab)
void searchArray::PushPoint(short x, short y, unsigned short direction, unsigned short cost, unsigned short mobility,
                            char occupied, char rvFlag1, signed char valueX, signed char valueY, char rvFlag2,
                            signed char previousX, signed char previousY)
{
    if (cost > mobility && mobility != 0)
        return;
    if (x < 0 || x > MAP_CELL_GRID_SIZE - 1 || y < 0 || y > MAP_CELL_GRID_SIZE - 1)
        return;
    if (m_queueCount >= SEARCH_QUEUE_CAPACITY)
        return;

    gSearchHigh = m_queueCount;
    gSearchLow = 0;
    gSearchCell = &m_cells[x][y];
    if (gSearchCell->visited) {
        if (!gSearchCell->rvFlag1 && rvFlag1)
            return;
        if (gSearchCell->distance <= cost && (!gSearchCell->rvFlag1 || rvFlag1))
            return;
    }

    for (;;) {
        gSearchMiddle = (gSearchLow + gSearchHigh) >> 1;
        gSearchQueueNode = &m_queue[gSearchMiddle];
        if (gSearchHigh <= gSearchLow)
            break;
        if (cost < gSearchQueueNode->distance)
            gSearchLow = gSearchMiddle + 1;
        else
            gSearchHigh = gSearchMiddle;
    }

    if (gSearchMiddle < m_queueCount)
        memmove(gSearchQueueNode + 1, gSearchQueueNode, (m_queueCount - gSearchMiddle) * sizeof(searchNode));
    m_queueCount++;

    if (cost > giCurTempMobility && rvFlag2 == 0) {
        gSearchQueueNode->rvFlag2 = 1;
        gSearchQueueNode->previousX = x - normalDirTable[direction].x;
        gSearchQueueNode->previousY = y - normalDirTable[direction].y;
    } else {
        gSearchQueueNode->rvFlag2 = rvFlag2;
        gSearchQueueNode->previousX = previousX;
        gSearchQueueNode->previousY = previousY;
    }
    gSearchQueueNode->x = (signed char)x;
    gSearchQueueNode->y = (signed char)y;
    gSearchQueueNode->direction = direction;
    gSearchQueueNode->distance = cost;
    gSearchQueueNode->occupied = occupied;
    gSearchQueueNode->rvFlag1 = rvFlag1;
    gSearchQueueNode->valueX = valueX;
    gSearchQueueNode->valueY = valueY;
    gSearchQueueNode->visited = 1;
    *gSearchCell = *gSearchQueueNode;
}

// Buka FINDPATH.cpp:186-316 without HoMM2's below-cell object probes.
VA(0x00425040, 0x280)
void searchArray::TestPossibleDirections(short x, short y, signed char* const terrain, signed char* const occupied,
                                         short allowOccupied, int waterMode)
{
    memset(occupied, 0, 8);
    gSearchCurrentCell = gpAdvManager->GetCell(x, y);

    for (gSearchDirection = 0; gSearchDirection < 8; gSearchDirection++) {
        gSearchNextX = x + normalDirTable[gSearchDirection].x;
        gSearchNextY = y + normalDirTable[gSearchDirection].y;
        if (gSearchNextX <= -7 || gSearchNextX >= MAP_CELL_GRID_SIZE || gSearchNextY <= -7
            || gSearchNextY >= MAP_CELL_GRID_SIZE) {
            gSearchTerrain = -1;
            goto storeDirection;
        }

        gSearchNextCell = gpAdvManager->GetCell(gSearchNextX, gSearchNextY);
        if (gSearchNextCell->m_unknown07 & 0x80) {
            gSearchTerrain = -1;
            goto storeDirection;
        }
        if (gbHumanPlayer[giCurPlayer] && !(gpGame->m_mapExtra[gSearchNextX][gSearchNextY] & giCurPlayerBit)) {
            gSearchTerrain = -1;
            goto storeDirection;
        }

        if (gSearchNextCell->m_triggerType & 0x80) {
            if (!allowOccupied) {
                if (gSearchNextX != m_specialTargetX || gSearchNextY != m_specialTargetY) {
                    gSearchTerrain = -1;
                    goto storeDirection;
                }
            } else {
                occupied[gSearchDirection] = 1;
            }
        }

        gSearchTerrain = giGroundToTerrain[gSearchNextCell->m_tileIndex];
        if (gSearchTerrain == 0) {
            if (waterMode) {
                if (gSearchNextCell->m_triggerType == 0xa3 || gSearchNextCell->m_triggerType == 0xbe) {
                    gSearchTerrain = -1;
                    goto storeDirection;
                }
            } else {
                if (gSearchNextCell->m_triggerType != 0xbd && gSearchNextCell->m_triggerType != 0xbe
                    && gSearchNextCell->m_triggerType != 0xa3) {
                    gSearchTerrain = -1;
                    goto storeDirection;
                }
            }
        } else if (waterMode && gSearchNextCell->m_triggerType != 0x1f) {
            gSearchTerrain = -1;
            goto storeDirection;
        }

        if ((1 << gSearchDirection) & 0x83) {
            if (gSearchCurrentCell->m_objectIndex != 0xff && !(gSearchCurrentCell->m_flags & 0x80)) {
                gSearchTerrain = -1;
                goto storeDirection;
            }
        } else if ((1 << gSearchDirection) & 0x38) {
            if (gSearchNextCell->m_objectIndex != 0xff && !(gSearchNextCell->m_flags & 0x80)) {
                if (gSearchNextCell->m_triggerType & 0x80) {
                    gSearchTriggerType = gSearchNextCell->m_triggerType & 0x7f;
                    if (gSearchTriggerType != 0x1a && gSearchTriggerType != 0x1d && gSearchTriggerType != 6
                        && gSearchTriggerType != 8 && gSearchTriggerType != 0xb && gSearchTriggerType != 0x30
                        && gSearchTriggerType != 2 && gSearchTriggerType != 3 && gSearchTriggerType != 4
                        && gSearchTriggerType != 9 && gSearchTriggerType != 0x1b && gSearchTriggerType != 0x24
                        && gSearchTriggerType != 0x2c && gSearchTriggerType != 0x2b) {
                        gSearchTerrain = -1;
                        goto storeDirection;
                    }
                } else {
                    gSearchTerrain = -1;
                    goto storeDirection;
                }
            }
        }

    storeDirection:
        terrain[gSearchDirection] = (signed char)gSearchTerrain;
    }
}

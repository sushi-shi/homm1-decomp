#include <match.h>

#include <SOURCE/FINDPATH.h>

#include <BASE/Misc.h>
#include <SOURCE/advManager.h>
#include <SOURCE/army.h>
#include <SOURCE/combatManager.h>
#include <SOURCE/game.h>
#include <SOURCE/KB.h>
#include <SOURCE/mapCell.h>
#include <SOURCE/mapObjectTypes.h>
#include <SOURCE/PATH.h>
#include <SOURCE/philAI.h>
#include <SOURCE/playerData.h>
#include <SOURCE/searchArray.h>

#include <stdlib.h>
#include <string.h>

// Pathfinder scratch state shared by PushPoint and TestPossibleDirections.
DATA(0x004c4f20)
static int gSearchNextY;
DATA(0x004c4f1c)
static int gSearchNextX;
DATA(0x004c4f18)
static short gSearchHigh;
DATA(0x004c4f14)
static mapCell* gSearchCurrentCell;
DATA(0x004c4f10)
static searchNode* gSearchQueueNode;
DATA(0x004c4f0c)
static short gSearchLow;
DATA(0x004c4f08)
static searchNode* gSearchCell;
DATA(0x004c4f04)
static int gSearchTriggerType;
DATA(0x004c4f00)
static int gSearchTerrain;
DATA(0x004c4ef8)
static unsigned int gSearchMiddle;
DATA(0x004c4ef4)
static int gSearchDirection;
DATA(0x004c4ef0)
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
        case TERRAIN_WATER:
        case TERRAIN_GRASS:
        case TERRAIN_LAVA:
        case TERRAIN_DIRT:
            cost = 4;
            break;
        case TERRAIN_SNOW:
        case TERRAIN_SWAMP:
            cost = 6;
            break;
        case TERRAIN_DESERT:
            cost = 8;
            break;
    }
    if (diagonal & SEARCH_DIAGONAL_COST_MASK)
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
    if (diagonal == FINDPATH_STEP_STRAIGHT)
        return giTerrainCost[terrain][diagonal];
    diagonalCost = giTerrainCost[terrain][FINDPATH_STEP_DIAGONAL];
    if (diagonalCost <= mobility)
        return giTerrainCost[terrain][diagonal];
    baseCost = giTerrainCost[terrain][FINDPATH_STEP_STRAIGHT];
    if (baseCost > mobility)
        baseCost = diagonalCost;
    return baseCost;
}

// Buka FINDPATH.cpp:405-582 without the moat slowdown: HoMM1 has no castle
// moat, and the retail body inlines Clear and QuickDistance.
VA(0x00424950, 0x2ff)
short searchArray::FindCombatPath(
    short sourceHex,
    short targetHex,
    army* unit,
    signed char attackPath
) {
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

    bestDistance = FINDPATH_INITIAL_BEST_DISTANCE;
    bestHex = ARMY_HEX_INVALID;
    if (attackPath)
        attackTargetHex = static_cast<signed char>(targetHex);
    else
        attackTargetHex = ARMY_HEX_INVALID;
    Clear();
    if (!ValidHex(sourceHex) || !ValidHex(targetHex) || unit == NULL)
        return 0;
    path = m_directions;
    PushCombatPoint(
        sourceHex,
        static_cast<signed char>(
            unit->m_facing == ARMY_FACING_LEFT ? COMBAT_DIRECTION_WEST : COMBAT_DIRECTION_EAST
        ),
        0,
        unit->m_stats.speed
    );
    while (m_queueCount > 0) {
        m_queueCount--;
        node = m_queue[m_queueCount];
        if (node.distance > unit->m_stats.speed)
            continue;
        distance = QuickDistance(
            gpCombatManager->m_hexCells[node.x].m_x,
            gpCombatManager->m_hexCells[node.x].m_y,
            gpCombatManager->m_hexCells[targetHex].m_x,
            gpCombatManager->m_hexCells[targetHex].m_y
        );
        if (unit->m_targetSide != COMBAT_SIDE_NONE) {
            attackMask = unit->GetAttackMask(node.x, ARMY_ATTACK_TARGET_ASSIGNED, attackTargetHex);
            if (attackMask != COMBAT_ALL_DIRECTIONS_BLOCKED) {
                for (direction = 0; direction < COMBAT_DIRECTION_COUNT; direction++) {
                    if (!(attackMask & (1 << direction))) {
                        *path++ = static_cast<unsigned char>(direction);
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
                PushCombatPoint(
                    unit->GetAdjacentCellIndex(node.x, direction),
                    direction,
                    node.distance + 1,
                    unit->m_stats.speed
                );
        }
    }
    if (unit->m_targetSide != COMBAT_SIDE_NONE) {
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
void searchArray::PushCombatPoint(
    short hex,
    short direction,
    unsigned short distance,
    unsigned short speed
) {
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
    node->x = static_cast<signed char>(hex);
    node->y = 0;
    node->direction = direction;
    node->distance = distance;
    cell->visited = 1;
    cell->direction = direction;
    cell->distance = distance;
}

// Buka FINDPATH.cpp:113-183; HoMM1 keeps word binary-search bounds.
VA(0x00424d90, 0x2ab)
void searchArray::PushPoint(
    short x,
    short y,
    unsigned short direction,
    unsigned short cost,
    unsigned short mobility,
    char occupied,
    char rvFlag1,
    signed char valueX,
    signed char valueY,
    char rvFlag2,
    signed char previousX,
    signed char previousY
) {
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
        memmove(
            gSearchQueueNode + 1,
            gSearchQueueNode,
            (m_queueCount - gSearchMiddle) * sizeof(searchNode)
        );
    m_queueCount++;

    if (cost > gCurTempMobility && rvFlag2 == 0) {
        gSearchQueueNode->rvFlag2 = 1;
        gSearchQueueNode->previousX = x - normalDirTable[direction].x;
        gSearchQueueNode->previousY = y - normalDirTable[direction].y;
    } else {
        gSearchQueueNode->rvFlag2 = rvFlag2;
        gSearchQueueNode->previousX = previousX;
        gSearchQueueNode->previousY = previousY;
    }
    gSearchQueueNode->x = static_cast<signed char>(x);
    gSearchQueueNode->y = static_cast<signed char>(y);
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
void searchArray::TestPossibleDirections(
    short x,
    short y,
    signed char* const terrain,
    signed char* const occupied,
    short allowOccupied,
    int waterMode
) {
    memset(occupied, 0, MAP_DIRECTION_COUNT);
    gSearchCurrentCell = gpAdvManager->GetCell(x, y);

    for (gSearchDirection = 0; gSearchDirection < MAP_DIRECTION_COUNT; gSearchDirection++) {
        gSearchNextX = x + normalDirTable[gSearchDirection].x;
        gSearchNextY = y + normalDirTable[gSearchDirection].y;
        if (gSearchNextX <= -7 || gSearchNextX >= MAP_CELL_GRID_SIZE || gSearchNextY <= -7
            || gSearchNextY >= MAP_CELL_GRID_SIZE) {
            gSearchTerrain = TERRAIN_INVALID;
            goto storeDirection;
        }

        gSearchNextCell = gpAdvManager->GetCell(gSearchNextX, gSearchNextY);
        if (gSearchNextCell->m_secondaryTrigger & MAP_CELL_SECONDARY_BLOCKED) {
            gSearchTerrain = TERRAIN_INVALID;
            goto storeDirection;
        }
        if (gbHumanPlayer[giCurPlayer]
            && !(gpGame->m_mapExtra[gSearchNextX][gSearchNextY] & giCurPlayerBit)) {
            gSearchTerrain = TERRAIN_INVALID;
            goto storeDirection;
        }

        if (gSearchNextCell->m_triggerType & MAP_TRIGGER_EVENT) {
            if (!allowOccupied) {
                if (gSearchNextX != m_specialTargetX || gSearchNextY != m_specialTargetY) {
                    gSearchTerrain = TERRAIN_INVALID;
                    goto storeDirection;
                }
            } else {
                occupied[gSearchDirection] = 1;
            }
        }

        gSearchTerrain = CELL_TERRAIN(gSearchNextCell);
        if (gSearchTerrain == TERRAIN_WATER) {
            if (waterMode) {
                if (gSearchNextCell->m_triggerType == (MAP_TRIGGER_EVENT | MAP_OBJECT_SHIPWRECK)
                    || gSearchNextCell->m_triggerType == (MAP_TRIGGER_EVENT | MAP_OBJECT_SHIP)) {
                    gSearchTerrain = TERRAIN_INVALID;
                    goto storeDirection;
                }
            } else {
                if (gSearchNextCell->m_triggerType != (MAP_TRIGGER_EVENT | MAP_OBJECT_HERO)
                    && gSearchNextCell->m_triggerType != (MAP_TRIGGER_EVENT | MAP_OBJECT_SHIP)
                    && gSearchNextCell->m_triggerType
                           != (MAP_TRIGGER_EVENT | MAP_OBJECT_SHIPWRECK)) {
                    gSearchTerrain = TERRAIN_INVALID;
                    goto storeDirection;
                }
            }
        } else if (waterMode && gSearchNextCell->m_triggerType != MAP_OBJECT_COAST) {
            gSearchTerrain = TERRAIN_INVALID;
            goto storeDirection;
        }

        if ((1 << gSearchDirection) & SEARCH_DIRECTION_EDGE_OBJECT_MASK) {
            if (gSearchCurrentCell->m_objectIndex != MAP_CELL_NO_FRAME
                && !(gSearchCurrentCell->m_flags & MAP_CELL_OBJECT_SHADOW_ONLY)) {
                gSearchTerrain = TERRAIN_INVALID;
                goto storeDirection;
            }
        } else if ((1 << gSearchDirection) & SEARCH_DIRECTION_OBJECT_MASK) {
            if (gSearchNextCell->m_objectIndex != MAP_CELL_NO_FRAME
                && !(gSearchNextCell->m_flags & MAP_CELL_OBJECT_SHADOW_ONLY)) {
                if (gSearchNextCell->m_triggerType & MAP_TRIGGER_EVENT) {
                    gSearchTriggerType = gSearchNextCell->m_triggerType & MAP_TRIGGER_TYPE_MASK;
                    if (gSearchTriggerType != MAP_OBJECT_MONSTER
                        && gSearchTriggerType != MAP_OBJECT_RESOURCE
                        && gSearchTriggerType != MAP_OBJECT_TREASURE_CHEST
                        && gSearchTriggerType != MAP_OBJECT_CAMPFIRE
                        && gSearchTriggerType != MAP_OBJECT_ANCIENT_LAMP
                        && gSearchTriggerType != MAP_OBJECT_ARTIFACT
                        && gSearchTriggerType != MAP_OBJECT_SIGNPOST
                        && gSearchTriggerType != MAP_OBJECT_BUOY
                        && gSearchTriggerType != MAP_OBJECT_SKELETON
                        && gSearchTriggerType != MAP_OBJECT_FOUNTAIN
                        && gSearchTriggerType != MAP_OBJECT_OBELISK
                        && gSearchTriggerType != MAP_OBJECT_STATUE
                        && gSearchTriggerType != MAP_OBJECT_WHIRLPOOL
                        && gSearchTriggerType != MAP_OBJECT_WELL) {
                        gSearchTerrain = TERRAIN_INVALID;
                        goto storeDirection;
                    }
                } else {
                    gSearchTerrain = TERRAIN_INVALID;
                    goto storeDirection;
                }
            }
        }

    storeDirection:
        terrain[gSearchDirection] = static_cast<signed char>(gSearchTerrain);
    }
}

// FINDPATH owns retail .bss 0x004c4ef0-0x004c4f2b: the search statics above
// and the working mobility SEARCH seeds.
DATA(0x004c4efc)
short gCurTempMobility;

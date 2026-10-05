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
DATA(0x004a6bbc)
static i32 gSearchNeighborY;
DATA(0x004a6bb8)
static i32 gSearchNeighborX;
DATA(0x004a6bc8)
static i16 gSearchHigh;
DATA(0x004a6bb4)
static mapCell* gSearchCurrentCell;
DATA(0x004a6b9c)
static searchNode* gSearchQueueSlot;
DATA(0x004a6ba8)
static i16 gSearchLow;
DATA(0x004a6ba4)
static searchNode* gSearchCell;
DATA(0x004a6bb0)
static i32 gSearchObjectType;
DATA(0x004a6bc4)
static i32 gSearchTerrainType;
DATA(0x004a6ba0)
static u32 gSearchPivot;
DATA(0x004a6bcc)
static i32 gSearchHeading;
DATA(0x004a6bac)
static mapCell* gScanNeighborCell;

VA(0x00429c60, 0xa)
searchArray::searchArray(void) {
    m_maxQueueCount = 0;
}

VA(0x00429c70, 0x23)
void searchArray::Clear(void) {
    memset(m_queue, 0, sizeof(m_queue));
    memset(m_cells, 0, sizeof(m_cells));
    m_pathLength = 0;
    m_queueCount = 0;
}

VA(0x00429ca0, 0x4e)
i16 searchArray::QuickDistance(i16 x1, i16 y1, i16 x2, i16 y2) {
    i16 yDistance;
    i16 xDistance;

    xDistance = abs(x1 - x2);
    yDistance = abs(y1 - y2);
    return xDistance < yDistance ? yDistance + xDistance / DISTANCE_MINOR_DIVISOR
                                 : xDistance + yDistance / DISTANCE_MINOR_DIVISOR;
}

// HoMM1-only per-terrain step cost that InitVars tabulates into giTerrainCost
// for both step kinds; a diagonal step costs half as much again.
VA(0x00429cf0, 0x54)
i16 TerrainStepCost(i8 terrain, i8 diagonal) {
    i16 cost = 0;
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

VA(0x00429d50, 0x4e)
i32 CalcTerrainCost(i32 terrain, i32 diagonal, i32 mobility, i32 waterMode) {
    if (waterMode == FINDPATH_WATER_MODE)
        terrain = FINDPATH_WATER_TERRAIN;
    if (diagonal == FINDPATH_STEP_STRAIGHT)
        return giTerrainCost[terrain][diagonal];
    if (mobility >= giTerrainCost[terrain][FINDPATH_STEP_DIAGONAL])
        return giTerrainCost[terrain][diagonal];
    if (mobility >= giTerrainCost[terrain][FINDPATH_STEP_STRAIGHT])
        return giTerrainCost[terrain][FINDPATH_STEP_STRAIGHT];
    return giTerrainCost[terrain][FINDPATH_STEP_DIAGONAL];
}

// HoMM1 has no castle moat, so combat paths take no moat slowdown.
VA(0x00429da0, 0x2cb)
i16 searchArray::FindCombatPath(i16 sourceHex, i16 targetHex, army* unit, i8 attackPath) {
    i32 bestHex;
    i32 direction;
    i8 attackTargetHex;
    u8* path;
    searchNode node;
    i32 distance;
    i16 attackMask;
    i16 moveMask;
    i32 bestDistance;
    i32 opposite;

    bestDistance = FINDPATH_INITIAL_BEST_DISTANCE;
    bestHex = ARMY_HEX_INVALID;
    if (attackPath)
        attackTargetHex = static_cast<i8>(targetHex);
    else
        attackTargetHex = ARMY_HEX_INVALID;
    Clear();
    if (!ValidHex(sourceHex) || !ValidHex(targetHex) || unit == NULL)
        return 0;
    path = m_directions;
    PushCombatPoint(
        sourceHex,
        static_cast<i8>(
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
                        *path++ = static_cast<u8>(direction);
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

// Combat nodes use the first column of m_cells.
VA(0x0042a070, 0x124)
void searchArray::PushCombatPoint(i16 hex, i16 direction, u16 distance, u16 speed) {
    i32 low;
    i32 high;
    u32 middle;
    searchNode* node;
    searchNode* cell;

    if (!ValidHex(hex))
        return;
    high = m_queueCount;
    low = 0;
    if (speed > 0 && distance > speed)
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
    node->x = static_cast<i8>(hex);
    node->y = 0;
    node->direction = direction;
    node->distance = distance;
    cell->visited = 1;
    cell->direction = direction;
    cell->distance = distance;
}

VA(0x0042a1a0, 0x299)
void searchArray::PushPoint(
    i16 x,
    i16 y,
    u16 direction,
    u16 cost,
    u16 mobility,
    i8 occupied,
    i8 rvFlag1,
    i8 valueX,
    i8 valueY,
    i8 rvFlag2,
    i8 previousX,
    i8 previousY
) {
    if (cost > mobility && mobility > 0)
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
        gSearchPivot = (gSearchLow + gSearchHigh) >> 1;
        gSearchQueueSlot = &m_queue[gSearchPivot];
        if (gSearchHigh <= gSearchLow)
            break;
        if (cost < gSearchQueueSlot->distance)
            gSearchLow = gSearchPivot + 1;
        else
            gSearchHigh = gSearchPivot;
    }

    if (gSearchPivot < m_queueCount)
        memmove(
            gSearchQueueSlot + 1,
            gSearchQueueSlot,
            (m_queueCount - gSearchPivot) * sizeof(searchNode)
        );
    m_queueCount++;

    if (cost > gCurTempMobility && rvFlag2 == 0) {
        gSearchQueueSlot->rvFlag2 = 1;
        gSearchQueueSlot->previousX = x - normalDirTable[direction].x;
        gSearchQueueSlot->previousY = y - normalDirTable[direction].y;
    } else {
        gSearchQueueSlot->rvFlag2 = rvFlag2;
        gSearchQueueSlot->previousX = previousX;
        gSearchQueueSlot->previousY = previousY;
    }
    gSearchQueueSlot->x = static_cast<i8>(x);
    gSearchQueueSlot->y = static_cast<i8>(y);
    gSearchQueueSlot->direction = direction;
    gSearchQueueSlot->distance = cost;
    gSearchQueueSlot->occupied = occupied;
    gSearchQueueSlot->rvFlag1 = rvFlag1;
    gSearchQueueSlot->valueX = valueX;
    gSearchQueueSlot->valueY = valueY;
    gSearchQueueSlot->visited = 1;
    *gSearchCell = *gSearchQueueSlot;
}

VA(0x0042a440, 0x234)
void searchArray::TestPossibleDirections(
    i16 x,
    i16 y,
    i8* const terrain,
    i8* const occupied,
    i16 allowOccupied,
    i32 waterMode
) {
    memset(occupied, 0, MAP_DIRECTION_COUNT);
    gSearchCurrentCell = gpAdvManager->GetCell(x, y);

    for (gSearchHeading = 0; gSearchHeading < MAP_DIRECTION_COUNT; gSearchHeading++) {
        gSearchNeighborX = x + normalDirTable[gSearchHeading].x;
        gSearchNeighborY = y + normalDirTable[gSearchHeading].y;
        if (gSearchNeighborX <= -7 || gSearchNeighborX >= MAP_CELL_GRID_SIZE || gSearchNeighborY <= -7
            || gSearchNeighborY >= MAP_CELL_GRID_SIZE) {
            gSearchTerrainType = TERRAIN_INVALID;
            goto storeDirection;
        }

        gScanNeighborCell = gpAdvManager->GetCell(gSearchNeighborX, gSearchNeighborY);
        if (gScanNeighborCell->m_secondaryTrigger & MAP_CELL_SECONDARY_BLOCKED) {
            gSearchTerrainType = TERRAIN_INVALID;
            goto storeDirection;
        }
        if (gbHumanPlayer[giCurPlayer]
            && !(gpGame->m_mapExtra[gSearchNeighborX][gSearchNeighborY] & giCurPlayerBit)) {
            gSearchTerrainType = TERRAIN_INVALID;
            goto storeDirection;
        }

        if (gScanNeighborCell->m_triggerType & MAP_TRIGGER_EVENT) {
            if (!allowOccupied) {
                if (gSearchNeighborX != m_specialTargetX || gSearchNeighborY != m_specialTargetY) {
                    gSearchTerrainType = TERRAIN_INVALID;
                    goto storeDirection;
                }
            } else {
                occupied[gSearchHeading] = 1;
            }
        }

        gSearchTerrainType = CELL_TERRAIN(gScanNeighborCell);
        if (gSearchTerrainType == TERRAIN_WATER) {
            if (waterMode) {
                if (gScanNeighborCell->m_triggerType == (MAP_TRIGGER_EVENT | MAP_OBJECT_SHIPWRECK)
                    || gScanNeighborCell->m_triggerType == (MAP_TRIGGER_EVENT | MAP_OBJECT_SHIP)) {
                    gSearchTerrainType = TERRAIN_INVALID;
                    goto storeDirection;
                }
            } else {
                if (gScanNeighborCell->m_triggerType != (MAP_TRIGGER_EVENT | MAP_OBJECT_HERO)
                    && gScanNeighborCell->m_triggerType != (MAP_TRIGGER_EVENT | MAP_OBJECT_SHIP)
                    && gScanNeighborCell->m_triggerType
                           != (MAP_TRIGGER_EVENT | MAP_OBJECT_SHIPWRECK)) {
                    gSearchTerrainType = TERRAIN_INVALID;
                    goto storeDirection;
                }
            }
        } else if (waterMode && gScanNeighborCell->m_triggerType != MAP_OBJECT_COAST) {
            gSearchTerrainType = TERRAIN_INVALID;
            goto storeDirection;
        }

        if ((1 << gSearchHeading) & SEARCH_DIRECTION_EDGE_OBJECT_MASK) {
            if (CELL_HAS_NON_SHADOW_OBJECT(gSearchCurrentCell)) {
                gSearchTerrainType = TERRAIN_INVALID;
                goto storeDirection;
            }
        } else if ((1 << gSearchHeading) & SEARCH_DIRECTION_OBJECT_MASK) {
            if (CELL_HAS_NON_SHADOW_OBJECT(gScanNeighborCell)) {
                if (gScanNeighborCell->m_triggerType & MAP_TRIGGER_EVENT) {
                    gSearchObjectType = gScanNeighborCell->m_triggerType & MAP_TRIGGER_TYPE_MASK;
                    if (gSearchObjectType != MAP_OBJECT_MONSTER
                        && gSearchObjectType != MAP_OBJECT_RESOURCE
                        && gSearchObjectType != MAP_OBJECT_TREASURE_CHEST
                        && gSearchObjectType != MAP_OBJECT_CAMPFIRE
                        && gSearchObjectType != MAP_OBJECT_ANCIENT_LAMP
                        && gSearchObjectType != MAP_OBJECT_ARTIFACT
                        && gSearchObjectType != MAP_OBJECT_SIGNPOST
                        && gSearchObjectType != MAP_OBJECT_BUOY
                        && gSearchObjectType != MAP_OBJECT_SKELETON
                        && gSearchObjectType != MAP_OBJECT_FOUNTAIN
                        && gSearchObjectType != MAP_OBJECT_OBELISK
                        && gSearchObjectType != MAP_OBJECT_STATUE
                        && gSearchObjectType != MAP_OBJECT_WHIRLPOOL
                        && gSearchObjectType != MAP_OBJECT_WELL) {
                        gSearchTerrainType = TERRAIN_INVALID;
                        goto storeDirection;
                    }
                } else {
                    gSearchTerrainType = TERRAIN_INVALID;
                    goto storeDirection;
                }
            }
        }

    storeDirection:
        terrain[gSearchHeading] = static_cast<i8>(gSearchTerrainType);
    }
}

// FINDPATH scratch occupies 0x004a6b9c-0x004a6bcf, including
// the working mobility SEARCH seeds.
DATA(0x004a6bc0)
i16 gCurTempMobility;

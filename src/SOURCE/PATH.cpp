#include <H1/Ints.h>

#include <SOURCE/PATH.h>

#include <BASE/Misc.h>
#include <SOURCE/advManager.h>
#include <SOURCE/army.h>
#include <SOURCE/combatManager.h>
#include <SOURCE/KB.h>
#include <SOURCE/kbwin.h>
#include <SOURCE/searchArray.h>

i16 army::FindPath(
    i16 sourceHex,
    i16 targetHex,
    i8 speed,
    b8 ignoreSpeed,
    i8 pathMode
) {
    i16 retVal;
    i32 savedSpeed;

    if (!ValidHex(sourceHex) || !ValidHex(targetHex))
        return 0;
    savedSpeed = m_stats.speed;
    if (ignoreSpeed)
        m_stats.speed = IGNORE_SPEED;
    retVal = gSearchArray->FindCombatPath(sourceHex, targetHex, this, pathMode);
    if (!retVal && (m_stats.attributes & MONSTER_FLAGS_WIDE) && !ARMY_PATH_ATTACKS(pathMode)) {
        switch (m_facing) {
            case ARMY_FACING_LEFT:
                targetHex = GetAdjacentCellIndex(targetHex, COMBAT_DIRECTION_EAST);
                break;
            case ARMY_FACING_RIGHT:
                targetHex = GetAdjacentCellIndex(targetHex, COMBAT_DIRECTION_WEST);
                break;
        }
        if (!ValidHex(targetHex))
            retVal = 0;
        else
            retVal = gSearchArray->FindCombatPath(sourceHex, targetHex, this, pathMode);
    }
    m_stats.speed = savedSpeed;
    return retVal;
}

i16 army::ValidPath(i16 targetHex, i8 pathMode) {
    i32 pathResult;
    i32 unusedExtra;

    if (!ValidHex(targetHex))
        return 0;
    if (m_stats.attributes & MONSTER_FLAGS_FLYING)
        return ValidFlight(targetHex, pathMode);
    pathResult = FindPath(m_hex, targetHex, m_stats.speed, false, pathMode);
    if (pathResult) {
        m_moveTargetHex = targetHex;
        return 1;
    }
    return 0;
}

i16 army::GetMoveMask(i16 sourceHex) {
    i16 blockedMaskVal;
    i16 mask;
    i16 direction;

    blockedMaskVal = 0;
    mask = 1;
    for (direction = COMBAT_DIRECTION_ADJACENT_FIRST; direction <= COMBAT_DIRECTION_ADJACENT_LAST;
         direction++) {
        if (!ValidMove(sourceHex, direction))
            blockedMaskVal |= mask;
        mask <<= 1;
    }
    return blockedMaskVal | (1 << COMBAT_DIRECTION_WIDE_NORTH)
           | (1 << COMBAT_DIRECTION_WIDE_SOUTH);
}

i16 army::GetAttackMask(
    i16 sourceHex,
    i8 targetMode,
    i8 targetHex
) {
    i16 theDirection;
    i16 hex;
    i16 curDirBit;
    i16 curMask;
    i16 nDirectionCount;

    curMask = (m_stats.attributes & MONSTER_FLAGS_WIDE) ? 0 : WIDE_DIRECTIONS_MASK;
    curDirBit = 1;
    nDirectionCount = (m_stats.attributes & MONSTER_FLAGS_WIDE) ? COMBAT_DIRECTION_COUNT
                                                                : COMBAT_DIRECTION_ADJACENT_COUNT;
    for (theDirection = COMBAT_DIRECTION_ADJACENT_FIRST; theDirection < nDirectionCount;
         theDirection++) {
        if (!ValidAttack(sourceHex, theDirection, targetMode, targetHex, &hex))
            curMask |= curDirBit;
        curDirBit <<= 1;
    }
    return curMask;
}

i16 army::ValidMove(i16 direction) {
    return ValidMove(m_hex, direction);
}

i16 army::ValidMove(i16 sourceHex, i16 direction) {
    i16 destHexNext;
    i8 rearSquare;
    b8 frontValid;
    b8 rearValidResult;

    if (!ValidHex(sourceHex))
        return 0;
    destHexNext = GetAdjacentCellIndex(sourceHex, direction);
    if (!ValidHex(destHexNext))
        return 0;
    frontValid = false;
    if (gCombatManager->m_hexCells[destHexNext].m_occupantSide == COMBAT_SIDE_NONE
        && gCombatManager->m_hexCells[destHexNext].m_obstacleIndex == COMBAT_OBSTACLE_NONE)
        frontValid = true;
    if (m_stats.attributes & MONSTER_FLAGS_WIDE) {
        rearSquare = ARMY_HEX_INVALID;
        switch (m_facing) {
            case ARMY_FACING_LEFT:
                if (direction == COMBAT_DIRECTION_EAST)
                    return frontValid;
                else
                    rearSquare = GetAdjacentCellIndex(destHexNext, COMBAT_DIRECTION_WEST);
                break;
            case ARMY_FACING_RIGHT:
                if (direction == COMBAT_DIRECTION_WEST)
                    return frontValid;
                else
                    rearSquare = GetAdjacentCellIndex(destHexNext, COMBAT_DIRECTION_EAST);
                break;
        }
        rearValidResult = false;
        if (ValidHex(rearSquare)
            && gCombatManager->m_hexCells[rearSquare].m_occupantSide == COMBAT_SIDE_NONE
            && gCombatManager->m_hexCells[rearSquare].m_obstacleIndex == COMBAT_OBSTACLE_NONE)
            rearValidResult = true;
        if (direction == COMBAT_DIRECTION_EAST || direction == COMBAT_DIRECTION_WEST)
            return rearValidResult;
        else {
            if (frontValid == true && rearValidResult == true)
                return 1;
            else
                return 0;
        }
    } else
        return frontValid;
}

i16 army::ValidAttack(
    i16 sourceHex,
    i16 direction,
    i16 targetMode,
    i16 requiredTargetHex,
    i16* attackHex
) {
    i16 adjacentHex;
    i8 occupantSide;

    if (!ValidHex(sourceHex))
        return 0;
    adjacentHex = sourceHex;
    if (m_stats.attributes & MONSTER_FLAGS_WIDE) {
        if (direction == COMBAT_DIRECTION_WIDE_NORTH)
            *attackHex = GetAdjacentCellIndex(
                sourceHex,
                m_facing == ARMY_FACING_LEFT
                    ? static_cast<i8>(COMBAT_DIRECTION_NORTHWEST)
                    : static_cast<i8>(COMBAT_DIRECTION_NORTHEAST)
            );
        else if (direction == COMBAT_DIRECTION_WIDE_SOUTH)
            *attackHex = GetAdjacentCellIndex(
                sourceHex,
                m_facing == ARMY_FACING_LEFT
                    ? static_cast<i8>(COMBAT_DIRECTION_SOUTHWEST)
                    : static_cast<i8>(COMBAT_DIRECTION_SOUTHEAST)
            );
        else {
            switch (m_facing) {
                case ARMY_FACING_LEFT:
                    if (direction >= COMBAT_DIRECTION_WESTERN_FIRST)
                        adjacentHex = GetAdjacentCellIndex(sourceHex, COMBAT_DIRECTION_WEST);
                    break;
                case ARMY_FACING_RIGHT:
                    if (direction <= COMBAT_DIRECTION_EASTERN_LAST)
                        adjacentHex = GetAdjacentCellIndex(sourceHex, COMBAT_DIRECTION_EAST);
                    break;
            }
            if (adjacentHex == ARMY_HEX_INVALID)
                return 0;
            *attackHex = GetAdjacentCellIndex(adjacentHex, direction);
        }
    } else
        *attackHex = GetAdjacentCellIndex(sourceHex, direction);
    if (!ValidHex(*attackHex))
        return 0;
    if (requiredTargetHex != ARMY_HEX_INVALID && *attackHex != requiredTargetHex)
        return 0;
    occupantSide = gCombatManager->m_hexCells[*attackHex].m_occupantSide;
    switch (targetMode) {
        case ARMY_ATTACK_TARGET_ASSIGNED:
            if (occupantSide == m_targetSide
                && gCombatManager->m_hexCells[*attackHex].m_occupantIndex == m_targetIndex)
                return 1;
            break;
        case ARMY_ATTACK_TARGET_ENEMY:
            if (occupantSide == COMBAT_OPPOSING_SIDE(gCombatManager->m_currentSide))
                return 1;
            break;
        case ARMY_ATTACK_TARGET_OCCUPIED:
            if (occupantSide != COMBAT_SIDE_NONE)
                return 1;
            break;
    }
    return 0;
}

i16 army::GetAdjacentCellIndex(i16 hex, i16 direction)
{
    if (hex == ARMY_HEX_INVALID)
        return ARMY_HEX_INVALID;
    if (direction == COMBAT_DIRECTION_WIDE_NORTH)
        direction = m_facing == ARMY_FACING_RIGHT
                        ? static_cast<i8>(COMBAT_DIRECTION_NORTHWEST)
                        : static_cast<i8>(COMBAT_DIRECTION_NORTHEAST);
    else if (direction == COMBAT_DIRECTION_WIDE_SOUTH)
        direction = m_facing == ARMY_FACING_RIGHT
                        ? static_cast<i8>(COMBAT_DIRECTION_SOUTHWEST)
                        : static_cast<i8>(COMBAT_DIRECTION_SOUTHEAST);
    H1_ASSERT(direction >= COMBAT_DIRECTION_ADJACENT_FIRST && direction < COMBAT_DIRECTION_ADJACENT_COUNT);
    H1_ASSERT(hex >= 0 && hex < COMBAT_HEX_COUNT);
    return gCombatAdjacency[hex][direction];
}

i16 GetAdjacentCellIndexNoArmy(i16 hex, i16 direction)
{
    if (hex == ARMY_HEX_INVALID)
        return ARMY_HEX_INVALID;
    if (direction == COMBAT_DIRECTION_WIDE_NORTH)
        direction = COMBAT_DIRECTION_NORTHWEST;
    else if (direction == COMBAT_DIRECTION_WIDE_SOUTH)
        direction = COMBAT_DIRECTION_SOUTHWEST;
    H1_ASSERT(direction >= COMBAT_DIRECTION_ADJACENT_FIRST && direction < COMBAT_DIRECTION_ADJACENT_COUNT);
    H1_ASSERT(hex >= 0 && hex < COMBAT_HEX_COUNT);
    return gCombatAdjacency[hex][direction];
}

i16 army::ValidRange(i16 targetHex) {
    i16 adjacentHex;
    i16 res;

    if (!ValidHex(targetHex))
        return 0;
    m_moveTargetHex = m_hex;
    if (!(m_stats.attributes & MONSTER_FLAGS_WIDE)) {
        m_attackDirection = GetBestDirection(m_hex, targetHex, WIDE_DIRECTIONS_MASK);
        adjacentHex = GetAdjacentCellIndex(m_hex, m_attackDirection);
        if (adjacentHex == targetHex)
            return 1;
        adjacentHex = GetAdjacentCellIndex(adjacentHex, m_attackDirection);
        if (adjacentHex == targetHex)
            return 1;
    } else {
        switch (m_facing) {
            case ARMY_FACING_RIGHT:
                res = GetBestDirection(m_hex, targetHex, WIDE_DIRECTIONS_MASK);
                if (res > COMBAT_DIRECTION_EASTERN_LAST) {
                    m_attackDirection = res;
                    adjacentHex = GetAdjacentCellIndex(m_hex, res);
                    if (adjacentHex == targetHex)
                        return 1;
                    adjacentHex = GetAdjacentCellIndex(adjacentHex, res);
                    if (adjacentHex == targetHex)
                        return 1;
                }
                res = GetBestDirection(m_hex + WIDE_HEX_OFFSET, targetHex, WIDE_DIRECTIONS_MASK);
                if (res < COMBAT_DIRECTION_WESTERN_FIRST) {
                    m_attackDirection = res;
                    adjacentHex = GetAdjacentCellIndex(m_hex + WIDE_HEX_OFFSET, res);
                    if (adjacentHex == targetHex)
                        return 1;
                    adjacentHex = GetAdjacentCellIndex(adjacentHex, res);
                    if (adjacentHex == targetHex)
                        return 1;
                }
                if (res == COMBAT_DIRECTION_WEST)
                    return 0;
                if (res == COMBAT_DIRECTION_NORTHWEST)
                    m_attackDirection = COMBAT_DIRECTION_WIDE_NORTH;
                else if (res == COMBAT_DIRECTION_SOUTHWEST)
                    m_attackDirection = COMBAT_DIRECTION_WIDE_SOUTH;
                adjacentHex = GetAdjacentCellIndex(m_hex + WIDE_HEX_OFFSET, res);
                if (adjacentHex == targetHex)
                    return 1;
                adjacentHex = GetAdjacentCellIndex(adjacentHex, res);
                if (adjacentHex == targetHex)
                    return 1;
                break;
            case ARMY_FACING_LEFT:
                res = GetBestDirection(m_hex, targetHex, WIDE_DIRECTIONS_MASK);
                if (res < COMBAT_DIRECTION_WESTERN_FIRST) {
                    m_attackDirection = res;
                    adjacentHex = GetAdjacentCellIndex(m_hex, res);
                    if (adjacentHex == targetHex)
                        return 1;
                    adjacentHex = GetAdjacentCellIndex(adjacentHex, res);
                    if (adjacentHex == targetHex)
                        return 1;
                    return 0;
                }
                res = GetBestDirection(m_hex - WIDE_HEX_OFFSET, targetHex, WIDE_DIRECTIONS_MASK);
                if (res > COMBAT_DIRECTION_EASTERN_LAST) {
                    m_attackDirection = res;
                    adjacentHex = GetAdjacentCellIndex(m_hex - WIDE_HEX_OFFSET, res);
                    if (adjacentHex == targetHex)
                        return 1;
                    adjacentHex = GetAdjacentCellIndex(adjacentHex, res);
                    if (adjacentHex == targetHex)
                        return 1;
                    return 0;
                }
                if (res == COMBAT_DIRECTION_EAST)
                    return 0;
                if (res == COMBAT_DIRECTION_NORTHEAST)
                    m_attackDirection = COMBAT_DIRECTION_WIDE_NORTH;
                else if (res == COMBAT_DIRECTION_SOUTHEAST)
                    m_attackDirection = COMBAT_DIRECTION_WIDE_SOUTH;
                adjacentHex = GetAdjacentCellIndex(m_hex - WIDE_HEX_OFFSET, res);
                if (adjacentHex == targetHex)
                    return 1;
                adjacentHex = GetAdjacentCellIndex(adjacentHex, res);
                if (adjacentHex == targetHex)
                    return 1;
                break;
        }
    }
    return 0;
}

i16
OppositeDirection(i16 direction) {
    if (direction < COMBAT_DIRECTION_ADJACENT_COUNT)
        return static_cast<i16>((static_cast<i32>(direction) + COMBAT_DIRECTION_OPPOSITE_OFFSET) % static_cast<i32>(COMBAT_DIRECTION_ADJACENT_COUNT));
    else {
        if (direction == COMBAT_DIRECTION_WIDE_NORTH)
            return COMBAT_DIRECTION_WIDE_SOUTH;
        else
            return COMBAT_DIRECTION_WIDE_NORTH;
    }
}

i16
army::GetBestDirection(i16 sourceHex, i16 targetHex, i16 blockedMask) {
    b8 isMovingRight;
    i8 curSourceRow;
    i8 targetRow;
    b8 goingUp;
    b8 goingLeft;
    i8 srcCol;
    i8 targetColumn;
    b8 isMovingDown;

    if (!ValidHex(sourceHex) || !ValidHex(targetHex))
        return COMBAT_DIRECTION_INVALID;
    srcCol = sourceHex % COMBAT_GRID_COLUMNS;
    curSourceRow = sourceHex / COMBAT_GRID_COLUMNS;
    targetColumn = targetHex % COMBAT_GRID_COLUMNS;
    targetRow = targetHex / COMBAT_GRID_COLUMNS;
    goingUp = false;
    isMovingDown = false;
    goingLeft = false;
    isMovingRight = false;
    if (targetColumn > srcCol)
        isMovingRight = true;
    else if (targetColumn != srcCol)
        goingLeft = true;
    if (targetRow > curSourceRow)
        isMovingDown = true;
    else if (targetRow != curSourceRow)
        goingUp = true;
    if (isMovingRight == goingLeft) {
        if (goingUp == true) {
            if (curSourceRow & 1) {
                if (!(blockedMask & COMBAT_DIRECTION_BIT_NORTHWEST))
                    return COMBAT_DIRECTION_NORTHWEST;
                else if (!(blockedMask & COMBAT_DIRECTION_BIT_NORTHEAST))
                    return COMBAT_DIRECTION_NORTHEAST;
                else if (!(blockedMask & COMBAT_DIRECTION_BIT_WEST))
                    return COMBAT_DIRECTION_WEST;
                else if (!(blockedMask & COMBAT_DIRECTION_BIT_EAST))
                    return COMBAT_DIRECTION_EAST;
                else if (!(blockedMask & COMBAT_DIRECTION_BIT_SOUTHWEST))
                    return COMBAT_DIRECTION_SOUTHWEST;
                else if (!(blockedMask & COMBAT_DIRECTION_BIT_SOUTHEAST))
                    return COMBAT_DIRECTION_SOUTHEAST;
                else if (!(blockedMask & COMBAT_DIRECTION_BIT_WIDE_NORTH))
                    return COMBAT_DIRECTION_WIDE_NORTH;
                else if (!(blockedMask & COMBAT_DIRECTION_BIT_WIDE_SOUTH))
                    return COMBAT_DIRECTION_WIDE_SOUTH;
            } else {
                if (!(blockedMask & COMBAT_DIRECTION_BIT_NORTHEAST))
                    return COMBAT_DIRECTION_NORTHEAST;
                else if (!(blockedMask & COMBAT_DIRECTION_BIT_NORTHWEST))
                    return COMBAT_DIRECTION_NORTHWEST;
                else if (!(blockedMask & COMBAT_DIRECTION_BIT_EAST))
                    return COMBAT_DIRECTION_EAST;
                else if (!(blockedMask & COMBAT_DIRECTION_BIT_WEST))
                    return COMBAT_DIRECTION_WEST;
                else if (!(blockedMask & COMBAT_DIRECTION_BIT_SOUTHEAST))
                    return COMBAT_DIRECTION_SOUTHEAST;
                else if (!(blockedMask & COMBAT_DIRECTION_BIT_SOUTHWEST))
                    return COMBAT_DIRECTION_SOUTHWEST;
                else if (!(blockedMask & COMBAT_DIRECTION_BIT_WIDE_NORTH))
                    return COMBAT_DIRECTION_WIDE_NORTH;
                else if (!(blockedMask & COMBAT_DIRECTION_BIT_WIDE_SOUTH))
                    return COMBAT_DIRECTION_WIDE_SOUTH;
            }
        } else {
            if (curSourceRow & 1) {
                if (!(blockedMask & COMBAT_DIRECTION_BIT_SOUTHWEST))
                    return COMBAT_DIRECTION_SOUTHWEST;
                else if (!(blockedMask & COMBAT_DIRECTION_BIT_SOUTHEAST))
                    return COMBAT_DIRECTION_SOUTHEAST;
                else if (!(blockedMask & COMBAT_DIRECTION_BIT_WEST))
                    return COMBAT_DIRECTION_WEST;
                else if (!(blockedMask & COMBAT_DIRECTION_BIT_EAST))
                    return COMBAT_DIRECTION_EAST;
                else if (!(blockedMask & COMBAT_DIRECTION_BIT_NORTHWEST))
                    return COMBAT_DIRECTION_NORTHWEST;
                else if (!(blockedMask & COMBAT_DIRECTION_BIT_NORTHEAST))
                    return COMBAT_DIRECTION_NORTHEAST;
                else if (!(blockedMask & COMBAT_DIRECTION_BIT_WIDE_SOUTH))
                    return COMBAT_DIRECTION_WIDE_SOUTH;
                else if (!(blockedMask & COMBAT_DIRECTION_BIT_WIDE_NORTH))
                    return COMBAT_DIRECTION_WIDE_NORTH;
            } else {
                if (!(blockedMask & COMBAT_DIRECTION_BIT_SOUTHEAST))
                    return COMBAT_DIRECTION_SOUTHEAST;
                else if (!(blockedMask & COMBAT_DIRECTION_BIT_SOUTHWEST))
                    return COMBAT_DIRECTION_SOUTHWEST;
                else if (!(blockedMask & COMBAT_DIRECTION_BIT_EAST))
                    return COMBAT_DIRECTION_EAST;
                else if (!(blockedMask & COMBAT_DIRECTION_BIT_WEST))
                    return COMBAT_DIRECTION_WEST;
                else if (!(blockedMask & COMBAT_DIRECTION_BIT_NORTHEAST))
                    return COMBAT_DIRECTION_NORTHEAST;
                else if (!(blockedMask & COMBAT_DIRECTION_BIT_NORTHWEST))
                    return COMBAT_DIRECTION_NORTHWEST;
                else if (!(blockedMask & COMBAT_DIRECTION_BIT_WIDE_SOUTH))
                    return COMBAT_DIRECTION_WIDE_SOUTH;
                else if (!(blockedMask & COMBAT_DIRECTION_BIT_WIDE_NORTH))
                    return COMBAT_DIRECTION_WIDE_NORTH;
            }
        }
    }
    if (goingLeft == true) {
        if (goingUp == true) {
            if (!(blockedMask & COMBAT_DIRECTION_BIT_NORTHWEST))
                return COMBAT_DIRECTION_NORTHWEST;
            else if (!(blockedMask & COMBAT_DIRECTION_BIT_WEST))
                return COMBAT_DIRECTION_WEST;
            else if (!(blockedMask & COMBAT_DIRECTION_BIT_NORTHEAST))
                return COMBAT_DIRECTION_NORTHEAST;
            else if (!(blockedMask & COMBAT_DIRECTION_BIT_SOUTHWEST))
                return COMBAT_DIRECTION_SOUTHWEST;
            else if (!(blockedMask & COMBAT_DIRECTION_BIT_EAST))
                return COMBAT_DIRECTION_EAST;
            else if (!(blockedMask & COMBAT_DIRECTION_BIT_SOUTHEAST))
                return COMBAT_DIRECTION_SOUTHEAST;
            else if (!(blockedMask & COMBAT_DIRECTION_BIT_WIDE_NORTH))
                return COMBAT_DIRECTION_WIDE_NORTH;
            else if (!(blockedMask & COMBAT_DIRECTION_BIT_WIDE_SOUTH))
                return COMBAT_DIRECTION_WIDE_SOUTH;
        } else if (isMovingDown == true) {
            if (!(blockedMask & COMBAT_DIRECTION_BIT_SOUTHWEST))
                return COMBAT_DIRECTION_SOUTHWEST;
            else if (!(blockedMask & COMBAT_DIRECTION_BIT_WEST))
                return COMBAT_DIRECTION_WEST;
            else if (!(blockedMask & COMBAT_DIRECTION_BIT_SOUTHEAST))
                return COMBAT_DIRECTION_SOUTHEAST;
            else if (!(blockedMask & COMBAT_DIRECTION_BIT_NORTHWEST))
                return COMBAT_DIRECTION_NORTHWEST;
            else if (!(blockedMask & COMBAT_DIRECTION_BIT_EAST))
                return COMBAT_DIRECTION_EAST;
            else if (!(blockedMask & COMBAT_DIRECTION_BIT_NORTHEAST))
                return COMBAT_DIRECTION_NORTHEAST;
            else if (!(blockedMask & COMBAT_DIRECTION_BIT_WIDE_SOUTH))
                return COMBAT_DIRECTION_WIDE_SOUTH;
            else if (!(blockedMask & COMBAT_DIRECTION_BIT_WIDE_NORTH))
                return COMBAT_DIRECTION_WIDE_NORTH;
        } else {
            if (!(blockedMask & COMBAT_DIRECTION_BIT_WEST))
                return COMBAT_DIRECTION_WEST;
            else if (!(blockedMask & COMBAT_DIRECTION_BIT_NORTHWEST))
                return COMBAT_DIRECTION_NORTHWEST;
            else if (!(blockedMask & COMBAT_DIRECTION_BIT_SOUTHWEST))
                return COMBAT_DIRECTION_SOUTHWEST;
            else if (!(blockedMask & COMBAT_DIRECTION_BIT_NORTHEAST))
                return COMBAT_DIRECTION_NORTHEAST;
            else if (!(blockedMask & COMBAT_DIRECTION_BIT_SOUTHEAST))
                return COMBAT_DIRECTION_SOUTHEAST;
            else if (!(blockedMask & COMBAT_DIRECTION_BIT_EAST))
                return COMBAT_DIRECTION_EAST;
            else if (!(blockedMask & COMBAT_DIRECTION_BIT_WIDE_SOUTH))
                return COMBAT_DIRECTION_WIDE_SOUTH;
            else if (!(blockedMask & COMBAT_DIRECTION_BIT_WIDE_NORTH))
                return COMBAT_DIRECTION_WIDE_NORTH;
        }
    } else if (isMovingRight == true) {
        if (goingUp == true) {
            if (!(blockedMask & COMBAT_DIRECTION_BIT_NORTHEAST))
                return COMBAT_DIRECTION_NORTHEAST;
            else if (!(blockedMask & COMBAT_DIRECTION_BIT_EAST))
                return COMBAT_DIRECTION_EAST;
            else if (!(blockedMask & COMBAT_DIRECTION_BIT_NORTHWEST))
                return COMBAT_DIRECTION_NORTHWEST;
            else if (!(blockedMask & COMBAT_DIRECTION_BIT_SOUTHEAST))
                return COMBAT_DIRECTION_SOUTHEAST;
            else if (!(blockedMask & COMBAT_DIRECTION_BIT_WEST))
                return COMBAT_DIRECTION_WEST;
            else if (!(blockedMask & COMBAT_DIRECTION_BIT_SOUTHWEST))
                return COMBAT_DIRECTION_SOUTHWEST;
            else if (!(blockedMask & COMBAT_DIRECTION_BIT_WIDE_NORTH))
                return COMBAT_DIRECTION_WIDE_NORTH;
            else if (!(blockedMask & COMBAT_DIRECTION_BIT_WIDE_SOUTH))
                return COMBAT_DIRECTION_WIDE_SOUTH;
        } else if (isMovingDown == true) {
            if (!(blockedMask & COMBAT_DIRECTION_BIT_SOUTHEAST))
                return COMBAT_DIRECTION_SOUTHEAST;
            else if (!(blockedMask & COMBAT_DIRECTION_BIT_EAST))
                return COMBAT_DIRECTION_EAST;
            else if (!(blockedMask & COMBAT_DIRECTION_BIT_SOUTHWEST))
                return COMBAT_DIRECTION_SOUTHWEST;
            else if (!(blockedMask & COMBAT_DIRECTION_BIT_NORTHEAST))
                return COMBAT_DIRECTION_NORTHEAST;
            else if (!(blockedMask & COMBAT_DIRECTION_BIT_NORTHWEST))
                return COMBAT_DIRECTION_NORTHWEST;
            else if (!(blockedMask & COMBAT_DIRECTION_BIT_WEST))
                return COMBAT_DIRECTION_WEST;
            else if (!(blockedMask & COMBAT_DIRECTION_BIT_WIDE_SOUTH))
                return COMBAT_DIRECTION_WIDE_SOUTH;
            else if (!(blockedMask & COMBAT_DIRECTION_BIT_WIDE_NORTH))
                return COMBAT_DIRECTION_WIDE_NORTH;
        } else {
            if (!(blockedMask & COMBAT_DIRECTION_BIT_EAST))
                return COMBAT_DIRECTION_EAST;
            else if (!(blockedMask & COMBAT_DIRECTION_BIT_NORTHEAST))
                return COMBAT_DIRECTION_NORTHEAST;
            else if (!(blockedMask & COMBAT_DIRECTION_BIT_SOUTHEAST))
                return COMBAT_DIRECTION_SOUTHEAST;
            else if (!(blockedMask & COMBAT_DIRECTION_BIT_NORTHWEST))
                return COMBAT_DIRECTION_NORTHWEST;
            else if (!(blockedMask & COMBAT_DIRECTION_BIT_SOUTHWEST))
                return COMBAT_DIRECTION_SOUTHWEST;
            else if (!(blockedMask & COMBAT_DIRECTION_BIT_WEST))
                return COMBAT_DIRECTION_WEST;
            else if (!(blockedMask & COMBAT_DIRECTION_BIT_WIDE_SOUTH))
                return COMBAT_DIRECTION_WIDE_SOUTH;
            else if (!(blockedMask & COMBAT_DIRECTION_BIT_WIDE_NORTH))
                return COMBAT_DIRECTION_WIDE_NORTH;
        }
    }
    return COMBAT_DIRECTION_INVALID;
}

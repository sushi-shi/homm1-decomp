#include <match.h>

#include <SOURCE/PATH.h>

#include <BASE/Misc.h>
#include <SOURCE/advManager.h>
#include <SOURCE/army.h>
#include <SOURCE/combatManager.h>
#include <SOURCE/KB.h>
#include <SOURCE/kbwin.h>
#include <SOURCE/searchArray.h>

// #line restores the original source file and line numbers of the asserts.

// The speed slot is unused; a two-hex creature retries from its rear hex.
VA(0x00446450, 0x11e)
i16 army::FindPath(i16 sourceHex, i16 targetHex, i8, i8 ignoreSpeed, i8 pathMode) {
    i16 retVal;
    i32 savedSpeed;

    if (!ValidHex(sourceHex) || !ValidHex(targetHex))
        return 0;
    savedSpeed = m_stats.speed;
    if (ignoreSpeed)
        m_stats.speed = IGNORE_SPEED;
    retVal = gpSearchArray->FindCombatPath(sourceHex, targetHex, this, pathMode);
    if (!retVal && (m_stats.attributes & MONSTER_FLAGS_WIDE) && !pathMode) {
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
            retVal = gpSearchArray->FindCombatPath(sourceHex, targetHex, this, pathMode);
    }
    m_stats.speed = savedSpeed;
    return retVal;
}

VA(0x0044656e, 0x86)
i16 army::ValidPath(i16 targetHex, i8 pathMode) {
    i32 pathResult;
    i32 unusedExtra;

    if (!ValidHex(targetHex))
        return 0;
    if (m_stats.attributes & MONSTER_FLAGS_FLYING)
        return ValidFlight(targetHex, pathMode);
    pathResult = FindPath(m_hex, targetHex, m_stats.speed, 0, pathMode);
    if (pathResult) {
        m_moveTargetHex = targetHex;
        return 1;
    }
    return 0;
}

VA(0x004465f4, 0x72)
i16 army::GetMoveMask(i16 sourceHex) {
    i16 blockedMaskVal;
    i16 mask;
    i16 direction;

    blockedMaskVal = 0;
    mask = 1;
    for (direction = 0; direction <= COMBAT_DIRECTION_ADJACENT_LAST; direction++) {
        if (!ValidMove(sourceHex, direction))
            blockedMaskVal |= mask;
        mask <<= 1;
    }
    return blockedMaskVal | (1 << COMBAT_DIRECTION_WIDE_WEST) | (1 << COMBAT_DIRECTION_WIDE_EAST);
}

VA(0x00446666, 0xac)
i16 army::GetAttackMask(i16 sourceHex, i8 targetMode, i8 targetHex) {
    i16 theDirection;
    i16 hex;
    i16 curDirBit;
    i16 curMask;
    i16 nDirectionCount;

    curMask = (m_stats.attributes & MONSTER_FLAGS_WIDE) ? 0 : SPECIAL_DIRECTION_MASK;
    curDirBit = 1;
    nDirectionCount = (m_stats.attributes & MONSTER_FLAGS_WIDE) ? COMBAT_DIRECTION_COUNT
                                                                : COMBAT_DIRECTION_ADJACENT_COUNT;
    for (theDirection = 0; theDirection < nDirectionCount; theDirection++) {
        if (!ValidAttack(sourceHex, theDirection, targetMode, targetHex, &hex))
            curMask |= curDirBit;
        curDirBit <<= 1;
    }
    return curMask;
}

VA(0x00446712, 0x23)
i16 army::ValidMove(i16 direction) {
    return ValidMove(m_hex, direction);
}

VA(0x00446735, 0x187)
i16 army::ValidMove(i16 sourceHex, i16 direction) {
    i16 destHexNext;
    i8 rearSquare;
    i8 frontValid;
    i8 rearValidResult;

    if (!ValidHex(sourceHex))
        return 0;
    destHexNext = GetAdjacentCellIndex(sourceHex, direction);
    if (!ValidHex(destHexNext))
        return 0;
    frontValid = 0;
    if (gpCombatManager->m_hexCells[destHexNext].m_occupantSide == COMBAT_SIDE_NONE
        && gpCombatManager->m_hexCells[destHexNext].m_obstacleIndex == COMBAT_OBSTACLE_NONE)
        frontValid = 1;
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
        rearValidResult = 0;
        if (ValidHex(rearSquare)
            && gpCombatManager->m_hexCells[rearSquare].m_occupantSide == COMBAT_SIDE_NONE
            && gpCombatManager->m_hexCells[rearSquare].m_obstacleIndex == COMBAT_OBSTACLE_NONE)
            rearValidResult = 1;
        if (direction == COMBAT_DIRECTION_EAST || direction == COMBAT_DIRECTION_WEST)
            return rearValidResult;
        else {
            if (frontValid == 1 && rearValidResult == 1)
                return 1;
            else
                return 0;
        }
    } else
        return frontValid;
}

VA(0x004468bc, 0x216)
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
        if (direction == COMBAT_DIRECTION_WIDE_WEST)
            *attackHex = GetAdjacentCellIndex(
                sourceHex,
                m_facing == ARMY_FACING_LEFT ? static_cast<i8>(COMBAT_DIRECTION_NORTHWEST)
                                             : static_cast<i8>(COMBAT_DIRECTION_NORTHEAST)
            );
        else if (direction == COMBAT_DIRECTION_WIDE_EAST)
            *attackHex = GetAdjacentCellIndex(
                sourceHex,
                m_facing == ARMY_FACING_LEFT ? static_cast<i8>(COMBAT_DIRECTION_SOUTHWEST)
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
    occupantSide = gpCombatManager->m_hexCells[*attackHex].m_occupantSide;
    switch (targetMode) {
        case ARMY_ATTACK_TARGET_ASSIGNED:
            if (occupantSide == m_targetSide
                && gpCombatManager->m_hexCells[*attackHex].m_occupantIndex == m_targetIndex)
                return 1;
            break;
        case ARMY_ATTACK_TARGET_ENEMY:
            if (occupantSide == 1 - gpCombatManager->m_currentSide)
                return 1;
            break;
        case ARMY_ATTACK_TARGET_OCCUPIED:
            if (occupantSide != COMBAT_SIDE_NONE)
                return 1;
            break;
    }
    return 0;
}

VA(0x00446ad2, 0xe4)
i16 army::GetAdjacentCellIndex(i16 hex, i16 direction)
#line 311 "E:\\Users\\igorl\\VSS\\HMM\\HMM1\\Source\\Game\\PATH.CPP"
{
    if (hex == ARMY_HEX_INVALID)
        return ARMY_HEX_INVALID;
    if (direction == COMBAT_DIRECTION_WIDE_WEST)
        direction = m_facing == ARMY_FACING_RIGHT ? static_cast<i8>(COMBAT_DIRECTION_NORTHWEST)
                                                  : static_cast<i8>(COMBAT_DIRECTION_NORTHEAST);
    else if (direction == COMBAT_DIRECTION_WIDE_EAST)
        direction = m_facing == ARMY_FACING_RIGHT ? static_cast<i8>(COMBAT_DIRECTION_SOUTHWEST)
                                                  : static_cast<i8>(COMBAT_DIRECTION_SOUTHEAST);
    // clang-format off
#line 322
    H1_ASSERT(direction >= 0 && direction < COMBAT_DIRECTION_ADJACENT_COUNT);
    // clang-format on
#line 323
    H1_ASSERT(hex >= 0 && hex < COMBAT_HEX_COUNT);
    return gCombatAdjacency[hex][direction];
}

VA(0x00446bb6, 0xbe)
i16 GetAdjacentCellIndexNoArmy(i16 hex, i16 direction)
#line 328 "E:\\Users\\igorl\\VSS\\HMM\\HMM1\\Source\\Game\\PATH.CPP"
{
    if (hex == ARMY_HEX_INVALID)
        return ARMY_HEX_INVALID;
    if (direction == COMBAT_DIRECTION_WIDE_WEST)
        direction = COMBAT_DIRECTION_NORTHWEST;
    else if (direction == COMBAT_DIRECTION_WIDE_EAST)
        direction = COMBAT_DIRECTION_SOUTHWEST;
    // clang-format off
#line 339
    H1_ASSERT(direction >= 0 && direction < COMBAT_DIRECTION_ADJACENT_COUNT);
    // clang-format on
#line 340
    H1_ASSERT(hex >= 0 && hex < COMBAT_HEX_COUNT);
    return gCombatAdjacency[hex][direction];
}

// @dead-code
// Zero-ref: no incoming call, jump or relocated reference in retail.
VA(0x00446c74, 0x45a)
i16 army::ValidRange(i16 targetHex) {
    i16 adjacentHex;
    i16 res;

    if (!ValidHex(targetHex))
        return 0;
    m_moveTargetHex = m_hex;
    if (!(m_stats.attributes & MONSTER_FLAGS_WIDE)) {
        m_attackDirection = GetBestDirection(m_hex, targetHex, SPECIAL_DIRECTION_MASK);
        adjacentHex = GetAdjacentCellIndex(m_hex, m_attackDirection);
        if (adjacentHex == targetHex)
            return 1;
        adjacentHex = GetAdjacentCellIndex(adjacentHex, m_attackDirection);
        if (adjacentHex == targetHex)
            return 1;
    } else {
        switch (m_facing) {
            case ARMY_FACING_RIGHT:
                res = GetBestDirection(m_hex, targetHex, SPECIAL_DIRECTION_MASK);
                if (res > COMBAT_DIRECTION_EASTERN_LAST) {
                    m_attackDirection = res;
                    adjacentHex = GetAdjacentCellIndex(m_hex, res);
                    if (adjacentHex == targetHex)
                        return 1;
                    adjacentHex = GetAdjacentCellIndex(adjacentHex, res);
                    if (adjacentHex == targetHex)
                        return 1;
                }
                res = GetBestDirection(m_hex + WIDE_HEX_OFFSET, targetHex, SPECIAL_DIRECTION_MASK);
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
                    m_attackDirection = COMBAT_DIRECTION_WIDE_WEST;
                else if (res == COMBAT_DIRECTION_SOUTHWEST)
                    m_attackDirection = COMBAT_DIRECTION_WIDE_EAST;
                adjacentHex = GetAdjacentCellIndex(m_hex + WIDE_HEX_OFFSET, res);
                if (adjacentHex == targetHex)
                    return 1;
                adjacentHex = GetAdjacentCellIndex(adjacentHex, res);
                if (adjacentHex == targetHex)
                    return 1;
                break;
            case ARMY_FACING_LEFT:
                res = GetBestDirection(m_hex, targetHex, SPECIAL_DIRECTION_MASK);
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
                res = GetBestDirection(m_hex - WIDE_HEX_OFFSET, targetHex, SPECIAL_DIRECTION_MASK);
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
                    m_attackDirection = COMBAT_DIRECTION_WIDE_WEST;
                else if (res == COMBAT_DIRECTION_SOUTHEAST)
                    m_attackDirection = COMBAT_DIRECTION_WIDE_EAST;
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

VA(0x004470ce, 0x35)
H1_ENUM_RETURN(CombatHexDirection, i16)
OppositeDirection(H1_ENUM_PARAM(CombatHexDirection, i16) direction) {
    if (static_cast<i32>(direction) < COMBAT_DIRECTION_ADJACENT_COUNT)
        return H1_ENUM_CAST(
            CombatHexDirection,
            i16,
            (static_cast<i32>(direction) + COMBAT_DIRECTION_OPPOSITE_OFFSET)
                % COMBAT_DIRECTION_ADJACENT_COUNT
        );
    else {
        if (direction == COMBAT_DIRECTION_WIDE_WEST)
            return COMBAT_DIRECTION_WIDE_EAST;
        else
            return COMBAT_DIRECTION_WIDE_WEST;
    }
}

// Nine-hex rows and byte row/column flags.
VA(0x00447103, 0x7b7)
i16 army::GetBestDirection(i16 sourceHex, i16 targetHex, i16 blockedMask) {
    i8 isMovingRight;
    i8 curSourceRow;
    i8 targetRow;
    i8 goingUp;
    i8 goingLeft;
    i8 srcCol;
    i8 targetColumn;
    i8 isMovingDown;

    if (!ValidHex(sourceHex) || !ValidHex(targetHex))
        return COMBAT_DIRECTION_INVALID;
    srcCol = sourceHex % COMBAT_GRID_COLUMNS;
    curSourceRow = sourceHex / COMBAT_GRID_COLUMNS;
    targetColumn = targetHex % COMBAT_GRID_COLUMNS;
    targetRow = targetHex / COMBAT_GRID_COLUMNS;
    goingUp = 0;
    isMovingDown = 0;
    goingLeft = 0;
    isMovingRight = 0;
    if (targetColumn > srcCol)
        isMovingRight = 1;
    else if (targetColumn != srcCol)
        goingLeft = 1;
    if (targetRow > curSourceRow)
        isMovingDown = 1;
    else if (targetRow != curSourceRow)
        goingUp = 1;
    if (isMovingRight == goingLeft) {
        if (goingUp == 1) {
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
                else if (!(blockedMask & COMBAT_DIRECTION_BIT_WIDE_WEST))
                    return COMBAT_DIRECTION_WIDE_WEST;
                else if (!(blockedMask & COMBAT_DIRECTION_BIT_WIDE_EAST))
                    return COMBAT_DIRECTION_WIDE_EAST;
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
                else if (!(blockedMask & COMBAT_DIRECTION_BIT_WIDE_WEST))
                    return COMBAT_DIRECTION_WIDE_WEST;
                else if (!(blockedMask & COMBAT_DIRECTION_BIT_WIDE_EAST))
                    return COMBAT_DIRECTION_WIDE_EAST;
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
                else if (!(blockedMask & COMBAT_DIRECTION_BIT_WIDE_EAST))
                    return COMBAT_DIRECTION_WIDE_EAST;
                else if (!(blockedMask & COMBAT_DIRECTION_BIT_WIDE_WEST))
                    return COMBAT_DIRECTION_WIDE_WEST;
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
                else if (!(blockedMask & COMBAT_DIRECTION_BIT_WIDE_EAST))
                    return COMBAT_DIRECTION_WIDE_EAST;
                else if (!(blockedMask & COMBAT_DIRECTION_BIT_WIDE_WEST))
                    return COMBAT_DIRECTION_WIDE_WEST;
            }
        }
    }
    if (goingLeft == 1) {
        if (goingUp == 1) {
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
            else if (!(blockedMask & COMBAT_DIRECTION_BIT_WIDE_WEST))
                return COMBAT_DIRECTION_WIDE_WEST;
            else if (!(blockedMask & COMBAT_DIRECTION_BIT_WIDE_EAST))
                return COMBAT_DIRECTION_WIDE_EAST;
        } else if (isMovingDown == 1) {
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
            else if (!(blockedMask & COMBAT_DIRECTION_BIT_WIDE_EAST))
                return COMBAT_DIRECTION_WIDE_EAST;
            else if (!(blockedMask & COMBAT_DIRECTION_BIT_WIDE_WEST))
                return COMBAT_DIRECTION_WIDE_WEST;
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
            else if (!(blockedMask & COMBAT_DIRECTION_BIT_WIDE_EAST))
                return COMBAT_DIRECTION_WIDE_EAST;
            else if (!(blockedMask & COMBAT_DIRECTION_BIT_WIDE_WEST))
                return COMBAT_DIRECTION_WIDE_WEST;
        }
    } else if (isMovingRight == 1) {
        if (goingUp == 1) {
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
            else if (!(blockedMask & COMBAT_DIRECTION_BIT_WIDE_WEST))
                return COMBAT_DIRECTION_WIDE_WEST;
            else if (!(blockedMask & COMBAT_DIRECTION_BIT_WIDE_EAST))
                return COMBAT_DIRECTION_WIDE_EAST;
        } else if (isMovingDown == 1) {
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
            else if (!(blockedMask & COMBAT_DIRECTION_BIT_WIDE_EAST))
                return COMBAT_DIRECTION_WIDE_EAST;
            else if (!(blockedMask & COMBAT_DIRECTION_BIT_WIDE_WEST))
                return COMBAT_DIRECTION_WIDE_WEST;
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
            else if (!(blockedMask & COMBAT_DIRECTION_BIT_WIDE_EAST))
                return COMBAT_DIRECTION_WIDE_EAST;
            else if (!(blockedMask & COMBAT_DIRECTION_BIT_WIDE_WEST))
                return COMBAT_DIRECTION_WIDE_WEST;
        }
    }
    return COMBAT_DIRECTION_INVALID;
}

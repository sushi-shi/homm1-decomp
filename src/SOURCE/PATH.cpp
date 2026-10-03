// Located from HoMM2 Buka 2.1; PoL 2.0 supplies the VC4 declaration.

#include <match.h>

#include <SOURCE/PATH.h>

#include <BASE/Misc.h>
#include <SOURCE/advManager.h>
#include <SOURCE/army.h>
#include <SOURCE/combatManager.h>
#include <SOURCE/KB.h>
#include <SOURCE/kbwin.h>
#include <SOURCE/searchArray.h>

// Buka 2.1 PATH.cpp CombatPathConstant: the blocked-mask bits for the two
// wide-creature directions, the speed FindPath grants when speed is ignored,
// and the second hex of a wide creature.
H1_ENUM_CONST_BEGIN(CombatPathConstant)
    SPECIAL_DIRECTION_MASK = 0xc0,
    IGNORE_SPEED = 99,
    WIDE_HEX_OFFSET = 1
H1_ENUM_CONST_END(CombatPathConstant)

// Retail compiled this file incrementally (/Gi): each ProcessAssert line is the
// function's compiler line static plus an offset; #line restores the original
// file and lines (docs/patterns/vc4-gi-line-var.md).

// Buka PATH.cpp FindPath; HoMM1 takes the speed slot unused and retries a
// two-hex creature from its rear hex.
VA(0x004180f0, 0x152)
i16 army::FindPath(i16 sourceHex, i16 targetHex, i8, i8 ignoreSpeed, i8 pathMode) {
    i16 pathResult;
    i32 savedSpeed;

    if (!ValidHex(sourceHex) || !ValidHex(targetHex))
        return 0;
    savedSpeed = m_stats.speed;
    if (ignoreSpeed)
        m_stats.speed = IGNORE_SPEED;
    pathResult = gpSearchArray->FindCombatPath(sourceHex, targetHex, this, pathMode);
    if (!pathResult && (m_stats.attributes & MONSTER_FLAGS_WIDE) && !pathMode) {
        switch (m_facing) {
            case ARMY_FACING_LEFT:
                targetHex = GetAdjacentCellIndex(targetHex, COMBAT_DIRECTION_EAST);
                break;
            case ARMY_FACING_RIGHT:
                targetHex = GetAdjacentCellIndex(targetHex, COMBAT_DIRECTION_WEST);
                break;
        }
        if (!ValidHex(targetHex))
            pathResult = 0;
        else
            pathResult = gpSearchArray->FindCombatPath(sourceHex, targetHex, this, pathMode);
    }
    m_stats.speed = savedSpeed;
    return pathResult;
}

// Buka PATH.cpp ValidPath.
VA(0x00418242, 0x9e)
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

// Buka PATH.cpp GetMoveMask.
VA(0x004182e0, 0x7b)
i16 army::GetMoveMask(i16 sourceHex) {
    i16 blockedMask;
    i16 mask;
    i16 direction;

    blockedMask = 0;
    mask = 1;
    for (direction = 0; direction <= COMBAT_DIRECTION_ADJACENT_LAST; direction++) {
        if (!ValidMove(sourceHex, direction))
            blockedMask |= mask;
        mask <<= 1;
    }
    return blockedMask | SPECIAL_DIRECTION_MASK;
}

// Buka PATH.cpp GetAttackMask.
VA(0x0041835b, 0xbf)
i16 army::GetAttackMask(i16 sourceHex, i8 targetMode, i8 targetHex) {
    i16 direction;
    i16 hex;
    i16 dirBit;
    i16 blockedMask;
    i16 nDirectionCount;

    if (m_stats.attributes & MONSTER_FLAGS_WIDE)
        blockedMask = 0;
    else
        blockedMask = SPECIAL_DIRECTION_MASK;
    dirBit = 1;
    if (m_stats.attributes & MONSTER_FLAGS_WIDE)
        nDirectionCount = COMBAT_DIRECTION_COUNT;
    else
        nDirectionCount = COMBAT_DIRECTION_ADJACENT_COUNT;
    for (direction = 0; direction < nDirectionCount; direction++) {
        if (!ValidAttack(sourceHex, direction, targetMode, targetHex, &hex))
            blockedMask |= dirBit;
        dirBit <<= 1;
    }
    return blockedMask;
}

// Buka PATH.cpp ValidMove(direction).
VA(0x0041841a, 0x2d)
i16 army::ValidMove(i16 direction) {
    return ValidMove(m_hex, direction);
}

// Buka PATH.cpp ValidMove; HoMM1 has no castle gate exception.
VA(0x00418447, 0x1fc)
i16 army::ValidMove(i16 sourceHex, i16 direction) {
    i8 frontValid;
    i16 dest;
    i8 backHex;
    i8 rearValid;

    if (!ValidHex(sourceHex))
        return 0;
    dest = GetAdjacentCellIndex(sourceHex, direction);
    if (!ValidHex(dest))
        return 0;
    frontValid = 0;
    if (gpCombatManager->m_hexCells[dest].m_occupantSide == COMBAT_SIDE_NONE
        && gpCombatManager->m_hexCells[dest].m_obstacleIndex == COMBAT_OBSTACLE_NONE)
        frontValid = 1;
    if (m_stats.attributes & MONSTER_FLAGS_WIDE) {
        backHex = ARMY_HEX_INVALID;
        switch (m_facing) {
            case ARMY_FACING_LEFT:
                if (direction == COMBAT_DIRECTION_EAST)
                    return frontValid;
                else
                    backHex = GetAdjacentCellIndex(dest, COMBAT_DIRECTION_WEST);
                break;
            case ARMY_FACING_RIGHT:
                if (direction == COMBAT_DIRECTION_WEST)
                    return frontValid;
                else
                    backHex = GetAdjacentCellIndex(dest, COMBAT_DIRECTION_EAST);
                break;
        }
        rearValid = 0;
        if (ValidHex(backHex)
            && gpCombatManager->m_hexCells[backHex].m_occupantSide == COMBAT_SIDE_NONE
            && gpCombatManager->m_hexCells[backHex].m_obstacleIndex == COMBAT_OBSTACLE_NONE)
            rearValid = 1;
        if (direction == COMBAT_DIRECTION_EAST || direction == COMBAT_DIRECTION_WEST)
            return rearValid;
        else {
            if (frontValid == 1 && rearValid == 1)
                return 1;
            else
                return 0;
        }
    } else
        return frontValid;
}

// Buka PATH.cpp ValidAttack.
VA(0x00418643, 0x295)
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
                static_cast<i8>(
                    m_facing == ARMY_FACING_LEFT ? COMBAT_DIRECTION_NORTHWEST
                                                 : COMBAT_DIRECTION_NORTHEAST
                )
            );
        else if (direction == COMBAT_DIRECTION_WIDE_EAST)
            *attackHex = GetAdjacentCellIndex(
                sourceHex,
                static_cast<i8>(
                    m_facing == ARMY_FACING_LEFT ? COMBAT_DIRECTION_SOUTHWEST
                                                 : COMBAT_DIRECTION_SOUTHEAST
                )
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
            if (m_targetSide == occupantSide
                && gpCombatManager->m_hexCells[*attackHex].m_occupantIndex == m_targetIndex)
                return 1;
            break;
        case ARMY_ATTACK_TARGET_ENEMY:
            if (1 - gpCombatManager->m_currentSide == occupantSide)
                return 1;
            break;
        case ARMY_ATTACK_TARGET_OCCUPIED:
            if (occupantSide != COMBAT_SIDE_NONE)
                return 1;
            break;
    }
    return 0;
}

// Buka PATH.cpp GetAdjacentCellIndex with HoMM1's asserts.
VA(0x004188d8, 0x11e)
i16 army::GetAdjacentCellIndex(i16 hex, i16 direction)
#line 311 "D:\\Heroes\\Source\\PATH.CPP"
{
    if (hex == ARMY_HEX_INVALID)
        return ARMY_HEX_INVALID;
    if (direction == COMBAT_DIRECTION_WIDE_WEST)
        direction = static_cast<i8>(
            m_facing == ARMY_FACING_RIGHT ? COMBAT_DIRECTION_NORTHWEST : COMBAT_DIRECTION_NORTHEAST
        );
    else if (direction == COMBAT_DIRECTION_WIDE_EAST)
        direction = static_cast<i8>(
            m_facing == ARMY_FACING_RIGHT ? COMBAT_DIRECTION_SOUTHWEST : COMBAT_DIRECTION_SOUTHEAST
        );
#line 322
    ProcessAssert(
        direction >= 0 && direction < COMBAT_DIRECTION_ADJACENT_COUNT,
        __FILE__,
        __LINE__
    );
#line 323
    ProcessAssert(hex >= 0 && hex < COMBAT_HEX_COUNT, __FILE__, __LINE__);
    return gCombatAdjacency[hex][direction];
}

// Buka PATH.cpp GetAdjacentCellIndexNoArmy with HoMM1's asserts.
VA(0x004189f6, 0xf8)
i16 GetAdjacentCellIndexNoArmy(i16 hex, i16 direction)
#line 328 "D:\\Heroes\\Source\\PATH.CPP"
{
    if (hex == ARMY_HEX_INVALID)
        return ARMY_HEX_INVALID;
    if (direction == COMBAT_DIRECTION_WIDE_WEST)
        direction = COMBAT_DIRECTION_NORTHWEST;
    else if (direction == COMBAT_DIRECTION_WIDE_EAST)
        direction = COMBAT_DIRECTION_SOUTHWEST;
#line 339
    ProcessAssert(
        direction >= 0 && direction < COMBAT_DIRECTION_ADJACENT_COUNT,
        __FILE__,
        __LINE__
    );
#line 340
    ProcessAssert(hex >= 0 && hex < COMBAT_HEX_COUNT, __FILE__, __LINE__);
    return gCombatAdjacency[hex][direction];
}

// Buka PATH.cpp ValidRange.
// @dead-code
// Zero-ref: no incoming call, jump or relocated reference in retail.
VA(0x00418aee, 0x4c2)
i16 army::ValidRange(i16 targetHex) {
    i16 adjacentHex;
    i16 directionResult;

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
                directionResult = GetBestDirection(m_hex, targetHex, SPECIAL_DIRECTION_MASK);
                if (directionResult > COMBAT_DIRECTION_EASTERN_LAST) {
                    m_attackDirection = directionResult;
                    adjacentHex = GetAdjacentCellIndex(m_hex, directionResult);
                    if (adjacentHex == targetHex)
                        return 1;
                    adjacentHex = GetAdjacentCellIndex(adjacentHex, directionResult);
                    if (adjacentHex == targetHex)
                        return 1;
                }
                directionResult =
                    GetBestDirection(m_hex + WIDE_HEX_OFFSET, targetHex, SPECIAL_DIRECTION_MASK);
                if (directionResult < COMBAT_DIRECTION_WESTERN_FIRST) {
                    m_attackDirection = directionResult;
                    adjacentHex = GetAdjacentCellIndex(m_hex + WIDE_HEX_OFFSET, directionResult);
                    if (adjacentHex == targetHex)
                        return 1;
                    adjacentHex = GetAdjacentCellIndex(adjacentHex, directionResult);
                    if (adjacentHex == targetHex)
                        return 1;
                }
                if (directionResult == COMBAT_DIRECTION_WEST)
                    return 0;
                if (directionResult == COMBAT_DIRECTION_NORTHWEST)
                    m_attackDirection = COMBAT_DIRECTION_WIDE_WEST;
                else if (directionResult == COMBAT_DIRECTION_SOUTHWEST)
                    m_attackDirection = COMBAT_DIRECTION_WIDE_EAST;
                adjacentHex = GetAdjacentCellIndex(m_hex + WIDE_HEX_OFFSET, directionResult);
                if (adjacentHex == targetHex)
                    return 1;
                adjacentHex = GetAdjacentCellIndex(adjacentHex, directionResult);
                if (adjacentHex == targetHex)
                    return 1;
                break;
            case ARMY_FACING_LEFT:
                directionResult = GetBestDirection(m_hex, targetHex, SPECIAL_DIRECTION_MASK);
                if (directionResult < COMBAT_DIRECTION_WESTERN_FIRST) {
                    m_attackDirection = directionResult;
                    adjacentHex = GetAdjacentCellIndex(m_hex, directionResult);
                    if (adjacentHex == targetHex)
                        return 1;
                    adjacentHex = GetAdjacentCellIndex(adjacentHex, directionResult);
                    if (adjacentHex == targetHex)
                        return 1;
                    return 0;
                }
                directionResult =
                    GetBestDirection(m_hex - WIDE_HEX_OFFSET, targetHex, SPECIAL_DIRECTION_MASK);
                if (directionResult > COMBAT_DIRECTION_EASTERN_LAST) {
                    m_attackDirection = directionResult;
                    adjacentHex = GetAdjacentCellIndex(m_hex - WIDE_HEX_OFFSET, directionResult);
                    if (adjacentHex == targetHex)
                        return 1;
                    adjacentHex = GetAdjacentCellIndex(adjacentHex, directionResult);
                    if (adjacentHex == targetHex)
                        return 1;
                    return 0;
                }
                if (directionResult == COMBAT_DIRECTION_EAST)
                    return 0;
                if (directionResult == COMBAT_DIRECTION_NORTHEAST)
                    m_attackDirection = COMBAT_DIRECTION_WIDE_WEST;
                else if (directionResult == COMBAT_DIRECTION_SOUTHEAST)
                    m_attackDirection = COMBAT_DIRECTION_WIDE_EAST;
                adjacentHex = GetAdjacentCellIndex(m_hex - WIDE_HEX_OFFSET, directionResult);
                if (adjacentHex == targetHex)
                    return 1;
                adjacentHex = GetAdjacentCellIndex(adjacentHex, directionResult);
                if (adjacentHex == targetHex)
                    return 1;
                break;
        }
    }
    return 0;
}

// HoMM2 donor behavior; HoMM1's WORD parameter/return prove the narrower API.
// donor Buka TU SOURCE/PATH; HoMM1 owner inferred from contiguous order
// evidence: retail body uses signed WORD loads and returns through AX;
// alternate=pol20:int OppositeDirection(int)@0x000be9e7
VA(0x00418fb0, 0x58)
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

// Buka PATH.cpp GetBestDirection with HoMM1's nine-hex rows and byte
// row/column flags.
VA(0x00419008, 0x984)
i16 army::GetBestDirection(i16 sourceHex, i16 targetHex, i16 blockedMask) {
    i8 targetCol;
    i8 targetRowVal;
    i8 sourceColumnCheck;
    i8 iIsMovingDown;
    i8 movingUp;
    i8 sourceRowVal;
    i8 rightFl;
    i8 iLeftFl;

    if (!ValidHex(sourceHex) || !ValidHex(targetHex))
        return COMBAT_DIRECTION_INVALID;
    sourceColumnCheck = sourceHex % COMBAT_GRID_COLUMNS;
    sourceRowVal = sourceHex / COMBAT_GRID_COLUMNS;
    targetCol = targetHex % COMBAT_GRID_COLUMNS;
    targetRowVal = targetHex / COMBAT_GRID_COLUMNS;
    movingUp = 0;
    iIsMovingDown = 0;
    iLeftFl = 0;
    rightFl = 0;
    if (sourceColumnCheck < targetCol)
        rightFl = 1;
    else if (sourceColumnCheck != targetCol)
        iLeftFl = 1;
    if (sourceRowVal < targetRowVal)
        iIsMovingDown = 1;
    else if (sourceRowVal != targetRowVal)
        movingUp = 1;
    if (iLeftFl == rightFl) {
        if (movingUp == 1) {
            if (sourceRowVal & 1) {
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
            if (sourceRowVal & 1) {
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
    if (iLeftFl == 1) {
        if (movingUp == 1) {
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
        } else if (iIsMovingDown == 1) {
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
    } else if (rightFl == 1) {
        if (movingUp == 1) {
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
        } else if (iIsMovingDown == 1) {
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

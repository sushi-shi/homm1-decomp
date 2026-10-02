// Located from HoMM2 Buka 2.1; PoL 2.0 supplies the VC4 declaration.

#include <match.h>

#include <SOURCE/PATH.h>

#include <BASE/Misc.h>
#include <H1/All.h>
#include <H1/KB.h>

// clang-format off
// Buka 2.1 PATH.cpp CombatPathConstant: the blocked-mask bits for the two
// wide-creature directions, the speed FindPath grants when speed is ignored,
// and the second hex of a wide creature.
H1_ENUM_CONST_BEGIN(CombatPathConstant)
    SPECIAL_DIRECTION_MASK = 0xc0,
    IGNORE_SPEED = 99,
    WIDE_HEX_OFFSET = 1
H1_ENUM_CONST_END(CombatPathConstant)
// clang-format on

// Compiler line-base words for PATH.CPP's ProcessAssert sites.
DATA(0x0048f4d4) short gAdjacentCellAssertLine = 311;
DATA(0x0048f510) short gAdjacentCellNoArmyAssertLine = 328;

// Buka PATH.cpp FindPath; HoMM1 takes the speed slot unused and retries a
// two-hex creature from its rear hex.
VA(0x004180f0, 0x152)
short army::FindPath(short sourceHex, short targetHex, signed char, signed char ignoreSpeed, signed char pathMode)
{
    short pathResult;
    int savedSpeed;

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
short army::ValidPath(short targetHex, signed char pathMode)
{
    int pathResult;
    int unusedExtra;

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
short army::GetMoveMask(short sourceHex)
{
    short blockedMask;
    short mask;
    short direction;

    blockedMask = 0;
    mask = 1;
    for (direction = 0; direction <= 5; direction++) {
        if (!ValidMove(sourceHex, direction))
            blockedMask |= mask;
        mask <<= 1;
    }
    return blockedMask | SPECIAL_DIRECTION_MASK;
}

// Buka PATH.cpp GetAttackMask.
VA(0x0041835b, 0xbf)
short army::GetAttackMask(short sourceHex, signed char targetMode, signed char targetHex)
{
    short direction;
    short hex;
    short dirBit;
    short blockedMask;
    short nDirectionCount;

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
short army::ValidMove(short direction)
{
    return ValidMove(m_hex, direction);
}

// Buka PATH.cpp ValidMove; HoMM1 has no castle gate exception.
VA(0x00418447, 0x1fc)
short army::ValidMove(short sourceHex, short direction)
{
    signed char frontValid;
    short dest;
    signed char backHex;
    signed char rearValid;

    if (!ValidHex(sourceHex))
        return 0;
    dest = GetAdjacentCellIndex(sourceHex, direction);
    if (!ValidHex(dest))
        return 0;
    frontValid = 0;
    if (gpCombatManager->m_hexCells[dest].m_occupantSide == COMBAT_SIDE_NONE && gpCombatManager->m_hexCells[dest].m_obstacleIndex == COMBAT_OBSTACLE_NONE)
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
        if (ValidHex(backHex) && gpCombatManager->m_hexCells[backHex].m_occupantSide == COMBAT_SIDE_NONE
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
short army::ValidAttack(short sourceHex, short direction, short targetMode, short requiredTargetHex, short *attackHex)
{
    short adjacentHex;
    signed char occupantSide;

    if (!ValidHex(sourceHex))
        return 0;
    adjacentHex = sourceHex;
    if (m_stats.attributes & MONSTER_FLAGS_WIDE) {
        if (direction == COMBAT_DIRECTION_WIDE_WEST)
            *attackHex = GetAdjacentCellIndex(sourceHex, (signed char)(m_facing == ARMY_FACING_LEFT ? COMBAT_DIRECTION_NORTHWEST : COMBAT_DIRECTION_NORTHEAST));
        else if (direction == COMBAT_DIRECTION_WIDE_EAST)
            *attackHex = GetAdjacentCellIndex(sourceHex, (signed char)(m_facing == ARMY_FACING_LEFT ? COMBAT_DIRECTION_SOUTHWEST : COMBAT_DIRECTION_SOUTHEAST));
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
            if (m_targetSide == occupantSide && gpCombatManager->m_hexCells[*attackHex].m_occupantIndex == m_targetIndex)
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
short army::GetAdjacentCellIndex(short hex, short direction)
{
    if (hex == ARMY_HEX_INVALID)
        return ARMY_HEX_INVALID;
    if (direction == COMBAT_DIRECTION_WIDE_WEST)
        direction = (signed char)(m_facing == ARMY_FACING_RIGHT ? COMBAT_DIRECTION_NORTHWEST : COMBAT_DIRECTION_NORTHEAST);
    else if (direction == COMBAT_DIRECTION_WIDE_EAST)
        direction = (signed char)(m_facing == ARMY_FACING_RIGHT ? COMBAT_DIRECTION_SOUTHWEST : COMBAT_DIRECTION_SOUTHEAST);
    ProcessAssert(direction >= 0 && direction < COMBAT_DIRECTION_ADJACENT_COUNT, "D:\\Heroes\\Source\\PATH.CPP", gAdjacentCellAssertLine + 11);
    ProcessAssert(hex >= 0 && hex < COMBAT_HEX_COUNT, "D:\\Heroes\\Source\\PATH.CPP", gAdjacentCellAssertLine + 12);
    return gCombatAdjacency[hex][direction];
}

// Buka PATH.cpp GetAdjacentCellIndexNoArmy with HoMM1's asserts.
VA(0x004189f6, 0xf8)
short GetAdjacentCellIndexNoArmy(short hex, short direction)
{
    if (hex == ARMY_HEX_INVALID)
        return ARMY_HEX_INVALID;
    if (direction == COMBAT_DIRECTION_WIDE_WEST)
        direction = COMBAT_DIRECTION_NORTHWEST;
    else if (direction == COMBAT_DIRECTION_WIDE_EAST)
        direction = COMBAT_DIRECTION_SOUTHWEST;
    ProcessAssert(direction >= 0 && direction < COMBAT_DIRECTION_ADJACENT_COUNT, "D:\\Heroes\\Source\\PATH.CPP", gAdjacentCellNoArmyAssertLine + 11);
    ProcessAssert(hex >= 0 && hex < COMBAT_HEX_COUNT, "D:\\Heroes\\Source\\PATH.CPP", gAdjacentCellNoArmyAssertLine + 12);
    return gCombatAdjacency[hex][direction];
}

// Buka PATH.cpp ValidRange.
// @dead-code
// Zero-ref: no incoming call, jump or relocated reference in retail.
VA(0x00418aee, 0x4c2)
short army::ValidRange(short targetHex)
{
    short adjacentHex;
    short directionResult;

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
                directionResult = GetBestDirection(m_hex + WIDE_HEX_OFFSET, targetHex, SPECIAL_DIRECTION_MASK);
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
                directionResult = GetBestDirection(m_hex - WIDE_HEX_OFFSET, targetHex, SPECIAL_DIRECTION_MASK);
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
H1_ENUM_RETURN(CombatHexDirection, short)
OppositeDirection(H1_ENUM_PARAM(CombatHexDirection, short) direction)
{
    if (static_cast<int>(direction) < COMBAT_DIRECTION_ADJACENT_COUNT)
        return H1_ENUM_CAST(CombatHexDirection, short,
            (static_cast<int>(direction) + COMBAT_DIRECTION_OPPOSITE_OFFSET)
                % COMBAT_DIRECTION_ADJACENT_COUNT);
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
short army::GetBestDirection(short sourceHex, short targetHex, short blockedMask)
{
    signed char targetCol;
    signed char targetRowVal;
    signed char sourceColumnCheck;
    signed char iIsMovingDown;
    signed char movingUp;
    signed char sourceRowVal;
    signed char rightFl;
    signed char iLeftFl;

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

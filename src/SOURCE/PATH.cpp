// Located from HoMM2 Buka 2.1; PoL 2.0 supplies the VC4 declaration.

#include <match.h>

#include <SOURCE/PATH.h>

#include <H1/All.h>
#include <BASE/Misc.h>

// Compiler line-base words for PATH.CPP's ProcessAssert sites.
DATA(0x0048f4d4) short gAdjacentCellAssertLine;
DATA(0x0048f510) short gAdjacentCellNoArmyAssertLine;

// Buka PATH.cpp FindPath; HoMM1 takes the speed slot unused and retries a
// two-hex creature from its rear hex.
VA(0x004180f0, 0x152)
short army::FindPath(short sourceHex, short targetHex, signed char, signed char ignoreSpeed, signed char pathMode)
{
    short pathResult;
    int savedSpeed;

    if (!ValidHex(sourceHex) || !ValidHex(targetHex))
        return 0;
    savedSpeed = m_speed;
    if (ignoreSpeed)
        m_speed = 99;
    pathResult = gpSearchArray->FindCombatPath(sourceHex, targetHex, this, pathMode);
    if (!pathResult && (m_attributes & 1) && !pathMode) {
        switch (m_facing) {
            case 1:
                targetHex = GetAdjacentCellIndex(targetHex, 1);
                break;
            case 0:
                targetHex = GetAdjacentCellIndex(targetHex, 4);
                break;
        }
        if (!ValidHex(targetHex))
            pathResult = 0;
        else
            pathResult = gpSearchArray->FindCombatPath(sourceHex, targetHex, this, pathMode);
    }
    m_speed = savedSpeed;
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
    if (m_attributes & 2)
        return ValidFlight(targetHex, pathMode);
    pathResult = FindPath(m_hex, targetHex, m_speed, 0, pathMode);
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
    return blockedMask | 0xc0;
}

// Buka PATH.cpp GetAttackMask.
VA(0x0041835b, 0xbf)
short army::GetAttackMask(short sourceHex, signed char targetMode, signed char targetHex)
{
    short hex;
    short blockedMask;
    short nDirectionCount;
    short mask;
    short direction;

    if (m_attributes & 1)
        blockedMask = 0;
    else
        blockedMask = 0xc0;
    mask = 1;
    if (m_attributes & 1)
        nDirectionCount = 8;
    else
        nDirectionCount = 6;
    for (direction = 0; direction < nDirectionCount; direction++) {
        if (!ValidAttack(sourceHex, direction, targetMode, targetHex, &hex))
            blockedMask |= mask;
        mask <<= 1;
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
    if (gpCombatManager->m_hexCells[dest].m_occupantSide == -1 && gpCombatManager->m_hexCells[dest].m_obstacle == -1)
        frontValid = 1;
    if (m_attributes & 1) {
        backHex = -1;
        switch (m_facing) {
            case 1:
                if (direction == 1)
                    return frontValid;
                else
                    backHex = GetAdjacentCellIndex(dest, 4);
                break;
            case 0:
                if (direction == 4)
                    return frontValid;
                else
                    backHex = GetAdjacentCellIndex(dest, 1);
                break;
        }
        rearValid = 0;
        if (ValidHex(backHex) && gpCombatManager->m_hexCells[backHex].m_occupantSide == -1
            && gpCombatManager->m_hexCells[backHex].m_obstacle == -1)
            rearValid = 1;
        if (direction == 1 || direction == 4)
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
    if (m_attributes & 1) {
        if (direction == 6)
            *attackHex = GetAdjacentCellIndex(sourceHex, (signed char)(m_facing == 1 ? 5 : 0));
        else if (direction == 7)
            *attackHex = GetAdjacentCellIndex(sourceHex, (signed char)(m_facing == 1 ? 3 : 2));
        else {
            switch (m_facing) {
                case 1:
                    if (direction >= 3)
                        adjacentHex = GetAdjacentCellIndex(sourceHex, 4);
                    break;
                case 0:
                    if (direction <= 2)
                        adjacentHex = GetAdjacentCellIndex(sourceHex, 1);
                    break;
            }
            if (adjacentHex == -1)
                return 0;
            *attackHex = GetAdjacentCellIndex(adjacentHex, direction);
        }
    } else
        *attackHex = GetAdjacentCellIndex(sourceHex, direction);
    if (!ValidHex(*attackHex))
        return 0;
    if (requiredTargetHex != -1 && *attackHex != requiredTargetHex)
        return 0;
    occupantSide = gpCombatManager->m_hexCells[*attackHex].m_occupantSide;
    switch (targetMode) {
        case 0:
            if (m_targetSide == occupantSide && gpCombatManager->m_hexCells[*attackHex].m_occupantIndex == m_targetIndex)
                return 1;
            break;
        case 1:
            if (1 - gpCombatManager->m_currentSide == occupantSide)
                return 1;
            break;
        case 2:
            if (occupantSide != -1)
                return 1;
            break;
    }
    return 0;
}

// Buka PATH.cpp GetAdjacentCellIndex with HoMM1's asserts.
VA(0x004188d8, 0x11e)
short army::GetAdjacentCellIndex(short hex, short direction)
{
    if (hex == -1)
        return -1;
    if (direction == 6)
        direction = (signed char)(m_facing == 0 ? 5 : 0);
    else if (direction == 7)
        direction = (signed char)(m_facing == 0 ? 3 : 2);
    ProcessAssert(direction >= 0 && direction < 6, "D:\\Heroes\\Source\\PATH.CPP", gAdjacentCellAssertLine + 11);
    ProcessAssert(hex >= 0 && hex < 45, "D:\\Heroes\\Source\\PATH.CPP", gAdjacentCellAssertLine + 12);
    return gCombatAdjacency[hex][direction];
}

// Buka PATH.cpp GetAdjacentCellIndexNoArmy with HoMM1's asserts.
VA(0x004189f6, 0xf8)
short GetAdjacentCellIndexNoArmy(short hex, short direction)
{
    if (hex == -1)
        return -1;
    if (direction == 6)
        direction = 5;
    else if (direction == 7)
        direction = 3;
    ProcessAssert(direction >= 0 && direction < 6, "D:\\Heroes\\Source\\PATH.CPP", gAdjacentCellNoArmyAssertLine + 11);
    ProcessAssert(hex >= 0 && hex < 45, "D:\\Heroes\\Source\\PATH.CPP", gAdjacentCellNoArmyAssertLine + 12);
    return gCombatAdjacency[hex][direction];
}

// Buka PATH.cpp ValidRange.
VA(0x00418aee, 0x4c2)
short army::ValidRange(short targetHex)
{
    short adjacentHex;
    short directionResult;

    if (!ValidHex(targetHex))
        return 0;
    m_moveTargetHex = m_hex;
    if (!(m_attributes & 1)) {
        m_attackDirection = GetBestDirection(m_hex, targetHex, 0xc0);
        adjacentHex = GetAdjacentCellIndex(m_hex, m_attackDirection);
        if (adjacentHex == targetHex)
            return 1;
        adjacentHex = GetAdjacentCellIndex(adjacentHex, m_attackDirection);
        if (adjacentHex == targetHex)
            return 1;
    } else {
        switch (m_facing) {
            case 0:
                directionResult = GetBestDirection(m_hex, targetHex, 0xc0);
                if (directionResult > 2) {
                    m_attackDirection = directionResult;
                    adjacentHex = GetAdjacentCellIndex(m_hex, directionResult);
                    if (adjacentHex == targetHex)
                        return 1;
                    adjacentHex = GetAdjacentCellIndex(adjacentHex, directionResult);
                    if (adjacentHex == targetHex)
                        return 1;
                }
                directionResult = GetBestDirection(m_hex + 1, targetHex, 0xc0);
                if (directionResult < 3) {
                    m_attackDirection = directionResult;
                    adjacentHex = GetAdjacentCellIndex(m_hex + 1, directionResult);
                    if (adjacentHex == targetHex)
                        return 1;
                    adjacentHex = GetAdjacentCellIndex(adjacentHex, directionResult);
                    if (adjacentHex == targetHex)
                        return 1;
                }
                if (directionResult == 4)
                    return 0;
                if (directionResult == 5)
                    m_attackDirection = 6;
                else if (directionResult == 3)
                    m_attackDirection = 7;
                adjacentHex = GetAdjacentCellIndex(m_hex + 1, directionResult);
                if (adjacentHex == targetHex)
                    return 1;
                adjacentHex = GetAdjacentCellIndex(adjacentHex, directionResult);
                if (adjacentHex == targetHex)
                    return 1;
                break;
            case 1:
                directionResult = GetBestDirection(m_hex, targetHex, 0xc0);
                if (directionResult < 3) {
                    m_attackDirection = directionResult;
                    adjacentHex = GetAdjacentCellIndex(m_hex, directionResult);
                    if (adjacentHex == targetHex)
                        return 1;
                    adjacentHex = GetAdjacentCellIndex(adjacentHex, directionResult);
                    if (adjacentHex == targetHex)
                        return 1;
                    return 0;
                }
                directionResult = GetBestDirection(m_hex - 1, targetHex, 0xc0);
                if (directionResult > 2) {
                    m_attackDirection = directionResult;
                    adjacentHex = GetAdjacentCellIndex(m_hex - 1, directionResult);
                    if (adjacentHex == targetHex)
                        return 1;
                    adjacentHex = GetAdjacentCellIndex(adjacentHex, directionResult);
                    if (adjacentHex == targetHex)
                        return 1;
                    return 0;
                }
                if (directionResult == 1)
                    return 0;
                if (directionResult == 0)
                    m_attackDirection = 6;
                else if (directionResult == 2)
                    m_attackDirection = 7;
                adjacentHex = GetAdjacentCellIndex(m_hex - 1, directionResult);
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
    signed char iLeftFl;
    signed char rightFl;

    if (!ValidHex(sourceHex) || !ValidHex(targetHex))
        return -1;
    sourceColumnCheck = sourceHex % 9;
    sourceRowVal = sourceHex / 9;
    targetCol = targetHex % 9;
    targetRowVal = targetHex / 9;
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
                if (!(blockedMask & 0x20))
                    return 5;
                else if (!(blockedMask & 0x1))
                    return 0;
                else if (!(blockedMask & 0x10))
                    return 4;
                else if (!(blockedMask & 0x2))
                    return 1;
                else if (!(blockedMask & 0x8))
                    return 3;
                else if (!(blockedMask & 0x4))
                    return 2;
                else if (!(blockedMask & 0x40))
                    return 6;
                else if (!(blockedMask & 0x80))
                    return 7;
            } else {
                if (!(blockedMask & 0x1))
                    return 0;
                else if (!(blockedMask & 0x20))
                    return 5;
                else if (!(blockedMask & 0x2))
                    return 1;
                else if (!(blockedMask & 0x10))
                    return 4;
                else if (!(blockedMask & 0x4))
                    return 2;
                else if (!(blockedMask & 0x8))
                    return 3;
                else if (!(blockedMask & 0x40))
                    return 6;
                else if (!(blockedMask & 0x80))
                    return 7;
            }
        } else {
            if (sourceRowVal & 1) {
                if (!(blockedMask & 0x8))
                    return 3;
                else if (!(blockedMask & 0x4))
                    return 2;
                else if (!(blockedMask & 0x10))
                    return 4;
                else if (!(blockedMask & 0x2))
                    return 1;
                else if (!(blockedMask & 0x20))
                    return 5;
                else if (!(blockedMask & 0x1))
                    return 0;
                else if (!(blockedMask & 0x80))
                    return 7;
                else if (!(blockedMask & 0x40))
                    return 6;
            } else {
                if (!(blockedMask & 0x4))
                    return 2;
                else if (!(blockedMask & 0x8))
                    return 3;
                else if (!(blockedMask & 0x2))
                    return 1;
                else if (!(blockedMask & 0x10))
                    return 4;
                else if (!(blockedMask & 0x1))
                    return 0;
                else if (!(blockedMask & 0x20))
                    return 5;
                else if (!(blockedMask & 0x80))
                    return 7;
                else if (!(blockedMask & 0x40))
                    return 6;
            }
        }
    }
    if (iLeftFl == 1) {
        if (movingUp == 1) {
            if (!(blockedMask & 0x20))
                return 5;
            else if (!(blockedMask & 0x10))
                return 4;
            else if (!(blockedMask & 0x1))
                return 0;
            else if (!(blockedMask & 0x8))
                return 3;
            else if (!(blockedMask & 0x2))
                return 1;
            else if (!(blockedMask & 0x4))
                return 2;
            else if (!(blockedMask & 0x40))
                return 6;
            else if (!(blockedMask & 0x80))
                return 7;
        } else if (iIsMovingDown == 1) {
            if (!(blockedMask & 0x8))
                return 3;
            else if (!(blockedMask & 0x10))
                return 4;
            else if (!(blockedMask & 0x4))
                return 2;
            else if (!(blockedMask & 0x20))
                return 5;
            else if (!(blockedMask & 0x2))
                return 1;
            else if (!(blockedMask & 0x1))
                return 0;
            else if (!(blockedMask & 0x80))
                return 7;
            else if (!(blockedMask & 0x40))
                return 6;
        } else {
            if (!(blockedMask & 0x10))
                return 4;
            else if (!(blockedMask & 0x20))
                return 5;
            else if (!(blockedMask & 0x8))
                return 3;
            else if (!(blockedMask & 0x1))
                return 0;
            else if (!(blockedMask & 0x4))
                return 2;
            else if (!(blockedMask & 0x2))
                return 1;
            else if (!(blockedMask & 0x80))
                return 7;
            else if (!(blockedMask & 0x40))
                return 6;
        }
    } else if (rightFl == 1) {
        if (movingUp == 1) {
            if (!(blockedMask & 0x1))
                return 0;
            else if (!(blockedMask & 0x2))
                return 1;
            else if (!(blockedMask & 0x20))
                return 5;
            else if (!(blockedMask & 0x4))
                return 2;
            else if (!(blockedMask & 0x10))
                return 4;
            else if (!(blockedMask & 0x8))
                return 3;
            else if (!(blockedMask & 0x40))
                return 6;
            else if (!(blockedMask & 0x80))
                return 7;
        } else if (iIsMovingDown == 1) {
            if (!(blockedMask & 0x4))
                return 2;
            else if (!(blockedMask & 0x2))
                return 1;
            else if (!(blockedMask & 0x8))
                return 3;
            else if (!(blockedMask & 0x1))
                return 0;
            else if (!(blockedMask & 0x20))
                return 5;
            else if (!(blockedMask & 0x10))
                return 4;
            else if (!(blockedMask & 0x80))
                return 7;
            else if (!(blockedMask & 0x40))
                return 6;
        } else {
            if (!(blockedMask & 0x2))
                return 1;
            else if (!(blockedMask & 0x1))
                return 0;
            else if (!(blockedMask & 0x4))
                return 2;
            else if (!(blockedMask & 0x20))
                return 5;
            else if (!(blockedMask & 0x8))
                return 3;
            else if (!(blockedMask & 0x10))
                return 4;
            else if (!(blockedMask & 0x80))
                return 7;
            else if (!(blockedMask & 0x40))
                return 6;
        }
    }
    return -1;
}

// Located from HoMM2 Buka 2.1 FLY.cpp; HoMM1 flies in whole-pixel steps,
// six frames per hex, and CanFit always tries the other side of a wide hex.

#include <match.h>

#include <BASE/bitmap.h>
#include <BASE/heroWindowManager.h>
#include <BASE/icon.h>
#include <BASE/audio.h>
#include <SOURCE/army.h>
#include <SOURCE/armyGroup.h>
#include <SOURCE/combatManager.h>
#include <SOURCE/hexcell.h>
#include <SOURCE/KB.h>
#include <SOURCE/kbwin.h>
#include <SOURCE/NOOPT.h>
#include <SOURCE/PATH.h>

#include <math.h>

// HoMM1: the hex arrives through a word pointer; a two-hex creature that
// does not fit facing forward moves its hex to the other side.
VA(0x0042a6a0, 0x200)
i16 army::CanFit(i16* hex) {
    hexcell* cell;
    i16 candidateHex;

    candidateHex = *hex;
    cell = NULL;
    if (!ValidHex(candidateHex) || candidateHex % COMBAT_GRID_COLUMNS == 0
        || candidateHex % COMBAT_GRID_COLUMNS == COMBAT_GRID_LAST_COLUMN)
        return 0;
    if (gpCombatManager->m_hexCells[candidateHex].m_occupantSide != COMBAT_SIDE_NONE
        || gpCombatManager->m_hexCells[candidateHex].m_obstacleIndex != COMBAT_OBSTACLE_NONE)
        return 0;
    if (m_stats.attributes & MONSTER_FLAGS_WIDE) {
        candidateHex = GetAdjacentCellIndex(
            *hex,
            m_facing == ARMY_FACING_RIGHT ? static_cast<i8>(COMBAT_DIRECTION_EAST)
                                         : static_cast<i8>(COMBAT_DIRECTION_WEST)
        );
        if (ValidHex(candidateHex))
            cell = &gpCombatManager->m_hexCells[candidateHex];
        if (ValidHex(candidateHex)
            && (cell->m_occupantSide == COMBAT_SIDE_NONE
                || (cell->m_occupantSide == gpCombatManager->m_currentSide
                    && cell->m_occupantIndex == gpCombatManager->m_currentArmyIndex))
            && cell->m_obstacleIndex == COMBAT_OBSTACLE_NONE) {
            return 1;
        } else {
            candidateHex = GetAdjacentCellIndex(
                *hex,
                m_facing == ARMY_FACING_RIGHT ? static_cast<i8>(COMBAT_DIRECTION_WEST)
                                             : static_cast<i8>(COMBAT_DIRECTION_EAST)
            );
            if (ValidHex(candidateHex))
                cell = &gpCombatManager->m_hexCells[candidateHex];
            else
                return 0;
            if ((cell->m_occupantSide == COMBAT_SIDE_NONE
                 || (cell->m_occupantSide == gpCombatManager->m_currentSide
                     && cell->m_occupantIndex == gpCombatManager->m_currentArmyIndex))
                && cell->m_obstacleIndex == COMBAT_OBSTACLE_NONE) {
                *hex = candidateHex;
                return 1;
            } else {
                return 0;
            }
        }
    } else {
        return 1;
    }
}

// Buka FLY.cpp ValidFlight; HoMM1 passes a flag that takes the destination
// as the enemy hex, and CanFit moves the landing hex in place.
VA(0x0042a8a0, 0x3f0)
i16 army::ValidFlight(i16 destination, i8 useDestination) {
    i16 directionMask;
    i16 temp;
    i16 hitHex;
    i16 attackDirections;
    i16 nextHex;
    i16 i;
    i8 dir;
    i16 j;
    army* opponent;
    i16 targetHex;
    i16 n;
    i8 attackDirection;

    if (!ValidHex(destination))
        return 0;
    if (m_targetSide < 0 || m_targetSide > COMBAT_SIDE_COUNT - 1 || m_targetIndex < 0
        || m_targetIndex > ARMY_GROUP_SLOT_COUNT - 1) {
        if (CanFit(&destination)) {
            m_moveTargetHex = destination;
            return 1;
        } else {
            return 0;
        }
    }
    opponent = &gpCombatManager->m_armies[m_targetSide][m_targetIndex];
    if (useDestination)
        targetHex = destination;
    else
        targetHex = opponent->m_hex;
    if (!ValidHex(targetHex))
        return 0;
    attackDirections = GetAttackMask(m_hex, ARMY_ATTACK_TARGET_ASSIGNED, ARMY_HEX_INVALID);
    while (attackDirections != COMBAT_ALL_DIRECTIONS_BLOCKED) {
        attackDirection = GetBestDirection(m_hex, targetHex, attackDirections);
        if (ValidAttack(
                m_hex,
                attackDirection,
                ARMY_ATTACK_TARGET_ASSIGNED,
                ARMY_HEX_INVALID,
                &hitHex
            )) {
            m_attackDirection = attackDirection;
            m_moveTargetHex = m_hex;
            return 1;
        } else {
            attackDirections |= 1 << attackDirection;
        }
    }
    directionMask = 0;
    if ((opponent->m_stats.attributes & MONSTER_FLAGS_WIDE) && !useDestination) {
        targetHex += opponent->m_facing == ARMY_FACING_RIGHT ? 1 : -1;
        directionMask = opponent->m_facing == ARMY_FACING_RIGHT
                            ? COMBAT_DIRECTION_BIT_WEST : COMBAT_DIRECTION_BIT_EAST;
    }
    while (directionMask != (1 << COMBAT_DIRECTION_ADJACENT_COUNT) - 1) {
        dir = GetBestDirection(targetHex, m_hex, directionMask);
        nextHex = GetAdjacentCellIndex(targetHex, dir);
        if (ValidHex(nextHex) && CanFit(&nextHex)) {
            m_moveTargetHex = nextHex;
            if (!(m_stats.attributes & MONSTER_FLAGS_WIDE)) {
                m_attackDirection = OppositeDirection(dir);
            } else {
                attackDirections =
                    ~GetAttackMask(m_moveTargetHex, ARMY_ATTACK_TARGET_ASSIGNED, ARMY_HEX_INVALID);
                for (n = 0; n < COMBAT_DIRECTION_COUNT; n++) {
                    if (attackDirections & (1 << n))
                        m_attackDirection = n;
                }
            }
            return 1;
        } else {
            directionMask |= 1 << dir;
        }
    }
    if ((opponent->m_stats.attributes & MONSTER_FLAGS_WIDE) && !useDestination) {
        targetHex += opponent->m_facing == ARMY_FACING_RIGHT ? -1 : 1;
        directionMask = opponent->m_facing == ARMY_FACING_RIGHT
                            ? COMBAT_DIRECTION_BIT_EAST : COMBAT_DIRECTION_BIT_WEST;
        while (directionMask != (1 << COMBAT_DIRECTION_ADJACENT_COUNT) - 1) {
            dir = GetBestDirection(targetHex, m_hex, directionMask);
            nextHex = GetAdjacentCellIndex(targetHex, dir);
            if (ValidHex(nextHex) && CanFit(&nextHex)) {
                m_moveTargetHex = nextHex;
                m_attackDirection = GetBestDirection(m_moveTargetHex, targetHex, 0);
                return 1;
            } else {
                directionMask |= 1 << dir;
            }
        }
    }
    return 0;
}

// Buka FLY.cpp FlyTo(void).
// @dead-code
// Zero-ref: no incoming call, jump or relocated reference in retail.
VA(0x0042ac90, 0x1c)
i16 army::FlyTo(void) {
    return FlyTo(m_moveTargetHex);
}

// HoMM1 flies along a straight pixel line: six frames per hex of the longer
// grid axis, the rounding remainder split over the two ends.
VA(0x0042acac, 0x70c)
i16 army::FlyTo(i16 destination) {
    i16 iFinalY;
    i16 centerY;
    i16 yLow;
    i16 posX;
    i16 xOff;
    i8 curRow;
    i16 rowDist;
    i16 colCount;
    i16 yStep;
    i16 posY;
    i16 steps;
    i16 i;
    i32 maxExtentX;
    i32 oldMaxY;
    i8 colFrom;
    i8 backwards;
    i8 toHexRow;
    i8 endCol;
    i16 yFrom;
    i32 oldX;
    i16 xStep;
    i16 xFrom;
    i32 oldY;
    i16 destX;
    i16 farX;
    i16 firstX;
    i16 destY;

    if (!ValidHex(destination))
        return 0;
    colFrom = m_hex % COMBAT_GRID_COLUMNS;
    curRow = m_hex / COMBAT_GRID_COLUMNS;
    endCol = destination % COMBAT_GRID_COLUMNS;
    toHexRow = destination / COMBAT_GRID_COLUMNS;
    colCount = endCol - colFrom;
    if (colCount < 0)
        colCount = -colCount;
    rowDist = toHexRow - curRow;
    if (rowDist < 0)
        rowDist = -rowDist;
    steps = colCount > rowDist ? colCount : rowDist;
    destX = gpCombatManager->m_hexCells[destination].m_x;
    destY = gpCombatManager->m_hexCells[destination].m_y;
    xFrom = gpCombatManager->m_hexCells[m_hex].m_x;
    yFrom = gpCombatManager->m_hexCells[m_hex].m_y;
    if (colCount == 0)
        xStep = 0;
    else
        xStep = (destX - xFrom) / (steps * 6);
    if (rowDist == 0)
        yStep = 0;
    else
        yStep = (destY - yFrom) / (steps * 6);
    firstX = xFrom + xStep;
    farX = destX - steps * 6 * xStep;
    xOff = (firstX + farX) / 2 - firstX;
    yLow = yFrom + yStep;
    iFinalY = destY - steps * 6 * yStep;
    centerY = (yLow + iFinalY) / 2 - yLow;
    backwards = 0;
    if ((xStep < 0 && m_facing == ARMY_FACING_RIGHT) || (xStep > 0 && m_facing == ARMY_FACING_LEFT))
        backwards = 1;
    hexcell frontCell;
    hexcell otherCell;
    frontCell.TakeOccupant(&gpCombatManager->m_hexCells[m_hex]);
    if (m_stats.attributes & MONSTER_FLAGS_WIDE)
        otherCell.TakeOccupant(
            &gpCombatManager->m_hexCells[m_hex + (m_facing == ARMY_FACING_LEFT ? -1 : 1)]
        );
    posX = xFrom + xOff;
    posY = yFrom + centerY;
    m_animationSequence = ARMY_ANIMATION_WALK;
    m_animationFrame = backwards == 1 ? 5 : 0;
    frontCell.m_occupantSide = COMBAT_SIDE_NONE;
    if (m_stats.attributes & MONSTER_FLAGS_WIDE)
        otherCell.m_occupantSide = COMBAT_SIDE_NONE;
    gpCombatManager->DrawFrame(0);
    gpWindowManager->m_screen->CopyTo(
        gpCombatManager->m_backgroundBuffer,
        0,
        0,
        0,
        0,
        LOGICAL_SCREEN_WIDTH,
        COMBAT_VIEW_HEIGHT
    );
    gpCombatManager->m_backgroundDrawn = 0;
    for (i = 0; i < steps * 6; i++) {
        if (i % 6 == 1)
            PlaySample(m_samples[ARMY_SAMPLE_MOVE]);
        if (i) {
            gpCombatManager->m_backgroundBuffer->CopyTo(
                gpWindowManager->m_screen,
                giMinExtentX,
                giMinExtentY,
                giMinExtentX,
                giMinExtentY,
                giMaxExtentX - giMinExtentX + 1,
                giMaxExtentY - giMinExtentY + 1
            );
            oldX = giMinExtentX;
            oldY = giMinExtentY;
            maxExtentX = giMaxExtentX;
            oldMaxY = giMaxExtentY;
        } else {
            oldX = 0;
            oldY = 0;
            maxExtentX = LOGICAL_SCREEN_WIDTH - 1;
            oldMaxY = COMBAT_VIEW_HEIGHT - 1;
        }
        giMinExtentY = COMBAT_EXTENT_MIN_START;
        giMinExtentX = giMinExtentY;
        giMaxExtentY = 0;
        giMaxExtentX = giMaxExtentY;
        gComputeExtent = 1;
        gSaveBiggestExtent = 1;
        DrawToBuffer(posX, posY);
        gComputeExtent = 0;
        gSaveBiggestExtent = 0;
        if (giMinExtentX < 0)
            giMinExtentX = 0;
        if (giMinExtentY < 0)
            giMinExtentY = 0;
        if (giMaxExtentX > LOGICAL_SCREEN_WIDTH - 1)
            giMaxExtentX = LOGICAL_SCREEN_WIDTH - 1;
        if (giMaxExtentY > COMBAT_VIEW_HEIGHT - 1)
            giMaxExtentY = COMBAT_VIEW_HEIGHT - 1;
        if (giMinExtentX < oldX)
            oldX = giMinExtentX;
        if (giMinExtentY < oldY)
            oldY = giMinExtentY;
        if (giMaxExtentX > maxExtentX)
            maxExtentX = giMaxExtentX;
        if (giMaxExtentY > oldMaxY)
            oldMaxY = giMaxExtentY;
        DelayTil(glTimers);
        glTimers[COMBAT_FRAME_TIMER_SLOT] = KBTickCount() + 75;
        UPDATE_INCLUSIVE_REGION(oldX, oldY, maxExtentX, oldMaxY);
        m_animationFrame += backwards == 1 ? -1 : 1;
        if (m_animationFrame > 5)
            m_animationFrame = 0;
        else if (m_animationFrame < 0)
            m_animationFrame = 5;
        posX += xStep;
        posY += yStep;
    }
    if (!m_spellEndCondition)
        CancelSpell();
    frontCell.m_occupantSide = gpCombatManager->m_currentSide;
    if (m_stats.attributes & MONSTER_FLAGS_WIDE)
        otherCell.m_occupantSide = gpCombatManager->m_currentSide;
    gpCombatManager->m_hexCells[destination].TakeOccupant(&frontCell);
    if (m_stats.attributes & MONSTER_FLAGS_WIDE)
        gpCombatManager->m_hexCells[destination + (m_facing == ARMY_FACING_LEFT ? -1 : 1)]
            .TakeOccupant(&otherCell);
    m_hex = destination;
    m_animationSequence = ARMY_ANIMATION_STAND;
    m_animationFrame = 1;
    gpCombatManager->UpdateGrid(destination, m_stats.attributes);
    gpCombatManager->DrawFrame(1);
    return 1;
}

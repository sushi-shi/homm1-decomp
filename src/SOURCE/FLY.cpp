// Flight moves in whole-pixel steps, six frames per hex, and CanFit always
// tries the other side of a wide hex.

#include <match.h>

#include <BASE/audio.h>
#include <BASE/bitmap.h>
#include <BASE/heroWindowManager.h>
#include <BASE/icon.h>
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
    hexcell* adjacentCell;
    i16 candidateHex;

    candidateHex = *hex;
    adjacentCell = NULL;
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
            adjacentCell = &gpCombatManager->m_hexCells[candidateHex];
        if (ValidHex(candidateHex)
            && (adjacentCell->m_occupantSide == COMBAT_SIDE_NONE
                || HEX_HAS_OCCUPANT(
                    *adjacentCell,
                    gpCombatManager->m_currentSide,
                    gpCombatManager->m_currentArmyIndex
                ))
            && adjacentCell->m_obstacleIndex == COMBAT_OBSTACLE_NONE) {
            return 1;
        } else {
            candidateHex = GetAdjacentCellIndex(
                *hex,
                m_facing == ARMY_FACING_RIGHT ? static_cast<i8>(COMBAT_DIRECTION_WEST)
                                              : static_cast<i8>(COMBAT_DIRECTION_EAST)
            );
            if (ValidHex(candidateHex))
                adjacentCell = &gpCombatManager->m_hexCells[candidateHex];
            else
                return 0;
            if ((adjacentCell->m_occupantSide == COMBAT_SIDE_NONE
                 || HEX_HAS_OCCUPANT(
                     *adjacentCell,
                     gpCombatManager->m_currentSide,
                     gpCombatManager->m_currentArmyIndex
                 ))
                && adjacentCell->m_obstacleIndex == COMBAT_OBSTACLE_NONE) {
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

// A non-zero path mode takes the destination as the enemy hex, and CanFit moves the
// landing hex in place.
VA(0x0042a8a0, 0x3f0)
i16 army::ValidFlight(i16 destination, i8 pathMode) {
    i16 enemyHex;
    i16 heldTemp;
    i16 attackHex;
    i16 attackDirectionsMask;
    i16 adjacentHex;
    i16 k;
    i8 landingDirection;
    i16 m;
    army* enemyStack;
    i16 directionMask;
    i16 n;
    i8 bestAttackDirection;

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
    enemyStack = &gpCombatManager->m_armies[m_targetSide][m_targetIndex];
    if (pathMode)
        enemyHex = destination;
    else
        enemyHex = enemyStack->m_hex;
    if (!ValidHex(enemyHex))
        return 0;
    attackDirectionsMask = GetAttackMask(m_hex, ARMY_ATTACK_TARGET_ASSIGNED, ARMY_HEX_INVALID);
    while (attackDirectionsMask != COMBAT_ALL_DIRECTIONS_BLOCKED) {
        bestAttackDirection = GetBestDirection(m_hex, enemyHex, attackDirectionsMask);
        if (ValidAttack(
                m_hex,
                bestAttackDirection,
                ARMY_ATTACK_TARGET_ASSIGNED,
                ARMY_HEX_INVALID,
                &attackHex
            )) {
            m_attackDirection = bestAttackDirection;
            m_moveTargetHex = m_hex;
            return 1;
        } else {
            attackDirectionsMask |= 1 << bestAttackDirection;
        }
    }
    directionMask = 0;
    if ((enemyStack->m_stats.attributes & MONSTER_FLAGS_WIDE) && !pathMode) {
        enemyHex += enemyStack->m_facing == ARMY_FACING_RIGHT ? 1 : -1;
        directionMask = enemyStack->m_facing == ARMY_FACING_RIGHT ? COMBAT_DIRECTION_BIT_WEST
                                                                  : COMBAT_DIRECTION_BIT_EAST;
    }
    while (directionMask != (1 << COMBAT_DIRECTION_ADJACENT_COUNT) - 1) {
        landingDirection = GetBestDirection(enemyHex, m_hex, directionMask);
        adjacentHex = GetAdjacentCellIndex(enemyHex, landingDirection);
        if (ValidHex(adjacentHex) && CanFit(&adjacentHex)) {
            m_moveTargetHex = adjacentHex;
            if (!(m_stats.attributes & MONSTER_FLAGS_WIDE)) {
                m_attackDirection = OppositeDirection(landingDirection);
            } else {
                attackDirectionsMask =
                    ~GetAttackMask(m_moveTargetHex, ARMY_ATTACK_TARGET_ASSIGNED, ARMY_HEX_INVALID);
                for (n = 0; n < COMBAT_DIRECTION_COUNT; n++) {
                    if (attackDirectionsMask & (1 << n))
                        m_attackDirection = n;
                }
            }
            return 1;
        } else {
            directionMask |= 1 << landingDirection;
        }
    }
    if ((enemyStack->m_stats.attributes & MONSTER_FLAGS_WIDE) && !pathMode) {
        enemyHex += enemyStack->m_facing == ARMY_FACING_RIGHT ? -1 : 1;
        directionMask = enemyStack->m_facing == ARMY_FACING_RIGHT ? COMBAT_DIRECTION_BIT_EAST
                                                                  : COMBAT_DIRECTION_BIT_WEST;
        while (directionMask != (1 << COMBAT_DIRECTION_ADJACENT_COUNT) - 1) {
            landingDirection = GetBestDirection(enemyHex, m_hex, directionMask);
            adjacentHex = GetAdjacentCellIndex(enemyHex, landingDirection);
            if (ValidHex(adjacentHex) && CanFit(&adjacentHex)) {
                m_moveTargetHex = adjacentHex;
                m_attackDirection = GetBestDirection(m_moveTargetHex, enemyHex, 0);
                return 1;
            } else {
                directionMask |= 1 << landingDirection;
            }
        }
    }
    return 0;
}

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
    i32 boxRightX;
    i8 colFrom;
    i8 flyBackwards;
    i32 boxTop;
    i8 curRow;
    i32 boxLeft;
    i16 gainX;
    i16 inFlightX;
    i16 gainY;
    i16 inFlightY;
    i16 landX;
    i16 landY;
    i16 adjustX;
    i16 k;
    i8 targetRowIndex;
    i8 aimColumn;
    i16 fullXLen;
    i16 flightSteps;
    i16 startX;
    i16 launchX;
    i16 y1;
    i16 adjustY;
    i16 fullYLen;
    i16 startY;
    i16 landPosX;
    i16 y2;
    i32 boxBottom;

    if (!ValidHex(destination))
        return 0;
    colFrom = m_hex % COMBAT_GRID_COLUMNS;
    curRow = m_hex / COMBAT_GRID_COLUMNS;
    aimColumn = destination % COMBAT_GRID_COLUMNS;
    targetRowIndex = destination / COMBAT_GRID_COLUMNS;
    fullXLen = aimColumn - colFrom;
    if (fullXLen < 0)
        fullXLen = -fullXLen;
    fullYLen = targetRowIndex - curRow;
    if (fullYLen < 0)
        fullYLen = -fullYLen;
    flightSteps = fullXLen > fullYLen ? fullXLen : fullYLen;
    landX = gpCombatManager->m_hexCells[destination].m_x;
    landY = gpCombatManager->m_hexCells[destination].m_y;
    startX = gpCombatManager->m_hexCells[m_hex].m_x;
    startY = gpCombatManager->m_hexCells[m_hex].m_y;
    if (fullXLen == 0)
        gainX = 0;
    else
        gainX = (landX - startX) / (flightSteps * 6);
    if (fullYLen == 0)
        gainY = 0;
    else
        gainY = (landY - startY) / (flightSteps * 6);
    launchX = startX + gainX;
    landPosX = landX - flightSteps * 6 * gainX;
    adjustX = (launchX + landPosX) / 2 - launchX;
    y1 = startY + gainY;
    y2 = landY - flightSteps * 6 * gainY;
    adjustY = (y1 + y2) / 2 - y1;
    flyBackwards = 0;
    if ((gainX < 0 && m_facing == ARMY_FACING_RIGHT) || (gainX > 0 && m_facing == ARMY_FACING_LEFT))
        flyBackwards = 1;
    hexcell headOccupant;
    hexcell tailSlot;
    headOccupant.TakeOccupant(&gpCombatManager->m_hexCells[m_hex]);
    if (m_stats.attributes & MONSTER_FLAGS_WIDE)
        tailSlot.TakeOccupant(
            &gpCombatManager->m_hexCells[m_hex + (m_facing == ARMY_FACING_LEFT ? -1 : 1)]
        );
    inFlightX = startX + adjustX;
    inFlightY = startY + adjustY;
    m_animationSequence = ARMY_ANIMATION_WALK;
    m_animationFrame = flyBackwards == 1 ? 5 : 0;
    headOccupant.m_occupantSide = COMBAT_SIDE_NONE;
    if (m_stats.attributes & MONSTER_FLAGS_WIDE)
        tailSlot.m_occupantSide = COMBAT_SIDE_NONE;
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
    for (k = 0; k < flightSteps * 6; k++) {
        if (k % 6 == 1)
            PlaySample(m_samples[ARMY_SAMPLE_MOVE]);
        if (k) {
            gpCombatManager->m_backgroundBuffer->CopyTo(
                gpWindowManager->m_screen,
                giMinExtentX,
                giMinExtentY,
                giMinExtentX,
                giMinExtentY,
                giMaxExtentX - giMinExtentX + 1,
                giMaxExtentY - giMinExtentY + 1
            );
            boxLeft = giMinExtentX;
            boxTop = giMinExtentY;
            boxRightX = giMaxExtentX;
            boxBottom = giMaxExtentY;
        } else {
            boxLeft = 0;
            boxTop = 0;
            boxRightX = LOGICAL_SCREEN_WIDTH - 1;
            boxBottom = COMBAT_VIEW_HEIGHT - 1;
        }
        giMinExtentY = COMBAT_EXTENT_MIN_START;
        giMinExtentX = giMinExtentY;
        giMaxExtentY = 0;
        giMaxExtentX = giMaxExtentY;
        gComputeExtent = 1;
        gSaveBiggestExtent = 1;
        DrawToBuffer(inFlightX, inFlightY);
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
        if (giMinExtentX < boxLeft)
            boxLeft = giMinExtentX;
        if (giMinExtentY < boxTop)
            boxTop = giMinExtentY;
        if (giMaxExtentX > boxRightX)
            boxRightX = giMaxExtentX;
        if (giMaxExtentY > boxBottom)
            boxBottom = giMaxExtentY;
        DelayTil(&glTimers[COMBAT_FRAME_TIMER_SLOT]);
        glTimers[COMBAT_FRAME_TIMER_SLOT] = KBTickCount() + 75;
        UPDATE_INCLUSIVE_REGION(boxLeft, boxTop, boxRightX, boxBottom);
        m_animationFrame += flyBackwards == 1 ? -1 : 1;
        if (m_animationFrame > 5)
            m_animationFrame = 0;
        else if (m_animationFrame < 0)
            m_animationFrame = 5;
        inFlightX += gainX;
        inFlightY += gainY;
    }
    if (!m_spellEndCondition)
        CancelSpell();
    headOccupant.m_occupantSide = gpCombatManager->m_currentSide;
    if (m_stats.attributes & MONSTER_FLAGS_WIDE)
        tailSlot.m_occupantSide = gpCombatManager->m_currentSide;
    gpCombatManager->m_hexCells[destination].TakeOccupant(&headOccupant);
    if (m_stats.attributes & MONSTER_FLAGS_WIDE)
        gpCombatManager->m_hexCells[destination + (m_facing == ARMY_FACING_LEFT ? -1 : 1)]
            .TakeOccupant(&tailSlot);
    m_hex = destination;
    m_animationSequence = ARMY_ANIMATION_STAND;
    m_animationFrame = 1;
    gpCombatManager->UpdateGrid(destination, m_stats.attributes);
    gpCombatManager->DrawFrame(1);
    return 1;
}

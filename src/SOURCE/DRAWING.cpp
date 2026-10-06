// Combat screen drawing. The combat window keeps one text line in the combat window and
// redraws the battlefield from the grid row UpdateGrid records.

#include <match.h>

#include <BASE/bitmap.h>
#include <BASE/heroWindow.h>
#include <BASE/heroWindowManager.h>
#include <BASE/icon.h>
#include <BASE/message.h>
#include <SOURCE/army.h>
#include <SOURCE/armyGroup.h>
#include <SOURCE/combatManager.h>
#include <SOURCE/combatTypes.h>
#include <SOURCE/game.h>
#include <SOURCE/KB.h>
#include <SOURCE/kbwin.h>
#include <SOURCE/NOOPT.h>
#include <SOURCE/wingraph.h>

#include <stdio.h>
#include <string.h>

// Lowers the first grid row that needs redrawing to the one above the hex.
VA(0x00423670, 0x50)
void combatManager::UpdateGrid(i16 hex, i16 attributes) {
    i16 row;

    row = hex / COMBAT_GRID_COLUMNS - 1;
    if (row < 0)
        row = 0;
    if (row < m_gridUpdateRow)
        m_gridUpdateRow = row;
}

// @dead-code
// Zero-ref: no incoming call, jump or relocated reference in retail.
VA(0x004236c0, 0x47)
void combatManager::UpdateGridForMove(
    i16 hex,
    H1_ENUM_PARAM(CombatHexDirection, i8) direction,
    i16 attributes
) {
    if (direction == COMBAT_DIRECTION_NORTHEAST || direction == COMBAT_DIRECTION_NORTHWEST)
        UpdateGrid(hex - COMBAT_GRID_COLUMNS, attributes);
    else
        UpdateGrid(hex, attributes);
}

// Sets the combat window's text line and redraws it outside the extent
// bookkeeping.
VA(0x00423707, 0xad)
// clang-format on
void combatManager::CombatMessage(char* text, b32 updateScreen) {
    i32 oldCompute;
    tag_message message;
    i32 prevLimit;

    SET_WIDGET_MESSAGE(message, WIDGET_COMMAND_SET_TEXT, COMBAT_STATUS_TEXT_CONTROL);
    message.text = text;
    m_combatWindow->BroadcastMessage(message);
    oldCompute = gComputeExtent;
    prevLimit = gLimitToExtent;
    gComputeExtent = gLimitToExtent = 0;
    m_combatWindow->DrawWindow(0, COMBAT_STATUS_FIRST_CONTROL, COMBAT_STATUS_TEXT_CONTROL);
    SaveCombatBorder();
    if (updateScreen)
        gWindowManager->UpdateScreenRegion(
            COMBAT_STATUS_X,
            COMBAT_STATUS_Y,
            COMBAT_STATUS_WIDTH,
            COMBAT_STATUS_HEIGHT
        );
    gComputeExtent = oldCompute;
    gLimitToExtent = prevLimit;
}

// The help line for the current mouse command.
VA(0x004237b4, 0x286)
void combatManager::CombatMessage(H1_ENUM_PARAM(CombatMessageCommand, i16) messageType) {
    army* currentArmy;
    army* targetArmy;
    H1_ENUM_LOCAL(CreatureType, i16) actingMonsterType;
    H1_ENUM_LOCAL(CreatureType, i16) targetMonsterType;

    currentArmy = &m_armies[m_currentSide][m_currentArmyIndex];
    actingMonsterType = currentArmy->m_creatureType;
    targetArmy = NULL;
    targetMonsterType = CREATURE_FIRST;
    if (currentArmy->m_targetSide >= COMBAT_SIDE_FIRST && currentArmy->m_targetIndex >= 0) {
        targetArmy = &m_armies[currentArmy->m_targetSide][currentArmy->m_targetIndex];
        targetMonsterType = targetArmy->m_creatureType;
    }
    switch (messageType) {
        case COMBAT_MESSAGE_COMMAND_DEFAULT:
            if ((currentArmy->m_stats.attributes & MONSTER_FLAGS_SHOOTER)
                && currentArmy->m_stats.shots == 0 && targetArmy)
                strcpy(gText, gCombatMessage[COMBAT_TEXT_NO_SHOTS]);
            else
                strcpy(gText, gCombatMessage[COMBAT_TEXT_NONE]);
            break;
        case COMBAT_MESSAGE_COMMAND_MOVE:
            sprintf(gText, gCombatMessage[COMBAT_TEXT_MOVE], gArmyNames[actingMonsterType]);
            break;
        case COMBAT_MESSAGE_COMMAND_FLY:
            sprintf(gText, gCombatMessage[COMBAT_TEXT_FLY], gArmyNames[actingMonsterType]);
            break;
        case COMBAT_MESSAGE_COMMAND_ATTACK:
            sprintf(gText, gCombatMessage[COMBAT_TEXT_ATTACK], gArmyNamesPlural[targetMonsterType]);
            break;
        case COMBAT_MESSAGE_COMMAND_SHOOT:
            sprintf(
                gText,
                gCombatMessage[COMBAT_TEXT_SHOOT],
                gArmyNamesPlural[targetMonsterType],
                currentArmy->m_stats.shots
            );
            break;
        case COMBAT_MESSAGE_COMMAND_OPTIONS:
            strcpy(gText, gCombatMessage[COMBAT_TEXT_GENERALS_OPTIONS]);
            break;
        case COMBAT_MESSAGE_COMMAND_OPPOSING_OPTIONS:
            strcpy(gText, gCombatMessage[COMBAT_TEXT_VIEW_OPPOSING_GENERAL]);
            break;
        case COMBAT_MESSAGE_COMMAND_VIEW_INFO:
            actingMonsterType =
                m_armies[m_currentSide][m_hexCells[m_selectedHex].m_occupantIndex].m_creatureType;
            if (actingMonsterType >= CREATURE_FIRST)
                sprintf(
                    gText,
                    gCombatMessage[COMBAT_TEXT_VIEW_INFO],
                    gArmyNames[actingMonsterType]
                );
            else
                sprintf(gText, "");
            break;
    }
    CombatMessage(gText, true);
}

// Marks every live stack for redraw; dead ones stay hidden (-1).
VA(0x00423a3a, 0x92)
void combatManager::ResetLimitCreature(void) {
    i32 j;
    H1_ENUM_LOCAL(CombatSide, i32) side;

    m_computeExtent = true;
    m_extendLimitDown = false;
    for (side = COMBAT_SIDE_FIRST; side < COMBAT_SIDE_COUNT; side++) {
        for (j = 0; j < ARMY_GROUP_SLOT_COUNT; j++) {
            m_limitCreatureCount[side][j] =
                (m_armies[side][j].m_stats.attributes & MONSTER_FLAGS_DEAD)
                    ? COMBAT_LIMIT_CREATURE_HIDDEN
                    : 0;
        }
    }
}

VA(0x00423acc, 0x19)
void combatManager::SetDrawRightToLeft(i8 rightToLeft) {
    m_drawRightToLeft = rightToLeft;
}

// Blits the rows from m_gridUpdateRow down (the first row also takes the
// 60-pixel top margin).
VA(0x00423ae5, 0xc6)
void combatManager::UpdateCombatArea(void) {
    i16 baseY;
    i16 height;

    if (!m_combatWindowOpen)
        return;
    baseY = m_gridUpdateRow * COMBAT_HEX_HEIGHT + COMBAT_FIELD_TOP;
    height = (COMBAT_GRID_ROWS - m_gridUpdateRow) * COMBAT_HEX_HEIGHT;
    if (!m_gridUpdateRow) {
        baseY = 0;
        height += COMBAT_FIELD_TOP;
    }
    if (height + baseY > COMBAT_VIEW_HEIGHT)
        height = COMBAT_VIEW_HEIGHT - baseY;
    gEnlargeScreenBlit = false;
    gWindowManager->UpdateScreenRegion(0, baseY, LOGICAL_SCREEN_WIDTH, height);
    gEnlargeScreenBlit = true;
    m_gridUpdateRow = COMBAT_GRID_ROWS;
}

// Draws the hex ground, the castle wall strip and the moat ends, then keeps
// the clean screen as the combat background.
VA(0x00423bab, 0x1c1)
void combatManager::DrawBackground(void) {
    i16 x;
    i16 y;
    i16 top;

    for (y = 0; y < COMBAT_GRID_ROWS; y++) {
        for (x = 0; x < COMBAT_GRID_COLUMNS; x++)
            m_hexCells[y * COMBAT_GRID_COLUMNS + x].DrawGround();
        if (m_castleSide[COMBAT_DEFENDER_SIDE])
            m_combatIcons[COMBAT_ICON_CASTLE]->DrawToBuffer(
                m_hexCells[y * COMBAT_GRID_COLUMNS + COMBAT_CASTLE_WALL_COLUMN].m_x,
                m_hexCells[y * COMBAT_GRID_COLUMNS + COMBAT_CASTLE_WALL_COLUMN].m_y,
                (y & 1) ? static_cast<i8>(5) : static_cast<i8>(6),
                ICON_DRAW_NORMAL,
                ICON_DRAW_OFFSET_FULL
            );
        top = y * COMBAT_HEX_HEIGHT + COMBAT_HEX_ORIGIN_Y;
        if (y == 0) {
            m_backgroundBitmap->DrawToBuffer(0, 0);
            if (m_castleSide[COMBAT_ATTACKER_SIDE] == 1) {
                m_combatIcons[COMBAT_ICON_CASTLE]
                    ->DrawToBuffer(0, top, 7, ICON_DRAW_FLIPPED, ICON_DRAW_OFFSET_FULL);
                m_combatIcons[COMBAT_ICON_CASTLE]
                    ->DrawToBuffer(0x6e, top, 7, ICON_DRAW_FLIPPED, ICON_DRAW_OFFSET_FULL);
            } else if (m_castleSide[COMBAT_DEFENDER_SIDE] == 1) {
                m_combatIcons[COMBAT_ICON_CASTLE]
                    ->DrawToBuffer(0x27f, top, 7, ICON_DRAW_NORMAL, ICON_DRAW_OFFSET_FULL);
                m_combatIcons[COMBAT_ICON_CASTLE]
                    ->DrawToBuffer(0x211, top, 7, ICON_DRAW_NORMAL, ICON_DRAW_OFFSET_FULL);
            }
        }
    }
    gWindowManager->m_screen
        ->CopyTo(m_backgroundBuffer, 0, 0, 0, 0, LOGICAL_SCREEN_WIDTH, COMBAT_VIEW_HEIGHT);
    m_backgroundDrawn = true;
}

// Redraws the battlefield: only the boxes around the stacks marked in
// m_limitCreatureCount when m_computeExtent is set, else the whole area.
// Rows draw obstacles, then occupants (right to left while m_drawRightToLeft is
// set), with the catapult (row 3) and the two heroes (rows 1 and 2).
#define armyRight selBoxRight // frame-slot spelling
VA(0x00423d6c, 0xca3)
void combatManager::DrawFrame(b8 updateScreen) {
    i16 col;
    b8 anyLimited;
    i32 armyRight;
    i32 armyTop;
    i32 i;
    i32 rearDelta;
    H1_ENUM_LOCAL(CombatSide, i32) side;
    i32 armyLeft;
    i32 armyBottom;
    i16 row;

    if (!m_combatWindowOpen)
        return;
    if (m_computeExtent) {
        gMaxExtentX = gMaxExtentY = 0;
        gMinExtentX = LOGICAL_SCREEN_WIDTH - 1;
        gMinExtentY = COMBAT_VIEW_HEIGHT - 1;
        anyLimited = false;
        for (side = COMBAT_SIDE_FIRST; side < COMBAT_SIDE_COUNT; side++) {
            for (i = 0; i < ARMY_GROUP_SLOT_COUNT; i++) {
                if (m_limitCreatureCount[side][i] > 0) {
                    anyLimited = true;
                    col = m_armies[side][i].m_hex % COMBAT_GRID_COLUMNS;
                    row = m_armies[side][i].m_hex / COMBAT_GRID_COLUMNS;
                    armyTop = row * COMBAT_HEX_HEIGHT;
                    armyBottom = (row + 2) * COMBAT_HEX_HEIGHT + 20;
                    if (m_extendLimitDown)
                        armyBottom += 60;
                    if (m_armies[side][i].m_facing == ARMY_FACING_LEFT) {
                        armyLeft = col * COMBAT_HEX_WIDTH - 110;
                        armyRight = (col + 1) * COMBAT_HEX_WIDTH + 70;
                    } else {
                        armyLeft = col * COMBAT_HEX_WIDTH - 70;
                        armyRight = (col + 1) * COMBAT_HEX_WIDTH + 110;
                    }
                    if (m_armies[side][i].m_effectAnimation == COMBAT_EFFECT_GOOD_LUCK
                        || m_armies[side][i].m_effectAnimation == COMBAT_EFFECT_BAD_LUCK
                        || m_armies[side][i].m_effectAnimation == COMBAT_EFFECT_GOOD_MORALE
                        || m_armies[side][i].m_effectAnimation == COMBAT_EFFECT_BAD_MORALE)
                        armyTop -= 100;
                    if (m_armies[side][i].m_creatureType == CREATURE_CAVALRY)
                        armyTop -= 60;
                    if (m_armies[side][i].m_creatureType == CREATURE_SPRITE
                        || m_armies[side][i].m_creatureType == CREATURE_ORC
                        || m_armies[side][i].m_creatureType == CREATURE_TROLL) {
                        if (m_armies[side][i].m_facing == ARMY_FACING_LEFT)
                            armyRight += 40;
                        else
                            armyLeft -= 40;
                    }
                    if (armyTop < gMinExtentY)
                        gMinExtentY = armyTop;
                    if (armyBottom > gMaxExtentY)
                        gMaxExtentY = armyBottom;
                    if (armyLeft < gMinExtentX)
                        gMinExtentX = armyLeft;
                    if (armyRight > gMaxExtentX)
                        gMaxExtentX = armyRight;
                    if (m_armies[side][i].m_stats.attributes & MONSTER_FLAGS_WIDE) {
                        rearDelta = side == COMBAT_DEFENDER_SIDE ? -1 : 1;
                        if (m_armies[side][i].m_facing == ARMY_FACING_LEFT) {
                            armyLeft = (col + rearDelta) * COMBAT_HEX_WIDTH - 110;
                            armyRight = (col + rearDelta + 1) * COMBAT_HEX_WIDTH + 70;
                        } else {
                            armyLeft = (col + rearDelta) * COMBAT_HEX_WIDTH - 70;
                            armyRight = (col + rearDelta + 1) * COMBAT_HEX_WIDTH + 110;
                        }
                    }
                    if (armyLeft < gMinExtentX)
                        gMinExtentX = armyLeft;
                    if (armyRight > gMaxExtentX)
                        gMaxExtentX = armyRight;
                }
            }
        }
        if (!anyLimited) {
            m_computeExtent = false;
            return;
        }
        if (gMinExtentX < 0)
            gMinExtentX = 0;
        if (gMinExtentY < 0)
            gMinExtentY = 0;
        if (gMaxExtentX > LOGICAL_SCREEN_WIDTH - 1)
            gMaxExtentX = LOGICAL_SCREEN_WIDTH - 1;
        if (gMaxExtentY > COMBAT_VIEW_HEIGHT - 1)
            gMaxExtentY = COMBAT_VIEW_HEIGHT - 1;
    }
    m_gridUpdateRow = 0;
    if (!gLimitToExtent) {
        if (m_backgroundDrawn) {
            if (m_computeExtent || m_redrawExtent)
                m_backgroundBuffer->CopyTo(
                    gWindowManager->m_screen,
                    gMinExtentX,
                    gMinExtentY,
                    gMinExtentX,
                    gMinExtentY,
                    gMaxExtentX - gMinExtentX + 1,
                    gMaxExtentY - gMinExtentY + 1
                );
            else
                m_backgroundBuffer->CopyTo(
                    gWindowManager->m_screen,
                    0,
                    0,
                    0,
                    0,
                    LOGICAL_SCREEN_WIDTH,
                    COMBAT_VIEW_HEIGHT
                );
        } else {
            DrawBackground();
        }
    }
    if (m_computeExtent) {
        gLimitToExtent = 1;
        gComputeExtent = 1;
    }
    if (!m_drawRightToLeft) {
        for (row = 0; row < COMBAT_GRID_ROWS; row++) {
            if (row == COMBAT_CATAPULT_ROW
                && m_catapultFrame[COMBAT_ATTACKER_SIDE] != COMBAT_CATAPULT_FRAME_NONE) {
                m_combatIcons[COMBAT_ICON_CATAPULT]->DrawToBuffer(
                    0x1b,
                    0x17b,
                    m_catapultFrame[COMBAT_ATTACKER_SIDE],
                    ICON_DRAW_NORMAL,
                    ICON_DRAW_OFFSET_FULL
                );
                m_combatIcons[COMBAT_ICON_CATAPULT]->DimToBuffer(
                    0x1b,
                    0x17b,
                    m_catapultFrame[COMBAT_ATTACKER_SIDE] == COMBAT_CATAPULT_RELEASE_FRAME ? 16
                                                                                           : 15,
                    ICON_DRAW_NORMAL,
                    ICON_DRAW_OFFSET_FULL
                );
            }
            for (col = 0; col < COMBAT_GRID_COLUMNS; col++)
                if (m_hexCells[row * COMBAT_GRID_COLUMNS + col].m_obstacleIndex
                    != COMBAT_OBSTACLE_NONE)
                    m_hexCells[row * COMBAT_GRID_COLUMNS + col].DrawObstacle();
            if (row == 0 && m_castleSide[COMBAT_DEFENDER_SIDE])
                m_combatIcons[COMBAT_ICON_KEEP]
                    ->DrawToBuffer(0x22d, 0, 0, ICON_DRAW_NORMAL, ICON_DRAW_OFFSET_FULL);
            for (col = 1; col <= COMBAT_GRID_LAST_COLUMN - 1; col++) {
                if (gLimitToExtent
                    && m_armies[m_currentSide][m_currentArmyIndex].m_hex
                           == row * COMBAT_GRID_COLUMNS + col)
                    gCurrArmyDrawn = true;
                m_hexCells[row * COMBAT_GRID_COLUMNS + col].DrawOccupant();
            }
            if (row == COMBAT_DEFENDER_HERO_ROW
                && m_heroClass[COMBAT_DEFENDER_SIDE] != COMBAT_HERO_CLASS_NONE) {
                m_combatIcons[COMBAT_ICON_TENT]->DrawToBuffer(
                    0x27f,
                    0xa9,
                    m_heroClass[COMBAT_DEFENDER_SIDE],
                    ICON_DRAW_FLIPPED,
                    ICON_DRAW_OFFSET_FULL
                );
                m_combatIcons[COMBAT_ICON_TENT]->DrawToBuffer(
                    0x250,
                    0xeb,
                    H1_ENUM_ENCODE(
                        PlayerColor,
                        gGame->m_players[m_playerId[COMBAT_DEFENDER_SIDE]].Color()
                    ) + 4,
                    ICON_DRAW_FLIPPED,
                    ICON_DRAW_OFFSET_FULL
                );
                m_combatIcons[COMBAT_ICON_TENT]->DrawToBuffer(
                    0x238,
                    0xfd,
                    m_heroClass[COMBAT_DEFENDER_SIDE] + 8,
                    ICON_DRAW_NORMAL,
                    ICON_DRAW_OFFSET_FULL
                );
            }
            if (row == COMBAT_ATTACKER_HERO_ROW
                && m_heroClass[COMBAT_ATTACKER_SIDE] != COMBAT_HERO_CLASS_NONE) {
                m_combatIcons[COMBAT_ICON_TENT]->DrawToBuffer(
                    0,
                    0x59,
                    m_heroClass[COMBAT_ATTACKER_SIDE],
                    ICON_DRAW_NORMAL,
                    ICON_DRAW_OFFSET_FULL
                );
                m_combatIcons[COMBAT_ICON_TENT]->DrawToBuffer(
                    0x2f,
                    0x9b,
                    H1_ENUM_ENCODE(
                        PlayerColor,
                        gGame->m_players[m_playerId[COMBAT_ATTACKER_SIDE]].Color()
                    ) + 4,
                    ICON_DRAW_NORMAL,
                    ICON_DRAW_OFFSET_FULL
                );
                m_combatIcons[COMBAT_ICON_TENT]->DrawToBuffer(
                    0x36,
                    0xad,
                    m_heroClass[COMBAT_ATTACKER_SIDE] + 8,
                    ICON_DRAW_NORMAL,
                    ICON_DRAW_OFFSET_FULL
                );
            }
        }
    } else {
        for (row = 0; row < COMBAT_GRID_ROWS; row++) {
            if (row == 0 && m_castleSide[COMBAT_DEFENDER_SIDE])
                m_combatIcons[COMBAT_ICON_KEEP]
                    ->DrawToBuffer(0x22d, 0, 0, ICON_DRAW_NORMAL, ICON_DRAW_OFFSET_FULL);
            if (row == COMBAT_CATAPULT_ROW
                && m_catapultFrame[COMBAT_ATTACKER_SIDE] != COMBAT_CATAPULT_FRAME_NONE) {
                m_combatIcons[COMBAT_ICON_CATAPULT]->DrawToBuffer(
                    0x1b,
                    0x17b,
                    m_catapultFrame[COMBAT_ATTACKER_SIDE],
                    ICON_DRAW_NORMAL,
                    ICON_DRAW_OFFSET_FULL
                );
                m_combatIcons[COMBAT_ICON_CATAPULT]->DimToBuffer(
                    0x1b,
                    0x17b,
                    m_catapultFrame[COMBAT_ATTACKER_SIDE] == COMBAT_CATAPULT_RELEASE_FRAME ? 16
                                                                                           : 15,
                    ICON_DRAW_NORMAL,
                    ICON_DRAW_OFFSET_FULL
                );
            }
            for (col = 0; col < COMBAT_GRID_COLUMNS; col++)
                if (m_hexCells[row * COMBAT_GRID_COLUMNS + col].m_obstacleIndex
                    != COMBAT_OBSTACLE_NONE)
                    m_hexCells[row * COMBAT_GRID_COLUMNS + col].DrawObstacle();
            for (col = COMBAT_GRID_LAST_INNER_COLUMN; col >= COMBAT_GRID_FIRST_INNER_COLUMN;
                 col--) {
                if (gLimitToExtent
                    && m_armies[m_currentSide][m_currentArmyIndex].m_hex
                           == row * COMBAT_GRID_COLUMNS + col)
                    gCurrArmyDrawn = true;
                m_hexCells[row * COMBAT_GRID_COLUMNS + col].DrawOccupant();
            }
            if (row == COMBAT_ATTACKER_HERO_ROW
                && m_heroClass[COMBAT_ATTACKER_SIDE] != COMBAT_HERO_CLASS_NONE) {
                m_combatIcons[COMBAT_ICON_TENT]->DrawToBuffer(
                    0,
                    0x59,
                    m_heroClass[COMBAT_ATTACKER_SIDE],
                    ICON_DRAW_NORMAL,
                    ICON_DRAW_OFFSET_FULL
                );
                m_combatIcons[COMBAT_ICON_TENT]->DrawToBuffer(
                    0x2f,
                    0x9b,
                    H1_ENUM_ENCODE(
                        PlayerColor,
                        gGame->m_players[m_playerId[COMBAT_ATTACKER_SIDE]].Color()
                    ) + 4,
                    ICON_DRAW_NORMAL,
                    ICON_DRAW_OFFSET_FULL
                );
                m_combatIcons[COMBAT_ICON_TENT]->DrawToBuffer(
                    0x36,
                    0xad,
                    m_heroClass[COMBAT_ATTACKER_SIDE] + 8,
                    ICON_DRAW_NORMAL,
                    ICON_DRAW_OFFSET_FULL
                );
            }
            if (row == COMBAT_DEFENDER_HERO_ROW
                && m_heroClass[COMBAT_DEFENDER_SIDE] != COMBAT_HERO_CLASS_NONE) {
                m_combatIcons[COMBAT_ICON_TENT]->DrawToBuffer(
                    0x27f,
                    0xa9,
                    m_heroClass[COMBAT_DEFENDER_SIDE],
                    ICON_DRAW_FLIPPED,
                    ICON_DRAW_OFFSET_FULL
                );
                m_combatIcons[COMBAT_ICON_TENT]->DrawToBuffer(
                    0x250,
                    0xeb,
                    H1_ENUM_ENCODE(
                        PlayerColor,
                        gGame->m_players[m_playerId[COMBAT_DEFENDER_SIDE]].Color()
                    ) + 4,
                    ICON_DRAW_FLIPPED,
                    ICON_DRAW_OFFSET_FULL
                );
                m_combatIcons[COMBAT_ICON_TENT]->DrawToBuffer(
                    0x238,
                    0xfd,
                    m_heroClass[COMBAT_DEFENDER_SIDE] + 8,
                    ICON_DRAW_NORMAL,
                    ICON_DRAW_OFFSET_FULL
                );
            }
        }
    }
    DrawCombatBorder();
    if (m_computeExtent || m_redrawExtent) {
        m_computeExtent = false;
        m_redrawExtent = false;
        gLimitToExtent = 0;
        gComputeExtent = 0;
        gFullCombatScreenDrawn = false;
        DelayTil(&gTimers[COMBAT_FRAME_TIMER_SLOT]);
        gTimers[COMBAT_FRAME_TIMER_SLOT] = KBTickCount() + 75;
        if (updateScreen == true) {
            if (gMaxExtentY > COMBAT_VIEW_HEIGHT)
                gMaxExtentY = COMBAT_VIEW_HEIGHT;
            gEnlargeScreenBlit = false;
            UPDATE_INCLUSIVE_REGION(gMinExtentX, gMinExtentY, gMaxExtentX, gMaxExtentY);
            gEnlargeScreenBlit = true;
            m_gridUpdateRow = COMBAT_GRID_ROWS;
        }
    } else if (updateScreen == true) {
        gFullCombatScreenDrawn = true;
        DelayTil(&gTimers[COMBAT_FRAME_TIMER_SLOT]);
        gTimers[COMBAT_FRAME_TIMER_SLOT] = KBTickCount() + 75;
        UpdateCombatArea();
    }
}
#undef armyRight

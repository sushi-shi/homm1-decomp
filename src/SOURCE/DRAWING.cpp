// Combat screen drawing; Buka 2.1 SOURCE/DRAWING correspondence. HoMM1
// keeps one text line in the combat window and redraws the battlefield
// from the grid row UpdateGrid records.

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
VA(0x004387b0, 0x5f)
void combatManager::UpdateGrid(i16 hex, i16) {
    i16 row;

    row = hex / COMBAT_GRID_COLUMNS - 1;
    if (row < 0)
        row = 0;
    if (m_gridUpdateRow > row)
        m_gridUpdateRow = row;
}

// @dead-code
// Zero-ref: no incoming call, jump or relocated reference in retail.
VA(0x0043880f, 0x5a)
void combatManager::UpdateGridForMove(i16 hex, i8 direction, i16 attributes) {
    if (direction == COMBAT_DIRECTION_NORTHEAST || direction == COMBAT_DIRECTION_NORTHWEST)
        UpdateGrid(hex - COMBAT_GRID_COLUMNS, attributes);
    else
        UpdateGrid(hex, attributes);
}

// clang-format off
// cmbtwin.bin's status line: CombatMessage sets the text widget (id 12),
// redraws widgets 2..12 of the text bar and blits the bar's screen rectangle.
H1_ENUM_CONST_BEGIN(CombatStatusLineConstant)
    COMBAT_STATUS_FIRST_CONTROL = 2,
    COMBAT_STATUS_TEXT_CONTROL = 0xc,
    COMBAT_STATUS_X = 0x30,
    COMBAT_STATUS_Y = 0x1cc,
    COMBAT_STATUS_WIDTH = 0x21f,
    COMBAT_STATUS_HEIGHT = 0x14
H1_ENUM_CONST_END(CombatStatusLineConstant)

// Sets the combat window's text line and redraws it outside the extent
// bookkeeping.
VA(0x00438869, 0xb5)
// clang-format on
void combatManager::CombatMessage(char* text, i32 updateScreen) {
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
        gpWindowManager->UpdateScreenRegion(
            COMBAT_STATUS_X,
            COMBAT_STATUS_Y,
            COMBAT_STATUS_WIDTH,
            COMBAT_STATUS_HEIGHT
        );
    gComputeExtent = oldCompute;
    gLimitToExtent = prevLimit;
}

// The help line for the current mouse command.
VA(0x0043891e, 0x2f3)
void combatManager::CombatMessage(H1_ENUM_PARAM(CombatMessageCommand, i16) messageType) {
    army* target;
    army* currentArmy;
    i16 targetMonster;
    i16 actingType;

    currentArmy = &m_armies[m_currentSide][m_currentArmyIndex];
    actingType = currentArmy->m_creatureType;
    target = NULL;
    targetMonster = 0;
    if (currentArmy->m_targetSide >= 0 && currentArmy->m_targetIndex >= 0) {
        target = &m_armies[currentArmy->m_targetSide][currentArmy->m_targetIndex];
        targetMonster = target->m_creatureType;
    }
    switch (messageType) {
        case COMBAT_MESSAGE_COMMAND_DEFAULT:
            if ((currentArmy->m_stats.attributes & MONSTER_FLAGS_SHOOTER)
                && currentArmy->m_stats.shots == 0 && target)
                strcpy(gText, gCombatMessage[COMBAT_TEXT_NO_SHOTS]);
            else
                strcpy(gText, gCombatMessage[COMBAT_TEXT_NONE]);
            break;
        case COMBAT_MESSAGE_COMMAND_MOVE:
            sprintf(gText, gCombatMessage[COMBAT_TEXT_MOVE], gArmyNames[actingType]);
            break;
        case COMBAT_MESSAGE_COMMAND_FLY:
            sprintf(gText, gCombatMessage[COMBAT_TEXT_FLY], gArmyNames[actingType]);
            break;
        case COMBAT_MESSAGE_COMMAND_ATTACK:
            sprintf(gText, gCombatMessage[COMBAT_TEXT_ATTACK], gArmyNames[targetMonster]);
            break;
        case COMBAT_MESSAGE_COMMAND_SHOOT:
            sprintf(
                gText,
                gCombatMessage[COMBAT_TEXT_SHOOT],
                gArmyNames[targetMonster],
                currentArmy->m_stats.shots,
                currentArmy->m_stats.shots > 1 ? "s" : ""
            );
            break;
        case COMBAT_MESSAGE_COMMAND_OPTIONS:
            strcpy(gText, gCombatMessage[COMBAT_TEXT_GENERALS_OPTIONS]);
            break;
        case COMBAT_MESSAGE_COMMAND_OPPOSING_OPTIONS:
            strcpy(gText, gCombatMessage[COMBAT_TEXT_VIEW_OPPOSING_GENERAL]);
            break;
        case COMBAT_MESSAGE_COMMAND_VIEW_INFO:
            actingType =
                m_armies[m_currentSide][m_hexCells[m_selectedHex].m_occupantIndex].m_creatureType;
            if (actingType >= 0)
                sprintf(gText, gCombatMessage[COMBAT_TEXT_VIEW_INFO], gArmyNames[actingType]);
            else
                sprintf(gText, "");
            break;
    }
    CombatMessage(gText, 1);
}

// Marks every live stack for redraw; dead ones stay hidden (-1).
VA(0x00438c11, 0xd4)
void combatManager::ResetLimitCreature(void) {
    i32 j;
    i32 side;

    m_computeExtent = 1;
    m_extendLimitDown = 0;
    for (side = 0; side < COMBAT_SIDE_COUNT; side++) {
        for (j = 0; j < ARMY_GROUP_SLOT_COUNT; j++) {
            if (m_armies[side][j].m_stats.attributes & MONSTER_FLAGS_DEAD)
                m_limitCreatureCount[side][j] = COMBAT_LIMIT_CREATURE_HIDDEN;
            else
                m_limitCreatureCount[side][j] = 0;
        }
    }
}

VA(0x00438ce5, 0x24)
void combatManager::SetGridMode(i8 mode) {
    m_gridMode = mode;
}

// Blits the rows from m_gridUpdateRow down (the first row also takes the
// 60-pixel top margin).
VA(0x00438d09, 0xde)
void combatManager::UpdateCombatArea(void) {
    i16 y;
    i16 height;

    if (!m_combatWindowOpen)
        return;
    y = m_gridUpdateRow * COMBAT_HEX_HEIGHT + COMBAT_FIELD_TOP;
    height = (COMBAT_GRID_ROWS - m_gridUpdateRow) * COMBAT_HEX_HEIGHT;
    if (!m_gridUpdateRow) {
        y = 0;
        height += COMBAT_FIELD_TOP;
    }
    if (y + height > COMBAT_VIEW_HEIGHT)
        height = COMBAT_VIEW_HEIGHT - y;
    gEnlargeScreenBlit = 0;
    gpWindowManager->UpdateScreenRegion(0, y, LOGICAL_SCREEN_WIDTH, height);
    gEnlargeScreenBlit = 1;
    m_gridUpdateRow = COMBAT_GRID_ROWS;
}

// Draws the hex ground, the castle wall strip and the moat ends, then keeps
// the clean screen as the combat background.
VA(0x00438de7, 0x1d4)
void combatManager::DrawBackground(void) {
    i16 x;
    i16 y;
    i16 wallY;

    for (y = 0; y < COMBAT_GRID_ROWS; y++) {
        for (x = 0; x < COMBAT_GRID_COLUMNS; x++)
            m_hexCells[y * COMBAT_GRID_COLUMNS + x].DrawGround();
        if (m_castleSide[COMBAT_DEFENDER_SIDE])
            m_combatIcons[COMBAT_ICON_CASTLE]->DrawToBuffer(
                m_hexCells[y * COMBAT_GRID_COLUMNS + COMBAT_CASTLE_WALL_COLUMN].m_x,
                m_hexCells[y * COMBAT_GRID_COLUMNS + COMBAT_CASTLE_WALL_COLUMN].m_y,
                static_cast<i8>((y & 1) ? 5 : 6),
                ICON_DRAW_NORMAL,
                ICON_DRAW_OFFSET_FULL
            );
        wallY = y * COMBAT_HEX_HEIGHT + COMBAT_HEX_ORIGIN_Y;
        if (y == 0) {
            m_backgroundBitmap->DrawToBuffer(0, 0);
            if (m_castleSide[COMBAT_ATTACKER_SIDE] == 1) {
                m_combatIcons[COMBAT_ICON_CASTLE]
                    ->DrawToBuffer(0, wallY, 7, ICON_DRAW_FLIPPED, ICON_DRAW_OFFSET_FULL);
                m_combatIcons[COMBAT_ICON_CASTLE]
                    ->DrawToBuffer(0x6e, wallY, 7, ICON_DRAW_FLIPPED, ICON_DRAW_OFFSET_FULL);
            } else if (m_castleSide[COMBAT_DEFENDER_SIDE] == 1) {
                m_combatIcons[COMBAT_ICON_CASTLE]
                    ->DrawToBuffer(0x27f, wallY, 7, ICON_DRAW_NORMAL, ICON_DRAW_OFFSET_FULL);
                m_combatIcons[COMBAT_ICON_CASTLE]
                    ->DrawToBuffer(0x211, wallY, 7, ICON_DRAW_NORMAL, ICON_DRAW_OFFSET_FULL);
            }
        }
    }
    gpWindowManager->m_screen
        ->CopyTo(m_backgroundBuffer, 0, 0, 0, 0, LOGICAL_SCREEN_WIDTH, COMBAT_VIEW_HEIGHT);
    m_backgroundDrawn = 1;
}

// Redraws the battlefield: only the boxes around the stacks marked in
// m_limitCreatureCount when m_computeExtent is set, else the whole area.
// Rows draw obstacles, then occupants (right to left while m_gridMode is
// set), with the catapult (row 3) and the two heroes (rows 1 and 2).
VA(0x00438fbb, 0xe27)
void combatManager::DrawFrame(i8 updateScreen) {
    i16 hexCol;
    i32 boxRight;
    i8 drawn;
    i32 side;
    i32 i;
    i32 boxBottom;
    i32 boxTop;
    i32 boxLeft;
    i32 sideDelta;
    i16 row;

    if (!m_combatWindowOpen)
        return;
    if (m_computeExtent) {
        giMaxExtentX = giMaxExtentY = 0;
        giMinExtentX = LOGICAL_SCREEN_WIDTH - 1;
        giMinExtentY = COMBAT_VIEW_HEIGHT - 1;
        drawn = 0;
        for (side = 0; side < COMBAT_SIDE_COUNT; side++) {
            for (i = 0; i < ARMY_GROUP_SLOT_COUNT; i++) {
                if (m_limitCreatureCount[side][i] > 0) {
                    drawn = 1;
                    hexCol = m_armies[side][i].m_hex % COMBAT_GRID_COLUMNS;
                    row = m_armies[side][i].m_hex / COMBAT_GRID_COLUMNS;
                    boxTop = row * COMBAT_HEX_HEIGHT;
                    boxBottom = (row + 2) * COMBAT_HEX_HEIGHT + 20;
                    if (m_extendLimitDown)
                        boxBottom += 60;
                    if (m_armies[side][i].m_facing == ARMY_FACING_LEFT) {
                        boxLeft = hexCol * COMBAT_HEX_WIDTH - 110;
                        boxRight = (hexCol + 1) * COMBAT_HEX_WIDTH + 70;
                    } else {
                        boxLeft = hexCol * COMBAT_HEX_WIDTH - 70;
                        boxRight = (hexCol + 1) * COMBAT_HEX_WIDTH + 110;
                    }
                    if (m_armies[side][i].m_effectAnimation == COMBAT_EFFECT_GOOD_LUCK
                        || m_armies[side][i].m_effectAnimation == COMBAT_EFFECT_BAD_LUCK
                        || m_armies[side][i].m_effectAnimation == COMBAT_EFFECT_GOOD_MORALE
                        || m_armies[side][i].m_effectAnimation == COMBAT_EFFECT_BAD_MORALE)
                        boxTop -= 100;
                    if (m_armies[side][i].m_creatureType == CREATURE_CAVALRY)
                        boxTop -= 60;
                    if (m_armies[side][i].m_creatureType == CREATURE_SPRITE
                        || m_armies[side][i].m_creatureType == CREATURE_ORC
                        || m_armies[side][i].m_creatureType == CREATURE_TROLL) {
                        if (m_armies[side][i].m_facing == ARMY_FACING_LEFT)
                            boxRight += 40;
                        else
                            boxLeft -= 40;
                    }
                    if (boxTop < giMinExtentY)
                        giMinExtentY = boxTop;
                    if (giMaxExtentY < boxBottom)
                        giMaxExtentY = boxBottom;
                    if (boxLeft < giMinExtentX)
                        giMinExtentX = boxLeft;
                    if (boxRight > giMaxExtentX)
                        giMaxExtentX = boxRight;
                    if (m_armies[side][i].m_stats.attributes & MONSTER_FLAGS_WIDE) {
                        if (side == COMBAT_DEFENDER_SIDE)
                            sideDelta = -1;
                        else
                            sideDelta = 1;
                        if (m_armies[side][i].m_facing == ARMY_FACING_LEFT) {
                            boxLeft = (hexCol + sideDelta) * COMBAT_HEX_WIDTH - 110;
                            boxRight = (hexCol + sideDelta + 1) * COMBAT_HEX_WIDTH + 70;
                        } else {
                            boxLeft = (hexCol + sideDelta) * COMBAT_HEX_WIDTH - 70;
                            boxRight = (hexCol + sideDelta + 1) * COMBAT_HEX_WIDTH + 110;
                        }
                    }
                    if (boxLeft < giMinExtentX)
                        giMinExtentX = boxLeft;
                    if (boxRight > giMaxExtentX)
                        giMaxExtentX = boxRight;
                }
            }
        }
        if (!drawn) {
            m_computeExtent = 0;
            return;
        }
        if (giMinExtentX < 0)
            giMinExtentX = 0;
        if (giMinExtentY < 0)
            giMinExtentY = 0;
        if (giMaxExtentX > LOGICAL_SCREEN_WIDTH - 1)
            giMaxExtentX = LOGICAL_SCREEN_WIDTH - 1;
        if (giMaxExtentY > COMBAT_VIEW_HEIGHT - 1)
            giMaxExtentY = COMBAT_VIEW_HEIGHT - 1;
    }
    m_gridUpdateRow = 0;
    if (!gLimitToExtent) {
        if (m_backgroundDrawn) {
            if (m_computeExtent || m_redrawExtent)
                m_backgroundBuffer->CopyTo(
                    gpWindowManager->m_screen,
                    giMinExtentX,
                    giMinExtentY,
                    giMinExtentX,
                    giMinExtentY,
                    giMaxExtentX - giMinExtentX + 1,
                    giMaxExtentY - giMinExtentY + 1
                );
            else
                m_backgroundBuffer->CopyTo(
                    gpWindowManager->m_screen,
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
    if (!m_gridMode) {
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
                    m_catapultFrame[COMBAT_ATTACKER_SIDE] == 7 ? 16 : 15,
                    ICON_DRAW_NORMAL,
                    ICON_DRAW_OFFSET_FULL
                );
            }
            for (hexCol = 0; hexCol < COMBAT_GRID_COLUMNS; hexCol++)
                if (m_hexCells[row * COMBAT_GRID_COLUMNS + hexCol].m_obstacleIndex
                    != COMBAT_OBSTACLE_NONE)
                    m_hexCells[row * COMBAT_GRID_COLUMNS + hexCol].DrawObstacle();
            if (row == 0 && m_castleSide[COMBAT_DEFENDER_SIDE])
                m_combatIcons[COMBAT_ICON_KEEP]
                    ->DrawToBuffer(0x22d, 0, 0, ICON_DRAW_NORMAL, ICON_DRAW_OFFSET_FULL);
            for (hexCol = 1; hexCol <= COMBAT_GRID_LAST_COLUMN - 1; hexCol++) {
                if (gLimitToExtent
                    && m_armies[m_currentSide][m_currentArmyIndex].m_hex
                           == row * COMBAT_GRID_COLUMNS + hexCol)
                    gCurrArmyDrawn = 1;
                m_hexCells[row * COMBAT_GRID_COLUMNS + hexCol].DrawOccupant();
            }
            if (row == COMBAT_DEFENDER_HERO_ROW
                && m_heroType[COMBAT_DEFENDER_SIDE] != COMBAT_HERO_TYPE_NONE) {
                m_combatIcons[COMBAT_ICON_TENT]->DrawToBuffer(
                    0x27f,
                    0xa9,
                    m_heroType[COMBAT_DEFENDER_SIDE],
                    ICON_DRAW_FLIPPED,
                    ICON_DRAW_OFFSET_FULL
                );
                m_combatIcons[COMBAT_ICON_TENT]->DrawToBuffer(
                    0x250,
                    0xeb,
                    gpGame->m_players[m_playerId[COMBAT_DEFENDER_SIDE]].Color() + 4,
                    ICON_DRAW_FLIPPED,
                    ICON_DRAW_OFFSET_FULL
                );
                m_combatIcons[COMBAT_ICON_TENT]->DrawToBuffer(
                    0x238,
                    0xfd,
                    m_heroType[COMBAT_DEFENDER_SIDE] + 8,
                    ICON_DRAW_NORMAL,
                    ICON_DRAW_OFFSET_FULL
                );
            }
            if (row == COMBAT_ATTACKER_HERO_ROW
                && m_heroType[COMBAT_ATTACKER_SIDE] != COMBAT_HERO_TYPE_NONE) {
                m_combatIcons[COMBAT_ICON_TENT]->DrawToBuffer(
                    0,
                    0x59,
                    m_heroType[COMBAT_ATTACKER_SIDE],
                    ICON_DRAW_NORMAL,
                    ICON_DRAW_OFFSET_FULL
                );
                m_combatIcons[COMBAT_ICON_TENT]->DrawToBuffer(
                    0x2f,
                    0x9b,
                    gpGame->m_players[m_playerId[COMBAT_ATTACKER_SIDE]].Color() + 4,
                    ICON_DRAW_NORMAL,
                    ICON_DRAW_OFFSET_FULL
                );
                m_combatIcons[COMBAT_ICON_TENT]->DrawToBuffer(
                    0x36,
                    0xad,
                    m_heroType[COMBAT_ATTACKER_SIDE] + 8,
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
                    m_catapultFrame[COMBAT_ATTACKER_SIDE] == 7 ? 16 : 15,
                    ICON_DRAW_NORMAL,
                    ICON_DRAW_OFFSET_FULL
                );
            }
            for (hexCol = 0; hexCol < COMBAT_GRID_COLUMNS; hexCol++)
                if (m_hexCells[row * COMBAT_GRID_COLUMNS + hexCol].m_obstacleIndex
                    != COMBAT_OBSTACLE_NONE)
                    m_hexCells[row * COMBAT_GRID_COLUMNS + hexCol].DrawObstacle();
            for (hexCol = COMBAT_GRID_LAST_COLUMN - 1; hexCol >= 1; hexCol--) {
                if (gLimitToExtent
                    && m_armies[m_currentSide][m_currentArmyIndex].m_hex
                           == row * COMBAT_GRID_COLUMNS + hexCol)
                    gCurrArmyDrawn = 1;
                m_hexCells[row * COMBAT_GRID_COLUMNS + hexCol].DrawOccupant();
            }
            if (row == COMBAT_ATTACKER_HERO_ROW
                && m_heroType[COMBAT_ATTACKER_SIDE] != COMBAT_HERO_TYPE_NONE) {
                m_combatIcons[COMBAT_ICON_TENT]->DrawToBuffer(
                    0,
                    0x59,
                    m_heroType[COMBAT_ATTACKER_SIDE],
                    ICON_DRAW_NORMAL,
                    ICON_DRAW_OFFSET_FULL
                );
                m_combatIcons[COMBAT_ICON_TENT]->DrawToBuffer(
                    0x2f,
                    0x9b,
                    gpGame->m_players[m_playerId[COMBAT_ATTACKER_SIDE]].Color() + 4,
                    ICON_DRAW_NORMAL,
                    ICON_DRAW_OFFSET_FULL
                );
                m_combatIcons[COMBAT_ICON_TENT]->DrawToBuffer(
                    0x36,
                    0xad,
                    m_heroType[COMBAT_ATTACKER_SIDE] + 8,
                    ICON_DRAW_NORMAL,
                    ICON_DRAW_OFFSET_FULL
                );
            }
            if (row == COMBAT_DEFENDER_HERO_ROW
                && m_heroType[COMBAT_DEFENDER_SIDE] != COMBAT_HERO_TYPE_NONE) {
                m_combatIcons[COMBAT_ICON_TENT]->DrawToBuffer(
                    0x27f,
                    0xa9,
                    m_heroType[COMBAT_DEFENDER_SIDE],
                    ICON_DRAW_FLIPPED,
                    ICON_DRAW_OFFSET_FULL
                );
                m_combatIcons[COMBAT_ICON_TENT]->DrawToBuffer(
                    0x250,
                    0xeb,
                    gpGame->m_players[m_playerId[COMBAT_DEFENDER_SIDE]].Color() + 4,
                    ICON_DRAW_FLIPPED,
                    ICON_DRAW_OFFSET_FULL
                );
                m_combatIcons[COMBAT_ICON_TENT]->DrawToBuffer(
                    0x238,
                    0xfd,
                    m_heroType[COMBAT_DEFENDER_SIDE] + 8,
                    ICON_DRAW_NORMAL,
                    ICON_DRAW_OFFSET_FULL
                );
            }
        }
    }
    DrawCombatBorder();
    if (m_computeExtent || m_redrawExtent) {
        m_computeExtent = 0;
        m_redrawExtent = 0;
        gLimitToExtent = 0;
        gComputeExtent = 0;
        gFullCombatScreenDrawn = 0;
        DelayTil(glTimers);
        glTimers[COMBAT_FRAME_TIMER_SLOT] = KBTickCount() + 75;
        if (updateScreen == 1) {
            if (giMaxExtentY > COMBAT_VIEW_HEIGHT)
                giMaxExtentY = COMBAT_VIEW_HEIGHT;
            gEnlargeScreenBlit = 0;
            gpWindowManager->UpdateScreenRegion(
                giMinExtentX,
                giMinExtentY,
                giMaxExtentX - giMinExtentX + 1,
                giMaxExtentY - giMinExtentY + 1
            );
            gEnlargeScreenBlit = 1;
            m_gridUpdateRow = COMBAT_GRID_ROWS;
        }
    } else if (updateScreen == 1) {
        gFullCombatScreenDrawn = 1;
        DelayTil(glTimers);
        glTimers[COMBAT_FRAME_TIMER_SLOT] = KBTickCount() + 75;
        UpdateCombatArea();
    }
}

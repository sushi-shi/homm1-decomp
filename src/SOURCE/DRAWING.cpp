// Combat screen drawing; Buka 2.1 SOURCE/DRAWING correspondence. HoMM1
// keeps one text line in the combat window and redraws the battlefield
// from the grid row UpdateGrid records.

#include <match.h>

#include <H1/All.h>
#include <H1/KB.h>
#include <SOURCE/combatTypes.h>
#include <SOURCE/NOOPT.h>
#include <SOURCE/wingraph.h>
#include <SOURCE/X_GLOBAL.h>

#include <stdio.h>
#include <string.h>

// Lowers the first grid row that needs redrawing to the one above the hex.
VA(0x004709f0, 0x5f)
void combatManager::UpdateGrid(short hex, int) {
    short row;

    row = hex / COMBAT_GRID_COLUMNS - 1;
    if (row < 0)
        row = 0;
    if (m_gridUpdateRow > row)
        m_gridUpdateRow = row;
}

// @dead-code
// Zero-ref: no incoming call, jump or relocated reference in retail.
VA(0x00470a4f, 0x5a)
void combatManager::UpdateGridForMove(short hex, signed char direction, int attributes) {
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
// clang-format on

// Sets the combat window's text line and redraws it outside the extent
// bookkeeping.
VA(0x00470aa9, 0xb5)
void combatManager::CombatMessage(char* text, int updateScreen) {
    int oldCompute;
    tag_message message;
    int prevLimit;

    message.type = MESSAGE_WIDGET;
    message.command = WIDGET_COMMAND_SET_TEXT;
    message.id = COMBAT_STATUS_TEXT_CONTROL;
    message.text = text;
    m_combatWindow->BroadcastMessage(message);
    oldCompute = gbComputeExtent;
    prevLimit = gbLimitToExtent;
    gbComputeExtent = gbLimitToExtent = 0;
    m_combatWindow->DrawWindow(0, COMBAT_STATUS_FIRST_CONTROL, COMBAT_STATUS_TEXT_CONTROL);
    SaveCombatBorder();
    if (updateScreen)
        gpWindowManager->UpdateScreenRegion(COMBAT_STATUS_X, COMBAT_STATUS_Y, COMBAT_STATUS_WIDTH, COMBAT_STATUS_HEIGHT);
    gbComputeExtent = oldCompute;
    gbLimitToExtent = prevLimit;
}

// The help line for the current mouse command.
VA(0x00470b5e, 0x2f3)
void combatManager::CombatMessage(H1_ENUM_PARAM(CombatMessageCommand, short) messageType) {
    army* currentArmy;
    short targetMonster;
    army* target;
    short actingType;

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
            if ((currentArmy->m_stats.attributes & MONSTER_FLAGS_SHOOTER) && currentArmy->m_stats.shots == 0 && target)
                strcpy(gText, cCombatMessage[COMBAT_TEXT_NO_SHOTS]);
            else
                strcpy(gText, cCombatMessage[COMBAT_TEXT_NONE]);
            break;
        case COMBAT_MESSAGE_COMMAND_MOVE:
            sprintf(gText, cCombatMessage[COMBAT_TEXT_MOVE], gArmyNames[actingType]);
            break;
        case COMBAT_MESSAGE_COMMAND_FLY:
            sprintf(gText, cCombatMessage[COMBAT_TEXT_FLY], gArmyNames[actingType]);
            break;
        case COMBAT_MESSAGE_COMMAND_ATTACK:
            sprintf(gText, cCombatMessage[COMBAT_TEXT_ATTACK], gArmyNames[targetMonster]);
            break;
        case COMBAT_MESSAGE_COMMAND_SHOOT:
            sprintf(gText, cCombatMessage[COMBAT_TEXT_SHOOT], gArmyNames[targetMonster], currentArmy->m_stats.shots,
                    currentArmy->m_stats.shots > 1 ? "s" : "");
            break;
        case COMBAT_MESSAGE_COMMAND_OPTIONS:
            strcpy(gText, cCombatMessage[COMBAT_TEXT_GENERALS_OPTIONS]);
            break;
        case COMBAT_MESSAGE_COMMAND_OPPOSING_OPTIONS:
            strcpy(gText, cCombatMessage[COMBAT_TEXT_VIEW_OPPOSING_GENERAL]);
            break;
        case COMBAT_MESSAGE_COMMAND_VIEW_INFO:
            actingType = m_armies[m_currentSide][m_hexCells[m_selectedHex].m_occupantIndex].m_creatureType;
            if (actingType >= 0)
                sprintf(gText, cCombatMessage[COMBAT_TEXT_VIEW_INFO], gArmyNames[actingType]);
            else
                sprintf(gText, "");
            break;
    }
    CombatMessage(gText, 1);
}

// Marks every live stack for redraw; dead ones stay hidden (-1).
VA(0x00470e51, 0xd4)
void combatManager::ResetLimitCreature(void) {
    int j;
    int side;

    m_computeExtent = 1;
    m_extendLimitDown = 0;
    for (side = 0; side < COMBAT_SIDE_COUNT; side++) {
        for (j = 0; j < ARMY_GROUP_SLOT_COUNT; j++) {
            if (m_armies[side][j].m_stats.attributes & MONSTER_FLAGS_DEAD)
                m_limitCreatureCount[side][j] = -1;
            else
                m_limitCreatureCount[side][j] = 0;
        }
    }
}

VA(0x00470f25, 0x24)
void combatManager::SetGridMode(signed char mode) {
    m_gridMode = mode;
}

// Blits the rows from m_gridUpdateRow down (the first row also takes the
// 60-pixel top margin).
VA(0x00470f49, 0xde)
void combatManager::UpdateCombatArea(void) {
    short y;
    short height;

    if (!m_combatWindowOpen)
        return;
    y = m_gridUpdateRow * 80 + 60;
    height = (COMBAT_GRID_ROWS - m_gridUpdateRow) * 80;
    if (!m_gridUpdateRow) {
        y = 0;
        height += 60;
    }
    if (y + height > COMBAT_VIEW_HEIGHT)
        height = COMBAT_VIEW_HEIGHT - y;
    gbEnlargeScreenBlit = 0;
    gpWindowManager->UpdateScreenRegion(0, y, LOGICAL_SCREEN_WIDTH, height);
    gbEnlargeScreenBlit = 1;
    m_gridUpdateRow = COMBAT_GRID_ROWS;
}

// Draws the hex ground, the castle wall strip and the moat ends, then keeps
// the clean screen as the combat background.
VA(0x00471027, 0x1d4)
void combatManager::DrawBackground(void) {
    short x;
    short y;
    short wallY;

    for (y = 0; y < COMBAT_GRID_ROWS; y++) {
        for (x = 0; x < COMBAT_GRID_COLUMNS; x++)
            m_hexCells[y * COMBAT_GRID_COLUMNS + x].DrawGround();
        if (m_castleSide[0])
            m_combatIcons[COMBAT_ICON_CASTLE]->DrawToBuffer(m_hexCells[y * COMBAT_GRID_COLUMNS + COMBAT_CASTLE_WALL_COLUMN].m_x, m_hexCells[y * COMBAT_GRID_COLUMNS + COMBAT_CASTLE_WALL_COLUMN].m_y,
                                           (signed char)((y & 1) ? 5 : 6), ICON_DRAW_NORMAL, 0);
        wallY = y * 80 + 0x8b;
        if (y == 0) {
            m_backgroundBitmap->DrawToBuffer(0, 0);
            if (m_castleSide[1] == 1) {
                m_combatIcons[COMBAT_ICON_CASTLE]->DrawToBuffer(0, wallY, 7, ICON_DRAW_FLIPPED, 0);
                m_combatIcons[COMBAT_ICON_CASTLE]->DrawToBuffer(0x6e, wallY, 7, ICON_DRAW_FLIPPED, 0);
            } else if (m_castleSide[0] == 1) {
                m_combatIcons[COMBAT_ICON_CASTLE]->DrawToBuffer(0x27f, wallY, 7, ICON_DRAW_NORMAL, 0);
                m_combatIcons[COMBAT_ICON_CASTLE]->DrawToBuffer(0x211, wallY, 7, ICON_DRAW_NORMAL, 0);
            }
        }
    }
    gpWindowManager->m_screen->CopyTo(m_backgroundBuffer, 0, 0, 0, 0, LOGICAL_SCREEN_WIDTH, COMBAT_VIEW_HEIGHT);
    m_backgroundDrawn = 1;
}

// Redraws the battlefield: only the boxes around the stacks marked in
// m_limitCreatureCount when m_computeExtent is set, else the whole area.
// Rows draw obstacles, then occupants (right to left while m_gridMode is
// set), with the catapult (row 3) and the two heroes (rows 1 and 2).
VA(0x004711fb, 0xe27)
void combatManager::DrawFrame(signed char updateScreen) {
    short hexCol;
    int side;
    int i;
    signed char drawn;
    int boxBottom;
    int boxRight;
    int boxTop;
    int boxLeft;
    int sideDelta;
    short row;

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
                    boxTop = row * 80;
                    boxBottom = (row + 2) * 80 + 20;
                    if (m_extendLimitDown)
                        boxBottom += 60;
                    if (m_armies[side][i].m_facing == ARMY_FACING_RIGHT) {
                        boxLeft = hexCol * 78 - 110;
                        boxRight = (hexCol + 1) * 78 + 70;
                    } else {
                        boxLeft = hexCol * 78 - 70;
                        boxRight = (hexCol + 1) * 78 + 110;
                    }
                    if (m_armies[side][i].m_effectAnimation == ARMY_EFFECT_GOOD_LUCK
                        || m_armies[side][i].m_effectAnimation == ARMY_EFFECT_BAD_LUCK
                        || m_armies[side][i].m_effectAnimation == ARMY_EFFECT_GOOD_MORALE
                        || m_armies[side][i].m_effectAnimation == ARMY_EFFECT_BAD_MORALE)
                        boxTop -= 100;
                    if (m_armies[side][i].m_creatureType == CREATURE_CAVALRY)
                        boxTop -= 60;
                    if (m_armies[side][i].m_creatureType == CREATURE_SPRITE || m_armies[side][i].m_creatureType == CREATURE_ORC
                        || m_armies[side][i].m_creatureType == CREATURE_TROLL) {
                        if (m_armies[side][i].m_facing == ARMY_FACING_RIGHT)
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
                        if (side == 0)
                            sideDelta = -1;
                        else
                            sideDelta = 1;
                        if (m_armies[side][i].m_facing == ARMY_FACING_RIGHT) {
                            boxLeft = (hexCol + sideDelta) * 78 - 110;
                            boxRight = (hexCol + sideDelta + 1) * 78 + 70;
                        } else {
                            boxLeft = (hexCol + sideDelta) * 78 - 70;
                            boxRight = (hexCol + sideDelta + 1) * 78 + 110;
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
    if (!gbLimitToExtent) {
        if (m_backgroundDrawn) {
            if (m_computeExtent || m_redrawExtent)
                m_backgroundBuffer->CopyTo(gpWindowManager->m_screen, giMinExtentX, giMinExtentY, giMinExtentX,
                                           giMinExtentY, giMaxExtentX - giMinExtentX + 1,
                                           giMaxExtentY - giMinExtentY + 1);
            else
                m_backgroundBuffer->CopyTo(gpWindowManager->m_screen, 0, 0, 0, 0, LOGICAL_SCREEN_WIDTH, COMBAT_VIEW_HEIGHT);
        } else {
            DrawBackground();
        }
    }
    if (m_computeExtent) {
        gbLimitToExtent = 1;
        gbComputeExtent = 1;
    }
    if (!m_gridMode) {
        for (row = 0; row < COMBAT_GRID_ROWS; row++) {
            if (row == 3 && m_catapultFrame[1] != -1) {
                m_combatIcons[COMBAT_ICON_CATAPULT]->DrawToBuffer(0x1b, 0x17b, m_catapultFrame[1], ICON_DRAW_NORMAL, 0);
                m_combatIcons[COMBAT_ICON_CATAPULT]->DimToBuffer(0x1b, 0x17b, m_catapultFrame[1] == 7 ? 16 : 15, ICON_DRAW_NORMAL, 0);
            }
            for (hexCol = 0; hexCol < COMBAT_GRID_COLUMNS; hexCol++)
                if (m_hexCells[row * COMBAT_GRID_COLUMNS + hexCol].m_obstacleIndex != -1)
                    m_hexCells[row * COMBAT_GRID_COLUMNS + hexCol].DrawObstacle();
            if (row == 0 && m_castleSide[0])
                m_combatIcons[COMBAT_ICON_KEEP]->DrawToBuffer(0x22d, 0, 0, ICON_DRAW_NORMAL, 0);
            for (hexCol = 1; hexCol <= 7; hexCol++) {
                if (gbLimitToExtent && m_armies[m_currentSide][m_currentArmyIndex].m_hex == row * COMBAT_GRID_COLUMNS + hexCol)
                    gbCurrArmyDrawn = 1;
                m_hexCells[row * COMBAT_GRID_COLUMNS + hexCol].DrawOccupant();
            }
            if (row == 2 && m_heroType[0] != -1) {
                m_combatIcons[COMBAT_ICON_TENT]->DrawToBuffer(0x27f, 0xa9, m_heroType[0], ICON_DRAW_FLIPPED, 0);
                m_combatIcons[COMBAT_ICON_TENT]->DrawToBuffer(0x250, 0xeb, gpGame->m_players[m_playerId[0]].Color() + 4, ICON_DRAW_FLIPPED, 0);
                m_combatIcons[COMBAT_ICON_TENT]->DrawToBuffer(0x238, 0xfd, m_heroType[0] + 8, ICON_DRAW_NORMAL, 0);
            }
            if (row == 1 && m_heroType[1] != -1) {
                m_combatIcons[COMBAT_ICON_TENT]->DrawToBuffer(0, 0x59, m_heroType[1], ICON_DRAW_NORMAL, 0);
                m_combatIcons[COMBAT_ICON_TENT]->DrawToBuffer(0x2f, 0x9b, gpGame->m_players[m_playerId[1]].Color() + 4, ICON_DRAW_NORMAL, 0);
                m_combatIcons[COMBAT_ICON_TENT]->DrawToBuffer(0x36, 0xad, m_heroType[1] + 8, ICON_DRAW_NORMAL, 0);
            }
        }
    } else {
        for (row = 0; row < COMBAT_GRID_ROWS; row++) {
            if (row == 0 && m_castleSide[0])
                m_combatIcons[COMBAT_ICON_KEEP]->DrawToBuffer(0x22d, 0, 0, ICON_DRAW_NORMAL, 0);
            if (row == 3 && m_catapultFrame[1] != -1) {
                m_combatIcons[COMBAT_ICON_CATAPULT]->DrawToBuffer(0x1b, 0x17b, m_catapultFrame[1], ICON_DRAW_NORMAL, 0);
                m_combatIcons[COMBAT_ICON_CATAPULT]->DimToBuffer(0x1b, 0x17b, m_catapultFrame[1] == 7 ? 16 : 15, ICON_DRAW_NORMAL, 0);
            }
            for (hexCol = 0; hexCol < COMBAT_GRID_COLUMNS; hexCol++)
                if (m_hexCells[row * COMBAT_GRID_COLUMNS + hexCol].m_obstacleIndex != -1)
                    m_hexCells[row * COMBAT_GRID_COLUMNS + hexCol].DrawObstacle();
            for (hexCol = 7; hexCol >= 1; hexCol--) {
                if (gbLimitToExtent && m_armies[m_currentSide][m_currentArmyIndex].m_hex == row * COMBAT_GRID_COLUMNS + hexCol)
                    gbCurrArmyDrawn = 1;
                m_hexCells[row * COMBAT_GRID_COLUMNS + hexCol].DrawOccupant();
            }
            if (row == 1 && m_heroType[1] != -1) {
                m_combatIcons[COMBAT_ICON_TENT]->DrawToBuffer(0, 0x59, m_heroType[1], ICON_DRAW_NORMAL, 0);
                m_combatIcons[COMBAT_ICON_TENT]->DrawToBuffer(0x2f, 0x9b, gpGame->m_players[m_playerId[1]].Color() + 4, ICON_DRAW_NORMAL, 0);
                m_combatIcons[COMBAT_ICON_TENT]->DrawToBuffer(0x36, 0xad, m_heroType[1] + 8, ICON_DRAW_NORMAL, 0);
            }
            if (row == 2 && m_heroType[0] != -1) {
                m_combatIcons[COMBAT_ICON_TENT]->DrawToBuffer(0x27f, 0xa9, m_heroType[0], ICON_DRAW_FLIPPED, 0);
                m_combatIcons[COMBAT_ICON_TENT]->DrawToBuffer(0x250, 0xeb, gpGame->m_players[m_playerId[0]].Color() + 4, ICON_DRAW_FLIPPED, 0);
                m_combatIcons[COMBAT_ICON_TENT]->DrawToBuffer(0x238, 0xfd, m_heroType[0] + 8, ICON_DRAW_NORMAL, 0);
            }
        }
    }
    DrawCombatBorder();
    if (m_computeExtent || m_redrawExtent) {
        m_computeExtent = 0;
        m_redrawExtent = 0;
        gbLimitToExtent = 0;
        gbComputeExtent = 0;
        gbFullCombatScreenDrawn = 0;
        DelayTil(glTimers);
        glTimers[0] = KBTickCount() + 75;
        if (updateScreen == 1) {
            if (giMaxExtentY > 460)
                giMaxExtentY = 460;
            gbEnlargeScreenBlit = 0;
            gpWindowManager->UpdateScreenRegion(giMinExtentX, giMinExtentY, giMaxExtentX - giMinExtentX + 1,
                                                giMaxExtentY - giMinExtentY + 1);
            gbEnlargeScreenBlit = 1;
            m_gridUpdateRow = COMBAT_GRID_ROWS;
        }
    } else if (updateScreen == 1) {
        gbFullCombatScreenDrawn = 1;
        DelayTil(glTimers);
        glTimers[0] = KBTickCount() + 75;
        UpdateCombatArea();
    }
}

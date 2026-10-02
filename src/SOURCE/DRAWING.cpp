// Combat screen drawing; Buka 2.1 SOURCE/DRAWING correspondence. HoMM1
// keeps one text line in the combat window and redraws the battlefield
// from the grid row UpdateGrid records.

#include <match.h>

#include <H1/All.h>
#include <H1/KB.h>
#include <SOURCE/NOOPT.h>
#include <SOURCE/X_GLOBAL.h>
#include <SOURCE/wingraph.h>

#include <stdio.h>
#include <string.h>

// Lowers the first grid row that needs redrawing to the one above the hex.
VA(0x004709f0, 0x5f)
void combatManager::UpdateGrid(short hex, int) {
    short row;

    row = hex / 9 - 1;
    if (row < 0)
        row = 0;
    if (m_gridUpdateRow > row)
        m_gridUpdateRow = row;
}

VA(0x00470a4f, 0x5a)
void combatManager::UpdateGridForMove(short hex, signed char direction, int attributes) {
    if (direction == 0 || direction == 5)
        UpdateGrid(hex - 9, attributes);
    else
        UpdateGrid(hex, attributes);
}

// Sets the combat window's text line and redraws it outside the extent
// bookkeeping.
VA(0x00470aa9, 0xb5)
void combatManager::CombatMessage(char* text, int updateScreen) {
    int oldCompute;
    tag_message message;
    int prevLimit;

    message.type = MESSAGE_WIDGET;
    message.command = WIDGET_COMMAND_SET_TEXT;
    message.id = 0xc;
    message.text = text;
    m_combatWindow->BroadcastMessage(message);
    oldCompute = gbComputeExtent;
    prevLimit = gbLimitToExtent;
    gbComputeExtent = gbLimitToExtent = 0;
    m_combatWindow->DrawWindow(0, 2, 0xc);
    SaveCombatBorder();
    if (updateScreen)
        gpWindowManager->UpdateScreenRegion(0x30, 0x1cc, 0x21f, 0x14);
    gbComputeExtent = oldCompute;
    gbLimitToExtent = prevLimit;
}

// The help line for the current mouse command.
VA(0x00470b5e, 0x2f3)
void combatManager::CombatMessage(short messageType) {
    army* currentArmy;
    short targetMonster;
    army* target;
    short actingType;

    currentArmy = &m_armies[m_currentSide][m_currentArmyIndex];
    actingType = currentArmy->m_creatureType;
    target = 0;
    targetMonster = 0;
    if (currentArmy->m_targetSide >= 0 && currentArmy->m_targetIndex >= 0) {
        target = &m_armies[currentArmy->m_targetSide][currentArmy->m_targetIndex];
        targetMonster = target->m_creatureType;
    }
    switch (messageType) {
        case 0:
            if ((currentArmy->m_stats.attributes & 4) && currentArmy->m_stats.shots == 0 && target)
                strcpy(gText, cCombatMessage[8]);
            else
                strcpy(gText, cCombatMessage[0]);
            break;
        case 1:
            sprintf(gText, cCombatMessage[1], gArmyNames[actingType]);
            break;
        case 2:
            sprintf(gText, cCombatMessage[2], gArmyNames[actingType]);
            break;
        case 7:
            sprintf(gText, cCombatMessage[3], gArmyNames[targetMonster]);
            break;
        case 3:
            sprintf(gText, cCombatMessage[4], gArmyNames[targetMonster], currentArmy->m_stats.shots,
                    currentArmy->m_stats.shots > 1 ? "s" : "");
            break;
        case 4:
            strcpy(gText, cCombatMessage[5]);
            break;
        case 13:
            strcpy(gText, cCombatMessage[6]);
            break;
        case 5:
            actingType = m_armies[m_currentSide][m_hexCells[m_selectedHex].m_occupantIndex].m_creatureType;
            if (actingType >= 0)
                sprintf(gText, cCombatMessage[7], gArmyNames[actingType]);
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
    m_unknown260 = 0;
    for (side = 0; side < 2; side++) {
        for (j = 0; j < 5; j++) {
            if (m_armies[side][j].m_stats.attributes & 0x10)
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
    height = (5 - m_gridUpdateRow) * 80;
    if (!m_gridUpdateRow) {
        y = 0;
        height += 60;
    }
    if (y + height > 460)
        height = 460 - y;
    gbEnlargeScreenBlit = 0;
    gpWindowManager->UpdateScreenRegion(0, y, 640, height);
    gbEnlargeScreenBlit = 1;
    m_gridUpdateRow = 5;
}

// Draws the hex ground, the castle wall strip and the moat ends, then keeps
// the clean screen as the combat background.
VA(0x00471027, 0x1d4)
void combatManager::DrawBackground(void) {
    short x;
    short y;
    short wallY;

    for (y = 0; y < 5; y++) {
        for (x = 0; x < 9; x++)
            m_hexCells[y * 9 + x].DrawGround();
        if (m_castleSide[0])
            m_combatIcons[5]->DrawToBuffer(m_hexCells[y * 9 + 5].m_x, m_hexCells[y * 9 + 5].m_y,
                                           (signed char)((y & 1) ? 5 : 6), 0, 0);
        wallY = y * 80 + 0x8b;
        if (y == 0) {
            m_backgroundBitmap->DrawToBuffer(0, 0);
            if (m_castleSide[1] == 1) {
                m_combatIcons[5]->DrawToBuffer(0, wallY, 7, 1, 0);
                m_combatIcons[5]->DrawToBuffer(0x6e, wallY, 7, 1, 0);
            } else if (m_castleSide[0] == 1) {
                m_combatIcons[5]->DrawToBuffer(0x27f, wallY, 7, 0, 0);
                m_combatIcons[5]->DrawToBuffer(0x211, wallY, 7, 0, 0);
            }
        }
    }
    gpWindowManager->m_screen->CopyTo(m_backgroundBuffer, 0, 0, 0, 0, 640, 460);
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
        giMinExtentX = 639;
        giMinExtentY = 459;
        drawn = 0;
        for (side = 0; side < 2; side++) {
            for (i = 0; i < 5; i++) {
                if (m_limitCreatureCount[side][i] > 0) {
                    drawn = 1;
                    hexCol = m_armies[side][i].m_hex % 9;
                    row = m_armies[side][i].m_hex / 9;
                    boxTop = row * 80;
                    boxBottom = (row + 2) * 80 + 20;
                    if (m_unknown260)
                        boxBottom += 60;
                    if (m_armies[side][i].m_facing == 1) {
                        boxLeft = hexCol * 78 - 110;
                        boxRight = (hexCol + 1) * 78 + 70;
                    } else {
                        boxLeft = hexCol * 78 - 70;
                        boxRight = (hexCol + 1) * 78 + 110;
                    }
                    if (m_armies[side][i].m_effectAnimation == 22 || m_armies[side][i].m_effectAnimation == 23
                        || m_armies[side][i].m_effectAnimation == 24 || m_armies[side][i].m_effectAnimation == 25)
                        boxTop -= 100;
                    if (m_armies[side][i].m_creatureType == 4)
                        boxTop -= 60;
                    if (m_armies[side][i].m_creatureType == 12 || m_armies[side][i].m_creatureType == 7
                        || m_armies[side][i].m_creatureType == 10) {
                        if (m_armies[side][i].m_facing == 1)
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
                    if (m_armies[side][i].m_stats.attributes & 1) {
                        if (side == 0)
                            sideDelta = -1;
                        else
                            sideDelta = 1;
                        if (m_armies[side][i].m_facing == 1) {
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
        if (giMaxExtentX > 639)
            giMaxExtentX = 639;
        if (giMaxExtentY > 459)
            giMaxExtentY = 459;
    }
    m_gridUpdateRow = 0;
    if (!gbLimitToExtent) {
        if (m_backgroundDrawn) {
            if (m_computeExtent || m_redrawExtent)
                m_backgroundBuffer->CopyTo(gpWindowManager->m_screen, giMinExtentX, giMinExtentY, giMinExtentX,
                                           giMinExtentY, giMaxExtentX - giMinExtentX + 1,
                                           giMaxExtentY - giMinExtentY + 1);
            else
                m_backgroundBuffer->CopyTo(gpWindowManager->m_screen, 0, 0, 0, 0, 640, 460);
        } else {
            DrawBackground();
        }
    }
    if (m_computeExtent) {
        gbLimitToExtent = 1;
        gbComputeExtent = 1;
    }
    if (!m_gridMode) {
        for (row = 0; row < 5; row++) {
            if (row == 3 && m_catapultFrame[1] != -1) {
                m_combatIcons[3]->DrawToBuffer(0x1b, 0x17b, m_catapultFrame[1], 0, 0);
                m_combatIcons[3]->DimToBuffer(0x1b, 0x17b, m_catapultFrame[1] == 7 ? 16 : 15, 0, 0);
            }
            for (hexCol = 0; hexCol < 9; hexCol++)
                if (m_hexCells[row * 9 + hexCol].m_obstacleIndex != -1)
                    m_hexCells[row * 9 + hexCol].DrawObstacle();
            if (row == 0 && m_castleSide[0])
                m_combatIcons[7]->DrawToBuffer(0x22d, 0, 0, 0, 0);
            for (hexCol = 1; hexCol <= 7; hexCol++) {
                if (gbLimitToExtent && m_armies[m_currentSide][m_currentArmyIndex].m_hex == row * 9 + hexCol)
                    gbCurrArmyDrawn = 1;
                m_hexCells[row * 9 + hexCol].DrawOccupant();
            }
            if (row == 2 && m_heroType[0] != -1) {
                m_combatIcons[4]->DrawToBuffer(0x27f, 0xa9, m_heroType[0], 1, 0);
                m_combatIcons[4]->DrawToBuffer(0x250, 0xeb, gpGame->m_players[m_playerId[0]].Color() + 4, 1, 0);
                m_combatIcons[4]->DrawToBuffer(0x238, 0xfd, m_heroType[0] + 8, 0, 0);
            }
            if (row == 1 && m_heroType[1] != -1) {
                m_combatIcons[4]->DrawToBuffer(0, 0x59, m_heroType[1], 0, 0);
                m_combatIcons[4]->DrawToBuffer(0x2f, 0x9b, gpGame->m_players[m_playerId[1]].Color() + 4, 0, 0);
                m_combatIcons[4]->DrawToBuffer(0x36, 0xad, m_heroType[1] + 8, 0, 0);
            }
        }
    } else {
        for (row = 0; row < 5; row++) {
            if (row == 0 && m_castleSide[0])
                m_combatIcons[7]->DrawToBuffer(0x22d, 0, 0, 0, 0);
            if (row == 3 && m_catapultFrame[1] != -1) {
                m_combatIcons[3]->DrawToBuffer(0x1b, 0x17b, m_catapultFrame[1], 0, 0);
                m_combatIcons[3]->DimToBuffer(0x1b, 0x17b, m_catapultFrame[1] == 7 ? 16 : 15, 0, 0);
            }
            for (hexCol = 0; hexCol < 9; hexCol++)
                if (m_hexCells[row * 9 + hexCol].m_obstacleIndex != -1)
                    m_hexCells[row * 9 + hexCol].DrawObstacle();
            for (hexCol = 7; hexCol >= 1; hexCol--) {
                if (gbLimitToExtent && m_armies[m_currentSide][m_currentArmyIndex].m_hex == row * 9 + hexCol)
                    gbCurrArmyDrawn = 1;
                m_hexCells[row * 9 + hexCol].DrawOccupant();
            }
            if (row == 1 && m_heroType[1] != -1) {
                m_combatIcons[4]->DrawToBuffer(0, 0x59, m_heroType[1], 0, 0);
                m_combatIcons[4]->DrawToBuffer(0x2f, 0x9b, gpGame->m_players[m_playerId[1]].Color() + 4, 0, 0);
                m_combatIcons[4]->DrawToBuffer(0x36, 0xad, m_heroType[1] + 8, 0, 0);
            }
            if (row == 2 && m_heroType[0] != -1) {
                m_combatIcons[4]->DrawToBuffer(0x27f, 0xa9, m_heroType[0], 1, 0);
                m_combatIcons[4]->DrawToBuffer(0x250, 0xeb, gpGame->m_players[m_playerId[0]].Color() + 4, 1, 0);
                m_combatIcons[4]->DrawToBuffer(0x238, 0xfd, m_heroType[0] + 8, 0, 0);
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
            m_gridUpdateRow = 5;
        }
    } else if (updateScreen == 1) {
        gbFullCombatScreenDrawn = 1;
        DelayTil(glTimers);
        glTimers[0] = KBTickCount() + 75;
        UpdateCombatArea();
    }
}

// Combat screen drawing; Buka 2.1 SOURCE/DRAWING correspondence. HoMM1
// keeps one text line in the combat window and redraws the battlefield
// from the grid row UpdateGrid records.

#include <match.h>

#include <H1/All.h>
#include <H1/KB.h>
#include <SOURCE/X_GLOBAL.h>

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
    message.payload.widget.command = WIDGET_COMMAND_SET_TEXT;
    message.payload.widget.id = 0xc;
    message.payload.widget.data.text = text;
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

    m_unknown727 = 1;
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
            m_backdropBitmap->DrawToBuffer(0, 0);
            if (m_castleSide[1] == 1) {
                m_combatIcons[5]->DrawToBuffer(0, wallY, 7, 1, 0);
                m_combatIcons[5]->DrawToBuffer(0x6e, wallY, 7, 1, 0);
            } else if (m_castleSide[0] == 1) {
                m_combatIcons[5]->DrawToBuffer(0x27f, wallY, 7, 0, 0);
                m_combatIcons[5]->DrawToBuffer(0x211, wallY, 7, 0, 0);
            }
        }
    }
    gpWindowManager->m_screen->CopyTo(m_backgroundBitmap, 0, 0, 0, 0, 640, 460);
    m_unknown299 = 1;
}

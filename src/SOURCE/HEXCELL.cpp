// Combat hex cells; Buka 2.1 SOURCE/HEXCELL correspondence. HoMM1 cells
// are twelve bytes and draw the castle towers and walls themselves.

#include <match.h>

#include <BASE/icon.h>
#include <H1/KB.h>
#include <SOURCE/combatManager.h>
#include <SOURCE/hexcell.h>

VA(0x0046e5b0, 0x4a)
hexcell::hexcell(void) {
    m_groundIcon = 0;
    m_groundFrame = 0;
    m_occupantSide = -1;
    m_occupantIndex = 0;
    m_obstacleIndex = -1;
    m_occupantFrame = -1;
    m_pathFlag = 0;
}

// Moves the live occupant from another cell into this one.
VA(0x0046e5fa, 0x4d)
hexcell* hexcell::TakeOccupant(hexcell* from) {
    m_occupantSide = from->m_occupantSide;
    m_occupantIndex = from->m_occupantIndex;
    m_occupantFrame = from->m_occupantFrame;
    from->m_occupantSide = -1;
    from->m_occupantFrame = -1;
    return this;
}

VA(0x0046e647, 0x4b)
void hexcell::DrawGround(void) {
    gpCombatManager->m_combatIcons[m_groundIcon + 3]->DrawToBuffer(m_x, m_y, m_groundFrame, 0, 0);
}

VA(0x0046e692, 0x8a)
void hexcell::DrawOccupant(void) {
    signed char frame;
    army* occupant;

    if (m_occupantSide != -1) {
        occupant = &gpCombatManager->m_armies[m_occupantSide][m_occupantIndex];
        frame = occupant->m_facing;
        if (m_occupantFrame != frame)
            occupant->DrawToBuffer(m_x, m_y);
    }
}

VA(0x0046e71c, 0x151)
void hexcell::DrawTower(signed char frame) {
    signed char flip;
    short row;

    flip = gpCombatManager->m_castleSide[1] == 1;
    gpCombatManager->m_combatIcons[8]->DrawToBuffer(flip ? m_x : m_x + 28, m_y, frame, 1, 0);
    row = (m_y - 139) / 80;
    if (row == 4)
        return;
    if (row & 1)
        gpCombatManager->m_combatIcons[8]->DrawToBuffer(flip ? m_x : m_x + 28, m_y, 9, 1, 0);
    else
        gpCombatManager->m_combatIcons[8]->DrawToBuffer(flip ? m_x - 28 : m_x, m_y, 9, 0, 0);
}

VA(0x0046e86d, 0x2b3)
void hexcell::DrawWall(void) {
    signed char flip;
    short row;
    short damageLevel;

    flip = gpCombatManager->m_castleSide[1] == 1;
    row = (m_y - 139) / 80;
    damageLevel = gpCombatManager->m_wallDamage;
    gpCombatManager->m_combatIcons[9]->DrawToBuffer(flip ? m_x - 15 : m_x + 15,
                                                    row == 0 ? m_y - 20 : m_y - 36,
                                                    gpCombatManager->m_wallFrame, 0, 0);
    if (damageLevel != -1) {
        if (row == 4) {
            gpCombatManager->m_combatIcons[9]->DrawToBuffer(flip ? m_x : m_x + 15, m_y + 8,
                                                            damageLevel, 0, 0);
            return;
        }
        if (row & 1) {
            gpCombatManager->m_combatIcons[9]->DrawToBuffer(flip ? m_x : m_x + 8, m_y + 40,
                                                            damageLevel, 0, 0);
            if (damageLevel > 0)
                gpCombatManager->m_combatIcons[9]->DrawToBuffer(flip ? m_x - 40 : m_x - 32,
                                                                m_y + 60, damageLevel - 1, 0, 0);
        } else {
            gpCombatManager->m_combatIcons[9]->DrawToBuffer(flip ? m_x - 28 : m_x - 8,
                                                            m_y + 40, damageLevel, 0, 0);
            if (damageLevel > 0)
                gpCombatManager->m_combatIcons[9]->DrawToBuffer(flip ? m_x + 20 : m_x + 40,
                                                                m_y + 60, damageLevel - 1, 0, 0);
        }
    }
}

VA(0x0046eb20, 0x181)
void hexcell::DrawObstacle(void) {
    if (m_obstacleType == 5) {
        switch (m_obstacleIndex) {
            case 8:
            case 10:
                DrawTower(m_obstacleIndex);
                break;
            case 64:
                if (gpCombatManager->m_wallFrame < 4 || gpCombatManager->m_unknown6e3 == 1)
                    DrawTower(8);
                DrawWall();
                break;
            case 65:
                if (gpCombatManager->m_wallFrame < 4 || gpCombatManager->m_unknown6e3 == 1)
                    DrawTower(10);
                DrawWall();
                break;
            case 66:
                DrawWall();
                break;
        }
    } else {
        gpCombatManager->m_combatIcons[5]->DrawToBuffer(m_x, m_y, m_obstacleIndex, 0, 0);
    }
}

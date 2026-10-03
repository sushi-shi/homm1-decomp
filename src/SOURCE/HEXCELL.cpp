// Combat hex cells; Buka 2.1 SOURCE/HEXCELL correspondence. HoMM1 cells
// are twelve bytes and draw the castle towers and walls themselves.

#include <match.h>

#include <BASE/icon.h>
#include <SOURCE/combatManager.h>
#include <SOURCE/hexcell.h>
#include <SOURCE/KB.h>

VA(0x0046e5b0, 0x4a)
hexcell::hexcell(void) {
    m_groundIcon = COMBAT_ICON_GROUND;
    m_groundFrame = 0;
    m_occupantSide = COMBAT_SIDE_NONE;
    m_occupantIndex = 0;
    m_obstacleIndex = COMBAT_OBSTACLE_NONE;
    m_occupantFrame = HEXCELL_OCCUPANT_FRAME_NONE;
    m_pathFlag = 0;
}

// Moves the live occupant from another cell into this one.
VA(0x0046e5fa, 0x4d)
hexcell* hexcell::TakeOccupant(hexcell* from) {
    m_occupantSide = from->m_occupantSide;
    m_occupantIndex = from->m_occupantIndex;
    m_occupantFrame = from->m_occupantFrame;
    from->m_occupantSide = COMBAT_SIDE_NONE;
    from->m_occupantFrame = HEXCELL_OCCUPANT_FRAME_NONE;
    return this;
}

VA(0x0046e647, 0x4b)
void hexcell::DrawGround(void) {
    gpCombatManager->m_combatIcons[m_groundIcon]
        ->DrawToBuffer(m_x, m_y, m_groundFrame, ICON_DRAW_NORMAL, ICON_DRAW_OFFSET_FULL);
}

VA(0x0046e692, 0x8a)
void hexcell::DrawOccupant(void) {
    signed char frame;
    army* occupant;

    if (m_occupantSide != COMBAT_SIDE_NONE) {
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

    flip = gpCombatManager->m_castleSide[COMBAT_ATTACKER_SIDE] == 1;
    gpCombatManager->m_combatIcons[COMBAT_ICON_CASTLE]
        ->DrawToBuffer(flip ? m_x : m_x + 28, m_y, frame, ICON_DRAW_FLIPPED, ICON_DRAW_OFFSET_FULL);
    row = (m_y - COMBAT_HEX_ORIGIN_Y) / COMBAT_HEX_HEIGHT;
    if (row == COMBAT_GRID_LAST_ROW)
        return;
    if (row & 1)
        gpCombatManager->m_combatIcons[COMBAT_ICON_CASTLE]
            ->DrawToBuffer(flip ? m_x : m_x + 28, m_y, 9, ICON_DRAW_FLIPPED, ICON_DRAW_OFFSET_FULL);
    else
        gpCombatManager->m_combatIcons[COMBAT_ICON_CASTLE]
            ->DrawToBuffer(flip ? m_x - 28 : m_x, m_y, 9, ICON_DRAW_NORMAL, ICON_DRAW_OFFSET_FULL);
}

VA(0x0046e86d, 0x2b3)
void hexcell::DrawWall(void) {
    signed char flip;
    short row;
    short damageLevel;

    flip = gpCombatManager->m_castleSide[COMBAT_ATTACKER_SIDE] == 1;
    row = (m_y - COMBAT_HEX_ORIGIN_Y) / COMBAT_HEX_HEIGHT;
    damageLevel = gpCombatManager->m_wallDamage;
    gpCombatManager->m_combatIcons[COMBAT_ICON_CLOUD]->DrawToBuffer(
        flip ? m_x - 15 : m_x + 15,
        row == 0 ? m_y - 20 : m_y - 36,
        gpCombatManager->m_wallFrame,
        ICON_DRAW_NORMAL,
        ICON_DRAW_OFFSET_FULL
    );
    if (damageLevel != COMBAT_WALL_DAMAGE_NONE) {
        if (row == COMBAT_GRID_LAST_ROW) {
            gpCombatManager->m_combatIcons[COMBAT_ICON_CLOUD]->DrawToBuffer(
                flip ? m_x : m_x + 15,
                m_y + 8,
                damageLevel,
                ICON_DRAW_NORMAL,
                ICON_DRAW_OFFSET_FULL
            );
            return;
        }
        if (row & 1) {
            gpCombatManager->m_combatIcons[COMBAT_ICON_CLOUD]->DrawToBuffer(
                flip ? m_x : m_x + 8,
                m_y + 40,
                damageLevel,
                ICON_DRAW_NORMAL,
                ICON_DRAW_OFFSET_FULL
            );
            if (damageLevel > 0)
                gpCombatManager->m_combatIcons[COMBAT_ICON_CLOUD]->DrawToBuffer(
                    flip ? m_x - 40 : m_x - 32,
                    m_y + 60,
                    damageLevel - 1,
                    ICON_DRAW_NORMAL,
                    ICON_DRAW_OFFSET_FULL
                );
        } else {
            gpCombatManager->m_combatIcons[COMBAT_ICON_CLOUD]->DrawToBuffer(
                flip ? m_x - 28 : m_x - 8,
                m_y + 40,
                damageLevel,
                ICON_DRAW_NORMAL,
                ICON_DRAW_OFFSET_FULL
            );
            if (damageLevel > 0)
                gpCombatManager->m_combatIcons[COMBAT_ICON_CLOUD]->DrawToBuffer(
                    flip ? m_x + 20 : m_x + 40,
                    m_y + 60,
                    damageLevel - 1,
                    ICON_DRAW_NORMAL,
                    ICON_DRAW_OFFSET_FULL
                );
        }
    }
}

VA(0x0046eb20, 0x181)
void hexcell::DrawObstacle(void) {
    if (m_obstacleType == COMBAT_ICON_CASTLE) {
        switch (m_obstacleIndex) {
            case COMBAT_WALL_INTACT:
            case COMBAT_WALL_DAMAGED:
                DrawTower(m_obstacleIndex);
                break;
            case COMBAT_WALL_INTACT_HIT:
                if (gpCombatManager->m_wallFrame < 4 || gpCombatManager->m_wallSurvives == 1)
                    DrawTower(COMBAT_WALL_INTACT);
                DrawWall();
                break;
            case COMBAT_WALL_DAMAGED_HIT:
                if (gpCombatManager->m_wallFrame < 4 || gpCombatManager->m_wallSurvives == 1)
                    DrawTower(COMBAT_WALL_DAMAGED);
                DrawWall();
                break;
            case COMBAT_WALL_COLLAPSING:
                DrawWall();
                break;
        }
    } else {
        gpCombatManager->m_combatIcons[COMBAT_ICON_OBSTACLES]
            ->DrawToBuffer(m_x, m_y, m_obstacleIndex, ICON_DRAW_NORMAL, ICON_DRAW_OFFSET_FULL);
    }
}

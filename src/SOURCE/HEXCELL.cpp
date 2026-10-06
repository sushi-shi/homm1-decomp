// Combat hex cells. HoMM1 cells are twelve bytes and draw the castle towers
// and walls themselves.

#include <match.h>

#include <BASE/icon.h>
#include <SOURCE/combatManager.h>
#include <SOURCE/hexcell.h>
#include <SOURCE/KB.h>

VA(0x0043b700, 0x3f)
hexcell::hexcell(void) {
    m_groundIcon = COMBAT_ICON_GROUND;
    m_groundFrame = 0;
    m_occupantSide = COMBAT_SIDE_NONE;
    m_occupantIndex = 0;
    m_obstacleIndex = COMBAT_OBSTACLE_NONE;
    m_occupantFootprintHalf = HEXCELL_FOOTPRINT_HALF_NONE;
    m_pathFlag = 0;
}

// Moves the live occupant from another cell into this one.
VA(0x0043b73f, 0x42)
hexcell* hexcell::TakeOccupant(hexcell* from) {
    m_occupantSide = from->m_occupantSide;
    m_occupantIndex = from->m_occupantIndex;
    m_occupantFootprintHalf = from->m_occupantFootprintHalf;
    from->m_occupantSide = COMBAT_SIDE_NONE;
    from->m_occupantFootprintHalf = HEXCELL_FOOTPRINT_HALF_NONE;
    return this;
}

VA(0x0043b781, 0x40)
void hexcell::DrawGround(void) {
    gCombatManager->m_combatIcons[m_groundIcon]
        ->DrawToBuffer(m_x, m_y, m_groundFrame, ICON_DRAW_NORMAL, ICON_DRAW_OFFSET_FULL);
}

VA(0x0043b7c1, 0x71)
void hexcell::DrawOccupant(void) {
    i8 occupantFacing;
    army* occupant;

    if (m_occupantSide != COMBAT_SIDE_NONE) {
        occupant = &gCombatManager->m_armies[m_occupantSide][m_occupantIndex];
        occupantFacing = occupant->m_facing;
        if (m_occupantFootprintHalf != occupantFacing)
            occupant->DrawToBuffer(m_x, m_y);
    }
}

#define mirrored level // frame-slot spelling
VA(0x0043b832, 0x124)
void hexcell::DrawTower(i8 frame) {
    i8 mirrored;
    i16 row;

    mirrored = gCombatManager->m_castleSide[COMBAT_ATTACKER_SIDE] == 1;
    gCombatManager->m_combatIcons[COMBAT_ICON_CASTLE]->DrawToBuffer(
        mirrored ? m_x : m_x + 28,
        m_y,
        frame,
        ICON_DRAW_FLIPPED,
        ICON_DRAW_OFFSET_FULL
    );
    row = (m_y - COMBAT_HEX_ORIGIN_Y) / COMBAT_HEX_HEIGHT;
    if (row == COMBAT_GRID_LAST_ROW)
        return;
    if (row & 1)
        gCombatManager->m_combatIcons[COMBAT_ICON_CASTLE]->DrawToBuffer(
            mirrored ? m_x : m_x + 28,
            m_y,
            9,
            ICON_DRAW_FLIPPED,
            ICON_DRAW_OFFSET_FULL
        );
    else
        gCombatManager->m_combatIcons[COMBAT_ICON_CASTLE]->DrawToBuffer(
            mirrored ? m_x - 28 : m_x,
            m_y,
            9,
            ICON_DRAW_NORMAL,
            ICON_DRAW_OFFSET_FULL
        );
}
#undef mirrored

VA(0x0043b956, 0x279)
void hexcell::DrawWall(void) {
    i8 level;
    i16 row;
    i16 rubbleFrame;

    level = gCombatManager->m_castleSide[COMBAT_ATTACKER_SIDE] == 1;
    row = (m_y - COMBAT_HEX_ORIGIN_Y) / COMBAT_HEX_HEIGHT;
    rubbleFrame = gCombatManager->m_wallDamage;
    gCombatManager->m_combatIcons[COMBAT_ICON_CLOUD]->DrawToBuffer(
        level ? m_x - 15 : m_x + 15,
        row == 0 ? m_y - 20 : m_y - 36,
        gCombatManager->m_wallFrame,
        ICON_DRAW_NORMAL,
        ICON_DRAW_OFFSET_FULL
    );
    if (rubbleFrame != COMBAT_WALL_DAMAGE_NONE) {
        if (row == COMBAT_GRID_LAST_ROW) {
            gCombatManager->m_combatIcons[COMBAT_ICON_CLOUD]->DrawToBuffer(
                level ? m_x : m_x + 15,
                m_y + 8,
                rubbleFrame,
                ICON_DRAW_NORMAL,
                ICON_DRAW_OFFSET_FULL
            );
            return;
        }
        if (row & 1) {
            gCombatManager->m_combatIcons[COMBAT_ICON_CLOUD]->DrawToBuffer(
                level ? m_x : m_x + 8,
                m_y + 40,
                rubbleFrame,
                ICON_DRAW_NORMAL,
                ICON_DRAW_OFFSET_FULL
            );
            if (rubbleFrame > 0)
                gCombatManager->m_combatIcons[COMBAT_ICON_CLOUD]->DrawToBuffer(
                    level ? m_x - 40 : m_x - 32,
                    m_y + 60,
                    rubbleFrame - 1,
                    ICON_DRAW_NORMAL,
                    ICON_DRAW_OFFSET_FULL
                );
        } else {
            gCombatManager->m_combatIcons[COMBAT_ICON_CLOUD]->DrawToBuffer(
                level ? m_x - 28 : m_x - 8,
                m_y + 40,
                rubbleFrame,
                ICON_DRAW_NORMAL,
                ICON_DRAW_OFFSET_FULL
            );
            if (rubbleFrame > 0)
                gCombatManager->m_combatIcons[COMBAT_ICON_CLOUD]->DrawToBuffer(
                    level ? m_x + 20 : m_x + 40,
                    m_y + 60,
                    rubbleFrame - 1,
                    ICON_DRAW_NORMAL,
                    ICON_DRAW_OFFSET_FULL
                );
        }
    }
}

VA(0x0043bbcf, 0x151)
void hexcell::DrawObstacle(void) {
    if (m_obstacleIcon == COMBAT_ICON_CASTLE) {
        switch (m_obstacleIndex) {
            case COMBAT_WALL_INTACT:
            case COMBAT_WALL_DAMAGED:
                DrawTower(m_obstacleIndex);
                break;
            case COMBAT_WALL_INTACT_HIT:
                if (gCombatManager->m_wallFrame < 4 || gCombatManager->m_wallSurvives == 1)
                    DrawTower(COMBAT_WALL_INTACT);
                DrawWall();
                break;
            case COMBAT_WALL_DAMAGED_HIT:
                if (gCombatManager->m_wallFrame < 4 || gCombatManager->m_wallSurvives == 1)
                    DrawTower(COMBAT_WALL_DAMAGED);
                DrawWall();
                break;
            case COMBAT_WALL_COLLAPSING:
                DrawWall();
                break;
        }
    } else {
        gCombatManager->m_combatIcons[COMBAT_ICON_OBSTACLES]
            ->DrawToBuffer(m_x, m_y, m_obstacleIndex, ICON_DRAW_NORMAL, ICON_DRAW_OFFSET_FULL);
    }
}

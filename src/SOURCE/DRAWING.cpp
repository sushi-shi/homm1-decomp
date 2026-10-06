#include <H1/Ints.h>

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

void combatManager::UpdateGrid(i16 hex, i16 attributes) {
    i16 row;

    row = hex / COMBAT_GRID_COLUMNS - 1;
    if (row < 0)
        row = 0;
    if (row < m_gridUpdateRow)
        m_gridUpdateRow = row;
}

void combatManager::UpdateGridForMove(
    i16 hex,
    i8 direction,
    i16 attributes
) {
    if (direction == COMBAT_DIRECTION_NORTHEAST || direction == COMBAT_DIRECTION_NORTHWEST)
        UpdateGrid(hex - COMBAT_GRID_COLUMNS, attributes);
    else
        UpdateGrid(hex, attributes);
}

void combatManager::CombatMessage(char* text, b32 updateScreen) {
    b32 oldCompute;
    tag_message message;
    b32 prevLimit;

    SET_WIDGET_MESSAGE(message, WIDGET_COMMAND_SET_TEXT, COMBAT_STATUS_TEXT_CONTROL);
    message.text = text;
    m_combatWindow->BroadcastMessage(message);
    oldCompute = gComputeExtent;
    prevLimit = gLimitToExtent;
    gComputeExtent = gLimitToExtent = false;
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

// One strike of `count` attackers: the real damage formula without luck,
// for the weakest and the strongest roll.
void combatManager::EstimateDamage(
    army* attacker,
    i32 count,
    army* target,
    i32 ranged,
    i32* damageMin,
    i32* damageMax
) {
    i32 defenseModifier;
    i32 low;
    i32 high;

    defenseModifier = 0;
    if (ranged && attacker->ShotCrossesCastleWall(target))
        defenseModifier = ARMY_CASTLE_WALL_DEFENSE_BONUS;
    low = attacker->m_stats.damageMin;
    high = attacker->m_stats.damageMax;
    if (attacker->m_damageMode == ARMY_DAMAGE_MAXIMUM)
        low = high;
    else if (attacker->m_damageMode == ARMY_DAMAGE_MINIMUM)
        high = low;
    *damageMin = static_cast<i32>(
        attacker->ScaleDamage(target, static_cast<float>(count * low), ranged, defenseModifier)
        + 0.5
    );
    *damageMax = static_cast<i32>(
        attacker->ScaleDamage(target, static_cast<float>(count * high), ranged, defenseModifier)
        + 0.5
    );
    if (*damageMin > COMBAT_DAMAGE_MAX)
        *damageMin = COMBAT_DAMAGE_MAX;
    if (*damageMin <= 0)
        *damageMin = 1;
    if (*damageMax > COMBAT_DAMAGE_MAX)
        *damageMax = COMBAT_DAMAGE_MAX;
    if (*damageMax <= 0)
        *damageMax = 1;
}

// Damage taken by a stack as army::Damage applies it: the creatures killed,
// and the hit points the top creature has lost.
static i32 ForecastLosses(i32* quantity, i32* hitPointsLost, i32 hitPoints, i32 damage) {
    i32 killed;

    killed = (damage + *hitPointsLost) / hitPoints;
    if (killed > *quantity)
        killed = *quantity;
    *hitPointsLost = (damage + *hitPointsLost) % hitPoints;
    *quantity -= killed;
    return killed;
}

// The damage and kills of the attack the player is pointing at, from the
// weakest to the strongest outcome. Elves shoot twice, and wolves and
// paladins strike twice, the second time with the attackers that survive
// the retaliation.
void combatManager::ForecastAttack(
    army* attacker,
    army* target,
    i32 ranged,
    CombatForecast* forecast
) {
    i32 low;
    i32 high;
    i32 unused;
    i32 weakQuantity;
    i32 weakLost;
    i32 strongQuantity;
    i32 strongLost;
    i32 fewAttackers;
    i32 manyAttackers;
    i32 attackerLost;
    i32 retaliationMin;
    i32 retaliationMax;

    EstimateDamage(attacker, attacker->m_quantity, target, ranged, &low, &high);
    forecast->damageMin = low;
    forecast->damageMax = high;
    weakQuantity = strongQuantity = target->m_quantity;
    weakLost = strongLost = target->m_hitPointsLost;
    forecast->killsMin = ForecastLosses(&weakQuantity, &weakLost, target->m_stats.hitPoints, low);
    forecast->killsMax =
        ForecastLosses(&strongQuantity, &strongLost, target->m_stats.hitPoints, high);
    fewAttackers = manyAttackers = attacker->m_quantity;
    if (ranged) {
        if (attacker->m_creatureType != CREATURE_ELF || attacker->m_stats.shots <= 1)
            return;
    } else {
        if (attacker->m_creatureType != CREATURE_WOLF
            && attacker->m_creatureType != CREATURE_PALADIN)
            return;
        if (target->m_spellEffect != SPELL_PARALYZE
            && (target->m_creatureType == CREATURE_GRIFFIN
                || !(target->m_stats.attributes & MONSTER_FLAGS_RETALIATED))) {
            retaliationMin = retaliationMax = 0;
            if (weakQuantity > 0)
                EstimateDamage(target, weakQuantity, attacker, 0, &unused, &retaliationMax);
            if (strongQuantity > 0)
                EstimateDamage(target, strongQuantity, attacker, 0, &retaliationMin, &unused);
            attackerLost = attacker->m_hitPointsLost;
            ForecastLosses(
                &fewAttackers,
                &attackerLost,
                attacker->m_stats.hitPoints,
                retaliationMax
            );
            attackerLost = attacker->m_hitPointsLost;
            ForecastLosses(
                &manyAttackers,
                &attackerLost,
                attacker->m_stats.hitPoints,
                retaliationMin
            );
        }
    }
    if (weakQuantity > 0 && fewAttackers > 0) {
        EstimateDamage(attacker, fewAttackers, target, ranged, &low, &unused);
        forecast->damageMin += low;
        forecast->killsMin +=
            ForecastLosses(&weakQuantity, &weakLost, target->m_stats.hitPoints, low);
    }
    if (strongQuantity > 0 && manyAttackers > 0) {
        EstimateDamage(attacker, manyAttackers, target, ranged, &unused, &high);
        forecast->damageMax += high;
        forecast->killsMax +=
            ForecastLosses(&strongQuantity, &strongLost, target->m_stats.hitPoints, high);
    }
}

// Appends the hit points left on a stack's top creature and its full hit
// points to gText.
static void AppendHitPoints(army* stack) {
    if (gText[0])
        strcat(gText, "  |  ");
    sprintf(
        gText + strlen(gText),
        localization::Tr("combat.status.hit_points"),
        stack->m_stats.hitPoints - stack->m_hitPointsLost,
        stack->m_stats.hitPoints
    );
}

// A count's grammatical form, from the catalog's table by the count's last
// two digits: 0 for the "one" form, 1 for "few", 2 for "many".
static i32 CountForm(i32 count) {
    return localization::Tr("common.count_forms")[count % 100] - '0';
}

void combatManager::CombatMessage(i16 messageType) {
    army* currentArmy;
    army* targetArmy;
    army* hoveredArmy;
    i16 actingMonsterType;
    i16 targetMonsterType;
    CombatForecast forecast;

    currentArmy = &m_armies[m_currentSide][m_currentArmyIndex];
    actingMonsterType = currentArmy->m_creatureType;
    targetArmy = NULL;
    targetMonsterType = CREATURE_FIRST;
    if (currentArmy->m_targetSide >= COMBAT_SIDE_FIRST && currentArmy->m_targetIndex >= 0) {
        targetArmy = &m_armies[currentArmy->m_targetSide][currentArmy->m_targetIndex];
        targetMonsterType = targetArmy->m_creatureType;
    }
    hoveredArmy = NULL;
    if (m_selectedHex >= 0 && m_hexCells[m_selectedHex].m_occupantSide >= 0
        && m_hexCells[m_selectedHex].m_occupantIndex >= 0)
        hoveredArmy = &m_armies[m_hexCells[m_selectedHex].m_occupantSide]
                               [m_hexCells[m_selectedHex].m_occupantIndex];
    switch (messageType) {
        case COMBAT_MESSAGE_COMMAND_DEFAULT:
            if ((currentArmy->m_stats.attributes & MONSTER_FLAGS_SHOOTER)
                && currentArmy->m_stats.shots == 0 && (targetArmy || hoveredArmy))
                strcpy(gText, gCombatMessage[COMBAT_TEXT_NO_SHOTS]);
            else
                strcpy(gText, gCombatMessage[COMBAT_TEXT_NONE]);
            if (gConfig.battleMessageFormat != BATTLE_MESSAGE_CLASSIC && hoveredArmy)
                AppendHitPoints(hoveredArmy);
            break;
        case COMBAT_MESSAGE_COMMAND_MOVE:
            sprintf(gText, gCombatMessage[COMBAT_TEXT_MOVE], gArmyNames[actingMonsterType]);
            break;
        case COMBAT_MESSAGE_COMMAND_FLY:
            sprintf(gText, gCombatMessage[COMBAT_TEXT_FLY], gArmyNames[actingMonsterType]);
            break;
        case COMBAT_MESSAGE_COMMAND_ATTACK:
        case COMBAT_MESSAGE_COMMAND_SHOOT:
            if (targetArmy && gConfig.battleMessageFormat == BATTLE_MESSAGE_FORECAST) {
                ForecastAttack(
                    currentArmy,
                    targetArmy,
                    messageType == COMBAT_MESSAGE_COMMAND_SHOOT,
                    &forecast
                );
                gText[0] = 0;
                AppendHitPoints(targetArmy);
                strcat(gText, "  |  ");
                if (forecast.damageMin == forecast.damageMax)
                    sprintf(
                        gText + strlen(gText),
                        localization::Tr("combat.forecast.damage"),
                        forecast.damageMin
                    );
                else
                    sprintf(
                        gText + strlen(gText),
                        localization::Tr("combat.forecast.damage_range"),
                        forecast.damageMin,
                        forecast.damageMax
                    );
                if (forecast.killsMax > 0) {
                    strcat(gText, "  |  ");
                    if (forecast.killsMin == forecast.killsMax)
                        sprintf(
                            gText + strlen(gText),
                            localization::Tr("combat.forecast.kills"),
                            forecast.killsMin
                        );
                    else
                        sprintf(
                            gText + strlen(gText),
                            localization::Tr("combat.forecast.kills_range"),
                            forecast.killsMin,
                            forecast.killsMax
                        );
                }
                break;
            }
            if (messageType == COMBAT_MESSAGE_COMMAND_ATTACK)
                sprintf(
                    gText,
                    gCombatMessage[COMBAT_TEXT_ATTACK],
                    gArmyNamesPlural[targetMonsterType]
                );
            else
                sprintf(
                    gText,
                    CountForm(currentArmy->m_stats.shots) == 0
                        ? localization::Tr("combat.shoot.one")
                    : CountForm(currentArmy->m_stats.shots) == 1
                        ? localization::Tr("combat.shoot.few")
                        : gCombatMessage[COMBAT_TEXT_SHOOT],
                    gArmyNamesPlural[targetMonsterType],
                    currentArmy->m_stats.shots
                );
            if (targetArmy && gConfig.battleMessageFormat == BATTLE_MESSAGE_CLASSIC_PLUS)
                AppendHitPoints(targetArmy);
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
            if (actingMonsterType >= CREATURE_FIRST) {
                sprintf(
                    gText,
                    gCombatMessage[COMBAT_TEXT_VIEW_INFO],
                    gArmyNames[actingMonsterType]
                );
                if (gConfig.battleMessageFormat != BATTLE_MESSAGE_CLASSIC)
                    AppendHitPoints(
                        &m_armies[m_currentSide][m_hexCells[m_selectedHex].m_occupantIndex]
                    );
            } else
                sprintf(gText, "");
            break;
    }
    CombatMessage(gText, true);
}

void combatManager::ResetLimitCreature(void) {
    i32 j;
    i32 side;

    m_computeExtent = true;
    m_extendLimitDown = false;
    for (side = COMBAT_SIDE_FIRST; side < COMBAT_SIDE_COUNT; side++) {
        // The slots past the side's stacks hold no army (uninitialised in
        // the first battle); they are hidden like dead stacks, which is how
        // they are drawn either way.
        for (j = 0; j < ARMY_GROUP_SLOT_COUNT; j++) {
            m_limitCreatureCount[side][j] =
                j >= m_numArmies[side] || (m_armies[side][j].m_stats.attributes & MONSTER_FLAGS_DEAD)
                    ? COMBAT_LIMIT_CREATURE_HIDDEN
                    : 0;
        }
    }
}

void combatManager::SetDrawRightToLeft(i8 rightToLeft) {
    m_drawRightToLeft = rightToLeft;
}

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

void combatManager::DrawFrame(b8 updateScreen) {
    i16 col;
    b8 anyLimited;
    i32 armyRight;
    i32 armyTop;
    i32 i;
    i32 rearDelta;
    i32 side;
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
        gLimitToExtent = true;
        gComputeExtent = true;
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
                    (gGame->m_players[m_playerId[COMBAT_DEFENDER_SIDE]].Color()) + 4,
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
                    (gGame->m_players[m_playerId[COMBAT_ATTACKER_SIDE]].Color()) + 4,
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
                    (gGame->m_players[m_playerId[COMBAT_ATTACKER_SIDE]].Color()) + 4,
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
                    (gGame->m_players[m_playerId[COMBAT_DEFENDER_SIDE]].Color()) + 4,
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
        gLimitToExtent = false;
        gComputeExtent = false;
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

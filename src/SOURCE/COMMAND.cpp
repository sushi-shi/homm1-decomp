// Located from HoMM2 Buka 2.1; PoL 2.0 supplies the VC4 declaration.

#include <match.h>

#include <BASE/bmap2.h>
#include <BASE/INPUTMGR_TYPES.h>
#include <BASE/Misc.h>
#include <H1/All.h>
#include <H1/KB.h>
#include <SOURCE/PATH.h>
#include <SOURCE/REMOTE.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// clang-format off
// cCombatHelp rows ProcessCombatMsg shows when the pointer is off the grid:
// over the auto-combat strip (left), the skip strip (right), or neither.
H1_ENUM_BEGIN(CombatHelpText)
    COMBAT_HELP_AUTO_COMBAT = 0,
    COMBAT_HELP_SKIP_UNIT = 1,
    COMBAT_HELP_NONE = 2
H1_ENUM_END(CombatHelpText)

// cBattleResults rows (DoVictory's win texts and DoLoseWindow's loss texts):
// the outcome lines, then the experience award with or without level-ups.
H1_ENUM_BEGIN(BattleResultText)
    BATTLE_RESULT_ENEMY_SURRENDERED = 0,
    BATTLE_RESULT_ENEMY_FLED = 1,
    BATTLE_RESULT_VICTORY = 2,
    BATTLE_RESULT_EXPERIENCE = 3,
    BATTLE_RESULT_HERO_SURRENDERS = 4,
    BATTLE_RESULT_HERO_FLEES = 5,
    BATTLE_RESULT_HERO_DEFEATED = 6,
    BATTLE_RESULT_FORCES_SURRENDER = 7,
    BATTLE_RESULT_FORCES_FLEE = 8,
    BATTLE_RESULT_FORCES_DEFEATED = 9,
    BATTLE_RESULT_EXPERIENCE_AND_LEVELS = 10
H1_ENUM_END(BattleResultText)

// Combat-window widget ids ProcessCombatMsg handles: the battlefield (0x40,
// Buka CombatControlId CONTROL_MAIN_BUTTON; ResetMouse hovers it), the button
// that stops grid selection and hides the pointer, and the skip-turn button
// that queues ACTION_SKIP_TURN.
H1_ENUM_BEGIN(CombatControlId)
    COMBAT_CONTROL_DISABLE_SELECTION = 2,
    COMBAT_CONTROL_SKIP_TURN = 8,
    COMBAT_CONTROL_FIELD = 0x40
H1_ENUM_END(CombatControlId)
// clang-format on

// Buka COMMAND.cpp Main; HoMM1 polls sound on the 75-tick timer and has no
// combat screen cycling or no-show mode.
VA(0x0040f2c0, 0x311)
short combatManager::Main(struct tag_message& message) {
    int result = MESSAGE_DISPATCH_CONSUME;
    army* thisArmy;
    CombatRemotePacket* packet;

    if (KBTickCount() > glTimers[COMBAT_FRAME_TIMER_SLOT]) {
        PollSound();
        glTimers[COMBAT_FRAME_TIMER_SLOT] = KBTickCount() + 0x4b;
    }
    CheckCastleAttack();
    if (CheckWin(&message))
        return MESSAGE_DISPATCH_FORWARD;
    packet = reinterpret_cast<CombatRemotePacket*>(GetRemoteData(1)); // API-forced: char* record.
    if (packet && packet->type == REMOTE_MESSAGE_RELIABLE) {
        switch (packet->command) {
            case REMOTE_COMMAND_COMBAT_ACTION:
                giNextAction = packet->nextAction;
                giNextActionExtra = packet->nextActionExtra;
                giNextActionGridIndex = packet->nextActionGridIndex;
                giNextActionGridIndex2 = packet->nextActionGridIndex2;
                goto processAction;
            case REMOTE_COMMAND_CHAT:
                PopNetBox(packet->text);
                break;
        }
    }
    if (!gbThisNetHasControl) {
        if (message.type == MESSAGE_KEY_DOWN) {
            switch (message.keyCode) {
                case INPUT_SCAN_F1:
                    PopNetBox(NULL);
                    break;
            }
        }
        return MESSAGE_DISPATCH_CONSUME;
    }
    thisArmy = &m_armies[m_currentSide][m_currentArmyIndex];
    if (thisArmy->m_spellEffect == SPELL_BERZERKER) {
        thisArmy->GoBerserk();
        if (CheckWin(&message))
            return MESSAGE_DISPATCH_FORWARD;
    }
    if (m_gridSelectionDisabled) {
        while (message.type != MESSAGE_KEY_DOWN && message.type != MESSAGE_LEFT_BUTTON_DOWN
               && message.type != MESSAGE_RIGHT_BUTTON_DOWN && message.type != MESSAGE_NONE)
            message = gpInputManager->GetEvent();
        if (message.type != MESSAGE_NONE) {
            m_gridSelectionDisabled = 0;
            gpMouseManager->ReallyShowPointer();
        }
    }
    CheckChangeSelector();
processAction:
    if (!giNextAction) {
        if (m_playerId[m_currentSide] == GAME_PLAYER_NONE || !gbThisNetHumanPlayer[m_playerId[m_currentSide]]
            || m_gridSelectionDisabled)
            CheckGetAIMove();
        else
            result = ProcessCombatMsg(message);
    }
    if (giNextAction)
        result = ProcessNextAction(message);
    return result;
}

// Buka COMMAND.cpp ValidHexToStandOn; HoMM1 rows are nine hexes wide and
// the edge columns are never standable.
VA(0x0040f5d1, 0xd6)
signed char combatManager::ValidHexToStandOn(int hex) {
    if (hex == -2)
        return 1;
    if (hex != ARMY_HEX_INVALID && hex % COMBAT_GRID_COLUMNS != COMBAT_GRID_LAST_COLUMN && hex % COMBAT_GRID_COLUMNS != 0
        && m_hexCells[hex].m_obstacleIndex == COMBAT_OBSTACLE_NONE
        && (m_hexCells[hex].m_occupantSide == COMBAT_SIDE_NONE
            || (m_hexCells[hex].m_occupantSide == m_currentSide
                && m_hexCells[hex].m_occupantIndex == m_currentArmyIndex)))
        return 1;
    else
        return 0;
}

// Buka COMMAND.cpp SetCombatDirections; HoMM1 reads the global adjacency
// table and keeps the 24-sector map as bytes.
VA(0x0040f6a7, 0x7e9)
void combatManager::SetCombatDirections(int targetHex) {
    int mapped;
    int targetSide;
    int numUnset;
    signed char hasPath[COMBAT_DIRECTION_COUNT];
    int after;
    int rear[COMBAT_DIRECTION_COUNT];
    int before;
    int outDir;
    int dir;
    int directionHexes[COMBAT_DIRECTION_COUNT];
    army* curArmy;
    int targetIndex;
    army* target;
    signed char canStand[COMBAT_DIRECTION_COUNT];

    curArmy = &m_armies[m_currentSide][m_currentArmyIndex];
    targetSide = curArmy->m_targetSide;
    targetIndex = curArmy->m_targetIndex;
    curArmy->m_targetSide = COMBAT_SIDE_NONE;
    curArmy->m_targetIndex = -1;
    target = &m_armies[targetSide][targetIndex];
    for (dir = 0; dir < COMBAT_DIRECTION_COUNT; dir++) {
        if (dir == COMBAT_DIRECTION_WIDE_WEST || dir == COMBAT_DIRECTION_WIDE_EAST) {
            if (curArmy->m_stats.attributes & MONSTER_FLAGS_WIDE) {
                if (curArmy->m_facing == ARMY_FACING_RIGHT) {
                    if (dir == COMBAT_DIRECTION_WIDE_WEST)
                        directionHexes[dir] =
                            gCombatAdjacency[targetHex][COMBAT_DIRECTION_NORTHWEST];
                    if (dir == COMBAT_DIRECTION_WIDE_EAST)
                        directionHexes[dir] =
                            gCombatAdjacency[targetHex][COMBAT_DIRECTION_SOUTHWEST];
                } else {
                    if (dir == COMBAT_DIRECTION_WIDE_WEST)
                        directionHexes[dir] =
                            gCombatAdjacency[targetHex][COMBAT_DIRECTION_NORTHEAST];
                    if (dir == COMBAT_DIRECTION_WIDE_EAST)
                        directionHexes[dir] =
                            gCombatAdjacency[targetHex][COMBAT_DIRECTION_SOUTHEAST];
                }
            } else
                directionHexes[dir] = -1;
        } else
            directionHexes[dir] = gCombatAdjacency[targetHex][dir];
        if ((curArmy->m_stats.attributes & MONSTER_FLAGS_WIDE) && directionHexes[dir] != -1) {
            if (curArmy->m_facing == ARMY_FACING_RIGHT) {
                if (dir == COMBAT_DIRECTION_NORTHWEST || dir == COMBAT_DIRECTION_WEST
                    || dir == COMBAT_DIRECTION_SOUTHWEST) {
                    if (directionHexes[dir] % COMBAT_GRID_COLUMNS == 1)
                        directionHexes[dir] = -1;
                    else
                        directionHexes[dir]--;
                }
                if (directionHexes[dir] % COMBAT_GRID_COLUMNS == 7)
                    rear[dir] = -1;
                else
                    rear[dir] = directionHexes[dir] + 1;
            } else {
                if (dir == COMBAT_DIRECTION_NORTHEAST || dir == COMBAT_DIRECTION_EAST
                    || dir == COMBAT_DIRECTION_SOUTHEAST) {
                    if (directionHexes[dir] % COMBAT_GRID_COLUMNS == 7)
                        directionHexes[dir] = -1;
                    else
                        directionHexes[dir]++;
                }
                if (directionHexes[dir] % COMBAT_GRID_COLUMNS == 1)
                    rear[dir] = -1;
                else
                    rear[dir] = directionHexes[dir] - 1;
            }
        } else
            rear[dir] = -2;
        if (ValidHexToStandOn(directionHexes[dir]) && ValidHexToStandOn(rear[dir]))
            canStand[dir] = 1;
        else
            canStand[dir] = 0;
    }
    if (curArmy->m_stats.attributes & MONSTER_FLAGS_FLYING) {
        for (dir = 0; dir < COMBAT_DIRECTION_COUNT; dir++)
            hasPath[dir] = canStand[dir];
    } else {
        for (dir = 0; dir < COMBAT_DIRECTION_COUNT; dir++) {
            if (canStand[dir]) {
                if (curArmy->m_hex == directionHexes[dir]
                    || curArmy->ValidPath(directionHexes[dir], ARMY_PATH_EXACT_TARGET_HEX))
                    hasPath[dir] = 1;
                else
                    hasPath[dir] = 0;
            } else
                hasPath[dir] = 0;
        }
    }
    m_validDirectionCount = 0;
    for (dir = 0; dir < COMBAT_DIRECTION_COUNT; dir++) {
        if (hasPath[dir])
            m_validDirectionCount++;
    }
    if (!m_validDirectionCount)
        hasPath[6] = 1;
    memset(m_directionMap, -1, sizeof(m_directionMap));
    for (dir = 0; dir < COMBAT_DIRECTION_COUNT; dir++) {
        outDir = dir;
        if (dir < COMBAT_DIRECTION_ADJACENT_COUNT)
            mapped = (dir + COMBAT_DIRECTION_OPPOSITE_OFFSET) % COMBAT_DIRECTION_ADJACENT_COUNT;
        else
            mapped = static_cast<signed char>(
                dir == COMBAT_DIRECTION_WIDE_WEST ? COMBAT_DIRECTION_WIDE_EAST
                                                  : COMBAT_DIRECTION_WIDE_WEST
            );
        if (hasPath[mapped]) {
            if (target->m_stats.attributes & MONSTER_FLAGS_WIDE) {
                if (dir == COMBAT_DIRECTION_NORTHEAST
                    && m_hexCells[targetHex - 1].m_occupantSide == targetSide
                    && m_hexCells[targetHex - 1].m_occupantIndex == targetIndex)
                    outDir = COMBAT_DIRECTION_WIDE_WEST;
                else if (dir == COMBAT_DIRECTION_NORTHWEST
                         && m_hexCells[targetHex + 1].m_occupantSide == targetSide
                         && m_hexCells[targetHex + 1].m_occupantIndex == targetIndex)
                    outDir = COMBAT_DIRECTION_WIDE_WEST;
                else if (dir == COMBAT_DIRECTION_SOUTHEAST
                         && m_hexCells[targetHex - 1].m_occupantSide == targetSide
                         && m_hexCells[targetHex - 1].m_occupantIndex == targetIndex)
                    outDir = COMBAT_DIRECTION_WIDE_EAST;
                else if (dir == COMBAT_DIRECTION_SOUTHWEST
                         && m_hexCells[targetHex + 1].m_occupantSide == targetSide
                         && m_hexCells[targetHex + 1].m_occupantIndex == targetIndex)
                    outDir = COMBAT_DIRECTION_WIDE_EAST;
            }
            if (dir < COMBAT_DIRECTION_ADJACENT_COUNT)
                memset(&m_directionMap[mapped * 4], outDir, 4);
            else if (dir == COMBAT_DIRECTION_WIDE_WEST) {
                m_directionMap[11] = outDir;
                m_directionMap[12] = outDir;
                m_directionMap[13] = outDir;
            } else {
                m_directionMap[0] = outDir;
                m_directionMap[1] = outDir;
                m_directionMap[23] = outDir;
            }
        }
    }
    numUnset = 24;
    while (numUnset > 0) {
        for (dir = 0; dir < 24; dir++) {
            if (m_directionMap[dir] == -1) {
                after = (dir + 1) % 24;
                before = (dir + 23) % 24;
                if (m_directionMap[after] >= 0 && m_directionMap[after] <= 7)
                    m_directionMap[dir] = m_directionMap[after] + 10;
                else if (m_directionMap[before] >= 0 && m_directionMap[before] <= 7)
                    m_directionMap[dir] = m_directionMap[before] + 10;
            }
        }
        numUnset = 0;
        for (dir = 0; dir < 24; dir++) {
            if (m_directionMap[dir] >= 10)
                m_directionMap[dir] -= 10;
            else if (m_directionMap[dir] == -1)
                numUnset++;
        }
    }
    curArmy->m_targetSide = targetSide;
    curArmy->m_targetIndex = targetIndex;
}

// Buka COMMAND.cpp CheckSetMouseDirection; HoMM1 hexes are 78 by 80 and
// odd rows shift by 66 pixels.
VA(0x0040fe90, 0x620)
void combatManager::CheckSetMouseDirection(int mouseX, int mouseY, int targetHex) {
    int hexDir;
    int savedDir;
    int alternate;
    float ratio;
    int index;
    army* curArmy;
    int distX;
    int distY;
    army* target;
    int backHex;

    if (m_gridSelectionDisabled)
        return;
    if (m_validDirectionCount <= 1 && m_mouseDirection >= 0)
        return;
    distX = mouseX - (targetHex % COMBAT_GRID_COLUMNS - 1) * 78;
    if ((targetHex / COMBAT_GRID_COLUMNS) & 1)
        distX -= 0x42;
    else
        distX -= 0x1b;
    distY = mouseY - 0x3c - targetHex / COMBAT_GRID_COLUMNS * 80;
    distX -= 0x27;
    distY -= 0x28;
    index = 0;
    if (distX < 0) {
        if (distY < 0)
            index += 18;
        else
            index += 12;
    } else {
        if (distY < 0)
            index += 0;
        else
            index += 6;
    }
    distX = abs(distX);
    distY = abs(distY);
    ratio = static_cast<float>(distX) / (static_cast<float>(distY));
    if (index == 0 || index == 12) {
        if (ratio > 3.73)
            index += 5;
        else if (ratio > 1.73)
            index += 4;
        else if (ratio > 1.0f)
            index += 3;
        else if (ratio > 0.58)
            index += 2;
        else if (ratio > 0.27)
            index++;
    } else {
        if (ratio < 0.27)
            index += 5;
        else if (ratio < 0.58)
            index += 4;
        else if (ratio < 1.0f)
            index += 3;
        else if (ratio < 1.73)
            index += 2;
        else if (ratio < 3.73)
            index++;
    }
    if (m_directionMap[index] == m_mouseDirection)
        return;
    m_mouseDirection = m_directionMap[index];
    hexDir = OppositeDirection(m_directionMap[index]);
    savedDir = hexDir;
    alternate = COMBAT_DIRECTION_INVALID;
    curArmy = &m_armies[m_currentSide][m_currentArmyIndex];
    target = &m_armies[curArmy->m_targetSide][curArmy->m_targetIndex];
    if (hexDir == COMBAT_DIRECTION_WIDE_WEST || hexDir == COMBAT_DIRECTION_WIDE_EAST) {
        if (curArmy->m_stats.attributes & MONSTER_FLAGS_WIDE) {
            if (curArmy->m_facing == ARMY_FACING_RIGHT && hexDir == COMBAT_DIRECTION_WIDE_WEST) {
                hexDir = COMBAT_DIRECTION_NORTHWEST;
                alternate = COMBAT_DIRECTION_NORTHEAST;
            } else if (curArmy->m_facing == ARMY_FACING_RIGHT
                       && hexDir == COMBAT_DIRECTION_WIDE_EAST) {
                hexDir = COMBAT_DIRECTION_SOUTHWEST;
                alternate = COMBAT_DIRECTION_SOUTHEAST;
            } else if (curArmy->m_facing == ARMY_FACING_LEFT
                       && hexDir == COMBAT_DIRECTION_WIDE_WEST) {
                hexDir = COMBAT_DIRECTION_NORTHEAST;
                alternate = COMBAT_DIRECTION_NORTHWEST;
            } else {
                hexDir = COMBAT_DIRECTION_SOUTHEAST;
                alternate = COMBAT_DIRECTION_SOUTHWEST;
            }
        } else {
            if (m_hexCells[targetHex - 1].m_occupantSide == curArmy->m_targetSide
                && m_hexCells[targetHex - 1].m_occupantIndex == curArmy->m_targetIndex)
                targetHex--;
            if (hexDir == COMBAT_DIRECTION_WIDE_WEST)
                hexDir = COMBAT_DIRECTION_NORTHEAST;
            else
                hexDir = COMBAT_DIRECTION_SOUTHEAST;
        }
    } else {
        if (curArmy->m_facing == ARMY_FACING_RIGHT
            && (curArmy->m_stats.attributes & MONSTER_FLAGS_WIDE)) {
            if (hexDir == COMBAT_DIRECTION_NORTHWEST || hexDir == COMBAT_DIRECTION_WEST
                || hexDir == COMBAT_DIRECTION_SOUTHWEST)
                targetHex--;
        } else if (curArmy->m_facing == ARMY_FACING_LEFT
                   && (curArmy->m_stats.attributes & MONSTER_FLAGS_WIDE)
                   && (hexDir == COMBAT_DIRECTION_NORTHEAST || hexDir == COMBAT_DIRECTION_EAST
                       || hexDir == COMBAT_DIRECTION_SOUTHEAST))
            targetHex++;
    }
    m_directionTargetHex = gCombatAdjacency[targetHex][hexDir];
    backHex = -2;
    if (curArmy->m_facing == ARMY_FACING_LEFT && (curArmy->m_stats.attributes & MONSTER_FLAGS_WIDE))
        backHex = m_directionTargetHex - 1;
    if (curArmy->m_facing == ARMY_FACING_RIGHT
        && (curArmy->m_stats.attributes & MONSTER_FLAGS_WIDE))
        backHex = m_directionTargetHex + 1;
    if (!ValidHexToStandOn(m_directionTargetHex) || !ValidHexToStandOn(backHex)) {
        if ((curArmy->m_stats.attributes & MONSTER_FLAGS_WIDE)
            && (savedDir == COMBAT_DIRECTION_WIDE_WEST || savedDir == COMBAT_DIRECTION_WIDE_EAST)) {
            if (curArmy->m_facing == ARMY_FACING_RIGHT)
                m_directionTargetHex += 1;
            else
                m_directionTargetHex -= 1;
        } else {
            if (alternate != COMBAT_DIRECTION_INVALID)
                m_directionTargetHex = gCombatAdjacency[targetHex][alternate];
        }
    }
    gpMouseManager->SetPointer(m_mouseDirection + COMBAT_POINTER_ATTACK_FIRST);
}

// Buka GetPointer precedes ProcessCombatMsg. HoMM1's sole caller passes one
// command and retail maps command 13 to pointer 5, preserving all others.
VA(0x004104b0, 0x34)
H1_ENUM_RETURN(CombatPointerCode, int)
combatManager::GetPointer(H1_ENUM_PARAM(CombatMessageCommand, int) command) {
    if (command == COMBAT_MESSAGE_COMMAND_OPPOSING_OPTIONS)
        return COMBAT_POINTER_VIEW;
    else
        return command;
}

// Buka COMMAND.cpp ProcessCombatMsg; HoMM1 hovers the combat field as
// widget 0x40 and handles F1, space, H, T and C keys.
VA(0x004104e4, 0x5bb)
int combatManager::ProcessCombatMsg(struct tag_message& message) {
    short mouseX = message.x;
    short mouseY = message.y;
    signed char unused = 0;
    short selectedHex;

    if (!(m_messageTypeMask & message.type))
        return 0;
    switch (message.type) {
        case MESSAGE_WIDGET:
            switch (message.command) {
                case WIDGET_COMMAND_HOVER:
                    if (m_gridSelectionDisabled)
                        break;
                    switch (message.id) {
                        case COMBAT_CONTROL_FIELD:
                            gpMouseManager->MouseCoords(mouseX, mouseY);
                            selectedHex = GetGridIndex(mouseX, mouseY);
                            if (m_selectedHex != selectedHex || selectedHex == -1) {
                                m_selectedHex = selectedHex;
                                m_previousCommand = COMBAT_INVALID_COMMAND;
                                m_currentCommand = GetCommand(m_selectedHex);
                                m_mouseDirection = -1;
                                if (m_currentCommand == COMBAT_MESSAGE_COMMAND_ATTACK) {
                                    SetCombatDirections(selectedHex);
                                    CheckSetMouseDirection(mouseX, mouseY, selectedHex);
                                } else
                                    gpMouseManager->SetPointer(GetPointer(m_currentCommand));
                            } else if (m_currentCommand == COMBAT_MESSAGE_COMMAND_ATTACK)
                                CheckSetMouseDirection(mouseX, mouseY, selectedHex);
                            if (m_previousCommand != m_currentCommand) {
                                m_previousCommand = m_currentCommand;
                                CombatMessage(m_currentCommand);
                            }
                            break;
                        default:
                            gpMouseManager->MouseCoords(mouseX, mouseY);
                            if (mouseX <= 0x32)
                                CombatMessage(cCombatHelp[COMBAT_HELP_AUTO_COMBAT], 1);
                            else if (mouseX >= 0x24e)
                                CombatMessage(cCombatHelp[COMBAT_HELP_SKIP_UNIT], 1);
                            else
                                CombatMessage(cCombatHelp[COMBAT_HELP_NONE], 1);
                            gpMouseManager->SetPointer(COMBAT_POINTER_DEFAULT);
                            m_selectedHex = -1;
                            m_previousCommand = COMBAT_INVALID_COMMAND;
                            break;
                    }
                    break;
                case WIDGET_NOTIFY_SELECT:
                    if (message.modifiers & MESSAGE_MODIFIER_RIGHT_BUTTON)
                        RightClick(m_selectedHex);
                    else {
                        switch (message.id) {
                            case COMBAT_CONTROL_FIELD:
                                DoCommand(m_currentCommand);
                                break;
                        }
                    }
                    break;
                case WIDGET_NOTIFY_DESELECT:
                    switch (message.id) {
                        case COMBAT_CONTROL_DISABLE_SELECTION:
                            if (!(message.modifiers & MESSAGE_MODIFIER_RIGHT_BUTTON)) {
                                m_gridSelectionDisabled = 1;
                                gpMouseManager->ReallyHidePointer();
                            }
                            break;
                        case COMBAT_CONTROL_SKIP_TURN:
                            if (!(message.modifiers & MESSAGE_MODIFIER_RIGHT_BUTTON))
                                giNextAction = ACTION_SKIP_TURN;
                            break;
                    }
                    break;
            }
            break;
        case MESSAGE_KEY_DOWN:
            switch (message.keyCode) {
                case INPUT_SCAN_F1:
                    PopNetBox(NULL);
                    break;
                case INPUT_SCAN_SPACE:
                    giNextAction = ACTION_SKIP_TURN;
                    break;
                case INPUT_SCAN_H:
                    if (m_heroes[m_currentSide]) {
                        gpMouseManager->SetPointer(COMBAT_POINTER_DEFAULT);
                        ViewGeneral(m_currentSide, 1, 0);
                        ResetMouse();
                    }
                    break;
                case INPUT_SCAN_T:
                    gpMouseManager->SetPointer(COMBAT_POINTER_DEFAULT);
                    ViewArmy(&m_armies[m_currentSide][m_currentArmyIndex], m_currentSide, 0);
                    ResetMouse();
                    break;
                case INPUT_SCAN_C:
                    if (!m_heroes[m_currentSide]) {
                        NormalDialog(
                            "You have no hero to cast a spell.",
                            NORMAL_DIALOG_TYPE_OK,
                            -1,
                            -1,
                            NORMAL_DIALOG_NO_RESOURCE,
                            0,
                            NORMAL_DIALOG_NO_RESOURCE,
                            0,
                            NORMAL_DIALOG_NO_OR_TEXT
                        );
                        break;
                    }
                    if (m_heroCastSpell[m_currentSide]) {
                        NormalDialog(
                            "You have already cast a spell this round.",
                            NORMAL_DIALOG_TYPE_OK,
                            -1,
                            -1,
                            NORMAL_DIALOG_NO_RESOURCE,
                            0,
                            NORMAL_DIALOG_NO_RESOURCE,
                            0,
                            NORMAL_DIALOG_NO_OR_TEXT
                        );
                        break;
                    }
                    gpMouseManager->SetPointer(COMBAT_POINTER_DEFAULT);
                    giCurGeneral = m_currentSide;
                    ViewSpells(0);
                    ResetMouse();
                    break;
            }
            break;
    }
    return 1;
}

// Buka COMMAND.cpp ResetRound; HoMM1 has five stacks a side, one keep and
// a byte spell-round counter.
VA(0x00410a9f, 0x139)
void combatManager::ResetRound(void) {
    int unusedRoundWord;
    int index;
    int side;
    army* curArmy;

    m_catapultAttacksRemaining[COMBAT_ATTACKER_SIDE] = m_catapultAttackCount[COMBAT_ATTACKER_SIDE];
    m_catapultAttacksRemaining[COMBAT_DEFENDER_SIDE] = m_catapultAttackCount[COMBAT_DEFENDER_SIDE];
    m_keepAttacksRemaining[COMBAT_ATTACKER_SIDE] = 1;
    m_keepAttacksRemaining[COMBAT_DEFENDER_SIDE] = 1;
    m_heroCastSpell[COMBAT_ATTACKER_SIDE] = m_heroCastSpell[COMBAT_DEFENDER_SIDE] = 0;
    for (side = 0; side < COMBAT_SIDE_COUNT; side++) {
        for (index = 0; index < ARMY_GROUP_SLOT_COUNT; index++) {
            curArmy = &m_armies[side][index];
            if (curArmy->m_quantity > 0) {
                curArmy->m_stats.attributes &= MONSTER_FLAGS_ROUND_PERSISTENT_MASK;
                if (curArmy->m_creatureType == CREATURE_TROLL)
                    curArmy->m_hitPointsLost = 0;
                if (curArmy->m_spellRounds > 0) {
                    curArmy->m_spellRounds--;
                    if (curArmy->m_spellRounds == 0)
                        curArmy->CancelSpell();
                }
            }
        }
    }
    m_currentSpeed = CREATURE_SPEED_BLAZING;
}

// Buka COMMAND.cpp CheckWin; HoMM1 returns the byte flag and names the
// winning side directly (-1 for a draw).
VA(0x00410bd8, 0x15d)
int combatManager::CheckWin(struct tag_message* message) {
    int armyIndex;
    signed char combatEnded;
    int unusedWinWord;

    combatEnded = 0;
    if (IsWinner(m_currentSide)) {
        combatEnded = 1;
        if (IsWinner(1 - m_currentSide))
            m_combatResult = COMBAT_RESULT_DRAW;
        else
            m_combatResult = m_currentSide;
    } else if (IsWinner(1 - m_currentSide)) {
        combatEnded = 1;
        m_combatResult = 1 - m_currentSide;
    } else if (m_sideRetreated[COMBAT_ATTACKER_SIDE] || m_sideRetreated[COMBAT_DEFENDER_SIDE]) {
        combatEnded = 1;
        gbRetreatWin = 1;
        if (m_sideRetreated[COMBAT_ATTACKER_SIDE])
            m_combatResult = COMBAT_RESULT_DEFENDER;
        else
            m_combatResult = COMBAT_RESULT_ATTACKER;
    }
    if (combatEnded) {
        DoVictory(m_combatResult);
        message->type = MESSAGE_EXECUTIVE;
        message->executiveCommand = EXECUTIVE_COMMAND_TERMINATE_LOOP;
    }
    return combatEnded;
}

// Buka COMMAND.cpp GetCommand; HoMM1 returns each command directly, has no
// small view or ballista and clears the target through the current stack.
VA(0x00410d35, 0x316)
signed char combatManager::GetCommand(short hex) {
    signed char unusedCol = hex % COMBAT_GRID_COLUMNS;
    signed char rowIndex = hex / COMBAT_GRID_COLUMNS;
    army* currentArmy;
    signed char targetIndex;
    signed char enemySide;

    if (hex == ARMY_HEX_INVALID)
        return COMBAT_MESSAGE_COMMAND_DEFAULT;
    switch (hex) {
        case COMBAT_DEFENDER_HERO_HEX:
            if (m_heroes[COMBAT_DEFENDER_SIDE]) {
                if (m_currentSide == COMBAT_DEFENDER_SIDE)
                    return COMBAT_MESSAGE_COMMAND_OPTIONS;
                else
                    return COMBAT_MESSAGE_COMMAND_OPPOSING_OPTIONS;
            } else
                return COMBAT_MESSAGE_COMMAND_DEFAULT;
        case COMBAT_ATTACKER_HERO_HEX:
            if (m_heroes[COMBAT_ATTACKER_SIDE]) {
                if (m_currentSide == COMBAT_ATTACKER_SIDE)
                    return COMBAT_MESSAGE_COMMAND_OPTIONS;
                else
                    return COMBAT_MESSAGE_COMMAND_OPPOSING_OPTIONS;
            } else
                return COMBAT_MESSAGE_COMMAND_DEFAULT;
        default:
            if (hex % COMBAT_GRID_COLUMNS == COMBAT_GRID_LAST_COLUMN)
                return COMBAT_MESSAGE_COMMAND_DEFAULT;
            enemySide = m_hexCells[hex].m_occupantSide;
            targetIndex = m_hexCells[hex].m_occupantIndex;
            currentArmy = &m_armies[m_currentSide][m_currentArmyIndex];
            currentArmy->m_targetSide = COMBAT_SIDE_NONE;
            currentArmy->m_targetIndex = -1;
            if (m_hexCells[hex].m_obstacleIndex != COMBAT_OBSTACLE_NONE)
                return COMBAT_MESSAGE_COMMAND_DEFAULT;
            else if (enemySide != COMBAT_SIDE_NONE) {
                switch (enemySide) {
                    case COMBAT_DEFENDER_SIDE:
                    case COMBAT_ATTACKER_SIDE:
                        if (m_currentSide == enemySide)
                            return COMBAT_MESSAGE_COMMAND_VIEW_INFO;
                        else {
                            currentArmy->m_targetSide = enemySide;
                            currentArmy->m_targetIndex = targetIndex;
                            if (currentArmy->m_stats.shots > 0
                                && currentArmy->GetAttackMask(currentArmy->m_hex, ARMY_ATTACK_TARGET_ENEMY, ARMY_HEX_INVALID)
                                       == COMBAT_ALL_DIRECTIONS_BLOCKED)
                                return COMBAT_MESSAGE_COMMAND_SHOOT;
                            if (currentArmy->ValidPath(hex, ARMY_PATH_EXACT_TARGET_HEX) == 1)
                                return COMBAT_MESSAGE_COMMAND_ATTACK;
                            else {
                                currentArmy->m_targetSide = COMBAT_SIDE_NONE;
                                currentArmy->m_targetIndex = -1;
                                return COMBAT_MESSAGE_COMMAND_DEFAULT;
                            }
                        }
                }
            } else {
                if (m_armies[m_currentSide][m_currentArmyIndex].ValidPath(hex, ARMY_PATH_ANY_TARGET_HEX) == 1)
                    return (m_armies[m_currentSide][m_currentArmyIndex].m_stats.attributes
                            & MONSTER_FLAGS_FLYING)
                               ? COMBAT_MESSAGE_COMMAND_FLY
                               : COMBAT_MESSAGE_COMMAND_MOVE;
            }
            break;
    }
    return COMBAT_MESSAGE_COMMAND_DEFAULT;
}

// Buka COMMAND.cpp RightClick; HoMM1 hero hexes are 26 and 9 and the
// army view also takes the side.
VA(0x0041104b, 0x1dc)
signed char combatManager::RightClick(signed char hex) {
    signed char unusedColumn = hex % COMBAT_GRID_COLUMNS;
    signed char row = hex / COMBAT_GRID_COLUMNS;

    if (hex == ARMY_HEX_INVALID)
        return 0;
    switch (hex) {
        case COMBAT_DEFENDER_HERO_HEX:
            if (m_heroes[COMBAT_DEFENDER_SIDE]) {
                ViewGeneral(COMBAT_DEFENDER_SIDE, 0, 1);
                ResetMouse();
            }
            return 0;
        case COMBAT_ATTACKER_HERO_HEX:
            if (m_heroes[COMBAT_ATTACKER_SIDE]) {
                ViewGeneral(COMBAT_ATTACKER_SIDE, 0, 1);
                ResetMouse();
            }
            return 0;
        default:
            if (hex % COMBAT_GRID_COLUMNS == COMBAT_GRID_LAST_COLUMN)
                return 0;
            signed char side = m_hexCells[hex].m_occupantSide;
            signed char armyIndex = m_hexCells[hex].m_occupantIndex;
            if (m_hexCells[hex].m_obstacleIndex != COMBAT_OBSTACLE_NONE)
                return 0;
            else if (side != COMBAT_SIDE_NONE) {
                switch (side) {
                    case COMBAT_DEFENDER_SIDE:
                    case COMBAT_ATTACKER_SIDE:
                        gpMouseManager->SetPointer(COMBAT_POINTER_DEFAULT);
                        ViewArmy(
                            &m_armies[side][m_hexCells[m_selectedHex].m_occupantIndex],
                            side,
                            1
                        );
                        ResetMouse();
                        return 0;
                }
            } else
                return 0;
            break;
    }
    return 0;
}

// Buka COMMAND.cpp DoCommand; HoMM1 has no ballista or negation sphere and
// views the army at the selected hex on the current side.
VA(0x00411227, 0x333)
void combatManager::DoCommand(signed char command) {
    int unusedValue1;
    int unusedValue2;
    army* currentArmy = &m_armies[m_currentSide][m_currentArmyIndex];

    switch (command) {
        case COMBAT_MESSAGE_COMMAND_DEFAULT:
            break;
        case COMBAT_MESSAGE_COMMAND_MOVE:
        case COMBAT_MESSAGE_COMMAND_FLY:
        case COMBAT_MESSAGE_COMMAND_SHOOT:
            giNextAction = ACTION_MOVE, giNextActionGridIndex = m_selectedHex;
            giNextActionExtra = -1;
            break;
        case COMBAT_MESSAGE_COMMAND_ATTACK:
            giNextActionGridIndex = m_selectedHex;
            if (m_playerId[m_currentSide] == GAME_PLAYER_NONE || !gbHumanPlayer[m_playerId[m_currentSide]]
                || m_gridSelectionDisabled) {
                giNextAction = ACTION_MOVE;
                giNextActionExtra = -1;
            } else {
                giNextAction = ACTION_ATTACK;
                giNextActionExtra = m_directionTargetHex;
            }
            break;
        case COMBAT_MESSAGE_COMMAND_OPTIONS:
            gpMouseManager->SetPointer(COMBAT_POINTER_DEFAULT);
            ViewGeneral(m_currentSide, 1, 0);
            ResetMouse();
            break;
        case COMBAT_MESSAGE_COMMAND_OPPOSING_OPTIONS:
            gpMouseManager->SetPointer(COMBAT_POINTER_DEFAULT);
            ViewGeneral(1 - m_currentSide, 1, 0);
            ResetMouse();
            break;
        case COMBAT_MESSAGE_COMMAND_VIEW_INFO:
            gpMouseManager->SetPointer(COMBAT_POINTER_DEFAULT);
            ViewArmy(
                &m_armies[m_currentSide][m_hexCells[m_selectedHex].m_occupantIndex],
                m_currentSide,
                0
            );
            ResetMouse();
            break;
        case COMBAT_MESSAGE_COMMAND_CAST_SPELL:
            ViewSpells(0);
            ResetMouse();
            break;
        case COMBAT_MESSAGE_COMMAND_RETREAT:
            NormalDialog(
                "Are you sure you want to retreat?",
                NORMAL_DIALOG_TYPE_YES_NO,
                0xc3,
                0x3c,
                NORMAL_DIALOG_NO_RESOURCE,
                0,
                NORMAL_DIALOG_NO_RESOURCE,
                0,
                NORMAL_DIALOG_NO_OR_TEXT
            );
            if (gpWindowManager->m_dialogResult == NORMAL_DIALOG_CONFIRM)
                giNextAction = ACTION_RETREAT;
            ResetMouse();
            break;
        case COMBAT_MESSAGE_COMMAND_SURRENDER:
            if (DoSurrender() == 1) {
                if (gpGame->m_players[m_playerId[m_currentSide]].m_resources[RESOURCE_GOLD]
                    < giSurrenderCost)
                    NormalDialog(
                        "You don't have enough gold!",
                        NORMAL_DIALOG_TYPE_OK,
                        -1,
                        -1,
                        NORMAL_DIALOG_NO_RESOURCE,
                        0,
                        NORMAL_DIALOG_NO_RESOURCE,
                        0,
                        NORMAL_DIALOG_NO_OR_TEXT
                    );
                else {
                    giNextAction = ACTION_SURRENDER;
                    giNextActionExtra = giSurrenderCost;
                }
            }
            ResetMouse();
            break;
    }
}

// Buka COMMAND.cpp WinCombatHandler; HoMM1 pages captured artifacts and
// cycles a single six-frame animation.
VA(0x0041155a, 0x1a3)
short WinCombatHandler(struct tag_message& message) {
    int finalDelay = 0x5a;
    short frame = 1;

    if (message.type == MESSAGE_WIDGET) {
        switch (message.command) {
            case WIDGET_NOTIFY_DESELECT:
                switch (message.id) {
                    case DIALOG_BUTTON_0:
                        if (iMaxTransferArtifacts > iCurTransferArtifact + 1) {
                            gpCombatManager->ClearWinLoseBottom(gpCombatManager->m_winLoseWindow);
                            iCurTransferArtifact++;
                            gpCombatManager->ShowWinLoseArtifact(
                                gpCombatManager->m_winLoseWindow,
                                iTransferArtifacts[iCurTransferArtifact]
                            );
                        } else {
                            gpWindowManager->m_dialogResult = message.id;
                            message.command = message.id = 10;
                            return MESSAGE_DISPATCH_FORWARD;
                        }
                        break;
                    default:
                        break;
                }
                break;
            default:
                break;
        }
    }
    if (KBTickCount() > glTimers[COMBAT_FRAME_TIMER_SLOT]) {
        message.type = MESSAGE_WIDGET;
        message.command = WIDGET_COMMAND_SET_FRAME;
        message.id = 1;
        gpGame->m_viewArmyResult++;
        message.value = gpGame->m_viewArmyResult % 6 + 1;
        gpCombatManager->m_winLoseWindow->BroadcastMessage(message);
        gpCombatManager->m_winLoseWindow->DrawWindow();
        glTimers[COMBAT_FRAME_TIMER_SLOT] = KBTickCount() + 0x5a;
    }
    return MESSAGE_DISPATCH_CONSUME;
}

// Buka COMMAND.cpp ClearWinLoseBottom (fifteen icon/text widget pairs).
VA(0x004116fd, 0x110)
void combatManager::ClearWinLoseBottom(class heroWindow* window) {
    int i;

    for (i = 0; i < 15; i++) {
        if (m_winLoseBottomWidgets[i]) {
            window->RemoveWidget(m_winLoseBottomWidgets[i]);
            delete m_winLoseBottomWidgets[i];
        }
        if (m_winLoseBottomTextWidgets[i]) {
            window->RemoveWidget(m_winLoseBottomTextWidgets[i]);
            delete m_winLoseBottomTextWidgets[i];
        }
        m_winLoseBottomWidgets[i] = NULL;
        m_winLoseBottomTextWidgets[i] = NULL;
    }
}

// Buka COMMAND.cpp ShowWinLoseArtifact.
VA(0x0041180d, 0x2fa)
void combatManager::ShowWinLoseArtifact(class heroWindow* window, int artifact) {
    char* artifactName;
    short boxWidth = 0x140;
    short bottom = 0x1ca;
    tag_message message;

    sprintf(gText, "You have captured an enemy artifact!");
    message.type = MESSAGE_WIDGET;
    message.command = WIDGET_COMMAND_SET_TEXT;
    message.id = 0x65;
    message.text = gText;
    m_winLoseWindow->BroadcastMessage(message);
    m_winLoseBottomWidgets[0] = new iconWidget(
        0x78,
        0x136,
        0x50,
        0x50,
        "winloseb.icn",
        0,
        ICON_DRAW_NORMAL,
        0x7d1,
        ICON_WIDGET_DRAW,
        1
    );
    if (m_winLoseBottomWidgets[0] == NULL)
        MemError();
    window->AddWidget(m_winLoseBottomWidgets[0], WINDOW_Z_ORDER_APPEND);
    m_winLoseBottomWidgets[1] = new iconWidget(
        0x80,
        0x13e,
        0x40,
        0x40,
        "artifact.icn",
        artifact,
        ICON_DRAW_NORMAL,
        0x7d2,
        ICON_WIDGET_DRAW,
        1
    );
    if (m_winLoseBottomWidgets[1] == NULL)
        MemError();
    window->AddWidget(m_winLoseBottomWidgets[1], WINDOW_Z_ORDER_APPEND);
    artifactName = static_cast<char*>(malloc(0x3c));
    sprintf(artifactName, gArtifactNames[artifact]);
    m_winLoseBottomTextWidgets[0] = new textWidget(
        0,
        0x18a,
        0x140,
        0xc,
        artifactName,
        "smalfont.fnt",
        1,
        0x835,
        WIDGET_KIND_TEXT
    );
    if (m_winLoseBottomTextWidgets[0] == NULL)
        MemError();
    window->AddWidget(m_winLoseBottomTextWidgets[0], WINDOW_Z_ORDER_APPEND);
    gpCombatManager->m_winLoseWindow->DrawWindow();
    {
        SAMPLE2 sample = NULL_SAMPLE2;
        sprintf(gText, "pickup%02d.82M", SRandom(1, 5));
        sample = LoadPlaySample(gText);
        WaitEndSample(sample, SAMPLE_WAIT_DEFAULT);
    }
}

// Buka COMMAND.cpp ShowDeadArmies; HoMM1 lays out up to five casualties a
// side with fixed 40-pixel spacing.
VA(0x00411b07, 0x7d0)
void combatManager::ShowDeadArmies(class heroWindow* window) {
    int numLost[COMBAT_SIDE_COUNT];
    char* buffer;
    int casualtyType[COMBAT_SIDE_COUNT][ARMY_GROUP_SLOT_COUNT];
    int iconSpacing;
    int armyIndex;
    int side;
    short boxWidth = 0x140;
    short bottom = 0x1ca;
    int rowY;
    tag_message message;
    int casualtyCount[COMBAT_SIDE_COUNT][ARMY_GROUP_SLOT_COUNT];
    int firstX;

    for (side = 0; side < 15; side++) {
        m_winLoseBottomWidgets[side] = NULL;
        m_winLoseBottomTextWidgets[side] = NULL;
    }
    for (side = 0; side < COMBAT_SIDE_COUNT; side++) {
        numLost[side] = 0;
        for (armyIndex = 0; armyIndex < ARMY_GROUP_SLOT_COUNT; armyIndex++) {
            if (m_armies[side][armyIndex].m_creatureType != CREATURE_NONE
                && m_armies[side][armyIndex].m_initialQuantity
                       > m_armies[side][armyIndex].m_quantity) {
                casualtyType[side][numLost[side]] = m_armies[side][armyIndex].m_creatureType;
                casualtyCount[side][numLost[side]] = m_armies[side][armyIndex].m_initialQuantity
                                                     - m_armies[side][armyIndex].m_quantity;
                numLost[side]++;
            }
        }
    }
    buffer = static_cast<char*>(malloc(0x1e));
    sprintf(buffer, "Battlefield Casualties");
    m_winLoseBottomTextWidgets[12] =
        new textWidget(0, 0x104, 0x140, 0x14, buffer, "smalfont.fnt", 1, 0x83e, WIDGET_KIND_TEXT);
    if (m_winLoseBottomTextWidgets[12] == NULL)
        MemError();
    window->AddWidget(m_winLoseBottomTextWidgets[12], WINDOW_Z_ORDER_APPEND);
    for (side = 0; side < COMBAT_SIDE_COUNT; side++) {
        if (side == COMBAT_ATTACKER_SIDE)
            rowY = 0x118;
        else
            rowY = 0x159;
        buffer = static_cast<char*>(malloc(0x1e));
        sprintf(buffer, side == COMBAT_ATTACKER_SIDE ? "Attacker" : "Defender");
        m_winLoseBottomTextWidgets[10 + side] = new textWidget(
            0,
            rowY,
            0x140,
            0x14,
            buffer,
            "smalfont.fnt",
            1,
            0x83e,
            WIDGET_KIND_TEXT
        );
        if (m_winLoseBottomTextWidgets[10 + side] == NULL)
            MemError();
        window->AddWidget(m_winLoseBottomTextWidgets[10 + side], WINDOW_Z_ORDER_APPEND);
        if (numLost[side] <= 0) {
            buffer = static_cast<char*>(malloc(10));
            sprintf(buffer, "None");
            m_winLoseBottomTextWidgets[side * 5] = new textWidget(
                0,
                rowY + 0x12,
                0x140,
                0x14,
                buffer,
                "smalfont.fnt",
                1,
                side * 5 + 0x834,
                WIDGET_KIND_TEXT
            );
            if (m_winLoseBottomTextWidgets[side * 5] == NULL)
                MemError();
            window->AddWidget(m_winLoseBottomTextWidgets[side * 5], WINDOW_Z_ORDER_APPEND);
        }
        iconSpacing = 0x28;
        firstX = (0x140 - numLost[side] * iconSpacing) / 2 + 3;
        for (armyIndex = 0; armyIndex < numLost[side]; armyIndex++) {
            m_winLoseBottomWidgets[side * 5 + armyIndex] = new iconWidget(
                armyIndex * iconSpacing + firstX,
                rowY + 0xf,
                0x20,
                0x1c,
                "mons32.icn",
                casualtyType[side][armyIndex],
                ICON_DRAW_NORMAL,
                side * 5 + armyIndex + 0x7d0,
                ICON_WIDGET_DRAW,
                1
            );
            if (m_winLoseBottomWidgets[side * 5 + armyIndex] == NULL)
                MemError();
            buffer = static_cast<char*>(malloc(9));
            sprintf(buffer, "%d", casualtyCount[side][armyIndex]);
            m_winLoseBottomTextWidgets[side * 5 + armyIndex] = new textWidget(
                armyIndex * iconSpacing + firstX,
                rowY + 0x2e,
                0x20,
                0xc,
                buffer,
                "smalfont.fnt",
                1,
                side * 5 + armyIndex + 0x834,
                WIDGET_KIND_TEXT
            );
            if (m_winLoseBottomTextWidgets[side * 5 + armyIndex] == NULL)
                MemError();
            window->AddWidget(m_winLoseBottomWidgets[side * 5 + armyIndex], WINDOW_Z_ORDER_APPEND);
            window->AddWidget(m_winLoseBottomTextWidgets[side * 5 + armyIndex], WINDOW_Z_ORDER_APPEND);
        }
    }
}

// Buka COMMAND.cpp DoVictory; HoMM1 has no necromancy or eagle eye and
// grabs the screen instead of fading it.
VA(0x004122d7, 0x7a1)
void combatManager::DoVictory(signed char winningSide) {
    int levelsGained;
    tag_message message;
    int i;
    char expText[156];

    levelsGained = 0;
    iMaxTransferArtifacts = 0;
    iCurTransferArtifact = -1;
    FreeArmies();
    CombatMessage(" ", 1);
    GrabScreenBitmap(gpWindowManager->m_screen, 0, 0);
    gpMouseManager->SetPointer(COMBAT_POINTER_DEFAULT);
    switch (winningSide) {
        case COMBAT_SIDE_NONE:
            gpSoundManager->SwitchAmbientMusic(MUSIC_TRACK_BATTLE_LOST);
            DoLoseWindow();
            break;
        case COMBAT_DEFENDER_SIDE:
        case COMBAT_ATTACKER_SIDE:
            if (m_heroes[winningSide]) {
                m_experienceValue[1 - winningSide] = ExperienceValueOfStack(1 - winningSide);
                if (gbRetreatWin)
                    m_experienceValue[1 - winningSide] -= 500;
                if (m_combatTowns[COMBAT_DEFENDER_SIDE] && winningSide == COMBAT_ATTACKER_SIDE)
                    m_experienceValue[1 - winningSide] += 500;
                levelsGained = gpAdvManager->GiveExperience(
                    m_heroes[winningSide],
                    m_experienceValue[1 - winningSide],
                    !gbThisNetHumanPlayer[m_heroes[winningSide]->m_owner]
                );
                if (!gbRetreatWin && m_heroes[COMBAT_ATTACKER_SIDE]
                    && m_heroes[COMBAT_DEFENDER_SIDE]) {
                    for (i = 0; i < HERO_ARTIFACT_SLOT_COUNT; i++) {
                        if (m_heroes[1 - winningSide]->m_artifacts[i] >= ARTIFACT_REGULAR_FIRST
                            && m_heroes[1 - winningSide]->m_artifacts[i] != ARTIFACT_MAGIC_BOOK) {
                            iTransferArtifacts[iMaxTransferArtifacts] =
                                m_heroes[1 - winningSide]->m_artifacts[i];
                            iMaxTransferArtifacts++;
                        }
                    }
                }
            }
            if (!(giCurPlayer == GAME_PLAYER_NONE || !gbThisNetHumanPlayer[giCurPlayer]
                  || m_playerId[winningSide] != giCurPlayer)
                || !(
                    giCurPlayer == GAME_PLAYER_NONE || m_playerId[winningSide] == GAME_PLAYER_NONE
                    || gbThisNetHumanPlayer[giCurPlayer]
                    || !gbThisNetHumanPlayer[m_playerId[winningSide]]
                )
                || !(
                    m_playerId[winningSide] == GAME_PLAYER_NONE || !gbThisNetHumanPlayer[m_playerId[winningSide]]
                )) {
                gpSoundManager->SwitchAmbientMusic(MUSIC_TRACK_BATTLE_WON);
                m_winLoseWindow = new heroWindow(0x9f, 2, "wincmbt.bin");
                if (m_winLoseWindow == NULL)
                    MemError();
                if (m_heroes[winningSide]) {
                    if (gbCombatSurrender)
                        sprintf(gText, cBattleResults[BATTLE_RESULT_ENEMY_SURRENDERED]);
                    else if (gbRetreatWin)
                        sprintf(gText, cBattleResults[BATTLE_RESULT_ENEMY_FLED]);
                    else
                        sprintf(gText, cBattleResults[BATTLE_RESULT_VICTORY]);
                    if (levelsGained > 0 && winningSide == COMBAT_DEFENDER_SIDE
                        && giNumHumanPlayers > 1)
                        sprintf(
                            expText,
                            cBattleResults[BATTLE_RESULT_EXPERIENCE_AND_LEVELS],
                            m_heroes[winningSide]->m_name,
                            m_experienceValue[1 - winningSide],
                            levelsGained
                        );
                    else
                        sprintf(
                            expText,
                            cBattleResults[BATTLE_RESULT_EXPERIENCE],
                            m_heroes[winningSide]->m_name,
                            m_experienceValue[1 - winningSide]
                        );
                    strcat(gText, expText);
                    m_heroes[winningSide]->ApplyBattleWinTemps();
                } else {
                    if (gbCombatSurrender)
                        sprintf(gText, cBattleResults[BATTLE_RESULT_ENEMY_SURRENDERED]);
                    else if (gbRetreatWin)
                        sprintf(gText, cBattleResults[BATTLE_RESULT_ENEMY_FLED]);
                    else
                        sprintf(gText, cBattleResults[BATTLE_RESULT_VICTORY]);
                }
                message.type = MESSAGE_WIDGET;
                message.command = WIDGET_COMMAND_SET_TEXT;
                message.id = 0x65;
                message.text = gText;
                m_winLoseWindow->BroadcastMessage(message);
                ShowDeadArmies(m_winLoseWindow);
                gpWindowManager->DoDialog(m_winLoseWindow, WinCombatHandler, 0);
                delete m_winLoseWindow;
                if (m_heroes[1 - winningSide])
                    m_heroes[1 - winningSide]->ApplyBattleLossTemps();
            } else {
                if (m_heroes[winningSide])
                    m_heroes[winningSide]->ApplyBattleWinTemps();
                if (m_heroes[1 - winningSide])
                    m_heroes[1 - winningSide]->ApplyBattleLossTemps();
                gpSoundManager->SwitchAmbientMusic(MUSIC_TRACK_BATTLE_LOST);
                DoLoseWindow();
            }
            break;
    }
    gMapX = gpAdvManager->m_mapOriginX;
    gMapY = gpAdvManager->m_mapOriginY;
}

// Buka COMMAND.cpp DoLoseWindow; HoMM1 walks the defeated hero across a
// scrolling backdrop until the window's button is released.
VA(0x00412a78, 0x549)
void combatManager::DoLoseWindow(void) {
    short walkFrame;
    short lAnimY;
    short unusedWalkX;
    short unusedWalkY;
    tag_message message;
    heroWindow* loseWindow;
    bitmap* bmp;
    short result;
    int iDelay;
    short offset;
    int losingSide;
    short width;
    short animX;
    short blitHeight;
    short iAreaWidth;
    short stop;
    icon* walkIcon;

    iDelay = 0xb4;
    animX = 0x31;
    lAnimY = 0x26;
    iAreaWidth = 0xdf;
    blitHeight = 0x7d;
    width = 0x280;
    unusedWalkX = 0x6f;
    unusedWalkY = 0x8a;
    iMaxTransferArtifacts = 0;
    walkFrame = 0;
    offset = 0;
    stop = 0;
    if (m_playerId[COMBAT_ATTACKER_SIDE] == giCurPlayer
        && gbThisNetHumanPlayer[m_playerId[COMBAT_ATTACKER_SIDE]])
        losingSide = 1;
    else if (m_playerId[COMBAT_DEFENDER_SIDE] == giCurPlayer
             && gbThisNetHumanPlayer[m_playerId[COMBAT_DEFENDER_SIDE]])
        losingSide = 0;
    else if (m_playerId[COMBAT_ATTACKER_SIDE] != GAME_PLAYER_NONE
             && gbThisNetHumanPlayer[m_playerId[COMBAT_ATTACKER_SIDE]])
        losingSide = 1;
    else
        losingSide = 0;
    loseWindow = new heroWindow(0x9f, 2, "losecmbt.bin");
    if (loseWindow == NULL)
        MemError();
    bmp = gpResourceManager->GetBitmap("losecmbt.bmp");
    gbLoadingMonoIcon = 1;
    walkIcon = gpResourceManager->GetIcon("losewalk.icn");
    gbLoadingMonoIcon = 0;
    if (m_heroes[losingSide]) {
        if (gbCombatSurrender)
            sprintf(gText, cBattleResults[BATTLE_RESULT_HERO_SURRENDERS], m_heroes[losingSide]->m_name);
        else if (gbRetreatWin)
            sprintf(gText, cBattleResults[BATTLE_RESULT_HERO_FLEES], m_heroes[losingSide]->m_name);
        else
            sprintf(gText, cBattleResults[BATTLE_RESULT_HERO_DEFEATED], m_heroes[losingSide]->m_name);
    } else {
        if (gbCombatSurrender)
            sprintf(gText, cBattleResults[BATTLE_RESULT_FORCES_SURRENDER]);
        else if (gbRetreatWin)
            sprintf(gText, cBattleResults[BATTLE_RESULT_FORCES_FLEE]);
        else
            sprintf(gText, cBattleResults[BATTLE_RESULT_FORCES_DEFEATED]);
    }
    message.type = MESSAGE_WIDGET;
    message.command = WIDGET_COMMAND_SET_TEXT;
    message.id = 0x65;
    message.text = gText;
    loseWindow->BroadcastMessage(message);
    ShowDeadArmies(loseWindow);
    gpWindowManager->AddWindow(loseWindow, WINDOW_Z_ORDER_APPEND, 0);
    BlitBitmap(bmp, offset, 0, 0xdf, 0x7d, gpWindowManager->m_screen, 0xd0, 0x28);
    walkIcon->FillToBuffer(0x10e, 0x8c, walkFrame, 0, ICON_DRAW_NORMAL, 0);
    gpWindowManager->UpdateScreenRegion(0x9f, 2, 0x140, 0x1ca);
    glTimers[COMBAT_FRAME_TIMER_SLOT] = KBTickCount() + 0xb4;
    do {
        if (KBTickCount() > glTimers[COMBAT_FRAME_TIMER_SLOT]) {
            BlitBitmap(bmp, offset, 0, 0xdf, 0x7d, gpWindowManager->m_screen, 0xd0, 0x28);
            walkIcon->FillToBuffer(0x10e, 0x8c, walkFrame, 0, ICON_DRAW_NORMAL, 0);
            gpWindowManager->UpdateScreenRegion(0xd0, 0x28, 0xdf, 0x7d);
            walkFrame++;
            walkFrame = walkFrame % 8;
            offset = offset + 2;
            if (offset > 0x1a0)
                offset = 0;
            glTimers[COMBAT_FRAME_TIMER_SLOT] = KBTickCount() + 0xb4;
        }
        Process1WindowsMessage();
        message = gpInputManager->GetEvent();
        gpMouseManager->Main(message);
        result = gpWindowManager->Main(message);
        if (result == MESSAGE_DISPATCH_FORWARD && message.type == MESSAGE_WIDGET
            && message.command == WIDGET_NOTIFY_DESELECT && message.id == DIALOG_BUTTON_0)
            stop = 1;
    } while (!stop);
    gpWindowManager->RemoveWindow(loseWindow);
    delete loseWindow;
    gpResourceManager->Dispose(walkIcon);
    gpResourceManager->Dispose(bmp);
}

// Buka COMMAND.cpp DoSurrender; HoMM1 charges half the stack cost and has
// no quill or diplomacy discount.
VA(0x00412fc1, 0x2c6)
short combatManager::DoSurrender(void) {
    heroWindow* win;
    short unusedResult;
    int armyIndex;
    tag_message message;
    short unusedType;

    giSurrenderCost = 0;
    for (armyIndex = 0; armyIndex < ARMY_GROUP_SLOT_COUNT; armyIndex++) {
        if (m_armies[m_currentSide][armyIndex].IsAlive())
            giSurrenderCost +=
                gMonsterDatabase[m_armies[m_currentSide][armyIndex].m_creatureType].cost / 2
                * m_armies[m_currentSide][armyIndex].m_quantity;
    }
    unusedType = 1;
    unusedResult = 2;
    win = new heroWindow(0x55, 0x50, "surrendr.bin");
    if (win == NULL)
        MemError();
    message.type = MESSAGE_WIDGET;
    message.command = WIDGET_COMMAND_SET_ICON;
    message.id = 1;
    sprintf(gText, "port%04d.icn", m_heroes[1 - m_currentSide]->m_portrait);
    message.text = gText;
    win->BroadcastMessage(message);
    message.command = WIDGET_COMMAND_SET_TEXT;
    message.id = 2;
    sprintf(
        gText,
        "%s states:\n\n\"I will accept your surrender and grant you and your troops safe passage "
        "for the "
        "price of %d gold.",
        m_heroes[1 - m_currentSide]->m_name,
        giSurrenderCost
    );
    win->BroadcastMessage(message);
    gpWindowManager->DoDialog(win, TrueFalseDialogHandler, 0);
    delete win;
    return gpWindowManager->m_dialogResult == DIALOG_BUTTON_2;
}

// Buka COMMAND.cpp CheckChangeSelector; HoMM1 redraws the grid from the
// lower of the old and new selector hexes.
VA(0x00413287, 0xc2)
void combatManager::CheckChangeSelector(void) {
    army* currentArmy;

    currentArmy = &m_armies[m_currentSide][m_currentArmyIndex];
    if (!m_limitCreature || m_limitCreatureHex != currentArmy->m_hex) {
        UpdateGrid(
            m_limitCreatureHex > currentArmy->m_hex ? currentArmy->m_hex : m_limitCreatureHex,
            1
        );
        m_limitCreatureHex = currentArmy->m_hex;
        m_limitCreature = 1;
        DrawFrame(1);
    }
}

// Buka COMMAND.cpp CheckCastleAttack; HoMM1 keys both on the castle side.
VA(0x00413349, 0xdf)
void combatManager::CheckCastleAttack(void) {
    if (m_castleSide[1 - m_currentSide]) {
        while (m_catapultAttacksRemaining[m_currentSide] > 0) {
            CatAttack(m_currentSide);
            m_catapultAttacksRemaining[m_currentSide]--;
        }
    }
    if (m_castleSide[m_currentSide]) {
        while (m_keepAttacksRemaining[m_currentSide] > 0) {
            KeepAttack();
            m_keepAttacksRemaining[m_currentSide]--;
        }
    }
}

// Buka COMMAND.cpp CheckGetAIMove; HoMM1 tries the retreat first.
VA(0x00413428, 0x79)
void combatManager::CheckGetAIMove(void) {
    if (AICheckRetreat())
        return;
    if (!m_heroCastSpell[m_currentSide] && DoSpellAI(m_currentSide))
        return;
    DoCompAI(m_currentSide);
}

// Buka COMMAND.cpp GetControl; HoMM1 always resets the pointer and has no
// small view.
VA(0x004134a1, 0x16a)
void combatManager::GetControl(void) {
    m_selectedHex = -1;
    m_previousCommand = COMBAT_INVALID_COMMAND;
    m_previousCommand = COMBAT_INVALID_COMMAND;
    gpMouseManager->SetPointer(COMBAT_POINTER_DEFAULT);
    CheckChangeSelector();
    if (!gbRemoteOn || m_playerId[COMBAT_ATTACKER_SIDE] < 0 || m_playerId[COMBAT_DEFENDER_SIDE] < 0
        || !gbHumanPlayer[m_playerId[COMBAT_DEFENDER_SIDE]]
        || (!gbHumanPlayer[m_playerId[COMBAT_ATTACKER_SIDE]]
            && (gbHumanPlayer[m_playerId[COMBAT_ATTACKER_SIDE]]
                || !m_playerId[COMBAT_DEFENDER_SIDE]))) {
        gbThisNetHasControl = 1;
        goto resetMouse;
    }
    if (m_playerId[m_currentSide] != GAME_PLAYER_NONE && gbHumanPlayer[m_playerId[m_currentSide]]
        && !gbThisNetHumanPlayer[m_playerId[m_currentSide]])
        gbThisNetHasControl = 0;
    else
        gbThisNetHasControl = 1;
resetMouse:
    ResetMouse();
}

// Buka COMMAND.cpp ResetMouse; HoMM1 sends a hover over the combat field.
VA(0x0041360b, 0xdb)
void combatManager::ResetMouse(void) {
    tag_message message;
    short x;
    short y;

    if (gbThisNetHasControl && m_playerId[m_currentSide] >= 0
        && gbHumanPlayer[m_playerId[m_currentSide]]) {
        m_selectedHex = -1;
        CombatMessage("", 1);
        gpMouseManager->MouseCoords(x, y);
        message.type = MESSAGE_WIDGET;
        message.command = WIDGET_COMMAND_HOVER;
        if (y > 0x1ca)
            message.id = 0;
        else
            message.id = COMBAT_CONTROL_FIELD;
        ProcessCombatMsg(message);
    } else
        gpMouseManager->SetPointer(COMBAT_POINTER_DEFAULT);
}

// Buka COMMAND.cpp ProcessNextAction; HoMM1 hides the pointer around the
// action, broadcasts it to a human net opponent and has no door or cycling.
VA(0x004136e6, 0x552)
short combatManager::ProcessNextAction(struct tag_message& message) {
    army* actingArmy;
    signed char advance;
    int result;
    int data[4];

    if (giNextAction)
        LogStr(
            "Process Act",
            giNextAction,
            giNextActionGridIndex,
            giNextActionGridIndex2,
            giNextActionExtra,
            m_currentSide,
            m_currentArmyIndex,
            m_armies[m_currentSide][m_currentArmyIndex].m_hex
        );
    if (gbThisNetHasControl && gbRemoteOn && m_playerId[COMBAT_ATTACKER_SIDE] >= 0
        && m_playerId[COMBAT_DEFENDER_SIDE] >= 0 && gbHumanPlayer[m_playerId[COMBAT_DEFENDER_SIDE]]
        && gbHumanPlayer[m_playerId[COMBAT_ATTACKER_SIDE]]) {
        int netPos;

        netPos = m_playerId[1 - m_currentSide];
        if (netPos < 0 || !gbHumanPlayer[netPos])
            netPos = giHostGamePos;
        data[0] = giNextAction;
        data[1] = giNextActionExtra;
        data[2] = giNextActionGridIndex;
        data[3] = giNextActionGridIndex2;
        result = TransmitRemoteData(
            reinterpret_cast<char*>(data),
            netPos,
            sizeof(data),
            REMOTE_COMMAND_COMBAT_ACTION,
            1,
            1,
            REMOTE_MESSAGE_DEFAULT,
            1
        ); // API-forced: char* payload.
        if (!result)
            ShutDown(NULL);
    }
    actingArmy = &m_armies[m_currentSide][m_currentArmyIndex];
    advance = 0;
    if (CheckWin(&message))
        return MESSAGE_DISPATCH_FORWARD;
    switch (giNextAction) {
        case ACTION_NONE:
            break;
        case ACTION_CAST_SPELL:
            gpMouseManager->ReallyHidePointer();
            CastSpell(giNextActionExtra, giNextActionGridIndex, 0, giNextActionGridIndex2);
            if (m_armies[m_currentSide][m_currentArmyIndex].m_quantity <= 0)
                advance = 1;
            break;
        case ACTION_MOVE:
            gpMouseManager->ReallyHidePointer();
            actingArmy->MoveAttack(giNextActionGridIndex, 0);
            actingArmy->m_stats.attributes |= MONSTER_FLAGS_TURN_SPENT;
            if (CheckWin(&message))
                return MESSAGE_DISPATCH_FORWARD;
            CheckApplyGoodMorale(m_currentSide, m_currentArmyIndex);
            advance = 1;
            break;
        case ACTION_ATTACK:
            gpMouseManager->ReallyHidePointer();
            if (giNextActionExtra != -1 && actingArmy->m_hex != giNextActionExtra)
                actingArmy->MoveAttack(giNextActionExtra, 1);
            actingArmy->MoveAttack(giNextActionGridIndex, 0);
            actingArmy->m_stats.attributes |= MONSTER_FLAGS_TURN_SPENT;
            if (CheckWin(&message))
                return MESSAGE_DISPATCH_FORWARD;
            CheckApplyGoodMorale(m_currentSide, m_currentArmyIndex);
            advance = 1;
            break;
        case ACTION_RETREAT:
            m_sideRetreated[m_currentSide] = 1;
            gbRetreatWin = 1;
            break;
        case ACTION_SURRENDER:
            gbCombatSurrender = 1;
            gbRetreatWin = 1;
            m_sideDefeated[m_currentSide] = 1;
            gpGame->m_players[m_playerId[m_currentSide]].m_resources[RESOURCE_GOLD] -=
                giNextActionExtra;
            gpGame->m_players[m_playerId[1 - m_currentSide]].m_resources[RESOURCE_GOLD] +=
                giNextActionExtra;
            break;
        case ACTION_SKIP_TURN:
            actingArmy->m_stats.attributes |= MONSTER_FLAGS_TURN_SPENT;
            advance = 1;
            break;
    }
    giNextAction = ACTION_NONE;
    if (CheckWin(&message))
        return MESSAGE_DISPATCH_FORWARD;
    if (advance && !GetNextArmy(1)) {
        ResetRound();
        GetNextArmy(1);
    }
    CheckChangeSelector();
    if (gbThisNetHasControl && !m_gridSelectionDisabled && m_playerId[m_currentSide] != GAME_PLAYER_NONE
        && gbThisNetHumanPlayer[m_playerId[m_currentSide]])
        gpMouseManager->ReallyShowPointer();
    else
        gpMouseManager->ReallyHidePointer();
    return MESSAGE_DISPATCH_CONSUME;
}

// COMMAND owns retail .bss 0x004a4b98-0x004a4bc7.
DATA(0x004a4b98)
signed char gbThisNetHasControl;
DATA(0x004a4b9c)
int iCurTransferArtifact;
DATA(0x004a4ba0)
signed char iMaxTransferArtifacts;
DATA(0x004a4ba4)
int giNextActionExtra;
DATA(0x004a4ba8)
int giNextActionGridIndex;
DATA(0x004a4bac)
int giSurrenderCost;
DATA(0x004a4bb0)
signed char iTransferArtifacts[HERO_ARTIFACT_SLOT_COUNT];
DATA(0x004a4bc0)
int giNextAction;
DATA(0x004a4bc4)
int giNextActionGridIndex2;

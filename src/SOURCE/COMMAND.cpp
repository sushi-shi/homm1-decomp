// Located from HoMM2 Buka 2.1; PoL 2.0 supplies the VC4 declaration.

#include <match.h>

#include <BASE/audio.h>
#include <BASE/bitmap.h>
#include <BASE/bmap2.h>
#include <BASE/heroWindow.h>
#include <BASE/heroWindowManager.h>
#include <BASE/icon.h>
#include <BASE/iconWidget.h>
#include <BASE/inputManager.h>
#include <BASE/message.h>
#include <BASE/Misc.h>
#include <BASE/miscwin.h>
#include <BASE/mouseManager.h>
#include <BASE/resourceManager.h>
#include <BASE/sample.h>
#include <BASE/textWidget.h>
#include <BASE/widget.h>
#include <SOURCE/advManager.h>
#include <SOURCE/army.h>
#include <SOURCE/armyGroup.h>
#include <SOURCE/combatManager.h>
#include <SOURCE/game.h>
#include <SOURCE/hero.h>
#include <SOURCE/KB.h>
#include <SOURCE/kbwin.h>
#include <SOURCE/PATH.h>
#include <SOURCE/philAI.h>
#include <SOURCE/playerData.h>
#include <SOURCE/REMOTE.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// gCombatHelp rows ProcessCombatMsg shows when the pointer is off the grid:
// over the auto-combat strip (left), the skip strip (right), or neither.
H1_ENUM_BEGIN(CombatHelpText)
    COMBAT_HELP_AUTO_COMBAT = 0,
    COMBAT_HELP_SKIP_UNIT = 1,
    COMBAT_HELP_NONE = 2
H1_ENUM_END(CombatHelpText)

// gBattleResults rows (DoVictory's win texts and DoLoseWindow's loss texts):
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

// win/losecmbt.bin widget ids: the animation, the result text, and the
// bottom panel ShowWinLoseArtifact (captured artifact) or ShowDeadArmies
// (casualties: icon/count ids are FIRST + side * ARMY_GROUP_SLOT_COUNT + slot,
// with the count id doubling as the side's "None" line) fills in.
H1_ENUM_BEGIN(CombatWinLoseControl)
    WIN_LOSE_ANIMATION = 1,
    WIN_LOSE_RESULT_TEXT = 0x65,
    WIN_LOSE_CASUALTY_ICON_FIRST = 0x7d0,
    WIN_LOSE_ARTIFACT_BACKGROUND = 0x7d1,
    WIN_LOSE_ARTIFACT_ICON = 0x7d2,
    WIN_LOSE_CASUALTY_TEXT_FIRST = 0x834,
    WIN_LOSE_ARTIFACT_NAME = 0x835,
    WIN_LOSE_CASUALTY_HEADING = 0x83e
H1_ENUM_END(CombatWinLoseControl)

// m_winLoseBottomTextWidgets slots: side * ARMY_GROUP_SLOT_COUNT + slot for
// the casualty counts, then the two side headings and the casualty title.
// DoVictory's experience line buffer (Buka 2.1 VICTORY_EXPERIENCE_TEXT_SIZE;
// retail's frame places message directly above a 152-byte array).
H1_ENUM_CONST_BEGIN(CombatVictoryConstant)
    COMBAT_VICTORY_EXPERIENCE_TEXT_SIZE = 152
H1_ENUM_CONST_END(CombatVictoryConstant)

H1_ENUM_CONST_BEGIN(CombatWinLoseSlot)
    WIN_LOSE_SLOT_SIDE_HEADING_FIRST = 10,
    WIN_LOSE_SLOT_CASUALTY_TITLE = 12,
    WIN_LOSE_SLOT_COUNT = 15
H1_ENUM_CONST_END(CombatWinLoseSlot)

// surrendr.bin widget ids DoSurrender fills (the victor's portrait, the offer).
H1_ENUM_BEGIN(SurrenderControl)
    SURRENDER_PORTRAIT = 1,
    SURRENDER_TEXT = 2
H1_ENUM_END(SurrenderControl)

// SetCombatDirections' rear hex for a one-hex stack: no rear hex to check
// (ValidHexToStandOn accepts it; CheckSetMouseDirection's backHex default).
H1_ENUM_CONST_BEGIN(CombatRearHexConstant)
    COMBAT_REAR_HEX_UNUSED = -2
H1_ENUM_CONST_END(CombatRearHexConstant)

// Combat-window widget ids ProcessCombatMsg handles: the battlefield (0x40,
// Buka CombatControlId CONTROL_MAIN_BUTTON; ResetMouse hovers it), the button
// that stops grid selection and hides the pointer, and the skip-turn button
// that queues ACTION_SKIP_TURN.
H1_ENUM_BEGIN(CombatControlId)
    COMBAT_CONTROL_DISABLE_SELECTION = 2,
    COMBAT_CONTROL_SKIP_TURN = 8,
    COMBAT_CONTROL_FIELD = 0x40
H1_ENUM_END(CombatControlId)

// Buka COMMAND.cpp Main; HoMM1 polls sound on the 75-tick timer and has no
// combat screen cycling or no-show mode.
VA(0x0041d460, 0x27c)
i16 combatManager::Main(struct tag_message& message) {
    i32 result = MESSAGE_DISPATCH_CONSUME;
    army* thisArmy;
    CombatRemotePacket* packet;

    if (glTimers[COMBAT_FRAME_TIMER_SLOT] < KBTickCount()) {
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
                case INPUT_SCAN_F2:
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
        if (m_playerId[m_currentSide] == GAME_PLAYER_NONE
            || !gbThisNetHumanPlayer[m_playerId[m_currentSide]] || m_gridSelectionDisabled)
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
VA(0x0041d6dc, 0xa7)
i8 combatManager::ValidHexToStandOn(i32 hex) {
    if (hex == COMBAT_REAR_HEX_UNUSED)
        return 1;
    if (hex != ARMY_HEX_INVALID && hex % COMBAT_GRID_COLUMNS != COMBAT_GRID_LAST_COLUMN
        && hex % COMBAT_GRID_COLUMNS != 0 && m_hexCells[hex].m_obstacleIndex == COMBAT_OBSTACLE_NONE
        && (m_hexCells[hex].m_occupantSide == COMBAT_SIDE_NONE
            || (m_hexCells[hex].m_occupantSide == m_currentSide
                && m_hexCells[hex].m_occupantIndex == m_currentArmyIndex)))
        return 1;
    else
        return 0;
}

// Buka COMMAND.cpp SetCombatDirections; HoMM1 reads the global adjacency
// table and keeps the 24-sector map as bytes.
VA(0x0041d783, 0x704)
void combatManager::SetCombatDirections(i32 targetHex) {
    i32 wasMapped;
    i32 owner;
    i32 oldUnset;
    i8 keptReachable[COMBAT_DIRECTION_COUNT];
    i32 next;
    i32 myRear[COMBAT_DIRECTION_COUNT];
    i32 bestBefore;
    i32 outDir;
    i32 dir;
    i32 directionHexes[COMBAT_DIRECTION_COUNT];
    army* curArmyPtr;
    i32 targetIndex;
    army* mainTarget;
    i8 canStandIn[COMBAT_DIRECTION_COUNT];

    curArmyPtr = &m_armies[m_currentSide][m_currentArmyIndex];
    owner = curArmyPtr->m_targetSide;
    targetIndex = curArmyPtr->m_targetIndex;
    CLEAR_ARMY_TARGET(curArmyPtr);
    mainTarget = &m_armies[owner][targetIndex];
    for (dir = 0; dir < COMBAT_DIRECTION_COUNT; dir++) {
        if (dir == COMBAT_DIRECTION_WIDE_WEST || dir == COMBAT_DIRECTION_WIDE_EAST) {
            if (curArmyPtr->m_stats.attributes & MONSTER_FLAGS_WIDE) {
                if (curArmyPtr->m_facing == ARMY_FACING_RIGHT) {
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
                directionHexes[dir] = ARMY_HEX_INVALID;
        } else
            directionHexes[dir] = gCombatAdjacency[targetHex][dir];
        if ((curArmyPtr->m_stats.attributes & MONSTER_FLAGS_WIDE)
            && directionHexes[dir] != ARMY_HEX_INVALID) {
            if (curArmyPtr->m_facing == ARMY_FACING_RIGHT) {
                if (dir == COMBAT_DIRECTION_NORTHWEST || dir == COMBAT_DIRECTION_WEST
                    || dir == COMBAT_DIRECTION_SOUTHWEST) {
                    if (directionHexes[dir] % COMBAT_GRID_COLUMNS == COMBAT_GRID_FIRST_INNER_COLUMN)
                        directionHexes[dir] = ARMY_HEX_INVALID;
                    else
                        directionHexes[dir]--;
                }
                if (directionHexes[dir] % COMBAT_GRID_COLUMNS == COMBAT_GRID_LAST_INNER_COLUMN)
                    myRear[dir] = ARMY_HEX_INVALID;
                else
                    myRear[dir] = directionHexes[dir] + 1;
            } else {
                if (dir == COMBAT_DIRECTION_NORTHEAST || dir == COMBAT_DIRECTION_EAST
                    || dir == COMBAT_DIRECTION_SOUTHEAST) {
                    if (directionHexes[dir] % COMBAT_GRID_COLUMNS == COMBAT_GRID_LAST_INNER_COLUMN)
                        directionHexes[dir] = ARMY_HEX_INVALID;
                    else
                        directionHexes[dir]++;
                }
                if (directionHexes[dir] % COMBAT_GRID_COLUMNS == COMBAT_GRID_FIRST_INNER_COLUMN)
                    myRear[dir] = ARMY_HEX_INVALID;
                else
                    myRear[dir] = directionHexes[dir] - 1;
            }
        } else
            myRear[dir] = COMBAT_REAR_HEX_UNUSED;
        if (ValidHexToStandOn(directionHexes[dir]) && ValidHexToStandOn(myRear[dir]))
            canStandIn[dir] = 1;
        else
            canStandIn[dir] = 0;
    }
    if (curArmyPtr->m_stats.attributes & MONSTER_FLAGS_FLYING) {
        for (dir = 0; dir < COMBAT_DIRECTION_COUNT; dir++)
            keptReachable[dir] = canStandIn[dir];
    } else {
        for (dir = 0; dir < COMBAT_DIRECTION_COUNT; dir++) {
            if (canStandIn[dir]) {
                if (curArmyPtr->m_hex == directionHexes[dir]
                    || curArmyPtr->ValidPath(directionHexes[dir], ARMY_PATH_EXACT_TARGET_HEX))
                    keptReachable[dir] = 1;
                else
                    keptReachable[dir] = 0;
            } else
                keptReachable[dir] = 0;
        }
    }
    m_validDirectionCount = 0;
    for (dir = 0; dir < COMBAT_DIRECTION_COUNT; dir++) {
        if (keptReachable[dir])
            m_validDirectionCount++;
    }
    if (!m_validDirectionCount)
        keptReachable[COMBAT_DIRECTION_WIDE_WEST] = 1;
    memset(m_directionMap, -1, sizeof(m_directionMap));
    for (dir = 0; dir < COMBAT_DIRECTION_COUNT; dir++) {
        outDir = dir;
        if (dir < COMBAT_DIRECTION_ADJACENT_COUNT)
            wasMapped = (dir + COMBAT_DIRECTION_OPPOSITE_OFFSET) % COMBAT_DIRECTION_ADJACENT_COUNT;
        else
            wasMapped = dir == COMBAT_DIRECTION_WIDE_WEST
                            ? static_cast<i8>(COMBAT_DIRECTION_WIDE_EAST)
                            : static_cast<i8>(COMBAT_DIRECTION_WIDE_WEST);
        if (keptReachable[wasMapped]) {
            if (mainTarget->m_stats.attributes & MONSTER_FLAGS_WIDE) {
                if (dir == COMBAT_DIRECTION_NORTHEAST
                    && m_hexCells[targetHex - 1].m_occupantSide == owner
                    && m_hexCells[targetHex - 1].m_occupantIndex == targetIndex)
                    outDir = COMBAT_DIRECTION_WIDE_WEST;
                else if (dir == COMBAT_DIRECTION_NORTHWEST
                         && m_hexCells[targetHex + 1].m_occupantSide == owner
                         && m_hexCells[targetHex + 1].m_occupantIndex == targetIndex)
                    outDir = COMBAT_DIRECTION_WIDE_WEST;
                else if (dir == COMBAT_DIRECTION_SOUTHEAST
                         && m_hexCells[targetHex - 1].m_occupantSide == owner
                         && m_hexCells[targetHex - 1].m_occupantIndex == targetIndex)
                    outDir = COMBAT_DIRECTION_WIDE_EAST;
                else if (dir == COMBAT_DIRECTION_SOUTHWEST
                         && m_hexCells[targetHex + 1].m_occupantSide == owner
                         && m_hexCells[targetHex + 1].m_occupantIndex == targetIndex)
                    outDir = COMBAT_DIRECTION_WIDE_EAST;
            }
            if (dir < COMBAT_DIRECTION_ADJACENT_COUNT)
                memset(
                    &m_directionMap[wasMapped * COMBAT_POINTER_SECTORS_PER_DIRECTION],
                    outDir,
                    COMBAT_POINTER_SECTORS_PER_DIRECTION
                );
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
    oldUnset = COMBAT_POINTER_SECTOR_COUNT;
    while (oldUnset > 0) {
        for (dir = 0; dir < COMBAT_POINTER_SECTOR_COUNT; dir++) {
            if (m_directionMap[dir] == COMBAT_DIRECTION_INVALID) {
                next = (dir + 1) % COMBAT_POINTER_SECTOR_COUNT;
                bestBefore = (dir + COMBAT_POINTER_SECTOR_COUNT - 1) % COMBAT_POINTER_SECTOR_COUNT;
                if (m_directionMap[next] >= 0 && m_directionMap[next] <= COMBAT_DIRECTION_LAST)
                    m_directionMap[dir] = m_directionMap[next] + COMBAT_POINTER_SECTOR_FILLED;
                else if (m_directionMap[bestBefore] >= 0
                         && m_directionMap[bestBefore] <= COMBAT_DIRECTION_LAST)
                    m_directionMap[dir] = m_directionMap[bestBefore] + COMBAT_POINTER_SECTOR_FILLED;
            }
        }
        oldUnset = 0;
        for (dir = 0; dir < COMBAT_POINTER_SECTOR_COUNT; dir++) {
            if (m_directionMap[dir] >= COMBAT_POINTER_SECTOR_FILLED)
                m_directionMap[dir] -= COMBAT_POINTER_SECTOR_FILLED;
            else if (m_directionMap[dir] == COMBAT_DIRECTION_INVALID)
                oldUnset++;
        }
    }
    curArmyPtr->m_targetSide = owner;
    curArmyPtr->m_targetIndex = targetIndex;
}

// Buka COMMAND.cpp CheckSetMouseDirection; HoMM1 hexes are 78 by 80 and
// odd rows shift by 66 pixels.
VA(0x0041de87, 0x564)
void combatManager::CheckSetMouseDirection(i32 mouseX, i32 mouseY, i32 targetHex) {
    i32 hexDir;
    i32 direction;
    i32 alternate;
    float endRatio;
    i32 slot;
    army* curArmyRef;
    i32 xPos;
    i32 myDistY;
    army* targetObj;
    i32 backHexVal;

    if (m_gridSelectionDisabled)
        return;
    if (m_validDirectionCount <= 1 && m_mouseDirection >= 0)
        return;
    xPos = mouseX - (targetHex % COMBAT_GRID_COLUMNS - 1) * COMBAT_HEX_WIDTH;
    if ((targetHex / COMBAT_GRID_COLUMNS) & 1)
        xPos -= 0x42;
    else
        xPos -= 0x1b;
    myDistY = mouseY - COMBAT_FIELD_TOP - targetHex / COMBAT_GRID_COLUMNS * COMBAT_HEX_HEIGHT;
    xPos -= 0x27;
    myDistY -= 0x28;
    slot = 0;
    if (xPos < 0) {
        if (myDistY < 0)
            slot += 18;
        else
            slot += 12;
    } else {
        if (myDistY < 0)
            slot += 0;
        else
            slot += 6;
    }
    xPos = abs(xPos);
    myDistY = abs(myDistY);
    endRatio = static_cast<float>(xPos) / (static_cast<float>(myDistY));
    if (slot == 0 || slot == 12) {
        if (endRatio > 3.73)
            slot += 5;
        else if (endRatio > 1.73)
            slot += 4;
        else if (endRatio > 1.0f)
            slot += 3;
        else if (endRatio > 0.58)
            slot += 2;
        else if (endRatio > 0.27)
            slot++;
    } else {
        if (endRatio < 0.27)
            slot += 5;
        else if (endRatio < 0.58)
            slot += 4;
        else if (endRatio < 1.0f)
            slot += 3;
        else if (endRatio < 1.73)
            slot += 2;
        else if (endRatio < 3.73)
            slot++;
    }
    if (m_directionMap[slot] == m_mouseDirection)
        return;
    m_mouseDirection = m_directionMap[slot];
    hexDir = OppositeDirection(m_directionMap[slot]);
    direction = hexDir;
    alternate = COMBAT_DIRECTION_INVALID;
    curArmyRef = &m_armies[m_currentSide][m_currentArmyIndex];
    targetObj = &m_armies[curArmyRef->m_targetSide][curArmyRef->m_targetIndex];
    if (hexDir == COMBAT_DIRECTION_WIDE_WEST || hexDir == COMBAT_DIRECTION_WIDE_EAST) {
        if (curArmyRef->m_stats.attributes & MONSTER_FLAGS_WIDE) {
            if (curArmyRef->m_facing == ARMY_FACING_RIGHT && hexDir == COMBAT_DIRECTION_WIDE_WEST) {
                hexDir = COMBAT_DIRECTION_NORTHWEST;
                alternate = COMBAT_DIRECTION_NORTHEAST;
            } else if (curArmyRef->m_facing == ARMY_FACING_RIGHT
                       && hexDir == COMBAT_DIRECTION_WIDE_EAST) {
                hexDir = COMBAT_DIRECTION_SOUTHWEST;
                alternate = COMBAT_DIRECTION_SOUTHEAST;
            } else if (curArmyRef->m_facing == ARMY_FACING_LEFT
                       && hexDir == COMBAT_DIRECTION_WIDE_WEST) {
                hexDir = COMBAT_DIRECTION_NORTHEAST;
                alternate = COMBAT_DIRECTION_NORTHWEST;
            } else {
                hexDir = COMBAT_DIRECTION_SOUTHEAST;
                alternate = COMBAT_DIRECTION_SOUTHWEST;
            }
        } else {
            if (m_hexCells[targetHex - 1].m_occupantSide == curArmyRef->m_targetSide
                && m_hexCells[targetHex - 1].m_occupantIndex == curArmyRef->m_targetIndex)
                targetHex--;
            if (hexDir == COMBAT_DIRECTION_WIDE_WEST)
                hexDir = COMBAT_DIRECTION_NORTHEAST;
            else
                hexDir = COMBAT_DIRECTION_SOUTHEAST;
        }
    } else {
        if (curArmyRef->m_facing == ARMY_FACING_RIGHT
            && (curArmyRef->m_stats.attributes & MONSTER_FLAGS_WIDE)) {
            if (hexDir == COMBAT_DIRECTION_NORTHWEST || hexDir == COMBAT_DIRECTION_WEST
                || hexDir == COMBAT_DIRECTION_SOUTHWEST)
                targetHex--;
        } else if (curArmyRef->m_facing == ARMY_FACING_LEFT
                   && (curArmyRef->m_stats.attributes & MONSTER_FLAGS_WIDE)
                   && (hexDir == COMBAT_DIRECTION_NORTHEAST || hexDir == COMBAT_DIRECTION_EAST
                       || hexDir == COMBAT_DIRECTION_SOUTHEAST))
            targetHex++;
    }
    m_directionTargetHex = gCombatAdjacency[targetHex][hexDir];
    backHexVal = COMBAT_REAR_HEX_UNUSED;
    if (curArmyRef->m_facing == ARMY_FACING_LEFT
        && (curArmyRef->m_stats.attributes & MONSTER_FLAGS_WIDE))
        backHexVal = m_directionTargetHex - 1;
    if (curArmyRef->m_facing == ARMY_FACING_RIGHT
        && (curArmyRef->m_stats.attributes & MONSTER_FLAGS_WIDE))
        backHexVal = m_directionTargetHex + 1;
    if (!ValidHexToStandOn(m_directionTargetHex) || !ValidHexToStandOn(backHexVal)) {
        if ((curArmyRef->m_stats.attributes & MONSTER_FLAGS_WIDE)
            && (direction == COMBAT_DIRECTION_WIDE_WEST
                || direction == COMBAT_DIRECTION_WIDE_EAST)) {
            if (curArmyRef->m_facing == ARMY_FACING_RIGHT)
                m_directionTargetHex = m_directionTargetHex + 1;
            else
                m_directionTargetHex = m_directionTargetHex - 1;
        } else {
            if (alternate != COMBAT_DIRECTION_INVALID)
                m_directionTargetHex = gCombatAdjacency[targetHex][alternate];
        }
    }
    gpMouseManager->SetPointer(m_mouseDirection + COMBAT_POINTER_ATTACK_FIRST);
}

// Buka GetPointer precedes ProcessCombatMsg. HoMM1's sole caller passes one
// command and retail maps command 13 to pointer 5, preserving all others.
VA(0x0041e3eb, 0x1d)
H1_ENUM_RETURN(CombatPointerCode, i32)
combatManager::GetPointer(H1_ENUM_PARAM(CombatMessageCommand, i32) command) {
    if (command == COMBAT_MESSAGE_COMMAND_OPPOSING_OPTIONS)
        return COMBAT_POINTER_VIEW;
    else
        return command;
}

// Buka COMMAND.cpp ProcessCombatMsg; HoMM1 hovers the combat field as
// widget 0x40 and handles F1, space, H, T and C keys.
VA(0x0041e408, 0x4f8)
i32 combatManager::ProcessCombatMsg(struct tag_message& message) {
    i16 mouseX = message.x;
    i16 mouseY = message.y;
    i8 unused = 0;
    i16 selectedHexVal;

    if (!(message.type & m_messageTypeMask))
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
                            selectedHexVal = GetGridIndex(mouseX, mouseY);
                            if (selectedHexVal != m_selectedHex
                                || selectedHexVal == ARMY_HEX_INVALID) {
                                m_selectedHex = selectedHexVal;
                                m_previousCommand = COMBAT_INVALID_COMMAND;
                                m_currentCommand = GetCommand(m_selectedHex);
                                m_mouseDirection = COMBAT_DIRECTION_INVALID;
                                if (m_currentCommand == COMBAT_MESSAGE_COMMAND_ATTACK) {
                                    SetCombatDirections(selectedHexVal);
                                    CheckSetMouseDirection(mouseX, mouseY, selectedHexVal);
                                } else
                                    gpMouseManager->SetPointer(GetPointer(m_currentCommand));
                            } else if (m_currentCommand == COMBAT_MESSAGE_COMMAND_ATTACK)
                                CheckSetMouseDirection(mouseX, mouseY, selectedHexVal);
                            if (m_currentCommand != m_previousCommand) {
                                m_previousCommand = m_currentCommand;
                                CombatMessage(m_currentCommand);
                            }
                            break;
                        default:
                            gpMouseManager->MouseCoords(mouseX, mouseY);
                            if (mouseX <= 0x32)
                                CombatMessage(gCombatHelp[COMBAT_HELP_AUTO_COMBAT], 1);
                            else if (mouseX >= 0x24e)
                                CombatMessage(gCombatHelp[COMBAT_HELP_SKIP_UNIT], 1);
                            else
                                CombatMessage(gCombatHelp[COMBAT_HELP_NONE], 1);
                            gpMouseManager->SetPointer(COMBAT_POINTER_DEFAULT);
                            m_selectedHex = ARMY_HEX_INVALID;
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
                case INPUT_SCAN_F2:
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
                            localization::Tr("combat.spell.no_hero"),
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
                            localization::Tr("combat.spell.already_cast"),
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
VA(0x0041e900, 0x122)
void combatManager::ResetRound(void) {
    i32 unusedRoundWord;
    i32 armyIndex;
    i32 side;
    army* currentArmy;

    m_catapultAttacksRemaining[COMBAT_ATTACKER_SIDE] = m_catapultAttackCount[COMBAT_ATTACKER_SIDE];
    m_catapultAttacksRemaining[COMBAT_DEFENDER_SIDE] = m_catapultAttackCount[COMBAT_DEFENDER_SIDE];
    m_keepAttacksRemaining[COMBAT_ATTACKER_SIDE] = 1;
    m_keepAttacksRemaining[COMBAT_DEFENDER_SIDE] = 1;
    m_heroCastSpell[COMBAT_ATTACKER_SIDE] = m_heroCastSpell[COMBAT_DEFENDER_SIDE] = 0;
    for (side = 0; side < COMBAT_SIDE_COUNT; side++) {
        for (armyIndex = 0; armyIndex < ARMY_GROUP_SLOT_COUNT; armyIndex++) {
            currentArmy = &m_armies[side][armyIndex];
            if (currentArmy->m_quantity > 0) {
                currentArmy->m_stats.attributes &= MONSTER_FLAGS_ROUND_PERSISTENT_MASK;
                if (currentArmy->m_creatureType == CREATURE_TROLL)
                    currentArmy->m_hitPointsLost = 0;
                if (currentArmy->m_spellRounds > 0) {
                    currentArmy->m_spellRounds--;
                    if (currentArmy->m_spellRounds == 0)
                        currentArmy->CancelSpell();
                }
            }
        }
    }
    m_currentSpeed = CREATURE_SPEED_BLAZING;
}

// Buka COMMAND.cpp CheckWin; HoMM1 returns the byte flag and names the
// winning side directly (-1 for a draw).
VA(0x0041ea22, 0x123)
i32 combatManager::CheckWin(struct tag_message* message) {
    i32 pos;
    i8 combatEnded;
    i32 unusedWinWordValue;

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
        m_combatResult = m_sideRetreated[COMBAT_ATTACKER_SIDE]
                             ? static_cast<i8>(COMBAT_RESULT_DEFENDER)
                             : static_cast<i8>(COMBAT_RESULT_ATTACKER);
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
VA(0x0041eb45, 0x269)
i8 combatManager::GetCommand(i16 hex) {
    i8 unusedCol = hex % COMBAT_GRID_COLUMNS;
    i8 rowIndex = hex / COMBAT_GRID_COLUMNS;
    army* oldArmy;
    i8 indexNum;
    i8 enemySide;

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
            indexNum = m_hexCells[hex].m_occupantIndex;
            oldArmy = &m_armies[m_currentSide][m_currentArmyIndex];
            CLEAR_ARMY_TARGET(oldArmy);
            if (m_hexCells[hex].m_obstacleIndex != COMBAT_OBSTACLE_NONE)
                return COMBAT_MESSAGE_COMMAND_DEFAULT;
            else if (enemySide != COMBAT_SIDE_NONE) {
                switch (enemySide) {
                    case COMBAT_DEFENDER_SIDE:
                    case COMBAT_ATTACKER_SIDE:
                        if (enemySide == m_currentSide)
                            return COMBAT_MESSAGE_COMMAND_VIEW_INFO;
                        else {
                            oldArmy->m_targetSide = enemySide;
                            oldArmy->m_targetIndex = indexNum;
                            if (oldArmy->m_stats.shots > 0
                                && oldArmy->GetAttackMask(
                                       oldArmy->m_hex,
                                       ARMY_ATTACK_TARGET_ENEMY,
                                       ARMY_HEX_INVALID
                                   ) == COMBAT_ALL_DIRECTIONS_BLOCKED)
                                return COMBAT_MESSAGE_COMMAND_SHOOT;
                            if (oldArmy->ValidPath(hex, ARMY_PATH_EXACT_TARGET_HEX) == 1)
                                return COMBAT_MESSAGE_COMMAND_ATTACK;
                            else {
                                CLEAR_ARMY_TARGET(oldArmy);
                                return COMBAT_MESSAGE_COMMAND_DEFAULT;
                            }
                        }
                }
            } else {
                if (m_armies[m_currentSide][m_currentArmyIndex]
                        .ValidPath(hex, ARMY_PATH_ANY_TARGET_HEX)
                    == 1)
                    return (m_armies[m_currentSide][m_currentArmyIndex].m_stats.attributes
                            & MONSTER_FLAGS_FLYING)
                               ? static_cast<i8>(COMBAT_MESSAGE_COMMAND_FLY)
                               : static_cast<i8>(COMBAT_MESSAGE_COMMAND_MOVE);
            }
            break;
    }
    return COMBAT_MESSAGE_COMMAND_DEFAULT;
}

// Buka COMMAND.cpp RightClick; HoMM1 hero hexes are 26 and 9 and the
// army view also takes the side.
VA(0x0041edae, 0x171)
i8 combatManager::RightClick(i8 hex) {
    i8 col = hex % COMBAT_GRID_COLUMNS;
    i8 row = hex / COMBAT_GRID_COLUMNS;

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
            i8 side = m_hexCells[hex].m_occupantSide;
            i8 armyIdx = m_hexCells[hex].m_occupantIndex;
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
VA(0x0041ef1f, 0x2ea)
void combatManager::DoCommand(i8 command) {
    i32 unusedValue1Value;
    i32 unusedValue2Value;
    army* currentArmy = &m_armies[m_currentSide][m_currentArmyIndex];

    switch (command) {
        case COMBAT_MESSAGE_COMMAND_DEFAULT:
            break;
        case COMBAT_MESSAGE_COMMAND_MOVE:
        case COMBAT_MESSAGE_COMMAND_FLY:
        case COMBAT_MESSAGE_COMMAND_SHOOT:
            giNextAction = ACTION_MOVE, giNextActionGridIndex = m_selectedHex;
            giNextActionExtra = ARMY_HEX_INVALID;
            break;
        case COMBAT_MESSAGE_COMMAND_ATTACK:
            giNextActionGridIndex = m_selectedHex;
            if (m_playerId[m_currentSide] == GAME_PLAYER_NONE
                || !gbHumanPlayer[m_playerId[m_currentSide]] || m_gridSelectionDisabled) {
                giNextAction = ACTION_MOVE;
                giNextActionExtra = ARMY_HEX_INVALID;
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
                localization::Tr("combat.confirm.retreat"),
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
                        localization::Tr("resource.gold.insufficient"),
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
VA(0x0041f209, 0x180)
i16 WinCombatHandler(struct tag_message& message) {
    i32 finalDelay = 0x5a;
    i16 curFrame = 1;

    if (message.type == MESSAGE_WIDGET) {
        switch (message.command) {
            case WIDGET_NOTIFY_DESELECT:
                switch (message.id) {
                    case DIALOG_BUTTON_0:
                        if (iCurTransferArtifact + 1 < iMaxTransferArtifacts) {
                            gpCombatManager->ClearWinLoseBottom(gpCombatManager->m_winLoseWindow);
                            iCurTransferArtifact++;
                            gpCombatManager->ShowWinLoseArtifact(
                                gpCombatManager->m_winLoseWindow,
                                iTransferArtifacts[iCurTransferArtifact]
                            );
                        } else {
                            FINISH_DIALOG_MESSAGE(message);
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
    if (glTimers[COMBAT_FRAME_TIMER_SLOT] < KBTickCount()) {
        SET_WIDGET_MESSAGE(message, WIDGET_COMMAND_SET_FRAME, WIN_LOSE_ANIMATION);
        gpGame->m_viewArmyResult++;
        message.value = gpGame->m_viewArmyResult % 6 + 1;
        gpCombatManager->m_winLoseWindow->BroadcastMessage(message);
        gpCombatManager->m_winLoseWindow->DrawWindow();
        glTimers[COMBAT_FRAME_TIMER_SLOT] = KBTickCount() + 0x5a;
    }
    return MESSAGE_DISPATCH_CONSUME;
}

// Buka COMMAND.cpp ClearWinLoseBottom (fifteen icon/text widget pairs).
VA(0x0041f389, 0x108)
void combatManager::ClearWinLoseBottom(class heroWindow* window) {
    i32 i;

    for (i = 0; i < WIN_LOSE_SLOT_COUNT; i++) {
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
VA(0x0041f491, 0x28d)
void combatManager::ShowWinLoseArtifact(class heroWindow* window, i32 artifact) {
    char* artifactName;
    i16 boxWidth = 0x140;
    i16 bottomEdge = 0x1ca;
    tag_message message;

    sprintf(gText, localization::Tr("combat.artifact.captured"));
    SET_WIDGET_MESSAGE(message, WIDGET_COMMAND_SET_TEXT, WIN_LOSE_RESULT_TEXT);
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
        WIN_LOSE_ARTIFACT_BACKGROUND,
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
        WIN_LOSE_ARTIFACT_ICON,
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
        WIN_LOSE_ARTIFACT_NAME,
        WIDGET_KIND_TEXT
    );
    if (m_winLoseBottomTextWidgets[0] == NULL)
        MemError();
    window->AddWidget(m_winLoseBottomTextWidgets[0], WINDOW_Z_ORDER_APPEND);
    gpCombatManager->m_winLoseWindow->DrawWindow();
    // Function-scope, declared here as the Buka 2.1 donor's playSample is:
    // retail gives it the first local slot.
    class sample* sample;
    sprintf(gText, "pickup%02d.82M", SRandom(1, 5));
    sample = LoadPlaySample(gText);
    WaitSample(sample);
}

// Buka COMMAND.cpp ShowDeadArmies; HoMM1 lays out up to five casualties a
// side with fixed 40-pixel spacing.
VA(0x0041f71e, 0x753)
void combatManager::ShowDeadArmies(class heroWindow* window) {
    i32 numLost[COMBAT_SIDE_COUNT];
    char* buffer;
    i32 casualtyTypeStr[COMBAT_SIDE_COUNT][ARMY_GROUP_SLOT_COUNT];
    i32 theIconSpacing;
    i32 armyIndex;
    i32 team;
    i16 boxWidth = 0x140;
    i16 bottomVal = 0x1ca;
    i32 rowYVal;
    tag_message message;
    i32 casualtyCount[COMBAT_SIDE_COUNT][ARMY_GROUP_SLOT_COUNT];
    i32 firstX;

    for (team = 0; team < WIN_LOSE_SLOT_COUNT; team++) {
        m_winLoseBottomWidgets[team] = NULL;
        m_winLoseBottomTextWidgets[team] = NULL;
    }
    for (team = 0; team < COMBAT_SIDE_COUNT; team++) {
        numLost[team] = 0;
        for (armyIndex = 0; armyIndex < ARMY_GROUP_SLOT_COUNT; armyIndex++) {
            if (m_armies[team][armyIndex].m_creatureType != CREATURE_NONE
                && m_armies[team][armyIndex].m_initialQuantity
                       > m_armies[team][armyIndex].m_quantity) {
                casualtyTypeStr[team][numLost[team]] = m_armies[team][armyIndex].m_creatureType;
                casualtyCount[team][numLost[team]] = m_armies[team][armyIndex].m_initialQuantity
                                                     - m_armies[team][armyIndex].m_quantity;
                numLost[team]++;
            }
        }
    }
    buffer = static_cast<char*>(malloc(0x1e));
    sprintf(buffer, localization::Tr("combat.casualties.title"));
    m_winLoseBottomTextWidgets[WIN_LOSE_SLOT_CASUALTY_TITLE] = new textWidget(
        0,
        0x104,
        0x140,
        0x14,
        buffer,
        "smalfont.fnt",
        1,
        WIN_LOSE_CASUALTY_HEADING,
        WIDGET_KIND_TEXT
    );
    if (m_winLoseBottomTextWidgets[WIN_LOSE_SLOT_CASUALTY_TITLE] == NULL)
        MemError();
    window->AddWidget(
        m_winLoseBottomTextWidgets[WIN_LOSE_SLOT_CASUALTY_TITLE],
        WINDOW_Z_ORDER_APPEND
    );
    for (team = 0; team < COMBAT_SIDE_COUNT; team++) {
        rowYVal = team == COMBAT_ATTACKER_SIDE ? 0x118 : 0x159;
        buffer = static_cast<char*>(malloc(0x1e));
        sprintf(
            buffer,
            team == COMBAT_ATTACKER_SIDE ? localization::Tr("combat.casualties.attacker")
                                         : localization::Tr("combat.casualties.defender")
        );
        m_winLoseBottomTextWidgets[WIN_LOSE_SLOT_SIDE_HEADING_FIRST + team] = new textWidget(
            0,
            rowYVal,
            0x140,
            0x14,
            buffer,
            "smalfont.fnt",
            1,
            WIN_LOSE_CASUALTY_HEADING,
            WIDGET_KIND_TEXT
        );
        if (m_winLoseBottomTextWidgets[WIN_LOSE_SLOT_SIDE_HEADING_FIRST + team] == NULL)
            MemError();
        window->AddWidget(
            m_winLoseBottomTextWidgets[WIN_LOSE_SLOT_SIDE_HEADING_FIRST + team],
            WINDOW_Z_ORDER_APPEND
        );
        if (numLost[team] <= 0) {
            buffer = static_cast<char*>(malloc(10));
            sprintf(buffer, localization::Tr("common.none"));
            m_winLoseBottomTextWidgets[team * ARMY_GROUP_SLOT_COUNT] = new textWidget(
                0,
                rowYVal + 0x12,
                0x140,
                0x14,
                buffer,
                "smalfont.fnt",
                1,
                team * ARMY_GROUP_SLOT_COUNT + WIN_LOSE_CASUALTY_TEXT_FIRST,
                WIDGET_KIND_TEXT
            );
            if (m_winLoseBottomTextWidgets[team * ARMY_GROUP_SLOT_COUNT] == NULL)
                MemError();
            window->AddWidget(
                m_winLoseBottomTextWidgets[team * ARMY_GROUP_SLOT_COUNT],
                WINDOW_Z_ORDER_APPEND
            );
        }
        theIconSpacing = 0x28;
        firstX = (0x140 - theIconSpacing * numLost[team]) / 2 + 3;
        for (armyIndex = 0; armyIndex < numLost[team]; armyIndex++) {
            m_winLoseBottomWidgets[team * ARMY_GROUP_SLOT_COUNT + armyIndex] = new iconWidget(
                firstX + theIconSpacing * armyIndex,
                rowYVal + 0xf,
                0x20,
                0x1c,
                "mons32.icn",
                casualtyTypeStr[team][armyIndex],
                ICON_DRAW_NORMAL,
                team * ARMY_GROUP_SLOT_COUNT + armyIndex + WIN_LOSE_CASUALTY_ICON_FIRST,
                ICON_WIDGET_DRAW,
                1
            );
            if (m_winLoseBottomWidgets[team * ARMY_GROUP_SLOT_COUNT + armyIndex] == NULL)
                MemError();
            buffer = static_cast<char*>(malloc(9));
            sprintf(buffer, "%d", casualtyCount[team][armyIndex]);
            m_winLoseBottomTextWidgets[team * ARMY_GROUP_SLOT_COUNT + armyIndex] = new textWidget(
                firstX + theIconSpacing * armyIndex,
                rowYVal + 0x2e,
                0x20,
                0xc,
                buffer,
                "smalfont.fnt",
                1,
                team * ARMY_GROUP_SLOT_COUNT + armyIndex + WIN_LOSE_CASUALTY_TEXT_FIRST,
                WIDGET_KIND_TEXT
            );
            if (m_winLoseBottomTextWidgets[team * ARMY_GROUP_SLOT_COUNT + armyIndex] == NULL)
                MemError();
            window->AddWidget(
                m_winLoseBottomWidgets[team * ARMY_GROUP_SLOT_COUNT + armyIndex],
                WINDOW_Z_ORDER_APPEND
            );
            window->AddWidget(
                m_winLoseBottomTextWidgets[team * ARMY_GROUP_SLOT_COUNT + armyIndex],
                WINDOW_Z_ORDER_APPEND
            );
        }
    }
}

// Buka COMMAND.cpp DoVictory; HoMM1 has no necromancy or eagle eye and
// grabs the screen instead of fading it.
VA(0x0041fe71, 0x731)
void combatManager::DoVictory(i8 winningSide) {
    i32 levelsGained;
    tag_message message;
    i32 cost; // unused, as in the Buka 2.1 donor; retail keeps its slot
    i32 i;
    char experienceText[COMBAT_VICTORY_EXPERIENCE_TEXT_SIZE];

    levelsGained = 0;
    iMaxTransferArtifacts = 0;
    iCurTransferArtifact = -1;
    FreeArmies();
    CombatMessage(" ", 1);
    GrabScreenBitmap(gpWindowManager->m_screen, 0, 0);
    gpMouseManager->SetPointer(COMBAT_POINTER_DEFAULT);
    switch (winningSide) {
        case COMBAT_SIDE_NONE:
            PlayMusic(MUSIC_TRACK_BATTLE_LOST);
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
                    m_playerId[winningSide] == GAME_PLAYER_NONE
                    || !gbThisNetHumanPlayer[m_playerId[winningSide]]
                )) {
                PlayMusic(MUSIC_TRACK_BATTLE_WON);
                m_winLoseWindow = new heroWindow(0x9f, 2, "wincmbt.bin");
                if (m_winLoseWindow == NULL)
                    MemError();
                if (m_heroes[winningSide]) {
                    if (gbCombatSurrender)
                        sprintf(gText, gBattleResults[BATTLE_RESULT_ENEMY_SURRENDERED]);
                    else if (gbRetreatWin)
                        sprintf(gText, gBattleResults[BATTLE_RESULT_ENEMY_FLED]);
                    else
                        sprintf(gText, gBattleResults[BATTLE_RESULT_VICTORY]);
                    if (levelsGained > 0 && winningSide == COMBAT_DEFENDER_SIDE
                        && giNumHumanPlayers > 1)
                        sprintf(
                            experienceText,
                            gBattleResults[BATTLE_RESULT_EXPERIENCE_AND_LEVELS],
                            m_heroes[winningSide]->m_name,
                            m_experienceValue[1 - winningSide],
                            levelsGained
                        );
                    else
                        sprintf(
                            experienceText,
                            gBattleResults[BATTLE_RESULT_EXPERIENCE],
                            m_heroes[winningSide]->m_name,
                            m_experienceValue[1 - winningSide]
                        );
                    strcat(gText, experienceText);
                    m_heroes[winningSide]->ApplyBattleWinTemps();
                } else {
                    if (gbCombatSurrender)
                        sprintf(gText, gBattleResults[BATTLE_RESULT_ENEMY_SURRENDERED]);
                    else if (gbRetreatWin)
                        sprintf(gText, gBattleResults[BATTLE_RESULT_ENEMY_FLED]);
                    else
                        sprintf(gText, gBattleResults[BATTLE_RESULT_VICTORY]);
                }
                SET_WIDGET_MESSAGE(message, WIDGET_COMMAND_SET_TEXT, WIN_LOSE_RESULT_TEXT);
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
                PlayMusic(MUSIC_TRACK_BATTLE_LOST);
                DoLoseWindow();
            }
            break;
    }
    gMapX = gpAdvManager->m_mapOriginX;
    gMapY = gpAdvManager->m_mapOriginY;
}

// Buka COMMAND.cpp DoLoseWindow; HoMM1 walks the defeated hero across a
// scrolling backdrop until the window's button is released.
VA(0x004205a2, 0x4dc)
void combatManager::DoLoseWindow(void) {
    i16 walkFrameNum;
    i16 lAnimY;
    i16 curWalkX;
    i16 curUnusedWalkY;
    tag_message packet;
    heroWindow* loseWindow;
    bitmap* newBmp;
    i16 res;
    i32 delayVal;
    i16 bestOffset;
    i32 party;
    i16 nextWidth;
    i16 sx;
    i16 blitHeightVal;
    i16 iAreaWidthVal;
    i16 stopRequested;
    icon* walkIcon;

    delayVal = 0xb4;
    sx = 0x31;
    lAnimY = 0x26;
    iAreaWidthVal = 0xdf;
    blitHeightVal = 0x7d;
    nextWidth = 0x280;
    curWalkX = 0x6f;
    curUnusedWalkY = 0x8a;
    iMaxTransferArtifacts = 0;
    walkFrameNum = 0;
    bestOffset = 0;
    stopRequested = 0;
    if (giCurPlayer == m_playerId[COMBAT_ATTACKER_SIDE]
        && gbThisNetHumanPlayer[m_playerId[COMBAT_ATTACKER_SIDE]])
        party = COMBAT_ATTACKER_SIDE;
    else if (giCurPlayer == m_playerId[COMBAT_DEFENDER_SIDE]
             && gbThisNetHumanPlayer[m_playerId[COMBAT_DEFENDER_SIDE]])
        party = COMBAT_DEFENDER_SIDE;
    else if (m_playerId[COMBAT_ATTACKER_SIDE] != GAME_PLAYER_NONE
             && gbThisNetHumanPlayer[m_playerId[COMBAT_ATTACKER_SIDE]])
        party = COMBAT_ATTACKER_SIDE;
    else
        party = COMBAT_DEFENDER_SIDE;
    loseWindow = new heroWindow(0x9f, 2, "losecmbt.bin");
    if (loseWindow == NULL)
        MemError();
    newBmp = gpResourceManager->GetBitmap("losecmbt.bmp");
    gLoadingMonoIcon = 1;
    walkIcon = gpResourceManager->GetIcon("losewalk.icn");
    gLoadingMonoIcon = 0;
    if (m_heroes[party]) {
        if (gbCombatSurrender)
            sprintf(gText, gBattleResults[BATTLE_RESULT_HERO_SURRENDERS], m_heroes[party]->m_name);
        else if (gbRetreatWin)
            sprintf(gText, gBattleResults[BATTLE_RESULT_HERO_FLEES], m_heroes[party]->m_name);
        else
            sprintf(gText, gBattleResults[BATTLE_RESULT_HERO_DEFEATED], m_heroes[party]->m_name);
    } else {
        if (gbCombatSurrender)
            sprintf(gText, gBattleResults[BATTLE_RESULT_FORCES_SURRENDER]);
        else if (gbRetreatWin)
            sprintf(gText, gBattleResults[BATTLE_RESULT_FORCES_FLEE]);
        else
            sprintf(gText, gBattleResults[BATTLE_RESULT_FORCES_DEFEATED]);
    }
    SET_WIDGET_MESSAGE(packet, WIDGET_COMMAND_SET_TEXT, WIN_LOSE_RESULT_TEXT);
    packet.text = gText;
    loseWindow->BroadcastMessage(packet);
    ShowDeadArmies(loseWindow);
    gpWindowManager->AddWindow(loseWindow, WINDOW_Z_ORDER_APPEND, 0);
    BlitBitmap(newBmp, bestOffset, 0, 0xdf, 0x7d, gpWindowManager->m_screen, 0xd0, 0x28);
    walkIcon->FillToBuffer(0x10e, 0x8c, walkFrameNum, 0, ICON_DRAW_NORMAL, ICON_DRAW_OFFSET_FULL);
    gpWindowManager->UpdateScreenRegion(0x9f, 2, 0x140, 0x1ca);
    glTimers[COMBAT_FRAME_TIMER_SLOT] = KBTickCount() + 0xb4;
    do {
        if (glTimers[COMBAT_FRAME_TIMER_SLOT] < KBTickCount()) {
            BlitBitmap(newBmp, bestOffset, 0, 0xdf, 0x7d, gpWindowManager->m_screen, 0xd0, 0x28);
            walkIcon->FillToBuffer(
                0x10e,
                0x8c,
                walkFrameNum,
                0,
                ICON_DRAW_NORMAL,
                ICON_DRAW_OFFSET_FULL
            );
            gpWindowManager->UpdateScreenRegion(0xd0, 0x28, 0xdf, 0x7d);
            walkFrameNum++;
            walkFrameNum = walkFrameNum % 8;
            bestOffset += 2;
            if (bestOffset > 0x1a0)
                bestOffset = 0;
            glTimers[COMBAT_FRAME_TIMER_SLOT] = KBTickCount() + 0xb4;
        }
        Process1WindowsMessage();
        packet = gpInputManager->GetEvent();
        gpMouseManager->Main(packet);
        res = gpWindowManager->Main(packet);
        if (res == MESSAGE_DISPATCH_FORWARD && packet.type == MESSAGE_WIDGET
            && packet.command == WIDGET_NOTIFY_DESELECT && packet.id == DIALOG_BUTTON_0)
            stopRequested = 1;
    } while (!stopRequested);
    gpWindowManager->RemoveWindow(loseWindow);
    delete loseWindow;
    gpResourceManager->Dispose(walkIcon);
    gpResourceManager->Dispose(newBmp);
}

// Buka COMMAND.cpp DoSurrender; HoMM1 charges half the stack cost and has
// no quill or diplomacy discount.
VA(0x00420a7e, 0x253)
i16 combatManager::DoSurrender(void) {
    heroWindow* win;
    i16 unusedResult;
    i32 armyIndex;
    tag_message message;
    i16 unusedTypeValue;

    giSurrenderCost = 0;
    for (armyIndex = 0; armyIndex < ARMY_GROUP_SLOT_COUNT; armyIndex++) {
        if (m_armies[m_currentSide][armyIndex].IsAlive())
            giSurrenderCost +=
                m_armies[m_currentSide][armyIndex].m_quantity
                * (gMonsterDatabase[m_armies[m_currentSide][armyIndex].m_creatureType].cost / 2);
    }
    unusedTypeValue = 1;
    unusedResult = 2;
    win = new heroWindow(0x55, 0x50, "surrendr.bin");
    if (win == NULL)
        MemError();
    SET_WIDGET_MESSAGE(message, WIDGET_COMMAND_SET_ICON, SURRENDER_PORTRAIT);
    sprintf(gText, "port%04d.icn", m_heroes[1 - m_currentSide]->m_portrait);
    message.text = gText;
    win->BroadcastMessage(message);
    message.command = WIDGET_COMMAND_SET_TEXT;
    message.id = SURRENDER_TEXT;
    sprintf(
        gText,
        localization::Tr("combat.surrender.offer"),
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
VA(0x00420cd1, 0xbb)
void combatManager::CheckChangeSelector(void) {
    army* currentArmy;

    currentArmy = &m_armies[m_currentSide][m_currentArmyIndex];
    if (!m_limitCreature || m_limitCreatureHex != currentArmy->m_hex) {
        UpdateGrid(
            m_limitCreatureHex < currentArmy->m_hex ? m_limitCreatureHex : currentArmy->m_hex,
            1
        );
        m_limitCreatureHex = currentArmy->m_hex;
        m_limitCreature = 1;
        DrawFrame(1);
    }
}

// Buka COMMAND.cpp CheckCastleAttack; HoMM1 keys both on the castle side.
VA(0x00420d8c, 0xf0)
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
VA(0x00420e7c, 0x5c)
void combatManager::CheckGetAIMove(void) {
    if (AICheckRetreat())
        return;
    if (!m_heroCastSpell[m_currentSide] && DoSpellAI(m_currentSide))
        return;
    DoCompAI(m_currentSide);
}

// Buka COMMAND.cpp GetControl; HoMM1 always resets the pointer and has no
// small view.
VA(0x00420ed8, 0x12b)
void combatManager::GetControl(void) {
    m_selectedHex = ARMY_HEX_INVALID;
    m_previousCommand = COMBAT_INVALID_COMMAND;
    m_previousCommand = COMBAT_INVALID_COMMAND;
    gpMouseManager->SetPointer(COMBAT_POINTER_DEFAULT);
    CheckChangeSelector();
    if (!gRemoteOn || m_playerId[COMBAT_ATTACKER_SIDE] < 0 || m_playerId[COMBAT_DEFENDER_SIDE] < 0
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
VA(0x00421003, 0xbd)
void combatManager::ResetMouse(void) {
    tag_message message;
    i16 x;
    i16 y;

    if (gbThisNetHasControl && m_playerId[m_currentSide] >= 0
        && gbHumanPlayer[m_playerId[m_currentSide]]) {
        m_selectedHex = ARMY_HEX_INVALID;
        CombatMessage("", 1);
        gpMouseManager->MouseCoords(x, y);
        message.type = MESSAGE_WIDGET;
        message.command = WIDGET_COMMAND_HOVER;
        message.id = y <= 0x1ca ? COMBAT_CONTROL_FIELD : 0;
        ProcessCombatMsg(message);
    } else
        gpMouseManager->SetPointer(COMBAT_POINTER_DEFAULT);
}

// Buka COMMAND.cpp ProcessNextAction; HoMM1 hides the pointer around the
// action, broadcasts it to a human net opponent and has no door or cycling.
VA(0x004210c0, 0x4c6)
i16 combatManager::ProcessNextAction(struct tag_message& message) {
    i32 actionData[4];
    i32 transmitResult;
    i32 remoteIndex;
    i8 doAdvance;
    army* actingArmy;

    if (gbThisNetHasControl && gRemoteOn && m_playerId[COMBAT_ATTACKER_SIDE] >= 0
        && m_playerId[COMBAT_DEFENDER_SIDE] >= 0 && gbHumanPlayer[m_playerId[COMBAT_DEFENDER_SIDE]]
        && gbHumanPlayer[m_playerId[COMBAT_ATTACKER_SIDE]]) {
        remoteIndex = m_playerId[1 - m_currentSide];
        if (remoteIndex < 0 || !gbHumanPlayer[remoteIndex])
            remoteIndex = giHostGamePos;
        actionData[0] = giNextAction;
        actionData[1] = giNextActionExtra;
        actionData[2] = giNextActionGridIndex;
        actionData[3] = giNextActionGridIndex2;
        transmitResult = TransmitRemoteData(
            reinterpret_cast<char*>(actionData), // API-forced: TransmitRemoteData takes char*.
            remoteIndex,
            sizeof(actionData),
            REMOTE_COMMAND_COMBAT_ACTION,
            1,
            1,
            REMOTE_MESSAGE_DEFAULT,
            1
        ); // API-forced: char* payload.
        if (!transmitResult)
            ShutDown(NULL);
    }
    actingArmy = &m_armies[m_currentSide][m_currentArmyIndex];
    doAdvance = 0;
    if (CheckWin(&message))
        return MESSAGE_DISPATCH_FORWARD;
    switch (giNextAction) {
        case ACTION_NONE:
            break;
        case ACTION_CAST_SPELL:
            gpMouseManager->ReallyHidePointer();
            CastSpell(giNextActionExtra, giNextActionGridIndex, 0, giNextActionGridIndex2);
            if (m_armies[m_currentSide][m_currentArmyIndex].m_quantity <= 0)
                doAdvance = 1;
            break;
        case ACTION_MOVE:
            gpMouseManager->ReallyHidePointer();
            actingArmy->MoveAttack(giNextActionGridIndex, 0);
            actingArmy->m_stats.attributes |= MONSTER_FLAGS_TURN_SPENT;
            if (CheckWin(&message))
                return MESSAGE_DISPATCH_FORWARD;
            CheckApplyGoodMorale(m_currentSide, m_currentArmyIndex);
            doAdvance = 1;
            break;
        case ACTION_ATTACK:
            gpMouseManager->ReallyHidePointer();
            if (giNextActionExtra != ARMY_HEX_INVALID && actingArmy->m_hex != giNextActionExtra)
                actingArmy->MoveAttack(giNextActionExtra, 1);
            actingArmy->MoveAttack(giNextActionGridIndex, 0);
            actingArmy->m_stats.attributes |= MONSTER_FLAGS_TURN_SPENT;
            if (CheckWin(&message))
                return MESSAGE_DISPATCH_FORWARD;
            CheckApplyGoodMorale(m_currentSide, m_currentArmyIndex);
            doAdvance = 1;
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
            doAdvance = 1;
            break;
    }
    giNextAction = ACTION_NONE;
    if (CheckWin(&message))
        return MESSAGE_DISPATCH_FORWARD;
    if (doAdvance && !GetNextArmy(1)) {
        ResetRound();
        GetNextArmy(1);
    }
    CheckChangeSelector();
    if (gbThisNetHasControl && !m_gridSelectionDisabled
        && m_playerId[m_currentSide] != GAME_PLAYER_NONE
        && gbThisNetHumanPlayer[m_playerId[m_currentSide]])
        gpMouseManager->ReallyShowPointer();
    else
        gpMouseManager->ReallyHidePointer();
    return MESSAGE_DISPATCH_CONSUME;
}

// COMMAND globals; unmigrated NWC addresses remain pending Buka review.
DATA(0x004a6a8c)
i8 gbThisNetHasControl;
DATA(0x004a6aa4)
i32 iCurTransferArtifact;
DATA(0x004a6ab0)
i8 iMaxTransferArtifacts;
DATA(0x004a6a88)
i32 giNextActionExtra;
DATA(0x004a6aa8)
i32 giNextActionGridIndex;
DATA(0x004a6ab4)
i32 giSurrenderCost;
DATA(0x004a6a94)
i8 iTransferArtifacts[HERO_ARTIFACT_SLOT_COUNT];
DATA(0x004a6aac)
i32 giNextAction;
DATA(0x004a6a90)
i32 giNextActionGridIndex2;

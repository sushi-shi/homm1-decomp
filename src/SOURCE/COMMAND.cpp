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

// Polls sound on the 75-tick timer.
VA(0x0041d460, 0x27c)
H1_ENUM_RETURN(MessageDispatchResult, i16) combatManager::Main(struct tag_message& message) {
    H1_ENUM_LOCAL(MessageDispatchResult, i32) result = MESSAGE_DISPATCH_CONSUME;
    army* currentArmy;
    RemoteMessage* packet;

    if (gTimers[COMBAT_FRAME_TIMER_SLOT] < KBTickCount()) {
        PollSound();
        gTimers[COMBAT_FRAME_TIMER_SLOT] = KBTickCount() + 0x4b;
    }
    CheckCastleAttack();
    if (CheckWin(&message))
        return MESSAGE_DISPATCH_FORWARD;
    packet = GetRemoteData(true);
    if (packet && packet->type == REMOTE_MESSAGE_RELIABLE) {
        switch (packet->command) {
            case REMOTE_COMMAND_COMBAT_ACTION:
                gNextAction = H1_ENUM_DECODE(CombatAction, packet->payload.combatAction.nextAction);
                gNextActionExtra = packet->payload.combatAction.nextActionExtra;
                gNextActionGridIndex = packet->payload.combatAction.nextActionGridIndex;
                gNextActionGridIndex2 = packet->payload.combatAction.nextActionGridIndex2;
                goto processAction;
            case REMOTE_COMMAND_CHAT:
                PopNetBox(packet->payload.data);
                break;
        }
    }
    if (!gThisNetHasControl) {
        if (message.type == MESSAGE_KEY_DOWN) {
            switch (message.keyCode) {
                case INPUT_SCAN_F2:
                    PopNetBox(NULL);
                    break;
            }
        }
        return MESSAGE_DISPATCH_CONSUME;
    }
    currentArmy = &m_armies[m_currentSide][m_currentArmyIndex];
    if (currentArmy->m_spellEffect == SPELL_BERZERKER) {
        currentArmy->GoBerserk();
        if (CheckWin(&message))
            return MESSAGE_DISPATCH_FORWARD;
    }
    if (m_gridSelectionDisabled) {
        while (message.type != MESSAGE_KEY_DOWN && message.type != MESSAGE_LEFT_BUTTON_DOWN
               && message.type != MESSAGE_RIGHT_BUTTON_DOWN && message.type != MESSAGE_NONE)
            message = gInputManager->GetEvent();
        if (message.type != MESSAGE_NONE) {
            m_gridSelectionDisabled = false;
            gMouseManager->ReallyShowPointer();
        }
    }
    CheckChangeSelector();
processAction:
    if (gNextAction == ACTION_NONE) {
        if (m_playerId[m_currentSide] == GAME_PLAYER_NONE
            || !gThisNetHumanPlayer[m_playerId[m_currentSide]] || m_gridSelectionDisabled)
            CheckGetAIMove();
        else
            result = ProcessCombatMsg(message);
    }
    if (gNextAction != ACTION_NONE)
        result = ProcessNextAction(message);
    return result;
}

// Rows are nine hexes wide and the edge columns are never standable.
VA(0x0041d6dc, 0xa7)
i8 combatManager::ValidHexToStandOn(i32 hex) {
    if (hex == COMBAT_REAR_HEX_UNUSED)
        return 1;
    if (hex != ARMY_HEX_INVALID && hex % COMBAT_GRID_COLUMNS != COMBAT_GRID_LAST_COLUMN
        && hex % COMBAT_GRID_COLUMNS != 0 && m_hexCells[hex].m_obstacleIndex == COMBAT_OBSTACLE_NONE
        && (m_hexCells[hex].m_occupantSide == COMBAT_SIDE_NONE
            || HEX_HAS_OCCUPANT(m_hexCells[hex], m_currentSide, m_currentArmyIndex)))
        return 1;
    else
        return 0;
}

// Reads the global adjacency table and keeps the 24-sector map as bytes.
VA(0x0041d783, 0x704)
void combatManager::SetCombatDirections(i32 targetHex) {
    H1_ENUM_ARRAY(i32, directionHexes, CombatHexDirection, COMBAT_DIRECTION_COUNT);
    H1_ENUM_LOCAL(CombatSide, i32) enemySide;
    i32 remainingSectors;
    H1_ENUM_ARRAY(b8, pathValid, CombatHexDirection, COMBAT_DIRECTION_COUNT);
    H1_ENUM_ARRAY(i32, behindHexes, CombatHexDirection, COMBAT_DIRECTION_COUNT);
    i32 next;
    army* currentArmy;
    H1_ENUM_LOCAL(CombatHexDirection, i32) attackDirection;
    i32 previousSector;
    H1_ENUM_LOCAL(CombatHexDirection, i32) approachDir;
    // Also the pointer-sector index of the last two loops.
    H1_ENUM_SHARED(CombatHexDirection, i32) dir;
    i32 targetIndex;
    army* enemyArmy;
    H1_ENUM_ARRAY(b8, canStandIn, CombatHexDirection, COMBAT_DIRECTION_COUNT);

    currentArmy = &m_armies[m_currentSide][m_currentArmyIndex];
    enemySide = currentArmy->m_targetSide;
    targetIndex = currentArmy->m_targetIndex;
    CLEAR_ARMY_TARGET(currentArmy);
    enemyArmy = &m_armies[enemySide][targetIndex];
    for (dir = COMBAT_DIRECTION_ADJACENT_FIRST; dir < COMBAT_DIRECTION_COUNT; dir++) {
        if (dir == COMBAT_DIRECTION_WIDE_WEST || dir == COMBAT_DIRECTION_WIDE_EAST) {
            if (currentArmy->m_stats.attributes & MONSTER_FLAGS_WIDE) {
                if (currentArmy->m_facing == ARMY_FACING_RIGHT) {
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
        if ((currentArmy->m_stats.attributes & MONSTER_FLAGS_WIDE)
            && directionHexes[dir] != ARMY_HEX_INVALID) {
            if (currentArmy->m_facing == ARMY_FACING_RIGHT) {
                if (dir == COMBAT_DIRECTION_NORTHWEST || dir == COMBAT_DIRECTION_WEST
                    || dir == COMBAT_DIRECTION_SOUTHWEST) {
                    if (directionHexes[dir] % COMBAT_GRID_COLUMNS == COMBAT_GRID_FIRST_INNER_COLUMN)
                        directionHexes[dir] = ARMY_HEX_INVALID;
                    else
                        directionHexes[dir]--;
                }
                if (directionHexes[dir] % COMBAT_GRID_COLUMNS == COMBAT_GRID_LAST_INNER_COLUMN)
                    behindHexes[dir] = ARMY_HEX_INVALID;
                else
                    behindHexes[dir] = directionHexes[dir] + 1;
            } else {
                if (dir == COMBAT_DIRECTION_NORTHEAST || dir == COMBAT_DIRECTION_EAST
                    || dir == COMBAT_DIRECTION_SOUTHEAST) {
                    if (directionHexes[dir] % COMBAT_GRID_COLUMNS == COMBAT_GRID_LAST_INNER_COLUMN)
                        directionHexes[dir] = ARMY_HEX_INVALID;
                    else
                        directionHexes[dir]++;
                }
                if (directionHexes[dir] % COMBAT_GRID_COLUMNS == COMBAT_GRID_FIRST_INNER_COLUMN)
                    behindHexes[dir] = ARMY_HEX_INVALID;
                else
                    behindHexes[dir] = directionHexes[dir] - 1;
            }
        } else
            behindHexes[dir] = COMBAT_REAR_HEX_UNUSED;
        if (ValidHexToStandOn(directionHexes[dir]) && ValidHexToStandOn(behindHexes[dir]))
            canStandIn[dir] = true;
        else
            canStandIn[dir] = false;
    }
    if (currentArmy->m_stats.attributes & MONSTER_FLAGS_FLYING) {
        for (dir = COMBAT_DIRECTION_ADJACENT_FIRST; dir < COMBAT_DIRECTION_COUNT; dir++)
            pathValid[dir] = canStandIn[dir];
    } else {
        for (dir = COMBAT_DIRECTION_ADJACENT_FIRST; dir < COMBAT_DIRECTION_COUNT; dir++) {
            if (canStandIn[dir]) {
                if (currentArmy->m_hex == directionHexes[dir]
                    || currentArmy->ValidPath(directionHexes[dir], ARMY_PATH_EXACT_TARGET_HEX))
                    pathValid[dir] = true;
                else
                    pathValid[dir] = false;
            } else
                pathValid[dir] = false;
        }
    }
    m_validDirectionCount = 0;
    for (dir = COMBAT_DIRECTION_ADJACENT_FIRST; dir < COMBAT_DIRECTION_COUNT; dir++) {
        if (pathValid[dir])
            m_validDirectionCount++;
    }
    if (!m_validDirectionCount)
        pathValid[COMBAT_DIRECTION_WIDE_WEST] = true;
    memset(m_directionMap, -1, sizeof(m_directionMap));
    for (dir = COMBAT_DIRECTION_ADJACENT_FIRST; dir < COMBAT_DIRECTION_COUNT; dir++) {
        attackDirection = dir;
        if (dir < COMBAT_DIRECTION_ADJACENT_COUNT)
            approachDir = COMBAT_OPPOSITE_ADJACENT(dir);
        else
            approachDir = dir == COMBAT_DIRECTION_WIDE_WEST
                              ? H1_ENUM_CAST(CombatHexDirection, i8, COMBAT_DIRECTION_WIDE_EAST)
                              : H1_ENUM_CAST(CombatHexDirection, i8, COMBAT_DIRECTION_WIDE_WEST);
        if (pathValid[approachDir]) {
            if (enemyArmy->m_stats.attributes & MONSTER_FLAGS_WIDE) {
                if (dir == COMBAT_DIRECTION_NORTHEAST
                    && HEX_HAS_OCCUPANT(m_hexCells[targetHex - 1], enemySide, targetIndex))
                    attackDirection = COMBAT_DIRECTION_WIDE_WEST;
                else if (dir == COMBAT_DIRECTION_NORTHWEST
                         && HEX_HAS_OCCUPANT(m_hexCells[targetHex + 1], enemySide, targetIndex))
                    attackDirection = COMBAT_DIRECTION_WIDE_WEST;
                else if (dir == COMBAT_DIRECTION_SOUTHEAST
                         && HEX_HAS_OCCUPANT(m_hexCells[targetHex - 1], enemySide, targetIndex))
                    attackDirection = COMBAT_DIRECTION_WIDE_EAST;
                else if (dir == COMBAT_DIRECTION_SOUTHWEST
                         && HEX_HAS_OCCUPANT(m_hexCells[targetHex + 1], enemySide, targetIndex))
                    attackDirection = COMBAT_DIRECTION_WIDE_EAST;
            }
            if (dir < COMBAT_DIRECTION_ADJACENT_COUNT)
                memset(
                    &m_directionMap
                        [H1_ENUM_ENCODE(CombatHexDirection, approachDir)
                         * COMBAT_POINTER_SECTORS_PER_DIRECTION],
                    H1_ENUM_ENCODE(CombatHexDirection, attackDirection),
                    COMBAT_POINTER_SECTORS_PER_DIRECTION
                );
            else if (dir == COMBAT_DIRECTION_WIDE_WEST) {
                m_directionMap[11] = H1_ENUM_ENCODE(CombatHexDirection, attackDirection);
                m_directionMap[12] = H1_ENUM_ENCODE(CombatHexDirection, attackDirection);
                m_directionMap[13] = H1_ENUM_ENCODE(CombatHexDirection, attackDirection);
            } else {
                m_directionMap[0] = H1_ENUM_ENCODE(CombatHexDirection, attackDirection);
                m_directionMap[1] = H1_ENUM_ENCODE(CombatHexDirection, attackDirection);
                m_directionMap[23] = H1_ENUM_ENCODE(CombatHexDirection, attackDirection);
            }
        }
    }
    remainingSectors = COMBAT_POINTER_SECTOR_COUNT;
    while (remainingSectors > 0) {
        for (dir = 0; dir < COMBAT_POINTER_SECTOR_COUNT; dir++) {
            if (H1_ENUM_DECODE(CombatHexDirection, m_directionMap[dir])
                == COMBAT_DIRECTION_INVALID) {
                next = (dir + 1) % COMBAT_POINTER_SECTOR_COUNT;
                previousSector =
                    (dir + COMBAT_POINTER_SECTOR_COUNT - 1) % COMBAT_POINTER_SECTOR_COUNT;
                if (m_directionMap[next] >= 0
                    && H1_ENUM_DECODE(CombatHexDirection, m_directionMap[next])
                           <= COMBAT_DIRECTION_LAST)
                    m_directionMap[dir] = m_directionMap[next] + COMBAT_POINTER_SECTOR_FILLED;
                else if (m_directionMap[previousSector] >= 0
                         && H1_ENUM_DECODE(CombatHexDirection, m_directionMap[previousSector])
                                <= COMBAT_DIRECTION_LAST)
                    m_directionMap[dir] =
                        m_directionMap[previousSector] + COMBAT_POINTER_SECTOR_FILLED;
            }
        }
        remainingSectors = 0;
        for (dir = 0; dir < COMBAT_POINTER_SECTOR_COUNT; dir++) {
            if (m_directionMap[dir] >= COMBAT_POINTER_SECTOR_FILLED)
                m_directionMap[dir] -= COMBAT_POINTER_SECTOR_FILLED;
            else if (H1_ENUM_DECODE(CombatHexDirection, m_directionMap[dir])
                     == COMBAT_DIRECTION_INVALID)
                remainingSectors++;
        }
    }
    currentArmy->m_targetSide = enemySide;
    currentArmy->m_targetIndex = targetIndex;
}

// Hexes are 78 by 80 and odd rows shift by 66 pixels.
#define sector slot // frame-slot spelling
VA(0x0041de87, 0x564)
void combatManager::CheckSetMouseDirection(i32 mouseX, i32 mouseY, i32 targetHex) {
    i32 yPos;
    H1_ENUM_LOCAL(CombatHexDirection, i32) directionCopy;
    H1_ENUM_LOCAL(CombatHexDirection, i32) alternateDirection;
    float endRatio;
    i32 sector;
    i32 rearHex;
    i32 xPos;
    H1_ENUM_LOCAL(CombatHexDirection, i32) hexDir;
    army* enemyArmy;
    army* movingUnit;

    if (m_gridSelectionDisabled)
        return;
    if (m_validDirectionCount <= 1 && m_mouseDirection >= COMBAT_DIRECTION_FIRST)
        return;
    xPos = mouseX - (targetHex % COMBAT_GRID_COLUMNS - 1) * COMBAT_HEX_WIDTH;
    if ((targetHex / COMBAT_GRID_COLUMNS) & 1)
        xPos -= 0x42;
    else
        xPos -= 0x1b;
    yPos = mouseY - COMBAT_FIELD_TOP - targetHex / COMBAT_GRID_COLUMNS * COMBAT_HEX_HEIGHT;
    xPos -= 0x27;
    yPos -= 0x28;
    sector = 0;
    if (xPos < 0) {
        if (yPos < 0)
            sector += COMBAT_CURSOR_SECTOR_LEFT_UP;
        else
            sector += COMBAT_CURSOR_SECTOR_LEFT_DOWN;
    } else {
        if (yPos < 0)
            sector += COMBAT_CURSOR_SECTOR_RIGHT_UP;
        else
            sector += COMBAT_CURSOR_SECTOR_RIGHT_DOWN;
    }
    xPos = abs(xPos);
    yPos = abs(yPos);
    endRatio = static_cast<float>(xPos) / (static_cast<float>(yPos));
    if (sector == COMBAT_CURSOR_SECTOR_RIGHT_UP || sector == COMBAT_CURSOR_SECTOR_LEFT_DOWN) {
        if (endRatio > 3.73)
            sector += 5;
        else if (endRatio > 1.73)
            sector += 4;
        else if (endRatio > 1.0f)
            sector += 3;
        else if (endRatio > 0.58)
            sector += 2;
        else if (endRatio > 0.27)
            sector++;
    } else {
        if (endRatio < 0.27)
            sector += 5;
        else if (endRatio < 0.58)
            sector += 4;
        else if (endRatio < 1.0f)
            sector += 3;
        else if (endRatio < 1.73)
            sector += 2;
        else if (endRatio < 3.73)
            sector++;
    }
    if (H1_ENUM_DECODE(CombatHexDirection, m_directionMap[sector]) == m_mouseDirection)
        return;
    m_mouseDirection = H1_ENUM_DECODE(CombatHexDirection, m_directionMap[sector]);
    hexDir = OppositeDirection(H1_ENUM_DECODE(CombatHexDirection, m_directionMap[sector]));
    directionCopy = hexDir;
    alternateDirection = COMBAT_DIRECTION_INVALID;
    movingUnit = &m_armies[m_currentSide][m_currentArmyIndex];
    enemyArmy = &m_armies[movingUnit->m_targetSide][movingUnit->m_targetIndex];
    if (hexDir == COMBAT_DIRECTION_WIDE_WEST || hexDir == COMBAT_DIRECTION_WIDE_EAST) {
        if (movingUnit->m_stats.attributes & MONSTER_FLAGS_WIDE) {
            if (movingUnit->m_facing == ARMY_FACING_RIGHT && hexDir == COMBAT_DIRECTION_WIDE_WEST) {
                hexDir = COMBAT_DIRECTION_NORTHWEST;
                alternateDirection = COMBAT_DIRECTION_NORTHEAST;
            } else if (movingUnit->m_facing == ARMY_FACING_RIGHT
                       && hexDir == COMBAT_DIRECTION_WIDE_EAST) {
                hexDir = COMBAT_DIRECTION_SOUTHWEST;
                alternateDirection = COMBAT_DIRECTION_SOUTHEAST;
            } else if (movingUnit->m_facing == ARMY_FACING_LEFT
                       && hexDir == COMBAT_DIRECTION_WIDE_WEST) {
                hexDir = COMBAT_DIRECTION_NORTHEAST;
                alternateDirection = COMBAT_DIRECTION_NORTHWEST;
            } else {
                hexDir = COMBAT_DIRECTION_SOUTHEAST;
                alternateDirection = COMBAT_DIRECTION_SOUTHWEST;
            }
        } else {
            if (HEX_HAS_OCCUPANT(
                    m_hexCells[targetHex - 1],
                    movingUnit->m_targetSide,
                    movingUnit->m_targetIndex
                ))
                targetHex--;
            if (hexDir == COMBAT_DIRECTION_WIDE_WEST)
                hexDir = COMBAT_DIRECTION_NORTHEAST;
            else
                hexDir = COMBAT_DIRECTION_SOUTHEAST;
        }
    } else {
        if (movingUnit->m_facing == ARMY_FACING_RIGHT
            && (movingUnit->m_stats.attributes & MONSTER_FLAGS_WIDE)) {
            if (hexDir == COMBAT_DIRECTION_NORTHWEST || hexDir == COMBAT_DIRECTION_WEST
                || hexDir == COMBAT_DIRECTION_SOUTHWEST)
                targetHex--;
        } else if (movingUnit->m_facing == ARMY_FACING_LEFT
                   && (movingUnit->m_stats.attributes & MONSTER_FLAGS_WIDE)
                   && (hexDir == COMBAT_DIRECTION_NORTHEAST || hexDir == COMBAT_DIRECTION_EAST
                       || hexDir == COMBAT_DIRECTION_SOUTHEAST))
            targetHex++;
    }
    m_directionTargetHex = gCombatAdjacency[targetHex][hexDir];
    rearHex = COMBAT_REAR_HEX_UNUSED;
    if (movingUnit->m_facing == ARMY_FACING_LEFT
        && (movingUnit->m_stats.attributes & MONSTER_FLAGS_WIDE))
        rearHex = m_directionTargetHex - 1;
    if (movingUnit->m_facing == ARMY_FACING_RIGHT
        && (movingUnit->m_stats.attributes & MONSTER_FLAGS_WIDE))
        rearHex = m_directionTargetHex + 1;
    if (!ValidHexToStandOn(m_directionTargetHex) || !ValidHexToStandOn(rearHex)) {
        if ((movingUnit->m_stats.attributes & MONSTER_FLAGS_WIDE)
            && (directionCopy == COMBAT_DIRECTION_WIDE_WEST
                || directionCopy == COMBAT_DIRECTION_WIDE_EAST)) {
            if (movingUnit->m_facing == ARMY_FACING_RIGHT)
                m_directionTargetHex = m_directionTargetHex + 1;
            else
                m_directionTargetHex = m_directionTargetHex - 1;
        } else {
            if (alternateDirection != COMBAT_DIRECTION_INVALID)
                m_directionTargetHex = gCombatAdjacency[targetHex][alternateDirection];
        }
    }
    gMouseManager->SetPointer(
        H1_ENUM_ENCODE(CombatHexDirection, m_mouseDirection) + COMBAT_POINTER_ATTACK_FIRST
    );
}
#undef sector

// The sole caller passes one command; command 13 maps to pointer 5 and all
// others pass through.
VA(0x0041e3eb, 0x1d)
H1_ENUM_RETURN(CombatPointerCode, i32)
combatManager::GetPointer(H1_ENUM_PARAM(CombatMessageCommand, i32) command) {
    if (command == COMBAT_MESSAGE_COMMAND_OPPOSING_OPTIONS)
        return COMBAT_POINTER_VIEW;
    else
        return H1_ENUM_CAST(CombatPointerCode, i32, command);
}

// Hovers the combat field as widget 0x40 and handles F1, space, H, T and C
// keys.
VA(0x0041e408, 0x4f8)
H1_ENUM_RETURN(MessageDispatchResult, i32) combatManager::ProcessCombatMsg(struct tag_message& message) {
    i16 mouseX = message.x;
    i16 mouseY = message.y;
    i8 unused = 0;
    i16 hoverGridIndex;

    if (!(message.type & m_messageTypeMask))
        return MESSAGE_DISPATCH_CONTINUE;
    switch (message.type) {
        case MESSAGE_WIDGET:
            switch (message.command) {
                case WIDGET_COMMAND_HOVER:
                    if (m_gridSelectionDisabled)
                        break;
                    switch (message.id) {
                        case COMBAT_CONTROL_FIELD:
                            gMouseManager->MouseCoords(mouseX, mouseY);
                            hoverGridIndex = GetGridIndex(mouseX, mouseY);
                            if (hoverGridIndex != m_selectedHex
                                || hoverGridIndex == ARMY_HEX_INVALID) {
                                m_selectedHex = hoverGridIndex;
                                m_previousCommand = COMBAT_INVALID_COMMAND;
                                m_currentCommand = GetCommand(m_selectedHex);
                                m_mouseDirection = COMBAT_DIRECTION_INVALID;
                                if (m_currentCommand == COMBAT_MESSAGE_COMMAND_ATTACK) {
                                    SetCombatDirections(hoverGridIndex);
                                    CheckSetMouseDirection(mouseX, mouseY, hoverGridIndex);
                                } else
                                    gMouseManager->SetPointer(GetPointer(m_currentCommand));
                            } else if (m_currentCommand == COMBAT_MESSAGE_COMMAND_ATTACK)
                                CheckSetMouseDirection(mouseX, mouseY, hoverGridIndex);
                            if (m_currentCommand != m_previousCommand) {
                                m_previousCommand = m_currentCommand;
                                CombatMessage(m_currentCommand);
                            }
                            break;
                        default:
                            gMouseManager->MouseCoords(mouseX, mouseY);
                            if (mouseX <= 0x32)
                                CombatMessage(gCombatHelp[COMBAT_HELP_AUTO_COMBAT], true);
                            else if (mouseX >= 0x24e)
                                CombatMessage(gCombatHelp[COMBAT_HELP_SKIP_UNIT], true);
                            else
                                CombatMessage(gCombatHelp[COMBAT_HELP_NONE], true);
                            gMouseManager->SetPointer(COMBAT_POINTER_DEFAULT);
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
                                m_gridSelectionDisabled = true;
                                gMouseManager->ReallyHidePointer();
                            }
                            break;
                        case COMBAT_CONTROL_SKIP_TURN:
                            if (!(message.modifiers & MESSAGE_MODIFIER_RIGHT_BUTTON))
                                gNextAction = ACTION_SKIP_TURN;
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
                    gNextAction = ACTION_SKIP_TURN;
                    break;
                case INPUT_SCAN_H:
                    if (m_heroes[m_currentSide]) {
                        gMouseManager->SetPointer(COMBAT_POINTER_DEFAULT);
                        ViewGeneral(m_currentSide, true, false);
                        ResetMouse();
                    }
                    break;
                case INPUT_SCAN_T:
                    gMouseManager->SetPointer(COMBAT_POINTER_DEFAULT);
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
                    gMouseManager->SetPointer(COMBAT_POINTER_DEFAULT);
                    gCurGeneral = m_currentSide;
                    ViewSpells(0);
                    ResetMouse();
                    break;
            }
            break;
    }
    return MESSAGE_DISPATCH_CONSUME;
}

// Five stacks a side, one keep and a byte spell-round counter.
VA(0x0041e900, 0x122)
void combatManager::ResetRound(void) {
    i32 unusedRoundWord;
    i32 armyIndex;
    H1_ENUM_LOCAL(CombatSide, i32) side;
    army* currentArmy;

    m_catapultAttacksRemaining[COMBAT_ATTACKER_SIDE] = m_catapultAttackCount[COMBAT_ATTACKER_SIDE];
    m_catapultAttacksRemaining[COMBAT_DEFENDER_SIDE] = m_catapultAttackCount[COMBAT_DEFENDER_SIDE];
    m_keepAttacksRemaining[COMBAT_ATTACKER_SIDE] = 1;
    m_keepAttacksRemaining[COMBAT_DEFENDER_SIDE] = 1;
    m_heroCastSpell[COMBAT_ATTACKER_SIDE] = m_heroCastSpell[COMBAT_DEFENDER_SIDE] = 0;
    for (side = COMBAT_DEFENDER_SIDE; side < COMBAT_SIDE_COUNT; side++) {
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

// Returns the byte flag and names the winning side directly (-1 for a
// draw).
VA(0x0041ea22, 0x123)
i32 combatManager::CheckWin(struct tag_message* message) {
    i32 pos;
    b8 combatEnded;
    i32 unusedWinWordValue;

    combatEnded = false;
    if (IsWinner(m_currentSide)) {
        combatEnded = true;
        if (IsWinner(COMBAT_OPPOSING_SIDE(m_currentSide)))
            m_combatResult = COMBAT_RESULT_DRAW;
        else
            m_combatResult = m_currentSide;
    } else if (IsWinner(COMBAT_OPPOSING_SIDE(m_currentSide))) {
        combatEnded = true;
        m_combatResult = COMBAT_OPPOSING_SIDE(m_currentSide);
    } else if (m_sideRetreated[COMBAT_ATTACKER_SIDE] || m_sideRetreated[COMBAT_DEFENDER_SIDE]) {
        combatEnded = true;
        gRetreatWin = 1;
        m_combatResult = m_sideRetreated[COMBAT_ATTACKER_SIDE]
                             ? H1_ENUM_CAST(CombatSide, i8, COMBAT_RESULT_DEFENDER)
                             : H1_ENUM_CAST(CombatSide, i8, COMBAT_RESULT_ATTACKER);
    }
    if (combatEnded) {
        DoVictory(m_combatResult);
        message->type = MESSAGE_EXECUTIVE;
        message->executiveCommand = EXECUTIVE_COMMAND_TERMINATE_LOOP;
    }
    return combatEnded;
}

// Returns each command directly and clears the target through the current
// stack.
#define enemyIndex indexNum // frame-slot spelling
VA(0x0041eb45, 0x269)
H1_ENUM_RETURN(CombatMessageCommand, i8) combatManager::GetCommand(i16 hex) {
    i8 unusedCol = hex % COMBAT_GRID_COLUMNS;
    i8 rowIndex = hex / COMBAT_GRID_COLUMNS;
    army* attackingStack;
    i8 enemyIndex;
    H1_ENUM_LOCAL(CombatSide, i8) enemySide;

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
            enemyIndex = m_hexCells[hex].m_occupantIndex;
            attackingStack = &m_armies[m_currentSide][m_currentArmyIndex];
            CLEAR_ARMY_TARGET(attackingStack);
            if (m_hexCells[hex].m_obstacleIndex != COMBAT_OBSTACLE_NONE)
                return COMBAT_MESSAGE_COMMAND_DEFAULT;
            else if (enemySide != COMBAT_SIDE_NONE) {
                switch (enemySide) {
                    case COMBAT_DEFENDER_SIDE:
                    case COMBAT_ATTACKER_SIDE:
                        if (enemySide == m_currentSide)
                            return COMBAT_MESSAGE_COMMAND_VIEW_INFO;
                        else {
                            attackingStack->m_targetSide = enemySide;
                            attackingStack->m_targetIndex = enemyIndex;
                            if (attackingStack->m_stats.shots > 0
                                && attackingStack->GetAttackMask(
                                       attackingStack->m_hex,
                                       ARMY_ATTACK_TARGET_ENEMY,
                                       ARMY_HEX_INVALID
                                   ) == COMBAT_ALL_DIRECTIONS_BLOCKED)
                                return COMBAT_MESSAGE_COMMAND_SHOOT;
                            if (attackingStack->ValidPath(hex, ARMY_PATH_EXACT_TARGET_HEX) == 1)
                                return COMBAT_MESSAGE_COMMAND_ATTACK;
                            else {
                                CLEAR_ARMY_TARGET(attackingStack);
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
                               ? H1_ENUM_CAST(CombatMessageCommand, i8, COMBAT_MESSAGE_COMMAND_FLY)
                               : H1_ENUM_CAST(
                                     CombatMessageCommand,
                                     i8,
                                     COMBAT_MESSAGE_COMMAND_MOVE
                                 );
            }
            break;
    }
    return COMBAT_MESSAGE_COMMAND_DEFAULT;
}
#undef enemyIndex

// The hero hexes are 26 and 9; the army view also takes the side.
VA(0x0041edae, 0x171)
i8 combatManager::RightClick(i8 hex) {
    i8 col = hex % COMBAT_GRID_COLUMNS;
    i8 row = hex / COMBAT_GRID_COLUMNS;

    if (hex == ARMY_HEX_INVALID)
        return 0;
    switch (hex) {
        case COMBAT_DEFENDER_HERO_HEX:
            if (m_heroes[COMBAT_DEFENDER_SIDE]) {
                ViewGeneral(COMBAT_DEFENDER_SIDE, false, true);
                ResetMouse();
            }
            return 0;
        case COMBAT_ATTACKER_HERO_HEX:
            if (m_heroes[COMBAT_ATTACKER_SIDE]) {
                ViewGeneral(COMBAT_ATTACKER_SIDE, false, true);
                ResetMouse();
            }
            return 0;
        default:
            if (hex % COMBAT_GRID_COLUMNS == COMBAT_GRID_LAST_COLUMN)
                return 0;
            H1_ENUM_LOCAL(CombatSide, i8) side = m_hexCells[hex].m_occupantSide;
            i8 armyIdx = m_hexCells[hex].m_occupantIndex;
            if (m_hexCells[hex].m_obstacleIndex != COMBAT_OBSTACLE_NONE)
                return 0;
            else if (side != COMBAT_SIDE_NONE) {
                switch (side) {
                    case COMBAT_DEFENDER_SIDE:
                    case COMBAT_ATTACKER_SIDE:
                        gMouseManager->SetPointer(COMBAT_POINTER_DEFAULT);
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

// Views the army at the selected hex on the current side.
VA(0x0041ef1f, 0x2ea)
void combatManager::DoCommand(H1_ENUM_PARAM(CombatMessageCommand, i8) command) {
    i32 unusedValue1Value;
    i32 unusedValue2Value;
    army* currentArmy = &m_armies[m_currentSide][m_currentArmyIndex];

    switch (command) {
        case COMBAT_MESSAGE_COMMAND_DEFAULT:
            break;
        case COMBAT_MESSAGE_COMMAND_MOVE:
        case COMBAT_MESSAGE_COMMAND_FLY:
        case COMBAT_MESSAGE_COMMAND_SHOOT:
            SET_NEXT_COMBAT_MOVE(m_selectedHex);
            gNextActionExtra = ARMY_HEX_INVALID;
            break;
        case COMBAT_MESSAGE_COMMAND_ATTACK:
            gNextActionGridIndex = m_selectedHex;
            if (m_playerId[m_currentSide] == GAME_PLAYER_NONE
                || !gHumanPlayer[m_playerId[m_currentSide]] || m_gridSelectionDisabled) {
                gNextAction = ACTION_MOVE;
                gNextActionExtra = ARMY_HEX_INVALID;
            } else {
                gNextAction = ACTION_ATTACK;
                gNextActionExtra = m_directionTargetHex;
            }
            break;
        case COMBAT_MESSAGE_COMMAND_OPTIONS:
            gMouseManager->SetPointer(COMBAT_POINTER_DEFAULT);
            ViewGeneral(m_currentSide, true, false);
            ResetMouse();
            break;
        case COMBAT_MESSAGE_COMMAND_OPPOSING_OPTIONS:
            gMouseManager->SetPointer(COMBAT_POINTER_DEFAULT);
            ViewGeneral(COMBAT_OPPOSING_SIDE(m_currentSide), true, false);
            ResetMouse();
            break;
        case COMBAT_MESSAGE_COMMAND_VIEW_INFO:
            gMouseManager->SetPointer(COMBAT_POINTER_DEFAULT);
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
            if (gWindowManager->m_dialogResult == NORMAL_DIALOG_CONFIRM)
                gNextAction = ACTION_RETREAT;
            ResetMouse();
            break;
        case COMBAT_MESSAGE_COMMAND_SURRENDER:
            if (DoSurrender() == 1) {
                if (gGame->m_players[m_playerId[m_currentSide]].m_resources[RESOURCE_GOLD]
                    < gSurrenderCost)
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
                    gNextAction = ACTION_SURRENDER;
                    gNextActionExtra = gSurrenderCost;
                }
            }
            ResetMouse();
            break;
    }
}

// Pages captured artifacts and cycles a single six-frame animation.
VA(0x0041f209, 0x180)
H1_ENUM_RETURN(MessageDispatchResult, i16) WinCombatHandler(struct tag_message& message) {
    i32 finalDelay = 0x5a;
    i16 curFrame = 1;

    if (message.type == MESSAGE_WIDGET) {
        switch (message.command) {
            case WIDGET_NOTIFY_DESELECT:
                switch (message.id) {
                    case DIALOG_BUTTON_0:
                        if (gCurTransferArtifact + 1 < gMaxTransferArtifacts) {
                            gCombatManager->ClearWinLoseBottom(gCombatManager->m_winLoseWindow);
                            gCurTransferArtifact++;
                            gCombatManager->ShowWinLoseArtifact(
                                gCombatManager->m_winLoseWindow,
                                gTransferArtifacts[gCurTransferArtifact]
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
    if (gTimers[COMBAT_FRAME_TIMER_SLOT] < KBTickCount()) {
        SET_WIDGET_MESSAGE(message, WIDGET_COMMAND_SET_FRAME, WIN_LOSE_ANIMATION);
        gGame->m_dialogAnimationCounter++;
        message.value = gGame->m_dialogAnimationCounter % 6 + 1;
        gCombatManager->m_winLoseWindow->BroadcastMessage(message);
        gCombatManager->m_winLoseWindow->DrawWindow();
        gTimers[COMBAT_FRAME_TIMER_SLOT] = KBTickCount() + 0x5a;
    }
    return MESSAGE_DISPATCH_CONSUME;
}

// Fifteen icon/text widget pairs.
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

VA(0x0041f491, 0x28d)
void combatManager::ShowWinLoseArtifact(
    class heroWindow* window,
    H1_ENUM_PARAM(ArtifactType, i32) artifact
) {
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
        H1_ENUM_ENCODE(ArtifactType, artifact),
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
    gCombatManager->m_winLoseWindow->DrawWindow();
    class sample* sample;
    sprintf(gText, "pickup%02d.82M", SRandom(1, 5));
    sample = LoadPlaySample(gText);
    WaitSample(sample);
}

// Lays out up to five casualties a side with fixed 40-pixel spacing.
#define side team // frame-slot spelling
VA(0x0041f71e, 0x753)
void combatManager::ShowDeadArmies(class heroWindow* window) {
    H1_ENUM_ARRAY(i32, numLost, CombatSide, COMBAT_SIDE_COUNT);
    char* buffer;
    H1_ENUM_ARRAY_ROWS(
        H1_ENUM_LOCAL(CreatureType, i32),
        deadMonsters,
        CombatSide,
        COMBAT_SIDE_COUNT,
        ARMY_GROUP_SLOT_COUNT
    );
    i32 iconDeltaX;
    i32 armyIndex;
    // A widget slot in the clearing loop, then the side whose casualties are laid out.
    H1_ENUM_SHARED(CombatSide, i32) side;
    i16 boxWidth = 0x140;
    i16 bottomVal = 0x1ca;
    i32 sectionPosY;
    tag_message message;
    H1_ENUM_ARRAY_ROWS(i32, casualtyCount, CombatSide, COMBAT_SIDE_COUNT, ARMY_GROUP_SLOT_COUNT);
    i32 firstX;

    for (side = 0; side < WIN_LOSE_SLOT_COUNT; side++) {
        m_winLoseBottomWidgets[side] = NULL;
        m_winLoseBottomTextWidgets[side] = NULL;
    }
    for (side = COMBAT_SIDE_FIRST; side < COMBAT_SIDE_COUNT; side++) {
        numLost[side] = 0;
        for (armyIndex = 0; armyIndex < ARMY_GROUP_SLOT_COUNT; armyIndex++) {
            if (m_armies[side][armyIndex].m_creatureType != CREATURE_NONE
                && m_armies[side][armyIndex].m_initialQuantity
                       > m_armies[side][armyIndex].m_quantity) {
                deadMonsters[side][numLost[side]] = m_armies[side][armyIndex].m_creatureType;
                casualtyCount[side][numLost[side]] = m_armies[side][armyIndex].m_initialQuantity
                                                     - m_armies[side][armyIndex].m_quantity;
                numLost[side]++;
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
    for (side = COMBAT_SIDE_FIRST; side < COMBAT_SIDE_COUNT; side++) {
        sectionPosY = side == COMBAT_ATTACKER_SIDE ? 0x118 : 0x159;
        buffer = static_cast<char*>(malloc(0x1e));
        sprintf(
            buffer,
            side == COMBAT_ATTACKER_SIDE ? localization::Tr("combat.casualties.attacker")
                                         : localization::Tr("combat.casualties.defender")
        );
        m_winLoseBottomTextWidgets
            [WIN_LOSE_SLOT_SIDE_HEADING_FIRST + H1_ENUM_ENCODE(CombatSide, side)] = new textWidget(
                0,
                sectionPosY,
                0x140,
                0x14,
                buffer,
                "smalfont.fnt",
                1,
                WIN_LOSE_CASUALTY_HEADING,
                WIDGET_KIND_TEXT
            );
        if (m_winLoseBottomTextWidgets
                [WIN_LOSE_SLOT_SIDE_HEADING_FIRST + H1_ENUM_ENCODE(CombatSide, side)]
            == NULL)
            MemError();
        window->AddWidget(
            m_winLoseBottomTextWidgets
                [WIN_LOSE_SLOT_SIDE_HEADING_FIRST + H1_ENUM_ENCODE(CombatSide, side)],
            WINDOW_Z_ORDER_APPEND
        );
        if (numLost[side] <= 0) {
            buffer = static_cast<char*>(malloc(10));
            sprintf(buffer, localization::Tr("common.none"));
            m_winLoseBottomTextWidgets[side * ARMY_GROUP_SLOT_COUNT] = new textWidget(
                0,
                sectionPosY + 0x12,
                0x140,
                0x14,
                buffer,
                "smalfont.fnt",
                1,
                side * ARMY_GROUP_SLOT_COUNT + WIN_LOSE_CASUALTY_TEXT_FIRST,
                WIDGET_KIND_TEXT
            );
            if (m_winLoseBottomTextWidgets[side * ARMY_GROUP_SLOT_COUNT] == NULL)
                MemError();
            window->AddWidget(
                m_winLoseBottomTextWidgets[side * ARMY_GROUP_SLOT_COUNT],
                WINDOW_Z_ORDER_APPEND
            );
        }
        iconDeltaX = 0x28;
        firstX = (0x140 - iconDeltaX * numLost[side]) / 2 + 3;
        for (armyIndex = 0; armyIndex < numLost[side]; armyIndex++) {
            m_winLoseBottomWidgets[side * ARMY_GROUP_SLOT_COUNT + armyIndex] = new iconWidget(
                firstX + iconDeltaX * armyIndex,
                sectionPosY + 0xf,
                0x20,
                0x1c,
                "mons32.icn",
                H1_ENUM_ENCODE(CreatureType, deadMonsters[side][armyIndex]),
                ICON_DRAW_NORMAL,
                side * ARMY_GROUP_SLOT_COUNT + armyIndex + WIN_LOSE_CASUALTY_ICON_FIRST,
                ICON_WIDGET_DRAW,
                1
            );
            if (m_winLoseBottomWidgets[side * ARMY_GROUP_SLOT_COUNT + armyIndex] == NULL)
                MemError();
            buffer = static_cast<char*>(malloc(9));
            sprintf(buffer, "%d", casualtyCount[side][armyIndex]);
            m_winLoseBottomTextWidgets[side * ARMY_GROUP_SLOT_COUNT + armyIndex] = new textWidget(
                firstX + iconDeltaX * armyIndex,
                sectionPosY + 0x2e,
                0x20,
                0xc,
                buffer,
                "smalfont.fnt",
                1,
                side * ARMY_GROUP_SLOT_COUNT + armyIndex + WIN_LOSE_CASUALTY_TEXT_FIRST,
                WIDGET_KIND_TEXT
            );
            if (m_winLoseBottomTextWidgets[side * ARMY_GROUP_SLOT_COUNT + armyIndex] == NULL)
                MemError();
            window->AddWidget(
                m_winLoseBottomWidgets[side * ARMY_GROUP_SLOT_COUNT + armyIndex],
                WINDOW_Z_ORDER_APPEND
            );
            window->AddWidget(
                m_winLoseBottomTextWidgets[side * ARMY_GROUP_SLOT_COUNT + armyIndex],
                WINDOW_Z_ORDER_APPEND
            );
        }
    }
}
#undef side

// Grabs the screen instead of fading it.
VA(0x0041fe71, 0x731)
void combatManager::DoVictory(H1_ENUM_PARAM(CombatSide, i8) winningSide) {
    i32 levelsGained;
    tag_message message;
    i32 cost;
    i32 i;
    char experienceText[COMBAT_VICTORY_EXPERIENCE_TEXT_SIZE];

    levelsGained = 0;
    gMaxTransferArtifacts = 0;
    gCurTransferArtifact = -1;
    FreeArmies();
    CombatMessage(" ", true);
    GrabScreenBitmap(gWindowManager->m_screen, 0, 0);
    gMouseManager->SetPointer(COMBAT_POINTER_DEFAULT);
    switch (winningSide) {
        case COMBAT_SIDE_NONE:
            PlayMusic(MUSIC_TRACK_BATTLE_LOST);
            DoLoseWindow();
            break;
        case COMBAT_DEFENDER_SIDE:
        case COMBAT_ATTACKER_SIDE:
            if (m_heroes[winningSide]) {
                m_experienceValue[COMBAT_OPPOSING_SIDE(winningSide)] =
                    ExperienceValueOfStack(COMBAT_OPPOSING_SIDE(winningSide));
                if (gRetreatWin)
                    m_experienceValue[COMBAT_OPPOSING_SIDE(winningSide)] -= 500;
                if (m_combatTowns[COMBAT_DEFENDER_SIDE] && winningSide == COMBAT_ATTACKER_SIDE)
                    m_experienceValue[COMBAT_OPPOSING_SIDE(winningSide)] += 500;
                levelsGained = gAdvManager->GiveExperience(
                    m_heroes[winningSide],
                    m_experienceValue[COMBAT_OPPOSING_SIDE(winningSide)],
                    !gThisNetHumanPlayer[m_heroes[winningSide]->m_owner]
                );
                if (!gRetreatWin && m_heroes[COMBAT_ATTACKER_SIDE]
                    && m_heroes[COMBAT_DEFENDER_SIDE]) {
                    for (i = 0; i < HERO_ARTIFACT_SLOT_COUNT; i++) {
                        if (m_heroes[COMBAT_OPPOSING_SIDE(winningSide)]->m_artifacts[i]
                                >= ARTIFACT_REGULAR_FIRST
                            && m_heroes[COMBAT_OPPOSING_SIDE(winningSide)]->m_artifacts[i]
                                   != ARTIFACT_MAGIC_BOOK) {
                            gTransferArtifacts[gMaxTransferArtifacts] =
                                m_heroes[COMBAT_OPPOSING_SIDE(winningSide)]->m_artifacts[i];
                            gMaxTransferArtifacts++;
                        }
                    }
                }
            }
            if (!(gCurPlayer == GAME_PLAYER_NONE || !gThisNetHumanPlayer[gCurPlayer]
                  || m_playerId[winningSide] != gCurPlayer)
                || !(
                    gCurPlayer == GAME_PLAYER_NONE || m_playerId[winningSide] == GAME_PLAYER_NONE
                    || gThisNetHumanPlayer[gCurPlayer]
                    || !gThisNetHumanPlayer[m_playerId[winningSide]]
                )
                || !(
                    m_playerId[winningSide] == GAME_PLAYER_NONE
                    || !gThisNetHumanPlayer[m_playerId[winningSide]]
                )) {
                PlayMusic(MUSIC_TRACK_BATTLE_WON);
                m_winLoseWindow = new heroWindow(0x9f, 2, "wincmbt.bin");
                if (m_winLoseWindow == NULL)
                    MemError();
                if (m_heroes[winningSide]) {
                    if (gCombatSurrender)
                        sprintf(gText, gBattleResults[BATTLE_RESULT_ENEMY_SURRENDERED]);
                    else if (gRetreatWin)
                        sprintf(gText, gBattleResults[BATTLE_RESULT_ENEMY_FLED]);
                    else
                        sprintf(gText, gBattleResults[BATTLE_RESULT_VICTORY]);
                    if (levelsGained > 0 && winningSide == COMBAT_DEFENDER_SIDE
                        && gNumHumanPlayers > 1)
                        sprintf(
                            experienceText,
                            gBattleResults[BATTLE_RESULT_EXPERIENCE_AND_LEVELS],
                            m_heroes[winningSide]->m_name,
                            m_experienceValue[COMBAT_OPPOSING_SIDE(winningSide)],
                            levelsGained
                        );
                    else
                        sprintf(
                            experienceText,
                            gBattleResults[BATTLE_RESULT_EXPERIENCE],
                            m_heroes[winningSide]->m_name,
                            m_experienceValue[COMBAT_OPPOSING_SIDE(winningSide)]
                        );
                    strcat(gText, experienceText);
                    m_heroes[winningSide]->ApplyBattleWinTemps();
                } else {
                    if (gCombatSurrender)
                        sprintf(gText, gBattleResults[BATTLE_RESULT_ENEMY_SURRENDERED]);
                    else if (gRetreatWin)
                        sprintf(gText, gBattleResults[BATTLE_RESULT_ENEMY_FLED]);
                    else
                        sprintf(gText, gBattleResults[BATTLE_RESULT_VICTORY]);
                }
                SET_WIDGET_MESSAGE(message, WIDGET_COMMAND_SET_TEXT, WIN_LOSE_RESULT_TEXT);
                message.text = gText;
                m_winLoseWindow->BroadcastMessage(message);
                ShowDeadArmies(m_winLoseWindow);
                gWindowManager->DoDialog(m_winLoseWindow, WinCombatHandler, false);
                delete m_winLoseWindow;
                if (m_heroes[COMBAT_OPPOSING_SIDE(winningSide)])
                    m_heroes[COMBAT_OPPOSING_SIDE(winningSide)]->ApplyBattleLossTemps();
            } else {
                if (m_heroes[winningSide])
                    m_heroes[winningSide]->ApplyBattleWinTemps();
                if (m_heroes[COMBAT_OPPOSING_SIDE(winningSide)])
                    m_heroes[COMBAT_OPPOSING_SIDE(winningSide)]->ApplyBattleLossTemps();
                PlayMusic(MUSIC_TRACK_BATTLE_LOST);
                DoLoseWindow();
            }
            break;
    }
    gMapX = gAdvManager->m_mapOriginX;
    gMapY = gAdvManager->m_mapOriginY;
}

// Walks the defeated hero across a scrolling backdrop until the window's
// button is released.
#define screenWidth nextWidth  // frame-slot spelling
#define walkFrame walkFrameNum // frame-slot spelling
VA(0x004205a2, 0x4dc)
void combatManager::DoLoseWindow(void) {
    i16 walkFrame;
    i16 scrollX;
    i16 walkPosX;
    i16 walkY;
    tag_message msg;
    heroWindow* loseWindow;
    bitmap* scrollBitmap;
    H1_ENUM_LOCAL(MessageDispatchResult, i16) dispatchResult;
    icon* walkIcon;
    i16 animationY;
    H1_ENUM_LOCAL(CombatSide, i32) losingParty;
    i16 screenWidth;
    i16 animationX;
    i16 animHeight;
    i16 areaWidth;
    i16 stopRequested;
    i32 delay;

    delay = 0xb4;
    animationX = 0x31;
    animationY = 0x26;
    areaWidth = 0xdf;
    animHeight = 0x7d;
    screenWidth = 0x280;
    walkPosX = 0x6f;
    walkY = 0x8a;
    gMaxTransferArtifacts = 0;
    walkFrame = 0;
    scrollX = 0;
    stopRequested = 0;
    if (gCurPlayer == m_playerId[COMBAT_ATTACKER_SIDE]
        && gThisNetHumanPlayer[m_playerId[COMBAT_ATTACKER_SIDE]])
        losingParty = COMBAT_ATTACKER_SIDE;
    else if (gCurPlayer == m_playerId[COMBAT_DEFENDER_SIDE]
             && gThisNetHumanPlayer[m_playerId[COMBAT_DEFENDER_SIDE]])
        losingParty = COMBAT_DEFENDER_SIDE;
    else if (m_playerId[COMBAT_ATTACKER_SIDE] != GAME_PLAYER_NONE
             && gThisNetHumanPlayer[m_playerId[COMBAT_ATTACKER_SIDE]])
        losingParty = COMBAT_ATTACKER_SIDE;
    else
        losingParty = COMBAT_DEFENDER_SIDE;
    loseWindow = new heroWindow(0x9f, 2, "losecmbt.bin");
    if (loseWindow == NULL)
        MemError();
    scrollBitmap = gResourceManager->GetBitmap("losecmbt.bmp");
    gLoadingMonoIcon = true;
    walkIcon = gResourceManager->GetIcon("losewalk.icn");
    gLoadingMonoIcon = false;
    if (m_heroes[losingParty]) {
        if (gCombatSurrender)
            sprintf(
                gText,
                gBattleResults[BATTLE_RESULT_HERO_SURRENDERS],
                m_heroes[losingParty]->m_name
            );
        else if (gRetreatWin)
            sprintf(gText, gBattleResults[BATTLE_RESULT_HERO_FLEES], m_heroes[losingParty]->m_name);
        else
            sprintf(
                gText,
                gBattleResults[BATTLE_RESULT_HERO_DEFEATED],
                m_heroes[losingParty]->m_name
            );
    } else {
        if (gCombatSurrender)
            sprintf(gText, gBattleResults[BATTLE_RESULT_FORCES_SURRENDER]);
        else if (gRetreatWin)
            sprintf(gText, gBattleResults[BATTLE_RESULT_FORCES_FLEE]);
        else
            sprintf(gText, gBattleResults[BATTLE_RESULT_FORCES_DEFEATED]);
    }
    SET_WIDGET_MESSAGE(msg, WIDGET_COMMAND_SET_TEXT, WIN_LOSE_RESULT_TEXT);
    msg.text = gText;
    loseWindow->BroadcastMessage(msg);
    ShowDeadArmies(loseWindow);
    gWindowManager->AddWindow(loseWindow, WINDOW_Z_ORDER_APPEND, 0);
    BlitBitmap(scrollBitmap, scrollX, 0, 0xdf, 0x7d, gWindowManager->m_screen, 0xd0, 0x28);
    walkIcon->FillToBuffer(0x10e, 0x8c, walkFrame, 0, ICON_DRAW_NORMAL, ICON_DRAW_OFFSET_FULL);
    gWindowManager->UpdateScreenRegion(0x9f, 2, 0x140, 0x1ca);
    gTimers[COMBAT_FRAME_TIMER_SLOT] = KBTickCount() + 0xb4;
    do {
        if (gTimers[COMBAT_FRAME_TIMER_SLOT] < KBTickCount()) {
            BlitBitmap(scrollBitmap, scrollX, 0, 0xdf, 0x7d, gWindowManager->m_screen, 0xd0, 0x28);
            walkIcon
                ->FillToBuffer(0x10e, 0x8c, walkFrame, 0, ICON_DRAW_NORMAL, ICON_DRAW_OFFSET_FULL);
            gWindowManager->UpdateScreenRegion(0xd0, 0x28, 0xdf, 0x7d);
            walkFrame++;
            walkFrame = walkFrame % 8;
            scrollX += 2;
            if (scrollX > 0x1a0)
                scrollX = 0;
            gTimers[COMBAT_FRAME_TIMER_SLOT] = KBTickCount() + 0xb4;
        }
        Process1WindowsMessage();
        msg = gInputManager->GetEvent();
        gMouseManager->Main(msg);
        dispatchResult = gWindowManager->Main(msg);
        if (dispatchResult == MESSAGE_DISPATCH_FORWARD && msg.type == MESSAGE_WIDGET
            && msg.command == WIDGET_NOTIFY_DESELECT && msg.id == DIALOG_BUTTON_0)
            stopRequested = 1;
    } while (!stopRequested);
    gWindowManager->RemoveWindow(loseWindow);
    delete loseWindow;
    gResourceManager->Dispose(walkIcon);
    gResourceManager->Dispose(scrollBitmap);
}
#undef screenWidth
#undef walkFrame

// Charges half the stack cost, with no discount.
VA(0x00420a7e, 0x253)
i16 combatManager::DoSurrender(void) {
    heroWindow* window;
    i16 unusedResult;
    i32 armyIndex;
    tag_message message;
    i16 unusedTypeValue;

    gSurrenderCost = 0;
    for (armyIndex = 0; armyIndex < ARMY_GROUP_SLOT_COUNT; armyIndex++) {
        if (m_armies[m_currentSide][armyIndex].IsAlive())
            gSurrenderCost +=
                m_armies[m_currentSide][armyIndex].m_quantity
                * (gMonsterDatabase[m_armies[m_currentSide][armyIndex].m_creatureType].cost / 2);
    }
    unusedTypeValue = 1;
    unusedResult = 2;
    window = new heroWindow(0x55, 0x50, "surrendr.bin");
    if (window == NULL)
        MemError();
    SET_WIDGET_MESSAGE(message, WIDGET_COMMAND_SET_ICON, SURRENDER_PORTRAIT);
    sprintf(gText, "port%04d.icn", m_heroes[COMBAT_OPPOSING_SIDE(m_currentSide)]->m_portrait);
    message.text = gText;
    window->BroadcastMessage(message);
    message.command = WIDGET_COMMAND_SET_TEXT;
    message.id = SURRENDER_TEXT;
    sprintf(
        gText,
        localization::Tr("combat.surrender.offer"),
        m_heroes[COMBAT_OPPOSING_SIDE(m_currentSide)]->m_name,
        gSurrenderCost
    );
    window->BroadcastMessage(message);
    gWindowManager->DoDialog(window, TrueFalseDialogHandler, false);
    delete window;
    return gWindowManager->m_dialogResult == DIALOG_BUTTON_2;
}

// Redraws the grid from the lower of the old and new selector hexes.
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
        m_limitCreature = true;
        DrawFrame(true);
    }
}

// Keys both attacks on the castle side.
VA(0x00420d8c, 0xf0)
void combatManager::CheckCastleAttack(void) {
    if (m_castleSide[COMBAT_OPPOSING_SIDE(m_currentSide)]) {
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

// Tries the retreat first.
VA(0x00420e7c, 0x5c)
void combatManager::CheckGetAIMove(void) {
    if (AICheckRetreat())
        return;
    if (!m_heroCastSpell[m_currentSide] && DoSpellAI(m_currentSide))
        return;
    DoCompAI(m_currentSide);
}

// Always resets the pointer.
VA(0x00420ed8, 0x12b)
void combatManager::GetControl(void) {
    m_selectedHex = ARMY_HEX_INVALID;
    m_previousCommand = COMBAT_INVALID_COMMAND;
    m_previousCommand = COMBAT_INVALID_COMMAND;
    gMouseManager->SetPointer(COMBAT_POINTER_DEFAULT);
    CheckChangeSelector();
    if (!gRemoteOn || m_playerId[COMBAT_ATTACKER_SIDE] < 0 || m_playerId[COMBAT_DEFENDER_SIDE] < 0
        || !gHumanPlayer[m_playerId[COMBAT_DEFENDER_SIDE]]
        || (!gHumanPlayer[m_playerId[COMBAT_ATTACKER_SIDE]]
            && (gHumanPlayer[m_playerId[COMBAT_ATTACKER_SIDE]]
                || !m_playerId[COMBAT_DEFENDER_SIDE]))) {
        gThisNetHasControl = true;
        goto resetMouse;
    }
    if (m_playerId[m_currentSide] != GAME_PLAYER_NONE && gHumanPlayer[m_playerId[m_currentSide]]
        && !gThisNetHumanPlayer[m_playerId[m_currentSide]])
        gThisNetHasControl = false;
    else
        gThisNetHasControl = true;
resetMouse:
    ResetMouse();
}

// Sends a hover over the combat field.
VA(0x00421003, 0xbd)
void combatManager::ResetMouse(void) {
    tag_message message;
    i16 x;
    i16 y;

    if (gThisNetHasControl && m_playerId[m_currentSide] >= 0
        && gHumanPlayer[m_playerId[m_currentSide]]) {
        m_selectedHex = ARMY_HEX_INVALID;
        CombatMessage("", true);
        gMouseManager->MouseCoords(x, y);
        message.type = MESSAGE_WIDGET;
        message.command = WIDGET_COMMAND_HOVER;
        message.id = y <= 0x1ca ? COMBAT_CONTROL_FIELD : COMBAT_CONTROL_NONE;
        ProcessCombatMsg(message);
    } else
        gMouseManager->SetPointer(COMBAT_POINTER_DEFAULT);
}

// Hides the pointer around the action and broadcasts it to a human net
// opponent.
VA(0x004210c0, 0x4c6)
H1_ENUM_RETURN(MessageDispatchResult, i16) combatManager::ProcessNextAction(struct tag_message& message) {
    CombatRemoteAction actionData;
    i32 transmitResult;
    i32 remoteIndex;
    b8 doAdvance;
    army* actingArmy;

    if (gThisNetHasControl && gRemoteOn && m_playerId[COMBAT_ATTACKER_SIDE] >= 0
        && m_playerId[COMBAT_DEFENDER_SIDE] >= 0 && gHumanPlayer[m_playerId[COMBAT_DEFENDER_SIDE]]
        && gHumanPlayer[m_playerId[COMBAT_ATTACKER_SIDE]]) {
        remoteIndex = m_playerId[COMBAT_OPPOSING_SIDE(m_currentSide)];
        if (remoteIndex < 0 || !gHumanPlayer[remoteIndex])
            remoteIndex = gHostGamePos;
        actionData.nextAction = H1_ENUM_ENCODE(CombatAction, gNextAction);
        actionData.nextActionExtra = gNextActionExtra;
        actionData.nextActionGridIndex = gNextActionGridIndex;
        actionData.nextActionGridIndex2 = gNextActionGridIndex2;
        transmitResult = TransmitRemoteData(
            &actionData,
            remoteIndex,
            sizeof(actionData),
            REMOTE_COMMAND_COMBAT_ACTION,
            true
        );
        if (!transmitResult)
            ShutDown(NULL);
    }
    actingArmy = &m_armies[m_currentSide][m_currentArmyIndex];
    doAdvance = false;
    if (CheckWin(&message))
        return MESSAGE_DISPATCH_FORWARD;
    switch (gNextAction) {
        case ACTION_NONE:
            break;
        case ACTION_CAST_SPELL:
            gMouseManager->ReallyHidePointer();
            CastSpell(
                H1_ENUM_DECODE(SpellType, gNextActionExtra),
                gNextActionGridIndex,
                false,
                gNextActionGridIndex2
            );
            if (m_armies[m_currentSide][m_currentArmyIndex].m_quantity <= 0)
                doAdvance = true;
            break;
        case ACTION_MOVE:
            gMouseManager->ReallyHidePointer();
            actingArmy->MoveAttack(gNextActionGridIndex, false);
            actingArmy->m_stats.attributes |= MONSTER_FLAGS_TURN_SPENT;
            if (CheckWin(&message))
                return MESSAGE_DISPATCH_FORWARD;
            CheckApplyGoodMorale(m_currentSide, m_currentArmyIndex);
            doAdvance = true;
            break;
        case ACTION_ATTACK:
            gMouseManager->ReallyHidePointer();
            if (gNextActionExtra != ARMY_HEX_INVALID && actingArmy->m_hex != gNextActionExtra)
                actingArmy->MoveAttack(gNextActionExtra, true);
            actingArmy->MoveAttack(gNextActionGridIndex, false);
            actingArmy->m_stats.attributes |= MONSTER_FLAGS_TURN_SPENT;
            if (CheckWin(&message))
                return MESSAGE_DISPATCH_FORWARD;
            CheckApplyGoodMorale(m_currentSide, m_currentArmyIndex);
            doAdvance = true;
            break;
        case ACTION_RETREAT:
            m_sideRetreated[m_currentSide] = 1;
            gRetreatWin = 1;
            break;
        case ACTION_SURRENDER:
            gCombatSurrender = 1;
            gRetreatWin = 1;
            m_sideSurrendered[m_currentSide] = 1;
            gGame->m_players[m_playerId[m_currentSide]].m_resources[RESOURCE_GOLD] -=
                gNextActionExtra;
            gGame->m_players[m_playerId[COMBAT_OPPOSING_SIDE(m_currentSide)]]
                .m_resources[RESOURCE_GOLD] += gNextActionExtra;
            break;
        case ACTION_SKIP_TURN:
            actingArmy->m_stats.attributes |= MONSTER_FLAGS_TURN_SPENT;
            doAdvance = true;
            break;
    }
    gNextAction = ACTION_NONE;
    if (CheckWin(&message))
        return MESSAGE_DISPATCH_FORWARD;
    if (doAdvance && !GetNextArmy(true)) {
        ResetRound();
        GetNextArmy(true);
    }
    CheckChangeSelector();
    if (gThisNetHasControl && !m_gridSelectionDisabled
        && m_playerId[m_currentSide] != GAME_PLAYER_NONE
        && gThisNetHumanPlayer[m_playerId[m_currentSide]])
        gMouseManager->ReallyShowPointer();
    else
        gMouseManager->ReallyHidePointer();
    return MESSAGE_DISPATCH_CONSUME;
}

// COMMAND globals.
DATA(0x004a6a8c)
b8 gThisNetHasControl;
DATA(0x004a6aa4)
i32 gCurTransferArtifact;
DATA(0x004a6ab0)
i8 gMaxTransferArtifacts;
DATA(0x004a6a88)
i32 gNextActionExtra;
DATA(0x004a6aa8)
i32 gNextActionGridIndex;
DATA(0x004a6ab4)
i32 gSurrenderCost;
DATA(0x004a6a94)
H1_ENUM_STORAGE(ArtifactType, i8) gTransferArtifacts[HERO_ARTIFACT_SLOT_COUNT];
DATA(0x004a6aac)
H1_ENUM_STORAGE(CombatAction, i32) gNextAction;
DATA(0x004a6a90)
i32 gNextActionGridIndex2;

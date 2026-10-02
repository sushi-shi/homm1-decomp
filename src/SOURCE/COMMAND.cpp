// Located from HoMM2 Buka 2.1; PoL 2.0 supplies the VC4 declaration.

#include <match.h>

#include <BASE/INPUTMGR_TYPES.h>

#include <H1/All.h>
#include <H1/KB.h>
#include <BASE/Misc.h>
#include <BASE/bmap2.h>
#include <SOURCE/PATH.h>
#include <SOURCE/REMOTE.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Buka COMMAND.cpp Main; HoMM1 polls sound on the 75-tick timer and has no
// combat screen cycling or no-show mode.
VA(0x0040f2c0, 0x311)
short combatManager::Main(struct tag_message &message)
{
    int result = MESSAGE_DISPATCH_CONSUME;
    CombatRemotePacket *packet;
    army *currentArmy;

    if (KBTickCount() > glTimers[0]) {
        PollSound();
        glTimers[0] = KBTickCount() + 0x4b;
    }
    CheckCastleAttack();
    if (CheckWin(&message))
        return MESSAGE_DISPATCH_FORWARD;
    packet = (CombatRemotePacket *)GetRemoteData(1);
    if (packet && packet->type == 2) {
        switch (packet->command) {
            case 0x17:
                giNextAction = packet->nextAction;
                giNextActionExtra = packet->nextActionExtra;
                giNextActionGridIndex = packet->nextActionGridIndex;
                giNextActionGridIndex2 = packet->nextActionGridIndex2;
                goto processAction;
            case 0xb:
                PopNetBox(packet->text);
                break;
        }
    }
    if (!gbThisNetHasControl) {
        if (message.type == MESSAGE_KEY_DOWN) {
            switch (message.payload.keyboard.keyCode) {
                case 0x3b:
                    PopNetBox(0);
                    break;
            }
        }
        return MESSAGE_DISPATCH_CONSUME;
    }
    currentArmy = &m_armies[m_currentSide][m_currentArmyIndex];
    if (currentArmy->m_spellEffect == 14) {
        currentArmy->GoBerserk();
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
        if (m_playerId[m_currentSide] == -1 || !gbThisNetHumanPlayer[m_playerId[m_currentSide]]
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
signed char combatManager::ValidHexToStandOn(int hex)
{
    if (hex == -2)
        return 1;
    if (hex != -1 && hex % 9 != 8 && hex % 9 != 0 && m_hexCells[hex].m_obstacle == -1
        && (m_hexCells[hex].m_occupantSide == -1
            || (m_hexCells[hex].m_occupantSide == m_currentSide
                && m_hexCells[hex].m_occupantIndex == m_currentArmyIndex)))
        return 1;
    else
        return 0;
}

// Buka COMMAND.cpp SetCombatDirections; HoMM1 reads the global adjacency
// table and keeps the 24-sector map as bytes.
VA(0x0040f6a7, 0x7e9)
void combatManager::SetCombatDirections(int targetHex)
{
    int mapped;
    int numUnset;
    signed char hasPath[8];
    int targetIndex;
    int after;
    int rear[8];
    army *curArmy;
    int before;
    int outDir;
    int dir;
    int directionHexes[8];
    army *target;
    int targetSide;
    signed char canStand[8];

    curArmy = &m_armies[m_currentSide][m_currentArmyIndex];
    targetSide = curArmy->m_targetSide;
    targetIndex = curArmy->m_targetIndex;
    curArmy->m_targetSide = -1;
    curArmy->m_targetIndex = -1;
    target = &m_armies[targetSide][targetIndex];
    for (dir = 0; dir < 8; dir++) {
        if (dir == 6 || dir == 7) {
            if (curArmy->m_attributes & 1) {
                if (curArmy->m_facing == 0) {
                    if (dir == 6)
                        directionHexes[dir] = gCombatAdjacency[targetHex][5];
                    if (dir == 7)
                        directionHexes[dir] = gCombatAdjacency[targetHex][3];
                } else {
                    if (dir == 6)
                        directionHexes[dir] = gCombatAdjacency[targetHex][0];
                    if (dir == 7)
                        directionHexes[dir] = gCombatAdjacency[targetHex][2];
                }
            } else
                directionHexes[dir] = -1;
        } else
            directionHexes[dir] = gCombatAdjacency[targetHex][dir];
        if ((curArmy->m_attributes & 1) && directionHexes[dir] != -1) {
            if (curArmy->m_facing == 0) {
                if (dir == 5 || dir == 4 || dir == 3) {
                    if (directionHexes[dir] % 9 == 1)
                        directionHexes[dir] = -1;
                    else
                        directionHexes[dir]--;
                }
                if (directionHexes[dir] % 9 == 7)
                    rear[dir] = -1;
                else
                    rear[dir] = directionHexes[dir] + 1;
            } else {
                if (dir == 0 || dir == 1 || dir == 2) {
                    if (directionHexes[dir] % 9 == 7)
                        directionHexes[dir] = -1;
                    else
                        directionHexes[dir]++;
                }
                if (directionHexes[dir] % 9 == 1)
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
    if (curArmy->m_attributes & 2) {
        for (dir = 0; dir < 8; dir++)
            hasPath[dir] = canStand[dir];
    } else {
        for (dir = 0; dir < 8; dir++) {
            if (canStand[dir]) {
                if (curArmy->m_hex == directionHexes[dir] || curArmy->ValidPath(directionHexes[dir], 1))
                    hasPath[dir] = 1;
                else
                    hasPath[dir] = 0;
            } else
                hasPath[dir] = 0;
        }
    }
    m_validDirectionCount = 0;
    for (dir = 0; dir < 8; dir++) {
        if (hasPath[dir])
            m_validDirectionCount++;
    }
    if (!m_validDirectionCount)
        hasPath[6] = 1;
    memset(m_directionMap, -1, sizeof(m_directionMap));
    for (dir = 0; dir < 8; dir++) {
        outDir = dir;
        if (dir < 6)
            mapped = (dir + 3) % 6;
        else
            mapped = (signed char)(dir == 6 ? 7 : 6);
        if (hasPath[mapped]) {
            if (target->m_attributes & 1) {
                if (dir == 0 && m_hexCells[targetHex - 1].m_occupantSide == targetSide
                    && m_hexCells[targetHex - 1].m_occupantIndex == targetIndex)
                    outDir = 6;
                else if (dir == 5 && m_hexCells[targetHex + 1].m_occupantSide == targetSide
                         && m_hexCells[targetHex + 1].m_occupantIndex == targetIndex)
                    outDir = 6;
                else if (dir == 2 && m_hexCells[targetHex - 1].m_occupantSide == targetSide
                         && m_hexCells[targetHex - 1].m_occupantIndex == targetIndex)
                    outDir = 7;
                else if (dir == 3 && m_hexCells[targetHex + 1].m_occupantSide == targetSide
                         && m_hexCells[targetHex + 1].m_occupantIndex == targetIndex)
                    outDir = 7;
            }
            if (dir < 6)
                memset(&m_directionMap[mapped * 4], outDir, 4);
            else if (dir == 6) {
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
void combatManager::CheckSetMouseDirection(int mouseX, int mouseY, int targetHex)
{
    int hexDir;
    int savedDir;
    int alternate;
    float ratio;
    int index;
    army *curArmy;
    int distX;
    int distY;
    army *target;
    int backHex;

    if (m_gridSelectionDisabled)
        return;
    if (m_validDirectionCount <= 1 && m_mouseDirection >= 0)
        return;
    distX = mouseX - (targetHex % 9 - 1) * 78;
    if ((targetHex / 9) & 1)
        distX -= 0x42;
    else
        distX -= 0x1b;
    distY = mouseY - 0x3c - targetHex / 9 * 80;
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
    ratio = (float)distX / ((float)distY);
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
    alternate = -1;
    curArmy = &m_armies[m_currentSide][m_currentArmyIndex];
    target = &m_armies[curArmy->m_targetSide][curArmy->m_targetIndex];
    if (hexDir == 6 || hexDir == 7) {
        if (curArmy->m_attributes & 1) {
            if (curArmy->m_facing == 0 && hexDir == 6) {
                hexDir = 5;
                alternate = 0;
            } else if (curArmy->m_facing == 0 && hexDir == 7) {
                hexDir = 3;
                alternate = 2;
            } else if (curArmy->m_facing == 1 && hexDir == 6) {
                hexDir = 0;
                alternate = 5;
            } else {
                hexDir = 2;
                alternate = 3;
            }
        } else {
            if (m_hexCells[targetHex - 1].m_occupantSide == curArmy->m_targetSide
                && m_hexCells[targetHex - 1].m_occupantIndex == curArmy->m_targetIndex)
                targetHex--;
            if (hexDir == 6)
                hexDir = 0;
            else
                hexDir = 2;
        }
    } else {
        if (curArmy->m_facing == 0 && (curArmy->m_attributes & 1)) {
            if (hexDir == 5 || hexDir == 4 || hexDir == 3)
                targetHex--;
        } else if (curArmy->m_facing == 1 && (curArmy->m_attributes & 1)
                   && (hexDir == 0 || hexDir == 1 || hexDir == 2))
            targetHex++;
    }
    m_directionTargetHex = gCombatAdjacency[targetHex][hexDir];
    backHex = -2;
    if (curArmy->m_facing == 1 && (curArmy->m_attributes & 1))
        backHex = m_directionTargetHex - 1;
    if (curArmy->m_facing == 0 && (curArmy->m_attributes & 1))
        backHex = m_directionTargetHex + 1;
    if (!ValidHexToStandOn(m_directionTargetHex) || !ValidHexToStandOn(backHex)) {
        if ((curArmy->m_attributes & 1) && (savedDir == 6 || savedDir == 7)) {
            if (curArmy->m_facing == 0)
                m_directionTargetHex += 1;
            else
                m_directionTargetHex -= 1;
        } else {
            if (alternate != -1)
                m_directionTargetHex = gCombatAdjacency[targetHex][alternate];
        }
    }
    gpMouseManager->SetPointer(m_mouseDirection + 7);
}

// Buka GetPointer precedes ProcessCombatMsg. HoMM1's sole caller passes one
// command and retail maps command 13 to pointer 5, preserving all others.
VA(0x004104b0, 0x34)
H1_ENUM_RETURN(CombatPointerCode, int)
combatManager::GetPointer(H1_ENUM_PARAM(CombatPointerCode, int) command)
{
    if (command == COMBAT_MESSAGE_COMMAND_OPPOSING_OPTIONS)
        return COMBAT_POINTER_VIEW;
    else
        return command;
}

// Buka COMMAND.cpp ProcessCombatMsg; HoMM1 hovers the combat field as
// widget 0x40 and handles F1, space, H, T and C keys.
VA(0x004104e4, 0x5bb)
int combatManager::ProcessCombatMsg(struct tag_message &message)
{
    short mouseX = message.payload.mouse.x;
    short mouseY = message.payload.mouse.y;
    signed char unused = 0;
    short selectedHex;

    if (!(m_messageTypeMask & message.type))
        return 0;
    switch (message.type) {
        case MESSAGE_WIDGET:
            switch (message.payload.widget.command) {
                case WIDGET_COMMAND_HOVER:
                    if (m_gridSelectionDisabled)
                        break;
                    switch (message.payload.widget.id) {
                        case 0x40:
                            gpMouseManager->MouseCoords(mouseX, mouseY);
                            selectedHex = GetGridIndex(mouseX, mouseY);
                            if (m_selectedHex != selectedHex || selectedHex == -1) {
                                m_selectedHex = selectedHex;
                                m_previousCommand = -99;
                                m_currentCommand = GetCommand(m_selectedHex);
                                m_mouseDirection = -1;
                                if (m_currentCommand == 7) {
                                    SetCombatDirections(selectedHex);
                                    CheckSetMouseDirection(mouseX, mouseY, selectedHex);
                                } else
                                    gpMouseManager->SetPointer(GetPointer(m_currentCommand));
                            } else if (m_currentCommand == 7)
                                CheckSetMouseDirection(mouseX, mouseY, selectedHex);
                            if (m_previousCommand != m_currentCommand) {
                                m_previousCommand = m_currentCommand;
                                CombatMessage(m_currentCommand);
                            }
                            break;
                        default:
                            gpMouseManager->MouseCoords(mouseX, mouseY);
                            if (mouseX <= 0x32)
                                CombatMessage(cCombatHelp[0], 1);
                            else if (mouseX >= 0x24e)
                                CombatMessage(cCombatHelp[1], 1);
                            else
                                CombatMessage(cCombatHelp[2], 1);
                            gpMouseManager->SetPointer(6);
                            m_selectedHex = -1;
                            m_previousCommand = -99;
                            break;
                    }
                    break;
                case WIDGET_NOTIFY_SELECT:
                    if (message.payload.widget.modifiers & MESSAGE_MODIFIER_RIGHT_BUTTON)
                        RightClick(m_selectedHex);
                    else {
                        switch (message.payload.widget.id) {
                            case 0x40:
                                DoCommand(m_currentCommand);
                                break;
                        }
                    }
                    break;
                case WIDGET_NOTIFY_DESELECT:
                    switch (message.payload.widget.id) {
                        case 2:
                            if (!(message.payload.widget.modifiers & MESSAGE_MODIFIER_RIGHT_BUTTON)) {
                                m_gridSelectionDisabled = 1;
                                gpMouseManager->ReallyHidePointer();
                            }
                            break;
                        case 8:
                            if (!(message.payload.widget.modifiers & MESSAGE_MODIFIER_RIGHT_BUTTON))
                                giNextAction = 3;
                            break;
                    }
                    break;
            }
            break;
        case MESSAGE_KEY_DOWN:
            switch (message.payload.keyboard.keyCode) {
                case 0x3b:
                    PopNetBox(0);
                    break;
                case 0x39:
                    giNextAction = 3;
                    break;
                case 0x23:
                    if (m_heroes[m_currentSide]) {
                        gpMouseManager->SetPointer(6);
                        ViewGeneral(m_currentSide, 1, 0);
                        ResetMouse();
                    }
                    break;
                case 0x14:
                    gpMouseManager->SetPointer(6);
                    ViewArmy(&m_armies[m_currentSide][m_currentArmyIndex], m_currentSide, 0);
                    ResetMouse();
                    break;
                case 0x2e:
                    if (!m_heroes[m_currentSide])
                        NormalDialog("You have no hero to cast a spell.", 1, -1, -1, -1, 0, -1, 0, -1);
                    else if (m_heroCastSpell[m_currentSide])
                        NormalDialog("You have already cast a spell this round.", 1, -1, -1, -1, 0, -1, 0, -1);
                    else {
                        gpMouseManager->SetPointer(6);
                        giCurGeneral = m_currentSide;
                        ViewSpells(0);
                        ResetMouse();
                    }
                    break;
            }
            break;
    }
    return 1;
}

// Buka COMMAND.cpp ResetRound; HoMM1 has five stacks a side, one keep and
// a byte spell-round counter.
VA(0x00410a9f, 0x139)
void combatManager::ResetRound(void)
{
    int unusedRoundWord;
    int index;
    int side;
    army *curArmy;

    m_catapultAttacksRemaining[1] = m_catapultAttackCount[1];
    m_catapultAttacksRemaining[0] = m_catapultAttackCount[0];
    m_keepAttacksRemaining[1] = 1;
    m_keepAttacksRemaining[0] = 1;
    m_heroCastSpell[1] = m_heroCastSpell[0] = 0;
    for (side = 0; side < 2; side++) {
        for (index = 0; index < 5; index++) {
            curArmy = &m_armies[side][index];
            if (curArmy->m_quantity > 0) {
                curArmy->m_attributes &= 0x1f;
                if (curArmy->m_creatureType == 10)
                    curArmy->m_hitPointsLost = 0;
                if (curArmy->m_spellRounds > 0) {
                    curArmy->m_spellRounds--;
                    if (curArmy->m_spellRounds == 0)
                        curArmy->CancelSpell();
                }
            }
        }
    }
    m_currentSpeed = 4;
}

// Buka COMMAND.cpp CheckWin; HoMM1 returns the byte flag and names the
// winning side directly (-1 for a draw).
VA(0x00410bd8, 0x15d)
int combatManager::CheckWin(struct tag_message *message)
{
    int armyIndex;
    signed char combatEnded;
    int unusedWinWord;

    combatEnded = 0;
    if (IsWinner(m_currentSide)) {
        combatEnded = 1;
        if (IsWinner(1 - m_currentSide))
            m_combatResult = -1;
        else
            m_combatResult = m_currentSide;
    } else if (IsWinner(1 - m_currentSide)) {
        combatEnded = 1;
        m_combatResult = 1 - m_currentSide;
    } else if (m_sideRetreated[1] || m_sideRetreated[0]) {
        combatEnded = 1;
        gbRetreatWin = 1;
        if (m_sideRetreated[1])
            m_combatResult = 0;
        else
            m_combatResult = 1;
    }
    if (combatEnded) {
        DoVictory(m_combatResult);
        message->type = MESSAGE_EXECUTIVE;
        message->payload.executive.command = EXECUTIVE_COMMAND_TERMINATE_LOOP;
    }
    return combatEnded;
}

// Buka COMMAND.cpp GetCommand; HoMM1 returns each command directly, has no
// small view or ballista and clears the target through the current stack.
VA(0x00410d35, 0x316)
signed char combatManager::GetCommand(short hex)
{
    signed char unusedCol = hex % 9;
    signed char rowIndex = hex / 9;
    army *currentArmy;
    signed char targetIndex;
    signed char enemySide;

    if (hex == -1)
        return 0;
    switch (hex) {
        case 26:
            if (m_heroes[0]) {
                if (m_currentSide == 0)
                    return 4;
                else
                    return 13;
            } else
                return 0;
        case 9:
            if (m_heroes[1]) {
                if (m_currentSide == 1)
                    return 4;
                else
                    return 13;
            } else
                return 0;
        default:
            if (hex % 9 == 8)
                return 0;
            enemySide = m_hexCells[hex].m_occupantSide;
            targetIndex = m_hexCells[hex].m_occupantIndex;
            currentArmy = &m_armies[m_currentSide][m_currentArmyIndex];
            currentArmy->m_targetSide = -1;
            currentArmy->m_targetIndex = -1;
            if (m_hexCells[hex].m_obstacle != -1)
                return 0;
            else if (enemySide != -1) {
                switch (enemySide) {
                    case 0:
                    case 1:
                        if (m_currentSide == enemySide)
                            return 5;
                        else {
                            currentArmy->m_targetSide = enemySide;
                            currentArmy->m_targetIndex = targetIndex;
                            if (currentArmy->m_shots > 0
                                && currentArmy->GetAttackMask(currentArmy->m_hex, 1, -1) == 0xff)
                                return 3;
                            if (currentArmy->ValidPath(hex, 1) == 1)
                                return 7;
                            else {
                                currentArmy->m_targetSide = -1;
                                currentArmy->m_targetIndex = -1;
                                return 0;
                            }
                        }
                }
            } else {
                if (m_armies[m_currentSide][m_currentArmyIndex].ValidPath(hex, 0) == 1)
                    return (m_armies[m_currentSide][m_currentArmyIndex].m_attributes & 2) ? 2 : 1;
            }
            break;
    }
    return 0;
}

// Buka COMMAND.cpp RightClick; HoMM1 hero hexes are 26 and 9 and the
// army view also takes the side.
VA(0x0041104b, 0x1dc)
signed char combatManager::RightClick(signed char hex)
{
    signed char unusedColumn = hex % 9;
    signed char row = hex / 9;

    if (hex == -1)
        return 0;
    switch (hex) {
        case 26:
            if (m_heroes[0]) {
                ViewGeneral(0, 0, 1);
                ResetMouse();
            }
            return 0;
        case 9:
            if (m_heroes[1]) {
                ViewGeneral(1, 0, 1);
                ResetMouse();
            }
            return 0;
        default:
            if (hex % 9 == 8)
                return 0;
            signed char side = m_hexCells[hex].m_occupantSide;
            signed char armyIndex = m_hexCells[hex].m_occupantIndex;
            if (m_hexCells[hex].m_obstacle != -1)
                return 0;
            else if (side != -1) {
                switch (side) {
                    case 0:
                    case 1:
                        gpMouseManager->SetPointer(6);
                        ViewArmy(&m_armies[side][m_hexCells[m_selectedHex].m_occupantIndex], side, 1);
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
void combatManager::DoCommand(signed char command)
{
    int unusedValue1;
    int unusedValue2;
    army *currentArmy = &m_armies[m_currentSide][m_currentArmyIndex];

    switch (command) {
        case 0:
            break;
        case 1:
        case 2:
        case 3:
            giNextAction = 2, giNextActionGridIndex = m_selectedHex;
            giNextActionExtra = -1;
            break;
        case 7:
            giNextActionGridIndex = m_selectedHex;
            if (m_playerId[m_currentSide] == -1 || !gbHumanPlayer[m_playerId[m_currentSide]]
                || m_gridSelectionDisabled) {
                giNextAction = 2;
                giNextActionExtra = -1;
            } else {
                giNextAction = 6;
                giNextActionExtra = m_directionTargetHex;
            }
            break;
        case 4:
            gpMouseManager->SetPointer(6);
            ViewGeneral(m_currentSide, 1, 0);
            ResetMouse();
            break;
        case 13:
            gpMouseManager->SetPointer(6);
            ViewGeneral(1 - m_currentSide, 1, 0);
            ResetMouse();
            break;
        case 5:
            gpMouseManager->SetPointer(6);
            ViewArmy(&m_armies[m_currentSide][m_hexCells[m_selectedHex].m_occupantIndex], m_currentSide, 0);
            ResetMouse();
            break;
        case 10:
            ViewSpells(0);
            ResetMouse();
            break;
        case 11:
            NormalDialog("Are you sure you want to retreat?", 2, 0xc3, 0x3c, -1, 0, -1, 0, -1);
            if (gpWindowManager->m_dialogResult == 0x7805)
                giNextAction = 4;
            ResetMouse();
            break;
        case 12:
            if (DoSurrender() == 1) {
                if (gpGame->m_players[m_playerId[m_currentSide]].m_resources[RESOURCE_GOLD] < giSurrenderCost)
                    NormalDialog("You don't have enough gold!", 1, -1, -1, -1, 0, -1, 0, -1);
                else {
                    giNextAction = 5;
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
short WinCombatHandler(struct tag_message &message)
{
    int finalDelay = 0x5a;
    short frame = 1;

    if (message.type == MESSAGE_WIDGET) {
        switch (message.payload.widget.command) {
            case WIDGET_NOTIFY_DESELECT:
                switch (message.payload.widget.id) {
                    case 0x7800:
                        if (iMaxTransferArtifacts > iCurTransferArtifact + 1) {
                            gpCombatManager->ClearWinLoseBottom(gpCombatManager->m_winLoseWindow);
                            iCurTransferArtifact++;
                            gpCombatManager->ShowWinLoseArtifact(gpCombatManager->m_winLoseWindow,
                                                                 iTransferArtifacts[iCurTransferArtifact]);
                        } else {
                            gpWindowManager->m_dialogResult = message.payload.widget.id;
                            message.payload.widget.command = message.payload.widget.id = 10;
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
    if (KBTickCount() > glTimers[0]) {
        message.type = MESSAGE_WIDGET;
        message.payload.widget.command = WIDGET_COMMAND_SET_FRAME;
        message.payload.widget.id = 1;
        gpGame->m_viewArmyResult++;
        message.payload.widget.data.value = gpGame->m_viewArmyResult % 6 + 1;
        gpCombatManager->m_winLoseWindow->BroadcastMessage(message);
        gpCombatManager->m_winLoseWindow->DrawWindow();
        glTimers[0] = KBTickCount() + 0x5a;
    }
    return MESSAGE_DISPATCH_CONSUME;
}

// Buka COMMAND.cpp ClearWinLoseBottom (fifteen icon/text widget pairs).
VA(0x004116fd, 0x110)
void combatManager::ClearWinLoseBottom(class heroWindow *window)
{
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
        m_winLoseBottomWidgets[i] = 0;
        m_winLoseBottomTextWidgets[i] = 0;
    }
}

// Buka COMMAND.cpp ShowWinLoseArtifact.
VA(0x0041180d, 0x2fa)
void combatManager::ShowWinLoseArtifact(class heroWindow *window, int artifact)
{
    char *artifactName;
    short boxWidth = 0x140;
    short bottom = 0x1ca;
    tag_message message;

    sprintf(gText, "You have captured an enemy artifact!");
    message.type = MESSAGE_WIDGET;
    message.payload.widget.command = WIDGET_COMMAND_SET_TEXT;
    message.payload.widget.id = 0x65;
    message.payload.widget.data.text = gText;
    m_winLoseWindow->BroadcastMessage(message);
    m_winLoseBottomWidgets[0] = new iconWidget(0x78, 0x136, 0x50, 0x50, "winloseb.icn", 0, 0, 0x7d1, 0x10, 1);
    if (m_winLoseBottomWidgets[0] == 0)
        MemError();
    window->AddWidget(m_winLoseBottomWidgets[0], -1);
    m_winLoseBottomWidgets[1] = new iconWidget(0x80, 0x13e, 0x40, 0x40, "artifact.icn", artifact, 0, 0x7d2, 0x10, 1);
    if (m_winLoseBottomWidgets[1] == 0)
        MemError();
    window->AddWidget(m_winLoseBottomWidgets[1], -1);
    artifactName = (char *)malloc(0x3c);
    sprintf(artifactName, gArtifactNames[artifact]);
    m_winLoseBottomTextWidgets[0] =
        new textWidget(0, 0x18a, 0x140, 0xc, artifactName, "smalfont.fnt", 1, 0x835, 0x200);
    if (m_winLoseBottomTextWidgets[0] == 0)
        MemError();
    window->AddWidget(m_winLoseBottomTextWidgets[0], -1);
    gpCombatManager->m_winLoseWindow->DrawWindow();
    {
        SAMPLE2 sample = NULL_SAMPLE2;
        sprintf(gText, "pickup%02d.82M", SRandom(1, 5));
        sample = LoadPlaySample(gText);
        WaitEndSample(sample, -1);
    }
}

// Buka COMMAND.cpp ShowDeadArmies; HoMM1 lays out up to five casualties a
// side with fixed 40-pixel spacing.
VA(0x00411b07, 0x7d0)
void combatManager::ShowDeadArmies(class heroWindow *window)
{
    char *buffer;
    int casualtyType[2][5];
    int iconSpacing;
    short boxWidth = 0x140;
    short bottom = 0x1ca;
    int armyIndex;
    int side;
    int rowY;
    tag_message message;
    int numLost[2];
    int casualtyCount[2][5];
    int firstX;

    for (side = 0; side < 15; side++) {
        m_winLoseBottomWidgets[side] = 0;
        m_winLoseBottomTextWidgets[side] = 0;
    }
    for (side = 0; side < 2; side++) {
        numLost[side] = 0;
        for (armyIndex = 0; armyIndex < 5; armyIndex++) {
            if (m_armies[side][armyIndex].m_creatureType != -1
                && m_armies[side][armyIndex].m_initialQuantity > m_armies[side][armyIndex].m_quantity) {
                casualtyType[side][numLost[side]] = m_armies[side][armyIndex].m_creatureType;
                casualtyCount[side][numLost[side]] =
                    m_armies[side][armyIndex].m_initialQuantity - m_armies[side][armyIndex].m_quantity;
                numLost[side]++;
            }
        }
    }
    buffer = (char *)malloc(0x1e);
    sprintf(buffer, "Battlefield Casualties");
    m_winLoseBottomTextWidgets[12] = new textWidget(0, 0x104, 0x140, 0x14, buffer, "smalfont.fnt", 1, 0x83e, 0x200);
    if (m_winLoseBottomTextWidgets[12] == 0)
        MemError();
    window->AddWidget(m_winLoseBottomTextWidgets[12], -1);
    for (side = 0; side < 2; side++) {
        if (side == 1)
            rowY = 0x118;
        else
            rowY = 0x159;
        buffer = (char *)malloc(0x1e);
        sprintf(buffer, side == 1 ? "Attacker" : "Defender");
        m_winLoseBottomTextWidgets[10 + side] =
            new textWidget(0, rowY, 0x140, 0x14, buffer, "smalfont.fnt", 1, 0x83e, 0x200);
        if (m_winLoseBottomTextWidgets[10 + side] == 0)
            MemError();
        window->AddWidget(m_winLoseBottomTextWidgets[10 + side], -1);
        if (numLost[side] <= 0) {
            buffer = (char *)malloc(10);
            sprintf(buffer, "None");
            m_winLoseBottomTextWidgets[side * 5] =
                new textWidget(0, rowY + 0x12, 0x140, 0x14, buffer, "smalfont.fnt", 1, side * 5 + 0x834, 0x200);
            if (m_winLoseBottomTextWidgets[side * 5] == 0)
                MemError();
            window->AddWidget(m_winLoseBottomTextWidgets[side * 5], -1);
        }
        iconSpacing = 0x28;
        firstX = (0x140 - numLost[side] * iconSpacing) / 2 + 3;
        for (armyIndex = 0; armyIndex < numLost[side]; armyIndex++) {
            m_winLoseBottomWidgets[side * 5 + armyIndex] =
                new iconWidget(armyIndex * iconSpacing + firstX, rowY + 0xf, 0x20, 0x1c, "mons32.icn",
                               casualtyType[side][armyIndex], 0, side * 5 + armyIndex + 0x7d0, 0x10, 1);
            if (m_winLoseBottomWidgets[side * 5 + armyIndex] == 0)
                MemError();
            buffer = (char *)malloc(9);
            sprintf(buffer, "%d", casualtyCount[side][armyIndex]);
            m_winLoseBottomTextWidgets[side * 5 + armyIndex] =
                new textWidget(armyIndex * iconSpacing + firstX, rowY + 0x2e, 0x20, 0xc, buffer, "smalfont.fnt", 1,
                               side * 5 + armyIndex + 0x834, 0x200);
            if (m_winLoseBottomTextWidgets[side * 5 + armyIndex] == 0)
                MemError();
            window->AddWidget(m_winLoseBottomWidgets[side * 5 + armyIndex], -1);
            window->AddWidget(m_winLoseBottomTextWidgets[side * 5 + armyIndex], -1);
        }
    }
}

// Buka COMMAND.cpp DoVictory; HoMM1 has no necromancy or eagle eye and
// grabs the screen instead of fading it.
VA(0x004122d7, 0x7a1)
void combatManager::DoVictory(signed char winningSide)
{
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
    gpMouseManager->SetPointer(6);
    switch (winningSide) {
        case -1:
            gpSoundManager->SwitchAmbientMusic(0x2b);
            DoLoseWindow();
            break;
        case 0:
        case 1:
            if (m_heroes[winningSide]) {
                m_experienceValue[1 - winningSide] = ExperienceValueOfStack(1 - winningSide);
                if (gbRetreatWin)
                    m_experienceValue[1 - winningSide] -= 500;
                if (m_combatTown && winningSide == 1)
                    m_experienceValue[1 - winningSide] += 500;
                levelsGained = gpAdvManager->GiveExperience(m_heroes[winningSide], m_experienceValue[1 - winningSide],
                                                            !gbThisNetHumanPlayer[m_heroes[winningSide]->m_owner]);
                if (!gbRetreatWin && m_heroes[1] && m_heroes[0]) {
                    for (i = 0; i < 14; i++) {
                        if (m_heroes[1 - winningSide]->m_artifacts[i] >= 4
                            && m_heroes[1 - winningSide]->m_artifacts[i] != 0x25) {
                            iTransferArtifacts[iMaxTransferArtifacts] = m_heroes[1 - winningSide]->m_artifacts[i];
                            iMaxTransferArtifacts++;
                        }
                    }
                }
            }
            if (!(giCurPlayer == -1 || !gbThisNetHumanPlayer[giCurPlayer] || m_playerId[winningSide] != giCurPlayer)
                || !(giCurPlayer == -1 || m_playerId[winningSide] == -1 || gbThisNetHumanPlayer[giCurPlayer]
                     || !gbThisNetHumanPlayer[m_playerId[winningSide]])
                || !(m_playerId[winningSide] == -1 || !gbThisNetHumanPlayer[m_playerId[winningSide]])) {
                gpSoundManager->SwitchAmbientMusic(0x2c);
                m_winLoseWindow = new heroWindow(0x9f, 2, "wincmbt.bin");
                if (m_winLoseWindow == 0)
                    MemError();
                if (m_heroes[winningSide]) {
                    if (gbCombatSurrender)
                        sprintf(gText, cBattleResults[0]);
                    else if (gbRetreatWin)
                        sprintf(gText, cBattleResults[1]);
                    else
                        sprintf(gText, cBattleResults[2]);
                    if (levelsGained > 0 && winningSide == 0 && giNumHumanPlayers > 1)
                        sprintf(expText, cBattleResults[10], m_heroes[winningSide]->m_name,
                                m_experienceValue[1 - winningSide], levelsGained);
                    else
                        sprintf(expText, cBattleResults[3], m_heroes[winningSide]->m_name,
                                m_experienceValue[1 - winningSide]);
                    strcat(gText, expText);
                    m_heroes[winningSide]->ApplyBattleWinTemps();
                } else {
                    if (gbCombatSurrender)
                        sprintf(gText, cBattleResults[0]);
                    else if (gbRetreatWin)
                        sprintf(gText, cBattleResults[1]);
                    else
                        sprintf(gText, cBattleResults[2]);
                }
                message.type = MESSAGE_WIDGET;
                message.payload.widget.command = WIDGET_COMMAND_SET_TEXT;
                message.payload.widget.id = 0x65;
                message.payload.widget.data.text = gText;
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
                gpSoundManager->SwitchAmbientMusic(0x2b);
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
void combatManager::DoLoseWindow(void)
{
    short walkFrame;
    short lAnimY;
    short unusedWalkX;
    short unusedWalkY;
    tag_message message;
    heroWindow *loseWindow;
    bitmap *bmp;
    short result;
    int iDelay;
    short offset;
    int losingSide;
    short width;
    short animX;
    short blitHeight;
    short iAreaWidth;
    short stop;
    icon *walkIcon;

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
    if (m_playerId[1] == giCurPlayer && gbThisNetHumanPlayer[m_playerId[1]])
        losingSide = 1;
    else if (m_playerId[0] == giCurPlayer && gbThisNetHumanPlayer[m_playerId[0]])
        losingSide = 0;
    else if (m_playerId[1] != -1 && gbThisNetHumanPlayer[m_playerId[1]])
        losingSide = 1;
    else
        losingSide = 0;
    loseWindow = new heroWindow(0x9f, 2, "losecmbt.bin");
    if (loseWindow == 0)
        MemError();
    bmp = gpResourceManager->GetBitmap("losecmbt.bmp");
    gbLoadingMonoIcon = 1;
    walkIcon = gpResourceManager->GetIcon("losewalk.icn");
    gbLoadingMonoIcon = 0;
    if (m_heroes[losingSide]) {
        if (gbCombatSurrender)
            sprintf(gText, cBattleResults[4], m_heroes[losingSide]->m_name);
        else if (gbRetreatWin)
            sprintf(gText, cBattleResults[5], m_heroes[losingSide]->m_name);
        else
            sprintf(gText, cBattleResults[6], m_heroes[losingSide]->m_name);
    } else {
        if (gbCombatSurrender)
            sprintf(gText, cBattleResults[7]);
        else if (gbRetreatWin)
            sprintf(gText, cBattleResults[8]);
        else
            sprintf(gText, cBattleResults[9]);
    }
    message.type = MESSAGE_WIDGET;
    message.payload.widget.command = WIDGET_COMMAND_SET_TEXT;
    message.payload.widget.id = 0x65;
    message.payload.widget.data.text = gText;
    loseWindow->BroadcastMessage(message);
    ShowDeadArmies(loseWindow);
    gpWindowManager->AddWindow(loseWindow, -1, 0);
    BlitBitmap(bmp, offset, 0, 0xdf, 0x7d, gpWindowManager->m_screen, 0xd0, 0x28);
    walkIcon->FillToBuffer(0x10e, 0x8c, walkFrame, 0, 0, 0);
    gpWindowManager->UpdateScreenRegion(0x9f, 2, 0x140, 0x1ca);
    glTimers[0] = KBTickCount() + 0xb4;
    do {
        if (KBTickCount() > glTimers[0]) {
            BlitBitmap(bmp, offset, 0, 0xdf, 0x7d, gpWindowManager->m_screen, 0xd0, 0x28);
            walkIcon->FillToBuffer(0x10e, 0x8c, walkFrame, 0, 0, 0);
            gpWindowManager->UpdateScreenRegion(0xd0, 0x28, 0xdf, 0x7d);
            walkFrame++;
            walkFrame = walkFrame % 8;
            offset = offset + 2;
            if (offset > 0x1a0)
                offset = 0;
            glTimers[0] = KBTickCount() + 0xb4;
        }
        Process1WindowsMessage();
        message = gpInputManager->GetEvent();
        gpMouseManager->Main(message);
        result = gpWindowManager->Main(message);
        if (result == MESSAGE_DISPATCH_FORWARD && message.type == MESSAGE_WIDGET
            && message.payload.widget.command == WIDGET_NOTIFY_DESELECT && message.payload.widget.id == 0x7800)
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
short combatManager::DoSurrender(void)
{
    heroWindow *win;
    short unusedResult;
    int armyIndex;
    tag_message message;
    short unusedType;

    giSurrenderCost = 0;
    for (armyIndex = 0; armyIndex < 5; armyIndex++) {
        if (m_armies[m_currentSide][armyIndex].IsAlive())
            giSurrenderCost += gMonsterDatabase[m_armies[m_currentSide][armyIndex].m_creatureType].cost / 2
                               * m_armies[m_currentSide][armyIndex].m_quantity;
    }
    unusedType = 1;
    unusedResult = 2;
    win = new heroWindow(0x55, 0x50, "surrendr.bin");
    if (win == 0)
        MemError();
    message.type = MESSAGE_WIDGET;
    message.payload.widget.command = WIDGET_COMMAND_SET_ICON;
    message.payload.widget.id = 1;
    sprintf(gText, "port%04d.icn", m_heroes[1 - m_currentSide]->m_unknown1d);
    message.payload.widget.data.text = gText;
    win->BroadcastMessage(message);
    message.payload.widget.command = WIDGET_COMMAND_SET_TEXT;
    message.payload.widget.id = 2;
    sprintf(gText,
            "%s states:\n\n\"I will accept your surrender and grant you and your troops safe passage for the "
            "price of %d gold.",
            m_heroes[1 - m_currentSide]->m_name, giSurrenderCost);
    win->BroadcastMessage(message);
    gpWindowManager->DoDialog(win, TrueFalseDialogHandler, 0);
    delete win;
    return gpWindowManager->m_dialogResult == 0x7802;
}

// Buka COMMAND.cpp CheckChangeSelector; HoMM1 redraws the grid from the
// lower of the old and new selector hexes.
VA(0x00413287, 0xc2)
void combatManager::CheckChangeSelector(void)
{
    army *currentArmy;

    currentArmy = &m_armies[m_currentSide][m_currentArmyIndex];
    if (!m_limitCreature || m_limitCreatureHex != currentArmy->m_hex) {
        UpdateGrid(m_limitCreatureHex > currentArmy->m_hex ? currentArmy->m_hex : m_limitCreatureHex, 1);
        m_limitCreatureHex = currentArmy->m_hex;
        m_limitCreature = 1;
        DrawFrame(1);
    }
}

// Buka COMMAND.cpp CheckCastleAttack; HoMM1 keys both on the castle side.
VA(0x00413349, 0xdf)
void combatManager::CheckCastleAttack(void)
{
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
void combatManager::CheckGetAIMove(void)
{
    if (AICheckRetreat())
        return;
    if (!m_heroCastSpell[m_currentSide] && DoSpellAI(m_currentSide))
        return;
    DoCompAI(m_currentSide);
}

// Buka COMMAND.cpp GetControl; HoMM1 always resets the pointer and has no
// small view.
VA(0x004134a1, 0x16a)
void combatManager::GetControl(void)
{
    m_selectedHex = -1;
    m_previousCommand = -99;
    m_previousCommand = -99;
    gpMouseManager->SetPointer(6);
    CheckChangeSelector();
    if (!gbRemoteOn || m_playerId[1] < 0 || m_playerId[0] < 0 || !gbHumanPlayer[m_playerId[0]]
        || (!gbHumanPlayer[m_playerId[1]] && (gbHumanPlayer[m_playerId[1]] || !m_playerId[0]))) {
        gbThisNetHasControl = 1;
        goto resetMouse;
    }
    if (m_playerId[m_currentSide] != -1 && gbHumanPlayer[m_playerId[m_currentSide]]
        && !gbThisNetHumanPlayer[m_playerId[m_currentSide]])
        gbThisNetHasControl = 0;
    else
        gbThisNetHasControl = 1;
resetMouse:
    ResetMouse();
}

// Buka COMMAND.cpp ResetMouse; HoMM1 sends a hover over the combat field.
VA(0x0041360b, 0xdb)
void combatManager::ResetMouse(void)
{
    tag_message message;
    short x;
    short y;

    if (gbThisNetHasControl && m_playerId[m_currentSide] >= 0 && gbHumanPlayer[m_playerId[m_currentSide]]) {
        m_selectedHex = -1;
        CombatMessage("", 1);
        gpMouseManager->MouseCoords(x, y);
        message.type = MESSAGE_WIDGET;
        message.payload.widget.command = WIDGET_COMMAND_HOVER;
        if (y > 0x1ca)
            message.payload.widget.id = 0;
        else
            message.payload.widget.id = 0x40;
        ProcessCombatMsg(message);
    } else
        gpMouseManager->SetPointer(6);
}

// Buka COMMAND.cpp ProcessNextAction; HoMM1 hides the pointer around the
// action, broadcasts it to a human net opponent and has no door or cycling.
VA(0x004136e6, 0x55a)
short combatManager::ProcessNextAction(struct tag_message &message)
{
    army *actingArmy;
    signed char advance;
    int result;
    int data[4];

    if (giNextAction)
        LogStr("Process Act", giNextAction, giNextActionGridIndex, giNextActionGridIndex2, giNextActionExtra,
               m_currentSide, m_currentArmyIndex, m_armies[m_currentSide][m_currentArmyIndex].m_hex);
    if (gbThisNetHasControl && gbRemoteOn && m_playerId[1] >= 0 && m_playerId[0] >= 0
        && gbHumanPlayer[m_playerId[0]] && gbHumanPlayer[m_playerId[1]]) {
        int netPos;

        netPos = m_playerId[1 - m_currentSide];
        if (netPos < 0 || !gbHumanPlayer[netPos])
            netPos = giRemoteDefaultPlayer;
        data[0] = giNextAction;
        data[1] = giNextActionExtra;
        data[2] = giNextActionGridIndex;
        data[3] = giNextActionGridIndex2;
        result = TransmitRemoteData((char *)data, netPos, sizeof(data), 0x17, 1, 1, -1, 1);
        if (!result)
            ShutDown(0);
    }
    actingArmy = &m_armies[m_currentSide][m_currentArmyIndex];
    advance = 0;
    if (CheckWin(&message))
        return MESSAGE_DISPATCH_FORWARD;
    switch (giNextAction) {
        case 0:
            break;
        case 1:
            gpMouseManager->ReallyHidePointer();
            CastSpell(giNextActionExtra, giNextActionGridIndex, 0, giNextActionGridIndex2);
            if (m_armies[m_currentSide][m_currentArmyIndex].m_quantity <= 0)
                advance = 1;
            break;
        case 2:
            gpMouseManager->ReallyHidePointer();
            actingArmy->MoveAttack(giNextActionGridIndex, 0);
            actingArmy->m_attributes |= 0x80;
            if (CheckWin(&message))
                return MESSAGE_DISPATCH_FORWARD;
            CheckApplyGoodMorale(m_currentSide, m_currentArmyIndex);
            advance = 1;
            break;
        case 6:
            gpMouseManager->ReallyHidePointer();
            if (giNextActionExtra != -1 && actingArmy->m_hex != giNextActionExtra)
                actingArmy->MoveAttack(giNextActionExtra, 1);
            actingArmy->MoveAttack(giNextActionGridIndex, 0);
            actingArmy->m_attributes |= 0x80;
            if (CheckWin(&message))
                return MESSAGE_DISPATCH_FORWARD;
            CheckApplyGoodMorale(m_currentSide, m_currentArmyIndex);
            advance = 1;
            break;
        case 4:
            m_sideRetreated[m_currentSide] = 1;
            gbRetreatWin = 1;
            break;
        case 5:
            gbCombatSurrender = 1;
            gbRetreatWin = 1;
            m_sideDefeated[m_currentSide] = 1;
            gpGame->m_players[m_playerId[m_currentSide]].m_resources[RESOURCE_GOLD] -= giNextActionExtra;
            gpGame->m_players[m_playerId[1 - m_currentSide]].m_resources[RESOURCE_GOLD] += giNextActionExtra;
            break;
        case 3:
            actingArmy->m_attributes |= 0x80;
            advance = 1;
            break;
    }
    giNextAction = 0;
    if (CheckWin(&message))
        return MESSAGE_DISPATCH_FORWARD;
    if (advance && !GetNextArmy(1)) {
        ResetRound();
        GetNextArmy(1);
    }
    CheckChangeSelector();
    if (gbThisNetHasControl && !m_gridSelectionDisabled && m_playerId[m_currentSide] != -1
        && gbThisNetHumanPlayer[m_playerId[m_currentSide]])
        gpMouseManager->ReallyShowPointer();
    else
        gpMouseManager->ReallyHidePointer();
    return MESSAGE_DISPATCH_CONSUME;
}

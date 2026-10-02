// Located from HoMM2 Buka 2.1; PoL 2.0 supplies the VC4 declaration.

#include <match.h>

#include <BASE/INPUTMGR_TYPES.h>

#include <H1/All.h>
#include <H1/KB.h>

// donor PoL RVA 0x0002a6d0; preferred Buka symbol ?Main@combatManager@@UAEHAAUtag_message@@@Z
// donor Buka TU SOURCE/COMMAND; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.520039;margin=0.117792;shape=0.347;size=0.894;calls=0.889;alternate=pol20:int combatManager::Main(struct tag_message &);   // virtual [override (implements baseManager pure virtual)]@0x0002a6d0
VA(0x0040f2c0, 0x311)
short combatManager::Main(struct tag_message &) { return 0; }

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

// donor PoL RVA 0x0002abbe; preferred Buka symbol ?SetCombatDirections@combatManager@@QAEXH@Z
// donor Buka TU SOURCE/COMMAND; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.555396;margin=0.346771;shape=0.387;size=0.928;calls=1.000;alternate=pol20:void combatManager::SetCombatDirections(int)@0x0002abbe
VA(0x0040f6a7, 0x7e9)
void combatManager::SetCombatDirections(int) {}

// donor PoL RVA 0x0002b45f; preferred Buka symbol ?CheckSetMouseDirection@combatManager@@QAEXHHH@Z
// donor Buka TU SOURCE/COMMAND; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.473098;margin=0.353023;shape=0.247;size=0.891;calls=0.857;alternate=pol20:void combatManager::CheckSetMouseDirection(int, int, int)@0x0002b45f
VA(0x0040fe90, 0x620)
void combatManager::CheckSetMouseDirection(int, int, int) {}

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

// donor PoL RVA 0x0002bb26; preferred Buka symbol ?ProcessCombatMsg@combatManager@@QAEHAAUtag_message@@@Z
// donor Buka TU SOURCE/COMMAND; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.392432;margin=0.340460;shape=0.273;size=0.720;calls=0.630;alternate=pol20:int combatManager::ProcessCombatMsg(struct tag_message &)@0x0002bb26
VA(0x004104e4, 0x5bb)
int combatManager::ProcessCombatMsg(struct tag_message &) { return 0; }

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

// donor PoL RVA 0x0002c8ff; preferred Buka symbol ?GetCommand@combatManager@@QAEHH@Z
// donor Buka TU SOURCE/COMMAND; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.402881;margin=0.493749;shape=0.255;size=0.779;calls=0.500;alternate=pol20:int combatManager::GetCommand(int)@0x0002c8ff
VA(0x00410d35, 0x316)
int combatManager::GetCommand(int) { return 0; }

// donor PoL RVA 0x0002ce19; preferred Buka symbol ?RightClick@combatManager@@QAEHH@Z
// donor Buka TU SOURCE/COMMAND; HoMM1 owner inferred from contiguous order
// evidence: graph:4;base=0.497332;margin=0.914777;shape=0.275;size=0.969;calls=0.875;alternate=pol20:int combatManager::RightClick(int)@0x0002ce19
VA(0x0041104b, 0x1dc)
int combatManager::RightClick(int) { return 0; }

// donor PoL RVA 0x0002d0bf; preferred Buka symbol ?DoCommand@combatManager@@QAEXH@Z
// donor Buka TU SOURCE/COMMAND; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.479004;margin=0.375974;shape=0.217;size=0.994;calls=0.842;alternate=pol20:void combatManager::DoCommand(int)@0x0002d0bf
VA(0x00411227, 0x333)
void combatManager::DoCommand(int) {}

// donor PoL RVA 0x0002d472; preferred Buka symbol ?WinCombatHandler@@YIHAAUtag_message@@@Z
// donor Buka TU SOURCE/COMMAND; HoMM1 owner inferred from contiguous order
// evidence: graph:3;base=0.240792;margin=0.378009;shape=0.252;size=0.331;calls=0.261;alternate=pol20:int WinCombatHandler(struct tag_message &)@0x0002d472
VA(0x0041155a, 0x1a3)
int WinCombatHandler(struct tag_message &) { return 0; }

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

// donor PoL RVA 0x0002e2bf; preferred Buka symbol ?ShowDeadArmies@combatManager@@QAEXPAVheroWindow@@@Z
// donor Buka TU SOURCE/COMMAND; HoMM1 owner inferred from contiguous order
// evidence: graph:5;base=0.669049;margin=0.415746;shape=0.366;size=0.795;calls=0.971;strings=mons32.icn|smalfont.fnt;alternate=pol20:void combatManager::ShowDeadArmies(class heroWindow *)@0x0002e2bf
VA(0x00411b07, 0x7d0)
void combatManager::ShowDeadArmies(class heroWindow *) {}

// donor PoL RVA 0x0002ec8b; preferred Buka symbol ?DoVictory@combatManager@@QAEXH@Z
// donor Buka TU SOURCE/COMMAND; HoMM1 owner inferred from contiguous order
// evidence: graph:3;base=0.616103;margin=0.112225;shape=0.417;size=0.688;calls=0.762;strings=wincmbt.bin;alternate=pol20:void combatManager::DoVictory(int)@0x0002ec8b
VA(0x004122d7, 0x7a1)
void combatManager::DoVictory(signed char) {}

// donor PoL RVA 0x0002f834; preferred Buka symbol ?DoLoseWindow@combatManager@@QAEXXZ
// donor Buka TU SOURCE/COMMAND; HoMM1 owner inferred from contiguous order
// evidence: graph:4;base=0.434201;margin=0.199849;shape=0.391;size=0.664;calls=0.594;alternate=pol20:void combatManager::DoLoseWindow(void)@0x0002f834
VA(0x00412a78, 0x549)
void combatManager::DoLoseWindow(void) {}

// donor PoL RVA 0x0002fbf0; preferred Buka symbol ?DoSurrender@combatManager@@QAEHXZ
// donor Buka TU SOURCE/COMMAND; HoMM1 owner inferred from contiguous order
// evidence: graph:3;base=0.579024;margin=0.030061;shape=0.292;size=0.742;calls=0.667;strings=port%04d.icn|surrendr.bin;alternate=pol20:int combatManager::DoSurrender(void)@0x0002fbf0
VA(0x00412fc1, 0x2c6)
int combatManager::DoSurrender(void) { return 0; }

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

// donor PoL RVA 0x00030536; preferred Buka symbol ?ProcessNextAction@combatManager@@QAEHAAUtag_message@@@Z
// donor Buka TU SOURCE/COMMAND; HoMM1 owner inferred from contiguous order
// evidence: graph:3;base=0.593971;margin=0.483478;shape=0.242;size=0.886;calls=0.629;strings=Process Act;alternate=pol20:int combatManager::ProcessNextAction(struct tag_message &)@0x00030536
VA(0x004136e6, 0x55a)
short combatManager::ProcessNextAction(struct tag_message &) { return 0; }

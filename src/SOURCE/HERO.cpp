// Located from HoMM2 Buka 2.1; PoL 2.0 supplies the VC4 declaration.

#include <match.h>

#include <BASE/Misc.h>
#include <H1/All.h>
#include <H1/KB.h>
#include <SOURCE/dialogTypes.h>

#include <stdio.h>
#include <string.h>

extern char* cHeroLevel[];
extern signed char gHeroSkillBonus[][9][HERO_PRIMARY_STAT_COUNT];
extern int gbInNewGameSetup;
void SRand(int);
int SRandom(int, int);

// donor PoL RVA 0x000c0790; preferred Buka symbol ?AICheckRetreat@combatManager@@QAEHXZ
// donor Buka TU SOURCE/AI; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.550932;margin=0.199965;shape=0.407;size=0.920;calls=0.857;alternate=pol20:int combatManager::AICheckRetreat(void)@0x000c0790
VA(0x00464520, 0x783)
int combatManager::AICheckRetreat(void) { return 0; }

// donor PoL RVA 0x0004b36e; preferred Buka symbol ?FreeResources@army@@QAEXXZ
// donor Buka TU SOURCE/ARMY; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.463954;margin=0.140795;shape=0.318;size=0.842;calls=0.750;alternate=pol20:void army::FreeResources(void)@0x0004b36e
VA(0x0046693b, 0xf1)
void army::FreeResources(void) {}

// donor PoL RVA 0x0004c7e5; preferred Buka symbol ?SpecialAttack@army@@QAEXXZ
// donor Buka TU SOURCE/ARMY; HoMM1 owner inferred from contiguous order
// evidence: graph:3;base=0.357822;margin=0.373223;shape=0.266;size=0.572;calls=0.585;alternate=pol20:void army::SpecialAttack(void)@0x0004c7e5
VA(0x00467b97, 0xcca)
void army::SpecialAttack(void) {}

// donor PoL RVA 0x0004e1a1; preferred Buka symbol ?DoAttack@army@@QAEXH@Z
// donor Buka TU SOURCE/ARMY; HoMM1 owner inferred from contiguous order
// evidence: graph:4;base=0.427643;margin=0.977018;shape=0.217;size=0.817;calls=0.724;alternate=pol20:void army::DoAttack(int)@0x0004e1a1
VA(0x00468ff3, 0x108d)
void army::DoAttack(int) {}

// donor PoL RVA 0x0004f93e; preferred Buka symbol ?CheckLuck@army@@QAEXXZ
// donor Buka TU SOURCE/ARMY; HoMM1 owner inferred from contiguous order
// evidence: graph:1;base=0.723107;margin=0.234495;shape=0.473;size=0.925;calls=0.875;strings=badluck.82m|goodluck.82m;alternate=pol20:void army::CheckLuck(void)@0x0004f93e
VA(0x0046a3dc, 0x249)
void army::CheckLuck(void) {}

// donor PoL RVA 0x0004fbc0; preferred Buka symbol ?DamageEnemy@army@@QAEXPAV1@PAH1HH@Z
// donor Buka TU SOURCE/ARMY; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.387284;margin=0.228483;shape=0.296;size=0.628;calls=0.714;alternate=pol20:void army::DamageEnemy(class army *, int *, int *, int, int)@0x0004fbc0
VA(0x0046a625, 0x2ae)
void army::DamageEnemy(class army *, int *, int *, int, int) {}

// donor PoL RVA 0x0005012e; preferred Buka symbol ?Damage@army@@QAEHJH@Z
// donor Buka TU SOURCE/ARMY; HoMM1 owner inferred from contiguous order
// evidence: graph:6;base=0.419408;margin=1.157007;shape=0.216;size=0.693;calls=1.000;alternate=pol20:int army::Damage(long int, int)@0x0005012e
VA(0x0046a8d3, 0x176)
int army::Damage(long int, int) { return 0; }

// donor PoL RVA 0x00052ad9; preferred Buka symbol ?MoveAttack@army@@QAEXHH@Z
// donor Buka TU SOURCE/ARMY; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.522810;margin=0.525862;shape=0.325;size=0.919;calls=0.929;alternate=pol20:void army::MoveAttack(int, int)@0x00052ad9
VA(0x0046b6e7, 0x3a9)
void army::MoveAttack(int, int) {}

// donor PoL RVA 0x0006c3a0; preferred Buka symbol ??0hero@@QAE@XZ
// donor Buka TU SOURCE/HERO; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.493986;margin=0.210035;shape=0.273;size=0.962;calls=1.000;alternate=pol20:void hero::constructor(void)@0x0006c3a0
VA(0x0046ba90, 0x68)
hero::hero(void) {
    m_id = 0;
    m_owner = 0;
    m_x = 0;
    m_y = 0;
    m_unknown1c = 0;
    m_unknown1d = 0;
    m_name[0] = 0;
    heroWin = 0;
    giHeroScreenSrcIndex = -1;
}

// donor PoL RVA 0x0006c4cd; preferred Buka symbol ?HasArtifact@hero@@QAEHH@Z
// donor Buka TU SOURCE/HERO; HoMM1 owner inferred from contiguous order
// evidence: graph:5;base=0.403615;margin=0.791427;shape=0.171;size=0.731;calls=1.000;alternate=pol20:int hero::HasArtifact(int)@0x0006c4cd
VA(0x0046baf8, 0x18)
void hero::GetArmyStrengths(unsigned long int* const) {}

VA(0x0046bb10, 0x5d)
signed char hero::HasArtifact(signed char artifact) {
    short i;

    for (i = 0; i < HERO_ARTIFACT_SLOT_COUNT; i++) {
        if (m_artifacts[i] == artifact)
            return 1;
    }
    return 0;
}

// donor PoL RVA 0x0006c526; preferred Buka symbol ?CalcMobility@hero@@QAEHXZ
// donor Buka TU SOURCE/HERO; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.418178;margin=0.237740;shape=0.210;size=0.862;calls=0.714;alternate=pol20:int hero::CalcMobility(void)@0x0006c526
VA(0x0046bb6d, 0x1ed)
short hero::CalcMobility(void) {
    short mobility[3] = {40, 50, 60};
    const short seaMobility = 60;
    const short lighthouseExtra = 20;
    const short astrolabe = 40;
    const short compass = 20;
    const short nomadBonus = 24;
    const short travelerBonus = 12;
    int result;
    short speed;
    int j;

    if (m_eventFlags & HERO_EVENT_EMBARKED) {
        if (gpGame->m_mines[1].owner == m_owner)
            result = seaMobility + lighthouseExtra;
        else
            result = seaMobility;
        if (HasArtifact(ARTIFACT_SAILORS_ASTROLABE))
            result += astrolabe;
        result = (int)(result * gfClassNavigationMod[m_unknown1c]);
    } else {
        speed = 3;
        for (j = 0; j < ARMY_GROUP_SLOT_COUNT; j++) {
            if (m_army.m_creatureTypes[j] != -1
                && gMonsterDatabase[m_army.m_creatureTypes[j]].speed < speed)
                speed = gMonsterDatabase[m_army.m_creatureTypes[j]].speed;
        }
        result = mobility[speed - 1];
        if (HasArtifact(ARTIFACT_NOMAD_BOOTS))
            result += nomadBonus;
        if (HasArtifact(ARTIFACT_TRAVELER_BOOTS))
            result += travelerBonus;
    }
    if (HasArtifact(ARTIFACT_TRUE_COMPASS))
        result += compass;
    if (m_owner >= 0 && !gbHumanPlayer[m_owner]
        && gpGame->m_players[m_owner].m_difficulty >= 3)
        result += 3;
    return result;
}

VA(0x0046bd5a, 0x56)
signed char hero::HasSpell(signed char spell) {
    int i;

    for (i = 0; i < HERO_SPELL_SLOT_COUNT; i++) {
        if (m_spells[i] == spell)
            return 1;
    }
    return 0;
}

VA(0x0046bdb0, 0xf0)
short hero::GetNumSpells(signed char type) {
    short combat = 0;
    short adventure = 0;
    short i;

    for (i = 0; i < HERO_COMBAT_SPELL_SLOT_COUNT; i++) {
        if (m_spells[i] != -1)
            combat++;
    }
    for (i = 0; i < HERO_SPELL_SLOT_COUNT - HERO_COMBAT_SPELL_SLOT_COUNT; i++) {
        if (m_spells[HERO_COMBAT_SPELL_SLOT_COUNT + i] != -1)
            adventure++;
    }
    switch (type) {
    case 0:
        return combat;
    case 1:
        return adventure;
    case 2:
        return adventure + combat;
    }
    return 0;
}

VA(0x0046bea0, 0x217)
void hero::UseSpell(signed char spell) {
    short i;
    int j;
    int k;

    if (spell >= 0 && spell < HERO_COMBAT_SPELL_SLOT_COUNT) {
        for (i = 0; i < HERO_COMBAT_SPELL_SLOT_COUNT; i++) {
            if (m_spells[i] == spell)
                break;
        }
        if (m_spellCharges[i] > 1) {
            m_spellCharges[i]--;
        } else {
            m_spells[i] = -1;
            m_spellCharges[i] = 0;
            for (j = i + 1; j < HERO_COMBAT_SPELL_SLOT_COUNT; j++) {
                m_spells[j - 1] = m_spells[j];
                m_spellCharges[j - 1] = m_spellCharges[j];
            }
            m_spells[HERO_COMBAT_SPELL_SLOT_COUNT - 1] = -1;
            m_spellCharges[HERO_COMBAT_SPELL_SLOT_COUNT - 1] = 0;
        }
    } else if (spell >= HERO_COMBAT_SPELL_SLOT_COUNT && spell < HERO_SPELL_SLOT_COUNT) {
        for (i = HERO_COMBAT_SPELL_SLOT_COUNT; i < HERO_SPELL_SLOT_COUNT; i++) {
            if (m_spells[i] == spell)
                break;
        }
        if (m_spellCharges[i] > 1) {
            m_spellCharges[i]--;
        } else {
            m_spells[i] = -1;
            m_spellCharges[i] = 0;
            for (k = i + 1; k < HERO_SPELL_SLOT_COUNT; k++) {
                m_spells[k - 1] = m_spells[k];
                m_spellCharges[k - 1] = m_spellCharges[k];
            }
            m_spells[HERO_SPELL_SLOT_COUNT - 1] = -1;
            m_spellCharges[HERO_SPELL_SLOT_COUNT - 1] = 0;
        }
    }
}

VA(0x0046c0b7, 0x1e3)
int hero::AddSpell(signed char spell, signed char charges, int checkOnly) {
    int added = 0;
    short i;

    if (spell >= 0 && spell < HERO_COMBAT_SPELL_SLOT_COUNT) {
        for (i = 0; i < HERO_COMBAT_SPELL_SLOT_COUNT; i++) {
            if (m_spells[i] == spell || m_spells[i] == -1) {
                if (m_spells[i] == spell)
                    added = charges - m_spellCharges[i];
                else
                    added = charges;
                if (checkOnly)
                    goto done;
                m_spells[i] = spell;
                m_spellCharges[i] = charges;
                break;
            }
        }
    }
    if (spell >= HERO_COMBAT_SPELL_SLOT_COUNT && spell < HERO_SPELL_SLOT_COUNT) {
        for (i = HERO_COMBAT_SPELL_SLOT_COUNT; i < HERO_SPELL_SLOT_COUNT; i++) {
            if (m_spells[i] == spell || m_spells[i] == -1) {
                if (m_spells[i] == spell)
                    added = charges - m_spellCharges[i];
                else
                    added = charges;
                if (checkOnly)
                    goto done;
                m_spells[i] = spell;
                m_spellCharges[i] = charges;
                break;
            }
        }
    }
done:
    return added;
}

// donor PoL RVA 0x0006f305; preferred Buka symbol ?RedrawHeroScreen@@YIXXZ
// donor Buka TU SOURCE/HERO; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.519414;margin=0.462427;shape=0.417;size=0.819;calls=1.000;alternate=pol20:void RedrawHeroScreen(void)@0x0006f305
VA(0x0046c29a, 0x53)
void hero::RedrawHeroScreen(void) {
    gpResourceManager->GetBackdrop("heroscrn.bmp", gpWindowManager->m_screen);
    heroWin->DrawWindow();
    gpWindowManager->UpdateScreenRegion(0, 0, 640, 480);
}

// donor PoL RVA 0x0006f354; preferred Buka symbol ?HeroView@@YIHHHH@Z
// donor Buka TU SOURCE/HERO; HoMM1 owner inferred from contiguous order
// evidence: graph:8;base=0.391018;margin=1.082891;shape=0.247;size=0.310;calls=0.359;strings=herowind.bin;alternate=pol20:int HeroView(int, int, int)@0x0006f354
VA(0x0046c2ed, 0x6c2)
void hero::HeroView(signed char) {}

// donor PoL RVA 0x0006cab1; preferred Buka symbol ?HeroMessageUpdate@@YIXPAD@Z
// donor Buka TU SOURCE/HERO; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.570065;margin=0.065812;shape=0.448;size=0.919;calls=1.000;alternate=pol20:void HeroMessageUpdate(char *)@0x0006cab1
VA(0x0046c9af, 0x7c)
void HeroMessageUpdate(char* text) {
    tag_message message;

    if (!gheroWin)
        return;
    message.type = MESSAGE_WIDGET;
    message.payload.widget.command = WIDGET_COMMAND_SET_TEXT;
    message.payload.widget.id = 302;
    message.payload.widget.data.text = text;
    gheroWin->BroadcastMessage(message);
    gheroWin->DrawWindow(0, 300, 302);
    gpWindowManager->UpdateScreenRegion(0, 459, 640, 20);
}

// donor PoL RVA 0x0006cb33; preferred Buka symbol ?HeroScreenUpdate@hero@@QAEXXZ
// donor Buka TU SOURCE/HERO; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.542456;margin=0.317795;shape=0.400;size=0.865;calls=1.000;alternate=pol20:void hero::HeroScreenUpdate(void)@0x0006cb33
VA(0x0046ca2b, 0xab)
void hero::HeroScreenUpdate(void) {
    tag_message message;
    short i;

    message.type = MESSAGE_WIDGET;
    UpdateArmies();
    for (i = 0; i < ARMY_GROUP_SLOT_COUNT; i++) {
        if (i == giHeroScreenSrcIndex)
            message.payload.widget.command = WIDGET_COMMAND_SET_FLAGS;
        else
            message.payload.widget.command = WIDGET_COMMAND_CLEAR_FLAGS;
        message.payload.widget.data.value = 4;
        message.payload.widget.id = i + 102;
        heroWin->BroadcastMessage(message);
    }
    heroWin->DrawWindow();
    gpWindowManager->UpdateScreenRegion(0, 0, 640, 480);
}

// donor PoL RVA 0x0006cbdb; preferred Buka symbol ?UpdateArmies@hero@@QAEXXZ
// donor Buka TU SOURCE/HERO; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.520009;margin=0.290164;shape=0.295;size=0.980;calls=0.909;alternate=pol20:void hero::UpdateArmies(void)@0x0006cbdb
VA(0x0046cad6, 0x1ba)
void hero::UpdateArmies(void) {
    tag_message message;
    short i;

    message.type = MESSAGE_WIDGET;
    for (i = 0; i < ARMY_GROUP_SLOT_COUNT; i++) {
        if (m_army.m_creatureTypes[i] == -1) {
            message.payload.widget.command = WIDGET_COMMAND_SET_FRAME;
            message.payload.widget.id = i + 87;
            message.payload.widget.data.value = 2;
            heroWin->BroadcastMessage(message);
            message.payload.widget.command = WIDGET_COMMAND_CLEAR_FLAGS;
            message.payload.widget.id = i + 92;
            message.payload.widget.data.value = 4;
            heroWin->BroadcastMessage(message);
            message.payload.widget.id = i + 97;
            heroWin->BroadcastMessage(message);
            message.payload.widget.id = i + 102;
            heroWin->BroadcastMessage(message);
        } else {
            message.payload.widget.command = WIDGET_COMMAND_SET_FRAME;
            message.payload.widget.id = i + 87;
            message.payload.widget.data.value = m_army.m_creatureTypes[i] / 6 + 3;
            heroWin->BroadcastMessage(message);
            message.payload.widget.id = i + 92;
            message.payload.widget.data.value = m_army.m_creatureTypes[i];
            heroWin->BroadcastMessage(message);
            message.payload.widget.command = WIDGET_COMMAND_SET_FLAGS;
            message.payload.widget.data.value = 4;
            heroWin->BroadcastMessage(message);
            sprintf(gText, "%d", m_army.m_creatureCounts[i]);
            message.payload.widget.command = WIDGET_COMMAND_SET_TEXT;
            message.payload.widget.id = i + 97;
            message.payload.widget.data.text = gText;
            heroWin->BroadcastMessage(message);
            message.payload.widget.command = WIDGET_COMMAND_SET_FLAGS;
            message.payload.widget.data.value = 4;
            heroWin->BroadcastMessage(message);
        }
    }
}

VA(0x0046cc90, 0x1af)
void hero::ViewStat(signed char stat, signed char quickView) {
    heroWindow* win;
    tag_message message;

    if (quickView) {
        sprintf(gText, "%s\n\n%s", gStatNames[stat], gStatDesc[stat]);
        NormalDialog(gText, 4, 0xb1, 0x19, -1, 0, -1, 0, -1);
        return;
    }
    win = new heroWindow(0xb1, 0x19, "vstat.bin");
    if (!win)
        MemError();
    strcpy(gText, gStatNames[stat]);
    message.type = MESSAGE_WIDGET;
    message.payload.widget.command = WIDGET_COMMAND_SET_TEXT;
    message.payload.widget.id = 1;
    message.payload.widget.data.text = gText;
    win->BroadcastMessage(message);
    strcpy(gText, gStatDesc[stat]);
    message.type = MESSAGE_WIDGET;
    message.payload.widget.command = WIDGET_COMMAND_SET_TEXT;
    message.payload.widget.id = 2;
    message.payload.widget.data.text = gText;
    win->BroadcastMessage(message);
    gpWindowManager->DoDialog(win, TrueFalseDialogHandler, 0);
    delete win;
}

VA(0x0046ce3f, 0x4a)
void hero::ViewArtifact(signed char artifact, signed char quickView) {
    NormalDialog(gArtifactDesc[artifact], quickView == 0 ? 1 : 4, -1, 0x1c, -1, 0, -1, 0, -1);
}

// donor PoL RVA 0x0006ce8b; preferred Buka symbol ?Dismiss@hero@@QAEHXZ
// donor Buka TU SOURCE/HERO; HoMM1 owner inferred from contiguous order
// evidence: graph:3;base=0.462026;margin=0.671820;shape=0.242;size=0.843;calls=1.000;alternate=pol20:int hero::Dismiss(void)@0x0006ce8b
VA(0x0046ce89, 0x59)
signed char hero::Dismiss(void) {
    NormalDialog("Are you sure you want to dismiss this Hero?", 2, 0xb1, 0x1c,
                 -1, 0, -1, 0, -1);
    if (gpWindowManager->m_dialogResult == NORMAL_DIALOG_CONFIRM) {
        Deallocate();
        return 1;
    }
    return 0;
}

// donor PoL RVA 0x0006cee8; preferred Buka symbol ?Deallocate@hero@@QAEXH@Z
// donor Buka TU SOURCE/HERO; HoMM1 owner inferred from contiguous order
// evidence: graph:5;base=0.482098;margin=0.987612;shape=0.300;size=0.883;calls=0.875;alternate=pol20:void hero::Deallocate(int)@0x0006cee8
VA(0x0046cee2, 0x452)
void hero::Deallocate(void) {
    playerData* player;
    signed char heroNum;
    short i;
    int owner;
    town* townRec;
    int slotNum;

    owner = m_owner;
    player = &gpGame->m_players[m_owner];
    gpAdvManager->MobilizeCurrHero(0);
    gpAdvManager->HideRoute(0, 0, 0);
    if (m_eventFlags & HERO_EVENT_EMBARKED) {
        for (i = 0; i < GAME_BOAT_COUNT; i++) {
            if (gpGame->m_boats[i].heroId == m_id) {
                gpGame->m_boats[i].heroId = -1;
                gpGame->m_boatSlots[i] = -1;
            }
        }
    }
    if (m_locationType == 0xa8) {
        townRec = gpGame->GetTown(m_occupiedTown);
        townRec->m_occupyingHeroId = -1;
    }
    if (m_owner != giCurPlayer || gpGame->m_players[m_owner].m_currentHero != m_id
        || !gpAdvManager->m_heroContextLocked)
        gpGame->RestoreCell(m_x, m_y, m_locationType, m_occupiedTown, 0, 1);
    if (!gbCombatSurrender) {
        for (i = 0; i < ARMY_GROUP_SLOT_COUNT; i++)
            m_army.Dismiss(i);
    }
    heroNum = -1;
    for (i = 0; i < player->m_heroCount; i++) {
        if (player->m_heroIds[i] == m_id)
            heroNum = i;
    }
    for (i = heroNum; i < player->m_heroCount - 1; i++)
        player->m_heroIds[i] = player->m_heroIds[i + 1];
    player->m_heroIds[player->m_heroCount - 1] = -1;
    if (player->m_currentHero == m_id) {
        player->m_currentHero = -1;
        if (m_owner == giCurPlayer) {
            gpAdvManager->m_cursorActive = 0;
            gpGame->m_map[m_x][m_y].m_flags &= ~0x40;
        }
        if (giCurPlayer == owner)
            gpAdvManager->m_heroContextLocked = 0;
    }
    player->m_heroCount--;
    player->m_heroLocatorPage = 0;
    gpGame->m_availableHeroes[m_id] = -1;
    if (gbRetreatWin) {
        slotNum = Random(0, 1);
        if (gpGame->m_availableHeroes[gpGame->m_players[m_owner].m_availableHeroIds[slotNum]] == 0x40)
            gpGame->m_availableHeroes[gpGame->m_players[m_owner].m_availableHeroIds[slotNum]] = -1;
        gpGame->m_players[m_owner].m_availableHeroIds[slotNum] = m_id;
        gpGame->m_availableHeroes[m_id] = 0x40;
    }
    m_owner = -1;
    m_destinationX = m_destinationY = -1;
    if (!gbCombatSurrender)
        gpGame->SetRandomHeroArmies(m_id, 0);
    CheckEndGame(0);
}

// donor PoL RVA 0x0006d50d; preferred Buka symbol ?GetLevel@hero@@QAEHH@Z
// donor Buka TU SOURCE/HERO; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.411151;margin=0.242527;shape=0.205;size=0.711;calls=1.000;alternate=pol20:int hero::GetLevel(int)@0x0006d50d
VA(0x0046d334, 0xd0)
int hero::GetExperience(int level) {
    int experience;
    int stage;
    int incr;

    if (level <= HERO_EXPERIENCE_LEVEL_TABLE_COUNT)
        return gMinExpForLevel[m_unknown1c][level - 1];
    stage = HERO_EXPERIENCE_LEVEL_TABLE_COUNT + 1;
    incr = (int)((gMinExpForLevel[m_unknown1c][HERO_EXPERIENCE_LEVEL_TABLE_COUNT - 1]
                  - gMinExpForLevel[m_unknown1c][HERO_EXPERIENCE_LEVEL_TABLE_COUNT - 2])
                 * 1.2);
    experience = gMinExpForLevel[m_unknown1c][HERO_EXPERIENCE_LEVEL_TABLE_COUNT - 1] + incr;
    while (stage < level) {
        incr = (int)(incr * 1.2);
        experience += incr;
        stage++;
    }
    return experience;
}

VA(0x0046d404, 0xf2)
int hero::GetLevel(int experienceValue) {
    int experience;
    int nLevel;
    int growth;

    for (nLevel = 1; nLevel <= HERO_EXPERIENCE_LEVEL_TABLE_COUNT; nLevel++) {
        if (gMinExpForLevel[m_unknown1c][nLevel - 1] > experienceValue)
            return nLevel - 1;
    }
    growth = (int)((gMinExpForLevel[m_unknown1c][HERO_EXPERIENCE_LEVEL_TABLE_COUNT - 1]
                    - gMinExpForLevel[m_unknown1c][HERO_EXPERIENCE_LEVEL_TABLE_COUNT - 2])
                   * 1.2);
    experience = gMinExpForLevel[m_unknown1c][HERO_EXPERIENCE_LEVEL_TABLE_COUNT - 1] + growth;
    nLevel = HERO_EXPERIENCE_LEVEL_TABLE_COUNT + 1;
    while (experience < experienceValue) {
        growth = (int)(growth * 1.2);
        experience += growth;
        nLevel++;
    }
    return nLevel - 1;
}

VA(0x0046d4f6, 0x14f)
void hero::ApplyBattleWinTemps(void) {
    if (m_eventFlags & HERO_EVENT_GRAVEYARD) {
        m_morale++;
        m_eventFlags -= HERO_EVENT_GRAVEYARD;
    }
    if (m_eventFlags & HERO_EVENT_SHIPWRECK) {
        m_morale++;
        m_eventFlags -= HERO_EVENT_SHIPWRECK;
    }
    if (m_eventFlags & HERO_EVENT_BUOY) {
        m_morale--;
        m_eventFlags -= HERO_EVENT_BUOY;
    }
    if (m_eventFlags & HERO_EVENT_OASIS) {
        m_morale--;
        m_eventFlags -= HERO_EVENT_OASIS;
    }
    if (m_eventFlags & HERO_EVENT_TEMPLE) {
        m_morale -= 2;
        m_eventFlags -= HERO_EVENT_TEMPLE;
    }
    if (m_eventFlags & HERO_EVENT_FAERIE_RING) {
        m_luck--;
        m_eventFlags -= HERO_EVENT_FAERIE_RING;
    }
    if (m_eventFlags & HERO_EVENT_FOUNTAIN) {
        m_luck--;
        m_eventFlags -= HERO_EVENT_FOUNTAIN;
    }
}

VA(0x0046d645, 0x1e)
void hero::ApplyBattleLossTemps(void) {
    ApplyBattleWinTemps();
}

// donor PoL RVA 0x0006d83f; preferred Buka symbol ?CheckLevel@hero@@QAEXXZ
// donor Buka TU SOURCE/HERO; HoMM1 owner inferred from contiguous order
// evidence: graph:3;base=0.312130;margin=0.246272;shape=0.276;size=0.445;calls=0.500;alternate=pol20:void hero::CheckLevel(void)@0x0006d83f
VA(0x0046d663, 0x2f4)
void hero::CheckLevel(void) {
    int lvl;
    int i;
    int stats[HERO_PRIMARY_STAT_COUNT];
    int levelCount;
    int highIndex;
    char text[50];
    int roll;

    lvl = GetLevel(m_experience);
    if (m_level == lvl)
        return;
    levelCount = lvl - m_level;
    sprintf(gText, cHeroLevel[0], m_name);
    if (levelCount == 1)
        sprintf(text, cHeroLevel[1]);
    else
        sprintf(text, cHeroLevel[2], levelCount);
    strcat(gText, text);
    stats[0] = 0;
    stats[1] = 0;
    stats[2] = 0;
    stats[3] = 0;
    for (i = m_level + 1; i <= lvl; i++) {
        highIndex = i - 2;
        if (highIndex > 8)
            highIndex = 8;
        SRand(m_randomSeed + i * 30);
        roll = SRandom(1, 100);
        if (gHeroSkillBonus[m_unknown1c][highIndex][0] > roll) {
            stats[0]++;
        } else {
            roll -= gHeroSkillBonus[m_unknown1c][highIndex][0];
            if (gHeroSkillBonus[m_unknown1c][highIndex][1] > roll) {
                stats[1]++;
            } else {
                roll -= gHeroSkillBonus[m_unknown1c][highIndex][1];
                if (gHeroSkillBonus[m_unknown1c][highIndex][2] > roll)
                    stats[2]++;
                else
                    stats[3]++;
            }
        }
    }
    for (i = 0; i < HERO_PRIMARY_STAT_COUNT; i++) {
        if (stats[i] > 0) {
            m_primaryStats[i] += stats[i];
            sprintf(text, "\n%s +%d", gStatNames[i], stats[i]);
            strcat(gText, text);
        }
    }
    m_level = lvl;
    if (!gbInNewGameSetup && m_owner >= 0 && gbThisNetHumanPlayer[m_owner]) {
        gpSoundManager->SwitchAmbientMusic(52);
        NormalDialog(gText, 1, -1, -1, 15, m_id, -1, 0, -1);
        gpSoundManager->SwitchAmbientMusic(gpAdvManager->m_currentTerrain);
    }
}

// donor PoL RVA 0x0006e0be; preferred Buka symbol ?UpdateHeroScreenStatusBar@@YIXAAUtag_message@@@Z
// donor Buka TU SOURCE/HERO; HoMM1 owner inferred from contiguous order
// evidence: graph:4;base=0.389885;margin=0.637226;shape=0.282;size=0.714;calls=0.718;alternate=pol20:void UpdateHeroScreenStatusBar(struct tag_message &)@0x0006e0be
VA(0x0046d957, 0x57)
int hero::NumArtifacts(void) {
    int count = 0;
    int i;

    for (i = 0; i < HERO_ARTIFACT_SLOT_COUNT; i++) {
        if (m_artifacts[i] >= 0)
            count++;
    }
    return count;
}

VA(0x0046d9ae, 0x52e)
void UpdateHeroScreenStatusBar(struct tag_message &) {}

// donor PoL RVA 0x0006e816; preferred Buka symbol ?HeroHandler@@YIHAAUtag_message@@@Z
// donor Buka TU SOURCE/HERO; HoMM1 owner inferred from contiguous order
// evidence: graph:4;base=0.360602;margin=0.481730;shape=0.266;size=0.702;calls=0.568;alternate=pol20:int HeroHandler(struct tag_message &)@0x0006e816
VA(0x0046dedc, 0x6d4)
int HeroHandler(struct tag_message &) { return 0; }

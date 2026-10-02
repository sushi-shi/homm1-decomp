// Located from HoMM2 Buka 2.1; PoL 2.0 supplies the VC4 declaration.

#include <match.h>

#include <BASE/Misc.h>
#include <H1/All.h>
#include <H1/KB.h>
#include <SOURCE/dialogTypes.h>
#include <SOURCE/kbwin.h>
#include <SOURCE/X_GLOBAL.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// clang-format off
// herowind.bin widget ids. Names follow UpdateHeroScreenStatusBar's
// cHeroScreen texts (0x00493998) and what HeroView, UpdateArmies and
// HeroHandler send to or do with each id; artifact and army slots are
// indexed from their first id, primary stats by HeroPrimaryStat.
H1_ENUM_BEGIN(HeroScreenControl)
    HERO_SCREEN_TITLE = 2,
    HERO_SCREEN_ARTIFACT_BACKGROUND_FIRST = 6,
    HERO_SCREEN_ARTIFACT_FIRST = 20,
    HERO_SCREEN_PORTRAIT = 65,
    HERO_SCREEN_STAT_VALUE_FIRST = 76,
    HERO_SCREEN_ATTACK = 81,
    HERO_SCREEN_STAT_FIRST = HERO_SCREEN_ATTACK,
    HERO_SCREEN_DEFENSE = 82,
    HERO_SCREEN_SPELL_POWER = 83,
    HERO_SCREEN_KNOWLEDGE = 84,
    HERO_SCREEN_CHARACTERISTICS = 85,
    HERO_SCREEN_CREST = 86,
    HERO_SCREEN_ARMY_BACKGROUND_FIRST = 87,
    HERO_SCREEN_ARMY_CREATURE_FIRST = 92,
    HERO_SCREEN_ARMY_COUNT_FIRST = 97,
    HERO_SCREEN_ARMY_SLOT_FIRST = 102,
    HERO_SCREEN_MORALE_FIRST = 200,
    HERO_SCREEN_MORALE_LAST = 202,
    HERO_SCREEN_LUCK_FIRST = 203,
    HERO_SCREEN_LUCK_LAST = 205,
    HERO_SCREEN_EXPERIENCE_ICON = 206,
    HERO_SCREEN_EXPERIENCE = 207,
    HERO_SCREEN_STATUS_FIRST = 300,
    HERO_SCREEN_STATUS_TEXT = 302,
    HERO_SCREEN_EXIT = DIALOG_BUTTON_0,
    HERO_SCREEN_DISMISS = DIALOG_BUTTON_3
H1_ENUM_END(HeroScreenControl)
// clang-format on

// donor PoL RVA 0x0006c3a0; preferred Buka symbol ??0hero@@QAE@XZ
// donor Buka TU SOURCE/HERO; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.493986;margin=0.210035;shape=0.273;size=0.962;calls=1.000;alternate=pol20:void hero::constructor(void)@0x0006c3a0
VA(0x0046ba90, 0x68)
hero::hero(void) {
    m_id = 0;
    m_owner = 0;
    m_x = 0;
    m_y = 0;
    m_heroClass = 0;
    m_portrait = 0;
    m_name[0] = 0;
    heroWin = NULL;
    giHeroScreenSrcIndex = -1;
}

// Buka 2.1 hero::GetArmyStrengths: an empty body in both games.
// @dead-code
// Zero-ref: no incoming call, jump or relocated reference in retail.
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
        result = static_cast<int>(result * gfClassNavigationMod[m_heroClass]);
    } else {
        speed = 3;
        for (j = 0; j < ARMY_GROUP_SLOT_COUNT; j++) {
            if (m_army.m_creatureTypes[j] != CREATURE_NONE
                && gMonsterDatabase[m_army.m_creatureTypes[j]].stats.speed < speed)
                speed = gMonsterDatabase[m_army.m_creatureTypes[j]].stats.speed;
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
        if (m_spells[i] != SPELL_NONE)
            combat++;
    }
    for (i = 0; i < HERO_SPELL_SLOT_COUNT - HERO_COMBAT_SPELL_SLOT_COUNT; i++) {
        if (m_spells[HERO_COMBAT_SPELL_SLOT_COUNT + i] != SPELL_NONE)
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
            m_spells[i] = SPELL_NONE;
            m_spellCharges[i] = 0;
            for (j = i + 1; j < HERO_COMBAT_SPELL_SLOT_COUNT; j++) {
                m_spells[j - 1] = m_spells[j];
                m_spellCharges[j - 1] = m_spellCharges[j];
            }
            m_spells[HERO_COMBAT_SPELL_SLOT_COUNT - 1] = SPELL_NONE;
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
            m_spells[i] = SPELL_NONE;
            m_spellCharges[i] = 0;
            for (k = i + 1; k < HERO_SPELL_SLOT_COUNT; k++) {
                m_spells[k - 1] = m_spells[k];
                m_spellCharges[k - 1] = m_spellCharges[k];
            }
            m_spells[HERO_SPELL_SLOT_COUNT - 1] = SPELL_NONE;
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
            if (m_spells[i] == spell || m_spells[i] == SPELL_NONE) {
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
            if (m_spells[i] == spell || m_spells[i] == SPELL_NONE) {
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
    gpWindowManager->UpdateScreenRegion(0, 0, LOGICAL_SCREEN_WIDTH, LOGICAL_SCREEN_HEIGHT);
}

// donor PoL RVA 0x0006f354; preferred Buka symbol ?HeroView@@YIHHHH@Z
// donor Buka TU SOURCE/HERO; HoMM1 owner inferred from contiguous order
// evidence: graph:8;base=0.391018;margin=1.082891;shape=0.247;size=0.310;calls=0.359;strings=herowind.bin;alternate=pol20:int HeroView(int, int, int)@0x0006f354
VA(0x0046c2ed, 0x6c2)
signed char hero::HeroView(signed char viewOnly) {
    int armyLuckLevel;
    int armyMoraleLevel;
    tag_message message;
    short i;
    int shown;

    gpAdvManager->TrimLoopingSounds(8);
    gbHeroWindShowing = 1;
    gpWindowManager->FadeScreen(1, 8, NULL);
    heroWin = new heroWindow(0, 0, "herowind.bin");
    if (!heroWin)
        MemError();
    gheroWin = heroWin;
    SetWinText(heroWin, 5);
    message.type = MESSAGE_WIDGET;
    sprintf(gText, "%s the %s", m_name, gClassNames[m_heroClass]);
    message.command = WIDGET_COMMAND_SET_TEXT;
    message.id = HERO_SCREEN_TITLE;
    message.text = gText;
    heroWin->BroadcastMessage(message);
    message.command = WIDGET_COMMAND_CLEAR_FLAGS;
    message.value = WIDGET_FLAG_DRAW;
    for (i = 0; i < ARMY_GROUP_SLOT_COUNT; i++) {
        message.id = i + HERO_SCREEN_STAT_FIRST;
        heroWin->BroadcastMessage(message);
        message.id = i + HERO_SCREEN_ARMY_SLOT_FIRST;
        heroWin->BroadcastMessage(message);
    }
    if (viewOnly || gpTownManager->m_castleDialogActive
        || (!gpCurPlayer->m_townCount && gpCurPlayer->m_heroCount == 1)) {
        message.id = HERO_SCREEN_DISMISS;
        message.value = WIDGET_FLAG_ENABLED | WIDGET_FLAG_DRAW;
        heroWin->BroadcastMessage(message);
    }
    sprintf(gText, "port%04d.icn", m_portrait);
    message.command = WIDGET_COMMAND_SET_ICON;
    message.id = HERO_SCREEN_PORTRAIT;
    message.text = gText;
    heroWin->BroadcastMessage(message);
    message.command = WIDGET_COMMAND_SET_TEXT;
    for (i = 0; i < HERO_PRIMARY_STAT_COUNT; i++) {
        sprintf(gText, "%d", m_primaryStats[i]);
        message.id = i + HERO_SCREEN_STAT_VALUE_FIRST;
        message.text = gText;
        heroWin->BroadcastMessage(message);
    }
    armyLuckLevel = gpGame->GetLuck(this, NULL);
    for (i = 0; i < 3; i++) {
        message.command = WIDGET_COMMAND_SET_FRAME;
        message.id = i + HERO_SCREEN_LUCK_FIRST;
        if (armyLuckLevel < 0)
            message.value = 12;
        else if (armyLuckLevel == 0)
            message.value = 16;
        else
            message.value = 11;
        heroWin->BroadcastMessage(message);
    }
    shown = abs(armyLuckLevel);
    if (shown <= 0)
        shown = 1;
    for (i = 3; i > shown; i--) {
        message.command = WIDGET_COMMAND_CLEAR_FLAGS;
        message.id = i + (HERO_SCREEN_LUCK_FIRST - 1);
        message.value = WIDGET_FLAG_ENABLED | WIDGET_FLAG_DRAW;
        heroWin->BroadcastMessage(message);
    }
    armyMoraleLevel = m_army.GetMorale(this, NULL);
    for (i = 0; i < 3; i++) {
        message.command = WIDGET_COMMAND_SET_FRAME;
        message.id = i + HERO_SCREEN_MORALE_FIRST;
        if (armyMoraleLevel < 0)
            message.value = 14;
        else if (armyMoraleLevel == 0)
            message.value = 17;
        else
            message.value = 13;
        heroWin->BroadcastMessage(message);
    }
    shown = abs(armyMoraleLevel);
    if (shown <= 0)
        shown = 1;
    for (i = 3; i > shown; i--) {
        message.command = WIDGET_COMMAND_CLEAR_FLAGS;
        message.id = i + (HERO_SCREEN_MORALE_FIRST - 1);
        message.value = WIDGET_FLAG_ENABLED | WIDGET_FLAG_DRAW;
        heroWin->BroadcastMessage(message);
    }
    sprintf(gText, "%ld", m_experience);
    message.command = WIDGET_COMMAND_SET_TEXT;
    message.id = HERO_SCREEN_EXPERIENCE;
    message.text = gText;
    heroWin->BroadcastMessage(message);
    sprintf(gText, "crst%04d.icn", m_heroClass + gpCurPlayer->Color() * 4);
    message.command = WIDGET_COMMAND_SET_ICON;
    message.id = HERO_SCREEN_CREST;
    heroWin->BroadcastMessage(message);
    UpdateArmies();
    for (i = 0; i < HERO_ARTIFACT_SLOT_COUNT; i++) {
        message.id = i + HERO_SCREEN_ARTIFACT_FIRST;
        if (m_artifacts[i] != ARTIFACT_NONE) {
            message.command = WIDGET_COMMAND_SET_FRAME;
            message.value = m_artifacts[i];
            heroWin->BroadcastMessage(message);
            if (m_artifacts[i] >= 4) {
                message.command = WIDGET_COMMAND_CLEAR_FLAGS;
                message.id = i + HERO_SCREEN_ARTIFACT_BACKGROUND_FIRST;
                message.value = WIDGET_FLAG_DRAW;
                heroWin->BroadcastMessage(message);
            }
        } else {
            message.command = WIDGET_COMMAND_CLEAR_FLAGS;
            message.value = WIDGET_FLAG_DRAW;
            heroWin->BroadcastMessage(message);
            message.id = i + HERO_SCREEN_ARTIFACT_BACKGROUND_FIRST;
            heroWin->BroadcastMessage(message);
        }
    }
    RedrawHeroScreen();
    gpWindowManager->FadeScreen(0, 8, NULL);
    gpHVHero = this;
    gpWindowManager->DoDialog(heroWin, HeroHandler, 0);
    gpWindowManager->FadeScreen(1, 8, NULL);
    delete heroWin;
    gheroWin = NULL;
    if (gpWindowManager->m_dialogResult == HERO_SCREEN_DISMISS) {
        return 1;
    } else {
        m_mobility = CalcMobility();
        if (m_mobility < m_remainingMobility)
            m_remainingMobility = m_mobility;
    }
    gbHeroWindShowing = 0;
    return 0;
}

// donor PoL RVA 0x0006cab1; preferred Buka symbol ?HeroMessageUpdate@@YIXPAD@Z
// donor Buka TU SOURCE/HERO; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.570065;margin=0.065812;shape=0.448;size=0.919;calls=1.000;alternate=pol20:void HeroMessageUpdate(char *)@0x0006cab1
VA(0x0046c9af, 0x7c)
void HeroMessageUpdate(char* text) {
    tag_message message;

    if (!gheroWin)
        return;
    message.type = MESSAGE_WIDGET;
    message.command = WIDGET_COMMAND_SET_TEXT;
    message.id = HERO_SCREEN_STATUS_TEXT;
    message.text = text;
    gheroWin->BroadcastMessage(message);
    gheroWin->DrawWindow(0, HERO_SCREEN_STATUS_FIRST, HERO_SCREEN_STATUS_TEXT);
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
            message.command = WIDGET_COMMAND_SET_FLAGS;
        else
            message.command = WIDGET_COMMAND_CLEAR_FLAGS;
        message.value = WIDGET_FLAG_DRAW;
        message.id = i + HERO_SCREEN_ARMY_SLOT_FIRST;
        heroWin->BroadcastMessage(message);
    }
    heroWin->DrawWindow();
    gpWindowManager->UpdateScreenRegion(0, 0, LOGICAL_SCREEN_WIDTH, LOGICAL_SCREEN_HEIGHT);
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
        if (m_army.m_creatureTypes[i] == CREATURE_NONE) {
            message.command = WIDGET_COMMAND_SET_FRAME;
            message.id = i + HERO_SCREEN_ARMY_BACKGROUND_FIRST;
            message.value = 2;
            heroWin->BroadcastMessage(message);
            message.command = WIDGET_COMMAND_CLEAR_FLAGS;
            message.id = i + HERO_SCREEN_ARMY_CREATURE_FIRST;
            message.value = WIDGET_FLAG_DRAW;
            heroWin->BroadcastMessage(message);
            message.id = i + HERO_SCREEN_ARMY_COUNT_FIRST;
            heroWin->BroadcastMessage(message);
            message.id = i + HERO_SCREEN_ARMY_SLOT_FIRST;
            heroWin->BroadcastMessage(message);
        } else {
            message.command = WIDGET_COMMAND_SET_FRAME;
            message.id = i + HERO_SCREEN_ARMY_BACKGROUND_FIRST;
            message.value = m_army.m_creatureTypes[i] / 6 + 3;
            heroWin->BroadcastMessage(message);
            message.id = i + HERO_SCREEN_ARMY_CREATURE_FIRST;
            message.value = m_army.m_creatureTypes[i];
            heroWin->BroadcastMessage(message);
            message.command = WIDGET_COMMAND_SET_FLAGS;
            message.value = WIDGET_FLAG_DRAW;
            heroWin->BroadcastMessage(message);
            sprintf(gText, "%d", m_army.m_creatureCounts[i]);
            message.command = WIDGET_COMMAND_SET_TEXT;
            message.id = i + HERO_SCREEN_ARMY_COUNT_FIRST;
            message.text = gText;
            heroWin->BroadcastMessage(message);
            message.command = WIDGET_COMMAND_SET_FLAGS;
            message.value = WIDGET_FLAG_DRAW;
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
        NormalDialog(gText, NORMAL_DIALOG_TYPE_QUICK_VIEW, 0xb1, 0x19, NORMAL_DIALOG_NO_RESOURCE, 0, NORMAL_DIALOG_NO_RESOURCE, 0, NORMAL_DIALOG_NO_OR_TEXT);
        return;
    }
    win = new heroWindow(0xb1, 0x19, "vstat.bin");
    if (!win)
        MemError();
    strcpy(gText, gStatNames[stat]);
    message.type = MESSAGE_WIDGET;
    message.command = WIDGET_COMMAND_SET_TEXT;
    message.id = 1;
    message.text = gText;
    win->BroadcastMessage(message);
    strcpy(gText, gStatDesc[stat]);
    message.type = MESSAGE_WIDGET;
    message.command = WIDGET_COMMAND_SET_TEXT;
    message.id = 2;
    message.text = gText;
    win->BroadcastMessage(message);
    gpWindowManager->DoDialog(win, TrueFalseDialogHandler, 0);
    delete win;
}

VA(0x0046ce3f, 0x4a)
void hero::ViewArtifact(signed char artifact, signed char quickView) {
    NormalDialog(gArtifactDesc[artifact], quickView == 0 ? NORMAL_DIALOG_TYPE_OK : NORMAL_DIALOG_TYPE_QUICK_VIEW, -1, 0x1c, NORMAL_DIALOG_NO_RESOURCE, 0, NORMAL_DIALOG_NO_RESOURCE, 0, NORMAL_DIALOG_NO_OR_TEXT);
}

// donor PoL RVA 0x0006ce8b; preferred Buka symbol ?Dismiss@hero@@QAEHXZ
// donor Buka TU SOURCE/HERO; HoMM1 owner inferred from contiguous order
// evidence: graph:3;base=0.462026;margin=0.671820;shape=0.242;size=0.843;calls=1.000;alternate=pol20:int hero::Dismiss(void)@0x0006ce8b
VA(0x0046ce89, 0x59)
signed char hero::Dismiss(void) {
    NormalDialog("Are you sure you want to dismiss this Hero?", NORMAL_DIALOG_TYPE_YES_NO, 0xb1, 0x1c,
                 NORMAL_DIALOG_NO_RESOURCE, 0, NORMAL_DIALOG_NO_RESOURCE, 0, NORMAL_DIALOG_NO_OR_TEXT);
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
        gpGame->RestoreCell(m_x, m_y, m_locationType, m_occupiedTown, NULL, 1);
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

// Buka 2.1 hero::GetExperience.
VA(0x0046d334, 0xd0)
int hero::GetExperience(int level) {
    int experience;
    int stage;
    int incr;

    if (level <= HERO_EXPERIENCE_LEVEL_TABLE_COUNT)
        return gMinExpForLevel[m_heroClass][level - 1];
    stage = HERO_EXPERIENCE_LEVEL_TABLE_COUNT + 1;
    incr = static_cast<int>((gMinExpForLevel[m_heroClass][HERO_EXPERIENCE_LEVEL_TABLE_COUNT - 1]
                  - gMinExpForLevel[m_heroClass][HERO_EXPERIENCE_LEVEL_TABLE_COUNT - 2])
                 * 1.2);
    experience = gMinExpForLevel[m_heroClass][HERO_EXPERIENCE_LEVEL_TABLE_COUNT - 1] + incr;
    while (stage < level) {
        incr = static_cast<int>(incr * 1.2);
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
        if (gMinExpForLevel[m_heroClass][nLevel - 1] > experienceValue)
            return nLevel - 1;
    }
    growth = static_cast<int>((gMinExpForLevel[m_heroClass][HERO_EXPERIENCE_LEVEL_TABLE_COUNT - 1]
                    - gMinExpForLevel[m_heroClass][HERO_EXPERIENCE_LEVEL_TABLE_COUNT - 2])
                   * 1.2);
    experience = gMinExpForLevel[m_heroClass][HERO_EXPERIENCE_LEVEL_TABLE_COUNT - 1] + growth;
    nLevel = HERO_EXPERIENCE_LEVEL_TABLE_COUNT + 1;
    while (experience < experienceValue) {
        growth = static_cast<int>(growth * 1.2);
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
        if (gHeroSkillBonus[m_heroClass][highIndex][0] > roll) {
            stats[0]++;
        } else {
            roll -= gHeroSkillBonus[m_heroClass][highIndex][0];
            if (gHeroSkillBonus[m_heroClass][highIndex][1] > roll) {
                stats[1]++;
            } else {
                roll -= gHeroSkillBonus[m_heroClass][highIndex][1];
                if (gHeroSkillBonus[m_heroClass][highIndex][2] > roll)
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
        NormalDialog(gText, NORMAL_DIALOG_TYPE_OK, -1, -1, NORMAL_DIALOG_HERO, m_id, NORMAL_DIALOG_NO_RESOURCE, 0, NORMAL_DIALOG_NO_OR_TEXT);
        gpSoundManager->SwitchAmbientMusic(gpAdvManager->m_currentTerrain);
    }
}

// Buka 2.1 hero::NumArtifacts.
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
void UpdateHeroScreenStatusBar(short widgetId) {
    tag_message message; // Unused; retail keeps the donor's message frame.
    short slot;

    switch (widgetId) {
    case HERO_SCREEN_CREST:
        strcpy(gText, cHeroScreen[0]);
        break;
    case HERO_SCREEN_ATTACK:
    case HERO_SCREEN_DEFENSE:
    case HERO_SCREEN_SPELL_POWER:
    case HERO_SCREEN_KNOWLEDGE:
        sprintf(gText, cHeroScreen[1], gStatNames[widgetId - HERO_SCREEN_STAT_FIRST]);
        break;
    case HERO_SCREEN_CHARACTERISTICS:
        sprintf(gText, cHeroScreen[2]);
        break;
    case HERO_SCREEN_MORALE_FIRST:
    case HERO_SCREEN_MORALE_FIRST + 1:
    case HERO_SCREEN_MORALE_LAST:
        if (gpHVHero->m_army.GetMorale(gpHVHero, NULL) > 0)
            sprintf(gText, cHeroScreen[3]);
        else if (gpHVHero->m_army.GetMorale(gpHVHero, NULL) == 0)
            sprintf(gText, cHeroScreen[4]);
        else
            sprintf(gText, cHeroScreen[5]);
        break;
    case HERO_SCREEN_LUCK_FIRST:
    case HERO_SCREEN_LUCK_FIRST + 1:
    case HERO_SCREEN_LUCK_LAST:
        if (gpGame->GetLuck(gpHVHero, NULL) > 0)
            sprintf(gText, cHeroScreen[6]);
        else if (gpGame->GetLuck(gpHVHero, NULL) == 0)
            sprintf(gText, cHeroScreen[7]);
        else
            sprintf(gText, cHeroScreen[8]);
        break;
    case HERO_SCREEN_EXPERIENCE_ICON:
    case HERO_SCREEN_EXPERIENCE:
        sprintf(gText, cHeroScreen[9]);
        break;
    case HERO_SCREEN_ARMY_SLOT_FIRST:
    case HERO_SCREEN_ARMY_SLOT_FIRST + 1:
    case HERO_SCREEN_ARMY_SLOT_FIRST + 2:
    case HERO_SCREEN_ARMY_SLOT_FIRST + 3:
    case HERO_SCREEN_ARMY_SLOT_FIRST + 4:
        slot = widgetId - HERO_SCREEN_ARMY_SLOT_FIRST;
        if (giHeroScreenSrcIndex == -1) {
            if (gpHVHero->m_army.m_creatureTypes[slot] != CREATURE_NONE)
                sprintf(gText, cHeroScreen[10], gArmyNames[gpHVHero->m_army.m_creatureTypes[slot]]);
            else
                strcpy(gText, cHeroScreen[11]);
        } else if (slot == giHeroScreenSrcIndex) {
            sprintf(gText, cHeroScreen[10], gArmyNames[gpHVHero->m_army.m_creatureTypes[slot]]);
        } else if (gpTownManager->m_castleDialogActive) {
            if (gpHVHero->m_army.m_creatureTypes[slot] != CREATURE_NONE)
                sprintf(gText, cHeroScreen[10], gArmyNames[gpHVHero->m_army.m_creatureTypes[slot]]);
            else
                strcpy(gText, cHeroScreen[11]);
        } else if (gpHVHero->m_army.m_creatureTypes[slot] == CREATURE_NONE) {
            sprintf(gText, cHeroScreen[12],
                    gArmyNames[gpHVHero->m_army.m_creatureTypes[giHeroScreenSrcIndex]]);
        } else {
            sprintf(gText, cHeroScreen[13],
                    gArmyNames[gpHVHero->m_army.m_creatureTypes[giHeroScreenSrcIndex]],
                    gArmyNames[gpHVHero->m_army.m_creatureTypes[slot]]);
        }
        break;
    case HERO_SCREEN_ARTIFACT_FIRST:
    case HERO_SCREEN_ARTIFACT_FIRST + 1:
    case HERO_SCREEN_ARTIFACT_FIRST + 2:
    case HERO_SCREEN_ARTIFACT_FIRST + 3:
    case HERO_SCREEN_ARTIFACT_FIRST + 4:
    case HERO_SCREEN_ARTIFACT_FIRST + 5:
    case HERO_SCREEN_ARTIFACT_FIRST + 6:
    case HERO_SCREEN_ARTIFACT_FIRST + 7:
    case HERO_SCREEN_ARTIFACT_FIRST + 8:
    case HERO_SCREEN_ARTIFACT_FIRST + 9:
    case HERO_SCREEN_ARTIFACT_FIRST + 10:
    case HERO_SCREEN_ARTIFACT_FIRST + 11:
    case HERO_SCREEN_ARTIFACT_FIRST + 12:
    case HERO_SCREEN_ARTIFACT_FIRST + 13:
        if (gpHVHero->m_artifacts[widgetId - HERO_SCREEN_ARTIFACT_FIRST] == ARTIFACT_NONE)
            sprintf(gText, cHeroScreen[11]);
        else if (gpHVHero->m_artifacts[widgetId - HERO_SCREEN_ARTIFACT_FIRST] == ARTIFACT_MAGIC_BOOK)
            strcpy(gText, cHeroScreen[14]);
        else
            sprintf(gText, cHeroScreen[15], gArtifactNames[gpHVHero->m_artifacts[widgetId - HERO_SCREEN_ARTIFACT_FIRST]]);
        break;
    case HERO_SCREEN_DISMISS:
        sprintf(gText, cHeroScreen[16], gpHVHero->m_name, gClassNames[gpHVHero->m_heroClass]);
        break;
    case HERO_SCREEN_EXIT:
        strcpy(gText, cHeroScreen[17]);
        break;
    default:
        strcpy(gText, cHeroScreen[18]);
        break;
    }
    HeroMessageUpdate(gText);
}

// donor PoL RVA 0x0006e816; preferred Buka symbol ?HeroHandler@@YIHAAUtag_message@@@Z
// donor Buka TU SOURCE/HERO; HoMM1 owner inferred from contiguous order
// evidence: graph:4;base=0.360602;margin=0.481730;shape=0.266;size=0.702;calls=0.568;alternate=pol20:int HeroHandler(struct tag_message &)@0x0006e816
VA(0x0046dedc, 0x6c8)
short HeroHandler(struct tag_message& message) {
    tag_message newEvent;
    int unusedValue15;
    int unusedValue21;
    int unusedValue16;
    int unusedValue22;
    int heroLevel;
    int nextLevelExp;
    signed char quickView;
    signed char finished = 0;
    short slot;
    short temporary;
    int spare;

    if (message.modifiers & MESSAGE_MODIFIER_RIGHT_BUTTON)
        quickView = 1;
    else
        quickView = 0;
    if (message.type == MESSAGE_WIDGET) {
        switch (message.command) {
        case WIDGET_COMMAND_HOVER:
            if (message.id == gpWindowManager->m_lastHoverId)
                break;
            gpWindowManager->m_lastHoverId = message.id;
            UpdateHeroScreenStatusBar(message.id);
            return 1;
        case WIDGET_NOTIFY_DESELECT:
            if (!quickView) {
                switch (message.id) {
                case HERO_SCREEN_DISMISS:
                    if (gpHVHero->Dismiss())
                        finished = 1;
                    break;
                case HERO_SCREEN_EXIT:
                    finished = 1;
                    break;
                default:
                    break;
                }
            }
            break;
        case WIDGET_NOTIFY_SELECT:
            switch (message.id) {
            case HERO_SCREEN_CREST:
                if (!quickView) {
                    gpGame->Overview();
                    gpHVHero->RedrawHeroScreen();
                    gpWindowManager->FadeScreen(0, 8, NULL);
                }
                break;
            case HERO_SCREEN_ATTACK:
            case HERO_SCREEN_DEFENSE:
            case HERO_SCREEN_SPELL_POWER:
            case HERO_SCREEN_KNOWLEDGE:
                gpHVHero->ViewStat(message.id - HERO_SCREEN_STAT_FIRST, quickView);
                break;
            case HERO_SCREEN_MORALE_FIRST:
            case HERO_SCREEN_MORALE_FIRST + 1:
            case HERO_SCREEN_MORALE_LAST:
                gpGame->ShowMoraleInfo(gpHVHero, quickView == 0 ? 1 : 4);
                break;
            case HERO_SCREEN_LUCK_FIRST:
            case HERO_SCREEN_LUCK_FIRST + 1:
            case HERO_SCREEN_LUCK_LAST:
                gpGame->ShowLuckInfo(gpHVHero, quickView == 0 ? 1 : 4);
                break;
            case HERO_SCREEN_EXPERIENCE_ICON:
            case HERO_SCREEN_EXPERIENCE:
                heroLevel = gpHVHero->GetLevel(gpHVHero->m_experience);
                nextLevelExp = gpHVHero->GetExperience(heroLevel + 1);
                sprintf(gText, "Level %d\n\nExperience %d\n\nNext level %d", heroLevel,
                        gpHVHero->m_experience, nextLevelExp);
                NormalDialog(gText, quickView == 0 ? NORMAL_DIALOG_TYPE_OK : NORMAL_DIALOG_TYPE_QUICK_VIEW, -1, -1, NORMAL_DIALOG_NO_RESOURCE, 0, NORMAL_DIALOG_NO_RESOURCE, 0, NORMAL_DIALOG_NO_OR_TEXT);
                break;
            case HERO_SCREEN_ARMY_SLOT_FIRST:
            case HERO_SCREEN_ARMY_SLOT_FIRST + 1:
            case HERO_SCREEN_ARMY_SLOT_FIRST + 2:
            case HERO_SCREEN_ARMY_SLOT_FIRST + 3:
            case HERO_SCREEN_ARMY_SLOT_FIRST + 4:
                slot = message.id - HERO_SCREEN_ARMY_SLOT_FIRST;
                if (!quickView && giHeroScreenSrcIndex == -1) {
                    if (gpHVHero->m_army.m_creatureTypes[slot] != CREATURE_NONE) {
                        giHeroScreenSrcIndex = slot;
                        gpHVHero->HeroScreenUpdate();
                    }
                } else if ((quickView && gpHVHero->m_army.m_creatureTypes[slot] != CREATURE_NONE)
                           || (!quickView
                               && message.id - HERO_SCREEN_ARMY_SLOT_FIRST == giHeroScreenSrcIndex)) {
                    gpGame->ViewArmy(119, 20, gpHVHero->m_army.m_creatureTypes[slot],
                                     gpHVHero->m_army.m_creatureCounts[slot], NULL,
                                     quickView || gpTownManager->m_castleDialogActive == 1
                                         || gpHVHero->m_army.GetNumArmies() == 1,
                                     0, quickView, gpHVHero, NULL, &gpHVHero->m_army);
                    if (!quickView)
                        giHeroScreenSrcIndex = -1;
                    gpHVHero->HeroScreenUpdate();
                } else if (!quickView && gpTownManager->m_castleDialogActive) {
                    if (gpHVHero->m_army.m_creatureTypes[slot] != CREATURE_NONE) {
                        giHeroScreenSrcIndex = slot;
                        gpHVHero->HeroScreenUpdate();
                    }
                } else if (!quickView) {
                    temporary = gpHVHero->m_army.m_creatureTypes[slot];
                    gpHVHero->m_army.m_creatureTypes[slot] =
                        gpHVHero->m_army.m_creatureTypes[giHeroScreenSrcIndex];
                    gpHVHero->m_army.m_creatureTypes[giHeroScreenSrcIndex] = temporary;
                    temporary = gpHVHero->m_army.m_creatureCounts[slot];
                    gpHVHero->m_army.m_creatureCounts[slot] =
                        gpHVHero->m_army.m_creatureCounts[giHeroScreenSrcIndex];
                    gpHVHero->m_army.m_creatureCounts[giHeroScreenSrcIndex] = temporary;
                    giHeroScreenSrcIndex = -1;
                    gpHVHero->HeroScreenUpdate();
                }
                if (!quickView) {
                    gpWindowManager->m_lastHoverId = -1;
                    UpdateHeroScreenStatusBar(message.id);
                }
                break;
            case HERO_SCREEN_ARTIFACT_FIRST:
            case HERO_SCREEN_ARTIFACT_FIRST + 1:
            case HERO_SCREEN_ARTIFACT_FIRST + 2:
            case HERO_SCREEN_ARTIFACT_FIRST + 3:
            case HERO_SCREEN_ARTIFACT_FIRST + 4:
            case HERO_SCREEN_ARTIFACT_FIRST + 5:
            case HERO_SCREEN_ARTIFACT_FIRST + 6:
            case HERO_SCREEN_ARTIFACT_FIRST + 7:
            case HERO_SCREEN_ARTIFACT_FIRST + 8:
            case HERO_SCREEN_ARTIFACT_FIRST + 9:
            case HERO_SCREEN_ARTIFACT_FIRST + 10:
            case HERO_SCREEN_ARTIFACT_FIRST + 11:
            case HERO_SCREEN_ARTIFACT_FIRST + 12:
            case HERO_SCREEN_ARTIFACT_FIRST + 13:
                if (gpHVHero->m_artifacts[message.id - HERO_SCREEN_ARTIFACT_FIRST] != ARTIFACT_NONE) {
                    if (!quickView
                        && gpHVHero->m_artifacts[message.id - HERO_SCREEN_ARTIFACT_FIRST]
                               == ARTIFACT_MAGIC_BOOK)
                        gpGame->ViewSpells(gpHVHero, 2, ViewSpecialHandler, 1);
                    else
                        gpHVHero->ViewArtifact(gpHVHero->m_artifacts[message.id - HERO_SCREEN_ARTIFACT_FIRST],
                                               quickView);
                }
                break;
            }
            break;
        default:
            break;
        }
    }
    if (finished) {
        gpWindowManager->m_dialogResult = message.id;
        message.command = message.id = WIDGET_COMMAND_DIALOG_SELECT;
        return 2;
    } else {
        return 1;
    }
}

// HERO owns retail .data 0x004a0a58-0x004a0b2b.
DATA(0x004a0a58)
class heroWindow* gheroWin = NULL;

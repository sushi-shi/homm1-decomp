// Located from HoMM2 Buka 2.1; PoL 2.0 supplies the VC4 declaration.

#include <match.h>

#include <BASE/heroWindow.h>
#include <BASE/heroWindowManager.h>
#include <BASE/message.h>
#include <BASE/Misc.h>
#include <BASE/miscwin.h>
#include <BASE/resourceManager.h>
#include <BASE/audio.h>
#include <BASE/widget.h>
#include <SOURCE/advManager.h>
#include <SOURCE/armyGroup.h>
#include <SOURCE/dialogTypes.h>
#include <SOURCE/EVENTS.h>
#include <SOURCE/game.h>
#include <SOURCE/hero.h>
#include <SOURCE/KB.h>
#include <SOURCE/kbwin.h>
#include <SOURCE/mapCell.h>
#include <SOURCE/mapObjectTypes.h>
#include <SOURCE/philAI.h>
#include <SOURCE/playerData.h>
#include <SOURCE/town.h>
#include <SOURCE/townManager.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// clang-format off
// herowind.bin widget ids. Names follow UpdateHeroScreenStatusBar's
// gHeroScreen texts (0x004937b0) and what HeroView, UpdateArmies and
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

// gHeroScreen (0x004937b0) status-bar texts, as UpdateHeroScreenStatusBar
// picks them: "Kingdom Overview", "View %s Info", "Additional hero
// characteristics", good/neutral/bad morale and luck, "View Experience
// Info", "Select %s", "Empty", "Move %s", "Exchange %s with %s", "View
// Spells", "View %s Info", "Dismiss %s the %s", "Exit Hero Screen",
// "Hero Screen" (Buka HeroScreenText numbering from 1).
H1_ENUM_BEGIN(HeroScreenText)
    HERO_TEXT_KINGDOM_OVERVIEW = 0,
    HERO_TEXT_PRIMARY_STAT = 1,
    HERO_TEXT_ADDITIONAL_STATS = 2,
    HERO_TEXT_GOOD_MORALE = 3,
    HERO_TEXT_NEUTRAL_MORALE = 4,
    HERO_TEXT_BAD_MORALE = 5,
    HERO_TEXT_GOOD_LUCK = 6,
    HERO_TEXT_NEUTRAL_LUCK = 7,
    HERO_TEXT_BAD_LUCK = 8,
    HERO_TEXT_EXPERIENCE = 9,
    HERO_TEXT_SELECT_ARMY = 10,
    HERO_TEXT_EMPTY = 11,
    HERO_TEXT_MOVE_ARMY = 12,
    HERO_TEXT_EXCHANGE_ARMIES = 13,
    HERO_TEXT_VIEW_SPELLS = 14,
    HERO_TEXT_ARTIFACT = 15,
    HERO_TEXT_DISMISS = 16,
    HERO_TEXT_EXIT = 17,
    HERO_TEXT_SCREEN = 18
H1_ENUM_END(HeroScreenText)

// Frames HeroView sets on the three luck and three morale icons, and how
// many of each the screen shows.
H1_ENUM_BEGIN(HeroScreenMoodFrame)
    HERO_LUCK_FRAME_GOOD = 11,
    HERO_LUCK_FRAME_BAD = 12,
    HERO_MORALE_FRAME_GOOD = 13,
    HERO_MORALE_FRAME_BAD = 14,
    HERO_LUCK_FRAME_NEUTRAL = 16,
    HERO_MORALE_FRAME_NEUTRAL = 17
H1_ENUM_END(HeroScreenMoodFrame)

H1_ENUM_CONST_BEGIN(HeroScreenMoodConstant)
    HERO_SCREEN_MOOD_ICON_COUNT = 3
H1_ENUM_CONST_END(HeroScreenMoodConstant)

// giHeroScreenSrcIndex: the army slot picked up on the hero screen, or NONE.
// UpdateArmies shows an empty slot with background frame EMPTY and a
// creature over its faction's frame (FACTION_FIRST + type / faction size).
H1_ENUM_CONST_BEGIN(HeroScreenArmyConstant)
    HERO_SCREEN_SOURCE_NONE = -1,
    HERO_ARMY_BACKGROUND_EMPTY_FRAME = 2,
    HERO_ARMY_BACKGROUND_FACTION_FIRST_FRAME = 3
H1_ENUM_CONST_END(HeroScreenArmyConstant)

// vstat.bin, ViewStat's primary-stat window: title and description texts.
H1_ENUM_BEGIN(HeroStatViewControl)
    HERO_STAT_VIEW_TITLE = 1,
    HERO_STAT_VIEW_DESCRIPTION = 2
H1_ENUM_END(HeroStatViewControl)

// gHeroLevel: CheckLevel's "%s has gained" lead, then one level or %d levels.
H1_ENUM_BEGIN(HeroLevelText)
    HERO_LEVEL_TEXT_GAINED = 0,
    HERO_LEVEL_TEXT_ONE_LEVEL = 1,
    HERO_LEVEL_TEXT_LEVELS = 2
H1_ENUM_END(HeroLevelText)

// donor PoL RVA 0x0006c3a0; preferred Buka symbol ??0hero@@QAE@XZ
// donor Buka TU SOURCE/HERO; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.493986;margin=0.210035;shape=0.273;size=0.962;calls=1.000;alternate=pol20:void hero::constructor(void)@0x0006c3a0
VA(0x00438f20, 0x5d)
// clang-format on
hero::hero(void) {
    m_id = 0;
    m_owner = 0;
    m_x = 0;
    m_y = 0;
    m_heroClass = 0;
    m_portrait = 0;
    m_name[0] = 0;
    heroWin = NULL;
    giHeroScreenSrcIndex = HERO_SCREEN_SOURCE_NONE;
}

// Buka 2.1 hero::GetArmyStrengths: an empty body in both games.
// @dead-code
// Zero-ref: no incoming call, jump or relocated reference in retail.
VA(0x00438f7d, 0xd)
void hero::GetArmyStrengths(u32* const) {}

VA(0x00438f8a, 0x4b)
i8 hero::HasArtifact(i8 artifact) {
    i16 i;

    for (i = 0; i < HERO_ARTIFACT_SLOT_COUNT; i++) {
        if (m_artifacts[i] == artifact)
            return 1;
    }
    return 0;
}

// donor PoL RVA 0x0006c526; preferred Buka symbol ?CalcMobility@hero@@QAEHXZ
// donor Buka TU SOURCE/HERO; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.418178;margin=0.237740;shape=0.210;size=0.862;calls=0.714;alternate=pol20:int hero::CalcMobility(void)@0x0006c526
VA(0x00438fd5, 0x1bd)
i16 hero::CalcMobility(void) {
    i16 mobilityTable[3] = {40, 50, 60};
    const i16 seaBaseMobility = 60;
    const i16 lighthousePoints = 20;
    const i16 astrolabeMobility = 40;
    const i16 compassMobility = 20;
    const i16 nomadBootsBonus = 24;
    const i16 travelerBonus = 12;
    i32 movePoints;
    i16 slowestSpeedValue;
    i32 creatureIndex;

    if (m_eventFlags & HERO_EVENT_EMBARKED) {
        if (gpGame->m_mines[MINE_SLOT_LIGHTHOUSE].owner == m_owner)
            movePoints = seaBaseMobility + lighthousePoints;
        else
            movePoints = seaBaseMobility;
        if (HasArtifact(ARTIFACT_SAILORS_ASTROLABE))
            movePoints += astrolabeMobility;
        movePoints = static_cast<i32>(movePoints * gClassNavigationMod[m_heroClass]);
    } else {
        slowestSpeedValue = CREATURE_SPEED_FAST;
        for (creatureIndex = 0; creatureIndex < ARMY_GROUP_SLOT_COUNT; creatureIndex++) {
            if (m_army.m_creatureTypes[creatureIndex] != CREATURE_NONE
                && gMonsterDatabase[m_army.m_creatureTypes[creatureIndex]].stats.speed
                       < slowestSpeedValue)
                slowestSpeedValue =
                    gMonsterDatabase[m_army.m_creatureTypes[creatureIndex]].stats.speed;
        }
        movePoints = mobilityTable[slowestSpeedValue - 1];
        if (HasArtifact(ARTIFACT_NOMAD_BOOTS))
            movePoints += nomadBootsBonus;
        if (HasArtifact(ARTIFACT_TRAVELER_BOOTS))
            movePoints += travelerBonus;
    }
    if (HasArtifact(ARTIFACT_TRUE_COMPASS))
        movePoints += compassMobility;
    if (m_owner >= 0 && !gbHumanPlayer[m_owner]
        && gpGame->m_players[m_owner].m_difficulty >= DIFFICULTY_COUNT - 1)
        movePoints += 3;
    return movePoints;
}

VA(0x00439192, 0x41)
i8 hero::HasSpell(i8 spell) {
    i32 i;

    for (i = 0; i < HERO_SPELL_SLOT_COUNT; i++) {
        if (m_spells[i] == spell)
            return 1;
    }
    return 0;
}

VA(0x004391d3, 0xc8)
i16 hero::GetNumSpells(H1_ENUM_PARAM(HeroSpellType, i8) type) {
    i16 combat = 0;
    i16 adventure = 0;
    i16 jx;

    for (jx = 0; jx < HERO_COMBAT_SPELL_SLOT_COUNT; jx++) {
        if (m_spells[jx] != SPELL_NONE)
            combat++;
    }
    for (jx = 0; jx < HERO_SPELL_SLOT_COUNT - HERO_COMBAT_SPELL_SLOT_COUNT; jx++) {
        if (m_spells[HERO_COMBAT_SPELL_SLOT_COUNT + jx] != SPELL_NONE)
            adventure++;
    }
    switch (type) {
        case SPELL_TYPE_COMBAT:
            return combat;
        case SPELL_TYPE_ADVENTURE:
            return adventure;
        case SPELL_TYPE_ALL:
            return combat + adventure;
    }
    return 0;
}

VA(0x0043929b, 0x204)
void hero::UseSpell(i8 spell) {
    i16 i;
    i32 j;
    i32 k;

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

VA(0x0043949f, 0x1a7)
i32 hero::AddSpell(i8 spell, i8 charges, i32 checkOnly) {
    i32 added = 0;
    i16 i;

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
VA(0x00439646, 0x48)
void hero::RedrawHeroScreen(void) {
    gpResourceManager->GetBackdrop("heroscrn.bmp", gpWindowManager->m_screen);
    heroWin->DrawWindow();
    gpWindowManager->UpdateScreenRegion(0, 0, LOGICAL_SCREEN_WIDTH, LOGICAL_SCREEN_HEIGHT);
}

// donor PoL RVA 0x0006f354; preferred Buka symbol ?HeroView@@YIHHHH@Z
// donor Buka TU SOURCE/HERO; HoMM1 owner inferred from contiguous order
// evidence: graph:8;base=0.391018;margin=1.082891;shape=0.247;size=0.310;calls=0.359;strings=herowind.bin;alternate=pol20:int HeroView(int, int, int)@0x0006f354
VA(0x0043968e, 0x64a)
i8 hero::HeroView(i8 viewOnly) {
    i32 heroLuck;
    i32 moraleValue;
    tag_message message;
    i16 i;
    i32 magnitude;

    gpAdvManager->TrimLoopingSounds(8);
    gHeroWindShowing = 1;
    gpWindowManager->FadeScreen(WINDOW_FADE_OUT, 8, NULL);
    heroWin = new heroWindow(0, 0, "herowind.bin");
    if (!heroWin)
        MemError();
    gheroWin = heroWin;
    SetWinText(heroWin, WINDOW_TEXT_HERO);
    message.type = MESSAGE_WIDGET;
    sprintf(gText, localization::Tr("hero.title"), m_name, gClassNames[m_heroClass]);
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
    heroLuck = gpGame->GetLuck(this, NULL);
    for (i = 0; i < HERO_SCREEN_MOOD_ICON_COUNT; i++) {
        message.command = WIDGET_COMMAND_SET_FRAME;
        message.id = i + HERO_SCREEN_LUCK_FIRST;
        if (heroLuck < 0)
            message.value = HERO_LUCK_FRAME_BAD;
        else if (heroLuck == 0)
            message.value = HERO_LUCK_FRAME_NEUTRAL;
        else
            message.value = HERO_LUCK_FRAME_GOOD;
        heroWin->BroadcastMessage(message);
    }
    magnitude = abs(heroLuck);
    if (magnitude <= 0)
        magnitude = 1;
    for (i = HERO_SCREEN_MOOD_ICON_COUNT; i > magnitude; i--) {
        message.command = WIDGET_COMMAND_CLEAR_FLAGS;
        message.id = i + (HERO_SCREEN_LUCK_FIRST - 1);
        message.value = WIDGET_FLAG_ENABLED | WIDGET_FLAG_DRAW;
        heroWin->BroadcastMessage(message);
    }
    moraleValue = m_army.GetMorale(this, NULL);
    for (i = 0; i < HERO_SCREEN_MOOD_ICON_COUNT; i++) {
        message.command = WIDGET_COMMAND_SET_FRAME;
        message.id = i + HERO_SCREEN_MORALE_FIRST;
        if (moraleValue < 0)
            message.value = HERO_MORALE_FRAME_BAD;
        else if (moraleValue == 0)
            message.value = HERO_MORALE_FRAME_NEUTRAL;
        else
            message.value = HERO_MORALE_FRAME_GOOD;
        heroWin->BroadcastMessage(message);
    }
    magnitude = abs(moraleValue);
    if (magnitude <= 0)
        magnitude = 1;
    for (i = HERO_SCREEN_MOOD_ICON_COUNT; i > magnitude; i--) {
        message.command = WIDGET_COMMAND_CLEAR_FLAGS;
        message.id = i + (HERO_SCREEN_MORALE_FIRST - 1);
        message.value = WIDGET_FLAG_ENABLED | WIDGET_FLAG_DRAW;
        heroWin->BroadcastMessage(message);
    }
    sprintf(gText, "\n%d", m_experience);
    message.command = WIDGET_COMMAND_SET_TEXT;
    message.id = HERO_SCREEN_EXPERIENCE;
    message.text = gText;
    heroWin->BroadcastMessage(message);
    sprintf(gText, "crst%04d.icn", gpCurPlayer->Color() * 4 + m_heroClass);
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
            if (m_artifacts[i] >= ARTIFACT_REGULAR_FIRST) {
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
    gpWindowManager->FadeScreen(WINDOW_FADE_IN, 8, NULL);
    gHVHero = this;
    gpWindowManager->DoDialog(heroWin, HeroHandler, 0);
    gpWindowManager->FadeScreen(WINDOW_FADE_OUT, 8, NULL);
    delete heroWin;
    gheroWin = NULL;
    if (gpWindowManager->m_dialogResult == HERO_SCREEN_DISMISS) {
        return 1;
    } else {
        m_mobility = CalcMobility();
        if (m_remainingMobility > m_mobility)
            m_remainingMobility = m_mobility;
    }
    gHeroWindShowing = 0;
    return 0;
}

// donor PoL RVA 0x0006cab1; preferred Buka symbol ?HeroMessageUpdate@@YIXPAD@Z
// donor Buka TU SOURCE/HERO; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.570065;margin=0.065812;shape=0.448;size=0.919;calls=1.000;alternate=pol20:void HeroMessageUpdate(char *)@0x0006cab1
VA(0x00439cd8, 0x6c)
void HeroMessageUpdate(char* text) {
    tag_message message;

    if (!gheroWin)
        return;
    SET_WIDGET_MESSAGE(message, WIDGET_COMMAND_SET_TEXT, HERO_SCREEN_STATUS_TEXT);
    message.text = text;
    gheroWin->BroadcastMessage(message);
    gheroWin->DrawWindow(0, HERO_SCREEN_STATUS_FIRST, HERO_SCREEN_STATUS_TEXT);
    gpWindowManager->UpdateScreenRegion(0, 459, 640, 20);
}

// donor PoL RVA 0x0006cb33; preferred Buka symbol ?HeroScreenUpdate@hero@@QAEXXZ
// donor Buka TU SOURCE/HERO; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.542456;margin=0.317795;shape=0.400;size=0.865;calls=1.000;alternate=pol20:void hero::HeroScreenUpdate(void)@0x0006cb33
VA(0x00439d44, 0x99)
void hero::HeroScreenUpdate(void) {
    tag_message message;
    i16 i;

    message.type = MESSAGE_WIDGET;
    UpdateArmies();
    for (i = 0; i < ARMY_GROUP_SLOT_COUNT; i++) {
        if (giHeroScreenSrcIndex == i)
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
VA(0x00439ddd, 0x1b6)
void hero::UpdateArmies(void) {
    tag_message message;
    i16 i;

    message.type = MESSAGE_WIDGET;
    for (i = 0; i < ARMY_GROUP_SLOT_COUNT; i++) {
        if (m_army.m_creatureTypes[i] == CREATURE_NONE) {
            message.command = WIDGET_COMMAND_SET_FRAME;
            message.id = i + HERO_SCREEN_ARMY_BACKGROUND_FIRST;
            message.value = HERO_ARMY_BACKGROUND_EMPTY_FRAME;
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
            message.value = m_army.m_creatureTypes[i] / CREATURE_FACTION_SIZE
                            + HERO_ARMY_BACKGROUND_FACTION_FIRST_FRAME;
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

VA(0x00439f93, 0x181)
void hero::ViewStat(i8 stat, i8 quickView) {
    heroWindow* win;
    tag_message message;

    if (quickView) {
        sprintf(gText, "%s\n\n%s", gStatNames[stat], gStatDesc[stat]);
        NormalDialog(
            gText,
            NORMAL_DIALOG_TYPE_QUICK_VIEW,
            0xb1,
            0x19,
            NORMAL_DIALOG_NO_RESOURCE,
            0,
            NORMAL_DIALOG_NO_RESOURCE,
            0,
            NORMAL_DIALOG_NO_OR_TEXT
        );
        return;
    }
    win = new heroWindow(0xb1, 0x19, "vstat.bin");
    if (!win)
        MemError();
    strcpy(gText, gStatNames[stat]);
    SET_WIDGET_MESSAGE(message, WIDGET_COMMAND_SET_TEXT, HERO_STAT_VIEW_TITLE);
    message.text = gText;
    win->BroadcastMessage(message);
    strcpy(gText, gStatDesc[stat]);
    SET_WIDGET_MESSAGE(message, WIDGET_COMMAND_SET_TEXT, HERO_STAT_VIEW_DESCRIPTION);
    message.text = gText;
    win->BroadcastMessage(message);
    gpWindowManager->DoDialog(win, TrueFalseDialogHandler, 0);
    delete win;
}

VA(0x0043a114, 0x3e)
void hero::ViewArtifact(i8 artifact, i8 quickView) {
    NormalDialog(
        gArtifactDesc[artifact],
        quickView == 0 ? NORMAL_DIALOG_TYPE_OK : NORMAL_DIALOG_TYPE_QUICK_VIEW,
        -1,
        0x1c,
        NORMAL_DIALOG_NO_RESOURCE,
        0,
        NORMAL_DIALOG_NO_RESOURCE,
        0,
        NORMAL_DIALOG_NO_OR_TEXT
    );
}

// donor PoL RVA 0x0006ce8b; preferred Buka symbol ?Dismiss@hero@@QAEHXZ
// donor Buka TU SOURCE/HERO; HoMM1 owner inferred from contiguous order
// evidence: graph:3;base=0.462026;margin=0.671820;shape=0.242;size=0.843;calls=1.000;alternate=pol20:int hero::Dismiss(void)@0x0006ce8b
VA(0x0043a152, 0x47)
i8 hero::Dismiss(void) {
    NormalDialog(
        localization::Tr("hero.dismiss.confirm"),
        NORMAL_DIALOG_TYPE_YES_NO,
        0xb1,
        0x1c,
        NORMAL_DIALOG_NO_RESOURCE,
        0,
        NORMAL_DIALOG_NO_RESOURCE,
        0,
        NORMAL_DIALOG_NO_OR_TEXT
    );
    if (gpWindowManager->m_dialogResult == NORMAL_DIALOG_CONFIRM) {
        Deallocate();
        return 1;
    }
    return 0;
}

// donor PoL RVA 0x0006cee8; preferred Buka symbol ?Deallocate@hero@@QAEXH@Z
// donor Buka TU SOURCE/HERO; HoMM1 owner inferred from contiguous order
// evidence: graph:5;base=0.482098;margin=0.987612;shape=0.300;size=0.883;calls=0.875;alternate=pol20:void hero::Deallocate(int)@0x0006cee8
VA(0x0043a199, 0x400)
void hero::Deallocate(void) {
    i32 oldOwner;
    i16 i;
    playerData* playerPtr;
    i8 heroNum;
    town* curTown;
    i32 availSlot;

    oldOwner = m_owner;
    playerPtr = &gpGame->m_players[m_owner];
    gpAdvManager->MobilizeCurrHero(0);
    gpAdvManager->HideRoute(0, 0, 0);
    if (m_eventFlags & HERO_EVENT_EMBARKED) {
        for (i = 0; i < GAME_BOAT_COUNT; i++) {
            if (gpGame->m_boats[i].heroId == m_id) {
                gpGame->m_boats[i].heroId = HERO_ID_NONE;
                gpGame->m_boatSlots[i] = HERO_ID_NONE;
            }
        }
    }
    if (m_locationType == (MAP_TRIGGER_EVENT | MAP_OBJECT_TOWN)) {
        curTown = gpGame->GetTown(m_occupiedTown);
        curTown->m_occupyingHeroId = TOWN_OCCUPYING_HERO_NONE;
    }
    if (giCurPlayer != m_owner || gpGame->m_players[m_owner].m_currentHero != m_id
        || !gpAdvManager->m_heroContextLocked)
        gpGame->RestoreCell(m_x, m_y, m_locationType, m_occupiedTown, NULL, 1);
    if (!gbCombatSurrender) {
        for (i = 0; i < ARMY_GROUP_SLOT_COUNT; i++)
            m_army.Dismiss(i);
    }
    heroNum = -1;
    for (i = 0; i < playerPtr->m_heroCount; i++) {
        if (playerPtr->m_heroIds[i] == m_id)
            heroNum = i;
    }
    for (i = heroNum; i < playerPtr->m_heroCount - 1; i++)
        playerPtr->m_heroIds[i] = playerPtr->m_heroIds[i + 1];
    playerPtr->m_heroIds[playerPtr->m_heroCount - 1] = HERO_ID_NONE;
    if (playerPtr->m_currentHero == m_id) {
        playerPtr->m_currentHero = HERO_ID_NONE;
        if (giCurPlayer == m_owner) {
            gpAdvManager->m_cursorActive = 0;
            gpGame->m_map[m_x][m_y].m_flags &= ~MAP_CELL_HERO_CURSOR;
        }
        if (oldOwner == giCurPlayer)
            gpAdvManager->m_heroContextLocked = 0;
    }
    playerPtr->m_heroCount--;
    playerPtr->m_heroLocatorPage = 0;
    gpGame->m_availableHeroes[m_id] = HERO_AVAILABILITY_UNAVAILABLE;
    if (gbRetreatWin) {
        availSlot = Random(0, HERO_AVAILABLE_SLOT_COUNT - 1);
        if (gpGame->m_availableHeroes[gpGame->m_players[m_owner].m_availableHeroIds[availSlot]]
            == HERO_AVAILABILITY_RETREATED)
            gpGame->m_availableHeroes[gpGame->m_players[m_owner].m_availableHeroIds[availSlot]] =
                HERO_AVAILABILITY_UNAVAILABLE;
        gpGame->m_players[m_owner].m_availableHeroIds[availSlot] = m_id;
        gpGame->m_availableHeroes[m_id] = HERO_AVAILABILITY_RETREATED;
    }
    m_owner = HERO_OWNER_NONE;
    m_destinationX = m_destinationY = HERO_DESTINATION_NONE;
    if (!gbCombatSurrender)
        gpGame->SetRandomHeroArmies(m_id, RANDOM_HERO_NORMAL_ARMY);
    CheckEndGame(0);
}

// Buka 2.1 hero::GetExperience.
VA(0x0043a599, 0xb5)
i32 hero::GetExperience(i32 level) {
    i32 experience;
    i32 curStage;
    i32 incr;

    if (level <= HERO_EXPERIENCE_LEVEL_TABLE_COUNT)
        return gMinExpForLevel[m_heroClass][level - 1];
    curStage = HERO_EXPERIENCE_LEVEL_TABLE_COUNT + 1;
    incr = static_cast<i32>(
        (gMinExpForLevel[m_heroClass][HERO_EXPERIENCE_LEVEL_TABLE_COUNT - 1]
         - gMinExpForLevel[m_heroClass][HERO_EXPERIENCE_LEVEL_TABLE_COUNT - 2])
        * 1.2
    );
    experience = gMinExpForLevel[m_heroClass][HERO_EXPERIENCE_LEVEL_TABLE_COUNT - 1] + incr;
    while (curStage < level) {
        incr = static_cast<i32>(incr * 1.2);
        experience += incr;
        curStage++;
    }
    return experience;
}

VA(0x0043a64e, 0xd7)
i32 hero::GetLevel(i32 experienceValue) {
    i32 experience;
    i32 levelCounter;
    i32 growth;

    for (levelCounter = 1; levelCounter <= HERO_EXPERIENCE_LEVEL_TABLE_COUNT; levelCounter++) {
        if (experienceValue < gMinExpForLevel[m_heroClass][levelCounter - 1])
            return levelCounter - 1;
    }
    growth = static_cast<i32>(
        (gMinExpForLevel[m_heroClass][HERO_EXPERIENCE_LEVEL_TABLE_COUNT - 1]
         - gMinExpForLevel[m_heroClass][HERO_EXPERIENCE_LEVEL_TABLE_COUNT - 2])
        * 1.2
    );
    experience = gMinExpForLevel[m_heroClass][HERO_EXPERIENCE_LEVEL_TABLE_COUNT - 1] + growth;
    levelCounter = HERO_EXPERIENCE_LEVEL_TABLE_COUNT + 1;
    while (experienceValue > experience) {
        growth = static_cast<i32>(growth * 1.2);
        experience += growth;
        levelCounter++;
    }
    return levelCounter - 1;
}

VA(0x0043a725, 0x179)
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
    if (m_eventFlags & HERO_EVENT_STATUE) {
        m_morale -= 2;
        m_eventFlags -= HERO_EVENT_STATUE;
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

VA(0x0043a89e, 0x13)
void hero::ApplyBattleLossTemps(void) {
    ApplyBattleWinTemps();
}

// donor PoL RVA 0x0006d83f; preferred Buka symbol ?CheckLevel@hero@@QAEXXZ
// donor Buka TU SOURCE/HERO; HoMM1 owner inferred from contiguous order
// evidence: graph:3;base=0.312130;margin=0.246272;shape=0.276;size=0.445;calls=0.500;alternate=pol20:void hero::CheckLevel(void)@0x0006d83f
VA(0x0043a8b1, 0x2b7)
void hero::CheckLevel(void) {
    i32 oldLvl;
    i32 i;
    i32 stats[HERO_PRIMARY_STAT_COUNT];
    i32 levelCount;
    i32 highIndexNo;
    char curText[50];
    i32 rollVal;

    oldLvl = GetLevel(m_experience);
    if (oldLvl == m_level)
        return;
    levelCount = oldLvl - m_level;
    sprintf(gText, gHeroLevel[HERO_LEVEL_TEXT_GAINED], m_name);
    if (levelCount == 1)
        sprintf(curText, gHeroLevel[HERO_LEVEL_TEXT_ONE_LEVEL]);
    else
        sprintf(curText, gHeroLevel[HERO_LEVEL_TEXT_LEVELS], levelCount);
    strcat(gText, curText);
    stats[HERO_PRIMARY_ATTACK] = 0;
    stats[HERO_PRIMARY_DEFENSE] = 0;
    stats[HERO_PRIMARY_SPELL_POWER] = 0;
    stats[HERO_PRIMARY_KNOWLEDGE] = 0;
    for (i = m_level + 1; i <= oldLvl; i++) {
        highIndexNo = i - HERO_SKILL_BONUS_FIRST_LEVEL;
        if (highIndexNo > HERO_SKILL_BONUS_ROW_LAST)
            highIndexNo = HERO_SKILL_BONUS_ROW_LAST;
        SRand(m_randomSeed + i * HERO_LEVEL_RANDOM_SEED_FACTOR);
        rollVal = SRandom(1, 100);
        if (rollVal < gHeroSkillBonus[m_heroClass][highIndexNo][HERO_PRIMARY_ATTACK]) {
            stats[HERO_PRIMARY_ATTACK]++;
        } else {
            rollVal -= gHeroSkillBonus[m_heroClass][highIndexNo][HERO_PRIMARY_ATTACK];
            if (rollVal < gHeroSkillBonus[m_heroClass][highIndexNo][HERO_PRIMARY_DEFENSE]) {
                stats[HERO_PRIMARY_DEFENSE]++;
            } else {
                rollVal -= gHeroSkillBonus[m_heroClass][highIndexNo][HERO_PRIMARY_DEFENSE];
                if (rollVal < gHeroSkillBonus[m_heroClass][highIndexNo][HERO_PRIMARY_SPELL_POWER])
                    stats[HERO_PRIMARY_SPELL_POWER]++;
                else
                    stats[HERO_PRIMARY_KNOWLEDGE]++;
            }
        }
    }
    for (i = 0; i < HERO_PRIMARY_STAT_COUNT; i++) {
        if (stats[i] > 0) {
            m_primaryStats[i] += stats[i];
            sprintf(curText, "\n%s +%d", gStatNames[i], stats[i]);
            strcat(gText, curText);
        }
    }
    m_level = oldLvl;
    if (!gbInNewGameSetup && m_owner >= 0 && gbThisNetHumanPlayer[m_owner]) {
        PlayMusic(MUSIC_TRACK_LEVEL_UP);
        NormalDialog(
            gText,
            NORMAL_DIALOG_TYPE_OK,
            -1,
            -1,
            NORMAL_DIALOG_HERO,
            m_id,
            NORMAL_DIALOG_NO_RESOURCE,
            0,
            NORMAL_DIALOG_NO_OR_TEXT
        );
        PlayMusic(gpAdvManager->m_currentTerrain);
    }
}

// Buka 2.1 hero::NumArtifacts.
VA(0x0043ab68, 0x4b)
i32 hero::NumArtifacts(void) {
    i32 count = 0;
    i32 i;

    for (i = 0; i < HERO_ARTIFACT_SLOT_COUNT; i++) {
        if (m_artifacts[i] >= 0)
            count++;
    }
    return count;
}

VA(0x0043abb3, 0x502)
void UpdateHeroScreenStatusBar(i16 widgetId) {
    tag_message message; // Unused; retail keeps the donor's message frame.
    i16 slot;

    switch (widgetId) {
        case HERO_SCREEN_CREST:
            strcpy(gText, gHeroScreen[HERO_TEXT_KINGDOM_OVERVIEW]);
            break;
        case HERO_SCREEN_ATTACK:
        case HERO_SCREEN_DEFENSE:
        case HERO_SCREEN_SPELL_POWER:
        case HERO_SCREEN_KNOWLEDGE:
            sprintf(
                gText,
                gHeroScreen[HERO_TEXT_PRIMARY_STAT],
                gStatNames[widgetId - HERO_SCREEN_STAT_FIRST]
            );
            break;
        case HERO_SCREEN_CHARACTERISTICS:
            sprintf(gText, gHeroScreen[HERO_TEXT_ADDITIONAL_STATS]);
            break;
        case HERO_SCREEN_MORALE_FIRST:
        case HERO_SCREEN_MORALE_FIRST + 1:
        case HERO_SCREEN_MORALE_LAST:
            if (gHVHero->m_army.GetMorale(gHVHero, NULL) > 0)
                sprintf(gText, gHeroScreen[HERO_TEXT_GOOD_MORALE]);
            else if (gHVHero->m_army.GetMorale(gHVHero, NULL) == 0)
                sprintf(gText, gHeroScreen[HERO_TEXT_NEUTRAL_MORALE]);
            else
                sprintf(gText, gHeroScreen[HERO_TEXT_BAD_MORALE]);
            break;
        case HERO_SCREEN_LUCK_FIRST:
        case HERO_SCREEN_LUCK_FIRST + 1:
        case HERO_SCREEN_LUCK_LAST:
            if (gpGame->GetLuck(gHVHero, NULL) > 0)
                sprintf(gText, gHeroScreen[HERO_TEXT_GOOD_LUCK]);
            else if (gpGame->GetLuck(gHVHero, NULL) == 0)
                sprintf(gText, gHeroScreen[HERO_TEXT_NEUTRAL_LUCK]);
            else
                sprintf(gText, gHeroScreen[HERO_TEXT_BAD_LUCK]);
            break;
        case HERO_SCREEN_EXPERIENCE_ICON:
        case HERO_SCREEN_EXPERIENCE:
            sprintf(gText, gHeroScreen[HERO_TEXT_EXPERIENCE]);
            break;
        case HERO_SCREEN_ARMY_SLOT_FIRST:
        case HERO_SCREEN_ARMY_SLOT_FIRST + 1:
        case HERO_SCREEN_ARMY_SLOT_FIRST + 2:
        case HERO_SCREEN_ARMY_SLOT_FIRST + 3:
        case HERO_SCREEN_ARMY_SLOT_FIRST + 4:
            slot = widgetId - HERO_SCREEN_ARMY_SLOT_FIRST;
            if (giHeroScreenSrcIndex == HERO_SCREEN_SOURCE_NONE) {
                if (gHVHero->m_army.m_creatureTypes[slot] != CREATURE_NONE)
                    sprintf(
                        gText,
                        gHeroScreen[HERO_TEXT_SELECT_ARMY],
                        gArmyNames[gHVHero->m_army.m_creatureTypes[slot]]
                    );
                else
                    strcpy(gText, gHeroScreen[HERO_TEXT_EMPTY]);
            } else if (slot == giHeroScreenSrcIndex) {
                sprintf(
                    gText,
                    gHeroScreen[HERO_TEXT_SELECT_ARMY],
                    gArmyNames[gHVHero->m_army.m_creatureTypes[slot]]
                );
            } else if (gpTownManager->m_castleDialogActive) {
                if (gHVHero->m_army.m_creatureTypes[slot] != CREATURE_NONE)
                    sprintf(
                        gText,
                        gHeroScreen[HERO_TEXT_SELECT_ARMY],
                        gArmyNames[gHVHero->m_army.m_creatureTypes[slot]]
                    );
                else
                    strcpy(gText, gHeroScreen[HERO_TEXT_EMPTY]);
            } else if (gHVHero->m_army.m_creatureTypes[slot] == CREATURE_NONE) {
                sprintf(
                    gText,
                    gHeroScreen[HERO_TEXT_MOVE_ARMY],
                    gArmyNames[gHVHero->m_army.m_creatureTypes[giHeroScreenSrcIndex]]
                );
            } else {
                sprintf(
                    gText,
                    gHeroScreen[HERO_TEXT_EXCHANGE_ARMIES],
                    gArmyNames[gHVHero->m_army.m_creatureTypes[giHeroScreenSrcIndex]],
                    gArmyNames[gHVHero->m_army.m_creatureTypes[slot]]
                );
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
            if (gHVHero->m_artifacts[widgetId - HERO_SCREEN_ARTIFACT_FIRST] == ARTIFACT_NONE)
                sprintf(gText, gHeroScreen[HERO_TEXT_EMPTY]);
            else if (gHVHero->m_artifacts[widgetId - HERO_SCREEN_ARTIFACT_FIRST]
                     == ARTIFACT_MAGIC_BOOK)
                strcpy(gText, gHeroScreen[HERO_TEXT_VIEW_SPELLS]);
            else
                sprintf(
                    gText,
                    gHeroScreen[HERO_TEXT_ARTIFACT],
                    gArtifactNames[gHVHero->m_artifacts[widgetId - HERO_SCREEN_ARTIFACT_FIRST]]
                );
            break;
        case HERO_SCREEN_DISMISS:
            sprintf(
                gText,
                gHeroScreen[HERO_TEXT_DISMISS],
                gHVHero->m_name,
                gClassNames[gHVHero->m_heroClass]
            );
            break;
        case HERO_SCREEN_EXIT:
            strcpy(gText, gHeroScreen[HERO_TEXT_EXIT]);
            break;
        default:
            strcpy(gText, gHeroScreen[HERO_TEXT_SCREEN]);
            break;
    }
    HeroMessageUpdate(gText);
}

// donor PoL RVA 0x0006e816; preferred Buka symbol ?HeroHandler@@YIHAAUtag_message@@@Z
// donor Buka TU SOURCE/HERO; HoMM1 owner inferred from contiguous order
// evidence: graph:4;base=0.360602;margin=0.481730;shape=0.266;size=0.702;calls=0.568;alternate=pol20:int HeroHandler(struct tag_message &)@0x0006e816
VA(0x0043b0b5, 0x610)
i16 HeroHandler(struct tag_message& message) {
    tag_message newEvent;
    i32 unusedValue15;
    i32 unusedValue21;
    i32 unusedValue16;
    i32 unusedValue22;
    i32 curHeroLevel;
    i32 nextLevelExp;
    i8 quickViewVal;
    i8 complete = 0;
    i16 slot;
    i16 temporaryVal;
    i32 curSpare;

    if (message.modifiers & MESSAGE_MODIFIER_RIGHT_BUTTON)
        quickViewVal = 1;
    else
        quickViewVal = 0;
    if (message.type == MESSAGE_WIDGET) {
        switch (message.command) {
            case WIDGET_COMMAND_HOVER:
                if (gpWindowManager->m_lastHoverId == message.id)
                    break;
                gpWindowManager->m_lastHoverId = message.id;
                UpdateHeroScreenStatusBar(message.id);
                return MESSAGE_DISPATCH_CONSUME;
            case WIDGET_NOTIFY_DESELECT:
                if (!quickViewVal) {
                    switch (message.id) {
                        case HERO_SCREEN_DISMISS:
                            if (gHVHero->Dismiss())
                                complete = 1;
                            break;
                        case HERO_SCREEN_EXIT:
                            complete = 1;
                            break;
                        default:
                            break;
                    }
                }
                break;
            case WIDGET_NOTIFY_SELECT:
                switch (message.id) {
                    case HERO_SCREEN_CREST:
                        if (!quickViewVal) {
                            gpGame->Overview();
                            gHVHero->RedrawHeroScreen();
                            gpWindowManager->FadeScreen(WINDOW_FADE_IN, 8, NULL);
                        }
                        break;
                    case HERO_SCREEN_ATTACK:
                    case HERO_SCREEN_DEFENSE:
                    case HERO_SCREEN_SPELL_POWER:
                    case HERO_SCREEN_KNOWLEDGE:
                        gHVHero->ViewStat(message.id - HERO_SCREEN_STAT_FIRST, quickViewVal);
                        break;
                    case HERO_SCREEN_MORALE_FIRST:
                    case HERO_SCREEN_MORALE_FIRST + 1:
                    case HERO_SCREEN_MORALE_LAST:
                        gpGame->ShowMoraleInfo(
                            gHVHero,
                            quickViewVal == 0 ? NORMAL_DIALOG_TYPE_OK
                                              : NORMAL_DIALOG_TYPE_QUICK_VIEW
                        );
                        break;
                    case HERO_SCREEN_LUCK_FIRST:
                    case HERO_SCREEN_LUCK_FIRST + 1:
                    case HERO_SCREEN_LUCK_LAST:
                        gpGame->ShowLuckInfo(
                            gHVHero,
                            quickViewVal == 0 ? NORMAL_DIALOG_TYPE_OK
                                              : NORMAL_DIALOG_TYPE_QUICK_VIEW
                        );
                        break;
                    case HERO_SCREEN_EXPERIENCE_ICON:
                    case HERO_SCREEN_EXPERIENCE:
                        curHeroLevel = gHVHero->GetLevel(gHVHero->m_experience);
                        nextLevelExp = gHVHero->GetExperience(curHeroLevel + 1);
                        sprintf(
                            gText,
                            localization::Tr("hero.experience.details"),
                            curHeroLevel,
                            gHVHero->m_experience,
                            nextLevelExp
                        );
                        NormalDialog(
                            gText,
                            quickViewVal == 0 ? NORMAL_DIALOG_TYPE_OK
                                              : NORMAL_DIALOG_TYPE_QUICK_VIEW,
                            -1,
                            -1,
                            NORMAL_DIALOG_NO_RESOURCE,
                            0,
                            NORMAL_DIALOG_NO_RESOURCE,
                            0,
                            NORMAL_DIALOG_NO_OR_TEXT
                        );
                        break;
                    case HERO_SCREEN_ARMY_SLOT_FIRST:
                    case HERO_SCREEN_ARMY_SLOT_FIRST + 1:
                    case HERO_SCREEN_ARMY_SLOT_FIRST + 2:
                    case HERO_SCREEN_ARMY_SLOT_FIRST + 3:
                    case HERO_SCREEN_ARMY_SLOT_FIRST + 4:
                        slot = message.id - HERO_SCREEN_ARMY_SLOT_FIRST;
                        if (!quickViewVal && giHeroScreenSrcIndex == HERO_SCREEN_SOURCE_NONE) {
                            if (gHVHero->m_army.m_creatureTypes[slot] != CREATURE_NONE) {
                                giHeroScreenSrcIndex = slot;
                                gHVHero->HeroScreenUpdate();
                            }
                        } else if ((quickViewVal
                                    && gHVHero->m_army.m_creatureTypes[slot] != CREATURE_NONE)
                                   || (!quickViewVal
                                       && giHeroScreenSrcIndex
                                              == message.id - HERO_SCREEN_ARMY_SLOT_FIRST)) {
                            gpGame->ViewArmy(
                                119,
                                20,
                                gHVHero->m_army.m_creatureTypes[slot],
                                gHVHero->m_army.m_creatureCounts[slot],
                                NULL,
                                quickViewVal || gpTownManager->m_castleDialogActive == 1
                                    || gHVHero->m_army.GetNumArmies() == 1,
                                0,
                                quickViewVal,
                                gHVHero,
                                NULL,
                                &gHVHero->m_army
                            );
                            if (!quickViewVal)
                                giHeroScreenSrcIndex = HERO_SCREEN_SOURCE_NONE;
                            gHVHero->HeroScreenUpdate();
                        } else if (!quickViewVal && gpTownManager->m_castleDialogActive) {
                            if (gHVHero->m_army.m_creatureTypes[slot] != CREATURE_NONE) {
                                giHeroScreenSrcIndex = slot;
                                gHVHero->HeroScreenUpdate();
                            }
                        } else if (!quickViewVal) {
                            temporaryVal = gHVHero->m_army.m_creatureTypes[slot];
                            gHVHero->m_army.m_creatureTypes[slot] =
                                gHVHero->m_army.m_creatureTypes[giHeroScreenSrcIndex];
                            gHVHero->m_army.m_creatureTypes[giHeroScreenSrcIndex] = temporaryVal;
                            temporaryVal = gHVHero->m_army.m_creatureCounts[slot];
                            gHVHero->m_army.m_creatureCounts[slot] =
                                gHVHero->m_army.m_creatureCounts[giHeroScreenSrcIndex];
                            gHVHero->m_army.m_creatureCounts[giHeroScreenSrcIndex] = temporaryVal;
                            giHeroScreenSrcIndex = HERO_SCREEN_SOURCE_NONE;
                            gHVHero->HeroScreenUpdate();
                        }
                        if (!quickViewVal) {
                            gpWindowManager->m_lastHoverId = WINDOW_MANAGER_NO_HOVER_WIDGET;
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
                        if (gHVHero->m_artifacts[message.id - HERO_SCREEN_ARTIFACT_FIRST]
                            != ARTIFACT_NONE) {
                            if (!quickViewVal
                                && gHVHero->m_artifacts[message.id - HERO_SCREEN_ARTIFACT_FIRST]
                                       == ARTIFACT_MAGIC_BOOK)
                                gpGame->ViewSpells(gHVHero, SPELL_TYPE_ALL, ViewSpecialHandler, 1);
                            else
                                gHVHero->ViewArtifact(
                                    gHVHero->m_artifacts[message.id - HERO_SCREEN_ARTIFACT_FIRST],
                                    quickViewVal
                                );
                        }
                        break;
                }
                break;
            default:
                break;
        }
    }
    if (complete) {
        FINISH_DIALOG_MESSAGE(message);
        return MESSAGE_DISPATCH_FORWARD;
    } else {
        return MESSAGE_DISPATCH_CONSUME;
    }
}

// HERO owns retail .data 0x004a12bc-0x004a0b2b.
DATA(0x004a6c3c)
class heroWindow* gheroWin = NULL;

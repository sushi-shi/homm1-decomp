#include <H1/Ints.h>

#include <BASE/audio.h>
#include <BASE/heroWindow.h>
#include <BASE/heroWindowManager.h>
#include <BASE/message.h>
#include <BASE/Misc.h>
#include <BASE/miscwin.h>
#include <BASE/resourceManager.h>
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

hero::hero(void) {
    m_id = 0;
    m_owner = 0;
    m_x = 0;
    m_y = 0;
    m_heroClass = 0;
    m_portrait = 0;
    m_name[0] = 0;
    gHeroScreenWindow = NULL;
    gHeroScreenSrcIndex = HERO_SCREEN_SOURCE_NONE;
}

void hero::GetArmyStrengths(u32* const) {}

i8 hero::HasArtifact(i8 artifact) {
    i16 i;

    for (i = 0; i < HERO_ARTIFACT_SLOT_COUNT; i++) {
        if (m_artifacts[i] == artifact)
            return 1;
    }
    return 0;
}

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

    if (IsEmbarked()) {
        if (gGame->m_mines[MINE_SLOT_LIGHTHOUSE].owner == m_owner)
            movePoints = seaBaseMobility + lighthousePoints;
        else
            movePoints = seaBaseMobility;
        if (HasArtifact(ARTIFACT_SAILORS_ASTROLABE))
            movePoints += astrolabeMobility;
        movePoints = movePoints * gClassNavigationMod[m_heroClass];
    } else {
        slowestSpeedValue = CREATURE_SPEED_FAST;
        for (creatureIndex = 0; creatureIndex < ARMY_GROUP_SLOT_COUNT; creatureIndex++) {
            if (m_army.m_creatureTypes[creatureIndex] != CREATURE_NONE
                && (gMonsterDatabase[m_army.m_creatureTypes[creatureIndex]].stats.speed) < slowestSpeedValue)
                slowestSpeedValue = (gMonsterDatabase[m_army.m_creatureTypes[creatureIndex]].stats.speed);
        }
        movePoints = mobilityTable[slowestSpeedValue - 1];
        if (HasArtifact(ARTIFACT_NOMAD_BOOTS))
            movePoints += nomadBootsBonus;
        if (HasArtifact(ARTIFACT_TRAVELER_BOOTS))
            movePoints += travelerBonus;
    }
    if (HasArtifact(ARTIFACT_TRUE_COMPASS))
        movePoints += compassMobility;
    if (m_owner >= 0 && !gHumanPlayer[m_owner]
        && (gGame->m_players[m_owner].m_difficulty)
               >= PLAYER_TYPE_MOBILITY_BONUS_FIRST)
        movePoints += 3;
    return movePoints;
}

i8 hero::HasSpell(i8 spell) {
    i32 i;

    for (i = 0; i < HERO_SPELL_SLOT_COUNT; i++) {
        if (m_spells[i] == spell)
            return 1;
    }
    return 0;
}

i16 hero::GetNumSpells(i8 type) {
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

void hero::UseSpell(i8 spell) {
    i16 i;
    i32 j;
    i32 k;

    if (spell >= SPELL_FIRST && spell < SPELL_ADVENTURE_FIRST) {
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
    } else if (spell >= SPELL_ADVENTURE_FIRST && spell < SPELL_COUNT) {
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

i32 hero::AddSpell(i8 spell, i8 charges, b32 checkOnly) {
    i32 added = 0;
    i16 i;

    if (spell >= SPELL_FIRST && spell < SPELL_ADVENTURE_FIRST) {
        for (i = 0; i < HERO_COMBAT_SPELL_SLOT_COUNT; i++) {
            if (m_spells[i] == spell || m_spells[i] == SPELL_NONE) {
                if (m_spells[i] == spell)
                    added = charges - m_spellCharges[i];
                else
                    added = charges;
                if (checkOnly)
                    goto done;
                m_spells[i] = spell;
                if (!gCheatUnlimitedSpells[gCurPlayer][m_id] || m_spellCharges[i] <= charges)
                    m_spellCharges[i] = charges;
                break;
            }
        }
    }
    if (spell >= SPELL_ADVENTURE_FIRST && spell < SPELL_COUNT) {
        for (i = HERO_COMBAT_SPELL_SLOT_COUNT; i < HERO_SPELL_SLOT_COUNT; i++) {
            if (m_spells[i] == spell || m_spells[i] == SPELL_NONE) {
                if (m_spells[i] == spell)
                    added = charges - m_spellCharges[i];
                else
                    added = charges;
                if (checkOnly)
                    goto done;
                m_spells[i] = spell;
                if (!gCheatUnlimitedSpells[gCurPlayer][m_id] || m_spellCharges[i] <= charges)
                    m_spellCharges[i] = charges;
                break;
            }
        }
    }
done:
    return added;
}

void hero::RedrawHeroScreen(void) {
    gResourceManager->GetBackdrop("heroscrn.bmp", gWindowManager->m_screen);
    gHeroScreenWindow->DrawWindow();
    gWindowManager->UpdateScreenRegion(0, 0, LOGICAL_SCREEN_WIDTH, LOGICAL_SCREEN_HEIGHT);
}

i8 hero::HeroView(b8 viewOnly) {
    i32 heroLuck;
    i32 moraleValue;
    tag_message message;
    i16 i;
    i32 magnitude;

    gAdvManager->TrimLoopingSounds(8);
    gHeroWindShowing = true;
    gWindowManager->FadeScreen(WINDOW_FADE_OUT, 8, NULL);
    gHeroScreenWindow = new heroWindow(0, 0, "herowind.bin");
    if (!gHeroScreenWindow)
        MemError();
    gHeroWin = gHeroScreenWindow;
    SetWinText(gHeroScreenWindow, WINDOW_TEXT_HERO);
    message.type = MESSAGE_WIDGET;
    sprintf(gText, localization::Tr("hero.title"), m_name, gClassNames[m_heroClass]);
    message.command = WIDGET_COMMAND_SET_TEXT;
    message.id = HERO_SCREEN_TITLE;
    message.text = gText;
    gHeroScreenWindow->BroadcastMessage(message);
    message.command = WIDGET_COMMAND_CLEAR_FLAGS;
    message.value = WIDGET_FLAG_DRAW;
    for (i = 0; i < ARMY_GROUP_SLOT_COUNT; i++) {
        message.id = i + HERO_SCREEN_STAT_FIRST;
        gHeroScreenWindow->BroadcastMessage(message);
        message.id = i + HERO_SCREEN_ARMY_SLOT_FIRST;
        gHeroScreenWindow->BroadcastMessage(message);
    }
    if (viewOnly || gTownManager->m_castleDialogActive
        || (!gCurPlayerData->m_townCount && gCurPlayerData->m_heroCount == 1)) {
        message.id = HERO_SCREEN_DISMISS;
        message.value = WIDGET_FLAG_ENABLED | WIDGET_FLAG_DRAW;
        gHeroScreenWindow->BroadcastMessage(message);
    }
    sprintf(gText, "port%04d.icn", m_portrait);
    message.command = WIDGET_COMMAND_SET_ICON;
    message.id = HERO_SCREEN_PORTRAIT;
    message.text = gText;
    gHeroScreenWindow->BroadcastMessage(message);
    message.command = WIDGET_COMMAND_SET_TEXT;
    for (i = 0; i < HERO_PRIMARY_STAT_COUNT; i++) {
        sprintf(gText, "%d", m_primaryStats[i]);
        message.id = i + HERO_SCREEN_STAT_VALUE_FIRST;
        message.text = gText;
        gHeroScreenWindow->BroadcastMessage(message);
    }
    heroLuck = gGame->GetLuck(this, NULL);
    for (i = 0; i < HERO_SCREEN_MOOD_ICON_COUNT; i++) {
        message.command = WIDGET_COMMAND_SET_FRAME;
        message.id = i + HERO_SCREEN_LUCK_FIRST;
        if (heroLuck < 0)
            message.value = HERO_LUCK_FRAME_BAD;
        else if (heroLuck == 0)
            message.value = HERO_LUCK_FRAME_NEUTRAL;
        else
            message.value = HERO_LUCK_FRAME_GOOD;
        gHeroScreenWindow->BroadcastMessage(message);
    }
    magnitude = abs(heroLuck);
    if (magnitude <= 0)
        magnitude = 1;
    for (i = HERO_SCREEN_MOOD_ICON_COUNT; i > magnitude; i--) {
        message.command = WIDGET_COMMAND_CLEAR_FLAGS;
        message.id = i + (HERO_SCREEN_LUCK_FIRST - 1);
        message.value = WIDGET_FLAG_ENABLED | WIDGET_FLAG_DRAW;
        gHeroScreenWindow->BroadcastMessage(message);
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
        gHeroScreenWindow->BroadcastMessage(message);
    }
    magnitude = abs(moraleValue);
    if (magnitude <= 0)
        magnitude = 1;
    for (i = HERO_SCREEN_MOOD_ICON_COUNT; i > magnitude; i--) {
        message.command = WIDGET_COMMAND_CLEAR_FLAGS;
        message.id = i + (HERO_SCREEN_MORALE_FIRST - 1);
        message.value = WIDGET_FLAG_ENABLED | WIDGET_FLAG_DRAW;
        gHeroScreenWindow->BroadcastMessage(message);
    }
    sprintf(gText, "\n%d", m_experience);
    message.command = WIDGET_COMMAND_SET_TEXT;
    message.id = HERO_SCREEN_EXPERIENCE;
    message.text = gText;
    gHeroScreenWindow->BroadcastMessage(message);
    sprintf(
        gText,
        "crst%04d.icn",
        (gCurPlayerData->Color()) * 4 + m_heroClass
    );
    message.command = WIDGET_COMMAND_SET_ICON;
    message.id = HERO_SCREEN_CREST;
    gHeroScreenWindow->BroadcastMessage(message);
    UpdateArmies();
    for (i = 0; i < HERO_ARTIFACT_SLOT_COUNT; i++) {
        message.id = i + HERO_SCREEN_ARTIFACT_FIRST;
        if (m_artifacts[i] != ARTIFACT_NONE) {
            message.command = WIDGET_COMMAND_SET_FRAME;
            message.value = m_artifacts[i];
            gHeroScreenWindow->BroadcastMessage(message);
            if (m_artifacts[i] >= ARTIFACT_REGULAR_FIRST) {
                message.command = WIDGET_COMMAND_CLEAR_FLAGS;
                message.id = i + HERO_SCREEN_ARTIFACT_BACKGROUND_FIRST;
                message.value = WIDGET_FLAG_DRAW;
                gHeroScreenWindow->BroadcastMessage(message);
            }
        } else {
            message.command = WIDGET_COMMAND_CLEAR_FLAGS;
            message.value = WIDGET_FLAG_DRAW;
            gHeroScreenWindow->BroadcastMessage(message);
            message.id = i + HERO_SCREEN_ARTIFACT_BACKGROUND_FIRST;
            gHeroScreenWindow->BroadcastMessage(message);
        }
    }
    RedrawHeroScreen();
    gWindowManager->FadeScreen(WINDOW_FADE_IN, 8, NULL);
    gInfoViewedHero = this;
    gWindowManager->DoDialog(gHeroScreenWindow, HeroHandler, false);
    gWindowManager->FadeScreen(WINDOW_FADE_OUT, 8, NULL);
    delete gHeroScreenWindow;
    gHeroWin = NULL;
    if (gWindowManager->m_dialogResult == HERO_SCREEN_DISMISS) {
        gHeroWindShowing = false;
        return 1;
    } else {
        m_mobility = CalcMobility();
        if (m_remainingMobility > m_mobility)
            m_remainingMobility = m_mobility;
    }
    gHeroWindShowing = false;
    return 0;
}

void HeroMessageUpdate(char* text) {
    tag_message message;

    if (!gHeroWin)
        return;
    SET_WIDGET_MESSAGE(message, WIDGET_COMMAND_SET_TEXT, HERO_SCREEN_STATUS_TEXT);
    message.text = text;
    gHeroWin->BroadcastMessage(message);
    gHeroWin->DrawWindow(0, HERO_SCREEN_STATUS_FIRST, HERO_SCREEN_STATUS_TEXT);
    gWindowManager->UpdateScreenRegion(0, 459, LOGICAL_SCREEN_WIDTH, 20);
}

void hero::HeroScreenUpdate(void) {
    tag_message message;
    i16 i;

    message.type = MESSAGE_WIDGET;
    UpdateArmies();
    for (i = 0; i < ARMY_GROUP_SLOT_COUNT; i++) {
        if (gHeroScreenSrcIndex == i)
            message.command = WIDGET_COMMAND_SET_FLAGS;
        else
            message.command = WIDGET_COMMAND_CLEAR_FLAGS;
        message.value = WIDGET_FLAG_DRAW;
        message.id = i + HERO_SCREEN_ARMY_SLOT_FIRST;
        gHeroScreenWindow->BroadcastMessage(message);
    }
    gHeroScreenWindow->DrawWindow();
    gWindowManager->UpdateScreenRegion(0, 0, LOGICAL_SCREEN_WIDTH, LOGICAL_SCREEN_HEIGHT);
}

void hero::UpdateArmies(void) {
    tag_message message;
    i16 i;

    message.type = MESSAGE_WIDGET;
    for (i = 0; i < ARMY_GROUP_SLOT_COUNT; i++) {
        if (m_army.m_creatureTypes[i] == CREATURE_NONE) {
            message.command = WIDGET_COMMAND_SET_FRAME;
            message.id = i + HERO_SCREEN_ARMY_BACKGROUND_FIRST;
            message.value = HERO_ARMY_BACKGROUND_EMPTY_FRAME;
            gHeroScreenWindow->BroadcastMessage(message);
            message.command = WIDGET_COMMAND_CLEAR_FLAGS;
            message.id = i + HERO_SCREEN_ARMY_CREATURE_FIRST;
            message.value = WIDGET_FLAG_DRAW;
            gHeroScreenWindow->BroadcastMessage(message);
            message.id = i + HERO_SCREEN_ARMY_COUNT_FIRST;
            gHeroScreenWindow->BroadcastMessage(message);
            message.id = i + HERO_SCREEN_ARMY_SLOT_FIRST;
            gHeroScreenWindow->BroadcastMessage(message);
        } else {
            message.command = WIDGET_COMMAND_SET_FRAME;
            message.id = i + HERO_SCREEN_ARMY_BACKGROUND_FIRST;
            message.value = CREATURE_FACTION(m_army.m_creatureTypes[i])
                            + HERO_ARMY_BACKGROUND_FACTION_FIRST_FRAME;
            gHeroScreenWindow->BroadcastMessage(message);
            message.id = i + HERO_SCREEN_ARMY_CREATURE_FIRST;
            message.value = m_army.m_creatureTypes[i];
            gHeroScreenWindow->BroadcastMessage(message);
            message.command = WIDGET_COMMAND_SET_FLAGS;
            message.value = WIDGET_FLAG_DRAW;
            gHeroScreenWindow->BroadcastMessage(message);
            sprintf(gText, "%d", m_army.m_creatureCounts[i]);
            message.command = WIDGET_COMMAND_SET_TEXT;
            message.id = i + HERO_SCREEN_ARMY_COUNT_FIRST;
            message.text = gText;
            gHeroScreenWindow->BroadcastMessage(message);
            message.command = WIDGET_COMMAND_SET_FLAGS;
            message.value = WIDGET_FLAG_DRAW;
            gHeroScreenWindow->BroadcastMessage(message);
        }
    }
}

void hero::ViewStat(i8 stat, i8 quickView) {
    heroWindow* win;
    tag_message message;

    if (quickView) {
        sprintf(gText, "%s\n\n%s", gStatNames[stat], gStatDesc[stat]);
        NormalDialog(gText, NORMAL_DIALOG_TYPE_QUICK_VIEW, 0xb1, 0x19);
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
    gWindowManager->DoDialog(win, TrueFalseDialogHandler, false);
    delete win;
}

void hero::ViewArtifact(i8 artifact, i8 quickView) {
    NormalDialog(
        gArtifactDesc[artifact],
        quickView == 0 ? NORMAL_DIALOG_TYPE_OK : NORMAL_DIALOG_TYPE_QUICK_VIEW,
        -1,
        0x1c
    );
}

i8 hero::Dismiss(void) {
    NormalDialog(localization::Tr("hero.dismiss.confirm"), NORMAL_DIALOG_TYPE_YES_NO, 0xb1, 0x1c);
    if (gWindowManager->m_dialogResult == NORMAL_DIALOG_CONFIRM) {
        Deallocate();
        return 1;
    }
    return 0;
}

void hero::Deallocate(void) {
    i32 oldOwner;
    i16 i;
    playerData* playerPtr;
    i8 heroNum;
    town* curTown;
    i32 availSlot;

    oldOwner = m_owner;
    playerPtr = &gGame->m_players[m_owner];
    gAdvManager->MobilizeCurrHero(false);
    gAdvManager->HideRoute(false, false, false);
    if (IsEmbarked()) {
        for (i = 0; i < GAME_BOAT_COUNT; i++) {
            if (gGame->m_boats[i].heroId == m_id) {
                gGame->m_boats[i].heroId = HERO_ID_NONE;
                gGame->m_boatSlots[i] = HERO_ID_NONE;
            }
        }
    }
    if (m_locationType == MAP_EVENT_TRIGGER(MAP_OBJECT_TOWN)) {
        curTown = gGame->GetTown(m_occupiedTown);
        curTown->m_occupyingHeroId = TOWN_OCCUPYING_HERO_NONE;
    }
    if (gCurPlayer != m_owner || gGame->m_players[m_owner].m_currentHero != m_id
        || !gAdvManager->m_heroContextLocked)
        gGame->RestoreCell(m_x, m_y, m_locationType, m_occupiedTown, NULL, 1);
    if (!gCombatSurrender) {
        for (i = 0; i < ARMY_GROUP_SLOT_COUNT; i++)
            m_army.Dismiss(i);
    }
    if (!gRetreatWin)
        ResetToStartingState();
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
        if (gCurPlayer == m_owner) {
            gAdvManager->m_cursorActive = false;
            gGame->m_map[m_x][m_y].m_flags &= ~MAP_CELL_HERO_CURSOR;
        }
        if (oldOwner == gCurPlayer)
            gAdvManager->m_heroContextLocked = false;
    }
    playerPtr->m_heroCount--;
    playerPtr->m_heroLocatorPage = 0;
    gGame->m_availableHeroes[m_id] = HERO_AVAILABILITY_UNAVAILABLE;
    if (gRetreatWin) {
        availSlot = Random(0, HERO_AVAILABLE_SLOT_COUNT - 1);
        if (gGame->m_availableHeroes[gGame->m_players[m_owner].m_availableHeroIds[availSlot]]
            == HERO_AVAILABILITY_IN_TAVERN)
            gGame->m_availableHeroes[gGame->m_players[m_owner].m_availableHeroIds[availSlot]] =
                HERO_AVAILABILITY_UNAVAILABLE;
        gGame->m_players[m_owner].m_availableHeroIds[availSlot] = m_id;
        gGame->m_availableHeroes[m_id] = HERO_AVAILABILITY_IN_TAVERN;
    }
    m_owner = GAME_PLAYER_NONE;
    m_destinationX = m_destinationY = HERO_DESTINATION_NONE;
    if (!gCombatSurrender)
        gGame->SetRandomHeroArmies(m_id, RANDOM_HERO_NORMAL_ARMY);
    CheckEndGame(false);
}

// A hero hired from a tavern starts with a full day's movement, unless he
// retreated or surrendered today: then he has no movement left, or with the
// SoftRetreatSurrender option keeps what he had left.
void hero::SetRecruitedMobility(void) {
    if (m_fledState != HERO_FLED_NONE) {
        m_fledState = HERO_FLED_NONE;
        if (gConfig.softRetreatSurrender) {
            m_mobility = m_remainingMobility > 1 ? m_remainingMobility : 1;
        } else {
            m_remainingMobility = 0;
            m_mobility = CalcMobility();
        }
    } else {
        m_remainingMobility = CalcMobility();
        m_mobility = m_remainingMobility;
    }
}

// A hero who was killed or dismissed returns to the pool as he started the
// game: first level, the class's starting skills, no artifacts besides a
// spellcaster's spell book, no spells, and no morale, luck or cowardice.
static i8 gHeroStartingStats[HERO_CLASS_COUNT][HERO_STARTING_STAT_COUNT] = {
    {1, 2, 1, 1, 1},
    {2, 1, 1, 1, 1},
    {0, 0, 2, 3, 1},
    {0, 0, 3, 2, 1},
};

void hero::ResetToStartingState(void) {
    i32 i;

    m_experience = 0;
    m_level = 1;
    for (i = 0; i < HERO_STARTING_STAT_COUNT; i++)
        m_primaryStats[i] = gHeroStartingStats[m_heroClass][i];
    m_morale = 0;
    m_luck = 0;
    m_cowardice = 0;
    for (i = 0; i < HERO_ARTIFACT_SLOT_COUNT; i++)
        m_artifacts[i] = ARTIFACT_NONE;
    if (m_heroClass >= HERO_CLASS_FIRST_SPELLCASTER)
        m_artifacts[0] = ARTIFACT_MAGIC_BOOK;
    for (i = 0; i < HERO_SPELL_SLOT_COUNT; i++) {
        m_spells[i] = SPELL_NONE;
        m_spellCharges[i] = 0;
    }
}

i32 hero::GetExperience(i32 level) {
    i32 experience;
    i32 curStage;
    i32 incr;

    if (level <= HERO_EXPERIENCE_LEVEL_TABLE_COUNT)
        return gMinExpForLevel[m_heroClass][level - 1];
    curStage = HERO_EXPERIENCE_LEVEL_TABLE_COUNT + 1;
    incr = (gMinExpForLevel[m_heroClass][HERO_EXPERIENCE_LEVEL_TABLE_COUNT - 1]
            - gMinExpForLevel[m_heroClass][HERO_EXPERIENCE_LEVEL_TABLE_COUNT - 2])
           * 1.2;
    experience = gMinExpForLevel[m_heroClass][HERO_EXPERIENCE_LEVEL_TABLE_COUNT - 1] + incr;
    while (curStage < level) {
        incr = incr * 1.2;
        experience += incr;
        curStage++;
    }
    return experience;
}

i32 hero::GetLevel(i32 experienceValue) {
    i32 experience;
    i32 levelCounter;
    i32 growth;

    for (levelCounter = 1; levelCounter <= HERO_EXPERIENCE_LEVEL_TABLE_COUNT; levelCounter++) {
        if (experienceValue < gMinExpForLevel[m_heroClass][levelCounter - 1])
            return levelCounter - 1;
    }
    growth = (gMinExpForLevel[m_heroClass][HERO_EXPERIENCE_LEVEL_TABLE_COUNT - 1]
              - gMinExpForLevel[m_heroClass][HERO_EXPERIENCE_LEVEL_TABLE_COUNT - 2])
             * 1.2;
    experience = gMinExpForLevel[m_heroClass][HERO_EXPERIENCE_LEVEL_TABLE_COUNT - 1] + growth;
    levelCounter = HERO_EXPERIENCE_LEVEL_TABLE_COUNT + 1;
    while (experienceValue > experience) {
        growth = growth * 1.2;
        experience += growth;
        levelCounter++;
    }
    return levelCounter - 1;
}

void hero::ApplyBattleWinTemps(void) {
    if (m_cowardice < 0)
        m_cowardice++;
    ClearBattleTemps();
}

void hero::ClearBattleTemps(void) {
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

void hero::ApplyBattleLossTemps(void) {
    if (!gCombatSurrender && m_cowardice > HERO_COWARDICE_MIN)
        m_cowardice--;
    ClearBattleTemps();
}

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
    if (!gInNewGameSetup && m_owner >= 0 && gThisNetHumanPlayer[m_owner]) {
        PlayMusic(MUSIC_TRACK_LEVEL_UP);
        NormalDialog(gText, NORMAL_DIALOG_TYPE_OK, -1, -1, NORMAL_DIALOG_HERO, m_id);
        PlayMusic(TERRAIN_MUSIC_TRACK(gAdvManager->m_currentTerrain));
    }
}

i32 hero::NumArtifacts(void) {
    i32 count = 0;
    i32 i;

    for (i = 0; i < HERO_ARTIFACT_SLOT_COUNT; i++) {
        if (m_artifacts[i] >= ARTIFACT_FIRST)
            count++;
    }
    return count;
}

void UpdateHeroScreenStatusBar(i16 widgetId) {
    tag_message message;
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
            if (gInfoViewedHero->m_army.GetMorale(gInfoViewedHero, NULL) > 0)
                sprintf(gText, gHeroScreen[HERO_TEXT_GOOD_MORALE]);
            else if (gInfoViewedHero->m_army.GetMorale(gInfoViewedHero, NULL) == 0)
                sprintf(gText, gHeroScreen[HERO_TEXT_NEUTRAL_MORALE]);
            else
                sprintf(gText, gHeroScreen[HERO_TEXT_BAD_MORALE]);
            break;
        case HERO_SCREEN_LUCK_FIRST:
        case HERO_SCREEN_LUCK_FIRST + 1:
        case HERO_SCREEN_LUCK_LAST:
            if (gGame->GetLuck(gInfoViewedHero, NULL) > 0)
                sprintf(gText, gHeroScreen[HERO_TEXT_GOOD_LUCK]);
            else if (gGame->GetLuck(gInfoViewedHero, NULL) == 0)
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
            if (gHeroScreenSrcIndex == HERO_SCREEN_SOURCE_NONE) {
                if (gInfoViewedHero->m_army.m_creatureTypes[slot] != CREATURE_NONE)
                    sprintf(
                        gText,
                        gHeroScreen[HERO_TEXT_SELECT_ARMY],
                        gArmyNamesPlural[gInfoViewedHero->m_army.m_creatureTypes[slot]]
                    );
                else
                    strcpy(gText, gHeroScreen[HERO_TEXT_EMPTY]);
            } else if (gHeroScreenSrcIndex == slot) {
                sprintf(
                    gText,
                    gHeroScreen[HERO_TEXT_SELECT_ARMY],
                    gArmyNamesPlural[gInfoViewedHero->m_army.m_creatureTypes[slot]]
                );
            } else if (gTownManager->m_castleDialogActive) {
                if (gInfoViewedHero->m_army.m_creatureTypes[slot] != CREATURE_NONE)
                    sprintf(
                        gText,
                        gHeroScreen[HERO_TEXT_SELECT_ARMY],
                        gArmyNamesPlural[gInfoViewedHero->m_army.m_creatureTypes[slot]]
                    );
                else
                    strcpy(gText, gHeroScreen[HERO_TEXT_EMPTY]);
            } else if (gInfoViewedHero->m_army.m_creatureTypes[slot] == CREATURE_NONE) {
                sprintf(
                    gText,
                    gHeroScreen[HERO_TEXT_MOVE_ARMY],
                    gArmyNamesPlural[gInfoViewedHero->m_army.m_creatureTypes[gHeroScreenSrcIndex]]
                );
            } else {
                sprintf(
                    gText,
                    gHeroScreen[HERO_TEXT_EXCHANGE_ARMIES],
                    gArmyNamesPlural[gInfoViewedHero->m_army.m_creatureTypes[gHeroScreenSrcIndex]],
                    gArmyNamesPlural[gInfoViewedHero->m_army.m_creatureTypes[slot]]
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
            if (gInfoViewedHero->m_artifacts[widgetId - HERO_SCREEN_ARTIFACT_FIRST]
                == ARTIFACT_NONE)
                sprintf(gText, gHeroScreen[HERO_TEXT_EMPTY]);
            else if (gInfoViewedHero->m_artifacts[widgetId - HERO_SCREEN_ARTIFACT_FIRST]
                     == ARTIFACT_MAGIC_BOOK)
                strcpy(gText, gHeroScreen[HERO_TEXT_VIEW_SPELLS]);
            else
                sprintf(
                    gText,
                    gHeroScreen[HERO_TEXT_ARTIFACT],
                    gArtifactNames[gInfoViewedHero
                                       ->m_artifacts[widgetId - HERO_SCREEN_ARTIFACT_FIRST]]
                );
            break;
        case HERO_SCREEN_DISMISS:
            sprintf(
                gText,
                gHeroScreen[HERO_TEXT_DISMISS],
                gInfoViewedHero->m_name,
                gClassNames[gInfoViewedHero->m_heroClass]
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

i16 HeroHandler(struct tag_message& message) {
    tag_message newEvent;
    i32 unusedValue15;
    i32 unusedValue21;
    i32 unusedValue16;
    i32 unusedValue22;
    i32 curHeroLevel;
    i32 nextLevelExp;
    b8 quickViewVal;
    b8 complete = false;
    i16 slot;
    i16 temporaryVal;
    i32 curSpare;

    if (message.modifiers & MESSAGE_MODIFIER_RIGHT_BUTTON)
        quickViewVal = true;
    else
        quickViewVal = false;
    if (message.type == MESSAGE_WIDGET) {
        switch (message.command) {
            case WIDGET_COMMAND_HOVER:
                if (gWindowManager->m_lastHoverId == message.id)
                    break;
                gWindowManager->m_lastHoverId = message.id;
                UpdateHeroScreenStatusBar(message.id);
                return MESSAGE_DISPATCH_CONSUME;
            case WIDGET_NOTIFY_DESELECT:
                if (!quickViewVal) {
                    switch (message.id) {
                        case HERO_SCREEN_DISMISS:
                            if (gInfoViewedHero->Dismiss())
                                complete = true;
                            break;
                        case HERO_SCREEN_EXIT:
                            complete = true;
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
                            gGame->Overview();
                            gInfoViewedHero->RedrawHeroScreen();
                            gWindowManager->FadeScreen(WINDOW_FADE_IN, 8, NULL);
                        }
                        break;
                    case HERO_SCREEN_ATTACK:
                    case HERO_SCREEN_DEFENSE:
                    case HERO_SCREEN_SPELL_POWER:
                    case HERO_SCREEN_KNOWLEDGE:
                        gInfoViewedHero->ViewStat(
                            message.id - HERO_SCREEN_STAT_FIRST,
                            quickViewVal
                        );
                        break;
                    case HERO_SCREEN_MORALE_FIRST:
                    case HERO_SCREEN_MORALE_FIRST + 1:
                    case HERO_SCREEN_MORALE_LAST:
                        gGame->ShowMoraleInfo(
                            gInfoViewedHero,
                            quickViewVal == false ? NORMAL_DIALOG_TYPE_OK
                                                  : NORMAL_DIALOG_TYPE_QUICK_VIEW
                        );
                        break;
                    case HERO_SCREEN_LUCK_FIRST:
                    case HERO_SCREEN_LUCK_FIRST + 1:
                    case HERO_SCREEN_LUCK_LAST:
                        gGame->ShowLuckInfo(
                            gInfoViewedHero,
                            quickViewVal == false ? NORMAL_DIALOG_TYPE_OK
                                                  : NORMAL_DIALOG_TYPE_QUICK_VIEW
                        );
                        break;
                    case HERO_SCREEN_EXPERIENCE_ICON:
                    case HERO_SCREEN_EXPERIENCE:
                        curHeroLevel = gInfoViewedHero->GetLevel(gInfoViewedHero->m_experience);
                        nextLevelExp = gInfoViewedHero->GetExperience(curHeroLevel + 1);
                        // Beyond the experience table GetLevel needs more than the
                        // threshold, so show the first value that reaches the level.
                        if (curHeroLevel >= HERO_EXPERIENCE_LEVEL_TABLE_COUNT)
                            nextLevelExp++;
                        sprintf(
                            gText,
                            localization::Tr("hero.experience.details"),
                            curHeroLevel,
                            gInfoViewedHero->m_experience,
                            nextLevelExp
                        );
                        NormalDialog(
                            gText,
                            quickViewVal == false ? NORMAL_DIALOG_TYPE_OK
                                                  : NORMAL_DIALOG_TYPE_QUICK_VIEW
                        );
                        break;
                    case HERO_SCREEN_ARMY_SLOT_FIRST:
                    case HERO_SCREEN_ARMY_SLOT_FIRST + 1:
                    case HERO_SCREEN_ARMY_SLOT_FIRST + 2:
                    case HERO_SCREEN_ARMY_SLOT_FIRST + 3:
                    case HERO_SCREEN_ARMY_SLOT_FIRST + 4:
                        slot = message.id - HERO_SCREEN_ARMY_SLOT_FIRST;
                        if (!quickViewVal && gHeroScreenSrcIndex == HERO_SCREEN_SOURCE_NONE) {
                            if (gInfoViewedHero->m_army.m_creatureTypes[slot] != CREATURE_NONE) {
                                gHeroScreenSrcIndex = slot;
                                gInfoViewedHero->HeroScreenUpdate();
                            }
                        } else if ((quickViewVal
                                    && gInfoViewedHero->m_army.m_creatureTypes[slot]
                                           != CREATURE_NONE)
                                   || (!quickViewVal
                                       && gHeroScreenSrcIndex
                                              == message.id - HERO_SCREEN_ARMY_SLOT_FIRST)) {
                            gGame->ViewArmy(
                                119,
                                20,
                                gInfoViewedHero->m_army.m_creatureTypes[slot],
                                gInfoViewedHero->m_army.m_creatureCounts[slot],
                                NULL,
                                quickViewVal || gTownManager->m_castleDialogActive == true
                                    || gInfoViewedHero->m_army.GetNumArmies() == 1,
                                ARMY_FACING_RIGHT,
                                quickViewVal,
                                gInfoViewedHero,
                                NULL,
                                &gInfoViewedHero->m_army
                            );
                            if (!quickViewVal)
                                gHeroScreenSrcIndex = HERO_SCREEN_SOURCE_NONE;
                            gInfoViewedHero->HeroScreenUpdate();
                        } else if (!quickViewVal && gTownManager->m_castleDialogActive) {
                            if (gInfoViewedHero->m_army.m_creatureTypes[slot] != CREATURE_NONE) {
                                gHeroScreenSrcIndex = slot;
                                gInfoViewedHero->HeroScreenUpdate();
                            }
                        } else if (!quickViewVal) {
                            temporaryVal = gInfoViewedHero->m_army.m_creatureTypes[slot];
                            gInfoViewedHero->m_army.m_creatureTypes[slot] =
                                gInfoViewedHero->m_army.m_creatureTypes[gHeroScreenSrcIndex];
                            gInfoViewedHero->m_army.m_creatureTypes[gHeroScreenSrcIndex] =
                                temporaryVal;
                            temporaryVal = gInfoViewedHero->m_army.m_creatureCounts[slot];
                            gInfoViewedHero->m_army.m_creatureCounts[slot] =
                                gInfoViewedHero->m_army.m_creatureCounts[gHeroScreenSrcIndex];
                            gInfoViewedHero->m_army.m_creatureCounts[gHeroScreenSrcIndex] =
                                temporaryVal;
                            gHeroScreenSrcIndex = HERO_SCREEN_SOURCE_NONE;
                            gInfoViewedHero->HeroScreenUpdate();
                        }
                        if (!quickViewVal) {
                            gWindowManager->m_lastHoverId = WINDOW_MANAGER_NO_HOVER_WIDGET;
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
                        if (gInfoViewedHero->m_artifacts[message.id - HERO_SCREEN_ARTIFACT_FIRST]
                            != ARTIFACT_NONE) {
                            if (!quickViewVal
                                && gInfoViewedHero
                                           ->m_artifacts[message.id - HERO_SCREEN_ARTIFACT_FIRST]
                                       == ARTIFACT_MAGIC_BOOK)
                                gGame->ViewSpells(
                                    gInfoViewedHero,
                                    SPELL_TYPE_ALL,
                                    ViewSpecialHandler,
                                    1
                                );
                            else
                                gInfoViewedHero->ViewArtifact(
                                    gInfoViewedHero
                                        ->m_artifacts[message.id - HERO_SCREEN_ARTIFACT_FIRST],
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

class heroWindow* gHeroWin = NULL;

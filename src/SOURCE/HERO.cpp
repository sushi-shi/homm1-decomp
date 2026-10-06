#include <match.h>

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

VA(0x00438f20, 0x5d)
hero::hero(void) {
    m_id = 0;
    m_owner = 0;
    m_x = 0;
    m_y = 0;
    m_heroClass = 0;
    m_portrait = 0;
    m_name[0] = 0;
    heroWin = NULL;
    gHeroScreenSrcIndex = HERO_SCREEN_SOURCE_NONE;
}

// @dead-code
// Zero-ref: no incoming call, jump or relocated reference in retail.
VA(0x00438f7d, 0xd)
void hero::GetArmyStrengths(u32* const) {}

VA(0x00438f8a, 0x4b)
i8 hero::HasArtifact(H1_ENUM_PARAM(ArtifactType, i8) artifact) {
    i16 i;

    for (i = 0; i < HERO_ARTIFACT_SLOT_COUNT; i++) {
        if (m_artifacts[i] == artifact)
            return 1;
    }
    return 0;
}

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
    H1_ENUM_LOCAL(CreatureSpeed, i16) slowestSpeedValue;
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
                && H1_ENUM_DECODE(
                       CreatureSpeed,
                       gMonsterDatabase[m_army.m_creatureTypes[creatureIndex]].stats.speed
                   ) < slowestSpeedValue)
                slowestSpeedValue = H1_ENUM_DECODE(
                    CreatureSpeed,
                    gMonsterDatabase[m_army.m_creatureTypes[creatureIndex]].stats.speed
                );
        }
        movePoints = mobilityTable[H1_ENUM_ENCODE(CreatureSpeed, slowestSpeedValue) - 1];
        if (HasArtifact(ARTIFACT_NOMAD_BOOTS))
            movePoints += nomadBootsBonus;
        if (HasArtifact(ARTIFACT_TRAVELER_BOOTS))
            movePoints += travelerBonus;
    }
    if (HasArtifact(ARTIFACT_TRUE_COMPASS))
        movePoints += compassMobility;
    if (m_owner >= 0 && !gHumanPlayer[m_owner]
        && H1_ENUM_DECODE(ComputerPlayerType, gGame->m_players[m_owner].m_difficulty)
               >= PLAYER_TYPE_MOBILITY_BONUS_FIRST)
        movePoints += 3;
    return movePoints;
}

VA(0x00439192, 0x41)
i8 hero::HasSpell(H1_ENUM_PARAM(SpellType, i8) spell) {
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
void hero::UseSpell(H1_ENUM_PARAM(SpellType, i8) spell) {
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

VA(0x0043949f, 0x1a7)
i32 hero::AddSpell(H1_ENUM_PARAM(SpellType, i8) spell, i8 charges, i32 checkOnly) {
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
                m_spellCharges[i] = charges;
                break;
            }
        }
    }
done:
    return added;
}

VA(0x00439646, 0x48)
void hero::RedrawHeroScreen(void) {
    gResourceManager->GetBackdrop("heroscrn.bmp", gWindowManager->m_screen);
    heroWin->DrawWindow();
    gWindowManager->UpdateScreenRegion(0, 0, LOGICAL_SCREEN_WIDTH, LOGICAL_SCREEN_HEIGHT);
}

VA(0x0043968e, 0x64a)
i8 hero::HeroView(i8 viewOnly) {
    i32 heroLuck;
    i32 moraleValue;
    tag_message message;
    i16 i;
    i32 magnitude;

    gAdvManager->TrimLoopingSounds(8);
    gHeroWindShowing = 1;
    gWindowManager->FadeScreen(WINDOW_FADE_OUT, 8, NULL);
    heroWin = new heroWindow(0, 0, "herowind.bin");
    if (!heroWin)
        MemError();
    gHeroWin = heroWin;
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
    if (viewOnly || gTownManager->m_castleDialogActive
        || (!gCurPlayerData->m_townCount && gCurPlayerData->m_heroCount == 1)) {
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
        sprintf(gText, "%d", m_primaryStats[H1_ENUM_DECODE(HeroPrimaryStat, i)]);
        message.id = i + HERO_SCREEN_STAT_VALUE_FIRST;
        message.text = gText;
        heroWin->BroadcastMessage(message);
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
    sprintf(
        gText,
        "crst%04d.icn",
        H1_ENUM_ENCODE(PlayerColor, gCurPlayerData->Color()) * 4 + m_heroClass
    );
    message.command = WIDGET_COMMAND_SET_ICON;
    message.id = HERO_SCREEN_CREST;
    heroWin->BroadcastMessage(message);
    UpdateArmies();
    for (i = 0; i < HERO_ARTIFACT_SLOT_COUNT; i++) {
        message.id = i + HERO_SCREEN_ARTIFACT_FIRST;
        if (m_artifacts[i] != ARTIFACT_NONE) {
            message.command = WIDGET_COMMAND_SET_FRAME;
            message.value = H1_ENUM_ENCODE(ArtifactType, m_artifacts[i]);
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
    gWindowManager->FadeScreen(WINDOW_FADE_IN, 8, NULL);
    gInfoViewedHero = this;
    gWindowManager->DoDialog(heroWin, HeroHandler, 0);
    gWindowManager->FadeScreen(WINDOW_FADE_OUT, 8, NULL);
    delete heroWin;
    gHeroWin = NULL;
    if (gWindowManager->m_dialogResult == HERO_SCREEN_DISMISS) {
        return 1;
    } else {
        m_mobility = CalcMobility();
        if (m_remainingMobility > m_mobility)
            m_remainingMobility = m_mobility;
    }
    gHeroWindShowing = 0;
    return 0;
}

VA(0x00439cd8, 0x6c)
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

VA(0x00439d44, 0x99)
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
        heroWin->BroadcastMessage(message);
    }
    heroWin->DrawWindow();
    gWindowManager->UpdateScreenRegion(0, 0, LOGICAL_SCREEN_WIDTH, LOGICAL_SCREEN_HEIGHT);
}

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
            message.value = CREATURE_FACTION(m_army.m_creatureTypes[i])
                            + HERO_ARMY_BACKGROUND_FACTION_FIRST_FRAME;
            heroWin->BroadcastMessage(message);
            message.id = i + HERO_SCREEN_ARMY_CREATURE_FIRST;
            message.value = H1_ENUM_ENCODE(CreatureType, m_army.m_creatureTypes[i]);
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
    gWindowManager->DoDialog(win, TrueFalseDialogHandler, 0);
    delete win;
}

VA(0x0043a114, 0x3e)
void hero::ViewArtifact(H1_ENUM_PARAM(ArtifactType, i8) artifact, i8 quickView) {
    NormalDialog(
        gArtifactDesc[artifact],
        quickView == 0 ? NORMAL_DIALOG_TYPE_OK : NORMAL_DIALOG_TYPE_QUICK_VIEW,
        -1,
        0x1c
    );
}

VA(0x0043a152, 0x47)
i8 hero::Dismiss(void) {
    NormalDialog(localization::Tr("hero.dismiss.confirm"), NORMAL_DIALOG_TYPE_YES_NO, 0xb1, 0x1c);
    if (gWindowManager->m_dialogResult == NORMAL_DIALOG_CONFIRM) {
        Deallocate();
        return 1;
    }
    return 0;
}

VA(0x0043a199, 0x400)
void hero::Deallocate(void) {
    i32 oldOwner;
    i16 i;
    playerData* playerPtr;
    i8 heroNum;
    town* curTown;
    i32 availSlot;

    oldOwner = m_owner;
    playerPtr = &gGame->m_players[m_owner];
    gAdvManager->MobilizeCurrHero(0);
    gAdvManager->HideRoute(0, 0, 0);
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
            gAdvManager->m_cursorActive = 0;
            gGame->m_map[m_x][m_y].m_flags &= ~MAP_CELL_HERO_CURSOR;
        }
        if (oldOwner == gCurPlayer)
            gAdvManager->m_heroContextLocked = 0;
    }
    playerPtr->m_heroCount--;
    playerPtr->m_heroLocatorPage = 0;
    gGame->m_availableHeroes[m_id] = HERO_AVAILABILITY_UNAVAILABLE;
    if (gRetreatWin) {
        availSlot = Random(0, HERO_AVAILABLE_SLOT_COUNT - 1);
        if (gGame->m_availableHeroes[gGame->m_players[m_owner].m_availableHeroIds[availSlot]]
            == HERO_AVAILABILITY_RETREATED)
            gGame->m_availableHeroes[gGame->m_players[m_owner].m_availableHeroIds[availSlot]] =
                HERO_AVAILABILITY_UNAVAILABLE;
        gGame->m_players[m_owner].m_availableHeroIds[availSlot] = m_id;
        gGame->m_availableHeroes[m_id] = HERO_AVAILABILITY_RETREATED;
    }
    m_owner = GAME_PLAYER_NONE;
    m_destinationX = m_destinationY = HERO_DESTINATION_NONE;
    if (!gCombatSurrender)
        gGame->SetRandomHeroArmies(m_id, RANDOM_HERO_NORMAL_ARMY);
    CheckEndGame(0);
}

VA(0x0043a599, 0xb5)
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

VA(0x0043a64e, 0xd7)
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

VA(0x0043a8b1, 0x2b7)
void hero::CheckLevel(void) {
    i32 oldLvl;
    // Counts the gained levels, then indexes the primary stats.
    H1_ENUM_SHARED(HeroPrimaryStat, i32) i;
    H1_ENUM_ARRAY(i32, stats, HeroPrimaryStat, HERO_PRIMARY_STAT_COUNT);
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

VA(0x0043ab68, 0x4b)
i32 hero::NumArtifacts(void) {
    i32 count = 0;
    i32 i;

    for (i = 0; i < HERO_ARTIFACT_SLOT_COUNT; i++) {
        if (m_artifacts[i] >= ARTIFACT_FIRST)
            count++;
    }
    return count;
}

// Buka names every army slot with the plural creature table.
VA(0x0043abb3, 0x502)
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

VA(0x0043b0b5, 0x610)
H1_ENUM_RETURN(MessageDispatchResult, i16) HeroHandler(struct tag_message& message) {
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
    // Integer storage that swaps a slot's creature type, then its count.
    i16 temporaryVal;
    i32 curSpare;

    if (message.modifiers & MESSAGE_MODIFIER_RIGHT_BUTTON)
        quickViewVal = 1;
    else
        quickViewVal = 0;
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
                            quickViewVal == 0 ? NORMAL_DIALOG_TYPE_OK
                                              : NORMAL_DIALOG_TYPE_QUICK_VIEW
                        );
                        break;
                    case HERO_SCREEN_LUCK_FIRST:
                    case HERO_SCREEN_LUCK_FIRST + 1:
                    case HERO_SCREEN_LUCK_LAST:
                        gGame->ShowLuckInfo(
                            gInfoViewedHero,
                            quickViewVal == 0 ? NORMAL_DIALOG_TYPE_OK
                                              : NORMAL_DIALOG_TYPE_QUICK_VIEW
                        );
                        break;
                    case HERO_SCREEN_EXPERIENCE_ICON:
                    case HERO_SCREEN_EXPERIENCE:
                        curHeroLevel = gInfoViewedHero->GetLevel(gInfoViewedHero->m_experience);
                        nextLevelExp = gInfoViewedHero->GetExperience(curHeroLevel + 1);
                        sprintf(
                            gText,
                            localization::Tr("hero.experience.details"),
                            curHeroLevel,
                            gInfoViewedHero->m_experience,
                            nextLevelExp
                        );
                        NormalDialog(
                            gText,
                            quickViewVal == 0 ? NORMAL_DIALOG_TYPE_OK
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
                                quickViewVal || gTownManager->m_castleDialogActive == 1
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
                            temporaryVal = H1_ENUM_ENCODE(
                                CreatureType,
                                gInfoViewedHero->m_army.m_creatureTypes[slot]
                            );
                            gInfoViewedHero->m_army.m_creatureTypes[slot] =
                                gInfoViewedHero->m_army.m_creatureTypes[gHeroScreenSrcIndex];
                            gInfoViewedHero->m_army.m_creatureTypes[gHeroScreenSrcIndex] =
                                H1_ENUM_DECODE(CreatureType, temporaryVal);
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

// HERO owns retail .data 0x004a12bc-0x004a0b2b.
DATA(0x004a6c3c)
class heroWindow* gHeroWin = NULL;

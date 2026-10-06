#include <match.h>

#include <SOURCE/EVENTS.h>

#include <BASE/audio.h>
#include <BASE/bmap2.h>
#include <BASE/executive.h>
#include <BASE/heroWindow.h>
#include <BASE/heroWindowManager.h>
#include <BASE/inputManager.h>
#include <BASE/message.h>
#include <BASE/Misc.h>
#include <BASE/miscwin.h>
#include <BASE/mouseManager.h>
#include <BASE/resource.h>
#include <BASE/sample.h>
#include <SOURCE/advManager.h>
#include <SOURCE/armyGroup.h>
#include <SOURCE/campaignTypes.h>
#include <SOURCE/combatManager.h>
#include <SOURCE/dialogTypes.h>
#include <SOURCE/game.h>
#include <SOURCE/hero.h>
#include <SOURCE/KB.h>
#include <SOURCE/kbwin.h>
#include <SOURCE/mapCell.h>
#include <SOURCE/mapObjectTypes.h>
#include <SOURCE/philAI.h>
#include <SOURCE/playerData.h>
#include <SOURCE/recruitUnit.h>
#include <SOURCE/REMOTE.h>
#include <SOURCE/searchArray.h>
#include <SOURCE/swapManager.h>
#include <SOURCE/town.h>
#include <SOURCE/townManager.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

VA(0x00424a50, 0x1d9d)
void advManager::DoEvent(class mapCell* cell, i32 x, i32 y) {
    hero* visitingHero;
    tag_message widgetEvent;
    i8 eventKind;
    tag_message unusedMessage;
    i8 removeObj;
    i32 fizzleEffect;
    i32 artifactId;
    i32 income;
    hero* opponent;
    i32 res;
    i8 teleX;
    i8 teleY;
    char resourceText[20];
    i8 portalCount;
    i32 eventResource;
    heroWindow* thiefWindow;
    boatRecord* boat;
    i8 guardMonster;
    mapCell* previousCell;
    town* theirTown;
    i32 numTroops;

    visitingHero = gGame->GetHero(gCurPlayerData->m_currentHero);
    eventKind = cell->m_triggerType & MAP_TRIGGER_TYPE_MASK;
    removeObj = 0;
    fizzleEffect = EVENT_FIZZLE_HERO_LOSS;
    gEventMusicPlaying = 1;
    gMouseManager->ReallyHidePointer();
    EventSound(eventKind, cell->m_objectMetadata);
    switch (eventKind) {
        case MAP_OBJECT_COAST:
            if (visitingHero->IsEmbarked()) {
                visitingHero->m_eventFlags &= ~HERO_EVENT_EMBARKED;
                visitingHero->m_remainingMobility = 0;
                visitingHero->m_direction = m_cursorDirection;
                m_cursorType = visitingHero->m_heroClass;
                m_cursorFrame = GetCursorBaseFrame(m_cursorDirection);
                m_cursorActive = 1;
                gWindowManager->SaveFizzleSource(
                    COAST_FIZZLE_X,
                    COAST_FIZZLE_Y,
                    COAST_FIZZLE_WIDTH,
                    COAST_FIZZLE_HEIGHT
                );
                CompleteDraw(m_mapOriginX, m_mapOriginY, 0);
                gWindowManager->FizzleForward(
                    COAST_FIZZLE_X,
                    COAST_FIZZLE_Y,
                    COAST_FIZZLE_WIDTH,
                    COAST_FIZZLE_HEIGHT,
                    FIZZLE_USE_DEFAULT_DELAY
                );
                CheckAdjacentMon(&guardMonster);
            }
            break;
        case MAP_OBJECT_SHIP:
            boat = &gGame->m_boats[cell->m_objectMetadata];
            gGame->RestoreCell(-1, -1, boat->savedTriggerType, boat->savedEventData, cell, 2);
            visitingHero->m_eventFlags |= HERO_EVENT_EMBARKED;
            visitingHero->m_remainingMobility = 0;
            boat->heroId = visitingHero->m_id;
            boat->owner = visitingHero->m_owner;
            m_cursorType = ADVMGR_HERO_ICON_BOAT;
            m_cursorDirection = boat->direction;
            m_cursorFrame = GetCursorBaseFrame(m_cursorDirection);
            m_cursorActive = 1;
            CompleteDraw(m_mapOriginX, m_mapOriginY, 0);
            UpdateScreen(0, 0);
            break;
        case MAP_OBJECT_MINE:
            if (gGame->m_mineOwners[cell->m_objectMetadata] == gCurPlayer)
                break;
            if (gGame->m_mines[cell->m_objectMetadata].type == RESOURCE_GOLD)
                income = MINE_GOLD_INCOME;
            else if (gGame->m_mines[cell->m_objectMetadata].type == RESOURCE_ORE)
                income = MINE_ORE_INCOME;
            else
                income = MINE_RARE_INCOME;
            EventWindow(
                gGame->m_mines[cell->m_objectMetadata].type + EVENT_TEXT_MINE_CAPTURED_BASE,
                NORMAL_DIALOG_TYPE_OK,
                "",
                gGame->m_mines[cell->m_objectMetadata].type,
                -income,
                NORMAL_DIALOG_NO_RESOURCE,
                0,
                NORMAL_DIALOG_NO_OR_TEXT
            );
            goto claimMine;
        case MAP_OBJECT_ALCHEMIST_LAB:
            if (gGame->m_mineOwners[cell->m_objectMetadata] == gCurPlayer)
                break;
            EventWindow(
                EVENT_TEXT_ALCHEMIST_CAPTURED,
                NORMAL_DIALOG_TYPE_OK,
                "",
                NORMAL_DIALOG_RESOURCE_MERCURY,
                -ALCHEMIST_MERCURY_INCOME,
                NORMAL_DIALOG_NO_RESOURCE,
                0,
                NORMAL_DIALOG_NO_OR_TEXT
            );
            goto claimMine;
        case MAP_OBJECT_SAWMILL:
            if (gGame->m_mineOwners[cell->m_objectMetadata] == gCurPlayer)
                break;
            EventWindow(
                EVENT_TEXT_SAWMILL_CAPTURED,
                NORMAL_DIALOG_TYPE_OK,
                "",
                NORMAL_DIALOG_RESOURCE_WOOD,
                -SAWMILL_WOOD_INCOME,
                NORMAL_DIALOG_NO_RESOURCE,
                0,
                NORMAL_DIALOG_NO_OR_TEXT
            );
            goto claimMine;
        claimMine:
            gGame->ClaimMine(cell->m_objectMetadata, gCurPlayer);
            break;
        case MAP_OBJECT_LIGHTHOUSE:
            if (gGame->m_mineOwners[MINE_SLOT_LIGHTHOUSE] == gCurPlayer)
                break;
            gGame->ClaimMine(MINE_SLOT_LIGHTHOUSE, gCurPlayer);
            EventWindow(
                EVENT_TEXT_LIGHTHOUSE_CAPTURED,
                NORMAL_DIALOG_TYPE_OK,
                "",
                NORMAL_DIALOG_NO_RESOURCE,
                0,
                NORMAL_DIALOG_NO_RESOURCE,
                0,
                NORMAL_DIALOG_NO_OR_TEXT
            );
            break;
        case MAP_OBJECT_DRAGON_CITY:
            if (gGame->m_mineOwners[MINE_SLOT_DRAGON_CITY] == gCurPlayer)
                break;
            EventWindow(
                EVENT_TEXT_DRAGON_CITY_PROMPT,
                NORMAL_DIALOG_TYPE_YES_NO,
                "",
                NORMAL_DIALOG_NO_RESOURCE,
                0,
                NORMAL_DIALOG_NO_RESOURCE,
                0,
                NORMAL_DIALOG_NO_OR_TEXT
            );
            if (gWindowManager->m_dialogResult == NORMAL_DIALOG_CONFIRM) {
                if (gGame->m_campaignType > 0
                    && gGame->m_campaignScenario == CAMPAIGN_SCENARIO_DRAGON_CITY)
                    numTroops = DRAGON_CITY_CAMPAIGN_DRAGON_COUNT;
                else
                    numTroops = DRAGON_CITY_DRAGON_COUNT;
                if (CombatMonsterEvent(
                        visitingHero,
                        CREATURE_DRAGON,
                        numTroops,
                        cell,
                        x,
                        y,
                        0,
                        x,
                        y
                    )
                    == COMBAT_RESULT_ATTACKER) {
                    gGame->ClaimMine(MINE_SLOT_DRAGON_CITY, gCurPlayer);
                    EventWindow(
                        EVENT_TEXT_DRAGON_CITY_CONQUERED,
                        NORMAL_DIALOG_TYPE_OK,
                        "",
                        NORMAL_DIALOG_RESOURCE_GOLD,
                        -DRAGON_CITY_GOLD_INCOME,
                        NORMAL_DIALOG_NO_RESOURCE,
                        0,
                        NORMAL_DIALOG_NO_OR_TEXT
                    );
                    break;
                }
                visitingHero->CheckLevel();
            }
            break;
        case MAP_OBJECT_TREASURE_CHEST:
            EventWindow(
                EVENT_TEXT_TREASURE_CHEST,
                NORMAL_DIALOG_TYPE_YES_NO,
                "",
                NORMAL_DIALOG_RESOURCE_GOLD,
                cell->m_objectMetadata * CHEST_GOLD_MULTIPLIER,
                NORMAL_DIALOG_EXPERIENCE,
                (cell->m_objectMetadata - CHEST_EXPERIENCE_LEVEL_OFFSET)
                    * CHEST_EXPERIENCE_MULTIPLIER,
                NORMAL_DIALOG_SHOW_OR_TEXT
            );
            if (gWindowManager->m_dialogResult == NORMAL_DIALOG_CONFIRM)
                GiveResource(
                    visitingHero,
                    RESOURCE_GOLD,
                    cell->m_objectMetadata * CHEST_GOLD_MULTIPLIER
                );
            else
                GiveExperience(
                    visitingHero,
                    (cell->m_objectMetadata - CHEST_EXPERIENCE_LEVEL_OFFSET)
                        * CHEST_EXPERIENCE_MULTIPLIER,
                    0
                );
            removeObj = 1;
            fizzleEffect = EVENT_FIZZLE_PICKUP;
            visitingHero->CheckLevel();
            break;
        case MAP_OBJECT_BUOY:
            if (visitingHero->m_eventFlags & HERO_EVENT_BUOY) {
                EventWindow(
                    EVENT_TEXT_BUOY_VISITED,
                    NORMAL_DIALOG_TYPE_OK,
                    "",
                    NORMAL_DIALOG_NO_RESOURCE,
                    0,
                    NORMAL_DIALOG_NO_RESOURCE,
                    0,
                    NORMAL_DIALOG_NO_OR_TEXT
                );
            } else {
                visitingHero->m_eventFlags |= HERO_EVENT_BUOY;
                visitingHero->m_morale++;
                EventWindow(
                    EVENT_TEXT_BUOY_REWARD,
                    NORMAL_DIALOG_TYPE_OK,
                    "",
                    NORMAL_DIALOG_MORALE_BONUS,
                    0,
                    NORMAL_DIALOG_NO_RESOURCE,
                    0,
                    NORMAL_DIALOG_NO_OR_TEXT
                );
            }
            break;
        case MAP_OBJECT_FAERIE_RING:
            if (visitingHero->m_eventFlags & HERO_EVENT_FAERIE_RING) {
                EventWindow(
                    EVENT_TEXT_FAERIE_RING_VISITED,
                    NORMAL_DIALOG_TYPE_OK,
                    "",
                    NORMAL_DIALOG_NO_RESOURCE,
                    0,
                    NORMAL_DIALOG_NO_RESOURCE,
                    0,
                    NORMAL_DIALOG_NO_OR_TEXT
                );
            } else {
                visitingHero->m_eventFlags |= HERO_EVENT_FAERIE_RING;
                visitingHero->m_luck++;
                EventWindow(
                    EVENT_TEXT_FAERIE_RING_REWARD,
                    NORMAL_DIALOG_TYPE_OK,
                    "",
                    NORMAL_DIALOG_LUCK_BONUS,
                    0,
                    NORMAL_DIALOG_NO_RESOURCE,
                    0,
                    NORMAL_DIALOG_NO_OR_TEXT
                );
            }
            break;
        case MAP_OBJECT_FOUNTAIN:
            if (visitingHero->m_eventFlags & HERO_EVENT_FOUNTAIN) {
                EventWindow(
                    EVENT_TEXT_FOUNTAIN_VISITED,
                    NORMAL_DIALOG_TYPE_OK,
                    "",
                    NORMAL_DIALOG_NO_RESOURCE,
                    0,
                    NORMAL_DIALOG_NO_RESOURCE,
                    0,
                    NORMAL_DIALOG_NO_OR_TEXT
                );
            } else {
                visitingHero->m_eventFlags |= HERO_EVENT_FOUNTAIN;
                visitingHero->m_luck++;
                EventWindow(
                    EVENT_TEXT_FOUNTAIN_REWARD,
                    NORMAL_DIALOG_TYPE_OK,
                    "",
                    NORMAL_DIALOG_LUCK_BONUS,
                    0,
                    NORMAL_DIALOG_NO_RESOURCE,
                    0,
                    NORMAL_DIALOG_NO_OR_TEXT
                );
            }
            break;
        case MAP_OBJECT_OASIS:
            if (visitingHero->m_eventFlags & HERO_EVENT_OASIS) {
                EventWindow(
                    EVENT_TEXT_OASIS_VISITED,
                    NORMAL_DIALOG_TYPE_OK,
                    "",
                    NORMAL_DIALOG_NO_RESOURCE,
                    0,
                    NORMAL_DIALOG_NO_RESOURCE,
                    0,
                    NORMAL_DIALOG_NO_OR_TEXT
                );
            } else {
                visitingHero->m_eventFlags |= HERO_EVENT_OASIS;
                visitingHero->m_morale++;
                EventWindow(
                    EVENT_TEXT_OASIS_REWARD,
                    NORMAL_DIALOG_TYPE_OK,
                    "",
                    NORMAL_DIALOG_MORALE_BONUS,
                    0,
                    NORMAL_DIALOG_NO_RESOURCE,
                    0,
                    NORMAL_DIALOG_NO_OR_TEXT
                );
            }
            break;
        case MAP_OBJECT_STATUE:
            if (visitingHero->m_eventFlags & HERO_EVENT_STATUE) {
                EventWindow(
                    EVENT_TEXT_STATUE_VISITED,
                    NORMAL_DIALOG_TYPE_OK,
                    "",
                    NORMAL_DIALOG_NO_RESOURCE,
                    0,
                    NORMAL_DIALOG_NO_RESOURCE,
                    0,
                    NORMAL_DIALOG_NO_OR_TEXT
                );
            } else {
                visitingHero->m_eventFlags |= HERO_EVENT_STATUE;
                visitingHero->m_morale += TEMPLE_MORALE_BONUS;
                EventWindow(
                    EVENT_TEXT_STATUE_REWARD,
                    NORMAL_DIALOG_TYPE_OK,
                    "",
                    NORMAL_DIALOG_MORALE_BONUS,
                    0,
                    NORMAL_DIALOG_MORALE_BONUS,
                    0,
                    NORMAL_DIALOG_NO_OR_TEXT
                );
            }
            break;
        case MAP_OBJECT_SKELETON:
            switch (cell->m_objectMetadata) {
                case SKELETON_EMPTY:
                    EventWindow(
                        EVENT_TEXT_SKELETON_EMPTY,
                        NORMAL_DIALOG_TYPE_OK,
                        "",
                        NORMAL_DIALOG_NO_RESOURCE,
                        0,
                        NORMAL_DIALOG_NO_RESOURCE,
                        0,
                        NORMAL_DIALOG_NO_OR_TEXT
                    );
                    break;
                case SKELETON_ARTIFACT:
                    if (visitingHero->NumArtifacts() == HERO_ARTIFACT_SLOT_COUNT) {
                        sprintf(gText, "%s.", localization::Tr("event.skeleton.treasure"));
                        EventWindow(
                            EVENT_TEXT_CUSTOM,
                            NORMAL_DIALOG_TYPE_OK,
                            gText,
                            NORMAL_DIALOG_RESOURCE_GOLD,
                            SKELETON_GOLD,
                            NORMAL_DIALOG_NO_RESOURCE,
                            0,
                            NORMAL_DIALOG_NO_OR_TEXT
                        );
                    } else {
                        artifactId = GiveRandomArtifact(visitingHero);
                        sprintf(
                            gText,
                            "%s %s",
                            gEventText[EVENT_TEXT_SKELETON_ARTIFACT],
                            gArtifactNames[artifactId]
                        );
                        EventWindow(
                            EVENT_TEXT_CUSTOM,
                            NORMAL_DIALOG_TYPE_OK,
                            gText,
                            NORMAL_DIALOG_ARTIFACT,
                            artifactId,
                            NORMAL_DIALOG_NO_RESOURCE,
                            0,
                            NORMAL_DIALOG_NO_OR_TEXT
                        );
                    }
                    cell->m_objectMetadata = SKELETON_EMPTY;
                    break;
            }
            break;
        case MAP_OBJECT_CAMPFIRE:
            EventWindow(
                EVENT_TEXT_CAMPFIRE,
                NORMAL_DIALOG_TYPE_OK,
                "",
                NORMAL_DIALOG_RESOURCE_GOLD,
                (cell->m_objectMetadata >> CAMPFIRE_AMOUNT_SHIFT) * CAMPFIRE_GOLD_MULTIPLIER,
                cell->m_objectMetadata & CAMPFIRE_RESOURCE_MASK,
                cell->m_objectMetadata >> CAMPFIRE_AMOUNT_SHIFT,
                NORMAL_DIALOG_NO_OR_TEXT
            );
            GiveResource(
                visitingHero,
                RESOURCE_GOLD,
                (cell->m_objectMetadata >> CAMPFIRE_AMOUNT_SHIFT) * CAMPFIRE_GOLD_MULTIPLIER
            );
            GiveResource(
                visitingHero,
                cell->m_objectMetadata & CAMPFIRE_RESOURCE_MASK,
                cell->m_objectMetadata >> CAMPFIRE_AMOUNT_SHIFT
            );
            removeObj = 1;
            fizzleEffect = EVENT_FIZZLE_PICKUP;
            gGame->m_mapSounds[m_mapOriginX + ADVMGR_VIEW_CENTER]
                              [m_mapOriginY + ADVMGR_VIEW_CENTER] = MAP_SOUND_NONE;
            SetEnvironmentOrigin(
                m_mapOriginX + ADVMGR_VIEW_CENTER,
                m_mapOriginY + ADVMGR_VIEW_CENTER,
                1
            );
            break;
        case MAP_OBJECT_GAZEBO:
            if (visitingHero->m_visitedSites & (1 << cell->m_objectMetadata)) {
                EventWindow(
                    EVENT_TEXT_GAZEBO_VISITED,
                    NORMAL_DIALOG_TYPE_OK,
                    "",
                    NORMAL_DIALOG_NO_RESOURCE,
                    0,
                    NORMAL_DIALOG_NO_RESOURCE,
                    0,
                    NORMAL_DIALOG_NO_OR_TEXT
                );
            } else {
                EventWindow(
                    EVENT_TEXT_GAZEBO_REWARD,
                    NORMAL_DIALOG_TYPE_OK,
                    "",
                    NORMAL_DIALOG_EXPERIENCE,
                    GAZEBO_EXPERIENCE,
                    NORMAL_DIALOG_NO_RESOURCE,
                    0,
                    NORMAL_DIALOG_NO_OR_TEXT
                );
                GiveExperience(visitingHero, GAZEBO_EXPERIENCE, 0);
                visitingHero->m_visitedSites |= 1 << cell->m_objectMetadata;
                visitingHero->CheckLevel();
            }
            break;
        case MAP_OBJECT_WATERWHEEL:
            if (!cell->m_objectMetadata) {
                EventWindow(
                    EVENT_TEXT_WATERWHEEL_EMPTY,
                    NORMAL_DIALOG_TYPE_OK,
                    "",
                    NORMAL_DIALOG_NO_RESOURCE,
                    0,
                    NORMAL_DIALOG_NO_RESOURCE,
                    0,
                    NORMAL_DIALOG_NO_OR_TEXT
                );
            } else {
                EventWindow(
                    EVENT_TEXT_WATERWHEEL_REWARD,
                    NORMAL_DIALOG_TYPE_OK,
                    "",
                    NORMAL_DIALOG_RESOURCE_GOLD,
                    cell->m_objectMetadata * WATERWHEEL_GOLD_MULTIPLIER,
                    NORMAL_DIALOG_NO_RESOURCE,
                    0,
                    NORMAL_DIALOG_NO_OR_TEXT
                );
                GiveResource(
                    visitingHero,
                    RESOURCE_GOLD,
                    cell->m_objectMetadata * WATERWHEEL_GOLD_MULTIPLIER
                );
                cell->m_objectMetadata = MAP_EVENT_DATA_EMPTY;
            }
            break;
        case MAP_OBJECT_RESOURCE:
            eventResource = cell->m_objectIndex - RESOURCE_PILE_OBJECT_BASE;
            GiveResource(
                visitingHero,
                eventResource,
                eventResource == RESOURCE_GOLD
                    ? cell->m_objectMetadata * RESOURCE_PILE_GOLD_MULTIPLIER
                    : cell->m_objectMetadata
            );
            strcpy(resourceText, gResourceNames[eventResource]);
            resourceText[0] = CyrillicToLower(resourceText[0]);
            sprintf(gText, gEventText[EVENT_TEXT_RESOURCE_PICKUP], resourceText);
            BVResMsg(
                gText,
                eventResource,
                eventResource == RESOURCE_GOLD
                    ? cell->m_objectMetadata * RESOURCE_PILE_GOLD_MULTIPLIER
                    : cell->m_objectMetadata
            );
            removeObj = 1;
            fizzleEffect = EVENT_FIZZLE_PICKUP;
            break;
        case MAP_OBJECT_WINDMILL:
            if (cell->m_objectMetadata <= WINDMILL_RESOURCE_LAST) {
                EventWindow(
                    EVENT_TEXT_WINDMILL_REWARD,
                    NORMAL_DIALOG_TYPE_OK,
                    "",
                    cell->m_objectMetadata,
                    WINDMILL_RESOURCE_AMOUNT,
                    NORMAL_DIALOG_NO_RESOURCE,
                    0,
                    NORMAL_DIALOG_NO_OR_TEXT
                );
                GiveResource(visitingHero, cell->m_objectMetadata, WINDMILL_RESOURCE_AMOUNT);
                cell->m_objectMetadata = WINDMILL_EMPTY;
            } else {
                EventWindow(
                    EVENT_TEXT_WINDMILL_EMPTY,
                    NORMAL_DIALOG_TYPE_OK,
                    "",
                    NORMAL_DIALOG_NO_RESOURCE,
                    0,
                    NORMAL_DIALOG_NO_RESOURCE,
                    0,
                    NORMAL_DIALOG_NO_OR_TEXT
                );
            }
            break;
        case MAP_OBJECT_ANCIENT_LAMP:
            EventWindow(
                EVENT_TEXT_GENIE_LAMP,
                NORMAL_DIALOG_TYPE_YES_NO,
                "",
                NORMAL_DIALOG_NO_RESOURCE,
                0,
                NORMAL_DIALOG_NO_RESOURCE,
                0,
                NORMAL_DIALOG_NO_OR_TEXT
            );
            if (gWindowManager->m_dialogResult == NORMAL_DIALOG_CONFIRM) {
                RecruitEvent(visitingHero, CREATURE_GENIE, cell);
                if (!cell->m_objectMetadata) {
                    removeObj = 1;
                    fizzleEffect = EVENT_FIZZLE_PICKUP;
                }
            }
            break;
        case MAP_OBJECT_WAGON_CAMP:
            if (!cell->m_objectMetadata) {
                EventWindow(
                    EVENT_TEXT_WAGON_EMPTY,
                    NORMAL_DIALOG_TYPE_OK,
                    "",
                    NORMAL_DIALOG_NO_RESOURCE,
                    0,
                    NORMAL_DIALOG_NO_RESOURCE,
                    0,
                    NORMAL_DIALOG_NO_OR_TEXT
                );
            } else {
                EventWindow(
                    EVENT_TEXT_WAGON_RECRUIT,
                    NORMAL_DIALOG_TYPE_YES_NO,
                    "",
                    NORMAL_DIALOG_NO_RESOURCE,
                    0,
                    NORMAL_DIALOG_NO_RESOURCE,
                    0,
                    NORMAL_DIALOG_NO_OR_TEXT
                );
                if (gWindowManager->m_dialogResult == NORMAL_DIALOG_CONFIRM)
                    RecruitEvent(visitingHero, CREATURE_ROGUE, cell);
            }
            break;
        case MAP_OBJECT_DESERT_TENT:
            if (!cell->m_objectMetadata) {
                EventWindow(
                    EVENT_TEXT_DESERT_TENT_EMPTY,
                    NORMAL_DIALOG_TYPE_OK,
                    "",
                    NORMAL_DIALOG_NO_RESOURCE,
                    0,
                    NORMAL_DIALOG_NO_RESOURCE,
                    0,
                    NORMAL_DIALOG_NO_OR_TEXT
                );
            } else {
                EventWindow(
                    EVENT_TEXT_DESERT_TENT_RECRUIT,
                    NORMAL_DIALOG_TYPE_YES_NO,
                    "",
                    NORMAL_DIALOG_NO_RESOURCE,
                    0,
                    NORMAL_DIALOG_NO_RESOURCE,
                    0,
                    NORMAL_DIALOG_NO_OR_TEXT
                );
                if (gWindowManager->m_dialogResult == NORMAL_DIALOG_CONFIRM)
                    RecruitEvent(visitingHero, CREATURE_NOMAD, cell);
            }
            break;
        case MAP_OBJECT_STRAW_HUT:
        case MAP_OBJECT_HOUSE:
        case MAP_OBJECT_CABIN:
        case MAP_OBJECT_DWARF_LOG_CABIN:
        case MAP_OBJECT_PEASANT_LOG_CABIN:
            HouseEvent(visitingHero, cell);
            break;
        case MAP_OBJECT_MONSTER:
            PlayerMonsterInteract(cell, cell, visitingHero, &removeObj, x, y, 0, x, y);
            break;
        case MAP_OBJECT_OBELISK:
            if (!(gGame->m_obeliskVisitors[cell->m_objectMetadata - 1]
                  & (1 << visitingHero->m_owner))) {
                gGame->VisitObelisk(visitingHero->m_owner);
                gGame->m_obeliskVisitors[cell->m_objectMetadata - 1] |= 1 << visitingHero->m_owner;
                EventWindow(
                    EVENT_TEXT_OBELISK_REWARD,
                    NORMAL_DIALOG_TYPE_OK,
                    "",
                    NORMAL_DIALOG_NO_RESOURCE,
                    0,
                    NORMAL_DIALOG_NO_RESOURCE,
                    0,
                    NORMAL_DIALOG_NO_OR_TEXT
                );
                ViewPuzzle();
            } else {
                EventWindow(
                    EVENT_TEXT_OBELISK_VISITED,
                    NORMAL_DIALOG_TYPE_OK,
                    "",
                    NORMAL_DIALOG_NO_RESOURCE,
                    0,
                    NORMAL_DIALOG_NO_RESOURCE,
                    0,
                    NORMAL_DIALOG_NO_OR_TEXT
                );
            }
            break;
        case MAP_OBJECT_RANKING_SHRINE:
            EventWindow(
                EVENT_TEXT_RANKING_SHRINE,
                NORMAL_DIALOG_TYPE_OK,
                "",
                NORMAL_DIALOG_NO_RESOURCE,
                0,
                NORMAL_DIALOG_NO_RESOURCE,
                0,
                NORMAL_DIALOG_NO_OR_TEXT
            );
            gMouseManager->SetPointer(ADVENTURE_POINTER_DEFAULT);
            thiefWindow = new heroWindow(0, 0, "thiefwin.bin");
            if (!thiefWindow)
                MemError();
            SetWinText(thiefWindow, WINDOW_TEXT_THIEVES_GUILD);
            gTownManager->SetupThievesGuild(thiefWindow, THIEVES_CATEGORY_COUNT);
            strcpy(gText, localization::Tr("event.shrine.rankings"));
            SET_WIDGET_MESSAGE(widgetEvent, WIDGET_COMMAND_SET_TEXT, 0);
            widgetEvent.text = gText;
            thiefWindow->BroadcastMessage(widgetEvent);
            gWindowManager->DoDialog(thiefWindow, TrueFalseDialogHandler, 0);
            delete thiefWindow;
            RedrawAdvScreen(1);
            break;
        case MAP_OBJECT_SPELL_SHRINE:
            sprintf(
                gText,
                "%s'%s'.",
                gEventText[EVENT_TEXT_SPELL_SHRINE],
                gSpellNames[cell->m_objectMetadata - MAP_EVENT_SPELL_OFFSET]
            );
            if (visitingHero->HasArtifact(ARTIFACT_MAGIC_BOOK)) {
                visitingHero->AddSpell(
                    cell->m_objectMetadata - MAP_EVENT_SPELL_OFFSET,
                    visitingHero->m_primaryStats[HERO_PRIMARY_KNOWLEDGE],
                    0
                );
                EventWindow(
                    EVENT_TEXT_CUSTOM,
                    NORMAL_DIALOG_TYPE_OK,
                    gText,
                    NORMAL_DIALOG_SPELL,
                    cell->m_objectMetadata - MAP_EVENT_SPELL_OFFSET,
                    NORMAL_DIALOG_NO_RESOURCE,
                    0,
                    NORMAL_DIALOG_NO_OR_TEXT
                );
            } else {
                strcat(gText, localization::Tr("event.shrine.no_spellbook"));
                EventWindow(
                    EVENT_TEXT_CUSTOM,
                    NORMAL_DIALOG_TYPE_OK,
                    gText,
                    NORMAL_DIALOG_NO_RESOURCE,
                    0,
                    NORMAL_DIALOG_NO_RESOURCE,
                    0,
                    NORMAL_DIALOG_NO_OR_TEXT
                );
            }
            break;
        case MAP_OBJECT_TOWN:
            TownEvent(cell, x, y);
            break;
        case MAP_OBJECT_WHIRLPOOL:
            DoWhirlpool(visitingHero);
        case MAP_OBJECT_STONE_LITHS:
            portalCount = 0;
            for (teleY = 0; teleY < MAP_CELL_GRID_SIZE; teleY++) {
                for (teleX = 0; teleX < MAP_CELL_GRID_SIZE; teleX++) {
                    if (gGame->m_map[teleX][teleY].m_triggerType
                            == static_cast<u8>(eventKind | MAP_TRIGGER_EVENT)
                        && MANHATTAN_LENGTH(teleX - x, teleY - y)
                               > (eventKind == MAP_OBJECT_STONE_LITHS ? STONE_LITHS_MIN_DISTANCE
                                                                      : WHIRLPOOL_MIN_DISTANCE))
                        portalCount++;
                }
            }
            if (portalCount >= 1) {
                if (portalCount > 1)
                    portalCount = Random(1, portalCount);
                for (teleY = 0; teleY < MAP_CELL_GRID_SIZE; teleY++) {
                    for (teleX = 0; teleX < MAP_CELL_GRID_SIZE; teleX++) {
                        if (gGame->m_map[teleX][teleY].m_triggerType
                                == static_cast<u8>(eventKind | MAP_TRIGGER_EVENT)
                            && MANHATTAN_LENGTH(teleX - x, teleY - y)
                                   > (eventKind == MAP_OBJECT_STONE_LITHS
                                          ? STONE_LITHS_MIN_DISTANCE
                                          : WHIRLPOOL_MIN_DISTANCE)) {
                            if (--portalCount <= 0)
                                goto teleport;
                        }
                    }
                }
            teleport:
                StopCursor(1);
                gAdvManager->TeleportTo(teleX, teleY, 1);
            }
            break;
        case MAP_OBJECT_ARTIFACT:
            if (visitingHero->NumArtifacts() == HERO_ARTIFACT_SLOT_COUNT) {
                NormalDialog(localization::Tr("event.artifact.full"), NORMAL_DIALOG_TYPE_OK);
                break;
            }
            switch (cell->m_objectMetadata) {
                case ARTIFACT_EVENT_MODE_PICKUP:
                    EventWindow(
                        EVENT_TEXT_CUSTOM,
                        NORMAL_DIALOG_TYPE_OK,
                        gArtifactEvent[cell->m_objectIndex],
                        NORMAL_DIALOG_ARTIFACT,
                        cell->m_objectIndex,
                        NORMAL_DIALOG_NO_RESOURCE,
                        0,
                        NORMAL_DIALOG_NO_OR_TEXT
                    );
                giveArtifact:
                    GiveArtifact(visitingHero, cell->m_objectIndex);
                    removeObj = 1;
                    fizzleEffect = EVENT_FIZZLE_PICKUP;
                    break;
                case ARTIFACT_EVENT_MODE_GUARDED:
                    EventWindow(
                        EVENT_TEXT_ARTIFACT_GUARDED,
                        NORMAL_DIALOG_TYPE_OK,
                        "",
                        NORMAL_DIALOG_NO_RESOURCE,
                        0,
                        NORMAL_DIALOG_NO_RESOURCE,
                        0,
                        NORMAL_DIALOG_NO_OR_TEXT
                    );
                    if (CombatMonsterEvent(
                            visitingHero,
                            CREATURE_ROGUE,
                            ARTIFACT_EVENT_GUARD_ROGUE_COUNT,
                            cell,
                            x,
                            y,
                            0,
                            x,
                            y
                        )
                        == COMBAT_RESULT_ATTACKER) {
                        sprintf(
                            gText,
                            gEventText[EVENT_TEXT_ARTIFACT_RECOVERED],
                            gArtifactNames[cell->m_objectIndex]
                        );
                        EventWindow(
                            EVENT_TEXT_CUSTOM,
                            NORMAL_DIALOG_TYPE_OK,
                            gText,
                            NORMAL_DIALOG_ARTIFACT,
                            cell->m_objectIndex,
                            NORMAL_DIALOG_NO_RESOURCE,
                            0,
                            NORMAL_DIALOG_NO_OR_TEXT
                        );
                        goto giveArtifact;
                    }
                    break;
                case ARTIFACT_EVENT_MODE_GOLD:
                    sprintf(
                        gText,
                        gEventText[EVENT_TEXT_LEPRECHAUN_OFFER],
                        gArtifactNames[cell->m_objectIndex]
                    );
                    EventWindow(
                        EVENT_TEXT_CUSTOM,
                        NORMAL_DIALOG_TYPE_YES_NO,
                        gText,
                        NORMAL_DIALOG_ARTIFACT,
                        cell->m_objectIndex,
                        NORMAL_DIALOG_NO_RESOURCE,
                        0,
                        NORMAL_DIALOG_NO_OR_TEXT
                    );
                    if (gWindowManager->m_dialogResult == NORMAL_DIALOG_CONFIRM) {
                        if (gGame->m_players[visitingHero->m_owner].m_resources[RESOURCE_GOLD]
                            >= ARTIFACT_EVENT_GOLD_COST) {
                            gGame->m_players[visitingHero->m_owner].m_resources[RESOURCE_GOLD] -=
                                ARTIFACT_EVENT_GOLD_COST;
                            goto giveArtifact;
                        } else {
                            EventWindow(
                                EVENT_TEXT_LEPRECHAUN_NO_GOLD,
                                NORMAL_DIALOG_TYPE_OK,
                                "",
                                NORMAL_DIALOG_NO_RESOURCE,
                                0,
                                NORMAL_DIALOG_NO_RESOURCE,
                                0,
                                NORMAL_DIALOG_NO_OR_TEXT
                            );
                        }
                    } else {
                        EventWindow(
                            EVENT_TEXT_LEPRECHAUN_REFUSAL,
                            NORMAL_DIALOG_TYPE_OK,
                            "",
                            NORMAL_DIALOG_NO_RESOURCE,
                            0,
                            NORMAL_DIALOG_NO_RESOURCE,
                            0,
                            NORMAL_DIALOG_NO_OR_TEXT
                        );
                        removeObj = 1;
                    }
                    break;
            }
            visitingHero->CheckLevel();
            break;
        case MAP_OBJECT_HERO:
            DemobilizeCurrHero();
            opponent = gGame->GetHero(cell->m_objectMetadata);
            if (opponent->m_owner == gCurPlayer) {
                HeroSwap(visitingHero, opponent);
            } else {
                theirTown = NULL;
                if (opponent->m_locationType == (MAP_TRIGGER_EVENT | MAP_OBJECT_TOWN)) {
                    theirTown = gGame->GetTown(opponent->m_occupiedTown);
                    theirTown->m_occupyingHeroId = opponent->m_id;
                }
                res = DoCombat(
                    x,
                    y,
                    visitingHero,
                    &visitingHero->m_army,
                    theirTown,
                    opponent,
                    &opponent->m_army,
                    x,
                    y,
                    COMBAT_RANDOM_SEED_NEW,
                    1
                );
                if (res == COMBAT_RESULT_ATTACKER && theirTown)
                    gGame->ClaimTown(theirTown->m_id, gCurPlayer);
            }
            break;
        case MAP_OBJECT_SIGNPOST:
            gSearchArray->FindNearestObject(
                visitingHero->m_x,
                visitingHero->m_y,
                visitingHero->m_direction,
                SEARCH_NO_COST_LIMIT,
                MAP_TRIGGER_EVENT | MAP_OBJECT_TOWN
            );
            if (GetCell(gSearchArray->m_specialTargetX, gSearchArray->m_specialTargetY)
                    ->m_triggerType
                == (MAP_TRIGGER_EVENT | MAP_OBJECT_TOWN)) {
                sprintf(
                    gText,
                    gEventText[EVENT_TEXT_SIGNPOST],
                    GetTownName(gGame->GetTownId(
                        gSearchArray->m_specialTargetX,
                        gSearchArray->m_specialTargetY
                    ))
                );
                EventWindow(
                    EVENT_TEXT_CUSTOM,
                    NORMAL_DIALOG_TYPE_OK,
                    gText,
                    NORMAL_DIALOG_NO_RESOURCE,
                    0,
                    NORMAL_DIALOG_NO_RESOURCE,
                    0,
                    NORMAL_DIALOG_NO_OR_TEXT
                );
            }
            break;
        case MAP_OBJECT_DAEMON_CAVE:
            EventWindow(
                EVENT_TEXT_DAEMON_CAVE_PROMPT,
                NORMAL_DIALOG_TYPE_YES_NO,
                "",
                NORMAL_DIALOG_NO_RESOURCE,
                0,
                NORMAL_DIALOG_NO_RESOURCE,
                0,
                NORMAL_DIALOG_NO_OR_TEXT
            );
            if (gWindowManager->m_dialogResult == NORMAL_DIALOG_CANCEL)
                break;
            switch (cell->m_objectMetadata) {
                case DAEMON_CAVE_EMPTY:
                    EventWindow(
                        EVENT_TEXT_DAEMON_CAVE_EMPTY,
                        NORMAL_DIALOG_TYPE_OK,
                        "",
                        NORMAL_DIALOG_NO_RESOURCE,
                        0,
                        NORMAL_DIALOG_NO_RESOURCE,
                        0,
                        NORMAL_DIALOG_NO_OR_TEXT
                    );
                    cell->m_objectMetadata = DAEMON_CAVE_EMPTY;
                    break;
                case DAEMON_REWARD_EXPERIENCE:
                    GiveExperience(visitingHero, DAEMON_EXPERIENCE, 0);
                    EventWindow(
                        EVENT_TEXT_DAEMON_CAVE_EXPERIENCE,
                        NORMAL_DIALOG_TYPE_OK,
                        "",
                        NORMAL_DIALOG_EXPERIENCE,
                        DAEMON_EXPERIENCE,
                        NORMAL_DIALOG_NO_RESOURCE,
                        0,
                        NORMAL_DIALOG_NO_OR_TEXT
                    );
                    cell->m_objectMetadata = DAEMON_CAVE_EMPTY;
                    visitingHero->CheckLevel();
                    break;
                case DAEMON_REWARD_ARTIFACT:
                    if (visitingHero->NumArtifacts() == HERO_ARTIFACT_SLOT_COUNT)
                        goto goldReward;
                    if (gGame->GetRandomArtifactId() == ARTIFACT_NONE)
                        goto goldReward;
                    GiveExperience(visitingHero, DAEMON_EXPERIENCE, 0);
                    artifactId = GiveRandomArtifact(visitingHero);
                    EventWindow(
                        EVENT_TEXT_DAEMON_CAVE_ARTIFACT,
                        NORMAL_DIALOG_TYPE_OK,
                        "",
                        NORMAL_DIALOG_ARTIFACT,
                        artifactId,
                        NORMAL_DIALOG_EXPERIENCE,
                        DAEMON_EXPERIENCE,
                        NORMAL_DIALOG_NO_OR_TEXT
                    );
                    cell->m_objectMetadata = DAEMON_CAVE_EMPTY;
                    visitingHero->CheckLevel();
                    break;
                case DAEMON_REWARD_EXPERIENCE_GOLD:
                goldReward:
                    EventWindow(
                        EVENT_TEXT_DAEMON_CAVE_GOLD,
                        NORMAL_DIALOG_TYPE_OK,
                        "",
                        NORMAL_DIALOG_RESOURCE_GOLD,
                        DAEMON_GOLD,
                        NORMAL_DIALOG_EXPERIENCE,
                        DAEMON_EXPERIENCE,
                        NORMAL_DIALOG_NO_OR_TEXT
                    );
                    GiveExperience(visitingHero, DAEMON_EXPERIENCE, 0);
                    GiveResource(visitingHero, RESOURCE_GOLD, DAEMON_GOLD);
                    cell->m_objectMetadata = DAEMON_CAVE_EMPTY;
                    visitingHero->CheckLevel();
                    break;
                case DAEMON_REWARD_RANSOM:
                    EventWindow(
                        EVENT_TEXT_DAEMON_CAVE_RANSOM,
                        NORMAL_DIALOG_TYPE_YES_NO,
                        "",
                        NORMAL_DIALOG_NO_RESOURCE,
                        0,
                        NORMAL_DIALOG_NO_RESOURCE,
                        0,
                        NORMAL_DIALOG_NO_OR_TEXT
                    );
                    if (gWindowManager->m_dialogResult == NORMAL_DIALOG_CONFIRM) {
                        if (gGame->m_players[visitingHero->m_owner].m_resources[RESOURCE_GOLD]
                            < DAEMON_GOLD) {
                            EventWindow(
                                EVENT_TEXT_DAEMON_CAVE_DEATH,
                                NORMAL_DIALOG_TYPE_OK,
                                "",
                                NORMAL_DIALOG_NO_RESOURCE,
                                0,
                                NORMAL_DIALOG_NO_RESOURCE,
                                0,
                                NORMAL_DIALOG_NO_OR_TEXT
                            );
                            HeroLoses(visitingHero);
                        } else {
                            gGame->m_players[visitingHero->m_owner].m_resources[RESOURCE_GOLD] -=
                                DAEMON_GOLD;
                        }
                    } else {
                        HeroLoses(visitingHero);
                    }
                    break;
            }
            break;
        case MAP_OBJECT_GRAVEYARD:
            EventWindow(
                EVENT_TEXT_GRAVEYARD_PROMPT,
                NORMAL_DIALOG_TYPE_YES_NO,
                "",
                NORMAL_DIALOG_NO_RESOURCE,
                0,
                NORMAL_DIALOG_NO_RESOURCE,
                0,
                NORMAL_DIALOG_NO_OR_TEXT
            );
            if (gWindowManager->m_dialogResult == NORMAL_DIALOG_CONFIRM) {
                switch (cell->m_objectMetadata) {
                    case GHOST_SITE_EMPTY:
                        EventWindow(
                            EVENT_TEXT_GRAVEYARD_EMPTY,
                            NORMAL_DIALOG_TYPE_OK,
                            "",
                            NORMAL_DIALOG_MORALE_PENALTY,
                            0,
                            NORMAL_DIALOG_NO_RESOURCE,
                            0,
                            NORMAL_DIALOG_NO_OR_TEXT
                        );
                        if (!(visitingHero->m_eventFlags & HERO_EVENT_GRAVEYARD)) {
                            visitingHero->m_eventFlags |= HERO_EVENT_GRAVEYARD;
                            visitingHero->m_morale--;
                        }
                        break;
                    default:
                        if (GhostEvent(visitingHero, cell, EVENT_TEXT_GRAVEYARD_REWARD, x, y))
                            cell->m_objectMetadata = GHOST_SITE_EMPTY;
                }
            }
            break;
        case MAP_OBJECT_SHIPWRECK:
            EventWindow(
                EVENT_TEXT_SHIPWRECK_PROMPT,
                NORMAL_DIALOG_TYPE_YES_NO,
                "",
                NORMAL_DIALOG_NO_RESOURCE,
                0,
                NORMAL_DIALOG_NO_RESOURCE,
                0,
                NORMAL_DIALOG_NO_OR_TEXT
            );
            if (gWindowManager->m_dialogResult == NORMAL_DIALOG_CONFIRM) {
                switch (cell->m_objectMetadata) {
                    case GHOST_SITE_EMPTY:
                        EventWindow(
                            EVENT_TEXT_SHIPWRECK_EMPTY,
                            NORMAL_DIALOG_TYPE_OK,
                            "",
                            NORMAL_DIALOG_MORALE_PENALTY,
                            0,
                            NORMAL_DIALOG_NO_RESOURCE,
                            0,
                            NORMAL_DIALOG_NO_OR_TEXT
                        );
                        if (!(visitingHero->m_eventFlags & HERO_EVENT_SHIPWRECK)) {
                            visitingHero->m_eventFlags |= HERO_EVENT_SHIPWRECK;
                            visitingHero->m_morale--;
                        }
                        break;
                    default:
                        previousCell = GetCell(
                            x - normalDirTable[visitingHero->m_direction].x,
                            y - normalDirTable[visitingHero->m_direction].y
                        );
                        if (GhostEvent(
                                visitingHero,
                                previousCell,
                                EVENT_TEXT_SHIPWRECK_REWARD,
                                x,
                                y
                            ))
                            cell->m_objectMetadata = GHOST_SITE_EMPTY;
                        break;
                }
            }
            break;
        default:
            break;
    }
    UpdateRadar(1, 0);
    UpdateHeroLocators(1, 1);
    UpdateTownLocators(1, 1);
    UpdBottomView(1, 1, 1);
    if (removeObj) {
        EraseObj(cell, x, y);
        FizzleCenter(fizzleEffect);
    } else {
        CompleteDraw(0);
    }
    UpdateScreen(0, 0);
    PlayMusic(m_currentTerrain);
    gMouseManager->ReallyShowPointer();
    CheckEndGame(0);
}

VA(0x004267ed, 0x1c8)
void advManager::EraseObj(class mapCell* cell, i32 x, i32 y) {
    i8 erased = 0;
    i32 j;
    i32 i;

    erased = 1;
    cell->m_triggerType = MAP_OBJECT_NONE;
    cell->m_objectIndex = MAP_CELL_NO_FRAME;
    if (cell->m_flags & MAP_CELL_OBJECT_ANIMATED)
        cell->m_flags -= MAP_CELL_OBJECT_ANIMATED;
    if ((cell->m_secondaryTrigger & MAP_TRIGGER_TYPE_MASK) > 0
        && (cell->m_secondaryTrigger & MAP_TRIGGER_TYPE_MASK) < 0x7f) {
        cell->m_triggerType = cell->m_secondaryTrigger & MAP_TRIGGER_TYPE_MASK;
        cell->m_secondaryTrigger -= cell->m_triggerType;
        for (i = x - 1; i <= x + 1; i++) {
            for (j = y - 1; j <= y + 1; j++) {
                if (i >= 0 && i < MAP_CELL_GRID_SIZE && j >= 0 && j < MAP_CELL_GRID_SIZE
                    && gGame->m_map[i][j].m_triggerType == cell->m_triggerType)
                    cell->m_objectMetadata = gGame->m_map[i][j].m_objectMetadata;
            }
        }
        gGame->SettleOverlay(x, y);
    }
    if (gGame->m_mapSounds[x][y] != MAP_SOUND_NONE) {
        gGame->m_mapSounds[x][y] = MAP_SOUND_NONE;
        if (gShowIt)
            SetEnvironmentOrigin(
                m_mapOriginX + ADVMGR_VIEW_CENTER,
                m_mapOriginY + ADVMGR_VIEW_CENTER,
                1
            );
    }
    gGame->SetupAdjacentMons();
}

VA(0x004269b5, 0xa3)
void advManager::HeroSwap(class hero* firstHero, class hero* secondHero) {
    swapManager* swapMgr;

    swapMgr = new swapManager(firstHero, secondHero);
    if (!swapMgr)
        MemError();
    gExec->DoDialog(swapMgr);
    delete swapMgr;
}

VA(0x00426a58, 0x191)
void advManager::TownEvent(class mapCell* cell, i32 x, i32 y) {
    hero* curHero;
    i32 result;
    hero* defender;
    town* recRef;

    recRef = gGame->GetTown(cell->m_objectMetadata);
    curHero = gGame->GetHero(gCurPlayerData->m_currentHero);
    DemobilizeCurrHero();
    if (recRef->m_owner == gCurPlayer) {
        recRef->m_occupyingHeroId = gCurPlayerData->CurrentHero();
        recRef->View();
    } else if (recRef->HasGarrison()) {
        defender = recRef->m_occupyingHeroId == TOWN_OCCUPYING_HERO_NONE
                       ? NULL
                       : gGame->GetHero(recRef->m_occupyingHeroId);
        result = DoCombat(
            x,
            y,
            curHero,
            &curHero->m_army,
            recRef,
            defender,
            &recRef->m_army,
            x,
            y,
            COMBAT_RANDOM_SEED_NEW,
            1
        );
        if (result == COMBAT_RESULT_ATTACKER)
            gGame->ClaimTown(recRef->m_id, gCurPlayer);
    } else {
        gGame->ClaimTown(recRef->m_id, gCurPlayer);
        UpdateRadar(1, 0);
        UpdateHeroLocators(1, 1);
        UpdateTownLocators(1, 1);
        recRef->m_occupyingHeroId = gCurPlayerData->CurrentHero();
        recRef->View();
    }
    recRef->GiveSpells();
    curHero->CheckLevel();
}

// Adventure-event music cue; HoMM1 keys the ambient track off the map
// object type and records that an event track is playing.
VA(0x00426be9, 0x1dd)
void advManager::EventSound(i16 eventType, i16 eventData) {
    i32 musicTrack = MUSIC_TRACK_NONE;

    switch (eventType) {
        case MAP_OBJECT_STRAW_HUT:
        case MAP_OBJECT_HOUSE:
        case MAP_OBJECT_CABIN:
        case MAP_OBJECT_DWARF_LOG_CABIN:
        case MAP_OBJECT_PEASANT_LOG_CABIN:
            musicTrack = MUSIC_TRACK_HOUSE;
            break;
        case MAP_OBJECT_ULTIMATE_ARTIFACT:
            musicTrack = MUSIC_TRACK_ULTIMATE_ARTIFACT;
            break;
        case MAP_OBJECT_LIGHTHOUSE:
            musicTrack = MUSIC_TRACK_LIGHTHOUSE;
            break;
        case MAP_OBJECT_SPELL_SHRINE:
            musicTrack = MUSIC_TRACK_SPELL_SHRINE;
            break;
        case MAP_OBJECT_ARTIFACT:
            if (eventData == 1)
                musicTrack = MUSIC_TRACK_TREASURE;
            break;
        case MAP_OBJECT_SKELETON:
        case MAP_OBJECT_TREASURE_CHEST:
        case MAP_OBJECT_CAMPFIRE:
        case MAP_OBJECT_WATERWHEEL:
        case MAP_OBJECT_WINDMILL:
            musicTrack = MUSIC_TRACK_TREASURE;
            break;
        case MAP_OBJECT_ALCHEMIST_LAB:
        case MAP_OBJECT_MINE:
        case MAP_OBJECT_SAWMILL:
            musicTrack = MUSIC_TRACK_MINE_CAPTURED;
            break;
        case MAP_OBJECT_BUOY:
        case MAP_OBJECT_OASIS:
            musicTrack = MUSIC_TRACK_BUOY_OASIS;
            break;
        case MAP_OBJECT_DAEMON_CAVE:
            musicTrack = MUSIC_TRACK_DAEMON_CAVE;
            break;
        case MAP_OBJECT_FAERIE_RING:
            musicTrack = MUSIC_TRACK_FAERIE_RING;
            break;
        case MAP_OBJECT_FOUNTAIN:
            musicTrack = MUSIC_TRACK_FOUNTAIN;
            break;
        case MAP_OBJECT_GAZEBO:
            musicTrack = MUSIC_TRACK_GAZEBO;
            break;
        case MAP_OBJECT_ANCIENT_LAMP:
            musicTrack = MUSIC_TRACK_ANCIENT_LAMP;
            break;
        case MAP_OBJECT_GRAVEYARD:
            musicTrack = MUSIC_TRACK_GRAVEYARD;
            break;
        case MAP_OBJECT_DRAGON_CITY:
            musicTrack = MUSIC_TRACK_DRAGON_CITY;
            break;
        case MAP_OBJECT_OBELISK:
            musicTrack = MUSIC_TRACK_OBELISK;
            break;
        case MAP_OBJECT_STATUE:
            musicTrack = MUSIC_TRACK_STATUE;
            break;
        case MAP_OBJECT_DESERT_TENT:
            musicTrack = 0xf;
            break;
        case MAP_OBJECT_STONE_LITHS:
            musicTrack = MUSIC_TRACK_TELEPORT;
            break;
        case MAP_OBJECT_WAGON_CAMP:
            musicTrack = MUSIC_TRACK_WAGON_CAMP;
            break;
        case MAP_OBJECT_WHIRLPOOL:
            musicTrack = MUSIC_TRACK_WHIRLPOOL;
            break;
        default:
            musicTrack = MUSIC_TRACK_NONE;
            break;
    }
    if (musicTrack != MUSIC_TRACK_NONE) {
        PlayMusic(musicTrack);
        gEventMusicPlaying = 1;
    } else {
        gEventMusicPlaying = 0;
    }
}

VA(0x00426dc6, 0xc0)
void advManager::EventWindow(
    i16 eventId,
    H1_ENUM_PARAM(NormalDialogType, i32) buttons,
    char* text,
    H1_ENUM_PARAM(NormalDialogResourceType, i32) type1,
    i32 value1,
    H1_ENUM_PARAM(NormalDialogResourceType, i32) type2,
    i32 value2,
    H1_ENUM_PARAM(NormalDialogOrText, i32) showOrText
) {
    i32 newValue1;
    i32 unusedValue7;
    i32 stopOk;
    i32 unusedValue8;
    i32 unusedValue9;
    i32 curUnusedValue11;
    i32 unusedValue12Value;
    char eventText[EVENT_TEXT_BUFFER_SIZE];
    i16 newUnused;

    stopOk = 0;
    GrabScreen();
    newUnused = 1;
    if (eventId >= 0 && eventId < EVENT_TEXT_WINDOW_END)
        sprintf(eventText, gEventText[eventId]);
    else if (eventId == EVENT_TEXT_CUSTOM)
        sprintf(eventText, text);
    else
        sprintf(eventText, "Event ID %d", eventId);
    NormalDialog(eventText, buttons, 0x61, -1, type1, value1, type2, value2, showOrText);
}

VA(0x00426e86, 0x90)
i16 advManager::GiveArtifact(class hero* eventHero, i8 artifact) {
    i16 slot;

    for (slot = 0; slot < HERO_ARTIFACT_SLOT_COUNT; slot++) {
        if (eventHero->m_artifacts[slot] == ARTIFACT_NONE)
            break;
    }
    if (slot == HERO_ARTIFACT_SLOT_COUNT)
        return -1;
    eventHero->m_artifacts[slot] = artifact;
    gGame->m_randomArtifacts[artifact] = eventHero->m_id;
    GiveTakeArtifactStat(eventHero, artifact, EVENT_ARTIFACT_GIVE);
    return slot;
}

VA(0x00426f16, 0x4f)
i32 advManager::GiveRandomArtifact(class hero* eventHero) {
    i8 artifact;

    artifact = gGame->GetRandomArtifactId();
    if (artifact == ARTIFACT_NONE)
        GiveResource(eventHero, RESOURCE_GOLD, EVENT_RANDOM_ARTIFACT_GOLD);
    else
        GiveArtifact(eventHero, artifact);
    return artifact;
}

VA(0x00426f65, 0x9e)
#line 1113 "E:\\Users\\igorl\\VSS\\HMM\\HMM1\\Source\\Game\\EVENTS.CPP"
i32 advManager::GiveExperience(class hero* eventHero, i32 experience, i8 checkLevel) {
    i32 prevLevel;
    i32 unusedValue1;
    i32 unusedValue2;
    i32 savedLevel;
    i32 levelGapVal;

    prevLevel = eventHero->GetLevel(eventHero->m_experience);
    eventHero->m_level = prevLevel;
    eventHero->m_experience += experience;
#line 1093
    H1_ASSERT(experience >= 0);
#line 1094
    H1_ASSERT(eventHero->m_experience >= 0);
    savedLevel = eventHero->GetLevel(eventHero->m_experience);
    if (checkLevel)
        eventHero->CheckLevel();
    return savedLevel - prevLevel;
}

VA(0x00427003, 0x6a)
void advManager::GiveResource(class hero* eventHero, i8 resource, i16 amount) {
    if (resource >= 0 && resource <= RESOURCE_LAST)
        gGame->m_players[eventHero->m_owner].m_resources[resource] += amount;
}

VA(0x0042706d, 0xbf)
void advManager::RecruitEvent(class hero* eventHero, i32 creatureType, class mapCell* cell) {
    tag_message recruitMessage;
    i16 availableCount;
    recruitUnit* recruitWindow;
    i32 eventResult;

    availableCount = cell->m_objectMetadata;
    recruitWindow = new recruitUnit(&eventHero->m_army, creatureType, &availableCount);
    if (!recruitWindow)
        MemError();
    gExec->DoDialog(recruitWindow);
    delete recruitWindow;
    cell->m_objectMetadata = availableCount;
}

VA(0x0042712c, 0x2ac)
i8 advManager::GhostEvent(class hero* eventHero, class mapCell* cell, i32 textId, i32 x, i32 y) {
    i32 artifact;

    switch (cell->m_objectMetadata) {
        case GHOST_SITE_SMALL:
            if (CombatMonsterEvent(
                    eventHero,
                    CREATURE_GHOST,
                    GHOST_SMALL_COUNT,
                    cell,
                    x,
                    y,
                    0,
                    x,
                    y
                )
                == COMBAT_RESULT_ATTACKER) {
                sprintf(gText, "%s", gEventText[textId]);
                EventWindow(
                    EVENT_TEXT_CUSTOM,
                    NORMAL_DIALOG_TYPE_OK,
                    gText,
                    NORMAL_DIALOG_RESOURCE_GOLD,
                    GHOST_SMALL_GOLD,
                    NORMAL_DIALOG_NO_RESOURCE,
                    0,
                    NORMAL_DIALOG_NO_OR_TEXT
                );
                GiveResource(eventHero, RESOURCE_GOLD, GHOST_SMALL_GOLD);
                eventHero->CheckLevel();
                return 1;
            }
            break;
        case GHOST_SITE_MEDIUM:
            if (CombatMonsterEvent(
                    eventHero,
                    CREATURE_GHOST,
                    GHOST_MEDIUM_COUNT,
                    cell,
                    x,
                    y,
                    0,
                    x,
                    y
                )
                == COMBAT_RESULT_ATTACKER) {
                sprintf(gText, "%s", gEventText[textId]);
                EventWindow(
                    EVENT_TEXT_CUSTOM,
                    NORMAL_DIALOG_TYPE_OK,
                    gText,
                    NORMAL_DIALOG_RESOURCE_GOLD,
                    GHOST_MEDIUM_GOLD,
                    NORMAL_DIALOG_NO_RESOURCE,
                    0,
                    NORMAL_DIALOG_NO_OR_TEXT
                );
                GiveResource(eventHero, RESOURCE_GOLD, GHOST_MEDIUM_GOLD);
                eventHero->CheckLevel();
                return 1;
            }
            break;
        case GHOST_SITE_LARGE:
            if (CombatMonsterEvent(
                    eventHero,
                    CREATURE_GHOST,
                    GHOST_LARGE_COUNT,
                    cell,
                    x,
                    y,
                    0,
                    x,
                    y
                )
                == COMBAT_RESULT_ATTACKER) {
                sprintf(gText, "%s", gEventText[textId]);
                EventWindow(
                    EVENT_TEXT_CUSTOM,
                    NORMAL_DIALOG_TYPE_OK,
                    gText,
                    NORMAL_DIALOG_RESOURCE_GOLD,
                    GHOST_LARGE_GOLD,
                    NORMAL_DIALOG_NO_RESOURCE,
                    0,
                    NORMAL_DIALOG_NO_OR_TEXT
                );
                GiveResource(eventHero, RESOURCE_GOLD, GHOST_LARGE_GOLD);
                eventHero->CheckLevel();
                return 1;
            }
            break;
        default:
            if (CombatMonsterEvent(eventHero, CREATURE_GHOST, GHOST_HUGE_COUNT, cell, x, y, 0, x, y)
                == COMBAT_RESULT_ATTACKER) {
                artifact = GiveRandomArtifact(eventHero);
                sprintf(gText, "%s", gEventText[textId]);
                if (artifact != ARTIFACT_NONE)
                    EventWindow(
                        EVENT_TEXT_CUSTOM,
                        NORMAL_DIALOG_TYPE_OK,
                        gText,
                        NORMAL_DIALOG_RESOURCE_GOLD,
                        GHOST_HUGE_GOLD,
                        NORMAL_DIALOG_ARTIFACT,
                        artifact,
                        NORMAL_DIALOG_NO_OR_TEXT
                    );
                else
                    EventWindow(
                        EVENT_TEXT_CUSTOM,
                        NORMAL_DIALOG_TYPE_OK,
                        gText,
                        NORMAL_DIALOG_RESOURCE_GOLD,
                        GHOST_HUGE_GOLD,
                        NORMAL_DIALOG_NO_RESOURCE,
                        0,
                        NORMAL_DIALOG_NO_OR_TEXT
                    );
                GiveResource(eventHero, RESOURCE_GOLD, GHOST_HUGE_GOLD);
                eventHero->CheckLevel();
                return 1;
            }
            break;
    }
    return 0;
}

VA(0x004273d8, 0x103)
void advManager::HouseEvent(class hero* eventHero, class mapCell* cell) {
    i16 houseIndex;

    houseIndex = (cell->m_triggerType & MAP_TRIGGER_TYPE_MASK) - MAP_OBJECT_HOUSE_FIRST;
    if (!cell->m_objectMetadata) {
        EventWindow(
            houseIndex * EVENT_TEXT_HOUSE_STRIDE + EVENT_TEXT_HOUSE_EMPTY,
            NORMAL_DIALOG_TYPE_OK,
            "",
            NORMAL_DIALOG_NO_RESOURCE,
            0,
            NORMAL_DIALOG_NO_RESOURCE,
            0,
            NORMAL_DIALOG_NO_OR_TEXT
        );
    } else {
        i8 creatures[EVENT_HOUSE_COUNT] =
            {CREATURE_GOBLIN, CREATURE_PEASANT, CREATURE_ARCHER, CREATURE_DWARF, CREATURE_PEASANT};

        EventWindow(
            houseIndex * EVENT_TEXT_HOUSE_STRIDE + EVENT_TEXT_HOUSE_RECRUIT,
            NORMAL_DIALOG_TYPE_YES_NO,
            "",
            NORMAL_DIALOG_NO_RESOURCE,
            0,
            NORMAL_DIALOG_NO_RESOURCE,
            0,
            NORMAL_DIALOG_NO_OR_TEXT
        );
        if (gWindowManager->m_dialogResult == NORMAL_DIALOG_CONFIRM) {
            if (eventHero->m_army.CanJoin(creatures[houseIndex])) {
                eventHero->m_army
                    .Add(creatures[houseIndex], cell->m_objectMetadata, ARMY_GROUP_EMPTY_SLOT);
                cell->m_objectMetadata = MAP_EVENT_DATA_EMPTY;
            } else {
                EventWindow(
                    houseIndex * EVENT_TEXT_HOUSE_STRIDE + EVENT_TEXT_HOUSE_RANKS_FULL,
                    NORMAL_DIALOG_TYPE_OK,
                    "",
                    NORMAL_DIALOG_NO_RESOURCE,
                    0,
                    NORMAL_DIALOG_NO_RESOURCE,
                    0,
                    NORMAL_DIALOG_NO_OR_TEXT
                );
            }
        }
    }
}

VA(0x004274db, 0x1e2)
i8 advManager::CombatMonsterEvent(
    class hero* eventHero,
    i8 monsterType,
    i16 count,
    class mapCell* cell,
    i32 x,
    i32 y,
    i8 heroDefends,
    i32 fromX,
    i32 fromY
) {
    i16 i;
    i32 res;

    DemobilizeCurrHero();
    if (fromX == -1) {
        fromX = x;
        fromY = y;
    } else {
        m_lastQuickViewX = fromX;
        m_lastQuickViewY = fromY;
        m_mineGuardianFacingLeft = eventHero->m_x < fromX;
        if (ComboDraw(0))
            UpdateScreen(0, 0);
        m_lastQuickViewX = QUICK_VIEW_CLEARED;
    }
    CLEAR_ARMY_GROUP(*gMonGroup);
    if (count / ARMY_GROUP_SLOT_COUNT > 0) {
        for (i = 0; i < ARMY_GROUP_SLOT_COUNT; i++) {
            gMonGroup->m_creatureTypes[i] = monsterType;
            gMonGroup->m_creatureCounts[i] = count / ARMY_GROUP_SLOT_COUNT;
        }
    }
    for (i = count % ARMY_GROUP_SLOT_COUNT - 1; i >= 0; i--) {
        gMonGroup->m_creatureTypes[i] = monsterType;
        gMonGroup->m_creatureCounts[i]++;
    }
    if (heroDefends)
        res = DoCombat(
            fromX,
            fromY,
            NULL,
            gMonGroup,
            NULL,
            eventHero,
            &eventHero->m_army,
            x,
            y,
            COMBAT_RANDOM_SEED_NEW,
            1
        );
    else
        res = DoCombat(
            fromX,
            fromY,
            eventHero,
            &eventHero->m_army,
            NULL,
            NULL,
            gMonGroup,
            x,
            y,
            COMBAT_RANDOM_SEED_NEW,
            1
        );
    MobilizeCurrHero(0);
    return res;
}

// Per-artifact primary-stat bonuses, called through gAdvManager.
VA(0x004276bd, 0x1e4)
void advManager::GiveTakeArtifactStat(class hero* targetHero, i8 artifact, i8 take) {
    i8 theStat = HERO_PRIMARY_NONE;
    i8 amount = 0;
    i32 i;

    switch (artifact) {
        case ARTIFACT_ULTIMATE_BOOK:
            theStat = HERO_PRIMARY_KNOWLEDGE;
            amount = 12;
            break;
        case ARTIFACT_ULTIMATE_SWORD:
            theStat = HERO_PRIMARY_ATTACK;
            amount = 12;
            break;
        case ARTIFACT_ULTIMATE_CLOAK:
            theStat = HERO_PRIMARY_DEFENSE;
            amount = 12;
            break;
        case ARTIFACT_ULTIMATE_WAND:
            theStat = HERO_PRIMARY_SPELL_POWER;
            amount = 12;
            break;
        case ARTIFACT_ARCANE_NECKLACE:
            theStat = HERO_PRIMARY_SPELL_POWER;
            amount = 4;
            break;
        case ARTIFACT_CASTERS_BRACELET:
        case ARTIFACT_MAGES_RING:
            theStat = HERO_PRIMARY_SPELL_POWER;
            amount = 2;
            break;
        case ARTIFACT_WITCHS_BROACH:
            theStat = HERO_PRIMARY_SPELL_POWER;
            amount = 3;
            break;
        case ARTIFACT_THUNDER_MACE:
        case ARTIFACT_GIANT_FLAIL:
            theStat = HERO_PRIMARY_ATTACK;
            amount = 1;
            break;
        case ARTIFACT_ARMORED_GAUNTLETS:
        case ARTIFACT_DEFENDER_HELM:
            theStat = HERO_PRIMARY_DEFENSE;
            amount = 1;
            break;
        case ARTIFACT_BALLISTA:
            theStat = HERO_PRIMARY_BALLISTA;
            amount = 3;
            break;
        case ARTIFACT_STEALTH_SHIELD:
            theStat = HERO_PRIMARY_DEFENSE;
            amount = 2;
            break;
        case ARTIFACT_DRAGON_SWORD:
            theStat = HERO_PRIMARY_ATTACK;
            amount = 3;
            break;
        case ARTIFACT_POWER_AXE:
            theStat = HERO_PRIMARY_ATTACK;
            amount = 2;
            break;
        case ARTIFACT_DIVINE_BREASTPLATE:
            theStat = HERO_PRIMARY_DEFENSE;
            amount = 3;
            break;
        case ARTIFACT_MINOR_SCROLL:
            theStat = HERO_PRIMARY_KNOWLEDGE;
            amount = 2;
            break;
        case ARTIFACT_MAJOR_SCROLL:
            theStat = HERO_PRIMARY_KNOWLEDGE;
            amount = 3;
            break;
        case ARTIFACT_SUPERIOR_SCROLL:
            theStat = HERO_PRIMARY_KNOWLEDGE;
            amount = 4;
            break;
        case ARTIFACT_FOREMOST_SCROLL:
            theStat = HERO_PRIMARY_KNOWLEDGE;
            amount = 5;
            break;
        case ARTIFACT_MEDAL_OF_VALOR:
        case ARTIFACT_MEDAL_OF_COURAGE:
        case ARTIFACT_MEDAL_OF_HONOR:
        case ARTIFACT_MEDAL_OF_DISTINCTION:
        case ARTIFACT_FIZBIN_OF_MISFORTUNE:
            break;
    }
    if (take == EVENT_ARTIFACT_TAKE)
        amount = -amount;
    if (theStat != HERO_PRIMARY_NONE) {
        targetHero->m_primaryStats[theStat] += amount;
        if (amount < 0 && theStat == HERO_PRIMARY_KNOWLEDGE) {
            for (i = 0; i < HERO_SPELL_SLOT_COUNT; i++) {
                if (targetHero->m_spellCharges[i]
                    > targetHero->m_primaryStats[HERO_PRIMARY_KNOWLEDGE])
                    targetHero->m_spellCharges[i] =
                        targetHero->m_primaryStats[HERO_PRIMARY_KNOWLEDGE];
            }
        }
    }
}

VA(0x004278a1, 0x1ed)
void advManager::TransferArtifacts(class hero* sourceHero, class hero* destHero) {
    i16 i;
    i16 j;

    if (!sourceHero || !destHero)
        return;
    for (i = 0; i < HERO_ARTIFACT_SLOT_COUNT; i++) {
        if (destHero->m_artifacts[i] == ARTIFACT_NONE) {
            for (j = 0; j < HERO_ARTIFACT_SLOT_COUNT; j++) {
                if (sourceHero->m_artifacts[j] != ARTIFACT_NONE
                    && sourceHero->m_artifacts[j] != ARTIFACT_MAGIC_BOOK) {
                    if (sourceHero->m_artifacts[j] <= ARTIFACT_ULTIMATE_LAST) {
                        if (gThisNetHumanPlayer[sourceHero->m_owner]
                            || gThisNetHumanPlayer[destHero->m_owner]) {
                            sprintf(
                                gText,
                                localization::Tr("event.artifact.disappears"),
                                gArtifactNames[sourceHero->m_artifacts[j]]
                            );
                            NormalDialog(
                                gText,
                                NORMAL_DIALOG_TYPE_OK,
                                -1,
                                -1,
                                NORMAL_DIALOG_ARTIFACT,
                                sourceHero->m_artifacts[j]
                            );
                        }
                        gGame->m_randomArtifacts[sourceHero->m_artifacts[j]] = HERO_ID_NONE;
                    } else {
                        GiveTakeArtifactStat(
                            destHero,
                            sourceHero->m_artifacts[j],
                            EVENT_ARTIFACT_GIVE
                        );
                        destHero->m_artifacts[i] = sourceHero->m_artifacts[j];
                        gGame->m_randomArtifacts[sourceHero->m_artifacts[j]] = destHero->m_id;
                    }
                    GiveTakeArtifactStat(
                        sourceHero,
                        sourceHero->m_artifacts[j],
                        EVENT_ARTIFACT_TAKE
                    );
                    sourceHero->m_artifacts[j] = ARTIFACT_NONE;
                    break;
                }
            }
        }
    }
}

VA(0x00427a8e, 0x6b)
void advManager::HeroLoses(class hero* lostHero) {
    if (!lostHero)
        return;
    CompleteDraw(m_mapOriginX, m_mapOriginY, 0);
    UpdateScreen(0, 0);
    lostHero->Deallocate();
    FizzleCenter(EVENT_FIZZLE_HERO_LOSS);
    UpdateRadar(1, 0);
    UpdateHeroLocators(1, 1);
}

VA(0x00427af9, 0x122)
void advManager::DoWhirlpool(class hero* eventHero) {
    i32 selectedSlot;
    i16 slotNo;
    i32 groupValues[ARMY_GROUP_SLOT_COUNT];
    i32 lowestValue;
    i32 creatureValue;

    if (!gHumanPlayer[eventHero->m_owner])
        return;
    if (Random(EVENT_WHIRLPOOL_TRIGGER_ROLL, EVENT_WHIRLPOOL_TRIGGER_MAX)
        != EVENT_WHIRLPOOL_TRIGGER_ROLL)
        return;
    lowestValue = EVENT_WHIRLPOOL_ARMY_VALUE_LIMIT;
    selectedSlot = -1;
    for (slotNo = 0; slotNo < ARMY_GROUP_SLOT_COUNT; slotNo++) {
        if (eventHero->m_army.m_creatureCounts[slotNo] > 0) {
            creatureValue =
                eventHero->m_army.m_creatureCounts[slotNo]
                * gMonsterDatabase[eventHero->m_army.m_creatureTypes[slotNo]].fightValue;
            if (creatureValue < lowestValue) {
                lowestValue = creatureValue;
                selectedSlot = slotNo;
            }
        }
    }
    if (eventHero->m_army.GetNumArmies() > 1) {
        eventHero->m_army.m_creatureCounts[selectedSlot] >>= 1;
        if (!eventHero->m_army.m_creatureCounts[selectedSlot])
            eventHero->m_army.m_creatureTypes[selectedSlot] = CREATURE_NONE;
    } else if (eventHero->m_army.m_creatureCounts[selectedSlot] > 1) {
        eventHero->m_army.m_creatureCounts[selectedSlot] >>= 1;
    }
}

VA(0x00427c1b, 0xc2)
void advManager::FizzleCenter(i32 fizzleType) {
    class sample* fizzleSample;

    if (!gShowIt)
        return;
    switch (fizzleType) {
        case EVENT_FIZZLE_HERO_LOSS:
            sprintf(gText, "killfade.82M");
            break;
        case EVENT_FIZZLE_PICKUP:
            sprintf(gText, "pickup%02d.82M", Random(1, 5));
            break;
        default:
            return;
    }
    fizzleSample = LoadPlaySample(gText);
    gWindowManager
        ->SaveFizzleSource(EVENT_FIZZLE_X, EVENT_FIZZLE_Y, EVENT_FIZZLE_WIDTH, EVENT_FIZZLE_HEIGHT);
    CompleteDraw(0);
    gWindowManager->FizzleForward(
        EVENT_FIZZLE_X,
        EVENT_FIZZLE_Y,
        EVENT_FIZZLE_WIDTH,
        EVENT_FIZZLE_HEIGHT,
        EVENT_FIZZLE_STEPS
    );
    WaitSample(fizzleSample);
}

VA(0x00427cdd, 0x1005)
void advManager::DoAIEvent(class mapCell* cell, class hero* eventHero, i32 x, i32 y) {
    i32 freeSlot;
    town* heroTown;
    playerData* ownerPlayerData;
    i32 purchaseValue;
    i32 handled;
    i32 c;
    i32 recruited;
    i32 recruitType;
    i32 isFree;
    i32 oldPlayer;
    i32 eventWork[4];
    i8 removeEvent;
    hero* opponent;
    i32 res;
    i32 battleResult;
    i8 eventType;
    i8 teleX;
    i32 canWin;
    i8 teleY;
    i32 armyStrength;
    i8 portalCount;
    i32 eventResource;
    i8 priorShowIt;
    boatRecord* boat;
    i32 success;
    i8 guardMonster;
    float theirLosses;
    float ourLosses;
    i32 troopCost[RESOURCE_COUNT];

    heroTown = NULL;
    eventType = cell->m_triggerType & MAP_TRIGGER_TYPE_MASK;
    removeEvent = 0;
    handled = 0;
    oldPlayer = gCurPlayer;
    ownerPlayerData = gCurPlayerData;
    --eventHero->m_remainingMobility;
    gMapVisitFlags[x][y] |= gCurPlayerBit;
    switch (eventType) {
        case MAP_OBJECT_COAST:
            if (eventHero->IsEmbarked()) {
                eventHero->m_eventFlags &= ~HERO_EVENT_EMBARKED;
                eventHero->m_remainingMobility = 0;
                eventHero->m_direction = m_cursorDirection;
                m_cursorType = eventHero->m_heroClass;
                m_cursorFrame = GetCursorBaseFrame(m_cursorDirection);
                m_cursorActive = 1;
                CheckAdjacentMon(&guardMonster);
            }
            break;
        case MAP_OBJECT_SHIP:
            boat = &gGame->m_boats[cell->m_objectMetadata];
            gGame->RestoreCell(-1, -1, boat->savedTriggerType, boat->savedEventData, cell, 3);
            eventHero->m_eventFlags |= HERO_EVENT_EMBARKED;
            eventHero->m_remainingMobility = 0;
            boat->heroId = eventHero->m_id;
            boat->owner = eventHero->m_owner;
            m_cursorType = ADVMGR_HERO_ICON_BOAT;
            m_cursorDirection = boat->direction;
            m_cursorFrame = GetCursorBaseFrame(m_cursorDirection);
            m_cursorActive = 1;
            break;
        case MAP_OBJECT_ALCHEMIST_LAB:
        case MAP_OBJECT_MINE:
        case MAP_OBJECT_SAWMILL:
            if (gGame->m_mineOwners[cell->m_objectMetadata] == gCurPlayer)
                break;
            gGame->ClaimMine(cell->m_objectMetadata, gCurPlayer);
            break;
        case MAP_OBJECT_LIGHTHOUSE:
            if (gGame->m_mineOwners[MINE_SLOT_LIGHTHOUSE] == gCurPlayer)
                break;
            gGame->ClaimMine(MINE_SLOT_LIGHTHOUSE, gCurPlayer);
            break;
        case MAP_OBJECT_DRAGON_CITY:
            if (gGame->m_mineOwners[MINE_SLOT_DRAGON_CITY] == gCurPlayer)
                break;
            for (c = 0; c < ARMY_GROUP_SLOT_COUNT; c++) {
                gMonGroup->m_creatureTypes[c] = CREATURE_DRAGON;
                gMonGroup->m_creatureCounts[c] = 1;
            }
            gPhilAI->ChooseEvaluateBattle(
                &eventHero->m_army,
                eventHero,
                gMonGroup,
                NULL,
                0,
                0,
                500,
                canWin,
                armyStrength
            );
            if (canWin) {
                c = DRAGON_CITY_DRAGON_COUNT;
                success = gPhilAI->CombatMonsterEvent(eventHero, CREATURE_DRAGON, &c, cell);
                if (success)
                    gGame->ClaimMine(MINE_SLOT_DRAGON_CITY, gCurPlayer);
            }
            break;
        case MAP_OBJECT_TREASURE_CHEST:
            if (gPhilAI->ChooseGoldOrExperience(
                    eventHero,
                    cell->m_objectMetadata * CHEST_GOLD_MULTIPLIER,
                    (cell->m_objectMetadata - CHEST_EXPERIENCE_LEVEL_OFFSET)
                        * CHEST_EXPERIENCE_MULTIPLIER
                ))
                GiveResource(
                    eventHero,
                    RESOURCE_GOLD,
                    cell->m_objectMetadata * CHEST_GOLD_MULTIPLIER
                );
            else
                GiveExperience(
                    eventHero,
                    (cell->m_objectMetadata - CHEST_EXPERIENCE_LEVEL_OFFSET)
                        * CHEST_EXPERIENCE_MULTIPLIER,
                    1
                );
            removeEvent = 1;
            break;
        case MAP_OBJECT_BUOY:
            if (!(eventHero->m_eventFlags & HERO_EVENT_BUOY)) {
                eventHero->m_eventFlags |= HERO_EVENT_BUOY;
                eventHero->m_morale++;
            }
            break;
        case MAP_OBJECT_FAERIE_RING:
            if (!(eventHero->m_eventFlags & HERO_EVENT_FAERIE_RING)) {
                eventHero->m_eventFlags |= HERO_EVENT_FAERIE_RING;
                eventHero->m_luck++;
            }
            break;
        case MAP_OBJECT_FOUNTAIN:
            if (!(eventHero->m_eventFlags & HERO_EVENT_FOUNTAIN)) {
                eventHero->m_eventFlags |= HERO_EVENT_FOUNTAIN;
                eventHero->m_luck++;
            }
            break;
        case MAP_OBJECT_OASIS:
            if (!(eventHero->m_eventFlags & HERO_EVENT_OASIS)) {
                eventHero->m_eventFlags |= HERO_EVENT_OASIS;
                eventHero->m_morale++;
            }
            break;
        case MAP_OBJECT_STATUE:
            if (!(eventHero->m_eventFlags & HERO_EVENT_STATUE)) {
                eventHero->m_eventFlags |= HERO_EVENT_STATUE;
                eventHero->m_morale += TEMPLE_MORALE_BONUS;
            }
            break;
        case MAP_OBJECT_SKELETON:
            switch (cell->m_objectMetadata) {
                case SKELETON_EMPTY:
                    break;
                case SKELETON_ARTIFACT:
                    GiveRandomArtifact(eventHero);
                    cell->m_objectMetadata = SKELETON_EMPTY;
                    break;
            }
            break;
        case MAP_OBJECT_CAMPFIRE:
            GiveResource(
                eventHero,
                RESOURCE_GOLD,
                (cell->m_objectMetadata >> CAMPFIRE_AMOUNT_SHIFT) * CAMPFIRE_GOLD_MULTIPLIER
            );
            GiveResource(
                eventHero,
                cell->m_objectMetadata & CAMPFIRE_RESOURCE_MASK,
                cell->m_objectMetadata >> CAMPFIRE_AMOUNT_SHIFT
            );
            removeEvent = 1;
            gGame->m_mapSounds[m_mapOriginX + ADVMGR_VIEW_CENTER]
                              [m_mapOriginY + ADVMGR_VIEW_CENTER] = MAP_SOUND_NONE;
            break;
        case MAP_OBJECT_GAZEBO:
            if (!(eventHero->m_visitedSites & (1 << cell->m_objectMetadata))) {
                GiveExperience(eventHero, GAZEBO_EXPERIENCE, 1);
                eventHero->m_visitedSites |= 1 << cell->m_objectMetadata;
            }
            break;
        case MAP_OBJECT_WATERWHEEL:
            if (cell->m_objectMetadata) {
                GiveResource(
                    eventHero,
                    RESOURCE_GOLD,
                    cell->m_objectMetadata * WATERWHEEL_GOLD_MULTIPLIER
                );
                cell->m_objectMetadata = MAP_EVENT_DATA_EMPTY;
            }
            break;
        case MAP_OBJECT_RESOURCE:
            eventResource = cell->m_objectIndex - RESOURCE_PILE_OBJECT_BASE;
            GiveResource(
                eventHero,
                eventResource,
                eventResource == RESOURCE_GOLD
                    ? cell->m_objectMetadata * RESOURCE_PILE_GOLD_MULTIPLIER
                    : cell->m_objectMetadata
            );
            removeEvent = 1;
            break;
        case MAP_OBJECT_WINDMILL:
            if (cell->m_objectMetadata != WINDMILL_EMPTY) {
                GiveResource(eventHero, cell->m_objectMetadata, WINDMILL_RESOURCE_AMOUNT);
                cell->m_objectMetadata = WINDMILL_EMPTY;
            }
            break;
        case MAP_OBJECT_ANCIENT_LAMP:
            recruitType = CREATURE_GENIE;
            isFree = 0;
            goto recruit;
        case MAP_OBJECT_WAGON_CAMP:
            recruitType = CREATURE_ROGUE;
            isFree = 0;
            goto recruit;
        case MAP_OBJECT_DESERT_TENT:
            recruitType = CREATURE_NOMAD;
            isFree = 0;
            goto recruit;
        case MAP_OBJECT_STRAW_HUT:
            recruitType = CREATURE_GOBLIN;
            isFree = 1;
            goto recruit;
        case MAP_OBJECT_HOUSE:
            recruitType = CREATURE_PEASANT;
            isFree = 1;
            goto recruit;
        case MAP_OBJECT_CABIN:
            recruitType = CREATURE_ARCHER;
            isFree = 1;
            goto recruit;
        case MAP_OBJECT_DWARF_LOG_CABIN:
            recruitType = CREATURE_DWARF;
            isFree = 1;
            goto recruit;
        case MAP_OBJECT_PEASANT_LOG_CABIN:
            recruitType = CREATURE_PEASANT;
            isFree = 1;
            goto recruit;
        recruit:
            if (cell->m_objectMetadata) {
                gPhilAI->EvaluateOneTimeCreaturePurchase(
                    eventHero,
                    recruitType,
                    cell->m_objectMetadata,
                    isFree,
                    recruited,
                    purchaseValue,
                    freeSlot
                );
                if (recruited > 0) {
                    gGame->GiveArmy(&eventHero->m_army, recruitType, recruited, freeSlot);
                    cell->m_objectMetadata -= recruited;
                    if (!isFree) {
                        GetMonsterCost(recruitType, troopCost);
                        for (c = 0; c < RESOURCE_COUNT; c++)
                            gCurPlayerData->m_resources[c] -= recruited * troopCost[c];
                    }
                }
            }
            if (!cell->m_objectMetadata && eventType == MAP_OBJECT_ANCIENT_LAMP)
                removeEvent = 1;
            break;
        case MAP_OBJECT_MONSTER:
            ComputerMonsterInteract(cell, eventHero, &removeEvent);
            break;
        case MAP_OBJECT_OBELISK:
            if (!(gGame->m_obeliskVisitors[cell->m_objectMetadata - 1] & gCurPlayerBit)) {
                gGame->VisitObelisk(eventHero->m_owner);
                gGame->m_obeliskVisitors[cell->m_objectMetadata - 1] |= gCurPlayerBit;
            }
            break;
        case MAP_OBJECT_RANKING_SHRINE:
            break;
        case MAP_OBJECT_SPELL_SHRINE:
            if (eventHero->HasArtifact(ARTIFACT_MAGIC_BOOK))
                eventHero->AddSpell(
                    cell->m_objectMetadata - MAP_EVENT_SPELL_OFFSET,
                    eventHero->m_primaryStats[HERO_PRIMARY_KNOWLEDGE],
                    0
                );
            break;
        case MAP_OBJECT_TOWN:
            gPhilAI->TownEvent(cell, eventHero, x, y);
            break;
        case MAP_OBJECT_WHIRLPOOL:
            DoWhirlpool(eventHero);
        case MAP_OBJECT_STONE_LITHS:
            portalCount = 0;
            for (teleY = 0; teleY < MAP_CELL_GRID_SIZE; teleY++) {
                for (teleX = 0; teleX < MAP_CELL_GRID_SIZE; teleX++) {
                    if (gGame->m_map[teleX][teleY].m_triggerType
                            == static_cast<u8>(eventType | MAP_TRIGGER_EVENT)
                        && MANHATTAN_LENGTH(teleX - x, teleY - y)
                               > (eventType == MAP_OBJECT_STONE_LITHS ? STONE_LITHS_MIN_DISTANCE
                                                                      : WHIRLPOOL_MIN_DISTANCE))
                        portalCount++;
                }
            }
            if (portalCount >= 1) {
                if (portalCount > 1)
                    portalCount = Random(1, portalCount);
                for (teleY = 0; teleY < MAP_CELL_GRID_SIZE; teleY++) {
                    for (teleX = 0; teleX < MAP_CELL_GRID_SIZE; teleX++) {
                        if (gGame->m_map[teleX][teleY].m_triggerType
                                == static_cast<u8>(eventType | MAP_TRIGGER_EVENT)
                            && MANHATTAN_LENGTH(teleX - x, teleY - y)
                                   > (eventType == MAP_OBJECT_STONE_LITHS
                                          ? STONE_LITHS_MIN_DISTANCE
                                          : WHIRLPOOL_MIN_DISTANCE)) {
                            if (--portalCount <= 0)
                                goto teleport;
                        }
                    }
                }
            teleport:
                StopCursor(1);
                gAdvManager->TeleportTo(teleX, teleY, 0);
            }
            break;
        case MAP_OBJECT_ARTIFACT:
            switch (cell->m_objectMetadata) {
                case ARTIFACT_EVENT_MODE_PICKUP:
                giveArtifact:
                    GiveArtifact(eventHero, cell->m_objectIndex);
                    removeEvent = 1;
                    break;
                case ARTIFACT_EVENT_MODE_GUARDED:
                    c = ARTIFACT_EVENT_GUARD_ROGUE_COUNT;
                    if (gPhilAI->CombatMonsterEvent(eventHero, CREATURE_ROGUE, &c, cell))
                        goto giveArtifact;
                    break;
                case ARTIFACT_EVENT_MODE_GOLD:
                    if (gPhilAI->ChooseToBuyArtifact(
                            eventHero,
                            cell->m_objectIndex,
                            ARTIFACT_EVENT_GOLD_COST
                        )) {
                        gGame->m_players[eventHero->m_owner].m_resources[RESOURCE_GOLD] -=
                            ARTIFACT_EVENT_GOLD_COST;
                        goto giveArtifact;
                    } else {
                        removeEvent = 1;
                    }
                    break;
            }
            break;
        case MAP_OBJECT_HERO:
            opponent = gGame->GetHero(cell->m_objectMetadata);
            priorShowIt = gShowIt;
            if (opponent->m_owner == gCurPlayer)
                return;
            if (opponent->m_locationType == (MAP_TRIGGER_EVENT | MAP_OBJECT_TOWN))
                heroTown = gGame->GetTown(opponent->m_occupiedTown);
            if (!gHumanPlayer[opponent->m_owner]) {
                battleResult = gPhilAI->QuickCombat(
                    &eventHero->m_army,
                    eventHero,
                    &opponent->m_army,
                    opponent,
                    0,
                    0,
                    ourLosses,
                    theirLosses
                );
                if (battleResult && heroTown)
                    battleResult = gPhilAI->QuickCombat(
                        &eventHero->m_army,
                        eventHero,
                        &heroTown->m_army,
                        NULL,
                        1,
                        heroTown->m_id,
                        ourLosses,
                        theirLosses
                    );
            } else {
                if (heroTown)
                    heroTown->m_occupyingHeroId = opponent->m_id;
                res = DoCombat(
                    x,
                    y,
                    eventHero,
                    &eventHero->m_army,
                    heroTown,
                    opponent,
                    &opponent->m_army,
                    x,
                    y,
                    COMBAT_RANDOM_SEED_NEW,
                    1
                );
                if (res == COMBAT_RESULT_ATTACKER && heroTown)
                    gGame->ClaimTown(heroTown->m_id, gCurPlayer);
            }
            CompleteDraw(0);
            break;
        case MAP_OBJECT_SIGNPOST:
            break;
        case MAP_OBJECT_DAEMON_CAVE:
            switch (cell->m_objectMetadata) {
                case DAEMON_CAVE_EMPTY:
                    break;
                case DAEMON_REWARD_EXPERIENCE:
                    GiveExperience(eventHero, DAEMON_EXPERIENCE, 1);
                    break;
                case DAEMON_REWARD_ARTIFACT:
                    GiveExperience(eventHero, DAEMON_EXPERIENCE, 1);
                    GiveRandomArtifact(eventHero);
                    break;
                case DAEMON_REWARD_EXPERIENCE_GOLD:
                    GiveExperience(eventHero, DAEMON_EXPERIENCE, 1);
                    GiveResource(eventHero, RESOURCE_GOLD, DAEMON_GOLD);
                    break;
                case DAEMON_REWARD_RANSOM:
                    if (gGame->m_players[eventHero->m_owner].m_resources[RESOURCE_GOLD]
                        >= DAEMON_GOLD) {
                        if (gPhilAI->ChooseToPayRansomOnHero(eventHero, DAEMON_GOLD))
                            gGame->m_players[eventHero->m_owner].m_resources[RESOURCE_GOLD] +=
                                -DAEMON_GOLD;
                        else
                            HeroLoses(eventHero);
                    } else {
                        HeroLoses(eventHero);
                    }
                    break;
            }
            cell->m_objectMetadata = DAEMON_CAVE_EMPTY;
            break;
        case MAP_OBJECT_GRAVEYARD:
        case MAP_OBJECT_SHIPWRECK:
            gPhilAI->FightEvent(eventHero, cell);
            break;
        default:
            break;
    }
    if (removeEvent)
        EraseObj(cell, x, y);
    gCurPlayer = oldPlayer;
    gCurPlayerData = ownerPlayerData;
    CheckEndGame(0);
}

VA(0x00428ce2, 0x17d)
void advManager::PlayerMonsterInteract(
    class mapCell* cell,
    class mapCell* combatCell,
    class hero* eventHero,
    i8* handled,
    i32 x,
    i32 y,
    i8 unused,
    i32 combatX,
    i32 combatY
) {
    i32 result;

    unused = 0;
    if (cell->m_objectMetadata & MONSTER_WILLING_FLAG) {
        if (gPhilAI->FightValueOfStack(&eventHero->m_army, eventHero, 0)
            > (cell->m_objectMetadata & MONSTER_COUNT_MASK)
                  * gMonsterDatabase[cell->m_objectIndex].fightValue * 1.75) {
            if (eventHero->m_army.CanJoin(cell->m_objectIndex)) {
                sprintf(
                    gText,
                    gEventText[EVENT_TEXT_FOLLOWERS],
                    gArmyNamesPlural[cell->m_objectIndex]
                );
                EventWindow(
                    EVENT_TEXT_CUSTOM,
                    NORMAL_DIALOG_TYPE_YES_NO,
                    gText,
                    NORMAL_DIALOG_NO_RESOURCE,
                    0,
                    NORMAL_DIALOG_NO_RESOURCE,
                    0,
                    NORMAL_DIALOG_NO_OR_TEXT
                );
                if (gWindowManager->m_dialogResult == NORMAL_DIALOG_CONFIRM) {
                    eventHero->m_army.Add(
                        cell->m_objectIndex,
                        cell->m_objectMetadata & MONSTER_COUNT_MASK,
                        ARMY_GROUP_EMPTY_SLOT
                    );
                    *handled = 1;
                    return;
                } else {
                    EventWindow(
                        EVENT_TEXT_MONSTER_REFUSAL,
                        NORMAL_DIALOG_TYPE_OK,
                        "",
                        NORMAL_DIALOG_NO_RESOURCE,
                        0,
                        NORMAL_DIALOG_NO_RESOURCE,
                        0,
                        NORMAL_DIALOG_NO_OR_TEXT
                    );
                }
            }
        }
    }
    result = CombatMonsterEvent(
        eventHero,
        cell->m_objectIndex,
        cell->m_objectMetadata & MONSTER_COUNT_MASK,
        combatCell,
        x,
        y,
        unused,
        combatX,
        combatY
    );
    if (result == COMBAT_RESULT_ATTACKER || result == COMBAT_RESULT_DRAW)
        *handled = 1;
}

// HoMM1's computer heroes absorb a willing stack (bit 7) they outmatch by
// 7:4, otherwise fight it through philAI's quick combat.
VA(0x00428e5f, 0x139)
void advManager::ComputerMonsterInteract(class mapCell* cell, class hero* eventHero, i8* handled) {
    i32 numToBuy;
    i32 quantity;
    i32 bestSlot;
    i32 retVal;
    i32 creatureCountIdx;

    if (cell->m_objectMetadata & MONSTER_WILLING_FLAG
        && gPhilAI->FightValueOfStack(&eventHero->m_army, eventHero, 0)
               > (cell->m_objectMetadata & MONSTER_COUNT_MASK)
                     * gMonsterDatabase[cell->m_objectIndex].fightValue * 1.75) {
        gPhilAI->EvaluateOneTimeCreaturePurchase(
            eventHero,
            cell->m_objectIndex,
            cell->m_objectMetadata & MONSTER_COUNT_MASK,
            1,
            numToBuy,
            quantity,
            bestSlot
        );
        if (numToBuy > 0) {
            gGame->GiveArmy(
                &eventHero->m_army,
                cell->m_objectIndex,
                cell->m_objectMetadata & MONSTER_COUNT_MASK,
                bestSlot
            );
            *handled = 1;
        }
    } else {
        creatureCountIdx = cell->m_objectMetadata & MONSTER_COUNT_MASK;
        retVal =
            gPhilAI->CombatMonsterEvent(eventHero, cell->m_objectIndex, &creatureCountIdx, cell);
        cell->m_objectMetadata = (cell->m_objectMetadata & MONSTER_WILLING_FLAG) + creatureCountIdx;
        if (retVal)
            *handled = 1;
    }
}

VA(0x00428f98, 0x16f)
i32 advManager::DoNetCombat(char* packet) {
    hero* leader;
    i32 theCellY;
    i32 curPosX;
    i32 randSeed;
    i32 ourFoe;
    i8 res;
    i32 party;
    hero* nextAttacker;
    i32 curStartY;
    i32 sx;
    armyGroup* defendArmy;
    armyGroup* selAttArmyPtr;
    town* tempTown;
    i32 allReserved;
    i32 curUnused2;

    nextAttacker = NULL;
    selAttArmyPtr = NULL;
    tempTown = NULL;
    leader = NULL;
    defendArmy = NULL;
    ReceiveHeroTownData(
        packet,
        &ourFoe,
        &curPosX,
        &theCellY,
        &nextAttacker,
        &selAttArmyPtr,
        &tempTown,
        &leader,
        &defendArmy,
        &sx,
        &curStartY,
        &randSeed,
        &res,
        &gRetreatWin,
        &gCombatSurrender
    );
    party = nextAttacker->m_owner;
    res = DoCombat(
        curPosX,
        theCellY,
        nextAttacker,
        selAttArmyPtr,
        tempTown,
        leader,
        defendArmy,
        sx,
        curStartY,
        randSeed,
        0
    );
    if (!gHumanPlayer[party])
        SendHeroTownData(
            curPosX,
            theCellY,
            nextAttacker,
            selAttArmyPtr,
            tempTown,
            leader,
            defendArmy,
            sx,
            curStartY,
            randSeed,
            ourFoe,
            res,
            gRetreatWin,
            gCombatSurrender
        );
    if (selAttArmyPtr)
        free(selAttArmyPtr);
    if (defendArmy)
        free(defendArmy);
    if (tempTown)
        free(tempTown);
    if (leader)
        free(leader);
    if (nextAttacker)
        free(nextAttacker);
    gRetreatWin = 0;
    return 1;
}

VA(0x00429107, 0x584)
i32 advManager::DoCombat(
    i32 x,
    i32 y,
    class hero* firstHero,
    class armyGroup* firstArmy,
    class town* combatTown,
    class hero* secondHero,
    class armyGroup* secondArmy,
    i32 setupCombatX,
    i32 setupCombatY,
    i32 randomSeed,
    i8 processLosses
) {
    armyGroup* army2NetRec;
    hero* hero2NetItem;
    hero* hero1Net;
    armyGroup* army1NetRef;
    town* townNetItem;
    i32 senderNum;
    char* receivedPacket;
    i8 res;
    tag_message message;
    i32 curPlayer;
    i32 attackPlayer;
    i32 oldPlayer;
    i8 showItSaved;
    i32 unused;

    gInCombat = 1;
    attackPlayer = firstHero ? firstHero->m_owner : -1;
    if (secondHero)
        curPlayer = secondHero->m_owner;
    else if (combatTown)
        curPlayer = combatTown->m_owner;
    else
        curPlayer = GAME_PLAYER_NONE;
    if (randomSeed == COMBAT_RANDOM_SEED_NEW)
        randomSeed = Random(1, COMBAT_RANDOM_SEED_MAX);
    DemobilizeCurrHero();
    oldPlayer = gCurPlayer;
    showItSaved = gShowIt;

    if (attackPlayer >= 0 && curPlayer >= 0 && gHumanPlayer[curPlayer]) {
        if (!gThisNetHumanPlayer[curPlayer]) {
            SendHeroTownData(
                x,
                y,
                firstHero,
                firstArmy,
                combatTown,
                secondHero,
                secondArmy,
                setupCombatX,
                setupCombatY,
                randomSeed,
                curPlayer,
                0,
                0,
                0
            );
            if (!gHumanPlayer[attackPlayer]) {
                while (1) {
                    PollSound();
                    FillBitmapArea(
                        gWindowManager->m_screen,
                        COMBAT_NETWORK_POLL_X,
                        COMBAT_NETWORK_POLL_Y,
                        COMBAT_NETWORK_POLL_WIDTH,
                        COMBAT_NETWORK_POLL_HEIGHT,
                        0
                    );
                    receivedPacket = CheckHandleNet();
                    if (receivedPacket) {
                        switch (EVENTS_REMOTE_MESSAGE(receivedPacket)->command) {
                            case REMOTE_COMMAND_HERO_TOWN_DATA:
                                ReceiveHeroTownData(
                                    receivedPacket,
                                    &senderNum,
                                    &x,
                                    &y,
                                    &hero1Net,
                                    &army1NetRef,
                                    &townNetItem,
                                    &hero2NetItem,
                                    &army2NetRec,
                                    &setupCombatX,
                                    &setupCombatY,
                                    &randomSeed,
                                    &res,
                                    &gRetreatWin,
                                    &gCombatSurrender
                                );
                                if (army1NetRef) {
                                    memcpy(firstArmy, army1NetRef, sizeof(armyGroup));
                                    free(army1NetRef);
                                }
                                if (army2NetRec) {
                                    memcpy(secondArmy, army2NetRec, sizeof(armyGroup));
                                    free(army2NetRec);
                                }
                                if (townNetItem) {
                                    memcpy(combatTown, townNetItem, sizeof(town));
                                    free(townNetItem);
                                }
                                if (hero2NetItem) {
                                    memcpy(secondHero, hero2NetItem, sizeof(hero));
                                    free(hero2NetItem);
                                }
                                if (hero1Net) {
                                    memcpy(firstHero, hero1Net, sizeof(hero));
                                    free(hero1Net);
                                }
                                gCombatManager->m_combatResult = res;
                                goto combatFinished;
                        }
                    }
                    Process1WindowsMessage();
                    message = gInputManager->GetEvent();
                    CheckHandleNetPlayerWait(message, 1);
                }
            }
        } else if (!gThisNetHumanPlayer[attackPlayer]) {
            gShowIt = 1;
            gGame->TurnOffAIMusic();
            sprintf(
                gText,
                localization::Tr("combat.network.attacked"),
                gColorNames[gGame->m_players[curPlayer].m_color],
                combatTown ? localization::Tr("combat.network.town")
                           : localization::Tr("combat.network.hero")
            );
            gText[0] = CyrillicToUpper(gText[0]);
            gGame->WaitForPlayer(gText, curPlayer);
        }
    }

    gShowIt = 1;
    gCombatManager->SetupCombat(
        x,
        y,
        firstHero,
        firstArmy,
        combatTown,
        secondHero,
        secondArmy,
        x,
        y,
        randomSeed
    );
    if (gHighMemBuffer > COMBAT_HIGH_MEMORY_LIMIT)
        gAdvDisposeLevel = ADV_DISPOSE_FULL;
    else if (gHighMemBuffer > COMBAT_LOW_MEMORY_LIMIT)
        gAdvDisposeLevel = ADV_DISPOSE_PARTIAL;
    gExec->CallManager(gCombatManager);
    gAdvDisposeLevel = ADV_DISPOSE_NONE;

combatFinished:
    if (firstHero)
        firstHero->CheckLevel();
    if (secondHero)
        secondHero->CheckLevel();
    if (processLosses) {
        switch (gCombatManager->m_combatResult) {
            case COMBAT_RESULT_ATTACKER:
                if (!gRetreatWin)
                    TransferArtifacts(secondHero, firstHero);
                HeroLoses(secondHero);
                break;
            case COMBAT_RESULT_DEFENDER:
                if (!gRetreatWin)
                    TransferArtifacts(firstHero, secondHero);
                HeroLoses(firstHero);
                break;
            case COMBAT_RESULT_DRAW:
                HeroLoses(firstHero);
                HeroLoses(secondHero);
                break;
            case COMBAT_RESULT_PENDING:
                break;
        }
    }
    gShowIt = showItSaved;
    gCurPlayer = oldPlayer;
    if (!gHumanPlayer[gCurPlayer]) {
        gGame->ShowComputerScreen();
        gGame->TurnOnAIMusic();
        SetNoDialogMenus(0);
    } else {
        SetNoDialogMenus(1);
    }
    MobilizeCurrHero(0);
    if (processLosses)
        gRetreatWin = 0;
    gInCombat = 0;
    return gCombatManager->m_combatResult;
}

VA(0x0042968b, 0x282)
void advManager::SendHeroTownData(
    i32 x,
    i32 y,
    class hero* firstHero,
    class armyGroup* firstArmy,
    class town* combatTown,
    class hero* secondHero,
    class armyGroup* secondArmy,
    i32 setupCombatX,
    i32 setupCombatY,
    i32 randomSeed,
    i8 remotePlayer,
    i8 combatResult,
    i8 retreatWin,
    i8 combatSurrender
) {
    char* reply;
    i32 result;
    // One allocation carries the combat record, then each hero fragment.
    union {
        combatRemoteData* combat;
        combatRemoteHeroFragment* heroFragment;
        char* bytes;
    } buffer;

    buffer.combat = NULL;
    buffer.combat = static_cast<combatRemoteData*>(malloc(COMBAT_REMOTE_BUFFER_SIZE));
    reply = NULL;
    buffer.combat->fragment = COMBAT_REMOTE_FRAGMENT_COMBAT;
    buffer.combat->x = x;
    buffer.combat->y = y;
    buffer.combat->hasFirstHero = firstHero != NULL;
    buffer.combat->hasTown = combatTown != NULL;
    buffer.combat->hasSecondHero = secondHero != NULL;
    buffer.combat->setupCombatX = setupCombatX;
    buffer.combat->setupCombatY = setupCombatY;
    buffer.combat->randomSeed = randomSeed;
    buffer.combat->combatResult = combatResult;
    buffer.combat->retreatWin = retreatWin;
    buffer.combat->combatSurrender = combatSurrender;
    buffer.combat->firstOwner = firstHero ? firstHero->m_owner : -1;
    buffer.combat->firstGold =
        firstHero ? gGame->m_players[firstHero->m_owner].m_resources[RESOURCE_GOLD] : 0;
    buffer.combat->secondOwner = secondHero ? secondHero->m_owner : -1;
    buffer.combat->secondGold =
        secondHero ? gGame->m_players[secondHero->m_owner].m_resources[RESOURCE_GOLD] : 0;
    memcpy(&buffer.combat->firstArmy, firstArmy, sizeof(armyGroup));
    memcpy(&buffer.combat->secondArmy, secondArmy, sizeof(armyGroup));
    if (combatTown)
        memcpy(&buffer.combat->combatTown, combatTown, sizeof(town));

    // API-forced: TransmitAndWait/TransmitRemoteData take char* payloads.
    result = TransmitAndWait(
        buffer.bytes,
        remotePlayer,
        sizeof(combatRemoteData),
        REMOTE_COMMAND_HERO_TOWN_DATA,
        REMOTE_COMMAND_HERO_TOWN_CONFIRM,
        &reply
    );
    if (!result)
        ShutDown(NULL);

    if (firstHero) {
        buffer.heroFragment->fragment = COMBAT_REMOTE_FRAGMENT_FIRST_HERO;
        memcpy(buffer.heroFragment->data, firstHero, sizeof(hero));
        // API-forced: TransmitRemoteData takes a char* payload.
        result = TransmitRemoteData(
            buffer.bytes,
            remotePlayer,
            sizeof(combatRemoteHeroFragment),
            REMOTE_COMMAND_HERO_TOWN_DATA,
            1
        );
        if (!result)
            ShutDown(NULL);
    }
    if (secondHero) {
        buffer.heroFragment->fragment = COMBAT_REMOTE_FRAGMENT_SECOND_HERO;
        memcpy(buffer.heroFragment->data, secondHero, sizeof(hero));
        // API-forced: TransmitRemoteData takes a char* payload.
        result = TransmitRemoteData(
            buffer.bytes,
            remotePlayer,
            sizeof(combatRemoteHeroFragment),
            REMOTE_COMMAND_HERO_TOWN_DATA,
            1
        );
        if (!result)
            ShutDown(NULL);
    }
    free(buffer.combat);
}

VA(0x0042990d, 0x30f)
void advManager::ReceiveHeroTownData(
    char* packet,
    i32* remotePlayer,
    i32* x,
    i32* y,
    class hero** firstHero,
    class armyGroup** firstArmy,
    class town** combatTown,
    class hero** secondHero,
    class armyGroup** secondArmy,
    i32* setupCombatX,
    i32* setupCombatY,
    i32* randomSeed,
    i8* combatResult,
    i8* retreatWin,
    i8* combatSurrender
) {
    i8 hasTownOn;
    i32 mainResult;
    i32 lastPacketTimeNum;
    i8 firstOwner;
    i8 defenderOwner;
    i8 bFirstHero;
    i8 hasSecondHero;

    *firstHero = NULL;
    *firstArmy = NULL;
    *combatTown = NULL;
    *secondHero = NULL;
    *secondArmy = NULL;
    bFirstHero = hasSecondHero = hasTownOn = 0;
    *remotePlayer = EVENTS_REMOTE_MESSAGE(packet)->sender;
    *x = EVENTS_REMOTE_MESSAGE(packet)->combat.x;
    *y = EVENTS_REMOTE_MESSAGE(packet)->combat.y;
    bFirstHero = EVENTS_REMOTE_MESSAGE(packet)->combat.hasFirstHero;
    hasTownOn = EVENTS_REMOTE_MESSAGE(packet)->combat.hasTown;
    hasSecondHero = EVENTS_REMOTE_MESSAGE(packet)->combat.hasSecondHero;
    *setupCombatX = EVENTS_REMOTE_MESSAGE(packet)->combat.setupCombatX;
    *setupCombatY = EVENTS_REMOTE_MESSAGE(packet)->combat.setupCombatY;
    *randomSeed = EVENTS_REMOTE_MESSAGE(packet)->combat.randomSeed;
    *combatResult = EVENTS_REMOTE_MESSAGE(packet)->combat.combatResult;
    *retreatWin = EVENTS_REMOTE_MESSAGE(packet)->combat.retreatWin;
    *combatSurrender = EVENTS_REMOTE_MESSAGE(packet)->combat.combatSurrender;
    firstOwner = EVENTS_REMOTE_MESSAGE(packet)->combat.firstOwner;
    if (firstOwner > 0)
        gGame->m_players[firstOwner].m_resources[RESOURCE_GOLD] =
            EVENTS_REMOTE_MESSAGE(packet)->combat.firstGold;
    defenderOwner = EVENTS_REMOTE_MESSAGE(packet)->combat.secondOwner;
    if (defenderOwner > 0)
        gGame->m_players[defenderOwner].m_resources[RESOURCE_GOLD] =
            EVENTS_REMOTE_MESSAGE(packet)->combat.secondGold;

    *firstArmy = static_cast<armyGroup*>(malloc(sizeof(armyGroup)));
    memcpy(*firstArmy, &EVENTS_REMOTE_MESSAGE(packet)->combat.firstArmy, sizeof(armyGroup));
    *secondArmy = static_cast<armyGroup*>(malloc(sizeof(armyGroup)));
    memcpy(*secondArmy, &EVENTS_REMOTE_MESSAGE(packet)->combat.secondArmy, sizeof(armyGroup));
    if (hasTownOn) {
        *combatTown = static_cast<town*>(malloc(sizeof(town)));
        memcpy(*combatTown, &EVENTS_REMOTE_MESSAGE(packet)->combat.combatTown, sizeof(town));
    }

    mainResult = TransmitRemoteData(NULL, *remotePlayer, 0, REMOTE_COMMAND_HERO_TOWN_CONFIRM, 1);
    if (!mainResult)
        ShutDown(NULL);

    lastPacketTimeNum = KBTickCount();
    while ((hasSecondHero && !*secondHero) || (bFirstHero && !*firstHero)) {
        PollSound();
        if (lastPacketTimeNum + REMOTE_WAIT_TIMEOUT < KBTickCount()) {
            NormalDialog(
                localization::Tr("combat.network.receive_error"),
                NORMAL_DIALOG_TYPE_YES_NO
            );
            if (gWindowManager->m_dialogResult == NORMAL_DIALOG_CONFIRM)
                lastPacketTimeNum = KBTickCount();
            else
                ShutDown(localization::Tr("combat.network.canceled"));
        }
        packet = GetRemoteData(1);
        if (packet && EVENTS_REMOTE_MESSAGE(packet)->type == REMOTE_MESSAGE_RELIABLE
            && EVENTS_REMOTE_MESSAGE(packet)->command == REMOTE_COMMAND_HERO_TOWN_DATA) {
            lastPacketTimeNum = KBTickCount();
            if (EVENTS_REMOTE_HERO(packet)->heroFragment.fragment
                == COMBAT_REMOTE_FRAGMENT_FIRST_HERO) {
                *firstHero = static_cast<hero*>(malloc(sizeof(hero)));
                memcpy(*firstHero, EVENTS_REMOTE_HERO(packet)->heroFragment.data, sizeof(hero));
            }
            if (EVENTS_REMOTE_HERO(packet)->heroFragment.fragment
                == COMBAT_REMOTE_FRAGMENT_SECOND_HERO) {
                *secondHero = static_cast<hero*>(malloc(sizeof(hero)));
                memcpy(*secondHero, EVENTS_REMOTE_HERO(packet)->heroFragment.data, sizeof(hero));
            }
        }
    }
}

DATA(0x004a6acc)
i8 gEventMusicPlaying;

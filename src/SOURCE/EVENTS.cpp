#include <H1/Ints.h>

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

void advManager::DoEvent(class mapCell* cell, i32 x, i32 y) {
    hero* visitingHero;
    tag_message widgetEvent;
    i8 eventKind;
    tag_message unusedMessage;
    b8 removeObj;
    i32 fizzleEffect;
    i32 artifactId;
    i32 income;
    hero* opponent;
    i32 fightOutcome;
    i8 teleX;
    i8 teleY;
    char resourceText[20];
    i8 portalCount;
    i32 eventResource;
    heroWindow* thiefWindow;
    boatRecord* boat;
    b8 guardMonster;
    town* theirTown;
    i32 numTroops;

    visitingHero = gGame->GetHero(gCurPlayerData->m_currentHero);
    eventKind = MAP_TRIGGER_OBJECT(cell->m_triggerType);
    removeObj = false;
    fizzleEffect = EVENT_FIZZLE_KILL;
    gEventMusicPlaying = true;
    gMouseManager->ReallyHidePointer();
    EventSound(eventKind, cell->m_objectMetadata);
    switch (eventKind) {
        case MAP_OBJECT_COAST:
            if (visitingHero->IsEmbarked()) {
                visitingHero->m_eventFlags &= ~HERO_EVENT_EMBARKED;
                if (!gCheatUnlimitedMovement[gCurPlayer][visitingHero->m_id])
                    visitingHero->m_remainingMobility = 0;
                visitingHero->m_direction = m_cursorDirection;
                m_cursorType = visitingHero->m_heroClass;
                m_cursorFrame = GetCursorBaseFrame(m_cursorDirection);
                m_cursorActive = true;
                gWindowManager->SaveFizzleSource(
                    COAST_FIZZLE_X,
                    COAST_FIZZLE_Y,
                    COAST_FIZZLE_WIDTH,
                    COAST_FIZZLE_HEIGHT
                );
                CompleteDraw(m_mapOriginX, m_mapOriginY, false);
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
            if (!gCheatUnlimitedMovement[gCurPlayer][visitingHero->m_id])
                visitingHero->m_remainingMobility = 0;
            boat->heroId = visitingHero->m_id;
            boat->owner = visitingHero->m_owner;
            m_cursorType = ADVMGR_HERO_ICON_BOAT;
            m_cursorDirection = boat->direction;
            m_cursorFrame = GetCursorBaseFrame(m_cursorDirection);
            m_cursorActive = true;
            CompleteDraw(m_mapOriginX, m_mapOriginY, false);
            UpdateScreen(false, false);
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
                EVENT_TEXT_MINE_CAPTURED(gGame->m_mines[cell->m_objectMetadata].type),
                NORMAL_DIALOG_TYPE_OK,
                "",
                NORMAL_DIALOG_RESOURCE(gGame->m_mines[cell->m_objectMetadata].type),
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
                        false,
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
                    false
                );
            removeObj = true;
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
                visitingHero->m_morale += STATUE_MORALE_BONUS;
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
                    // With no free slot, or no artifact left, the hero is
                    // paid in gold.
                    artifactId = GiveRandomArtifact(cell, visitingHero);
                    if (artifactId == ARTIFACT_NONE) {
                        sprintf(gText, "%s.", localization::Tr("event.skeleton.treasure"));
                        EventWindow(
                            EVENT_TEXT_CUSTOM,
                            NORMAL_DIALOG_TYPE_OK,
                            gText,
                            NORMAL_DIALOG_RESOURCE_GOLD,
                            EVENT_RANDOM_ARTIFACT_GOLD,
                            NORMAL_DIALOG_NO_RESOURCE,
                            0,
                            NORMAL_DIALOG_NO_OR_TEXT
                        );
                    } else {
                        sprintf(
                            gText,
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
        case MAP_OBJECT_MEGALITH:
            switch (cell->m_objectMetadata) {
                case STRONGHOLD_GUARDED:
                    NormalDialog(
                        localization::Tr("event.stronghold.approach"),
                        NORMAL_DIALOG_TYPE_YES_NO
                    );
                    if (gWindowManager->m_dialogResult != NORMAL_DIALOG_CONFIRM)
                        break;
                    NormalDialog(
                        localization::Tr("event.stronghold.attacked"),
                        NORMAL_DIALOG_TYPE_OK
                    );
                    if (CombatMonsterEvent(
                            visitingHero,
                            CREATURE_PALADIN,
                            STRONGHOLD_PALADIN_COUNT,
                            cell,
                            x,
                            y,
                            1,
                            x,
                            y
                        )
                        != COMBAT_RESULT_DEFENDER)
                        break;
                    if (visitingHero->m_army.CanJoin(CREATURE_GHOST)) {
                        cell->m_objectMetadata = STRONGHOLD_ABANDONED;
                        visitingHero->m_army
                            .Add(CREATURE_GHOST, STRONGHOLD_GHOST_COUNT, ARMY_GROUP_EMPTY_SLOT);
                        RedrawAdvScreen(true);
                        NormalDialog(
                            localization::Tr("event.stronghold.ghosts_join"),
                            NORMAL_DIALOG_TYPE_OK
                        );
                    } else {
                        cell->m_objectMetadata = STRONGHOLD_GHOSTS_WAITING;
                        NormalDialog(
                            localization::Tr("event.stronghold.ghosts_wait"),
                            NORMAL_DIALOG_TYPE_OK
                        );
                    }
                    break;
                case STRONGHOLD_GHOSTS_WAITING:
                    if (visitingHero->m_army.CanJoin(CREATURE_GHOST)) {
                        cell->m_objectMetadata = STRONGHOLD_ABANDONED;
                        visitingHero->m_army
                            .Add(CREATURE_GHOST, STRONGHOLD_GHOST_COUNT, ARMY_GROUP_EMPTY_SLOT);
                        RedrawAdvScreen(true);
                        NormalDialog(
                            localization::Tr("event.stronghold.waiting_ghosts_join"),
                            NORMAL_DIALOG_TYPE_OK
                        );
                    } else {
                        NormalDialog(
                            localization::Tr("event.stronghold.ghosts_still_wait"),
                            NORMAL_DIALOG_TYPE_OK
                        );
                    }
                    break;
                default:
                    NormalDialog(
                        localization::Tr("event.stronghold.abandoned"),
                        NORMAL_DIALOG_TYPE_OK
                    );
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
                NORMAL_DIALOG_RESOURCE(
                    (cell->m_objectMetadata & CAMPFIRE_RESOURCE_MASK)
                ),
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
                (cell->m_objectMetadata & CAMPFIRE_RESOURCE_MASK),
                cell->m_objectMetadata >> CAMPFIRE_AMOUNT_SHIFT
            );
            removeObj = true;
            fizzleEffect = EVENT_FIZZLE_PICKUP;
            gGame->m_mapSounds[x][y] = MAP_SOUND_NONE;
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
                GiveExperience(visitingHero, GAZEBO_EXPERIENCE, false);
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
            eventResource =
                (cell->m_objectIndex - RESOURCE_PILE_OBJECT_BASE);
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
            removeObj = true;
            fizzleEffect = EVENT_FIZZLE_PICKUP;
            break;
        case MAP_OBJECT_WINDMILL:
            if (cell->m_objectMetadata <= WINDMILL_RESOURCE_LAST) {
                EventWindow(
                    EVENT_TEXT_WINDMILL_REWARD,
                    NORMAL_DIALOG_TYPE_OK,
                    "",
                    NORMAL_DIALOG_RESOURCE(cell->m_objectMetadata),
                    WINDMILL_RESOURCE_AMOUNT,
                    NORMAL_DIALOG_NO_RESOURCE,
                    0,
                    NORMAL_DIALOG_NO_OR_TEXT
                );
                GiveResource(
                    visitingHero,
                    cell->m_objectMetadata,
                    WINDMILL_RESOURCE_AMOUNT
                );
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
                    removeObj = true;
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
            PlayerMonsterInteract(cell, cell, visitingHero, &removeObj, x, y, false, x, y);
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
            gWindowManager->DoDialog(thiefWindow, TrueFalseDialogHandler, false);
            delete thiefWindow;
            RedrawAdvScreen(true);
            break;
        case MAP_OBJECT_SPELL_SHRINE:
            sprintf(
                gText,
                localization::Tr("event.shrine.spell_format"),
                gEventText[EVENT_TEXT_SPELL_SHRINE],
                gSpellNames
                    [(cell->m_objectMetadata - MAP_EVENT_SPELL_OFFSET)]
            );
            if (visitingHero->HasArtifact(ARTIFACT_MAGIC_BOOK)) {
                visitingHero->AddSpell(
                    (cell->m_objectMetadata - MAP_EVENT_SPELL_OFFSET),
                    visitingHero->m_primaryStats[HERO_PRIMARY_KNOWLEDGE],
                    false
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
                            == static_cast<u8>(MAP_EVENT_TRIGGER(eventKind))
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
                                == static_cast<u8>(MAP_EVENT_TRIGGER(eventKind))
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
                StopCursor(true);
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
                    removeObj = true;
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
                            false,
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
                        removeObj = true;
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
                if (opponent->m_locationType == MAP_EVENT_TRIGGER(MAP_OBJECT_TOWN)) {
                    theirTown = gGame->GetTown(opponent->m_locationMetadata);
                    theirTown->m_occupyingHeroId = opponent->m_id;
                }
                fightOutcome = DoCombat(
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
                    true
                );
                if (fightOutcome == COMBAT_RESULT_ATTACKER && theirTown)
                    gGame->ClaimTown(theirTown->m_id, gCurPlayer);
            }
            break;
        case MAP_OBJECT_SIGNPOST:
            gSearchArray->FindNearestObject(
                visitingHero->m_x,
                visitingHero->m_y,
                visitingHero->m_direction,
                SEARCH_NO_COST_LIMIT,
                MAP_EVENT_TRIGGER(MAP_OBJECT_TOWN)
            );
            if (GetCell(gSearchArray->m_specialTargetX, gSearchArray->m_specialTargetY)
                    ->m_triggerType
                == MAP_EVENT_TRIGGER(MAP_OBJECT_TOWN)) {
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
                    GiveExperience(visitingHero, DAEMON_EXPERIENCE, false);
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
                    GiveExperience(visitingHero, DAEMON_EXPERIENCE, false);
                    artifactId = GiveRandomArtifact(cell, visitingHero);
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
                    GiveExperience(visitingHero, DAEMON_EXPERIENCE, false);
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
                        if (GhostEvent(visitingHero, cell, EVENT_TEXT_SHIPWRECK_REWARD, x, y))
                            cell->m_objectMetadata = GHOST_SITE_EMPTY;
                        break;
                }
            }
            break;
        default:
            break;
    }
    UpdateRadar(true, false);
    UpdateHeroLocators(true, 1);
    UpdateTownLocators(true, 1);
    UpdBottomView(true, true, true);
    if (removeObj) {
        EraseObj(cell, x, y);
        FizzleCenter(fizzleEffect);
    } else {
        CompleteDraw(false);
    }
    UpdateScreen(false, false);
    PlayMusic(TERRAIN_MUSIC_TRACK(m_currentTerrain));
    gMouseManager->ReallyShowPointer();
    CheckEndGame(false);
}

void advManager::EraseObj(class mapCell* cell, i32 x, i32 y) {
    b8 erased = false;
    i32 j;
    i32 i;

    erased = true;
    cell->m_triggerType = MAP_OBJECT_TRIGGER(MAP_OBJECT_NONE);
    cell->m_objectIndex = MAP_CELL_NO_FRAME;
    if (cell->m_flags & MAP_CELL_OBJECT_ANIMATED)
        MAP_CELL_SUBTRACT_FLAG(cell->m_flags, MAP_CELL_OBJECT_ANIMATED);
    if ((cell->m_secondaryTrigger & MAP_TRIGGER_TYPE_MASK) > 0
        && (cell->m_secondaryTrigger & MAP_TRIGGER_TYPE_MASK) < 0x7f) {
        cell->m_triggerType = cell->m_secondaryTrigger & MAP_TRIGGER_TYPE_MASK;
        cell->m_secondaryTrigger -= cell->m_triggerType;
        for (i = x - 1; i <= x + 1; i++) {
            for (j = y - 1; j <= y + 1; j++) {
                if (MAP_CELL_IN_BOUNDS(i, j)
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

void advManager::HeroSwap(class hero* firstHero, class hero* secondHero) {
    swapManager* swapMgr;

    swapMgr = new swapManager(firstHero, secondHero);
    if (!swapMgr)
        MemError();
    gExec->DoDialog(swapMgr);
    delete swapMgr;
}

void advManager::TownEvent(class mapCell* cell, i32 x, i32 y) {
    hero* attackingHero;
    i32 combatOutcome;
    hero* defendingHero;
    town* eventTown;

    eventTown = gGame->GetTown(cell->m_objectMetadata);
    attackingHero = gGame->GetHero(gCurPlayerData->m_currentHero);
    DemobilizeCurrHero();
    if (eventTown->m_owner == gCurPlayer) {
        eventTown->m_occupyingHeroId = gCurPlayerData->CurrentHero();
        eventTown->View();
    } else if (eventTown->HasGarrison()) {
        defendingHero = eventTown->m_occupyingHeroId == TOWN_OCCUPYING_HERO_NONE
                            ? NULL
                            : gGame->GetHero(eventTown->m_occupyingHeroId);
        combatOutcome = DoCombat(
            x,
            y,
            attackingHero,
            &attackingHero->m_army,
            eventTown,
            defendingHero,
            &eventTown->m_army,
            x,
            y,
            COMBAT_RANDOM_SEED_NEW,
            true
        );
        if (combatOutcome == COMBAT_RESULT_ATTACKER)
            gGame->ClaimTown(eventTown->m_id, gCurPlayer);
    } else {
        gGame->ClaimTown(eventTown->m_id, gCurPlayer);
        UpdateRadar(true, false);
        UpdateHeroLocators(true, 1);
        UpdateTownLocators(true, 1);
        eventTown->m_occupyingHeroId = gCurPlayerData->CurrentHero();
        eventTown->View();
    }
    eventTown->GiveSpells();
    attackingHero->CheckLevel();
}

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
            if (eventData == ARTIFACT_EVENT_MODE_PICKUP)
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
            musicTrack = MUSIC_TRACK_DESERT_TENT;
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
        gEventMusicPlaying = true;
    } else {
        gEventMusicPlaying = false;
    }
}

void advManager::EventWindow(
    i16 eventId,
    i32 buttons,
    char* text,
    i32 type1,
    i32 value1,
    i32 type2,
    i32 value2,
    i32 showOrText
) {
    i32 newValue1;
    i32 unusedValue7;
    b32 stopOk;
    i32 unusedValue8;
    i32 unusedValue9;
    i32 curUnusedValue11;
    i32 unusedValue12Value;
    char eventText[EVENT_TEXT_BUFFER_SIZE];
    i16 newUnused;

    stopOk = false;
    GrabScreen();
    newUnused = 1;
    if (eventId >= EVENT_TEXT_FIRST && eventId < EVENT_TEXT_WINDOW_END)
        sprintf(eventText, gEventText[eventId]);
    else if (eventId == EVENT_TEXT_CUSTOM) {
        // The text is shown as it is, cut to the window's buffer.
        strncpy(eventText, text, sizeof(eventText) - 1);
        eventText[sizeof(eventText) - 1] = 0;
    }
    else
        sprintf(eventText, localization::Tr("event.unknown"), eventId);
    NormalDialog(
        eventText,
        buttons,
        NORMAL_DIALOG_ADVENTURE_X,
        NORMAL_DIALOG_AUTO_POSITION,
        type1,
        value1,
        type2,
        value2,
        showOrText
    );
}

i16 advManager::GiveArtifact(class hero* eventHero, i8 artifact) {
    i16 slot;

    for (slot = 0; slot < HERO_ARTIFACT_SLOT_COUNT; slot++) {
        if (eventHero->m_artifacts[slot] == ARTIFACT_NONE)
            break;
    }
    if (slot == HERO_ARTIFACT_SLOT_COUNT)
        return GIVE_ARTIFACT_NO_SLOT;
    eventHero->m_artifacts[slot] = artifact;
    if (artifact >= 0 && artifact < ARTIFACT_REGULAR_END)
        gGame->m_artifactHolders[artifact] = eventHero->m_id;
    GiveTakeArtifactStat(eventHero, artifact, EVENT_ARTIFACT_GIVE);
    return slot;
}

// A site's artifact; with none left, or no free slot for it, the hero takes
// gold instead and no artifact is returned.
i32 advManager::GiveRandomArtifact(class mapCell* cell, class hero* eventHero) {
    i8 artifact;

    artifact = gGame->CellRandomArtifactId(cell - &m_mapData[0][0]);
    if (artifact == ARTIFACT_NONE || eventHero->NumArtifacts() == HERO_ARTIFACT_SLOT_COUNT) {
        GiveResource(eventHero, RESOURCE_GOLD, EVENT_RANDOM_ARTIFACT_GOLD);
        return ARTIFACT_NONE;
    }
    GiveArtifact(eventHero, artifact);
    return artifact;
}

i32 advManager::GiveExperience(class hero* eventHero, i32 experience, b8 checkLevel) {
    i32 prevLevel;
    i32 unusedValue1;
    i32 unusedValue2;
    i32 levelNow;
    i32 levelGapVal;

    prevLevel = eventHero->GetLevel(eventHero->m_experience);
    eventHero->m_level = prevLevel;
    if (experience > 0)
        eventHero->m_experience += experience;
    levelNow = eventHero->GetLevel(eventHero->m_experience);
    if (checkLevel)
        eventHero->CheckLevel();
    return levelNow - prevLevel;
}

void advManager::GiveResource(
    class hero* eventHero,
    i8 resource,
    i16 amount
) {
    if (resource >= RESOURCE_FIRST && resource <= RESOURCE_LAST)
        gGame->m_players[eventHero->m_owner].m_resources[resource] += amount;
}

void advManager::RecruitEvent(
    class hero* eventHero,
    i32 creatureType,
    class mapCell* cell
) {
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

b8 advManager::GhostEvent(
    class hero* eventHero,
    class mapCell* cell,
    i32 textId,
    i32 x,
    i32 y
) {
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
                    false,
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
                return true;
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
                    false,
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
                return true;
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
                    false,
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
                return true;
            }
            break;
        default:
            if (CombatMonsterEvent(
                    eventHero,
                    CREATURE_GHOST,
                    GHOST_HUGE_COUNT,
                    cell,
                    x,
                    y,
                    false,
                    x,
                    y
                )
                == COMBAT_RESULT_ATTACKER) {
                artifact = GiveRandomArtifact(cell, eventHero);
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
                    // The artifact's gold comes with the site's own.
                    EventWindow(
                        EVENT_TEXT_CUSTOM,
                        NORMAL_DIALOG_TYPE_OK,
                        gText,
                        NORMAL_DIALOG_RESOURCE_GOLD,
                        GHOST_HUGE_GOLD + EVENT_RANDOM_ARTIFACT_GOLD,
                        NORMAL_DIALOG_NO_RESOURCE,
                        0,
                        NORMAL_DIALOG_NO_OR_TEXT
                    );
                GiveResource(eventHero, RESOURCE_GOLD, GHOST_HUGE_GOLD);
                eventHero->CheckLevel();
                return true;
            }
            break;
    }
    return false;
}

void advManager::HouseEvent(class hero* eventHero, class mapCell* cell) {
    i16 houseIndex;

    houseIndex = MAP_TRIGGER_OBJECT(cell->m_triggerType) - MAP_OBJECT_HOUSE_FIRST;
    if (!cell->m_objectMetadata) {
        EventWindow(
            EVENT_TEXT_HOUSE(houseIndex, EVENT_TEXT_HOUSE_EMPTY),
            NORMAL_DIALOG_TYPE_OK,
            "",
            NORMAL_DIALOG_NO_RESOURCE,
            0,
            NORMAL_DIALOG_NO_RESOURCE,
            0,
            NORMAL_DIALOG_NO_OR_TEXT
        );
    } else {
        i8
        creatures[EVENT_HOUSE_COUNT] =
            {CREATURE_GOBLIN, CREATURE_PEASANT, CREATURE_ARCHER, CREATURE_DWARF, CREATURE_PEASANT};

        EventWindow(
            EVENT_TEXT_HOUSE(houseIndex, EVENT_TEXT_HOUSE_RECRUIT),
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
                    EVENT_TEXT_HOUSE(houseIndex, EVENT_TEXT_HOUSE_RANKS_FULL),
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

i8 advManager::CombatMonsterEvent(
    class hero* eventHero,
    i8 monsterType,
    i16 monsterCount,
    class mapCell* cell,
    i32 x,
    i32 y,
    b8 heroDefends,
    i32 combatX,
    i32 combatY
) {
    i16 i;
    i32 combatRes;

    DemobilizeCurrHero();
    if (combatX == COMBAT_MONSTER_CELL_AT_EVENT) {
        combatX = x;
        combatY = y;
    } else {
        m_combatMonsterX = combatX;
        m_combatMonsterY = combatY;
        m_combatMonsterFacingLeft = eventHero->m_x < combatX;
        if (ComboDraw(false))
            UpdateScreen(false, false);
        m_combatMonsterX = COMBAT_MONSTER_CELL_CLEARED;
    }
    CLEAR_ARMY_GROUP(*gMonGroup);
    if (monsterCount / ARMY_GROUP_SLOT_COUNT > 0) {
        for (i = 0; i < ARMY_GROUP_SLOT_COUNT; i++) {
            gMonGroup->m_creatureTypes[i] = monsterType;
            gMonGroup->m_creatureCounts[i] = monsterCount / ARMY_GROUP_SLOT_COUNT;
        }
    }
    for (i = monsterCount % ARMY_GROUP_SLOT_COUNT - 1; i >= 0; i--) {
        gMonGroup->m_creatureTypes[i] = monsterType;
        gMonGroup->m_creatureCounts[i]++;
    }
    if (heroDefends)
        combatRes = DoCombat(
            combatX,
            combatY,
            NULL,
            gMonGroup,
            NULL,
            eventHero,
            &eventHero->m_army,
            x,
            y,
            COMBAT_RANDOM_SEED_NEW,
            true
        );
    else
        combatRes = DoCombat(
            combatX,
            combatY,
            eventHero,
            &eventHero->m_army,
            NULL,
            NULL,
            gMonGroup,
            x,
            y,
            COMBAT_RANDOM_SEED_NEW,
            true
        );
    MobilizeCurrHero(false);
    return combatRes;
}

void advManager::GiveTakeArtifactStat(
    class hero* targetHero,
    i8 artifact,
    i8 take
) {
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
            theStat = HERO_PRIMARY_SIEGE;
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
                        gGame->m_artifactHolders[sourceHero->m_artifacts[j]] = HERO_ID_NONE;
                    } else {
                        GiveTakeArtifactStat(
                            destHero,
                            sourceHero->m_artifacts[j],
                            EVENT_ARTIFACT_GIVE
                        );
                        destHero->m_artifacts[i] = sourceHero->m_artifacts[j];
                        gGame->m_artifactHolders[sourceHero->m_artifacts[j]] = destHero->m_id;
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

void advManager::HeroLoses(class hero* lostHero) {
    if (!lostHero)
        return;
    CompleteDraw(m_mapOriginX, m_mapOriginY, false);
    UpdateScreen(false, false);
    lostHero->Deallocate();
    FizzleCenter(EVENT_FIZZLE_KILL);
    UpdateRadar(true, false);
    UpdateHeroLocators(true, 1);
}

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
    selectedSlot = EVENT_WHIRLPOOL_NO_SLOT;
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

void advManager::FizzleCenter(i32 fizzleType) {
    class sample* fizzleSample;

    if (!gShowIt)
        return;
    switch (fizzleType) {
        case EVENT_FIZZLE_KILL:
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
    CompleteDraw(false);
    gWindowManager->FizzleForward(
        EVENT_FIZZLE_X,
        EVENT_FIZZLE_Y,
        EVENT_FIZZLE_WIDTH,
        EVENT_FIZZLE_HEIGHT,
        EVENT_FIZZLE_STEPS
    );
    WaitSample(fizzleSample);
}

void advManager::DoAIEvent(class mapCell* cell, class hero* eventHero, i32 x, i32 y) {
    i32 replacementSlot;
    town* heroTown;
    playerData* ownerPlayerData;
    i32 purchaseValue;
    b32 handled;
    i32 c;
    i32 recruited;
    i32 recruitType;
    b32 isFree;
    i32 oldPlayer;
    i32 eventWork[4];
    b8 removeEvent;
    hero* opponent;
    i32 fightOutcome;
    i32 battleResult;
    i8 eventType;
    i8 teleX;
    i32 worthFighting;
    i8 teleY;
    i32 armyStrength;
    i8 portalCount;
    i32 eventResource;
    b8 priorShowIt;
    boatRecord* boat;
    i32 success;
    b8 guardMonster;
    float theirLosses;
    float ourLosses;
    i32 troopCost[RESOURCE_COUNT];

    heroTown = NULL;
    eventType = MAP_TRIGGER_OBJECT(cell->m_triggerType);
    removeEvent = false;
    handled = false;
    oldPlayer = gCurPlayer;
    ownerPlayerData = gCurPlayerData;
    // Visiting costs the computer a movement point, unless the
    // SlightlyHarderAI option spares it.
    if (!gConfig.slightlyHarderAI)
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
                m_cursorActive = true;
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
            m_cursorActive = true;
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
                worthFighting,
                armyStrength
            );
            if (worthFighting) {
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
                    true
                );
            removeEvent = true;
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
                eventHero->m_morale += STATUE_MORALE_BONUS;
            }
            break;
        case MAP_OBJECT_SKELETON:
            switch (cell->m_objectMetadata) {
                case SKELETON_EMPTY:
                    break;
                case SKELETON_ARTIFACT:
                    GiveRandomArtifact(cell, eventHero);
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
                (cell->m_objectMetadata & CAMPFIRE_RESOURCE_MASK),
                cell->m_objectMetadata >> CAMPFIRE_AMOUNT_SHIFT
            );
            removeEvent = true;
            gGame->m_mapSounds[x][y] = MAP_SOUND_NONE;
            break;
        case MAP_OBJECT_GAZEBO:
            if (!(eventHero->m_visitedSites & (1 << cell->m_objectMetadata))) {
                GiveExperience(eventHero, GAZEBO_EXPERIENCE, true);
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
            eventResource =
                (cell->m_objectIndex - RESOURCE_PILE_OBJECT_BASE);
            GiveResource(
                eventHero,
                eventResource,
                eventResource == RESOURCE_GOLD
                    ? cell->m_objectMetadata * RESOURCE_PILE_GOLD_MULTIPLIER
                    : cell->m_objectMetadata
            );
            removeEvent = true;
            break;
        case MAP_OBJECT_WINDMILL:
            if (cell->m_objectMetadata != WINDMILL_EMPTY) {
                GiveResource(
                    eventHero,
                    cell->m_objectMetadata,
                    WINDMILL_RESOURCE_AMOUNT
                );
                cell->m_objectMetadata = WINDMILL_EMPTY;
            }
            break;
        case MAP_OBJECT_ANCIENT_LAMP:
            recruitType = CREATURE_GENIE;
            isFree = false;
            goto recruit;
        case MAP_OBJECT_WAGON_CAMP:
            recruitType = CREATURE_ROGUE;
            isFree = false;
            goto recruit;
        case MAP_OBJECT_DESERT_TENT:
            recruitType = CREATURE_NOMAD;
            isFree = false;
            goto recruit;
        case MAP_OBJECT_STRAW_HUT:
            recruitType = CREATURE_GOBLIN;
            isFree = true;
            goto recruit;
        case MAP_OBJECT_HOUSE:
            recruitType = CREATURE_PEASANT;
            isFree = true;
            goto recruit;
        case MAP_OBJECT_CABIN:
            recruitType = CREATURE_ARCHER;
            isFree = true;
            goto recruit;
        case MAP_OBJECT_DWARF_LOG_CABIN:
            recruitType = CREATURE_DWARF;
            isFree = true;
            goto recruit;
        case MAP_OBJECT_PEASANT_LOG_CABIN:
            recruitType = CREATURE_PEASANT;
            isFree = true;
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
                    replacementSlot
                );
                if (recruited > 0) {
                    gGame->GiveArmy(&eventHero->m_army, recruitType, recruited, replacementSlot);
                    cell->m_objectMetadata -= recruited;
                    if (!isFree) {
                        GetMonsterCost(recruitType, troopCost);
                        for (c = 0; c < RESOURCE_COUNT; c++)
                            gCurPlayerData->m_resources[c] -=
                                recruited * troopCost[c];
                    }
                }
            }
            if (!cell->m_objectMetadata && eventType == MAP_OBJECT_ANCIENT_LAMP)
                removeEvent = true;
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
                    (cell->m_objectMetadata - MAP_EVENT_SPELL_OFFSET),
                    eventHero->m_primaryStats[HERO_PRIMARY_KNOWLEDGE],
                    false
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
                            == static_cast<u8>(MAP_EVENT_TRIGGER(eventType))
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
                                == static_cast<u8>(MAP_EVENT_TRIGGER(eventType))
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
                StopCursor(true);
                gAdvManager->TeleportTo(teleX, teleY, 0);
            }
            break;
        case MAP_OBJECT_ARTIFACT:
            // As for a player's hero, a hero with every slot full leaves the
            // artifact where it lies.
            if (eventHero->NumArtifacts() == HERO_ARTIFACT_SLOT_COUNT)
                break;
            switch (cell->m_objectMetadata) {
                case ARTIFACT_EVENT_MODE_PICKUP:
                giveArtifact:
                    GiveArtifact(eventHero, cell->m_objectIndex);
                    removeEvent = true;
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
                        removeEvent = true;
                    }
                    break;
            }
            break;
        case MAP_OBJECT_HERO:
            opponent = gGame->GetHero(cell->m_objectMetadata);
            priorShowIt = gShowIt;
            if (opponent->m_owner == gCurPlayer)
                return;
            if (opponent->m_locationType == MAP_EVENT_TRIGGER(MAP_OBJECT_TOWN))
                heroTown = gGame->GetTown(opponent->m_locationMetadata);
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
                fightOutcome = DoCombat(
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
                    true
                );
                if (fightOutcome == COMBAT_RESULT_ATTACKER && heroTown)
                    gGame->ClaimTown(heroTown->m_id, gCurPlayer);
            }
            CompleteDraw(false);
            break;
        case MAP_OBJECT_SIGNPOST:
            break;
        case MAP_OBJECT_DAEMON_CAVE:
            switch (cell->m_objectMetadata) {
                case DAEMON_CAVE_EMPTY:
                    break;
                case DAEMON_REWARD_EXPERIENCE:
                    GiveExperience(eventHero, DAEMON_EXPERIENCE, true);
                    break;
                case DAEMON_REWARD_ARTIFACT:
                    GiveExperience(eventHero, DAEMON_EXPERIENCE, true);
                    // A hero with no free slot takes the gold instead.
                    if (eventHero->NumArtifacts() == HERO_ARTIFACT_SLOT_COUNT)
                        GiveResource(eventHero, RESOURCE_GOLD, DAEMON_GOLD);
                    else
                        GiveRandomArtifact(cell, eventHero);
                    break;
                case DAEMON_REWARD_EXPERIENCE_GOLD:
                    GiveExperience(eventHero, DAEMON_EXPERIENCE, true);
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
    CheckEndGame(false);
}

void advManager::PlayerMonsterInteract(
    class mapCell* cell,
    class mapCell* combatCell,
    class hero* eventHero,
    b8* removeMonsterObject,
    i32 x,
    i32 y,
    b8 heroDefends,
    i32 combatX,
    i32 combatY
) {
    i32 result;

    heroDefends = false;
    if (cell->m_objectMetadata & MONSTER_WILLING_FLAG) {
        if (gPhilAI->FightValueOfStack(&eventHero->m_army, eventHero, false)
            > (cell->m_objectMetadata & MONSTER_COUNT_MASK)
                  * gMonsterDatabase[cell->m_objectIndex].fightValue
                  * 1.75) {
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
                    *removeMonsterObject = true;
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
        heroDefends,
        combatX,
        combatY
    );
    if (result == COMBAT_RESULT_ATTACKER || result == COMBAT_RESULT_DRAW)
        *removeMonsterObject = true;
}

void advManager::ComputerMonsterInteract(
    class mapCell* cell,
    class hero* eventHero,
    b8* removeMonsterObject
) {
    i32 purchaseCount;
    i32 purchaseValue;
    i32 replacementSlot;
    i32 won;
    i32 monsterCount;

    if (cell->m_objectMetadata & MONSTER_WILLING_FLAG
        && gPhilAI->FightValueOfStack(&eventHero->m_army, eventHero, false)
               > (cell->m_objectMetadata & MONSTER_COUNT_MASK)
                     * gMonsterDatabase[cell->m_objectIndex]
                           .fightValue
                     * 1.75) {
        gPhilAI->EvaluateOneTimeCreaturePurchase(
            eventHero,
            cell->m_objectIndex,
            cell->m_objectMetadata & MONSTER_COUNT_MASK,
            true,
            purchaseCount,
            purchaseValue,
            replacementSlot
        );
        if (purchaseCount > 0) {
            gGame->GiveArmy(
                &eventHero->m_army,
                cell->m_objectIndex,
                cell->m_objectMetadata & MONSTER_COUNT_MASK,
                replacementSlot
            );
            *removeMonsterObject = true;
        }
    } else {
        monsterCount = cell->m_objectMetadata & MONSTER_COUNT_MASK;
        won = gPhilAI->CombatMonsterEvent(
            eventHero,
            cell->m_objectIndex,
            &monsterCount,
            cell
        );
        cell->m_objectMetadata = (cell->m_objectMetadata & MONSTER_WILLING_FLAG) + monsterCount;
        if (won)
            *removeMonsterObject = true;
    }
}

i32 advManager::DoNetCombat(RemoteMessage* packet) {
    hero* secondHero;
    i32 combatY;
    i32 combatX;
    i32 randSeed;
    i32 remotePlayer;
    i8 combatRes;
    i32 firstSide;
    hero* firstHero;
    i32 eventY;
    i32 eventX;
    armyGroup* secondArmy;
    armyGroup* firstArmy;
    town* battleTown;
    i32 allReserved;
    i32 curUnused2;

    firstHero = NULL;
    firstArmy = NULL;
    battleTown = NULL;
    secondHero = NULL;
    secondArmy = NULL;
    ReceiveHeroTownData(
        packet,
        &remotePlayer,
        &combatX,
        &combatY,
        &firstHero,
        &firstArmy,
        &battleTown,
        &secondHero,
        &secondArmy,
        &eventX,
        &eventY,
        &randSeed,
        &combatRes,
        &gRetreatWin,
        &gCombatSurrender
    );
    firstSide = firstHero->m_owner;
    combatRes = (DoCombat( combatX, combatY, firstHero, firstArmy, battleTown, secondHero, secondArmy, eventX, eventY, randSeed, false ));
    if (!gHumanPlayer[firstSide])
        SendHeroTownData(
            combatX,
            combatY,
            firstHero,
            firstArmy,
            battleTown,
            secondHero,
            secondArmy,
            eventX,
            eventY,
            randSeed,
            remotePlayer,
            combatRes,
            gRetreatWin,
            gCombatSurrender
        );
    if (firstArmy)
        free(firstArmy);
    if (secondArmy)
        free(secondArmy);
    if (battleTown)
        free(battleTown);
    if (secondHero)
        free(secondHero);
    if (firstHero)
        free(firstHero);
    gRetreatWin = false;
    gCombatSurrender = false;
    return 1;
}

i32 advManager::DoCombat(
    i32 x,
    i32 y,
    class hero* firstHero,
    class armyGroup* firstArmy,
    class town* combatTown,
    class hero* secondHero,
    class armyGroup* secondArmy,
    i32 eventX,
    i32 eventY,
    i32 randomSeed,
    b8 processLosses
) {
    armyGroup* receivedSecondArmy;
    hero* receivedSecondHero;
    hero* receivedFirstHero;
    armyGroup* receivedFirstArmy;
    town* receivedTown;
    i32 senderPlayer;
    RemoteMessage* receivedPacket;
    i8 combatRes;
    tag_message message;
    i32 defenderSide;
    i32 attackPlayer;
    i32 oldPlayer;
    b8 showItSaved;
    i32 unused;

    gInCombat = true;
    attackPlayer = firstHero ? firstHero->m_owner : -1;
    if (secondHero)
        defenderSide = secondHero->m_owner;
    else if (combatTown)
        defenderSide = combatTown->m_owner;
    else
        defenderSide = GAME_PLAYER_NONE;
    if (randomSeed == COMBAT_RANDOM_SEED_NEW)
        randomSeed = Random(1, COMBAT_RANDOM_SEED_MAX);
    DemobilizeCurrHero();
    oldPlayer = gCurPlayer;
    showItSaved = gShowIt;

    if (attackPlayer >= 0 && defenderSide >= 0 && gHumanPlayer[defenderSide]) {
        if (!gThisNetHumanPlayer[defenderSide]) {
            SendHeroTownData(
                x,
                y,
                firstHero,
                firstArmy,
                combatTown,
                secondHero,
                secondArmy,
                eventX,
                eventY,
                randomSeed,
                defenderSide,
                0,
                false,
                false
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
                        switch (receivedPacket->command) {
                            case REMOTE_COMMAND_HERO_TOWN_DATA:
                                ReceiveHeroTownData(
                                    receivedPacket,
                                    &senderPlayer,
                                    &x,
                                    &y,
                                    &receivedFirstHero,
                                    &receivedFirstArmy,
                                    &receivedTown,
                                    &receivedSecondHero,
                                    &receivedSecondArmy,
                                    &eventX,
                                    &eventY,
                                    &randomSeed,
                                    &combatRes,
                                    &gRetreatWin,
                                    &gCombatSurrender
                                );
                                if (receivedFirstArmy) {
                                    memcpy(firstArmy, receivedFirstArmy, sizeof(armyGroup));
                                    free(receivedFirstArmy);
                                }
                                if (receivedSecondArmy) {
                                    memcpy(secondArmy, receivedSecondArmy, sizeof(armyGroup));
                                    free(receivedSecondArmy);
                                }
                                if (receivedTown) {
                                    memcpy(combatTown, receivedTown, sizeof(town));
                                    free(receivedTown);
                                }
                                if (receivedSecondHero) {
                                    memcpy(secondHero, receivedSecondHero, sizeof(hero));
                                    free(receivedSecondHero);
                                }
                                if (receivedFirstHero) {
                                    memcpy(firstHero, receivedFirstHero, sizeof(hero));
                                    free(receivedFirstHero);
                                }
                                gCombatManager->m_combatResult =
                                    combatRes;
                                goto combatFinished;
                        }
                    }
                    Process1WindowsMessage();
                    message = gInputManager->GetEvent();
                    CheckHandleNetPlayerWait(message, true);
                }
            }
        } else if (!gThisNetHumanPlayer[attackPlayer]) {
            gShowIt = true;
            gGame->TurnOffAIMusic();
            sprintf(
                gText,
                localization::Tr("combat.network.attacked"),
                gColorNames[gGame->m_players[defenderSide].m_color],
                combatTown ? localization::Tr("combat.network.town")
                           : localization::Tr("combat.network.hero")
            );
            gText[0] = CyrillicToUpper(gText[0]);
            gGame->WaitForPlayer(gText, defenderSide);
        }
    }

    gShowIt = true;
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
    MobilizeCurrHero(false);
    if (processLosses) {
        gRetreatWin = false;
        gCombatSurrender = false;
    }
    gInCombat = false;
    return gCombatManager->m_combatResult;
}

void advManager::SendHeroTownData(
    i32 x,
    i32 y,
    class hero* firstHero,
    class armyGroup* firstArmy,
    class town* combatTown,
    class hero* secondHero,
    class armyGroup* secondArmy,
    i32 eventX,
    i32 eventY,
    i32 randomSeed,
    i8 remotePlayer,
    i8 combatResult,
    b8 retreatWin,
    b8 combatSurrender
) {
    RemoteMessage* reply;
    i32 result;
    union {
        combatRemoteData* combat;
        combatRemoteHeroFragment* heroFragment;
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
    buffer.combat->eventX = eventX;
    buffer.combat->eventY = eventY;
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

    result = TransmitAndWait(
        buffer.combat,
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
        result = TransmitRemoteData(
            buffer.heroFragment,
            remotePlayer,
            sizeof(combatRemoteHeroFragment),
            REMOTE_COMMAND_HERO_TOWN_DATA,
            true
        );
        if (!result)
            ShutDown(NULL);
    }
    if (secondHero) {
        buffer.heroFragment->fragment = COMBAT_REMOTE_FRAGMENT_SECOND_HERO;
        memcpy(buffer.heroFragment->data, secondHero, sizeof(hero));
        result = TransmitRemoteData(
            buffer.heroFragment,
            remotePlayer,
            sizeof(combatRemoteHeroFragment),
            REMOTE_COMMAND_HERO_TOWN_DATA,
            true
        );
        if (!result)
            ShutDown(NULL);
    }
    free(buffer.combat);
}

void advManager::ReceiveHeroTownData(
    RemoteMessage* packet,
    i32* remotePlayer,
    i32* x,
    i32* y,
    class hero** firstHero,
    class armyGroup** firstArmy,
    class town** combatTown,
    class hero** secondHero,
    class armyGroup** secondArmy,
    i32* eventX,
    i32* eventY,
    i32* randomSeed,
    i8* combatResult,
    b8* retreatWin,
    b8* combatSurrender
) {
    b8 hasTownOn;
    i32 confirmSent;
    i32 lastReceiveTick;
    i8 firstOwner;
    i8 secondOwner;
    b8 firstHeroIncluded;
    b8 hasSecondHero;

    *firstHero = NULL;
    *firstArmy = NULL;
    *combatTown = NULL;
    *secondHero = NULL;
    *secondArmy = NULL;
    firstHeroIncluded = hasSecondHero = hasTownOn = false;
    *remotePlayer = packet->sender;
    *x = EVENTS_REMOTE_MESSAGE(packet)->combat.x;
    *y = EVENTS_REMOTE_MESSAGE(packet)->combat.y;
    firstHeroIncluded = EVENTS_REMOTE_MESSAGE(packet)->combat.hasFirstHero;
    hasTownOn = EVENTS_REMOTE_MESSAGE(packet)->combat.hasTown;
    hasSecondHero = EVENTS_REMOTE_MESSAGE(packet)->combat.hasSecondHero;
    *eventX = EVENTS_REMOTE_MESSAGE(packet)->combat.eventX;
    *eventY = EVENTS_REMOTE_MESSAGE(packet)->combat.eventY;
    *randomSeed = EVENTS_REMOTE_MESSAGE(packet)->combat.randomSeed;
    *combatResult = EVENTS_REMOTE_MESSAGE(packet)->combat.combatResult;
    *retreatWin = EVENTS_REMOTE_MESSAGE(packet)->combat.retreatWin;
    *combatSurrender = EVENTS_REMOTE_MESSAGE(packet)->combat.combatSurrender;
    firstOwner = EVENTS_REMOTE_MESSAGE(packet)->combat.firstOwner;
    if (firstOwner >= 0 && firstOwner < GAME_PLAYER_COUNT)
        gGame->m_players[firstOwner].m_resources[RESOURCE_GOLD] =
            EVENTS_REMOTE_MESSAGE(packet)->combat.firstGold;
    secondOwner = EVENTS_REMOTE_MESSAGE(packet)->combat.secondOwner;
    if (secondOwner >= 0 && secondOwner < GAME_PLAYER_COUNT)
        gGame->m_players[secondOwner].m_resources[RESOURCE_GOLD] =
            EVENTS_REMOTE_MESSAGE(packet)->combat.secondGold;

    *firstArmy = static_cast<armyGroup*>(malloc(sizeof(armyGroup)));
    memcpy(*firstArmy, &EVENTS_REMOTE_MESSAGE(packet)->combat.firstArmy, sizeof(armyGroup));
    *secondArmy = static_cast<armyGroup*>(malloc(sizeof(armyGroup)));
    memcpy(*secondArmy, &EVENTS_REMOTE_MESSAGE(packet)->combat.secondArmy, sizeof(armyGroup));
    if (hasTownOn) {
        *combatTown = static_cast<town*>(malloc(sizeof(town)));
        memcpy(*combatTown, &EVENTS_REMOTE_MESSAGE(packet)->combat.combatTown, sizeof(town));
    }

    confirmSent =
        TransmitRemoteData(NULL, *remotePlayer, 0, REMOTE_COMMAND_HERO_TOWN_CONFIRM, true);
    if (!confirmSent)
        ShutDown(NULL);

    lastReceiveTick = KBTickCount();
    while ((hasSecondHero && !*secondHero) || (firstHeroIncluded && !*firstHero)) {
        PollSound();
        if (lastReceiveTick + REMOTE_WAIT_TIMEOUT < KBTickCount()) {
            NormalDialog(
                localization::Tr("combat.network.receive_error"),
                NORMAL_DIALOG_TYPE_YES_NO
            );
            if (gWindowManager->m_dialogResult == NORMAL_DIALOG_CONFIRM)
                lastReceiveTick = KBTickCount();
            else
                ShutDown(localization::Tr("combat.network.canceled"));
        }
        packet = GetRemoteData(true);
        if (packet && packet->type == REMOTE_MESSAGE_RELIABLE
            && packet->command == REMOTE_COMMAND_HERO_TOWN_DATA) {
            lastReceiveTick = KBTickCount();
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

b8 gEventMusicPlaying;

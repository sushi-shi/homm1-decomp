// Located from HoMM2 Buka 2.1; PoL 2.0 supplies the VC4 declaration.

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

// advManager::EventWindow's eventId: the gEventText row it prints, or
// EVENT_TEXT_CUSTOM for caller text (Buka 2.1 EVENTS.h MapEventTextId; HoMM1
// numbers its own rows). The five houses use RECRUIT/RANKS_FULL/EMPTY of the
// first house plus three rows per house.
H1_ENUM_BEGIN(MapEventTextId)
    EVENT_TEXT_CUSTOM = -1,
    EVENT_TEXT_ALCHEMIST_CAPTURED = 0,
    EVENT_TEXT_SIGNPOST = 1,
    EVENT_TEXT_BUOY_VISITED = 2,
    EVENT_TEXT_BUOY_REWARD = 3,
    EVENT_TEXT_DAEMON_CAVE_EMPTY = 4,
    EVENT_TEXT_DAEMON_CAVE_EXPERIENCE = 5,
    EVENT_TEXT_DAEMON_CAVE_ARTIFACT = 6,
    EVENT_TEXT_DAEMON_CAVE_GOLD = 7,
    EVENT_TEXT_DAEMON_CAVE_RANSOM = 8,
    EVENT_TEXT_DAEMON_CAVE_DEATH = 9,
    EVENT_TEXT_DAEMON_CAVE_PROMPT = 10,
    EVENT_TEXT_TREASURE_CHEST = 11,
    EVENT_TEXT_FAERIE_RING_VISITED = 12,
    EVENT_TEXT_FAERIE_RING_REWARD = 13,
    EVENT_TEXT_CAMPFIRE = 14,
    EVENT_TEXT_FOUNTAIN_VISITED = 15,
    EVENT_TEXT_FOUNTAIN_REWARD = 16,
    EVENT_TEXT_GAZEBO_VISITED = 17,
    EVENT_TEXT_GAZEBO_REWARD = 18,
    EVENT_TEXT_GENIE_LAMP = 19,
    EVENT_TEXT_GRAVEYARD_PROMPT = 20,
    EVENT_TEXT_GRAVEYARD_EMPTY = 21,
    EVENT_TEXT_GRAVEYARD_REWARD = 22,
    EVENT_TEXT_HOUSE_RECRUIT = 23,
    EVENT_TEXT_HOUSE_RANKS_FULL = 24,
    EVENT_TEXT_HOUSE_EMPTY = 25,
    EVENT_TEXT_DRAGON_CITY_PROMPT = 38,
    EVENT_TEXT_DRAGON_CITY_CONQUERED = 39,
    EVENT_TEXT_LIGHTHOUSE_CAPTURED = 40,
    EVENT_TEXT_WATERWHEEL_EMPTY = 41,
    // Mine of resource r: MINE_CAPTURED_BASE + r (ore 43 .. gold 47).
    EVENT_TEXT_MINE_CAPTURED_BASE = 41,
    EVENT_TEXT_WATERWHEEL_REWARD = 42,
    EVENT_TEXT_FOLLOWERS = 48,
    EVENT_TEXT_MONSTER_REFUSAL = 49,
    EVENT_TEXT_OBELISK_REWARD = 50,
    EVENT_TEXT_OBELISK_VISITED = 51,
    EVENT_TEXT_OASIS_VISITED = 52,
    EVENT_TEXT_OASIS_REWARD = 53,
    EVENT_TEXT_RESOURCE_PICKUP = 54,
    EVENT_TEXT_SAWMILL_CAPTURED = 55,
    EVENT_TEXT_RANKING_SHRINE = 56,
    EVENT_TEXT_SPELL_SHRINE = 57,
    EVENT_TEXT_SHIPWRECK_PROMPT = 58,
    EVENT_TEXT_SHIPWRECK_EMPTY = 59,
    EVENT_TEXT_SHIPWRECK_REWARD = 60,
    EVENT_TEXT_STATUE_REWARD = 61,
    EVENT_TEXT_STATUE_VISITED = 62,
    EVENT_TEXT_DESERT_TENT_EMPTY = 63,
    EVENT_TEXT_DESERT_TENT_RECRUIT = 64,
    EVENT_TEXT_WAGON_EMPTY = 65,
    EVENT_TEXT_WAGON_RECRUIT = 66,
    EVENT_TEXT_WINDMILL_EMPTY = 68,
    EVENT_TEXT_WINDMILL_REWARD = 69,
    EVENT_TEXT_ARTIFACT_GUARDED = 70,
    EVENT_TEXT_LEPRECHAUN_OFFER = 71,
    EVENT_TEXT_LEPRECHAUN_REFUSAL = 72,
    EVENT_TEXT_LEPRECHAUN_NO_GOLD = 73,
    EVENT_TEXT_ARTIFACT_RECOVERED = 74,
    EVENT_TEXT_SKELETON_EMPTY = 75,
    EVENT_TEXT_SKELETON_ARTIFACT = 76
H1_ENUM_END(MapEventTextId)

// HouseEvent's five recruiting houses (straw hut .. ): three gEventText rows
// each and one creature each.
H1_ENUM_CONST_BEGIN(HouseEventConstant)
    EVENT_TEXT_HOUSE_STRIDE = 3,
    EVENT_HOUSE_COUNT = 5
H1_ENUM_CONST_END(HouseEventConstant)

// @early-stop 99.45: m_mapSounds[m_mapOriginX + 7][m_mapOriginY + 7] in
// the campfire arm - retail adds the column term first. As in EraseObj,
// the column mul (one add below it) is lighter than the row's (x 72) for
// every handle state and no authentic spelling is known; one more
// code-free node on the column reproduces retail (99.69 with `+ 0`). The
// abs(tx - x) + abs(ty - y) operand orders are handle state (solver:
// whole-TU shift T=58 leaves only this site, 4 lines).
// donor PoL RVA 0x000a8530; preferred Buka symbol ?DoEvent@advManager@@QAEXPAVmapCell@@HH@Z
// donor Buka TU SOURCE/EVENTS; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.400929;margin=0.083641;shape=0.269;size=0.325;calls=0.342;strings=%s %s|thiefwin.bin;alternate=pol20:void advManager::DoEvent(class mapCell *, int, int)@0x000a8530
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

    visitingHero = gpGame->GetHero(gpCurPlayer->m_currentHero);
    eventKind = cell->m_triggerType & MAP_TRIGGER_TYPE_MASK;
    removeObj = 0;
    fizzleEffect = EVENT_FIZZLE_HERO_LOSS;
    gEventMusicPlaying = 1;
    gpMouseManager->ReallyHidePointer();
    EventSound(eventKind, cell->m_objectMetadata);
    switch (eventKind) {
        case MAP_OBJECT_COAST:
            if (visitingHero->m_eventFlags & HERO_EVENT_EMBARKED) {
                visitingHero->m_eventFlags &= ~HERO_EVENT_EMBARKED;
                visitingHero->m_remainingMobility = 0;
                visitingHero->m_direction = m_cursorDirection;
                m_cursorType = visitingHero->m_heroClass;
                m_cursorFrame = GetCursorBaseFrame(m_cursorDirection);
                m_cursorActive = 1;
                gpWindowManager->SaveFizzleSource(
                    COAST_FIZZLE_X,
                    COAST_FIZZLE_Y,
                    COAST_FIZZLE_WIDTH,
                    COAST_FIZZLE_HEIGHT
                );
                CompleteDraw(m_mapOriginX, m_mapOriginY, 0);
                gpWindowManager->FizzleForward(
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
            boat = &gpGame->m_boats[cell->m_objectMetadata];
            gpGame->RestoreCell(-1, -1, boat->savedTriggerType, boat->savedEventData, cell, 2);
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
            if (gpGame->m_mineOwners[cell->m_objectMetadata] == giCurPlayer)
                break;
            if (gpGame->m_mines[cell->m_objectMetadata].type == RESOURCE_GOLD)
                income = MINE_GOLD_INCOME;
            else if (gpGame->m_mines[cell->m_objectMetadata].type == RESOURCE_ORE)
                income = MINE_ORE_INCOME;
            else
                income = MINE_RARE_INCOME;
            EventWindow(
                gpGame->m_mines[cell->m_objectMetadata].type + EVENT_TEXT_MINE_CAPTURED_BASE,
                NORMAL_DIALOG_TYPE_OK,
                "",
                gpGame->m_mines[cell->m_objectMetadata].type,
                -income,
                NORMAL_DIALOG_NO_RESOURCE,
                0,
                NORMAL_DIALOG_NO_OR_TEXT
            );
            goto claimMine;
        case MAP_OBJECT_ALCHEMIST_LAB:
            if (gpGame->m_mineOwners[cell->m_objectMetadata] == giCurPlayer)
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
            if (gpGame->m_mineOwners[cell->m_objectMetadata] == giCurPlayer)
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
            gpGame->ClaimMine(cell->m_objectMetadata, giCurPlayer);
            break;
        case MAP_OBJECT_LIGHTHOUSE:
            if (gpGame->m_mineOwners[MINE_SLOT_LIGHTHOUSE] == giCurPlayer)
                break;
            gpGame->ClaimMine(MINE_SLOT_LIGHTHOUSE, giCurPlayer);
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
            if (gpGame->m_mineOwners[MINE_SLOT_DRAGON_CITY] == giCurPlayer)
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
            if (gpWindowManager->m_dialogResult == NORMAL_DIALOG_CONFIRM) {
                if (gpGame->m_campaignType > 0
                    && gpGame->m_campaignScenario == DRAGON_CITY_CAMPAIGN_SCENARIO)
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
                    gpGame->ClaimMine(MINE_SLOT_DRAGON_CITY, giCurPlayer);
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
            if (gpWindowManager->m_dialogResult == NORMAL_DIALOG_CONFIRM)
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
            gpGame->m_mapSounds[m_mapOriginX + ENVIRONMENT_BORDER]
                               [m_mapOriginY + ENVIRONMENT_BORDER] = MAP_SOUND_NONE;
            SetEnvironmentOrigin(
                m_mapOriginX + ENVIRONMENT_BORDER,
                m_mapOriginY + ENVIRONMENT_BORDER,
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
            if (gpWindowManager->m_dialogResult == NORMAL_DIALOG_CONFIRM) {
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
                if (gpWindowManager->m_dialogResult == NORMAL_DIALOG_CONFIRM)
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
                if (gpWindowManager->m_dialogResult == NORMAL_DIALOG_CONFIRM)
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
            if (!(gpGame->m_obeliskVisitors[cell->m_objectMetadata - 1]
                  & (1 << visitingHero->m_owner))) {
                gpGame->VisitObelisk(visitingHero->m_owner);
                gpGame->m_obeliskVisitors[cell->m_objectMetadata - 1] |= 1 << visitingHero->m_owner;
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
            gpMouseManager->SetPointer(ADVENTURE_POINTER_DEFAULT);
            thiefWindow = new heroWindow(0, 0, "thiefwin.bin");
            if (!thiefWindow)
                MemError();
            SetWinText(thiefWindow, WINDOW_TEXT_THIEVES_GUILD);
            gpTownManager->SetupThievesGuild(thiefWindow, THIEVES_CATEGORY_COUNT);
            strcpy(gText, localization::Tr("event.shrine.rankings"));
            SET_WIDGET_MESSAGE(widgetEvent, WIDGET_COMMAND_SET_TEXT, 0);
            widgetEvent.text = gText;
            thiefWindow->BroadcastMessage(widgetEvent);
            gpWindowManager->DoDialog(thiefWindow, TrueFalseDialogHandler, 0);
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
                    if (gpGame->m_map[teleX][teleY].m_triggerType
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
                        if (gpGame->m_map[teleX][teleY].m_triggerType
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
                gpAdvManager->TeleportTo(teleX, teleY, 1);
            }
            break;
        case MAP_OBJECT_ARTIFACT:
            if (visitingHero->NumArtifacts() == HERO_ARTIFACT_SLOT_COUNT) {
                NormalDialog(
                    localization::Tr("event.artifact.full"),
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
            extern char* gArtifactEvent[];
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
                    if (gpWindowManager->m_dialogResult == NORMAL_DIALOG_CONFIRM) {
                        if (gpGame->m_players[visitingHero->m_owner].m_resources[RESOURCE_GOLD]
                            >= ARTIFACT_EVENT_GOLD_COST) {
                            gpGame->m_players[visitingHero->m_owner].m_resources[RESOURCE_GOLD] -=
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
            opponent = gpGame->GetHero(cell->m_objectMetadata);
            if (opponent->m_owner == giCurPlayer) {
                HeroSwap(visitingHero, opponent);
            } else {
                theirTown = NULL;
                if (opponent->m_locationType == (MAP_TRIGGER_EVENT | MAP_OBJECT_TOWN)) {
                    theirTown = gpGame->GetTown(opponent->m_occupiedTown);
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
                    gpGame->ClaimTown(theirTown->m_id, giCurPlayer);
            }
            break;
        case MAP_OBJECT_SIGNPOST:
            gpSearchArray->FindNearestObject(
                visitingHero->m_x,
                visitingHero->m_y,
                visitingHero->m_direction,
                SEARCH_NO_COST_LIMIT,
                MAP_TRIGGER_EVENT | MAP_OBJECT_TOWN
            );
            if (GetCell(gpSearchArray->m_specialTargetX, gpSearchArray->m_specialTargetY)
                    ->m_triggerType
                == (MAP_TRIGGER_EVENT | MAP_OBJECT_TOWN)) {
                sprintf(
                    gText,
                    gEventText[EVENT_TEXT_SIGNPOST],
                    GetTownName(gpGame->GetTownId(
                        gpSearchArray->m_specialTargetX,
                        gpSearchArray->m_specialTargetY
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
            if (gpWindowManager->m_dialogResult == NORMAL_DIALOG_CANCEL)
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
                    if (gpGame->GetRandomArtifactId() == ARTIFACT_NONE)
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
                    if (gpWindowManager->m_dialogResult == NORMAL_DIALOG_CONFIRM) {
                        if (gpGame->m_players[visitingHero->m_owner].m_resources[RESOURCE_GOLD]
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
                            gpGame->m_players[visitingHero->m_owner].m_resources[RESOURCE_GOLD] -=
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
            if (gpWindowManager->m_dialogResult == NORMAL_DIALOG_CONFIRM) {
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
            if (gpWindowManager->m_dialogResult == NORMAL_DIALOG_CONFIRM) {
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
    gpMouseManager->ReallyShowPointer();
    CheckEndGame(0);
}

// @early-stop 97.31: m_mapSounds[x][y] (test and store) - retail adds the
// column term first (mov eax,y; ecx=x*72; add eax,ecx). sortnode model:
// the column mul 4(load y, 1) weighs 0x47 against the row's 4(load x, 72)
// 0x53, so VC4 emits the row first for every handle state (x/y/gpGame/
// locals, whole-TU and per-function shifts, all slot-preserving local
// orders: replay and compiles). Retail needs one extra code-free node on the
// column (a `y + 0` compile is exact). Callers pass full ints; casts,
// pointer/1-D spellings and an inline accessor do not add it; a 1-byte
// element or row struct for game::m_mapSounds flips this site but not
// DoEvent's and shifts every game.h TU's handles, so no authentic
// construct is known.
// Buka advManager::EraseObj reduced to HoMM1's single-cell layers: the cell
// falls back to the trigger kept in the low seven bits of byte 7 and borrows
// the metadata of a neighbouring cell with that trigger.
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
                    && gpGame->m_map[i][j].m_triggerType == cell->m_triggerType)
                    cell->m_objectMetadata = gpGame->m_map[i][j].m_objectMetadata;
            }
        }
        gpGame->SettleOverlay(x, y);
    }
    if (gpGame->m_mapSounds[x][y] != MAP_SOUND_NONE) {
        gpGame->m_mapSounds[x][y] = MAP_SOUND_NONE;
        if (bShowIt)
            SetEnvironmentOrigin(
                m_mapOriginX + ENVIRONMENT_BORDER,
                m_mapOriginY + ENVIRONMENT_BORDER,
                1
            );
    }
    gpGame->SetupAdjacentMons();
}

// donor PoL RVA 0x000aea02; preferred Buka symbol ?HeroSwap@advManager@@QAEXPAVhero@@0@Z
// donor Buka TU SOURCE/EVENTS; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.621863;margin=0.183841;shape=0.591;size=0.854;calls=1.000;alternate=pol20:void advManager::HeroSwap(class hero *, class hero *)@0x000aea02
VA(0x004269b5, 0xa3)
void advManager::HeroSwap(class hero* firstHero, class hero* secondHero) {
    swapManager* swapMgr;

    swapMgr = new swapManager(firstHero, secondHero);
    if (!swapMgr)
        MemError();
    gpExec->DoDialog(swapMgr);
    delete swapMgr;
}

// donor PoL RVA 0x000af87c; preferred Buka symbol ?TownEvent@advManager@@QAEXPAVmapCell@@HH@Z
// donor Buka TU SOURCE/EVENTS; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.530630;margin=0.268223;shape=0.333;size=0.946;calls=1.000;alternate=pol20:void advManager::TownEvent(class mapCell *, int, int)@0x000af87c
VA(0x00426a58, 0x191)
void advManager::TownEvent(class mapCell* cell, i32 x, i32 y) {
    hero* curHero;
    i32 result;
    hero* defender;
    town* recRef;

    recRef = gpGame->GetTown(cell->m_objectMetadata);
    curHero = gpGame->GetHero(gpCurPlayer->m_currentHero);
    DemobilizeCurrHero();
    if (recRef->m_owner == giCurPlayer) {
        recRef->m_occupyingHeroId = gpCurPlayer->CurrentHero();
        recRef->View();
    } else if (recRef->HasGarrison()) {
        defender = recRef->m_occupyingHeroId == TOWN_OCCUPYING_HERO_NONE
                       ? NULL
                       : gpGame->GetHero(recRef->m_occupyingHeroId);
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
            gpGame->ClaimTown(recRef->m_id, giCurPlayer);
    } else {
        gpGame->ClaimTown(recRef->m_id, giCurPlayer);
        UpdateRadar(1, 0);
        UpdateHeroLocators(1, 1);
        UpdateTownLocators(1, 1);
        recRef->m_occupyingHeroId = gpCurPlayer->CurrentHero();
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

// donor PoL RVA 0x000aff6c; preferred Buka symbol ?EventWindow@advManager@@QAEXHHPADHHHHH@Z
// donor Buka TU SOURCE/EVENTS; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.574046;margin=0.505217;shape=0.246;size=0.761;calls=0.800;strings=Event ID %d;alternate=pol20:void advManager::EventWindow(int, int, char *, int, int, int, int, int)@0x000aff6c
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
    gpGame->m_randomArtifacts[artifact] = eventHero->m_id;
    GiveTakeArtifactStat(eventHero, artifact, EVENT_ARTIFACT_GIVE);
    return slot;
}

// donor PoL RVA 0x000b00e9; preferred Buka symbol ?GiveRandomArtifact@advManager@@QAEHPAVhero@@@Z
// donor Buka TU SOURCE/EVENTS; HoMM1 owner inferred from contiguous order
// evidence: graph:3;base=0.483471;margin=0.618508;shape=0.306;size=0.821;calls=1.000;alternate=pol20:int advManager::GiveRandomArtifact(class hero *)@0x000b00e9
VA(0x00426f16, 0x4f)
i32 advManager::GiveRandomArtifact(class hero* eventHero) {
    i8 artifact;

    artifact = gpGame->GetRandomArtifactId();
    if (artifact == ARTIFACT_NONE)
        GiveResource(eventHero, RESOURCE_GOLD, EVENT_RANDOM_ARTIFACT_GOLD);
    else
        GiveArtifact(eventHero, artifact);
    return artifact;
}

// donor PoL RVA 0x000b0147; preferred Buka symbol ?GiveExperience@advManager@@QAEHPAVhero@@HH@Z
// donor Buka TU SOURCE/EVENTS; HoMM1 owner inferred from contiguous order
// evidence: graph:4;base=0.329448;margin=0.686602;shape=0.229;size=0.551;calls=0.600;alternate=pol20:int advManager::GiveExperience(class hero *, int, int)@0x000b0147
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
        gpGame->m_players[eventHero->m_owner].m_resources[resource] += amount;
}

// donor PoL RVA 0x000b022e; preferred Buka symbol ?RecruitEvent@advManager@@QAEXPAVhero@@HPAVmapCell@@@Z
// donor Buka TU SOURCE/EVENTS; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.608108;margin=0.082972;shape=0.542;size=0.949;calls=0.833;alternate=pol20:void advManager::RecruitEvent(class hero *, int, class mapCell *)@0x000b022e
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
    gpExec->DoDialog(recruitWindow);
    delete recruitWindow;
    cell->m_objectMetadata = availableCount;
}

// donor PoL RVA 0x000b07e5; preferred Buka symbol ?GhostEvent@advManager@@QAEHPAVhero@@PAVmapCell@@PADHH@Z
// donor Buka TU SOURCE/EVENTS; HoMM1 owner inferred from contiguous order
// evidence: graph:3;base=0.581832;margin=0.097486;shape=0.425;size=0.973;calls=1.000;alternate=pol20:int advManager::GhostEvent(class hero *, class mapCell *, char *, int, int)@0x000b07e5
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

// donor PoL RVA 0x000b0add; preferred Buka symbol ?HouseEvent@advManager@@QAEXPAVhero@@PAVmapCell@@@Z
// donor Buka TU SOURCE/EVENTS; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.402651;margin=0.176834;shape=0.333;size=0.492;calls=1.000;alternate=pol20:void advManager::HouseEvent(class hero *, class mapCell *)@0x000b0add
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
        if (gpWindowManager->m_dialogResult == NORMAL_DIALOG_CONFIRM) {
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
    CLEAR_ARMY_GROUP(*gpMonGroup);
    if (count / ARMY_GROUP_SLOT_COUNT > 0) {
        for (i = 0; i < ARMY_GROUP_SLOT_COUNT; i++) {
            gpMonGroup->m_creatureTypes[i] = monsterType;
            gpMonGroup->m_creatureCounts[i] = count / ARMY_GROUP_SLOT_COUNT;
        }
    }
    for (i = count % ARMY_GROUP_SLOT_COUNT - 1; i >= 0; i--) {
        gpMonGroup->m_creatureTypes[i] = monsterType;
        gpMonGroup->m_creatureCounts[i]++;
    }
    if (heroDefends)
        res = DoCombat(
            fromX,
            fromY,
            NULL,
            gpMonGroup,
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
            gpMonGroup,
            x,
            y,
            COMBAT_RANDOM_SEED_NEW,
            1
        );
    MobilizeCurrHero(0);
    return res;
}

// Buka's free GiveTakeArtifactStat; HoMM1 keeps per-artifact primary-stat
// bonuses here and is called through gpAdvManager.
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

// donor PoL RVA 0x000b1973; preferred Buka symbol ?TransferArtifacts@advManager@@QAEXPAVhero@@0@Z
// donor Buka TU SOURCE/EVENTS; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.505054;margin=0.383531;shape=0.360;size=0.848;calls=0.800;alternate=pol20:void advManager::TransferArtifacts(class hero *, class hero *)@0x000b1973
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
                        if (gbThisNetHumanPlayer[sourceHero->m_owner]
                            || gbThisNetHumanPlayer[destHero->m_owner]) {
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
                                sourceHero->m_artifacts[j],
                                NORMAL_DIALOG_NO_RESOURCE,
                                0,
                                NORMAL_DIALOG_NO_OR_TEXT
                            );
                        }
                        gpGame->m_randomArtifacts[sourceHero->m_artifacts[j]] = GAME_HERO_NONE;
                    } else {
                        GiveTakeArtifactStat(
                            destHero,
                            sourceHero->m_artifacts[j],
                            EVENT_ARTIFACT_GIVE
                        );
                        destHero->m_artifacts[i] = sourceHero->m_artifacts[j];
                        gpGame->m_randomArtifacts[sourceHero->m_artifacts[j]] = destHero->m_id;
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

// donor PoL RVA 0x000b1b50; preferred Buka symbol ?HeroLoses@advManager@@QAEXPAVhero@@@Z
// donor Buka TU SOURCE/EVENTS; HoMM1 owner inferred from contiguous order
// evidence: graph:4;base=0.567256;margin=0.641527;shape=0.448;size=0.872;calls=1.000;alternate=pol20:void advManager::HeroLoses(class hero *)@0x000b1b50
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

// donor PoL RVA 0x000b1bcf; preferred Buka symbol ?DoWhirlpool@advManager@@QAEXPAVhero@@@Z
// donor Buka TU SOURCE/EVENTS; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.515247;margin=0.370456;shape=0.302;size=0.900;calls=1.000;alternate=pol20:void advManager::DoWhirlpool(class hero *)@0x000b1bcf
VA(0x00427af9, 0x122)
void advManager::DoWhirlpool(class hero* eventHero) {
    i32 selectedSlot;
    i16 slotNo;
    i32 groupValues[ARMY_GROUP_SLOT_COUNT];
    i32 lowestValue;
    i32 creatureValue;

    if (!gbHumanPlayer[eventHero->m_owner])
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

// donor PoL RVA 0x000b1d01; preferred Buka symbol ?FizzleCenter@advManager@@QAEXH@Z
// donor Buka TU SOURCE/EVENTS; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.628535;margin=0.385528;shape=0.317;size=0.884;calls=0.800;strings=killfade.82M|pickup%02d.82M;alternate=pol20:void advManager::FizzleCenter(int)@0x000b1d01
VA(0x00427c1b, 0xc2)
void advManager::FizzleCenter(i32 fizzleType) {
    class sample* fizzleSample;

    if (!bShowIt)
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
    gpWindowManager
        ->SaveFizzleSource(EVENT_FIZZLE_X, EVENT_FIZZLE_Y, EVENT_FIZZLE_WIDTH, EVENT_FIZZLE_HEIGHT);
    CompleteDraw(0);
    gpWindowManager->FizzleForward(
        EVENT_FIZZLE_X,
        EVENT_FIZZLE_Y,
        EVENT_FIZZLE_WIDTH,
        EVENT_FIZZLE_HEIGHT,
        EVENT_FIZZLE_STEPS
    );
    WaitSample(fizzleSample);
}

// donor PoL RVA 0x000b1e43; preferred Buka symbol ?DoAIEvent@advManager@@QAEXPAVmapCell@@PAVhero@@HH@Z
// donor Buka TU SOURCE/EVENTS; HoMM1 owner inferred from contiguous order
// evidence: graph:7;base=0.283401;margin=1.483201;shape=0.274;size=0.397;calls=0.409;alternate=pol20:void advManager::DoAIEvent(class mapCell *, class hero *, int, int)@0x000b1e43
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
    oldPlayer = giCurPlayer;
    ownerPlayerData = gpCurPlayer;
    --eventHero->m_remainingMobility;
    mapVisited[x][y] |= giCurPlayerBit;
    switch (eventType) {
        case MAP_OBJECT_COAST:
            if (eventHero->m_eventFlags & HERO_EVENT_EMBARKED) {
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
            boat = &gpGame->m_boats[cell->m_objectMetadata];
            gpGame->RestoreCell(-1, -1, boat->savedTriggerType, boat->savedEventData, cell, 3);
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
            if (gpGame->m_mineOwners[cell->m_objectMetadata] == giCurPlayer)
                break;
            gpGame->ClaimMine(cell->m_objectMetadata, giCurPlayer);
            break;
        case MAP_OBJECT_LIGHTHOUSE:
            if (gpGame->m_mineOwners[MINE_SLOT_LIGHTHOUSE] == giCurPlayer)
                break;
            gpGame->ClaimMine(MINE_SLOT_LIGHTHOUSE, giCurPlayer);
            break;
        case MAP_OBJECT_DRAGON_CITY:
            if (gpGame->m_mineOwners[MINE_SLOT_DRAGON_CITY] == giCurPlayer)
                break;
            for (c = 0; c < ARMY_GROUP_SLOT_COUNT; c++) {
                gpMonGroup->m_creatureTypes[c] = CREATURE_DRAGON;
                gpMonGroup->m_creatureCounts[c] = 1;
            }
            gpPhilAI->ChooseEvaluateBattle(
                &eventHero->m_army,
                eventHero,
                gpMonGroup,
                NULL,
                0,
                0,
                500,
                canWin,
                armyStrength
            );
            if (canWin) {
                c = DRAGON_CITY_DRAGON_COUNT;
                success = gpPhilAI->CombatMonsterEvent(eventHero, CREATURE_DRAGON, &c, cell);
                if (success)
                    gpGame->ClaimMine(MINE_SLOT_DRAGON_CITY, giCurPlayer);
            }
            break;
        case MAP_OBJECT_TREASURE_CHEST:
            if (gpPhilAI->ChooseGoldOrExperience(
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
            gpGame->m_mapSounds[m_mapOriginX + ENVIRONMENT_BORDER]
                               [m_mapOriginY + ENVIRONMENT_BORDER] = MAP_SOUND_NONE;
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
                gpPhilAI->EvaluateOneTimeCreaturePurchase(
                    eventHero,
                    recruitType,
                    cell->m_objectMetadata,
                    isFree,
                    recruited,
                    purchaseValue,
                    freeSlot
                );
                if (recruited > 0) {
                    gpGame->GiveArmy(&eventHero->m_army, recruitType, recruited, freeSlot);
                    cell->m_objectMetadata -= recruited;
                    if (!isFree) {
                        GetMonsterCost(recruitType, troopCost);
                        for (c = 0; c < RESOURCE_COUNT; c++)
                            gpCurPlayer->m_resources[c] -= recruited * troopCost[c];
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
            if (!(gpGame->m_obeliskVisitors[cell->m_objectMetadata - 1] & giCurPlayerBit)) {
                gpGame->VisitObelisk(eventHero->m_owner);
                gpGame->m_obeliskVisitors[cell->m_objectMetadata - 1] |= giCurPlayerBit;
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
            gpPhilAI->TownEvent(cell, eventHero, x, y);
            break;
        case MAP_OBJECT_WHIRLPOOL:
            DoWhirlpool(eventHero);
        case MAP_OBJECT_STONE_LITHS:
            portalCount = 0;
            for (teleY = 0; teleY < MAP_CELL_GRID_SIZE; teleY++) {
                for (teleX = 0; teleX < MAP_CELL_GRID_SIZE; teleX++) {
                    if (gpGame->m_map[teleX][teleY].m_triggerType
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
                        if (gpGame->m_map[teleX][teleY].m_triggerType
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
                gpAdvManager->TeleportTo(teleX, teleY, 0);
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
                    if (gpPhilAI->CombatMonsterEvent(eventHero, CREATURE_ROGUE, &c, cell))
                        goto giveArtifact;
                    break;
                case ARTIFACT_EVENT_MODE_GOLD:
                    if (gpPhilAI->ChooseToBuyArtifact(
                            eventHero,
                            cell->m_objectIndex,
                            ARTIFACT_EVENT_GOLD_COST
                        )) {
                        gpGame->m_players[eventHero->m_owner].m_resources[RESOURCE_GOLD] -=
                            ARTIFACT_EVENT_GOLD_COST;
                        goto giveArtifact;
                    } else {
                        removeEvent = 1;
                    }
                    break;
            }
            break;
        case MAP_OBJECT_HERO:
            opponent = gpGame->GetHero(cell->m_objectMetadata);
            priorShowIt = bShowIt;
            if (opponent->m_owner == giCurPlayer)
                return;
            if (opponent->m_locationType == (MAP_TRIGGER_EVENT | MAP_OBJECT_TOWN))
                heroTown = gpGame->GetTown(opponent->m_occupiedTown);
            if (!gbHumanPlayer[opponent->m_owner]) {
                battleResult = gpPhilAI->QuickCombat(
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
                    battleResult = gpPhilAI->QuickCombat(
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
                    gpGame->ClaimTown(heroTown->m_id, giCurPlayer);
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
                    if (gpGame->m_players[eventHero->m_owner].m_resources[RESOURCE_GOLD]
                        >= DAEMON_GOLD) {
                        if (gpPhilAI->ChooseToPayRansomOnHero(eventHero, DAEMON_GOLD))
                            gpGame->m_players[eventHero->m_owner].m_resources[RESOURCE_GOLD] +=
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
            gpPhilAI->FightEvent(eventHero, cell);
            break;
        default:
            break;
    }
    if (removeEvent)
        EraseObj(cell, x, y);
    giCurPlayer = oldPlayer;
    gpCurPlayer = ownerPlayerData;
    CheckEndGame(0);
}

// donor PoL RVA 0x000b4fd5; preferred Buka symbol ?PlayerMonsterInteract@advManager@@QAEXPAVmapCell@@0PAVhero@@PAHHHHHH@Z
// donor Buka TU SOURCE/EVENTS; HoMM1 owner inferred from contiguous order
// evidence: graph:4;base=0.214069;margin=0.493239;shape=0.272;size=0.223;calls=0.212;alternate=pol20:void advManager::PlayerMonsterInteract(class mapCell *, class mapCell *, class hero *, int *, int, int, int, int, int)@0x000b4fd5
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
        if (gpPhilAI->FightValueOfStack(&eventHero->m_army, eventHero, 0, 0, 0)
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
                if (gpWindowManager->m_dialogResult == NORMAL_DIALOG_CONFIRM) {
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
        && gpPhilAI->FightValueOfStack(&eventHero->m_army, eventHero, 0, 0, 0)
               > (cell->m_objectMetadata & MONSTER_COUNT_MASK)
                     * gMonsterDatabase[cell->m_objectIndex].fightValue * 1.75) {
        gpPhilAI->EvaluateOneTimeCreaturePurchase(
            eventHero,
            cell->m_objectIndex,
            cell->m_objectMetadata & MONSTER_COUNT_MASK,
            1,
            numToBuy,
            quantity,
            bestSlot
        );
        if (numToBuy > 0) {
            gpGame->GiveArmy(
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
            gpPhilAI->CombatMonsterEvent(eventHero, cell->m_objectIndex, &creatureCountIdx, cell);
        cell->m_objectMetadata = (cell->m_objectMetadata & MONSTER_WILLING_FLAG) + creatureCountIdx;
        if (retVal)
            *handled = 1;
    }
}

// donor PoL RVA 0x000b5c40; preferred Buka symbol ?DoNetCombat@advManager@@QAEHPAD@Z
// donor Buka TU SOURCE/EVENTS; HoMM1 owner inferred from contiguous order
// evidence: graph:3;base=0.634004;margin=0.818203;shape=0.529;size=0.995;calls=1.000;alternate=pol20:int advManager::DoNetCombat(char *)@0x000b5c40
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
        &gbRetreatWin,
        &gbCombatSurrender
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
    if (!gbHumanPlayer[party])
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
            gbRetreatWin,
            gbCombatSurrender
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
    gbRetreatWin = 0;
    return 1;
}

// Remote combat hand-off (Buka CombatRemoteCommand / CombatRemoteFragment):
// SendHeroTownData sends the combat record as COMMAND (answered by
// CONFIRM), then each hero in its own fragment.
H1_ENUM_CONST_BEGIN(CombatRemoteConstant)
    COMBAT_REMOTE_COMMAND = 0x15,
    COMBAT_REMOTE_CONFIRM_COMMAND = 0x16,
    COMBAT_REMOTE_FRAGMENT_COMBAT = 0,
    COMBAT_REMOTE_FRAGMENT_FIRST_HERO = 1,
    COMBAT_REMOTE_FRAGMENT_SECOND_HERO = 2,
    COMBAT_REMOTE_BUFFER_SIZE = 0xff,
    COMBAT_REMOTE_TIMEOUT = 20000
H1_ENUM_CONST_END(CombatRemoteConstant)

// SendHeroTownData's payload after the remote-message header, as in Buka's
// combatRemoteData; hero records follow one fragment byte.
#pragma pack(push, 1)
struct combatRemoteData {
    i8 fragment;
    i8 x;
    i8 y;
    i8 hasFirstHero;
    i8 hasTown;
    i8 hasSecondHero;
    i8 setupCombatX;
    i8 setupCombatY;
    i32 randomSeed;
    i8 combatResult;
    i8 retreatWin;
    i8 combatSurrender;
    i8 firstOwner;
    i32 firstGold;
    i8 secondOwner;
    i32 secondGold;
    armyGroup firstArmy;
    armyGroup secondArmy;
    town combatTown;
};

struct combatRemoteHeroFragment {
    i8 fragment;
    char data[sizeof(hero)];
};

struct combatRemoteMessage {
    i8 sender;
    i32 id;
    i8 type;
    i8 command;
    i16 payloadSize;
    combatRemoteData combat;
};

struct heroRemoteMessage {
    i8 sender;
    i32 id;
    i8 type;
    i8 command;
    i16 payloadSize;
    combatRemoteHeroFragment heroFragment;
};
#pragma pack(pop)

// CheckHandleNet, GetRemoteData and ReceiveHeroTownData pass received records
// as char*; the Buka 2.1 donor reads them through these views.
#define EVENTS_REMOTE_MESSAGE(buffer)                                                              \
    (reinterpret_cast<combatRemoteMessage*>(buffer)) // API-forced: char* records.
#define EVENTS_REMOTE_HERO(buffer)                                                                 \
    (reinterpret_cast<heroRemoteMessage*>(buffer)) // API-forced: char* records.

// donor PoL RVA 0x000b5e10; preferred Buka symbol ?DoCombat@advManager@@QAEHHHPAVhero@@PAVarmyGroup@@PAVtown@@01HHHH@Z
// donor Buka TU SOURCE/EVENTS; HoMM1 owner inferred from contiguous order
// evidence: graph:3;base=0.590184;margin=0.564001;shape=0.455;size=0.978;calls=0.927;alternate=pol20:int advManager::DoCombat(int, int, class hero *, class armyGroup *, class town *, class hero *, class armyGroup *, int, int, int, int)@0x000b5e10
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
    oldPlayer = giCurPlayer;
    showItSaved = bShowIt;

    if (attackPlayer >= 0 && curPlayer >= 0 && gbHumanPlayer[curPlayer]) {
        if (!gbThisNetHumanPlayer[curPlayer]) {
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
            if (!gbHumanPlayer[attackPlayer]) {
                while (1) {
                    PollSound();
                    FillBitmapArea(
                        gpWindowManager->m_screen,
                        COMBAT_NETWORK_POLL_X,
                        COMBAT_NETWORK_POLL_Y,
                        COMBAT_NETWORK_POLL_WIDTH,
                        COMBAT_NETWORK_POLL_HEIGHT,
                        0
                    );
                    receivedPacket = CheckHandleNet();
                    if (receivedPacket) {
                        switch (EVENTS_REMOTE_MESSAGE(receivedPacket)->command) {
                            case COMBAT_REMOTE_COMMAND:
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
                                    &gbRetreatWin,
                                    &gbCombatSurrender
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
                                gpCombatManager->m_combatResult = res;
                                goto combatFinished;
                        }
                    }
                    Process1WindowsMessage();
                    message = gpInputManager->GetEvent();
                    CheckHandleNetPlayerWait(message, 1);
                }
            }
        } else if (!gbThisNetHumanPlayer[attackPlayer]) {
            bShowIt = 1;
            gpGame->TurnOffAIMusic();
            sprintf(
                gText,
                localization::Tr("combat.network.attacked"),
                gColorNames[gpGame->m_players[curPlayer].m_color],
                combatTown ? localization::Tr("combat.network.town")
                           : localization::Tr("combat.network.hero")
            );
            gText[0] = CyrillicToUpper(gText[0]);
            gpGame->WaitForPlayer(gText, curPlayer);
        }
    }

    bShowIt = 1;
    gpCombatManager->SetupCombat(
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
    gpExec->CallManager(gpCombatManager);
    gAdvDisposeLevel = ADV_DISPOSE_NONE;

combatFinished:
    if (firstHero)
        firstHero->CheckLevel();
    if (secondHero)
        secondHero->CheckLevel();
    if (processLosses) {
        switch (gpCombatManager->m_combatResult) {
            case COMBAT_RESULT_ATTACKER:
                if (!gbRetreatWin)
                    TransferArtifacts(secondHero, firstHero);
                HeroLoses(secondHero);
                break;
            case COMBAT_RESULT_DEFENDER:
                if (!gbRetreatWin)
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
    bShowIt = showItSaved;
    giCurPlayer = oldPlayer;
    if (!gbHumanPlayer[giCurPlayer]) {
        gpGame->ShowComputerScreen();
        gpGame->TurnOnAIMusic();
        SetNoDialogMenus(0);
    } else {
        SetNoDialogMenus(1);
    }
    MobilizeCurrHero(0);
    if (processLosses)
        gbRetreatWin = 0;
    gInCombat = 0;
    return gpCombatManager->m_combatResult;
}

// donor PoL RVA 0x000b645e; preferred Buka symbol ?SendHeroTownData@advManager@@QAEXHHPAVhero@@PAVarmyGroup@@PAVtown@@01HHHHHHH@Z
// donor Buka TU SOURCE/EVENTS; HoMM1 owner inferred from contiguous order
// evidence: graph:4;base=0.543308;margin=0.967008;shape=0.438;size=0.943;calls=0.684;alternate=pol20:void advManager::SendHeroTownData(int, int, class hero *, class armyGroup *, class town *, class hero *, class armyGroup *, int, int, int, int, int, int, int)@0x000b645e
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
        firstHero ? gpGame->m_players[firstHero->m_owner].m_resources[RESOURCE_GOLD] : 0;
    buffer.combat->secondOwner = secondHero ? secondHero->m_owner : -1;
    buffer.combat->secondGold =
        secondHero ? gpGame->m_players[secondHero->m_owner].m_resources[RESOURCE_GOLD] : 0;
    memcpy(&buffer.combat->firstArmy, firstArmy, sizeof(armyGroup));
    memcpy(&buffer.combat->secondArmy, secondArmy, sizeof(armyGroup));
    if (combatTown)
        memcpy(&buffer.combat->combatTown, combatTown, sizeof(town));

    // API-forced: TransmitAndWait/TransmitRemoteData take char* payloads.
    result = TransmitAndWait(
        buffer.bytes,
        remotePlayer,
        sizeof(combatRemoteData),
        COMBAT_REMOTE_COMMAND,
        COMBAT_REMOTE_CONFIRM_COMMAND,
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
            COMBAT_REMOTE_COMMAND,
            1,
            1,
            REMOTE_MESSAGE_DEFAULT,
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
            COMBAT_REMOTE_COMMAND,
            1,
            1,
            REMOTE_MESSAGE_DEFAULT,
            1
        );
        if (!result)
            ShutDown(NULL);
    }
    free(buffer.combat);
}

// donor PoL RVA 0x000b67cd; preferred Buka symbol ?ReceiveHeroTownData@advManager@@QAEXPADPAH11PAPAVhero@@PAPAVarmyGroup@@PAPAVtown@@23111PAC55@Z
// donor Buka TU SOURCE/EVENTS; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.493176;margin=0.152223;shape=0.314;size=0.857;calls=0.909;alternate=pol20:void advManager::ReceiveHeroTownData(char *, int *, int *, int *, class hero * *, class armyGroup * *, class town * *, class hero * *, class armyGroup * *, int *, int *, int *, signed char *, signed char *, signed char *)@0x000b67cd
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
        gpGame->m_players[firstOwner].m_resources[RESOURCE_GOLD] =
            EVENTS_REMOTE_MESSAGE(packet)->combat.firstGold;
    defenderOwner = EVENTS_REMOTE_MESSAGE(packet)->combat.secondOwner;
    if (defenderOwner > 0)
        gpGame->m_players[defenderOwner].m_resources[RESOURCE_GOLD] =
            EVENTS_REMOTE_MESSAGE(packet)->combat.secondGold;

    *firstArmy = static_cast<armyGroup*>(malloc(sizeof(armyGroup)));
    memcpy(*firstArmy, &EVENTS_REMOTE_MESSAGE(packet)->combat.firstArmy, sizeof(armyGroup));
    *secondArmy = static_cast<armyGroup*>(malloc(sizeof(armyGroup)));
    memcpy(*secondArmy, &EVENTS_REMOTE_MESSAGE(packet)->combat.secondArmy, sizeof(armyGroup));
    if (hasTownOn) {
        *combatTown = static_cast<town*>(malloc(sizeof(town)));
        memcpy(*combatTown, &EVENTS_REMOTE_MESSAGE(packet)->combat.combatTown, sizeof(town));
    }

    mainResult = TransmitRemoteData(
        NULL,
        *remotePlayer,
        0,
        COMBAT_REMOTE_CONFIRM_COMMAND,
        1,
        1,
        REMOTE_MESSAGE_DEFAULT,
        1
    );
    if (!mainResult)
        ShutDown(NULL);

    lastPacketTimeNum = KBTickCount();
    while ((hasSecondHero && !*secondHero) || (bFirstHero && !*firstHero)) {
        PollSound();
        if (lastPacketTimeNum + COMBAT_REMOTE_TIMEOUT < KBTickCount()) {
            NormalDialog(
                localization::Tr("combat.network.receive_error"),
                NORMAL_DIALOG_TYPE_YES_NO,
                -1,
                -1,
                NORMAL_DIALOG_NO_RESOURCE,
                0,
                NORMAL_DIALOG_NO_RESOURCE,
                0,
                NORMAL_DIALOG_NO_OR_TEXT
            );
            if (gpWindowManager->m_dialogResult == NORMAL_DIALOG_CONFIRM)
                lastPacketTimeNum = KBTickCount();
            else
                ShutDown(localization::Tr("combat.network.canceled"));
        }
        packet = GetRemoteData(1);
        if (packet && EVENTS_REMOTE_MESSAGE(packet)->type == REMOTE_MESSAGE_RELIABLE
            && EVENTS_REMOTE_MESSAGE(packet)->command == COMBAT_REMOTE_COMMAND) {
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

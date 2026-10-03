// Located from HoMM2 Buka 2.1; PoL 2.0 supplies the VC4 declaration.

#include <match.h>

#include <SOURCE/EVENTS.h>

#include <BASE/bmap2.h>
#include <BASE/inputManager.h>
#include <BASE/INPUTMGR_TYPES.h>
#include <BASE/Misc.h>
#include <H1/All.h>
#include <H1/KB.h>
#include <SOURCE/dialogTypes.h>
#include <SOURCE/kbwin.h>
#include <SOURCE/mapObjectTypes.h>
#include <SOURCE/REMOTE.h>
#include <SOURCE/X_GLOBAL.h>

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
VA(0x0045dde0, 0x1f1a)
void advManager::DoEvent(class mapCell* cell, int x, int y) {
    hero* pHero;
    tag_message unused;
    signed char objType;
    int artifactId;
    int fizzleMode;
    tag_message event;
    signed char erase;
    boatRecord* ship;
    heroWindow* win;
    char resourceName[20];
    int resType;
    signed char tx;
    signed char teleportCount;
    int res;
    signed char adjacentMonster;
    signed char ty;
    int income;
    hero* enemyHero;
    int numDefenders;
    mapCell* prevCell;
    town* occupiedTown;

    pHero = gpGame->GetHero(gpCurPlayer->m_currentHero);
    objType = cell->m_triggerType & MAP_TRIGGER_TYPE_MASK;
    erase = 0;
    fizzleMode = EVENT_FIZZLE_HERO_LOSS;
    gbEventMusicPlaying = 1;
    gpMouseManager->ReallyHidePointer();
    EventSound(objType, cell->m_objectMetadata);
    switch (objType) {
        case MAP_OBJECT_COAST:
            if (pHero->m_eventFlags & HERO_EVENT_EMBARKED) {
                pHero->m_eventFlags &= ~HERO_EVENT_EMBARKED;
                pHero->m_remainingMobility = 0;
                pHero->m_direction = m_cursorDirection;
                m_cursorType = pHero->m_heroClass;
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
                CheckAdjacentMon(&adjacentMonster);
            }
            break;
        case MAP_OBJECT_SHIP:
            ship = &gpGame->m_boats[cell->m_objectMetadata];
            gpGame->RestoreCell(-1, -1, ship->savedTriggerType, ship->savedEventData, cell, 2);
            pHero->m_eventFlags |= HERO_EVENT_EMBARKED;
            pHero->m_remainingMobility = 0;
            ship->heroId = pHero->m_id;
            ship->owner = pHero->m_owner;
            m_cursorType = ADVMGR_HERO_ICON_BOAT;
            m_cursorDirection = ship->direction;
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
                    numDefenders = DRAGON_CITY_CAMPAIGN_DRAGON_COUNT;
                else
                    numDefenders = DRAGON_CITY_DRAGON_COUNT;
                if (CombatMonsterEvent(pHero, CREATURE_DRAGON, numDefenders, cell, x, y, 0, x, y)
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
                pHero->CheckLevel();
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
                GiveResource(pHero, RESOURCE_GOLD, cell->m_objectMetadata * CHEST_GOLD_MULTIPLIER);
            else
                GiveExperience(
                    pHero,
                    (cell->m_objectMetadata - CHEST_EXPERIENCE_LEVEL_OFFSET)
                        * CHEST_EXPERIENCE_MULTIPLIER,
                    0
                );
            erase = 1;
            fizzleMode = EVENT_FIZZLE_PICKUP;
            pHero->CheckLevel();
            break;
        case MAP_OBJECT_BUOY:
            if (pHero->m_eventFlags & HERO_EVENT_BUOY) {
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
                pHero->m_eventFlags |= HERO_EVENT_BUOY;
                pHero->m_morale++;
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
            if (pHero->m_eventFlags & HERO_EVENT_FAERIE_RING) {
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
                pHero->m_eventFlags |= HERO_EVENT_FAERIE_RING;
                pHero->m_luck++;
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
            if (pHero->m_eventFlags & HERO_EVENT_FOUNTAIN) {
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
                pHero->m_eventFlags |= HERO_EVENT_FOUNTAIN;
                pHero->m_luck++;
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
            if (pHero->m_eventFlags & HERO_EVENT_OASIS) {
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
                pHero->m_eventFlags |= HERO_EVENT_OASIS;
                pHero->m_morale++;
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
            if (pHero->m_eventFlags & HERO_EVENT_STATUE) {
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
                pHero->m_eventFlags |= HERO_EVENT_STATUE;
                pHero->m_morale += TEMPLE_MORALE_BONUS;
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
                    if (pHero->NumArtifacts() == HERO_ARTIFACT_SLOT_COUNT) {
                        sprintf(gText, "%s.", "Treasure");
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
                        artifactId = GiveRandomArtifact(pHero);
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
                pHero,
                RESOURCE_GOLD,
                (cell->m_objectMetadata >> CAMPFIRE_AMOUNT_SHIFT) * CAMPFIRE_GOLD_MULTIPLIER
            );
            GiveResource(
                pHero,
                cell->m_objectMetadata & CAMPFIRE_RESOURCE_MASK,
                cell->m_objectMetadata >> CAMPFIRE_AMOUNT_SHIFT
            );
            erase = 1;
            fizzleMode = EVENT_FIZZLE_PICKUP;
            gpGame->m_mapSounds[m_mapOriginX + ENVIRONMENT_BORDER]
                               [m_mapOriginY + ENVIRONMENT_BORDER] = MAP_SOUND_NONE;
            SetEnvironmentOrigin(
                m_mapOriginX + ENVIRONMENT_BORDER,
                m_mapOriginY + ENVIRONMENT_BORDER,
                1
            );
            break;
        case MAP_OBJECT_GAZEBO:
            if (pHero->m_visitedSites & (1 << cell->m_objectMetadata)) {
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
                GiveExperience(pHero, GAZEBO_EXPERIENCE, 0);
                pHero->m_visitedSites |= 1 << cell->m_objectMetadata;
                pHero->CheckLevel();
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
                    pHero,
                    RESOURCE_GOLD,
                    cell->m_objectMetadata * WATERWHEEL_GOLD_MULTIPLIER
                );
                cell->m_objectMetadata = MAP_EVENT_DATA_EMPTY;
            }
            break;
        case MAP_OBJECT_RESOURCE:
            resType = cell->m_objectIndex - RESOURCE_PILE_OBJECT_BASE;
            GiveResource(
                pHero,
                resType,
                resType == RESOURCE_GOLD ? cell->m_objectMetadata * RESOURCE_PILE_GOLD_MULTIPLIER
                                         : cell->m_objectMetadata
            );
            strcpy(resourceName, gResourceNames[resType]);
            resourceName[0] += 'a' - 'A';
            sprintf(gText, gEventText[EVENT_TEXT_RESOURCE_PICKUP], resourceName);
            BVResMsg(
                gText,
                resType,
                resType == RESOURCE_GOLD ? cell->m_objectMetadata * RESOURCE_PILE_GOLD_MULTIPLIER
                                         : cell->m_objectMetadata
            );
            erase = 1;
            fizzleMode = EVENT_FIZZLE_PICKUP;
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
                GiveResource(pHero, cell->m_objectMetadata, WINDMILL_RESOURCE_AMOUNT);
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
                RecruitEvent(pHero, CREATURE_GENIE, cell);
                if (!cell->m_objectMetadata) {
                    erase = 1;
                    fizzleMode = EVENT_FIZZLE_PICKUP;
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
                    RecruitEvent(pHero, CREATURE_ROGUE, cell);
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
                    RecruitEvent(pHero, CREATURE_NOMAD, cell);
            }
            break;
        case MAP_OBJECT_STRAW_HUT:
        case MAP_OBJECT_HOUSE:
        case MAP_OBJECT_CABIN:
        case MAP_OBJECT_DWARF_LOG_CABIN:
        case MAP_OBJECT_PEASANT_LOG_CABIN:
            HouseEvent(pHero, cell);
            break;
        case MAP_OBJECT_MONSTER:
            PlayerMonsterInteract(cell, cell, pHero, &erase, x, y, 0, x, y);
            break;
        case MAP_OBJECT_OBELISK:
            if (!(gpGame->m_obeliskVisitors[cell->m_objectMetadata - 1] & (1 << pHero->m_owner))) {
                gpGame->VisitObelisk(pHero->m_owner);
                gpGame->m_obeliskVisitors[cell->m_objectMetadata - 1] |= 1 << pHero->m_owner;
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
            win = new heroWindow(0, 0, "thiefwin.bin");
            if (!win)
                MemError();
            SetWinText(win, WINDOW_TEXT_THIEVES_GUILD);
            gpTownManager->SetupThievesGuild(win, THIEVES_CATEGORY_COUNT);
            strcpy(gText, "Shrine - Player Rankings");
            SET_WIDGET_MESSAGE(event, WIDGET_COMMAND_SET_TEXT, 0);
            event.text = gText;
            win->BroadcastMessage(event);
            gpWindowManager->DoDialog(win, TrueFalseDialogHandler, 0);
            delete win;
            RedrawAdvScreen(1);
            break;
        case MAP_OBJECT_SPELL_SHRINE:
            sprintf(
                gText,
                "%s'%s'.",
                gEventText[EVENT_TEXT_SPELL_SHRINE],
                gSpellNames[cell->m_objectMetadata - MAP_EVENT_SPELL_OFFSET]
            );
            if (pHero->HasArtifact(ARTIFACT_MAGIC_BOOK)) {
                pHero->AddSpell(
                    cell->m_objectMetadata - MAP_EVENT_SPELL_OFFSET,
                    pHero->m_primaryStats[HERO_PRIMARY_KNOWLEDGE],
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
                strcat(gText, "  Unfortunately, you have no Magic Book to record the spell with.");
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
            if (giEventMusicVolume != EVENT_MUSIC_VOLUME_NONE)
                gConfig.musicVolume = giEventMusicVolume;
            giEventMusicVolume = EVENT_MUSIC_VOLUME_NONE;
            TownEvent(cell, x, y);
            break;
        case MAP_OBJECT_WHIRLPOOL:
            DoWhirlpool(pHero);
        case MAP_OBJECT_STONE_LITHS:
            teleportCount = 0;
            for (ty = 0; ty < MAP_CELL_GRID_SIZE; ty++) {
                for (tx = 0; tx < MAP_CELL_GRID_SIZE; tx++) {
                    if (gpGame->m_map[tx][ty].m_triggerType
                            == (unsigned char)(objType | MAP_TRIGGER_EVENT)
                        && MANHATTAN_LENGTH(tx - x, ty - y)
                               > (objType == MAP_OBJECT_STONE_LITHS ? STONE_LITHS_MIN_DISTANCE
                                                                    : WHIRLPOOL_MIN_DISTANCE))
                        teleportCount++;
                }
            }
            if (teleportCount >= 1) {
                if (teleportCount > 1)
                    teleportCount = Random(1, teleportCount);
                for (ty = 0; ty < MAP_CELL_GRID_SIZE; ty++) {
                    for (tx = 0; tx < MAP_CELL_GRID_SIZE; tx++) {
                        if (gpGame->m_map[tx][ty].m_triggerType
                                == (unsigned char)(objType | MAP_TRIGGER_EVENT)
                            && MANHATTAN_LENGTH(tx - x, ty - y)
                                   > (objType == MAP_OBJECT_STONE_LITHS ? STONE_LITHS_MIN_DISTANCE
                                                                        : WHIRLPOOL_MIN_DISTANCE)) {
                            if (--teleportCount <= 0)
                                goto teleport;
                        }
                    }
                }
            teleport:
                StopCursor(1);
                gpAdvManager->TeleportTo(tx, ty, 1);
            }
            break;
        case MAP_OBJECT_ARTIFACT:
            if (pHero->NumArtifacts() == HERO_ARTIFACT_SLOT_COUNT) {
                NormalDialog(
                    "You cannot pick up this artifact, you already have a full load!",
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
                    GiveArtifact(pHero, cell->m_objectIndex);
                    erase = 1;
                    fizzleMode = EVENT_FIZZLE_PICKUP;
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
                            pHero,
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
                        if (gpGame->m_players[pHero->m_owner].m_resources[RESOURCE_GOLD]
                            >= ARTIFACT_EVENT_GOLD_COST) {
                            gpGame->m_players[pHero->m_owner].m_resources[RESOURCE_GOLD] -=
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
                        erase = 1;
                    }
                    break;
            }
            pHero->CheckLevel();
            break;
        case MAP_OBJECT_HERO:
            DemobilizeCurrHero();
            enemyHero = gpGame->GetHero(cell->m_objectMetadata);
            if (enemyHero->m_owner == giCurPlayer) {
                HeroSwap(pHero, enemyHero);
            } else {
                occupiedTown = NULL;
                if (enemyHero->m_locationType == (MAP_TRIGGER_EVENT | MAP_OBJECT_TOWN)) {
                    occupiedTown = gpGame->GetTown(enemyHero->m_occupiedTown);
                    occupiedTown->m_occupyingHeroId = enemyHero->m_id;
                }
                res = DoCombat(
                    x,
                    y,
                    pHero,
                    &pHero->m_army,
                    occupiedTown,
                    enemyHero,
                    &enemyHero->m_army,
                    x,
                    y,
                    COMBAT_RANDOM_SEED_NEW,
                    1
                );
                if (res == COMBAT_RESULT_ATTACKER && occupiedTown)
                    gpGame->ClaimTown(occupiedTown->m_id, giCurPlayer);
            }
            break;
        case MAP_OBJECT_SIGNPOST:
            gpSearchArray->FindNearestObject(
                pHero->m_x,
                pHero->m_y,
                pHero->m_direction,
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
                    GiveExperience(pHero, DAEMON_EXPERIENCE, 0);
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
                    pHero->CheckLevel();
                    break;
                case DAEMON_REWARD_ARTIFACT:
                    if (pHero->NumArtifacts() == HERO_ARTIFACT_SLOT_COUNT)
                        goto goldReward;
                    if (gpGame->GetRandomArtifactId() == ARTIFACT_NONE)
                        goto goldReward;
                    GiveExperience(pHero, DAEMON_EXPERIENCE, 0);
                    artifactId = GiveRandomArtifact(pHero);
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
                    pHero->CheckLevel();
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
                    GiveExperience(pHero, DAEMON_EXPERIENCE, 0);
                    GiveResource(pHero, RESOURCE_GOLD, DAEMON_GOLD);
                    cell->m_objectMetadata = DAEMON_CAVE_EMPTY;
                    pHero->CheckLevel();
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
                        if (gpGame->m_players[pHero->m_owner].m_resources[RESOURCE_GOLD]
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
                            HeroLoses(pHero);
                        } else {
                            gpGame->m_players[pHero->m_owner].m_resources[RESOURCE_GOLD] -=
                                DAEMON_GOLD;
                        }
                    } else {
                        HeroLoses(pHero);
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
                        if (!(pHero->m_eventFlags & HERO_EVENT_GRAVEYARD)) {
                            pHero->m_eventFlags |= HERO_EVENT_GRAVEYARD;
                            pHero->m_morale--;
                        }
                        break;
                    default:
                        if (GhostEvent(pHero, cell, EVENT_TEXT_GRAVEYARD_REWARD, x, y))
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
                        if (!(pHero->m_eventFlags & HERO_EVENT_SHIPWRECK)) {
                            pHero->m_eventFlags |= HERO_EVENT_SHIPWRECK;
                            pHero->m_morale--;
                        }
                        break;
                    default:
                        prevCell = GetCell(
                            x - normalDirTable[pHero->m_direction].x,
                            y - normalDirTable[pHero->m_direction].y
                        );
                        if (GhostEvent(pHero, prevCell, EVENT_TEXT_SHIPWRECK_REWARD, x, y))
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
    if (erase) {
        EraseObj(cell, x, y);
        FizzleCenter(fizzleMode);
    } else {
        CompleteDraw(0);
    }
    UpdateScreen(0, 0);
    gpSoundManager->SwitchAmbientMusic(m_currentTerrain);
    gpMouseManager->ReallyShowPointer();
    CheckEndGame(0);
}

// @early-stop 97.31: m_mapSounds[x][y] (test and store) - retail adds the
// column term first (mov eax,y; ecx=x*72; add eax,ecx). vc4trace sortnode:
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
VA(0x0045fcfa, 0x1d7)
void advManager::EraseObj(class mapCell* cell, int x, int y) {
    signed char erased = 0;
    int j;
    int i;

    erased = 1;
    cell->m_triggerType = MAP_OBJECT_NONE;
    cell->m_objectIndex = MAP_CELL_NO_FRAME;
    if ((cell->m_secondaryTrigger & MAP_TRIGGER_TYPE_MASK) > 0
        && (cell->m_secondaryTrigger & MAP_TRIGGER_TYPE_MASK) < 0x7f) {
        cell->m_triggerType = cell->m_secondaryTrigger & MAP_TRIGGER_TYPE_MASK;
        cell->m_secondaryTrigger = cell->m_secondaryTrigger - cell->m_triggerType;
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
VA(0x0045fed1, 0xcd)
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
VA(0x0045ff9e, 0x1bf)
void advManager::TownEvent(class mapCell* cell, int x, int y) {
    hero* curHero;
    int result;
    hero* defender;
    town* townRec;

    townRec = gpGame->GetTown(cell->m_objectMetadata);
    curHero = gpGame->GetHero(gpCurPlayer->m_currentHero);
    DemobilizeCurrHero();
    if (townRec->m_owner == giCurPlayer) {
        townRec->m_occupyingHeroId = gpCurPlayer->CurrentHero();
        townRec->View();
    } else if (townRec->HasGarrison()) {
        defender = townRec->m_occupyingHeroId == TOWN_OCCUPYING_HERO_NONE
                       ? NULL
                       : gpGame->GetHero(townRec->m_occupyingHeroId);
        result = DoCombat(
            x,
            y,
            curHero,
            &curHero->m_army,
            townRec,
            defender,
            &townRec->m_army,
            x,
            y,
            COMBAT_RANDOM_SEED_NEW,
            1
        );
        if (result == COMBAT_RESULT_ATTACKER)
            gpGame->ClaimTown(townRec->m_id, giCurPlayer);
    } else {
        gpGame->ClaimTown(townRec->m_id, giCurPlayer);
        UpdateRadar(1, 0);
        UpdateHeroLocators(1, 1);
        UpdateTownLocators(1, 1);
        townRec->m_occupyingHeroId = gpCurPlayer->CurrentHero();
        townRec->View();
    }
    townRec->GiveSpells();
    curHero->CheckLevel();
}

// Adventure-event music cue; HoMM1 keys the ambient track off the map
// object type and records that an event track is playing.
VA(0x0046015d, 0x243)
void advManager::EventSound(short eventType, short eventData) {
    int musicTrack = MUSIC_TRACK_NONE;

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
        gpSoundManager->SwitchAmbientMusic(musicTrack);
        gbEventMusicPlaying = 1;
    } else {
        gbEventMusicPlaying = 0;
    }
}

// donor PoL RVA 0x000aff6c; preferred Buka symbol ?EventWindow@advManager@@QAEXHHPADHHHHH@Z
// donor Buka TU SOURCE/EVENTS; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.574046;margin=0.505217;shape=0.246;size=0.761;calls=0.800;strings=Event ID %d;alternate=pol20:void advManager::EventWindow(int, int, char *, int, int, int, int, int)@0x000aff6c
VA(0x004603a0, 0xde)
void advManager::EventWindow(
    short eventId,
    H1_ENUM_PARAM(NormalDialogType, int) buttons,
    char* text,
    H1_ENUM_PARAM(NormalDialogResourceType, int) type1,
    int value1,
    H1_ENUM_PARAM(NormalDialogResourceType, int) type2,
    int value2,
    H1_ENUM_PARAM(NormalDialogOrText, int) showOrText
) {
    int unusedValue1;
    int unusedValue7;
    int finished;
    int unusedValue8;
    int unusedValue9;
    int unusedValue11;
    int unusedValue12;
    char eventText[EVENT_TEXT_BUFFER_SIZE];
    short unusedStyle;

    finished = 0;
    GrabScreen();
    unusedStyle = 1;
    if (eventId >= 0 && eventId < EVENT_TEXT_WINDOW_END)
        sprintf(eventText, gEventText[eventId]);
    else if (eventId == EVENT_TEXT_CUSTOM)
        sprintf(eventText, text);
    else
        sprintf(eventText, "Event ID %d", eventId);
    NormalDialog(eventText, buttons, 0x61, -1, type1, value1, type2, value2, showOrText);
}

VA(0x0046047e, 0xa9)
short advManager::GiveArtifact(class hero* eventHero, signed char artifact) {
    short slot;

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
VA(0x00460527, 0x5f)
int advManager::GiveRandomArtifact(class hero* eventHero) {
    signed char artifact;

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
VA(0x00460586, 0xb0)
#line 1110 "D:\\Heroes\\Source\\EVENTS.CPP"
int advManager::GiveExperience(class hero* eventHero, int experience, signed char checkLevel) {
    int prevLevel;
    int unusedValue1;
    int unusedValue2;
    int newLevel;
    int levelGap;

    prevLevel = eventHero->GetLevel(eventHero->m_experience);
    eventHero->m_level = prevLevel;
    eventHero->m_experience += experience;
#line 1118
    ProcessAssert(experience >= 0, __FILE__, __LINE__);
#line 1119
    ProcessAssert(eventHero->m_experience >= 0, __FILE__, __LINE__);
    newLevel = eventHero->GetLevel(eventHero->m_experience);
    if (checkLevel)
        eventHero->CheckLevel();
    return newLevel - prevLevel;
}

VA(0x00460636, 0x5a)
void advManager::GiveResource(class hero* eventHero, signed char resource, short amount) {
    if (resource >= 0 && resource <= RESOURCE_LAST)
        gpGame->m_players[eventHero->m_owner].m_resources[resource] += amount;
}

// donor PoL RVA 0x000b022e; preferred Buka symbol ?RecruitEvent@advManager@@QAEXPAVhero@@HPAVmapCell@@@Z
// donor Buka TU SOURCE/EVENTS; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.608108;margin=0.082972;shape=0.542;size=0.949;calls=0.833;alternate=pol20:void advManager::RecruitEvent(class hero *, int, class mapCell *)@0x000b022e
VA(0x00460690, 0xec)
void advManager::RecruitEvent(class hero* eventHero, int creatureType, class mapCell* cell) {
    tag_message message;
    short availableCount;
    recruitUnit* recruitWindow;
    int result;

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
VA(0x0046077c, 0x2e0)
signed char
advManager::GhostEvent(class hero* eventHero, class mapCell* cell, int textId, int x, int y) {
    int artifact;

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
VA(0x00460a5c, 0x11e)
void advManager::HouseEvent(class hero* eventHero, class mapCell* cell) {
    short houseIndex;

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
        signed char creatures[EVENT_HOUSE_COUNT] =
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

VA(0x00460b7a, 0x200)
signed char advManager::CombatMonsterEvent(
    class hero* eventHero,
    signed char monsterType,
    short count,
    class mapCell* cell,
    int x,
    int y,
    signed char heroDefends,
    int fromX,
    int fromY
) {
    short i;
    int res;

    DemobilizeCurrHero();
    if (fromX == -1) {
        fromX = x;
        fromY = y;
    } else {
        m_lastQuickViewX = fromX;
        m_lastQuickViewY = fromY;
        if (eventHero->m_x >= fromX)
            m_mineGuardianFacingLeft = 0;
        else
            m_mineGuardianFacingLeft = 1;
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
VA(0x00460d7a, 0x243)
void advManager::GiveTakeArtifactStat(
    class hero* targetHero,
    signed char artifact,
    signed char take
) {
    signed char stat = HERO_PRIMARY_NONE;
    signed char amount = 0;
    int i;

    switch (artifact) {
        case ARTIFACT_ULTIMATE_BOOK:
            stat = HERO_PRIMARY_KNOWLEDGE;
            amount = 12;
            break;
        case ARTIFACT_ULTIMATE_SWORD:
            stat = HERO_PRIMARY_ATTACK;
            amount = 12;
            break;
        case ARTIFACT_ULTIMATE_CLOAK:
            stat = HERO_PRIMARY_DEFENSE;
            amount = 12;
            break;
        case ARTIFACT_ULTIMATE_WAND:
            stat = HERO_PRIMARY_SPELL_POWER;
            amount = 12;
            break;
        case ARTIFACT_ARCANE_NECKLACE:
            stat = HERO_PRIMARY_SPELL_POWER;
            amount = 4;
            break;
        case ARTIFACT_CASTERS_BRACELET:
        case ARTIFACT_MAGES_RING:
            stat = HERO_PRIMARY_SPELL_POWER;
            amount = 2;
            break;
        case ARTIFACT_WITCHS_BROACH:
            stat = HERO_PRIMARY_SPELL_POWER;
            amount = 3;
            break;
        case ARTIFACT_THUNDER_MACE:
        case ARTIFACT_GIANT_FLAIL:
            stat = HERO_PRIMARY_ATTACK;
            amount = 1;
            break;
        case ARTIFACT_ARMORED_GAUNTLETS:
        case ARTIFACT_DEFENDER_HELM:
            stat = HERO_PRIMARY_DEFENSE;
            amount = 1;
            break;
        case ARTIFACT_BALLISTA:
            stat = HERO_PRIMARY_BALLISTA;
            amount = 3;
            break;
        case ARTIFACT_STEALTH_SHIELD:
            stat = HERO_PRIMARY_DEFENSE;
            amount = 2;
            break;
        case ARTIFACT_DRAGON_SWORD:
            stat = HERO_PRIMARY_ATTACK;
            amount = 3;
            break;
        case ARTIFACT_POWER_AXE:
            stat = HERO_PRIMARY_ATTACK;
            amount = 2;
            break;
        case ARTIFACT_DIVINE_BREASTPLATE:
            stat = HERO_PRIMARY_DEFENSE;
            amount = 3;
            break;
        case ARTIFACT_MINOR_SCROLL:
            stat = HERO_PRIMARY_KNOWLEDGE;
            amount = 2;
            break;
        case ARTIFACT_MAJOR_SCROLL:
            stat = HERO_PRIMARY_KNOWLEDGE;
            amount = 3;
            break;
        case ARTIFACT_SUPERIOR_SCROLL:
            stat = HERO_PRIMARY_KNOWLEDGE;
            amount = 4;
            break;
        case ARTIFACT_FOREMOST_SCROLL:
            stat = HERO_PRIMARY_KNOWLEDGE;
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
    if (stat != HERO_PRIMARY_NONE) {
        targetHero->m_primaryStats[stat] += amount;
        if (amount < 0 && stat == HERO_PRIMARY_KNOWLEDGE) {
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
VA(0x00460fbd, 0x200)
void advManager::TransferArtifacts(class hero* sourceHero, class hero* destHero) {
    short i;
    short j;

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
                                "As you reach for the %s, it mysteriously disappears.",
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
VA(0x004611bd, 0x7d)
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
VA(0x0046123a, 0x137)
void advManager::DoWhirlpool(class hero* eventHero) {
    int weakest;
    short slotNo;
    int groupValues[ARMY_GROUP_SLOT_COUNT];
    long worth;
    long lowestValue;

    if (!gbHumanPlayer[eventHero->m_owner])
        return;
    if (Random(EVENT_WHIRLPOOL_TRIGGER_ROLL, EVENT_WHIRLPOOL_TRIGGER_MAX)
        != EVENT_WHIRLPOOL_TRIGGER_ROLL)
        return;
    lowestValue = EVENT_WHIRLPOOL_ARMY_VALUE_LIMIT;
    weakest = -1;
    for (slotNo = 0; slotNo < ARMY_GROUP_SLOT_COUNT; slotNo++) {
        if (eventHero->m_army.m_creatureCounts[slotNo] > 0) {
            worth = gMonsterDatabase[eventHero->m_army.m_creatureTypes[slotNo]].fightValue
                    * eventHero->m_army.m_creatureCounts[slotNo];
            if (lowestValue > worth) {
                lowestValue = worth;
                weakest = slotNo;
            }
        }
    }
    if (eventHero->m_army.GetNumArmies() > 1) {
        eventHero->m_army.m_creatureCounts[weakest] >>= 1;
        if (!eventHero->m_army.m_creatureCounts[weakest])
            eventHero->m_army.m_creatureTypes[weakest] = CREATURE_NONE;
    } else if (eventHero->m_army.m_creatureCounts[weakest] > 1) {
        eventHero->m_army.m_creatureCounts[weakest] >>= 1;
    }
}

// donor PoL RVA 0x000b1d01; preferred Buka symbol ?FizzleCenter@advManager@@QAEXH@Z
// donor Buka TU SOURCE/EVENTS; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.628535;margin=0.385528;shape=0.317;size=0.884;calls=0.800;strings=killfade.82M|pickup%02d.82M;alternate=pol20:void advManager::FizzleCenter(int)@0x000b1d01
VA(0x00461371, 0x113)
void advManager::FizzleCenter(int fizzleType) {
    SAMPLE2 fizzleSample;

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
    fizzleSample = NULL_SAMPLE2;
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
    WaitEndSample(fizzleSample, SAMPLE_WAIT_DEFAULT);
}

// donor PoL RVA 0x000b1e43; preferred Buka symbol ?DoAIEvent@advManager@@QAEXPAVmapCell@@PAVhero@@HH@Z
// donor Buka TU SOURCE/EVENTS; HoMM1 owner inferred from contiguous order
// evidence: graph:7;base=0.283401;margin=1.483201;shape=0.274;size=0.397;calls=0.409;alternate=pol20:void advManager::DoAIEvent(class mapCell *, class hero *, int, int)@0x000b1e43
VA(0x00461484, 0x1141)
void advManager::DoAIEvent(class mapCell* cell, class hero* eventHero, int x, int y) {
    int troopType;
    int available;
    int purchaseValue;
    int bestSlot;
    playerData* origPlayerData;
    signed char eventType;
    int numHired;
    int counter;
    town* theCastle;
    int junk[4];
    int savedPlayer;
    signed char erase;
    int handled;
    int battleResult;
    int win;
    boatRecord* ship;
    signed char oldShowIt;
    int strength;
    int resType;
    signed char ty;
    signed char tx;
    signed char teleportCount;
    int res;
    int cost[RESOURCE_COUNT];
    int victory;
    signed char adjacentMonster;
    hero* enemyHero;
    float heroLosses;
    float theirLosses;

    theCastle = NULL;
    eventType = cell->m_triggerType & MAP_TRIGGER_TYPE_MASK;
    erase = 0;
    handled = 0;
    savedPlayer = giCurPlayer;
    origPlayerData = gpCurPlayer;
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
                CheckAdjacentMon(&adjacentMonster);
            }
            break;
        case MAP_OBJECT_SHIP:
            ship = &gpGame->m_boats[cell->m_objectMetadata];
            gpGame->RestoreCell(-1, -1, ship->savedTriggerType, ship->savedEventData, cell, 3);
            eventHero->m_eventFlags |= HERO_EVENT_EMBARKED;
            eventHero->m_remainingMobility = 0;
            ship->heroId = eventHero->m_id;
            ship->owner = eventHero->m_owner;
            m_cursorType = ADVMGR_HERO_ICON_BOAT;
            m_cursorDirection = ship->direction;
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
            for (counter = 0; counter < ARMY_GROUP_SLOT_COUNT; counter++) {
                gpMonGroup->m_creatureTypes[counter] = CREATURE_DRAGON;
                gpMonGroup->m_creatureCounts[counter] = 1;
            }
            gpPhilAI->ChooseEvaluateBattle(
                &eventHero->m_army,
                eventHero,
                gpMonGroup,
                NULL,
                0,
                0,
                500,
                win,
                strength
            );
            if (win) {
                counter = DRAGON_CITY_DRAGON_COUNT;
                victory = gpPhilAI->CombatMonsterEvent(eventHero, CREATURE_DRAGON, &counter, cell);
                if (victory)
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
            erase = 1;
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
            erase = 1;
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
            resType = cell->m_objectIndex - RESOURCE_PILE_OBJECT_BASE;
            GiveResource(
                eventHero,
                resType,
                resType == RESOURCE_GOLD ? cell->m_objectMetadata * RESOURCE_PILE_GOLD_MULTIPLIER
                                         : cell->m_objectMetadata
            );
            erase = 1;
            break;
        case MAP_OBJECT_WINDMILL:
            if (cell->m_objectMetadata != WINDMILL_EMPTY) {
                GiveResource(eventHero, cell->m_objectMetadata, WINDMILL_RESOURCE_AMOUNT);
                cell->m_objectMetadata = WINDMILL_EMPTY;
            }
            break;
        case MAP_OBJECT_ANCIENT_LAMP:
            troopType = CREATURE_GENIE;
            available = 0;
            goto recruit;
        case MAP_OBJECT_WAGON_CAMP:
            troopType = CREATURE_ROGUE;
            available = 0;
            goto recruit;
        case MAP_OBJECT_DESERT_TENT:
            troopType = CREATURE_NOMAD;
            available = 0;
            goto recruit;
        case MAP_OBJECT_STRAW_HUT:
            troopType = CREATURE_GOBLIN;
            available = 1;
            goto recruit;
        case MAP_OBJECT_HOUSE:
            troopType = CREATURE_PEASANT;
            available = 1;
            goto recruit;
        case MAP_OBJECT_CABIN:
            troopType = CREATURE_ARCHER;
            available = 1;
            goto recruit;
        case MAP_OBJECT_DWARF_LOG_CABIN:
            troopType = CREATURE_DWARF;
            available = 1;
            goto recruit;
        case MAP_OBJECT_PEASANT_LOG_CABIN:
            troopType = CREATURE_PEASANT;
            available = 1;
            goto recruit;
        recruit:
            if (cell->m_objectMetadata) {
                gpPhilAI->EvaluateOneTimeCreaturePurchase(
                    eventHero,
                    troopType,
                    cell->m_objectMetadata,
                    available,
                    numHired,
                    purchaseValue,
                    bestSlot
                );
                if (numHired > 0) {
                    gpGame->GiveArmy(&eventHero->m_army, troopType, numHired, bestSlot);
                    cell->m_objectMetadata = cell->m_objectMetadata - numHired;
                    if (!available) {
                        GetMonsterCost(troopType, cost);
                        for (counter = 0; counter < RESOURCE_COUNT; counter++)
                            gpCurPlayer->m_resources[counter] -= -(-(cost[counter] * numHired));
                    }
                }
            }
            if (!cell->m_objectMetadata && eventType == MAP_OBJECT_ANCIENT_LAMP)
                erase = 1;
            break;
        case MAP_OBJECT_MONSTER:
            ComputerMonsterInteract(cell, eventHero, &erase);
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
            teleportCount = 0;
            for (ty = 0; ty < MAP_CELL_GRID_SIZE; ty++) {
                for (tx = 0; tx < MAP_CELL_GRID_SIZE; tx++) {
                    if (gpGame->m_map[tx][ty].m_triggerType
                            == (unsigned char)(eventType | MAP_TRIGGER_EVENT)
                        && MANHATTAN_LENGTH(tx - x, ty - y)
                               > (eventType == MAP_OBJECT_STONE_LITHS ? STONE_LITHS_MIN_DISTANCE
                                                                      : WHIRLPOOL_MIN_DISTANCE))
                        teleportCount++;
                }
            }
            if (teleportCount >= 1) {
                if (teleportCount > 1)
                    teleportCount = Random(1, teleportCount);
                for (ty = 0; ty < MAP_CELL_GRID_SIZE; ty++) {
                    for (tx = 0; tx < MAP_CELL_GRID_SIZE; tx++) {
                        if (gpGame->m_map[tx][ty].m_triggerType
                                == (unsigned char)(eventType | MAP_TRIGGER_EVENT)
                            && MANHATTAN_LENGTH(tx - x, ty - y)
                                   > (eventType == MAP_OBJECT_STONE_LITHS
                                          ? STONE_LITHS_MIN_DISTANCE
                                          : WHIRLPOOL_MIN_DISTANCE)) {
                            if (--teleportCount <= 0)
                                goto teleport;
                        }
                    }
                }
            teleport:
                StopCursor(1);
                gpAdvManager->TeleportTo(tx, ty, 0);
            }
            break;
        case MAP_OBJECT_ARTIFACT:
            switch (cell->m_objectMetadata) {
                case ARTIFACT_EVENT_MODE_PICKUP:
                giveArtifact:
                    GiveArtifact(eventHero, cell->m_objectIndex);
                    erase = 1;
                    break;
                case ARTIFACT_EVENT_MODE_GUARDED:
                    counter = ARTIFACT_EVENT_GUARD_ROGUE_COUNT;
                    if (gpPhilAI->CombatMonsterEvent(eventHero, CREATURE_ROGUE, &counter, cell))
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
                        erase = 1;
                    }
                    break;
            }
            break;
        case MAP_OBJECT_HERO:
            enemyHero = gpGame->GetHero(cell->m_objectMetadata);
            oldShowIt = bShowIt;
            if (enemyHero->m_owner == giCurPlayer)
                return;
            if (enemyHero->m_locationType == (MAP_TRIGGER_EVENT | MAP_OBJECT_TOWN))
                theCastle = gpGame->GetTown(enemyHero->m_occupiedTown);
            if (!gbHumanPlayer[enemyHero->m_owner]) {
                battleResult = gpPhilAI->QuickCombat(
                    &eventHero->m_army,
                    eventHero,
                    &enemyHero->m_army,
                    enemyHero,
                    0,
                    0,
                    heroLosses,
                    theirLosses
                );
                if (battleResult && theCastle)
                    battleResult = gpPhilAI->QuickCombat(
                        &eventHero->m_army,
                        eventHero,
                        &theCastle->m_army,
                        NULL,
                        1,
                        theCastle->m_id,
                        heroLosses,
                        theirLosses
                    );
            } else {
                if (theCastle)
                    theCastle->m_occupyingHeroId = enemyHero->m_id;
                res = DoCombat(
                    x,
                    y,
                    eventHero,
                    &eventHero->m_army,
                    theCastle,
                    enemyHero,
                    &enemyHero->m_army,
                    x,
                    y,
                    COMBAT_RANDOM_SEED_NEW,
                    1
                );
                if (res == COMBAT_RESULT_ATTACKER && theCastle)
                    gpGame->ClaimTown(theCastle->m_id, giCurPlayer);
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
    if (erase)
        EraseObj(cell, x, y);
    giCurPlayer = savedPlayer;
    gpCurPlayer = origPlayerData;
    CheckEndGame(0);
}

// donor PoL RVA 0x000b4fd5; preferred Buka symbol ?PlayerMonsterInteract@advManager@@QAEXPAVmapCell@@0PAVhero@@PAHHHHHH@Z
// donor Buka TU SOURCE/EVENTS; HoMM1 owner inferred from contiguous order
// evidence: graph:4;base=0.214069;margin=0.493239;shape=0.272;size=0.223;calls=0.212;alternate=pol20:void advManager::PlayerMonsterInteract(class mapCell *, class mapCell *, class hero *, int *, int, int, int, int, int)@0x000b4fd5
VA(0x004625c5, 0x19a)
void advManager::PlayerMonsterInteract(
    class mapCell* cell,
    class mapCell* combatCell,
    class hero* eventHero,
    signed char* handled,
    int x,
    int y,
    signed char unused,
    int combatX,
    int combatY
) {
    int result;

    unused = 0;
    if (cell->m_objectMetadata & MONSTER_WILLING_FLAG) {
        if (gpPhilAI->FightValueOfStack(&eventHero->m_army, eventHero, 0, 0, 0)
            > gMonsterDatabase[cell->m_objectIndex].fightValue
                  * (cell->m_objectMetadata & MONSTER_COUNT_MASK) * 1.75) {
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
VA(0x0046275f, 0x152)
void advManager::ComputerMonsterInteract(
    class mapCell* cell,
    class hero* eventHero,
    signed char* handled
) {
    int numToBuy;
    int purchaseValue;
    int bestSlot;
    int result;
    int creatureCount;

    if (cell->m_objectMetadata & MONSTER_WILLING_FLAG
        && gpPhilAI->FightValueOfStack(&eventHero->m_army, eventHero, 0, 0, 0)
               > gMonsterDatabase[cell->m_objectIndex].fightValue
                     * (cell->m_objectMetadata & MONSTER_COUNT_MASK) * 1.75) {
        gpPhilAI->EvaluateOneTimeCreaturePurchase(
            eventHero,
            cell->m_objectIndex,
            cell->m_objectMetadata & MONSTER_COUNT_MASK,
            1,
            numToBuy,
            purchaseValue,
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
        creatureCount = cell->m_objectMetadata & MONSTER_COUNT_MASK;
        result = gpPhilAI->CombatMonsterEvent(eventHero, cell->m_objectIndex, &creatureCount, cell);
        cell->m_objectMetadata = (cell->m_objectMetadata & MONSTER_WILLING_FLAG) + creatureCount;
        if (result)
            *handled = 1;
    }
}

// donor PoL RVA 0x000b5c40; preferred Buka symbol ?DoNetCombat@advManager@@QAEHPAD@Z
// donor Buka TU SOURCE/EVENTS; HoMM1 owner inferred from contiguous order
// evidence: graph:3;base=0.634004;margin=0.818203;shape=0.529;size=0.995;calls=1.000;alternate=pol20:int advManager::DoNetCombat(char *)@0x000b5c40
VA(0x004628b1, 0x18f)
int advManager::DoNetCombat(char* packet) {
    hero* defendingHero;
    int cellY;
    int cellX;
    int seed;
    int opponent;
    signed char result;
    int side;
    hero* attackingHero;
    int srcY;
    int srcX;
    armyGroup* defendArmy;
    armyGroup* attArmy;
    town* siegeTown;
    int unused;
    int unused2;

    attackingHero = NULL;
    attArmy = NULL;
    siegeTown = NULL;
    defendingHero = NULL;
    defendArmy = NULL;
    ReceiveHeroTownData(
        packet,
        &opponent,
        &cellX,
        &cellY,
        &attackingHero,
        &attArmy,
        &siegeTown,
        &defendingHero,
        &defendArmy,
        &srcX,
        &srcY,
        &seed,
        &result,
        &gbRetreatWin,
        &gbCombatSurrender
    );
    side = attackingHero->m_owner;
    result = DoCombat(
        cellX,
        cellY,
        attackingHero,
        attArmy,
        siegeTown,
        defendingHero,
        defendArmy,
        srcX,
        srcY,
        seed,
        0
    );
    if (!gbHumanPlayer[side])
        SendHeroTownData(
            cellX,
            cellY,
            attackingHero,
            attArmy,
            siegeTown,
            defendingHero,
            defendArmy,
            srcX,
            srcY,
            seed,
            opponent,
            result,
            gbRetreatWin,
            gbCombatSurrender
        );
    if (attArmy)
        free(attArmy);
    if (defendArmy)
        free(defendArmy);
    if (siegeTown)
        free(siegeTown);
    if (defendingHero)
        free(defendingHero);
    if (attackingHero)
        free(attackingHero);
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
    signed char fragment;
    signed char x;
    signed char y;
    signed char hasFirstHero;
    signed char hasTown;
    signed char hasSecondHero;
    signed char setupCombatX;
    signed char setupCombatY;
    int randomSeed;
    signed char combatResult;
    signed char retreatWin;
    signed char combatSurrender;
    signed char firstOwner;
    int firstGold;
    signed char secondOwner;
    int secondGold;
    armyGroup firstArmy;
    armyGroup secondArmy;
    town combatTown;
};

struct combatRemoteHeroFragment {
    signed char fragment;
    char data[sizeof(hero)];
};

struct combatRemoteMessage {
    signed char sender;
    int id;
    signed char type;
    signed char command;
    short payloadSize;
    combatRemoteData combat;
};

struct heroRemoteMessage {
    signed char sender;
    int id;
    signed char type;
    signed char command;
    short payloadSize;
    combatRemoteHeroFragment heroFragment;
};
#pragma pack(pop)

// donor PoL RVA 0x000b5e10; preferred Buka symbol ?DoCombat@advManager@@QAEHHHPAVhero@@PAVarmyGroup@@PAVtown@@01HHHH@Z
// donor Buka TU SOURCE/EVENTS; HoMM1 owner inferred from contiguous order
// evidence: graph:3;base=0.590184;margin=0.564001;shape=0.455;size=0.978;calls=0.927;alternate=pol20:int advManager::DoCombat(int, int, class hero *, class armyGroup *, class town *, class hero *, class armyGroup *, int, int, int, int)@0x000b5e10
VA(0x00462a40, 0x5c6)
int advManager::DoCombat(
    int x,
    int y,
    class hero* firstHero,
    class armyGroup* firstArmy,
    class town* combatTown,
    class hero* secondHero,
    class armyGroup* secondArmy,
    int setupCombatX,
    int setupCombatY,
    int randomSeed,
    signed char processLosses
) {
    armyGroup* army2Net;
    hero* hero2Net;
    hero* hero1Net;
    armyGroup* army1Net;
    town* townNet;
    int sender;
    char* receivedPacket;
    signed char combatResult;
    tag_message message;
    int defendPlayer;
    int attackPlayer;
    int savedPlayer;
    signed char savedShowIt;
    int unused;

    gbInCombat = 1;
    attackPlayer = firstHero ? firstHero->m_owner : -1;
    if (secondHero)
        defendPlayer = secondHero->m_owner;
    else if (combatTown)
        defendPlayer = combatTown->m_owner;
    else
        defendPlayer = GAME_PLAYER_NONE;
    if (randomSeed == COMBAT_RANDOM_SEED_NEW)
        randomSeed = Random(1, COMBAT_RANDOM_SEED_MAX);
    DemobilizeCurrHero();
    savedPlayer = giCurPlayer;
    savedShowIt = bShowIt;

    if (attackPlayer >= 0 && defendPlayer >= 0 && gbHumanPlayer[defendPlayer]) {
        if (!gbThisNetHumanPlayer[defendPlayer]) {
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
                defendPlayer,
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
                        switch (((combatRemoteMessage*)receivedPacket)->command) {
                            case COMBAT_REMOTE_COMMAND:
                                ReceiveHeroTownData(
                                    receivedPacket,
                                    &sender,
                                    &x,
                                    &y,
                                    &hero1Net,
                                    &army1Net,
                                    &townNet,
                                    &hero2Net,
                                    &army2Net,
                                    &setupCombatX,
                                    &setupCombatY,
                                    &randomSeed,
                                    &combatResult,
                                    &gbRetreatWin,
                                    &gbCombatSurrender
                                );
                                if (army1Net) {
                                    memcpy(firstArmy, army1Net, sizeof(armyGroup));
                                    free(army1Net);
                                }
                                if (army2Net) {
                                    memcpy(secondArmy, army2Net, sizeof(armyGroup));
                                    free(army2Net);
                                }
                                if (townNet) {
                                    memcpy(combatTown, townNet, sizeof(town));
                                    free(townNet);
                                }
                                if (hero2Net) {
                                    memcpy(secondHero, hero2Net, sizeof(hero));
                                    free(hero2Net);
                                }
                                if (hero1Net) {
                                    memcpy(firstHero, hero1Net, sizeof(hero));
                                    free(hero1Net);
                                }
                                gpCombatManager->m_combatResult = combatResult;
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
                "%s player\'s %s is under attack!",
                gColorNames[gpGame->m_players[defendPlayer].m_color],
                combatTown ? "Town" : "Hero"
            );
            gText[0] -= 'a' - 'A';
            gpGame->WaitForPlayer(gText, defendPlayer);
        }
    }

    bShowIt = 1;
    if (giEventMusicVolume != EVENT_MUSIC_VOLUME_NONE)
        gConfig.musicVolume = giEventMusicVolume;
    giEventMusicVolume = EVENT_MUSIC_VOLUME_NONE;
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
    if (giHighMemBuffer > COMBAT_HIGH_MEMORY_LIMIT)
        gAdvDisposeLevel = ADV_DISPOSE_FULL;
    else if (giHighMemBuffer > COMBAT_LOW_MEMORY_LIMIT)
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
    bShowIt = savedShowIt;
    giCurPlayer = savedPlayer;
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
    gbInCombat = 0;
    return gpCombatManager->m_combatResult;
}

// donor PoL RVA 0x000b645e; preferred Buka symbol ?SendHeroTownData@advManager@@QAEXHHPAVhero@@PAVarmyGroup@@PAVtown@@01HHHHHHH@Z
// donor Buka TU SOURCE/EVENTS; HoMM1 owner inferred from contiguous order
// evidence: graph:4;base=0.543308;margin=0.967008;shape=0.438;size=0.943;calls=0.684;alternate=pol20:void advManager::SendHeroTownData(int, int, class hero *, class armyGroup *, class town *, class hero *, class armyGroup *, int, int, int, int, int, int, int)@0x000b645e
VA(0x00463006, 0x2da)
void advManager::SendHeroTownData(
    int x,
    int y,
    class hero* firstHero,
    class armyGroup* firstArmy,
    class town* combatTown,
    class hero* secondHero,
    class armyGroup* secondArmy,
    int setupCombatX,
    int setupCombatY,
    int randomSeed,
    signed char remotePlayer,
    signed char combatResult,
    signed char retreatWin,
    signed char combatSurrender
) {
    char* reply;
    int result;
    combatRemoteData* buf = NULL;

    buf = static_cast<combatRemoteData*>(malloc(COMBAT_REMOTE_BUFFER_SIZE));
    reply = NULL;
    buf->fragment = COMBAT_REMOTE_FRAGMENT_COMBAT;
    buf->x = x;
    buf->y = y;
    buf->hasFirstHero = firstHero != NULL;
    buf->hasTown = combatTown != NULL;
    buf->hasSecondHero = secondHero != NULL;
    buf->setupCombatX = setupCombatX;
    buf->setupCombatY = setupCombatY;
    buf->randomSeed = randomSeed;
    buf->combatResult = combatResult;
    buf->retreatWin = retreatWin;
    buf->combatSurrender = combatSurrender;
    buf->firstOwner = firstHero ? firstHero->m_owner : -1;
    buf->firstGold =
        firstHero ? gpGame->m_players[firstHero->m_owner].m_resources[RESOURCE_GOLD] : 0;
    buf->secondOwner = secondHero ? secondHero->m_owner : -1;
    buf->secondGold =
        secondHero ? gpGame->m_players[secondHero->m_owner].m_resources[RESOURCE_GOLD] : 0;
    memcpy(&buf->firstArmy, firstArmy, sizeof(armyGroup));
    memcpy(&buf->secondArmy, secondArmy, sizeof(armyGroup));
    if (combatTown)
        memcpy(&buf->combatTown, combatTown, sizeof(town));

    // API-forced: TransmitAndWait/TransmitRemoteData take char* payloads.
    result = TransmitAndWait(
        reinterpret_cast<char*>(buf),
        remotePlayer,
        sizeof(combatRemoteData),
        COMBAT_REMOTE_COMMAND,
        COMBAT_REMOTE_CONFIRM_COMMAND,
        &reply
    );
    if (!result)
        ShutDown(NULL);

    if (firstHero) {
        ((combatRemoteHeroFragment*)buf)->fragment = COMBAT_REMOTE_FRAGMENT_FIRST_HERO;
        memcpy(((combatRemoteHeroFragment*)buf)->data, firstHero, sizeof(hero));
        // API-forced: TransmitRemoteData takes a char* payload.
        result = TransmitRemoteData(
            reinterpret_cast<char*>(buf),
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
        ((combatRemoteHeroFragment*)buf)->fragment = COMBAT_REMOTE_FRAGMENT_SECOND_HERO;
        memcpy(((combatRemoteHeroFragment*)buf)->data, secondHero, sizeof(hero));
        // API-forced: TransmitRemoteData takes a char* payload.
        result = TransmitRemoteData(
            reinterpret_cast<char*>(buf),
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
    free(buf);
}

// donor PoL RVA 0x000b67cd; preferred Buka symbol ?ReceiveHeroTownData@advManager@@QAEXPADPAH11PAPAVhero@@PAPAVarmyGroup@@PAPAVtown@@23111PAC55@Z
// donor Buka TU SOURCE/EVENTS; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.493176;margin=0.152223;shape=0.314;size=0.857;calls=0.909;alternate=pol20:void advManager::ReceiveHeroTownData(char *, int *, int *, int *, class hero * *, class armyGroup * *, class town * *, class hero * *, class armyGroup * *, int *, int *, int *, signed char *, signed char *, signed char *)@0x000b67cd
VA(0x004632e0, 0x34c)
void advManager::ReceiveHeroTownData(
    char* packet,
    int* remotePlayer,
    int* x,
    int* y,
    class hero** firstHero,
    class armyGroup** firstArmy,
    class town** combatTown,
    class hero** secondHero,
    class armyGroup** secondArmy,
    int* setupCombatX,
    int* setupCombatY,
    int* randomSeed,
    signed char* combatResult,
    signed char* retreatWin,
    signed char* combatSurrender
) {
    signed char hasTown;
    int result;
    long lastPacketTime;
    signed char firstOwner;
    signed char defenderOwner;
    signed char bFirstHero;
    signed char hasSecondHero;

    *firstHero = NULL;
    *firstArmy = NULL;
    *combatTown = NULL;
    *secondHero = NULL;
    *secondArmy = NULL;
    bFirstHero = hasSecondHero = hasTown = 0;
    *remotePlayer = ((combatRemoteMessage*)packet)->sender;
    *x = ((combatRemoteMessage*)packet)->combat.x;
    *y = ((combatRemoteMessage*)packet)->combat.y;
    bFirstHero = ((combatRemoteMessage*)packet)->combat.hasFirstHero;
    hasTown = ((combatRemoteMessage*)packet)->combat.hasTown;
    hasSecondHero = ((combatRemoteMessage*)packet)->combat.hasSecondHero;
    *setupCombatX = ((combatRemoteMessage*)packet)->combat.setupCombatX;
    *setupCombatY = ((combatRemoteMessage*)packet)->combat.setupCombatY;
    *randomSeed = ((combatRemoteMessage*)packet)->combat.randomSeed;
    *combatResult = ((combatRemoteMessage*)packet)->combat.combatResult;
    *retreatWin = ((combatRemoteMessage*)packet)->combat.retreatWin;
    *combatSurrender = ((combatRemoteMessage*)packet)->combat.combatSurrender;
    firstOwner = ((combatRemoteMessage*)packet)->combat.firstOwner;
    if (firstOwner > 0)
        gpGame->m_players[firstOwner].m_resources[RESOURCE_GOLD] =
            ((combatRemoteMessage*)packet)->combat.firstGold;
    defenderOwner = ((combatRemoteMessage*)packet)->combat.secondOwner;
    if (defenderOwner > 0)
        gpGame->m_players[defenderOwner].m_resources[RESOURCE_GOLD] =
            ((combatRemoteMessage*)packet)->combat.secondGold;

    *firstArmy = static_cast<armyGroup*>(malloc(sizeof(armyGroup)));
    memcpy(*firstArmy, &((combatRemoteMessage*)packet)->combat.firstArmy, sizeof(armyGroup));
    *secondArmy = static_cast<armyGroup*>(malloc(sizeof(armyGroup)));
    memcpy(*secondArmy, &((combatRemoteMessage*)packet)->combat.secondArmy, sizeof(armyGroup));
    if (hasTown) {
        *combatTown = static_cast<town*>(malloc(sizeof(town)));
        memcpy(*combatTown, &((combatRemoteMessage*)packet)->combat.combatTown, sizeof(town));
    }

    result = TransmitRemoteData(
        NULL,
        *remotePlayer,
        0,
        COMBAT_REMOTE_CONFIRM_COMMAND,
        1,
        1,
        REMOTE_MESSAGE_DEFAULT,
        1
    );
    if (!result)
        ShutDown(NULL);

    lastPacketTime = KBTickCount();
    while ((hasSecondHero && !*secondHero) || (bFirstHero && !*firstHero)) {
        PollSound();
        if (KBTickCount() > lastPacketTime + COMBAT_REMOTE_TIMEOUT) {
            NormalDialog(
                "Error receiving data.  Keep trying??",
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
                lastPacketTime = KBTickCount();
            else
                ShutDown("Game canceled.");
        }
        packet = GetRemoteData(1);
        if (packet && ((combatRemoteMessage*)packet)->type == REMOTE_MESSAGE_RELIABLE
            && ((combatRemoteMessage*)packet)->command == COMBAT_REMOTE_COMMAND) {
            lastPacketTime = KBTickCount();
            if (((heroRemoteMessage*)packet)->heroFragment.fragment
                == COMBAT_REMOTE_FRAGMENT_FIRST_HERO) {
                *firstHero = static_cast<hero*>(malloc(sizeof(hero)));
                memcpy(*firstHero, ((heroRemoteMessage*)packet)->heroFragment.data, sizeof(hero));
            }
            if (((heroRemoteMessage*)packet)->heroFragment.fragment
                == COMBAT_REMOTE_FRAGMENT_SECOND_HERO) {
                *secondHero = static_cast<hero*>(malloc(sizeof(hero)));
                memcpy(*secondHero, ((heroRemoteMessage*)packet)->heroFragment.data, sizeof(hero));
            }
        }
    }
}

// EVENTS owns retail .data 0x004a0504-0x004a07bb and .bss 0x004ca904. GiveExperience's
// assertion line is its /Gi compiler line static (1110, docs/patterns/vc4-gi-line-var.md).
DATA(0x004a0504)
int giEventMusicVolume = EVENT_MUSIC_VOLUME_NONE;
DATA(0x004ca904)
signed char gbEventMusicPlaying;

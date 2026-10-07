#include <match.h>

#include <BASE/audio.h>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <BASE/BITS.h>
#include <BASE/BMAP2.h>
#include <BASE/display.h>
#include <BASE/inputManager.h>
#include <BASE/mouseManager.h>
#include <BASE/Misc.h>
#include <BASE/font.h>
#include <BASE/heroWindowManager.h>
#include <BASE/miscwin.h>
#include <SOURCE/advManager.h>
#include <SOURCE/army.h>
#include <SOURCE/armyGroup.h>
#include <SOURCE/combatManager.h>
#include <SOURCE/game.h>
#include <SOURCE/hero.h>
#include <SOURCE/mapCell.h>
#include <SOURCE/philAI.h>
#include <SOURCE/playerData.h>
#include <SOURCE/searchArray.h>
#include <SOURCE/town.h>
#include <SOURCE/townManager.h>
#include <SOURCE/KB.h>
#include <SOURCE/campaignTypes.h>
#include <SOURCE/EVENTS.h>
#include <SOURCE/highScoreManager.h>
#include <SOURCE/kbwin.h>
#include <SOURCE/mapObjectTypes.h>

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

DATA(0x004ca188)
b8 gShowComputerRoute = false;
// No retail code reads this; it holds its retail .bss place.
DATA(0x004ca18c)
i32 gUnusedPhilAIWords[3] = {0, 0, 0};
DATA(0x0049ef78)
float gAttackHumanBonus = 2.0f;
DATA(0x0049ef7c)
float gAttackComputerBonus = 0.8f;
DATA(0x004b7434)
i16 gHeroEventStratRVOfPos[MAP_CELL_GRID_SIZE][MAP_CELL_GRID_SIZE];
DATA(0x004b4bac)
float gBerserkFactor;
DATA(0x004c8cb8)
i32 gLastFrameRateTimer;
DATA(0x004b4aa4)
i8 gCurPlayer;
#define gWinChance gCombatWinningFraction // spelling fixes .bss order
DATA(0x004b4b9c)
float gWinChance;
DATA(0x004b2174)
i32 gEventSlot;
DATA(0x004ca164)
i8 gBuildShipyard[GAME_PLAYER_COUNT];
DATA(0x004ca16c)
i32 gMaxHeroesForThisPlayer;
DATA(0x004b4ba0)
i8 gBuildBoat[GAME_PLAYER_COUNT];
DATA(0x004b220c)
float gReduceFactor;
DATA(0x004c8cd0)
u8 gCurPlayerBit;
DATA(0x004b2170)
i8 gBestShipyardDist;
DATA(0x004c8cc8)
b32 gHeroBuiltThisTurn;
DATA(0x004c8cd4)
i16 gHeroLiveChance[GAME_HERO_COUNT];
#define gAttackerLoss gExpectedOurCasualties // spelling fixes .bss order
DATA(0x004bb11c)
i32 gAttackerLoss;
#define gDefenderLoss gOpponentLosses // spelling fixes .bss order
DATA(0x004bb120)
i32 gDefenderLoss;
DATA(0x004b2218)
i32 gHumanTownConquered;
DATA(0x004c8cc4)
i32 gCurTurn;
DATA(0x004bb100)
H1_ENUM_ARRAY(i32, gCreatureCost, ResourceType, RESOURCE_COUNT);
DATA(0x004b9cc0)
i8 gTurnValueOfMine[MAP_CELL_GRID_SIZE][MAP_CELL_GRID_SIZE];
DATA(0x004b221c)
i32 gDummy;
DATA(0x004b9cb4)
b8 gPossibleShipyardFound;
DATA(0x004bb13c)
H1_ENUM_ARRAY(float, gAITurnCostResource, ResourceType, RESOURCE_COUNT);
DATA(0x004b7430)
u8 gCurWatchPlayerHighBit;
DATA(0x004ca168)
i32 gCurPlaceToVisit;
DATA(0x004b2220)
i8 gBestShipyardId;
DATA(0x004c8d1c)
i8 gMapVisitFlags[MAP_CELL_GRID_SIZE][MAP_CELL_GRID_SIZE];
DATA(0x004b2224)
i16 gHeroStrategicRVOfPos[MAP_CELL_GRID_SIZE][MAP_CELL_GRID_SIZE];
DATA(0x004ca180)
b8 gActualBoatFound;
DATA(0x004b9cbc)
u8 gCurWatchPlayerBit;
DATA(0x004b4ba8)
playerData* gCurPlayerData;
DATA(0x004b2178)
float gHeroInteractionBonus[GAME_HERO_COUNT];
DATA(0x004ca160)
b32 gBerserk;
DATA(0x004ca178)
u8 gCurPlayerHighBit;
DATA(0x004b4bb0)
i16 gLiveChanceOfPos[MAP_CELL_GRID_SIZE][MAP_CELL_GRID_SIZE];
DATA(0x004b9cb8)
i8 gBuildBoatStuffTurn[GAME_PLAYER_COUNT];
DATA(0x004b4aa8)
i32 gPlacesVisited[AI_PLACE_VISIT_COUNT][AI_PLACE_COORDINATE_COUNT];
DATA(0x004c8cc0)
b32 gTroopReload;
DATA(0x004ca170)
b8 gActualShipyardFound;

// Named AI factors, kept in .rdata ahead of the anonymous float literals.
DATA(0x0048a4ac)
static const float AI_TARGET_HUMAN_VALUE_FACTOR = 1.5f;
DATA(0x0048a4b0)
static const float AI_STRATEGIC_POSITION_SCORE_FACTOR = 1.25f;
DATA(0x0048a4b4)
static const float AI_CREATURE_SAME_RACE_FACTOR = 1.1f;
DATA(0x0048a4b8)
static const float AI_FUTURE_DEFLATION_RATE = 0.15f;
DATA(0x0048a4bc)
static const float AI_HERO_PURCHASE_SAME_RACE_FACTOR = 1.12f;
DATA(0x0048a4c0)
static const float AI_ATTENTION_IDENTITY_FLOAT = 1.0f;
DATA(0x0048a4c4)
static const float AI_ATTENTION_IDENTITY = 1.0f;

// Routes the status-line print through the AI object's debug font.
VA(0x00447900, 0x14)
void AiPrint(char* text) {
    gPhilAI->ShowDebugText(text);
}

// @dead-code
// Zero-ref: no incoming call, jump or relocated reference in retail.
VA(0x00447914, 0x3d)
void AbsAiPrint(char* text) {
    i32 savedDebugLevel;

    if (gDebugLevel == DEBUG_LEVEL_NONE)
        return;
    savedDebugLevel = gDebugLevel;
    gDebugLevel = MISC_FORCED_DEBUG_LEVEL;
    gPhilAI->ShowDebugText(text);
    gDebugLevel = savedDebugLevel;
}

// No off-map guard; indexes [x][y].
VA(0x00447951, 0x150)
void ResetHeroRVs(b32 resetAll, i32 x, i32 y) {
    i32 i;
    i32 j;

    for (i = 0; i < MAP_CELL_GRID_SIZE; i++) {
        for (j = 0; j < MAP_CELL_GRID_SIZE; j++) {
            if (resetAll) {
                if (MANHATTAN_LENGTH(x - i, y - j) < 10)
                    gHeroStrategicRVOfPos[i][j] = RV_UNSET;
            } else {
                gHeroStrategicRVOfPos[i][j] = RV_UNSET;
                gHeroEventStratRVOfPos[i][j] = RV_UNSET;
            }
        }
    }
    gHeroEventStratRVOfPos[x][y] = RV_UNSET;
    for (i = 0; i < GAME_HERO_COUNT; i++) {
        if (!resetAll
            || MANHATTAN_LENGTH(x - gGame->m_heroRecs[i].m_x, y - gGame->m_heroRecs[i].m_x) < 10)
            gHeroLiveChance[i] = RV_UNSET;
    }
}

VA(0x00447aa1, 0x1ca)
void CheckDoMain(i32, b32 doMain) {
    if (gLastFrameRateTimer + 15 < KBTickCount()
        || gTimers[ADVENTURE_FRAME_TIMER_SLOT] < KBTickCount()) {
        Process1WindowsMessage();
        PollSound();
        if (gTimers[ADVENTURE_FRAME_TIMER_SLOT] < KBTickCount()) {
            if (!doMain) {
                b32 oldShowIt = gShowIt;
                i32 oldX = gAdvManager->m_previousOriginX;
                i32 oldY = gAdvManager->m_previousOriginY;
                gDrawSavedCursor = true;
                if (gConfig.blackoutComputer == 0 && !gRemoteOn)
                    gShowIt = true;
                else
                    gShowIt = false;
                if (!gShowIt)
                    gSpecialHideCursor = true;
                if (gAdvManager->ComboDraw(
                        gAdvManager->m_previousOriginX,
                        gAdvManager->m_previousOriginY,
                        false
                    ))
                    gAdvManager->UpdateScreen(false, false);
                else
                    gAdvManager->UpdBottomView(false, true, true);
                gShowIt = oldShowIt;
                gDrawSavedCursor = false;
                gSpecialHideCursor = false;
                gAdvManager->m_previousOriginX = oldX;
                gAdvManager->m_previousOriginY = oldY;
            }
            gTimers[ADVENTURE_FRAME_TIMER_SLOT] = KBTickCount() + TIMER_DELAY;
        } else if (gMouseManager->m_drawnX != gMouseManager->m_mouseX - gMouseManager->m_hotspotX
                   || gMouseManager->m_drawnY
                          != gMouseManager->m_mouseY - gMouseManager->m_hotspotY) {
            gMouseManager->MovePointer(gMouseManager->m_mouseX, gMouseManager->m_mouseY);
        }
        gLastFrameRateTimer = KBTickCount();
    }
}

// Intentionally empty status hook.
VA(0x00447c6b, 0x5)
void ShowStatus() {}

// AI status line drawn with philAI's debug font across the bottom
// twenty screen rows; gated on the second debug level.
VA(0x00447c70, 0x7e)
void philAI::ShowDebugText(char* text) {
    if (gDebugLevel >= AI_DEBUG_LEVEL_STATUS_TEXT_MIN) {
        FillBitmapArea(gWindowManager->m_screen, 0, 460, LOGICAL_SCREEN_WIDTH, 20, 0);
        m_debugFont->DrawBoundedString(text, 0, 464, LOGICAL_SCREEN_WIDTH, 16, 1, FONT_ALIGN_LEFT);
        BlitBitmapToScreen(gWindowManager->m_screen, 0, 460, LOGICAL_SCREEN_WIDTH, 20, 0, 460);
    }
}

// The three build arrays, plus the debug-font owner.
VA(0x00447cee, 0x51)
philAI::philAI() {
    i32 i;

    m_debugFont = NULL;
    for (i = 0; i < GAME_PLAYER_COUNT; i++) {
        gBuildShipyard[i] = GAME_TOWN_NONE;
        gBuildBoat[i] = GAME_TOWN_NONE;
        gBuildBoatStuffTurn[i] = 0;
    }
}

VA(0x00447d3f, 0x88)
void philAI::DoAllHeroInteractions(void) {
    i32 i;

    for (i = 0; i < gCurPlayerData->m_townCount; i++) {
        town* townPointer = gGame->GetTown(gCurPlayerData->m_townIds[i]);
        if (townPointer->m_occupyingHeroId != TOWN_OCCUPYING_HERO_NONE)
            HeroInteractionAtTown(
                gGame->GetHero(townPointer->m_occupyingHeroId),
                townPointer,
                false,
                &gDummy
            );
    }
}

VA(0x00447dc7, 0x3ef)
void philAI::CheckBuyStuff(void) {
    b32 done = false;
    b32 bought = false;
    BHC bestBuy;
    town* dockTown;

    gGame->CheckHeroConsistency();
    if (gCurPlayerData->m_resources[RESOURCE_GOLD] < 200)
        return;
    dockTown = NULL;
    if (gBuildShipyard[gCurPlayer] >= 0)
        dockTown = &gGame->m_castleRecs[gBuildShipyard[gCurPlayer]];
    else if (gBuildBoat[gCurPlayer] >= 0)
        dockTown = &gGame->m_castleRecs[gBuildBoat[gCurPlayer]];
    if (gBuildShipyard[gCurPlayer] >= 0)
        dockTown = gGame->GetTown(gBuildShipyard[gCurPlayer]);
    else if (gBuildBoat[gCurPlayer] >= 0)
        dockTown = gGame->GetTown(gBuildBoat[gCurPlayer]);
    if (dockTown && dockTown->m_owner != gCurPlayer) {
        gBuildShipyard[gCurPlayer] = GAME_TOWN_NONE;
        gBuildBoat[gCurPlayer] = GAME_TOWN_NONE;
        dockTown = NULL;
    }
    if (gBuildShipyard[gCurPlayer] >= 0) {
        if (CanBuy(dockTown, BUILDING_SLOT_SHIPYARD)
            && CanBuild(dockTown, BUILDING_SLOT_SHIPYARD)) {
            BuildBuilding(dockTown, BUILDING_SLOT_SHIPYARD);
            gBuildShipyard[gCurPlayer] = GAME_TOWN_NONE;
        } else {
            gCurPlayerData->m_resources[RESOURCE_GOLD] -= AI_SHIPYARD_GOLD_RESERVE;
            gCurPlayerData->m_resources[RESOURCE_WOOD] -= AI_SHIPYARD_WOOD_RESERVE;
        }
    }
    if (gBuildBoat[gCurPlayer] >= 0) {
        if ((dockTown->m_buildings & H1_ENUM_BIT(BuildingSlotType, BUILDING_SLOT_SHIPYARD))
            && gCurPlayerData->m_resources[RESOURCE_GOLD] >= TOWN_BOAT_GOLD_COST
            && gCurPlayerData->m_resources[RESOURCE_WOOD] >= TOWN_BOAT_WOOD_COST) {
            if (gGame->CreateBoat(dockTown->m_x - 1, dockTown->m_y + 1) != GAME_TABLE_FREE) {
                gCurPlayerData->m_resources[RESOURCE_GOLD] -= TOWN_BOAT_GOLD_COST;
                gCurPlayerData->m_resources[RESOURCE_WOOD] -= TOWN_BOAT_WOOD_COST;
            }
            gBuildBoat[gCurPlayer] = GAME_TOWN_NONE;
        } else {
            gCurPlayerData->m_resources[RESOURCE_GOLD] -= TOWN_BOAT_GOLD_COST;
            gCurPlayerData->m_resources[RESOURCE_WOOD] -= TOWN_BOAT_WOOD_COST;
        }
    }
    DoAllHeroInteractions();
    while (!done) {
        GetBestBHC(gCurPlayer, bestBuy);
        if (bestBuy.type >= PURCHASE_FIRST && CanBuyBHC(bestBuy)) {
            switch (bestBuy.type) {
                case PURCHASE_BUILDING:
                    BuildBuilding(
                        bestBuy.townPointer,
                        H1_ENUM_DECODE(BuildingSlotType, bestBuy.what)
                    );
                    break;
                case PURCHASE_HERO:
                    BuildHero(bestBuy.townPointer, bestBuy.what);
                    break;
                case PURCHASE_CREATURE:
                    BuildCreature(bestBuy.townPointer, bestBuy.what, bestBuy.num);
                    break;
            }
            bought = true;
        } else
            done = true;
    }
    if (gBuildShipyard[gCurPlayer] >= 0) {
        gCurPlayerData->m_resources[RESOURCE_GOLD] += AI_SHIPYARD_GOLD_RESERVE;
        gCurPlayerData->m_resources[RESOURCE_WOOD] += AI_SHIPYARD_WOOD_RESERVE;
    }
    if (gBuildBoat[gCurPlayer] >= 0) {
        gCurPlayerData->m_resources[RESOURCE_GOLD] += TOWN_BOAT_GOLD_COST;
        gCurPlayerData->m_resources[RESOURCE_WOOD] += TOWN_BOAT_WOOD_COST;
    }
    DoAllHeroInteractions();
}

VA(0x004481b6, 0x194)
b32 philAI::GoodAdjacent(hero* aiHero, H1_ENUM_PARAM(MapDirection, i32) * direction) {
    i32 chance;
    H1_ENUM_LOCAL(MapDirection, i32) bestDirection;
    i32 cellX;
    i32 cellY;
    i32 eventValue;
    i32 bestEventValue;
    H1_ENUM_LOCAL(MapDirection, i32) heading;

    bestDirection = MAP_DIRECTION_NONE;
    bestEventValue = 100;
    if (MAP_TRIGGER_OBJECT(gAdvManager->GetCell(aiHero->m_x, aiHero->m_y)->m_triggerType)
        == MAP_OBJECT_STONE_LITHS)
        return false;
    for (heading = MAP_DIRECTION_FIRST; heading < MAP_DIRECTION_COUNT; heading++) {
        if (gAdvManager->ValidMoveWithEvent(aiHero, heading)) {
            cellX = aiHero->m_x + gNormalDirTable[H1_ENUM_ENCODE(MapDirection, heading)].x;
            cellY = aiHero->m_y + gNormalDirTable[H1_ENUM_ENCODE(MapDirection, heading)].y;
            if ((gAdvManager->GetCell(cellX, cellY)->m_triggerType & MAP_TRIGGER_EVENT)
                && !(gMapExtra[cellX][cellY] & MAP_EXTRA_MONSTER_ADJACENT)
                && MAP_TRIGGER_OBJECT(gAdvManager->GetCell(cellX, cellY)->m_triggerType)
                       != MAP_OBJECT_STONE_LITHS
                && MAP_TRIGGER_OBJECT(gAdvManager->GetCell(cellX, cellY)->m_triggerType)
                       != MAP_OBJECT_WHIRLPOOL) {
                eventValue = ValueOfEventAtPosition(aiHero, cellX, cellY, AI_EVENT_TARGET, &chance);
                if (chance > 80 && eventValue > bestEventValue) {
                    bestEventValue = eventValue;
                    bestDirection = heading;
                }
            }
        }
    }
    if (bestDirection != MAP_DIRECTION_NONE) {
        *direction = bestDirection;
        return true;
    }
    return false;
}

VA(0x0044834a, 0x384)
void philAI::CheckReload(hero* aiHero) {
    i32 mapY;
    mapCell* cell;
    i32 heroFightValue;
    i32 mapX;
    float friendlySupportSum;
    float enemyPressure;
    i32 armyFightValue;

    gTroopReload = false;
    gReduceFactor = 1.0f;
    friendlySupportSum = 0.0f;
    enemyPressure = 0.0f;
    heroFightValue = FightValueOfStack(&aiHero->m_army, aiHero, false);
    if (heroFightValue < AI_FIGHT_VALUE_MIN)
        heroFightValue = AI_FIGHT_VALUE_MIN;
    gSearchArray->SeedPosition(
        aiHero->m_x,
        aiHero->m_y,
        H1_ENUM_ENCODE(MapDirection, aiHero->m_direction),
        aiHero->m_mobility << 2,
        aiHero->IsEmbarked(),
        false,
        aiHero->m_remainingMobility,
        aiHero->m_heroClass,
        SEARCH_INVALID_COORDINATE,
        SEARCH_INVALID_COORDINATE,
        false,
        false
    );
    for (mapX = 0; mapX < MAP_CELL_GRID_SIZE; mapX++) {
        for (mapY = 0; mapY < MAP_CELL_GRID_SIZE; mapY++) {
            if (gSearchArray->m_cells[mapX][mapY].visited) {
                cell = gAdvManager->GetCell(mapX, mapY);
                switch (cell->m_triggerType) {
                    case MAP_EVENT_TRIGGER(MAP_OBJECT_TOWN):
                        armyFightValue = FightValueOfStack(
                            &gGame->GetTown(cell->m_objectMetadata)->m_army,
                            NULL,
                            false
                        );
                        if (gGame->m_townOwners[cell->m_objectMetadata] == aiHero->m_owner) {
                            if (armyFightValue > heroFightValue * 2)
                                friendlySupportSum +=
                                    (static_cast<float>(armyFightValue) / (heroFightValue * 2)
                                     - 1.0f)
                                    * (aiHero->m_mobility + 10)
                                    / (gSearchArray->m_cells[mapX][mapY].distance + 10);
                        } else if (armyFightValue > heroFightValue >> 1) {
                            enemyPressure +=
                                (static_cast<float>(armyFightValue) / (heroFightValue >> 1) - 1.0f)
                                * (aiHero->m_mobility + 30)
                                / (gSearchArray->m_cells[mapX][mapY].distance + 30);
                        }
                        break;
                    case MAP_EVENT_TRIGGER(MAP_OBJECT_HERO):
                        if (gGame->m_availableHeroes[cell->m_objectMetadata] != aiHero->m_owner) {
                            armyFightValue = FightValueOfStack(
                                &gGame->GetHero(cell->m_objectMetadata)->m_army,
                                NULL,
                                false
                            );
                            if (armyFightValue > heroFightValue >> 1)
                                enemyPressure +=
                                    (static_cast<float>(armyFightValue) / (heroFightValue >> 1)
                                     - 1.0f)
                                    * (aiHero->m_mobility + 30)
                                    / (gSearchArray->m_cells[mapX][mapY].distance + 30);
                        }
                }
            }
        }
    }
    if (friendlySupportSum > 1.0f && enemyPressure > 1.0f) {
        gReduceFactor = 3.0f / (2.0f + friendlySupportSum + enemyPressure);
        gTroopReload = true;
    }
}

#define enemyHero theHero // frame-slot spelling
VA(0x004486ce, 0x216)
void philAI::CheckBerserk(hero* aiHero) {
    i32 enemyStrength;
    i32 x;
    mapCell* cell;
    i32 y;
    hero* enemyHero;
    i32 strongest = -1;
    i32 ownFightValue;

    gBerserk = false;
    gBerserkFactor = 1.0f;
    ownFightValue = FightValueOfStack(&aiHero->m_army, aiHero, true);
    if (ownFightValue < AI_FIGHT_VALUE_MIN)
        ownFightValue = AI_FIGHT_VALUE_MIN;
    if (ownFightValue < AI_BERSERK_FIGHT_VALUE_MIN)
        return;
    for (x = 0; x < MAP_CELL_GRID_SIZE; x++) {
        for (y = 0; y < MAP_CELL_GRID_SIZE; y++) {
            cell = gAdvManager->GetCell(x, y);
            switch (cell->m_triggerType) {
                case MAP_EVENT_TRIGGER(MAP_OBJECT_TOWN):
                    if (gGame->m_townOwners[cell->m_objectMetadata] != aiHero->m_owner) {
                        if (gGame->m_townOwners[cell->m_objectMetadata] != GAME_PLAYER_NONE) {
                            enemyStrength = FightValueOfStack(
                                &gGame->GetTown(cell->m_objectMetadata)->m_army,
                                NULL,
                                true,
                                1,
                                cell->m_objectMetadata
                            );
                            if (enemyStrength > ownFightValue)
                                return;
                            if (enemyStrength > strongest)
                                strongest = enemyStrength;
                        }
                    }
                    break;
                case MAP_EVENT_TRIGGER(MAP_OBJECT_HERO):
                    if (gGame->m_availableHeroes[cell->m_objectMetadata] != aiHero->m_owner) {
                        enemyHero = gGame->GetHero(cell->m_objectMetadata);
                        enemyStrength = FightValueOfStack(
                            &enemyHero->m_army,
                            NULL,
                            true,
                            enemyHero->m_locationType == MAP_EVENT_TRIGGER(MAP_OBJECT_TOWN),
                            enemyHero->m_occupiedTown
                        );
                        if (enemyStrength * 2 > ownFightValue)
                            return;
                        if (enemyStrength * 2 > strongest)
                            strongest = enemyStrength * 2;
                    }
                    break;
            }
        }
    }
    if (strongest <= 0)
        return;
    gBerserkFactor = strongest * 0.75 / ownFightValue;
    gBerserk = true;
}
#undef enemyHero

// Casts Dimension Door toward the planned route's destination when that
// skips enough steps.
VA(0x004488e4, 0x18b)
b8 philAI::DoDimensionDoor(hero* aiHero) {
    i32 cellX;
    i32 pathIndex;
    i32 cellY;
    i32 skippedCount;
    i32 bestX, bestY;
    mapCell* cell;
    if (aiHero->m_remainingMobility < 4)
        return false;
    bestX = -1;
    cellX = aiHero->m_x;
    cellY = aiHero->m_y;
    for (pathIndex = gSearchArray->m_pathLength - 1; pathIndex >= 1; pathIndex--) {
        cellX += gNormalDirTable[gSearchArray->m_directions[pathIndex]].x;
        cellY += gNormalDirTable[gSearchArray->m_directions[pathIndex]].y;
        if (abs(cellX - aiHero->m_x) <= 7 && abs(cellY - aiHero->m_y) <= 7) {
            cell = gAdvManager->GetCell(cellX, cellY);
            if (!(cell->m_triggerType & MAP_TRIGGER_EVENT)
                && !(cell->m_secondaryTrigger & MAP_CELL_SECONDARY_BLOCKED)) {
                bestX = cellX;
                bestY = cellY;
                skippedCount = gSearchArray->m_pathLength - pathIndex;
            }
        }
    }
    if (bestX == -1 || skippedCount <= 4)
        return false;
    gAdvManager->TeleportTo(bestX, bestY, 0);
    if (aiHero->m_remainingMobility < SPELL_TRAVEL_MOBILITY_COST)
        aiHero->m_remainingMobility = 0;
    else
        aiHero->m_remainingMobility -= SPELL_TRAVEL_MOBILITY_COST;
    aiHero->UseSpell(SPELL_DIMENSION_DOOR);
    return true;
}

#define savedShowIt nextOldShowIt // frame-slot spelling
#define unusedValue newDummyValue // frame-slot spelling
#define unusedFlags flagsState    // frame-slot spelling
#define unusedArray tempArrayData // frame-slot spelling
VA(0x00448a6f, 0x72f)
void philAI::DoAI(i32 player) {
    i32 pathIndex;
    b32 allMoveDone;
    b8 halfShown;
    i32 stepCount;
    i32 stepQuota;
    mapCell* eventCell;
    i32 unusedValue;
    b8 nextStopAfterStep;
    b32 savedShowIt;
    H1_ENUM_LOCAL(MapDirection, i32) adjacentDirection;
    i32 stopPosX;
    i32 stopPosY;
    i16 minValue;
    i32 unusedArray[4];
    hero* movingHero;
    i32 unusedFlags;
    b32 exhaustedMobility;
    b8 nearMonster;

    halfShown = false;
    if (gGameOver)
        return;
    if (gLimitPlayer && player != gLimitPlayer)
        return;
    GetTurnAIVars(player);
    ShowStatus();
    CheckBuyStuff();
    IncrementHourGlass();
    SuspendSamples();
    SuspendMusic();
    while ((movingHero = DetermineHeroToMove(player)) != NULL) {
        gHumanTownConquered = GAME_TOWN_NONE;
        gCurPlaceToVisit = 0;
        if (gGameOver) {
            ResumeSamples();
            ResumeMusic();
            return;
        }
        CheckReload(movingHero);
        CheckBerserk(movingHero);
        gShowComputerRoute = false;
        if (gConfig.blackoutComputer == 0 && !gRemoteOn
            && (gGame->m_mapExtra[movingHero->m_x][movingHero->m_y] & gCurWatchPlayerHighBit)) {
            gShowIt = true;
            gAdvManager->SetHeroContext(movingHero->m_id, false);
        } else {
            gShowIt = false;
            gAdvManager->SetHeroContext(movingHero->m_id, false);
        }
        allMoveDone = false;
        ResetHeroRVs(false, 0, 0);
        stepQuota = movingHero->IsEmbarked() ? 15 : 5;
        minValue = movingHero->m_mobility + 42;
        stepQuota = stepQuota * (1.7 - gCurPlayerData->m_difficulty * 0.1);
        minValue =
            minValue
            * ((gCurPlayerData->m_difficulty - H1_ENUM_ENCODE(ComputerPlayerType, PLAYER_TYPE_DUMB))
                   * 0.06
               + 0.8);
        while (!allMoveDone && movingHero->m_remainingMobility >= 4) {
            if (gGameOver) {
                ResumeSamples();
                ResumeMusic();
                return;
            }
            if (movingHero->m_remainingMobility == movingHero->m_mobility
                && gCurPlayerData->m_ultimateArtifactHintChance > 15
                && gCurPlayerData->m_ultimateArtifactHintX == movingHero->m_x
                && gCurPlayerData->m_ultimateArtifactHintY == movingHero->m_y)
                gAdvManager->ProcessSearch(movingHero->m_x, movingHero->m_y);
        retarget:
            DetermineTargetPosition(
                movingHero,
                movingHero->m_destinationX,
                movingHero->m_destinationY,
                minValue
            );
            for (pathIndex = 0; pathIndex < gCurPlaceToVisit; pathIndex++) {
                if (gPlacesVisited[pathIndex][AI_PLACE_X] == movingHero->m_destinationX
                    && gPlacesVisited[pathIndex][AI_PLACE_Y] == movingHero->m_destinationY
                    && gAdvManager->GetCell(movingHero->m_destinationX, movingHero->m_destinationY)
                               ->m_triggerType
                           != MAP_EVENT_TRIGGER(MAP_OBJECT_TOWN))
                    movingHero->m_remainingMobility = 0;
            }
            if (gCurPlaceToVisit < AI_PLACE_VISIT_COUNT) {
                gPlacesVisited[gCurPlaceToVisit][AI_PLACE_X] = movingHero->m_x;
                gPlacesVisited[gCurPlaceToVisit][AI_PLACE_Y] = movingHero->m_y;
                gCurPlaceToVisit++;
            }
            gShowComputerRoute = true;
            if (movingHero->m_mobility == movingHero->m_remainingMobility) {
                halfShown = false;
                IncrementHourGlass();
            }
            if (movingHero->m_destinationX != HERO_DESTINATION_NONE
                && movingHero->m_destinationY != HERO_DESTINATION_NONE) {
                eventCell = NULL;
                gAdvManager->SetHeroContext(movingHero->m_id, false);
                gSearchArray->BuildPath(
                    movingHero->m_x,
                    movingHero->m_y,
                    movingHero->m_destinationX,
                    movingHero->m_destinationY,
                    movingHero->m_remainingMobility
                );
                if (gSearchArray->m_pathLength > 0) {
                    gAdvManager->UpdateScreen(false, false);
                    if (movingHero->HasSpell(SPELL_DIMENSION_DOOR) && DoDimensionDoor(movingHero))
                        goto retarget;
                    stepCount = 0;
                    pathIndex = gSearchArray->m_pathLength - 1;
                    exhaustedMobility = false;
                    nearMonster = false;
                    while (pathIndex >= 0 && stepCount < stepQuota) {
                        nextStopAfterStep = stepCount + 1 == stepQuota || pathIndex == 0;
                        if (pathIndex > 0 && GoodAdjacent(movingHero, &adjacentDirection)) {
                            gSearchArray->m_directions[pathIndex] =
                                H1_ENUM_ENCODE(MapDirection, adjacentDirection);
                            nextStopAfterStep = true;
                        }
                        if (gAdvManager->GetMoveShowIt(
                                H1_ENUM_DECODE(MapDirection, gSearchArray->m_directions[pathIndex])
                            )) {
                            savedShowIt = gShowIt;
                            gShowIt = true;
                            gMouseManager->ReallyHidePointer();
                            gShowIt = savedShowIt;
                        }
                        eventCell = gAdvManager->MoveHero(
                            H1_ENUM_DECODE(MapDirection, gSearchArray->m_directions[pathIndex]),
                            nextStopAfterStep,
                            &stopPosX,
                            &stopPosY,
                            &exhaustedMobility,
                            true,
                            &nearMonster
                        );
                        stepCount++;
                        if (eventCell || exhaustedMobility || nearMonster)
                            break;
                        pathIndex--;
                    }
                    if (movingHero->m_owner == GAME_PLAYER_NONE)
                        goto nextHero;
                    if (movingHero->m_remainingMobility <= movingHero->m_mobility >> 1
                        && !halfShown) {
                        halfShown = true;
                        IncrementHourGlass();
                    }
                    if (pathIndex < 0 && gCurPlayerData->m_ultimateArtifactHintChance > 15
                        && gCurPlayerData->m_ultimateArtifactHintX == movingHero->m_x
                        && gCurPlayerData->m_ultimateArtifactHintY == movingHero->m_y) {
                        if (movingHero->m_remainingMobility == movingHero->m_mobility)
                            gAdvManager->ProcessSearch(
                                ADVMGR_SEARCH_VIEW_CENTER,
                                ADVMGR_SEARCH_VIEW_CENTER
                            );
                        else
                            movingHero->m_remainingMobility = 0;
                    }
                    if (pathIndex < 0
                        && (((movingHero->m_x != movingHero->m_destinationX
                              || movingHero->m_y != movingHero->m_destinationY)
                             && !eventCell)
                            || movingHero->m_remainingMobility < 4
                            || (exhaustedMobility && !eventCell)))
                        allMoveDone = true;
                    savedShowIt = gShowIt;
                    gShowIt = true;
                    gMouseManager->ReallyShowPointer();
                    gShowIt = savedShowIt;
                    gAdvManager->UpdateRadar(true, false);
                } else {
                    allMoveDone = true;
                }
                if (eventCell) {
                    gAdvManager->DoAIEvent(eventCell, movingHero, stopPosX, stopPosY);
                    if (gCurPlayerData->m_currentHero == HERO_ID_NONE)
                        goto nextHero;
                    ResetHeroRVs(true, movingHero->m_destinationX, movingHero->m_destinationY);
                }
            } else {
                allMoveDone = true;
            }
        }
        movingHero->m_remainingMobility = 0;
        gAdvManager->DeactivateCurrHero();
    nextHero:
        if (movingHero->m_locationType == MAP_EVENT_TRIGGER(MAP_OBJECT_TOWN))
            CheckBuyStuff();
    }
    ResumeSamples();
    ResumeMusic();
}
#undef savedShowIt
#undef unusedValue
#undef unusedFlags
#undef unusedArray

// Refreshes every player's game attention value.
VA(0x0044919e, 0x3f)
void philAI::GetGameAIVars(void) {
    i32 i;

    for (i = 0; i < gGame->m_playerCount; i++)
        GetGameAttentionValue(i);
}

#define heroX xPosVal          // frame-slot spelling
#define heroY theYPos          // frame-slot spelling
#define enemyPlayer basePlayer // frame-slot spelling
#define playerIndex indexNum   // frame-slot spelling
#define townPointer townPtr    // frame-slot spelling
#define fightValue fightVal    // frame-slot spelling
VA(0x004491dd, 0x5eb)
void philAI::GetTurnAIVars(i32 player) {
    i32 fightTotalSum;
    playerData* enemyPlayer;
    i32 heroY;
    i32 heroX;
    i32 playerIndex;
    // Counts towns, then the regular artifacts, players and heroes.
    H1_ENUM_SHARED(ArtifactType, i32) i;
    float fightValue;
    i32 y;
    i32 turnDistance;
    i32 heroIndex;
    i32 unusedFightValue;
    i32 artifactValueSum;
    hero* heroPointer;
    i32 nearX;
    town* townPointer;

    gCurTurn = GAME_DAY_NUMBER(*gGame);
    GetTurnAttentionValue(player);
    TurnCostResource(player);
    gCurHourGlassPhase = AI_HOUR_GLASS_PHASE_FIRST;
    gSandAnim = 0;
    gCurPlayerData->m_aiData.m_obeliskValue = TurnValueOfObelisk(player);
    gCurPlayerData->m_aiData.m_unexploredValue = MeanRVOfUnexploredTerritory(player);
    gHeroBuiltThisTurn = false;
    if (gCurTurn - gBuildBoatStuffTurn[player] > 8) {
        gBuildShipyard[player] = GAME_TOWN_NONE;
        gBuildBoat[player] = GAME_TOWN_NONE;
    }
    unusedFightValue = 0;
    fightValue = 0.0f;
    fightTotalSum = 0;
    for (i = 0; i < gCurPlayerData->m_heroCount; i++) {
        heroPointer = gGame->GetHero(gCurPlayerData->m_heroIds[i]);
        fightValue = FightValueOfStack(&heroPointer->m_army, heroPointer, false);
        fightTotalSum = fightTotalSum + fightValue;
        heroPointer->m_aiFightValue = fightValue * 4e-05 + 0.4;
    }
    for (i = 0; i < gCurPlayerData->m_townCount; i++) {
        townPointer = gGame->GetTown(gCurPlayerData->m_townIds[i]);
        fightValue = FightValueOfStack(&townPointer->m_army, NULL, false);
        fightTotalSum = fightTotalSum + fightValue;
    }
    gCurPlayerData->m_aiData.m_upgradeValueWeight =
        static_cast<float>(
            gCurPlayerData->m_resources[RESOURCE_GOLD]
            + gCurPlayerData->m_aiData.m_income[RESOURCE_GOLD]
        ) / (fightTotalSum + 1000)
        + gCurPlayerData->m_aiData.m_attentionWeights.upgradeBase;
    artifactValueSum = 0;
    for (i = ARTIFACT_REGULAR_FIRST; i < ARTIFACT_REGULAR_END; i++)
        artifactValueSum += gArtifactBaseRV[i];
    for (i = 0; i < gGame->m_playerCount; i++)
        gGame->m_players[i].m_aiData.m_artifactPoolShare =
            1.0 / (gGame->m_playerCount + gGame->m_deadPlayerCount);
    gCurPlayerData->m_aiData.m_artifactValue = artifactValueSum / 33.0;
    memset(gTurnValueOfMine, 7, sizeof(gTurnValueOfMine));
    for (playerIndex = 0; playerIndex < gGame->m_playerCount; playerIndex++) {
        if (playerIndex != gCurPlayer) {
            enemyPlayer = &gGame->m_players[playerIndex];
            for (heroIndex = 0; heroIndex < enemyPlayer->m_heroCount; heroIndex++) {
                heroX = gGame->GetHero(enemyPlayer->m_heroIds[heroIndex])->m_x;
                heroY = gGame->GetHero(enemyPlayer->m_heroIds[heroIndex])->m_y;
                for (nearX = heroX - 10; nearX <= heroX + 10; nearX++) {
                    for (y = heroY - 10; y <= heroY + 10; y++) {
                        if (MAP_CELL_IN_BOUNDS(nearX, y)) {
                            turnDistance = abs(MANHATTAN_LENGTH(nearX - heroX, y - heroY) - 4) >> 2;
                            if (turnDistance < gTurnValueOfMine[nearX][y])
                                gTurnValueOfMine[nearX][y] = turnDistance;
                        }
                    }
                }
            }
        }
    }
    for (i = 0; i < GAME_HERO_COUNT; i++)
        gHeroInteractionBonus[i] = 1.0f;
    if (H1_ENUM_DECODE(ComputerPlayerType, gCurPlayerData->m_difficulty) == PLAYER_TYPE_DUMB) {
        gAttackHumanBonus = 0.6f;
        gAttackComputerBonus = 1.3f;
    } else if (H1_ENUM_DECODE(ComputerPlayerType, gCurPlayerData->m_difficulty)
               == PLAYER_TYPE_AVERAGE) {
        gAttackHumanBonus = 1.0f;
        gAttackComputerBonus = 1.0f;
    } else {
        gAttackHumanBonus = gCurPlayerData->m_difficulty * 0.07 + 1.0;
        gAttackComputerBonus = 1.1 - gCurPlayerData->m_difficulty * 0.12;
    }
    if (gIAmGreatest)
        gAttackComputerBonus = 0.1f;
    gMaxHeroesForThisPlayer = 3;
    if (gGame->m_playerCount - gGame->m_deadPlayerCount == GAME_PLAYERS_TWO)
        gMaxHeroesForThisPlayer++;
    if (gGame->m_playerCount - gGame->m_deadPlayerCount == GAME_PLAYERS_THREE)
        gMaxHeroesForThisPlayer++;
    if (gCurPlayerData->m_townCount >= 5)
        gMaxHeroesForThisPlayer++;
    if (gCurPlayerData->m_townCount >= 10)
        gMaxHeroesForThisPlayer++;
}
#undef heroX
#undef heroY
#undef enemyPlayer
#undef playerIndex
#undef townPointer
#undef fightValue

VA(0x004497c8, 0x54c)
void philAI::GetBestBHC(i32 player, BHC& best) {
    float bhcValue = 1.0f;
    float topVal = -99.0f;
    i32 totalStrength = 0;
    i32 totalWeights = 0;
    i32 idealStrength[GAME_TOWN_COUNT];
    i32 townStrength[GAME_TOWN_COUNT];
    BHC choice;
    i32 townNo;
    town* townPointer;
    i32 avgStrength;

    for (townNo = 0; townNo < gCurPlayerData->m_townCount; townNo++) {
        townPointer = &gGame->m_castleRecs[gCurPlayerData->m_townIds[townNo]];
        townStrength[townNo] = FightValueOfStack(&townPointer->m_army, NULL, false) + 400;
        totalStrength += townStrength[townNo];
        if (townPointer->m_buildings & H1_ENUM_BIT(BuildingSlotType, BUILDING_SLOT_CASTLE))
            totalWeights += 10;
        else
            totalWeights += 7;
    }
    if (totalWeights < 1)
        totalWeights = 1;
    avgStrength = totalStrength / totalWeights;
    for (townNo = 0; townNo < gCurPlayerData->m_townCount; townNo++) {
        townPointer = &gGame->m_castleRecs[gCurPlayerData->m_townIds[townNo]];
        idealStrength[townNo] =
            avgStrength
                * ((townPointer->m_buildings & H1_ENUM_BIT(BuildingSlotType, BUILDING_SLOT_CASTLE))
                       ? 10
                       : 7)
            + 400;
    }
    for (townNo = 0; townNo < gCurPlayerData->m_townCount; townNo++) {
        townPointer = &gGame->m_castleRecs[gCurPlayerData->m_townIds[townNo]];
        if (gCurTurn > 3 && (!gRemoteOn || gRemoteReady) && townPointer->m_turnsOwned < 3)
            continue;
        CheckDoMain(0, false);
        GetBestBuilding(townPointer, choice, bhcValue);
        bhcValue = bhcValue * ((100 - Random(0, 10)) / 100.0);
        if (bhcValue > topVal) {
            topVal = bhcValue;
            best = choice;
        }
        CheckDoMain(0, false);
        GetBestCreature(townPointer, choice, bhcValue);
        bhcValue = bhcValue
                   * (static_cast<float>(idealStrength[townNo])
                          / (static_cast<float>(townStrength[townNo])) / 3.0f
                      + 0.66);
        bhcValue = bhcValue * ((100 - Random(0, 10)) / 100.0);
        if (bhcValue > topVal) {
            topVal = bhcValue;
            best = choice;
        }
        CheckDoMain(0, false);
        if (gCurPlayerData->m_heroCount < gMaxHeroesForThisPlayer
            && (townPointer->m_buildings & H1_ENUM_BIT(BuildingSlotType, BUILDING_SLOT_CASTLE))) {
            GetBestHero(townPointer, choice, bhcValue);
            bhcValue = bhcValue * ((100 - Random(0, 10)) / 100.0);
            if (!gHeroBuiltThisTurn && gCurTurn > 5 && bhcValue > 0.0f) {
                if ((gCurPlayerData->m_aiData.m_income[RESOURCE_GOLD] >= 1250
                     && gCurPlayerData->m_heroCount < gMaxHeroesForThisPlayer - 2)
                    || gCurPlayerData->m_heroCount <= 1)
                    bhcValue += 500.0f;
                else if (gCurPlayerData->m_aiData.m_income[RESOURCE_GOLD] >= 1500
                         && gCurPlayerData->m_heroCount < gMaxHeroesForThisPlayer - 1)
                    bhcValue = bhcValue * 1.3;
            } else if (gCurPlayerData->m_heroCount == 0) {
                bhcValue += 500.0f;
            }
            if (bhcValue > topVal) {
                topVal = bhcValue;
                best = choice;
            }
        }
    }
    if (topVal < 0.02)
        best.type = PURCHASE_NONE;
}

// The current player's hero with the most remaining mobility, counted with a
// byte index.
VA(0x00449d14, 0xea)
hero* philAI::DetermineHeroToMove(i32 player) {
    i32 bestHero;
    i32 bestMobility;
    i32 mobility;
    i8 i;

    bestMobility = 0;
    bestHero = -1;
    if (gCurPlayerData->HasMobileHero()) {
        for (i = 0; i < gCurPlayerData->m_heroCount; i++) {
            mobility = gGame->m_heroRecs[gGame->m_players[player].m_heroIds[i]].m_remainingMobility;
            if (mobility > bestMobility) {
                bestMobility = mobility;
                bestHero = i;
            }
        }
    }
    if (bestHero >= 0)
        return &gGame->m_heroRecs[gGame->m_players[player].m_heroIds[bestHero]];
    gGame->m_players[player].m_currentHero = HERO_ID_NONE;
    return NULL;
}

#define cell thisCellRec // frame-slot spelling
VA(0x00449dfe, 0x89d)
void philAI::DetermineTargetPosition(hero* aiHero, i8& targetX, i8& targetY, i16 mobility) {
    town* portTown;
    H1_ENUM_LOCAL(TerrainType, i32) ground;
    i32 gridStep;
    i32 posValue;
    i32 maxRV;
    b32 good;
    i32 rowCounter;
    mapCell* cell;
    i32 heroIndex;
    i16 bestY;
    i16 searchX;
    i32 colCounter;
    i16 bestX;
    i16 searchY;

    bestX = -1;
    bestY = -1;
    maxRV = -999999;
    gBestShipyardId = GAME_TOWN_NONE;
    gPossibleShipyardFound = false;
    gActualShipyardFound = false;
    gActualBoatFound = false;
    gridStep = aiHero->m_mobility / 6;
    cell = gAdvManager->GetCell(aiHero->m_x, aiHero->m_y);
    ground = CELL_TERRAIN(cell);
    if (ground == TERRAIN_SNOW || ground == TERRAIN_SWAMP) {
        gridStep--;
        mobility = mobility * 1.25;
    }
    if (ground == TERRAIN_DESERT) {
        gridStep -= 2;
        mobility = mobility * 1.5;
    }
    if (gridStep < 3)
        gridStep = 3;
    gSearchArray->SeedPosition(
        aiHero->m_x,
        aiHero->m_y,
        H1_ENUM_ENCODE(MapDirection, aiHero->m_direction),
        mobility * 3,
        aiHero->IsEmbarked(),
        true,
        aiHero->m_remainingMobility,
        aiHero->m_heroClass,
        SEARCH_INVALID_COORDINATE,
        SEARCH_INVALID_COORDINATE,
        false,
        false
    );
    gSearchArray->m_cells[aiHero->m_x][aiHero->m_y].visited = 0;
    colCounter = -1;
    for (searchX = 0; searchX < MAP_CELL_GRID_SIZE; searchX++) {
        rowCounter = -1;
        colCounter++;
        if (colCounter >= gridStep)
            colCounter = 0;
        for (searchY = 0; searchY < MAP_CELL_GRID_SIZE; searchY++) {
            rowCounter++;
            if (rowCounter >= gridStep)
                rowCounter = 0;
            if (gSearchArray->m_cells[searchX][searchY].visited) {
                cell = gAdvManager->GetCell(searchX, searchY);
                if (gSearchArray->m_cells[searchX][searchY].distance > mobility) {
                    if (gSearchArray->m_cells[searchX][searchY].distance > mobility * 2)
                        good = false;
                    else
                        good = cell->m_triggerType == MAP_EVENT_TRIGGER(MAP_OBJECT_TOWN)
                               || cell->m_triggerType == MAP_EVENT_TRIGGER(MAP_OBJECT_HERO)
                               || (cell->m_triggerType == MAP_EVENT_TRIGGER(MAP_OBJECT_SHIP)
                                   && !aiHero->IsEmbarked());
                } else {
                    good =
                        (cell->m_triggerType & MAP_TRIGGER_EVENT)
                        || (cell->m_triggerType == MAP_OBJECT_TRIGGER(MAP_OBJECT_COAST)
                            && aiHero->IsEmbarked())
                        || (searchX % gridStep == 0 && searchY % gridStep == 0
                            && ((aiHero->IsEmbarked() && CELL_TERRAIN(cell) == TERRAIN_WATER)
                                || (!aiHero->IsEmbarked() && CELL_TERRAIN(cell) != TERRAIN_WATER)))
                        || (searchX == gCurPlayerData->m_ultimateArtifactHintX
                            && searchY == gCurPlayerData->m_ultimateArtifactHintY);
                }
                if (good) {
                    for (heroIndex = 0; heroIndex < gCurPlayerData->m_heroCount; heroIndex++) {
                        if (cell->m_triggerType != MAP_EVENT_TRIGGER(MAP_OBJECT_TOWN)
                            && cell->m_triggerType != MAP_EVENT_TRIGGER(MAP_OBJECT_HERO)
                            && gCurPlayerData->m_heroIds[heroIndex] != aiHero->m_id
                            && gGame->m_heroRecs[gCurPlayerData->m_heroIds[heroIndex]]
                                       .m_destinationX
                                   == searchX
                            && gGame->m_heroRecs[gCurPlayerData->m_heroIds[heroIndex]]
                                       .m_destinationY
                                   == searchY) {
                            posValue = -2000;
                            goto scored;
                        }
                    }
                    CheckDoMain(0, false);
                    posValue = RVOfPosition(
                        aiHero,
                        searchX,
                        searchY,
                        gSearchArray->m_cells[searchX][searchY].rvFlag1,
                        gSearchArray->m_cells[searchX][searchY].valueX,
                        gSearchArray->m_cells[searchX][searchY].valueY,
                        gSearchArray->m_cells[searchX][searchY].rvFlag2,
                        gSearchArray->m_cells[searchX][searchY].previousX,
                        gSearchArray->m_cells[searchX][searchY].previousY,
                        AI_EVENT_TARGET
                    );
                    posValue = posValue * (Random(1, 50) + 75);
                    posValue /= 100;
                } else {
                    posValue = -100;
                }
                if (searchX == targetX && searchY == targetY) {
                    posValue = posValue * AI_TARGET_HUMAN_VALUE_FACTOR;
                    if (MANHATTAN_LENGTH(searchX - aiHero->m_x, searchY - aiHero->m_y) > 3)
                        posValue++;
                }
            scored:
                if (posValue > maxRV) {
                    bestX = searchX;
                    bestY = searchY;
                    maxRV = posValue;
                } else if (posValue == maxRV && posValue == 0) {
                    if (MANHATTAN_LENGTH(searchX - aiHero->m_x, searchY - aiHero->m_y)
                        > MANHATTAN_LENGTH(bestX - aiHero->m_x, bestY - aiHero->m_y)) {
                        bestX = searchX;
                        bestY = searchY;
                    }
                }
            }
        }
    }
    if (maxRV < 75 && (gPossibleShipyardFound || gActualShipyardFound) && !gActualBoatFound
        && gCurTurn > 3) {
        if ((gActualShipyardFound || gBuildShipyard[gCurPlayer] < 0
             || gBuildShipyard[gCurPlayer] == gBestShipyardId)
            && gCurPlayerData->m_resources[RESOURCE_WOOD]
                       + gCurPlayerData->m_aiData.m_income[RESOURCE_WOOD] * 6
                   >= (!gActualShipyardFound ? AI_SHIPYARD_WOOD_RESERVE : 0)
                          + TOWN_BOAT_WOOD_COST) {
            if (!gActualShipyardFound)
                gBuildShipyard[gCurPlayer] = gBestShipyardId;
            gBuildBoat[gCurPlayer] = gBestShipyardId;
            gBuildBoatStuffTurn[gCurPlayer] = gCurTurn;
            portTown = gGame->GetTown(gBestShipyardId);
            maxRV = 123;
            bestX = portTown->m_x;
            bestY = portTown->m_y;
            if (aiHero->m_x == bestX && aiHero->m_y == bestY)
                aiHero->m_remainingMobility = 0;
        }
        CheckBuyStuff();
    }
    targetX = bestX;
    targetY = bestY;
}
#undef cell

#define defStr curDefStr     // frame-slot spelling
#define defenderPower defP   // frame-slot spelling
#define artifactSlot slotNum // frame-slot spelling
VA(0x0044a69b, 0x55f)
void philAI::ProbableOutcomeOfBattle(
    armyGroup* attacker,
    hero* attackerHero,
    armyGroup* defender,
    hero* defenderHero,
    armyGroup* townArmy,
    i8 useTown,
    i8 townId,
    i32 enemyPlayer,
    float& winChance,
    i32& attackerLoss,
    i32& defenderLoss,
    i32& expectedAttackerLoss,
    i32& expectedDefenderLoss,
    i32& outcomeValue
) {
    float attFight;
    i32 artifactSlot;
    float defenderFight;
    i32 defArts;
    i32 notUsed;
    i32 experience;
    float attStr;
    float defenderPower;
    float defStr;
    float rawFight[AI_BATTLE_SIDE_COUNT];
    float attackerPower;
    float powCurve;
    float lossWeight;
    i32 attArts;

    attArts = 0;
    defArts = 0;
    attFight = FightValueOfStack(attacker, attackerHero, true);
    defenderFight = FightValueOfStack(defender, defenderHero, true, useTown, townId);
    if (townArmy)
        defenderFight += FightValueOfStack(townArmy, NULL, true);
    rawFight[AI_BATTLE_ATTACKER] = FightValueOfStack(attacker, attackerHero, false);
    rawFight[AI_BATTLE_DEFENDER] = FightValueOfStack(defender, defenderHero, false);
    if (townArmy)
        rawFight[AI_BATTLE_DEFENDER] += FightValueOfStack(townArmy, NULL, false);
    if (useTown)
        defenderFight = defenderFight * 1.11;
    defStr = defenderFight;
    if (enemyPlayer == GAME_PLAYER_NONE) {
        attStr = attFight * (gCurPlayerData->m_difficulty * 0.15 + 0.7);
    } else {
        attStr = attFight;
        if (gHumanPlayer[enemyPlayer]) {
            defStr = defStr * 1.14;
            if (H1_ENUM_DECODE(ComputerPlayerType, gCurPlayerData->m_difficulty)
                == PLAYER_TYPE_DUMB)
                attStr = attStr * 1.5;
        }
    }
    if (attStr < 1.0f)
        attStr = 1.0f;
    if (defStr < 1.0f)
        defStr = 1.0f;
    powCurve = 2.75f;
    if (attStr > 1000000.0f || defStr > 1000000.0f)
        powCurve = 2.0f;
    attackerPower = pow(attStr, powCurve);
    defenderPower = pow(defStr, powCurve);
    winChance = attackerPower / (attackerPower + defenderPower);
    if (winChance < 0.08)
        winChance = 0.0f;
    else if (winChance < 0.12)
        winChance = winChance - 0.07;
    else if (winChance < 0.2)
        winChance = winChance - 0.05;
    else if (winChance < 0.3)
        winChance = winChance - 0.04;
    else if (winChance < 0.4)
        winChance = winChance - 0.02;
    attackerLoss = (1.0 - winChance) * rawFight[AI_BATTLE_ATTACKER];
    defenderLoss = rawFight[AI_BATTLE_DEFENDER] * winChance;
    expectedAttackerLoss =
        attackerLoss * winChance + (1.0f - winChance) * rawFight[AI_BATTLE_ATTACKER];
    expectedDefenderLoss =
        defenderLoss * (1.0f - winChance) + rawFight[AI_BATTLE_DEFENDER] * winChance;
    lossWeight = 1.33 - gCurPlayerData->m_aiData.m_attentionWeights.upgradeBase;
    outcomeValue = -expectedAttackerLoss * lossWeight * lossWeight;
    if (enemyPlayer >= 0) {
        lossWeight = gCurPlayerData->m_aiData.m_attentionWeights.upgradeBase + 0.66;
        if (gHumanPlayer[enemyPlayer])
            outcomeValue =
                outcomeValue + expectedDefenderLoss * gAttackHumanBonus * lossWeight * lossWeight;
        else
            outcomeValue = outcomeValue
                           + expectedDefenderLoss * gAttackComputerBonus * lossWeight * lossWeight;
    }
    outcomeValue = outcomeValue * gCurPlayerData->m_aiData.m_upgradeValueWeight;
    if (attackerHero) {
        for (artifactSlot = 0; artifactSlot < HERO_ARTIFACT_SLOT_COUNT; artifactSlot++) {
            if (ARTIFACT_HAS_BASE_VALUE(attackerHero->m_artifacts[artifactSlot]))
                attArts += gArtifactBaseRV[attackerHero->m_artifacts[artifactSlot]];
        }
        outcomeValue = outcomeValue - (attArts + 1400) * (1.0f - winChance);
        experience = gGame->ExperienceValueOfStack(defender, defenderHero);
        outcomeValue = outcomeValue + experience * 0.8 * winChance * attackerHero->m_aiFightValue;
    }
    if (defenderHero) {
        for (artifactSlot = 0; artifactSlot < HERO_ARTIFACT_SLOT_COUNT; artifactSlot++) {
            if (ARTIFACT_HAS_BASE_VALUE(defenderHero->m_artifacts[artifactSlot]))
                defArts += gArtifactBaseRV[defenderHero->m_artifacts[artifactSlot]];
        }
        outcomeValue =
            outcomeValue
            + (defArts + 1250)
                  * (gHumanPlayer[defenderHero->m_owner] ? gAttackHumanBonus : gAttackComputerBonus)
                  * winChance;
    }
}
#undef defStr
#undef defenderPower
#undef artifactSlot

// @dead-code
// Zero-ref: no incoming call, jump or relocated reference in retail.
VA(0x0044abfa, 0x13)
float philAI::GetOddsOfWinning(i32) {
    return 1.0f;
}

// The base value, scaled per slot by attention weights and dwelling counts,
// the enemy threat and the purchase deflator.
VA(0x0044ac0d, 0x507)
void philAI::ValueOfBuyingBuilding(
    town* townPointer,
    H1_ENUM_PARAM(BuildingSlotType, i32) building,
    i32& resourceValue,
    float& benefitCost
) {
    H1_ENUM_LOCAL(TownType, i16) factionId;
    i32 castleTier;
    H1_ENUM_ARRAY(i32, buildingCost, ResourceType, RESOURCE_COUNT);
    H1_ENUM_LOCAL(CreatureType, i32) dwellingCreature;
    i32 lastDwellingIndex;
    i32 level;
    float dangerRating;
    b32 creaturePresent;
    float townLossRisk;
    i32 enemyStrength;
    i32 filledStackCount;
    i32 i;
    i32 weightedAttack;
    float enemyAttackChance;
    i32 dwellingsOwned;
    float score;
    i32 attackWeeks;

    factionId = townPointer->m_type;
    dwellingsOwned = 0;
    lastDwellingIndex = -1;
    for (i = 0; i < BUILDING_SLOT_DWELLING_COUNT; i++) {
        if (townPointer->m_buildings
            & H1_ENUM_BIT(BuildingSlotType, i + BUILDING_SLOT_DWELLING_FIRST)) {
            dwellingsOwned++;
            lastDwellingIndex = i;
        }
    }
    filledStackCount = 0;
    for (i = 0; i < ARMY_GROUP_SLOT_COUNT; i++)
        if (townPointer->m_army.m_creatureCounts[i] > 0)
            filledStackCount++;
    score = GetBuildingBaseResourceValue(
        factionId,
        building,
        building == BUILDING_SLOT_MAGE_GUILD ? townPointer->m_buildState : 0
    );
    if (building == BUILDING_SLOT_MAGE_GUILD && townPointer->m_buildState > 0)
        score -= GetBuildingBaseResourceValue(factionId, building, townPointer->m_buildState - 1);
    switch (building) {
        case BUILDING_SLOT_CASTLE:
            score =
                score * (gCurPlayerData->m_aiData.m_attentionWeights.buildingValue * 2.0f + 0.33);
            castleTier = dwellingsOwned;
            score = score * (1.6 - castleTier * 0.2);
            break;
        case BUILDING_SLOT_MAGE_GUILD:
            score =
                score * (gCurPlayerData->m_aiData.m_attentionWeights.buildingValue * 2.0f + 0.33);
            score = score
                    * (1.33
                       - gCurPlayerData->BuildingsOwned(
                             factionId,
                             BUILDING_SLOT_MAGE_GUILD,
                             MAGE_GUILD_STATE_LEVEL_1
                         ) * 0.33);
            break;
        case BUILDING_SLOT_THIEVES_GUILD:
            break;
        case BUILDING_SLOT_SHIPYARD:
            score = 0;
            break;
        case BUILDING_SLOT_WELL:
            score = score * (gCurPlayerData->m_aiData.m_attentionWeights.buildingValue + 0.66);
            score = score * (gCurPlayerData->m_aiData.m_attentionWeights.upgradeBase * 2.0f + 0.33);
            score = score * (dwellingsOwned * 0.33 + 0.66);
            break;
        case BUILDING_SLOT_TAVERN:
            score = FightValueOfStack(&townPointer->m_army, NULL, false) / 3000.0f * score;
            break;
        case BUILDING_SLOT_DWELLING_1:
        case BUILDING_SLOT_DWELLING_2:
        case BUILDING_SLOT_DWELLING_3:
        case BUILDING_SLOT_DWELLING_4:
        case BUILDING_SLOT_DWELLING_5:
        case BUILDING_SLOT_DWELLING_6:
            if (filledStackCount == ARMY_GROUP_SLOT_COUNT) {
                creaturePresent = false;
                for (i = 0; i < ARMY_GROUP_SLOT_COUNT; i++)
                    if (townPointer->m_army.m_creatureTypes[i]
                        == gDwellingType[townPointer->m_type]
                                        [building - BUILDING_SLOT_DWELLING_FIRST])
                        creaturePresent = true;
                if (!creaturePresent)
                    break;
            }
            score = score * (gCurPlayerData->m_aiData.m_attentionWeights.buildingValue + 0.66);
            score = score * (gCurPlayerData->m_aiData.m_attentionWeights.upgradeBase * 2.0f + 0.33);
            score = score * (1.0 - gCurPlayerData->BuildingsOwned(factionId, building, 0) * 0.05);
            if (building - BUILDING_SLOT_DWELLING_FIRST < lastDwellingIndex)
                score = score * (1.66 - dwellingsOwned * 0.33);
            if (townPointer->m_buildings & H1_ENUM_BIT(BuildingSlotType, BUILDING_SLOT_WELL))
                score = score * 1.1;
            for (level = 0; level < BUILDING_SLOT_DWELLING_COUNT; level++) {
                dwellingCreature = gDwellingType[townPointer->m_type][level];
                if ((townPointer->m_buildings
                     & H1_ENUM_BIT(BuildingSlotType, level + BUILDING_SLOT_DWELLING_FIRST))
                    && townPointer->m_dwellingAvailable[level] > 0
                    && gMonsterDatabase[dwellingCreature].iconIndex * 1.2
                           > gMonsterDatabase[gDwellingType[townPointer->m_type]
                                                           [building
                                                            - BUILDING_SLOT_DWELLING_FIRST]]
                                 .iconIndex) {
                    score = 0;
                    break;
                }
            }
            break;
    }
    LikelihoodOfEnemyAttacking(
        townPointer,
        NULL,
        enemyAttackChance,
        townLossRisk,
        enemyStrength,
        weightedAttack,
        attackWeeks,
        dangerRating
    );
    score = score * (1.0 - dangerRating * 3.0);
    if (score < 0.0f)
        score = 0;
    GetBuildingCost(
        factionId,
        building,
        buildingCost,
        building == BUILDING_SLOT_MAGE_GUILD ? townPointer->m_buildState : 0
    );
    score = score * FutureDeflator(buildingCost);
    resourceValue = score;
    benefitCost = score / RVConversion(buildingCost);
}

VA(0x0044b114, 0x101)
void philAI::GetBestBuilding(town* townPointer, BHC& purchase, float& benefitCost) {
    float buildingBenefitCost;
    H1_ENUM_LOCAL(BuildingSlotType, i32) bestBuilding;
    float bestBenefitCost;
    H1_ENUM_LOCAL(BuildingSlotType, i32) curBuilding;
    i32 resourceValue;
    float grade;
    float maxScore;

    bestBenefitCost = -99.0f;
    maxScore = -99.0f;
    bestBuilding = BUILDING_SLOT_NONE;
    for (curBuilding = BUILDING_SLOT_MAGE_GUILD; curBuilding <= BUILDING_SLOT_DWELLING_LAST;
         curBuilding++) {
        if (!(townPointer->m_buildings & H1_ENUM_BIT(BuildingSlotType, curBuilding))
            || (curBuilding == BUILDING_SLOT_MAGE_GUILD
                && townPointer->m_buildState < TOWN_MAGE_GUILD_COST_LEVEL_LAST)) {
            if (CanBuild(townPointer, curBuilding)) {
                ValueOfBuyingBuilding(townPointer, curBuilding, resourceValue, buildingBenefitCost);
                grade = (Random(1, 5) + 95) * buildingBenefitCost / 100.0f;
                if (grade > maxScore) {
                    bestBuilding = curBuilding;
                    bestBenefitCost = buildingBenefitCost;
                    maxScore = grade;
                }
            }
        }
    }
    purchase.townPointer = townPointer;
    purchase.type = PURCHASE_BUILDING;
    purchase.what = H1_ENUM_ENCODE(BuildingSlotType, bestBuilding);
    benefitCost = bestBenefitCost;
}

#define slot n // frame-slot spelling
VA(0x0044b215, 0x268)
void philAI::ValueOfBuyingCreature(
    town* townPointer,
    H1_ENUM_PARAM(CreatureType, i32) creature,
    i32& resourceValue,
    i32 purchaseCount,
    float& benefitCost
) {
    hero* occupant;
    i32 weightedAttack;
    H1_ENUM_ARRAY(i32, buyCost, ResourceType, RESOURCE_COUNT);
    float peril;
    // Counts breath-attack stacks.
    i32 breathStacks;
    i32 creatureRV;
    i32 costRV;
    float attackOdds;
    float lossOdds;
    float dangerFactor;
    i32 attackStrength;
    i32 slot;
    i32 weekCount;
    i32 townSlot;

    breathStacks = 0;
    GetMonsterCost(creature, buyCost);
    costRV = purchaseCount * RVConversion(buyCost);
    creatureRV = purchaseCount * gMonsterDatabase[creature].fightValue
                 * gCurPlayerData->m_aiData.m_upgradeValueWeight;
    if (townPointer->m_occupyingHeroId != TOWN_OCCUPYING_HERO_NONE) {
        occupant = gGame->GetHero(townPointer->m_occupyingHeroId);
        creatureRV = creatureRV * 1.1;
        if (CREATURE_FACTION(creature) == occupant->m_heroClass)
            creatureRV = creatureRV * AI_CREATURE_SAME_RACE_FACTOR;
        if ((gMonsterDatabase[creature].stats.attributes & MONSTER_FLAGS_BREATH_ATTACK)) {
            for (slot = 0; slot < ARMY_GROUP_SLOT_COUNT; slot++) {
                if (occupant->m_army.m_creatureTypes[slot] != CREATURE_NONE
                    && (gMonsterDatabase[occupant->m_army.m_creatureTypes[slot]].stats.attributes
                        & MONSTER_FLAGS_BREATH_ATTACK))
                    breathStacks++;
            }
            creatureRV = creatureRV * (1.18 - breathStacks * 0.06);
        }
        creatureRV =
            creatureRV
            * (gGame->m_players[townPointer->m_owner].m_aiData.m_attentionWeights.upgradeBase
               + 0.66);
    }
    if ((gMonsterDatabase[creature].stats.attributes & MONSTER_FLAGS_BREATH_ATTACK)) {
        for (townSlot = 0; townSlot < ARMY_GROUP_SLOT_COUNT; townSlot++) {
            if (townPointer->m_army.m_creatureTypes[townSlot] != CREATURE_NONE
                && (gMonsterDatabase[townPointer->m_army.m_creatureTypes[townSlot]].stats.attributes
                    & MONSTER_FLAGS_BREATH_ATTACK))
                breathStacks++;
        }
        creatureRV = creatureRV * (1.18 - breathStacks * 0.06);
    }
    LikelihoodOfEnemyAttacking(
        townPointer,
        NULL,
        attackOdds,
        lossOdds,
        attackStrength,
        weightedAttack,
        weekCount,
        peril
    );
    dangerFactor = peril + 0.96;
    creatureRV = creatureRV * (dangerFactor * dangerFactor * dangerFactor);
    creatureRV = creatureRV * FutureDeflator(buyCost);
    resourceValue = creatureRV;
    benefitCost = static_cast<float>(resourceValue) / (static_cast<float>(costRV));
}
#undef slot

VA(0x0044b47d, 0x188)
void philAI::GetBestCreature(town* townPointer, BHC& best, float& bestValue) {
    i32 bestNumber;
    i32 resourceValue;
    float rawBenefitCost;
    b32 canAdd;
    float bestRaw;
    i32 bestDwelling;
    i32 armyIndex;
    H1_ENUM_LOCAL(CreatureType, i32) creature;
    float randomScore;
    i32 count;
    float maxScore;
    i32 dwelling;

    bestDwelling = -1;
    bestNumber = 0;
    bestRaw = -99.0f;
    maxScore = -99.0f;
    for (dwelling = 0; dwelling < BUILDING_SLOT_DWELLING_COUNT; dwelling++) {
        creature = gDwellingType[townPointer->m_type][dwelling];
        if ((townPointer->m_buildings
             & H1_ENUM_BIT(BuildingSlotType, dwelling + BUILDING_SLOT_DWELLING_FIRST))
            && townPointer->m_dwellingAvailable[dwelling] > 0) {
            canAdd = false;
            for (armyIndex = 0; armyIndex < ARMY_GROUP_SLOT_COUNT; armyIndex++) {
                if (townPointer->m_army.m_creatureTypes[armyIndex] == CREATURE_NONE
                    || townPointer->m_army.m_creatureTypes[armyIndex] == creature)
                    canAdd = true;
            }
            if (canAdd) {
                count = CreaturesToBuy(townPointer, dwelling);
                if (count > 0) {
                    ValueOfBuyingCreature(
                        townPointer,
                        creature,
                        resourceValue,
                        count,
                        rawBenefitCost
                    );
                    randomScore = (Random(1, 10) + 90) * rawBenefitCost / 100.0;
                    if (randomScore > maxScore) {
                        bestDwelling = dwelling;
                        bestRaw = rawBenefitCost;
                        maxScore = randomScore;
                        bestNumber = count;
                    }
                }
            }
        }
    }
    best.townPointer = townPointer;
    best.type = PURCHASE_CREATURE;
    best.what = bestDwelling;
    best.num = bestNumber;
    bestValue = bestRaw;
}

// The town overload indexes the six dwelling stocks and faction table.
VA(0x0044b605, 0x3f)
i32 philAI::CreaturesToBuy(town* townPointer, i32 level) {
    i32 availableCount = townPointer->m_dwellingAvailable[level];
    return CreaturesToBuy(gDwellingType[townPointer->m_type][level], availableCount);
}

VA(0x0044b644, 0x47)
i32 philAI::CreaturesToBuy(H1_ENUM_PARAM(CreatureType, i32) creatureType, i32 availableCount) {
    i32 purchaseCount = MaxBuyableCreatures(creatureType);
    if (purchaseCount > 1)
        purchaseCount >>= 1;
    if (purchaseCount > availableCount)
        purchaseCount = availableCount;
    if (purchaseCount > 1)
        return purchaseCount;
    else
        return 0;
}

// The last resource's affordable count wins.
VA(0x0044b68b, 0x82)
i32 philAI::MaxBuyableCreatures(H1_ENUM_PARAM(CreatureType, i32) creatureType) {
    i32 affordable;
    H1_ENUM_LOCAL(ResourceType, i32) i;
    H1_ENUM_ARRAY(i32, cost, ResourceType, RESOURCE_COUNT);

    GetMonsterCost(creatureType, cost);
    for (i = RESOURCE_FIRST; i < RESOURCE_COUNT; i++) {
        if (cost[i] == 0)
            affordable = 9999;
        else if (gCurPlayerData->m_resources[i] > 0)
            affordable = gCurPlayerData->m_resources[i] / cost[i];
        else
            affordable = 0;
    }
    return affordable;
}

VA(0x0044b70d, 0x184)
void philAI::ValueOfBuyingHero(
    town* townPointer,
    hero* heroPointer,
    i32& resourceValue,
    float& benefitCost
) {
    i32 heroRV;
    i32 i;
    i32 unusedChance;
    i32 costRV;
    H1_ENUM_ARRAY(i32, heroCost, ResourceType, RESOURCE_COUNT);

    heroCost[RESOURCE_WOOD] = 0;
    heroCost[RESOURCE_MERCURY] = 0;
    heroCost[RESOURCE_ORE] = 0;
    heroCost[RESOURCE_SULFUR] = 0;
    heroCost[RESOURCE_CRYSTAL] = 0;
    heroCost[RESOURCE_GEMS] = 0;
    heroCost[RESOURCE_GOLD] = 2500;
    costRV = RVConversion(heroCost);
    heroRV = heroPointer->m_experience + 2000;
    for (i = 0; i < HERO_ARTIFACT_SLOT_COUNT; i++) {
        if (ARTIFACT_HAS_BASE_VALUE(heroPointer->m_artifacts[i]))
            heroRV += gArtifactBaseRV[heroPointer->m_artifacts[i]];
    }
    heroRV += heroPointer->m_experience / 2;
    heroRV = heroRV
             * (gCurPlayerData->m_aiData.m_attentionWeights.heroValue + 1.0
                - gCurPlayerData->m_aiData.m_attentionWeights.upgradeBase);
    if (gTownHeroClass[H1_ENUM_ENCODE(TownType, townPointer->m_type)] == heroPointer->m_heroClass)
        heroRV = heroRV * AI_HERO_PURCHASE_SAME_RACE_FACTOR;
    heroRV += StrategicValueOfPosition(
        heroPointer,
        heroPointer->m_x,
        heroPointer->m_y,
        false,
        &unusedChance
    );
    heroRV -= 200;
    heroRV = heroRV * FutureDeflator(heroCost);
    benefitCost = static_cast<float>(heroRV) / costRV;
    resourceValue = heroRV;
}

// ValueOfEventAtPosition module state.
#define gExpectedAttackerLoss gEstOurForce // spelling fixes .bss order
DATA(0x004bb124)
i32 gExpectedAttackerLoss;
#define gExpectedDefenderLoss gEstGuardPower // spelling fixes .bss order
DATA(0x004bb128)
i32 gExpectedDefenderLoss;
#define gOutcome gSimOutcome // spelling fixes .bss order
DATA(0x004bb12c)
i32 gOutcome;
#define gArtifactPickupValue gPrizeUnguardedValue // spelling fixes .bss order
DATA(0x004bb130)
i32 gArtifactPickupValue;

#define resourceValue activeCostVal // frame-slot spelling
VA(0x0044b891, 0x12c)
void philAI::GetBestHero(town* townPointer, BHC& best, float& bestValue) {
    i32 availableIndex;
    float rawBenefitCost;
    i32 bestHero;
    hero* candidate;
    float randomScore;
    float maxScore;
    float bestRaw;
    i32 resourceValue;

    bestHero = -1;
    bestRaw = -99.0f;
    maxScore = -99.0f;
    for (availableIndex = 0; availableIndex < HERO_AVAILABLE_SLOT_COUNT; availableIndex++) {
        candidate = &gGame->m_heroRecs[gCurPlayerData->m_availableHeroIds[availableIndex]];
        ValueOfBuyingHero(townPointer, candidate, resourceValue, rawBenefitCost);
        randomScore = rawBenefitCost * (Random(1, 10) + 90.0) / 100.0;
        if (randomScore > maxScore) {
            bestHero = availableIndex;
            bestRaw = rawBenefitCost;
            maxScore = randomScore;
        }
    }
    best.townPointer = townPointer;
    best.type = PURCHASE_HERO;
    best.what = bestHero;
    bestValue = bestRaw;
    if (gGame->m_map[townPointer->m_x][townPointer->m_y].m_triggerType
        == MAP_EVENT_TRIGGER(MAP_OBJECT_HERO))
        bestValue -= 200.0f;
}
#undef resourceValue

VA(0x0044b9bd, 0x54)
void philAI::LikelihoodOfEnemyAttacking(
    town* townPointer,
    hero* heroPointer,
    float& attackChance,
    float& lossRisk,
    i32& attackStrength,
    i32& weightedAttack,
    i32& attackWeeks,
    float& dangerRating
) {
    attackChance = 0.15f;
    lossRisk = 0.6f;
    attackStrength = 3000;
    weightedAttack = attackStrength * attackChance;
    attackWeeks = 6;
    dangerRating = attackChance * lossRisk;
}

VA(0x0044ba11, 0xf)
i32 philAI::MeanRVOfUnexploredTerritory(i32 player) {
    return 0;
}

// Randomized game weights tempered by the number of players.
VA(0x0044ba20, 0x14d)
void philAI::GetGameAttentionValue(i32 player) {
    playerAttentionWeights* attention = &gGame->m_players[player].m_aiData.m_attentionWeights;
    attention->gameWeightA = static_cast<float>(Random(0, 100) / 500.0) + 0.23;
    attention->gameWeightB = static_cast<float>(Random(0, 100) / 500.0) + 0.23;
    attention->gameWeightB *= (AI_ATTENTION_IDENTITY_FLOAT + 3.0) / 4.0;
    attention->gameWeightB *= (5.0 - AI_ATTENTION_IDENTITY) / 4.0;
    attention->gameWeightA *= (AI_ATTENTION_IDENTITY + 3.0) / 4.0;
    attention->gameWeightB = attention->gameWeightB * ((3.0 - gGame->m_playerCount) * 0.15 + 1.0);
    attention->gameWeightA = attention->gameWeightA * ((3.0 - gGame->m_playerCount) * 0.07 + 1.0);
    attention->gameRemainder = ((1.0f - attention->gameWeightB) - attention->gameWeightA);
}

// Reset the game weights and scale the hero weight down as the game ages.
VA(0x0044bb6d, 0xc6)
void philAI::GetTurnAttentionValue(i32 player) {
    playerAttentionWeights* attentionWeights =
        &gGame->m_players[player].m_aiData.m_attentionWeights;
    attentionWeights->gameWeightA = 0.4f;
    attentionWeights->gameWeightB = 0.3f;
    attentionWeights->gameRemainder = 0.3f;
    attentionWeights->buildingValue = attentionWeights->gameWeightA;
    attentionWeights->heroValue = attentionWeights->gameWeightB;
    attentionWeights->upgradeBase = attentionWeights->gameRemainder;
    float scale;
    if (gCurTurn < 5)
        scale = 1.6f;
    else if (gCurTurn < 10)
        scale = 1.4f;
    else if (gCurTurn < 20)
        scale = 1.2f;
    else if (gCurTurn < 30)
        scale = 1.0f;
    else
        scale = 0.8f;
    attentionWeights->heroValue = attentionWeights->heroValue * scale;
}

VA(0x0044bc33, 0x71)
i32 philAI::RVConversion(i32* const resources) {
    return resources[H1_ENUM_ENCODE(ResourceType, RESOURCE_GOLD)]
               * gAITurnCostResource[RESOURCE_GOLD]
           + resources[H1_ENUM_ENCODE(ResourceType, RESOURCE_WOOD)]
                 * gAITurnCostResource[RESOURCE_WOOD]
           + resources[H1_ENUM_ENCODE(ResourceType, RESOURCE_ORE)]
                 * gAITurnCostResource[RESOURCE_ORE]
           + resources[H1_ENUM_ENCODE(ResourceType, RESOURCE_CRYSTAL)]
                 * gAITurnCostResource[RESOURCE_CRYSTAL]
           + resources[H1_ENUM_ENCODE(ResourceType, RESOURCE_SULFUR)]
                 * gAITurnCostResource[RESOURCE_SULFUR]
           + resources[H1_ENUM_ENCODE(ResourceType, RESOURCE_MERCURY)]
                 * gAITurnCostResource[RESOURCE_MERCURY]
           + resources[H1_ENUM_ENCODE(ResourceType, RESOURCE_GEMS)]
                 * gAITurnCostResource[RESOURCE_GEMS];
}

// The slowest shortfall in turns of income, 99 when a short resource has no
// income.
VA(0x0044bca4, 0xc7)
float philAI::TurnsToBuy(i32* const resources) {
    float maxT = 0;
    H1_ENUM_LOCAL(ResourceType, i32) resourceIndex;
    float turnCount;
    for (resourceIndex = RESOURCE_FIRST; resourceIndex < RESOURCE_COUNT; resourceIndex++) {
        if (gCurPlayerData->m_resources[resourceIndex]
            < resources[H1_ENUM_ENCODE(ResourceType, resourceIndex)]) {
            if (gCurPlayerData->m_aiData.m_income[resourceIndex] > 0)
                turnCount = (resources[H1_ENUM_ENCODE(ResourceType, resourceIndex)]
                             - gCurPlayerData->m_resources[resourceIndex])
                                / gCurPlayerData->m_aiData.m_income[resourceIndex]
                            + 1;
            else
                turnCount = 99.0f;
            maxT = __max(turnCount, maxT);
        }
    }
    return maxT;
}

VA(0x0044bd6b, 0x493)
i32 philAI::RVOfPosition(
    hero* aiHero,
    i16 x,
    i16 y,
    i8 hasEvent,
    i16 eventX,
    i16 eventY,
    i8 hasStrategicEvent,
    i16 strategicX,
    i16 strategicY,
    i32 eventMode
) {
    i32 targetOdds;
    H1_ENUM_LOCAL(MapObjectType, i32) triggerObjectType;
    i32 followWorth;
    i32 targetEventValue;
    i32 totalValue;
    i32 positionStrategicValue;
    float journeyTurns;
    i32 triggerType;
    i32 destinationSafety;
    i32 followChance;
    i32 guardEventChance;
    i32 primaryEventChance;
    i32 originSurvival;
    i32 monsterX;
    i32 startStrategicValue;
    i32 monsterY;
    followWorth = 0;
    destinationSafety = AI_CHANCE_CERTAIN;
    followChance = AI_CHANCE_CERTAIN;
    triggerType = gAdvManager->GetCell(x, y)->m_triggerType;
    triggerObjectType = MAP_TRIGGER_OBJECT(triggerType);
    targetOdds = AI_CHANCE_CERTAIN;
    primaryEventChance = AI_CHANCE_CERTAIN;
    guardEventChance = AI_CHANCE_CERTAIN;
    startStrategicValue =
        StrategicValueOfPosition(aiHero, aiHero->m_x, aiHero->m_y, false, &originSurvival);
    positionStrategicValue = StrategicValueOfPosition(aiHero, x, y, false, &destinationSafety);
    if (triggerObjectType == MAP_OBJECT_SHIP && positionStrategicValue < 0)
        positionStrategicValue = 0;
    totalValue = 0;
    if (hasEvent)
        totalValue +=
            ValueOfEventAtPosition(aiHero, eventX, eventY, AI_EVENT_IMMEDIATE, &primaryEventChance);
    if (hasStrategicEvent) {
        followWorth = StrategicValueOfPosition(aiHero, strategicX, strategicY, true, &followChance);
        if (followWorth < 0)
            totalValue += followWorth;
    }
    if (gAdvManager->FindAdjacentMonster(
            x,
            y,
            &monsterX,
            &monsterY,
            SEARCH_INVALID_COORDINATE,
            SEARCH_INVALID_COORDINATE
        )) {
        switch (triggerObjectType) {
            case MAP_OBJECT_SIGNPOST:
            case MAP_OBJECT_SKELETON:
            case MAP_OBJECT_TREASURE_CHEST:
            case MAP_OBJECT_CAMPFIRE:
            case MAP_OBJECT_FOUNTAIN:
            case MAP_OBJECT_ANCIENT_LAMP:
            case MAP_OBJECT_MONSTER:
            case MAP_OBJECT_OBELISK:
            case MAP_OBJECT_RESOURCE:
            case MAP_OBJECT_STATUE:
            case MAP_OBJECT_WELL:
            case MAP_OBJECT_ARTIFACT:
                break;
            default:
                targetEventValue = ValueOfEventAtPosition(
                    aiHero,
                    monsterX,
                    monsterY,
                    AI_EVENT_IMMEDIATE,
                    &guardEventChance
                );
                if (targetEventValue < 0)
                    totalValue += targetEventValue;
                if (primaryEventChance == AI_CHANCE_CERTAIN)
                    primaryEventChance = guardEventChance;
                else
                    primaryEventChance = primaryEventChance * guardEventChance / 100;
                break;
        }
    }
    if ((triggerType & MAP_TRIGGER_EVENT)
        || (x == gCurPlayerData->m_ultimateArtifactHintX
            && y == gCurPlayerData->m_ultimateArtifactHintY))
        targetEventValue = ValueOfEventAtPosition(aiHero, x, y, eventMode, &targetOdds);
    else
        targetEventValue = 0;
    if (targetOdds < AI_CHANCE_CERTAIN)
        positionStrategicValue = positionStrategicValue * targetOdds / 100;
    if (destinationSafety < AI_CHANCE_CERTAIN) {
        targetEventValue = targetEventValue * destinationSafety / 100;
        positionStrategicValue = positionStrategicValue * destinationSafety / 100;
    }
    if (followChance < AI_CHANCE_CERTAIN) {
        targetEventValue = targetEventValue * followChance / 100;
        positionStrategicValue = positionStrategicValue * followChance / 100;
    }
    if (primaryEventChance < AI_CHANCE_CERTAIN) {
        if (totalValue > 0)
            totalValue =
                (totalValue + targetEventValue + positionStrategicValue) * primaryEventChance / 100;
        else
            totalValue += (targetEventValue + positionStrategicValue) * primaryEventChance / 100;
    } else {
        totalValue += targetEventValue;
    }
    journeyTurns = static_cast<float>(gSearchArray->m_cells[x][y].distance) / aiHero->m_mobility;
    if (aiHero->IsEmbarked())
        journeyTurns = journeyTurns * 0.5 + 0.5;
    else if (journeyTurns > 5.0f)
        journeyTurns *= 3.0f;
    else if (journeyTurns > 4.0f)
        journeyTurns = journeyTurns * 2.5;
    else if (journeyTurns > 3.0f)
        journeyTurns = journeyTurns * 2.0;
    else if (journeyTurns > 2.0f)
        journeyTurns = journeyTurns * 1.7;
    else if (journeyTurns > 1.5)
        journeyTurns = journeyTurns * 1.4;
    else if (journeyTurns > 1.0f)
        journeyTurns = journeyTurns * 1.2;
    totalValue = totalValue / (journeyTurns + 0.2);
    positionStrategicValue = positionStrategicValue * 2 / (1.0f + journeyTurns);
    if (primaryEventChance == AI_CHANCE_CERTAIN)
        totalValue += positionStrategicValue;
    if (aiHero->IsEmbarked() && triggerType == MAP_OBJECT_TRIGGER(MAP_OBJECT_COAST))
        totalValue += 40;
    return totalValue;
}

// StrategicValueOfPosition's shared search, constructed by its dynamic
// initializer between RVOfPosition and its first user.
DATA(0x004bb160)
searchArray gStrategicSearchArray;
RVA_DYNINIT(0x0004c208, 0xf, gStrategicSearchArray)
RVA_DYNINIT(0x0004c1fe, 0xa, gStrategicSearchArray)

#define unusedValue nextExtra2 // frame-slot spelling
VA(0x0044c217, 0x83a)
i32 philAI::StrategicValueOfPosition(
    hero* aiHero,
    i16 targetX,
    i16 targetY,
    b8 immediate,
    i32* liveChance
) {
    DATA(0x004ca198)
    static b8 gSVSearchArrayInUse = false;
    i32 gap;
    searchArray* activeSearchArray;
    i32 wasInBoat;
    i32 unusedValue;
    mapCell* cell;
    i32 searchY;
    i32 seedLimit;
    i32 reach;
    i32 searchX;
    i32 heroNum;
    i32 danger;
    H1_ENUM_LOCAL(TerrainType, i32) targetTerrain;
    i32 strategicWorth;
    searchArray* ownSearch;
    H1_ENUM_LOCAL(TerrainType, i32) terrain;

    if (!immediate && gHeroStrategicRVOfPos[targetX][targetY] != RV_UNSET) {
        *liveChance = gLiveChanceOfPos[targetX][targetY];
        return gHeroStrategicRVOfPos[targetX][targetY];
    }
    strategicWorth = 0;
    ownSearch = NULL;
    *liveChance = AI_CHANCE_CERTAIN;
    if (gSVSearchArrayInUse) {
        ownSearch = new searchArray;
        if (!ownSearch)
            MemError();
        activeSearchArray = ownSearch;
    } else {
        gSVSearchArrayInUse = true;
        activeSearchArray = &gStrategicSearchArray;
    }
    wasInBoat = aiHero->IsEmbarked();
    if (wasInBoat
        && gAdvManager->GetCell(targetX, targetY)->m_triggerType
               == MAP_OBJECT_TRIGGER(MAP_OBJECT_COAST))
        wasInBoat = 0;
    if (immediate) {
        seedLimit = 60;
    } else {
        seedLimit = 36;
    }
    activeSearchArray->SeedPosition(
        targetX,
        targetY,
        H1_ENUM_ENCODE(MapDirection, MAP_DIRECTION_EAST),
        seedLimit,
        wasInBoat,
        false,
        SEARCH_UNLIMITED_COST,
        aiHero->m_heroClass,
        SEARCH_INVALID_COORDINATE,
        SEARCH_INVALID_COORDINATE,
        false,
        false
    );
    activeSearchArray->m_cells[targetX][targetY].visited = 0;
    for (searchX = 0; searchX < MAP_CELL_GRID_SIZE; searchX++) {
        for (searchY = 0; searchY < MAP_CELL_GRID_SIZE; searchY++) {
            if (activeSearchArray->m_cells[searchX][searchY].visited) {
                cell = gAdvManager->GetCell(searchX, searchY);
                if ((!immediate && (cell->m_triggerType & MAP_TRIGGER_EVENT))
                    || (immediate && cell->m_triggerType == MAP_EVENT_TRIGGER(MAP_OBJECT_HERO))) {
                    CheckDoMain(0, false);
                    strategicWorth +=
                        ValueOfEventAtPosition(
                            aiHero,
                            searchX,
                            searchY,
                            AI_EVENT_STRATEGIC,
                            &gDummy
                        )
                        / (activeSearchArray->m_cells[searchX][searchY].distance + 2.0);
                }
                if (cell->m_triggerType == MAP_EVENT_TRIGGER(MAP_OBJECT_HERO)) {
                    if (gHeroLiveChance[cell->m_objectMetadata] == RV_UNSET)
                        ValueOfEventAtPosition(
                            aiHero,
                            searchX,
                            searchY,
                            AI_EVENT_STRATEGIC,
                            &gDummy
                        );
                    if (gHeroLiveChance[cell->m_objectMetadata] != RV_UNSET
                        && gHeroLiveChance[cell->m_objectMetadata] < AI_CHANCE_CERTAIN) {
                        reach = gGame->GetHero(cell->m_objectMetadata)->m_mobility;
                        if (gHumanPlayer[gGame->m_availableHeroes[cell->m_objectMetadata]]) {
                            if (activeSearchArray->m_cells[searchX][searchY].distance <= reach) {
                                if (activeSearchArray->m_cells[searchX][searchY].distance <= 14)
                                    danger = 100 - gHeroLiveChance[cell->m_objectMetadata];
                                else
                                    danger =
                                        (100 - gHeroLiveChance[cell->m_objectMetadata])
                                        * (reach
                                           - activeSearchArray->m_cells[searchX][searchY].distance
                                           + 10)
                                        / reach;
                            } else {
                                danger = (100 - gHeroLiveChance[cell->m_objectMetadata]) * 0.2;
                            }
                        } else {
                            danger = (100 - gHeroLiveChance[cell->m_objectMetadata])
                                     * (reach + 20
                                        - activeSearchArray->m_cells[searchX][searchY].distance)
                                     / (reach + 20);
                        }
                        *liveChance = *liveChance * (100 - danger) / 100;
                    }
                }
                if (activeSearchArray->m_cells[searchX][searchY].distance < 32
                    && gAdvManager->GetCell(searchX, searchY)->m_triggerType
                           == MAP_EVENT_TRIGGER(MAP_OBJECT_HERO)
                    && gAdvManager->GetCell(searchX, searchY)->m_objectMetadata != aiHero->m_id
                    && gGame->m_availableHeroes[gAdvManager->GetCell(searchX, searchY)
                                                    ->m_objectMetadata]
                           == aiHero->m_owner)
                    strategicWorth -=
                        (32 - activeSearchArray->m_cells[searchX][searchY].distance) * 1250 >> 5;
            }
        }
    }
    targetTerrain = CELL_TERRAIN(gAdvManager->GetCell(targetX, targetY));
    for (heroNum = 0; heroNum < gCurPlayerData->m_heroCount; heroNum++) {
        if (gCurPlayerData->m_heroIds[heroNum] != aiHero->m_id) {
            gap = MANHATTAN_LENGTH(
                gGame->m_heroRecs[gCurPlayerData->m_heroIds[heroNum]].m_destinationX - targetX,
                gGame->m_heroRecs[gCurPlayerData->m_heroIds[heroNum]].m_destinationY - targetY
            );
            if (gap < 9) {
                terrain = CELL_TERRAIN(gAdvManager->GetCell(
                    gGame->m_heroRecs[gCurPlayerData->m_heroIds[heroNum]].m_destinationX,
                    gGame->m_heroRecs[gCurPlayerData->m_heroIds[heroNum]].m_destinationY
                ));
                if (!((targetTerrain == TERRAIN_WATER && terrain > TERRAIN_WATER_LAST)
                      || (targetTerrain > TERRAIN_WATER_LAST && terrain == TERRAIN_WATER)))
                    strategicWorth -= (9 - gap) * 1250 / 9;
            }
        }
    }
    if (ownSearch)
        delete ownSearch;
    else
        gSVSearchArrayInUse = false;
    strategicWorth = strategicWorth * AI_STRATEGIC_POSITION_SCORE_FACTOR;
    if (strategicWorth > 32000)
        strategicWorth = 32000;
    if (!immediate) {
        gHeroStrategicRVOfPos[targetX][targetY] = strategicWorth;
        gLiveChanceOfPos[targetX][targetY] = *liveChance;
    }
    return strategicWorth;
}
#undef unusedValue

// ValueOfEventAtPosition module state.
#define gArtifactGuardedValue gPrizeGuardedWorth // spelling fixes .bss order
DATA(0x004bb134)
i32 gArtifactGuardedValue;

// Built structures' base values plus a fixed gold-turn allowance.
VA(0x0044ca51, 0xb9)
i32 philAI::ValueOfTown(town* townPointer) {
    i32 sum = 0;
    H1_ENUM_LOCAL(BuildingSlotType, i32) building;
    for (building = BUILDING_SLOT_MAGE_GUILD; building < BUILDING_SLOT_COUNT; building++) {
        if (townPointer->m_buildings & H1_ENUM_BIT(BuildingSlotType, building))
            sum += GetBuildingBaseResourceValue(
                townPointer->m_type,
                building,
                __max(townPointer->m_buildState, 0)
            );
    }
    sum = sum + 250.0f * gAITurnCostResource[RESOURCE_GOLD] * 5.0f * 1.5;
    sum += 750;
    return sum;
}

// Each resource's turn cost scales its base value against the player's
// relative stock-plus-income share.
VA(0x0044cb0a, 0x10b)
void philAI::TurnCostResource(i32 player) {
    playerAIData* playerAI;
    H1_ENUM_ARRAY(float, ratio, ResourceType, RESOURCE_COUNT);
    float avg;
    H1_ENUM_LOCAL(ResourceType, i32) resourceIndex;
    i32 totalRV;
    H1_ENUM_ARRAY(i32, value, ResourceType, RESOURCE_COUNT);
    playerAI = &gGame->m_players[player].m_aiData;
    totalRV = 0;
    for (resourceIndex = RESOURCE_FIRST; resourceIndex < RESOURCE_COUNT; resourceIndex++) {
        value[resourceIndex] = gResourceBaseValue[resourceIndex]
                               * ((playerAI->m_income[resourceIndex] * 5) * 0.7
                                  + gGame->m_players[player].m_resources[resourceIndex]);
        totalRV += value[resourceIndex];
    }
    avg = (totalRV / H1_ENUM_ENCODE(ResourceType, RESOURCE_COUNT));
    for (resourceIndex = RESOURCE_FIRST; resourceIndex < RESOURCE_COUNT; resourceIndex++) {
        ratio[resourceIndex] = value[resourceIndex] / avg;
        gAITurnCostResource[resourceIndex] =
            (gResourceBaseValue[resourceIndex] / (ratio[resourceIndex] / 2.0f + 0.5));
    }
}

#define aiData ai // frame-slot spelling
VA(0x0044cc15, 0xfb)
float philAI::TurnValueOfObelisk(i32 player) {
    playerAIData* aiData;
    i32 each;
    aiData = &gGame->m_players[player].m_aiData;
    each = gArtifactBaseRV[gGame->m_ultimateArtifactId] / 110;
    if (gGame->m_ultimateArtifactId == ARTIFACT_NONE)
        return 0.0f;
    aiData->m_obeliskValue = each * 48 / gGame->m_obeliskCount;
    aiData->m_obeliskValue =
        aiData->m_obeliskValue
        * (1.5 - abs(32 - gGame->m_players[player].CountPuzzlePiecesRemoved()) / 48.0f);
    aiData->m_obeliskValue = aiData->m_obeliskValue * (aiData->m_attentionWeights.heroValue + 0.66);
    return aiData->m_obeliskValue;
}
#undef aiData

VA(0x0044cd10, 0x47)
float philAI::FutureDeflator(i32* const resources) {
    float turns = TurnsToBuy(resources);
    float value = 1.0f - turns * AI_FUTURE_DEFLATION_RATE;
    if (value < 0.0)
        value = 0;
    return value;
}

VA(0x0044cd57, 0x638)
i32 philAI::FightValueOfStack(
    armyGroup* group,
    hero* heroPointer,
    b32 useAdjustedFightValue,
    i8 useTown,
    i8 townId
) {
    town* castle;
    float spellMultiplier;
    i32 spellScore;
    i32 heroLuck;
    i32 keepArrows;
    i32 armyValue;
    i32 castleValue;
    i32 bestScore;
    float countMod;
    // Counts army slots, then the castle's building slots, then spell slots.
    H1_ENUM_SHARED(BuildingSlotType, i32) slot;
    i32 combatStatSum;
    i32 morale;
    i32 magicTotal;
    i32 stackWorth;

    armyValue = 0;
    magicTotal = 0;
    castleValue = 0;
    for (slot = 0; slot < ARMY_GROUP_SLOT_COUNT; slot++) {
        if (group->m_creatureTypes[slot] != CREATURE_NONE) {
            stackWorth = group->m_creatureCounts[slot]
                         * gMonsterDatabase[group->m_creatureTypes[slot]].fightValue;
            if (useAdjustedFightValue) {
                if (group->m_creatureCounts[slot] > 180)
                    countMod = 1.7f;
                else if (group->m_creatureCounts[slot] > 140)
                    countMod = 1.3f;
                else if (group->m_creatureCounts[slot] > 100)
                    countMod = 1.1f;
                else if (group->m_creatureCounts[slot] > 75)
                    countMod = 0.95f;
                else if (group->m_creatureCounts[slot] > 50)
                    countMod = 0.81f;
                else if (group->m_creatureCounts[slot] > 35)
                    countMod = 0.57f;
                else if (group->m_creatureCounts[slot] > 23)
                    countMod = 0.37f;
                else if (group->m_creatureCounts[slot] > 16)
                    countMod = 0.25f;
                else if (group->m_creatureCounts[slot] > 11)
                    countMod = 0.13f;
                else if (group->m_creatureCounts[slot] > 8)
                    countMod = 0.06f;
                else if (group->m_creatureCounts[slot] > 5)
                    countMod = 0.0f;
                else if (group->m_creatureCounts[slot] > 3)
                    countMod = -0.05f;
                else if (group->m_creatureCounts[slot] > 2)
                    countMod = -0.1f;
                else
                    countMod = -0.14f;
                if ((gMonsterDatabase[group->m_creatureTypes[slot]].stats.attributes
                     & MONSTER_FLAGS_SHOOTER)
                    || group->m_creatureTypes[slot] == CREATURE_SPRITE
                    || group->m_creatureTypes[slot] == CREATURE_ROGUE)
                    countMod = countMod * 0.7;
                else if (group->m_creatureTypes[slot] == CREATURE_GRIFFIN)
                    countMod = countMod * 1.2;
                stackWorth = stackWorth * (1.0f + countMod);
            }
            armyValue += stackWorth;
        }
    }
    if (useTown) {
        keepArrows = COMBAT_AI_CASTLE_BASE_ARCHERS;
        castle = gGame->GetTown(townId);
        for (slot = BUILDING_SLOT_DWELLING_FIRST; slot <= BUILDING_SLOT_DWELLING_LAST; slot++)
            if (castle->m_buildings & H1_ENUM_BIT(BuildingSlotType, slot))
                keepArrows += COMBAT_AI_CASTLE_ARCHERS_PER_DWELLING;
        for (slot = BUILDING_SLOT_MAGE_GUILD; slot <= BUILDING_SLOT_GENERIC_LAST; slot++)
            if (castle->m_buildings & H1_ENUM_BIT(BuildingSlotType, slot))
                keepArrows++;
        castleValue = keepArrows * 120;
    }
    if (useAdjustedFightValue && heroPointer) {
        combatStatSum = heroPointer->m_primaryStats[HERO_PRIMARY_ATTACK]
                        + heroPointer->m_primaryStats[HERO_PRIMARY_DEFENSE] + STAT_CURVE_OFFSET;
        if (combatStatSum < 0)
            combatStatSum = 0;
        if (combatStatSum > STAT_CURVE_LAST)
            combatStatSum = STAT_CURVE_LAST;
        armyValue = armyValue * gStatPower[combatStatSum];
        castleValue = castleValue * gStatPower[combatStatSum];
        morale = heroPointer->m_army.GetMorale(heroPointer, NULL);
        if (morale > 0)
            armyValue = armyValue * (morale + 48) / 48;
        else if (morale < 0)
            armyValue = armyValue * (morale + 24) / 24;
        heroLuck = gGame->GetLuck(heroPointer, NULL);
        if (heroLuck)
            armyValue = armyValue * (heroLuck + 16) / 16;
        if (heroPointer->m_primaryStats[HERO_PRIMARY_SPELL_POWER] == HERO_SPELL_POWER_ONE)
            spellMultiplier = 0.25f;
        else if (heroPointer->m_primaryStats[HERO_PRIMARY_SPELL_POWER] == HERO_SPELL_POWER_TWO)
            spellMultiplier = 0.5f;
        else
            spellMultiplier = 1.0f;
        bestScore = -1;
        for (slot = 0; slot < HERO_SPELL_SLOT_COUNT; slot++) {
            if (heroPointer->m_spells[slot] >= SPELL_FIRST
                && (gSpellAIFlags[heroPointer->m_spells[slot]] & SPELL_AI_FLAG_COMBAT)) {
                spellScore =
                    gSpellAIValue[heroPointer->m_spells[slot]]
                    * ((gSpellAIFlags[heroPointer->m_spells[slot]]
                        & SPELL_AI_FLAG_SCALES_WITH_POWER)
                           ? (heroPointer->m_primaryStats[HERO_PRIMARY_SPELL_POWER]
                                      <= STAT_CURVE_LAST
                                  ? gBattleStat[heroPointer
                                                    ->m_primaryStats[HERO_PRIMARY_SPELL_POWER]]
                                  : gBattleStat[STAT_CURVE_LAST])
                           : spellMultiplier);
                magicTotal += spellScore
                              * gSpellCastNumMod
                                  [heroPointer->m_spellCharges[slot] <= SPELL_CAST_COUNT_LAST
                                       ? heroPointer->m_spellCharges[slot]
                                       : SPELL_CAST_COUNT_LAST];
                if (spellScore > bestScore)
                    bestScore = spellScore;
            }
        }
        if (magicTotal > bestScore * 3.5)
            magicTotal = bestScore * 3.5;
        if (magicTotal > armyValue * 2)
            magicTotal = armyValue * 1.25;
        else if (magicTotal > armyValue * 1.5)
            magicTotal = armyValue;
        else if (magicTotal > armyValue)
            magicTotal = armyValue * 0.75;
    }
    if (castleValue > armyValue * 2)
        castleValue = armyValue * 1.5;
    else if (castleValue > armyValue * 1.5)
        castleValue = armyValue * 1.25;
    else if (castleValue > armyValue)
        castleValue = armyValue * 0.9;
    armyValue += magicTotal;
    armyValue += castleValue;
    return armyValue;
}

VA(0x0044d38f, 0x192)
void philAI::EvaluateOneTimeCreaturePurchase(
    hero* aiHero,
    H1_ENUM_PARAM(CreatureType, i32) creature,
    i32 availableCount,
    b32 useAvailableCount,
    i32& purchaseCount,
    i32& purchaseValue,
    i32& replacementSlot
) {
    i32 leastStackValue;
    i32 replacementValue;
    i32 index;
    i32 purchasedValue;

    purchaseCount = 0;
    purchaseValue = 0;
    replacementSlot = -1;
    leastStackValue = 999999;
    if (useAvailableCount)
        purchaseCount = availableCount;
    else
        purchaseCount = MaxBuyableCreatures(creature);
    if (purchaseCount > availableCount)
        purchaseCount = availableCount;
    if (purchaseCount == 0)
        return;
    purchasedValue = purchaseCount * gMonsterDatabase[creature].fightValue;
    if (aiHero->m_army.CanJoin(creature) == 0) {
        for (index = 0; index < ARMY_GROUP_SLOT_COUNT; index++) {
            if (aiHero->m_army.m_creatureTypes[index] == creature) {
                replacementSlot = -1;
                index = ARMY_GROUP_SLOT_COUNT;
            } else {
                // Weighs the stack by the monster record of its slot number,
                // not of its creature type.
                replacementValue =
                    aiHero->m_army.m_creatureCounts[index]
                    * gMonsterDatabase[H1_ENUM_DECODE(CreatureType, index)].fightValue;
                if (replacementValue < leastStackValue) {
                    leastStackValue = replacementValue;
                    replacementSlot = index;
                }
            }
        }
    }
    if (replacementSlot != -1)
        purchasedValue -= leastStackValue;
    purchaseValue =
        purchasedValue * gGame->m_players[aiHero->m_owner].m_aiData.m_upgradeValueWeight;
    if (!useAvailableCount) {
        GetMonsterCost(creature, gCreatureCost);
        purchaseValue -= purchaseCount * RVConversion(gCreatureCost);
    }
    if (purchaseValue < 0) {
        purchaseValue = 0;
        purchaseCount = 0;
    }
}

VA(0x0044d521, 0x300)
i32 philAI::QuickCombat(
    armyGroup* attacker,
    hero* attackerHero,
    armyGroup* defender,
    hero* defenderHero,
    i8 townBattle,
    i8 townId,
    float& attackerCasualtyFraction,
    float& defenderCasualtyFraction
) {
    i32 attackerLossValue;
    float lostFraction;
    i32 tmp;
    i32 defExp;
    i32 ignored;
    i32 expectedDefenseLosses;
    i32 defenderLossValue;
    float roll;
    float diff;
    float attackerChance;
    i32 atkExp;
    b32 attackerWin;
    armyGroup* army;
    float victorChance;
    i32 result;
    i32 expectedAttackLosses;

    atkExp = gGame->ExperienceValueOfStack(attacker, attackerHero);
    defExp = gGame->ExperienceValueOfStack(defender, defenderHero);
    attackerWin = false;
    army = NULL;
    ProbableOutcomeOfBattle(
        attacker,
        attackerHero,
        defender,
        defenderHero,
        NULL,
        townBattle,
        townId,
        defenderHero != NULL ? defenderHero->m_owner : -1,
        attackerChance,
        attackerLossValue,
        defenderLossValue,
        expectedAttackLosses,
        expectedDefenseLosses,
        result
    );
    roll = Random(0, 100) / 100.0;
    if (roll < attackerChance) {
        attackerWin = true;
        victorChance = attackerChance;
        army = attacker;
    } else {
        victorChance = 1.0f - attackerChance;
        army = defender;
    }
    diff = roll > attackerChance ? roll - attackerChance : attackerChance - roll;
    if (attackerWin && attackerChance > 0.6)
        diff *= attackerChance + 0.65;
    lostFraction = (1.0 - diff) * (1.0 - diff);
    if (victorChance > 0.8 && lostFraction > 0.2)
        lostFraction *= lostFraction;
    if (victorChance > 0.96 && lostFraction > (1.0f - victorChance) / 2.0f)
        lostFraction = (1.0f - victorChance) / 2.0f;
    if (attackerWin) {
        if (attackerHero != NULL) {
            gAdvManager->GiveExperience(attackerHero, defExp, true);
            attackerHero->ApplyBattleWinTemps();
        }
        defenderCasualtyFraction = 1.0f;
        attackerCasualtyFraction = lostFraction;
    } else {
        if (attackerHero != NULL) {
            attackerHero->m_remainingMobility = 0;
            attackerHero->ApplyBattleLossTemps();
        }
        if (defenderHero != NULL)
            attackerHero->ApplyBattleWinTemps();
        defenderCasualtyFraction = lostFraction * diff;
        attackerCasualtyFraction = 1.0f;
        if (attackerCasualtyFraction >= 0.99 && defenderHero != NULL)
            gAdvManager->GiveExperience(defenderHero, defExp, true);
    }
    if (attackerCasualtyFraction > 0.99)
        gAdvManager->TransferArtifacts(attackerHero, defenderHero);
    else if (defenderCasualtyFraction > 0.99)
        gAdvManager->TransferArtifacts(defenderHero, attackerHero);
    DamageGroup(attacker, attackerHero, defenderHero, attackerCasualtyFraction);
    DamageGroup(defender, defenderHero, attackerHero, defenderCasualtyFraction);
    if (attackerWin && townBattle)
        gGame->ClaimTown(townId, gCurPlayer);
    return attackerWin;
}

#define targetShare myTargetShare // frame-slot spelling
VA(0x0044d821, 0xa15)
void philAI::HeroInteractionAtTown(
    hero* heroPointer,
    town* townPointer,
    b32 evaluateOnly,
    i32* value
) {
    i32 townFV;
    armyGroup* fromArmy;
    i32 statSum;
    b32 more;
    float townShareDiff;
    i32 heroTroops;
    i32 heroArmyValue;
    i32 whichSpell;
    b32 moveToHero;
    i32 estTransferValue;
    float garrisonShare;
    i32 choice;
    armyGroup* targetArmyGroup;
    i32 innerIndex;
    float targetShare;
    i32 i;
    i32 transferRating;
    b32 hasRoom;
    float transferFactor;
    float curveTerm;
    i32 speedLimit;
    i32 transferredCount;
    i32 bestFV;
    i32 stackFV;

    *value = 0;
    if (evaluateOnly) {
        if ((townPointer->m_buildings & H1_ENUM_BIT(BuildingSlotType, BUILDING_SLOT_SHIPYARD))
            && gBestShipyardId != townPointer->m_id) {
            i = MANHATTAN_LENGTH(
                townPointer->m_x - heroPointer->m_x,
                townPointer->m_y - heroPointer->m_y
            );
            if (gActualShipyardFound) {
                if (i < gBestShipyardDist) {
                    gBestShipyardDist = i;
                    gBestShipyardId = townPointer->m_id;
                }
            } else {
                gBestShipyardDist = i;
                gBestShipyardId = townPointer->m_id;
            }
            gPossibleShipyardFound = true;
            gActualShipyardFound = true;
        } else if ((townPointer->m_buildings & H1_ENUM_BIT(BuildingSlotType, BUILDING_SLOT_CASTLE))
                   && gAdvManager->GetCell(townPointer->m_x - 1, townPointer->m_y + 1)->m_tileIndex
                          < MAP_CELL_TILES_PER_TERRAIN
                   && !gActualShipyardFound && gBestShipyardId != townPointer->m_id) {
            i = MANHATTAN_LENGTH(
                townPointer->m_x - heroPointer->m_x,
                townPointer->m_y - heroPointer->m_y
            );
            if (gPossibleShipyardFound) {
                if (i < gBestShipyardDist) {
                    gBestShipyardDist = i;
                    gBestShipyardId = townPointer->m_id;
                }
            } else {
                gBestShipyardDist = i;
                gBestShipyardId = townPointer->m_id;
            }
            gPossibleShipyardFound = true;
        }
    } else if (heroPointer->m_primaryStats[HERO_PRIMARY_KNOWLEDGE] > 0
               && !heroPointer->HasArtifact(ARTIFACT_MAGIC_BOOK)
               && (townPointer->m_buildings
                   & H1_ENUM_BIT(BuildingSlotType, BUILDING_SLOT_MAGE_GUILD))) {
        if (gCurPlayerData->m_resources[RESOURCE_GOLD] >= TOWN_SPELL_BOOK_COST) {
            gAdvManager->GiveArtifact(heroPointer, ARTIFACT_MAGIC_BOOK);
            gCurPlayerData->m_resources[RESOURCE_GOLD] -= TOWN_SPELL_BOOK_COST;
        } else {
            heroPointer->m_remainingMobility = 0;
        }
    }
    if ((townPointer->m_buildings & H1_ENUM_BIT(BuildingSlotType, BUILDING_SLOT_MAGE_GUILD))
        && (evaluateOnly || heroPointer->HasArtifact(ARTIFACT_MAGIC_BOOK))) {
        for (i = 0; i < gMageGuildSpellCount[townPointer->m_buildState]; i++) {
            whichSpell = heroPointer->AddSpell(
                townPointer->m_mageGuildSpells[i],
                heroPointer->m_primaryStats[HERO_PRIMARY_KNOWLEDGE],
                evaluateOnly
            );
            *value += StatChangeValue(
                          heroPointer->m_primaryStats[HERO_PRIMARY_KNOWLEDGE] - whichSpell,
                          heroPointer->m_primaryStats[HERO_PRIMARY_KNOWLEDGE]
                      )
                      * gSpellAIValue[townPointer->m_mageGuildSpells[i]]
                      * ((gSpellAIFlags[townPointer->m_mageGuildSpells[i]]
                          & SPELL_AI_FLAG_SCALES_WITH_POWER)
                             ? heroPointer->m_primaryStats[HERO_PRIMARY_KNOWLEDGE]
                             : 1);
        }
    }
    heroArmyValue = FightValueOfStack(&heroPointer->m_army, NULL, false);
    townFV = FightValueOfStack(&townPointer->m_army, NULL, false);
    garrisonShare = static_cast<double>(townFV) / (townFV + heroArmyValue);
    statSum = 0;
    statSum = heroPointer->m_primaryStats[HERO_PRIMARY_ATTACK]
              + heroPointer->m_primaryStats[HERO_PRIMARY_DEFENSE];
    if (statSum > 10)
        statSum = 10;
    if (townPointer->m_buildings & H1_ENUM_BIT(BuildingSlotType, BUILDING_SLOT_CASTLE))
        targetShare = 0.54 - statSum * 0.02;
    else
        targetShare = 0.33 - statSum * 0.01;
    townShareDiff =
        (targetShare < garrisonShare ? garrisonShare - targetShare : targetShare - garrisonShare);
    if (townShareDiff < targetShare * 0.15)
        return;
    moveToHero = false;
    if (targetShare < garrisonShare)
        moveToHero = true;
    if (evaluateOnly) {
        if (heroArmyValue < townFV)
            transferFactor = 0.25f;
        else
            transferFactor = 0.13f;
        curveTerm = 1.0f + townShareDiff - 0.22;
        transferRating = (curveTerm * curveTerm - 1.0f)
                         * gCurPlayerData->m_aiData.m_upgradeValueWeight * (townFV + heroArmyValue)
                         * transferFactor;
        if (transferRating < 0)
            transferRating = 0;
        hasRoom = false;
        if (moveToHero) {
            for (i = 0; i < ARMY_GROUP_SLOT_COUNT; i++)
                if (heroPointer->m_army.m_creatureCounts[i] <= 0)
                    hasRoom = true;
        } else {
            for (i = 0; i < ARMY_GROUP_SLOT_COUNT; i++)
                if (townPointer->m_army.m_creatureCounts[i] <= 0)
                    hasRoom = true;
        }
        if (!hasRoom) {
            for (i = 0; i < ARMY_GROUP_SLOT_COUNT; i++) {
                for (innerIndex = 0; innerIndex < ARMY_GROUP_SLOT_COUNT; innerIndex++) {
                    if (townPointer->m_army.m_creatureTypes[i]
                        == heroPointer->m_army.m_creatureTypes[innerIndex]) {
                        hasRoom = true;
                        break;
                    }
                }
            }
        }
        if (!hasRoom)
            transferRating = 0;
        *value += transferRating;
        return;
    }
    if (moveToHero)
        townShareDiff = townShareDiff + 0.04;
    estTransferValue = (heroArmyValue + townFV) * townShareDiff;
    fromArmy = moveToHero ? &townPointer->m_army : &heroPointer->m_army;
    targetArmyGroup = moveToHero ? &heroPointer->m_army : &townPointer->m_army;
    more = true;
    gTroopReload = false;
    while (more) {
        if (!moveToHero) {
            heroTroops = 0;
            for (i = 0; i < ARMY_GROUP_SLOT_COUNT; i++)
                if (heroPointer->m_army.m_creatureTypes[i] != CREATURE_NONE)
                    heroTroops += heroPointer->m_army.m_creatureCounts[i];
            if (heroTroops <= 1)
                return;
        }
        choice = -1;
        for (i = 0; i < ARMY_GROUP_SLOT_COUNT; i++) {
            if (choice == -1) {
                for (innerIndex = 0; innerIndex < ARMY_GROUP_SLOT_COUNT; innerIndex++) {
                    if (fromArmy->m_creatureTypes[i] != CREATURE_NONE
                        && fromArmy->m_creatureTypes[i]
                               == targetArmyGroup->m_creatureTypes[innerIndex]) {
                        choice = i;
                        break;
                    }
                }
            }
        }
        if (choice == -1) {
            bestFV = -9999;
            if (moveToHero)
                speedLimit = H1_ENUM_ENCODE(CreatureSpeed, CREATURE_SPEED_SLOW);
            else
                speedLimit = H1_ENUM_ENCODE(CreatureSpeed, CREATURE_SPEED_FAST);
            for (i = 0; i < ARMY_GROUP_SLOT_COUNT; i++) {
                if (fromArmy->m_creatureTypes[i] != CREATURE_NONE) {
                    stackFV = fromArmy->m_creatureCounts[i]
                              * gMonsterDatabase[fromArmy->m_creatureTypes[i]].fightValue;
                    if ((moveToHero
                         && gMonsterDatabase[fromArmy->m_creatureTypes[i]].stats.speed > speedLimit)
                        || (!moveToHero
                            && gMonsterDatabase[fromArmy->m_creatureTypes[i]].stats.speed
                                   < speedLimit)) {
                        speedLimit = gMonsterDatabase[fromArmy->m_creatureTypes[i]].stats.speed;
                        bestFV = stackFV;
                        choice = i;
                    } else if (gMonsterDatabase[fromArmy->m_creatureTypes[i]].stats.speed
                                   == speedLimit
                               && stackFV > bestFV) {
                        bestFV = stackFV;
                        choice = i;
                    }
                }
            }
        }
        if (choice == -1) {
            more = false;
        } else if (targetArmyGroup->CanJoin(fromArmy->m_creatureTypes[choice])) {
            transferredCount = static_cast<float>(
                static_cast<double>(estTransferValue)
                    / gMonsterDatabase[fromArmy->m_creatureTypes[choice]].fightValue
                + 0.5
            );
            if (transferredCount > 0) {
                if (transferredCount > fromArmy->m_creatureCounts[choice]) {
                    transferredCount = fromArmy->m_creatureCounts[choice];
                } else {
                    more = false;
                    if (transferredCount >= fromArmy->m_creatureCounts[choice] * 0.65
                        || transferredCount >= fromArmy->m_creatureCounts[choice] - 1) {
                        if ((fromArmy->m_creatureCounts[choice] - transferredCount)
                                * gMonsterDatabase[fromArmy->m_creatureTypes[choice]].fightValue
                            < ((moveToHero ? townFV : heroArmyValue) - estTransferValue) * 0.2)
                            transferredCount = fromArmy->m_creatureCounts[choice];
                    }
                }
                if (!moveToHero && transferredCount >= heroTroops) {
                    transferredCount = heroTroops - 1;
                    more = false;
                }
                if (gMonsterDatabase[fromArmy->m_creatureTypes[choice]].fightValue
                        * transferredCount * 1.2
                    > estTransferValue)
                    more = false;
                else
                    estTransferValue -=
                        gMonsterDatabase[fromArmy->m_creatureTypes[choice]].fightValue
                        * transferredCount;
                targetArmyGroup->Add(
                    fromArmy->m_creatureTypes[choice],
                    transferredCount,
                    ARMY_GROUP_EMPTY_SLOT
                );
                fromArmy->m_creatureCounts[choice] -= transferredCount;
                if (fromArmy->m_creatureCounts[choice] == 0)
                    fromArmy->m_creatureTypes[choice] = CREATURE_NONE;
            } else {
                more = false;
            }
        } else {
            more = false;
        }
    }
    if (!evaluateOnly && gHumanTownConquered == townPointer->m_id
        && heroPointer->m_remainingMobility <= 20)
        heroPointer->m_remainingMobility = 0;
}
#undef targetShare

// Weighs the experience by the hero's AI fight value.
VA(0x0044e236, 0x3f)
i32 philAI::ChooseGoldOrExperience(hero* heroPointer, i32 gold, i32 experience) {
    i32 goldRV;
    i32 expRV;

    expRV = experience * heroPointer->m_aiFightValue;
    goldRV = gold * gAITurnCostResource[RESOURCE_GOLD];
    return goldRV > expRV;
}

VA(0x0044e275, 0xa4)
void philAI::ChooseEvaluateBattle(
    armyGroup* attackerArmy,
    hero* attackerHero,
    armyGroup* defenderArmy,
    hero* defenderHero,
    i32 isCastle,
    i32 castleId,
    i32 rewardValue,
    i32& canWin,
    i32& rating
) {
    float winChance;
    i32 attackLossValue;
    i32 defenseLossValue;
    i32 expectedAttackLosses;
    i32 expectedDefenseLosses;
    i32 thisVacant;
    i32 netValue;

    ProbableOutcomeOfBattle(
        attackerArmy,
        attackerHero,
        defenderArmy,
        defenderHero,
        NULL,
        isCastle,
        castleId,
        defenderHero != NULL ? defenderHero->m_owner : -1,
        winChance,
        attackLossValue,
        defenseLossValue,
        expectedAttackLosses,
        expectedDefenseLosses,
        netValue
    );
    netValue = netValue + rewardValue * winChance;
    if (netValue <= 0) {
        rating = 0;
        canWin = 0;
    } else {
        rating = netValue;
        canWin = 1;
    }
}

// Treasure-artifact purchase: affordable gold and an artifact worth
// more than its gold cost.
VA(0x0044e319, 0x42)
b32 philAI::ChooseToBuyArtifact(
    hero* heroPointer,
    H1_ENUM_PARAM(ArtifactType, i32) artifact,
    i32 goldCost
) {
    if (gCurPlayerData->m_resources[RESOURCE_GOLD] >= goldCost
        && gArtifactBaseRV[artifact] > goldCost * gAITurnCostResource[RESOURCE_GOLD])
        return true;
    else
        return false;
}

// Returns one for the ransom choice; the daemon-cave caller passes a hero and
// the gold amount.
VA(0x0044e35b, 0x12)
b32 philAI::ChooseToPayRansomOnHero(hero* heroPointer, i32 goldCost) {
    return true;
}

// The town update is written in place: the mage guild level, castle
// conversion and new dwelling stock.
VA(0x0044e36d, 0x140)
void philAI::BuildBuilding(town* townPointer, H1_ENUM_PARAM(BuildingSlotType, i16) building) {
    H1_ENUM_LOCAL(ResourceType, i32) i;
    H1_ENUM_ARRAY(i32, cost, ResourceType, RESOURCE_COUNT);

    GetBuildingCost(townPointer->m_type, building, cost, townPointer->m_buildState);
    for (i = RESOURCE_FIRST; i < RESOURCE_COUNT; i++)
        gCurPlayerData->m_resources[i] -= cost[i];
    if (building == BUILDING_SLOT_MAGE_GUILD) {
        if (townPointer->m_buildings & H1_ENUM_BIT(BuildingSlotType, BUILDING_SLOT_MAGE_GUILD))
            townPointer->m_buildState++;
        if (townPointer->m_occupyingHeroId != TOWN_OCCUPYING_HERO_NONE)
            townPointer->GiveSpells();
    }
    townPointer->m_buildings |= H1_ENUM_BIT(BuildingSlotType, building);
    if (building >= BUILDING_SLOT_DWELLING_FIRST && building <= BUILDING_SLOT_DWELLING_LAST)
        townPointer->m_dwellingAvailable[building - BUILDING_SLOT_DWELLING_FIRST] =
            gMonsterDatabase[gDwellingType[townPointer->m_type]
                                          [building - BUILDING_SLOT_DWELLING_FIRST]]
                .growth;
    if (building == BUILDING_SLOT_CASTLE) {
        townPointer->m_buildings &= ~H1_ENUM_BIT(BuildingSlotType, BUILDING_SLOT_TENT);
        townPointer->XformToCastle();
    }
    BitSet(gGame->m_townBuiltToday, townPointer->m_id);
    ShowStatus();
}

// The hero stands on the town cell and a random faction refills the tavern
// slot.
VA(0x0044e4ad, 0x223)
void philAI::BuildHero(town* townPointer, i16 availableHeroIndex) {
    hero* newHero;
    i16 townX;
    i16 townY;

    gCurPlayerData->m_resources[RESOURCE_GOLD] -= gHeroGoldCost;
    gCurPlayerData->m_heroIds[gCurPlayerData->m_heroCount] =
        gCurPlayerData->m_availableHeroIds[availableHeroIndex];
    gCurPlayerData->m_heroCount++;
    townX = townPointer->m_x;
    townY = townPointer->m_y;
    newHero = gGame->GetHero(gCurPlayerData->m_availableHeroIds[availableHeroIndex]);
    gGame->SetRandomHeroArmies(newHero->m_id, RANDOM_HERO_STRONG_ARMY);
    newHero->m_owner = gCurPlayer;
    newHero->m_x = townX;
    newHero->m_y = townY;
    newHero->m_eventFlags = HERO_EVENT_NONE;
    newHero->m_direction = MAP_DIRECTION_EAST;
    newHero->m_remainingMobility = newHero->CalcMobility();
    newHero->m_mobility = newHero->m_remainingMobility;
    newHero->m_locationType = gGame->m_map[townX][townY].m_triggerType;
    newHero->m_occupiedTown = gGame->m_map[townX][townY].m_objectMetadata;
    gGame->m_map[townX][townY].m_triggerType = MAP_EVENT_TRIGGER(MAP_OBJECT_HERO);
    gGame->m_map[townX][townY].m_objectMetadata =
        gCurPlayerData->m_availableHeroIds[availableHeroIndex];
    gGame->m_availableHeroes[newHero->m_id] = townPointer->m_owner;
    townPointer->m_occupyingHeroId = newHero->m_id;
    townPointer->GiveSpells();
    gCurPlayerData->m_availableHeroIds[availableHeroIndex] =
        gGame->GetNewHeroId(Random(0, HERO_CLASS_COUNT - 1));
    gGame->m_availableHeroes[gCurPlayerData->m_availableHeroIds[availableHeroIndex]] =
        HERO_AVAILABILITY_RETREATED;
    gHeroBuiltThisTurn = true;
    ShowStatus();
}

// Pay, take the stock and add the stack to the garrison.
VA(0x0044e6d0, 0xad)
void philAI::BuildCreature(town* townPointer, i32 dwelling, i32 purchaseCount) {
    H1_ENUM_ARRAY(i32, cost, ResourceType, RESOURCE_COUNT);
    H1_ENUM_LOCAL(CreatureType, i32) creature;
    H1_ENUM_LOCAL(ResourceType, i32) n;

    creature = gDwellingType[townPointer->m_type][dwelling];
    GetMonsterCost(creature, cost);
    for (n = RESOURCE_FIRST; n < RESOURCE_COUNT; n++)
        gCurPlayerData->m_resources[n] -= purchaseCount * cost[n];
    townPointer->m_dwellingAvailable[dwelling] -= purchaseCount;
    townPointer->m_army.Add(creature, purchaseCount, ARMY_GROUP_EMPTY_SLOT);
    ShowStatus();
}

VA(0x0044e77d, 0x13f)
b32 philAI::CanBuyBHC(BHC& purchase) {
    H1_ENUM_LOCAL(ResourceType, i32) index;
    H1_ENUM_LOCAL(CreatureType, i32) creature;
    H1_ENUM_ARRAY(i32, cost, ResourceType, RESOURCE_COUNT);
    switch (purchase.type) {
        case PURCHASE_BUILDING:
            if (CanBuy(purchase.townPointer, H1_ENUM_DECODE(BuildingSlotType, purchase.what)))
                return true;
            break;
        case PURCHASE_HERO:
            if (gCurPlayerData->m_resources[RESOURCE_GOLD] >= gHeroGoldCost
                && purchase.townPointer->m_occupyingHeroId == TOWN_OCCUPYING_HERO_NONE
                && !gHeroBuiltThisTurn)
                return true;
            break;
        case PURCHASE_CREATURE:
            creature = gDwellingType[purchase.townPointer->m_type][purchase.what];
            if (purchase.num > purchase.townPointer->m_dwellingAvailable[purchase.what])
                return false;
            if (!purchase.townPointer->m_army.CanJoin(creature))
                return false;
            GetMonsterCost(creature, cost);
            for (index = RESOURCE_FIRST; index < RESOURCE_COUNT; index++)
                if (gCurPlayerData->m_resources[index] < cost[index] * purchase.num)
                    return false;
            return true;
    }
    return false;
}

VA(0x0044e8bc, 0x170)
i8 philAI::CombatMonsterEvent(
    hero* heroPointer,
    H1_ENUM_PARAM(CreatureType, i8) monsterType,
    i32* monsterCount,
    mapCell* cell
) {
    float guardLosses;
    float heroCasualtyFraction;
    i32 result;
    i16 remaining;
    i16 i;

    CLEAR_ARMY_GROUP(*gMonGroup);
    if (*monsterCount / ARMY_GROUP_SLOT_COUNT > 0) {
        for (i = 0; i < ARMY_GROUP_SLOT_COUNT; i++) {
            gMonGroup->m_creatureTypes[i] = monsterType;
            gMonGroup->m_creatureCounts[i] = *monsterCount / ARMY_GROUP_SLOT_COUNT;
        }
    }
    for (i = *monsterCount % ARMY_GROUP_SLOT_COUNT - 1; i >= 0; i--) {
        gMonGroup->m_creatureTypes[i] = monsterType;
        gMonGroup->m_creatureCounts[i]++;
    }
    result = gPhilAI->QuickCombat(
        &heroPointer->m_army,
        heroPointer,
        gMonGroup,
        NULL,
        0,
        0,
        heroCasualtyFraction,
        guardLosses
    );
    remaining = 0;
    for (i = 0; i < ARMY_GROUP_SLOT_COUNT; i++)
        remaining += gMonGroup->m_creatureCounts[i];
    *monsterCount = remaining;
    if (result != 0)
        return 1;
    return 0;
}

#define netValue theWorth             // frame-slot spelling
#define rewardValue activeRewardValue // frame-slot spelling
VA(0x0044ea2c, 0x22f)
void philAI::FightEvent(hero* heroPointer, mapCell* cell) {
    i32 canFight;
    i32 rewardValue;
    i32 unusedValue;
    i32 netValue;
    i16 ghostCounts[GHOST_SITE_HUGE - GHOST_SITE_SMALL + 1];
    float heroCasualtyFraction;
    i16 n;
    float ghostLossFraction;
    i32 heroVictory;

    if (cell->m_objectMetadata == GHOST_SITE_EMPTY)
        return;
    ghostCounts[GHOST_SITE_SMALL - GHOST_SITE_SMALL] = GHOST_SMALL_COUNT / ARMY_GROUP_SLOT_COUNT;
    ghostCounts[GHOST_SITE_MEDIUM - GHOST_SITE_SMALL] = GHOST_MEDIUM_COUNT / ARMY_GROUP_SLOT_COUNT;
    ghostCounts[GHOST_SITE_LARGE - GHOST_SITE_SMALL] = GHOST_LARGE_COUNT / ARMY_GROUP_SLOT_COUNT;
    ghostCounts[GHOST_SITE_HUGE - GHOST_SITE_SMALL] = GHOST_HUGE_COUNT / ARMY_GROUP_SLOT_COUNT;
    for (n = 0; n < ARMY_GROUP_SLOT_COUNT; n++) {
        gMonGroup->m_creatureTypes[n] = CREATURE_GHOST;
        gMonGroup->m_creatureCounts[n] = ghostCounts[cell->m_objectMetadata - GHOST_SITE_SMALL];
    }
    switch (cell->m_objectMetadata) {
        case GHOST_SITE_SMALL:
            rewardValue = 1000.0f * gAITurnCostResource[RESOURCE_GOLD];
            break;
        case GHOST_SITE_MEDIUM:
            rewardValue = 2000.0f * gAITurnCostResource[RESOURCE_GOLD];
            break;
        case GHOST_SITE_LARGE:
            rewardValue = 5000.0f * gAITurnCostResource[RESOURCE_GOLD];
            break;
        case GHOST_SITE_HUGE:
            rewardValue = 2000.0f * gAITurnCostResource[RESOURCE_GOLD]
                          + gCurPlayerData->m_aiData.m_artifactValue;
            break;
        default:
            return;
    }
    ChooseEvaluateBattle(
        &heroPointer->m_army,
        heroPointer,
        gMonGroup,
        NULL,
        0,
        0,
        rewardValue,
        canFight,
        netValue
    );
    if (canFight) {
        heroVictory = QuickCombat(
            &heroPointer->m_army,
            heroPointer,
            gMonGroup,
            NULL,
            0,
            0,
            heroCasualtyFraction,
            ghostLossFraction
        );
        if (heroVictory) {
            switch (cell->m_objectMetadata) {
                case GHOST_SITE_SMALL:
                    gAdvManager->GiveResource(heroPointer, RESOURCE_GOLD, GHOST_SMALL_GOLD);
                    break;
                case GHOST_SITE_MEDIUM:
                    gAdvManager->GiveResource(heroPointer, RESOURCE_GOLD, GHOST_MEDIUM_GOLD);
                    break;
                case GHOST_SITE_LARGE:
                    gAdvManager->GiveResource(heroPointer, RESOURCE_GOLD, GHOST_LARGE_GOLD);
                    break;
                case GHOST_SITE_HUGE:
                    gAdvManager->GiveResource(heroPointer, RESOURCE_GOLD, GHOST_HUGE_GOLD);
                    gAdvManager->GiveRandomArtifact(heroPointer);
                    break;
            }
            cell->m_objectMetadata = GHOST_SITE_EMPTY;
        }
    }
}
#undef netValue
#undef rewardValue

VA(0x0044ec5b, 0x55)
i32 philAI::DamageGroup(armyGroup* group, hero* loser, hero* winner, float casualtyFraction) {
    if (casualtyFraction < 0.99) {
        group->DamageGroup(casualtyFraction);
        return 0;
    } else {
        if (loser != NULL)
            gAdvManager->HeroLoses(loser);
        else
            group->DamageGroup(casualtyFraction);
        return 1;
    }
}

// Primary-stat valuation: the table worth of the new level (capped at
// twenty) less that of the old one; used for hero stat gains.
VA(0x0044ecb0, 0x4f)
float philAI::StatChangeValue(i32 oldValue, i32 newValue) {
    return (newValue > SPELL_CAST_COUNT_LAST ? gSpellCastNumMod[SPELL_CAST_COUNT_LAST]
                                             : gSpellCastNumMod[newValue])
           - (oldValue > SPELL_CAST_COUNT_LAST ? gSpellCastNumMod[SPELL_CAST_COUNT_LAST]
                                               : gSpellCastNumMod[oldValue]);
}

// The AI-turn hourglass advances faster with fewer (prospective) heroes and
// stops at its last phase.
VA(0x0044ecff, 0xcc)
void philAI::IncrementHourGlass(void) {
    i32 heroCount = gCurPlayerData->m_heroCount;
    if (heroCount < 4 && gCurPlayerData->m_resources[RESOURCE_GOLD] >= 2500 && !gHeroBuiltThisTurn)
        heroCount++;
    gCurHourGlassPhase++;
    if (heroCount == AI_HOUR_GLASS_ONE_HERO) {
        gCurHourGlassPhase++;
        gCurHourGlassPhase++;
    }
    if (heroCount == AI_HOUR_GLASS_TWO_HEROES && gCurHourGlassPhase != AI_HOUR_GLASS_PHASE_1)
        gCurHourGlassPhase++;
    if (heroCount == AI_HOUR_GLASS_THREE_HEROES
        && (gCurHourGlassPhase == AI_HOUR_GLASS_PHASE_3
            || gCurHourGlassPhase == AI_HOUR_GLASS_PHASE_6))
        gCurHourGlassPhase++;
    if (gCurHourGlassPhase > AI_HOUR_GLASS_PHASE_LAST)
        gCurHourGlassPhase = AI_HOUR_GLASS_PHASE_LAST;
}

VA(0x0044edcb, 0x1e9)
void philAI::TownEvent(mapCell* cell, hero* heroPointer, i32 x, i32 y) {
    float garrisonCasualtyFraction;
    float heroCasualtyFraction;
    i32 savedPlayer;
    town* targetCastle;
    i32 heroVictory;
    hero* defenderHero;
    H1_ENUM_LOCAL(CombatSide, i32) combatResult;

    targetCastle = gGame->GetTown(cell->m_objectMetadata);
    savedPlayer = gCurPlayer;
    gAdvManager->DemobilizeCurrHero();
    if (targetCastle->m_owner != gCurPlayer) {
        if (targetCastle->HasGarrison()) {
            if (targetCastle->m_owner < 0 || !gHumanPlayer[targetCastle->m_owner]) {
                heroVictory = QuickCombat(
                    &heroPointer->m_army,
                    heroPointer,
                    &targetCastle->m_army,
                    NULL,
                    1,
                    targetCastle->m_id,
                    heroCasualtyFraction,
                    garrisonCasualtyFraction
                );
            } else {
                defenderHero = targetCastle->m_occupyingHeroId == TOWN_OCCUPYING_HERO_NONE
                                   ? NULL
                                   : gGame->GetHero(targetCastle->m_occupyingHeroId);
                combatResult = gAdvManager->DoCombat(
                    x,
                    y,
                    heroPointer,
                    &heroPointer->m_army,
                    targetCastle,
                    defenderHero,
                    &targetCastle->m_army,
                    x,
                    y,
                    COMBAT_RANDOM_SEED_NEW,
                    true
                );
                if (combatResult == COMBAT_RESULT_ATTACKER) {
                    gGame->ClaimTown(targetCastle->m_id, gCurPlayer);
                    gHumanTownConquered = targetCastle->m_id;
                }
            }
        } else {
            gGame->ClaimTown(targetCastle->m_id, gCurPlayer);
        }
    }
    if (targetCastle->m_owner == gCurPlayer && heroPointer->m_x == x && heroPointer->m_y == y) {
        targetCastle->m_occupyingHeroId = gCurPlayerData->CurrentHero();
        heroPointer->m_locationType = MAP_EVENT_TRIGGER(MAP_OBJECT_TOWN);
        heroPointer->m_occupiedTown = targetCastle->m_id;
        HeroInteractionAtTown(heroPointer, targetCastle, false, &gDummy);
    }
    gAdvManager->MobilizeCurrHero(false);
    targetCastle->GiveSpells();
}

// ValueOfEventAtPosition module state.
#define gArtifactPurchaseValue gItemPurchaseScore // spelling fixes .bss order
DATA(0x004bb138)
i32 gArtifactPurchaseValue;
#define gEventTownId gHeroGarrisonId // spelling fixes .bss order
DATA(0x004c8cb4)
i32 gEventTownId;
DATA(0x004ca15c)
b32 gCellSeen;
DATA(0x004b2214)
i32 gPurchaseTotal;
DATA(0x004c8ccc)
i32 gRecruitStackSlot;
DATA(0x004ca17c)
armyGroup* gEventTownArmy;
DATA(0x004bb158)
H1_ENUM_STORAGE(MapObjectType, i32) gDefaultEventType;
DATA(0x004c8cbc)
mapCell* gEventLocation;
#define gReduceByReload gDiscountByReloadFlag // spelling fixes .bss order
DATA(0x004b4b98)
b32 gReduceByReload;
#define gReduceByBerserk gDiscountForRampageFlag // spelling fixes .bss order
DATA(0x004ca174)
b32 gReduceByBerserk;
#define gEventTown gEventTownRec // spelling fixes .bss order
DATA(0x004ca184)
town* gEventTown;
DATA(0x004b2208)
i32 gVisitResult;
DATA(0x004bb15c)
i32 gEventMonsterCount;
DATA(0x004b2210)
i32 gEventTownScore;
#define gEventHero gFoe // spelling fixes .bss order
DATA(0x004b4ba4)
hero* gEventHero;

#define windmillResources theList    // frame-slot spelling
#define replacementSlot theArmySlot2 // frame-slot spelling
#define unusedChance tempChance      // frame-slot spelling
VA(0x0044efb4, 0x1d37)
i32 philAI::ValueOfEventAtPosition(hero* aiHero, i16 x, i16 y, i32 immediate, i32* liveChance) {
    DATA(0x0049ef80)
    static b32 gEvaluatingTravelGates = true;
    i32 numToBuy;
    i32 shouldBattle;
    H1_ENUM_ARRAY(i32, windmillResources, ResourceType, RESOURCE_COUNT);
    i32 ghostCount;
    i32 gateStrategicValue;
    i32 bestGateY;
    i32 twinY;
    i32 stayWorth;
    i32 twinX;
    i32 unusedChance;
    i32 lootGold;
    i32 replacementSlot;
    i32 bestExitValue;
    i32 captureValue;
    i32 dragonBattle;
    mapCell* candidateCell;
    i32 bestGateX;

    if (!immediate && gHeroEventStratRVOfPos[x][y] != RV_UNSET)
        return gHeroEventStratRVOfPos[x][y];
    gReduceByReload = true;
    gReduceByBerserk = true;
    *liveChance = AI_CHANCE_CERTAIN;
    gVisitResult = 0;
    gEventLocation = gAdvManager->GetCell(x, y);
    gCellSeen = gMapVisitFlags[x][y] && gCurPlayerBit;
    switch (MAP_TRIGGER_OBJECT(gEventLocation->m_triggerType)) {
        case MAP_OBJECT_ARTIFACT:
            gArtifactPickupValue =
                gArtifactBaseRV[H1_ENUM_DECODE(ArtifactType, gEventLocation->m_objectIndex)];
            for (gEventSlot = 0; gEventSlot < ARMY_GROUP_SLOT_COUNT; gEventSlot++) {
                gMonGroup->m_creatureTypes[gEventSlot] = CREATURE_ROGUE;
                gMonGroup->m_creatureCounts[gEventSlot] =
                    ARTIFACT_EVENT_GUARD_ROGUE_COUNT / ARMY_GROUP_SLOT_COUNT;
            }
            ProbableOutcomeOfBattle(
                &aiHero->m_army,
                aiHero,
                gMonGroup,
                NULL,
                NULL,
                0,
                0,
                GAME_PLAYER_NONE,
                gWinChance,
                gAttackerLoss,
                gDefenderLoss,
                gExpectedAttackerLoss,
                gExpectedDefenderLoss,
                gOutcome
            );
            gArtifactGuardedValue =
                gOutcome
                + gArtifactBaseRV[H1_ENUM_DECODE(ArtifactType, gEventLocation->m_objectIndex)]
                      * gWinChance;
            gArtifactPurchaseValue =
                gArtifactBaseRV[H1_ENUM_DECODE(ArtifactType, gEventLocation->m_objectIndex)]
                - 2000.0f * gAITurnCostResource[RESOURCE_GOLD];
            if (gArtifactPurchaseValue < 0)
                gArtifactPurchaseValue = 0;
            if (gCellSeen) {
                switch (gEventLocation->m_objectMetadata) {
                    case ARTIFACT_EVENT_MODE_PICKUP:
                        gVisitResult = gArtifactPickupValue;
                        break;
                    case ARTIFACT_EVENT_MODE_GUARDED:
                        gVisitResult = gArtifactGuardedValue;
                        break;
                    case ARTIFACT_EVENT_MODE_GOLD:
                        gVisitResult = gArtifactPurchaseValue;
                        break;
                }
            } else {
                gVisitResult = gArtifactPickupValue * 0.6 + gArtifactGuardedValue * 0.2
                               + gArtifactPurchaseValue * 0.2;
            }
            break;
        case MAP_OBJECT_ALCHEMIST_LAB:
        case MAP_OBJECT_MINE:
        case MAP_OBJECT_SAWMILL:
            if (gGame->m_mineOwners[gEventLocation->m_objectMetadata] == aiHero->m_owner) {
                gVisitResult = 0;
            } else if (gIAmGreatest && gGame->m_mineOwners[gEventLocation->m_objectMetadata] >= 0
                       && !gHumanPlayer[gGame->m_mineOwners[gEventLocation->m_objectMetadata]]) {
                gVisitResult = 0;
            } else {
                gVisitResult =
                    gMineIncome[gGame->m_mines[gEventLocation->m_objectMetadata].type]
                    * gAITurnCostResource[gGame->m_mines[gEventLocation->m_objectMetadata].type]
                    * gTurnValueOfMine[x][y];
                if (gGame->m_mineOwners[gEventLocation->m_objectMetadata] >= 0)
                    gVisitResult =
                        gVisitResult
                        * (gHumanPlayer[gGame->m_mineOwners[gEventLocation->m_objectMetadata]]
                               ? gAttackHumanBonus
                               : gAttackComputerBonus);
            }
            break;
        case MAP_OBJECT_OBELISK:
            if (gGame->m_obeliskVisitors[gEventLocation->m_objectMetadata - 1] & gCurPlayerBit)
                gVisitResult = 0;
            else
                gVisitResult = gCurPlayerData->m_aiData.m_obeliskValue;
            break;
        case MAP_OBJECT_MONSTER:
            gEventMonsterCount = gEventLocation->m_objectMetadata & MONSTER_COUNT_MASK;
            CLEAR_ARMY_GROUP(*gMonGroup);
            if (gEventMonsterCount / ARMY_GROUP_SLOT_COUNT > 0) {
                for (gEventSlot = 0; gEventSlot < ARMY_GROUP_SLOT_COUNT; gEventSlot++) {
                    gMonGroup->m_creatureTypes[gEventSlot] =
                        H1_ENUM_DECODE(CreatureType, gEventLocation->m_objectIndex);
                    gMonGroup->m_creatureCounts[gEventSlot] =
                        gEventMonsterCount / ARMY_GROUP_SLOT_COUNT;
                }
            }
            for (gEventSlot = gEventMonsterCount % ARMY_GROUP_SLOT_COUNT - 1; gEventSlot >= 0;
                 gEventSlot--) {
                gMonGroup->m_creatureTypes[gEventSlot] =
                    H1_ENUM_DECODE(CreatureType, gEventLocation->m_objectIndex);
                gMonGroup->m_creatureCounts[gEventSlot]++;
            }
            ProbableOutcomeOfBattle(
                &aiHero->m_army,
                aiHero,
                gMonGroup,
                NULL,
                NULL,
                0,
                0,
                GAME_PLAYER_NONE,
                gWinChance,
                gAttackerLoss,
                gDefenderLoss,
                gExpectedAttackerLoss,
                gExpectedDefenderLoss,
                gOutcome
            );
            EvaluateOneTimeCreaturePurchase(
                aiHero,
                H1_ENUM_DECODE(CreatureType, gEventLocation->m_objectIndex),
                gEventMonsterCount,
                true,
                numToBuy,
                gAttackerLoss,
                replacementSlot
            );
            if ((gEventLocation->m_objectMetadata & MONSTER_WILLING_FLAG)
                && gPhilAI->FightValueOfStack(&aiHero->m_army, aiHero, false)
                       > (gEventLocation->m_objectMetadata & MONSTER_COUNT_MASK)
                             * gMonsterDatabase
                                   [H1_ENUM_DECODE(CreatureType, gEventLocation->m_objectIndex)]
                                       .fightValue
                             * 1.75) {
                *liveChance = AI_CHANCE_CERTAIN;
                *liveChance = gWinChance * 60.0f + 40.0f;
                if (aiHero->m_army.CanJoin(
                        H1_ENUM_DECODE(CreatureType, gEventLocation->m_objectIndex)
                    ))
                    gVisitResult = gAttackerLoss;
                else
                    gVisitResult = 0;
                gVisitResult = gVisitResult * 0.6 + gOutcome * 0.4;
            } else {
                *liveChance = gWinChance * 100.0f;
                gVisitResult = gOutcome;
            }
            if (gVisitResult < 0)
                gReduceByReload = false;
            break;
        case MAP_OBJECT_HERO:
            if (gGame->m_availableHeroes[gEventLocation->m_objectMetadata] == aiHero->m_owner) {
                gHeroLiveChance[gEventLocation->m_objectMetadata] = AI_CHANCE_CERTAIN;
                if (!immediate || gTroopReload)
                    gVisitResult = 0;
                else
                    gVisitResult = -5000;
                *liveChance = 0;
            } else if (gIAmGreatest
                       && !gHumanPlayer
                              [gGame->m_availableHeroes[gEventLocation->m_objectMetadata]]) {
                gVisitResult = 0;
                *liveChance = AI_CHANCE_CERTAIN;
            } else {
                gEventTownScore = 0;
                gEventTown = NULL;
                gEventTownArmy = NULL;
                gEventHero = gGame->GetHero(gEventLocation->m_objectMetadata);
                if (gEventHero->m_locationType == MAP_EVENT_TRIGGER(MAP_OBJECT_TOWN)) {
                    gEventTown = gGame->GetTown(gEventHero->m_occupiedTown);
                    gEventTownArmy = &gEventTown->m_army;
                    gEventTownScore = ValueOfTown(gEventTown);
                    gEventTownId = gEventTown->m_id;
                    if (gEventTown->m_owner >= 0)
                        gEventTownScore =
                            gEventTownScore
                            * ((5 - gGame->m_playerCount) * 0.25
                                   * (gHumanPlayer[gEventTown->m_owner] ? gAttackHumanBonus
                                                                        : gAttackComputerBonus)
                               + 1.0);
                }
                if (immediate && gDebugLevel == AI_DEBUG_LEVEL_EVENT && x == AI_DEBUG_TRACE_COLUMN)
                    gDebugLevel = AI_DEBUG_LEVEL_BATTLE;
                ProbableOutcomeOfBattle(
                    &aiHero->m_army,
                    aiHero,
                    &gEventHero->m_army,
                    gEventHero,
                    gEventTownArmy,
                    gEventTownArmy != NULL,
                    gEventTownId,
                    gEventHero->m_owner,
                    gWinChance,
                    gAttackerLoss,
                    gDefenderLoss,
                    gExpectedAttackerLoss,
                    gExpectedDefenderLoss,
                    gVisitResult
                );
                if (immediate && gDebugLevel == AI_DEBUG_LEVEL_BATTLE)
                    gDebugLevel = AI_DEBUG_LEVEL_EVENT;
                *liveChance = gWinChance * 100.0f;
                if (gEventTownScore > 0)
                    gVisitResult = gVisitResult + gEventTownScore * gWinChance;
                if (immediate && gHumanPlayer[gEventHero->m_owner] && gVisitResult > 200)
                    gVisitResult = gVisitResult * 1.5;
                if (gWinChance > 0.75)
                    gHeroLiveChance[gEventLocation->m_objectMetadata] = AI_CHANCE_CERTAIN;
                else if (gWinChance > 0.5)
                    gHeroLiveChance[gEventLocation->m_objectMetadata] = gWinChance * 136.0f;
                else if (gWinChance > 0.4)
                    gHeroLiveChance[gEventLocation->m_objectMetadata] = gWinChance * 130.0f;
                else if (gWinChance > 0.3)
                    gHeroLiveChance[gEventLocation->m_objectMetadata] = gWinChance * 125.0f;
                else if (gWinChance > 0.2)
                    gHeroLiveChance[gEventLocation->m_objectMetadata] = gWinChance * 113.0f;
                else
                    gHeroLiveChance[gEventLocation->m_objectMetadata] = gWinChance * 100.0f;
                if (gHeroLiveChance[gEventLocation->m_objectMetadata] > AI_CHANCE_CERTAIN)
                    gHeroLiveChance[gEventLocation->m_objectMetadata] = AI_CHANCE_CERTAIN;
                if (!immediate && gWinChance < 0.4)
                    gVisitResult = gVisitResult * (3.0f - gWinChance * 2.0f);
                if (!immediate && gWinChance < 0.2)
                    gVisitResult = gVisitResult * (2.0f - gWinChance * 2.0f);
                if (gVisitResult < 0)
                    gReduceByReload = false;
                gReduceByBerserk = false;
            }
            break;
        case MAP_OBJECT_TOWN:
            gEventTown = gGame->GetTown(gEventLocation->m_objectMetadata);
            if (gGame->m_townOwners[gEventLocation->m_objectMetadata] == aiHero->m_owner) {
                if (gEventTown->m_occupyingHeroId != TOWN_OCCUPYING_HERO_NONE) {
                    gVisitResult = 0;
                } else {
                    gVisitResult = 0;
                    HeroInteractionAtTown(aiHero, gEventTown, true, &gVisitResult);
                    gVisitResult = gVisitResult * gHeroInteractionBonus[aiHero->m_id];
                }
                gReduceByReload = false;
            } else if (gIAmGreatest && gGame->m_townOwners[gEventLocation->m_objectMetadata] >= 0
                       && !gHumanPlayer[gGame->m_townOwners[gEventLocation->m_objectMetadata]]) {
                gVisitResult = 0;
            } else {
                gEventTownScore = ValueOfTown(gEventTown);
                if (immediate && gDebugLevel == AI_DEBUG_LEVEL_EVENT && x == AI_DEBUG_TRACE_COLUMN)
                    gDebugLevel = AI_DEBUG_LEVEL_BATTLE;
                if (gGame->GetTown(gEventLocation->m_objectMetadata)->m_occupyingHeroId
                    != TOWN_OCCUPYING_HERO_NONE)
                    ProbableOutcomeOfBattle(
                        &aiHero->m_army,
                        aiHero,
                        &gGame->m_heroRecs[gEventTown->m_occupyingHeroId].m_army,
                        &gGame->m_heroRecs[gEventTown->m_occupyingHeroId],
                        &gEventTown->m_army,
                        1,
                        gEventLocation->m_objectMetadata,
                        gEventTown->m_owner,
                        gWinChance,
                        gAttackerLoss,
                        gDefenderLoss,
                        gExpectedAttackerLoss,
                        gExpectedDefenderLoss,
                        gOutcome
                    );
                else if (gEventTown->HasGarrison())
                    ProbableOutcomeOfBattle(
                        &aiHero->m_army,
                        aiHero,
                        &gEventTown->m_army,
                        NULL,
                        NULL,
                        1,
                        gEventLocation->m_objectMetadata,
                        gEventTown->m_owner,
                        gWinChance,
                        gAttackerLoss,
                        gDefenderLoss,
                        gExpectedAttackerLoss,
                        gExpectedDefenderLoss,
                        gOutcome
                    );
                else {
                    gWinChance = 1.0f;
                    gOutcome = 0;
                }
                *liveChance = gWinChance * 100.0f;
                if (immediate && gDebugLevel == AI_DEBUG_LEVEL_BATTLE)
                    gDebugLevel = AI_DEBUG_LEVEL_EVENT;
                if (gEventTown->m_owner >= 0)
                    gEventTownScore =
                        gEventTownScore
                        * (((5 - gGame->m_playerCount) * 0.25 + 0.9)
                           * (gHumanPlayer[gEventTown->m_owner] ? gAttackHumanBonus
                                                                : gAttackComputerBonus));
                gVisitResult = gEventTownScore * gWinChance + gOutcome;
                if (gGame->m_townOwners[gEventLocation->m_objectMetadata] != GAME_PLAYER_NONE)
                    gReduceByBerserk = false;
            }
            break;
        case MAP_OBJECT_DAEMON_CAVE:
            if (gEventLocation->m_objectMetadata == DAEMON_CAVE_EMPTY) {
                gVisitResult = 0;
            } else {
                gVisitResult = aiHero->m_aiFightValue * 0.3 * 1000.0
                               + (aiHero->m_aiFightValue * 0.1 * 1000.0
                                  + gCurPlayerData->m_aiData.m_artifactValue)
                               + (aiHero->m_aiFightValue * 0.3 * 1000.0
                                  + 2500.0f * gAITurnCostResource[RESOURCE_GOLD])
                               + gAITurnCostResource[RESOURCE_GOLD] * -750.0;
                if (gEventLocation->m_objectMetadata == DAEMON_REWARD_RANSOM
                    && gCurPlayerData->m_resources[RESOURCE_GOLD] < DAEMON_GOLD)
                    gVisitResult = -100;
            }
            break;
        case MAP_OBJECT_OASIS:
            if (!(aiHero->m_eventFlags & HERO_EVENT_OASIS))
                gVisitResult = 200.0f * aiHero->m_aiFightValue;
            break;
        case MAP_OBJECT_BUOY:
            if (!(aiHero->m_eventFlags & HERO_EVENT_BUOY))
                gVisitResult = 200.0f * aiHero->m_aiFightValue;
            break;
        case MAP_OBJECT_STATUE:
            if (!(aiHero->m_eventFlags & HERO_EVENT_STATUE))
                gVisitResult = 400.0f * aiHero->m_aiFightValue;
            break;
        case MAP_OBJECT_FAERIE_RING:
            if (!(aiHero->m_eventFlags & HERO_EVENT_FAERIE_RING))
                gVisitResult = 200.0f * aiHero->m_aiFightValue;
            break;
        case MAP_OBJECT_FOUNTAIN:
            if (!(aiHero->m_eventFlags & HERO_EVENT_FOUNTAIN))
                gVisitResult = 200.0f * aiHero->m_aiFightValue;
            break;
        case MAP_OBJECT_TREASURE_CHEST:
            gVisitResult = 1500.0f * gAITurnCostResource[RESOURCE_GOLD];
            break;
        case MAP_OBJECT_CAMPFIRE:
            gVisitResult =
                500.0f * gAITurnCostResource[RESOURCE_GOLD]
                + (gAITurnCostResource[RESOURCE_WOOD] + gAITurnCostResource[RESOURCE_ORE]
                   + gAITurnCostResource[RESOURCE_CRYSTAL] + gAITurnCostResource[RESOURCE_SULFUR]
                   + gAITurnCostResource[RESOURCE_MERCURY] + gAITurnCostResource[RESOURCE_GEMS])
                      / 6.0f * 5.0f;
            break;
        case MAP_OBJECT_SPELL_SHRINE:
            if (aiHero->m_primaryStats[HERO_PRIMARY_KNOWLEDGE] > 0
                && aiHero->HasArtifact(ARTIFACT_MAGIC_BOOK)) {
                gEventSlot = aiHero->AddSpell(
                    H1_ENUM_DECODE(
                        SpellType,
                        gEventLocation->m_objectMetadata - MAP_EVENT_SPELL_OFFSET
                    ),
                    aiHero->m_primaryStats[HERO_PRIMARY_KNOWLEDGE],
                    true
                );
                gVisitResult = gSpellAIValue[H1_ENUM_DECODE(
                    SpellType,
                    gEventLocation->m_objectMetadata - MAP_EVENT_SPELL_OFFSET
                )];
                gVisitResult = gVisitResult
                               * StatChangeValue(
                                   aiHero->m_primaryStats[HERO_PRIMARY_KNOWLEDGE] - gEventSlot,
                                   aiHero->m_primaryStats[HERO_PRIMARY_KNOWLEDGE]
                               );
                if (gSpellAIFlags[H1_ENUM_DECODE(
                        SpellType,
                        gEventLocation->m_objectMetadata - MAP_EVENT_SPELL_OFFSET
                    )]
                    & SPELL_AI_FLAG_SCALES_WITH_POWER)
                    gVisitResult =
                        gVisitResult
                        * (aiHero->m_primaryStats[HERO_PRIMARY_KNOWLEDGE] <= STAT_CURVE_LAST
                               ? gStatPower[aiHero->m_primaryStats[HERO_PRIMARY_KNOWLEDGE]]
                               : gStatPower[STAT_CURVE_LAST]);
            } else {
                gVisitResult = 0;
            }
            break;
        case MAP_OBJECT_GAZEBO:
            if (aiHero->m_visitedSites & (1 << gEventLocation->m_objectMetadata))
                gVisitResult = 0;
            else
                gVisitResult = aiHero->m_aiFightValue * 1000.0f;
            break;
        case MAP_OBJECT_LIGHTHOUSE:
            if (gGame->m_mines[MINE_SLOT_LIGHTHOUSE].owner == aiHero->m_owner)
                gVisitResult = 0;
            else
                gVisitResult = 1000;
            break;
        case MAP_OBJECT_RESOURCE:
            switch (H1_ENUM_DECODE(
                ResourceType,
                gEventLocation->m_objectIndex - RESOURCE_PILE_OBJECT_BASE
            )) {
                case RESOURCE_GOLD:
                    gVisitResult = gEventLocation->m_objectMetadata * RESOURCE_PILE_GOLD_MULTIPLIER
                                   * gAITurnCostResource[RESOURCE_GOLD];
                    break;
                case RESOURCE_WOOD:
                    gVisitResult =
                        gEventLocation->m_objectMetadata * gAITurnCostResource[RESOURCE_WOOD];
                    break;
                case RESOURCE_ORE:
                    gVisitResult =
                        gEventLocation->m_objectMetadata * gAITurnCostResource[RESOURCE_ORE];
                    break;
                case RESOURCE_CRYSTAL:
                    gVisitResult =
                        gEventLocation->m_objectMetadata * gAITurnCostResource[RESOURCE_CRYSTAL];
                    break;
                case RESOURCE_SULFUR:
                    gVisitResult =
                        gEventLocation->m_objectMetadata * gAITurnCostResource[RESOURCE_SULFUR];
                    break;
                case RESOURCE_MERCURY:
                    gVisitResult =
                        gEventLocation->m_objectMetadata * gAITurnCostResource[RESOURCE_MERCURY];
                    break;
                case RESOURCE_GEMS:
                    gVisitResult =
                        gEventLocation->m_objectMetadata * gAITurnCostResource[RESOURCE_GEMS];
                    break;
            }
            break;
        case MAP_OBJECT_WINDMILL:
            if (gEventLocation->m_objectMetadata == WINDMILL_EMPTY) {
                gVisitResult = 0;
            } else {
                memset(windmillResources, 0, sizeof(windmillResources));
                windmillResources[H1_ENUM_DECODE(ResourceType, gEventLocation->m_objectMetadata)] =
                    WINDMILL_RESOURCE_AMOUNT;
                gVisitResult = RVConversion(windmillResources);
            }
            break;
        case MAP_OBJECT_SKELETON:
            if (gEventLocation->m_objectMetadata == SKELETON_EMPTY)
                gVisitResult = 0;
            else
                gVisitResult = gCurPlayerData->m_aiData.m_artifactValue * 0.1;
            break;
        case MAP_OBJECT_ANCIENT_LAMP:
            EvaluateOneTimeCreaturePurchase(
                aiHero,
                CREATURE_GENIE,
                gEventLocation->m_objectMetadata,
                false,
                gPurchaseTotal,
                gVisitResult,
                gRecruitStackSlot
            );
            gReduceByReload = false;
            break;
        case MAP_OBJECT_STRAW_HUT:
            EvaluateOneTimeCreaturePurchase(
                aiHero,
                CREATURE_GOBLIN,
                gEventLocation->m_objectMetadata,
                true,
                gPurchaseTotal,
                gVisitResult,
                gRecruitStackSlot
            );
            break;
        case MAP_OBJECT_HOUSE:
        case MAP_OBJECT_PEASANT_LOG_CABIN:
            EvaluateOneTimeCreaturePurchase(
                aiHero,
                CREATURE_PEASANT,
                gEventLocation->m_objectMetadata,
                true,
                gPurchaseTotal,
                gVisitResult,
                gRecruitStackSlot
            );
            gReduceByReload = false;
            break;
        case MAP_OBJECT_CABIN:
            EvaluateOneTimeCreaturePurchase(
                aiHero,
                CREATURE_ARCHER,
                gEventLocation->m_objectMetadata,
                true,
                gPurchaseTotal,
                gVisitResult,
                gRecruitStackSlot
            );
            gReduceByReload = false;
            break;
        case MAP_OBJECT_DWARF_LOG_CABIN:
            EvaluateOneTimeCreaturePurchase(
                aiHero,
                CREATURE_DWARF,
                gEventLocation->m_objectMetadata,
                true,
                gPurchaseTotal,
                gVisitResult,
                gRecruitStackSlot
            );
            gReduceByReload = false;
            break;
        case MAP_OBJECT_DESERT_TENT:
            EvaluateOneTimeCreaturePurchase(
                aiHero,
                CREATURE_NOMAD,
                gEventLocation->m_objectMetadata,
                false,
                gPurchaseTotal,
                gVisitResult,
                gRecruitStackSlot
            );
            gReduceByReload = false;
            break;
        case MAP_OBJECT_WAGON_CAMP:
            EvaluateOneTimeCreaturePurchase(
                aiHero,
                CREATURE_ROGUE,
                gEventLocation->m_objectMetadata,
                false,
                gPurchaseTotal,
                gVisitResult,
                gRecruitStackSlot
            );
            gReduceByReload = false;
            break;
        case MAP_OBJECT_GRAVEYARD:
        case MAP_OBJECT_SHIPWRECK:
            if (gEventLocation->m_objectMetadata == GHOST_SITE_EMPTY) {
                gVisitResult = 0;
            } else {
                switch (gEventLocation->m_objectMetadata) {
                    case GHOST_SITE_SMALL:
                        ghostCount = GHOST_SMALL_COUNT / ARMY_GROUP_SLOT_COUNT;
                        lootGold = GHOST_SMALL_GOLD;
                        break;
                    case GHOST_SITE_MEDIUM:
                        ghostCount = GHOST_MEDIUM_COUNT / ARMY_GROUP_SLOT_COUNT;
                        lootGold = GHOST_MEDIUM_GOLD;
                        break;
                    case GHOST_SITE_LARGE:
                        ghostCount = GHOST_LARGE_COUNT / ARMY_GROUP_SLOT_COUNT;
                        lootGold = GHOST_LARGE_GOLD;
                        break;
                    case GHOST_SITE_HUGE:
                        ghostCount = GHOST_HUGE_COUNT / ARMY_GROUP_SLOT_COUNT;
                        lootGold = GHOST_HUGE_GOLD;
                        break;
                }
                for (gEventSlot = 0; gEventSlot < ARMY_GROUP_SLOT_COUNT; gEventSlot++) {
                    gMonGroup->m_creatureTypes[gEventSlot] = CREATURE_GHOST;
                    gMonGroup->m_creatureCounts[gEventSlot] = ghostCount;
                }
                ChooseEvaluateBattle(
                    &aiHero->m_army,
                    aiHero,
                    gMonGroup,
                    NULL,
                    0,
                    0,
                    lootGold * gAITurnCostResource[RESOURCE_GOLD]
                        + (gEventLocation->m_objectMetadata == GHOST_SITE_HUGE
                               ? gCurPlayerData->m_aiData.m_artifactValue
                               : 0),
                    shouldBattle,
                    gVisitResult
                );
            }
            break;
        case MAP_OBJECT_DRAGON_CITY:
            captureValue =
                1000.0f * gAITurnCostResource[RESOURCE_GOLD] * gTurnValueOfMine[x][y] * 1.5;
            for (gEventSlot = 0; gEventSlot < ARMY_GROUP_SLOT_COUNT; gEventSlot++) {
                gMonGroup->m_creatureTypes[gEventSlot] = CREATURE_DRAGON;
                gMonGroup->m_creatureCounts[gEventSlot] =
                    DRAGON_CITY_DRAGON_COUNT / ARMY_GROUP_SLOT_COUNT;
            }
            if (gGame->m_mineOwners[MINE_SLOT_DRAGON_CITY] == aiHero->m_owner)
                gVisitResult = 0;
            else if (gGame->m_mineOwners[MINE_SLOT_DRAGON_CITY] != GAME_PLAYER_NONE)
                ChooseEvaluateBattle(
                    &aiHero->m_army,
                    aiHero,
                    gMonGroup,
                    NULL,
                    0,
                    0,
                    captureValue
                        * (gGame->m_players[gGame->m_mineOwners[MINE_SLOT_DRAGON_CITY]]
                               .m_aiData.m_artifactPoolShare
                           + 1.0),
                    dragonBattle,
                    gVisitResult
                );
            else
                ChooseEvaluateBattle(
                    &aiHero->m_army,
                    aiHero,
                    gMonGroup,
                    NULL,
                    0,
                    0,
                    captureValue,
                    dragonBattle,
                    gVisitResult
                );
            break;
        case MAP_OBJECT_STONE_LITHS:
        case MAP_OBJECT_WHIRLPOOL:
            if (!gEvaluatingTravelGates) {
                gVisitResult = 0;
                break;
            }
            gEvaluatingTravelGates = false;
            bestExitValue = -9999;
            for (twinY = 0; twinY < MAP_CELL_GRID_SIZE; twinY++) {
                for (twinX = 0; twinX < MAP_CELL_GRID_SIZE; twinX++) {
                    candidateCell = gAdvManager->GetCell(twinX, twinY);
                    if (MANHATTAN_LENGTH(twinX - x, twinY - y)
                            > (MAP_TRIGGER_OBJECT(gEventLocation->m_triggerType)
                                       == MAP_OBJECT_STONE_LITHS
                                   ? STONE_LITHS_MIN_DISTANCE
                                   : WHIRLPOOL_MIN_DISTANCE)
                        && candidateCell->m_triggerType == gEventLocation->m_triggerType) {
                        gateStrategicValue =
                            StrategicValueOfPosition(aiHero, twinX, twinY, false, &unusedChance);
                        gateStrategicValue = gateStrategicValue * 0.85;
                        if (gateStrategicValue > bestExitValue) {
                            bestExitValue = gateStrategicValue;
                            bestGateX = twinX;
                            bestGateY = twinY;
                        }
                    }
                }
            }
            stayWorth =
                StrategicValueOfPosition(aiHero, aiHero->m_x, aiHero->m_y, false, &unusedChance);
            if (bestExitValue > stayWorth + 200)
                gVisitResult = bestExitValue - stayWorth - 200;
            else
                gVisitResult = -200;
            gEvaluatingTravelGates = true;
            gReduceByReload = false;
            break;
        case MAP_OBJECT_WATERWHEEL:
            gVisitResult = gEventLocation->m_objectMetadata * WATERWHEEL_GOLD_MULTIPLIER
                           * gAITurnCostResource[RESOURCE_GOLD];
            break;
        case MAP_OBJECT_SHIP:
            gActualBoatFound = true;
            gVisitResult = 100;
            break;
        case MAP_OBJECT_SIGNPOST:
        case MAP_OBJECT_RANKING_SHRINE:
            gVisitResult = 0;
            break;
        case MAP_OBJECT_ROSEBUSH:
        case MAP_OBJECT_COAST:
        case MAP_OBJECT_TREE_STUMP:
        case MAP_OBJECT_OAK_TREE:
            gVisitResult = 0;
            break;
        default:
            if (gCurPlayerData->m_ultimateArtifactHintChance > 15
                && gCurPlayerData->m_ultimateArtifactHintX == x
                && gCurPlayerData->m_ultimateArtifactHintY == y) {
                gVisitResult = gUltArtifactAvgValue
                               * (gCurPlayerData->m_ultimateArtifactHintChance - 15) / 100;
            } else {
                gDefaultEventType = MAP_TRIGGER_OBJECT(gEventLocation->m_triggerType);
                if (gDefaultEventType >= MAP_OBJECT_NON_EVENT_FIRST
                    && gDefaultEventType <= MAP_OBJECT_TREES_LAST)
                    gVisitResult = 0;
            }
            break;
    }
    if (gTroopReload && gReduceByReload)
        gVisitResult = gVisitResult * gReduceFactor;
    if (gBerserk && gReduceByBerserk)
        gVisitResult = gVisitResult * gBerserkFactor;
    if (!immediate) {
        if (gVisitResult > 0 && (gMapExtra[x][y] & MAP_EXTRA_MONSTER_ADJACENT)
            && MAP_TRIGGER_OBJECT(gEventLocation->m_triggerType) != MAP_OBJECT_MONSTER)
            gVisitResult = 0;
        if (gVisitResult < 0
            && MAP_TRIGGER_OBJECT(gEventLocation->m_triggerType) != MAP_OBJECT_HERO)
            gVisitResult = 0;
        else if (gVisitResult > 32000)
            gVisitResult = 32000;
        else if (gVisitResult < -32000)
            gVisitResult = -32000;
        gHeroEventStratRVOfPos[x][y] = gVisitResult;
    }
    return gVisitResult;
}
#undef windmillResources
#undef replacementSlot
#undef unusedChance

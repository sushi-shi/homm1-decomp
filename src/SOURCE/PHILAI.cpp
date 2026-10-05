// Located from HoMM2 Buka 2.1; PoL 2.0 supplies the VC4 declaration.

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

// KB owns gDwellingType (retail KB .data band).
extern i8 gDwellingType[4][6];

// PHILAI's module state in retail address order: .data 0x0049f4c8-0x0048f827
// (shared with its logging helpers' literals), then .bss 0x004acec0-0x004c4eef
// (VC4 orders .bss by name hash, not by definition).
DATA(0x004ca188)
i8 gShowComputerRoute = 0;
DATA(0x0049ef78)
float gAttackHumanBonus = 2.0f;
DATA(0x0049ef7c)
float gAttackComputerBonus = 0.8f;
DATA(0x004b7434)
i16 gaiHeroEventStratRVOfPos[MAP_CELL_GRID_SIZE][MAP_CELL_GRID_SIZE];
DATA(0x004b4bac)
float fBerserkFactor;
DATA(0x004c8cb8)
i32 iLastFrameRateTimer;
DATA(0x004b4aa4)
i8 giCurPlayer;
DATA(0x004b4b9c)
float gWinChance;
DATA(0x004b2174)
i32 gEventLoop;
DATA(0x004ca164)
i8 giBuildShipyard[GAME_PLAYER_COUNT];
DATA(0x004ca16c)
i32 giMaxHeroesForThisPlayer;
DATA(0x004b4ba0)
i8 giBuildBoat[GAME_PLAYER_COUNT];
DATA(0x004b220c)
float fReduceFactor;
DATA(0x004c8cd0)
u8 giCurPlayerBit;
DATA(0x004b2170)
i8 giBestShipyardDist;
DATA(0x004c8cc8)
i32 bHeroBuiltThisTurn;
DATA(0x004c8cd4)
i16 gaiHeroLiveChance[GAME_HERO_COUNT];
DATA(0x004bb11c)
i32 gAttackerLoss;
DATA(0x004bb120)
i32 gDefenderLoss;
DATA(0x004b2218)
i32 giHumanTownConquered;
DATA(0x004c8cc4)
i32 giCurTurn;
DATA(0x004bb100)
i32 costTemp[RESOURCE_COUNT];
DATA(0x004b9cc0)
i8 gaiTurnValueOfMine[MAP_CELL_GRID_SIZE][MAP_CELL_GRID_SIZE];
DATA(0x004b221c)
i32 iDummy;
DATA(0x004b9cb4)
i8 gbPossibleShipyardFound;
DATA(0x004bb13c)
float gafAITurnCostResource[RESOURCE_COUNT];
DATA(0x004b7430)
u8 gCurWatchPlayerHighBit;
DATA(0x004ca168)
i32 iCurPlaceToVisit;
DATA(0x004b2220)
i8 giBestShipyardId;
DATA(0x004c8d1c)
i8 mapVisited[MAP_CELL_GRID_SIZE][MAP_CELL_GRID_SIZE];
DATA(0x004b2224)
i16 gaiHeroStrategicRVOfPos[MAP_CELL_GRID_SIZE][MAP_CELL_GRID_SIZE];
DATA(0x004ca180)
i8 gbActualBoatFound;
DATA(0x004b9cbc)
u8 giCurWatchPlayerBit;
DATA(0x004b4ba8)
playerData* gpCurPlayer;
DATA(0x004b2178)
float gfHeroInteractionBonus[GAME_HERO_COUNT];
DATA(0x004ca160)
i32 gbBerserk;
DATA(0x004ca178)
u8 giCurPlayerHighBit;
DATA(0x004b4bb0)
i16 gaiLiveChanceOfPos[MAP_CELL_GRID_SIZE][MAP_CELL_GRID_SIZE];
DATA(0x004b9cb8)
i8 giBuildBoatStuffTurn[GAME_PLAYER_COUNT];
DATA(0x004b4aa8)
i32 iPlacesVisited[ADVMGR_PLACE_VISIT_COUNT][ADVMGR_PLACE_COORDINATE_COUNT];
DATA(0x004c8cc0)
i32 gbTroopReload;
DATA(0x004ca170)
i8 gbActualShipyardFound;

// Buka 2.1's named AI factors. They are loaded, not folded, at /Od, and
// retail .rdata keeps them in this declaration order at 0x0048d070 ahead of
// the anonymous float literals.
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

// Misc's logging helpers open this object in retail (0x00427da5..0x00419f15),
// directly after the SVSearchArray initializer wrapper _$E2 at 0x00427d90.
// HoMM1's retained logging path opens KB.LOG afresh and writes its banner.
// NWC-only: no standalone Buka body; see buka-function-map.json.
void LogTruncate() {
    char logText[MISC_LOG_TEXT_CAPACITY];
    FILE* out;

    if (giDebugLevel < MISC_FILE_DEBUG_BEGIN)
        return;
    out = fopen("KB.LOG", "wt+");
    strcpy(logText, "===========New Log==========\n");
    fputs(logText, out);
    fclose(out);
}

// donor PoL RVA 0x000c6120; preferred Buka symbol ?LogStr@@YIXPAD@Z
// donor Buka TU BASE/Misc; HoMM1 owner inferred from contiguous order
// evidence: graph:1;base=0.724628;margin=0.262043;shape=0.429;size=0.928;calls=1.000;strings=KB.LOG;alternate=pol20:void LogStr(char *)@0x000c6120
// NWC-only: no standalone Buka body; see buka-function-map.json.
void LogStr(char* text) {
    char logText[MISC_LOG_TEXT_CAPACITY];
    FILE* out;

    if (giDebugLevel < MISC_FILE_DEBUG_BEGIN)
        return;
    out = fopen("KB.LOG", "at+");
    strcpy(logText, text);
    strcat(logText, "\n");
    fputs(logText, out);
    fclose(out);
    if (giDebugLevel == MISC_DEBUGGER_OUTPUT_LEVEL)
        OutputDebugStringA(logText);
}

// HoMM1 keeps the two-value logging form used by its AI call sites.
// NWC-only: no standalone Buka body; see buka-function-map.json.
void LogInt(char* label, i32 value) {
    char logText[MISC_LOG_VALUE_TEXT_CAPACITY];
    FILE* out;

    if (giDebugLevel < MISC_FILE_DEBUG_BEGIN)
        return;
    out = fopen("KB.LOG", "at+");
    sprintf(logText, "%s : % 8d \n", label, value);
    fputs(logText, out);
    fclose(out);
    if (giDebugLevel == MISC_DEBUGGER_OUTPUT_LEVEL)
        OutputDebugStringA(logText);
}

// Prior LZHUF source uses this two-long LogStr overload.
// NWC-only: no standalone Buka body; see buka-function-map.json.
void LogStr(char* label, i32 value1, i32 value2) {
    char logText[MISC_LOG_VALUE_TEXT_CAPACITY];
    FILE* out;

    if (giDebugLevel < MISC_FILE_DEBUG_BEGIN)
        return;
    out = fopen("KB.LOG", "at+");
    sprintf(logText, "%s : % 8d  % 8d\n", label, value1, value2);
    fputs(logText, out);
    fclose(out);
    if (giDebugLevel == MISC_DEBUGGER_OUTPUT_LEVEL)
        OutputDebugStringA(logText);
}

// HoMM1's five-value logging form; DDAppPaint logs its blit rectangles here.
// NWC-only: no standalone Buka body; see buka-function-map.json.
void LogStr(char* label, i32 value1, i32 value2, i32 value3, i32 value4, i32 value5) {
    char logText[MISC_LOG_VALUES_TEXT_CAPACITY];
    FILE* out;

    if (giDebugLevel < MISC_FILE_DEBUG_BEGIN)
        return;
    out = fopen("KB.LOG", "at+");
    sprintf(
        logText,
        "%s : % 8d  % 8d  % 8d  % 8d  % 8d\n",
        label,
        value1,
        value2,
        value3,
        value4,
        value5
    );
    fputs(logText, out);
    fclose(out);
    if (giDebugLevel == MISC_DEBUGGER_OUTPUT_LEVEL)
        OutputDebugStringA(logText);
}

// Seven-value form used by combat's action trace.
// NWC-only: no standalone Buka body; see buka-function-map.json.
void LogStr(
    char* label,
    i32 value1,
    i32 value2,
    i32 value3,
    i32 value4,
    i32 value5,
    i32 value6,
    i32 value7
) {
    char logText[MISC_LOG_VALUES_TEXT_CAPACITY];
    FILE* out;

    if (giDebugLevel < MISC_FILE_DEBUG_BEGIN)
        return;
    out = fopen("KB.LOG", "at+");
    sprintf(
        logText,
        "%s: % 6d % 6d % 6d % 6d % 6d % 6d % 6d\n",
        label,
        value1,
        value2,
        value3,
        value4,
        value5,
        value6,
        value7
    );
    fputs(logText, out);
    fclose(out);
    if (giDebugLevel == MISC_DEBUGGER_OUTPUT_LEVEL)
        OutputDebugStringA(logText);
}

// HoMM1 routes the status-line print through the AI object's debug font.
VA(0x00447900, 0x14)
void AiPrint(char* text) {
    gpPhilAI->ShowDebugText(text);
}

// @dead-code
// Zero-ref: no incoming call, jump or relocated reference in retail.
VA(0x00447914, 0x3d)
void AbsAiPrint(char* text) {
    i32 saved;

    if (giDebugLevel == 0)
        return;
    saved = giDebugLevel;
    giDebugLevel = MISC_FORCED_DEBUG_LEVEL;
    gpPhilAI->ShowDebugText(text);
    giDebugLevel = saved;
}

// philAI.h: the AI strategic-value maps philAI::DoAI resets through ResetHeroRVs.

// Buka ResetHeroRVs; HoMM1 has no off-map guard and indexes [x][y].
VA(0x00447951, 0x150)
void ResetHeroRVs(i32 resetAll, i32 x, i32 y) {
    i32 i;
    i32 j;

    for (i = 0; i < MAP_CELL_GRID_SIZE; i++) {
        for (j = 0; j < MAP_CELL_GRID_SIZE; j++) {
            if (resetAll) {
                if (MANHATTAN_LENGTH(x - i, y - j) < 10)
                    gaiHeroStrategicRVOfPos[i][j] = RV_UNSET;
            } else {
                gaiHeroStrategicRVOfPos[i][j] = RV_UNSET;
                gaiHeroEventStratRVOfPos[i][j] = RV_UNSET;
            }
        }
    }
    gaiHeroEventStratRVOfPos[x][y] = RV_UNSET;
    for (i = 0; i < GAME_HERO_COUNT; i++) {
        if (!resetAll
            || MANHATTAN_LENGTH(x - gpGame->m_heroRecs[i].m_x, y - gpGame->m_heroRecs[i].m_x) < 10)
            gaiHeroLiveChance[i] = RV_UNSET;
    }
}

// donor PoL RVA 0x000379d0; preferred Buka symbol ?CheckDoMain@@YIXHH@Z
// donor Buka TU SOURCE/PHILAI; HoMM1 owner inferred from contiguous order
// evidence: graph:5;base=0.463287;margin=0.627999;shape=0.348;size=0.713;calls=0.909;alternate=pol20:void CheckDoMain(int, int)@0x000379d0
VA(0x00447aa1, 0x1ca)
void CheckDoMain(i32, i32 doMain) {
    if (iLastFrameRateTimer + 15 < KBTickCount()
        || glTimers[ADVENTURE_FRAME_TIMER_SLOT] < KBTickCount()) {
        Process1WindowsMessage();
        PollSound();
        if (glTimers[ADVENTURE_FRAME_TIMER_SLOT] < KBTickCount()) {
            if (doMain == 0) {
                i32 oldShowIt = bShowIt;
                i32 oldX = gpAdvManager->m_previousOriginX;
                i32 oldY = gpAdvManager->m_previousOriginY;
                gDrawSavedCursor = 1;
                if (gConfig.blackoutComputer == 0 && gRemoteOn == 0)
                    bShowIt = 1;
                else
                    bShowIt = 0;
                if (bShowIt == 0)
                    bSpecialHideCursor = 1;
                if (gpAdvManager->ComboDraw(
                        gpAdvManager->m_previousOriginX,
                        gpAdvManager->m_previousOriginY,
                        0
                    ))
                    gpAdvManager->UpdateScreen(0, 0);
                else
                    gpAdvManager->UpdBottomView(0, 1, 1);
                bShowIt = oldShowIt;
                gDrawSavedCursor = 0;
                bSpecialHideCursor = 0;
                gpAdvManager->m_previousOriginX = oldX;
                gpAdvManager->m_previousOriginY = oldY;
            }
            glTimers[ADVENTURE_FRAME_TIMER_SLOT] = KBTickCount() + TIMER_DELAY;
        } else if (gpMouseManager->m_drawnX != gpMouseManager->m_mouseX - gpMouseManager->m_hotspotX
                   || gpMouseManager->m_drawnY
                          != gpMouseManager->m_mouseY - gpMouseManager->m_hotspotY) {
            gpMouseManager->MovePointer(gpMouseManager->m_mouseX, gpMouseManager->m_mouseY);
        }
        iLastFrameRateTimer = KBTickCount();
    }
}

// Both donors retain this intentionally empty status hook; retail has no side effects.
VA(0x00447c6b, 0x5)
void ShowStatus() {}

// HoMM1-only AI status line drawn with philAI's debug font across the bottom
// twenty screen rows; retail gates it on the second debug level.
VA(0x00447c70, 0x7e)
void philAI::ShowDebugText(char* text) {
    if (giDebugLevel >= 2) {
        FillBitmapArea(gpWindowManager->m_screen, 0, 460, LOGICAL_SCREEN_WIDTH, 20, 0);
        m_debugFont->DrawBoundedString(text, 0, 464, LOGICAL_SCREEN_WIDTH, 16, 1, FONT_ALIGN_LEFT);
        BlitBitmapToScreen(gpWindowManager->m_screen, 0, 460, LOGICAL_SCREEN_WIDTH, 20, 0, 460);
    }
}

// Preferred Buka's three build arrays, plus HoMM1's surviving debug-font owner.
VA(0x00447cee, 0x51)
philAI::philAI() {
    i32 i;

    m_debugFont = NULL;
    for (i = 0; i < GAME_PLAYER_COUNT; i++) {
        giBuildShipyard[i] = GAME_TOWN_NONE;
        giBuildBoat[i] = GAME_TOWN_NONE;
        giBuildBoatStuffTurn[i] = 0;
    }
}

// donor PoL RVA 0x00037bb5; preferred Buka symbol ?DoAllHeroInteractions@philAI@@QAEXXZ
// donor Buka TU SOURCE/PHILAI; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.414032;margin=0.418353;shape=0.188;size=0.773;calls=1.000;alternate=pol20:void philAI::DoAllHeroInteractions(void)@0x00037bb5
VA(0x00447d3f, 0x88)
void philAI::DoAllHeroInteractions(void) {
    i32 i;

    for (i = 0; i < gpCurPlayer->m_townCount; i++) {
        town* pTown = gpGame->GetTown(gpCurPlayer->m_townIds[i]);
        if (pTown->m_occupyingHeroId != TOWN_OCCUPYING_HERO_NONE)
            HeroInteractionAtTown(gpGame->GetHero(pTown->m_occupyingHeroId), pTown, 0, &iDummy);
    }
}

// donor PoL RVA 0x00037fdf; preferred Buka symbol ?CheckBuyStuff@philAI@@QAEXXZ
// donor Buka TU SOURCE/PHILAI; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.641449;margin=0.572995;shape=0.312;size=0.907;calls=0.824;strings=CheckBuy End  |CheckBuy Start;alternate=pol20:void philAI::CheckBuyStuff(void)@0x00037fdf
VA(0x00447dc7, 0x3ef)
void philAI::CheckBuyStuff(void) {
    i32 done = 0;
    i32 bought = 0;
    BHC bestBuy;
    town* dockTown;

    gpGame->CheckHeroConsistency();
    if (gpCurPlayer->m_resources[RESOURCE_GOLD] < 200)
        return;
    dockTown = NULL;
    if (giBuildShipyard[giCurPlayer] >= 0)
        dockTown = &gpGame->m_castleRecs[giBuildShipyard[giCurPlayer]];
    else if (giBuildBoat[giCurPlayer] >= 0)
        dockTown = &gpGame->m_castleRecs[giBuildBoat[giCurPlayer]];
    if (giBuildShipyard[giCurPlayer] >= 0)
        dockTown = gpGame->GetTown(giBuildShipyard[giCurPlayer]);
    else if (giBuildBoat[giCurPlayer] >= 0)
        dockTown = gpGame->GetTown(giBuildBoat[giCurPlayer]);
    if (dockTown && dockTown->m_owner != giCurPlayer) {
        giBuildShipyard[giCurPlayer] = GAME_TOWN_NONE;
        giBuildBoat[giCurPlayer] = GAME_TOWN_NONE;
        dockTown = NULL;
    }
    if (giBuildShipyard[giCurPlayer] >= 0) {
        if (CanBuy(dockTown, BUILDING_SLOT_SHIPYARD)
            && CanBuild(dockTown, BUILDING_SLOT_SHIPYARD)) {
            BuildBuilding(dockTown, BUILDING_SLOT_SHIPYARD);
            giBuildShipyard[giCurPlayer] = GAME_TOWN_NONE;
        } else {
            gpCurPlayer->m_resources[RESOURCE_GOLD] -= AI_SHIPYARD_GOLD_RESERVE;
            gpCurPlayer->m_resources[RESOURCE_WOOD] -= AI_SHIPYARD_WOOD_RESERVE;
        }
    }
    if (giBuildBoat[giCurPlayer] >= 0) {
        if ((dockTown->m_buildings & (1 << BUILDING_SLOT_SHIPYARD))
            && gpCurPlayer->m_resources[RESOURCE_GOLD] >= TOWN_BOAT_GOLD_COST
            && gpCurPlayer->m_resources[RESOURCE_WOOD] >= TOWN_BOAT_WOOD_COST) {
            if (gpGame->CreateBoat(dockTown->m_x - 1, dockTown->m_y + 1) != GAME_TABLE_FREE) {
                gpCurPlayer->m_resources[RESOURCE_GOLD] -= TOWN_BOAT_GOLD_COST;
                gpCurPlayer->m_resources[RESOURCE_WOOD] -= TOWN_BOAT_WOOD_COST;
            }
            giBuildBoat[giCurPlayer] = GAME_TOWN_NONE;
        } else {
            gpCurPlayer->m_resources[RESOURCE_GOLD] -= TOWN_BOAT_GOLD_COST;
            gpCurPlayer->m_resources[RESOURCE_WOOD] -= TOWN_BOAT_WOOD_COST;
        }
    }
    DoAllHeroInteractions();
    while (!done) {
        GetBestBHC(giCurPlayer, bestBuy);
        if (bestBuy.type >= PURCHASE_FIRST && CanBuyBHC(bestBuy)) {
            switch (bestBuy.type) {
                case PURCHASE_BUILDING:
                    BuildBuilding(bestBuy.pTown, bestBuy.what);
                    break;
                case PURCHASE_HERO:
                    BuildHero(bestBuy.pTown, bestBuy.what);
                    break;
                case PURCHASE_CREATURE:
                    BuildCreature(bestBuy.pTown, bestBuy.what, bestBuy.num);
                    break;
            }
            bought = 1;
        } else
            done = 1;
    }
    if (giBuildShipyard[giCurPlayer] >= 0) {
        gpCurPlayer->m_resources[RESOURCE_GOLD] += AI_SHIPYARD_GOLD_RESERVE;
        gpCurPlayer->m_resources[RESOURCE_WOOD] += AI_SHIPYARD_WOOD_RESERVE;
    }
    if (giBuildBoat[giCurPlayer] >= 0) {
        gpCurPlayer->m_resources[RESOURCE_GOLD] += TOWN_BOAT_GOLD_COST;
        gpCurPlayer->m_resources[RESOURCE_WOOD] += TOWN_BOAT_WOOD_COST;
    }
    DoAllHeroInteractions();
}

// donor PoL RVA 0x0003849d; preferred Buka symbol ?GoodAdjacent@philAI@@QAEHPAH@Z
// donor Buka TU SOURCE/PHILAI; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.410367;margin=0.413392;shape=0.366;size=0.596;calls=0.600;alternate=pol20:int philAI::GoodAdjacent(int *)@0x0003849d
VA(0x004481b6, 0x194)
i32 philAI::GoodAdjacent(hero* pHero, i32* direction) {
    i32 bestDirection;
    i32 idx;
    i32 x;
    i32 y;
    i32 num;
    i32 maxValueVal;
    i32 iChance;

    bestDirection = -1;
    maxValueVal = 100;
    if ((gpAdvManager->GetCell(pHero->m_x, pHero->m_y)->m_triggerType & MAP_TRIGGER_TYPE_MASK)
        == MAP_OBJECT_STONE_LITHS)
        return 0;
    for (idx = 0; idx < MAP_DIRECTION_COUNT; idx++) {
        if (gpAdvManager->ValidMoveWithEvent(pHero, idx)) {
            x = pHero->m_x + normalDirTable[idx].x;
            y = pHero->m_y + normalDirTable[idx].y;
            if ((gpAdvManager->GetCell(x, y)->m_triggerType & MAP_TRIGGER_EVENT)
                && !(mapExtra[x][y] & MAP_EXTRA_MONSTER_ADJACENT)
                && (gpAdvManager->GetCell(x, y)->m_triggerType & MAP_TRIGGER_TYPE_MASK)
                       != MAP_OBJECT_STONE_LITHS
                && (gpAdvManager->GetCell(x, y)->m_triggerType & MAP_TRIGGER_TYPE_MASK)
                       != MAP_OBJECT_WHIRLPOOL) {
                num = ValueOfEventAtPosition(pHero, x, y, 2, &iChance);
                if (iChance > 80 && num > maxValueVal) {
                    maxValueVal = num;
                    bestDirection = idx;
                }
            }
        }
    }
    if (bestDirection != -1) {
        *direction = bestDirection;
        return 1;
    }
    return 0;
}

// donor PoL RVA 0x00038785; preferred Buka symbol ?CheckReload@philAI@@QAEXXZ
// donor Buka TU SOURCE/PHILAI; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.433109;margin=0.377228;shape=0.310;size=0.859;calls=0.417;alternate=pol20:void philAI::CheckReload(void)@0x00038785
VA(0x0044834a, 0x384)
void philAI::CheckReload(hero* pHero) {
    i32 mapY;
    mapCell* tile;
    i32 heroFightValue;
    i32 mapX;
    i32 enemy;
    float enemyPressure;
    float isAlly;

    gbTroopReload = 0;
    fReduceFactor = 1.0f;
    isAlly = 0.0f;
    enemyPressure = 0.0f;
    heroFightValue = FightValueOfStack(&pHero->m_army, pHero, 0, 0, 0);
    if (heroFightValue < 100)
        heroFightValue = 100;
    gpSearchArray->SeedPosition(
        pHero->m_x,
        pHero->m_y,
        pHero->m_direction,
        pHero->m_mobility << 2,
        pHero->m_eventFlags & HERO_EVENT_EMBARKED,
        0,
        pHero->m_remainingMobility,
        pHero->m_heroClass,
        SEARCH_INVALID_COORDINATE,
        SEARCH_INVALID_COORDINATE,
        0,
        0
    );
    for (mapX = 0; mapX < MAP_CELL_GRID_SIZE; mapX++) {
        for (mapY = 0; mapY < MAP_CELL_GRID_SIZE; mapY++) {
            if (gpSearchArray->m_cells[mapX][mapY].visited) {
                tile = gpAdvManager->GetCell(mapX, mapY);
                switch (tile->m_triggerType) {
                    case MAP_TRIGGER_EVENT | MAP_OBJECT_TOWN:
                        enemy = FightValueOfStack(
                            &gpGame->GetTown(tile->m_objectMetadata)->m_army,
                            NULL,
                            0,
                            0,
                            0
                        );
                        if (gpGame->m_townOwners[tile->m_objectMetadata] == pHero->m_owner) {
                            if (enemy > heroFightValue * 2)
                                isAlly += (static_cast<float>(enemy) / (heroFightValue * 2) - 1.0f)
                                          * (pHero->m_mobility + 10)
                                          / (gpSearchArray->m_cells[mapX][mapY].distance + 10);
                        } else if (enemy > heroFightValue >> 1) {
                            enemyPressure +=
                                (static_cast<float>(enemy) / (heroFightValue >> 1) - 1.0f)
                                * (pHero->m_mobility + 30)
                                / (gpSearchArray->m_cells[mapX][mapY].distance + 30);
                        }
                        break;
                    case MAP_TRIGGER_EVENT | MAP_OBJECT_HERO:
                        if (gpGame->m_availableHeroes[tile->m_objectMetadata] != pHero->m_owner) {
                            enemy = FightValueOfStack(
                                &gpGame->GetHero(tile->m_objectMetadata)->m_army,
                                NULL,
                                0,
                                0,
                                0
                            );
                            if (enemy > heroFightValue >> 1)
                                enemyPressure +=
                                    (static_cast<float>(enemy) / (heroFightValue >> 1) - 1.0f)
                                    * (pHero->m_mobility + 30)
                                    / (gpSearchArray->m_cells[mapX][mapY].distance + 30);
                        }
                }
            }
        }
    }
    if (isAlly > 1.0f && enemyPressure > 1.0f) {
        fReduceFactor = 3.0f / (2.0f + isAlly + enemyPressure);
        gbTroopReload = 1;
    }
}

// donor PoL RVA 0x00038c3d; preferred Buka symbol ?CheckBerserk@philAI@@QAEXXZ
// donor Buka TU SOURCE/PHILAI; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.559466;margin=0.357720;shape=0.362;size=0.984;calls=1.000;alternate=pol20:void philAI::CheckBerserk(void)@0x00038c3d
VA(0x004486ce, 0x216)
void philAI::CheckBerserk(hero* pHero) {
    i32 enemy;
    i32 x;
    mapCell* cell;
    i32 y;
    hero* theHero;
    i32 theBest = -1;
    i32 val;

    gbBerserk = 0;
    fBerserkFactor = 1.0f;
    val = FightValueOfStack(&pHero->m_army, pHero, 1, 0, 0);
    if (val < 100)
        val = 100;
    if (val < 30000)
        return;
    for (x = 0; x < MAP_CELL_GRID_SIZE; x++) {
        for (y = 0; y < MAP_CELL_GRID_SIZE; y++) {
            cell = gpAdvManager->GetCell(x, y);
            switch (cell->m_triggerType) {
                case MAP_TRIGGER_EVENT | MAP_OBJECT_TOWN:
                    if (gpGame->m_townOwners[cell->m_objectMetadata] != pHero->m_owner) {
                        if (gpGame->m_townOwners[cell->m_objectMetadata] != GAME_PLAYER_NONE) {
                            enemy = FightValueOfStack(
                                &gpGame->GetTown(cell->m_objectMetadata)->m_army,
                                NULL,
                                1,
                                1,
                                cell->m_objectMetadata
                            );
                            if (enemy > val)
                                return;
                            if (enemy > theBest)
                                theBest = enemy;
                        }
                    }
                    break;
                case MAP_TRIGGER_EVENT | MAP_OBJECT_HERO:
                    if (gpGame->m_availableHeroes[cell->m_objectMetadata] != pHero->m_owner) {
                        theHero = gpGame->GetHero(cell->m_objectMetadata);
                        enemy = FightValueOfStack(
                            &theHero->m_army,
                            NULL,
                            1,
                            theHero->m_locationType == (MAP_TRIGGER_EVENT | MAP_OBJECT_TOWN),
                            theHero->m_occupiedTown
                        );
                        if (enemy * 2 > val)
                            return;
                        if (enemy * 2 > theBest)
                            theBest = enemy * 2;
                    }
                    break;
            }
        }
    }
    if (theBest <= 0)
        return;
    fBerserkFactor = theBest * 0.75 / val;
    gbBerserk = 1;
}

// Buka 2.1 DoDimensionDoor with DimensionDoorTo inlined for the given hero:
// HoMM1 teleports with three arguments and returns a byte flag.
VA(0x004488e4, 0x18b)
i8 philAI::DoDimensionDoor(hero* pHero) {
    i32 x;
    i32 i;
    i32 y;
    i32 len;
    i32 bestX, bestY;
    mapCell* cell;
    if (pHero->m_remainingMobility < 4)
        return 0;
    bestX = -1;
    x = pHero->m_x;
    y = pHero->m_y;
    for (i = gpSearchArray->m_pathLength - 1; i >= 1; i--) {
        x += normalDirTable[gpSearchArray->m_directions[i]].x;
        y += normalDirTable[gpSearchArray->m_directions[i]].y;
        if (abs(x - pHero->m_x) <= 7 && abs(y - pHero->m_y) <= 7) {
            cell = gpAdvManager->GetCell(x, y);
            if (!(cell->m_triggerType & MAP_TRIGGER_EVENT)
                && !(cell->m_secondaryTrigger & MAP_CELL_SECONDARY_BLOCKED)) {
                bestX = x;
                bestY = y;
                len = gpSearchArray->m_pathLength - i;
            }
        }
    }
    if (bestX == -1 || len <= 4)
        return 0;
    gpAdvManager->TeleportTo(bestX, bestY, 0);
    if (pHero->m_remainingMobility < SPELL_TRAVEL_MOBILITY_COST)
        pHero->m_remainingMobility = 0;
    else
        pHero->m_remainingMobility -= SPELL_TRAVEL_MOBILITY_COST;
    pHero->UseSpell(SPELL_DIMENSION_DOOR);
    return 1;
}

// donor PoL RVA 0x00039631; preferred Buka symbol ?DoAI@philAI@@QAEXH@Z
// donor Buka TU SOURCE/PHILAI; HoMM1 owner inferred from contiguous order
// evidence: graph:5;base=0.641984;margin=1.146879;shape=0.398;size=0.795;calls=0.741;strings====================================|DO AI|DO AI 1;alternate=pol20:void philAI::DoAI(int)@0x00039631
VA(0x00448a6f, 0x72f)
void philAI::DoAI(i32 player) {
    i32 pathIndex;
    i32 allMoveDone;
    i32 bestDirection;
    i32 origStep;
    i32 baseSteps;
    mapCell* eventCell;
    i32 newDummyValue;
    i8 nextStopAfterStep;
    i32 nextOldShowIt;
    i8 halfShown;
    i32 oldX;
    i32 ourY;
    i16 minRV;
    i8 moveInterrupt;
    hero* savedAiHero;
    i32 flagsState;
    i32 code;
    i32 tempArrayData[4];

    halfShown = 0;
    if (gGameOver)
        return;
    if (giLimitPlayer && player != giLimitPlayer)
        return;
    GetTurnAIVars(player);
    ShowStatus();
    CheckBuyStuff();
    IncrementHourGlass();
    SuspendSamples();
    SuspendMusic();
    while ((savedAiHero = DetermineHeroToMove(player)) != NULL) {
        giHumanTownConquered = GAME_TOWN_NONE;
        iCurPlaceToVisit = 0;
        if (gGameOver) {
            ResumeSamples();
            ResumeMusic();
            return;
        }
        CheckReload(savedAiHero);
        CheckBerserk(savedAiHero);
        gShowComputerRoute = 0;
        if (gConfig.blackoutComputer == 0 && gRemoteOn == 0
            && (gpGame->m_mapExtra[savedAiHero->m_x][savedAiHero->m_y] & gCurWatchPlayerHighBit)) {
            bShowIt = 1;
            gpAdvManager->SetHeroContext(savedAiHero->m_id, 0);
        } else {
            bShowIt = 0;
            gpAdvManager->SetHeroContext(savedAiHero->m_id, 0);
        }
        allMoveDone = 0;
        ResetHeroRVs(0, 0, 0);
        origStep = (savedAiHero->m_eventFlags & HERO_EVENT_EMBARKED) ? 15 : 5;
        minRV = savedAiHero->m_mobility + 42;
        origStep = static_cast<i32>(origStep * (1.7 - gpCurPlayer->m_difficulty * 0.1));
        minRV =
            static_cast<i16>(minRV * ((gpCurPlayer->m_difficulty - PLAYER_TYPE_DUMB) * 0.06 + 0.8));
        while (!allMoveDone && savedAiHero->m_remainingMobility >= 4) {
            if (gGameOver) {
                ResumeSamples();
                ResumeMusic();
                return;
            }
            if (savedAiHero->m_remainingMobility == savedAiHero->m_mobility
                && gpCurPlayer->m_ultimateArtifactHintChance > 15
                && gpCurPlayer->m_ultimateArtifactHintX == savedAiHero->m_x
                && gpCurPlayer->m_ultimateArtifactHintY == savedAiHero->m_y)
                gpAdvManager->ProcessSearch(savedAiHero->m_x, savedAiHero->m_y);
        retarget:
            DetermineTargetPosition(
                savedAiHero,
                savedAiHero->m_destinationX,
                savedAiHero->m_destinationY,
                minRV
            );
            for (pathIndex = 0; pathIndex < iCurPlaceToVisit; pathIndex++) {
                if (iPlacesVisited[pathIndex][0] == savedAiHero->m_destinationX
                    && iPlacesVisited[pathIndex][1] == savedAiHero->m_destinationY
                    && gpAdvManager
                               ->GetCell(savedAiHero->m_destinationX, savedAiHero->m_destinationY)
                               ->m_triggerType
                           != (MAP_TRIGGER_EVENT | MAP_OBJECT_TOWN))
                    savedAiHero->m_remainingMobility = 0;
            }
            if (iCurPlaceToVisit < ADVMGR_PLACE_VISIT_COUNT) {
                iPlacesVisited[iCurPlaceToVisit][0] = savedAiHero->m_x;
                iPlacesVisited[iCurPlaceToVisit][1] = savedAiHero->m_y;
                iCurPlaceToVisit++;
            }
            gShowComputerRoute = 1;
            if (savedAiHero->m_mobility == savedAiHero->m_remainingMobility) {
                halfShown = 0;
                IncrementHourGlass();
            }
            if (savedAiHero->m_destinationX != HERO_DESTINATION_NONE
                && savedAiHero->m_destinationY != HERO_DESTINATION_NONE) {
                eventCell = NULL;
                gpAdvManager->SetHeroContext(savedAiHero->m_id, 0);
                gpSearchArray->BuildPath(
                    savedAiHero->m_x,
                    savedAiHero->m_y,
                    savedAiHero->m_destinationX,
                    savedAiHero->m_destinationY,
                    savedAiHero->m_remainingMobility
                );
                if (gpSearchArray->m_pathLength > 0) {
                    gpAdvManager->UpdateScreen(0, 0);
                    if (savedAiHero->HasSpell(SPELL_DIMENSION_DOOR) && DoDimensionDoor(savedAiHero))
                        goto retarget;
                    baseSteps = 0;
                    pathIndex = gpSearchArray->m_pathLength - 1;
                    code = 0;
                    moveInterrupt = 0;
                    while (pathIndex >= 0 && baseSteps < origStep) {
                        nextStopAfterStep = (baseSteps + 1 == origStep || pathIndex == 0) ? 1 : 0;
                        if (pathIndex > 0 && GoodAdjacent(savedAiHero, &bestDirection)) {
                            gpSearchArray->m_directions[pathIndex] = bestDirection;
                            nextStopAfterStep = 1;
                        }
                        if (gpAdvManager->GetMoveShowIt(gpSearchArray->m_directions[pathIndex])) {
                            nextOldShowIt = bShowIt;
                            bShowIt = 1;
                            gpMouseManager->ReallyHidePointer();
                            bShowIt = nextOldShowIt;
                        }
                        eventCell = gpAdvManager->MoveHero(
                            gpSearchArray->m_directions[pathIndex],
                            nextStopAfterStep,
                            &oldX,
                            &ourY,
                            &code,
                            1,
                            &moveInterrupt
                        );
                        baseSteps++;
                        if (eventCell || code || moveInterrupt)
                            break;
                        pathIndex--;
                    }
                    if (savedAiHero->m_owner == HERO_OWNER_NONE)
                        goto nextHero;
                    if (savedAiHero->m_remainingMobility <= savedAiHero->m_mobility >> 1
                        && !halfShown) {
                        halfShown = 1;
                        IncrementHourGlass();
                    }
                    if (pathIndex < 0 && gpCurPlayer->m_ultimateArtifactHintChance > 15
                        && gpCurPlayer->m_ultimateArtifactHintX == savedAiHero->m_x
                        && gpCurPlayer->m_ultimateArtifactHintY == savedAiHero->m_y) {
                        if (savedAiHero->m_remainingMobility == savedAiHero->m_mobility)
                            gpAdvManager->ProcessSearch(
                                ADVMGR_SEARCH_VIEW_CENTER,
                                ADVMGR_SEARCH_VIEW_CENTER
                            );
                        else
                            savedAiHero->m_remainingMobility = 0;
                    }
                    if (pathIndex < 0
                        && (((savedAiHero->m_x != savedAiHero->m_destinationX
                              || savedAiHero->m_y != savedAiHero->m_destinationY)
                             && !eventCell)
                            || savedAiHero->m_remainingMobility < 4 || (code && !eventCell)))
                        allMoveDone = 1;
                    nextOldShowIt = bShowIt;
                    bShowIt = 1;
                    gpMouseManager->ReallyShowPointer();
                    bShowIt = nextOldShowIt;
                    gpAdvManager->UpdateRadar(1, 0);
                } else {
                    allMoveDone = 1;
                }
                if (eventCell) {
                    gpAdvManager->DoAIEvent(eventCell, savedAiHero, oldX, ourY);
                    if (gpCurPlayer->m_currentHero == INVALID_HERO)
                        goto nextHero;
                    ResetHeroRVs(1, savedAiHero->m_destinationX, savedAiHero->m_destinationY);
                }
            } else {
                allMoveDone = 1;
            }
        }
        savedAiHero->m_remainingMobility = 0;
        gpAdvManager->DeactivateCurrHero();
    nextHero:
        if (savedAiHero->m_locationType == (MAP_TRIGGER_EVENT | MAP_OBJECT_TOWN))
            CheckBuyStuff();
    }
    ResumeSamples();
    ResumeMusic();
}

// Buka 2.1 GetGameAIVars refreshes every player's game attention value.
VA(0x0044919e, 0x3f)
void philAI::GetGameAIVars(void) {
    i32 i;

    for (i = 0; i < gpGame->m_playerCount; i++)
        GetGameAttentionValue(i);
}

// donor PoL RVA 0x0003a329; preferred Buka symbol ?GetTurnAIVars@philAI@@QAEXH@Z
// donor Buka TU SOURCE/PHILAI; HoMM1 owner inferred from contiguous order
// evidence: graph:3;base=0.345425;margin=0.144973;shape=0.282;size=0.515;calls=0.667;alternate=pol20:void philAI::GetTurnAIVars(int)@0x0003a329
VA(0x004491dd, 0x5eb)
void philAI::GetTurnAIVars(i32 player) {
    i32 fightTotalSum;
    playerData* basePlayer;
    i32 theYPos;
    i32 mineTotal;
    i32 xPosVal;
    i32 i;
    float fightVal;
    i32 y;
    i32 indexNum;
    hero* heroPointer;
    i32 unusedFightValue;
    i32 artTotal;
    i32 entry;
    i32 oldX;
    town* townPtr;

    giCurTurn = gpGame->m_day + (gpGame->m_week - 1) * CALENDAR_DAYS_PER_WEEK
                + (gpGame->m_month - 1) * CALENDAR_DAYS_PER_MONTH;
    GetTurnAttentionValue(player);
    TurnCostResource(player);
    gCurHourGlassPhase = 0;
    gSandAnim = 0;
    gpCurPlayer->m_aiData.m_obeliskValue = static_cast<i32>(TurnValueOfObelisk(player));
    gpCurPlayer->m_aiData.m_unexploredValue = MeanRVOfUnexploredTerritory(player);
    bHeroBuiltThisTurn = 0;
    if (giCurTurn - giBuildBoatStuffTurn[player] > 8) {
        giBuildShipyard[player] = GAME_TOWN_NONE;
        giBuildBoat[player] = GAME_TOWN_NONE;
    }
    unusedFightValue = 0;
    fightVal = 0.0f;
    fightTotalSum = 0;
    for (i = 0; i < gpCurPlayer->m_heroCount; i++) {
        heroPointer = gpGame->GetHero(gpCurPlayer->m_heroIds[i]);
        fightVal =
            static_cast<float>(FightValueOfStack(&heroPointer->m_army, heroPointer, 0, 0, 0));
        fightTotalSum = static_cast<i32>(fightTotalSum + fightVal);
        heroPointer->m_aiFightValue = fightVal * 4e-05 + 0.4;
    }
    for (i = 0; i < gpCurPlayer->m_townCount; i++) {
        townPtr = gpGame->GetTown(gpCurPlayer->m_townIds[i]);
        fightVal = static_cast<float>(FightValueOfStack(&townPtr->m_army, NULL, 0, 0, 0));
        fightTotalSum = static_cast<i32>(fightTotalSum + fightVal);
    }
    gpCurPlayer->m_aiData.m_upgradeValueWeight =
        static_cast<float>(
            gpCurPlayer->m_resources[RESOURCE_GOLD] + gpCurPlayer->m_aiData.m_income[RESOURCE_GOLD]
        ) / (fightTotalSum + 1000)
        + gpCurPlayer->m_aiData.m_attentionWeights.upgradeBase;
    artTotal = 0;
    for (i = ARTIFACT_REGULAR_FIRST; i < ARTIFACT_REGULAR_END; i++)
        artTotal += gArtifactBaseRV[i];
    for (i = 0; i < gpGame->m_playerCount; i++)
        gpGame->m_players[i].m_aiData.m_artifactPoolShare =
            1.0 / (gpGame->m_playerCount + gpGame->m_deadPlayerCount);
    gpCurPlayer->m_aiData.m_artifactValue = artTotal / 33.0;
    memset(gaiTurnValueOfMine, 7, sizeof(gaiTurnValueOfMine));
    for (indexNum = 0; indexNum < gpGame->m_playerCount; indexNum++) {
        if (indexNum != giCurPlayer) {
            basePlayer = &gpGame->m_players[indexNum];
            for (entry = 0; entry < basePlayer->m_heroCount; entry++) {
                xPosVal = gpGame->GetHero(basePlayer->m_heroIds[entry])->m_x;
                theYPos = gpGame->GetHero(basePlayer->m_heroIds[entry])->m_y;
                for (oldX = xPosVal - 10; oldX <= xPosVal + 10; oldX++) {
                    for (y = theYPos - 10; y <= theYPos + 10; y++) {
                        if (oldX >= 0 && oldX < MAP_CELL_GRID_SIZE && y >= 0
                            && y < MAP_CELL_GRID_SIZE) {
                            mineTotal = abs(MANHATTAN_LENGTH(oldX - xPosVal, y - theYPos) - 4) >> 2;
                            if (mineTotal < gaiTurnValueOfMine[oldX][y])
                                gaiTurnValueOfMine[oldX][y] = mineTotal;
                        }
                    }
                }
            }
        }
    }
    for (i = 0; i < GAME_HERO_COUNT; i++)
        gfHeroInteractionBonus[i] = 1.0f;
    if (gpCurPlayer->m_difficulty == PLAYER_TYPE_DUMB) {
        gAttackHumanBonus = 0.6f;
        gAttackComputerBonus = 1.3f;
    } else if (gpCurPlayer->m_difficulty == PLAYER_TYPE_AVERAGE) {
        gAttackHumanBonus = 1.0f;
        gAttackComputerBonus = 1.0f;
    } else {
        gAttackHumanBonus = gpCurPlayer->m_difficulty * 0.07 + 1.0;
        gAttackComputerBonus = 1.1 - gpCurPlayer->m_difficulty * 0.12;
    }
    if (gbIAmGreatest)
        gAttackComputerBonus = 0.1f;
    giMaxHeroesForThisPlayer = 3;
    if (gpGame->m_playerCount - gpGame->m_deadPlayerCount == 2)
        giMaxHeroesForThisPlayer++;
    if (gpGame->m_playerCount - gpGame->m_deadPlayerCount == 3)
        giMaxHeroesForThisPlayer++;
    if (gpCurPlayer->m_townCount >= 5)
        giMaxHeroesForThisPlayer++;
    if (gpCurPlayer->m_townCount >= 10)
        giMaxHeroesForThisPlayer++;
}

// donor PoL RVA 0x0003b154; preferred Buka symbol ?GetBestBHC@philAI@@QAEXHAAUBHC@@@Z
// donor Buka TU SOURCE/PHILAI; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.679791;margin=0.499826;shape=0.401;size=0.937;calls=0.722;strings=BestBHC |Turns Owned;alternate=pol20:void philAI::GetBestBHC(int, struct BHC &)@0x0003b154
VA(0x004497c8, 0x54c)
void philAI::GetBestBHC(i32, BHC& best) {
    float newFValue = 1.0f;
    float newValue = -99.0f;
    i32 total = 0;
    i32 totalWeights = 0;
    i32 ideal[GAME_TOWN_COUNT];
    i32 thisStrengths[GAME_TOWN_COUNT];
    BHC choice;
    i32 townNo;
    town* townPointer;
    i32 curMeanStrength;

    for (townNo = 0; townNo < gpCurPlayer->m_townCount; townNo++) {
        townPointer = &gpGame->m_castleRecs[gpCurPlayer->m_townIds[townNo]];
        thisStrengths[townNo] = FightValueOfStack(&townPointer->m_army, NULL, 0, 0, 0) + 400;
        total += thisStrengths[townNo];
        if (townPointer->m_buildings & (1 << BUILDING_SLOT_CASTLE))
            totalWeights += 10;
        else
            totalWeights += 7;
    }
    if (totalWeights < 1)
        totalWeights = 1;
    curMeanStrength = total / totalWeights;
    for (townNo = 0; townNo < gpCurPlayer->m_townCount; townNo++) {
        townPointer = &gpGame->m_castleRecs[gpCurPlayer->m_townIds[townNo]];
        ideal[townNo] =
            curMeanStrength * ((townPointer->m_buildings & (1 << BUILDING_SLOT_CASTLE)) ? 10 : 7)
            + 400;
    }
    for (townNo = 0; townNo < gpCurPlayer->m_townCount; townNo++) {
        townPointer = &gpGame->m_castleRecs[gpCurPlayer->m_townIds[townNo]];
        extern i32 gRemoteReady;
        if (giCurTurn > 3 && (!gRemoteOn || gRemoteReady) && townPointer->m_turnsOwned < 3)
            continue;
        CheckDoMain(0, 0);
        GetBestBuilding(townPointer, choice, newFValue);
        newFValue = newFValue * ((100 - Random(0, 10)) / 100.0);
        if (newFValue > newValue) {
            newValue = newFValue;
            best = choice;
        }
        CheckDoMain(0, 0);
        GetBestCreature(townPointer, choice, newFValue);
        newFValue = newFValue
                    * (static_cast<float>(ideal[townNo])
                           / (static_cast<float>(thisStrengths[townNo])) / 3.0f
                       + 0.66);
        newFValue = newFValue * ((100 - Random(0, 10)) / 100.0);
        if (newFValue > newValue) {
            newValue = newFValue;
            best = choice;
        }
        CheckDoMain(0, 0);
        if (gpCurPlayer->m_heroCount < giMaxHeroesForThisPlayer
            && (townPointer->m_buildings & (1 << BUILDING_SLOT_CASTLE))) {
            GetBestHero(townPointer, choice, newFValue);
            newFValue = newFValue * ((100 - Random(0, 10)) / 100.0);
            if (!bHeroBuiltThisTurn && giCurTurn > 5 && newFValue > 0.0f) {
                if ((gpCurPlayer->m_aiData.m_income[RESOURCE_GOLD] >= 1250
                     && gpCurPlayer->m_heroCount < giMaxHeroesForThisPlayer - 2)
                    || gpCurPlayer->m_heroCount <= 1)
                    newFValue += 500.0f;
                else if (gpCurPlayer->m_aiData.m_income[RESOURCE_GOLD] >= 1500
                         && gpCurPlayer->m_heroCount < giMaxHeroesForThisPlayer - 1)
                    newFValue = newFValue * 1.3;
            } else if (gpCurPlayer->m_heroCount == 0) {
                newFValue += 500.0f;
            }
            if (newFValue > newValue) {
                newValue = newFValue;
                best = choice;
            }
        }
    }
    if (newValue < 0.02)
        best.type = PURCHASE_NONE;
}

// Buka 2.1 DetermineHeroToMove: the current player's hero with the most
// remaining mobility; HoMM1 counts with a byte index.
VA(0x00449d14, 0xea)
hero* philAI::DetermineHeroToMove(i32 player) {
    i32 bestHero;
    i32 bestMobility;
    i32 mobility;
    i8 i;

    bestMobility = 0;
    bestHero = -1;
    if (gpCurPlayer->HasMobileHero()) {
        for (i = 0; i < gpCurPlayer->m_heroCount; i++) {
            mobility =
                gpGame->m_heroRecs[gpGame->m_players[player].m_heroIds[i]].m_remainingMobility;
            if (mobility > bestMobility) {
                bestMobility = mobility;
                bestHero = i;
            }
        }
    }
    if (bestHero >= 0)
        return &gpGame->m_heroRecs[gpGame->m_players[player].m_heroIds[bestHero]];
    gpGame->m_players[player].m_currentHero = INVALID_HERO;
    return NULL;
}

// donor PoL RVA 0x0003b865; preferred Buka symbol ?DetermineTargetPosition@philAI@@QAEHAAH0H0@Z
// donor Buka TU SOURCE/PHILAI; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.393525;margin=0.196143;shape=0.308;size=0.686;calls=0.529;alternate=pol20:int philAI::DetermineTargetPosition(int &, int &, int, int &)@0x0003b865
VA(0x00449dfe, 0x89d)
void philAI::DetermineTargetPosition(hero* pHero, i8& targetX, i8& targetY, i16 mobility) {
    i32 theBestRV;
    i32 mainValue;
    i32 curSpacing;
    i32 lastType;
    town* portTown;
    i32 selCnt;
    i32 validFlag;
    i32 colPhase;
    i32 entry;
    i16 oldX;
    i16 bestY;
    mapCell* thisCellRec;
    i16 bestX;
    i16 y;

    bestX = -1;
    bestY = -1;
    theBestRV = -999999;
    giBestShipyardId = GAME_TOWN_NONE;
    gbPossibleShipyardFound = 0;
    gbActualShipyardFound = 0;
    gbActualBoatFound = 0;
    curSpacing = pHero->m_mobility / 6;
    thisCellRec = gpAdvManager->GetCell(pHero->m_x, pHero->m_y);
    lastType = CELL_TERRAIN(thisCellRec);
    if (lastType == TERRAIN_SNOW || lastType == TERRAIN_SWAMP) {
        curSpacing--;
        mobility = static_cast<i16>(mobility * 1.25);
    }
    if (lastType == TERRAIN_DESERT) {
        curSpacing -= 2;
        mobility = static_cast<i16>(mobility * 1.5);
    }
    if (curSpacing < 3)
        curSpacing = 3;
    gpSearchArray->SeedPosition(
        pHero->m_x,
        pHero->m_y,
        pHero->m_direction,
        mobility * 3,
        pHero->m_eventFlags & HERO_EVENT_EMBARKED,
        1,
        pHero->m_remainingMobility,
        pHero->m_heroClass,
        SEARCH_INVALID_COORDINATE,
        SEARCH_INVALID_COORDINATE,
        0,
        0
    );
    gpSearchArray->m_cells[pHero->m_x][pHero->m_y].visited = 0;
    colPhase = -1;
    for (oldX = 0; oldX < MAP_CELL_GRID_SIZE; oldX++) {
        selCnt = -1;
        colPhase++;
        if (colPhase >= curSpacing)
            colPhase = 0;
        for (y = 0; y < MAP_CELL_GRID_SIZE; y++) {
            selCnt++;
            if (selCnt >= curSpacing)
                selCnt = 0;
            if (gpSearchArray->m_cells[oldX][y].visited) {
                thisCellRec = gpAdvManager->GetCell(oldX, y);
                if (gpSearchArray->m_cells[oldX][y].distance > mobility) {
                    if (gpSearchArray->m_cells[oldX][y].distance > mobility * 2)
                        validFlag = 0;
                    else
                        validFlag =
                            thisCellRec->m_triggerType == (MAP_TRIGGER_EVENT | MAP_OBJECT_TOWN)
                            || thisCellRec->m_triggerType == (MAP_TRIGGER_EVENT | MAP_OBJECT_HERO)
                            || (thisCellRec->m_triggerType == (MAP_TRIGGER_EVENT | MAP_OBJECT_SHIP)
                                && !(pHero->m_eventFlags & HERO_EVENT_EMBARKED));
                } else {
                    validFlag = (thisCellRec->m_triggerType & MAP_TRIGGER_EVENT)
                                || (thisCellRec->m_triggerType == MAP_OBJECT_COAST
                                    && (pHero->m_eventFlags & HERO_EVENT_EMBARKED))
                                || (oldX % curSpacing == 0 && y % curSpacing == 0
                                    && (((pHero->m_eventFlags & HERO_EVENT_EMBARKED)
                                         && CELL_TERRAIN(thisCellRec) == TERRAIN_WATER)
                                        || (!(pHero->m_eventFlags & HERO_EVENT_EMBARKED)
                                            && CELL_TERRAIN(thisCellRec) != TERRAIN_WATER)))
                                || (oldX == gpCurPlayer->m_ultimateArtifactHintX
                                    && y == gpCurPlayer->m_ultimateArtifactHintY);
                }
                if (validFlag) {
                    for (entry = 0; entry < gpCurPlayer->m_heroCount; entry++) {
                        if (thisCellRec->m_triggerType != (MAP_TRIGGER_EVENT | MAP_OBJECT_TOWN)
                            && thisCellRec->m_triggerType != (MAP_TRIGGER_EVENT | MAP_OBJECT_HERO)
                            && gpCurPlayer->m_heroIds[entry] != pHero->m_id
                            && gpGame->m_heroRecs[gpCurPlayer->m_heroIds[entry]].m_destinationX
                                   == oldX
                            && gpGame->m_heroRecs[gpCurPlayer->m_heroIds[entry]].m_destinationY
                                   == y) {
                            mainValue = -2000;
                            goto scored;
                        }
                    }
                    CheckDoMain(0, 0);
                    mainValue = RVOfPosition(
                        pHero,
                        oldX,
                        y,
                        gpSearchArray->m_cells[oldX][y].rvFlag1,
                        gpSearchArray->m_cells[oldX][y].valueX,
                        gpSearchArray->m_cells[oldX][y].valueY,
                        gpSearchArray->m_cells[oldX][y].rvFlag2,
                        gpSearchArray->m_cells[oldX][y].previousX,
                        gpSearchArray->m_cells[oldX][y].previousY,
                        2
                    );
                    mainValue = mainValue * (Random(1, 50) + 75);
                    mainValue /= 100;
                } else {
                    mainValue = -100;
                }
                if (oldX == targetX && y == targetY) {
                    mainValue = static_cast<i32>(mainValue * AI_TARGET_HUMAN_VALUE_FACTOR);
                    if (MANHATTAN_LENGTH(oldX - pHero->m_x, y - pHero->m_y) > 3)
                        mainValue++;
                }
            scored:
                if (mainValue > theBestRV) {
                    bestX = oldX;
                    bestY = y;
                    theBestRV = mainValue;
                } else if (mainValue == theBestRV && mainValue == 0) {
                    if (MANHATTAN_LENGTH(oldX - pHero->m_x, y - pHero->m_y)
                        > MANHATTAN_LENGTH(bestX - pHero->m_x, bestY - pHero->m_y)) {
                        bestX = oldX;
                        bestY = y;
                    }
                }
            }
        }
    }
    if (theBestRV < 75 && (gbPossibleShipyardFound || gbActualShipyardFound) && !gbActualBoatFound
        && giCurTurn > 3) {
        if ((gbActualShipyardFound || giBuildShipyard[giCurPlayer] < 0
             || giBuildShipyard[giCurPlayer] == giBestShipyardId)
            && gpCurPlayer->m_resources[RESOURCE_WOOD]
                       + gpCurPlayer->m_aiData.m_income[RESOURCE_WOOD] * 6
                   >= (!gbActualShipyardFound ? 20 : 0) + 10) {
            if (!gbActualShipyardFound)
                giBuildShipyard[giCurPlayer] = giBestShipyardId;
            giBuildBoat[giCurPlayer] = giBestShipyardId;
            giBuildBoatStuffTurn[giCurPlayer] = giCurTurn;
            portTown = gpGame->GetTown(giBestShipyardId);
            theBestRV = 123;
            bestX = portTown->m_x;
            bestY = portTown->m_y;
            if (pHero->m_x == bestX && pHero->m_y == bestY)
                pHero->m_remainingMobility = 0;
        }
        CheckBuyStuff();
    }
    targetX = bestX;
    targetY = bestY;
}

// donor PoL RVA 0x0003c6e2; preferred Buka symbol ?ProbableOutcomeOfBattle@philAI@@QAEXPAVarmyGroup@@PAVhero@@010HHHAAMAAH3333@Z
// donor Buka TU SOURCE/PHILAI; HoMM1 owner inferred from contiguous order
// evidence: graph:4;base=0.549946;margin=0.581641;shape=0.449;size=0.953;calls=0.724;alternate=pol20:void philAI::ProbableOutcomeOfBattle(class armyGroup *, class hero *, class armyGroup *, class hero *, class armyGroup *, int, int, int, float &, int &, int &, int &, int &, int &)@0x0003c6e2
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
    i32& attackerRemaining,
    i32& defenderRemaining,
    i32& outcomeValue
) {
    float attArmy;
    i32 slotNum;
    float defendingArmyValue;
    i32 artsD;
    i32 notUsed;
    i32 theExp;
    float attackStrengthValue;
    float defP;
    float curDefStr;
    float rawFightArray[2];
    float attackerPower;
    float power;
    float curFactorValue;
    i32 attArts;

    attArts = 0;
    artsD = 0;
    attArmy = static_cast<float>(FightValueOfStack(attacker, attackerHero, 1, 0, 0));
    defendingArmyValue =
        static_cast<float>(FightValueOfStack(defender, defenderHero, 1, useTown, townId));
    if (townArmy)
        defendingArmyValue += static_cast<float>(FightValueOfStack(townArmy, NULL, 1, 0, 0));
    rawFightArray[0] = static_cast<float>(FightValueOfStack(attacker, attackerHero, 0, 0, 0));
    rawFightArray[1] = static_cast<float>(FightValueOfStack(defender, defenderHero, 0, 0, 0));
    if (townArmy)
        rawFightArray[1] += static_cast<float>(FightValueOfStack(townArmy, NULL, 0, 0, 0));
    if (useTown)
        defendingArmyValue = defendingArmyValue * 1.11;
    curDefStr = defendingArmyValue;
    if (enemyPlayer == GAME_PLAYER_NONE) {
        attackStrengthValue = attArmy * (gpCurPlayer->m_difficulty * 0.15 + 0.7);
    } else {
        attackStrengthValue = attArmy;
        if (gbHumanPlayer[enemyPlayer]) {
            curDefStr = curDefStr * 1.14;
            if (gpCurPlayer->m_difficulty == 1)
                attackStrengthValue = attackStrengthValue * 1.5;
        }
    }
    if (attackStrengthValue < 1.0f)
        attackStrengthValue = 1.0f;
    if (curDefStr < 1.0f)
        curDefStr = 1.0f;
    power = 2.75f;
    if (attackStrengthValue > 1000000.0f || curDefStr > 1000000.0f)
        power = 2.0f;
    attackerPower = pow(attackStrengthValue, power);
    defP = pow(curDefStr, power);
    winChance = attackerPower / (attackerPower + defP);
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
    attackerLoss = static_cast<i32>((1.0 - winChance) * rawFightArray[0]);
    defenderLoss = static_cast<i32>(rawFightArray[1] * winChance);
    attackerRemaining =
        static_cast<i32>(attackerLoss * winChance + (1.0f - winChance) * rawFightArray[0]);
    defenderRemaining =
        static_cast<i32>(defenderLoss * (1.0f - winChance) + rawFightArray[1] * winChance);
    curFactorValue = 1.33 - gpCurPlayer->m_aiData.m_attentionWeights.upgradeBase;
    outcomeValue = static_cast<i32>(-attackerRemaining * curFactorValue * curFactorValue);
    if (enemyPlayer >= 0) {
        curFactorValue = gpCurPlayer->m_aiData.m_attentionWeights.upgradeBase + 0.66;
        if (gbHumanPlayer[enemyPlayer])
            outcomeValue = static_cast<i32>(
                outcomeValue
                + defenderRemaining * gAttackHumanBonus * curFactorValue * curFactorValue
            );
        else
            outcomeValue = static_cast<i32>(
                outcomeValue
                + defenderRemaining * gAttackComputerBonus * curFactorValue * curFactorValue
            );
    }
    outcomeValue = static_cast<i32>(outcomeValue * gpCurPlayer->m_aiData.m_upgradeValueWeight);
    if (attackerHero) {
        for (slotNum = 0; slotNum < HERO_ARTIFACT_SLOT_COUNT; slotNum++) {
            if (attackerHero->m_artifacts[slotNum] >= 0
                && attackerHero->m_artifacts[slotNum] < ARTIFACT_REGULAR_END)
                attArts += gArtifactBaseRV[attackerHero->m_artifacts[slotNum]];
        }
        outcomeValue = static_cast<i32>(outcomeValue - (attArts + 1400) * (1.0f - winChance));
        theExp = gpGame->ExperienceValueOfStack(defender, defenderHero);
        outcomeValue = static_cast<i32>(
            outcomeValue + theExp * 0.8 * winChance * attackerHero->m_aiFightValue
        );
    }
    if (defenderHero) {
        for (slotNum = 0; slotNum < HERO_ARTIFACT_SLOT_COUNT; slotNum++) {
            if (defenderHero->m_artifacts[slotNum] >= 0
                && defenderHero->m_artifacts[slotNum] < ARTIFACT_REGULAR_END)
                artsD += gArtifactBaseRV[defenderHero->m_artifacts[slotNum]];
        }
        outcomeValue = static_cast<i32>(
            outcomeValue
            + (artsD + 1250)
                  * (gbHumanPlayer[defenderHero->m_owner] ? gAttackHumanBonus
                                                          : gAttackComputerBonus)
                  * winChance
        );
    }
}

// Buka 2.1 GetOddsOfWinning returns the exact constant seen in retail's fld.
// @dead-code
// Zero-ref: pinned retail has no incoming direct call/jump or relocated reference.
VA(0x0044abfa, 0x13)
float philAI::GetOddsOfWinning(i32) {
    return 1.0f;
}

// Buka 2.1 ValueOfBuyingBuilding without the HoMM2 special buildings: the
// base value, scaled per slot by attention weights and dwelling counts, the
// enemy threat and the purchase deflator.
VA(0x0044ac0d, 0x507)
void philAI::ValueOfBuyingBuilding(
    town* townPointer,
    i32 building,
    i32& resourceValue,
    float& benefitCost
) {
    i32 buildingCost[RESOURCE_COUNT];
    i32 theAttackWeek;
    i32 dwellingsOwned;
    i32 num;
    i32 nextHighestDwellingId;
    i32 idx;
    float dangerRating;
    i32 creatureLocatedOk;
    i32 jj;
    i32 selTurns;
    i32 theSlots;
    float totalEnemyStrength;
    i32 currentCreatureTypeNum;
    float attackOddsVal;
    i16 factionIdIndex;
    float nextBenefit;
    i32 selLevel;

    factionIdIndex = townPointer->m_type;
    dwellingsOwned = 0;
    nextHighestDwellingId = -1;
    for (jj = 0; jj < BUILDING_SLOT_DWELLING_COUNT; jj++) {
        if (townPointer->m_buildings & (1 << (jj + BUILDING_SLOT_DWELLING_FIRST))) {
            dwellingsOwned++;
            nextHighestDwellingId = jj;
        }
    }
    theSlots = 0;
    for (jj = 0; jj < ARMY_GROUP_SLOT_COUNT; jj++)
        if (townPointer->m_army.m_creatureCounts[jj] > 0)
            theSlots++;
    nextBenefit = static_cast<float>(GetBuildingBaseResourceValue(
        factionIdIndex,
        building,
        building == BUILDING_SLOT_MAGE_GUILD ? townPointer->m_buildState : 0
    ));
    if (building == BUILDING_SLOT_MAGE_GUILD && townPointer->m_buildState > 0)
        nextBenefit -= static_cast<float>(
            GetBuildingBaseResourceValue(factionIdIndex, building, townPointer->m_buildState - 1)
        );
    switch (building) {
        case BUILDING_SLOT_CASTLE:
            nextBenefit = nextBenefit
                          * (gpCurPlayer->m_aiData.m_attentionWeights.buildingValue * 2.0f + 0.33);
            selLevel = dwellingsOwned;
            nextBenefit = nextBenefit * (1.6 - selLevel * 0.2);
            break;
        case BUILDING_SLOT_MAGE_GUILD:
            nextBenefit = nextBenefit
                          * (gpCurPlayer->m_aiData.m_attentionWeights.buildingValue * 2.0f + 0.33);
            nextBenefit =
                nextBenefit * (1.33 - gpCurPlayer->BuildingsOwned(factionIdIndex, 0, 0) * 0.33);
            break;
        case BUILDING_SLOT_THIEVES_GUILD:
            break;
        case BUILDING_SLOT_SHIPYARD:
            nextBenefit = 0;
            break;
        case BUILDING_SLOT_WELL:
            nextBenefit =
                nextBenefit * (gpCurPlayer->m_aiData.m_attentionWeights.buildingValue + 0.66);
            nextBenefit =
                nextBenefit * (gpCurPlayer->m_aiData.m_attentionWeights.upgradeBase * 2.0f + 0.33);
            nextBenefit = nextBenefit * (dwellingsOwned * 0.33 + 0.66);
            break;
        case BUILDING_SLOT_TAVERN:
            nextBenefit =
                FightValueOfStack(&townPointer->m_army, NULL, 0, 0, 0) / 3000.0f * nextBenefit;
            break;
        case BUILDING_SLOT_DWELLING_1:
        case BUILDING_SLOT_DWELLING_2:
        case BUILDING_SLOT_DWELLING_3:
        case BUILDING_SLOT_DWELLING_4:
        case BUILDING_SLOT_DWELLING_5:
        case BUILDING_SLOT_DWELLING_6:
            if (theSlots == ARMY_GROUP_SLOT_COUNT) {
                creatureLocatedOk = 0;
                for (jj = 0; jj < ARMY_GROUP_SLOT_COUNT; jj++)
                    if (townPointer->m_army.m_creatureTypes[jj]
                        == gDwellingType[townPointer->m_type]
                                        [building - BUILDING_SLOT_DWELLING_FIRST])
                        creatureLocatedOk = 1;
                if (!creatureLocatedOk)
                    break;
            }
            nextBenefit =
                nextBenefit * (gpCurPlayer->m_aiData.m_attentionWeights.buildingValue + 0.66);
            nextBenefit =
                nextBenefit * (gpCurPlayer->m_aiData.m_attentionWeights.upgradeBase * 2.0f + 0.33);
            nextBenefit = nextBenefit
                          * (1.0 - gpCurPlayer->BuildingsOwned(factionIdIndex, building, 0) * 0.05);
            if (building - BUILDING_SLOT_DWELLING_FIRST < nextHighestDwellingId)
                nextBenefit = nextBenefit * (1.66 - dwellingsOwned * 0.33);
            if (townPointer->m_buildings & (1 << BUILDING_SLOT_WELL))
                nextBenefit = nextBenefit * 1.1;
            for (idx = 0; idx < BUILDING_SLOT_DWELLING_COUNT; idx++) {
                currentCreatureTypeNum = gDwellingType[townPointer->m_type][idx];
                if ((townPointer->m_buildings & (1 << (idx + BUILDING_SLOT_DWELLING_FIRST)))
                    && townPointer->m_garrison[idx] > 0
                    && gMonsterDatabase[currentCreatureTypeNum].iconIndex * 1.2
                           > gMonsterDatabase[gDwellingType[townPointer->m_type]
                                                           [building
                                                            - BUILDING_SLOT_DWELLING_FIRST]]
                                 .iconIndex) {
                    nextBenefit = 0;
                    break;
                }
            }
            break;
    }
    LikelihoodOfEnemyAttacking(
        townPointer,
        NULL,
        attackOddsVal,
        totalEnemyStrength,
        selTurns,
        num,
        theAttackWeek,
        dangerRating
    );
    nextBenefit = nextBenefit * (1.0 - dangerRating * 3.0);
    if (nextBenefit < 0.0f)
        nextBenefit = 0;
    GetBuildingCost(
        factionIdIndex,
        building,
        buildingCost,
        building == BUILDING_SLOT_MAGE_GUILD ? townPointer->m_buildState : 0
    );
    nextBenefit = nextBenefit * FutureDeflator(buildingCost);
    resourceValue = static_cast<i32>(nextBenefit);
    benefitCost = nextBenefit / RVConversion(buildingCost);
}

// donor PoL RVA 0x0003d6b7; preferred Buka symbol ?GetBestBuilding@philAI@@QAEXPAVtown@@AAUBHC@@AAM@Z
// donor Buka TU SOURCE/PHILAI; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.526596;margin=0.075083;shape=0.323;size=0.949;calls=1.000;alternate=pol20:void philAI::GetBestBuilding(class town *, struct BHC &, float &)@0x0003d6b7
VA(0x0044b114, 0x101)
void philAI::GetBestBuilding(town* townPointer, BHC& purchase, float& benefitCost) {
    float quantity;
    i32 bestBuilding;
    float bestCost;
    i32 curBuilding;
    i32 curRv;
    float points;
    float grade;

    bestCost = -99.0f;
    points = -99.0f;
    bestBuilding = -1;
    for (curBuilding = BUILDING_SLOT_MAGE_GUILD; curBuilding <= BUILDING_SLOT_DWELLING_LAST;
         curBuilding++) {
        if (!(townPointer->m_buildings & (1 << curBuilding))
            || (curBuilding == BUILDING_SLOT_MAGE_GUILD
                && townPointer->m_buildState < TOWN_MAGE_GUILD_COST_LEVEL_LAST)) {
            if (CanBuild(townPointer, curBuilding)) {
                ValueOfBuyingBuilding(townPointer, curBuilding, curRv, quantity);
                grade = (Random(1, 5) + 95) * quantity / 100.0f;
                if (grade > points) {
                    bestBuilding = curBuilding;
                    bestCost = quantity;
                    points = grade;
                }
            }
        }
    }
    purchase.pTown = townPointer;
    purchase.type = PURCHASE_BUILDING;
    purchase.what = bestBuilding;
    benefitCost = bestCost;
}

// donor PoL RVA 0x0003d852; preferred Buka symbol ?ValueOfBuyingCreature@philAI@@QAEXPAVtown@@HAAHHAAM@Z
// donor Buka TU SOURCE/PHILAI; HoMM1 owner inferred from contiguous order
// evidence: graph:3;base=0.431663;margin=0.517459;shape=0.208;size=0.811;calls=0.923;alternate=pol20:void philAI::ValueOfBuyingCreature(class town *, int, int &, int, float &)@0x0003d852
VA(0x0044b215, 0x268)
void philAI::ValueOfBuyingCreature(
    town* townPointer,
    i32 creature,
    i32& resourceValue,
    i32 purchaseCount,
    float& benefitCost
) {
    i32 nWeeks;
    i32 pointsValue;
    float peril;
    i32 baseCost[RESOURCE_COUNT];
    // Counts breath-attack stacks; retail allocation follows this local name
    // (renaming it moves registers).
    i32 archers;
    i32 creatRVVal;
    i32 rvCost;
    float attackChanceValue;
    float prevStrength;
    float firstFactor;
    i32 savedTurns;
    i32 n;
    hero* occupant;
    i32 activeSlotNum;

    archers = 0;
    GetMonsterCost(creature, baseCost);
    rvCost = purchaseCount * RVConversion(baseCost);
    creatRVVal = static_cast<i32>(
        purchaseCount * gMonsterDatabase[creature].fightValue
        * gpCurPlayer->m_aiData.m_upgradeValueWeight
    );
    if (townPointer->m_occupyingHeroId != TOWN_OCCUPYING_HERO_NONE) {
        occupant = gpGame->GetHero(townPointer->m_occupyingHeroId);
        creatRVVal = static_cast<i32>(creatRVVal * 1.1);
        if (creature / CREATURE_FACTION_SIZE == occupant->m_heroClass)
            creatRVVal = static_cast<i32>(creatRVVal * AI_CREATURE_SAME_RACE_FACTOR);
        if ((gMonsterDatabase[creature].stats.attributes & MONSTER_FLAGS_BREATH_ATTACK)) {
            for (n = 0; n < ARMY_GROUP_SLOT_COUNT; n++) {
                if (occupant->m_army.m_creatureTypes[n] != CREATURE_NONE
                    && (gMonsterDatabase[occupant->m_army.m_creatureTypes[n]].stats.attributes
                        & MONSTER_FLAGS_BREATH_ATTACK))
                    archers++;
            }
            creatRVVal = static_cast<i32>(creatRVVal * (1.18 - archers * 0.06));
        }
        creatRVVal = static_cast<i32>(
            creatRVVal
            * (gpGame->m_players[townPointer->m_owner].m_aiData.m_attentionWeights.upgradeBase
               + 0.66)
        );
    }
    if ((gMonsterDatabase[creature].stats.attributes & MONSTER_FLAGS_BREATH_ATTACK)) {
        for (activeSlotNum = 0; activeSlotNum < ARMY_GROUP_SLOT_COUNT; activeSlotNum++) {
            if (townPointer->m_army.m_creatureTypes[activeSlotNum] != CREATURE_NONE
                && (gMonsterDatabase[townPointer->m_army.m_creatureTypes[activeSlotNum]]
                        .stats.attributes
                    & MONSTER_FLAGS_BREATH_ATTACK))
                archers++;
        }
        creatRVVal = static_cast<i32>(creatRVVal * (1.18 - archers * 0.06));
    }
    LikelihoodOfEnemyAttacking(
        townPointer,
        NULL,
        attackChanceValue,
        prevStrength,
        savedTurns,
        pointsValue,
        nWeeks,
        peril
    );
    firstFactor = peril + 0.96;
    creatRVVal = static_cast<i32>(creatRVVal * (firstFactor * firstFactor * firstFactor));
    creatRVVal = static_cast<i32>(creatRVVal * FutureDeflator(baseCost));
    resourceValue = creatRVVal;
    benefitCost = static_cast<float>(resourceValue) / (static_cast<float>(rvCost));
}

// donor PoL RVA 0x0003db58; preferred Buka symbol ?GetBestCreature@philAI@@QAEXPAVtown@@AAUBHC@@AAM@Z
// donor Buka TU SOURCE/PHILAI; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.467416;margin=0.566378;shape=0.359;size=0.696;calls=1.000;alternate=pol20:void philAI::GetBestCreature(class town *, struct BHC &, float &)@0x0003db58
VA(0x0044b47d, 0x188)
void philAI::GetBestCreature(town* townPointer, BHC& best, float& bestValue) {
    float bestCostVal;
    float rand;
    float prevWorth;
    float points;
    i32 bestBuyValue;
    i32 baseDwelling;
    i32 curDwelling;
    i32 curMon;
    i32 canAddSet;
    i32 iArmy;
    i32 curRv;
    i32 numUnits;

    baseDwelling = -1;
    bestBuyValue = 0;
    bestCostVal = -99.0f;
    points = -99.0f;
    for (curDwelling = 0; curDwelling < BUILDING_SLOT_DWELLING_COUNT; curDwelling++) {
        curMon = gDwellingType[townPointer->m_type][curDwelling];
        if ((townPointer->m_buildings & (1 << (curDwelling + BUILDING_SLOT_DWELLING_FIRST)))
            && townPointer->m_garrison[curDwelling] > 0) {
            canAddSet = 0;
            for (iArmy = 0; iArmy < ARMY_GROUP_SLOT_COUNT; iArmy++) {
                if (townPointer->m_army.m_creatureTypes[iArmy] == CREATURE_NONE
                    || townPointer->m_army.m_creatureTypes[iArmy] == curMon)
                    canAddSet = 1;
            }
            if (canAddSet) {
                numUnits = CreaturesToBuy(townPointer, curDwelling);
                if (numUnits > 0) {
                    ValueOfBuyingCreature(townPointer, curMon, curRv, numUnits, prevWorth);
                    rand = (Random(1, 10) + 90) * prevWorth / 100.0;
                    if (rand > points) {
                        baseDwelling = curDwelling;
                        bestCostVal = prevWorth;
                        points = rand;
                        bestBuyValue = numUnits;
                    }
                }
            }
        }
    }
    best.pTown = townPointer;
    best.type = PURCHASE_CREATURE;
    best.what = baseDwelling;
    best.num = bestBuyValue;
    bestValue = bestCostVal;
}

// Buka's town overload indexes the six dwelling stocks and faction table.
VA(0x0044b605, 0x3f)
i32 philAI::CreaturesToBuy(town* townPointer, i32 level) {
    i32 nGarrison = townPointer->m_garrison[level];
    return CreaturesToBuy(gDwellingType[townPointer->m_type][level], nGarrison);
}

// Buka 2.1 purchase count logic and retail's ordered call/branches agree.
VA(0x0044b644, 0x47)
i32 philAI::CreaturesToBuy(i32 creatureType, i32 availableCount) {
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

// donor PoL RVA 0x0003df5a; preferred Buka symbol ?MaxBuyableCreatures@philAI@@QAEHH@Z
// donor Buka TU SOURCE/PHILAI; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.533802;margin=0.565073;shape=0.392;size=0.839;calls=1.000;alternate=pol20:int philAI::MaxBuyableCreatures(int)@0x0003df5a
// Buka 2.1 body: the last resource's affordable count wins.
VA(0x0044b68b, 0x82)
i32 philAI::MaxBuyableCreatures(i32 creatureType) {
    i32 resourceIndex;
    i32 i;
    i32 cost[RESOURCE_COUNT];

    GetMonsterCost(creatureType, cost);
    for (i = 0; i < RESOURCE_COUNT; i++) {
        if (cost[i] == 0)
            resourceIndex = 9999;
        else if (gpCurPlayer->m_resources[i] > 0)
            resourceIndex = gpCurPlayer->m_resources[i] / cost[i];
        else
            resourceIndex = 0;
    }
    return resourceIndex;
}

// donor PoL RVA 0x0003dff6; preferred Buka symbol ?ValueOfBuyingHero@philAI@@QAEXPAVtown@@PAVhero@@AAHAAM@Z
// donor Buka TU SOURCE/PHILAI; HoMM1 owner inferred from contiguous order
// evidence: graph:3;base=0.452634;margin=0.608146;shape=0.221;size=0.851;calls=1.000;alternate=pol20:void philAI::ValueOfBuyingHero(class town *, class hero *, int &, float &)@0x0003dff6
VA(0x0044b70d, 0x184)
void philAI::ValueOfBuyingHero(
    town* townPointer,
    hero* heroPointer,
    i32& resourceValue,
    float& benefitCost
) {
    i32 tmpNum;
    i32 i;
    i32 heroRV;
    i32 heroCost[RESOURCE_COUNT];
    i32 costRVVal;

    heroCost[RESOURCE_WOOD] = 0;
    heroCost[RESOURCE_MERCURY] = 0;
    heroCost[RESOURCE_ORE] = 0;
    heroCost[RESOURCE_SULFUR] = 0;
    heroCost[RESOURCE_CRYSTAL] = 0;
    heroCost[RESOURCE_GEMS] = 0;
    heroCost[RESOURCE_GOLD] = 2500;
    costRVVal = RVConversion(heroCost);
    heroRV = heroPointer->m_experience + 2000;
    for (i = 0; i < HERO_ARTIFACT_SLOT_COUNT; i++) {
        if (heroPointer->m_artifacts[i] >= 0 && heroPointer->m_artifacts[i] < ARTIFACT_REGULAR_END)
            heroRV += gArtifactBaseRV[heroPointer->m_artifacts[i]];
    }
    heroRV += heroPointer->m_experience / 2;
    heroRV = static_cast<i32>(
        heroRV
        * (gpCurPlayer->m_aiData.m_attentionWeights.heroValue + 1.0
           - gpCurPlayer->m_aiData.m_attentionWeights.upgradeBase)
    );
    if (gTownHeroClass[townPointer->m_type] == heroPointer->m_heroClass)
        heroRV = static_cast<i32>(heroRV * AI_HERO_PURCHASE_SAME_RACE_FACTOR);
    heroRV += StrategicValueOfPosition(heroPointer, heroPointer->m_x, heroPointer->m_y, 0, &tmpNum);
    heroRV -= 200;
    heroRV = static_cast<i32>(heroRV * FutureDeflator(heroCost));
    benefitCost = static_cast<float>(heroRV) / costRVVal;
    resourceValue = heroRV;
}

// ValueOfEventAtPosition module state (.bss order follows names, not position).
DATA(0x004bb124)
i32 gAttackerRemaining;
DATA(0x004bb128)
i32 gDefenderRemaining;
DATA(0x004bb12c)
i32 gOutcome;
DATA(0x004bb130)
i32 gArtifactChoice1;

// donor PoL RVA 0x0003e2a8; preferred Buka symbol ?GetBestHero@philAI@@QAEXPAVtown@@AAUBHC@@AAM@Z
// donor Buka TU SOURCE/PHILAI; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.649528;margin=0.194728;shape=0.311;size=0.925;calls=0.800;strings=Town:%2d  Hero    : % 15i   Raw BC = %8.2f,  RandBC = %8.2f.;alternate=pol20:void philAI::GetBestHero(class town *, struct BHC &, float &)@0x0003e2a8
VA(0x0044b891, 0x12c)
void philAI::GetBestHero(town* townPointer, BHC& best, float& bestValue) {
    i32 bestHero;
    float prevWorth;
    i32 curHero;
    hero* availHero;
    float adjusted;
    float points;
    float bestCostVal;
    i32 activeCostVal;

    bestHero = -1;
    bestCostVal = -99.0f;
    points = -99.0f;
    for (curHero = 0; curHero < HERO_AVAILABLE_SLOT_COUNT; curHero++) {
        availHero = &gpGame->m_heroRecs[gpCurPlayer->m_availableHeroIds[curHero]];
        ValueOfBuyingHero(townPointer, availHero, activeCostVal, prevWorth);
        adjusted = prevWorth * (Random(1, 10) + 90.0) / 100.0;
        if (adjusted > points) {
            bestHero = curHero;
            bestCostVal = prevWorth;
            points = adjusted;
        }
    }
    best.pTown = townPointer;
    best.type = PURCHASE_HERO;
    best.what = bestHero;
    bestValue = bestCostVal;
    if (gpGame->m_map[townPointer->m_x][townPointer->m_y].m_triggerType
        == (MAP_TRIGGER_EVENT | MAP_OBJECT_HERO))
        bestValue -= 200.0f;
}

// donor PoL RVA 0x0003e459; preferred Buka symbol ?LikelihoodOfEnemyAttacking@philAI@@QAEXPAVtown@@PAVhero@@AAM2AAH332@Z
// donor Buka TU SOURCE/PHILAI; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.488198;margin=0.360468;shape=0.323;size=0.832;calls=1.000;alternate=pol20:void philAI::LikelihoodOfEnemyAttacking(class town *, class hero *, float &, float &, int &, int &, int &, float &)@0x0003e459
VA(0x0044b9bd, 0x54)
void philAI::LikelihoodOfEnemyAttacking(
    town*,
    hero*,
    float& chanceA,
    float& chanceB,
    i32& nAttack,
    i32& nValue,
    i32& nWeeks,
    float& fOut
) {
    chanceA = 0.15f;
    chanceB = 0.6f;
    nAttack = 3000;
    nValue = static_cast<i32>(static_cast<float>(nAttack) * chanceA);
    nWeeks = 6;
    fOut = chanceA * chanceB;
}

// This zero result is the Buka 2.1 body and the pinned retail instruction.
VA(0x0044ba11, 0xf)
i32 philAI::MeanRVOfUnexploredTerritory(i32) {
    return 0;
}

// Buka 2.1 GetGameAttentionValue: randomized game weights tempered by the
// number of players.
VA(0x0044ba20, 0x14d)
void philAI::GetGameAttentionValue(i32 player) {
    playerAttentionWeights* attention = &gpGame->m_players[player].m_aiData.m_attentionWeights;
    attention->gameWeightA = static_cast<float>(Random(0, 100) / 500.0) + 0.23;
    attention->gameWeightB = static_cast<float>(Random(0, 100) / 500.0) + 0.23;
    attention->gameWeightB *= (AI_ATTENTION_IDENTITY_FLOAT + 3.0) / 4.0;
    attention->gameWeightB *= (5.0 - AI_ATTENTION_IDENTITY) / 4.0;
    attention->gameWeightA *= (AI_ATTENTION_IDENTITY + 3.0) / 4.0;
    attention->gameWeightB = attention->gameWeightB * ((3.0 - gpGame->m_playerCount) * 0.15 + 1.0);
    attention->gameWeightA = attention->gameWeightA * ((3.0 - gpGame->m_playerCount) * 0.07 + 1.0);
    attention->gameRemainder = ((1.0f - attention->gameWeightB) - attention->gameWeightA);
}

// Buka 2.1 GetTurnAttentionValue: reset the game weights and scale the hero
// weight down as the game ages.
VA(0x0044bb6d, 0xc6)
void philAI::GetTurnAttentionValue(i32 player) {
    playerAttentionWeights* attentionWeights =
        &gpGame->m_players[player].m_aiData.m_attentionWeights;
    attentionWeights->gameWeightA = 0.4f;
    attentionWeights->gameWeightB = 0.3f;
    attentionWeights->gameRemainder = 0.3f;
    attentionWeights->buildingValue = attentionWeights->gameWeightA;
    attentionWeights->heroValue = attentionWeights->gameWeightB;
    attentionWeights->upgradeBase = attentionWeights->gameRemainder;
    float scale;
    if (giCurTurn < 5)
        scale = 1.6f;
    else if (giCurTurn < 10)
        scale = 1.4f;
    else if (giCurTurn < 20)
        scale = 1.2f;
    else if (giCurTurn < 30)
        scale = 1.0f;
    else
        scale = 0.8f;
    attentionWeights->heroValue = attentionWeights->heroValue * scale;
}

// donor PoL RVA 0x0003e7a2; preferred Buka symbol ?RVConversion@philAI@@QAEHQAH@Z
// donor Buka TU SOURCE/PHILAI; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.411394;margin=0.429857;shape=0.250;size=0.681;calls=1.000;alternate=pol20:int philAI::RVConversion(int * const)@0x0003e7a2
VA(0x0044bc33, 0x71)
i32 philAI::RVConversion(i32* const resources) {
    return static_cast<i32>(
        static_cast<float>(resources[RESOURCE_GOLD]) * gafAITurnCostResource[RESOURCE_GOLD]
        + static_cast<float>(resources[RESOURCE_WOOD]) * gafAITurnCostResource[RESOURCE_WOOD]
        + static_cast<float>(resources[RESOURCE_ORE]) * gafAITurnCostResource[RESOURCE_ORE]
        + static_cast<float>(resources[RESOURCE_CRYSTAL]) * gafAITurnCostResource[RESOURCE_CRYSTAL]
        + static_cast<float>(resources[RESOURCE_SULFUR]) * gafAITurnCostResource[RESOURCE_SULFUR]
        + static_cast<float>(resources[RESOURCE_MERCURY]) * gafAITurnCostResource[RESOURCE_MERCURY]
        + static_cast<float>(resources[RESOURCE_GEMS]) * gafAITurnCostResource[RESOURCE_GEMS]
    );
}

// Buka 2.1 TurnsToBuy: the slowest shortfall in turns of income, 99 when a
// short resource has no income.
VA(0x0044bca4, 0xc7)
float philAI::TurnsToBuy(i32* const resources) {
    float maxT = 0;
    i32 resourceIndex;
    float fTurns;
    for (resourceIndex = 0; resourceIndex < RESOURCE_COUNT; resourceIndex++) {
        if (gpCurPlayer->m_resources[resourceIndex] < resources[resourceIndex]) {
            if (gpCurPlayer->m_aiData.m_income[resourceIndex] > 0)
                fTurns = static_cast<float>(
                    (resources[resourceIndex] - gpCurPlayer->m_resources[resourceIndex])
                        / gpCurPlayer->m_aiData.m_income[resourceIndex]
                    + 1
                );
            else
                fTurns = 99.0f;
            maxT = __max(fTurns, maxT);
        }
    }
    return maxT;
}

// donor PoL RVA 0x0003e918; preferred Buka symbol ?RVOfPosition@philAI@@QAEHHHHHHHHHHH@Z
// donor Buka TU SOURCE/PHILAI; HoMM1 owner inferred from contiguous order
// evidence: graph:3;base=0.513260;margin=0.500427;shape=0.306;size=0.971;calls=0.812;alternate=pol20:int philAI::RVOfPosition(int, int, int, int, int, int, int, int, int, int)@0x0003e918
VA(0x0044bd6b, 0x493)
i32 philAI::RVOfPosition(
    hero* pHero,
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
    i32 iMonsterChance;
    i32 totalValue;
    i32 oldChance;
    i32 oldDelta;
    i32 curLocType;
    i32 oldVal;
    float estTurnsVal;
    i32 newHeroLiveChance;
    i32 activeChance;
    i32 chanceVal;
    i32 adjacentEventChance;
    i32 savedValue;
    i32 num;
    i32 xPos;
    i32 newCurTriggerType;
    i32 posY;

    savedValue = 0;
    activeChance = 100;
    adjacentEventChance = 100;
    newCurTriggerType = gpAdvManager->GetCell(x, y)->m_triggerType;
    curLocType = newCurTriggerType & MAP_TRIGGER_TYPE_MASK;
    chanceVal = 100;
    oldChance = 100;
    iMonsterChance = 100;
    num = StrategicValueOfPosition(pHero, pHero->m_x, pHero->m_y, 0, &newHeroLiveChance);
    oldDelta = StrategicValueOfPosition(pHero, x, y, 0, &activeChance);
    if (curLocType == MAP_OBJECT_SHIP && oldDelta < 0)
        oldDelta = 0;
    totalValue = 0;
    if (hasEvent)
        totalValue += ValueOfEventAtPosition(pHero, eventX, eventY, 1, &oldChance);
    if (hasStrategicEvent) {
        savedValue =
            StrategicValueOfPosition(pHero, strategicX, strategicY, 1, &adjacentEventChance);
        if (savedValue < 0)
            totalValue += savedValue;
    }
    if (gpAdvManager->FindAdjacentMonster(
            x,
            y,
            &xPos,
            &posY,
            SEARCH_INVALID_COORDINATE,
            SEARCH_INVALID_COORDINATE
        )) {
        switch (curLocType) {
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
                oldVal = ValueOfEventAtPosition(pHero, xPos, posY, 1, &iMonsterChance);
                if (oldVal < 0)
                    totalValue += oldVal;
                if (oldChance == 100)
                    oldChance = iMonsterChance;
                else
                    oldChance = oldChance * iMonsterChance / 100;
                break;
        }
    }
    if ((newCurTriggerType & MAP_TRIGGER_EVENT)
        || (x == gpCurPlayer->m_ultimateArtifactHintX && y == gpCurPlayer->m_ultimateArtifactHintY))
        oldVal = ValueOfEventAtPosition(pHero, x, y, eventMode, &chanceVal);
    else
        oldVal = 0;
    if (chanceVal < 100)
        oldDelta = oldDelta * chanceVal / 100;
    if (activeChance < 100) {
        oldVal = oldVal * activeChance / 100;
        oldDelta = oldDelta * activeChance / 100;
    }
    if (adjacentEventChance < 100) {
        oldVal = oldVal * adjacentEventChance / 100;
        oldDelta = oldDelta * adjacentEventChance / 100;
    }
    if (oldChance < 100) {
        if (totalValue > 0)
            totalValue = (totalValue + oldVal + oldDelta) * oldChance / 100;
        else
            totalValue += (oldVal + oldDelta) * oldChance / 100;
    } else {
        totalValue += oldVal;
    }
    estTurnsVal = static_cast<float>(gpSearchArray->m_cells[x][y].distance) / pHero->m_mobility;
    if (pHero->m_eventFlags & HERO_EVENT_EMBARKED)
        estTurnsVal = estTurnsVal * 0.5 + 0.5;
    else if (estTurnsVal > 5.0f)
        estTurnsVal *= 3.0f;
    else if (estTurnsVal > 4.0f)
        estTurnsVal = estTurnsVal * 2.5;
    else if (estTurnsVal > 3.0f)
        estTurnsVal = estTurnsVal * 2.0;
    else if (estTurnsVal > 2.0f)
        estTurnsVal = estTurnsVal * 1.7;
    else if (estTurnsVal > 1.5)
        estTurnsVal = estTurnsVal * 1.4;
    else if (estTurnsVal > 1.0f)
        estTurnsVal = estTurnsVal * 1.2;
    totalValue = static_cast<i32>(totalValue / (estTurnsVal + 0.2));
    oldDelta = static_cast<i32>(oldDelta * 2 / (1.0f + estTurnsVal));
    if (oldChance == 100)
        totalValue += oldDelta;
    if ((pHero->m_eventFlags & HERO_EVENT_EMBARKED) && newCurTriggerType == MAP_OBJECT_COAST)
        totalValue += 40;
    return totalValue;
}

// Buka SVSearchArray: StrategicValueOfPosition's shared search, constructed
// by its dynamic initializer between RVOfPosition and its first user.
DATA(0x004bb160)
searchArray SVSearchArray;
RVA_DYNINIT(0x0004c208, 0xf, SVSearchArray)
// Its .CRT$XCU thunk (0x0048e008 -> 0x00427d90) opens this retail object:
// int3 padding precedes it and LogTruncate follows without a gap.
RVA_DYNINIT(0x0004c1fe, 0xa, SVSearchArray)

// donor PoL RVA 0x0003ef45; preferred Buka symbol ?StrategicValueOfPosition@philAI@@QAEHHHHHPAHH@Z
// donor Buka TU SOURCE/PHILAI; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.499321;margin=0.324582;shape=0.341;size=0.829;calls=0.957;alternate=pol20:int philAI::StrategicValueOfPosition(int, int, int, int, int *, int)@0x0003ef45
VA(0x0044c217, 0x83a)
i32 philAI::StrategicValueOfPosition(
    hero* pHero,
    i16 targetX,
    i16 targetY,
    i8 immediate,
    i32* liveChance
) {
    DATA(0x004ca198)
    static i8 gSVSearchArrayInUse = 0;
    i32 nGap;
    searchArray* searchData;
    i32 wasInBoat;
    i32 nextExtra2;
    mapCell* cell;
    i32 thisY;
    i32 newSeedRange;
    i32 curReach;
    i32 oldX;
    i32 prevHeroNo;
    i32 danger;
    i32 baseTerrainNum;
    i32 myValue;
    searchArray* theSearch;
    i32 curTerrain;

    if (!immediate && gaiHeroStrategicRVOfPos[targetX][targetY] != RV_UNSET) {
        *liveChance = gaiLiveChanceOfPos[targetX][targetY];
        return gaiHeroStrategicRVOfPos[targetX][targetY];
    }
    myValue = 0;
    theSearch = NULL;
    *liveChance = 100;
    if (gSVSearchArrayInUse) {
        theSearch = new searchArray;
        if (!theSearch)
            MemError();
        searchData = theSearch;
    } else {
        gSVSearchArrayInUse = 1;
        searchData = &SVSearchArray;
    }
    wasInBoat = pHero->m_eventFlags & HERO_EVENT_EMBARKED;
    if (wasInBoat && gpAdvManager->GetCell(targetX, targetY)->m_triggerType == MAP_OBJECT_COAST)
        wasInBoat = 0;
    if (immediate) {
        newSeedRange = 60;
    } else {
        newSeedRange = 36;
    }
    searchData->SeedPosition(
        targetX,
        targetY,
        MAP_DIRECTION_EAST,
        newSeedRange,
        wasInBoat,
        0,
        SEARCH_UNLIMITED_COST,
        pHero->m_heroClass,
        SEARCH_INVALID_COORDINATE,
        SEARCH_INVALID_COORDINATE,
        0,
        0
    );
    searchData->m_cells[targetX][targetY].visited = 0;
    for (oldX = 0; oldX < MAP_CELL_GRID_SIZE; oldX++) {
        for (thisY = 0; thisY < MAP_CELL_GRID_SIZE; thisY++) {
            if (searchData->m_cells[oldX][thisY].visited) {
                cell = gpAdvManager->GetCell(oldX, thisY);
                if ((!immediate && (cell->m_triggerType & MAP_TRIGGER_EVENT))
                    || (immediate
                        && cell->m_triggerType == (MAP_TRIGGER_EVENT | MAP_OBJECT_HERO))) {
                    CheckDoMain(0, 0);
                    myValue += ValueOfEventAtPosition(pHero, oldX, thisY, 0, &iDummy)
                               / (searchData->m_cells[oldX][thisY].distance + 2.0);
                }
                if (cell->m_triggerType == (MAP_TRIGGER_EVENT | MAP_OBJECT_HERO)) {
                    if (gaiHeroLiveChance[cell->m_objectMetadata] == RV_UNSET)
                        ValueOfEventAtPosition(pHero, oldX, thisY, 0, &iDummy);
                    if (gaiHeroLiveChance[cell->m_objectMetadata] != RV_UNSET
                        && gaiHeroLiveChance[cell->m_objectMetadata] < 100) {
                        curReach = gpGame->GetHero(cell->m_objectMetadata)->m_mobility;
                        if (gbHumanPlayer[gpGame->m_availableHeroes[cell->m_objectMetadata]]) {
                            if (searchData->m_cells[oldX][thisY].distance <= curReach) {
                                if (searchData->m_cells[oldX][thisY].distance <= 14)
                                    danger = 100 - gaiHeroLiveChance[cell->m_objectMetadata];
                                else
                                    danger = (100 - gaiHeroLiveChance[cell->m_objectMetadata])
                                             * (curReach - searchData->m_cells[oldX][thisY].distance
                                                + 10)
                                             / curReach;
                            } else {
                                danger = static_cast<i32>(
                                    (100 - gaiHeroLiveChance[cell->m_objectMetadata]) * 0.2
                                );
                            }
                        } else {
                            danger = (100 - gaiHeroLiveChance[cell->m_objectMetadata])
                                     * (curReach + 20 - searchData->m_cells[oldX][thisY].distance)
                                     / (curReach + 20);
                        }
                        *liveChance = *liveChance * (100 - danger) / 100;
                    }
                }
                if (searchData->m_cells[oldX][thisY].distance < 32
                    && gpAdvManager->GetCell(oldX, thisY)->m_triggerType
                           == (MAP_TRIGGER_EVENT | MAP_OBJECT_HERO)
                    && gpAdvManager->GetCell(oldX, thisY)->m_objectMetadata != pHero->m_id
                    && gpGame->m_availableHeroes[gpAdvManager->GetCell(oldX, thisY)
                                                     ->m_objectMetadata]
                           == pHero->m_owner)
                    myValue -= (32 - searchData->m_cells[oldX][thisY].distance) * 1250 >> 5;
            }
        }
    }
    baseTerrainNum = CELL_TERRAIN(gpAdvManager->GetCell(targetX, targetY));
    for (prevHeroNo = 0; prevHeroNo < gpCurPlayer->m_heroCount; prevHeroNo++) {
        if (gpCurPlayer->m_heroIds[prevHeroNo] != pHero->m_id) {
            nGap =
                abs(gpGame->m_heroRecs[gpCurPlayer->m_heroIds[prevHeroNo]].m_destinationX - targetX)
                + abs(
                    gpGame->m_heroRecs[gpCurPlayer->m_heroIds[prevHeroNo]].m_destinationY - targetY
                );
            if (nGap < 9) {
                curTerrain = giGroundToTerrain
                    [gpAdvManager
                         ->GetCell(
                             gpGame->m_heroRecs[gpCurPlayer->m_heroIds[prevHeroNo]].m_destinationX,
                             gpGame->m_heroRecs[gpCurPlayer->m_heroIds[prevHeroNo]].m_destinationY
                         )
                         ->m_tileIndex];
                if (!((baseTerrainNum == TERRAIN_WATER && curTerrain > TERRAIN_WATER_LAST)
                      || (baseTerrainNum > TERRAIN_WATER_LAST && curTerrain == TERRAIN_WATER)))
                    myValue -= (9 - nGap) * 1250 / 9;
            }
        }
    }
    if (theSearch)
        delete theSearch;
    else
        gSVSearchArrayInUse = 0;
    myValue = static_cast<i32>(myValue * AI_STRATEGIC_POSITION_SCORE_FACTOR);
    if (myValue > 32000)
        myValue = 32000;
    if (!immediate) {
        gaiHeroStrategicRVOfPos[targetX][targetY] = myValue;
        gaiLiveChanceOfPos[targetX][targetY] = *liveChance;
    }
    return myValue;
}

// ValueOfEventAtPosition module state (.bss order follows names, not position).
DATA(0x004bb134)
i32 gArtifactChoice2;

// Buka 2.1 ValueOfTown without the later scenario-town bonuses: built
// structures' base values plus a fixed gold-turn allowance.
VA(0x0044ca51, 0xb9)
i32 philAI::ValueOfTown(town* townPointer) {
    i32 sum = 0;
    i32 building;
    for (building = BUILDING_SLOT_MAGE_GUILD; building < BUILDING_SLOT_COUNT; building++) {
        if (townPointer->m_buildings & (1 << building))
            sum += GetBuildingBaseResourceValue(
                townPointer->m_type,
                building,
                __max(townPointer->m_buildState, 0)
            );
    }
    sum = static_cast<i32>(sum + 250.0f * gafAITurnCostResource[RESOURCE_GOLD] * 5.0f * 1.5);
    sum += 750;
    return sum;
}

// Buka 2.1 TurnCostResource: each resource's turn cost scales its base
// value against the player's relative stock-plus-income share.
VA(0x0044cb0a, 0x10b)
void philAI::TurnCostResource(i32 player) {
    playerAIData* playerAI;
    float ratio[RESOURCE_COUNT];
    float avg;
    i32 resourceIndex;
    i32 totalRV;
    i32 value[RESOURCE_COUNT];
    playerAI = &gpGame->m_players[player].m_aiData;
    totalRV = 0;
    for (resourceIndex = 0; resourceIndex < RESOURCE_COUNT; resourceIndex++) {
        value[resourceIndex] = static_cast<i32>(
            gResourceBaseValue[resourceIndex]
            * ((playerAI->m_income[resourceIndex] * 5) * 0.7
               + gpGame->m_players[player].m_resources[resourceIndex])
        );
        totalRV += value[resourceIndex];
    }
    avg = (totalRV / RESOURCE_COUNT);
    for (resourceIndex = 0; resourceIndex < RESOURCE_COUNT; resourceIndex++) {
        ratio[resourceIndex] = value[resourceIndex] / avg;
        gafAITurnCostResource[resourceIndex] =
            (gResourceBaseValue[resourceIndex] / (ratio[resourceIndex] / 2.0f + 0.5));
    }
}

// Buka 2.1 TurnValueOfObelisk without the later victory/explorer terms.
VA(0x0044cc15, 0xfb)
float philAI::TurnValueOfObelisk(i32 player) {
    playerAIData* ai;
    i32 each;
    ai = &gpGame->m_players[player].m_aiData;
    each = gArtifactBaseRV[gpGame->m_ultimateArtifactId] / 110;
    if (gpGame->m_ultimateArtifactId == ARTIFACT_NONE)
        return 0.0f;
    ai->m_obeliskValue = each * 48 / gpGame->m_obeliskCount;
    ai->m_obeliskValue = static_cast<i32>(
        ai->m_obeliskValue
        * (1.5 - abs(32 - gpGame->m_players[player].CountVisitedObelisks()) / 48.0f)
    );
    ai->m_obeliskValue =
        static_cast<i32>(ai->m_obeliskValue * (ai->m_attentionWeights.heroValue + 0.66));
    return ai->m_obeliskValue;
}

// donor PoL RVA 0x0003fe81; preferred Buka symbol ?FutureDeflator@philAI@@QAEMQAH@Z
// donor Buka TU SOURCE/PHILAI; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.484127;margin=0.409347;shape=0.286;size=0.877;calls=1.000;alternate=pol20:float philAI::FutureDeflator(int * const)@0x0003fe81
VA(0x0044cd10, 0x47)
float philAI::FutureDeflator(i32* const resources) {
    float turns = TurnsToBuy(resources);
    float value = 1.0f - turns * AI_FUTURE_DEFLATION_RATE;
    if (value < 0.0)
        value = 0;
    return value;
}

// donor PoL RVA 0x0003fed2; preferred Buka symbol ?FightValueOfStack@philAI@@QAEHPAVarmyGroup@@PAVhero@@HHHH@Z
// donor Buka TU SOURCE/PHILAI; HoMM1 owner inferred from contiguous order
// evidence: graph:4;base=0.393076;margin=0.447055;shape=0.297;size=0.768;calls=0.382;alternate=pol20:int philAI::FightValueOfStack(class armyGroup *, class hero *, int, int, int, int)@0x0003fed2
// HoMM1 retail returns with ret 0x14: five stack arguments.
VA(0x0044cd57, 0x638)
i32 philAI::FightValueOfStack(
    armyGroup* group,
    hero* heroPointer,
    i32 useHero,
    i8 useTown,
    i8 townId
) {
    i32 worthValue;
    float savedMod;
    i32 spellScore;
    i32 theValue;
    i32 baseSlot;
    i32 theArmyWorth;
    i32 curArrows;
    i32 newLuck;
    i32 castleValue;
    town* pTown;
    i32 nextStats;
    i32 morale;
    i32 curTotal;
    float quantityMod;

    theArmyWorth = 0;
    curTotal = 0;
    castleValue = 0;
    for (baseSlot = 0; baseSlot < ARMY_GROUP_SLOT_COUNT; baseSlot++) {
        if (group->m_creatureTypes[baseSlot] != CREATURE_NONE) {
            worthValue = group->m_creatureCounts[baseSlot]
                         * gMonsterDatabase[group->m_creatureTypes[baseSlot]].fightValue;
            if (useHero) {
                if (group->m_creatureCounts[baseSlot] > 180)
                    quantityMod = 1.7f;
                else if (group->m_creatureCounts[baseSlot] > 140)
                    quantityMod = 1.3f;
                else if (group->m_creatureCounts[baseSlot] > 100)
                    quantityMod = 1.1f;
                else if (group->m_creatureCounts[baseSlot] > 75)
                    quantityMod = 0.95f;
                else if (group->m_creatureCounts[baseSlot] > 50)
                    quantityMod = 0.81f;
                else if (group->m_creatureCounts[baseSlot] > 35)
                    quantityMod = 0.57f;
                else if (group->m_creatureCounts[baseSlot] > 23)
                    quantityMod = 0.37f;
                else if (group->m_creatureCounts[baseSlot] > 16)
                    quantityMod = 0.25f;
                else if (group->m_creatureCounts[baseSlot] > 11)
                    quantityMod = 0.13f;
                else if (group->m_creatureCounts[baseSlot] > 8)
                    quantityMod = 0.06f;
                else if (group->m_creatureCounts[baseSlot] > 5)
                    quantityMod = 0.0f;
                else if (group->m_creatureCounts[baseSlot] > 3)
                    quantityMod = -0.05f;
                else if (group->m_creatureCounts[baseSlot] > 2)
                    quantityMod = -0.1f;
                else
                    quantityMod = -0.14f;
                if ((gMonsterDatabase[group->m_creatureTypes[baseSlot]].stats.attributes
                     & MONSTER_FLAGS_SHOOTER)
                    || group->m_creatureTypes[baseSlot] == CREATURE_SPRITE
                    || group->m_creatureTypes[baseSlot] == CREATURE_ROGUE)
                    quantityMod = quantityMod * 0.7;
                else if (group->m_creatureTypes[baseSlot] == CREATURE_GRIFFIN)
                    quantityMod = quantityMod * 1.2;
                worthValue = static_cast<i32>(worthValue * (1.0f + quantityMod));
            }
            theArmyWorth += worthValue;
        }
    }
    if (useTown) {
        curArrows = 5;
        pTown = gpGame->GetTown(townId);
        for (baseSlot = BUILDING_SLOT_DWELLING_FIRST; baseSlot <= BUILDING_SLOT_DWELLING_LAST;
             baseSlot++)
            if (pTown->m_buildings & (1 << baseSlot))
                curArrows += 4;
        for (baseSlot = BUILDING_SLOT_MAGE_GUILD; baseSlot <= BUILDING_SLOT_GENERIC_LAST;
             baseSlot++)
            if (pTown->m_buildings & (1 << baseSlot))
                curArrows++;
        castleValue = curArrows * 120;
    }
    if (useHero && heroPointer) {
        nextStats = heroPointer->m_primaryStats[HERO_PRIMARY_ATTACK]
                    + heroPointer->m_primaryStats[HERO_PRIMARY_DEFENSE] + 20;
        if (nextStats < 0)
            nextStats = 0;
        if (nextStats > 40)
            nextStats = 40;
        theArmyWorth = static_cast<i32>(theArmyWorth * gStatPower[nextStats]);
        castleValue = static_cast<i32>(castleValue * gStatPower[nextStats]);
        morale = heroPointer->m_army.GetMorale(heroPointer, NULL);
        if (morale > 0)
            theArmyWorth = theArmyWorth * (morale + 48) / 48;
        else if (morale < 0)
            theArmyWorth = theArmyWorth * (morale + 24) / 24;
        newLuck = gpGame->GetLuck(heroPointer, NULL);
        if (newLuck)
            theArmyWorth = theArmyWorth * (newLuck + 16) / 16;
        if (heroPointer->m_primaryStats[HERO_PRIMARY_SPELL_POWER] == 1)
            savedMod = 0.25f;
        else if (heroPointer->m_primaryStats[HERO_PRIMARY_SPELL_POWER] == 2)
            savedMod = 0.5f;
        else
            savedMod = 1.0f;
        theValue = -1;
        for (baseSlot = 0; baseSlot < HERO_SPELL_SLOT_COUNT; baseSlot++) {
            if (heroPointer->m_spells[baseSlot] >= 0
                && (gSpellAIFlags[heroPointer->m_spells[baseSlot]] & SPELL_AI_FLAG_COMBAT)) {
                spellScore = static_cast<i32>(
                    gSpellAIValue[heroPointer->m_spells[baseSlot]]
                    * ((gSpellAIFlags[heroPointer->m_spells[baseSlot]]
                        & SPELL_AI_FLAG_SCALES_WITH_POWER)
                           ? (heroPointer->m_primaryStats[HERO_PRIMARY_SPELL_POWER] <= 40
                                  ? gBattleStat[heroPointer
                                                    ->m_primaryStats[HERO_PRIMARY_SPELL_POWER]]
                                  : gBattleStat[40])
                           : savedMod)
                );
                curTotal += spellScore
                            * gSpellCastNumMod
                                [heroPointer->m_spellCharges[baseSlot] <= 20
                                     ? heroPointer->m_spellCharges[baseSlot]
                                     : 20];
                if (spellScore > theValue)
                    theValue = spellScore;
            }
        }
        if (curTotal > theValue * 3.5)
            curTotal = static_cast<i32>(theValue * 3.5);
        if (curTotal > theArmyWorth * 2)
            curTotal = static_cast<i32>(theArmyWorth * 1.25);
        else if (curTotal > theArmyWorth * 1.5)
            curTotal = theArmyWorth;
        else if (curTotal > theArmyWorth)
            curTotal = static_cast<i32>(theArmyWorth * 0.75);
    }
    if (castleValue > theArmyWorth * 2)
        castleValue = static_cast<i32>(theArmyWorth * 1.5);
    else if (castleValue > theArmyWorth * 1.5)
        castleValue = static_cast<i32>(theArmyWorth * 1.25);
    else if (castleValue > theArmyWorth)
        castleValue = static_cast<i32>(theArmyWorth * 0.9);
    theArmyWorth += curTotal;
    theArmyWorth += castleValue;
    return theArmyWorth;
}

// donor PoL RVA 0x00040aca; preferred Buka symbol ?EvaluateOneTimeCreaturePurchase@philAI@@QAEXHHHAAH00@Z
// donor Buka TU SOURCE/PHILAI; HoMM1 owner inferred from contiguous order
// evidence: graph:3;base=0.513526;margin=0.703256;shape=0.342;size=0.854;calls=1.000;alternate=pol20:void philAI::EvaluateOneTimeCreaturePurchase(int, int, int, int &, int &, int &)@0x00040aca
VA(0x0044d38f, 0x192)
void philAI::EvaluateOneTimeCreaturePurchase(
    hero* pHero,
    i32 creature,
    i32 availableCount,
    i32 useAvailableCount,
    i32& purchaseCount,
    i32& purchaseValue,
    i32& replacementSlot
) {
    i32 leastStackValue;
    i32 replacementValue;
    i32 num;
    i32 n;

    purchaseCount = 0;
    purchaseValue = 0;
    replacementSlot = -1;
    leastStackValue = 999999;
    if (useAvailableCount != 0)
        purchaseCount = availableCount;
    else
        purchaseCount = MaxBuyableCreatures(creature);
    if (purchaseCount > availableCount)
        purchaseCount = availableCount;
    if (purchaseCount == 0)
        return;
    num = purchaseCount * gMonsterDatabase[creature].fightValue;
    if (pHero->m_army.CanJoin(creature) == 0) {
        for (n = 0; n < ARMY_GROUP_SLOT_COUNT; n++) {
            if (pHero->m_army.m_creatureTypes[n] == creature) {
                replacementSlot = -1;
                n = ARMY_GROUP_SLOT_COUNT;
            } else {
                replacementValue =
                    pHero->m_army.m_creatureCounts[n] * gMonsterDatabase[n].fightValue;
                if (replacementValue < leastStackValue) {
                    leastStackValue = replacementValue;
                    replacementSlot = n;
                }
            }
        }
    }
    if (replacementSlot != -1)
        num -= leastStackValue;
    purchaseValue =
        static_cast<i32>(num * gpGame->m_players[pHero->m_owner].m_aiData.m_upgradeValueWeight);
    if (useAvailableCount == 0) {
        GetMonsterCost(creature, costTemp);
        purchaseValue -= purchaseCount * RVConversion(costTemp);
    }
    if (purchaseValue < 0) {
        purchaseValue = 0;
        purchaseCount = 0;
    }
}

// donor PoL RVA 0x00040cb1; preferred Buka symbol ?QuickCombat@philAI@@QAEHPAVarmyGroup@@PAVhero@@01HHAAM2@Z
// donor Buka TU SOURCE/PHILAI; HoMM1 owner inferred from contiguous order
// evidence: graph:5;base=0.373791;margin=0.600389;shape=0.327;size=0.583;calls=0.548;alternate=pol20:int philAI::QuickCombat(class armyGroup *, class hero *, class armyGroup *, class hero *, int, int, float &, float &)@0x00040cb1
VA(0x0044d521, 0x300)
i32 philAI::QuickCombat(
    armyGroup* attacker,
    hero* attackerHero,
    armyGroup* defender,
    hero* defenderHero,
    i8 townBattle,
    i8 townId,
    float& attackerDamage,
    float& defenderDamage
) {
    i32 aDeadNum;
    float fracLostVal;
    i32 win;
    float newRnd;
    i32 ignored;
    i32 dLeft;
    i32 dDead;
    armyGroup* newWinner;
    float diff;
    float curWinChance;
    i32 atkExp;
    i32 tmp;
    i32 theDefenderExp;
    float curWChance;
    i32 resValue;
    i32 aLeftNum;

    atkExp = gpGame->ExperienceValueOfStack(attacker, attackerHero);
    theDefenderExp = gpGame->ExperienceValueOfStack(defender, defenderHero);
    win = 0;
    newWinner = NULL;
    ProbableOutcomeOfBattle(
        attacker,
        attackerHero,
        defender,
        defenderHero,
        NULL,
        townBattle,
        townId,
        defenderHero != NULL ? defenderHero->m_owner : -1,
        curWinChance,
        aDeadNum,
        dDead,
        aLeftNum,
        dLeft,
        resValue
    );
    newRnd = Random(0, 100) / 100.0;
    if (newRnd < curWinChance) {
        win = 1;
        curWChance = curWinChance;
        newWinner = attacker;
    } else {
        curWChance = 1.0f - curWinChance;
        newWinner = defender;
    }
    // VC4 narrows this double through a stack temporary only for the C-style
    // cast (retail frame 0x50); static_cast<float> drops the slot.
    diff = (float)(newRnd > curWinChance ? newRnd - curWinChance : curWinChance - newRnd);
    if (win != 0 && curWinChance > 0.6)
        diff *= curWinChance + 0.65;
    fracLostVal = (1.0 - diff) * (1.0 - diff);
    if (curWChance > 0.8 && fracLostVal > 0.2)
        fracLostVal *= fracLostVal;
    if (curWChance > 0.96 && fracLostVal > (1.0f - curWChance) / 2.0f)
        fracLostVal = (1.0f - curWChance) / 2.0f;
    if (win != 0) {
        if (attackerHero != NULL) {
            gpAdvManager->GiveExperience(attackerHero, theDefenderExp, 1);
            attackerHero->ApplyBattleWinTemps();
        }
        defenderDamage = 1.0f;
        attackerDamage = fracLostVal;
    } else {
        if (attackerHero != NULL) {
            attackerHero->m_remainingMobility = 0;
            attackerHero->ApplyBattleLossTemps();
        }
        if (defenderHero != NULL)
            attackerHero->ApplyBattleWinTemps();
        defenderDamage = fracLostVal * diff;
        attackerDamage = 1.0f;
        if (attackerDamage >= 0.99 && defenderHero != NULL)
            gpAdvManager->GiveExperience(defenderHero, theDefenderExp, 1);
    }
    if (attackerDamage > 0.99)
        gpAdvManager->TransferArtifacts(attackerHero, defenderHero);
    else if (defenderDamage > 0.99)
        gpAdvManager->TransferArtifacts(defenderHero, attackerHero);
    DamageGroup(attacker, attackerHero, defenderHero, attackerDamage);
    DamageGroup(defender, defenderHero, attackerHero, defenderDamage);
    if (win != 0 && townBattle)
        gpGame->ClaimTown(townId, giCurPlayer);
    return win;
}

// donor PoL RVA 0x0004183b; preferred Buka symbol ?HeroInteractionAtTown@philAI@@QAEXPAVhero@@PAVtown@@HPAH@Z
// donor Buka TU SOURCE/PHILAI; HoMM1 owner inferred from contiguous order
// evidence: graph:3;base=0.484024;margin=0.235954;shape=0.394;size=0.661;calls=0.952;alternate=pol20:void philAI::HeroInteractionAtTown(class hero *, class town *, int, int *)@0x0004183b
VA(0x0044d821, 0xa15)
void philAI::HeroInteractionAtTown(
    hero* heroPointer,
    town* townPointer,
    i32 doInteraction,
    i32* value
) {
    i32 townFV;
    armyGroup* fromArmy;
    i32 statSum;
    i32 more;
    float townShareDiff;
    i32 stackCount;
    i32 battlePower;
    i32 whichSpell;
    i32 otherIndex;
    i32 estTransferValue;
    float garrisonShare;
    i32 choice;
    armyGroup* targetArmyGroup;
    i32 innerIndex;
    float myTargetShare;
    i32 i;
    i32 transferRating;
    i32 hasRoom;
    float transferFactor;
    float curveTerm;
    i32 speedLimit;
    i32 transferredCount;
    i32 bestFV;
    i32 stackFV;

    *value = 0;
    if (doInteraction) {
        if ((townPointer->m_buildings & (1 << BUILDING_SLOT_SHIPYARD))
            && giBestShipyardId != townPointer->m_id) {
            i = MANHATTAN_LENGTH(
                townPointer->m_x - heroPointer->m_x,
                townPointer->m_y - heroPointer->m_y
            );
            if (gbActualShipyardFound) {
                if (i < giBestShipyardDist) {
                    giBestShipyardDist = i;
                    giBestShipyardId = townPointer->m_id;
                }
            } else {
                giBestShipyardDist = i;
                giBestShipyardId = townPointer->m_id;
            }
            gbPossibleShipyardFound = 1;
            gbActualShipyardFound = 1;
        } else if ((townPointer->m_buildings & (1 << BUILDING_SLOT_CASTLE))
                   && gpAdvManager->GetCell(townPointer->m_x - 1, townPointer->m_y + 1)->m_tileIndex
                          < MAP_CELL_TILES_PER_TERRAIN
                   && !gbActualShipyardFound && giBestShipyardId != townPointer->m_id) {
            i = MANHATTAN_LENGTH(
                townPointer->m_x - heroPointer->m_x,
                townPointer->m_y - heroPointer->m_y
            );
            if (gbPossibleShipyardFound) {
                if (i < giBestShipyardDist) {
                    giBestShipyardDist = i;
                    giBestShipyardId = townPointer->m_id;
                }
            } else {
                giBestShipyardDist = i;
                giBestShipyardId = townPointer->m_id;
            }
            gbPossibleShipyardFound = 1;
        }
    } else if (heroPointer->m_primaryStats[HERO_PRIMARY_KNOWLEDGE] > 0
               && !heroPointer->HasArtifact(ARTIFACT_MAGIC_BOOK)
               && (townPointer->m_buildings & (1 << BUILDING_SLOT_MAGE_GUILD))) {
        if (gpCurPlayer->m_resources[RESOURCE_GOLD] >= 500) {
            gpAdvManager->GiveArtifact(heroPointer, ARTIFACT_MAGIC_BOOK);
            gpCurPlayer->m_resources[RESOURCE_GOLD] -= 500;
        } else {
            heroPointer->m_remainingMobility = 0;
        }
    }
    if ((townPointer->m_buildings & (1 << BUILDING_SLOT_MAGE_GUILD))
        && (doInteraction || heroPointer->HasArtifact(ARTIFACT_MAGIC_BOOK))) {
        for (i = 0; i < gMageGuildSpellCount[townPointer->m_buildState]; i++) {
            whichSpell = heroPointer->AddSpell(
                townPointer->m_mageGuildSpells[i],
                heroPointer->m_primaryStats[HERO_PRIMARY_KNOWLEDGE],
                doInteraction
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
    battlePower = FightValueOfStack(&heroPointer->m_army, NULL, 0, 0, 0);
    townFV = FightValueOfStack(&townPointer->m_army, NULL, 0, 0, 0);
    garrisonShare = static_cast<double>(townFV) / (townFV + battlePower);
    statSum = 0;
    statSum = heroPointer->m_primaryStats[HERO_PRIMARY_ATTACK]
              + heroPointer->m_primaryStats[HERO_PRIMARY_DEFENSE];
    if (statSum > 10)
        statSum = 10;
    if (townPointer->m_buildings & (1 << BUILDING_SLOT_CASTLE))
        myTargetShare = 0.54 - statSum * 0.02;
    else
        myTargetShare = 0.33 - statSum * 0.01;
    townShareDiff =
        (myTargetShare < garrisonShare ? garrisonShare - myTargetShare
                                       : myTargetShare - garrisonShare);
    if (townShareDiff < myTargetShare * 0.15)
        return;
    otherIndex = 0;
    if (myTargetShare < garrisonShare)
        otherIndex = 1;
    if (doInteraction) {
        if (battlePower < townFV)
            transferFactor = 0.25f;
        else
            transferFactor = 0.13f;
        curveTerm = 1.0f + townShareDiff - 0.22;
        transferRating = static_cast<i32>(
            (curveTerm * curveTerm - 1.0f) * gpCurPlayer->m_aiData.m_upgradeValueWeight
            * (townFV + battlePower) * transferFactor
        );
        if (transferRating < 0)
            transferRating = 0;
        hasRoom = 0;
        if (otherIndex) {
            for (i = 0; i < ARMY_GROUP_SLOT_COUNT; i++)
                if (heroPointer->m_army.m_creatureCounts[i] <= 0)
                    hasRoom = 1;
        } else {
            for (i = 0; i < ARMY_GROUP_SLOT_COUNT; i++)
                if (townPointer->m_army.m_creatureCounts[i] <= 0)
                    hasRoom = 1;
        }
        if (!hasRoom) {
            for (i = 0; i < ARMY_GROUP_SLOT_COUNT; i++) {
                for (innerIndex = 0; innerIndex < ARMY_GROUP_SLOT_COUNT; innerIndex++) {
                    if (townPointer->m_army.m_creatureTypes[i]
                        == heroPointer->m_army.m_creatureTypes[innerIndex]) {
                        hasRoom = 1;
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
    if (otherIndex)
        townShareDiff = townShareDiff + 0.04;
    estTransferValue = static_cast<i32>((battlePower + townFV) * townShareDiff);
    fromArmy = otherIndex ? &townPointer->m_army : &heroPointer->m_army;
    targetArmyGroup = otherIndex ? &heroPointer->m_army : &townPointer->m_army;
    more = 1;
    gbTroopReload = 0;
    while (more) {
        if (!otherIndex) {
            stackCount = 0;
            for (i = 0; i < ARMY_GROUP_SLOT_COUNT; i++)
                if (heroPointer->m_army.m_creatureTypes[i] != CREATURE_NONE)
                    stackCount += heroPointer->m_army.m_creatureCounts[i];
            if (stackCount <= 1)
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
            if (otherIndex)
                speedLimit = 1;
            else
                speedLimit = 3;
            for (i = 0; i < ARMY_GROUP_SLOT_COUNT; i++) {
                if (fromArmy->m_creatureTypes[i] != CREATURE_NONE) {
                    stackFV = fromArmy->m_creatureCounts[i]
                              * gMonsterDatabase[fromArmy->m_creatureTypes[i]].fightValue;
                    if ((otherIndex
                         && gMonsterDatabase[fromArmy->m_creatureTypes[i]].stats.speed > speedLimit)
                        || (!otherIndex
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
            more = 0;
        } else if (targetArmyGroup->CanJoin(fromArmy->m_creatureTypes[choice])) {
            transferredCount = static_cast<i32>(static_cast<float>(
                static_cast<double>(estTransferValue)
                    / gMonsterDatabase[fromArmy->m_creatureTypes[choice]].fightValue
                + 0.5
            ));
            if (transferredCount > 0) {
                if (transferredCount > fromArmy->m_creatureCounts[choice]) {
                    transferredCount = fromArmy->m_creatureCounts[choice];
                } else {
                    more = 0;
                    if (transferredCount >= fromArmy->m_creatureCounts[choice] * 0.65
                        || transferredCount >= fromArmy->m_creatureCounts[choice] - 1) {
                        if ((fromArmy->m_creatureCounts[choice] - transferredCount)
                                * gMonsterDatabase[fromArmy->m_creatureTypes[choice]].fightValue
                            < ((otherIndex ? townFV : battlePower) - estTransferValue) * 0.2)
                            transferredCount = fromArmy->m_creatureCounts[choice];
                    }
                }
                if (!otherIndex && transferredCount >= stackCount) {
                    transferredCount = stackCount - 1;
                    more = 0;
                }
                if (gMonsterDatabase[fromArmy->m_creatureTypes[choice]].fightValue
                        * transferredCount * 1.2
                    > estTransferValue)
                    more = 0;
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
                more = 0;
            }
        } else {
            more = 0;
        }
    }
    if (!doInteraction && giHumanTownConquered == townPointer->m_id
        && heroPointer->m_remainingMobility <= 20)
        heroPointer->m_remainingMobility = 0;
}

// Buka 2.1 ChooseGoldOrExperience; HoMM1 weighs the experience by the
// hero's AI fight value instead of a fixed gold threshold.
VA(0x0044e236, 0x3f)
i32 philAI::ChooseGoldOrExperience(hero* thisHero, i32 gold, i32 experience) {
    i32 goldRV;
    i32 expRV;

    expRV = static_cast<i32>(experience * thisHero->m_aiFightValue);
    goldRV = static_cast<i32>(gold * gafAITurnCostResource[RESOURCE_GOLD]);
    return goldRV > expRV;
}

// donor PoL RVA 0x000425b0; preferred Buka symbol ?ChooseEvaluateBattle@philAI@@QAEXPAVarmyGroup@@PAVhero@@01HHHAAH2@Z
// donor Buka TU SOURCE/PHILAI; HoMM1 owner inferred from contiguous order
// evidence: graph:3;base=0.572029;margin=0.742196;shape=0.490;size=0.824;calls=1.000;alternate=pol20:void philAI::ChooseEvaluateBattle(class armyGroup *, class hero *, class armyGroup *, class hero *, int, int, int, int &, int &)@0x000425b0
VA(0x0044e275, 0xa4)
void philAI::ChooseEvaluateBattle(
    armyGroup* attackerArmy,
    hero* attackerHero,
    armyGroup* defenderArmy,
    hero* defenderHero,
    i32 isCastle,
    i32 castleId,
    i32 rewardValue,
    i32& outFlag,
    i32& outValue
) {
    float chance;
    i32 curA;
    i32 curB;
    i32 leftA;
    i32 leftB;
    i32 thisVacant;
    i32 rating;

    ProbableOutcomeOfBattle(
        attackerArmy,
        attackerHero,
        defenderArmy,
        defenderHero,
        NULL,
        isCastle,
        castleId,
        defenderHero != NULL ? defenderHero->m_owner : -1,
        chance,
        curA,
        curB,
        leftA,
        leftB,
        rating
    );
    rating = static_cast<i32>(rating + rewardValue * chance);
    if (rating <= 0) {
        outValue = 0;
        outFlag = 0;
    } else {
        outValue = rating;
        outFlag = 1;
    }
}

// HoMM1 treasure-artifact purchase: affordable gold and an artifact worth
// more than its gold cost (Buka NetValueOfArtifact's valuation).
VA(0x0044e319, 0x42)
i32 philAI::ChooseToBuyArtifact(hero*, i32 artifact, i32 goldCost) {
    if (gpCurPlayer->m_resources[RESOURCE_GOLD] >= goldCost
        && gArtifactBaseRV[artifact] > goldCost * gafAITurnCostResource[RESOURCE_GOLD])
        return 1;
    else
        return 0;
}

// Buka 2.1 returns one for the ransom choice. HoMM1's daemon-cave caller
// passes a hero and the gold amount; the retail body returns the same one.
VA(0x0044e35b, 0x12)
i32 philAI::ChooseToPayRansomOnHero(hero*, i32) {
    return 1;
}

// Buka 2.1 BuildBuilding with HoMM1's town update written in place: the mage
// guild level, castle conversion and new dwelling stock.
VA(0x0044e36d, 0x140)
void philAI::BuildBuilding(town* townPointer, i16 building) {
    i32 i;
    i32 cost[RESOURCE_COUNT];

    GetBuildingCost(townPointer->m_type, building, cost, townPointer->m_buildState);
    for (i = 0; i < RESOURCE_COUNT; i++)
        gpCurPlayer->m_resources[i] -= cost[i];
    if (building == BUILDING_SLOT_MAGE_GUILD) {
        if (townPointer->m_buildings & (1 << BUILDING_SLOT_MAGE_GUILD))
            townPointer->m_buildState++;
        if (townPointer->m_occupyingHeroId != TOWN_OCCUPYING_HERO_NONE)
            townPointer->GiveSpells();
    }
    townPointer->m_buildings |= 1 << building;
    if (building >= BUILDING_SLOT_DWELLING_FIRST && building <= BUILDING_SLOT_DWELLING_LAST)
        townPointer->m_garrison[building - BUILDING_SLOT_DWELLING_FIRST] =
            gMonsterDatabase[gDwellingType[townPointer->m_type]
                                          [building - BUILDING_SLOT_DWELLING_FIRST]]
                .growth;
    if (building == BUILDING_SLOT_CASTLE) {
        townPointer->m_buildings &= ~(1 << BUILDING_SLOT_TENT);
        townPointer->XformToCastle();
    }
    BitSet(gpGame->m_townBuiltToday, townPointer->m_id);
    ShowStatus();
}

// Buka 2.1 BuildHero without the later network/army bookkeeping: the hero
// stands on the town cell and a random faction refills the tavern slot.
VA(0x0044e4ad, 0x223)
void philAI::BuildHero(town* townPointer, i16 availableHeroIndex) {
    hero* newHero;
    i16 townX;
    i16 townY;

    gpCurPlayer->m_resources[RESOURCE_GOLD] -= gHeroGoldCost;
    gpCurPlayer->m_heroIds[gpCurPlayer->m_heroCount] =
        gpCurPlayer->m_availableHeroIds[availableHeroIndex];
    gpCurPlayer->m_heroCount++;
    townX = townPointer->m_x;
    townY = townPointer->m_y;
    newHero = gpGame->GetHero(gpCurPlayer->m_availableHeroIds[availableHeroIndex]);
    gpGame->SetRandomHeroArmies(newHero->m_id, RANDOM_HERO_STRONG_ARMY);
    newHero->m_owner = giCurPlayer;
    newHero->m_x = townX;
    newHero->m_y = townY;
    newHero->m_eventFlags = 0;
    newHero->m_direction = MAP_DIRECTION_EAST;
    newHero->m_remainingMobility = newHero->CalcMobility();
    newHero->m_mobility = newHero->m_remainingMobility;
    newHero->m_locationType = gpGame->m_map[townX][townY].m_triggerType;
    newHero->m_occupiedTown = gpGame->m_map[townX][townY].m_objectMetadata;
    gpGame->m_map[townX][townY].m_triggerType = (MAP_TRIGGER_EVENT | MAP_OBJECT_HERO);
    gpGame->m_map[townX][townY].m_objectMetadata =
        gpCurPlayer->m_availableHeroIds[availableHeroIndex];
    gpGame->m_availableHeroes[newHero->m_id] = townPointer->m_owner;
    townPointer->m_occupyingHeroId = newHero->m_id;
    townPointer->GiveSpells();
    gpCurPlayer->m_availableHeroIds[availableHeroIndex] = gpGame->GetNewHeroId(Random(0, 3));
    gpGame->m_availableHeroes[gpCurPlayer->m_availableHeroIds[availableHeroIndex]] =
        HERO_AVAILABILITY_RETREATED;
    bHeroBuiltThisTurn = 1;
    ShowStatus();
}

// Buka 2.1 BuildCreature without the full-army eviction: pay, take the stock
// and add the stack to the garrison.
VA(0x0044e6d0, 0xad)
void philAI::BuildCreature(town* townPointer, i32 dwelling, i32 purchaseCount) {
    i32 cost[RESOURCE_COUNT];
    i32 creature;
    i32 n;

    creature = gDwellingType[townPointer->m_type][dwelling];
    GetMonsterCost(creature, cost);
    for (n = 0; n < RESOURCE_COUNT; n++)
        gpCurPlayer->m_resources[n] -= purchaseCount * cost[n];
    townPointer->m_garrison[dwelling] -= purchaseCount;
    townPointer->m_army.Add(creature, purchaseCount, ARMY_GROUP_EMPTY_SLOT);
    ShowStatus();
}

// donor PoL RVA 0x00042ead; preferred Buka symbol ?CanBuyBHC@philAI@@QAEHAAUBHC@@@Z
// donor Buka TU SOURCE/PHILAI; HoMM1 owner inferred from contiguous order
// evidence: graph:3;base=0.453882;margin=0.780446;shape=0.369;size=0.712;calls=0.667;alternate=pol20:int philAI::CanBuyBHC(struct BHC &)@0x00042ead
VA(0x0044e77d, 0x13f)
i32 philAI::CanBuyBHC(BHC& purchase) {
    i32 index;
    i32 j;
    i32 cost[RESOURCE_COUNT];
    switch (purchase.type) {
        case PURCHASE_BUILDING:
            if (CanBuy(purchase.pTown, purchase.what))
                return 1;
            break;
        case PURCHASE_HERO:
            if (gpCurPlayer->m_resources[RESOURCE_GOLD] >= gHeroGoldCost
                && purchase.pTown->m_occupyingHeroId == TOWN_OCCUPYING_HERO_NONE
                && bHeroBuiltThisTurn == 0)
                return 1;
            break;
        case PURCHASE_CREATURE:
            j = gDwellingType[purchase.pTown->m_type][purchase.what];
            if (purchase.num > purchase.pTown->m_garrison[purchase.what])
                return 0;
            if (!purchase.pTown->m_army.CanJoin(j))
                return 0;
            GetMonsterCost(j, cost);
            for (index = 0; index < RESOURCE_COUNT; index++)
                if (gpCurPlayer->m_resources[index] < cost[index] * purchase.num)
                    return 0;
            return 1;
    }
    return 0;
}

// donor PoL RVA 0x00043007; preferred Buka symbol ?CombatMonsterEvent@philAI@@QAEHPAVhero@@HPAHPAVmapCell@@@Z
// donor Buka TU SOURCE/PHILAI; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.554517;margin=0.429398;shape=0.381;size=0.912;calls=1.000;alternate=pol20:int philAI::CombatMonsterEvent(class hero *, int, int *, class mapCell *)@0x00043007
VA(0x0044e8bc, 0x170)
i8 philAI::CombatMonsterEvent(hero* h, i8 monType, i32* pCount, mapCell*) {
    float casualtyRatio;
    float fLoss;
    i32 result;
    i16 newCount;
    i16 i;

    CLEAR_ARMY_GROUP(*gpMonGroup);
    if (*pCount / ARMY_GROUP_SLOT_COUNT > 0) {
        for (i = 0; i < ARMY_GROUP_SLOT_COUNT; i++) {
            gpMonGroup->m_creatureTypes[i] = monType;
            gpMonGroup->m_creatureCounts[i] = *pCount / ARMY_GROUP_SLOT_COUNT;
        }
    }
    for (i = *pCount % ARMY_GROUP_SLOT_COUNT - 1; i >= 0; i--) {
        gpMonGroup->m_creatureTypes[i] = monType;
        gpMonGroup->m_creatureCounts[i]++;
    }
    result = gpPhilAI->QuickCombat(&h->m_army, h, gpMonGroup, NULL, 0, 0, casualtyRatio, fLoss);
    newCount = 0;
    for (i = 0; i < ARMY_GROUP_SLOT_COUNT; i++)
        newCount += gpMonGroup->m_creatureCounts[i];
    *pCount = newCount;
    if (result != 0)
        return 1;
    return 0;
}

// donor PoL RVA 0x0004316b; preferred Buka symbol ?FightEvent@philAI@@QAEHPAVhero@@PAVmapCell@@H@Z
// donor Buka TU SOURCE/PHILAI; HoMM1 owner inferred from contiguous order
// evidence: graph:5;base=0.277616;margin=0.834782;shape=0.262;size=0.393;calls=0.393;alternate=pol20:int philAI::FightEvent(class hero *, class mapCell *, int)@0x0004316b
VA(0x0044ea2c, 0x22f)
void philAI::FightEvent(hero* heroPointer, mapCell* cell) {
    float attackerLoss;
    i32 activeRewardValue;
    i32 unusedValue;
    i32 theWorth;
    i16 curGuards[4];
    float defenderLoss;
    i32 bit;
    i16 n;
    i32 localWon;

    if (cell->m_objectMetadata == GHOST_SITE_EMPTY)
        return;
    curGuards[0] = 2;
    curGuards[1] = 3;
    curGuards[2] = 5;
    curGuards[3] = 10;
    for (n = 0; n < ARMY_GROUP_SLOT_COUNT; n++) {
        gpMonGroup->m_creatureTypes[n] = CREATURE_GHOST;
        gpMonGroup->m_creatureCounts[n] = curGuards[cell->m_objectMetadata - GHOST_SITE_SMALL];
    }
    switch (cell->m_objectMetadata) {
        case GHOST_SITE_SMALL:
            activeRewardValue = static_cast<i32>(1000.0f * gafAITurnCostResource[RESOURCE_GOLD]);
            break;
        case GHOST_SITE_MEDIUM:
            activeRewardValue = static_cast<i32>(2000.0f * gafAITurnCostResource[RESOURCE_GOLD]);
            break;
        case GHOST_SITE_LARGE:
            activeRewardValue = static_cast<i32>(5000.0f * gafAITurnCostResource[RESOURCE_GOLD]);
            break;
        case GHOST_SITE_HUGE:
            activeRewardValue = static_cast<i32>(
                2000.0f * gafAITurnCostResource[RESOURCE_GOLD]
                + gpCurPlayer->m_aiData.m_artifactValue
            );
            break;
        default:
            return;
    }
    ChooseEvaluateBattle(
        &heroPointer->m_army,
        heroPointer,
        gpMonGroup,
        NULL,
        0,
        0,
        activeRewardValue,
        bit,
        theWorth
    );
    if (bit) {
        localWon = QuickCombat(
            &heroPointer->m_army,
            heroPointer,
            gpMonGroup,
            NULL,
            0,
            0,
            defenderLoss,
            attackerLoss
        );
        if (localWon) {
            switch (cell->m_objectMetadata) {
                case GHOST_SITE_SMALL:
                    gpAdvManager->GiveResource(heroPointer, RESOURCE_GOLD, GHOST_SMALL_GOLD);
                    break;
                case GHOST_SITE_MEDIUM:
                    gpAdvManager->GiveResource(heroPointer, RESOURCE_GOLD, GHOST_MEDIUM_GOLD);
                    break;
                case GHOST_SITE_LARGE:
                    gpAdvManager->GiveResource(heroPointer, RESOURCE_GOLD, GHOST_LARGE_GOLD);
                    break;
                case GHOST_SITE_HUGE:
                    gpAdvManager->GiveResource(heroPointer, RESOURCE_GOLD, GHOST_HUGE_GOLD);
                    gpAdvManager->GiveRandomArtifact(heroPointer);
                    break;
            }
            cell->m_objectMetadata = GHOST_SITE_EMPTY;
        }
    }
}

// donor PoL RVA 0x00043842; preferred Buka symbol ?DamageGroup@philAI@@QAEHPAVarmyGroup@@PAVhero@@1M@Z
// donor Buka TU SOURCE/PHILAI; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.548912;margin=0.547381;shape=0.486;size=0.739;calls=1.000;alternate=pol20:int philAI::DamageGroup(class armyGroup *, class hero *, class hero *, float)@0x00043842
VA(0x0044ec5b, 0x55)
i32 philAI::DamageGroup(armyGroup* ag, hero* loser, hero*, float dmg) {
    if (dmg < 0.99) {
        ag->DamageGroup(dmg);
        return 0;
    } else {
        if (loser != NULL)
            gpAdvManager->HeroLoses(loser);
        else
            ag->DamageGroup(dmg);
        return 1;
    }
}

// HoMM1 primary-stat valuation: the table worth of the new level (capped at
// twenty) less that of the old one; used for hero stat gains.
VA(0x0044ecb0, 0x4f)
float philAI::StatChangeValue(i32 oldValue, i32 newValue) {
    return (newValue > 20 ? gSpellCastNumMod[20] : gSpellCastNumMod[newValue])
           - (oldValue > 20 ? gSpellCastNumMod[20] : gSpellCastNumMod[oldValue]);
}

// Buka 2.1 IncrementHourGlass: the AI-turn hourglass advances faster with
// fewer (prospective) heroes and stops at its last phase.
VA(0x0044ecff, 0xcc)
void philAI::IncrementHourGlass(void) {
    i32 heroCount = gpCurPlayer->m_heroCount;
    if (heroCount < 4 && gpCurPlayer->m_resources[RESOURCE_GOLD] >= 2500 && bHeroBuiltThisTurn == 0)
        heroCount++;
    gCurHourGlassPhase++;
    if (heroCount == 1) {
        gCurHourGlassPhase++;
        gCurHourGlassPhase++;
    }
    if (heroCount == 2 && gCurHourGlassPhase != 1)
        gCurHourGlassPhase++;
    if (heroCount == 3 && (gCurHourGlassPhase == 3 || gCurHourGlassPhase == 6))
        gCurHourGlassPhase++;
    if (gCurHourGlassPhase > 9)
        gCurHourGlassPhase = 9;
}

// donor PoL RVA 0x00043980; preferred Buka symbol ?TownEvent@philAI@@QAEXPAVmapCell@@PAVhero@@HH@Z
// donor Buka TU SOURCE/PHILAI; HoMM1 owner inferred from contiguous order
// evidence: graph:3;base=0.582749;margin=0.601748;shape=0.455;size=0.921;calls=1.000;alternate=pol20:void philAI::TownEvent(class mapCell *, class hero *, int, int)@0x00043980
VA(0x0044edcb, 0x1e9)
void philAI::TownEvent(mapCell* cell, hero* heroPointer, i32 x, i32 y) {
    float attackerLoss;
    float defenderLoss;
    i32 savedOwner;
    i32 quickResult;
    town* townPointerPtr;
    hero* theHero;
    i32 firstOutcome;

    townPointerPtr = gpGame->GetTown(cell->m_objectMetadata);
    savedOwner = giCurPlayer;
    gpAdvManager->DemobilizeCurrHero();
    if (townPointerPtr->m_owner != giCurPlayer) {
        if (townPointerPtr->HasGarrison()) {
            if (townPointerPtr->m_owner < 0 || gbHumanPlayer[townPointerPtr->m_owner] == 0) {
                quickResult = QuickCombat(
                    &heroPointer->m_army,
                    heroPointer,
                    &townPointerPtr->m_army,
                    NULL,
                    1,
                    townPointerPtr->m_id,
                    defenderLoss,
                    attackerLoss
                );
            } else {
                theHero = townPointerPtr->m_occupyingHeroId == TOWN_OCCUPYING_HERO_NONE
                              ? NULL
                              : gpGame->GetHero(townPointerPtr->m_occupyingHeroId);
                firstOutcome = gpAdvManager->DoCombat(
                    x,
                    y,
                    heroPointer,
                    &heroPointer->m_army,
                    townPointerPtr,
                    theHero,
                    &townPointerPtr->m_army,
                    x,
                    y,
                    -1,
                    1
                );
                if (firstOutcome == COMBAT_RESULT_ATTACKER) {
                    gpGame->ClaimTown(townPointerPtr->m_id, giCurPlayer);
                    giHumanTownConquered = townPointerPtr->m_id;
                }
            }
        } else {
            gpGame->ClaimTown(townPointerPtr->m_id, giCurPlayer);
        }
    }
    if (townPointerPtr->m_owner == giCurPlayer && heroPointer->m_x == x && heroPointer->m_y == y) {
        townPointerPtr->m_occupyingHeroId = gpCurPlayer->CurrentHero();
        heroPointer->m_locationType = (MAP_TRIGGER_EVENT | MAP_OBJECT_TOWN);
        heroPointer->m_occupiedTown = townPointerPtr->m_id;
        HeroInteractionAtTown(heroPointer, townPointerPtr, 0, &iDummy);
    }
    gpAdvManager->MobilizeCurrHero(0);
    townPointerPtr->GiveSpells();
}

// ValueOfEventAtPosition module state (.bss order follows names, not position).
DATA(0x004bb138)
i32 gArtifactChoice3;
DATA(0x004c8cb4)
i32 gEventTownId;
DATA(0x004ca15c)
i32 gEventSeen;
DATA(0x004b2214)
i32 gPurchaseNum;
DATA(0x004c8ccc)
i32 gPurchaseSlot;
DATA(0x004ca17c)
armyGroup* gEventTownArmy;
DATA(0x004bb158)
i32 gDefaultEventType;
DATA(0x004c8cbc)
mapCell* gEventCell;
DATA(0x004b4b98)
i32 gReduceByReload;
DATA(0x004ca174)
i32 gReduceByBerserk;
DATA(0x004ca184)
town* gEventTown;
DATA(0x004b2208)
i32 gEventRV;
DATA(0x004bb15c)
i32 gMonsterCount;
DATA(0x004b2210)
i32 gTownValue;
DATA(0x004b4ba4)
hero* gEventHero;

// Buka retail preserves the explicit Boolean and floating-point evaluation
// order below. The inherited VC4 reassociation verdict does not apply to VC6.
// donor PoL RVA 0x00043fc4; preferred Buka symbol ?ValueOfEventAtPosition@philAI@@QAEHHHHPAH@Z
// donor Buka TU SOURCE/PHILAI; HoMM1 owner inferred from contiguous order
// evidence: graph:4;base=0.465517;margin=0.659381;shape=0.256;size=0.790;calls=0.952;alternate=pol20:int philAI::ValueOfEventAtPosition(int, int, int, int *)@0x00043fc4
VA(0x0044efb4, 0x1d37)
i32 philAI::ValueOfEventAtPosition(hero* pHero, i16 x, i16 y, i32 immediate, i32* liveChance) {
    DATA(0x0049ef80)
    static i32 gEvaluatingTravelGates = 1;
    i32 numToBuy;
    i32 won9Requested;
    i32 theList[RESOURCE_COUNT];
    i32 guardCount1Value;
    i32 origExitRV5;
    mapCell* loc;
    i32 gateY28Val;
    i32 origBestRV;
    i32 gateX1Val;
    i32 tempChance;
    i32 nextGoldCost;
    i32 theArmySlot2;
    i32 lastPositionValue;
    i32 newPrize5;
    i32 bBattleWon9;
    i32 chosenExitY27Value;
    i32 prevChosenExitX0;

    if (!immediate && gaiHeroEventStratRVOfPos[x][y] != RV_UNSET)
        return gaiHeroEventStratRVOfPos[x][y];
    gReduceByReload = 1;
    gReduceByBerserk = 1;
    *liveChance = 100;
    gEventRV = 0;
    gEventCell = gpAdvManager->GetCell(x, y);
    gEventSeen = mapVisited[x][y] && giCurPlayerBit;
    switch (gEventCell->m_triggerType & MAP_TRIGGER_TYPE_MASK) {
        case MAP_OBJECT_ARTIFACT:
            gArtifactChoice1 = gArtifactBaseRV[gEventCell->m_objectIndex];
            for (gEventLoop = 0; gEventLoop < ARMY_GROUP_SLOT_COUNT; gEventLoop++) {
                gpMonGroup->m_creatureTypes[gEventLoop] = CREATURE_ROGUE;
                gpMonGroup->m_creatureCounts[gEventLoop] = 10;
            }
            ProbableOutcomeOfBattle(
                &pHero->m_army,
                pHero,
                gpMonGroup,
                NULL,
                NULL,
                0,
                0,
                GAME_PLAYER_NONE,
                gWinChance,
                gAttackerLoss,
                gDefenderLoss,
                gAttackerRemaining,
                gDefenderRemaining,
                gOutcome
            );
            gArtifactChoice2 = static_cast<i32>(
                gOutcome + gArtifactBaseRV[gEventCell->m_objectIndex] * gWinChance
            );
            gArtifactChoice3 = static_cast<i32>(
                gArtifactBaseRV[gEventCell->m_objectIndex]
                - 2000.0f * gafAITurnCostResource[RESOURCE_GOLD]
            );
            if (gArtifactChoice3 < 0)
                gArtifactChoice3 = 0;
            if (gEventSeen) {
                switch (gEventCell->m_objectMetadata) {
                    case ARTIFACT_EVENT_MODE_PICKUP:
                        gEventRV = gArtifactChoice1;
                        break;
                    case ARTIFACT_EVENT_MODE_GUARDED:
                        gEventRV = gArtifactChoice2;
                        break;
                    case ARTIFACT_EVENT_MODE_GOLD:
                        gEventRV = gArtifactChoice3;
                        break;
                }
            } else {
                gEventRV = static_cast<i32>(
                    gArtifactChoice1 * 0.6 + gArtifactChoice2 * 0.2 + gArtifactChoice3 * 0.2
                );
            }
            break;
        case 1:
        case MAP_OBJECT_MINE:
        case MAP_OBJECT_SAWMILL:
            if (gpGame->m_mineOwners[gEventCell->m_objectMetadata] == pHero->m_owner) {
                gEventRV = 0;
            } else if (gbIAmGreatest && gpGame->m_mineOwners[gEventCell->m_objectMetadata] >= 0
                       && !gbHumanPlayer[gpGame->m_mineOwners[gEventCell->m_objectMetadata]]) {
                gEventRV = 0;
            } else {
                gEventRV = static_cast<i32>(
                    gMineIncome[gpGame->m_mines[gEventCell->m_objectMetadata].type]
                    * gafAITurnCostResource[gpGame->m_mines[gEventCell->m_objectMetadata].type]
                    * gaiTurnValueOfMine[x][y]
                );
                if (gpGame->m_mineOwners[gEventCell->m_objectMetadata] >= 0)
                    gEventRV = static_cast<i32>(
                        gEventRV
                        * (gbHumanPlayer[gpGame->m_mineOwners[gEventCell->m_objectMetadata]]
                               ? gAttackHumanBonus
                               : gAttackComputerBonus)
                    );
            }
            break;
        case MAP_OBJECT_OBELISK:
            if (gpGame->m_obeliskVisitors[gEventCell->m_objectMetadata - 1] & giCurPlayerBit)
                gEventRV = 0;
            else
                gEventRV = gpCurPlayer->m_aiData.m_obeliskValue;
            break;
        case MAP_OBJECT_MONSTER:
            gMonsterCount = gEventCell->m_objectMetadata & MONSTER_COUNT_MASK;
            CLEAR_ARMY_GROUP(*gpMonGroup);
            if (gMonsterCount / ARMY_GROUP_SLOT_COUNT > 0) {
                for (gEventLoop = 0; gEventLoop < ARMY_GROUP_SLOT_COUNT; gEventLoop++) {
                    gpMonGroup->m_creatureTypes[gEventLoop] = gEventCell->m_objectIndex;
                    gpMonGroup->m_creatureCounts[gEventLoop] =
                        gMonsterCount / ARMY_GROUP_SLOT_COUNT;
                }
            }
            for (gEventLoop = gMonsterCount % ARMY_GROUP_SLOT_COUNT - 1; gEventLoop >= 0;
                 gEventLoop--) {
                gpMonGroup->m_creatureTypes[gEventLoop] = gEventCell->m_objectIndex;
                gpMonGroup->m_creatureCounts[gEventLoop]++;
            }
            ProbableOutcomeOfBattle(
                &pHero->m_army,
                pHero,
                gpMonGroup,
                NULL,
                NULL,
                0,
                0,
                GAME_PLAYER_NONE,
                gWinChance,
                gAttackerLoss,
                gDefenderLoss,
                gAttackerRemaining,
                gDefenderRemaining,
                gOutcome
            );
            EvaluateOneTimeCreaturePurchase(
                pHero,
                gEventCell->m_objectIndex,
                gMonsterCount,
                1,
                numToBuy,
                gAttackerLoss,
                theArmySlot2
            );
            if ((gEventCell->m_objectMetadata & MONSTER_WILLING_FLAG)
                && gpPhilAI->FightValueOfStack(&pHero->m_army, pHero, 0, 0, 0)
                       > (gEventCell->m_objectMetadata & MONSTER_COUNT_MASK)
                             * gMonsterDatabase[gEventCell->m_objectIndex].fightValue * 1.75) {
                *liveChance = 100;
                *liveChance = static_cast<i32>(gWinChance * 60.0f + 40.0f);
                if (pHero->m_army.CanJoin(gEventCell->m_objectIndex))
                    gEventRV = gAttackerLoss;
                else
                    gEventRV = 0;
                gEventRV = static_cast<i32>(gEventRV * 0.6 + gOutcome * 0.4);
            } else {
                *liveChance = static_cast<i32>(gWinChance * 100.0f);
                gEventRV = gOutcome;
            }
            if (gEventRV < 0)
                gReduceByReload = 0;
            break;
        case MAP_OBJECT_HERO:
            if (gpGame->m_availableHeroes[gEventCell->m_objectMetadata] == pHero->m_owner) {
                gaiHeroLiveChance[gEventCell->m_objectMetadata] = 100;
                if (!immediate || gbTroopReload)
                    gEventRV = 0;
                else
                    gEventRV = -5000;
                *liveChance = 0;
            } else if (gbIAmGreatest
                       && !gbHumanPlayer[gpGame->m_availableHeroes[gEventCell->m_objectMetadata]]) {
                gEventRV = 0;
                *liveChance = 100;
            } else {
                gTownValue = 0;
                gEventTown = NULL;
                gEventTownArmy = NULL;
                gEventHero = gpGame->GetHero(gEventCell->m_objectMetadata);
                if (gEventHero->m_locationType == (MAP_TRIGGER_EVENT | MAP_OBJECT_TOWN)) {
                    gEventTown = gpGame->GetTown(gEventHero->m_occupiedTown);
                    gEventTownArmy = &gEventTown->m_army;
                    gTownValue = ValueOfTown(gEventTown);
                    gEventTownId = gEventTown->m_id;
                    if (gEventTown->m_owner >= 0)
                        gTownValue = static_cast<i32>(
                            gTownValue
                            * ((5 - gpGame->m_playerCount) * 0.25
                                   * (gbHumanPlayer[gEventTown->m_owner] ? gAttackHumanBonus
                                                                         : gAttackComputerBonus)
                               + 1.0)
                        );
                }
                if (immediate && giDebugLevel == 5 && x == 15)
                    giDebugLevel = 9;
                ProbableOutcomeOfBattle(
                    &pHero->m_army,
                    pHero,
                    &gEventHero->m_army,
                    gEventHero,
                    gEventTownArmy,
                    gEventTownArmy != NULL,
                    gEventTownId,
                    gEventHero->m_owner,
                    gWinChance,
                    gAttackerLoss,
                    gDefenderLoss,
                    gAttackerRemaining,
                    gDefenderRemaining,
                    gEventRV
                );
                if (immediate && giDebugLevel == 9)
                    giDebugLevel = 5;
                *liveChance = static_cast<i32>(gWinChance * 100.0f);
                if (gTownValue > 0)
                    gEventRV = static_cast<i32>(gEventRV + gTownValue * gWinChance);
                if (immediate && gbHumanPlayer[gEventHero->m_owner] && gEventRV > 200)
                    gEventRV = static_cast<i32>(gEventRV * 1.5);
                if (gWinChance > 0.75)
                    gaiHeroLiveChance[gEventCell->m_objectMetadata] = 100;
                else if (gWinChance > 0.5)
                    gaiHeroLiveChance[gEventCell->m_objectMetadata] =
                        static_cast<i16>(gWinChance * 136.0f);
                else if (gWinChance > 0.4)
                    gaiHeroLiveChance[gEventCell->m_objectMetadata] =
                        static_cast<i16>(gWinChance * 130.0f);
                else if (gWinChance > 0.3)
                    gaiHeroLiveChance[gEventCell->m_objectMetadata] =
                        static_cast<i16>(gWinChance * 125.0f);
                else if (gWinChance > 0.2)
                    gaiHeroLiveChance[gEventCell->m_objectMetadata] =
                        static_cast<i16>(gWinChance * 113.0f);
                else
                    gaiHeroLiveChance[gEventCell->m_objectMetadata] =
                        static_cast<i16>(gWinChance * 100.0f);
                if (gaiHeroLiveChance[gEventCell->m_objectMetadata] > 100)
                    gaiHeroLiveChance[gEventCell->m_objectMetadata] = 100;
                if (!immediate && gWinChance < 0.4)
                    gEventRV = static_cast<i32>(gEventRV * (3.0f - gWinChance * 2.0f));
                if (!immediate && gWinChance < 0.2)
                    gEventRV = static_cast<i32>(gEventRV * (2.0f - gWinChance * 2.0f));
                if (gEventRV < 0)
                    gReduceByReload = 0;
                gReduceByBerserk = 0;
            }
            break;
        case MAP_OBJECT_TOWN:
            gEventTown = gpGame->GetTown(gEventCell->m_objectMetadata);
            if (gpGame->m_townOwners[gEventCell->m_objectMetadata] == pHero->m_owner) {
                if (gEventTown->m_occupyingHeroId != TOWN_OCCUPYING_HERO_NONE) {
                    gEventRV = 0;
                } else {
                    gEventRV = 0;
                    HeroInteractionAtTown(pHero, gEventTown, 1, &gEventRV);
                    gEventRV = static_cast<i32>(gEventRV * gfHeroInteractionBonus[pHero->m_id]);
                }
                gReduceByReload = 0;
            } else if (gbIAmGreatest && gpGame->m_townOwners[gEventCell->m_objectMetadata] >= 0
                       && !gbHumanPlayer[gpGame->m_townOwners[gEventCell->m_objectMetadata]]) {
                gEventRV = 0;
            } else {
                gTownValue = ValueOfTown(gEventTown);
                if (immediate && giDebugLevel == 5 && x == 15)
                    giDebugLevel = 9;
                if (gpGame->GetTown(gEventCell->m_objectMetadata)->m_occupyingHeroId
                    != TOWN_OCCUPYING_HERO_NONE)
                    ProbableOutcomeOfBattle(
                        &pHero->m_army,
                        pHero,
                        &gpGame->m_heroRecs[gEventTown->m_occupyingHeroId].m_army,
                        &gpGame->m_heroRecs[gEventTown->m_occupyingHeroId],
                        &gEventTown->m_army,
                        1,
                        gEventCell->m_objectMetadata,
                        gEventTown->m_owner,
                        gWinChance,
                        gAttackerLoss,
                        gDefenderLoss,
                        gAttackerRemaining,
                        gDefenderRemaining,
                        gOutcome
                    );
                else if (gEventTown->HasGarrison())
                    ProbableOutcomeOfBattle(
                        &pHero->m_army,
                        pHero,
                        &gEventTown->m_army,
                        NULL,
                        NULL,
                        1,
                        gEventCell->m_objectMetadata,
                        gEventTown->m_owner,
                        gWinChance,
                        gAttackerLoss,
                        gDefenderLoss,
                        gAttackerRemaining,
                        gDefenderRemaining,
                        gOutcome
                    );
                else {
                    gWinChance = 1.0f;
                    gOutcome = 0;
                }
                *liveChance = static_cast<i32>(gWinChance * 100.0f);
                if (immediate && giDebugLevel == 9)
                    giDebugLevel = 5;
                if (gEventTown->m_owner >= 0)
                    gTownValue = static_cast<i32>(
                        gTownValue
                        * (((5 - gpGame->m_playerCount) * 0.25 + 0.9)
                           * (gbHumanPlayer[gEventTown->m_owner] ? gAttackHumanBonus
                                                                 : gAttackComputerBonus))
                    );
                gEventRV = static_cast<i32>(gTownValue * gWinChance + gOutcome);
                if (gpGame->m_townOwners[gEventCell->m_objectMetadata] != GAME_PLAYER_NONE)
                    gReduceByBerserk = 0;
            }
            break;
        case MAP_OBJECT_DAEMON_CAVE:
            if (gEventCell->m_objectMetadata == DAEMON_CAVE_EMPTY) {
                gEventRV = 0;
            } else {
                gEventRV = static_cast<i32>(
                    pHero->m_aiFightValue * 0.3 * 1000.0
                    + (pHero->m_aiFightValue * 0.1 * 1000.0 + gpCurPlayer->m_aiData.m_artifactValue)
                    + (pHero->m_aiFightValue * 0.3 * 1000.0
                       + 2500.0f * gafAITurnCostResource[RESOURCE_GOLD])
                    + gafAITurnCostResource[RESOURCE_GOLD] * -750.0
                );
                if (gEventCell->m_objectMetadata == DAEMON_REWARD_RANSOM
                    && gpCurPlayer->m_resources[RESOURCE_GOLD] < DAEMON_GOLD)
                    gEventRV = -100;
            }
            break;
        case MAP_OBJECT_OASIS:
            if (!(pHero->m_eventFlags & HERO_EVENT_OASIS))
                gEventRV = static_cast<i32>(200.0f * pHero->m_aiFightValue);
            break;
        case MAP_OBJECT_BUOY:
            if (!(pHero->m_eventFlags & HERO_EVENT_BUOY))
                gEventRV = static_cast<i32>(200.0f * pHero->m_aiFightValue);
            break;
        case MAP_OBJECT_STATUE:
            if (!(pHero->m_eventFlags & HERO_EVENT_STATUE))
                gEventRV = static_cast<i32>(400.0f * pHero->m_aiFightValue);
            break;
        case MAP_OBJECT_FAERIE_RING:
            if (!(pHero->m_eventFlags & HERO_EVENT_FAERIE_RING))
                gEventRV = static_cast<i32>(200.0f * pHero->m_aiFightValue);
            break;
        case MAP_OBJECT_FOUNTAIN:
            if (!(pHero->m_eventFlags & HERO_EVENT_FOUNTAIN))
                gEventRV = static_cast<i32>(200.0f * pHero->m_aiFightValue);
            break;
        case MAP_OBJECT_TREASURE_CHEST:
            gEventRV = static_cast<i32>(1500.0f * gafAITurnCostResource[RESOURCE_GOLD]);
            break;
        case MAP_OBJECT_CAMPFIRE:
            gEventRV = static_cast<i32>(
                500.0f * gafAITurnCostResource[RESOURCE_GOLD]
                + (gafAITurnCostResource[RESOURCE_WOOD] + gafAITurnCostResource[RESOURCE_ORE]
                   + gafAITurnCostResource[RESOURCE_CRYSTAL]
                   + gafAITurnCostResource[RESOURCE_SULFUR]
                   + gafAITurnCostResource[RESOURCE_MERCURY] + gafAITurnCostResource[RESOURCE_GEMS])
                      / 6.0f * 5.0f
            );
            break;
        case MAP_OBJECT_SPELL_SHRINE:
            if (pHero->m_primaryStats[HERO_PRIMARY_KNOWLEDGE] > 0
                && pHero->HasArtifact(ARTIFACT_MAGIC_BOOK)) {
                gEventLoop = pHero->AddSpell(
                    gEventCell->m_objectMetadata - 1,
                    pHero->m_primaryStats[HERO_PRIMARY_KNOWLEDGE],
                    1
                );
                gEventRV = gSpellAIValue[gEventCell->m_objectMetadata - 1];
                gEventRV = static_cast<i32>(
                    gEventRV
                    * StatChangeValue(
                        pHero->m_primaryStats[HERO_PRIMARY_KNOWLEDGE] - gEventLoop,
                        pHero->m_primaryStats[HERO_PRIMARY_KNOWLEDGE]
                    )
                );
                if (gSpellAIFlags[gEventCell->m_objectMetadata - 1]
                    & SPELL_AI_FLAG_SCALES_WITH_POWER)
                    gEventRV = static_cast<i32>(
                        gEventRV
                        * (pHero->m_primaryStats[HERO_PRIMARY_KNOWLEDGE] <= 40
                               ? gStatPower[pHero->m_primaryStats[HERO_PRIMARY_KNOWLEDGE]]
                               : gStatPower[40])
                    );
            } else {
                gEventRV = 0;
            }
            break;
        case MAP_OBJECT_GAZEBO:
            if (pHero->m_visitedSites & (1 << gEventCell->m_objectMetadata))
                gEventRV = 0;
            else
                gEventRV = static_cast<i32>(pHero->m_aiFightValue * 1000.0f);
            break;
        case MAP_OBJECT_LIGHTHOUSE:
            if (gpGame->m_mines[1].owner == pHero->m_owner)
                gEventRV = 0;
            else
                gEventRV = 1000;
            break;
        case MAP_OBJECT_RESOURCE:
            switch (gEventCell->m_objectIndex - RESOURCE_PILE_OBJECT_BASE) {
                case RESOURCE_GOLD:
                    gEventRV = static_cast<i32>(
                        gEventCell->m_objectMetadata * RESOURCE_PILE_GOLD_MULTIPLIER
                        * gafAITurnCostResource[RESOURCE_GOLD]
                    );
                    break;
                case RESOURCE_WOOD:
                    gEventRV = static_cast<i32>(
                        gEventCell->m_objectMetadata * gafAITurnCostResource[RESOURCE_WOOD]
                    );
                    break;
                case RESOURCE_ORE:
                    gEventRV = static_cast<i32>(
                        gEventCell->m_objectMetadata * gafAITurnCostResource[RESOURCE_ORE]
                    );
                    break;
                case RESOURCE_CRYSTAL:
                    gEventRV = static_cast<i32>(
                        gEventCell->m_objectMetadata * gafAITurnCostResource[RESOURCE_CRYSTAL]
                    );
                    break;
                case RESOURCE_SULFUR:
                    gEventRV = static_cast<i32>(
                        gEventCell->m_objectMetadata * gafAITurnCostResource[RESOURCE_SULFUR]
                    );
                    break;
                case RESOURCE_MERCURY:
                    gEventRV = static_cast<i32>(
                        gEventCell->m_objectMetadata * gafAITurnCostResource[RESOURCE_MERCURY]
                    );
                    break;
                case RESOURCE_GEMS:
                    gEventRV = static_cast<i32>(
                        gEventCell->m_objectMetadata * gafAITurnCostResource[RESOURCE_GEMS]
                    );
                    break;
            }
            break;
        case MAP_OBJECT_WINDMILL:
            if (gEventCell->m_objectMetadata == WINDMILL_EMPTY) {
                gEventRV = 0;
            } else {
                memset(theList, 0, sizeof(theList));
                theList[gEventCell->m_objectMetadata] = WINDMILL_RESOURCE_AMOUNT;
                gEventRV = RVConversion(theList);
            }
            break;
        case MAP_OBJECT_SKELETON:
            if (gEventCell->m_objectMetadata == SKELETON_EMPTY)
                gEventRV = 0;
            else
                gEventRV = static_cast<i32>(gpCurPlayer->m_aiData.m_artifactValue * 0.1);
            break;
        case MAP_OBJECT_ANCIENT_LAMP:
            EvaluateOneTimeCreaturePurchase(
                pHero,
                CREATURE_GENIE,
                gEventCell->m_objectMetadata,
                0,
                gPurchaseNum,
                gEventRV,
                gPurchaseSlot
            );
            gReduceByReload = 0;
            break;
        case MAP_OBJECT_STRAW_HUT:
            EvaluateOneTimeCreaturePurchase(
                pHero,
                CREATURE_GOBLIN,
                gEventCell->m_objectMetadata,
                1,
                gPurchaseNum,
                gEventRV,
                gPurchaseSlot
            );
            break;
        case MAP_OBJECT_HOUSE:
        case MAP_OBJECT_PEASANT_LOG_CABIN:
            EvaluateOneTimeCreaturePurchase(
                pHero,
                CREATURE_PEASANT,
                gEventCell->m_objectMetadata,
                1,
                gPurchaseNum,
                gEventRV,
                gPurchaseSlot
            );
            gReduceByReload = 0;
            break;
        case MAP_OBJECT_CABIN:
            EvaluateOneTimeCreaturePurchase(
                pHero,
                CREATURE_ARCHER,
                gEventCell->m_objectMetadata,
                1,
                gPurchaseNum,
                gEventRV,
                gPurchaseSlot
            );
            gReduceByReload = 0;
            break;
        case MAP_OBJECT_DWARF_LOG_CABIN:
            EvaluateOneTimeCreaturePurchase(
                pHero,
                CREATURE_DWARF,
                gEventCell->m_objectMetadata,
                1,
                gPurchaseNum,
                gEventRV,
                gPurchaseSlot
            );
            gReduceByReload = 0;
            break;
        case MAP_OBJECT_DESERT_TENT:
            EvaluateOneTimeCreaturePurchase(
                pHero,
                CREATURE_NOMAD,
                gEventCell->m_objectMetadata,
                0,
                gPurchaseNum,
                gEventRV,
                gPurchaseSlot
            );
            gReduceByReload = 0;
            break;
        case MAP_OBJECT_WAGON_CAMP:
            EvaluateOneTimeCreaturePurchase(
                pHero,
                CREATURE_ROGUE,
                gEventCell->m_objectMetadata,
                0,
                gPurchaseNum,
                gEventRV,
                gPurchaseSlot
            );
            gReduceByReload = 0;
            break;
        case MAP_OBJECT_GRAVEYARD:
        case MAP_OBJECT_SHIPWRECK:
            if (gEventCell->m_objectMetadata == GHOST_SITE_EMPTY) {
                gEventRV = 0;
            } else {
                switch (gEventCell->m_objectMetadata) {
                    case GHOST_SITE_SMALL:
                        guardCount1Value = 2;
                        nextGoldCost = GHOST_SMALL_GOLD;
                        break;
                    case GHOST_SITE_MEDIUM:
                        guardCount1Value = 3;
                        nextGoldCost = GHOST_MEDIUM_GOLD;
                        break;
                    case GHOST_SITE_LARGE:
                        guardCount1Value = 5;
                        nextGoldCost = GHOST_LARGE_GOLD;
                        break;
                    case GHOST_SITE_HUGE:
                        guardCount1Value = 10;
                        nextGoldCost = GHOST_HUGE_GOLD;
                        break;
                }
                for (gEventLoop = 0; gEventLoop < ARMY_GROUP_SLOT_COUNT; gEventLoop++) {
                    gpMonGroup->m_creatureTypes[gEventLoop] = CREATURE_GHOST;
                    gpMonGroup->m_creatureCounts[gEventLoop] = guardCount1Value;
                }
                ChooseEvaluateBattle(
                    &pHero->m_army,
                    pHero,
                    gpMonGroup,
                    NULL,
                    0,
                    0,
                    static_cast<i32>(
                        nextGoldCost * gafAITurnCostResource[RESOURCE_GOLD]
                        + (gEventCell->m_objectMetadata == GHOST_SITE_HUGE
                               ? gpCurPlayer->m_aiData.m_artifactValue
                               : 0)
                    ),
                    won9Requested,
                    gEventRV
                );
            }
            break;
        case MAP_OBJECT_DRAGON_CITY:
            newPrize5 = static_cast<i32>(
                1000.0f * gafAITurnCostResource[RESOURCE_GOLD] * gaiTurnValueOfMine[x][y] * 1.5
            );
            for (gEventLoop = 0; gEventLoop < ARMY_GROUP_SLOT_COUNT; gEventLoop++) {
                gpMonGroup->m_creatureTypes[gEventLoop] = CREATURE_DRAGON;
                gpMonGroup->m_creatureCounts[gEventLoop] = 1;
            }
            if (gpGame->m_mineOwners[0] == pHero->m_owner)
                gEventRV = 0;
            else if (gpGame->m_mineOwners[0] != GAME_PLAYER_NONE)
                ChooseEvaluateBattle(
                    &pHero->m_army,
                    pHero,
                    gpMonGroup,
                    NULL,
                    0,
                    0,
                    static_cast<i32>(
                        newPrize5
                        * (gpGame->m_players[gpGame->m_mineOwners[0]].m_aiData.m_artifactPoolShare
                           + 1.0)
                    ),
                    bBattleWon9,
                    gEventRV
                );
            else
                ChooseEvaluateBattle(
                    &pHero->m_army,
                    pHero,
                    gpMonGroup,
                    NULL,
                    0,
                    0,
                    newPrize5,
                    bBattleWon9,
                    gEventRV
                );
            break;
        case MAP_OBJECT_STONE_LITHS:
        case MAP_OBJECT_WHIRLPOOL:
            if (!gEvaluatingTravelGates) {
                gEventRV = 0;
                break;
            }
            gEvaluatingTravelGates = 0;
            origBestRV = -9999;
            for (gateY28Val = 0; gateY28Val < MAP_CELL_GRID_SIZE; gateY28Val++) {
                for (gateX1Val = 0; gateX1Val < MAP_CELL_GRID_SIZE; gateX1Val++) {
                    loc = gpAdvManager->GetCell(gateX1Val, gateY28Val);
                    if (MANHATTAN_LENGTH(gateX1Val - x, gateY28Val - y)
                            > ((gEventCell->m_triggerType & MAP_TRIGGER_TYPE_MASK)
                                       == MAP_OBJECT_STONE_LITHS
                                   ? 1
                                   : 3)
                        && loc->m_triggerType == gEventCell->m_triggerType) {
                        origExitRV5 =
                            StrategicValueOfPosition(pHero, gateX1Val, gateY28Val, 0, &tempChance);
                        origExitRV5 = static_cast<i32>(origExitRV5 * 0.85);
                        if (origExitRV5 > origBestRV) {
                            origBestRV = origExitRV5;
                            prevChosenExitX0 = gateX1Val;
                            chosenExitY27Value = gateY28Val;
                        }
                    }
                }
            }
            lastPositionValue =
                StrategicValueOfPosition(pHero, pHero->m_x, pHero->m_y, 0, &tempChance);
            if (origBestRV > lastPositionValue + 200)
                gEventRV = origBestRV - lastPositionValue - 200;
            else
                gEventRV = -200;
            gEvaluatingTravelGates = 1;
            gReduceByReload = 0;
            break;
        case MAP_OBJECT_WATERWHEEL:
            gEventRV = static_cast<i32>(
                gEventCell->m_objectMetadata * WATERWHEEL_GOLD_MULTIPLIER
                * gafAITurnCostResource[RESOURCE_GOLD]
            );
            break;
        case MAP_OBJECT_SHIP:
            gbActualBoatFound = 1;
            gEventRV = 100;
            break;
        case MAP_OBJECT_SIGNPOST:
        case MAP_OBJECT_RANKING_SHRINE:
            gEventRV = 0;
            break;
        case MAP_OBJECT_ROSEBUSH:
        case MAP_OBJECT_COAST:
        case MAP_OBJECT_TREE_STUMP:
        case MAP_OBJECT_OAK_TREE:
            gEventRV = 0;
            break;
        default:
            if (gpCurPlayer->m_ultimateArtifactHintChance > 15
                && gpCurPlayer->m_ultimateArtifactHintX == x
                && gpCurPlayer->m_ultimateArtifactHintY == y) {
                gEventRV =
                    gUltArtifactAvgValue * (gpCurPlayer->m_ultimateArtifactHintChance - 15) / 100;
            } else {
                gDefaultEventType = gEventCell->m_triggerType & MAP_TRIGGER_TYPE_MASK;
                if (gDefaultEventType >= MAP_OBJECT_NON_EVENT_FIRST
                    && gDefaultEventType <= MAP_OBJECT_TREES_LAST)
                    gEventRV = 0;
            }
            break;
    }
    if (gbTroopReload && gReduceByReload)
        gEventRV = static_cast<i32>(gEventRV * fReduceFactor);
    if (gbBerserk && gReduceByBerserk)
        gEventRV = static_cast<i32>(gEventRV * fBerserkFactor);
    if (!immediate) {
        if (gEventRV > 0 && (mapExtra[x][y] & MAP_EXTRA_MONSTER_ADJACENT)
            && (gEventCell->m_triggerType & MAP_TRIGGER_TYPE_MASK) != MAP_OBJECT_MONSTER)
            gEventRV = 0;
        if (gEventRV < 0 && (gEventCell->m_triggerType & MAP_TRIGGER_TYPE_MASK) != MAP_OBJECT_HERO)
            gEventRV = 0;
        else if (gEventRV > 32000)
            gEventRV = 32000;
        else if (gEventRV < -32000)
            gEventRV = -32000;
        gaiHeroEventStratRVOfPos[x][y] = gEventRV;
    }
    return gEventRV;
}

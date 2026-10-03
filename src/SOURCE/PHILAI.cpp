// Located from HoMM2 Buka 2.1; PoL 2.0 supplies the VC4 declaration.

#include <match.h>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <BASE/BITS.h>
#include <BASE/BMAP2.h>
#include <BASE/INPUTMGR_TYPES.h>
#include <BASE/Misc.h>
#include <BASE/MISC_TYPES.h>
#include <H1/All.h>
#include <SOURCE/KB.h>
#include <SOURCE/campaignTypes.h>
#include <SOURCE/EVENTS.h>
#include <SOURCE/highScoreRuntime.h>
#include <SOURCE/kbwin.h>
#include <SOURCE/mapObjectTypes.h>

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// KB owns gDwellingType (retail KB .data band).
extern signed char gDwellingType[4][6];

// PHILAI's module state in retail address order: .data 0x0048f54c-0x0048f827
// (shared with its logging helpers' literals), then .bss 0x004acec0-0x004c4eef
// (VC4 orders .bss by name hash, not by definition).
DATA(0x0048f54c)
signed char giShowComputerRoute = 0;
DATA(0x0048f55c)
float gfAttackHumanBonus = 2.0f;
DATA(0x0048f560)
float gfAttackComputerBonus = 0.8f;
DATA(0x0048f7b8)
signed char bSVSearchArrayInUse = 0;
DATA(0x0048f824)
int bEvaluatingTravelGates = 1;
DATA(0x004acec0)
short gaiHeroEventStratRVOfPos[MAP_CELL_GRID_SIZE][MAP_CELL_GRID_SIZE];
DATA(0x004af740)
float fBerserkFactor;
DATA(0x004af744)
int iLastFrameRateTimer;
DATA(0x004af748)
signed char giCurPlayer;
DATA(0x004af74c)
float fWinChance;
DATA(0x004af750)
int iEventLoop;
DATA(0x004af754)
signed char giBuildShipyard[GAME_PLAYER_COUNT];
DATA(0x004af758)
int giMaxHeroesForThisPlayer;
DATA(0x004af75c)
signed char giBuildBoat[GAME_PLAYER_COUNT];
DATA(0x004af760)
float fReduceFactor;
DATA(0x004af764)
unsigned char giCurPlayerBit;
DATA(0x004af768)
signed char giBestShipyardDist;
DATA(0x004af76c)
int bHeroBuiltThisTurn;
DATA(0x004af770)
short gaiHeroLiveChance[GAME_HERO_COUNT];
DATA(0x004af7b8)
int iAttackerLoss;
DATA(0x004af7bc)
int iDefenderLoss;
DATA(0x004af7c8)
int giHumanTownConquered;
DATA(0x004af7dc)
int giCurTurn;
DATA(0x004af7e8)
int costTemp[RESOURCE_COUNT];
DATA(0x004af808)
signed char gaiTurnValueOfMine[MAP_CELL_GRID_SIZE][MAP_CELL_GRID_SIZE];
DATA(0x004b0c48)
int iDummy;
DATA(0x004b0c4c)
signed char gbPossibleShipyardFound;
DATA(0x004be7b0)
float gafAITurnCostResource[RESOURCE_COUNT];
DATA(0x004be7cc)
unsigned char giCurWatchPlayerHighBit;
DATA(0x004be7d0)
int iCurPlaceToVisit;
DATA(0x004be7dc)
signed char giBestShipyardId;
DATA(0x004be7e0)
signed char mapVisited[MAP_CELL_GRID_SIZE][MAP_CELL_GRID_SIZE];
DATA(0x004bfc28)
short gaiHeroStrategicRVOfPos[MAP_CELL_GRID_SIZE][MAP_CELL_GRID_SIZE];
DATA(0x004c24a8)
signed char gbActualBoatFound;
DATA(0x004c24ac)
unsigned char giCurWatchPlayerBit;
DATA(0x004c24b0)
playerData* gpCurPlayer;
DATA(0x004c24b8)
float gfHeroInteractionBonus[GAME_HERO_COUNT];
DATA(0x004c2548)
int gbBerserk;
DATA(0x004c255c)
unsigned char giCurPlayerHighBit;
DATA(0x004c2560)
short gaiLiveChanceOfPos[MAP_CELL_GRID_SIZE][MAP_CELL_GRID_SIZE];
DATA(0x004c4de0)
signed char giBuildBoatStuffTurn[GAME_PLAYER_COUNT];
DATA(0x004c4de8)
int iPlacesVisited[ADVMGR_PLACE_VISIT_COUNT][ADVMGR_PLACE_COORDINATE_COUNT];
DATA(0x004c4ee4)
int gbTroopReload;
DATA(0x004c4eec)
signed char gbActualShipyardFound;

// Buka 2.1's named AI factors. They are loaded, not folded, at /Od, and
// retail .rdata keeps them in this declaration order at 0x0048c0a8 ahead of
// the anonymous float literals.
DATA(0x0048c0a8)
static const float AI_TARGET_HUMAN_VALUE_FACTOR = 1.5f;
DATA(0x0048c0ac)
static const float AI_STRATEGIC_POSITION_SCORE_FACTOR = 1.25f;
DATA(0x0048c0b0)
static const float AI_CREATURE_SAME_RACE_FACTOR = 1.1f;
DATA(0x0048c0b4)
static const float AI_FUTURE_DEFLATION_RATE = 0.15f;
DATA(0x0048c0b8)
static const float AI_HERO_PURCHASE_SAME_RACE_FACTOR = 1.12f;
DATA(0x0048c0bc)
static const float AI_ATTENTION_IDENTITY_FLOAT = 1.0f;
DATA(0x0048c0c0)
static const float AI_ATTENTION_IDENTITY = 1.0f;

// Misc's logging helpers open this object in retail (0x004199a5..0x00419f15),
// directly after the SVSearchArray initializer wrapper _$E2 at 0x00419990.
// HoMM1's retained logging path opens KB.LOG afresh and writes its banner.
VA(0x004199a5, 0x79)
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
VA(0x00419a1e, 0xa6)
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
VA(0x00419ac4, 0x86)
void LogInt(char* label, int value) {
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
VA(0x00419b4a, 0x8a)
void LogStr(char* label, long value1, long value2) {
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
VA(0x00419bd4, 0xab)
void LogStr(char* label, long value1, long value2, long value3, long value4, long value5) {
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
VA(0x00419c7f, 0xb3)
void LogStr(
    char* label,
    long value1,
    long value2,
    long value3,
    long value4,
    long value5,
    long value6,
    long value7
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
VA(0x00419d32, 0x1f)
void AiPrint(char* text) {
    gpPhilAI->ShowDebugText(text);
}

// @dead-code
// Zero-ref: no incoming call, jump or relocated reference in retail.
VA(0x00419d51, 0x4e)
void AbsAiPrint(char* text) {
    int saved;

    if (giDebugLevel == 0)
        return;
    saved = giDebugLevel;
    giDebugLevel = MISC_FORCED_DEBUG_LEVEL;
    gpPhilAI->ShowDebugText(text);
    giDebugLevel = saved;
}

// philAI.h: the AI strategic-value maps philAI::DoAI resets through ResetHeroRVs.

// Buka ResetHeroRVs; HoMM1 has no off-map guard and indexes [x][y].
VA(0x00419d9f, 0x177)
void ResetHeroRVs(int resetAll, int x, int y) {
    int i;
    int j;

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
            || MANHATTAN_LENGTH(y - gpGame->m_heroRecs[i].m_x, x - gpGame->m_heroRecs[i].m_x) < 10)
            gaiHeroLiveChance[i] = RV_UNSET;
    }
}

// donor PoL RVA 0x000379d0; preferred Buka symbol ?CheckDoMain@@YIXHH@Z
// donor Buka TU SOURCE/PHILAI; HoMM1 owner inferred from contiguous order
// evidence: graph:5;base=0.463287;margin=0.627999;shape=0.348;size=0.713;calls=0.909;alternate=pol20:void CheckDoMain(int, int)@0x000379d0
VA(0x00419f16, 0x1ef)
void CheckDoMain(int, int doMain) {
    if (KBTickCount() > iLastFrameRateTimer + 15
        || KBTickCount() > glTimers[ADVENTURE_FRAME_TIMER_SLOT]) {
        Process1WindowsMessage();
        PollSound();
        if (KBTickCount() > glTimers[ADVENTURE_FRAME_TIMER_SLOT]) {
            if (doMain == 0) {
                int oldShowIt = bShowIt;
                int oldX = gpAdvManager->m_previousOriginX;
                int oldY = gpAdvManager->m_previousOriginY;
                gbDrawSavedCursor = 1;
                if (gConfig.blackoutComputer == 0 && gbRemoteOn == 0)
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
                gbDrawSavedCursor = 0;
                bSpecialHideCursor = 0;
                gpAdvManager->m_previousOriginX = oldX;
                gpAdvManager->m_previousOriginY = oldY;
            }
            glTimers[ADVENTURE_FRAME_TIMER_SLOT] = KBTickCount() + TIMER_DELAY;
        } else if (gpMouseManager->m_mouseX - gpMouseManager->m_hotspotX != gpMouseManager->m_drawnX
                   || gpMouseManager->m_mouseY - gpMouseManager->m_hotspotY
                          != gpMouseManager->m_drawnY) {
            gpMouseManager->MovePointer(gpMouseManager->m_mouseX, gpMouseManager->m_mouseY);
        }
        iLastFrameRateTimer = KBTickCount();
    }
}

// Both donors retain this intentionally empty status hook; retail has no side effects.
VA(0x0041a105, 0x10)
void ShowStatus() {}

// HoMM1-only AI status line drawn with philAI's debug font across the bottom
// twenty screen rows; retail gates it on the second debug level.
VA(0x0041a115, 0x8c)
void philAI::ShowDebugText(char* text) {
    if (giDebugLevel >= 2) {
        FillBitmapArea(gpWindowManager->m_screen, 0, 460, LOGICAL_SCREEN_WIDTH, 20, 0);
        m_debugFont->DrawBoundedString(text, 0, 464, LOGICAL_SCREEN_WIDTH, 16, 1, FONT_ALIGN_LEFT);
        BlitBitmapToScreen(gpWindowManager->m_screen, 0, 460, LOGICAL_SCREEN_WIDTH, 20, 0, 460);
    }
}

// Preferred Buka's three build arrays, plus HoMM1's surviving debug-font owner.
VA(0x0041a1a1, 0x5e)
philAI::philAI() {
    int i;

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
VA(0x0041a1ff, 0xb0)
void philAI::DoAllHeroInteractions(void) {
    int i;

    for (i = 0; i < gpCurPlayer->m_townCount; i++) {
        town* pTown = gpGame->GetTown(gpCurPlayer->m_townIds[i]);
        if (pTown->m_occupyingHeroId != TOWN_OCCUPYING_HERO_NONE)
            HeroInteractionAtTown(gpGame->GetHero(pTown->m_occupyingHeroId), pTown, 0, &iDummy);
    }
}

// donor PoL RVA 0x00037fdf; preferred Buka symbol ?CheckBuyStuff@philAI@@QAEXXZ
// donor Buka TU SOURCE/PHILAI; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.641449;margin=0.572995;shape=0.312;size=0.907;calls=0.824;strings=CheckBuy End  |CheckBuy Start;alternate=pol20:void philAI::CheckBuyStuff(void)@0x00037fdf
VA(0x0041a2af, 0x445)
void philAI::CheckBuyStuff(void) {
    int done = 0;
    int boughtSomething = 0;
    BHC bestBHC;
    town* dockTown;

    gpGame->CheckHeroConsistency();
    if (gpCurPlayer->m_resources[RESOURCE_GOLD] < 200)
        return;
    LogInt("CheckBuy Start", gpCurPlayer->m_resources[RESOURCE_GOLD]);
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
        giBuildBoat[giCurPlayer] = giBuildShipyard[giCurPlayer];
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
        GetBestBHC(giCurPlayer, bestBHC);
        if (bestBHC.type >= PURCHASE_FIRST && CanBuyBHC(bestBHC)) {
            switch (bestBHC.type) {
                case PURCHASE_BUILDING:
                    BuildBuilding(bestBHC.pTown, bestBHC.what);
                    break;
                case PURCHASE_HERO:
                    BuildHero(bestBHC.pTown, bestBHC.what);
                    break;
                case PURCHASE_CREATURE:
                    BuildCreature(bestBHC.pTown, bestBHC.what, bestBHC.num);
                    break;
            }
            boughtSomething = 1;
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
    LogInt("CheckBuy End  ", gpCurPlayer->m_resources[RESOURCE_GOLD]);
}

// donor PoL RVA 0x0003849d; preferred Buka symbol ?GoodAdjacent@philAI@@QAEHPAH@Z
// donor Buka TU SOURCE/PHILAI; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.410367;margin=0.413392;shape=0.366;size=0.596;calls=0.600;alternate=pol20:int philAI::GoodAdjacent(int *)@0x0003849d
VA(0x0041a6f4, 0x1a9)
int philAI::GoodAdjacent(hero* pHero, int* direction) {
    int bestDirection;
    int dirIndex;
    int x;
    int y;
    int value;
    int maxValue;
    int iChance;

    bestDirection = -1;
    maxValue = 100;
    if ((gpAdvManager->GetCell(pHero->m_x, pHero->m_y)->m_triggerType & MAP_TRIGGER_TYPE_MASK)
        == MAP_OBJECT_STONE_LITHS)
        return 0;
    for (dirIndex = 0; dirIndex < MAP_DIRECTION_COUNT; dirIndex++) {
        if (gpAdvManager->ValidMoveWithEvent(pHero, dirIndex)) {
            x = normalDirTable[dirIndex].x + pHero->m_x;
            y = normalDirTable[dirIndex].y + pHero->m_y;
            if ((gpAdvManager->GetCell(x, y)->m_triggerType & MAP_TRIGGER_EVENT)
                && !(mapExtra[x][y] & MAP_EXTRA_MONSTER_ADJACENT)
                && (gpAdvManager->GetCell(x, y)->m_triggerType & MAP_TRIGGER_TYPE_MASK)
                       != MAP_OBJECT_STONE_LITHS
                && (gpAdvManager->GetCell(x, y)->m_triggerType & MAP_TRIGGER_TYPE_MASK)
                       != MAP_OBJECT_WHIRLPOOL) {
                value = ValueOfEventAtPosition(pHero, x, y, 2, &iChance);
                if (iChance > 80 && value > maxValue) {
                    maxValue = value;
                    bestDirection = dirIndex;
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
VA(0x0041a89d, 0x473)
void philAI::CheckReload(hero* pHero) {
    int mapY;
    mapCell* visitedCell;
    int heroFightValue;
    int mapX;
    int enemy;
    float enemyPressure;
    float friendly;

    gbTroopReload = 0;
    fReduceFactor = 1.0f;
    friendly = 0.0f;
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
                visitedCell = gpAdvManager->GetCell(mapX, mapY);
                switch (visitedCell->m_triggerType) {
                    case MAP_TRIGGER_EVENT | MAP_OBJECT_TOWN:
                        enemy = FightValueOfStack(
                            &gpGame->GetTown(visitedCell->m_objectMetadata)->m_army,
                            NULL,
                            0,
                            0,
                            0
                        );
                        if (gpGame->m_townOwners[visitedCell->m_objectMetadata] == pHero->m_owner) {
                            if (enemy > heroFightValue * 2)
                                friendly +=
                                    (static_cast<float>(enemy) / (heroFightValue * 2) - 1.0f)
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
                        if (gpGame->m_availableHeroes[visitedCell->m_objectMetadata]
                            != pHero->m_owner) {
                            enemy = FightValueOfStack(
                                &gpGame->GetHero(visitedCell->m_objectMetadata)->m_army,
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
    if (friendly > 1.0f && enemyPressure > 1.0f) {
        fReduceFactor = 3.0f / (friendly + enemyPressure + 2.0f);
        gbTroopReload = 1;
    }
}

// donor PoL RVA 0x00038c3d; preferred Buka symbol ?CheckBerserk@philAI@@QAEXXZ
// donor Buka TU SOURCE/PHILAI; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.559466;margin=0.357720;shape=0.362;size=0.984;calls=1.000;alternate=pol20:void philAI::CheckBerserk(void)@0x00038c3d
VA(0x0041ad10, 0x294)
void philAI::CheckBerserk(hero* pHero) {
    int enemy;
    int x;
    mapCell* cell;
    int y;
    hero* enemyHero;
    int best = -1;
    int heroFightValue;

    gbBerserk = 0;
    fBerserkFactor = 1.0f;
    heroFightValue = FightValueOfStack(&pHero->m_army, pHero, 1, 0, 0);
    if (heroFightValue < 100)
        heroFightValue = 100;
    if (heroFightValue < 30000)
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
                            if (enemy > heroFightValue)
                                return;
                            if (enemy > best)
                                best = enemy;
                        }
                    }
                    break;
                case MAP_TRIGGER_EVENT | MAP_OBJECT_HERO:
                    if (gpGame->m_availableHeroes[cell->m_objectMetadata] != pHero->m_owner) {
                        enemyHero = gpGame->GetHero(cell->m_objectMetadata);
                        enemy = FightValueOfStack(
                            &enemyHero->m_army,
                            NULL,
                            1,
                            enemyHero->m_locationType == (MAP_TRIGGER_EVENT | MAP_OBJECT_TOWN),
                            enemyHero->m_occupiedTown
                        );
                        if (enemy * 2 > heroFightValue)
                            return;
                        if (enemy * 2 > best)
                            best = enemy * 2;
                    }
                    break;
            }
        }
    }
    if (best <= 0)
        return;
    fBerserkFactor = best * 0.75 / heroFightValue;
    gbBerserk = 1;
}

// Buka 2.1 DoDimensionDoor with DimensionDoorTo inlined for the given hero:
// HoMM1 teleports with three arguments and returns a byte flag.
VA(0x0041afa4, 0x1a0)
signed char philAI::DoDimensionDoor(hero* pHero) {
    int x;
    int i;
    int y;
    int length;
    int bestX, bestY;
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
                length = gpSearchArray->m_pathLength - i;
            }
        }
    }
    if (bestX == -1 || length <= 4)
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
VA(0x0041b144, 0x8f0)
void philAI::DoAI(int player) {
    int pathIndex;
    int moveDone;
    int bestDirection;
    int stepMax;
    int steps;
    mapCell* eventCell;
    int dummy;
    signed char stopAfterStep;
    int oldShowIt;
    signed char halfShown;
    int x;
    int y;
    short minRV;
    signed char moveInterrupt;
    hero* aiHero;
    int flag;
    int moveResult;
    int tempArray[4];

    halfShown = 0;
    LogInt("DO AI 1", player);
    if (gbGameOver)
        return;
    if (giLimitPlayer && giLimitPlayer != player)
        return;
    LogInt("DO AI", player);
    GetTurnAIVars(player);
    ShowStatus();
    for (pathIndex = 0; pathIndex < RESOURCE_COUNT; pathIndex++) {
        sprintf(
            gText,
            "RES - %15s  %d  %d",
            gResourceNames[pathIndex],
            gpCurPlayer->m_resources[pathIndex],
            gpCurPlayer->m_aiData.m_income[pathIndex]
        );
        LogStr(gText);
    }
    CheckBuyStuff();
    IncrementHourGlass();
    while ((aiHero = DetermineHeroToMove(player)) != NULL) {
        giHumanTownConquered = GAME_TOWN_NONE;
        iCurPlaceToVisit = 0;
        if (gbGameOver)
            return;
        LogStr("\n\n\n\n");
        LogStr("===================================");
        LogInt("Player with HeroTOMOVE", player);
        LogStr(aiHero->m_name);
        LogStr("\n");
        CheckReload(aiHero);
        CheckBerserk(aiHero);
        giShowComputerRoute = 0;
        if (gConfig.blackoutComputer == 0 && gbRemoteOn == 0
            && (gpGame->m_mapExtra[aiHero->m_x][aiHero->m_y] & giCurWatchPlayerBit)) {
            bShowIt = 1;
            gpAdvManager->SetHeroContext(aiHero->m_id, 0);
        } else {
            bShowIt = 0;
            gpAdvManager->SetHeroContext(aiHero->m_id, 0);
        }
        moveDone = 0;
        ResetHeroRVs(0, 0, 0);
        if (aiHero->m_eventFlags & HERO_EVENT_EMBARKED)
            stepMax = 15;
        else
            stepMax = 5;
        minRV = aiHero->m_mobility + 42;
        stepMax = static_cast<int>(stepMax * (1.7 - gpCurPlayer->m_difficulty * 0.1));
        if (gConfig.slowVideo)
            stepMax *= 2;
        if (gConfig.slowVideo)
            minRV = static_cast<short>(
                minRV * ((gpCurPlayer->m_difficulty - PLAYER_TYPE_DUMB) * 0.03 + 0.8)
            );
        else
            minRV = static_cast<short>(
                minRV * ((gpCurPlayer->m_difficulty - PLAYER_TYPE_DUMB) * 0.06 + 0.8)
            );
        while (!moveDone && aiHero->m_remainingMobility >= 4) {
            if (gbGameOver)
                return;
            if (aiHero->m_remainingMobility == aiHero->m_mobility
                && gpCurPlayer->m_ultimateArtifactHintChance > 15
                && gpCurPlayer->m_ultimateArtifactHintX == aiHero->m_x
                && gpCurPlayer->m_ultimateArtifactHintY == aiHero->m_y)
                gpAdvManager->ProcessSearch(aiHero->m_x, aiHero->m_y);
        retarget:
            DetermineTargetPosition(aiHero, aiHero->m_destinationX, aiHero->m_destinationY, minRV);
            for (pathIndex = 0; pathIndex < iCurPlaceToVisit; pathIndex++) {
                if (iPlacesVisited[pathIndex][0] == aiHero->m_destinationX
                    && iPlacesVisited[pathIndex][1] == aiHero->m_destinationY
                    && gpAdvManager->GetCell(aiHero->m_destinationX, aiHero->m_destinationY)
                               ->m_triggerType
                           != (MAP_TRIGGER_EVENT | MAP_OBJECT_TOWN))
                    aiHero->m_remainingMobility = 0;
            }
            if (iCurPlaceToVisit < ADVMGR_PLACE_VISIT_COUNT) {
                iPlacesVisited[iCurPlaceToVisit][0] = aiHero->m_x;
                iPlacesVisited[iCurPlaceToVisit][1] = aiHero->m_y;
                iCurPlaceToVisit++;
            }
            giShowComputerRoute = 1;
            if (aiHero->m_mobility == aiHero->m_remainingMobility) {
                halfShown = 0;
                IncrementHourGlass();
            }
            if (aiHero->m_destinationX != HERO_DESTINATION_NONE
                && aiHero->m_destinationY != HERO_DESTINATION_NONE) {
                eventCell = NULL;
                gpAdvManager->SetHeroContext(aiHero->m_id, 0);
                gpSearchArray->BuildPath(
                    aiHero->m_x,
                    aiHero->m_y,
                    aiHero->m_destinationX,
                    aiHero->m_destinationY,
                    aiHero->m_remainingMobility
                );
                if (gpSearchArray->m_pathLength > 0) {
                    gpAdvManager->UpdateScreen(0, 0);
                    if (aiHero->HasSpell(SPELL_DIMENSION_DOOR) && DoDimensionDoor(aiHero))
                        goto retarget;
                    steps = 0;
                    pathIndex = gpSearchArray->m_pathLength - 1;
                    moveResult = 0;
                    moveInterrupt = 0;
                    while (pathIndex >= 0 && stepMax > steps) {
                        stopAfterStep = (steps + 1 == stepMax || pathIndex == 0) ? 1 : 0;
                        if (pathIndex > 0 && GoodAdjacent(aiHero, &bestDirection)) {
                            gpSearchArray->m_directions[pathIndex] = bestDirection;
                            stopAfterStep = 1;
                        }
                        if (gpAdvManager->GetMoveShowIt(gpSearchArray->m_directions[pathIndex])) {
                            oldShowIt = bShowIt;
                            bShowIt = 1;
                            gpMouseManager->ReallyHidePointer();
                            bShowIt = oldShowIt;
                        }
                        eventCell = gpAdvManager->MoveHero(
                            gpSearchArray->m_directions[pathIndex],
                            stopAfterStep,
                            &x,
                            &y,
                            &moveResult,
                            1,
                            &moveInterrupt
                        );
                        steps++;
                        if (eventCell || moveResult || moveInterrupt)
                            break;
                        pathIndex--;
                    }
                    if (aiHero->m_owner == HERO_OWNER_NONE)
                        goto nextHero;
                    if (aiHero->m_remainingMobility <= aiHero->m_mobility >> 1 && !halfShown) {
                        halfShown = 1;
                        IncrementHourGlass();
                    }
                    if (pathIndex < 0 && gpCurPlayer->m_ultimateArtifactHintChance > 15
                        && gpCurPlayer->m_ultimateArtifactHintX == aiHero->m_x
                        && gpCurPlayer->m_ultimateArtifactHintY == aiHero->m_y) {
                        if (aiHero->m_mobility == aiHero->m_remainingMobility)
                            gpAdvManager->ProcessSearch(
                                ADVMGR_SEARCH_VIEW_CENTER,
                                ADVMGR_SEARCH_VIEW_CENTER
                            );
                        else
                            aiHero->m_remainingMobility = 0;
                    }
                    if (pathIndex < 0
                        && (((aiHero->m_x != aiHero->m_destinationX
                              || aiHero->m_y != aiHero->m_destinationY)
                             && !eventCell)
                            || aiHero->m_remainingMobility < 4 || (moveResult && !eventCell)))
                        moveDone = 1;
                    oldShowIt = bShowIt;
                    bShowIt = 1;
                    gpMouseManager->ReallyShowPointer();
                    bShowIt = oldShowIt;
                    gpAdvManager->UpdateRadar(1, 0);
                } else {
                    moveDone = 1;
                }
                if (eventCell) {
                    gpAdvManager->DoAIEvent(eventCell, aiHero, x, y);
                    if (gpCurPlayer->m_currentHero == INVALID_HERO)
                        goto nextHero;
                    ResetHeroRVs(1, aiHero->m_destinationX, aiHero->m_destinationY);
                }
            } else {
                moveDone = 1;
            }
        }
        aiHero->m_remainingMobility = 0;
        gpAdvManager->DeactivateCurrHero();
    nextHero:
        if (aiHero->m_locationType == (MAP_TRIGGER_EVENT | MAP_OBJECT_TOWN))
            CheckBuyStuff();
    }
}

// Buka 2.1 GetGameAIVars refreshes every player's game attention value.
VA(0x0041ba34, 0x4b)
void philAI::GetGameAIVars(void) {
    int i;

    for (i = 0; i < gpGame->m_playerCount; i++)
        GetGameAttentionValue(i);
}

// donor PoL RVA 0x0003a329; preferred Buka symbol ?GetTurnAIVars@philAI@@QAEXH@Z
// donor Buka TU SOURCE/PHILAI; HoMM1 owner inferred from contiguous order
// evidence: graph:3;base=0.345425;margin=0.144973;shape=0.282;size=0.515;calls=0.667;alternate=pol20:void philAI::GetTurnAIVars(int)@0x0003a329
VA(0x0041ba7f, 0x6a0)
void philAI::GetTurnAIVars(int player) {
    int totalFightValue;
    playerData* pPlayer;
    int yPos;
    int mineValue;
    int xPos;
    int i;
    float fFightVal;
    int y;
    int otherIndex;
    hero* heroPointer;
    int unusedFightValue;
    int artTotal;
    int hIndex;
    int x;
    town* townPointer;

    giCurTurn = gpGame->m_day + (gpGame->m_week - 1) * CALENDAR_DAYS_PER_WEEK
                + (gpGame->m_month - 1) * CALENDAR_DAYS_PER_MONTH;
    GetTurnAttentionValue(player);
    TurnCostResource(player);
    iCurHourGlassPhase = 0;
    iSandAnim = 0;
    gpCurPlayer->m_aiData.m_obeliskValue = static_cast<int>(TurnValueOfObelisk(player));
    gpCurPlayer->m_aiData.m_unexploredValue = MeanRVOfUnexploredTerritory(player);
    bHeroBuiltThisTurn = 0;
    if (giCurTurn - giBuildBoatStuffTurn[player] > 8) {
        giBuildShipyard[player] = GAME_TOWN_NONE;
        giBuildBoat[player] = GAME_TOWN_NONE;
    }
    unusedFightValue = 0;
    fFightVal = 0.0f;
    totalFightValue = 0;
    for (i = 0; i < gpCurPlayer->m_heroCount; i++) {
        heroPointer = gpGame->GetHero(gpCurPlayer->m_heroIds[i]);
        fFightVal =
            static_cast<float>(FightValueOfStack(&heroPointer->m_army, heroPointer, 0, 0, 0));
        totalFightValue = static_cast<int>(totalFightValue + fFightVal);
        heroPointer->m_aiFightValue = fFightVal * 4e-05 + 0.4;
    }
    for (i = 0; i < gpCurPlayer->m_townCount; i++) {
        townPointer = gpGame->GetTown(gpCurPlayer->m_townIds[i]);
        fFightVal = static_cast<float>(FightValueOfStack(&townPointer->m_army, NULL, 0, 0, 0));
        totalFightValue = static_cast<int>(totalFightValue + fFightVal);
    }
    gpCurPlayer->m_aiData.m_upgradeValueWeight =
        static_cast<float>(
            gpCurPlayer->m_resources[RESOURCE_GOLD] + gpCurPlayer->m_aiData.m_income[RESOURCE_GOLD]
        ) / (totalFightValue + 1000)
        + gpCurPlayer->m_aiData.m_attentionWeights.upgradeBase;
    artTotal = 0;
    for (i = ARTIFACT_REGULAR_FIRST; i < ARTIFACT_REGULAR_END; i++)
        artTotal += gArtifactBaseRV[i];
    for (i = 0; i < gpGame->m_playerCount; i++)
        gpGame->m_players[i].m_aiData.m_artifactPoolShare =
            1.0 / (gpGame->m_playerCount + gpGame->m_deadPlayerCount);
    gpCurPlayer->m_aiData.m_artifactValue = artTotal / 33.0;
    memset(gaiTurnValueOfMine, 7, sizeof(gaiTurnValueOfMine));
    for (otherIndex = 0; otherIndex < gpGame->m_playerCount; otherIndex++) {
        if (otherIndex != giCurPlayer) {
            pPlayer = &gpGame->m_players[otherIndex];
            for (hIndex = 0; hIndex < pPlayer->m_heroCount; hIndex++) {
                xPos = gpGame->GetHero(pPlayer->m_heroIds[hIndex])->m_x;
                yPos = gpGame->GetHero(pPlayer->m_heroIds[hIndex])->m_y;
                for (x = xPos - 10; x <= xPos + 10; x++) {
                    for (y = yPos - 10; y <= yPos + 10; y++) {
                        if (x >= 0 && x < MAP_CELL_GRID_SIZE && y >= 0 && y < MAP_CELL_GRID_SIZE) {
                            mineValue = abs(MANHATTAN_LENGTH(x - xPos, y - yPos) - 4) >> 2;
                            if (gaiTurnValueOfMine[x][y] > mineValue)
                                gaiTurnValueOfMine[x][y] = mineValue;
                        }
                    }
                }
            }
        }
    }
    for (i = 0; i < GAME_HERO_COUNT; i++)
        gfHeroInteractionBonus[i] = 1.0f;
    if (gpCurPlayer->m_difficulty == PLAYER_TYPE_DUMB) {
        gfAttackHumanBonus = 0.6f;
        gfAttackComputerBonus = 1.3f;
    } else if (gpCurPlayer->m_difficulty == PLAYER_TYPE_AVERAGE) {
        gfAttackHumanBonus = 1.0f;
        gfAttackComputerBonus = 1.0f;
    } else {
        gfAttackHumanBonus = gpCurPlayer->m_difficulty * 0.07 + 1.0;
        gfAttackComputerBonus = 1.1 - gpCurPlayer->m_difficulty * 0.12;
    }
    if (gbIAmGreatest)
        gfAttackComputerBonus = 0.1f;
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
VA(0x0041c11f, 0x600)
void philAI::GetBestBHC(int, BHC& best) {
    float fValue = 1.0f;
    float bestBHCValue = -99.0f;
    int total = 0;
    int totalWeights = 0;
    int ideal[GAME_TOWN_COUNT];
    int strengths[GAME_TOWN_COUNT];
    BHC choice;
    int townNo;
    town* townPointer;
    int meanStrength;

    for (townNo = 0; townNo < gpCurPlayer->m_townCount; townNo++) {
        townPointer = &gpGame->m_castleRecs[gpCurPlayer->m_townIds[townNo]];
        strengths[townNo] = FightValueOfStack(&townPointer->m_army, NULL, 0, 0, 0) + 400;
        total += strengths[townNo];
        if (townPointer->m_buildings & (1 << BUILDING_SLOT_CASTLE))
            totalWeights += 10;
        else
            totalWeights += 7;
    }
    if (totalWeights < 1)
        totalWeights = 1;
    meanStrength = total / totalWeights;
    for (townNo = 0; townNo < gpCurPlayer->m_townCount; townNo++) {
        townPointer = &gpGame->m_castleRecs[gpCurPlayer->m_townIds[townNo]];
        ideal[townNo] =
            meanStrength * ((townPointer->m_buildings & (1 << BUILDING_SLOT_CASTLE)) ? 10 : 7)
            + 400;
    }
    for (townNo = 0; townNo < gpCurPlayer->m_townCount; townNo++) {
        townPointer = &gpGame->m_castleRecs[gpCurPlayer->m_townIds[townNo]];
        LogInt("Turns Owned", townPointer->m_turnsOwned);
        extern int gbRemoteReady;
        if (giCurTurn > 3 && (!gbRemoteOn || gbRemoteReady) && townPointer->m_turnsOwned < 3)
            continue;
        CheckDoMain(0, 0);
        GetBestBuilding(townPointer, choice, fValue);
        fValue = fValue * ((100 - Random(0, 10)) / 100.0);
        if (fValue > bestBHCValue) {
            bestBHCValue = fValue;
            best = choice;
        }
        CheckDoMain(0, 0);
        GetBestCreature(townPointer, choice, fValue);
        fValue = fValue
                 * (static_cast<float>(ideal[townNo]) / static_cast<float>(strengths[townNo]) / 3.0f
                    + 0.66);
        fValue = fValue * ((100 - Random(0, 10)) / 100.0);
        if (fValue > bestBHCValue) {
            bestBHCValue = fValue;
            best = choice;
        }
        CheckDoMain(0, 0);
        if (gpCurPlayer->m_heroCount < giMaxHeroesForThisPlayer
            && (townPointer->m_buildings & (1 << BUILDING_SLOT_CASTLE))) {
            GetBestHero(townPointer, choice, fValue);
            fValue = fValue * ((100 - Random(0, 10)) / 100.0);
            if (!bHeroBuiltThisTurn && giCurTurn > 5 && fValue > 0.0f) {
                if ((gpCurPlayer->m_aiData.m_income[RESOURCE_GOLD] >= 1250
                     && gpCurPlayer->m_heroCount < giMaxHeroesForThisPlayer - 2)
                    || gpCurPlayer->m_heroCount <= 1)
                    fValue += 500.0f;
                else if (gpCurPlayer->m_aiData.m_income[RESOURCE_GOLD] >= 1500
                         && gpCurPlayer->m_heroCount < giMaxHeroesForThisPlayer - 1)
                    fValue = fValue * 1.3;
            } else if (gpCurPlayer->m_heroCount == 0) {
                fValue += 500.0f;
            }
            if (bestBHCValue < fValue) {
                bestBHCValue = fValue;
                best = choice;
            }
        }
    }
    LogStr("BestBHC ", best.type, static_cast<int>(bestBHCValue * 100.0f), best.what, 0, 0);
    if (bestBHCValue < 0.02)
        best.type = PURCHASE_NONE;
}

// Buka 2.1 DetermineHeroToMove: the current player's hero with the most
// remaining mobility; HoMM1 counts with a byte index.
VA(0x0041c71f, 0x11c)
hero* philAI::DetermineHeroToMove(int player) {
    int bestHero;
    int bestMobility;
    int mobility;
    signed char i;

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
VA(0x0041c83b, 0x932)
void philAI::DetermineTargetPosition(
    hero* pHero,
    signed char& targetX,
    signed char& targetY,
    short mobility
) {
    int bestRV;
    int cellValue;
    int spacing;
    int heroTerrainType;
    town* portTown;
    int rowCnt;
    int valid;
    int colPhase;
    int heroIndex;
    short x;
    short bestY;
    mapCell* thisCell;
    short bestX;
    short y;

    bestX = -1;
    bestY = -1;
    bestRV = -999999;
    giBestShipyardId = GAME_TOWN_NONE;
    gbPossibleShipyardFound = 0;
    gbActualShipyardFound = 0;
    gbActualBoatFound = 0;
    spacing = pHero->m_mobility / 6;
    thisCell = gpAdvManager->GetCell(pHero->m_x, pHero->m_y);
    heroTerrainType = CELL_TERRAIN(thisCell);
    if (heroTerrainType == TERRAIN_SNOW || heroTerrainType == TERRAIN_SWAMP) {
        spacing--;
        mobility = static_cast<short>(mobility * 1.25);
    }
    if (heroTerrainType == TERRAIN_DESERT) {
        spacing -= 2;
        mobility = static_cast<short>(mobility * 1.5);
    }
    if (spacing < 3)
        spacing = 3;
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
    for (x = 0; x < MAP_CELL_GRID_SIZE; x++) {
        rowCnt = -1;
        colPhase++;
        if (colPhase >= spacing)
            colPhase = 0;
        for (y = 0; y < MAP_CELL_GRID_SIZE; y++) {
            rowCnt++;
            if (rowCnt >= spacing)
                rowCnt = 0;
            if (gpSearchArray->m_cells[x][y].visited) {
                thisCell = gpAdvManager->GetCell(x, y);
                if (gpSearchArray->m_cells[x][y].distance > mobility) {
                    if (gpSearchArray->m_cells[x][y].distance > mobility * 2)
                        valid = 0;
                    else
                        valid = thisCell->m_triggerType == (MAP_TRIGGER_EVENT | MAP_OBJECT_TOWN)
                                || thisCell->m_triggerType == (MAP_TRIGGER_EVENT | MAP_OBJECT_HERO)
                                || (thisCell->m_triggerType == (MAP_TRIGGER_EVENT | MAP_OBJECT_SHIP)
                                    && !(pHero->m_eventFlags & HERO_EVENT_EMBARKED));
                } else {
                    valid =
                        (thisCell->m_triggerType & MAP_TRIGGER_EVENT)
                        || (thisCell->m_triggerType == MAP_OBJECT_COAST
                            && (pHero->m_eventFlags & HERO_EVENT_EMBARKED))
                        || (x % spacing == 0 && y % spacing == 0
                            && (((pHero->m_eventFlags & HERO_EVENT_EMBARKED)
                                 && CELL_TERRAIN(thisCell) == TERRAIN_WATER)
                                || (!(pHero->m_eventFlags & HERO_EVENT_EMBARKED)
                                    && CELL_TERRAIN(thisCell) != TERRAIN_WATER)))
                        || (x == gpCurPlayer->m_ultimateArtifactHintX
                            && y == gpCurPlayer->m_ultimateArtifactHintY);
                }
                if (valid) {
                    for (heroIndex = 0; heroIndex < gpCurPlayer->m_heroCount; heroIndex++) {
                        if (thisCell->m_triggerType != (MAP_TRIGGER_EVENT | MAP_OBJECT_TOWN)
                            && thisCell->m_triggerType != (MAP_TRIGGER_EVENT | MAP_OBJECT_HERO)
                            && gpCurPlayer->m_heroIds[heroIndex] != pHero->m_id
                            && gpGame->m_heroRecs[gpCurPlayer->m_heroIds[heroIndex]].m_destinationX
                                   == x
                            && gpGame->m_heroRecs[gpCurPlayer->m_heroIds[heroIndex]].m_destinationY
                                   == y) {
                            cellValue = -2000;
                            goto scored;
                        }
                    }
                    CheckDoMain(0, 0);
                    cellValue = RVOfPosition(
                        pHero,
                        x,
                        y,
                        gpSearchArray->m_cells[x][y].rvFlag1,
                        gpSearchArray->m_cells[x][y].valueX,
                        gpSearchArray->m_cells[x][y].valueY,
                        gpSearchArray->m_cells[x][y].rvFlag2,
                        gpSearchArray->m_cells[x][y].previousX,
                        gpSearchArray->m_cells[x][y].previousY,
                        2
                    );
                    cellValue = cellValue * (Random(1, 50) + 75);
                    cellValue /= 100;
                } else {
                    cellValue = -100;
                }
                if (x == targetX && y == targetY) {
                    cellValue = static_cast<int>(cellValue * AI_TARGET_HUMAN_VALUE_FACTOR);
                    if (MANHATTAN_LENGTH(x - pHero->m_x, y - pHero->m_y) > 3)
                        cellValue++;
                }
            scored:
                if (cellValue > bestRV) {
                    bestX = x;
                    bestY = y;
                    bestRV = cellValue;
                } else if (cellValue == bestRV && cellValue == 0) {
                    if (MANHATTAN_LENGTH(bestY - pHero->m_y, bestX - pHero->m_x)
                        < MANHATTAN_LENGTH(x - pHero->m_x, y - pHero->m_y)) {
                        bestX = x;
                        bestY = y;
                    }
                }
            }
        }
    }
    if (bestRV < 75 && (gbPossibleShipyardFound || gbActualShipyardFound) && !gbActualBoatFound
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
            bestRV = 123;
            bestX = portTown->m_x;
            bestY = portTown->m_y;
            if (pHero->m_x == bestX && pHero->m_y == bestY)
                pHero->m_remainingMobility = 0;
        }
        CheckBuyStuff();
    }
    targetX = bestX;
    targetY = bestY;
    LogStr(
        "Hero, Best RV",
        pHero->m_owner,
        bestRV,
        targetX * 1000 + targetY,
        pHero->m_x * 1000 + pHero->m_y,
        0
    );
    LogStr("\n\n****");
}

// donor PoL RVA 0x0003c6e2; preferred Buka symbol ?ProbableOutcomeOfBattle@philAI@@QAEXPAVarmyGroup@@PAVhero@@010HHHAAMAAH3333@Z
// donor Buka TU SOURCE/PHILAI; HoMM1 owner inferred from contiguous order
// evidence: graph:4;base=0.549946;margin=0.581641;shape=0.449;size=0.953;calls=0.724;alternate=pol20:void philAI::ProbableOutcomeOfBattle(class armyGroup *, class hero *, class armyGroup *, class hero *, class armyGroup *, int, int, int, float &, int &, int &, int &, int &, int &)@0x0003c6e2
VA(0x0041d16d, 0x64d)
void philAI::ProbableOutcomeOfBattle(
    armyGroup* attacker,
    hero* attackerHero,
    armyGroup* defender,
    hero* defenderHero,
    armyGroup* townArmy,
    signed char useTown,
    signed char townId,
    int enemyPlayer,
    float& winChance,
    int& attackerLoss,
    int& defenderLoss,
    int& attackerRemaining,
    int& defenderRemaining,
    int& outcomeValue
) {
    float attArmy;
    int artSlot;
    float defendingArmy;
    int artsD;
    int notUsed;
    int exp;
    float attackStrength;
    float defP;
    float defStr;
    float rawFight[2];
    float attackerPower;
    float power;
    float factor;
    int attArts;

    attArts = 0;
    artsD = 0;
    attArmy = static_cast<float>(FightValueOfStack(attacker, attackerHero, 1, 0, 0));
    defendingArmy =
        static_cast<float>(FightValueOfStack(defender, defenderHero, 1, useTown, townId));
    if (townArmy)
        defendingArmy += static_cast<float>(FightValueOfStack(townArmy, NULL, 1, 0, 0));
    rawFight[0] = static_cast<float>(FightValueOfStack(attacker, attackerHero, 0, 0, 0));
    rawFight[1] = static_cast<float>(FightValueOfStack(defender, defenderHero, 0, 0, 0));
    if (townArmy)
        rawFight[1] += static_cast<float>(FightValueOfStack(townArmy, NULL, 0, 0, 0));
    if (useTown)
        defendingArmy = defendingArmy * 1.11;
    defStr = defendingArmy;
    if (enemyPlayer == GAME_PLAYER_NONE) {
        attackStrength = attArmy * (gpCurPlayer->m_difficulty * 0.15 + 0.7);
    } else {
        attackStrength = attArmy;
        if (gbHumanPlayer[enemyPlayer]) {
            defStr = defStr * 1.14;
            if (gpCurPlayer->m_difficulty == 1)
                attackStrength = attackStrength * 1.5;
        }
    }
    if (attackStrength < 1.0f)
        attackStrength = 1.0f;
    if (defStr < 1.0f)
        defStr = 1.0f;
    power = 2.75f;
    if (attackStrength > 1000000.0f || defStr > 1000000.0f)
        power = 2.0f;
    attackerPower = pow(attackStrength, power);
    defP = pow(defStr, power);
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
    attackerLoss = static_cast<int>((1.0 - winChance) * rawFight[0]);
    defenderLoss = static_cast<int>(rawFight[1] * winChance);
    attackerRemaining =
        static_cast<int>(attackerLoss * winChance + (1.0f - winChance) * rawFight[0]);
    defenderRemaining =
        static_cast<int>(defenderLoss * (1.0f - winChance) + rawFight[1] * winChance);
    factor = 1.33 - gpCurPlayer->m_aiData.m_attentionWeights.upgradeBase;
    outcomeValue = static_cast<int>(-attackerRemaining * factor * factor);
    if (enemyPlayer >= 0) {
        factor = gpCurPlayer->m_aiData.m_attentionWeights.upgradeBase + 0.66;
        if (gbHumanPlayer[enemyPlayer])
            outcomeValue = static_cast<int>(
                outcomeValue + (defenderRemaining * factor * factor) * gfAttackHumanBonus
            );
        else
            outcomeValue = static_cast<int>(
                outcomeValue + defenderRemaining * gfAttackComputerBonus * factor * factor
            );
    }
    outcomeValue = static_cast<int>(outcomeValue * gpCurPlayer->m_aiData.m_upgradeValueWeight);
    if (attackerHero) {
        for (artSlot = 0; artSlot < HERO_ARTIFACT_SLOT_COUNT; artSlot++) {
            if (attackerHero->m_artifacts[artSlot] >= 0
                && attackerHero->m_artifacts[artSlot] < ARTIFACT_REGULAR_END)
                attArts += gArtifactBaseRV[attackerHero->m_artifacts[artSlot]];
        }
        outcomeValue = static_cast<int>(outcomeValue - (attArts + 1400) * (1.0f - winChance));
        exp = gpGame->ExperienceValueOfStack(defender, defenderHero);
        outcomeValue =
            static_cast<int>(outcomeValue + exp * attackerHero->m_aiFightValue * winChance * 0.8);
    }
    if (defenderHero) {
        for (artSlot = 0; artSlot < HERO_ARTIFACT_SLOT_COUNT; artSlot++) {
            if (defenderHero->m_artifacts[artSlot] >= 0
                && defenderHero->m_artifacts[artSlot] < ARTIFACT_REGULAR_END)
                artsD += gArtifactBaseRV[defenderHero->m_artifacts[artSlot]];
        }
        outcomeValue = static_cast<int>(
            outcomeValue
            + (artsD + 1250)
                  * (gbHumanPlayer[defenderHero->m_owner] ? gfAttackHumanBonus
                                                          : gfAttackComputerBonus)
                  * winChance
        );
    }
}

// Buka 2.1 GetOddsOfWinning returns the exact constant seen in retail's fld.
// @dead-code
// Zero-ref: pinned retail has no incoming direct call/jump or relocated reference.
VA(0x0041d7ba, 0x1e)
float philAI::GetOddsOfWinning(int) {
    return 1.0f;
}

// Buka 2.1 ValueOfBuyingBuilding without the HoMM2 special buildings: the
// base value, scaled per slot by attention weights and dwelling counts, the
// enemy threat and the purchase deflator.
VA(0x0041d7d8, 0x59b)
void philAI::ValueOfBuyingBuilding(
    town* townPointer,
    int building,
    int& resourceValue,
    float& benefitCost
) {
    int buildingCost[RESOURCE_COUNT];
    int attackWeek;
    int dwellingsOwned;
    int projectedAttackValue;
    int highestDwellingId;
    int dwellingIndex;
    float dangerRating;
    int creatureLocated;
    int i;
    int currentAttackTurns;
    int numFilledSlots;
    float totalEnemyStrength;
    int currentCreatureType;
    float fAttackOdds;
    short factionId;
    float curBenefit;
    int buildingLevel;

    factionId = townPointer->m_type;
    dwellingsOwned = 0;
    highestDwellingId = -1;
    for (i = 0; i < BUILDING_SLOT_DWELLING_COUNT; i++) {
        if (townPointer->m_buildings & (1 << (i + BUILDING_SLOT_DWELLING_FIRST))) {
            dwellingsOwned++;
            highestDwellingId = i;
        }
    }
    numFilledSlots = 0;
    for (i = 0; i < ARMY_GROUP_SLOT_COUNT; i++)
        if (townPointer->m_army.m_creatureCounts[i] > 0)
            numFilledSlots++;
    curBenefit = static_cast<float>(GetBuildingBaseResourceValue(
        factionId,
        building,
        static_cast<signed char>(
            building == BUILDING_SLOT_MAGE_GUILD ? townPointer->m_buildState : 0
        )
    ));
    if (building == BUILDING_SLOT_MAGE_GUILD && townPointer->m_buildState > 0)
        curBenefit -= static_cast<float>(
            GetBuildingBaseResourceValue(factionId, building, townPointer->m_buildState - 1)
        );
    switch (building) {
        case BUILDING_SLOT_CASTLE:
            curBenefit =
                (gpCurPlayer->m_aiData.m_attentionWeights.buildingValue * 2.0f + 0.33) * curBenefit;
            buildingLevel = dwellingsOwned;
            curBenefit = (1.6 - buildingLevel * 0.2) * curBenefit;
            break;
        case BUILDING_SLOT_MAGE_GUILD:
            curBenefit =
                (gpCurPlayer->m_aiData.m_attentionWeights.buildingValue * 2.0f + 0.33) * curBenefit;
            curBenefit = (1.33 - gpCurPlayer->BuildingsOwned(factionId, 0, 0) * 0.33) * curBenefit;
            break;
        case BUILDING_SLOT_THIEVES_GUILD:
            break;
        case BUILDING_SLOT_SHIPYARD:
            curBenefit = 0;
            break;
        case BUILDING_SLOT_WELL:
            curBenefit =
                (gpCurPlayer->m_aiData.m_attentionWeights.buildingValue + 0.66) * curBenefit;
            curBenefit =
                (gpCurPlayer->m_aiData.m_attentionWeights.upgradeBase * 2.0f + 0.33) * curBenefit;
            curBenefit = (dwellingsOwned * 0.33 + 0.66) * curBenefit;
            break;
        case BUILDING_SLOT_TAVERN:
            curBenefit =
                FightValueOfStack(&townPointer->m_army, NULL, 0, 0, 0) / 3000.0f * curBenefit;
            break;
        case BUILDING_SLOT_DWELLING_1:
        case BUILDING_SLOT_DWELLING_2:
        case BUILDING_SLOT_DWELLING_3:
        case BUILDING_SLOT_DWELLING_4:
        case BUILDING_SLOT_DWELLING_5:
        case BUILDING_SLOT_DWELLING_6:
            if (numFilledSlots == ARMY_GROUP_SLOT_COUNT) {
                creatureLocated = 0;
                for (i = 0; i < ARMY_GROUP_SLOT_COUNT; i++)
                    if (gDwellingType[townPointer->m_type][building - BUILDING_SLOT_DWELLING_FIRST]
                        == townPointer->m_army.m_creatureTypes[i])
                        creatureLocated = 1;
                if (!creatureLocated)
                    break;
            }
            curBenefit =
                (gpCurPlayer->m_aiData.m_attentionWeights.buildingValue + 0.66) * curBenefit;
            curBenefit =
                (gpCurPlayer->m_aiData.m_attentionWeights.upgradeBase * 2.0f + 0.33) * curBenefit;
            curBenefit =
                (1.0 - gpCurPlayer->BuildingsOwned(factionId, building, 0) * 0.05) * curBenefit;
            if (building - BUILDING_SLOT_DWELLING_FIRST < highestDwellingId)
                curBenefit = (1.66 - dwellingsOwned * 0.33) * curBenefit;
            if (townPointer->m_buildings & (1 << BUILDING_SLOT_WELL))
                curBenefit = curBenefit * 1.1;
            for (dwellingIndex = 0; dwellingIndex < BUILDING_SLOT_DWELLING_COUNT; dwellingIndex++) {
                currentCreatureType = gDwellingType[townPointer->m_type][dwellingIndex];
                if ((townPointer->m_buildings
                     & (1 << (dwellingIndex + BUILDING_SLOT_DWELLING_FIRST)))
                    && townPointer->m_garrison[dwellingIndex] > 0
                    && gMonsterDatabase[gDwellingType[townPointer->m_type]
                                                     [building - BUILDING_SLOT_DWELLING_FIRST]]
                               .iconIndex
                           < gMonsterDatabase[currentCreatureType].iconIndex * 1.2) {
                    curBenefit = 0;
                    break;
                }
            }
            break;
    }
    LikelihoodOfEnemyAttacking(
        townPointer,
        NULL,
        fAttackOdds,
        totalEnemyStrength,
        currentAttackTurns,
        projectedAttackValue,
        attackWeek,
        dangerRating
    );
    curBenefit = (1.0 - dangerRating * 3.0) * curBenefit;
    if (curBenefit < 0.0f)
        curBenefit = 0;
    GetBuildingCost(
        factionId,
        building,
        buildingCost,
        static_cast<signed char>(
            building == BUILDING_SLOT_MAGE_GUILD ? townPointer->m_buildState : 0
        )
    );
    curBenefit = FutureDeflator(buildingCost) * curBenefit;
    resourceValue = static_cast<int>(curBenefit);
    benefitCost = curBenefit / RVConversion(buildingCost);
}

// donor PoL RVA 0x0003d6b7; preferred Buka symbol ?GetBestBuilding@philAI@@QAEXPAVtown@@AAUBHC@@AAM@Z
// donor Buka TU SOURCE/PHILAI; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.526596;margin=0.075083;shape=0.323;size=0.949;calls=1.000;alternate=pol20:void philAI::GetBestBuilding(class town *, struct BHC &, float &)@0x0003d6b7
VA(0x0041dd73, 0x185)
void philAI::GetBestBuilding(town* townPointer, BHC& purchase, float& benefitCost) {
    float buildingValue;
    int bestBuilding;
    float bestCost;
    int curBuilding;
    int costRV;
    float bestScore;
    float score;

    bestCost = -99.0f;
    bestScore = -99.0f;
    bestBuilding = -1;
    for (curBuilding = BUILDING_SLOT_MAGE_GUILD; curBuilding <= BUILDING_SLOT_DWELLING_LAST;
         curBuilding++) {
        if (!(townPointer->m_buildings & (1 << curBuilding))
            || (curBuilding == BUILDING_SLOT_MAGE_GUILD
                && townPointer->m_buildState < TOWN_MAGE_GUILD_COST_LEVEL_LAST)) {
            if (CanBuild(townPointer, curBuilding)) {
                ValueOfBuyingBuilding(townPointer, curBuilding, costRV, buildingValue);
                score = (Random(1, 5) + 95) * buildingValue / 100.0f;
                if (score > bestScore) {
                    bestBuilding = curBuilding;
                    bestCost = buildingValue;
                    bestScore = score;
                }
                if (giDebugLevel >= 5) {
                    sprintf(
                        gText,
                        "Town:%2d  Building: % 15s   Raw BC = %8.2f,  RandBC = %8.2f.",
                        townPointer->m_id,
                        GetBuildingName(townPointer->m_type, curBuilding),
                        buildingValue,
                        score
                    );
                    LogStr(gText);
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
VA(0x0041def8, 0x2f5)
void philAI::ValueOfBuyingCreature(
    town* townPointer,
    int creature,
    int& resourceValue,
    int purchaseCount,
    float& benefitCost
) {
    int nWeeks;
    int nPoints;
    float peril;
    int monsterCost[RESOURCE_COUNT];
    // Counts breath-attack stacks; retail allocation follows this local name
    // (renaming it moves registers).
    int archers;
    int creatRV;
    int rvCost;
    float attackChance;
    float foeStrength;
    float factor;
    int nTurns;
    int n;
    hero* occupant;
    int slotNum;

    archers = 0;
    GetMonsterCost(creature, monsterCost);
    rvCost = purchaseCount * RVConversion(monsterCost);
    creatRV = static_cast<int>(
        purchaseCount * gMonsterDatabase[creature].fightValue
        * gpCurPlayer->m_aiData.m_upgradeValueWeight
    );
    if (townPointer->m_occupyingHeroId != TOWN_OCCUPYING_HERO_NONE) {
        occupant = gpGame->GetHero(townPointer->m_occupyingHeroId);
        creatRV = static_cast<int>(creatRV * 1.1);
        if (occupant->m_heroClass == creature / CREATURE_FACTION_SIZE)
            creatRV = static_cast<int>(creatRV * AI_CREATURE_SAME_RACE_FACTOR);
        if ((gMonsterDatabase[creature].stats.attributes & MONSTER_FLAGS_BREATH_ATTACK)) {
            for (n = 0; n < ARMY_GROUP_SLOT_COUNT; n++) {
                if (occupant->m_army.m_creatureTypes[n] != CREATURE_NONE
                    && (gMonsterDatabase[occupant->m_army.m_creatureTypes[n]].stats.attributes
                        & MONSTER_FLAGS_BREATH_ATTACK))
                    archers++;
            }
            creatRV = static_cast<int>(creatRV * (1.18 - archers * 0.06));
        }
        creatRV = static_cast<int>(
            creatRV
            * (gpGame->m_players[townPointer->m_owner].m_aiData.m_attentionWeights.upgradeBase
               + 0.66)
        );
    }
    if ((gMonsterDatabase[creature].stats.attributes & MONSTER_FLAGS_BREATH_ATTACK)) {
        for (slotNum = 0; slotNum < ARMY_GROUP_SLOT_COUNT; slotNum++) {
            if (townPointer->m_army.m_creatureTypes[slotNum] != CREATURE_NONE
                && (gMonsterDatabase[townPointer->m_army.m_creatureTypes[slotNum]].stats.attributes
                    & MONSTER_FLAGS_BREATH_ATTACK))
                archers++;
        }
        creatRV = static_cast<int>(creatRV * (1.18 - archers * 0.06));
    }
    LikelihoodOfEnemyAttacking(
        townPointer,
        NULL,
        attackChance,
        foeStrength,
        nTurns,
        nPoints,
        nWeeks,
        peril
    );
    factor = peril + 0.96;
    creatRV = static_cast<int>(creatRV * (factor * factor * factor));
    creatRV = static_cast<int>(creatRV * FutureDeflator(monsterCost));
    resourceValue = creatRV;
    benefitCost = static_cast<float>(resourceValue) / static_cast<float>(rvCost);
}

// donor PoL RVA 0x0003db58; preferred Buka symbol ?GetBestCreature@philAI@@QAEXPAVtown@@AAUBHC@@AAM@Z
// donor Buka TU SOURCE/PHILAI; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.467416;margin=0.566378;shape=0.359;size=0.696;calls=1.000;alternate=pol20:void philAI::GetBestCreature(class town *, struct BHC &, float &)@0x0003db58
VA(0x0041e1ed, 0x211)
void philAI::GetBestCreature(town* townPointer, BHC& best, float& bestValue) {
    float bestCost;
    float rand;
    float worth;
    float bestRandScore;
    int bestBuy;
    int topDwelling;
    int curDwelling;
    int mon;
    int canAdd;
    int iArmy;
    int costRV;
    int numUnits;

    topDwelling = -1;
    bestBuy = 0;
    bestCost = -99.0f;
    bestRandScore = -99.0f;
    for (curDwelling = 0; curDwelling < BUILDING_SLOT_DWELLING_COUNT; curDwelling++) {
        mon = gDwellingType[townPointer->m_type][curDwelling];
        if ((townPointer->m_buildings & (1 << (curDwelling + BUILDING_SLOT_DWELLING_FIRST)))
            && townPointer->m_garrison[curDwelling] > 0) {
            canAdd = 0;
            for (iArmy = 0; iArmy < ARMY_GROUP_SLOT_COUNT; iArmy++) {
                if (townPointer->m_army.m_creatureTypes[iArmy] == CREATURE_NONE
                    || townPointer->m_army.m_creatureTypes[iArmy] == mon)
                    canAdd = 1;
            }
            if (canAdd) {
                numUnits = CreaturesToBuy(townPointer, curDwelling);
                if (numUnits > 0) {
                    ValueOfBuyingCreature(townPointer, mon, costRV, numUnits, worth);
                    rand = (Random(1, 10) + 90) * worth / 100.0;
                    if (rand > bestRandScore) {
                        topDwelling = curDwelling;
                        bestCost = worth;
                        bestRandScore = rand;
                        bestBuy = numUnits;
                    }
                    if (giDebugLevel >= 5) {
                        sprintf(
                            gText,
                            "Town:%2d  Creature: % 15s   Raw BC = %8.2f,  RandBC = %8.2f.",
                            townPointer->m_id,
                            GetMonsterName(mon),
                            worth,
                            rand
                        );
                        LogStr(gText);
                    }
                }
            }
        }
    }
    best.pTown = townPointer;
    best.type = PURCHASE_CREATURE;
    best.what = topDwelling;
    best.num = bestBuy;
    bestValue = bestCost;
}

// Buka's town overload indexes the six dwelling stocks and faction table.
VA(0x0041e3fe, 0x48)
int philAI::CreaturesToBuy(town* townPointer, int level) {
    int nGarrison = townPointer->m_garrison[level];
    return CreaturesToBuy(gDwellingType[townPointer->m_type][level], nGarrison);
}

// Buka 2.1 purchase count logic and retail's ordered call/branches agree.
VA(0x0041e446, 0x5f)
int philAI::CreaturesToBuy(int creatureType, int availableCount) {
    int purchaseCount = MaxBuyableCreatures(creatureType);
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
VA(0x0041e4a5, 0x9b)
int philAI::MaxBuyableCreatures(int creatureType) {
    int monsterCost[RESOURCE_COUNT];
    int maxUnits;
    int i;

    GetMonsterCost(creatureType, monsterCost);
    for (i = 0; i < RESOURCE_COUNT; i++) {
        if (monsterCost[i] == 0)
            maxUnits = 9999;
        else if (gpCurPlayer->m_resources[i] > 0)
            maxUnits = gpCurPlayer->m_resources[i] / monsterCost[i];
        else
            maxUnits = 0;
    }
    return maxUnits;
}

// donor PoL RVA 0x0003dff6; preferred Buka symbol ?ValueOfBuyingHero@philAI@@QAEXPAVtown@@PAVhero@@AAHAAM@Z
// donor Buka TU SOURCE/PHILAI; HoMM1 owner inferred from contiguous order
// evidence: graph:3;base=0.452634;margin=0.608146;shape=0.221;size=0.851;calls=1.000;alternate=pol20:void philAI::ValueOfBuyingHero(class town *, class hero *, int &, float &)@0x0003dff6
VA(0x0041e540, 0x1bc)
void philAI::ValueOfBuyingHero(
    town* townPointer,
    hero* heroPointer,
    int& resourceValue,
    float& benefitCost
) {
    int tmp;
    int i;
    int heroRV;
    int heroCost[RESOURCE_COUNT];
    int costRV;

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
        if (heroPointer->m_artifacts[i] >= 0 && heroPointer->m_artifacts[i] < ARTIFACT_REGULAR_END)
            heroRV += gArtifactBaseRV[heroPointer->m_artifacts[i]];
    }
    heroRV += heroPointer->m_experience / 2;
    heroRV = static_cast<int>(
        heroRV
        * (gpCurPlayer->m_aiData.m_attentionWeights.heroValue + 1.0
           - gpCurPlayer->m_aiData.m_attentionWeights.upgradeBase)
    );
    if (gTownHeroClass[townPointer->m_type] == heroPointer->m_heroClass)
        heroRV = static_cast<int>(heroRV * AI_HERO_PURCHASE_SAME_RACE_FACTOR);
    heroRV += StrategicValueOfPosition(heroPointer, heroPointer->m_x, heroPointer->m_y, 0, &tmp);
    heroRV -= 200;
    heroRV = static_cast<int>(heroRV * FutureDeflator(heroCost));
    benefitCost = static_cast<float>(heroRV) / costRV;
    resourceValue = heroRV;
}

// ValueOfEventAtPosition module state (.bss order follows names, not position).
DATA(0x004af7c0)
int iAttackerRemaining;
DATA(0x004af7c4)
int iDefenderRemaining;
DATA(0x004af7cc)
int iOutcome;
DATA(0x004af7d0)
int iArtifactChoice1;

// donor PoL RVA 0x0003e2a8; preferred Buka symbol ?GetBestHero@philAI@@QAEXPAVtown@@AAUBHC@@AAM@Z
// donor Buka TU SOURCE/PHILAI; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.649528;margin=0.194728;shape=0.311;size=0.925;calls=0.800;strings=Town:%2d  Hero    : % 15i   Raw BC = %8.2f,  RandBC = %8.2f.;alternate=pol20:void philAI::GetBestHero(class town *, struct BHC &, float &)@0x0003e2a8
VA(0x0041e6fc, 0x1a0)
void philAI::GetBestHero(town* townPointer, BHC& best, float& bestValue) {
    int bestHero;
    float worth;
    int curHero;
    hero* availHero;
    float adjusted;
    float bestScore;
    float bestCost;
    int cost;

    bestHero = -1;
    bestCost = -99.0f;
    bestScore = -99.0f;
    for (curHero = 0; curHero < HERO_AVAILABLE_SLOT_COUNT; curHero++) {
        availHero = &gpGame->m_heroRecs[gpCurPlayer->m_availableHeroIds[curHero]];
        ValueOfBuyingHero(townPointer, availHero, cost, worth);
        adjusted = (Random(1, 10) + 90.0) * worth / 100.0;
        if (adjusted > bestScore) {
            bestHero = curHero;
            bestCost = worth;
            bestScore = adjusted;
        }
        if (giDebugLevel >= 5) {
            sprintf(
                gText,
                "Town:%2d  Hero    : % 15i   Raw BC = %8.2f,  RandBC = %8.2f.",
                townPointer->m_id,
                curHero,
                worth,
                adjusted
            );
            LogStr(gText);
        }
    }
    best.pTown = townPointer;
    best.type = PURCHASE_HERO;
    best.what = bestHero;
    bestValue = bestCost;
    if (gpGame->m_map[townPointer->m_x][townPointer->m_y].m_triggerType
        == (MAP_TRIGGER_EVENT | MAP_OBJECT_HERO))
        bestValue -= 200.0f;
}

// donor PoL RVA 0x0003e459; preferred Buka symbol ?LikelihoodOfEnemyAttacking@philAI@@QAEXPAVtown@@PAVhero@@AAM2AAH332@Z
// donor Buka TU SOURCE/PHILAI; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.488198;margin=0.360468;shape=0.323;size=0.832;calls=1.000;alternate=pol20:void philAI::LikelihoodOfEnemyAttacking(class town *, class hero *, float &, float &, int &, int &, int &, float &)@0x0003e459
VA(0x0041e89c, 0x65)
void philAI::LikelihoodOfEnemyAttacking(
    town*,
    hero*,
    float& chanceA,
    float& chanceB,
    int& nAttack,
    int& nValue,
    int& nWeeks,
    float& fOut
) {
    chanceA = 0.15f;
    chanceB = 0.6f;
    nAttack = 3000;
    nValue = static_cast<int>(static_cast<float>(nAttack) * chanceA);
    nWeeks = 6;
    fOut = chanceA * chanceB;
}

// This zero result is the Buka 2.1 body and the pinned retail instruction.
VA(0x0041e901, 0x1a)
int philAI::MeanRVOfUnexploredTerritory(int) {
    return 0;
}

// Buka 2.1 GetGameAttentionValue: randomized game weights tempered by the
// number of players.
VA(0x0041e91b, 0x1d3)
void philAI::GetGameAttentionValue(int player) {
    playerAttentionWeights* attention = &gpGame->m_players[player].m_aiData.m_attentionWeights;
    attention->gameWeightA = Random(0, 100) / 500.0 + 0.23;
    attention->gameWeightB = Random(0, 100) / 500.0 + 0.23;
    attention->gameWeightB *= (AI_ATTENTION_IDENTITY_FLOAT + 3.0) / 4.0;
    attention->gameWeightB *= (5.0 - AI_ATTENTION_IDENTITY) / 4.0;
    attention->gameWeightA *= (AI_ATTENTION_IDENTITY + 3.0) / 4.0;
    attention->gameWeightB = attention->gameWeightB * ((3.0 - gpGame->m_playerCount) * 0.15 + 1.0);
    attention->gameWeightA = attention->gameWeightA * ((3.0 - gpGame->m_playerCount) * 0.07 + 1.0);
    attention->gameRemainder = ((1.0f - attention->gameWeightB) - attention->gameWeightA);
}

// Buka 2.1 GetTurnAttentionValue: reset the game weights and scale the hero
// weight down as the game ages.
VA(0x0041eaee, 0xed)
void philAI::GetTurnAttentionValue(int player) {
    playerAttentionWeights* attentionWeights =
        &gpGame->m_players[player].m_aiData.m_attentionWeights;
    attentionWeights->gameWeightA = 0.4f;
    attentionWeights->gameWeightB = 0.3f;
    attentionWeights->gameRemainder = 0.3f;
    attentionWeights->buildingValue = attentionWeights->gameWeightA;
    attentionWeights->heroValue = attentionWeights->gameWeightB;
    attentionWeights->upgradeBase = attentionWeights->gameRemainder;
    float multiplier;
    if (giCurTurn < 5)
        multiplier = 1.6f;
    else if (giCurTurn < 10)
        multiplier = 1.4f;
    else if (giCurTurn < 20)
        multiplier = 1.2f;
    else if (giCurTurn < 30)
        multiplier = 1.0f;
    else
        multiplier = 0.8f;
    attentionWeights->heroValue = attentionWeights->heroValue * multiplier;
}

// donor PoL RVA 0x0003e7a2; preferred Buka symbol ?RVConversion@philAI@@QAEHQAH@Z
// donor Buka TU SOURCE/PHILAI; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.411394;margin=0.429857;shape=0.250;size=0.681;calls=1.000;alternate=pol20:int philAI::RVConversion(int * const)@0x0003e7a2
VA(0x0041ebdb, 0xa6)
int philAI::RVConversion(int* const resources) {
    return static_cast<int>(
        ((((((static_cast<float>(resources[static_cast<int>(RESOURCE_GOLD)])
              * gafAITurnCostResource[static_cast<int>(RESOURCE_GOLD)])
             + static_cast<float>(resources[static_cast<int>(RESOURCE_GEMS)])
                   * gafAITurnCostResource[static_cast<int>(RESOURCE_GEMS)])
            + static_cast<float>(resources[static_cast<int>(RESOURCE_MERCURY)])
                  * gafAITurnCostResource[static_cast<int>(RESOURCE_MERCURY)])
           + static_cast<float>(resources[static_cast<int>(RESOURCE_ORE)])
                 * gafAITurnCostResource[static_cast<int>(RESOURCE_ORE)])
          + static_cast<float>(resources[static_cast<int>(RESOURCE_SULFUR)])
                * gafAITurnCostResource[static_cast<int>(RESOURCE_SULFUR)])
         + static_cast<float>(resources[static_cast<int>(RESOURCE_CRYSTAL)])
               * gafAITurnCostResource[static_cast<int>(RESOURCE_CRYSTAL)])
        + static_cast<float>(resources[static_cast<int>(RESOURCE_WOOD)])
              * gafAITurnCostResource[static_cast<int>(RESOURCE_WOOD)]
    );
}

// Buka 2.1 TurnsToBuy: the slowest shortfall in turns of income, 99 when a
// short resource has no income.
VA(0x0041ec81, 0xca)
float philAI::TurnsToBuy(int* const resources) {
    float maxT = 0;
    int resourceIndex;
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
VA(0x0041ed4b, 0x55e)
int philAI::RVOfPosition(
    hero* pHero,
    short x,
    short y,
    signed char hasEvent,
    short eventX,
    short eventY,
    signed char hasStrategicEvent,
    short strategicX,
    short strategicY,
    int eventMode
) {
    int iMonsterChance;
    int totalValue;
    int estLiveChance;
    int delta;
    int curLocType;
    int curEventVal;
    float estTurns;
    int heroLiveChance;
    int theTargetLiveChance;
    int curEventChance;
    int adjacentEventChance;
    int strategicEventValue;
    int curStrategicValue;
    int adjacentX;
    int curTriggerType;
    int adjacentY;

    strategicEventValue = 0;
    theTargetLiveChance = 100;
    adjacentEventChance = 100;
    curTriggerType = gpAdvManager->GetCell(x, y)->m_triggerType;
    curLocType = curTriggerType & MAP_TRIGGER_TYPE_MASK;
    curEventChance = 100;
    estLiveChance = 100;
    iMonsterChance = 100;
    curStrategicValue = StrategicValueOfPosition(pHero, pHero->m_x, pHero->m_y, 0, &heroLiveChance);
    delta = StrategicValueOfPosition(pHero, x, y, 0, &theTargetLiveChance);
    if (curLocType == MAP_OBJECT_SHIP && delta < 0)
        delta = 0;
    totalValue = 0;
    if (hasEvent)
        totalValue += ValueOfEventAtPosition(pHero, eventX, eventY, 1, &estLiveChance);
    if (hasStrategicEvent) {
        strategicEventValue =
            StrategicValueOfPosition(pHero, strategicX, strategicY, 1, &adjacentEventChance);
        if (strategicEventValue < 0)
            totalValue += strategicEventValue;
    }
    if (gpAdvManager->FindAdjacentMonster(
            x,
            y,
            &adjacentX,
            &adjacentY,
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
                curEventVal =
                    ValueOfEventAtPosition(pHero, adjacentX, adjacentY, 1, &iMonsterChance);
                if (curEventVal < 0)
                    totalValue += curEventVal;
                if (estLiveChance == 100)
                    estLiveChance = iMonsterChance;
                else
                    estLiveChance = estLiveChance * iMonsterChance / 100;
                break;
        }
    }
    if ((curTriggerType & MAP_TRIGGER_EVENT)
        || (x == gpCurPlayer->m_ultimateArtifactHintX && y == gpCurPlayer->m_ultimateArtifactHintY))
        curEventVal = ValueOfEventAtPosition(pHero, x, y, eventMode, &curEventChance);
    else
        curEventVal = 0;
    if (curEventChance < 100)
        delta = delta * curEventChance / 100;
    if (theTargetLiveChance < 100) {
        curEventVal = curEventVal * theTargetLiveChance / 100;
        delta = delta * theTargetLiveChance / 100;
    }
    if (adjacentEventChance < 100) {
        curEventVal = curEventVal * adjacentEventChance / 100;
        delta = delta * adjacentEventChance / 100;
    }
    if (estLiveChance < 100) {
        if (totalValue > 0)
            totalValue = (totalValue + curEventVal + delta) * estLiveChance / 100;
        else
            totalValue += (curEventVal + delta) * estLiveChance / 100;
    } else {
        totalValue += curEventVal;
    }
    estTurns = static_cast<float>(gpSearchArray->m_cells[x][y].distance) / pHero->m_mobility;
    if (pHero->m_eventFlags & HERO_EVENT_EMBARKED)
        estTurns = estTurns * 0.5 + 0.5;
    else if (estTurns > 5.0f)
        estTurns *= 3.0f;
    else if (estTurns > 4.0f)
        estTurns = estTurns * 2.5;
    else if (estTurns > 3.0f)
        estTurns = estTurns * 2.0;
    else if (estTurns > 2.0f)
        estTurns = estTurns * 1.7;
    else if (estTurns > 1.5)
        estTurns = estTurns * 1.4;
    else if (estTurns > 1.0f)
        estTurns = estTurns * 1.2;
    totalValue = static_cast<int>(totalValue / (estTurns + 0.2));
    delta = static_cast<int>(delta * 2 / (estTurns + 1.0f));
    if (estLiveChance == 100)
        totalValue += delta;
    if ((pHero->m_eventFlags & HERO_EVENT_EMBARKED) && curTriggerType == MAP_OBJECT_COAST)
        totalValue += 40;
    return totalValue;
}

// Buka SVSearchArray: StrategicValueOfPosition's shared search, constructed
// by its dynamic initializer between RVOfPosition and its first user.
DATA(0x004b0c50)
searchArray SVSearchArray;
RVA_DYNINIT(0x0001f2a9, 0x1a, SVSearchArray)
// Its .CRT$XCU thunk (0x0048e008 -> 0x00419990) opens this retail object:
// int3 padding precedes it and LogTruncate follows without a gap.
RVA_DYNINIT(0x00019990, 0x15, SVSearchArray)

// donor PoL RVA 0x0003ef45; preferred Buka symbol ?StrategicValueOfPosition@philAI@@QAEHHHHHPAHH@Z
// donor Buka TU SOURCE/PHILAI; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.499321;margin=0.324582;shape=0.341;size=0.829;calls=0.957;alternate=pol20:int philAI::StrategicValueOfPosition(int, int, int, int, int *, int)@0x0003ef45
VA(0x0041f2c3, 0x8bd)
int philAI::StrategicValueOfPosition(
    hero* pHero,
    short targetX,
    short targetY,
    signed char immediate,
    int* liveChance
) {
    int nGap;
    searchArray* pSearch;
    int inBoat;
    int extra2;
    mapCell* cell;
    int thisY;
    int newSeedRange;
    int iReach;
    int x;
    int heroIndex;
    int danger;
    int baseTerrain;
    int myValue;
    searchArray* madeSearch;
    int destTerrain;

    if (!immediate && gaiHeroStrategicRVOfPos[targetX][targetY] != RV_UNSET) {
        *liveChance = gaiLiveChanceOfPos[targetX][targetY];
        return gaiHeroStrategicRVOfPos[targetX][targetY];
    }
    myValue = 0;
    madeSearch = NULL;
    *liveChance = 100;
    if (bSVSearchArrayInUse) {
        madeSearch = new searchArray;
        if (!madeSearch)
            MemError();
        pSearch = madeSearch;
    } else {
        bSVSearchArrayInUse = 1;
        pSearch = &SVSearchArray;
    }
    inBoat = pHero->m_eventFlags & HERO_EVENT_EMBARKED;
    if (inBoat && gpAdvManager->GetCell(targetX, targetY)->m_triggerType == MAP_OBJECT_COAST)
        inBoat = 0;
    if (immediate) {
        newSeedRange = 60;
    } else {
        newSeedRange = 36;
        if (gConfig.slowVideo)
            newSeedRange = 24;
    }
    pSearch->SeedPosition(
        targetX,
        targetY,
        MAP_DIRECTION_EAST,
        newSeedRange,
        inBoat,
        0,
        SEARCH_UNLIMITED_COST,
        pHero->m_heroClass,
        SEARCH_INVALID_COORDINATE,
        SEARCH_INVALID_COORDINATE,
        0,
        0
    );
    pSearch->m_cells[targetX][targetY].visited = 0;
    for (x = 0; x < MAP_CELL_GRID_SIZE; x++) {
        for (thisY = 0; thisY < MAP_CELL_GRID_SIZE; thisY++) {
            if (pSearch->m_cells[x][thisY].visited) {
                cell = gpAdvManager->GetCell(x, thisY);
                if ((!immediate && (cell->m_triggerType & MAP_TRIGGER_EVENT))
                    || (immediate
                        && cell->m_triggerType == (MAP_TRIGGER_EVENT | MAP_OBJECT_HERO))) {
                    CheckDoMain(0, 0);
                    myValue += ValueOfEventAtPosition(pHero, x, thisY, 0, &iDummy)
                               / (pSearch->m_cells[x][thisY].distance + 2.0);
                }
                if (cell->m_triggerType == (MAP_TRIGGER_EVENT | MAP_OBJECT_HERO)) {
                    if (gaiHeroLiveChance[cell->m_objectMetadata] == RV_UNSET)
                        ValueOfEventAtPosition(pHero, x, thisY, 0, &iDummy);
                    if (gaiHeroLiveChance[cell->m_objectMetadata] != RV_UNSET
                        && gaiHeroLiveChance[cell->m_objectMetadata] < 100) {
                        iReach = gpGame->GetHero(cell->m_objectMetadata)->m_mobility;
                        if (gbHumanPlayer[gpGame->m_availableHeroes[cell->m_objectMetadata]]) {
                            if (pSearch->m_cells[x][thisY].distance <= iReach) {
                                if (pSearch->m_cells[x][thisY].distance <= 14)
                                    danger = 100 - gaiHeroLiveChance[cell->m_objectMetadata];
                                else
                                    danger = (iReach - pSearch->m_cells[x][thisY].distance + 10)
                                             * (100 - gaiHeroLiveChance[cell->m_objectMetadata])
                                             / iReach;
                            } else {
                                danger = static_cast<int>(
                                    (100 - gaiHeroLiveChance[cell->m_objectMetadata]) * 0.2
                                );
                            }
                        } else {
                            danger = (iReach + 20 - pSearch->m_cells[x][thisY].distance)
                                     * (100 - gaiHeroLiveChance[cell->m_objectMetadata])
                                     / (iReach + 20);
                        }
                        *liveChance = (100 - danger) * *liveChance / 100;
                    }
                }
                if (pSearch->m_cells[x][thisY].distance < 32
                    && gpAdvManager->GetCell(x, thisY)->m_triggerType
                           == (MAP_TRIGGER_EVENT | MAP_OBJECT_HERO)
                    && gpAdvManager->GetCell(x, thisY)->m_objectMetadata != pHero->m_id
                    && gpGame->m_availableHeroes[gpAdvManager->GetCell(x, thisY)->m_objectMetadata]
                           == pHero->m_owner)
                    myValue -= (32 - pSearch->m_cells[x][thisY].distance) * 1250 >> 5;
            }
        }
    }
    baseTerrain = CELL_TERRAIN(gpAdvManager->GetCell(targetX, targetY));
    for (heroIndex = 0; heroIndex < gpCurPlayer->m_heroCount; heroIndex++) {
        if (gpCurPlayer->m_heroIds[heroIndex] != pHero->m_id) {
            nGap =
                abs(gpGame->m_heroRecs[gpCurPlayer->m_heroIds[heroIndex]].m_destinationX - targetX)
                + abs(
                    gpGame->m_heroRecs[gpCurPlayer->m_heroIds[heroIndex]].m_destinationY - targetY
                );
            if (nGap < 9) {
                destTerrain = giGroundToTerrain
                    [gpAdvManager
                         ->GetCell(
                             gpGame->m_heroRecs[gpCurPlayer->m_heroIds[heroIndex]].m_destinationX,
                             gpGame->m_heroRecs[gpCurPlayer->m_heroIds[heroIndex]].m_destinationY
                         )
                         ->m_tileIndex];
                if (!((baseTerrain == TERRAIN_WATER && destTerrain > TERRAIN_WATER_LAST)
                      || (baseTerrain > TERRAIN_WATER_LAST && destTerrain == TERRAIN_WATER)))
                    myValue -= (9 - nGap) * 1250 / 9;
            }
        }
    }
    if (madeSearch)
        delete madeSearch;
    else
        bSVSearchArrayInUse = 0;
    myValue = static_cast<int>(myValue * AI_STRATEGIC_POSITION_SCORE_FACTOR);
    if (myValue > 32000)
        myValue = 32000;
    if (!immediate) {
        gaiHeroStrategicRVOfPos[targetX][targetY] = myValue;
        gaiLiveChanceOfPos[targetX][targetY] = *liveChance;
    }
    return myValue;
}

// ValueOfEventAtPosition module state (.bss order follows names, not position).
DATA(0x004af7d4)
int iArtifactChoice2;

// Buka 2.1 ValueOfTown without the later scenario-town bonuses: built
// structures' base values plus a fixed gold-turn allowance.
VA(0x0041fb80, 0xb1)
int philAI::ValueOfTown(town* townPointer) {
    int sum = 0;
    int building;
    for (building = BUILDING_SLOT_MAGE_GUILD; building < BUILDING_SLOT_COUNT; building++) {
        if (townPointer->m_buildings & (1 << building))
            sum += GetBuildingBaseResourceValue(
                townPointer->m_type,
                building,
                __max(townPointer->m_buildState, 0)
            );
    }
    sum = static_cast<int>(sum + gafAITurnCostResource[RESOURCE_GOLD] * 1250.0f * 1.5);
    sum += 750;
    return sum;
}

// Buka 2.1 TurnCostResource: each resource's turn cost scales its base
// value against the player's relative stock-plus-income share.
VA(0x0041fc31, 0x176)
void philAI::TurnCostResource(int player) {
    playerAIData* playerAI;
    float ratio[RESOURCE_COUNT];
    float avg;
    int i;
    int totalRV;
    int value[RESOURCE_COUNT];
    playerAI = &gpGame->m_players[player].m_aiData;
    totalRV = 0;
    for (i = 0; i < RESOURCE_COUNT; i++) {
        value[i] = static_cast<int>(
            gResourceBaseValue[i]
            * ((playerAI->m_income[i] * 5) * 0.7 + gpGame->m_players[player].m_resources[i])
        );
        totalRV += value[i];
    }
    avg = (totalRV / RESOURCE_COUNT);
    for (i = 0; i < RESOURCE_COUNT; i++) {
        ratio[i] = value[i] / avg;
        gafAITurnCostResource[i] = (gResourceBaseValue[i] / (ratio[i] / 2.0f + 0.5));
    }
}

// Buka 2.1 TurnValueOfObelisk without the later victory/explorer terms.
VA(0x0041fda7, 0x134)
float philAI::TurnValueOfObelisk(int player) {
    playerAIData* playerAI;
    int each;
    playerAI = &gpGame->m_players[player].m_aiData;
    each = gArtifactBaseRV[gpGame->m_ultimateArtifactId] / 110;
    if (gpGame->m_ultimateArtifactId == ARTIFACT_NONE)
        return 0.0f;
    playerAI->m_obeliskValue = each * 48 / gpGame->m_obeliskCount;
    playerAI->m_obeliskValue = static_cast<int>(
        playerAI->m_obeliskValue
        * (1.5 - abs(32 - gpGame->m_players[player].CountVisitedObelisks()) / 48.0f)
    );
    playerAI->m_obeliskValue = static_cast<int>(
        playerAI->m_obeliskValue * (playerAI->m_attentionWeights.heroValue + 0.66)
    );
    return playerAI->m_obeliskValue;
}

// donor PoL RVA 0x0003fe81; preferred Buka symbol ?FutureDeflator@philAI@@QAEMQAH@Z
// donor Buka TU SOURCE/PHILAI; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.484127;margin=0.409347;shape=0.286;size=0.877;calls=1.000;alternate=pol20:float philAI::FutureDeflator(int * const)@0x0003fe81
VA(0x0041fedb, 0x51)
float philAI::FutureDeflator(int* const resources) {
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
VA(0x0041ff2c, 0x764)
int philAI::FightValueOfStack(
    armyGroup* group,
    hero* heroPointer,
    int useHero,
    signed char useTown,
    signed char townId
) {
    int worth;
    float fPowerMod;
    int spellScore;
    int bestValue;
    int slot;
    int armyWorth;
    int nArrows;
    int luck;
    int castleValue;
    town* pTown;
    int stats;
    int morale;
    int magicTotal;
    float quantityMod;

    armyWorth = 0;
    magicTotal = 0;
    castleValue = 0;
    for (slot = 0; slot < ARMY_GROUP_SLOT_COUNT; slot++) {
        if (group->m_creatureTypes[slot] != CREATURE_NONE) {
            worth = gMonsterDatabase[group->m_creatureTypes[slot]].fightValue
                    * group->m_creatureCounts[slot];
            if (useHero) {
                if (group->m_creatureCounts[slot] > 180)
                    quantityMod = 1.7f;
                else if (group->m_creatureCounts[slot] > 140)
                    quantityMod = 1.3f;
                else if (group->m_creatureCounts[slot] > 100)
                    quantityMod = 1.1f;
                else if (group->m_creatureCounts[slot] > 75)
                    quantityMod = 0.95f;
                else if (group->m_creatureCounts[slot] > 50)
                    quantityMod = 0.81f;
                else if (group->m_creatureCounts[slot] > 35)
                    quantityMod = 0.57f;
                else if (group->m_creatureCounts[slot] > 23)
                    quantityMod = 0.37f;
                else if (group->m_creatureCounts[slot] > 16)
                    quantityMod = 0.25f;
                else if (group->m_creatureCounts[slot] > 11)
                    quantityMod = 0.13f;
                else if (group->m_creatureCounts[slot] > 8)
                    quantityMod = 0.06f;
                else if (group->m_creatureCounts[slot] > 5)
                    quantityMod = 0.0f;
                else if (group->m_creatureCounts[slot] > 3)
                    quantityMod = -0.05f;
                else if (group->m_creatureCounts[slot] > 2)
                    quantityMod = -0.1f;
                else
                    quantityMod = -0.14f;
                if ((gMonsterDatabase[group->m_creatureTypes[slot]].stats.attributes
                     & MONSTER_FLAGS_SHOOTER)
                    || group->m_creatureTypes[slot] == CREATURE_SPRITE
                    || group->m_creatureTypes[slot] == CREATURE_ROGUE)
                    quantityMod = quantityMod * 0.7;
                else if (group->m_creatureTypes[slot] == CREATURE_GRIFFIN)
                    quantityMod = quantityMod * 1.2;
                worth = static_cast<int>(worth * (quantityMod + 1.0f));
            }
            armyWorth += worth;
        }
    }
    if (useTown) {
        nArrows = 5;
        pTown = gpGame->GetTown(townId);
        for (slot = BUILDING_SLOT_DWELLING_FIRST; slot <= BUILDING_SLOT_DWELLING_LAST; slot++)
            if (pTown->m_buildings & (1 << slot))
                nArrows += 4;
        for (slot = BUILDING_SLOT_MAGE_GUILD; slot <= BUILDING_SLOT_GENERIC_LAST; slot++)
            if (pTown->m_buildings & (1 << slot))
                nArrows++;
        castleValue = nArrows * 120;
    }
    if (useHero && heroPointer) {
        stats = heroPointer->m_primaryStats[HERO_PRIMARY_ATTACK]
                + heroPointer->m_primaryStats[HERO_PRIMARY_DEFENSE] + 20;
        if (stats < 0)
            stats = 0;
        if (stats > 40)
            stats = 40;
        armyWorth = static_cast<int>(armyWorth * gfStatPower[stats]);
        castleValue = static_cast<int>(castleValue * gfStatPower[stats]);
        morale = heroPointer->m_army.GetMorale(heroPointer, NULL);
        if (morale > 0)
            armyWorth = armyWorth * (morale + 48) / 48;
        else if (morale < 0)
            armyWorth = armyWorth * (morale + 24) / 24;
        luck = gpGame->GetLuck(heroPointer, NULL);
        if (luck)
            armyWorth = armyWorth * (luck + 16) / 16;
        if (heroPointer->m_primaryStats[HERO_PRIMARY_SPELL_POWER] == 1)
            fPowerMod = 0.25f;
        else if (heroPointer->m_primaryStats[HERO_PRIMARY_SPELL_POWER] == 2)
            fPowerMod = 0.5f;
        else
            fPowerMod = 1.0f;
        bestValue = -1;
        for (slot = 0; slot < HERO_SPELL_SLOT_COUNT; slot++) {
            if (heroPointer->m_spells[slot] >= 0
                && (gcSpellAIFlags[heroPointer->m_spells[slot]] & SPELL_AI_FLAG_COMBAT)) {
                spellScore = static_cast<int>(
                    giSpellAIValue[heroPointer->m_spells[slot]]
                    * ((gcSpellAIFlags[heroPointer->m_spells[slot]]
                        & SPELL_AI_FLAG_SCALES_WITH_POWER)
                           ? (heroPointer->m_primaryStats[HERO_PRIMARY_SPELL_POWER] > 40
                                  ? gfBattleStat[40]
                                  : gfBattleStat[heroPointer
                                                     ->m_primaryStats[HERO_PRIMARY_SPELL_POWER]])
                           : fPowerMod)
                );
                magicTotal +=
                    spellScore
                    * gfSpellCastNumMod
                        [heroPointer->m_spellCharges[slot] < 20 ? heroPointer->m_spellCharges[slot]
                                                                : 20];
                if (bestValue < spellScore)
                    bestValue = spellScore;
            }
        }
        if (magicTotal > bestValue * 3.5)
            magicTotal = static_cast<int>(bestValue * 3.5);
        if (magicTotal > armyWorth * 2)
            magicTotal = static_cast<int>(armyWorth * 1.25);
        else if (magicTotal > armyWorth * 1.5)
            magicTotal = armyWorth;
        else if (magicTotal > armyWorth)
            magicTotal = static_cast<int>(armyWorth * 0.75);
    }
    if (castleValue > armyWorth * 2)
        castleValue = static_cast<int>(armyWorth * 1.5);
    else if (castleValue > armyWorth * 1.5)
        castleValue = static_cast<int>(armyWorth * 1.25);
    else if (castleValue > armyWorth)
        castleValue = static_cast<int>(armyWorth * 0.9);
    armyWorth += magicTotal;
    armyWorth += castleValue;
    return armyWorth;
}

// donor PoL RVA 0x00040aca; preferred Buka symbol ?EvaluateOneTimeCreaturePurchase@philAI@@QAEXHHHAAH00@Z
// donor Buka TU SOURCE/PHILAI; HoMM1 owner inferred from contiguous order
// evidence: graph:3;base=0.513526;margin=0.703256;shape=0.342;size=0.854;calls=1.000;alternate=pol20:void philAI::EvaluateOneTimeCreaturePurchase(int, int, int, int &, int &, int &)@0x00040aca
VA(0x00420690, 0x1da)
void philAI::EvaluateOneTimeCreaturePurchase(
    hero* pHero,
    int creature,
    int availableCount,
    int useAvailableCount,
    int& purchaseCount,
    int& purchaseValue,
    int& replacementSlot
) {
    int leastStackValue;
    int replacementValue;
    int purchaseFightValue;
    int i;

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
    purchaseFightValue = purchaseCount * gMonsterDatabase[creature].fightValue;
    if (pHero->m_army.CanJoin(creature) == 0) {
        for (i = 0; i < ARMY_GROUP_SLOT_COUNT; i++) {
            if (pHero->m_army.m_creatureTypes[i] == creature) {
                replacementSlot = -1;
                i = ARMY_GROUP_SLOT_COUNT;
            } else {
                replacementValue =
                    pHero->m_army.m_creatureCounts[i] * gMonsterDatabase[i].fightValue;
                if (replacementValue < leastStackValue) {
                    leastStackValue = replacementValue;
                    replacementSlot = i;
                }
            }
        }
    }
    if (replacementSlot != -1)
        purchaseFightValue -= leastStackValue;
    purchaseValue = static_cast<int>(
        purchaseFightValue * gpGame->m_players[pHero->m_owner].m_aiData.m_upgradeValueWeight
    );
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
VA(0x0042086a, 0x3a7)
int philAI::QuickCombat(
    armyGroup* attacker,
    hero* attackerHero,
    armyGroup* defender,
    hero* defenderHero,
    signed char townBattle,
    signed char townId,
    float& attackerDamage,
    float& defenderDamage
) {
    int aDead;
    float fracLost;
    int win;
    float rnd;
    int unused;
    int dLeft;
    int dDead;
    armyGroup* winner;
    float diff;
    float winChance;
    int atkExp;
    int tmp;
    int defenderExp;
    float wChance;
    int res;
    int aLeft;

    atkExp = gpGame->ExperienceValueOfStack(attacker, attackerHero);
    defenderExp = gpGame->ExperienceValueOfStack(defender, defenderHero);
    win = 0;
    winner = NULL;
    ProbableOutcomeOfBattle(
        attacker,
        attackerHero,
        defender,
        defenderHero,
        NULL,
        townBattle,
        townId,
        defenderHero != NULL ? defenderHero->m_owner : -1,
        winChance,
        aDead,
        dDead,
        aLeft,
        dLeft,
        res
    );
    rnd = Random(0, 100) / 100.0;
    if (rnd < winChance) {
        win = 1;
        wChance = winChance;
        winner = attacker;
    } else {
        wChance = 1.0f - winChance;
        winner = defender;
    }
    // VC4 narrows this double through a stack temporary only for the C-style
    // cast (retail frame 0x50); static_cast<float> drops the slot.
    diff = (float)(rnd > winChance ? rnd - winChance : winChance - rnd);
    if (win != 0 && winChance > 0.6)
        diff *= winChance + 0.65;
    fracLost = (1.0 - diff) * (1.0 - diff);
    if (wChance > 0.8 && fracLost > 0.2)
        fracLost *= fracLost;
    if (wChance > 0.96 && fracLost > (1.0f - wChance) / 2.0f)
        fracLost = (1.0f - wChance) / 2.0f;
    if (win != 0) {
        if (attackerHero != NULL) {
            gpAdvManager->GiveExperience(attackerHero, defenderExp, 1);
            attackerHero->ApplyBattleWinTemps();
        }
        defenderDamage = 1.0f;
        attackerDamage = fracLost;
    } else {
        if (attackerHero != NULL) {
            attackerHero->m_remainingMobility = 0;
            attackerHero->ApplyBattleLossTemps();
        }
        if (defenderHero != NULL)
            attackerHero->ApplyBattleWinTemps();
        defenderDamage = diff * fracLost;
        attackerDamage = 1.0f;
        if (attackerDamage >= 0.99 && defenderHero != NULL)
            gpAdvManager->GiveExperience(defenderHero, defenderExp, 1);
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
VA(0x00420c11, 0xbae)
void philAI::HeroInteractionAtTown(
    hero* heroPointer,
    town* townPointer,
    int doInteraction,
    int* value
) {
    int transferRating;
    int pick;
    int speedLimit;
    int stackFV;
    int nRunning;
    float fTownShare;
    int curBest;
    int garrisonFV;
    int heroStrength;
    int statSum;
    int armyCount;
    int moveNum;
    int j;
    float estWeight;
    int estTransferValue;
    int toHero;
    float curveTerm;
    armyGroup* fromArmy;
    int room;
    float fShareDiff;
    int newLearned;
    float myTargetShare;
    armyGroup* toArmy;
    int i;

    *value = 0;
    if (doInteraction) {
        if ((townPointer->m_buildings & (1 << BUILDING_SLOT_SHIPYARD))
            && townPointer->m_id != giBestShipyardId) {
            i = MANHATTAN_LENGTH(townPointer->m_x - heroPointer->m_x, townPointer->m_y - heroPointer->m_y);
            if (gbActualShipyardFound) {
                if (giBestShipyardDist > i) {
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
                   && !gbActualShipyardFound && townPointer->m_id != giBestShipyardId) {
            i = MANHATTAN_LENGTH(townPointer->m_x - heroPointer->m_x, townPointer->m_y - heroPointer->m_y);
            if (gbPossibleShipyardFound) {
                if (giBestShipyardDist > i) {
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
            newLearned = heroPointer->AddSpell(
                townPointer->m_mageGuildSpells[i],
                heroPointer->m_primaryStats[HERO_PRIMARY_KNOWLEDGE],
                doInteraction
            );
            *value += StatChangeValue(
                          heroPointer->m_primaryStats[HERO_PRIMARY_KNOWLEDGE] - newLearned,
                          heroPointer->m_primaryStats[HERO_PRIMARY_KNOWLEDGE]
                      )
                      * giSpellAIValue[townPointer->m_mageGuildSpells[i]]
                      * ((gcSpellAIFlags[townPointer->m_mageGuildSpells[i]]
                          & SPELL_AI_FLAG_SCALES_WITH_POWER)
                             ? heroPointer->m_primaryStats[HERO_PRIMARY_KNOWLEDGE]
                             : 1);
        }
    }
    heroStrength = FightValueOfStack(&heroPointer->m_army, NULL, 0, 0, 0);
    garrisonFV = FightValueOfStack(&townPointer->m_army, NULL, 0, 0, 0);
    fTownShare = static_cast<float>(garrisonFV) / (heroStrength + garrisonFV);
    statSum = 0;
    statSum = heroPointer->m_primaryStats[HERO_PRIMARY_ATTACK]
              + heroPointer->m_primaryStats[HERO_PRIMARY_DEFENSE];
    if (statSum > 10)
        statSum = 10;
    if (townPointer->m_buildings & (1 << BUILDING_SLOT_CASTLE))
        myTargetShare = 0.54 - statSum * 0.02;
    else
        myTargetShare = 0.33 - statSum * 0.01;
    fShareDiff =
        (myTargetShare < fTownShare ? fTownShare - myTargetShare : myTargetShare - fTownShare);
    if (myTargetShare * 0.15 > fShareDiff)
        return;
    toHero = 0;
    if (fTownShare > myTargetShare)
        toHero = 1;
    if (doInteraction) {
        if (heroStrength < garrisonFV)
            estWeight = 0.25f;
        else
            estWeight = 0.13f;
        curveTerm = fShareDiff + 1.0f - 0.22;
        transferRating = static_cast<int>(
            (curveTerm * curveTerm - 1.0f) * (heroStrength + garrisonFV)
            * gpCurPlayer->m_aiData.m_upgradeValueWeight * estWeight
        );
        if (transferRating < 0)
            transferRating = 0;
        room = 0;
        if (toHero) {
            for (i = 0; i < ARMY_GROUP_SLOT_COUNT; i++)
                if (heroPointer->m_army.m_creatureCounts[i] <= 0)
                    room = 1;
        } else {
            for (i = 0; i < ARMY_GROUP_SLOT_COUNT; i++)
                if (townPointer->m_army.m_creatureCounts[i] <= 0)
                    room = 1;
        }
        if (!room) {
            for (i = 0; i < ARMY_GROUP_SLOT_COUNT; i++) {
                for (j = 0; j < ARMY_GROUP_SLOT_COUNT; j++) {
                    if (townPointer->m_army.m_creatureTypes[i]
                        == heroPointer->m_army.m_creatureTypes[j]) {
                        room = 1;
                        break;
                    }
                }
            }
        }
        if (!room)
            transferRating = 0;
        *value += transferRating;
        return;
    }
    if (toHero)
        fShareDiff = fShareDiff + 0.04;
    estTransferValue = static_cast<int>((heroStrength + garrisonFV) * fShareDiff);
    fromArmy = toHero ? &townPointer->m_army : &heroPointer->m_army;
    toArmy = toHero ? &heroPointer->m_army : &townPointer->m_army;
    nRunning = 1;
    gbTroopReload = 0;
    while (nRunning) {
        if (!toHero) {
            armyCount = 0;
            for (i = 0; i < ARMY_GROUP_SLOT_COUNT; i++)
                if (heroPointer->m_army.m_creatureTypes[i] != CREATURE_NONE)
                    armyCount += heroPointer->m_army.m_creatureCounts[i];
            if (armyCount <= 1)
                return;
        }
        pick = -1;
        for (i = 0; i < ARMY_GROUP_SLOT_COUNT; i++) {
            if (pick == -1) {
                for (j = 0; j < ARMY_GROUP_SLOT_COUNT; j++) {
                    if (fromArmy->m_creatureTypes[i] != CREATURE_NONE
                        && fromArmy->m_creatureTypes[i] == toArmy->m_creatureTypes[j]) {
                        pick = i;
                        break;
                    }
                }
            }
        }
        if (pick == -1) {
            curBest = -9999;
            if (toHero)
                speedLimit = 1;
            else
                speedLimit = 3;
            for (i = 0; i < ARMY_GROUP_SLOT_COUNT; i++) {
                if (fromArmy->m_creatureTypes[i] != CREATURE_NONE) {
                    stackFV = gMonsterDatabase[fromArmy->m_creatureTypes[i]].fightValue
                              * fromArmy->m_creatureCounts[i];
                    if ((toHero
                         && gMonsterDatabase[fromArmy->m_creatureTypes[i]].stats.speed > speedLimit)
                        || (!toHero
                            && gMonsterDatabase[fromArmy->m_creatureTypes[i]].stats.speed
                                   < speedLimit)) {
                        speedLimit = gMonsterDatabase[fromArmy->m_creatureTypes[i]].stats.speed;
                        curBest = stackFV;
                        pick = i;
                    } else if (gMonsterDatabase[fromArmy->m_creatureTypes[i]].stats.speed
                                   == speedLimit
                               && curBest < stackFV) {
                        curBest = stackFV;
                        pick = i;
                    }
                }
            }
        }
        if (pick == -1) {
            nRunning = 0;
        } else if (toArmy->CanJoin(fromArmy->m_creatureTypes[pick])) {
            moveNum = static_cast<int>(
                static_cast<float>(estTransferValue)
                    / gMonsterDatabase[fromArmy->m_creatureTypes[pick]].fightValue
                + 0.5
            );
            if (moveNum > 0) {
                if (fromArmy->m_creatureCounts[pick] < moveNum) {
                    moveNum = fromArmy->m_creatureCounts[pick];
                } else {
                    nRunning = 0;
                    if (moveNum >= fromArmy->m_creatureCounts[pick] * 0.65
                        || moveNum >= fromArmy->m_creatureCounts[pick] - 1) {
                        if (((toHero ? garrisonFV : heroStrength) - estTransferValue) * 0.2
                            > gMonsterDatabase[fromArmy->m_creatureTypes[pick]].fightValue
                                  * (fromArmy->m_creatureCounts[pick] - moveNum))
                            moveNum = fromArmy->m_creatureCounts[pick];
                    }
                }
                if (!toHero && moveNum >= armyCount) {
                    moveNum = armyCount - 1;
                    nRunning = 0;
                }
                if (estTransferValue
                    < gMonsterDatabase[fromArmy->m_creatureTypes[pick]].fightValue * moveNum * 1.2)
                    nRunning = 0;
                else
                    estTransferValue -=
                        gMonsterDatabase[fromArmy->m_creatureTypes[pick]].fightValue * moveNum;
                toArmy->Add(fromArmy->m_creatureTypes[pick], moveNum, ARMY_GROUP_EMPTY_SLOT);
                fromArmy->m_creatureCounts[pick] -= moveNum;
                if (fromArmy->m_creatureCounts[pick] == 0)
                    fromArmy->m_creatureTypes[pick] = CREATURE_NONE;
            } else {
                nRunning = 0;
            }
        } else {
            nRunning = 0;
        }
    }
    if (!doInteraction && townPointer->m_id == giHumanTownConquered
        && heroPointer->m_remainingMobility <= 20)
        heroPointer->m_remainingMobility = 0;
}

// Buka 2.1 ChooseGoldOrExperience; HoMM1 weighs the experience by the
// hero's AI fight value instead of a fixed gold threshold.
VA(0x004217bf, 0x61)
int philAI::ChooseGoldOrExperience(hero* thisHero, int gold, int experience) {
    int goldRV;
    int expRV;

    expRV = static_cast<int>(experience * thisHero->m_aiFightValue);
    goldRV = static_cast<int>(gold * gafAITurnCostResource[RESOURCE_GOLD]);
    return goldRV > expRV;
}

// donor PoL RVA 0x000425b0; preferred Buka symbol ?ChooseEvaluateBattle@philAI@@QAEXPAVarmyGroup@@PAVhero@@01HHHAAH2@Z
// donor Buka TU SOURCE/PHILAI; HoMM1 owner inferred from contiguous order
// evidence: graph:3;base=0.572029;margin=0.742196;shape=0.490;size=0.824;calls=1.000;alternate=pol20:void philAI::ChooseEvaluateBattle(class armyGroup *, class hero *, class armyGroup *, class hero *, int, int, int, int &, int &)@0x000425b0
VA(0x00421820, 0xc7)
void philAI::ChooseEvaluateBattle(
    armyGroup* attackerArmy,
    hero* attackerHero,
    armyGroup* defenderArmy,
    hero* defenderHero,
    int isCastle,
    int castleId,
    int rewardValue,
    int& outFlag,
    int& outValue
) {
    float chance;
    int lossA;
    int lossB;
    int leftA;
    int leftB;
    int empty;
    int score;

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
        lossA,
        lossB,
        leftA,
        leftB,
        score
    );
    score = static_cast<int>(score + rewardValue * chance);
    if (score <= 0) {
        outValue = 0;
        outFlag = 0;
    } else {
        outValue = score;
        outFlag = 1;
    }
}

// HoMM1 treasure-artifact purchase: affordable gold and an artifact worth
// more than its gold cost (Buka NetValueOfArtifact's valuation).
VA(0x004218e7, 0x69)
int philAI::ChooseToBuyArtifact(hero*, int artifact, int goldCost) {
    if (gpCurPlayer->m_resources[RESOURCE_GOLD] >= goldCost
        && gArtifactBaseRV[artifact] > goldCost * gafAITurnCostResource[RESOURCE_GOLD])
        return 1;
    else
        return 0;
}

// Buka 2.1 returns one for the ransom choice. HoMM1's daemon-cave caller
// passes a hero and the gold amount; the retail body returns the same one.
VA(0x00421950, 0x1d)
int philAI::ChooseToPayRansomOnHero(hero*, int) {
    return 1;
}

// Buka 2.1 BuildBuilding with HoMM1's town update written in place: the mage
// guild level, castle conversion and new dwelling stock.
VA(0x0042196d, 0x194)
void philAI::BuildBuilding(town* townPointer, short building) {
    int i;
    int cost[RESOURCE_COUNT];

    sprintf(
        gText,
        "Player %d built %s in town %d.\n",
        giCurPlayer,
        GetBuildingName(townPointer->m_type, building),
        townPointer->m_id
    );
    LogStr(gText);
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
VA(0x00421b01, 0x25c)
void philAI::BuildHero(town* townPointer, short availableHeroIndex) {
    hero* newHero;
    short townX;
    short townY;

    sprintf(gText, "Player %d built hero in town %d.\n", giCurPlayer, townPointer->m_id);
    LogStr(gText);
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
VA(0x00421d5d, 0x100)
void philAI::BuildCreature(town* townPointer, int dwelling, int purchaseCount) {
    int cost[RESOURCE_COUNT];
    int creature;
    int i;

    sprintf(
        gText,
        "Player %d built %d %s in town %d.\n",
        giCurPlayer,
        purchaseCount,
        GetMonsterName(gDwellingType[townPointer->m_type][dwelling]),
        townPointer->m_id
    );
    LogStr(gText);
    creature = gDwellingType[townPointer->m_type][dwelling];
    GetMonsterCost(creature, cost);
    for (i = 0; i < RESOURCE_COUNT; i++)
        gpCurPlayer->m_resources[i] -= cost[i] * purchaseCount;
    townPointer->m_garrison[dwelling] -= purchaseCount;
    townPointer->m_army.Add(creature, purchaseCount, ARMY_GROUP_EMPTY_SLOT);
    ShowStatus();
}

// donor PoL RVA 0x00042ead; preferred Buka symbol ?CanBuyBHC@philAI@@QAEHAAUBHC@@@Z
// donor Buka TU SOURCE/PHILAI; HoMM1 owner inferred from contiguous order
// evidence: graph:3;base=0.453882;margin=0.780446;shape=0.369;size=0.712;calls=0.667;alternate=pol20:int philAI::CanBuyBHC(struct BHC &)@0x00042ead
VA(0x00421e5d, 0x188)
int philAI::CanBuyBHC(BHC& purchase) {
    int index;
    int j;
    int cost[RESOURCE_COUNT];
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
VA(0x00421fe5, 0x177)
signed char philAI::CombatMonsterEvent(hero* h, signed char monType, int* pCount, mapCell*) {
    float casualtyRatio;
    float fLoss;
    int result;
    short newCount;
    short i;

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
VA(0x0042215c, 0x26a)
void philAI::FightEvent(hero* heroPointer, mapCell* cell) {
    float attackerLoss;
    int rewardValue;
    int unusedValue;
    int evalValue;
    short guards[4];
    float defenderLoss;
    int flag;
    short n;
    int won;

    if (cell->m_objectMetadata == GHOST_SITE_EMPTY)
        return;
    guards[0] = 2;
    guards[1] = 3;
    guards[2] = 5;
    guards[3] = 10;
    for (n = 0; n < ARMY_GROUP_SLOT_COUNT; n++) {
        gpMonGroup->m_creatureTypes[n] = CREATURE_GHOST;
        gpMonGroup->m_creatureCounts[n] = guards[cell->m_objectMetadata - GHOST_SITE_SMALL];
    }
    switch (cell->m_objectMetadata) {
        case GHOST_SITE_SMALL:
            rewardValue = static_cast<int>(gafAITurnCostResource[RESOURCE_GOLD] * 1000.0f);
            break;
        case GHOST_SITE_MEDIUM:
            rewardValue = static_cast<int>(gafAITurnCostResource[RESOURCE_GOLD] * 2000.0f);
            break;
        case GHOST_SITE_LARGE:
            rewardValue = static_cast<int>(gafAITurnCostResource[RESOURCE_GOLD] * 5000.0f);
            break;
        case GHOST_SITE_HUGE:
            rewardValue = static_cast<int>(
                gafAITurnCostResource[RESOURCE_GOLD] * 2000.0f
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
        rewardValue,
        flag,
        evalValue
    );
    if (flag) {
        won = QuickCombat(
            &heroPointer->m_army,
            heroPointer,
            gpMonGroup,
            NULL,
            0,
            0,
            defenderLoss,
            attackerLoss
        );
        if (won) {
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
VA(0x004223c6, 0x73)
int philAI::DamageGroup(armyGroup* ag, hero* loser, hero*, float dmg) {
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
VA(0x00422439, 0x66)
float philAI::StatChangeValue(int oldValue, int newValue) {
    float newRV;
    float oldRV;

    if (newValue > 20)
        newRV = gfSpellCastNumMod[20];
    else
        newRV = gfSpellCastNumMod[newValue];
    if (oldValue > 20)
        oldRV = gfSpellCastNumMod[20];
    else
        oldRV = gfSpellCastNumMod[oldValue];
    return newRV - oldRV;
}

// Buka 2.1 IncrementHourGlass: the AI-turn hourglass advances faster with
// fewer (prospective) heroes and stops at its last phase.
VA(0x0042249f, 0xcb)
void philAI::IncrementHourGlass(void) {
    int heroCount = gpCurPlayer->m_heroCount;
    if (heroCount < 4 && gpCurPlayer->m_resources[RESOURCE_GOLD] >= 2500 && bHeroBuiltThisTurn == 0)
        heroCount++;
    iCurHourGlassPhase++;
    if (heroCount == 1) {
        iCurHourGlassPhase++;
        iCurHourGlassPhase++;
    }
    if (heroCount == 2 && iCurHourGlassPhase != 1)
        iCurHourGlassPhase++;
    if (heroCount == 3 && (iCurHourGlassPhase == 3 || iCurHourGlassPhase == 6))
        iCurHourGlassPhase++;
    if (iCurHourGlassPhase > 9)
        iCurHourGlassPhase = 9;
}

// donor PoL RVA 0x00043980; preferred Buka symbol ?TownEvent@philAI@@QAEXPAVmapCell@@PAVhero@@HH@Z
// donor Buka TU SOURCE/PHILAI; HoMM1 owner inferred from contiguous order
// evidence: graph:3;base=0.582749;margin=0.601748;shape=0.455;size=0.921;calls=1.000;alternate=pol20:void philAI::TownEvent(class mapCell *, class hero *, int, int)@0x00043980
VA(0x0042256a, 0x221)
void philAI::TownEvent(mapCell* cell, hero* heroPointer, int x, int y) {
    float attackerLoss;
    float defenderLoss;
    int owner;
    int quickResult;
    town* townPointer;
    hero* defendingHero;
    int outcome;

    townPointer = gpGame->GetTown(cell->m_objectMetadata);
    owner = giCurPlayer;
    gpAdvManager->DemobilizeCurrHero();
    if (townPointer->m_owner != giCurPlayer) {
        if (townPointer->HasGarrison()) {
            if (townPointer->m_owner < 0 || gbHumanPlayer[townPointer->m_owner] == 0) {
                quickResult = QuickCombat(
                    &heroPointer->m_army,
                    heroPointer,
                    &townPointer->m_army,
                    NULL,
                    1,
                    townPointer->m_id,
                    defenderLoss,
                    attackerLoss
                );
            } else {
                defendingHero = townPointer->m_occupyingHeroId == TOWN_OCCUPYING_HERO_NONE
                                    ? NULL
                                    : gpGame->GetHero(townPointer->m_occupyingHeroId);
                outcome = gpAdvManager->DoCombat(
                    x,
                    y,
                    heroPointer,
                    &heroPointer->m_army,
                    townPointer,
                    defendingHero,
                    &townPointer->m_army,
                    x,
                    y,
                    -1,
                    1
                );
                if (outcome == COMBAT_RESULT_ATTACKER) {
                    gpGame->ClaimTown(townPointer->m_id, giCurPlayer);
                    giHumanTownConquered = townPointer->m_id;
                }
            }
        } else {
            gpGame->ClaimTown(townPointer->m_id, giCurPlayer);
        }
    }
    if (townPointer->m_owner == giCurPlayer && heroPointer->m_x == x && heroPointer->m_y == y) {
        townPointer->m_occupyingHeroId = gpCurPlayer->CurrentHero();
        heroPointer->m_locationType = (MAP_TRIGGER_EVENT | MAP_OBJECT_TOWN);
        heroPointer->m_occupiedTown = townPointer->m_id;
        HeroInteractionAtTown(heroPointer, townPointer, 0, &iDummy);
    }
    gpAdvManager->MobilizeCurrHero(0);
    townPointer->GiveSpells();
}

// ValueOfEventAtPosition module state (.bss order follows names, not position).
DATA(0x004af7d8)
int iArtifactChoice3;
DATA(0x004af7e0)
int iEventTownId;
DATA(0x004be7a4)
int bEventSeen;
DATA(0x004be7a8)
int iPurchaseNum;
DATA(0x004be7d4)
int iPurchaseSlot;
DATA(0x004be7d8)
armyGroup* pEventTownArmy;
DATA(0x004bfc20)
int iDefaultEventType;
DATA(0x004c254c)
mapCell* pEventCell;
DATA(0x004c2550)
int gbReduceByReload;
DATA(0x004c2554)
int gbReduceByBerserk;
DATA(0x004c2558)
town* pEventTown;
DATA(0x004c4ed8)
int iEventRV;
DATA(0x004c4edc)
int iMonsterCount;
DATA(0x004c4ee0)
int iTownValue;
DATA(0x004c4ee8)
hero* pEventHero;

// @early-stop 99.77: the daemon-cave reward sum. Retail adds
// fv*300 + gold*2500.0f first and keeps (fv*100 + m_artifactValue) as a unit;
// VC4 here reassociates the float chain (m_artifactValue moves next to the
// first term, gold*2500.0f after fv*300). Float reassociation is outside
// the vc4trace replay; the solver's 64 TU shifts (32 classes) never beat
// this state and TU-state trials stay at 99.773. Regroupings, swapped
// inner order, double/float casts of either term and paired grouping of
// the tail do not reproduce it. One handle-state cmp operand order
// (locals -0x38/-0x14) also remains.
// donor PoL RVA 0x00043fc4; preferred Buka symbol ?ValueOfEventAtPosition@philAI@@QAEHHHHPAH@Z
// donor Buka TU SOURCE/PHILAI; HoMM1 owner inferred from contiguous order
// evidence: graph:4;base=0.465517;margin=0.659381;shape=0.256;size=0.790;calls=0.952;alternate=pol20:int philAI::ValueOfEventAtPosition(int, int, int, int *)@0x00043fc4
VA(0x0042278b, 0x2083)
int philAI::ValueOfEventAtPosition(hero* pHero, short x, short y, int immediate, int* liveChance) {
    int numToBuy;
    int bWon9;
    int costList[RESOURCE_COUNT];
    int guardCount1;
    int exitRV5;
    mapCell* exitCell;
    int gateY28;
    int bestRV1;
    int gateX1;
    int exitLiveChance;
    int goldCost;
    int armySlot2;
    int positionValue;
    int prize5;
    int bBattleWon9;
    int chosenExitY27;
    int chosenExitX0;

    if (!immediate && gaiHeroEventStratRVOfPos[x][y] != RV_UNSET)
        return gaiHeroEventStratRVOfPos[x][y];
    gbReduceByReload = 1;
    gbReduceByBerserk = 1;
    *liveChance = 100;
    iEventRV = 0;
    pEventCell = gpAdvManager->GetCell(x, y);
    if (mapVisited[x][y] && giCurPlayerBit)
        bEventSeen = 1;
    else
        bEventSeen = 0;
    switch (pEventCell->m_triggerType & MAP_TRIGGER_TYPE_MASK) {
        case MAP_OBJECT_ARTIFACT:
            iArtifactChoice1 = gArtifactBaseRV[pEventCell->m_objectIndex];
            for (iEventLoop = 0; iEventLoop < ARMY_GROUP_SLOT_COUNT; iEventLoop++) {
                gpMonGroup->m_creatureTypes[iEventLoop] = CREATURE_ROGUE;
                gpMonGroup->m_creatureCounts[iEventLoop] = 10;
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
                fWinChance,
                iAttackerLoss,
                iDefenderLoss,
                iAttackerRemaining,
                iDefenderRemaining,
                iOutcome
            );
            iArtifactChoice2 = static_cast<int>(
                gArtifactBaseRV[pEventCell->m_objectIndex] * fWinChance + iOutcome
            );
            iArtifactChoice3 = static_cast<int>(
                gArtifactBaseRV[pEventCell->m_objectIndex]
                - gafAITurnCostResource[RESOURCE_GOLD] * 2000.0f
            );
            if (iArtifactChoice3 < 0)
                iArtifactChoice3 = 0;
            if (bEventSeen) {
                switch (pEventCell->m_objectMetadata) {
                    case ARTIFACT_EVENT_MODE_PICKUP:
                        iEventRV = iArtifactChoice1;
                        break;
                    case ARTIFACT_EVENT_MODE_GUARDED:
                        iEventRV = iArtifactChoice2;
                        break;
                    case ARTIFACT_EVENT_MODE_GOLD:
                        iEventRV = iArtifactChoice3;
                        break;
                }
            } else {
                iEventRV = static_cast<int>(
                    iArtifactChoice1 * 0.6 + iArtifactChoice3 * 0.2 + iArtifactChoice2 * 0.2
                );
            }
            break;
        case 1:
        case MAP_OBJECT_MINE:
        case MAP_OBJECT_SAWMILL:
            if (gpGame->m_mineOwners[pEventCell->m_objectMetadata] == pHero->m_owner) {
                iEventRV = 0;
            } else if (gbIAmGreatest && gpGame->m_mineOwners[pEventCell->m_objectMetadata] >= 0
                       && !gbHumanPlayer[gpGame->m_mineOwners[pEventCell->m_objectMetadata]]) {
                iEventRV = 0;
            } else {
                iEventRV = static_cast<int>(
                    giMineIncome[gpGame->m_mines[pEventCell->m_objectMetadata].type]
                    * gafAITurnCostResource[gpGame->m_mines[pEventCell->m_objectMetadata].type]
                    * gaiTurnValueOfMine[x][y]
                );
                if (gpGame->m_mineOwners[pEventCell->m_objectMetadata] >= 0)
                    iEventRV = static_cast<int>(
                        iEventRV
                        * (gbHumanPlayer[gpGame->m_mineOwners[pEventCell->m_objectMetadata]]
                               ? gfAttackHumanBonus
                               : gfAttackComputerBonus)
                    );
            }
            break;
        case MAP_OBJECT_OBELISK:
            if (gpGame->m_obeliskVisitors[pEventCell->m_objectMetadata - 1] & giCurPlayerBit)
                iEventRV = 0;
            else
                iEventRV = gpCurPlayer->m_aiData.m_obeliskValue;
            break;
        case MAP_OBJECT_MONSTER:
            iMonsterCount = pEventCell->m_objectMetadata & MONSTER_COUNT_MASK;
            CLEAR_ARMY_GROUP(*gpMonGroup);
            if (iMonsterCount / ARMY_GROUP_SLOT_COUNT > 0) {
                for (iEventLoop = 0; iEventLoop < ARMY_GROUP_SLOT_COUNT; iEventLoop++) {
                    gpMonGroup->m_creatureTypes[iEventLoop] = pEventCell->m_objectIndex;
                    gpMonGroup->m_creatureCounts[iEventLoop] =
                        iMonsterCount / ARMY_GROUP_SLOT_COUNT;
                }
            }
            for (iEventLoop = iMonsterCount % ARMY_GROUP_SLOT_COUNT - 1; iEventLoop >= 0;
                 iEventLoop--) {
                gpMonGroup->m_creatureTypes[iEventLoop] = pEventCell->m_objectIndex;
                gpMonGroup->m_creatureCounts[iEventLoop]++;
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
                fWinChance,
                iAttackerLoss,
                iDefenderLoss,
                iAttackerRemaining,
                iDefenderRemaining,
                iOutcome
            );
            EvaluateOneTimeCreaturePurchase(
                pHero,
                pEventCell->m_objectIndex,
                iMonsterCount,
                1,
                numToBuy,
                iAttackerLoss,
                armySlot2
            );
            if ((pEventCell->m_objectMetadata & MONSTER_WILLING_FLAG)
                && gpPhilAI->FightValueOfStack(&pHero->m_army, pHero, 0, 0, 0)
                       > gMonsterDatabase[pEventCell->m_objectIndex].fightValue
                             * (pEventCell->m_objectMetadata & MONSTER_COUNT_MASK) * 1.75) {
                *liveChance = 100;
                *liveChance = static_cast<int>(fWinChance * 60.0f + 40.0f);
                if (pHero->m_army.CanJoin(pEventCell->m_objectIndex))
                    iEventRV = iAttackerLoss;
                else
                    iEventRV = 0;
                iEventRV = static_cast<int>(iEventRV * 0.6 + iOutcome * 0.4);
            } else {
                *liveChance = static_cast<int>(fWinChance * 100.0f);
                iEventRV = iOutcome;
            }
            if (iEventRV < 0)
                gbReduceByReload = 0;
            break;
        case MAP_OBJECT_HERO:
            if (gpGame->m_availableHeroes[pEventCell->m_objectMetadata] == pHero->m_owner) {
                gaiHeroLiveChance[pEventCell->m_objectMetadata] = 100;
                if (!immediate || gbTroopReload)
                    iEventRV = 0;
                else
                    iEventRV = -5000;
                *liveChance = 0;
            } else if (gbIAmGreatest
                       && !gbHumanPlayer[gpGame->m_availableHeroes[pEventCell->m_objectMetadata]]) {
                iEventRV = 0;
                *liveChance = 100;
            } else {
                iTownValue = 0;
                pEventTown = NULL;
                pEventTownArmy = NULL;
                pEventHero = gpGame->GetHero(pEventCell->m_objectMetadata);
                if (pEventHero->m_locationType == (MAP_TRIGGER_EVENT | MAP_OBJECT_TOWN)) {
                    pEventTown = gpGame->GetTown(pEventHero->m_occupiedTown);
                    pEventTownArmy = &pEventTown->m_army;
                    iTownValue = ValueOfTown(pEventTown);
                    iEventTownId = pEventTown->m_id;
                    if (pEventTown->m_owner >= 0)
                        iTownValue = static_cast<int>(
                            ((gbHumanPlayer[pEventTown->m_owner] ? gfAttackHumanBonus
                                                                 : gfAttackComputerBonus)
                                 * ((5 - gpGame->m_playerCount) * 0.25)
                             + 1.0)
                            * iTownValue
                        );
                }
                if (immediate && giDebugLevel == 5 && x == 15)
                    giDebugLevel = 9;
                ProbableOutcomeOfBattle(
                    &pHero->m_army,
                    pHero,
                    &pEventHero->m_army,
                    pEventHero,
                    pEventTownArmy,
                    pEventTownArmy != NULL,
                    iEventTownId,
                    pEventHero->m_owner,
                    fWinChance,
                    iAttackerLoss,
                    iDefenderLoss,
                    iAttackerRemaining,
                    iDefenderRemaining,
                    iEventRV
                );
                if (immediate && giDebugLevel == 9)
                    giDebugLevel = 5;
                *liveChance = static_cast<int>(fWinChance * 100.0f);
                if (iTownValue > 0)
                    iEventRV = static_cast<int>(iTownValue * fWinChance + iEventRV);
                if (immediate && gbHumanPlayer[pEventHero->m_owner] && iEventRV > 200)
                    iEventRV = static_cast<int>(iEventRV * 1.5);
                if (fWinChance > 0.75)
                    gaiHeroLiveChance[pEventCell->m_objectMetadata] = 100;
                else if (fWinChance > 0.5)
                    gaiHeroLiveChance[pEventCell->m_objectMetadata] =
                        static_cast<short>(fWinChance * 136.0f);
                else if (fWinChance > 0.4)
                    gaiHeroLiveChance[pEventCell->m_objectMetadata] =
                        static_cast<short>(fWinChance * 130.0f);
                else if (fWinChance > 0.3)
                    gaiHeroLiveChance[pEventCell->m_objectMetadata] =
                        static_cast<short>(fWinChance * 125.0f);
                else if (fWinChance > 0.2)
                    gaiHeroLiveChance[pEventCell->m_objectMetadata] =
                        static_cast<short>(fWinChance * 113.0f);
                else
                    gaiHeroLiveChance[pEventCell->m_objectMetadata] =
                        static_cast<short>(fWinChance * 100.0f);
                if (gaiHeroLiveChance[pEventCell->m_objectMetadata] > 100)
                    gaiHeroLiveChance[pEventCell->m_objectMetadata] = 100;
                if (!immediate && fWinChance < 0.4)
                    iEventRV = static_cast<int>(iEventRV * (3.0f - fWinChance * 2.0f));
                if (!immediate && fWinChance < 0.2)
                    iEventRV = static_cast<int>(iEventRV * (2.0f - fWinChance * 2.0f));
                if (iEventRV < 0)
                    gbReduceByReload = 0;
                gbReduceByBerserk = 0;
            }
            break;
        case MAP_OBJECT_TOWN:
            pEventTown = gpGame->GetTown(pEventCell->m_objectMetadata);
            if (gpGame->m_townOwners[pEventCell->m_objectMetadata] == pHero->m_owner) {
                if (pEventTown->m_occupyingHeroId != TOWN_OCCUPYING_HERO_NONE) {
                    iEventRV = 0;
                } else {
                    iEventRV = 0;
                    HeroInteractionAtTown(pHero, pEventTown, 1, &iEventRV);
                    iEventRV = static_cast<int>(iEventRV * gfHeroInteractionBonus[pHero->m_id]);
                }
                gbReduceByReload = 0;
            } else if (gbIAmGreatest && gpGame->m_townOwners[pEventCell->m_objectMetadata] >= 0
                       && !gbHumanPlayer[gpGame->m_townOwners[pEventCell->m_objectMetadata]]) {
                iEventRV = 0;
            } else {
                iTownValue = ValueOfTown(pEventTown);
                if (immediate && giDebugLevel == 5 && x == 15)
                    giDebugLevel = 9;
                if (gpGame->GetTown(pEventCell->m_objectMetadata)->m_occupyingHeroId
                    != TOWN_OCCUPYING_HERO_NONE)
                    ProbableOutcomeOfBattle(
                        &pHero->m_army,
                        pHero,
                        &gpGame->m_heroRecs[pEventTown->OccupyingHero()].m_army,
                        &gpGame->m_heroRecs[pEventTown->OccupyingHero()],
                        &pEventTown->m_army,
                        1,
                        pEventCell->m_objectMetadata,
                        pEventTown->m_owner,
                        fWinChance,
                        iAttackerLoss,
                        iDefenderLoss,
                        iAttackerRemaining,
                        iDefenderRemaining,
                        iOutcome
                    );
                else if (pEventTown->HasGarrison())
                    ProbableOutcomeOfBattle(
                        &pHero->m_army,
                        pHero,
                        &pEventTown->m_army,
                        NULL,
                        NULL,
                        1,
                        pEventCell->m_objectMetadata,
                        pEventTown->m_owner,
                        fWinChance,
                        iAttackerLoss,
                        iDefenderLoss,
                        iAttackerRemaining,
                        iDefenderRemaining,
                        iOutcome
                    );
                else {
                    fWinChance = 1.0f;
                    iOutcome = 0;
                }
                *liveChance = static_cast<int>(fWinChance * 100.0f);
                if (immediate && giDebugLevel == 9)
                    giDebugLevel = 5;
                if (pEventTown->m_owner >= 0)
                    iTownValue = static_cast<int>(
                        ((5 - gpGame->m_playerCount) * 0.25 + 0.9)
                        * (gbHumanPlayer[pEventTown->m_owner] ? gfAttackHumanBonus
                                                              : gfAttackComputerBonus)
                        * iTownValue
                    );
                iEventRV = static_cast<int>(iTownValue * fWinChance + iOutcome);
                if (gpGame->m_townOwners[pEventCell->m_objectMetadata] != GAME_PLAYER_NONE)
                    gbReduceByBerserk = 0;
            }
            break;
        case MAP_OBJECT_DAEMON_CAVE:
            if (pEventCell->m_objectMetadata == DAEMON_CAVE_EMPTY) {
                iEventRV = 0;
            } else {
                iEventRV = static_cast<int>(
                    pHero->m_aiFightValue * 300.0 + gafAITurnCostResource[RESOURCE_GOLD] * 2500.0f
                    + (gpCurPlayer->m_aiData.m_artifactValue + pHero->m_aiFightValue * 100.0)
                    + pHero->m_aiFightValue * 300.0 + gafAITurnCostResource[RESOURCE_GOLD] * -750.0
                );
                if (pEventCell->m_objectMetadata == DAEMON_REWARD_RANSOM
                    && gpCurPlayer->m_resources[RESOURCE_GOLD] < DAEMON_GOLD)
                    iEventRV = -100;
            }
            break;
        case MAP_OBJECT_OASIS:
            if (!(pHero->m_eventFlags & HERO_EVENT_OASIS))
                iEventRV = static_cast<int>(pHero->m_aiFightValue * 200.0f);
            break;
        case MAP_OBJECT_BUOY:
            if (!(pHero->m_eventFlags & HERO_EVENT_BUOY))
                iEventRV = static_cast<int>(pHero->m_aiFightValue * 200.0f);
            break;
        case MAP_OBJECT_STATUE:
            if (!(pHero->m_eventFlags & HERO_EVENT_STATUE))
                iEventRV = static_cast<int>(pHero->m_aiFightValue * 400.0f);
            break;
        case MAP_OBJECT_FAERIE_RING:
            if (!(pHero->m_eventFlags & HERO_EVENT_FAERIE_RING))
                iEventRV = static_cast<int>(pHero->m_aiFightValue * 200.0f);
            break;
        case MAP_OBJECT_FOUNTAIN:
            if (!(pHero->m_eventFlags & HERO_EVENT_FOUNTAIN))
                iEventRV = static_cast<int>(pHero->m_aiFightValue * 200.0f);
            break;
        case MAP_OBJECT_TREASURE_CHEST:
            iEventRV = static_cast<int>(gafAITurnCostResource[RESOURCE_GOLD] * 1500.0f);
            break;
        case MAP_OBJECT_CAMPFIRE:
            iEventRV = static_cast<int>(
                (gafAITurnCostResource[RESOURCE_GEMS] + gafAITurnCostResource[RESOURCE_MERCURY]
                 + gafAITurnCostResource[RESOURCE_ORE] + gafAITurnCostResource[RESOURCE_SULFUR]
                 + gafAITurnCostResource[RESOURCE_CRYSTAL] + gafAITurnCostResource[RESOURCE_WOOD])
                    / 6.0f * 5.0f
                + gafAITurnCostResource[RESOURCE_GOLD] * 500.0f
            );
            break;
        case MAP_OBJECT_SPELL_SHRINE:
            if (pHero->m_primaryStats[HERO_PRIMARY_KNOWLEDGE] > 0
                && pHero->HasArtifact(ARTIFACT_MAGIC_BOOK)) {
                iEventLoop = pHero->AddSpell(
                    pEventCell->m_objectMetadata - 1,
                    pHero->m_primaryStats[HERO_PRIMARY_KNOWLEDGE],
                    1
                );
                iEventRV = giSpellAIValue[pEventCell->m_objectMetadata - 1];
                iEventRV = static_cast<int>(
                    StatChangeValue(
                        pHero->m_primaryStats[HERO_PRIMARY_KNOWLEDGE] - iEventLoop,
                        pHero->m_primaryStats[HERO_PRIMARY_KNOWLEDGE]
                    )
                    * iEventRV
                );
                if (gcSpellAIFlags[pEventCell->m_objectMetadata - 1]
                    & SPELL_AI_FLAG_SCALES_WITH_POWER)
                    iEventRV = static_cast<int>(
                        iEventRV
                        * (pHero->m_primaryStats[HERO_PRIMARY_KNOWLEDGE] > 40
                               ? gfStatPower[40]
                               : gfStatPower[pHero->m_primaryStats[HERO_PRIMARY_KNOWLEDGE]])
                    );
            } else {
                iEventRV = 0;
            }
            break;
        case MAP_OBJECT_GAZEBO:
            if (pHero->m_visitedSites & (1 << pEventCell->m_objectMetadata))
                iEventRV = 0;
            else
                iEventRV = static_cast<int>(pHero->m_aiFightValue * 1000.0f);
            break;
        case MAP_OBJECT_LIGHTHOUSE:
            if (gpGame->m_mines[1].owner == pHero->m_owner)
                iEventRV = 0;
            else
                iEventRV = 1000;
            break;
        case MAP_OBJECT_RESOURCE:
            switch (pEventCell->m_objectIndex - RESOURCE_PILE_OBJECT_BASE) {
                case RESOURCE_GOLD:
                    iEventRV = static_cast<int>(
                        pEventCell->m_objectMetadata * RESOURCE_PILE_GOLD_MULTIPLIER
                        * gafAITurnCostResource[RESOURCE_GOLD]
                    );
                    break;
                case RESOURCE_WOOD:
                    iEventRV = static_cast<int>(
                        pEventCell->m_objectMetadata * gafAITurnCostResource[RESOURCE_WOOD]
                    );
                    break;
                case RESOURCE_ORE:
                    iEventRV = static_cast<int>(
                        pEventCell->m_objectMetadata * gafAITurnCostResource[RESOURCE_ORE]
                    );
                    break;
                case RESOURCE_CRYSTAL:
                    iEventRV = static_cast<int>(
                        pEventCell->m_objectMetadata * gafAITurnCostResource[RESOURCE_CRYSTAL]
                    );
                    break;
                case RESOURCE_SULFUR:
                    iEventRV = static_cast<int>(
                        pEventCell->m_objectMetadata * gafAITurnCostResource[RESOURCE_SULFUR]
                    );
                    break;
                case RESOURCE_MERCURY:
                    iEventRV = static_cast<int>(
                        pEventCell->m_objectMetadata * gafAITurnCostResource[RESOURCE_MERCURY]
                    );
                    break;
                case RESOURCE_GEMS:
                    iEventRV = static_cast<int>(
                        pEventCell->m_objectMetadata * gafAITurnCostResource[RESOURCE_GEMS]
                    );
                    break;
            }
            break;
        case MAP_OBJECT_WINDMILL:
            if (pEventCell->m_objectMetadata == WINDMILL_EMPTY) {
                iEventRV = 0;
            } else {
                memset(costList, 0, sizeof(costList));
                costList[pEventCell->m_objectMetadata] = WINDMILL_RESOURCE_AMOUNT;
                iEventRV = RVConversion(costList);
            }
            break;
        case MAP_OBJECT_SKELETON:
            if (pEventCell->m_objectMetadata == SKELETON_EMPTY)
                iEventRV = 0;
            else
                iEventRV = static_cast<int>(gpCurPlayer->m_aiData.m_artifactValue * 0.1);
            break;
        case MAP_OBJECT_ANCIENT_LAMP:
            EvaluateOneTimeCreaturePurchase(
                pHero,
                CREATURE_GENIE,
                pEventCell->m_objectMetadata,
                0,
                iPurchaseNum,
                iEventRV,
                iPurchaseSlot
            );
            gbReduceByReload = 0;
            break;
        case MAP_OBJECT_STRAW_HUT:
            EvaluateOneTimeCreaturePurchase(
                pHero,
                CREATURE_GOBLIN,
                pEventCell->m_objectMetadata,
                1,
                iPurchaseNum,
                iEventRV,
                iPurchaseSlot
            );
            break;
        case MAP_OBJECT_HOUSE:
        case MAP_OBJECT_PEASANT_LOG_CABIN:
            EvaluateOneTimeCreaturePurchase(
                pHero,
                CREATURE_PEASANT,
                pEventCell->m_objectMetadata,
                1,
                iPurchaseNum,
                iEventRV,
                iPurchaseSlot
            );
            gbReduceByReload = 0;
            break;
        case MAP_OBJECT_CABIN:
            EvaluateOneTimeCreaturePurchase(
                pHero,
                CREATURE_ARCHER,
                pEventCell->m_objectMetadata,
                1,
                iPurchaseNum,
                iEventRV,
                iPurchaseSlot
            );
            gbReduceByReload = 0;
            break;
        case MAP_OBJECT_DWARF_LOG_CABIN:
            EvaluateOneTimeCreaturePurchase(
                pHero,
                CREATURE_DWARF,
                pEventCell->m_objectMetadata,
                1,
                iPurchaseNum,
                iEventRV,
                iPurchaseSlot
            );
            gbReduceByReload = 0;
            break;
        case MAP_OBJECT_DESERT_TENT:
            EvaluateOneTimeCreaturePurchase(
                pHero,
                CREATURE_NOMAD,
                pEventCell->m_objectMetadata,
                0,
                iPurchaseNum,
                iEventRV,
                iPurchaseSlot
            );
            gbReduceByReload = 0;
            break;
        case MAP_OBJECT_WAGON_CAMP:
            EvaluateOneTimeCreaturePurchase(
                pHero,
                CREATURE_ROGUE,
                pEventCell->m_objectMetadata,
                0,
                iPurchaseNum,
                iEventRV,
                iPurchaseSlot
            );
            gbReduceByReload = 0;
            break;
        case MAP_OBJECT_GRAVEYARD:
        case MAP_OBJECT_SHIPWRECK:
            if (pEventCell->m_objectMetadata == GHOST_SITE_EMPTY) {
                iEventRV = 0;
            } else {
                switch (pEventCell->m_objectMetadata) {
                    case GHOST_SITE_SMALL:
                        guardCount1 = 2;
                        goldCost = GHOST_SMALL_GOLD;
                        break;
                    case GHOST_SITE_MEDIUM:
                        guardCount1 = 3;
                        goldCost = GHOST_MEDIUM_GOLD;
                        break;
                    case GHOST_SITE_LARGE:
                        guardCount1 = 5;
                        goldCost = GHOST_LARGE_GOLD;
                        break;
                    case GHOST_SITE_HUGE:
                        guardCount1 = 10;
                        goldCost = GHOST_HUGE_GOLD;
                        break;
                }
                for (iEventLoop = 0; iEventLoop < ARMY_GROUP_SLOT_COUNT; iEventLoop++) {
                    gpMonGroup->m_creatureTypes[iEventLoop] = CREATURE_GHOST;
                    gpMonGroup->m_creatureCounts[iEventLoop] = guardCount1;
                }
                ChooseEvaluateBattle(
                    &pHero->m_army,
                    pHero,
                    gpMonGroup,
                    NULL,
                    0,
                    0,
                    static_cast<int>(
                        goldCost * gafAITurnCostResource[RESOURCE_GOLD]
                        + (pEventCell->m_objectMetadata == GHOST_SITE_HUGE
                               ? gpCurPlayer->m_aiData.m_artifactValue
                               : 0)
                    ),
                    bWon9,
                    iEventRV
                );
            }
            break;
        case MAP_OBJECT_DRAGON_CITY:
            prize5 = static_cast<int>(
                gaiTurnValueOfMine[x][y] * gafAITurnCostResource[RESOURCE_GOLD] * 1000.0f * 1.5
            );
            for (iEventLoop = 0; iEventLoop < ARMY_GROUP_SLOT_COUNT; iEventLoop++) {
                gpMonGroup->m_creatureTypes[iEventLoop] = CREATURE_DRAGON;
                gpMonGroup->m_creatureCounts[iEventLoop] = 1;
            }
            if (gpGame->m_mineOwners[0] == pHero->m_owner)
                iEventRV = 0;
            else if (gpGame->m_mineOwners[0] != GAME_PLAYER_NONE)
                ChooseEvaluateBattle(
                    &pHero->m_army,
                    pHero,
                    gpMonGroup,
                    NULL,
                    0,
                    0,
                    static_cast<int>(
                        (gpGame->m_players[gpGame->m_mineOwners[0]].m_aiData.m_artifactPoolShare
                         + 1.0)
                        * prize5
                    ),
                    bBattleWon9,
                    iEventRV
                );
            else
                ChooseEvaluateBattle(
                    &pHero->m_army,
                    pHero,
                    gpMonGroup,
                    NULL,
                    0,
                    0,
                    prize5,
                    bBattleWon9,
                    iEventRV
                );
            break;
        case MAP_OBJECT_STONE_LITHS:
        case MAP_OBJECT_WHIRLPOOL:
            if (!bEvaluatingTravelGates) {
                iEventRV = 0;
                break;
            }
            bEvaluatingTravelGates = 0;
            bestRV1 = -9999;
            for (gateY28 = 0; gateY28 < MAP_CELL_GRID_SIZE; gateY28++) {
                for (gateX1 = 0; gateX1 < MAP_CELL_GRID_SIZE; gateX1++) {
                    exitCell = gpAdvManager->GetCell(gateX1, gateY28);
                    if (MANHATTAN_LENGTH(gateX1 - x, gateY28 - y)
                            > ((pEventCell->m_triggerType & MAP_TRIGGER_TYPE_MASK)
                                       == MAP_OBJECT_STONE_LITHS
                                   ? 1
                                   : 3)
                        && exitCell->m_triggerType == pEventCell->m_triggerType) {
                        exitRV5 =
                            StrategicValueOfPosition(pHero, gateX1, gateY28, 0, &exitLiveChance);
                        exitRV5 = static_cast<int>(exitRV5 * 0.85);
                        if (exitRV5 > bestRV1) {
                            bestRV1 = exitRV5;
                            chosenExitX0 = gateX1;
                            chosenExitY27 = gateY28;
                        }
                    }
                }
            }
            positionValue =
                StrategicValueOfPosition(pHero, pHero->m_x, pHero->m_y, 0, &exitLiveChance);
            if (positionValue + 200 < bestRV1)
                iEventRV = bestRV1 - positionValue - 200;
            else
                iEventRV = -200;
            bEvaluatingTravelGates = 1;
            gbReduceByReload = 0;
            break;
        case MAP_OBJECT_WATERWHEEL:
            iEventRV = static_cast<int>(
                pEventCell->m_objectMetadata * WATERWHEEL_GOLD_MULTIPLIER
                * gafAITurnCostResource[RESOURCE_GOLD]
            );
            break;
        case MAP_OBJECT_SHIP:
            gbActualBoatFound = 1;
            iEventRV = 100;
            break;
        case MAP_OBJECT_SIGNPOST:
        case MAP_OBJECT_RANKING_SHRINE:
            iEventRV = 0;
            break;
        case MAP_OBJECT_ROSEBUSH:
        case MAP_OBJECT_COAST:
        case MAP_OBJECT_TREE_STUMP:
        case MAP_OBJECT_OAK_TREE:
            iEventRV = 0;
            break;
        default:
            if (gpCurPlayer->m_ultimateArtifactHintChance > 15
                && gpCurPlayer->m_ultimateArtifactHintX == x
                && gpCurPlayer->m_ultimateArtifactHintY == y) {
                iEventRV =
                    (gpCurPlayer->m_ultimateArtifactHintChance - 15) * gUltArtifactAvgValue / 100;
            } else {
                iDefaultEventType = pEventCell->m_triggerType & MAP_TRIGGER_TYPE_MASK;
                if (iDefaultEventType >= MAP_OBJECT_NON_EVENT_FIRST
                    && iDefaultEventType <= MAP_OBJECT_TREES_LAST)
                    iEventRV = 0;
            }
            break;
    }
    if (gbTroopReload && gbReduceByReload)
        iEventRV = static_cast<int>(iEventRV * fReduceFactor);
    if (gbBerserk && gbReduceByBerserk)
        iEventRV = static_cast<int>(iEventRV * fBerserkFactor);
    if (!immediate) {
        if (iEventRV > 0 && (mapExtra[x][y] & MAP_EXTRA_MONSTER_ADJACENT)
            && (pEventCell->m_triggerType & MAP_TRIGGER_TYPE_MASK) != MAP_OBJECT_MONSTER)
            iEventRV = 0;
        if (iEventRV < 0 && (pEventCell->m_triggerType & MAP_TRIGGER_TYPE_MASK) != MAP_OBJECT_HERO)
            iEventRV = 0;
        else if (iEventRV > 32000)
            iEventRV = 32000;
        else if (iEventRV < -32000)
            iEventRV = -32000;
        gaiHeroEventStratRVOfPos[x][y] = iEventRV;
    }
    return iEventRV;
}

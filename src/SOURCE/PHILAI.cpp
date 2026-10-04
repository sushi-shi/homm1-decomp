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
DATA(0x0049f4d8)
float gAttackHumanBonus = 2.0f;
DATA(0x0049f4dc)
float gAttackComputerBonus = 0.8f;
DATA(0x004af300)
i16 gaiHeroEventStratRVOfPos[MAP_CELL_GRID_SIZE][MAP_CELL_GRID_SIZE];
DATA(0x004aca74)
float fBerserkFactor;
DATA(0x004c0b90)
i32 iLastFrameRateTimer;
DATA(0x004b4aa4)
i8 giCurPlayer;
DATA(0x004aca64)
float gWinChance;
DATA(0x004aa034)
i32 gEventLoop;
DATA(0x004ca164)
i8 giBuildShipyard[GAME_PLAYER_COUNT];
DATA(0x004c2048)
i32 giMaxHeroesForThisPlayer;
DATA(0x004b4ba0)
i8 giBuildBoat[GAME_PLAYER_COUNT];
DATA(0x004aa0cc)
float fReduceFactor;
DATA(0x004c8cd0)
u8 giCurPlayerBit;
DATA(0x004aa030)
i8 giBestShipyardDist;
DATA(0x004c8cc8)
i32 bHeroBuiltThisTurn;
DATA(0x004c0bb0)
i16 gaiHeroLiveChance[GAME_HERO_COUNT];
DATA(0x004b2fec)
i32 gAttackerLoss;
DATA(0x004b2ff0)
i32 gDefenderLoss;
DATA(0x004aa0d8)
i32 giHumanTownConquered;
DATA(0x004c0b9c)
i32 giCurTurn;
DATA(0x004b2fd0)
i32 costTemp[RESOURCE_COUNT];
DATA(0x004b1b90)
i8 gaiTurnValueOfMine[MAP_CELL_GRID_SIZE][MAP_CELL_GRID_SIZE];
DATA(0x004b221c)
i32 iDummy;
DATA(0x004b1b80)
i8 gbPossibleShipyardFound;
DATA(0x004bb13c)
float gafAITurnCostResource[RESOURCE_COUNT];
DATA(0x004b7430)
u8 gCurWatchPlayerHighBit;
DATA(0x004c2044)
i32 iCurPlaceToVisit;
DATA(0x004aa0e0)
i8 giBestShipyardId;
DATA(0x004c0bf8)
i8 mapVisited[MAP_CELL_GRID_SIZE][MAP_CELL_GRID_SIZE];
DATA(0x004aa0e8)
i16 gaiHeroStrategicRVOfPos[MAP_CELL_GRID_SIZE][MAP_CELL_GRID_SIZE];
DATA(0x004c205c)
i8 gbActualBoatFound;
DATA(0x004b9cbc)
u8 giCurWatchPlayerBit;
DATA(0x004b4ba8)
playerData* gpCurPlayer;
DATA(0x004aa038)
float gfHeroInteractionBonus[GAME_HERO_COUNT];
DATA(0x004c203c)
i32 gbBerserk;
DATA(0x004c2054)
u8 giCurPlayerHighBit;
DATA(0x004aca78)
i16 gaiLiveChanceOfPos[MAP_CELL_GRID_SIZE][MAP_CELL_GRID_SIZE];
DATA(0x004b9cb8)
i8 giBuildBoatStuffTurn[GAME_PLAYER_COUNT];
DATA(0x004ac970)
i32 iPlacesVisited[ADVMGR_PLACE_VISIT_COUNT][ADVMGR_PLACE_COORDINATE_COUNT];
DATA(0x004c0b98)
i32 gbTroopReload;
DATA(0x004c204c)
i8 gbActualShipyardFound;

// Buka 2.1's named AI factors. They are loaded, not folded, at /Od, and
// retail .rdata keeps them in this declaration order at 0x0048d070 ahead of
// the anonymous float literals.
DATA(0x0048d070)
static const float AI_TARGET_HUMAN_VALUE_FACTOR = 1.5f;
DATA(0x0048d074)
static const float AI_STRATEGIC_POSITION_SCORE_FACTOR = 1.25f;
DATA(0x0048d078)
static const float AI_CREATURE_SAME_RACE_FACTOR = 1.1f;
DATA(0x0048a4b8)
static const float AI_FUTURE_DEFLATION_RATE = 0.15f;
DATA(0x0048d080)
static const float AI_HERO_PURCHASE_SAME_RACE_FACTOR = 1.12f;
DATA(0x0048d084)
static const float AI_ATTENTION_IDENTITY_FLOAT = 1.0f;
DATA(0x0048d088)
static const float AI_ATTENTION_IDENTITY = 1.0f;

// Misc's logging helpers open this object in retail (0x00427da5..0x00419f15),
// directly after the SVSearchArray initializer wrapper _$E2 at 0x00427d90.
// HoMM1's retained logging path opens KB.LOG afresh and writes its banner.
VA(0x00427da5, 0x70)
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
VA(0x00427e15, 0x9d)
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
VA(0x00427eb2, 0x86)
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
VA(0x00427f38, 0x8a)
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
VA(0x00427fc2, 0xa2)
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
VA(0x00428064, 0xaa)
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
VA(0x0042810e, 0x1f)
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
VA(0x0042817b, 0x177)
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
            || MANHATTAN_LENGTH(y - gpGame->m_heroRecs[i].m_x, x - gpGame->m_heroRecs[i].m_x) < 10)
            gaiHeroLiveChance[i] = RV_UNSET;
    }
}

// donor PoL RVA 0x000379d0; preferred Buka symbol ?CheckDoMain@@YIXHH@Z
// donor Buka TU SOURCE/PHILAI; HoMM1 owner inferred from contiguous order
// evidence: graph:5;base=0.463287;margin=0.627999;shape=0.348;size=0.713;calls=0.909;alternate=pol20:void CheckDoMain(int, int)@0x000379d0
VA(0x004282f2, 0x1ef)
void CheckDoMain(i32, i32 doMain) {
    if (KBTickCount() > iLastFrameRateTimer + 15
        || KBTickCount() > glTimers[ADVENTURE_FRAME_TIMER_SLOT]) {
        Process1WindowsMessage();
        PollSound();
        if (KBTickCount() > glTimers[ADVENTURE_FRAME_TIMER_SLOT]) {
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
        } else if (gpMouseManager->m_mouseX - gpMouseManager->m_hotspotX != gpMouseManager->m_drawnX
                   || gpMouseManager->m_mouseY - gpMouseManager->m_hotspotY
                          != gpMouseManager->m_drawnY) {
            gpMouseManager->MovePointer(gpMouseManager->m_mouseX, gpMouseManager->m_mouseY);
        }
        iLastFrameRateTimer = KBTickCount();
    }
}

// Both donors retain this intentionally empty status hook; retail has no side effects.
VA(0x004284e1, 0x10)
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
VA(0x0042868b, 0x445)
void philAI::CheckBuyStuff(void) {
    i32 done = 0;
    i32 boughtSomething = 0;
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
VA(0x00428ad0, 0x1a9)
i32 philAI::GoodAdjacent(hero* pHero, i32* direction) {
    i32 bestDirection;
    i32 dirIndex;
    i32 x;
    i32 y;
    i32 value;
    i32 maxValue;
    i32 iChance;

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
VA(0x00428c79, 0x3e5)
void philAI::CheckReload(hero* pHero) {
    i32 mapY;
    mapCell* visitedCell;
    i32 heroFightValue;
    i32 mapX;
    i32 enemy;
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
VA(0x0042905e, 0x280)
void philAI::CheckBerserk(hero* pHero) {
    i32 enemy;
    i32 x;
    mapCell* cell;
    i32 y;
    hero* enemyHero;
    i32 best = -1;
    i32 heroFightValue;

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
VA(0x004292de, 0x1a0)
i8 philAI::DoDimensionDoor(hero* pHero) {
    i32 x;
    i32 i;
    i32 y;
    i32 length;
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
VA(0x0042947e, 0x8f0)
void philAI::DoAI(i32 player) {
    i32 pathIndex;
    i32 moveDone;
    i32 bestDirection;
    i32 stepMax;
    i32 steps;
    mapCell* eventCell;
    i32 dummy;
    i8 stopAfterStep;
    i32 oldShowIt;
    i8 halfShown;
    i32 x;
    i32 y;
    i16 minRV;
    i8 moveInterrupt;
    hero* aiHero;
    i32 flag;
    i32 moveResult;
    i32 tempArray[4];

    halfShown = 0;
    LogInt("DO AI 1", player);
    if (gGameOver)
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
    SuspendSamples();
    SuspendMusic();
    while ((aiHero = DetermineHeroToMove(player)) != NULL) {
        giHumanTownConquered = GAME_TOWN_NONE;
        iCurPlaceToVisit = 0;
        if (gGameOver) {
            ResumeSamples();
            ResumeMusic();
            return;
        }
        LogStr("\n\n\n\n");
        LogStr("===================================");
        LogInt("Player with HeroTOMOVE", player);
        LogStr(aiHero->m_name);
        LogStr("\n");
        CheckReload(aiHero);
        CheckBerserk(aiHero);
        gShowComputerRoute = 0;
        if (gConfig.blackoutComputer == 0 && gRemoteOn == 0
            && (gpGame->m_mapExtra[aiHero->m_x][aiHero->m_y] & gCurWatchPlayerHighBit)) {
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
        stepMax = static_cast<i32>(stepMax * (1.7 - gpCurPlayer->m_difficulty * 0.1));
        minRV = static_cast<i16>(
            minRV * ((gpCurPlayer->m_difficulty - PLAYER_TYPE_DUMB) * 0.06 + 0.8)
        );
        while (!moveDone && aiHero->m_remainingMobility >= 4) {
            if (gGameOver) {
                ResumeSamples();
                ResumeMusic();
                return;
            }
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
            gShowComputerRoute = 1;
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
VA(0x00429db9, 0x654)
void philAI::GetTurnAIVars(i32 player) {
    i32 totalFightValue;
    playerData* pPlayer;
    i32 yPos;
    i32 mineValue;
    i32 xPos;
    i32 i;
    float fFightVal;
    i32 y;
    i32 otherIndex;
    hero* heroPointer;
    i32 unusedFightValue;
    i32 artTotal;
    i32 hIndex;
    i32 x;
    town* townPointer;

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
    fFightVal = 0.0f;
    totalFightValue = 0;
    for (i = 0; i < gpCurPlayer->m_heroCount; i++) {
        heroPointer = gpGame->GetHero(gpCurPlayer->m_heroIds[i]);
        fFightVal =
            static_cast<float>(FightValueOfStack(&heroPointer->m_army, heroPointer, 0, 0, 0));
        totalFightValue = static_cast<i32>(totalFightValue + fFightVal);
        heroPointer->m_aiFightValue = fFightVal * 4e-05 + 0.4;
    }
    for (i = 0; i < gpCurPlayer->m_townCount; i++) {
        townPointer = gpGame->GetTown(gpCurPlayer->m_townIds[i]);
        fFightVal = static_cast<float>(FightValueOfStack(&townPointer->m_army, NULL, 0, 0, 0));
        totalFightValue = static_cast<i32>(totalFightValue + fFightVal);
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
VA(0x0042a40d, 0x5d0)
void philAI::GetBestBHC(i32, BHC& best) {
    float fValue = 1.0f;
    float bestBHCValue = -99.0f;
    i32 total = 0;
    i32 totalWeights = 0;
    i32 ideal[GAME_TOWN_COUNT];
    i32 strengths[GAME_TOWN_COUNT];
    BHC choice;
    i32 townNo;
    town* townPointer;
    i32 meanStrength;

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
        extern i32 gRemoteReady;
        if (giCurTurn > 3 && (!gRemoteOn || gRemoteReady) && townPointer->m_turnsOwned < 3)
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
    LogStr("BestBHC ", best.type, static_cast<i32>(bestBHCValue * 100.0f), best.what, 0, 0);
    if (bestBHCValue < 0.02)
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
VA(0x0042aaf9, 0x932)
void philAI::DetermineTargetPosition(hero* pHero, i8& targetX, i8& targetY, i16 mobility) {
    i32 bestRV;
    i32 cellValue;
    i32 spacing;
    i32 heroTerrainType;
    town* portTown;
    i32 rowCnt;
    i32 valid;
    i32 colPhase;
    i32 heroIndex;
    i16 x;
    i16 bestY;
    mapCell* thisCell;
    i16 bestX;
    i16 y;

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
        mobility = static_cast<i16>(mobility * 1.25);
    }
    if (heroTerrainType == TERRAIN_DESERT) {
        spacing -= 2;
        mobility = static_cast<i16>(mobility * 1.5);
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
                    valid = (thisCell->m_triggerType & MAP_TRIGGER_EVENT)
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
                    cellValue = static_cast<i32>(cellValue * AI_TARGET_HUMAN_VALUE_FACTOR);
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
VA(0x0042b42b, 0x63a)
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
    i32 artSlot;
    float defendingArmy;
    i32 artsD;
    i32 notUsed;
    i32 exp;
    float attackStrength;
    float defP;
    float defStr;
    float rawFight[2];
    float attackerPower;
    float power;
    float factor;
    i32 attArts;

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
    attackerLoss = static_cast<i32>((1.0 - winChance) * rawFight[0]);
    defenderLoss = static_cast<i32>(rawFight[1] * winChance);
    attackerRemaining =
        static_cast<i32>(attackerLoss * winChance + (1.0f - winChance) * rawFight[0]);
    defenderRemaining =
        static_cast<i32>(defenderLoss * (1.0f - winChance) + rawFight[1] * winChance);
    factor = 1.33 - gpCurPlayer->m_aiData.m_attentionWeights.upgradeBase;
    outcomeValue = static_cast<i32>(-attackerRemaining * factor * factor);
    if (enemyPlayer >= 0) {
        factor = gpCurPlayer->m_aiData.m_attentionWeights.upgradeBase + 0.66;
        if (gbHumanPlayer[enemyPlayer])
            outcomeValue = static_cast<i32>(
                outcomeValue + (defenderRemaining * factor * factor) * gAttackHumanBonus
            );
        else
            outcomeValue = static_cast<i32>(
                outcomeValue + defenderRemaining * gAttackComputerBonus * factor * factor
            );
    }
    outcomeValue = static_cast<i32>(outcomeValue * gpCurPlayer->m_aiData.m_upgradeValueWeight);
    if (attackerHero) {
        for (artSlot = 0; artSlot < HERO_ARTIFACT_SLOT_COUNT; artSlot++) {
            if (attackerHero->m_artifacts[artSlot] >= 0
                && attackerHero->m_artifacts[artSlot] < ARTIFACT_REGULAR_END)
                attArts += gArtifactBaseRV[attackerHero->m_artifacts[artSlot]];
        }
        outcomeValue = static_cast<i32>(outcomeValue - (attArts + 1400) * (1.0f - winChance));
        exp = gpGame->ExperienceValueOfStack(defender, defenderHero);
        outcomeValue =
            static_cast<i32>(outcomeValue + exp * attackerHero->m_aiFightValue * winChance * 0.8);
    }
    if (defenderHero) {
        for (artSlot = 0; artSlot < HERO_ARTIFACT_SLOT_COUNT; artSlot++) {
            if (defenderHero->m_artifacts[artSlot] >= 0
                && defenderHero->m_artifacts[artSlot] < ARTIFACT_REGULAR_END)
                artsD += gArtifactBaseRV[defenderHero->m_artifacts[artSlot]];
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
VA(0x0042ba65, 0x1e)
float philAI::GetOddsOfWinning(i32) {
    return 1.0f;
}

// Buka 2.1 ValueOfBuyingBuilding without the HoMM2 special buildings: the
// base value, scaled per slot by attention weights and dwelling counts, the
// enemy threat and the purchase deflator.
VA(0x0042ba83, 0x572)
void philAI::ValueOfBuyingBuilding(
    town* townPointer,
    i32 building,
    i32& resourceValue,
    float& benefitCost
) {
    i32 buildingCost[RESOURCE_COUNT];
    i32 attackWeek;
    i32 dwellingsOwned;
    i32 projectedAttackValue;
    i32 highestDwellingId;
    i32 dwellingIndex;
    float dangerRating;
    i32 creatureLocated;
    i32 i;
    i32 currentAttackTurns;
    i32 numFilledSlots;
    float totalEnemyStrength;
    i32 currentCreatureType;
    float fAttackOdds;
    i16 factionId;
    float curBenefit;
    i32 buildingLevel;

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
        static_cast<i8>(building == BUILDING_SLOT_MAGE_GUILD ? townPointer->m_buildState : 0)
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
        static_cast<i8>(building == BUILDING_SLOT_MAGE_GUILD ? townPointer->m_buildState : 0)
    );
    curBenefit = FutureDeflator(buildingCost) * curBenefit;
    resourceValue = static_cast<i32>(curBenefit);
    benefitCost = curBenefit / RVConversion(buildingCost);
}

// donor PoL RVA 0x0003d6b7; preferred Buka symbol ?GetBestBuilding@philAI@@QAEXPAVtown@@AAUBHC@@AAM@Z
// donor Buka TU SOURCE/PHILAI; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.526596;margin=0.075083;shape=0.323;size=0.949;calls=1.000;alternate=pol20:void philAI::GetBestBuilding(class town *, struct BHC &, float &)@0x0003d6b7
VA(0x0042bff5, 0x16f)
void philAI::GetBestBuilding(town* townPointer, BHC& purchase, float& benefitCost) {
    float buildingValue;
    i32 bestBuilding;
    float bestCost;
    i32 curBuilding;
    i32 costRV;
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
VA(0x0042c164, 0x2e1)
void philAI::ValueOfBuyingCreature(
    town* townPointer,
    i32 creature,
    i32& resourceValue,
    i32 purchaseCount,
    float& benefitCost
) {
    i32 nWeeks;
    i32 nPoints;
    float peril;
    i32 monsterCost[RESOURCE_COUNT];
    // Counts breath-attack stacks; retail allocation follows this local name
    // (renaming it moves registers).
    i32 archers;
    i32 creatRV;
    i32 rvCost;
    float attackChance;
    float foeStrength;
    float factor;
    i32 nTurns;
    i32 n;
    hero* occupant;
    i32 slotNum;

    archers = 0;
    GetMonsterCost(creature, monsterCost);
    rvCost = purchaseCount * RVConversion(monsterCost);
    creatRV = static_cast<i32>(
        purchaseCount * gMonsterDatabase[creature].fightValue
        * gpCurPlayer->m_aiData.m_upgradeValueWeight
    );
    if (townPointer->m_occupyingHeroId != TOWN_OCCUPYING_HERO_NONE) {
        occupant = gpGame->GetHero(townPointer->m_occupyingHeroId);
        creatRV = static_cast<i32>(creatRV * 1.1);
        if (occupant->m_heroClass == creature / CREATURE_FACTION_SIZE)
            creatRV = static_cast<i32>(creatRV * AI_CREATURE_SAME_RACE_FACTOR);
        if ((gMonsterDatabase[creature].stats.attributes & MONSTER_FLAGS_BREATH_ATTACK)) {
            for (n = 0; n < ARMY_GROUP_SLOT_COUNT; n++) {
                if (occupant->m_army.m_creatureTypes[n] != CREATURE_NONE
                    && (gMonsterDatabase[occupant->m_army.m_creatureTypes[n]].stats.attributes
                        & MONSTER_FLAGS_BREATH_ATTACK))
                    archers++;
            }
            creatRV = static_cast<i32>(creatRV * (1.18 - archers * 0.06));
        }
        creatRV = static_cast<i32>(
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
        creatRV = static_cast<i32>(creatRV * (1.18 - archers * 0.06));
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
    creatRV = static_cast<i32>(creatRV * (factor * factor * factor));
    creatRV = static_cast<i32>(creatRV * FutureDeflator(monsterCost));
    resourceValue = creatRV;
    benefitCost = static_cast<float>(resourceValue) / static_cast<float>(rvCost);
}

// donor PoL RVA 0x0003db58; preferred Buka symbol ?GetBestCreature@philAI@@QAEXPAVtown@@AAUBHC@@AAM@Z
// donor Buka TU SOURCE/PHILAI; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.467416;margin=0.566378;shape=0.359;size=0.696;calls=1.000;alternate=pol20:void philAI::GetBestCreature(class town *, struct BHC &, float &)@0x0003db58
VA(0x0042c445, 0x1f5)
void philAI::GetBestCreature(town* townPointer, BHC& best, float& bestValue) {
    float bestCost;
    float rand;
    float worth;
    float bestRandScore;
    i32 bestBuy;
    i32 topDwelling;
    i32 curDwelling;
    i32 mon;
    i32 canAdd;
    i32 iArmy;
    i32 costRV;
    i32 numUnits;

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
VA(0x0042c6e1, 0x9b)
i32 philAI::MaxBuyableCreatures(i32 creatureType) {
    i32 monsterCost[RESOURCE_COUNT];
    i32 maxUnits;
    i32 i;

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
VA(0x0042c77c, 0x1a8)
void philAI::ValueOfBuyingHero(
    town* townPointer,
    hero* heroPointer,
    i32& resourceValue,
    float& benefitCost
) {
    i32 tmp;
    i32 i;
    i32 heroRV;
    i32 heroCost[RESOURCE_COUNT];
    i32 costRV;

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
    heroRV = static_cast<i32>(
        heroRV
        * (gpCurPlayer->m_aiData.m_attentionWeights.heroValue + 1.0
           - gpCurPlayer->m_aiData.m_attentionWeights.upgradeBase)
    );
    if (gTownHeroClass[townPointer->m_type] == heroPointer->m_heroClass)
        heroRV = static_cast<i32>(heroRV * AI_HERO_PURCHASE_SAME_RACE_FACTOR);
    heroRV += StrategicValueOfPosition(heroPointer, heroPointer->m_x, heroPointer->m_y, 0, &tmp);
    heroRV -= 200;
    heroRV = static_cast<i32>(heroRV * FutureDeflator(heroCost));
    benefitCost = static_cast<float>(heroRV) / costRV;
    resourceValue = heroRV;
}

// ValueOfEventAtPosition module state (.bss order follows names, not position).
DATA(0x004b2ff4)
i32 gAttackerRemaining;
DATA(0x004b2ff8)
i32 gDefenderRemaining;
DATA(0x004b2ffc)
i32 gOutcome;
DATA(0x004b3000)
i32 gArtifactChoice1;

// donor PoL RVA 0x0003e2a8; preferred Buka symbol ?GetBestHero@philAI@@QAEXPAVtown@@AAUBHC@@AAM@Z
// donor Buka TU SOURCE/PHILAI; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.649528;margin=0.194728;shape=0.311;size=0.925;calls=0.800;strings=Town:%2d  Hero    : % 15i   Raw BC = %8.2f,  RandBC = %8.2f.;alternate=pol20:void philAI::GetBestHero(class town *, struct BHC &, float &)@0x0003e2a8
VA(0x0042c924, 0x184)
void philAI::GetBestHero(town* townPointer, BHC& best, float& bestValue) {
    i32 bestHero;
    float worth;
    i32 curHero;
    hero* availHero;
    float adjusted;
    float bestScore;
    float bestCost;
    i32 cost;

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
VA(0x0042cb0d, 0x1a)
i32 philAI::MeanRVOfUnexploredTerritory(i32) {
    return 0;
}

// Buka 2.1 GetGameAttentionValue: randomized game weights tempered by the
// number of players.
VA(0x0042cb27, 0x147)
void philAI::GetGameAttentionValue(i32 player) {
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
VA(0x0042cc6e, 0xed)
void philAI::GetTurnAttentionValue(i32 player) {
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
VA(0x0042cd5b, 0xa6)
i32 philAI::RVConversion(i32* const resources) {
    return static_cast<i32>(
        ((((((static_cast<float>(resources[static_cast<i32>(RESOURCE_ORE)])
              * gafAITurnCostResource[static_cast<i32>(RESOURCE_ORE)])
             + static_cast<float>(resources[static_cast<i32>(RESOURCE_GEMS)])
                   * gafAITurnCostResource[static_cast<i32>(RESOURCE_GEMS)])
            + static_cast<float>(resources[static_cast<i32>(RESOURCE_MERCURY)])
                  * gafAITurnCostResource[static_cast<i32>(RESOURCE_MERCURY)])
           + static_cast<float>(resources[static_cast<i32>(RESOURCE_GOLD)])
                 * gafAITurnCostResource[static_cast<i32>(RESOURCE_GOLD)])
          + static_cast<float>(resources[static_cast<i32>(RESOURCE_WOOD)])
                * gafAITurnCostResource[static_cast<i32>(RESOURCE_WOOD)])
         + static_cast<float>(resources[static_cast<i32>(RESOURCE_SULFUR)])
               * gafAITurnCostResource[static_cast<i32>(RESOURCE_SULFUR)])
        + static_cast<float>(resources[static_cast<i32>(RESOURCE_CRYSTAL)])
              * gafAITurnCostResource[static_cast<i32>(RESOURCE_CRYSTAL)]
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
VA(0x0042cecb, 0x522)
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
    i32 estLiveChance;
    i32 delta;
    i32 curLocType;
    i32 curEventVal;
    float estTurns;
    i32 heroLiveChance;
    i32 theTargetLiveChance;
    i32 curEventChance;
    i32 adjacentEventChance;
    i32 strategicEventValue;
    i32 curStrategicValue;
    i32 adjacentX;
    i32 curTriggerType;
    i32 adjacentY;

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
    totalValue = static_cast<i32>(totalValue / (estTurns + 0.2));
    delta = static_cast<i32>(delta * 2 / (estTurns + 1.0f));
    if (estLiveChance == 100)
        totalValue += delta;
    if ((pHero->m_eventFlags & HERO_EVENT_EMBARKED) && curTriggerType == MAP_OBJECT_COAST)
        totalValue += 40;
    return totalValue;
}

// Buka SVSearchArray: StrategicValueOfPosition's shared search, constructed
// by its dynamic initializer between RVOfPosition and its first user.
DATA(0x004b3038)
searchArray SVSearchArray;
RVA_DYNINIT(0x0002d3ed, 0x1a, SVSearchArray)
// Its .CRT$XCU thunk (0x0048e008 -> 0x00427d90) opens this retail object:
// int3 padding precedes it and LogTruncate follows without a gap.
RVA_DYNINIT(0x00027d90, 0x15, SVSearchArray)

// donor PoL RVA 0x0003ef45; preferred Buka symbol ?StrategicValueOfPosition@philAI@@QAEHHHHHPAHH@Z
// donor Buka TU SOURCE/PHILAI; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.499321;margin=0.324582;shape=0.341;size=0.829;calls=0.957;alternate=pol20:int philAI::StrategicValueOfPosition(int, int, int, int, int *, int)@0x0003ef45
VA(0x0042d407, 0x8a9)
i32 philAI::StrategicValueOfPosition(
    hero* pHero,
    i16 targetX,
    i16 targetY,
    i8 immediate,
    i32* liveChance
) {
    DATA(0x0049f734)
    static i8 gSVSearchArrayInUse = 0;
    i32 nGap;
    searchArray* pSearch;
    i32 inBoat;
    i32 extra2;
    mapCell* cell;
    i32 thisY;
    i32 newSeedRange;
    i32 iReach;
    i32 x;
    i32 heroIndex;
    i32 danger;
    i32 baseTerrain;
    i32 myValue;
    searchArray* madeSearch;
    i32 destTerrain;

    if (!immediate && gaiHeroStrategicRVOfPos[targetX][targetY] != RV_UNSET) {
        *liveChance = gaiLiveChanceOfPos[targetX][targetY];
        return gaiHeroStrategicRVOfPos[targetX][targetY];
    }
    myValue = 0;
    madeSearch = NULL;
    *liveChance = 100;
    if (gSVSearchArrayInUse) {
        madeSearch = new searchArray;
        if (!madeSearch)
            MemError();
        pSearch = madeSearch;
    } else {
        gSVSearchArrayInUse = 1;
        pSearch = &SVSearchArray;
    }
    inBoat = pHero->m_eventFlags & HERO_EVENT_EMBARKED;
    if (inBoat && gpAdvManager->GetCell(targetX, targetY)->m_triggerType == MAP_OBJECT_COAST)
        inBoat = 0;
    if (immediate) {
        newSeedRange = 60;
    } else {
        newSeedRange = 36;
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
                                danger = static_cast<i32>(
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
DATA(0x004b3004)
i32 gArtifactChoice2;

// Buka 2.1 ValueOfTown without the later scenario-town bonuses: built
// structures' base values plus a fixed gold-turn allowance.
VA(0x0042dcb0, 0xb1)
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
    sum = static_cast<i32>(sum + gafAITurnCostResource[RESOURCE_GOLD] * 1250.0f * 1.5);
    sum += 750;
    return sum;
}

// Buka 2.1 TurnCostResource: each resource's turn cost scales its base
// value against the player's relative stock-plus-income share.
VA(0x0042dd61, 0x139)
void philAI::TurnCostResource(i32 player) {
    playerAIData* playerAI;
    float ratio[RESOURCE_COUNT];
    float avg;
    i32 i;
    i32 totalRV;
    i32 value[RESOURCE_COUNT];
    playerAI = &gpGame->m_players[player].m_aiData;
    totalRV = 0;
    for (i = 0; i < RESOURCE_COUNT; i++) {
        value[i] = static_cast<i32>(
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
VA(0x0042de9a, 0x11e)
float philAI::TurnValueOfObelisk(i32 player) {
    playerAIData* playerAI;
    i32 each;
    playerAI = &gpGame->m_players[player].m_aiData;
    each = gArtifactBaseRV[gpGame->m_ultimateArtifactId] / 110;
    if (gpGame->m_ultimateArtifactId == ARTIFACT_NONE)
        return 0.0f;
    playerAI->m_obeliskValue = each * 48 / gpGame->m_obeliskCount;
    playerAI->m_obeliskValue = static_cast<i32>(
        playerAI->m_obeliskValue
        * (1.5 - abs(32 - gpGame->m_players[player].CountVisitedObelisks()) / 48.0f)
    );
    playerAI->m_obeliskValue = static_cast<i32>(
        playerAI->m_obeliskValue * (playerAI->m_attentionWeights.heroValue + 0.66)
    );
    return playerAI->m_obeliskValue;
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
VA(0x0042e009, 0x764)
i32 philAI::FightValueOfStack(
    armyGroup* group,
    hero* heroPointer,
    i32 useHero,
    i8 useTown,
    i8 townId
) {
    i32 worth;
    float fPowerMod;
    i32 spellScore;
    i32 bestValue;
    i32 slot;
    i32 armyWorth;
    i32 nArrows;
    i32 luck;
    i32 castleValue;
    town* pTown;
    i32 stats;
    i32 morale;
    i32 magicTotal;
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
                worth = static_cast<i32>(worth * (quantityMod + 1.0f));
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
        armyWorth = static_cast<i32>(armyWorth * gStatPower[stats]);
        castleValue = static_cast<i32>(castleValue * gStatPower[stats]);
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
                && (gSpellAIFlags[heroPointer->m_spells[slot]] & SPELL_AI_FLAG_COMBAT)) {
                spellScore = static_cast<i32>(
                    gSpellAIValue[heroPointer->m_spells[slot]]
                    * ((gSpellAIFlags[heroPointer->m_spells[slot]]
                        & SPELL_AI_FLAG_SCALES_WITH_POWER)
                           ? (heroPointer->m_primaryStats[HERO_PRIMARY_SPELL_POWER] > 40
                                  ? gBattleStat[40]
                                  : gBattleStat[heroPointer
                                                    ->m_primaryStats[HERO_PRIMARY_SPELL_POWER]])
                           : fPowerMod)
                );
                magicTotal +=
                    spellScore
                    * gSpellCastNumMod
                        [heroPointer->m_spellCharges[slot] < 20 ? heroPointer->m_spellCharges[slot]
                                                                : 20];
                if (bestValue < spellScore)
                    bestValue = spellScore;
            }
        }
        if (magicTotal > bestValue * 3.5)
            magicTotal = static_cast<i32>(bestValue * 3.5);
        if (magicTotal > armyWorth * 2)
            magicTotal = static_cast<i32>(armyWorth * 1.25);
        else if (magicTotal > armyWorth * 1.5)
            magicTotal = armyWorth;
        else if (magicTotal > armyWorth)
            magicTotal = static_cast<i32>(armyWorth * 0.75);
    }
    if (castleValue > armyWorth * 2)
        castleValue = static_cast<i32>(armyWorth * 1.5);
    else if (castleValue > armyWorth * 1.5)
        castleValue = static_cast<i32>(armyWorth * 1.25);
    else if (castleValue > armyWorth)
        castleValue = static_cast<i32>(armyWorth * 0.9);
    armyWorth += magicTotal;
    armyWorth += castleValue;
    return armyWorth;
}

// donor PoL RVA 0x00040aca; preferred Buka symbol ?EvaluateOneTimeCreaturePurchase@philAI@@QAEXHHHAAH00@Z
// donor Buka TU SOURCE/PHILAI; HoMM1 owner inferred from contiguous order
// evidence: graph:3;base=0.513526;margin=0.703256;shape=0.342;size=0.854;calls=1.000;alternate=pol20:void philAI::EvaluateOneTimeCreaturePurchase(int, int, int, int &, int &, int &)@0x00040aca
VA(0x0042e76d, 0x1da)
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
    i32 purchaseFightValue;
    i32 i;

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
    purchaseValue = static_cast<i32>(
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
VA(0x0042e947, 0x35f)
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
    i32 aDead;
    float fracLost;
    i32 win;
    float rnd;
    i32 unused;
    i32 dLeft;
    i32 dDead;
    armyGroup* winner;
    float diff;
    float winChance;
    i32 atkExp;
    i32 tmp;
    i32 defenderExp;
    float wChance;
    i32 res;
    i32 aLeft;

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
VA(0x0042eca6, 0xb86)
void philAI::HeroInteractionAtTown(
    hero* heroPointer,
    town* townPointer,
    i32 doInteraction,
    i32* value
) {
    i32 transferRating;
    i32 pick;
    i32 speedLimit;
    i32 stackFV;
    i32 nRunning;
    float fTownShare;
    i32 curBest;
    i32 garrisonFV;
    i32 heroStrength;
    i32 statSum;
    i32 armyCount;
    i32 moveNum;
    i32 j;
    float estWeight;
    i32 estTransferValue;
    i32 toHero;
    float curveTerm;
    armyGroup* fromArmy;
    i32 room;
    float fShareDiff;
    i32 newLearned;
    float myTargetShare;
    armyGroup* toArmy;
    i32 i;

    *value = 0;
    if (doInteraction) {
        if ((townPointer->m_buildings & (1 << BUILDING_SLOT_SHIPYARD))
            && townPointer->m_id != giBestShipyardId) {
            i = MANHATTAN_LENGTH(
                townPointer->m_x - heroPointer->m_x,
                townPointer->m_y - heroPointer->m_y
            );
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
            i = MANHATTAN_LENGTH(
                townPointer->m_x - heroPointer->m_x,
                townPointer->m_y - heroPointer->m_y
            );
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
                      * gSpellAIValue[townPointer->m_mageGuildSpells[i]]
                      * ((gSpellAIFlags[townPointer->m_mageGuildSpells[i]]
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
        transferRating = static_cast<i32>(
            ((curveTerm * curveTerm - 1.0f) * (heroStrength + garrisonFV))
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
    estTransferValue = static_cast<i32>((heroStrength + garrisonFV) * fShareDiff);
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
            moveNum = static_cast<i32>(
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
VA(0x0042f88d, 0xc7)
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
    i32 lossA;
    i32 lossB;
    i32 leftA;
    i32 leftB;
    i32 empty;
    i32 score;

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
    score = static_cast<i32>(score + rewardValue * chance);
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
VA(0x0042f9bd, 0x1d)
i32 philAI::ChooseToPayRansomOnHero(hero*, i32) {
    return 1;
}

// Buka 2.1 BuildBuilding with HoMM1's town update written in place: the mage
// guild level, castle conversion and new dwelling stock.
VA(0x0042f9da, 0x194)
void philAI::BuildBuilding(town* townPointer, i16 building) {
    i32 i;
    i32 cost[RESOURCE_COUNT];

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
VA(0x0042fb6e, 0x25c)
void philAI::BuildHero(town* townPointer, i16 availableHeroIndex) {
    hero* newHero;
    i16 townX;
    i16 townY;

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
VA(0x0042fdca, 0x100)
void philAI::BuildCreature(town* townPointer, i32 dwelling, i32 purchaseCount) {
    i32 cost[RESOURCE_COUNT];
    i32 creature;
    i32 i;

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
VA(0x0042feca, 0x188)
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
VA(0x004301c9, 0x26a)
void philAI::FightEvent(hero* heroPointer, mapCell* cell) {
    float attackerLoss;
    i32 rewardValue;
    i32 unusedValue;
    i32 evalValue;
    i16 guards[4];
    float defenderLoss;
    i32 flag;
    i16 n;
    i32 won;

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
            rewardValue = static_cast<i32>(gafAITurnCostResource[RESOURCE_GOLD] * 1000.0f);
            break;
        case GHOST_SITE_MEDIUM:
            rewardValue = static_cast<i32>(gafAITurnCostResource[RESOURCE_GOLD] * 2000.0f);
            break;
        case GHOST_SITE_LARGE:
            rewardValue = static_cast<i32>(gafAITurnCostResource[RESOURCE_GOLD] * 5000.0f);
            break;
        case GHOST_SITE_HUGE:
            rewardValue = static_cast<i32>(
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
VA(0x004304a6, 0x66)
float philAI::StatChangeValue(i32 oldValue, i32 newValue) {
    float newRV;
    float oldRV;

    if (newValue > 20)
        newRV = gSpellCastNumMod[20];
    else
        newRV = gSpellCastNumMod[newValue];
    if (oldValue > 20)
        oldRV = gSpellCastNumMod[20];
    else
        oldRV = gSpellCastNumMod[oldValue];
    return newRV - oldRV;
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
VA(0x004305d7, 0x221)
void philAI::TownEvent(mapCell* cell, hero* heroPointer, i32 x, i32 y) {
    float attackerLoss;
    float defenderLoss;
    i32 owner;
    i32 quickResult;
    town* townPointer;
    hero* defendingHero;
    i32 outcome;

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
DATA(0x004b3008)
i32 gArtifactChoice3;
DATA(0x004c0b8c)
i32 gEventTownId;
DATA(0x004c2038)
i32 gEventSeen;
DATA(0x004aa0d4)
i32 gPurchaseNum;
DATA(0x004c0ba4)
i32 gPurchaseSlot;
DATA(0x004c2058)
armyGroup* gEventTownArmy;
DATA(0x004b302c)
i32 gDefaultEventType;
DATA(0x004c0b94)
mapCell* gEventCell;
DATA(0x004aca60)
i32 gReduceByReload;
DATA(0x004c2050)
i32 gReduceByBerserk;
DATA(0x004c2060)
town* gEventTown;
DATA(0x004aa0c8)
i32 gEventRV;
DATA(0x004b3030)
i32 gMonsterCount;
DATA(0x004aa0d0)
i32 gTownValue;
DATA(0x004aca6c)
hero* gEventHero;

// @early-stop 99.77: the daemon-cave reward sum. Retail adds
// fv*300 + gold*2500.0f first and keeps (fv*100 + m_artifactValue) as a unit;
// VC4 here reassociates the float chain (m_artifactValue moves next to the
// first term, gold*2500.0f after fv*300). Float reassociation is outside
// a C1/C2 trace replay; the solver's 64 TU shifts (32 classes) never beat
// this state and TU-state trials stay at 99.773. Regroupings, swapped
// inner order, double/float casts of either term and paired grouping of
// the tail do not reproduce it. One handle-state cmp operand order
// (locals -0x38/-0x14) also remains.
// donor PoL RVA 0x00043fc4; preferred Buka symbol ?ValueOfEventAtPosition@philAI@@QAEHHHHPAH@Z
// donor Buka TU SOURCE/PHILAI; HoMM1 owner inferred from contiguous order
// evidence: graph:4;base=0.465517;margin=0.659381;shape=0.256;size=0.790;calls=0.952;alternate=pol20:int philAI::ValueOfEventAtPosition(int, int, int, int *)@0x00043fc4
VA(0x004307f8, 0x206d)
i32 philAI::ValueOfEventAtPosition(hero* pHero, i16 x, i16 y, i32 immediate, i32* liveChance) {
    DATA(0x0049f7a0)
    static i32 gEvaluatingTravelGates = 1;
    i32 numToBuy;
    i32 bWon9;
    i32 costList[RESOURCE_COUNT];
    i32 guardCount1;
    i32 exitRV5;
    mapCell* exitCell;
    i32 gateY28;
    i32 bestRV1;
    i32 gateX1;
    i32 exitLiveChance;
    i32 goldCost;
    i32 armySlot2;
    i32 positionValue;
    i32 prize5;
    i32 bBattleWon9;
    i32 chosenExitY27;
    i32 chosenExitX0;

    if (!immediate && gaiHeroEventStratRVOfPos[x][y] != RV_UNSET)
        return gaiHeroEventStratRVOfPos[x][y];
    gReduceByReload = 1;
    gReduceByBerserk = 1;
    *liveChance = 100;
    gEventRV = 0;
    gEventCell = gpAdvManager->GetCell(x, y);
    if (mapVisited[x][y] && giCurPlayerBit)
        gEventSeen = 1;
    else
        gEventSeen = 0;
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
                gArtifactBaseRV[gEventCell->m_objectIndex] * gWinChance + gOutcome
            );
            gArtifactChoice3 = static_cast<i32>(
                gArtifactBaseRV[gEventCell->m_objectIndex]
                - gafAITurnCostResource[RESOURCE_GOLD] * 2000.0f
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
                    gArtifactChoice1 * 0.6 + gArtifactChoice3 * 0.2 + gArtifactChoice2 * 0.2
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
                armySlot2
            );
            if ((gEventCell->m_objectMetadata & MONSTER_WILLING_FLAG)
                && gpPhilAI->FightValueOfStack(&pHero->m_army, pHero, 0, 0, 0)
                       > gMonsterDatabase[gEventCell->m_objectIndex].fightValue
                             * (gEventCell->m_objectMetadata & MONSTER_COUNT_MASK) * 1.75) {
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
                            ((gbHumanPlayer[gEventTown->m_owner] ? gAttackHumanBonus
                                                                 : gAttackComputerBonus)
                                 * ((5 - gpGame->m_playerCount) * 0.25)
                             + 1.0)
                            * gTownValue
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
                    gEventRV = static_cast<i32>(gTownValue * gWinChance + gEventRV);
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
                        &gpGame->m_heroRecs[gEventTown->OccupyingHero()].m_army,
                        &gpGame->m_heroRecs[gEventTown->OccupyingHero()],
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
                        ((5 - gpGame->m_playerCount) * 0.25 + 0.9)
                        * (gbHumanPlayer[gEventTown->m_owner] ? gAttackHumanBonus
                                                              : gAttackComputerBonus)
                        * gTownValue
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
                    (pHero->m_aiFightValue * 300.0 + gafAITurnCostResource[RESOURCE_GOLD] * 2500.0f)
                    + (gpCurPlayer->m_aiData.m_artifactValue + pHero->m_aiFightValue * 100.0)
                    + pHero->m_aiFightValue * 300.0 + gafAITurnCostResource[RESOURCE_GOLD] * -750.0
                );
                if (gEventCell->m_objectMetadata == DAEMON_REWARD_RANSOM
                    && gpCurPlayer->m_resources[RESOURCE_GOLD] < DAEMON_GOLD)
                    gEventRV = -100;
            }
            break;
        case MAP_OBJECT_OASIS:
            if (!(pHero->m_eventFlags & HERO_EVENT_OASIS))
                gEventRV = static_cast<i32>(pHero->m_aiFightValue * 200.0f);
            break;
        case MAP_OBJECT_BUOY:
            if (!(pHero->m_eventFlags & HERO_EVENT_BUOY))
                gEventRV = static_cast<i32>(pHero->m_aiFightValue * 200.0f);
            break;
        case MAP_OBJECT_STATUE:
            if (!(pHero->m_eventFlags & HERO_EVENT_STATUE))
                gEventRV = static_cast<i32>(pHero->m_aiFightValue * 400.0f);
            break;
        case MAP_OBJECT_FAERIE_RING:
            if (!(pHero->m_eventFlags & HERO_EVENT_FAERIE_RING))
                gEventRV = static_cast<i32>(pHero->m_aiFightValue * 200.0f);
            break;
        case MAP_OBJECT_FOUNTAIN:
            if (!(pHero->m_eventFlags & HERO_EVENT_FOUNTAIN))
                gEventRV = static_cast<i32>(pHero->m_aiFightValue * 200.0f);
            break;
        case MAP_OBJECT_TREASURE_CHEST:
            gEventRV = static_cast<i32>(gafAITurnCostResource[RESOURCE_GOLD] * 1500.0f);
            break;
        case MAP_OBJECT_CAMPFIRE:
            gEventRV = static_cast<i32>(
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
                gEventLoop = pHero->AddSpell(
                    gEventCell->m_objectMetadata - 1,
                    pHero->m_primaryStats[HERO_PRIMARY_KNOWLEDGE],
                    1
                );
                gEventRV = gSpellAIValue[gEventCell->m_objectMetadata - 1];
                gEventRV = static_cast<i32>(
                    StatChangeValue(
                        pHero->m_primaryStats[HERO_PRIMARY_KNOWLEDGE] - gEventLoop,
                        pHero->m_primaryStats[HERO_PRIMARY_KNOWLEDGE]
                    )
                    * gEventRV
                );
                if (gSpellAIFlags[gEventCell->m_objectMetadata - 1]
                    & SPELL_AI_FLAG_SCALES_WITH_POWER)
                    gEventRV = static_cast<i32>(
                        gEventRV
                        * (pHero->m_primaryStats[HERO_PRIMARY_KNOWLEDGE] > 40
                               ? gStatPower[40]
                               : gStatPower[pHero->m_primaryStats[HERO_PRIMARY_KNOWLEDGE]])
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
                memset(costList, 0, sizeof(costList));
                costList[gEventCell->m_objectMetadata] = WINDMILL_RESOURCE_AMOUNT;
                gEventRV = RVConversion(costList);
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
                for (gEventLoop = 0; gEventLoop < ARMY_GROUP_SLOT_COUNT; gEventLoop++) {
                    gpMonGroup->m_creatureTypes[gEventLoop] = CREATURE_GHOST;
                    gpMonGroup->m_creatureCounts[gEventLoop] = guardCount1;
                }
                ChooseEvaluateBattle(
                    &pHero->m_army,
                    pHero,
                    gpMonGroup,
                    NULL,
                    0,
                    0,
                    static_cast<i32>(
                        goldCost * gafAITurnCostResource[RESOURCE_GOLD]
                        + (gEventCell->m_objectMetadata == GHOST_SITE_HUGE
                               ? gpCurPlayer->m_aiData.m_artifactValue
                               : 0)
                    ),
                    bWon9,
                    gEventRV
                );
            }
            break;
        case MAP_OBJECT_DRAGON_CITY:
            prize5 = static_cast<i32>(
                gaiTurnValueOfMine[x][y] * gafAITurnCostResource[RESOURCE_GOLD] * 1000.0f * 1.5
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
                        (gpGame->m_players[gpGame->m_mineOwners[0]].m_aiData.m_artifactPoolShare
                         + 1.0)
                        * prize5
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
                    prize5,
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
            bestRV1 = -9999;
            for (gateY28 = 0; gateY28 < MAP_CELL_GRID_SIZE; gateY28++) {
                for (gateX1 = 0; gateX1 < MAP_CELL_GRID_SIZE; gateX1++) {
                    exitCell = gpAdvManager->GetCell(gateX1, gateY28);
                    if (MANHATTAN_LENGTH(gateX1 - x, gateY28 - y)
                            > ((gEventCell->m_triggerType & MAP_TRIGGER_TYPE_MASK)
                                       == MAP_OBJECT_STONE_LITHS
                                   ? 1
                                   : 3)
                        && exitCell->m_triggerType == gEventCell->m_triggerType) {
                        exitRV5 =
                            StrategicValueOfPosition(pHero, gateX1, gateY28, 0, &exitLiveChance);
                        exitRV5 = static_cast<i32>(exitRV5 * 0.85);
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
                gEventRV = bestRV1 - positionValue - 200;
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
                    (gpCurPlayer->m_ultimateArtifactHintChance - 15) * gUltArtifactAvgValue / 100;
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

// Located from HoMM2 Buka 2.1; PoL 2.0 supplies the VC4 declaration.

#include <match.h>

#include <BASE/BITS.h>
#include <BASE/BMAP2.h>
#include <BASE/INPUTMGR_TYPES.h>
#include <BASE/MISC_TYPES.h>
#include <BASE/Misc.h>
#include <H1/All.h>
#include <H1/KB.h>
#include <SOURCE/highScoreRuntime.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

DATA(0x004913a0)
signed char gDwellingType[4][6];
DATA(0x004af754)
signed char giBuildShipyard[AI_PLAYER_COUNT];
DATA(0x004af75c)
signed char giBuildBoat[AI_PLAYER_COUNT];
DATA(0x004c4de0)
signed char giBuildBoatStuffTurn[AI_PLAYER_COUNT];

// donor PoL RVA 0x000379d0; preferred Buka symbol ?CheckDoMain@@YIXHH@Z
// donor Buka TU SOURCE/PHILAI; HoMM1 owner inferred from contiguous order
// evidence: graph:5;base=0.463287;margin=0.627999;shape=0.348;size=0.713;calls=0.909;alternate=pol20:void CheckDoMain(int, int)@0x000379d0
VA(0x00419f16, 0x1ef)
void CheckDoMain(int, int doMain) {
    if (KBTickCount() > iLastFrameRateTimer + 15 || KBTickCount() > glTimers[0]) {
        Process1WindowsMessage();
        PollSound();
        if (KBTickCount() > glTimers[0]) {
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
            glTimers[0] = KBTickCount() + 120;
        } else if (gpMouseManager->m_mouseX - gpMouseManager->m_hotspotX != gpMouseManager->m_drawnX
                   || gpMouseManager->m_mouseY - gpMouseManager->m_hotspotY != gpMouseManager->m_drawnY) {
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
        FillBitmapArea(gpWindowManager->m_screen, 0, 460, 640, 20, 0);
        m_debugFont->DrawBoundedString(text, 0, 464, 640, 16, 1, 0);
        BlitBitmapToScreen(gpWindowManager->m_screen, 0, 460, 640, 20, 0, 460);
    }
}

// Preferred Buka's three build arrays, plus HoMM1's surviving debug-font owner.
VA(0x0041a1a1, 0x5e)
philAI::philAI() {
    int i;

    m_debugFont = 0;
    for (i = AI_PLAYER_BEGIN; i < AI_PLAYER_END; i++) {
        giBuildShipyard[i] = -1;
        giBuildBoat[i] = -1;
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
        if (pTown->m_occupyingHeroId != -1)
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
    dockTown = 0;
    if (giBuildShipyard[giCurPlayer] >= 0)
        dockTown = &gpGame->m_castleRecs[giBuildShipyard[giCurPlayer]];
    else if (giBuildBoat[giCurPlayer] >= 0)
        dockTown = &gpGame->m_castleRecs[giBuildBoat[giCurPlayer]];
    if (giBuildShipyard[giCurPlayer] >= 0)
        dockTown = gpGame->GetTown(giBuildShipyard[giCurPlayer]);
    else if (giBuildBoat[giCurPlayer] >= 0)
        dockTown = gpGame->GetTown(giBuildBoat[giCurPlayer]);
    if (dockTown && dockTown->m_owner != giCurPlayer) {
        giBuildShipyard[giCurPlayer] = -1;
        giBuildBoat[giCurPlayer] = giBuildShipyard[giCurPlayer];
        dockTown = 0;
    }
    if (giBuildShipyard[giCurPlayer] >= 0) {
        if (CanBuy(dockTown, 3) && CanBuild(dockTown, 3)) {
            BuildBuilding(dockTown, 3);
            giBuildShipyard[giCurPlayer] = -1;
        } else {
            gpCurPlayer->m_resources[RESOURCE_GOLD] -= 2000;
            gpCurPlayer->m_resources[RESOURCE_WOOD] -= 20;
        }
    }
    if (giBuildBoat[giCurPlayer] >= 0) {
        if ((dockTown->m_buildings & 8) && gpCurPlayer->m_resources[RESOURCE_GOLD] >= 1000
            && gpCurPlayer->m_resources[RESOURCE_WOOD] >= 10) {
            if (gpGame->CreateBoat(dockTown->m_x - 1, dockTown->m_y + 1) != -1) {
                gpCurPlayer->m_resources[RESOURCE_GOLD] -= 1000;
                gpCurPlayer->m_resources[RESOURCE_WOOD] -= 10;
            }
            giBuildBoat[giCurPlayer] = -1;
        } else {
            gpCurPlayer->m_resources[RESOURCE_GOLD] -= 1000;
            gpCurPlayer->m_resources[RESOURCE_WOOD] -= 10;
        }
    }
    DoAllHeroInteractions();
    while (!done) {
        GetBestBHC(giCurPlayer, bestBHC);
        if (bestBHC.type >= 0 && CanBuyBHC(bestBHC)) {
            switch (bestBHC.type) {
            case 0:
                BuildBuilding(bestBHC.pTown, bestBHC.what);
                break;
            case 1:
                BuildHero(bestBHC.pTown, bestBHC.what);
                break;
            case 2:
                BuildCreature(bestBHC.pTown, bestBHC.what, bestBHC.num);
                break;
            }
            boughtSomething = 1;
        } else
            done = 1;
    }
    if (giBuildShipyard[giCurPlayer] >= 0) {
        gpCurPlayer->m_resources[RESOURCE_GOLD] += 2000;
        gpCurPlayer->m_resources[RESOURCE_WOOD] += 20;
    }
    if (giBuildBoat[giCurPlayer] >= 0) {
        gpCurPlayer->m_resources[RESOURCE_GOLD] += 1000;
        gpCurPlayer->m_resources[RESOURCE_WOOD] += 10;
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
    int maxValue;
    int value;
    int iChance;

    bestDirection = -1;
    maxValue = 100;
    if ((gpAdvManager->GetCell(pHero->m_x, pHero->m_y)->m_triggerType & 0x7f) == 0x29)
        return 0;
    for (dirIndex = 0; dirIndex < 8; dirIndex++) {
        if (gpAdvManager->ValidMoveWithEvent(pHero, dirIndex)) {
            x = normalDirTable[dirIndex].x + pHero->m_x;
            y = normalDirTable[dirIndex].y + pHero->m_y;
            if ((gpAdvManager->GetCell(x, y)->m_triggerType & 0x80) && !(mapExtra[x][y] & 0x80)
                && (gpAdvManager->GetCell(x, y)->m_triggerType & 0x7f) != 0x29
                && (gpAdvManager->GetCell(x, y)->m_triggerType & 0x7f) != 0x2c) {
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
    int mapX;
    mapCell* visitedCell;
    int heroFightValue;
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
        pHero->m_eventFlags & 0x80,
        0,
        pHero->m_remainingMobility,
        pHero->m_unknown1c,
        -1,
        -1,
        0,
        0
    );
    for (mapX = 0; mapX < 72; mapX++) {
        for (mapY = 0; mapY < 72; mapY++) {
            if (gpSearchArray->m_cells[mapX][mapY].visited) {
                visitedCell = gpAdvManager->GetCell(mapX, mapY);
                switch (visitedCell->m_triggerType) {
                case 0xa8:
                    enemy = FightValueOfStack(
                        &gpGame->GetTown(visitedCell->m_objectMetadata)->m_army, 0, 0, 0, 0
                    );
                    if (gpGame->m_townOwners[visitedCell->m_objectMetadata] == pHero->m_owner) {
                        if (enemy > heroFightValue * 2)
                            friendly += ((float)enemy / (heroFightValue * 2) - 1.0f)
                                        * (pHero->m_mobility + 10)
                                        / (gpSearchArray->m_cells[mapX][mapY].distance + 10);
                    } else if (enemy > heroFightValue >> 1) {
                        enemyPressure += ((float)enemy / (heroFightValue >> 1) - 1.0f)
                                         * (pHero->m_mobility + 30)
                                         / (gpSearchArray->m_cells[mapX][mapY].distance + 30);
                    }
                    break;
                case 0xbd:
                    if (gpGame->m_availableHeroes[visitedCell->m_objectMetadata] != pHero->m_owner) {
                        enemy = FightValueOfStack(
                            &gpGame->GetHero(visitedCell->m_objectMetadata)->m_army, 0, 0, 0, 0
                        );
                        if (enemy > heroFightValue >> 1)
                            enemyPressure += ((float)enemy / (heroFightValue >> 1) - 1.0f)
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
    int heroFightValue;
    int x;
    mapCell* cell;
    int y;
    hero* enemyHero;
    int best = -1;
    int enemy;

    gbBerserk = 0;
    fBerserkFactor = 1.0f;
    heroFightValue = FightValueOfStack(&pHero->m_army, pHero, 1, 0, 0);
    if (heroFightValue < 100)
        heroFightValue = 100;
    if (heroFightValue < 30000)
        return;
    for (x = 0; x < 72; x++) {
        for (y = 0; y < 72; y++) {
            cell = gpAdvManager->GetCell(x, y);
            switch (cell->m_triggerType) {
            case 0xa8:
                if (gpGame->m_townOwners[cell->m_objectMetadata] != pHero->m_owner) {
                    if (gpGame->m_townOwners[cell->m_objectMetadata] != -1) {
                        enemy = FightValueOfStack(
                            &gpGame->GetTown(cell->m_objectMetadata)->m_army,
                            0,
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
            case 0xbd:
                if (gpGame->m_availableHeroes[cell->m_objectMetadata] != pHero->m_owner) {
                    enemyHero = gpGame->GetHero(cell->m_objectMetadata);
                    enemy = FightValueOfStack(
                        &enemyHero->m_army,
                        0,
                        1,
                        enemyHero->m_locationType == 0xa8,
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
    int i;
    int x, y;
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
            if (!(cell->m_triggerType & 0x80) && !(cell->m_unknown07 & 0x80)) {
                bestX = x;
                bestY = y;
                length = gpSearchArray->m_pathLength - i;
            }
        }
    }
    if (bestX == -1 || length <= 4)
        return 0;
    gpAdvManager->TeleportTo(bestX, bestY, 0);
    if (pHero->m_remainingMobility < 12)
        pHero->m_remainingMobility = 0;
    else
        pHero->m_remainingMobility -= 12;
    pHero->UseSpell(27);
    return 1;
}

// donor PoL RVA 0x00039631; preferred Buka symbol ?DoAI@philAI@@QAEXH@Z
// donor Buka TU SOURCE/PHILAI; HoMM1 owner inferred from contiguous order
// evidence: graph:5;base=0.641984;margin=1.146879;shape=0.398;size=0.795;calls=0.741;strings====================================|DO AI|DO AI 1;alternate=pol20:void philAI::DoAI(int)@0x00039631
VA(0x0041b144, 0x8f0)
void philAI::DoAI(int) {}

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
    float fFightVal;
    hero* heroPointer;
    int hIndex;
    int i;
    int y;
    int x;
    int artTotal;
    int otherIndex;
    int unusedFightValue;
    town* townPointer;

    giCurTurn = gpGame->m_day + (gpGame->m_week - 1) * 7 + (gpGame->m_month - 1) * 28;
    GetTurnAttentionValue(player);
    TurnCostResource(player);
    iCurHourGlassPhase = 0;
    iSandAnim = 0;
    gpCurPlayer->m_aiData.m_obeliskValue = (int)TurnValueOfObelisk(player);
    gpCurPlayer->m_aiData.m_unexploredValue = MeanRVOfUnexploredTerritory(player);
    bHeroBuiltThisTurn = 0;
    if (giCurTurn - giBuildBoatStuffTurn[player] > 8) {
        giBuildShipyard[player] = -1;
        giBuildBoat[player] = -1;
    }
    unusedFightValue = 0;
    fFightVal = 0.0f;
    totalFightValue = 0;
    for (i = 0; i < gpCurPlayer->m_heroCount; i++) {
        heroPointer = gpGame->GetHero(gpCurPlayer->m_heroIds[i]);
        fFightVal = (float)FightValueOfStack(&heroPointer->m_army, heroPointer, 0, 0, 0);
        totalFightValue = (int)(totalFightValue + fFightVal);
        heroPointer->m_aiFightValue = fFightVal * 4e-05 + 0.4;
    }
    for (i = 0; i < gpCurPlayer->m_townCount; i++) {
        townPointer = gpGame->GetTown(gpCurPlayer->m_townIds[i]);
        fFightVal = (float)FightValueOfStack(&townPointer->m_army, 0, 0, 0, 0);
        totalFightValue = (int)(totalFightValue + fFightVal);
    }
    gpCurPlayer->m_aiData.m_upgradeValueWeight =
        (float)(gpCurPlayer->m_resources[RESOURCE_GOLD] + gpCurPlayer->m_aiData.m_income[RESOURCE_GOLD])
            / (totalFightValue + 1000)
        + gpCurPlayer->m_aiData.m_attentionWeights.upgradeBase;
    artTotal = 0;
    for (i = 4; i < 37; i++)
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
                        if (x >= 0 && x < 72 && y >= 0 && y < 72) {
                            mineValue = abs(abs(x - xPos) + abs(y - yPos) - 4) >> 2;
                            if (gaiTurnValueOfMine[x][y] > mineValue)
                                gaiTurnValueOfMine[x][y] = mineValue;
                        }
                    }
                }
            }
        }
    }
    for (i = 0; i < 36; i++)
        gfHeroInteractionBonus[i] = 1.0f;
    if (gpCurPlayer->m_difficulty == 1) {
        gfAttackHumanBonus = 0.6f;
        gfAttackComputerBonus = 1.3f;
    } else if (gpCurPlayer->m_difficulty == 2) {
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
    int ideal[36];
    int strengths[36];
    BHC choice;
    int townNo;
    town* townPointer;
    int meanStrength;

    for (townNo = 0; townNo < gpCurPlayer->m_townCount; townNo++) {
        townPointer = &gpGame->m_castleRecs[gpCurPlayer->m_townIds[townNo]];
        strengths[townNo] = FightValueOfStack(&townPointer->m_army, 0, 0, 0, 0) + 400;
        total += strengths[townNo];
        if (townPointer->m_buildings & 0x40)
            totalWeights += 10;
        else
            totalWeights += 7;
    }
    if (totalWeights < 1)
        totalWeights = 1;
    meanStrength = total / totalWeights;
    for (townNo = 0; townNo < gpCurPlayer->m_townCount; townNo++) {
        townPointer = &gpGame->m_castleRecs[gpCurPlayer->m_townIds[townNo]];
        ideal[townNo] = meanStrength * ((townPointer->m_buildings & 0x40) ? 10 : 7) + 400;
    }
    for (townNo = 0; townNo < gpCurPlayer->m_townCount; townNo++) {
        townPointer = &gpGame->m_castleRecs[gpCurPlayer->m_townIds[townNo]];
        LogInt("Turns Owned", townPointer->m_turnsOwned);
        if (giCurTurn > 3 && (!gbRemoteOn || gbSerialCompression) && townPointer->m_turnsOwned < 3)
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
        fValue = fValue * ((float)ideal[townNo] / (float)strengths[townNo] / 3.0f + 0.66);
        fValue = fValue * ((100 - Random(0, 10)) / 100.0);
        if (fValue > bestBHCValue) {
            bestBHCValue = fValue;
            best = choice;
        }
        CheckDoMain(0, 0);
        if (gpCurPlayer->m_heroCount < giMaxHeroesForThisPlayer && (townPointer->m_buildings & 0x40)) {
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
    LogStr("BestBHC ", best.type, (int)(bestBHCValue * 100.0f), best.what, 0, 0);
    if (bestBHCValue < 0.02)
        best.type = -1;
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
            mobility = gpGame->m_heroRecs[gpGame->m_players[player].m_heroIds[i]].m_remainingMobility;
            if (mobility > bestMobility) {
                bestMobility = mobility;
                bestHero = i;
            }
        }
    }
    if (bestHero >= 0)
        return &gpGame->m_heroRecs[gpGame->m_players[player].m_heroIds[bestHero]];
    gpGame->m_players[player].m_currentHero = -1;
    return 0;
}

// @early-stop
// Complete & correct; two residuals are /Od codegen-shape picks (verified via scratch cl,
// not source-steerable): (1) the hero-slot 2D access gpGame[0x4a0+player*283+i] — cl emits
// the full player*283 then `+i`; retail strength-reduces to (i-player)+player*284 (identical
// address). (2) the fight-value max `cmp` loads the fresh value where retail loads the
// accumulator (the same operand-memory pick parked on SetupRelativeHeroStrengths).

// donor PoL RVA 0x0003b865; preferred Buka symbol ?DetermineTargetPosition@philAI@@QAEHAAH0H0@Z
// donor Buka TU SOURCE/PHILAI; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.393525;margin=0.196143;shape=0.308;size=0.686;calls=0.529;alternate=pol20:int philAI::DetermineTargetPosition(int &, int &, int, int &)@0x0003b865
VA(0x0041c83b, 0x932)
int philAI::DetermineTargetPosition(int&, int&, int, int&) {
    return 0;
}

// donor PoL RVA 0x0003c6e2; preferred Buka symbol ?ProbableOutcomeOfBattle@philAI@@QAEXPAVarmyGroup@@PAVhero@@010HHHAAMAAH3333@Z
// donor Buka TU SOURCE/PHILAI; HoMM1 owner inferred from contiguous order
// evidence: graph:4;base=0.549946;margin=0.581641;shape=0.449;size=0.953;calls=0.724;alternate=pol20:void philAI::ProbableOutcomeOfBattle(class armyGroup *, class hero *, class armyGroup *, class hero *, class armyGroup *, int, int, int, float &, int &, int &, int &, int &, int &)@0x0003c6e2
VA(0x0041d16d, 0x64d)
void philAI::ProbableOutcomeOfBattle(
    class armyGroup*,
    class hero*,
    class armyGroup*,
    class hero*,
    class armyGroup*,
    signed char,
    signed char,
    int,
    float&,
    int&,
    int&,
    int&,
    int&,
    int&
) {}

// Buka 2.1 GetOddsOfWinning returns the exact constant seen in retail's fld.
// @dead-code
// Zero-ref: pinned retail has no incoming direct call/jump or relocated reference.
VA(0x0041d7ba, 0x1e)
float philAI::GetOddsOfWinning(int) {
    return 1.0f;
}

// donor PoL RVA 0x0003d6b7; preferred Buka symbol ?GetBestBuilding@philAI@@QAEXPAVtown@@AAUBHC@@AAM@Z
// donor Buka TU SOURCE/PHILAI; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.526596;margin=0.075083;shape=0.323;size=0.949;calls=1.000;alternate=pol20:void philAI::GetBestBuilding(class town *, struct BHC &, float &)@0x0003d6b7
VA(0x0041dd73, 0x185)
void philAI::GetBestBuilding(town* townPointer, BHC& purchase, float& benefitCost) {
    float buildingValue;
    int bestBuilding;
    float bestScore;
    float bestCost;
    int curBuilding;
    int costRV;
    float score;

    bestCost = -99.0f;
    bestScore = -99.0f;
    bestBuilding = -1;
    for (curBuilding = 0; curBuilding <= 12; curBuilding++) {
        if (!(townPointer->m_buildings & (1 << curBuilding))
            || (curBuilding == 0 && townPointer->m_buildState < 3)) {
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
    purchase.type = 0;
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
    int monsterCost[PLAYER_RESOURCE_COUNT];
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
    if (townPointer->m_occupyingHeroId != -1) {
        occupant = gpGame->GetHero(townPointer->m_occupyingHeroId);
        creatRV = static_cast<int>(creatRV * 1.1);
        if (occupant->m_unknown1c == creature / 6)
            creatRV = static_cast<int>(creatRV * 1.1f);
        if ((gMonsterDatabase[creature].attributes & 8)) {
            for (n = 0; n < 5; n++) {
                if (occupant->m_army.m_creatureTypes[n] != -1
                    && (gMonsterDatabase[occupant->m_army.m_creatureTypes[n]].attributes & 8))
                    archers++;
            }
            creatRV = static_cast<int>(creatRV * (1.18 - archers * 0.06));
        }
        creatRV = static_cast<int>(
            creatRV
            * (gpGame->m_players[townPointer->m_owner].m_aiData.m_attentionWeights.upgradeBase + 0.66)
        );
    }
    if ((gMonsterDatabase[creature].attributes & 8)) {
        for (slotNum = 0; slotNum < 5; slotNum++) {
            if (townPointer->m_army.m_creatureTypes[slotNum] != -1
                && (gMonsterDatabase[townPointer->m_army.m_creatureTypes[slotNum]].attributes & 8))
                archers++;
        }
        creatRV = static_cast<int>(creatRV * (1.18 - archers * 0.06));
    }
    LikelihoodOfEnemyAttacking(townPointer, 0, attackChance, foeStrength, nTurns, nPoints, nWeeks, peril);
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
    int canAdd;
    int bestBuy;
    float worth;
    float bestRandScore;
    float bestCost;
    int topDwelling;
    int curDwelling;
    float rand;
    int mon;
    int iArmy;
    int numUnits;
    int costRV;

    topDwelling = -1;
    bestBuy = 0;
    bestCost = -99.0f;
    bestRandScore = -99.0f;
    for (curDwelling = 0; curDwelling < 6; curDwelling++) {
        mon = gDwellingType[townPointer->m_type][curDwelling];
        if ((townPointer->m_buildings & (1 << (curDwelling + 7)))
            && townPointer->m_garrison[curDwelling] > 0) {
            canAdd = 0;
            for (iArmy = 0; iArmy < 5; iArmy++) {
                if (townPointer->m_army.m_creatureTypes[iArmy] == -1
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
    best.type = 2;
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
    int monsterCost[PLAYER_RESOURCE_COUNT];
    int maxUnits;
    int i;

    GetMonsterCost(creatureType, monsterCost);
    for (i = 0; i < PLAYER_RESOURCE_COUNT; i++) {
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
void philAI::ValueOfBuyingHero(town* townPointer, hero* heroPointer, int& resourceValue, float& benefitCost) {
    int tmp;
    int heroRV;
    int i;
    int heroCost[PLAYER_RESOURCE_COUNT];
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
    for (i = 0; i < 14; i++) {
        if (heroPointer->m_artifacts[i] >= 0 && heroPointer->m_artifacts[i] < 37)
            heroRV += gArtifactBaseRV[heroPointer->m_artifacts[i]];
    }
    heroRV += heroPointer->m_experience / 2;
    heroRV = static_cast<int>(
        heroRV
        * (gpCurPlayer->m_aiData.m_attentionWeights.heroValue + 1.0
           - gpCurPlayer->m_aiData.m_attentionWeights.upgradeBase)
    );
    if (gTownHeroClass[townPointer->m_type] == heroPointer->m_unknown1c)
        heroRV = static_cast<int>(heroRV * 1.12f);
    heroRV += StrategicValueOfPosition(heroPointer, heroPointer->m_x, heroPointer->m_y, 0, &tmp);
    heroRV -= 200;
    heroRV = static_cast<int>(heroRV * FutureDeflator(heroCost));
    benefitCost = static_cast<float>(heroRV) / costRV;
    resourceValue = heroRV;
}

// donor PoL RVA 0x0003e2a8; preferred Buka symbol ?GetBestHero@philAI@@QAEXPAVtown@@AAUBHC@@AAM@Z
// donor Buka TU SOURCE/PHILAI; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.649528;margin=0.194728;shape=0.311;size=0.925;calls=0.800;strings=Town:%2d  Hero    : % 15i   Raw BC = %8.2f,  RandBC = %8.2f.;alternate=pol20:void philAI::GetBestHero(class town *, struct BHC &, float &)@0x0003e2a8
VA(0x0041e6fc, 0x1a0)
void philAI::GetBestHero(town* townPointer, BHC& best, float& bestValue) {
    int bestHero;
    float worth;
    int curHero;
    float bestScore;
    hero* availHero;
    float adjusted;
    float bestCost;
    int cost;

    bestHero = -1;
    bestCost = -99.0f;
    bestScore = -99.0f;
    for (curHero = 0; curHero < 2; curHero++) {
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
    best.type = 1;
    best.what = bestHero;
    bestValue = bestCost;
    if (gpGame->m_map[townPointer->m_x][townPointer->m_y].m_triggerType == 0xbd)
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

// Buka 2.1's attention identity constants are loaded, not folded, at /Od.
static const float AI_ATTENTION_IDENTITY_FLOAT = 1.0f;
static const float AI_ATTENTION_IDENTITY = 1.0f;

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
            maxT = fTurns > maxT ? fTurns : maxT;
        }
    }
    return maxT;
}

// donor PoL RVA 0x0003e918; preferred Buka symbol ?RVOfPosition@philAI@@QAEHHHHHHHHHHH@Z
// donor Buka TU SOURCE/PHILAI; HoMM1 owner inferred from contiguous order
// evidence: graph:3;base=0.513260;margin=0.500427;shape=0.306;size=0.971;calls=0.812;alternate=pol20:int philAI::RVOfPosition(int, int, int, int, int, int, int, int, int, int)@0x0003e918
VA(0x0041ed4b, 0x55e)
int philAI::RVOfPosition(int, int, int, int, int, int, int, int, int, int) {
    return 0;
}

// donor PoL RVA 0x0003ef45; preferred Buka symbol ?StrategicValueOfPosition@philAI@@QAEHHHHHPAHH@Z
// donor Buka TU SOURCE/PHILAI; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.499321;margin=0.324582;shape=0.341;size=0.829;calls=0.957;alternate=pol20:int philAI::StrategicValueOfPosition(int, int, int, int, int *, int)@0x0003ef45
VA(0x0041f2c3, 0x8bd)
int philAI::StrategicValueOfPosition(hero*, short, short, signed char, int*) {
    return 0;
}

// Buka 2.1 ValueOfTown without the later scenario-town bonuses: built
// structures' base values plus a fixed gold-turn allowance.
VA(0x0041fb80, 0xb1)
int philAI::ValueOfTown(town* townPointer) {
    int sum = 0;
    int building;
    for (building = 0; building < 13; building++) {
        if (townPointer->m_buildings & (1 << building))
            sum += GetBuildingBaseResourceValue(
                townPointer->m_type,
                building,
                townPointer->m_buildState > 0 ? townPointer->m_buildState : 0
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
    if (gpGame->m_ultimateArtifactId == -1)
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

// @early-stop
// Complete & correct except the two castle-match `==` compares: cl unconditionally loads
// the byte operand (town castleX/Y) before the word operand (game field); retail evaluates
// left-to-right (word first). Verified via scratch cl: byte-first is hard-wired, not
// source-steerable. Same equality result.

// donor PoL RVA 0x0003fe81; preferred Buka symbol ?FutureDeflator@philAI@@QAEMQAH@Z
// donor Buka TU SOURCE/PHILAI; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.484127;margin=0.409347;shape=0.286;size=0.877;calls=1.000;alternate=pol20:float philAI::FutureDeflator(int * const)@0x0003fe81
VA(0x0041fedb, 0x51)
float philAI::FutureDeflator(int* const resources) {
    float turns = TurnsToBuy(resources);
    float value = 1.0f - turns * 0.15f;
    if (value < 0.0)
        value = 0;
    return value;
}

// donor PoL RVA 0x0003fed2; preferred Buka symbol ?FightValueOfStack@philAI@@QAEHPAVarmyGroup@@PAVhero@@HHHH@Z
// donor Buka TU SOURCE/PHILAI; HoMM1 owner inferred from contiguous order
// evidence: graph:4;base=0.393076;margin=0.447055;shape=0.297;size=0.768;calls=0.382;alternate=pol20:int philAI::FightValueOfStack(class armyGroup *, class hero *, int, int, int, int)@0x0003fed2
// HoMM1 retail returns with ret 0x14: five stack arguments.
VA(0x0041ff2c, 0x764)
int philAI::FightValueOfStack(class armyGroup*, class hero*, int, int, signed char) {
    return 0;
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
        for (i = 0; i < 5; i++) {
            if (pHero->m_army.m_creatureTypes[i] == creature) {
                replacementSlot = -1;
                i = 5;
            } else {
                replacementValue = pHero->m_army.m_creatureCounts[i] * gMonsterDatabase[i].fightValue;
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
    float fracLost;
    float winChance;
    float rnd;
    armyGroup* winner;
    int aDead;
    int unused;
    int dDead;
    int win;
    int aLeft;
    int dLeft;
    int tmp;
    int atkExp;
    int defenderExp;
    float diff;
    int res;
    float wChance;

    atkExp = gpGame->ExperienceValueOfStack(attacker, attackerHero);
    defenderExp = gpGame->ExperienceValueOfStack(defender, defenderHero);
    win = 0;
    winner = 0;
    ProbableOutcomeOfBattle(
        attacker,
        attackerHero,
        defender,
        defenderHero,
        0,
        townBattle,
        townId,
        defenderHero != 0 ? defenderHero->m_owner : -1,
        winChance,
        aDead,
        dDead,
        aLeft,
        dLeft,
        res
    );
    if ((rnd = Random(0, 100) / 100.0) < winChance) {
        win = 1;
        wChance = winChance;
        winner = attacker;
    } else {
        wChance = 1.0f - winChance;
        winner = defender;
    }
    diff = static_cast<float>(winChance < rnd ? rnd - winChance : winChance - rnd);
    if (win != 0 && winChance > 0.6)
        diff *= winChance + 0.65;
    fracLost = (1.0 - diff) * (1.0 - diff);
    if (wChance > 0.8 && fracLost > 0.2)
        fracLost *= fracLost;
    if (wChance > 0.96 && fracLost > (1.0f - wChance) / 2.0f)
        fracLost = (1.0f - wChance) / 2.0f;
    if (win != 0) {
        if (attackerHero != 0) {
            gpAdvManager->GiveExperience(attackerHero, defenderExp, 1);
            attackerHero->ApplyBattleWinTemps();
        }
        defenderDamage = 1.0f;
        attackerDamage = fracLost;
    } else {
        if (attackerHero != 0) {
            attackerHero->m_remainingMobility = 0;
            attackerHero->ApplyBattleLossTemps();
        }
        if (defenderHero != 0)
            attackerHero->ApplyBattleWinTemps();
        defenderDamage = diff * fracLost;
        attackerDamage = 1.0f;
        if (attackerDamage >= 0.99 && defenderHero != 0)
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
void philAI::HeroInteractionAtTown(class hero*, class town*, int, int*) {}

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
        0,
        isCastle,
        castleId,
        defenderHero != 0 ? defenderHero->m_owner : -1,
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
    int cost[PLAYER_RESOURCE_COUNT];

    sprintf(
        gText,
        "Player %d built %s in town %d.\n",
        giCurPlayer,
        GetBuildingName(townPointer->m_type, building),
        townPointer->m_id
    );
    LogStr(gText);
    GetBuildingCost(townPointer->m_type, building, cost, townPointer->m_buildState);
    for (i = 0; i < PLAYER_RESOURCE_COUNT; i++)
        gpCurPlayer->m_resources[i] -= cost[i];
    if (building == 0) {
        if (townPointer->m_buildings & 1)
            townPointer->m_buildState++;
        if (townPointer->m_occupyingHeroId != -1)
            townPointer->GiveSpells();
    }
    townPointer->m_buildings |= 1 << building;
    if (building >= 7 && building <= 12)
        townPointer->m_garrison[building - 7] =
            gMonsterDatabase[gDwellingType[townPointer->m_type][building - 7]].growth;
    if (building == 6) {
        townPointer->m_buildings &= ~0x20;
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
    gpGame->SetRandomHeroArmies(newHero->m_id, 1);
    newHero->m_owner = giCurPlayer;
    newHero->m_x = townX;
    newHero->m_y = townY;
    newHero->m_eventFlags = 0;
    newHero->m_direction = 2;
    newHero->m_remainingMobility = newHero->CalcMobility();
    newHero->m_mobility = newHero->m_remainingMobility;
    newHero->m_locationType = gpGame->m_map[townX][townY].m_triggerType;
    newHero->m_occupiedTown = gpGame->m_map[townX][townY].m_objectMetadata;
    gpGame->m_map[townX][townY].m_triggerType = 0xbd;
    gpGame->m_map[townX][townY].m_objectMetadata =
        gpCurPlayer->m_availableHeroIds[availableHeroIndex];
    gpGame->m_availableHeroes[newHero->m_id] = townPointer->m_owner;
    townPointer->m_occupyingHeroId = newHero->m_id;
    townPointer->GiveSpells();
    gpCurPlayer->m_availableHeroIds[availableHeroIndex] = gpGame->GetNewHeroId(Random(0, 3));
    gpGame->m_availableHeroes[gpCurPlayer->m_availableHeroIds[availableHeroIndex]] = 0x40;
    bHeroBuiltThisTurn = 1;
    ShowStatus();
}

// Buka 2.1 BuildCreature without the full-army eviction: pay, take the stock
// and add the stack to the garrison.
VA(0x00421d5d, 0x100)
void philAI::BuildCreature(town* townPointer, int dwelling, int purchaseCount) {
    int cost[PLAYER_RESOURCE_COUNT];
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
    for (i = 0; i < PLAYER_RESOURCE_COUNT; i++)
        gpCurPlayer->m_resources[i] -= cost[i] * purchaseCount;
    townPointer->m_garrison[dwelling] -= purchaseCount;
    townPointer->m_army.Add(creature, purchaseCount, -1);
    ShowStatus();
}

// donor PoL RVA 0x00042ead; preferred Buka symbol ?CanBuyBHC@philAI@@QAEHAAUBHC@@@Z
// donor Buka TU SOURCE/PHILAI; HoMM1 owner inferred from contiguous order
// evidence: graph:3;base=0.453882;margin=0.780446;shape=0.369;size=0.712;calls=0.667;alternate=pol20:int philAI::CanBuyBHC(struct BHC &)@0x00042ead
VA(0x00421e5d, 0x188)
int philAI::CanBuyBHC(BHC& purchase) {
    int index;
    int j;
    int cost[PLAYER_RESOURCE_COUNT];
    switch (purchase.type) {
    case 0:
        if (CanBuy(purchase.pTown, purchase.what))
            return 1;
        break;
    case 1:
        if (gpCurPlayer->m_resources[RESOURCE_GOLD] >= gHeroGoldCost
            && purchase.pTown->m_occupyingHeroId == -1 && bHeroBuiltThisTurn == 0)
            return 1;
        break;
    case 2:
        j = gDwellingType[purchase.pTown->m_type][purchase.what];
        if (purchase.num > purchase.pTown->m_garrison[purchase.what])
            return 0;
        if (!purchase.pTown->m_army.CanJoin(j))
            return 0;
        GetMonsterCost(j, cost);
        for (index = 0; index < PLAYER_RESOURCE_COUNT; index++)
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
signed char philAI::CombatMonsterEvent(hero* h, int monType, int* pCount, mapCell*) {
    float casualtyRatio;
    float fLoss;
    int result;
    short newCount;
    short i;

    memset(gpMonGroup->m_creatureTypes, -1, sizeof(gpMonGroup->m_creatureTypes));
    memset(gpMonGroup->m_creatureCounts, 0, sizeof(gpMonGroup->m_creatureCounts));
    if (*pCount / 5 > 0) {
        for (i = 0; i < 5; i++) {
            gpMonGroup->m_creatureTypes[i] = monType;
            gpMonGroup->m_creatureCounts[i] = *pCount / 5;
        }
    }
    for (i = *pCount % 5 - 1; i >= 0; i--) {
        gpMonGroup->m_creatureTypes[i] = monType;
        gpMonGroup->m_creatureCounts[i]++;
    }
    result = gpPhilAI->QuickCombat(&h->m_army, h, gpMonGroup, 0, 0, 0, casualtyRatio, fLoss);
    newCount = 0;
    for (i = 0; i < 5; i++)
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

    if (cell->m_objectMetadata == 1)
        return;
    guards[0] = 2;
    guards[1] = 3;
    guards[2] = 5;
    guards[3] = 10;
    for (n = 0; n < 5; n++) {
        gpMonGroup->m_creatureTypes[n] = 26;
        gpMonGroup->m_creatureCounts[n] = guards[cell->m_objectMetadata - 2];
    }
    switch (cell->m_objectMetadata) {
    case 2:
        rewardValue = static_cast<int>(gafAITurnCostResource[RESOURCE_GOLD] * 1000.0f);
        break;
    case 3:
        rewardValue = static_cast<int>(gafAITurnCostResource[RESOURCE_GOLD] * 2000.0f);
        break;
    case 4:
        rewardValue = static_cast<int>(gafAITurnCostResource[RESOURCE_GOLD] * 5000.0f);
        break;
    case 5:
        rewardValue = static_cast<int>(
            gafAITurnCostResource[RESOURCE_GOLD] * 2000.0f + gpCurPlayer->m_aiData.m_artifactValue
        );
        break;
    default:
        return;
    }
    ChooseEvaluateBattle(&heroPointer->m_army, heroPointer, gpMonGroup, 0, 0, 0, rewardValue, flag, evalValue);
    if (flag) {
        won = QuickCombat(&heroPointer->m_army, heroPointer, gpMonGroup, 0, 0, 0, defenderLoss, attackerLoss);
        if (won) {
            switch (cell->m_objectMetadata) {
            case 2:
                gpAdvManager->GiveResource(heroPointer, RESOURCE_GOLD, 1000);
                break;
            case 3:
                gpAdvManager->GiveResource(heroPointer, RESOURCE_GOLD, 2000);
                break;
            case 4:
                gpAdvManager->GiveResource(heroPointer, RESOURCE_GOLD, 5000);
                break;
            case 5:
                gpAdvManager->GiveResource(heroPointer, RESOURCE_GOLD, 2000);
                gpAdvManager->GiveRandomArtifact(heroPointer);
                break;
            }
            cell->m_objectMetadata = 1;
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
        if (loser != 0)
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
        newRV = gfStatValue[20];
    else
        newRV = gfStatValue[newValue];
    if (oldValue > 20)
        oldRV = gfStatValue[20];
    else
        oldRV = gfStatValue[oldValue];
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
                    0,
                    1,
                    townPointer->m_id,
                    defenderLoss,
                    attackerLoss
                );
            } else {
                defendingHero = townPointer->m_occupyingHeroId == -1
                                    ? 0
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
                if (outcome == 1) {
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
        heroPointer->m_locationType = 0xa8;
        heroPointer->m_occupiedTown = townPointer->m_id;
        HeroInteractionAtTown(heroPointer, townPointer, 0, &iDummy);
    }
    gpAdvManager->MobilizeCurrHero(0);
    townPointer->GiveSpells();
}

// donor PoL RVA 0x00043fc4; preferred Buka symbol ?ValueOfEventAtPosition@philAI@@QAEHHHHPAH@Z
// donor Buka TU SOURCE/PHILAI; HoMM1 owner inferred from contiguous order
// evidence: graph:4;base=0.465517;margin=0.659381;shape=0.256;size=0.790;calls=0.952;alternate=pol20:int philAI::ValueOfEventAtPosition(int, int, int, int *)@0x00043fc4
VA(0x0042278b, 0x2085)
int philAI::ValueOfEventAtPosition(hero*, int, int, int, int*) {
    return 0;
}

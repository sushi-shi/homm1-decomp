// Located from HoMM2 Buka 2.1; PoL 2.0 supplies the VC4 declaration.

#include <match.h>

#include <BASE/Misc.h>
#include <BASE/bmap2.h>
#include <BASE/inputManager.h>
#include <BASE/INPUTMGR_TYPES.h>
#include <SOURCE/X_GLOBAL.h>
#include <H1/All.h>
#include <H1/KB.h>
#include <SOURCE/kbwin.h>
#include <SOURCE/REMOTE.h>

#include <SOURCE/dialogTypes.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// EVENTS assertion records (file literals and line base), as in MOUSEMGR.
extern short gEventsAssertLine;
extern char* gEventText[];
extern signed char gbEventMusicPlaying;
extern char* gArtifactNames[];
extern SAMPLE2 gNullSample;
extern armyGroup* gpMonsterGroup;
extern char* gResourceNames[];
extern char* gArtifactDesc[];
extern char* gSpellNames[];
void BVResMsg(char*, int, int);
extern signed char gbInCombat;
// DoEvent and DoCombat restore a music volume parked here (-1 when none).
extern int giEventMusicVolume;
// Per-cell bitmask of the players whose heroes have stood there.
extern signed char mapVisited[MAP_CELL_GRID_SIZE][MAP_CELL_GRID_SIZE];

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
    signed char ty;
    int res;
    signed char adjacentMonster;
    int income;
    hero* enemyHero;
    int numDefenders;
    mapCell* prevCell;
    town* occupiedTown;

    pHero = gpGame->GetHero(gpCurPlayer->m_currentHero);
    objType = cell->m_triggerType & 0x7f;
    erase = 0;
    fizzleMode = 0;
    gbEventMusicPlaying = 1;
    gpMouseManager->ReallyHidePointer();
    EventSound(objType, cell->m_objectMetadata);
    switch (objType) {
        case 31:
            if (pHero->m_eventFlags & HERO_EVENT_EMBARKED) {
                pHero->m_eventFlags &= ~HERO_EVENT_EMBARKED;
                pHero->m_remainingMobility = 0;
                pHero->m_direction = m_cursorDirection;
                m_cursorType = pHero->m_unknown1c;
                m_cursorFrame = GetCursorBaseFrame(m_cursorDirection);
                m_cursorActive = 1;
                gpWindowManager->SaveFizzleSource(0xc0, 0xc0, 0x60, 0x60);
                CompleteDraw(m_mapOriginX, m_mapOriginY, 0);
                gpWindowManager->FizzleForward(0xc0, 0xc0, 0x60, 0x60, -1);
                CheckAdjacentMon(&adjacentMonster);
            }
            break;
        case 62:
            ship = &gpGame->m_boats[cell->m_objectMetadata];
            gpGame->RestoreCell(-1, -1, ship->savedTriggerType, ship->savedEventData, cell, 2);
            pHero->m_eventFlags |= HERO_EVENT_EMBARKED;
            pHero->m_remainingMobility = 0;
            ship->heroId = pHero->m_id;
            ship->owner = pHero->m_owner;
            m_cursorType = 4;
            m_cursorDirection = ship->direction;
            m_cursorFrame = GetCursorBaseFrame(m_cursorDirection);
            m_cursorActive = 1;
            CompleteDraw(m_mapOriginX, m_mapOriginY, 0);
            UpdateScreen(0, 0);
            break;
        case 25:
            if (gpGame->m_mineOwners[cell->m_objectMetadata] == giCurPlayer)
                break;
            if (gpGame->m_mines[cell->m_objectMetadata].type == 6)
                income = 1000;
            else if (gpGame->m_mines[cell->m_objectMetadata].type == 2)
                income = 2;
            else
                income = 1;
            EventWindow(gpGame->m_mines[cell->m_objectMetadata].type + 0x29, 1, "",
                        gpGame->m_mines[cell->m_objectMetadata].type, -income, -1,
                        0, -1);
            goto claimMine;
        case 1:
            if (gpGame->m_mineOwners[cell->m_objectMetadata] == giCurPlayer)
                break;
            EventWindow(0, 1, "", 1, -1, -1, 0, -1);
            goto claimMine;
        case 32:
            if (gpGame->m_mineOwners[cell->m_objectMetadata] == giCurPlayer)
                break;
            EventWindow(0x37, 1, "", 0, -2, -1, 0, -1);
            goto claimMine;
        claimMine:
            gpGame->ClaimMine(cell->m_objectMetadata, giCurPlayer);
            break;
        case 23:
            if (gpGame->m_mineOwners[1] == giCurPlayer)
                break;
            gpGame->ClaimMine(1, giCurPlayer);
            EventWindow(0x28, 1, "", -1, 0, -1, 0, -1);
            break;
        case 22:
            if (gpGame->m_mineOwners[0] == giCurPlayer)
                break;
            EventWindow(0x26, 2, "", -1, 0, -1, 0, -1);
            if (gpWindowManager->m_dialogResult == NORMAL_DIALOG_CONFIRM) {
                if (gpGame->m_campaignType > 0 && gpGame->m_campaignScenario == 8)
                    numDefenders = 20;
                else
                    numDefenders = 5;
                if (CombatMonsterEvent(pHero, 0x17, numDefenders, cell, x, y, 0, x, y) == 1) {
                    gpGame->ClaimMine(0, giCurPlayer);
                    EventWindow(0x27, 1, "", 6, -1000, -1, 0, -1);
                    break;
                }
                pHero->CheckLevel();
            }
            break;
        case 6:
            EventWindow(0xb, 2, "", 6, cell->m_objectMetadata * 500, 0xe,
                        (cell->m_objectMetadata - 1) * 500, 1);
            if (gpWindowManager->m_dialogResult == NORMAL_DIALOG_CONFIRM)
                GiveResource(pHero, 6, cell->m_objectMetadata * 500);
            else
                GiveExperience(pHero, (cell->m_objectMetadata - 1) * 500, 0);
            erase = 1;
            fizzleMode = 1;
            pHero->CheckLevel();
            break;
        case 3:
            if (pHero->m_eventFlags & HERO_EVENT_BUOY) {
                EventWindow(2, 1, "", -1, 0, -1, 0, -1);
            } else {
                pHero->m_eventFlags |= HERO_EVENT_BUOY;
                pHero->m_morale++;
                EventWindow(3, 1, "", 0xc, 0, -1, 0, -1);
            }
            break;
        case 7:
            if (pHero->m_eventFlags & HERO_EVENT_FAERIE_RING) {
                EventWindow(0xc, 1, "", -1, 0, -1, 0, -1);
            } else {
                pHero->m_eventFlags |= HERO_EVENT_FAERIE_RING;
                pHero->m_luck++;
                EventWindow(0xd, 1, "", 0xa, 0, -1, 0, -1);
            }
            break;
        case 9:
            if (pHero->m_eventFlags & HERO_EVENT_FOUNTAIN) {
                EventWindow(0xf, 1, "", -1, 0, -1, 0, -1);
            } else {
                pHero->m_eventFlags |= HERO_EVENT_FOUNTAIN;
                pHero->m_luck++;
                EventWindow(0x10, 1, "", 0xa, 0, -1, 0, -1);
            }
            break;
        case 28:
            if (pHero->m_eventFlags & HERO_EVENT_OASIS) {
                EventWindow(0x34, 1, "", -1, 0, -1, 0, -1);
            } else {
                pHero->m_eventFlags |= HERO_EVENT_OASIS;
                pHero->m_morale++;
                EventWindow(0x35, 1, "", 0xc, 0, -1, 0, -1);
            }
            break;
        case 36:
            if (pHero->m_eventFlags & HERO_EVENT_TEMPLE) {
                EventWindow(0x3e, 1, "", -1, 0, -1, 0, -1);
            } else {
                pHero->m_eventFlags |= HERO_EVENT_TEMPLE;
                pHero->m_morale += 2;
                EventWindow(0x3d, 1, "", 0xc, 0, 0xc, 0, -1);
            }
            break;
        case 4:
            switch (cell->m_objectMetadata) {
                case 1:
                    EventWindow(0x4b, 1, "", -1, 0, -1, 0, -1);
                    break;
                case 2:
                    if (pHero->NumArtifacts() == HERO_ARTIFACT_SLOT_COUNT) {
                        sprintf(gText, "%s.", "Treasure");
                        EventWindow(-1, 1, gText, 6, 1000, -1, 0, -1);
                    } else {
                        artifactId = GiveRandomArtifact(pHero);
                        sprintf(gText, "%s %s", gEventText[76], gArtifactNames[artifactId]);
                        EventWindow(-1, 1, gText, 7, artifactId, -1, 0, -1);
                    }
                    cell->m_objectMetadata = 1;
                    break;
            }
            break;
        case 8:
            EventWindow(0xe, 1, "", 6, (cell->m_objectMetadata >> 4) * 100,
                        cell->m_objectMetadata & 0xf,
                        cell->m_objectMetadata >> 4, -1);
            GiveResource(pHero, 6, (cell->m_objectMetadata >> 4) * 100);
            GiveResource(pHero, cell->m_objectMetadata & 0xf,
                         cell->m_objectMetadata >> 4);
            erase = 1;
            fizzleMode = 1;
            gpGame->m_mapSounds[m_mapOriginX + 7][m_mapOriginY + 7] = -1;
            SetEnvironmentOrigin(m_mapOriginX + 7, m_mapOriginY + 7, 1);
            break;
        case 10:
            if (pHero->m_visitedSites & (1 << cell->m_objectMetadata)) {
                EventWindow(0x11, 1, "", -1, 0, -1, 0, -1);
            } else {
                EventWindow(0x12, 1, "", 0xe, 1000, -1, 0, -1);
                GiveExperience(pHero, 1000, 0);
                pHero->m_visitedSites |= 1 << cell->m_objectMetadata;
                pHero->CheckLevel();
            }
            break;
        case 24:
            if (!cell->m_objectMetadata) {
                EventWindow(0x29, 1, "", -1, 0, -1, 0, -1);
            } else {
                EventWindow(0x2a, 1, "", 6, cell->m_objectMetadata * 500, -1, 0, -1);
                GiveResource(pHero, 6, cell->m_objectMetadata * 500);
                cell->m_objectMetadata = 0;
            }
            break;
        case 29:
            resType = cell->m_objectIndex - 0x3d;
            GiveResource(pHero, resType,
                         resType == 6 ? cell->m_objectMetadata * 100
                                           : cell->m_objectMetadata);
            strcpy(resourceName, gResourceNames[resType]);
            resourceName[0] += 32;
            sprintf(gText, gEventText[54], resourceName);
            BVResMsg(gText, resType,
                     resType == 6 ? cell->m_objectMetadata * 100
                                       : cell->m_objectMetadata);
            erase = 1;
            fizzleMode = 1;
            break;
        case 45:
            if (cell->m_objectMetadata <= 6) {
                EventWindow(0x45, 1, "", cell->m_objectMetadata, 2, -1, 0, -1);
                GiveResource(pHero, cell->m_objectMetadata, 2);
                cell->m_objectMetadata = 99;
            } else {
                EventWindow(0x44, 1, "", -1, 0, -1, 0, -1);
            }
            break;
        case 11:
            EventWindow(0x13, 2, "", -1, 0, -1, 0, -1);
            if (gpWindowManager->m_dialogResult == NORMAL_DIALOG_CONFIRM) {
                RecruitEvent(pHero, 0x1b, cell);
                if (!cell->m_objectMetadata) {
                    erase = 1;
                    fizzleMode = 1;
                }
            }
            break;
        case 42:
            if (!cell->m_objectMetadata) {
                EventWindow(0x41, 1, "", -1, 0, -1, 0, -1);
            } else {
                EventWindow(0x42, 2, "", -1, 0, -1, 0, -1);
                if (gpWindowManager->m_dialogResult == NORMAL_DIALOG_CONFIRM)
                    RecruitEvent(pHero, 0x18, cell);
            }
            break;
        case 39:
            if (!cell->m_objectMetadata) {
                EventWindow(0x3f, 1, "", -1, 0, -1, 0, -1);
            } else {
                EventWindow(0x40, 2, "", -1, 0, -1, 0, -1);
                if (gpWindowManager->m_dialogResult == NORMAL_DIALOG_CONFIRM)
                    RecruitEvent(pHero, 0x19, cell);
            }
            break;
        case 13:
        case 14:
        case 15:
        case 16:
        case 17:
            HouseEvent(pHero, cell);
            break;
        case 26:
            PlayerMonsterInteract(cell, cell, pHero, &erase, x, y, 0, x, y);
            break;
        case 27:
            if (!(gpGame->m_obeliskVisitors[cell->m_objectMetadata - 1]
                  & (1 << pHero->m_owner))) {
                gpGame->VisitObelisk(pHero->m_owner);
                gpGame->m_obeliskVisitors[cell->m_objectMetadata - 1] |=
                    1 << pHero->m_owner;
                EventWindow(0x32, 1, "", -1, 0, -1, 0, -1);
                ViewPuzzle();
            } else {
                EventWindow(0x33, 1, "", -1, 0, -1, 0, -1);
            }
            break;
        case 33:
            EventWindow(0x38, 1, "", -1, 0, -1, 0, -1);
            gpMouseManager->SetPointer(0);
            win = new heroWindow(0, 0, "thiefwin.bin");
            if (!win)
                MemError();
            SetWinText(win, 0xf);
            gpTownManager->SetupThievesGuild(win, 8);
            strcpy(gText, "Shrine - Player Rankings");
            event.type = MESSAGE_WIDGET;
            event.command = WIDGET_COMMAND_SET_TEXT;
            event.id = 0;
            event.text = gText;
            win->BroadcastMessage(event);
            gpWindowManager->DoDialog(win, TrueFalseDialogHandler, 0);
            delete win;
            RedrawAdvScreen(1);
            break;
        case 34:
            sprintf(gText, "%s'%s'.", gEventText[57],
                    gSpellNames[cell->m_objectMetadata - 1]);
            if (pHero->HasArtifact(ARTIFACT_MAGIC_BOOK)) {
                pHero->AddSpell(cell->m_objectMetadata - 1,
                                    pHero->m_primaryStats[3], 0);
                EventWindow(-1, 1, gText, 8, cell->m_objectMetadata - 1, -1, 0, -1);
            } else {
                strcat(gText, "  Unfortunately, you have no Magic Book to record the spell with.");
                EventWindow(-1, 1, gText, -1, 0, -1, 0, -1);
            }
            break;
        case 40:
            if (giEventMusicVolume != -1)
                gConfig.musicVolume = giEventMusicVolume;
            giEventMusicVolume = -1;
            TownEvent(cell, x, y);
            break;
        case 44:
            DoWhirlpool(pHero);
        case 41:
            teleportCount = 0;
            for (ty = 0; ty < MAP_CELL_GRID_SIZE; ty++) {
                for (tx = 0; tx < MAP_CELL_GRID_SIZE; tx++) {
                    if (gpGame->m_map[tx][ty].m_triggerType
                            == (unsigned char)(objType | 0x80)
                        && abs(tx - x) + abs(ty - y) > (objType == 41 ? 1 : 3))
                        teleportCount++;
                }
            }
            if (teleportCount >= 1) {
                if (teleportCount > 1)
                    teleportCount = Random(1, teleportCount);
                for (ty = 0; ty < MAP_CELL_GRID_SIZE; ty++) {
                    for (tx = 0; tx < MAP_CELL_GRID_SIZE; tx++) {
                        if (gpGame->m_map[tx][ty].m_triggerType
                                == (unsigned char)(objType | 0x80)
                            && abs(tx - x) + abs(ty - y) > (objType == 41 ? 1 : 3)) {
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
        case 48:
            if (pHero->NumArtifacts() == HERO_ARTIFACT_SLOT_COUNT) {
                NormalDialog("You cannot pick up this artifactId, you already have a full load!", 1, -1,
                             -1, -1, 0, -1, 0, -1);
                break;
            }
            switch (cell->m_objectMetadata) {
                case 1:
                    EventWindow(-1, 1, gArtifactDesc[cell->m_objectIndex], 7, cell->m_objectIndex, -1,
                                0, -1);
                giveArtifact:
                    GiveArtifact(pHero, cell->m_objectIndex);
                    erase = 1;
                    fizzleMode = 1;
                    break;
                case 2:
                    EventWindow(0x46, 1, "", -1, 0, -1, 0, -1);
                    if (CombatMonsterEvent(pHero, 0x18, 0x32, cell, x, y, 0, x, y) == 1) {
                        sprintf(gText, gEventText[74], gArtifactNames[cell->m_objectIndex]);
                        EventWindow(-1, 1, gText, 7, cell->m_objectIndex, -1, 0, -1);
                        goto giveArtifact;
                    }
                    break;
                case 3:
                    sprintf(gText, gEventText[71], gArtifactNames[cell->m_objectIndex]);
                    EventWindow(-1, 2, gText, 7, cell->m_objectIndex, -1, 0, -1);
                    if (gpWindowManager->m_dialogResult == NORMAL_DIALOG_CONFIRM) {
                        if (gpGame->m_players[pHero->m_owner].m_resources[6] >= 2000) {
                            gpGame->m_players[pHero->m_owner].m_resources[6] -= 2000;
                            goto giveArtifact;
                        } else {
                            EventWindow(0x49, 1, "", -1, 0, -1, 0, -1);
                        }
                    } else {
                        EventWindow(0x48, 1, "", -1, 0, -1, 0, -1);
                        erase = 1;
                    }
                    break;
            }
            pHero->CheckLevel();
            break;
        case 61:
            DemobilizeCurrHero();
            enemyHero = gpGame->GetHero(cell->m_objectMetadata);
            if (enemyHero->m_owner == giCurPlayer) {
                HeroSwap(pHero, enemyHero);
            } else {
                occupiedTown = 0;
                if (enemyHero->m_locationType == 0xa8) {
                    occupiedTown = gpGame->GetTown(enemyHero->m_occupiedTown);
                    occupiedTown->m_occupyingHeroId = enemyHero->m_id;
                }
                res = DoCombat(x, y, pHero, &pHero->m_army, occupiedTown,
                                            enemyHero, &enemyHero->m_army, x, y, -1, 1);
                if (res == 1 && occupiedTown)
                    gpGame->ClaimTown(occupiedTown->m_id, giCurPlayer);
            }
            break;
        case 2:
            gpSearchArray->FindNearestObject(pHero->m_x, pHero->m_y, pHero->m_direction,
                                             -1, 0xa8);
            if (GetCell(gpSearchArray->m_specialTargetX, gpSearchArray->m_specialTargetY)->m_triggerType
                == 0xa8) {
                sprintf(gText, gEventText[1],
                        GetTownName(gpGame->GetTownId(gpSearchArray->m_specialTargetX,
                                                      gpSearchArray->m_specialTargetY)));
                EventWindow(-1, 1, gText, -1, 0, -1, 0, -1);
            }
            break;
        case 5:
            EventWindow(0xa, 2, "", -1, 0, -1, 0, -1);
            if (gpWindowManager->m_dialogResult == NORMAL_DIALOG_CANCEL)
                break;
            switch (cell->m_objectMetadata) {
                case 1:
                    EventWindow(4, 1, "", -1, 0, -1, 0, -1);
                    cell->m_objectMetadata = 1;
                    break;
                case 2:
                    GiveExperience(pHero, 1000, 0);
                    EventWindow(5, 1, "", 0xe, 1000, -1, 0, -1);
                    cell->m_objectMetadata = 1;
                    pHero->CheckLevel();
                    break;
                case 3:
                    if (pHero->NumArtifacts() == HERO_ARTIFACT_SLOT_COUNT)
                        goto goldReward;
                    if (gpGame->GetRandomArtifactId() == -1)
                        goto goldReward;
                    GiveExperience(pHero, 1000, 0);
                    artifactId = GiveRandomArtifact(pHero);
                    EventWindow(6, 1, "", 7, artifactId, 0xe, 1000, -1);
                    cell->m_objectMetadata = 1;
                    pHero->CheckLevel();
                    break;
                case 4:
                goldReward:
                    EventWindow(7, 1, "", 6, 2500, 0xe, 1000, -1);
                    GiveExperience(pHero, 1000, 0);
                    GiveResource(pHero, 6, 2500);
                    cell->m_objectMetadata = 1;
                    pHero->CheckLevel();
                    break;
                case 5:
                    EventWindow(8, 2, "", -1, 0, -1, 0, -1);
                    if (gpWindowManager->m_dialogResult == NORMAL_DIALOG_CONFIRM) {
                        if (gpGame->m_players[pHero->m_owner].m_resources[6] < 2500) {
                            EventWindow(9, 1, "", -1, 0, -1, 0, -1);
                            HeroLoses(pHero);
                        } else {
                            gpGame->m_players[pHero->m_owner].m_resources[6] -= 2500;
                        }
                    } else {
                        HeroLoses(pHero);
                    }
                    break;
            }
            break;
        case 12:
            EventWindow(0x14, 2, "", -1, 0, -1, 0, -1);
            if (gpWindowManager->m_dialogResult == NORMAL_DIALOG_CONFIRM) {
                switch (cell->m_objectMetadata) {
                    case 1:
                        EventWindow(0x15, 1, "", 0xd, 0, -1, 0, -1);
                        if (!(pHero->m_eventFlags & HERO_EVENT_GRAVEYARD)) {
                            pHero->m_eventFlags |= HERO_EVENT_GRAVEYARD;
                            pHero->m_morale--;
                        }
                        break;
                    default:
                        if (GhostEvent(pHero, cell, 0x16, x, y))
                            cell->m_objectMetadata = 1;
                }
            }
            break;
        case 35:
            EventWindow(0x3a, 2, "", -1, 0, -1, 0, -1);
            if (gpWindowManager->m_dialogResult == NORMAL_DIALOG_CONFIRM) {
                switch (cell->m_objectMetadata) {
                    case 1:
                        EventWindow(0x3b, 1, "", 0xd, 0, -1, 0, -1);
                        if (!(pHero->m_eventFlags & HERO_EVENT_SHIPWRECK)) {
                            pHero->m_eventFlags |= HERO_EVENT_SHIPWRECK;
                            pHero->m_morale--;
                        }
                        break;
                    default:
                        prevCell = GetCell(x - normalDirTable[pHero->m_direction].x,
                                            y - normalDirTable[pHero->m_direction].y);
                        if (GhostEvent(pHero, prevCell, 0x3c, x, y))
                            cell->m_objectMetadata = 1;
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

// Buka advManager::EraseObj reduced to HoMM1's single-cell layers: the cell
// falls back to the trigger kept in the low seven bits of byte 7 and borrows
// the metadata of a neighbouring cell with that trigger.
VA(0x0045fcfa, 0x1d7)
void advManager::EraseObj(class mapCell* cell, int x, int y) {
    signed char erased = 0;
    int i;
    int j;

    erased = 1;
    cell->m_triggerType = 0;
    cell->m_objectIndex = 0xff;
    if ((cell->m_unknown07 & 0x7f) > 0 && (cell->m_unknown07 & 0x7f) < 0x7f) {
        cell->m_triggerType = cell->m_unknown07 & 0x7f;
        cell->m_unknown07 = cell->m_unknown07 - cell->m_triggerType;
        for (i = x - 1; i <= x + 1; i++) {
            for (j = y - 1; j <= y + 1; j++) {
                if (i >= 0 && i < MAP_CELL_GRID_SIZE && j >= 0 && j < MAP_CELL_GRID_SIZE
                    && gpGame->m_map[i][j].m_triggerType == cell->m_triggerType)
                    cell->m_objectMetadata = gpGame->m_map[i][j].m_objectMetadata;
            }
        }
        gpGame->SettleOverlay(x, y);
    }
    if (gpGame->m_mapSounds[x][y] != -1) {
        gpGame->m_mapSounds[x][y] = -1;
        if (bShowIt)
            SetEnvironmentOrigin(m_mapOriginX + 7, m_mapOriginY + 7, 1);
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
        defender = townRec->m_occupyingHeroId == -1 ? 0 : gpGame->GetHero(townRec->m_occupyingHeroId);
        result = DoCombat(x, y, curHero, &curHero->m_army, townRec, defender, &townRec->m_army, x, y,
                          -1, 1);
        if (result == 1)
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
    int musicTrack = -1;

    switch (eventType) {
    case 13:
    case 14:
    case 15:
    case 16:
    case 17:
        musicTrack = 0x16;
        break;
    case 63:
        musicTrack = 0x2e;
        break;
    case 23:
        musicTrack = 0x1a;
        break;
    case 34:
        musicTrack = 0x1b;
        break;
    case 48:
        if (eventData == 1)
            musicTrack = 0x1c;
        break;
    case 4:
    case 6:
    case 8:
    case 24:
    case 45:
        musicTrack = 0x1c;
        break;
    case 1:
    case 25:
    case 32:
        musicTrack = 0x17;
        break;
    case 3:
    case 28:
        musicTrack = 0x14;
        break;
    case 5:
        musicTrack = 7;
        break;
    case 7:
        musicTrack = 8;
        break;
    case 9:
        musicTrack = 0x18;
        break;
    case 10:
        musicTrack = 9;
        break;
    case 11:
        musicTrack = 0xa;
        break;
    case 12:
        musicTrack = 0xb;
        break;
    case 22:
        musicTrack = 0xc;
        break;
    case 27:
        musicTrack = 0x15;
        break;
    case 36:
        musicTrack = 0xe;
        break;
    case 39:
        musicTrack = 0xf;
        break;
    case 41:
        musicTrack = 0x10;
        break;
    case 42:
        musicTrack = 0x11;
        break;
    case 44:
        musicTrack = 0x19;
        break;
    default:
        musicTrack = -1;
        break;
    }
    if (musicTrack != -1) {
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
void advManager::EventWindow(short eventId, int buttons, char* text, int type1, int value1,
                             int type2, int value2, int type3) {
    int unusedValue1;
    int unusedValue7;
    int finished;
    int unusedValue8;
    int unusedValue9;
    int unusedValue11;
    int unusedValue12;
    char eventText[500];
    short unusedStyle;

    finished = 0;
    GrabScreen();
    unusedStyle = 1;
    if (eventId >= 0 && eventId < 76)
        sprintf(eventText, gEventText[eventId]);
    else if (eventId == -1)
        sprintf(eventText, text);
    else
        sprintf(eventText, "Event ID %d", eventId);
    NormalDialog(eventText, buttons, 0x61, -1, type1, value1, type2, value2, type3);
}

VA(0x0046047e, 0xa9)
short advManager::GiveArtifact(class hero* eventHero, signed char artifact) {
    short slot;

    for (slot = 0; slot < HERO_ARTIFACT_SLOT_COUNT; slot++) {
        if (eventHero->m_artifacts[slot] == -1)
            break;
    }
    if (slot == HERO_ARTIFACT_SLOT_COUNT)
        return -1;
    eventHero->m_artifacts[slot] = artifact;
    gpGame->m_randomArtifacts[artifact] = eventHero->m_id;
    GiveTakeArtifactStat(eventHero, artifact, 0);
    return slot;
}

// donor PoL RVA 0x000b00e9; preferred Buka symbol ?GiveRandomArtifact@advManager@@QAEHPAVhero@@@Z
// donor Buka TU SOURCE/EVENTS; HoMM1 owner inferred from contiguous order
// evidence: graph:3;base=0.483471;margin=0.618508;shape=0.306;size=0.821;calls=1.000;alternate=pol20:int advManager::GiveRandomArtifact(class hero *)@0x000b00e9
VA(0x00460527, 0x5f)
int advManager::GiveRandomArtifact(class hero* eventHero) {
    signed char artifact;

    artifact = gpGame->GetRandomArtifactId();
    if (artifact == -1)
        GiveResource(eventHero, 6, 1000);
    else
        GiveArtifact(eventHero, artifact);
    return artifact;
}

// donor PoL RVA 0x000b0147; preferred Buka symbol ?GiveExperience@advManager@@QAEHPAVhero@@HH@Z
// donor Buka TU SOURCE/EVENTS; HoMM1 owner inferred from contiguous order
// evidence: graph:4;base=0.329448;margin=0.686602;shape=0.229;size=0.551;calls=0.600;alternate=pol20:int advManager::GiveExperience(class hero *, int, int)@0x000b0147
VA(0x00460586, 0xb0)
int advManager::GiveExperience(class hero* eventHero, int experience, signed char checkLevel) {
    int prevLevel;
    int unusedValue1;
    int unusedValue2;
    int newLevel;
    int levelGap;

    prevLevel = eventHero->GetLevel(eventHero->m_experience);
    eventHero->m_level = prevLevel;
    eventHero->m_experience += experience;
    ProcessAssert(experience >= 0, "D:\\Heroes\\Source\\EVENTS.CPP", gEventsAssertLine + 8);
    ProcessAssert(eventHero->m_experience >= 0, "D:\\Heroes\\Source\\EVENTS.CPP", gEventsAssertLine + 9);
    newLevel = eventHero->GetLevel(eventHero->m_experience);
    if (checkLevel)
        eventHero->CheckLevel();
    return newLevel - prevLevel;
}

VA(0x00460636, 0x5a)
void advManager::GiveResource(class hero* eventHero, signed char resource, short amount) {
    if (resource >= 0 && resource <= 6)
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
signed char advManager::GhostEvent(class hero* eventHero, class mapCell* cell, int textId, int x,
                                   int y) {
    int artifact;

    switch (cell->m_objectMetadata) {
    case 2:
        if (CombatMonsterEvent(eventHero, 26, 10, cell, x, y, 0, x, y) == 1) {
            sprintf(gText, "%s", gEventText[textId]);
            EventWindow(-1, 1, gText, 6, 1000, -1, 0, -1);
            GiveResource(eventHero, 6, 1000);
            eventHero->CheckLevel();
            return 1;
        }
        break;
    case 3:
        if (CombatMonsterEvent(eventHero, 26, 15, cell, x, y, 0, x, y) == 1) {
            sprintf(gText, "%s", gEventText[textId]);
            EventWindow(-1, 1, gText, 6, 2000, -1, 0, -1);
            GiveResource(eventHero, 6, 2000);
            eventHero->CheckLevel();
            return 1;
        }
        break;
    case 4:
        if (CombatMonsterEvent(eventHero, 26, 25, cell, x, y, 0, x, y) == 1) {
            sprintf(gText, "%s", gEventText[textId]);
            EventWindow(-1, 1, gText, 6, 5000, -1, 0, -1);
            GiveResource(eventHero, 6, 5000);
            eventHero->CheckLevel();
            return 1;
        }
        break;
    default:
        if (CombatMonsterEvent(eventHero, 26, 50, cell, x, y, 0, x, y) == 1) {
            artifact = GiveRandomArtifact(eventHero);
            sprintf(gText, "%s", gEventText[textId]);
            if (artifact != -1)
                EventWindow(-1, 1, gText, 6, 2000, 7, artifact, -1);
            else
                EventWindow(-1, 1, gText, 6, 2000, -1, 0, -1);
            GiveResource(eventHero, 6, 2000);
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

    houseIndex = (cell->m_triggerType & 0x7f) - 13;
    if (!cell->m_objectMetadata) {
        EventWindow(houseIndex * 3 + 25, 1, "", -1, 0, -1, 0, -1);
    } else {
        signed char creatures[5] = {6, 0, 1, 13, 0};

        EventWindow(houseIndex * 3 + 23, 2, "", -1, 0, -1, 0, -1);
        if (gpWindowManager->m_dialogResult == NORMAL_DIALOG_CONFIRM) {
            if (eventHero->m_army.CanJoin(creatures[houseIndex])) {
                eventHero->m_army.Add(creatures[houseIndex], cell->m_objectMetadata, -1);
                cell->m_objectMetadata = 0;
            } else {
                EventWindow(houseIndex * 3 + 24, 1, "", -1, 0, -1, 0, -1);
            }
        }
    }
}

VA(0x00460b7a, 0x200)
signed char advManager::CombatMonsterEvent(class hero* eventHero, signed char monsterType,
                                           short count, class mapCell* cell, int x, int y,
                                           signed char heroDefends, int fromX, int fromY) {
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
        m_lastQuickViewX = -1;
    }
    memset(gpMonsterGroup->m_creatureTypes, -1, 5);
    memset(gpMonsterGroup->m_creatureCounts, 0, 10);
    if (count / 5 > 0) {
        for (i = 0; i < ARMY_GROUP_SLOT_COUNT; i++) {
            gpMonsterGroup->m_creatureTypes[i] = monsterType;
            gpMonsterGroup->m_creatureCounts[i] = count / 5;
        }
    }
    for (i = count % 5 - 1; i >= 0; i--) {
        gpMonsterGroup->m_creatureTypes[i] = monsterType;
        gpMonsterGroup->m_creatureCounts[i]++;
    }
    if (heroDefends)
        res = DoCombat(fromX, fromY, 0, gpMonsterGroup, 0, eventHero, &eventHero->m_army, x, y,
                          -1, 1);
    else
        res = DoCombat(fromX, fromY, eventHero, &eventHero->m_army, 0, 0, gpMonsterGroup, x, y,
                          -1, 1);
    MobilizeCurrHero(0);
    return res;
}

// Buka's free GiveTakeArtifactStat; HoMM1 keeps per-artifact primary-stat
// bonuses here and is called through gpAdvManager.
VA(0x00460d7a, 0x243)
void advManager::GiveTakeArtifactStat(class hero* targetHero, signed char artifact, signed char take) {
    signed char stat = -1;
    signed char amount = 0;
    int i;

    switch (artifact) {
    case 0:
        stat = 3;
        amount = 12;
        break;
    case 1:
        stat = 0;
        amount = 12;
        break;
    case 2:
        stat = 1;
        amount = 12;
        break;
    case 3:
        stat = 2;
        amount = 12;
        break;
    case 4:
        stat = 2;
        amount = 4;
        break;
    case 5:
    case 6:
        stat = 2;
        amount = 2;
        break;
    case 7:
        stat = 2;
        amount = 3;
        break;
    case 13:
    case 16:
        stat = 0;
        amount = 1;
        break;
    case 14:
    case 15:
        stat = 1;
        amount = 1;
        break;
    case 17:
        stat = 4;
        amount = 3;
        break;
    case 18:
        stat = 1;
        amount = 2;
        break;
    case 19:
        stat = 0;
        amount = 3;
        break;
    case 20:
        stat = 0;
        amount = 2;
        break;
    case 21:
        stat = 1;
        amount = 3;
        break;
    case 22:
        stat = 3;
        amount = 2;
        break;
    case 23:
        stat = 3;
        amount = 3;
        break;
    case 24:
        stat = 3;
        amount = 4;
        break;
    case 25:
        stat = 3;
        amount = 5;
        break;
    case 8:
    case 9:
    case 10:
    case 11:
    case 12:
        break;
    }
    if (take == 1)
        amount = -amount;
    if (stat != -1) {
        targetHero->m_primaryStats[stat] += amount;
        if (amount < 0 && stat == 3) {
            for (i = 0; i < HERO_SPELL_SLOT_COUNT; i++) {
                if (targetHero->m_spellCharges[i] > targetHero->m_primaryStats[3])
                    targetHero->m_spellCharges[i] = targetHero->m_primaryStats[3];
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
        if (destHero->m_artifacts[i] == -1) {
            for (j = 0; j < HERO_ARTIFACT_SLOT_COUNT; j++) {
                if (sourceHero->m_artifacts[j] != -1
                    && sourceHero->m_artifacts[j] != ARTIFACT_MAGIC_BOOK) {
                    if (sourceHero->m_artifacts[j] <= 3) {
                        if (gbThisNetHumanPlayer[sourceHero->m_owner]
                            || gbThisNetHumanPlayer[destHero->m_owner]) {
                            sprintf(gText,
                                    "As you reach for the %s, it mysteriously disappears.",
                                    gArtifactNames[sourceHero->m_artifacts[j]]);
                            NormalDialog(gText, 1, -1, -1, 7, sourceHero->m_artifacts[j], -1, 0, -1);
                        }
                        gpGame->m_randomArtifacts[sourceHero->m_artifacts[j]] = -1;
                    } else {
                        GiveTakeArtifactStat(destHero, sourceHero->m_artifacts[j], 0);
                        destHero->m_artifacts[i] = sourceHero->m_artifacts[j];
                        gpGame->m_randomArtifacts[sourceHero->m_artifacts[j]] = destHero->m_id;
                    }
                    GiveTakeArtifactStat(sourceHero, sourceHero->m_artifacts[j], 1);
                    sourceHero->m_artifacts[j] = -1;
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
    FizzleCenter(0);
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
    if (Random(1, 3) != 1)
        return;
    lowestValue = 99999999;
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
            eventHero->m_army.m_creatureTypes[weakest] = -1;
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
    case 0:
        sprintf(gText, "killfade.82M");
        break;
    case 1:
        sprintf(gText, "pickup%02d.82M", Random(1, 5));
        break;
    default:
        return;
    }
    fizzleSample = gNullSample;
    fizzleSample = LoadPlaySample(gText);
    gpWindowManager->SaveFizzleSource(180, 172, 120, 120);
    CompleteDraw(0);
    gpWindowManager->FizzleForward(180, 172, 120, 120, 65);
    WaitEndSample(fizzleSample, -1);
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
    int cost[7];
    int victory;
    signed char adjacentMonster;
    hero* enemyHero;
    float heroLosses;
    float theirLosses;

    theCastle = 0;
    eventType = cell->m_triggerType & 0x7f;
    erase = 0;
    handled = 0;
    savedPlayer = giCurPlayer;
    origPlayerData = gpCurPlayer;
    --eventHero->m_remainingMobility;
    mapVisited[x][y] |= giCurPlayerBit;
    switch (eventType) {
        case 31:
            if (eventHero->m_eventFlags & HERO_EVENT_EMBARKED) {
                eventHero->m_eventFlags &= ~HERO_EVENT_EMBARKED;
                eventHero->m_remainingMobility = 0;
                eventHero->m_direction = m_cursorDirection;
                m_cursorType = eventHero->m_unknown1c;
                m_cursorFrame = GetCursorBaseFrame(m_cursorDirection);
                m_cursorActive = 1;
                CheckAdjacentMon(&adjacentMonster);
            }
            break;
        case 62:
            ship = &gpGame->m_boats[cell->m_objectMetadata];
            gpGame->RestoreCell(-1, -1, ship->savedTriggerType, ship->savedEventData, cell, 3);
            eventHero->m_eventFlags |= HERO_EVENT_EMBARKED;
            eventHero->m_remainingMobility = 0;
            ship->heroId = eventHero->m_id;
            ship->owner = eventHero->m_owner;
            m_cursorType = 4;
            m_cursorDirection = ship->direction;
            m_cursorFrame = GetCursorBaseFrame(m_cursorDirection);
            m_cursorActive = 1;
            break;
        case 1:
        case 25:
        case 32:
            if (gpGame->m_mineOwners[cell->m_objectMetadata] == giCurPlayer)
                break;
            gpGame->ClaimMine(cell->m_objectMetadata, giCurPlayer);
            break;
        case 23:
            if (gpGame->m_mineOwners[1] == giCurPlayer)
                break;
            gpGame->ClaimMine(1, giCurPlayer);
            break;
        case 22:
            if (gpGame->m_mineOwners[0] == giCurPlayer)
                break;
            for (counter = 0; counter < 5; counter++) {
                gpMonsterGroup->m_creatureTypes[counter] = 0x17;
                gpMonsterGroup->m_creatureCounts[counter] = 1;
            }
            gpPhilAI->ChooseEvaluateBattle(&eventHero->m_army, eventHero, gpMonsterGroup, 0, 0, 0, 500,
                                           win, strength);
            if (win) {
                counter = 5;
                victory = gpPhilAI->CombatMonsterEvent(eventHero, 0x17, &counter, cell);
                if (victory)
                    gpGame->ClaimMine(0, giCurPlayer);
            }
            break;
        case 6:
            if (gpPhilAI->ChooseGoldOrExperience(eventHero,
                                                 cell->m_objectMetadata * 500,
                                                 (cell->m_objectMetadata - 1) * 500))
                GiveResource(eventHero, 6, cell->m_objectMetadata * 500);
            else
                GiveExperience(eventHero, (cell->m_objectMetadata - 1) * 500, 1);
            erase = 1;
            break;
        case 3:
            if (!(eventHero->m_eventFlags & HERO_EVENT_BUOY)) {
                eventHero->m_eventFlags |= HERO_EVENT_BUOY;
                eventHero->m_morale++;
            }
            break;
        case 7:
            if (!(eventHero->m_eventFlags & HERO_EVENT_FAERIE_RING)) {
                eventHero->m_eventFlags |= HERO_EVENT_FAERIE_RING;
                eventHero->m_luck++;
            }
            break;
        case 9:
            if (!(eventHero->m_eventFlags & HERO_EVENT_FOUNTAIN)) {
                eventHero->m_eventFlags |= HERO_EVENT_FOUNTAIN;
                eventHero->m_luck++;
            }
            break;
        case 28:
            if (!(eventHero->m_eventFlags & HERO_EVENT_OASIS)) {
                eventHero->m_eventFlags |= HERO_EVENT_OASIS;
                eventHero->m_morale++;
            }
            break;
        case 36:
            if (!(eventHero->m_eventFlags & HERO_EVENT_TEMPLE)) {
                eventHero->m_eventFlags |= HERO_EVENT_TEMPLE;
                eventHero->m_morale += 2;
            }
            break;
        case 4:
            switch (cell->m_objectMetadata) {
                case 1:
                    break;
                case 2:
                    GiveRandomArtifact(eventHero);
                    cell->m_objectMetadata = 1;
                    break;
            }
            break;
        case 8:
            GiveResource(eventHero, 6, (cell->m_objectMetadata >> 4) * 100);
            GiveResource(eventHero, cell->m_objectMetadata & 0xf,
                         cell->m_objectMetadata >> 4);
            erase = 1;
            gpGame->m_mapSounds[m_mapOriginX + 7][m_mapOriginY + 7] = -1;
            break;
        case 10:
            if (!(eventHero->m_visitedSites & (1 << cell->m_objectMetadata))) {
                GiveExperience(eventHero, 1000, 1);
                eventHero->m_visitedSites |= 1 << cell->m_objectMetadata;
            }
            break;
        case 24:
            if (cell->m_objectMetadata) {
                GiveResource(eventHero, 6, cell->m_objectMetadata * 500);
                cell->m_objectMetadata = 0;
            }
            break;
        case 29:
            resType = cell->m_objectIndex - 0x3d;
            GiveResource(eventHero, resType,
                         resType == 6 ? cell->m_objectMetadata * 100
                                           : cell->m_objectMetadata);
            erase = 1;
            break;
        case 45:
            if (cell->m_objectMetadata != 99) {
                GiveResource(eventHero, cell->m_objectMetadata, 2);
                cell->m_objectMetadata = 99;
            }
            break;
        case 11:
            troopType = 0x1b;
            available = 0;
            goto recruit;
        case 42:
            troopType = 0x18;
            available = 0;
            goto recruit;
        case 39:
            troopType = 0x19;
            available = 0;
            goto recruit;
        case 13:
            troopType = 6;
            available = 1;
            goto recruit;
        case 14:
            troopType = 0;
            available = 1;
            goto recruit;
        case 15:
            troopType = 1;
            available = 1;
            goto recruit;
        case 16:
            troopType = 0xd;
            available = 1;
            goto recruit;
        case 17:
            troopType = 0;
            available = 1;
            goto recruit;
        recruit:
            if (cell->m_objectMetadata) {
                gpPhilAI->EvaluateOneTimeCreaturePurchase(
                    eventHero, troopType, cell->m_objectMetadata, available,
                    numHired, purchaseValue, bestSlot);
                if (numHired > 0) {
                    gpGame->GiveArmy(&eventHero->m_army, troopType, numHired,
                                     bestSlot);
                    cell->m_objectMetadata = cell->m_objectMetadata - numHired;
                    if (!available) {
                        GetMonsterCost(troopType, cost);
                        for (counter = 0; counter < 7; counter++)
                            gpCurPlayer->m_resources[counter] -= -(-(cost[counter] * numHired));
                    }
                }
            }
            if (!cell->m_objectMetadata && eventType == 11)
                erase = 1;
            break;
        case 26:
            ComputerMonsterInteract(cell, eventHero, &erase);
            break;
        case 27:
            if (!(gpGame->m_obeliskVisitors[cell->m_objectMetadata - 1] & giCurPlayerBit)) {
                gpGame->VisitObelisk(eventHero->m_owner);
                gpGame->m_obeliskVisitors[cell->m_objectMetadata - 1] |= giCurPlayerBit;
            }
            break;
        case 33:
            break;
        case 34:
            if (eventHero->HasArtifact(ARTIFACT_MAGIC_BOOK))
                eventHero->AddSpell(cell->m_objectMetadata - 1,
                                    eventHero->m_primaryStats[3], 0);
            break;
        case 40:
            gpPhilAI->TownEvent(cell, eventHero, x, y);
            break;
        case 44:
            DoWhirlpool(eventHero);
        case 41:
            teleportCount = 0;
            for (ty = 0; ty < MAP_CELL_GRID_SIZE; ty++) {
                for (tx = 0; tx < MAP_CELL_GRID_SIZE; tx++) {
                    if (gpGame->m_map[tx][ty].m_triggerType
                            == (unsigned char)(eventType | 0x80)
                        && abs(tx - x) + abs(ty - y) > (eventType == 41 ? 1 : 3))
                        teleportCount++;
                }
            }
            if (teleportCount >= 1) {
                if (teleportCount > 1)
                    teleportCount = Random(1, teleportCount);
                for (ty = 0; ty < MAP_CELL_GRID_SIZE; ty++) {
                    for (tx = 0; tx < MAP_CELL_GRID_SIZE; tx++) {
                        if (gpGame->m_map[tx][ty].m_triggerType
                                == (unsigned char)(eventType | 0x80)
                            && abs(tx - x) + abs(ty - y) > (eventType == 41 ? 1 : 3)) {
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
        case 48:
            switch (cell->m_objectMetadata) {
                case 1:
                giveArtifact:
                    GiveArtifact(eventHero, cell->m_objectIndex);
                    erase = 1;
                    break;
                case 2:
                    counter = 50;
                    if (gpPhilAI->CombatMonsterEvent(eventHero, 0x18, &counter, cell))
                        goto giveArtifact;
                    break;
                case 3:
                    if (gpPhilAI->ChooseToBuyArtifact(eventHero, cell->m_objectIndex, 2000)) {
                        gpGame->m_players[eventHero->m_owner].m_resources[6] -= 2000;
                        goto giveArtifact;
                    } else {
                        erase = 1;
                    }
                    break;
            }
            break;
        case 61:
            enemyHero = gpGame->GetHero(cell->m_objectMetadata);
            oldShowIt = bShowIt;
            if (enemyHero->m_owner == giCurPlayer)
                return;
            if (enemyHero->m_locationType == 0xa8)
                theCastle = gpGame->GetTown(enemyHero->m_occupiedTown);
            if (!gbHumanPlayer[enemyHero->m_owner]) {
                battleResult = gpPhilAI->QuickCombat(&eventHero->m_army, eventHero, &enemyHero->m_army,
                                                     enemyHero, 0, 0, heroLosses, theirLosses);
                if (battleResult && theCastle)
                    battleResult = gpPhilAI->QuickCombat(&eventHero->m_army, eventHero,
                                                         &theCastle->m_army, 0, 1,
                                                         theCastle->m_id, heroLosses,
                                                         theirLosses);
            } else {
                if (theCastle)
                    theCastle->m_occupyingHeroId = enemyHero->m_id;
                res = DoCombat(x, y, eventHero, &eventHero->m_army, theCastle,
                                            enemyHero, &enemyHero->m_army, x, y, -1, 1);
                if (res == 1 && theCastle)
                    gpGame->ClaimTown(theCastle->m_id, giCurPlayer);
            }
            CompleteDraw(0);
            break;
        case 2:
            break;
        case 5:
            switch (cell->m_objectMetadata) {
                case 1:
                    break;
                case 2:
                    GiveExperience(eventHero, 1000, 1);
                    break;
                case 3:
                    GiveExperience(eventHero, 1000, 1);
                    GiveRandomArtifact(eventHero);
                    break;
                case 4:
                    GiveExperience(eventHero, 1000, 1);
                    GiveResource(eventHero, 6, 2500);
                    break;
                case 5:
                    if (gpGame->m_players[eventHero->m_owner].m_resources[6] >= 2500) {
                        if (gpPhilAI->ChooseToPayRansomOnHero(eventHero, 2500))
                            gpGame->m_players[eventHero->m_owner].m_resources[6] += -2500;
                        else
                            HeroLoses(eventHero);
                    } else {
                        HeroLoses(eventHero);
                    }
                    break;
            }
            cell->m_objectMetadata = 1;
            break;
        case 12:
        case 35:
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
void advManager::PlayerMonsterInteract(class mapCell* cell, class mapCell* combatCell,
                                       class hero* eventHero, signed char* handled, int x, int y,
                                       signed char unused, int combatX, int combatY) {
    int result;

    unused = 0;
    if (cell->m_objectMetadata & 0x80) {
        if (gpPhilAI->FightValueOfStack(&eventHero->m_army, eventHero, 0, 0, 0)
            > gMonsterDatabase[cell->m_objectIndex].fightValue
                  * (cell->m_objectMetadata & 0x7f) * 1.75) {
            if (eventHero->m_army.CanJoin(cell->m_objectIndex)) {
                sprintf(gText, gEventText[48], gArmyNamesPlural[cell->m_objectIndex]);
                EventWindow(-1, 2, gText, -1, 0, -1, 0, -1);
                if (gpWindowManager->m_dialogResult == NORMAL_DIALOG_CONFIRM) {
                    eventHero->m_army.Add(cell->m_objectIndex,
                                          cell->m_objectMetadata & 0x7f, -1);
                    *handled = 1;
                    return;
                } else {
                    EventWindow(49, 1, "", -1, 0, -1, 0, -1);
                }
            }
        }
    }
    result = CombatMonsterEvent(eventHero, cell->m_objectIndex,
                                cell->m_objectMetadata & 0x7f, combatCell, x, y,
                                unused, combatX, combatY);
    if (result == 1 || result == -1)
        *handled = 1;
}

// HoMM1's computer heroes absorb a willing stack (bit 7) they outmatch by
// 7:4, otherwise fight it through philAI's quick combat.
VA(0x0046275f, 0x152)
void advManager::ComputerMonsterInteract(class mapCell* cell, class hero* eventHero,
                                         signed char* handled) {
    int numToBuy;
    int purchaseValue;
    int bestSlot;
    int result;
    int creatureCount;

    if (cell->m_objectMetadata & 0x80
        && gpPhilAI->FightValueOfStack(&eventHero->m_army, eventHero, 0, 0, 0)
               > gMonsterDatabase[cell->m_objectIndex].fightValue
                     * (cell->m_objectMetadata & 0x7f) * 1.75) {
        gpPhilAI->EvaluateOneTimeCreaturePurchase(
            eventHero, cell->m_objectIndex, cell->m_objectMetadata & 0x7f, 1,
            numToBuy, purchaseValue, bestSlot);
        if (numToBuy > 0) {
            gpGame->GiveArmy(&eventHero->m_army, cell->m_objectIndex,
                             cell->m_objectMetadata & 0x7f, bestSlot);
            *handled = 1;
        }
    } else {
        creatureCount = cell->m_objectMetadata & 0x7f;
        result = gpPhilAI->CombatMonsterEvent(eventHero, cell->m_objectIndex, &creatureCount, cell);
        cell->m_objectMetadata = (cell->m_objectMetadata & 0x80) + creatureCount;
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

    attackingHero = 0;
    attArmy = 0;
    siegeTown = 0;
    defendingHero = 0;
    defendArmy = 0;
    ReceiveHeroTownData(packet, &opponent, &cellX, &cellY, &attackingHero, &attArmy, &siegeTown,
                        &defendingHero, &defendArmy, &srcX, &srcY, &seed, &result, &gbRetreatWin,
                        &gbCombatSurrender);
    side = attackingHero->m_owner;
    result = DoCombat(cellX, cellY, attackingHero, attArmy, siegeTown, defendingHero, defendArmy,
                      srcX, srcY, seed, 0);
    if (!gbHumanPlayer[side])
        SendHeroTownData(cellX, cellY, attackingHero, attArmy, siegeTown, defendingHero,
                         defendArmy, srcX, srcY, seed, opponent, result, gbRetreatWin,
                         gbCombatSurrender);
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

// Declared at first use: this C1 symbol order gives DoAIEvent retail's operand
// order (docs/patterns/vc4-operand-sort-key-is-the-symbol-handle.md).
extern char* gColorNames[];

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
int advManager::DoCombat(int x, int y, class hero* firstHero, class armyGroup* firstArmy,
                         class town* combatTown, class hero* secondHero,
                         class armyGroup* secondArmy, int setupCombatX, int setupCombatY,
                         int randomSeed, signed char processLosses) {
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
        defendPlayer = -1;
    if (randomSeed == -1)
        randomSeed = Random(1, 1000);
    DemobilizeCurrHero();
    savedPlayer = giCurPlayer;
    savedShowIt = bShowIt;

    if (attackPlayer >= 0 && defendPlayer >= 0 && gbHumanPlayer[defendPlayer]) {
        if (!gbThisNetHumanPlayer[defendPlayer]) {
            SendHeroTownData(x, y, firstHero, firstArmy, combatTown, secondHero, secondArmy,
                             setupCombatX, setupCombatY, randomSeed, defendPlayer, 0, 0, 0);
            if (!gbHumanPlayer[attackPlayer]) {
                while (1) {
                    PollSound();
                    FillBitmapArea(gpWindowManager->m_screen, 30, 30, 4, 4, 0);
                    receivedPacket = CheckHandleNet();
                    if (receivedPacket) {
                        switch (((combatRemoteMessage*)receivedPacket)->command) {
                            case 0x15:
                                ReceiveHeroTownData(receivedPacket, &sender, &x, &y,
                                                    &hero1Net, &army1Net,
                                                    &townNet, &hero2Net,
                                                    &army2Net, &setupCombatX,
                                                    &setupCombatY, &randomSeed, &combatResult,
                                                    &gbRetreatWin, &gbCombatSurrender);
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
            sprintf(gText, "%s player\'s %s is under attack!",
                    gColorNames[gpGame->m_players[defendPlayer].m_unknown11],
                    combatTown ? "Town" : "Hero");
            gText[0] -= 32;
            gpGame->WaitForPlayer(gText, defendPlayer);
        }
    }

    bShowIt = 1;
    if (giEventMusicVolume != -1)
        gConfig.musicVolume = giEventMusicVolume;
    giEventMusicVolume = -1;
    gpCombatManager->SetupCombat(x, y, firstHero, firstArmy, combatTown, secondHero, secondArmy,
                                 x, y, randomSeed);
    if (giHighMemBuffer > 1450)
        gAdvDisposeLevel = 2;
    else if (giHighMemBuffer > 600)
        gAdvDisposeLevel = 1;
    gpExec->CallManager(gpCombatManager);
    gAdvDisposeLevel = 0;

combatFinished:
    if (firstHero)
        firstHero->CheckLevel();
    if (secondHero)
        secondHero->CheckLevel();
    if (processLosses) {
        switch (gpCombatManager->m_combatResult) {
            case 1:
                if (!gbRetreatWin)
                    TransferArtifacts(secondHero, firstHero);
                HeroLoses(secondHero);
                break;
            case 0:
                if (!gbRetreatWin)
                    TransferArtifacts(firstHero, secondHero);
                HeroLoses(firstHero);
                break;
            case -1:
                HeroLoses(firstHero);
                HeroLoses(secondHero);
                break;
            case 3:
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
void advManager::SendHeroTownData(int x, int y, class hero* firstHero, class armyGroup* firstArmy,
                                  class town* combatTown, class hero* secondHero,
                                  class armyGroup* secondArmy, int setupCombatX,
                                  int setupCombatY, int randomSeed, signed char remotePlayer,
                                  signed char combatResult, signed char retreatWin,
                                  signed char combatSurrender) {
    char* reply;
    int result;
    combatRemoteData* buf = 0;

    buf = (combatRemoteData*)malloc(0xff);
    reply = 0;
    buf->fragment = 0;
    buf->x = x;
    buf->y = y;
    buf->hasFirstHero = firstHero != 0;
    buf->hasTown = combatTown != 0;
    buf->hasSecondHero = secondHero != 0;
    buf->setupCombatX = setupCombatX;
    buf->setupCombatY = setupCombatY;
    buf->randomSeed = randomSeed;
    buf->combatResult = combatResult;
    buf->retreatWin = retreatWin;
    buf->combatSurrender = combatSurrender;
    buf->firstOwner = firstHero ? firstHero->m_owner : -1;
    buf->firstGold = firstHero ? gpGame->m_players[firstHero->m_owner].m_resources[6] : 0;
    buf->secondOwner = secondHero ? secondHero->m_owner : -1;
    buf->secondGold = secondHero ? gpGame->m_players[secondHero->m_owner].m_resources[6] : 0;
    memcpy(&buf->firstArmy, firstArmy, sizeof(armyGroup));
    memcpy(&buf->secondArmy, secondArmy, sizeof(armyGroup));
    if (combatTown)
        memcpy(&buf->combatTown, combatTown, sizeof(town));

    result = TransmitAndWait((char*)buf, remotePlayer, sizeof(combatRemoteData), 0x15, 0x16,
                             &reply);
    if (!result)
        ShutDown(0);

    if (firstHero) {
        ((combatRemoteHeroFragment*)buf)->fragment = 1;
        memcpy(((combatRemoteHeroFragment*)buf)->data, firstHero, sizeof(hero));
        result = TransmitRemoteData((char*)buf, remotePlayer, sizeof(combatRemoteHeroFragment),
                                    0x15, 1, 1, -1, 1);
        if (!result)
            ShutDown(0);
    }
    if (secondHero) {
        ((combatRemoteHeroFragment*)buf)->fragment = 2;
        memcpy(((combatRemoteHeroFragment*)buf)->data, secondHero, sizeof(hero));
        result = TransmitRemoteData((char*)buf, remotePlayer, sizeof(combatRemoteHeroFragment),
                                    0x15, 1, 1, -1, 1);
        if (!result)
            ShutDown(0);
    }
    free(buf);
}

// donor PoL RVA 0x000b67cd; preferred Buka symbol ?ReceiveHeroTownData@advManager@@QAEXPADPAH11PAPAVhero@@PAPAVarmyGroup@@PAPAVtown@@23111PAC55@Z
// donor Buka TU SOURCE/EVENTS; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.493176;margin=0.152223;shape=0.314;size=0.857;calls=0.909;alternate=pol20:void advManager::ReceiveHeroTownData(char *, int *, int *, int *, class hero * *, class armyGroup * *, class town * *, class hero * *, class armyGroup * *, int *, int *, int *, signed char *, signed char *, signed char *)@0x000b67cd
VA(0x004632e0, 0x34c)
void advManager::ReceiveHeroTownData(char* packet, int* remotePlayer, int* x, int* y,
                                     class hero** firstHero, class armyGroup** firstArmy,
                                     class town** combatTown, class hero** secondHero,
                                     class armyGroup** secondArmy, int* setupCombatX,
                                     int* setupCombatY, int* randomSeed, signed char* combatResult,
                                     signed char* retreatWin, signed char* combatSurrender) {
    signed char hasTown;
    int result;
    long lastPacketTime;
    signed char firstOwner;
    signed char defenderOwner;
    signed char bFirstHero;
    signed char hasSecondHero;

    *firstHero = 0;
    *firstArmy = 0;
    *combatTown = 0;
    *secondHero = 0;
    *secondArmy = 0;
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
        gpGame->m_players[firstOwner].m_resources[6] = ((combatRemoteMessage*)packet)->combat.firstGold;
    defenderOwner = ((combatRemoteMessage*)packet)->combat.secondOwner;
    if (defenderOwner > 0)
        gpGame->m_players[defenderOwner].m_resources[6] =
            ((combatRemoteMessage*)packet)->combat.secondGold;

    *firstArmy = (armyGroup*)malloc(sizeof(armyGroup));
    memcpy(*firstArmy, &((combatRemoteMessage*)packet)->combat.firstArmy, sizeof(armyGroup));
    *secondArmy = (armyGroup*)malloc(sizeof(armyGroup));
    memcpy(*secondArmy, &((combatRemoteMessage*)packet)->combat.secondArmy, sizeof(armyGroup));
    if (hasTown) {
        *combatTown = (town*)malloc(sizeof(town));
        memcpy(*combatTown, &((combatRemoteMessage*)packet)->combat.combatTown, sizeof(town));
    }

    result = TransmitRemoteData(0, *remotePlayer, 0, 0x16, 1, 1, -1, 1);
    if (!result)
        ShutDown(0);

    lastPacketTime = KBTickCount();
    while ((hasSecondHero && !*secondHero) || (bFirstHero && !*firstHero)) {
        PollSound();
        if (KBTickCount() > lastPacketTime + 20000) {
            NormalDialog("Error receiving data.  Keep trying??", 2, -1, -1, -1, 0, -1, 0, -1);
            if (gpWindowManager->m_dialogResult == NORMAL_DIALOG_CONFIRM)
                lastPacketTime = KBTickCount();
            else
                ShutDown("Game canceled.");
        }
        packet = GetRemoteData(1);
        if (packet && ((combatRemoteMessage*)packet)->type == 2
            && ((combatRemoteMessage*)packet)->command == 0x15) {
            lastPacketTime = KBTickCount();
            if (((heroRemoteMessage*)packet)->heroFragment.fragment == 1) {
                *firstHero = (hero*)malloc(sizeof(hero));
                memcpy(*firstHero, ((heroRemoteMessage*)packet)->heroFragment.data, sizeof(hero));
            }
            if (((heroRemoteMessage*)packet)->heroFragment.fragment == 2) {
                *secondHero = (hero*)malloc(sizeof(hero));
                memcpy(*secondHero, ((heroRemoteMessage*)packet)->heroFragment.data,
                       sizeof(hero));
            }
        }
    }
}

// EVENTS owns retail .data 0x004a0504-0x004a07bb and .bss 0x004ca904. Retail
// emits gEventsAssertLine (source-line base 1110) among GiveExperience's literals.
DATA(0x004a0504)
int giEventMusicVolume = -1;
DATA(0x004a06a0)
short gEventsAssertLine = 1110;
DATA(0x004ca904)
signed char gbEventMusicPlaying;

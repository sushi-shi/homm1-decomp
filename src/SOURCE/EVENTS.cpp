// Located from HoMM2 Buka 2.1; PoL 2.0 supplies the VC4 declaration.

#include <match.h>

#include <BASE/Misc.h>
#include <H1/All.h>
#include <H1/KB.h>
#include <SOURCE/kbwin.h>

#include <SOURCE/dialogTypes.h>

#include <stdio.h>
#include <string.h>

// EVENTS assertion records (file literals and line base), as in MOUSEMGR.
extern short gEventsAssertLine;
extern char gEventsAssertFile1[];
extern char gEventsAssertFile2[];
extern char* gEventText[];
extern signed char gbEventMusicPlaying;
extern char* gArtifactNames[];
extern SAMPLE2 gNullSample;
extern armyGroup* gpMonsterGroup;

// donor PoL RVA 0x000a8530; preferred Buka symbol ?DoEvent@advManager@@QAEXPAVmapCell@@HH@Z
// donor Buka TU SOURCE/EVENTS; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.400929;margin=0.083641;shape=0.269;size=0.325;calls=0.342;strings=%s %s|thiefwin.bin;alternate=pol20:void advManager::DoEvent(class mapCell *, int, int)@0x000a8530
VA(0x0045dde0, 0x1f1a)
void advManager::DoEvent(class mapCell *, int, int) {}

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
    ProcessAssert(experience >= 0, gEventsAssertFile1, gEventsAssertLine + 8);
    ProcessAssert(eventHero->m_experience >= 0, gEventsAssertFile2, gEventsAssertLine + 9);
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

    availableCount = (unsigned char)cell->m_objectMetadata;
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

    switch ((unsigned char)cell->m_objectMetadata) {
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
    if (!(unsigned char)cell->m_objectMetadata) {
        EventWindow(houseIndex * 3 + 25, 1, "", -1, 0, -1, 0, -1);
    } else {
        signed char creatures[5] = {6, 0, 1, 13, 0};

        EventWindow(houseIndex * 3 + 23, 2, "", -1, 0, -1, 0, -1);
        if (gpWindowManager->m_dialogResult == NORMAL_DIALOG_CONFIRM) {
            if (eventHero->m_army.CanJoin(creatures[houseIndex])) {
                eventHero->m_army.Add(creatures[houseIndex], (unsigned char)cell->m_objectMetadata, -1);
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
void advManager::DoAIEvent(class mapCell *, class hero *, int, int) {}

// donor PoL RVA 0x000b4fd5; preferred Buka symbol ?PlayerMonsterInteract@advManager@@QAEXPAVmapCell@@0PAVhero@@PAHHHHHH@Z
// donor Buka TU SOURCE/EVENTS; HoMM1 owner inferred from contiguous order
// evidence: graph:4;base=0.214069;margin=0.493239;shape=0.272;size=0.223;calls=0.212;alternate=pol20:void advManager::PlayerMonsterInteract(class mapCell *, class mapCell *, class hero *, int *, int, int, int, int, int)@0x000b4fd5
VA(0x004625c5, 0x19a)
void advManager::PlayerMonsterInteract(class mapCell *, class mapCell *, class hero *, signed char *, int, int, int, int, int) {}

// donor PoL RVA 0x000b5c40; preferred Buka symbol ?DoNetCombat@advManager@@QAEHPAD@Z
// donor Buka TU SOURCE/EVENTS; HoMM1 owner inferred from contiguous order
// evidence: graph:3;base=0.634004;margin=0.818203;shape=0.529;size=0.995;calls=1.000;alternate=pol20:int advManager::DoNetCombat(char *)@0x000b5c40
VA(0x004628b1, 0x18f)
int advManager::DoNetCombat(char *) { return 0; }

// donor PoL RVA 0x000b5e10; preferred Buka symbol ?DoCombat@advManager@@QAEHHHPAVhero@@PAVarmyGroup@@PAVtown@@01HHHH@Z
// donor Buka TU SOURCE/EVENTS; HoMM1 owner inferred from contiguous order
// evidence: graph:3;base=0.590184;margin=0.564001;shape=0.455;size=0.978;calls=0.927;alternate=pol20:int advManager::DoCombat(int, int, class hero *, class armyGroup *, class town *, class hero *, class armyGroup *, int, int, int, int)@0x000b5e10
VA(0x00462a40, 0x5c6)
int advManager::DoCombat(int, int, class hero *, class armyGroup *, class town *, class hero *, class armyGroup *, int, int, int, int) { return 0; }

// donor PoL RVA 0x000b645e; preferred Buka symbol ?SendHeroTownData@advManager@@QAEXHHPAVhero@@PAVarmyGroup@@PAVtown@@01HHHHHHH@Z
// donor Buka TU SOURCE/EVENTS; HoMM1 owner inferred from contiguous order
// evidence: graph:4;base=0.543308;margin=0.967008;shape=0.438;size=0.943;calls=0.684;alternate=pol20:void advManager::SendHeroTownData(int, int, class hero *, class armyGroup *, class town *, class hero *, class armyGroup *, int, int, int, int, int, int, int)@0x000b645e
VA(0x00463006, 0x2da)
void advManager::SendHeroTownData(int, int, class hero *, class armyGroup *, class town *, class hero *, class armyGroup *, int, int, int, int, int, int, int) {}

// donor PoL RVA 0x000b67cd; preferred Buka symbol ?ReceiveHeroTownData@advManager@@QAEXPADPAH11PAPAVhero@@PAPAVarmyGroup@@PAPAVtown@@23111PAC55@Z
// donor Buka TU SOURCE/EVENTS; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.493176;margin=0.152223;shape=0.314;size=0.857;calls=0.909;alternate=pol20:void advManager::ReceiveHeroTownData(char *, int *, int *, int *, class hero * *, class armyGroup * *, class town * *, class hero * *, class armyGroup * *, int *, int *, int *, signed char *, signed char *, signed char *)@0x000b67cd
VA(0x004632e0, 0x350)
void advManager::ReceiveHeroTownData(char *, int *, int *, int *, class hero * *, class armyGroup * *, class town * *, class hero * *, class armyGroup * *, int *, int *, int *, signed char *, signed char *, signed char *) {}

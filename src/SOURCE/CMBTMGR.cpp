// Located from HoMM2 Buka 2.1; PoL 2.0 supplies the VC4 declaration.

#include <match.h>

#include <BASE/Misc.h>
#include <H1/All.h>
#include <H1/KB.h>
#include <BASE/INPUTMGR_TYPES.h>
#include <BASE/icon.h>
#include <BASE/mouseManager.h>
#include <SOURCE/wingraph.h>
#include <SOURCE/kbwin.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Battlefield backdrops (0x00490db0), ground tiles (0x00491058) and
// obstacle icons (0x00491078) per combat terrain.
extern char* cCombatBkgNames[];
extern char* cCombatGroundNames[];
extern char* cCombatObstacleNames[];
// CheckApplyGoodMorale grants one extra turn at a time.
DATA(0x00490d50)
int bInHighMoraleBonus;
// SetupCombat saves the adventure random seed here; GenerateMap restores it.
DATA(0x00490d54)
int giSeed;

// Buka CMBTMGR.cpp combatManager(); HoMM1 keeps no message buffers.
VA(0x0044b440, 0x1b8)
combatManager::combatManager(void)
{
    m_unknown25e = 0;
    m_unknown6f9 = -1;
    m_currentSide = 0;
    m_limitCreatureHex = 0;
    m_limitCreature = 0;
    m_unknown6c0 = 1;
    m_unknown25c = 0;
    m_currentCommand = 0;
    m_unknown6e8 = 0;
    m_currentSpeed = 4;
    m_savedBorder = 0;
    m_heroType[0] = m_heroType[1] = m_unknown6c5 = m_unknown6c7 = m_wallFrame = m_wallDamage = -1;
    m_unknown6d9 = m_unknown6db = 0;
    m_castleSide[0] = m_castleSide[1] = 0;
    m_unknown72f = 0;
}

// donor PoL RVA 0x0008ff0a; preferred Buka symbol ?CombineGroups@combatManager@@QAEXPAVarmyGroup@@0@Z
// donor Buka TU SOURCE/CMBTMGR; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.491936;margin=0.502339;shape=0.296;size=0.801;calls=1.000;alternate=pol20:void combatManager::CombineGroups(class armyGroup *, class armyGroup *)@0x0008ff0a
VA(0x0044b5f8, 0x138)
void combatManager::CombineGroups(armyGroup* from, armyGroup* to) {
    short i;
    short j;

    if (!from || !to)
        return;
    for (i = 0; i < 5; i++) {
        if (to->IsMember(from->m_creatureTypes[i])) {
            to->Add(from->m_creatureTypes[i], from->m_creatureCounts[i], -1);
            from->Dismiss(i);
        }
    }
    for (i = 0; i < 5; i++) {
        if (from->m_creatureTypes[i] != -1) {
            for (j = 0; j < 5; j++) {
                if (to->m_creatureTypes[j] == -1) {
                    to->Add(from->m_creatureTypes[i], from->m_creatureCounts[i], j);
                    from->Dismiss(i);
                }
            }
        }
    }
}

// Buka CMBTMGR.cpp SetupCombat; HoMM1's attacker is side 1.
VA(0x0044b730, 0x3db)
void combatManager::SetupCombat(int mapX, int mapY, hero* attackerHero, armyGroup* attackerGroup,
                                town* defenderTown, hero* defenderHero, armyGroup* defenderGroup,
                                int combatX, int combatY, int randomSeed)
{
    int i;

    giSeed = randomSeed;
    SRand(combatX * 100 + combatY);
    m_combatX = combatX;
    m_combatY = combatY;
    if (mapX >= 0 && mapY >= 0)
        m_battlefieldCell = gpAdvManager->GetCell(mapX, mapY);
    else
        m_battlefieldCell = 0;
    m_terrainType = giGroundToTerrain[m_battlefieldCell->m_tileIndex];
    if (attackerHero) {
        m_playerId[1] = attackerHero->m_owner;
        attackerGroup = &attackerHero->m_army;
    } else {
        m_playerId[1] = -1;
    }
    if (defenderHero) {
        m_playerId[0] = defenderHero->m_owner;
        defenderGroup = &defenderHero->m_army;
    } else if (defenderTown) {
        m_playerId[0] = defenderTown->m_owner;
        defenderGroup = &defenderTown->m_army;
    } else {
        m_playerId[0] = -1;
    }
    for (i = 0; i < 2; i++) {
        if (m_playerId[i] >= 0)
            m_unknown2b8[i] = gbHumanPlayer[m_playerId[i]];
        else
            m_unknown2b8[i] = 0;
        if (i == 1)
            m_heroes[i] = attackerHero;
        else
            m_heroes[i] = defenderHero;
        if (m_heroes[i])
            m_heroType[i] = m_heroes[i]->m_unknown1c;
        else
            m_heroType[i] = -1;
        if (i == 1)
            m_armyGroups[i] = attackerGroup;
        else
            m_armyGroups[i] = defenderGroup;
        m_catapultAttackCount[i] = m_catapultAttacksRemaining[i] = 1;
        if (m_heroes[i] && m_heroes[i]->HasArtifact(0x11))
            m_catapultAttackCount[i] = m_catapultAttacksRemaining[i] = 2;
        m_keepAttacksRemaining[i] = 1;
        m_unknown6df[i] = 0;
        m_heroCastSpell[i] = 0;
    }
    m_castleSide[1] = 0;
    if (defenderTown) {
        if (defenderTown->m_occupyingHeroId != -1) {
            m_armyGroups[0] = &m_heroes[0]->m_army;
            CombineGroups(&defenderTown->m_army, &m_heroes[0]->m_army);
            m_unknown6df[0] = 1;
        } else {
            m_unknown6df[0] = 0;
        }
        if (defenderTown->m_buildings & 0x40)
            m_castleSide[0] = 1;
        else
            m_castleSide[0] = 0;
        m_combatTowns[0] = defenderTown;
        m_originalCombatTown = m_combatTowns[0];
    } else {
        m_castleSide[0] = 0;
        m_combatTowns[0] = 0;
    }
    m_combatTowns[1] = 0;
}

// Buka CMBTMGR.cpp Open: screen buffer, combat window, icons, armies and
// field, then the fade-in and a random combat theme.
VA(0x0044bb0b, 0x40e)
short combatManager::Open(short priority)
{
    int song;
    SAMPLE2 sample;
    int musicList[4];

    m_messageTypeMask = 0x32f;
    m_unknown72f = 0;
    m_savedBorder = 0;
    gpSoundManager->PlayAmbientMusic(-1, 0, -1);
    m_backgroundBuffer = new bitmap(0, 640, 460);
    m_backgroundDrawn = 0;
    sample = NULL_SAMPLE2;
    sample = LoadPlaySample("PREBATTL.82M");
    giNextAction = 0;
    gpWindowManager->FadeScreen(1, 8, 0);
    m_sideRetreated[0] = 0;
    m_sideRetreated[1] = 0;
    m_combatResult = 3;
    gbUseClippedIconRenderer = 0;
    m_unknown727 = 0;
    m_unknown72b = 0;
    gCurLoadedSpellIcon = 0;
    gCurLoadedSpellEffect = 0;
    gpMouseManager->SetPointer("cmbtmous.mse", 6);
    m_combatWindow = new heroWindow(0, 0, "cmbtwin.bin");
    if (!m_combatWindow)
        MemError();
    gpWindowManager->AddWindow(m_combatWindow, -1, 1);
    m_font = gpResourceManager->GetFont("smalfont.fnt");
    LoadIcons();
    LoadArmies();
    m_selectedHex = -1;
    m_limitCreatureHex = -1;
    m_previousCommand = 0x9d;
    GenerateMap();
    gbRetreatWin = 0;
    gbCombatSurrender = 0;
    m_sideDefeated[0] = 0;
    m_sideDefeated[1] = 0;
    m_limitCreature = 1;
    SetUnknown25e(0);
    m_unknown25c = 0;
    m_unknown72f = 1;
    DrawFrame(1);
    glTimers[0] = KBTickCount() + 75;
    m_combatPalette = gpResourceManager->GetPalette("kb.pal");
    KBChangeMenu(hmnuCmbt);
    CombatMessage("", 1);
    gpWindowManager->FadeScreen(0, 8, m_combatPalette);
    gbLimitedCombatUpdatePalette = 1;
    gpMouseManager->NewUpdate(1);
    gpMouseManager->WarpPointer(m_hexCells[m_limitCreatureHex].m_x, m_hexCells[m_limitCreatureHex].m_y - 50);
    gpMouseManager->ReallyShowPointer();
    ResetMouse();
    m_gridSelectionDisabled = 0;
    WaitEndSample(sample, -1);
    musicList[0] = 0x29;
    musicList[1] = 0x2a;
    musicList[2] = 0x28;
    musicList[3] = 0x35;
    song = musicList[SRandom(0, 3)];
    gpSoundManager->SwitchAmbientMusic(song);
    m_messageMask = 0x200;
    m_priority = priority;
    m_active = 1;
    strcpy(m_name, "combatManager");
    return 0;
}

// Buka CMBTMGR.cpp Close; a wandering-monster cell keeps the surviving
// count of the side that held it.
VA(0x0044bf19, 0x1ea)
void combatManager::Close(void)
{
    int i;
    int survivor;

    gpSoundManager->SwitchAmbientMusic(-1);
    DrawCombatBorder();
    gbLimitedCombatUpdatePalette = 0;
    gpWindowManager->FadeScreen(1, 8, 0);
    delete m_backgroundBuffer;
    for (i = 0; i < 2; i++)
        UpdateArmyGroup(i);
    if (m_battlefieldCell->m_triggerType == 0x9a) {
        survivor = (signed char)(m_playerId[0] != -1);
        m_battlefieldCell->m_objectMetadata = 0;
        for (i = 0; i < 5; i++) {
            if (m_armyGroups[survivor]->m_creatureTypes[i] != -1)
                m_battlefieldCell->m_objectMetadata += m_armyGroups[survivor]->m_creatureCounts[i];
        }
    }
    gpWindowManager->RemoveWindow(m_combatWindow);
    FreeArmies();
    FreeIcons();
    gpResourceManager->Dispose(m_font);
    gpResourceManager->Dispose(m_combatPalette);
    delete m_combatWindow;
    if (m_savedBorder)
        free(m_savedBorder);
    m_active = 0;
    m_unknown72f = 0;
}

// Buka CMBTMGR.cpp UpdateArmyGroup: copy surviving counts back into the
// side's army group; a dead stack empties its slot.
VA(0x0044c103, 0x161)
void combatManager::UpdateArmyGroup(signed char side)
{
    short i;
    short j;

    for (i = 0; i < m_numArmies[side]; i++) {
        for (j = 0; j < 5; j++) {
            if (m_armyGroups[side]->m_creatureTypes[j] == m_armies[side][i].m_creatureType)
                break;
        }
        if (j < 5) {
            if (m_armies[side][i].m_stats.attributes & 0x10) {
                m_armyGroups[side]->m_creatureTypes[j] = -1;
                m_armyGroups[side]->m_creatureCounts[j] = 0;
            } else {
                m_armyGroups[side]->m_creatureCounts[j] = m_armies[side][i].m_quantity;
            }
        }
    }
}

// Buka CMBTMGR.cpp GenerateMap; HoMM1 also places both armies, scatters
// ground patches and, outside a siege, up to two obstacles.
VA(0x0044c264, 0x7be)
void combatManager::GenerateMap(void)
{
    short x;
    short i;
    short y;
    int randomRow;
    int randomCol;
    short count;
    short armyCount;

    if (m_castleSide[0] == 1)
        m_unknown6c7 = 0;
    else
        m_unknown6c7 = -1;
    if (m_castleSide[1] == 1)
        m_unknown6c5 = 0;
    else
        m_unknown6c5 = -1;
    for (y = 0; y < 5; y++) {
        for (x = 0; x < 9; x++) {
            m_hexCells[y * 9 + x].m_y = y * 80 + 139;
            m_hexCells[y * 9 + x].m_x = ((y & 1) ? 27 : -12) + x * 78;
            m_hexCells[y * 9 + x].m_groundIcon = 0;
            m_hexCells[y * 9 + x].m_groundFrame = (signed char)SRandom(0, 3) + 4;
            if (x == 0) {
                if (m_castleSide[1] == 1)
                    m_hexCells[y * 9 + x].m_groundIcon = 5;
                if (y & 1)
                    m_hexCells[y * 9 + x].m_groundFrame = 0;
                else
                    m_hexCells[y * 9 + x].m_groundFrame = 1;
            } else if (x == 8) {
                if (m_castleSide[0] == 1)
                    m_hexCells[y * 9 + x].m_groundIcon = 5;
                if (y & 1)
                    m_hexCells[y * 9 + x].m_groundFrame = 3;
                else
                    m_hexCells[y * 9 + x].m_groundFrame = 2;
            }
            m_hexCells[y * 9 + x].m_occupantSide = -1;
            m_hexCells[y * 9 + x].m_occupantIndex = -1;
            m_hexCells[y * 9 + x].m_occupantFrame = -1;
            m_hexCells[y * 9 + x].m_obstacleIndex = -1;
            m_hexCells[y * 9 + x].m_pathFlag = 0;
        }
    }
    count = SRandom(8, 15);
    for (i = 0; i < count; i++) {
        randomRow = SRandom(0, 4);
        randomCol = SRandom(1, 7);
        m_hexCells[randomRow * 9 + randomCol].m_groundFrame = (signed char)SRandom(0, 2) + 8;
    }
    if (m_castleSide[0]) {
        for (x = 6; x < 8; x++) {
            for (y = 0; y < 5; y++) {
                m_hexCells[y * 9 + x].m_groundIcon = 5;
                m_hexCells[y * 9 + x].m_groundFrame = 4;
            }
        }
        for (y = 0; y < 5; y++) {
            m_hexCells[y * 9 + 5].m_obstacleType = 5;
            m_hexCells[y * 9 + 5].m_obstacleIndex = 8;
        }
    }
    armyCount = 0;
    for (i = 0; i < 5; i++) {
        if (m_armyGroups[1]->m_creatureTypes[i] != -1) {
            m_armies[1][armyCount].m_hex = i * 9 + 1;
            m_armies[1][armyCount].m_stats.attributes &= 0x3f;
            m_hexCells[i * 9 + 1].m_occupantSide = 1;
            m_hexCells[i * 9 + 1].m_occupantIndex = armyCount;
            if (m_armies[1][armyCount].m_stats.attributes & 1) {
                m_hexCells[i * 9 + 2].m_occupantSide = 1;
                m_hexCells[i * 9 + 2].m_occupantIndex = armyCount;
                m_hexCells[i * 9 + 1].m_occupantFrame = 1;
                m_hexCells[i * 9 + 2].m_occupantFrame = 0;
            }
            armyCount++;
        }
    }
    armyCount = 0;
    for (i = 0; i < 5; i++) {
        if (m_armyGroups[0]->m_creatureTypes[i] != -1) {
            m_armies[0][armyCount].m_hex = i * 9 + 7;
            m_armies[0][armyCount].m_stats.attributes &= 0x3f;
            m_hexCells[i * 9 + 7].m_occupantSide = 0;
            m_hexCells[i * 9 + 7].m_occupantIndex = armyCount;
            if (m_armies[0][armyCount].m_stats.attributes & 1) {
                m_hexCells[i * 9 + 6].m_occupantSide = 0;
                m_hexCells[i * 9 + 6].m_occupantIndex = armyCount;
                m_hexCells[i * 9 + 6].m_occupantFrame = 1;
                m_hexCells[i * 9 + 7].m_occupantFrame = 0;
            }
            armyCount++;
        }
    }
    count = 0;
    if (!m_castleSide[1] && !m_castleSide[0]) {
        count = SRandom(0, 3);
        for (i = 0; i < count; i++) {
            x = SRandom(3, 5);
            y = SRandom(0, 4);
            while (m_hexCells[y * 9 + x].m_occupantSide != -1) {
                x = SRandom(3, 5);
                y = SRandom(0, 4);
            }
            m_hexCells[y * 9 + x].m_obstacleType = 2;
            m_hexCells[y * 9 + x].m_obstacleIndex = SRandom(0, 2);
            if ((m_terrainType == 0 || m_terrainType == 4) && m_hexCells[y * 9 + x].m_obstacleIndex == 2)
                m_hexCells[y * 9 + x].m_obstacleIndex = 0;
        }
    }
    m_currentSide = 0;
    m_currentSpeed = 4;
    GetNextArmy(0);
    m_unknown25c = 0;
    SRand(giSeed);
}

// Buka CMBTMGR.cpp GetBackgroundName; a graveyard (or a hero standing on
// one) forces the graveyard field.
VA(0x0044ca22, 0x18e)
char* combatManager::GetBackgroundName(void)
{
    if ((m_battlefieldCell->m_triggerType & 0x7f) == 0xc
        || ((m_battlefieldCell->m_triggerType & 0x7f) == 0x3d
            && (gpGame->GetHero(m_battlefieldCell->m_objectMetadata)->m_locationType & 0x7f) == 0xc)) {
        m_terrainType = 6;
        return cCombatBkgNames[10];
    }
    switch (m_terrainType) {
        case 0:
            return cCombatBkgNames[9];
        case 3:
            return cCombatBkgNames[4];
        case 4:
            return cCombatBkgNames[5];
        case 5:
            return cCombatBkgNames[6];
        case 1:
            if (MoreTreesNear())
                return cCombatBkgNames[0];
            else
                return cCombatBkgNames[1];
        case 2:
            if (MoreTreesNear())
                return cCombatBkgNames[2];
            else
                return cCombatBkgNames[3];
        case 6:
            if (MoreTreesNear())
                return cCombatBkgNames[7];
            else
                return cCombatBkgNames[8];
    }
    return cCombatBkgNames[0];
}

// Buka CMBTMGR.cpp MoreTreesNear: tree (9) against mountain (8) objects
// within two cells of the battle.
VA(0x0044cbb0, 0x1e7)
signed char combatManager::MoreTreesNear(void)
{
    int yPos;
    int xPos;
    short numTrees;
    short step;
    short homeX;
    signed char typeTable[3][8];
    short numMountains;
    mapCell* nearCell;
    short homeY;
    unsigned char nearbyTileset;
    short k;

    memset(typeTable, -1, sizeof(typeTable));
    homeX = m_combatX;
    homeY = m_combatY;
    for (step = 0; step < 3; step++) {
        for (k = 0; k < 8; k++) {
            xPos = normalDirTable[k].x * step + homeX;
            yPos = normalDirTable[k].y * step + homeY;
            if (xPos >= 0 && xPos < 72 && yPos >= 0 && yPos < 72) {
                nearCell = gpAdvManager->GetCell(xPos, yPos);
                nearbyTileset = nearCell->m_objectTileset & 0xf;
                if (nearbyTileset == 8)
                    typeTable[step][k] = 0;
                else if (nearbyTileset == 9)
                    typeTable[step][k] = 1;
            }
        }
    }
    numTrees = 0;
    numMountains = 0;
    for (step = 0; step < 3; step++) {
        for (k = 0; k < 8; k++) {
            if (typeTable[step][k] == 0)
                numMountains++;
            if (typeTable[step][k] == 1)
                numTrees++;
        }
    }
    if (numTrees > numMountains)
        return 1;
    return 0;
}

// Buka CMBTMGR.cpp LoadIcons.
VA(0x0044cd97, 0x1d7)
void combatManager::LoadIcons(void)
{
    int i;

    for (i = 0; i < 9; i++)
        m_combatIcons[i] = 0;
    m_combatIcons[8] = gpResourceManager->GetIcon("spells.icn");
    m_backgroundBitmap = gpResourceManager->GetBitmap(GetBackgroundName());
    m_combatIcons[0] = gpResourceManager->GetIcon(cCombatGroundNames[m_terrainType]);
    m_combatIcons[2] = gpResourceManager->GetIcon(cCombatObstacleNames[m_terrainType]);
    m_combatIcons[1] = gpResourceManager->GetIcon("textbar.icn");
    m_combatIcons[4] = gpResourceManager->GetIcon("tent.icn");
    m_combatIcons[6] = gpResourceManager->GetIcon("cloud.icn");
    if (m_castleSide[1] || m_castleSide[0]) {
        m_combatIcons[3] = gpResourceManager->GetIcon("catapult.icn");
        sprintf(gText, "castle%02d.icn", m_combatTowns[(signed char)(m_castleSide[1] == 1)]->m_type);
        m_combatIcons[5] = gpResourceManager->GetIcon(gText);
        sprintf(gText, "keep%02d.icn", m_combatTowns[0]->m_type);
        m_combatIcons[7] = gpResourceManager->GetIcon(gText);
    }
}

// Buka CMBTMGR.cpp FreeIcons.
VA(0x0044cf6e, 0x7b)
void combatManager::FreeIcons(void)
{
    short i;

    for (i = 0; i < 9; i++) {
        if (m_combatIcons[i])
            gpResourceManager->Dispose(m_combatIcons[i]);
    }
    gpResourceManager->Dispose(m_backgroundBitmap);
}

// Buka CMBTMGR.cpp LoadArmies; HoMM1 places stacks itself after Init.
VA(0x0044cfe9, 0x287)
void combatManager::LoadArmies(void)
{
    short j;
    short i;

    m_numArmies[1] = m_numArmies[0] = 0;
    for (i = 0; i < 5; i++) {
        for (j = 0; j < 2; j++) {
            m_armies[j][i].m_quantity = 0;
            m_armies[j][i].m_creatureType = -1;
        }
    }
    for (j = 0; j < 2; j++) {
        for (i = 0; i < 5; i++)
            m_armies[j][i].InitClean();
    }
    for (i = 0; i < 5; i++) {
        if (m_armyGroups[1]->m_creatureTypes[i] != -1) {
            m_armies[1][m_numArmies[1]].Init(m_armyGroups[1]->m_creatureTypes[i],
                                             m_armyGroups[1]->m_creatureCounts[i], 1, m_numArmies[1]);
            m_armies[1][m_numArmies[1]].LoadResources();
            m_numArmies[1]++;
        }
        if (m_armyGroups[0]->m_creatureTypes[i] != -1) {
            m_armies[0][m_numArmies[0]].Init(m_armyGroups[0]->m_creatureTypes[i],
                                             m_armyGroups[0]->m_creatureCounts[i], 0, m_numArmies[0]);
            m_armies[0][m_numArmies[0]].LoadResources();
            m_numArmies[0]++;
        }
    }
}

// Buka CMBTMGR.cpp FreeArmies; HoMM1 frees the defenders first.
VA(0x0044d270, 0xdc)
void combatManager::FreeArmies(void)
{
    short i;

    gpSoundManager->StopAllSamples();
    for (i = 0; i < m_numArmies[1]; i++)
        m_armies[1][i].FreeResources();
    for (i = 0; i < m_numArmies[0]; i++)
        m_armies[0][i].FreeResources();
    if (gCurLoadedSpellIcon)
        gpResourceManager->Dispose(gCurLoadedSpellIcon);
    gCurLoadedSpellIcon = 0;
    gCurLoadedSpellEffect = 0;
}

// HoMM1 retail 0x0044d34c: no callers and an empty body; HoMM2's combat log
// for unshown battles is the nearest one-argument fit.
VA(0x0044d34c, 0x18)
void combatManager::NoShowCombatLog(char*)
{
}

// Buka CMBTMGR.cpp GetGridIndex over HoMM1's 9x5 grid: rows 80 pixels high
// from y 60, odd rows indented by 66 and even rows by 27, hexes 78 wide.
VA(0x0044d364, 0xbe)
short combatManager::GetGridIndex(short x, short y)
{
    y -= 60;
    y /= 80;
    if (y & 1) {
        if (x < 66) {
            x = -1;
        } else {
            x -= 66;
            x /= 78;
        }
    } else {
        x -= 27;
        x /= 78;
    }
    x++;
    if (y == 5)
        return -1;
    else
        return y * 9 + x;
}

// Buka CMBTMGR.cpp CheckApplyGoodMorale; HoMM1 rolls the group's morale.
VA(0x0044d422, 0x1d9)
void combatManager::CheckApplyGoodMorale(int side, int index)
{
    armyGroup* theGroup;
    army* activeArmy;
    SAMPLE2 sample;
    int morale;

    if (side < 0 || index < 0)
        return;
    if (bInHighMoraleBonus) {
        bInHighMoraleBonus = 0;
        return;
    }
    bInHighMoraleBonus = 0;
    theGroup = m_armyGroups[side];
    activeArmy = &m_armies[side][index];
    if (!activeArmy->m_quantity)
        return;
    morale = theGroup->GetMorale(m_heroes[side], m_combatTowns[side]);
    if (morale <= 0 || SRandom(1, 24) > morale)
        return;
    bInHighMoraleBonus = 1;
    sprintf(gText, "goodmrle.82M");
    sample = LoadPlaySample(gText);
    if (activeArmy->m_quantity <= 1)
        sprintf(gText, "High morale enables the %s to attack again.", gArmyNames[activeArmy->m_creatureType]);
    else
        sprintf(gText, "High morale enables the %s to attack again.", gArmyNamesPlural[activeArmy->m_creatureType]);
    CombatMessage(gText, 1);
    activeArmy->SpellEffect(24, 180);
    activeArmy->ResetAnimation(1);
    if (activeArmy->m_stats.attributes & 0x80)
        activeArmy->m_stats.attributes -= 0x80;
    activeArmy->m_stats.attributes |= 0x20;
    WaitEndSample(sample, -1);
}

// Buka CMBTMGR.cpp CheckApplyBadMorale; a computer side skips one roll
// in four.
VA(0x0044d5fb, 0x1c6)
int combatManager::CheckApplyBadMorale(int side, int index)
{
    armyGroup* theGroup;
    army* activeArmy;
    SAMPLE2 sample;
    int morale;

    if (side < 0 || index < 0)
        return 0;
    theGroup = m_armyGroups[side];
    activeArmy = &m_armies[side][index];
    morale = theGroup->GetMorale(m_heroes[side], m_combatTowns[side]);
    if (morale >= 0 || SRandom(1, 12) > -morale)
        return 0;
    if (!m_unknown2b8[side] && SRandom(1, 4) == 1)
        return 0;
    sample = NULL_SAMPLE2;
    sample = LoadPlaySample("BADMRLE.82M");
    if (activeArmy->m_quantity <= 1)
        sprintf(gText, "Low morale causes the %s to freeze in panic.", gArmyNames[activeArmy->m_creatureType]);
    else
        sprintf(gText, "Low morale causes the %s to freeze in panic.", gArmyNamesPlural[activeArmy->m_creatureType]);
    CombatMessage(gText, 1);
    activeArmy->m_unknown09 = 2;
    activeArmy->SpellEffect(25, 180);
    activeArmy->ResetAnimation(1);
    activeArmy->m_stats.attributes |= 0x80;
    WaitEndSample(sample, -1);
    return 1;
}

// Buka CMBTMGR.cpp GetNextArmy: the fastest unspent stack, alternating
// sides, high-morale stacks first.
VA(0x0044d7c1, 0x209)
signed char combatManager::GetNextArmy(int checkMorale)
{
    army* pArmy;
    signed char iSpeed;
    int sideIter;
    short temp;
    signed char stackCounter;
    signed char stackSide;
    int bSkip;

    stackSide = m_currentSide;
    for (iSpeed = 0; iSpeed < 5; iSpeed++) {
        for (sideIter = 0; sideIter < 2; sideIter++) {
            stackSide ^= 1;
            for (stackCounter = 0; stackCounter < m_numArmies[stackSide]; stackCounter++) {
                bSkip = 0;
                pArmy = &m_armies[stackSide][stackCounter];
                if ((pArmy->m_stats.attributes & 0x90) || pArmy->m_spellEffect == 0x12 || pArmy->m_spellEffect == 7
                    || (pArmy->m_stats.speed != m_currentSpeed && !(pArmy->m_stats.attributes & 0x20)))
                    bSkip = 1;
                if (!bSkip && !iSpeed && !(pArmy->m_stats.attributes & 0x20))
                    bSkip = 1;
                if (!bSkip && checkMorale && CheckApplyBadMorale(stackSide, stackCounter))
                    bSkip = 1;
                if (!bSkip)
                    break;
            }
            if (m_numArmies[stackSide] != stackCounter) {
                m_currentSide = stackSide;
                m_currentArmyIndex = stackCounter;
                GetControl();
                return 1;
            }
        }
        if (iSpeed) {
            m_currentSpeed--;
            if (!m_currentSpeed)
                m_currentSpeed = 4;
        }
    }
    GetControl();
    return 0;
}

// Buka CMBTMGR.cpp IsWinner: the other side surrendered, retreated or has
// no live stack left.
VA(0x0044d9ca, 0xd3)
signed char combatManager::IsWinner(signed char side)
{
    signed char isWinner;
    short i;

    if (m_sideDefeated[1 - side])
        return 1;
    if (m_sideRetreated[1 - side])
        return 1;
    side ^= 1;
    isWinner = 1;
    for (i = 0; i < m_numArmies[side]; i++) {
        if (!(m_armies[side][i].m_stats.attributes & 0x10))
            isWinner = 0;
    }
    return isWinner;
}

// HoMM1 retail 0x0044e7f2: unreferenced; reloads the armies and rebuilds
// the field before a full redraw.
VA(0x0044e7f2, 0x4e)
void combatManager::RegenerateField(void)
{
    FreeArmies();
    LoadArmies();
    GenerateMap();
    SetUnknown25e(0);
    m_unknown25c = 0;
    DrawFrame(1);
}

// Buka CMBTMGR.cpp ExperienceValueOfStack: fight value of the side's
// losses, plus 500 for a defeated hero.
VA(0x0044f3cb, 0x114)
int combatManager::ExperienceValueOfStack(signed char side)
{
    int i;
    int value;

    value = 0;
    for (i = 0; i < 5; i++) {
        if (m_armies[side][i].m_creatureType != -1)
            value += (m_armies[side][i].m_initialQuantity - m_armies[side][i].m_quantity)
                     * gMonsterDatabase[m_armies[side][i].m_creatureType].fightValue;
    }
    if (m_heroes[side])
        value += 500;
    return value;
}

// Buka CMBTMGR.cpp ResetHitByCreature.
VA(0x0044f4df, 0x78)
void combatManager::ResetHitByCreature(void)
{
    int i;
    int j;

    for (i = 0; i < 2; i++) {
        for (j = 0; j < 5; j++)
            m_armies[i][j].m_unknown34 = 0;
    }
}

// HoMM1's combat grid is nine columns by five rows.
VA(0x0044f557, 0x30)
int ValidHex(int hex) {
    return hex >= 0 && hex <= 44;
}

// HoMM1 SaveCombatBorder: keep the twenty screen rows under the field.
VA(0x0044f587, 0x64)
void combatManager::SaveCombatBorder(void)
{
    if (!m_savedBorder)
        m_savedBorder = (char*)malloc(0x3200);
    memcpy(m_savedBorder, gpWindowManager->m_screen->m_pixels + 0x47e00, 0x3200);
}

// HoMM1 DrawCombatBorder: put the saved rows back.
VA(0x0044f5eb, 0x53)
void combatManager::DrawCombatBorder(void)
{
    if (!m_savedBorder)
        return;
    memcpy(gpWindowManager->m_screen->m_pixels + 0x47e00, m_savedBorder, 0x3200);
}

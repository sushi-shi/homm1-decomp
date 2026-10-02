// Located from HoMM2 Buka 2.1; PoL 2.0 supplies the VC4 declaration.

#include <match.h>

#include <BASE/BITS.h>
#include <BASE/INPUTMGR_TYPES.h>
#include <BASE/LZHUF.h>
#include <BASE/Misc.h>
#include <BASE/TILE.h>
#include <BASE/WINMGR_TYPES.h>
#include <H1/All.h>
#include <H1/KB.h>
#include <SOURCE/artifactTypes.h>
#include <SOURCE/campaignTypes.h>
#include <SOURCE/combatTypes.h>
#include <SOURCE/FINDPATH.h>
#include <SOURCE/mapObjectTypes.h>
#include <SOURCE/REMOTE.h>
#include <SOURCE/X_GLOBAL.h>

#include <fcntl.h>
#include <io.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

// donor PoL RVA 0x000708b0; preferred Buka symbol ?Write@playerData@@QAEXH@Z
// donor Buka TU SOURCE/GAME; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.582104;margin=0.473963;shape=0.500;size=0.883;calls=0.885;alternate=pol20:void playerData::Write(int)@0x000708b0
VA(0x00438c00, 0x1f8)
void playerData::Write(int file) {
    char unused[52];

    write(file, m_unknown00, sizeof(m_unknown00));
    write(file, &m_color, 1);
    write(file, &m_difficulty, 1);
    write(file, &m_heroCount, 1);
    write(file, &m_currentHero, 1);
    write(file, &m_heroLocatorPage, 1);
    write(file, m_heroIds, sizeof(m_heroIds));
    write(file, m_availableHeroIds, sizeof(m_availableHeroIds));
    memset(unused, 0, 50);
    write(file, unused, 50);
    write(file, &m_ultimateArtifactHintChance, 1);
    write(file, &m_ultimateArtifactHintX, 1);
    write(file, &m_ultimateArtifactHintY, 1);
    write(file, &m_daysLeft, 1);
    write(file, &m_townCount, 1);
    write(file, &m_currentTown, 1);
    write(file, &m_townLocatorPage, 1);
    write(file, m_townIds, sizeof(m_townIds));
    write(file, m_resources, sizeof(m_resources));
    write(file, m_aiData.m_income, sizeof(m_aiData.m_income));
    write(file, &m_unknown99[1], 1);
    write(file, &m_unknown99[1], 1);
    write(file, m_obelisksVisited, sizeof(m_obelisksVisited));
}

// donor PoL RVA 0x00070aed; preferred Buka symbol ?Read@playerData@@QAEXH@Z
// donor Buka TU SOURCE/GAME; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.555449;margin=0.750197;shape=0.444;size=0.878;calls=0.880;alternate=pol20:void playerData::Read(int)@0x00070aed
VA(0x00438df8, 0x1e8)
void playerData::Read(int file) {
    char unused[52];

    read(file, m_unknown00, sizeof(m_unknown00));
    read(file, &m_color, 1);
    read(file, &m_difficulty, 1);
    read(file, &m_heroCount, 1);
    read(file, &m_currentHero, 1);
    read(file, &m_heroLocatorPage, 1);
    read(file, m_heroIds, sizeof(m_heroIds));
    read(file, m_availableHeroIds, sizeof(m_availableHeroIds));
    read(file, unused, 50);
    read(file, &m_ultimateArtifactHintChance, 1);
    read(file, &m_ultimateArtifactHintX, 1);
    read(file, &m_ultimateArtifactHintY, 1);
    read(file, &m_daysLeft, 1);
    read(file, &m_townCount, 1);
    read(file, &m_currentTown, 1);
    read(file, &m_townLocatorPage, 1);
    read(file, m_townIds, sizeof(m_townIds));
    read(file, m_resources, sizeof(m_resources));
    read(file, m_aiData.m_income, sizeof(m_aiData.m_income));
    read(file, &m_unknown99[1], 1);
    read(file, &m_unknown99[1], 1);
    read(file, m_obelisksVisited, sizeof(m_obelisksVisited));
}

// donor PoL RVA 0x00070d1a; preferred Buka symbol ?NextHero@playerData@@QAEHH@Z
// donor Buka TU SOURCE/GAME; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.473378;margin=0.450383;shape=0.230;size=0.850;calls=1.000;alternate=pol20:int playerData::NextHero(int)@0x00070d1a
VA(0x00438fe0, 0x12c)
signed char playerData::NextHero(int) {
    int curHero = -1;
    int i;

    if (gpCurPlayer->m_currentHero != -1) {
        for (i = 0; i < gpCurPlayer->m_heroCount; ++i) {
            if (gpCurPlayer->m_heroIds[i] == gpCurPlayer->m_currentHero)
                curHero = i;
        }
    }

    for (i = curHero + 1; i < gpCurPlayer->m_heroCount; ++i) {
        if (gpGame->IsMobile(gpCurPlayer->m_heroIds[i]))
            return m_heroIds[i];
    }
    for (i = 0; i < curHero + 1; ++i) {
        if (gpGame->IsMobile(gpCurPlayer->m_heroIds[i]))
            return m_heroIds[i];
    }
    return -1;
}

// Buka 2.1 playerData::HasMobileHero.
VA(0x0043910c, 0x68)
signed char playerData::HasMobileHero(void) {
    for (short i = 0; i < m_heroCount; ++i) {
        if (gpGame->IsMobile(m_heroIds[i]))
            return 1;
    }
    return 0;
}

// HoMM1 counts this player's visited-obelisk bits.
VA(0x00439174, 0x5f)
signed char playerData::CountVisitedObelisks(void) {
    signed char count = 0;
    for (short i = 0; i < 48; ++i) {
        if (BitTest(m_obelisksVisited, i))
            ++count;
    }
    return count;
}

// Buka 2.1 playerData::BuildingsOwned; slot 0 is the mage guild.
VA(0x004391d3, 0xd1)
int playerData::BuildingsOwned(int townType, int buildingIndex, int buildState) {
    int count = 0;
    int i;
    for (i = 0; i < m_townCount; ++i) {
        town* ownedTown = &gpGame->m_castleRecs[m_townIds[i]];
        if (buildingIndex < BUILDING_SLOT_DWELLING_FIRST || ownedTown->m_type == townType) {
            if (buildingIndex == BUILDING_SLOT_MAGE_GUILD) {
                if (ownedTown->m_buildings & (1 << BUILDING_SLOT_MAGE_GUILD)) {
                    if (ownedTown->m_buildState == buildState)
                        ++count;
                }
            } else {
                if (ownedTown->m_buildings & (1 << buildingIndex))
                    ++count;
            }
        }
    }
    return count;
}

// Buka 2.1 playerData::NumOfGivenArtifact over HoMM1's fourteen hero slots.
VA(0x004392a4, 0x99)
int playerData::NumOfGivenArtifact(int artifact) {
    int count = 0;
    int i;
    int j;
    for (i = 0; i < m_heroCount; i++) {
        for (j = 0; j < 14; j++) {
            if (gpGame->m_heroRecs[m_heroIds[i]].m_artifacts[j] == artifact)
                count++;
        }
    }
    return count;
}

// Buka 2.1 ComputeUALoc: HoMM1 needs eleven obelisks (four percent each over
// ten) and skips player 0's hint.
VA(0x0043933d, 0x386)
void ComputeUALoc(int player) {
    int y;
    int tries;
    int x;
    int heading;
    int numObelisks;

    if (player > 0) {
        numObelisks = gpGame->m_players[player].CountVisitedObelisks();
        if (numObelisks < 11 || gpGame->m_ultimateArtifactId == -1) {
            gpGame->m_players[player].m_ultimateArtifactHintChance = 0;
            gpGame->m_players[player].m_ultimateArtifactHintX = -1;
            gpGame->m_players[player].m_ultimateArtifactHintY = -1;
        } else {
            gpGame->m_players[player].m_ultimateArtifactHintChance = (numObelisks - 11) * 4;
            if (gpGame->m_players[player].m_ultimateArtifactHintChance >= Random(1, 100)) {
                gpGame->m_players[player].m_ultimateArtifactHintX = gpGame->m_ultimateArtifactX;
                gpGame->m_players[player].m_ultimateArtifactHintY = gpGame->m_ultimateArtifactY;
            } else {
                x = -1;
                y = -1;
                heading = 0;
                tries = 0;
                while (!(x >= 0 && x < MAP_CELL_GRID_SIZE && y >= 0 && y < MAP_CELL_GRID_SIZE
                         && gpGame->m_map[x][y].m_triggerType == MAP_OBJECT_NONE
                         && gpGame->m_map[x][y].m_objectIndex == 0xff
                         && gpGame->m_map[x][y].m_overlayIndex == 0xff
                         && gpGame->m_map[x][y].m_tileIndex >= 20)) {
                    tries++;
                    heading = 0;
                    while (heading == 0)
                        heading = 3 - Random(0, 2) - Random(0, 2) - Random(0, 2);
                    x = gpGame->m_ultimateArtifactX + heading;
                    heading = 0;
                    while (heading == 0)
                        heading = 3 - Random(0, 2) - Random(0, 2) - Random(0, 2);
                    y = gpGame->m_ultimateArtifactY + heading;
                    if (tries >= 200) {
                        x = gpGame->m_ultimateArtifactX;
                        y = gpGame->m_ultimateArtifactY;
                        goto saveLocation;
                    }
                }
            saveLocation:
                gpGame->m_players[player].m_ultimateArtifactHintX = x;
                gpGame->m_players[player].m_ultimateArtifactHintY = y;
            }
        }
    }
}

// DoEvent's obelisk visit: remove this player's share of the 48 puzzle
// pieces (Buka 2.1 SetupPuzzlePieces' picker), then re-roll the hint.
VA(0x004396c3, 0x1b0)
void game::VisitObelisk(signed char player) {
    short attempts;
    signed char visited;
    signed char fallback;
    signed char piece;
    int pieces;
    short numRemoved;
    int removeCount;

    pieces = 48;
    removeCount = pieces / m_obeliskCount;
    if (removeCount < 1)
        removeCount = 1;
    for (numRemoved = 0; numRemoved < removeCount; numRemoved++) {
        visited = m_players[player].CountVisitedObelisks();
        for (piece = 0; piece < pieces; piece += Random(1, 5)) {
            if (!BitTest(m_players[player].m_obelisksVisited, piece))
                break;
        }
        for (attempts = 0; attempts < 100; attempts++) {
            fallback = Random(0, pieces - 1);
            if (!BitTest(m_players[player].m_obelisksVisited, fallback))
                break;
        }
        if (piece < pieces)
            BitSet(m_players[player].m_obelisksVisited, piece);
        else
            BitSet(m_players[player].m_obelisksVisited, fallback);
    }
    ComputeUALoc(player);
}

// Buka 2.1 game::IsMobile.
VA(0x00439873, 0xb3)
signed char game::IsMobile(signed char heroId) {
    if (heroId == -1)
        return 0;
    hero* mobileHero = &m_heroRecs[heroId];
    int terrain = giGroundToTerrain[gpAdvManager->GetCell(mobileHero->m_x, mobileHero->m_y)->m_tileIndex];
    return mobileHero->m_remainingMobility >= CalcTerrainCost(
               terrain,
               mobileHero->m_direction & 1,
               mobileHero->m_remainingMobility,
               mobileHero->m_heroClass
           );
}

// Buka 2.1 game::GetWorldMapData.
VA(0x00439926, 0x1e)
mapCell (*game::GetWorldMapData(void))[MAP_CELL_GRID_SIZE] {
    return m_map;
}

// Buka 2.1 game::CreateBoat without the network map-change notice.
VA(0x00439944, 0xd9)
signed char game::CreateBoat(signed char x, signed char y) {
    signed char boatIdx = Scan(m_boatSlots, 0, GAME_BOAT_COUNT);
    if (boatIdx != -1) {
        m_boatSlots[boatIdx] = boatIdx;
        boatRecord* boat = &m_boats[boatIdx];
        boat->id = boatIdx;
        boat->x = x;
        boat->y = y;
        boat->direction = 2;
        boat->owner = giCurPlayer;
        mapCell* square = &m_map[x][y];
        boat->savedTriggerType = square->m_triggerType;
        boat->savedEventData = square->m_objectMetadata;
        square->m_triggerType = (MAP_TRIGGER_EVENT | MAP_OBJECT_SHIP);
        square->m_objectMetadata = boatIdx;
    }
    return boatIdx;
}

// Buka 2.1 game::Scan.
VA(0x00439a1d, 0x5f)
signed char game::Scan(signed char* array, signed char start, signed char length) {
    signed char i;
    for (i = start; i < start + length; ++i) {
        if (array[i] == -1)
            return i;
    }
    return -1;
}

// Buka 2.1 game::RandomScan; HoMM1 always looks for a free (-1) entry.
VA(0x00439a7c, 0x74)
signed char game::RandomScan(signed char* array, signed char start, signed char range, int) {
    signed char index = -1;
    int i;
    for (i = 0; i < 10000; ++i) {
        index = start + Random(0, range - 1);
        if (array[index] == -1)
            return index;
    }
    return -1;
}

// HoMM1 nine heroes per class; a 0x40 entry is the fallback pick.
VA(0x00439af0, 0x10e)
signed char game::GetNewHeroId(signed char heroClass) {
    signed char freeSlot = -1;
    signed char id = -1;
    short first = heroClass * 9;
    int i;
    freeSlot = Scan(m_availableHeroes, first, 9);
    if (freeSlot != -1) {
        id = RandomScan(m_availableHeroes, first, 9, 9);
    } else {
        freeSlot = Scan(m_availableHeroes, 0, GAME_HERO_COUNT);
        if (freeSlot != -1) {
            id = RandomScan(m_availableHeroes, 0, GAME_HERO_COUNT, GAME_HERO_COUNT);
        } else {
            for (i = 0; i < GAME_HERO_COUNT; ++i) {
                if (m_availableHeroes[i] == 0x40)
                    id = i;
            }
        }
    }
    if (id != -1)
        return id;
    else
        return 0;
}

// Buka 2.1 game::GetTownId.
VA(0x00439bfe, 0x8f)
signed char game::GetTownId(signed char x, signed char y) {
    for (short i = 0; i < GAME_TOWN_COUNT; ++i) {
        if (m_castleRecs[i].m_x == x && m_castleRecs[i].m_y == y)
            return i;
    }
    return -1;
}

// Buka 2.1 game::GetMineId.
VA(0x00439c8d, 0x87)
signed char game::GetMineId(signed char x, signed char y) {
    for (short i = 0; i < GAME_MINE_COUNT; ++i) {
        if (m_mines[i].x == x && m_mines[i].y == y)
            return i;
    }
    return -1;
}

// donor PoL RVA 0x00071d89; preferred Buka symbol ?GenerateStandardFileName@@YIXPAD0@Z
// donor Buka TU SOURCE/GAME; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.427111;margin=0.052277;shape=0.267;size=0.694;calls=1.000;alternate=pol20:void GenerateStandardFileName(char *, char *)@0x00071d89
VA(0x00439d14, 0x129)
void GenerateStandardFileName(char *source, char *destination) {
    char *extension;
    int indexOut;
    int idx;
    char character;
    int size;

    extension = FindLastToken(source, '.');
    if (!extension) {
        strcpy(destination, source);
        return;
    }
    *extension = 0;
    indexOut = 0;
    size = strlen(source);
    for (idx = 0; idx < size; idx++) {
        character = source[idx];
        if (character >= 'a' && character <= 'z')
            character = character - ('a' - 'A');
        if ((character >= 'A' && character <= 'Z') || (character >= '0' && character <= '9')
            || character == '_') {
            destination[indexOut] = character;
            indexOut++;
        }
        if (indexOut >= 8)
            idx = 999;
    }
    *extension = '.';
    strcpy(destination + indexOut, extension);
}

// donor PoL RVA 0x00071eb7; preferred Buka symbol ?SaveGame@game@@QAEHPADHC@Z
// donor Buka TU SOURCE/GAME; HoMM1 owner inferred from contiguous order
// evidence: graph:3;base=0.606443;margin=0.348788;shape=0.410;size=0.678;calls=0.698;strings=%s%s|%s.%s|%s.GM%d;alternate=pol20:int game::SaveGame(char *, int, signed char)@0x00071eb7
inline void game::ReadWorldMap(int fd) {
    read(fd, m_map, sizeof(m_map));
}

inline void game::WriteWorldMap(int fd) {
    write(fd, m_map, sizeof(m_map));
}

// Buka 2.1 game::SaveGame for HoMM1's single save layout: name, globals,
// campaign state, map header, players, world map, records and visibility.
VA(0x00439e3d, 0x7b2)
short game::SaveGame(char* filename, signed char generateName) {
    int nHumans;
    int saveFlag;
    char human[GAME_PLAYER_COUNT];
    int iFile;
    int file;
    int junk[4];
    char filePath[452];
    char fileName[460];
    char buffer[100];

    gpAdvManager->DemobilizeCurrHero();
    if (generateName) {
        if (m_campaignType > 0) {
            sprintf(fileName, "%s.%s", filename, "CGM");
        } else {
            nHumans = 0;
            for (iFile = 0; iFile < GAME_PLAYER_COUNT; iFile++) {
                if (!m_playerDead[iFile] && gbHumanPlayer[iFile])
                    nHumans++;
            }
            sprintf(fileName, "%s.GM%d", filename, nHumans);
        }
    } else {
        sprintf(fileName, filename);
    }
    if (!strcmpi(fileName, "REMOTE.GAM")) {
        extern char gcDataPath[];
        sprintf(filePath, "%s%s", gcDataPath, fileName);
    } else {
        extern char gcGamePath[];
        sprintf(filePath, "%s%s", gcGamePath, fileName);
        if (strnicmp(fileName, "AUTOSAVE", 8) && strnicmp(fileName, "PLYREXIT", 8))
            strcpy(gpGame->m_saveName, filename);
    }
    file = open(filePath, O_WRONLY | O_CREAT | O_TRUNC | O_BINARY, S_IWRITE);
    if (file == -1)
        FileError(filePath);
    write(file, &gbKingOfTheHill, 1);
    write(file, this, 2);
    write(file, &giMonthType, 1);
    write(file, &giMonthSpecial, 1);
    write(file, &giWeekType, 1);
    write(file, &giWeekSpecial, 1);
    write(file, &m_campaignType, 4);
    write(file, &m_campaignScenario, 4);
    write(file, &m_campaignDay, 4);
    write(file, &m_campaignScenariosWon, 4);
    memset(buffer, 0, 0x2c);
    write(file, buffer, 0x2c);
    write(file, m_mapDescription, sizeof(m_mapDescription));
    write(file, &m_mapSize, 1);
    write(file, &m_mapDifficulty, 1);
    write(file, m_mapName, sizeof(m_mapName));
    GenerateStandardFileName(m_saveName, buffer);
    write(file, buffer, 0x11);
    write(file, &m_difficulty, 1);
    write(file, &m_playerCount, 1);
    gSaveCurPlayer = giCurPlayer;
    write(file, &gSaveCurPlayer, 1);
    write(file, &m_deadPlayerCount, 1);
    write(file, m_playerDead, sizeof(m_playerDead));
    for (iFile = 0; iFile < GAME_PLAYER_COUNT; iFile++) {
        human[iFile] = gbHumanPlayer[iFile];
        if (m_playerDead[iFile])
            human[iFile] = 0;
    }
    write(file, human, GAME_PLAYER_COUNT);
    write(file, &m_day, 2);
    write(file, &m_week, 2);
    write(file, &m_month, 2);
    for (iFile = 0; iFile < GAME_PLAYER_COUNT; iFile++)
        m_players[iFile].Write(file);
    WriteWorldMap(file);
    write(file, &m_obeliskCount, 1);
    write(file, m_heroRecs, sizeof(m_heroRecs));
    write(file, m_availableHeroes, sizeof(m_availableHeroes));
    write(file, m_castleRecs, sizeof(m_castleRecs));
    write(file, m_townOwners, sizeof(m_townOwners));
    write(file, m_townBuiltToday, sizeof(m_townBuiltToday));
    write(file, m_mines, sizeof(m_mines));
    write(file, m_mineOwners, sizeof(m_mineOwners));
    write(file, m_randomArtifacts, sizeof(m_randomArtifacts));
    write(file, m_boats, sizeof(m_boats));
    write(file, m_boatSlots, sizeof(m_boatSlots));
    write(file, m_obeliskVisitors, sizeof(m_obeliskVisitors));
    write(file, &m_ultimateArtifactX, 1);
    write(file, &m_ultimateArtifactY, 1);
    write(file, &m_ultimateArtifactId, 1);
    write(file, m_mapSounds, sizeof(m_mapSounds));
    write(file, m_mapExtra, sizeof(m_mapExtra));
    write(file, mapVisited, sizeof(mapVisited));
    close(file);
    return 1;
}

// donor PoL RVA 0x000735bf; preferred Buka symbol ?LoadGame@game@@QAEXPADHH@Z
// donor Buka TU SOURCE/GAME; HoMM1 owner inferred from contiguous order
// evidence: graph:5;base=0.668603;margin=0.422052;shape=0.401;size=0.926;calls=0.741;strings=%s%s|.\DATA\|.\GAMES\;alternate=pol20:void game::LoadGame(char *, int, int)@0x000735bf
// Alias: retail reaches the debug-level dword (0x004c7c94, giDebugLevel), not
// SETUP's byte iMPExtendedType (SETUP.h); rename at the use.
extern int iMPExtendedType;

// Buka 2.1 game::LoadGame for HoMM1's save layout; origdata.bin restores
// the default hero names and blank visibility, and the seats are re-dealt
// to this session's human players.
VA(0x0043a5ef, 0x9b2)
short game::LoadGame(char* filename, int origData, int) {
    int junk2;
    int numHumans;
    int i;
    int handle;
    char pathName[452];
    signed char humans[GAME_PLAYER_COUNT];
    int junk;
    char buffer[0x2c];

    numHumans = 0;
    gbGameOver = 0;
    m_noMapHeroes = 1;
    extern char gcDataPath[];
    extern char gcGamePath[];
    if (origData || !strcmp(filename, "REMOTE.GAM"))
        sprintf(pathName, "%s%s", gcDataPath, filename);
    else
        sprintf(pathName, "%s%s", gcGamePath, filename);
    handle = open(pathName, O_BINARY);
    if (handle == -1)
        FileError(pathName);
    ClearMapExtra();
    read(handle, &gbKingOfTheHill, 1);
    read(handle, this, 2);
    read(handle, &giMonthType, 1);
    read(handle, &giMonthSpecial, 1);
    read(handle, &giWeekType, 1);
    read(handle, &giWeekSpecial, 1);
    read(handle, &m_campaignType, 4);
    read(handle, &m_campaignScenario, 4);
    read(handle, &m_campaignDay, 4);
    read(handle, &m_campaignScenariosWon, 4);
    read(handle, buffer, 0x2c);
    read(handle, m_mapDescription, sizeof(m_mapDescription));
    read(handle, &m_mapSize, 1);
    read(handle, &m_mapDifficulty, 1);
    read(handle, m_mapName, sizeof(m_mapName));
    read(handle, m_saveName, 0x11);
    sprintf(m_saveName, filename);
    read(handle, &m_difficulty, 1);
    read(handle, &m_playerCount, 1);
    read(handle, &gSaveCurPlayer, 1);
    giCurPlayer = gSaveCurPlayer;
    read(handle, &m_deadPlayerCount, 1);
    read(handle, m_playerDead, sizeof(m_playerDead));
    read(handle, humans, GAME_PLAYER_COUNT);
    for (i = 0; i < GAME_PLAYER_COUNT; i++) {
        if ((humans[i] || giDebugLevel >= 2) && numHumans < giNumHumanPlayers) {
            numHumans++;
            gbHumanPlayer[i] = 1;
        } else {
            gbHumanPlayer[i] = 0;
        }
    }
    for (i = 0; i < GAME_PLAYER_COUNT; i++) {
        if (gbHumanPlayer[i]) {
            if (!gbRemoteOn || i == giThisGamePos)
                gbThisNetHumanPlayer[i] = 1;
            else
                gbThisNetHumanPlayer[i] = 0;
        } else {
            gbThisNetHumanPlayer[i] = 0;
        }
    }
    read(handle, &m_day, 2);
    read(handle, &m_week, 2);
    read(handle, &m_month, 2);
    giCurTurn = (m_month - 1) * 28 + (m_week - 1) * 7 + m_day;
    for (i = 0; i < GAME_PLAYER_COUNT; i++)
        m_players[i].Read(handle);
    ReadWorldMap(handle);
    read(handle, &m_obeliskCount, 1);
    read(handle, m_heroRecs, sizeof(m_heroRecs));
    if (origData) {
        for (i = 0; i < GAME_HERO_COUNT; i++) {
            strcpy(m_heroRecs[i].m_name, gHeroNames[i][0]);
            strcpy(m_heroRecs[i].m_shortName, gHeroNames[i][1]);
        }
    }
    read(handle, m_availableHeroes, sizeof(m_availableHeroes));
    read(handle, m_castleRecs, sizeof(m_castleRecs));
    read(handle, m_townOwners, sizeof(m_townOwners));
    read(handle, m_townBuiltToday, sizeof(m_townBuiltToday));
    read(handle, m_mines, sizeof(m_mines));
    read(handle, m_mineOwners, sizeof(m_mineOwners));
    read(handle, m_randomArtifacts, sizeof(m_randomArtifacts));
    read(handle, m_boats, sizeof(m_boats));
    read(handle, m_boatSlots, sizeof(m_boatSlots));
    read(handle, m_obeliskVisitors, sizeof(m_obeliskVisitors));
    read(handle, &m_ultimateArtifactX, 1);
    read(handle, &m_ultimateArtifactY, 1);
    read(handle, &m_ultimateArtifactId, 1);
    if (origData) {
        memset(m_mapSounds, -1, sizeof(m_mapSounds));
        memset(m_mapExtra, 0, sizeof(m_mapExtra));
        memset(mapVisited, 0, sizeof(mapVisited));
        strcpy(gpGame->m_saveName, "NEWGAME");
    } else {
        read(handle, m_mapSounds, sizeof(m_mapSounds));
        read(handle, m_mapExtra, sizeof(m_mapExtra));
        read(handle, mapVisited, sizeof(mapVisited));
        if (strcmp(filename, "REMOTE.GAM"))
            strcpy(gpGame->m_saveName, filename);
    }
    close(handle);
    gpAdvManager->m_heroContextLocked = 0;
    gpCurPlayer = &gpGame->m_players[giCurPlayer];
    giCurPlayerBit = 1 << giCurPlayer;
    giCurWatchPlayer = giCurPlayer;
    while (!gbThisNetHumanPlayer[giCurWatchPlayer])
        giCurWatchPlayer = (giCurWatchPlayer + 1) % m_playerCount;
    giCurWatchPlayerBit = 1 << giCurWatchPlayer;
    giCurPlayerHighBit = 1 << (giCurPlayer + 4);
    giCurWatchPlayerHighBit = 1 << (giCurWatchPlayer + 4);
    bShowIt = gbThisNetHumanPlayer[giCurPlayer];
    memset(mapExtra, 0, sizeof(mapExtra));
    if (!origData)
        SetupAdjacentMons();
    return 1;
}

// clang-format off
// newgame.bin widget ids. The opponent toggles are players 1..3 (id - 1);
// difficulty buttons are FIRST + game::m_difficulty. OK and CANCEL are role
// names on the reserved dialog slots (gNewGameHelp: 0x7802 accepts, 0x7801
// returns to the main menu).
H1_ENUM_BEGIN(NewGameControl)
    NEW_GAME_OPPONENT_FIRST = 2,
    NEW_GAME_OPPONENT_LAST = 4,
    NEW_GAME_COLOR = 8,
    NEW_GAME_SCENARIO_SELECT = 0xc,
    NEW_GAME_DIFFICULTY_FIRST = 0xd,
    NEW_GAME_DIFFICULTY_LAST = 0x10,
    NEW_GAME_SCENARIO_NAME = 0x11,
    NEW_GAME_KING_OF_THE_HILL = 0x13,
    NEW_GAME_RATING = 0x14,
    NEW_GAME_CANCEL = DIALOG_BUTTON_1,
    NEW_GAME_OK = DIALOG_BUTTON_2
H1_ENUM_END(NewGameControl)

// NewGameHandler's right-click help: the gNewGameHelp row shown.
H1_ENUM_BEGIN(NewGameHelp)
    NEW_GAME_HELP_NONE = -1,
    NEW_GAME_HELP_ACCEPT = 0,
    NEW_GAME_HELP_MAIN_MENU = 1,
    NEW_GAME_HELP_KING_OF_THE_HILL = 2,
    NEW_GAME_HELP_SCENARIO = 3,
    NEW_GAME_HELP_DIFFICULTY = 4,
    NEW_GAME_HELP_OPPONENT = 5,
    NEW_GAME_HELP_COLOR = 6,
    NEW_GAME_HELP_RATING = 7,
    NEW_GAME_HELP_HUMAN_OPPONENT = 8
H1_ENUM_END(NewGameHelp)
// clang-format on

// Buka 2.1 NewGameHandler without HoMM2's remote chat and player races:
// right clicks show help, the player toggles cycle the opponents and OK
// packs the chosen opponents before closing the dialog.
VA(0x0043afa1, 0x581)
short NewGameHandler(tag_message& message) {
    int iPlayer;
    int i;
    int helpIndex;
    if (message.type == MESSAGE_WIDGET) {
        if (message.modifiers & MESSAGE_MODIFIER_RIGHT_BUTTON) {
            if (IS_WIDGET_SELECTION_NOTIFICATION(message.command)) {
                helpIndex = NEW_GAME_HELP_NONE;
                switch (message.id) {
                    case NEW_GAME_OK:
                        helpIndex = NEW_GAME_HELP_ACCEPT;
                        break;
                    case NEW_GAME_CANCEL:
                        helpIndex = NEW_GAME_HELP_MAIN_MENU;
                        break;
                    case NEW_GAME_KING_OF_THE_HILL:
                        helpIndex = NEW_GAME_HELP_KING_OF_THE_HILL;
                        break;
                    case NEW_GAME_SCENARIO_NAME:
                        helpIndex = NEW_GAME_HELP_SCENARIO;
                        break;
                    case 0x12:
                        helpIndex = NEW_GAME_HELP_SCENARIO;
                        break;
                    case NEW_GAME_SCENARIO_SELECT:
                        helpIndex = NEW_GAME_HELP_SCENARIO;
                        break;
                    case NEW_GAME_DIFFICULTY_FIRST:
                        helpIndex = NEW_GAME_HELP_DIFFICULTY;
                        break;
                    case NEW_GAME_DIFFICULTY_FIRST + 1:
                        helpIndex = NEW_GAME_HELP_DIFFICULTY;
                        break;
                    case NEW_GAME_DIFFICULTY_FIRST + 2:
                        helpIndex = NEW_GAME_HELP_DIFFICULTY;
                        break;
                    case NEW_GAME_DIFFICULTY_LAST:
                        helpIndex = NEW_GAME_HELP_DIFFICULTY;
                        break;
                    case NEW_GAME_OPPONENT_FIRST:
                    case NEW_GAME_OPPONENT_FIRST + 1:
                    case NEW_GAME_OPPONENT_LAST:
                        if (message.id - 1 < giNumHumanPlayers)
                            helpIndex = NEW_GAME_HELP_HUMAN_OPPONENT;
                        else
                            helpIndex = NEW_GAME_HELP_OPPONENT;
                        break;
                    case NEW_GAME_COLOR:
                        helpIndex = NEW_GAME_HELP_COLOR;
                        break;
                    case NEW_GAME_RATING:
                        helpIndex = NEW_GAME_HELP_RATING;
                        break;
                }
                if (helpIndex >= 0)
                    NormalDialog(gNewGameHelp[helpIndex], NORMAL_DIALOG_TYPE_QUICK_VIEW, -1, -1, NORMAL_DIALOG_NO_RESOURCE, 0, NORMAL_DIALOG_NO_RESOURCE, 0, NORMAL_DIALOG_NO_OR_TEXT);
            }
        } else {
            switch (message.command) {
                case WIDGET_NOTIFY_DESELECT:
                    switch (message.id) {
                        case NEW_GAME_OK:
                            gpGame->m_playerCount = 0;
                            for (i = 0; i < 4; i++) {
                                if (gpGame->m_players[i].m_difficulty > 0)
                                    gpGame->m_playerCount++;
                            }
                            if (gpGame->m_playerCount < 2) {
                                NormalDialog("A game requires at least one iPlayer.", NORMAL_DIALOG_TYPE_OK, 0xb1, 0x3c, NORMAL_DIALOG_NO_RESOURCE, 0, NORMAL_DIALOG_NO_RESOURCE, 0, NORMAL_DIALOG_NO_OR_TEXT);
                                break;
                            } else {
                                if (!gpGame->m_players[1].m_difficulty) {
                                    if (gpGame->m_players[2].m_difficulty) {
                                        gpGame->m_players[1].m_difficulty = gpGame->m_players[2].m_difficulty;
                                        gpGame->m_players[2].m_difficulty = 0;
                                    } else {
                                        gpGame->m_players[1].m_difficulty = gpGame->m_players[3].m_difficulty;
                                        gpGame->m_players[3].m_difficulty = 0;
                                    }
                                }
                                if (!gpGame->m_players[2].m_difficulty && gpGame->m_players[3].m_difficulty) {
                                    gpGame->m_players[2].m_difficulty = gpGame->m_players[3].m_difficulty;
                                    gpGame->m_players[3].m_difficulty = 0;
                                }
                            }
                        case NEW_GAME_CANCEL:
                            gpWindowManager->m_dialogResult = message.id;
                            message.command = message.id =
                                WIDGET_COMMAND_DIALOG_SELECT;
                            return MESSAGE_DISPATCH_FORWARD;
                        default:
                            break;
                    }
                    break;
                case WIDGET_NOTIFY_SELECT:
                    switch (message.id) {
                        case NEW_GAME_DIFFICULTY_FIRST:
                        case NEW_GAME_DIFFICULTY_FIRST + 1:
                        case NEW_GAME_DIFFICULTY_FIRST + 2:
                        case NEW_GAME_DIFFICULTY_LAST:
                            gpGame->m_difficulty = message.id - NEW_GAME_DIFFICULTY_FIRST;
                            break;
                        case NEW_GAME_OPPONENT_FIRST:
                        case NEW_GAME_OPPONENT_FIRST + 1:
                        case NEW_GAME_OPPONENT_LAST:
                            iPlayer = message.id - 1;
                            gpGame->m_players[iPlayer].m_difficulty++;
                            gpGame->m_players[iPlayer].m_difficulty %= 5;
                            if (giNumHumanPlayers > iPlayer && !gpGame->m_players[iPlayer].m_difficulty)
                                gpGame->m_players[iPlayer].m_difficulty = 1;
                            break;
                        case NEW_GAME_COLOR:
                            gpGame->m_players[0].m_color = (gpGame->m_players[0].m_color + 1) % 4;
                            break;
                        case NEW_GAME_KING_OF_THE_HILL:
                            gbKingOfTheHill = 1 - gbKingOfTheHill;
                            break;
                        case NEW_GAME_SCENARIO_SELECT:
                        case NEW_GAME_SCENARIO_NAME:
                        case 0x12:
                            game::GetMap();
                            break;
                        default:
                            break;
                    }
                    gpGame->UpdateNewGameWindow();
                    gpGame->m_newGameWindow->DrawWindow();
                    break;
                default:
                    break;
            }
        }
    }
    return MESSAGE_DISPATCH_CONSUME;
}

// Buka 2.1 game::UpdateNewGameWindow for HoMM1's new-game screen: map name,
// difficulty, opponent types and labels, rating, crest and King of the Hill.
VA(0x0043b522, 0x2c3)
void game::UpdateNewGameWindow(void) {
    tag_message message;
    short i;
    char* period;

    strcpy(gText, gFullMapName);
    period = strchr(gText, '.');
    if (period)
        *period = 0;
    message.type = MESSAGE_WIDGET;
    message.command = WIDGET_COMMAND_SET_TEXT;
    message.id = NEW_GAME_SCENARIO_NAME;
    message.text = gText;
    m_newGameWindow->BroadcastMessage(message);
    message.command = WIDGET_COMMAND_CLEAR_FLAGS;
    message.value = WIDGET_FLAG_DRAW;
    for (i = 0; i < 4; i++) {
        message.id = i + NEW_GAME_DIFFICULTY_FIRST;
        m_newGameWindow->BroadcastMessage(message);
    }
    message.command = WIDGET_COMMAND_SET_FLAGS;
    message.id = m_difficulty + NEW_GAME_DIFFICULTY_FIRST;
    m_newGameWindow->BroadcastMessage(message);
    message.command = WIDGET_COMMAND_SET_FRAME;
    for (i = 1; i < 4; i++) {
        message.id = i + 1;
        if (i < giNumHumanPlayers)
            message.value = 0x1a;
        else
            message.value = m_players[i].m_difficulty + 5;
        m_newGameWindow->BroadcastMessage(message);
    }
    message.command = WIDGET_COMMAND_SET_TEXT;
    for (i = 1; i < 4; i++) {
        message.id = i + 4;
        if (i < giNumHumanPlayers)
            message.text = gHumanPlayerTypeNames[m_players[i].m_difficulty];
        else
            message.text = gPlayerTypeNames[m_players[i].m_difficulty];
        m_newGameWindow->BroadcastMessage(message);
    }
    gpGame->m_difficultyRating = CalcDifficultyRating();
    message.id = NEW_GAME_RATING;
    sprintf(gText, "%s %d%%", "Difficulty Rating:", gpGame->m_difficultyRating);
    message.text = gText;
    m_newGameWindow->BroadcastMessage(message);
    message.command = WIDGET_COMMAND_SET_FRAME;
    if (m_players[0].m_color != -1) {
        message.id = NEW_GAME_COLOR;
        message.value = m_players[0].m_color * 2 + 11;
        m_newGameWindow->BroadcastMessage(message);
    }
    message.command = WIDGET_COMMAND_SET_FRAME;
    message.id = NEW_GAME_KING_OF_THE_HILL;
    message.value = gbKingOfTheHill + 27;
    m_newGameWindow->BroadcastMessage(message);
}

// Buka 2.1 game::GiveTroopsToNeutralTown inlined over every town: an
// unowned town on the map gains a random tier of its own creatures.
VA(0x0043b7e5, 0x2c3)
void game::GiveTroopsToNeutralTowns(void) {
    int howMany;
    int die;
    int i;
    int tier;
    int monster;
    for (i = 0; i < GAME_TOWN_COUNT; i++) {
        if ((m_castleRecs[i].m_x > 0 || m_castleRecs[i].m_y > 0) && m_castleRecs[i].m_owner < 0) {
            die = Random(1, 15);
            if (die <= 5) {
                tier = 10;
                howMany = Random(8, 15);
            } else if (die <= 10) {
                tier = 20;
                howMany = Random(5, 7);
            } else if (die <= 13) {
                tier = 30;
                howMany = Random(3, 5);
            } else {
                tier = 40;
                howMany = Random(1, 3);
            }
            switch (m_castleRecs[i].m_type + tier) {
                case 10:
                    monster = CREATURE_PEASANT;
                    break;
                case 20:
                    monster = CREATURE_ARCHER;
                    break;
                case 30:
                    monster = CREATURE_PIKEMAN;
                    break;
                case 40:
                    monster = CREATURE_SWORDSMAN;
                    break;
                case 12:
                    monster = CREATURE_GOBLIN;
                    break;
                case 22:
                    monster = CREATURE_ORC;
                    break;
                case 32:
                    monster = CREATURE_WOLF;
                    break;
                case 42:
                    monster = CREATURE_OGRE;
                    break;
                case 11:
                    monster = CREATURE_SPRITE;
                    break;
                case 21:
                    monster = CREATURE_DWARF;
                    break;
                case 31:
                    monster = CREATURE_ELF;
                    break;
                case 41:
                    monster = CREATURE_DRUID;
                    break;
                case 13:
                    monster = CREATURE_CENTAUR;
                    break;
                case 23:
                    monster = CREATURE_GARGOYLE;
                    break;
                case 33:
                    monster = CREATURE_GRIFFIN;
                    break;
                case 43:
                    monster = CREATURE_MINOTAUR;
                    break;
            }
            GiveArmy(&m_castleRecs[i].m_army, monster, howMany, -1);
        }
    }
}

// Buka 2.1 game::NewGame: HoMM1 starts campaigns directly, restores the
// previous setup choices and falls back to a default map when the remembered
// one does not fit the human player count.
VA(0x0043baa8, 0x3eb)
signed char game::NewGame(void) {
    if (!SetupGame(1))
        return 0;
    if (giCampaignChoice > 0) {
        InitEntireCampaign(giCampaignChoice);
        return 1;
    }
    if (gbWaitForRemoteReceive)
        return 1;
    LoadGame("origdata.bin", 1, 0);
    m_newGameWindow = new heroWindow(310, 14, "newgame.bin");
    if (!m_newGameWindow)
        MemError();
    SetWinText(m_newGameWindow, 7);
    if (gbNewGameSettingsSaved) {
        gpGame->m_difficulty = gcSavedDifficulty;
        m_players[1].m_difficulty = gcSavedPlayerTypes[1];
        m_players[2].m_difficulty = gcSavedPlayerTypes[2];
        m_players[3].m_difficulty = gcSavedPlayerTypes[3];
        gbKingOfTheHill = gbSavedKingOfTheHill;
        m_players[0].m_color = gcSavedCrest;
    }
    if (!strnicmp(gMapName, "camp", 4) || (giNumHumanPlayers == 1 && gMapName[4] != '1')
        || (giNumHumanPlayers == 2 && gMapName[5] != '2')
        || (giNumHumanPlayers == 3 && gMapName[6] != '3')
        || (giNumHumanPlayers == 4 && gMapName[7] != '4')) {
        if (giNumHumanPlayers == 1) {
            strcpy(gMapName, "AES31000.map");
            strcpy(gFullMapName, "Claw ( Easy )");
            strcpy(gMapDescription, "The Griffons will protect you until you are ready to make your move.");
            giMapSize = 0;
            giMapDifficulty = 0;
        } else {
            strcpy(gMapName, "CNM51234.map");
            strcpy(gFullMapName, "Around the Bay");
            strcpy(gMapDescription, "A large island of tight passes with a circular feel.");
            giMapSize = 1;
            giMapDifficulty = 1;
        }
    }
    UpdateNewGameWindow();
    gpMouseManager->ReallyShowPointer();
    gpWindowManager->DoDialog(m_newGameWindow, NewGameHandler, 0);
    delete m_newGameWindow;
    if (gpWindowManager->m_dialogResult == DIALOG_BUTTON_1)
        return 0;
    strcpy(m_mapName, gFullMapName);
    strcpy(m_mapDescription, gMapDescription);
    m_mapSize = giMapSize;
    m_mapDifficulty = giMapDifficulty;
    strcpy(m_mapName, gFullMapName);
    gbNewGameSettingsSaved = 1;
    gcSavedDifficulty = gpGame->m_difficulty;
    gcSavedPlayerTypes[1] = m_players[1].m_difficulty;
    gcSavedPlayerTypes[2] = m_players[2].m_difficulty;
    gcSavedPlayerTypes[3] = m_players[3].m_difficulty;
    gbSavedKingOfTheHill = gbKingOfTheHill;
    gcSavedCrest = m_players[0].m_color;
    NewMap(gMapName);
    return 1;
}

// HoMM1 identity: advManager::ControlPanel calls it on gpGame with three
// arguments and the callee returns with `ret 0xc` (Buka game::ShowCampaignInfo).

VA(0x0043be93, 0x2ad)
void game::ShowCampaignInfo(int scenario, int fromMenu, int) {
    heroWindow* window;
    tag_message message;

    gpMouseManager->SetPointer("advmice.mse", ADVENTURE_POINTER_DEFAULT);
    window = new heroWindow(105, 96, "campaign.bin");
    if (!window)
        MemError();
    message.type = MESSAGE_WIDGET;
    message.command = WIDGET_COMMAND_SET_TEXT;
    message.id = 1;
    strcpy(gText, gCampaignScenarioNames[scenario]);
    message.text = gText;
    window->BroadcastMessage(message);
    message.id = 2;
    strcpy(gText, gCampaignScenarioText[scenario]);
    message.text = gText;
    window->BroadcastMessage(message);
    message.text = gText;
    window->BroadcastMessage(message);
    message.type = MESSAGE_WIDGET;
    message.id = 3;
    message.command = WIDGET_COMMAND_SET_FRAME;
    message.value = gpGame->m_campaignScenariosWon + 4;
    window->BroadcastMessage(message);
    message.type = MESSAGE_WIDGET;
    message.command = WIDGET_COMMAND_CLEAR_FLAGS;
    message.value = WIDGET_FLAG_ENABLED | WIDGET_FLAG_DRAW;
    if (fromMenu) {
        message.id = DIALOG_BUTTON_2;
        window->BroadcastMessage(message);
    } else {
        message.id = DIALOG_BUTTON_0;
        window->BroadcastMessage(message);
        message.id = 0x385;
        window->BroadcastMessage(message);
    }
    if (!fromMenu)
        gpSoundManager->SwitchAmbientMusic(MUSIC_TRACK_MAIN_MENU);
    gpWindowManager->DoDialog(window, EventWindowHandler, 0);
    delete window;
    if (gpWindowManager->m_dialogResult == 0x385) {
        NormalDialog("Are you sure you want to restart this scenario?", NORMAL_DIALOG_TYPE_YES_NO, -1, -1, NORMAL_DIALOG_NO_RESOURCE, 0, NORMAL_DIALOG_NO_RESOURCE, 0, NORMAL_DIALOG_NO_OR_TEXT);
        if (gpWindowManager->m_dialogResult == NORMAL_DIALOG_CONFIRM) {
            InitCampaignMap(m_campaignScenario, 0);
            gpAdvManager->m_routeShown = 0;
            giBottomViewOverride = 0;
            gpWindowManager->FadeScreen(WINDOW_FADE_OUT, WINDOW_MANAGER_DIALOG_FADE_STEP, gPalette);
            gpAdvManager->SetInitialMapOrigin();
            gpAdvManager->RedrawAdvScreen(1);
            gpWindowManager->FadeScreen(WINDOW_FADE_IN, WINDOW_MANAGER_DIALOG_FADE_STEP, gPalette);
        }
    }
}

// Buka 2.1 game::InitEntireCampaign; HoMM1 reloads origdata.bin first and
// starts the campaign calendar on day 1.
VA(0x0043c140, 0x7f)
void game::InitEntireCampaign(int side) {
    LoadGame("origdata.bin", 1, 0);
    strcpy(gFullMapName, "");
    gpGame->m_difficulty = 3;
    m_campaignType = side;
    m_campaignScenario = 0;
    m_campaignScenariosWon = 0;
    m_campaignDay = 1;
    InitCampaignMap(m_campaignScenario, 0);
}

// Buka 2.1 game::InitCampaignMap reduced to HoMM1's CAMP%d.CMP maps: the
// calendar continues from m_campaignDay and the scenario table seeds the
// opponents and every player's resources.
VA(0x0043c1bf, 0x28c)
void game::InitCampaignMap(int scenario, int) {
    int saveType;
    int savedScenario;
    int i;
    int j;
    int savedState;
    int savedDay;

    saveType = m_campaignType;
    savedScenario = m_campaignScenario;
    savedState = m_campaignScenariosWon;
    savedDay = m_campaignDay;
    LoadGame("origdata.bin", 1, 0);
    m_campaignType = saveType;
    m_campaignScenario = savedScenario;
    m_campaignScenariosWon = savedState;
    m_campaignDay = savedDay;
    m_month = (m_campaignDay - 1) / 28 + 1;
    m_week = (m_campaignDay - 1 - (m_month - 1) * 28) / 7 + 1;
    m_day = (m_campaignDay - 1) % 7 + 1;
    giCurTurn = (m_month - 1) * 28 + (m_week - 1) * 7 + m_day;
    gbKingOfTheHill = gCampaignScenarios[scenario].kingOfTheHill;
    giNumHumanPlayers = 0;
    m_players[0].m_difficulty = 4;
    m_players[0].m_color = gCampaignSideCrests[m_campaignType - 1][0];
    m_playerCount = 1;
    for (i = 1; i < 4; i++) {
        m_players[i].m_difficulty = gCampaignScenarios[scenario].playerTypes[i];
        if (m_players[i].m_difficulty)
            m_playerCount++;
    }
    giNumHumanPlayers = 1;
    sprintf(gMapName, "CAMP%d.CMP", scenario + 1);
    NewMap(gMapName);
    for (i = 0; i < 4; i++) {
        for (j = 0; j < 7; j++)
            m_players[i].m_resources[j] = gCampaignScenarios[scenario].resources[i][j];
    }
}

// Buka 2.1 game::NewMap for HoMM1: map setup helpers, a starting town and
// hero per player (campaign crests pick them), two tavern heroes, the
// ultimate artifact site, starting resources, town threat ranks and the
// first neutral garrisons.
VA(0x0043c44b, 0x1009)
void game::NewMap(char* mapName) {
    int nextThreat;
    int anyFree;
    signed char heroY;
    signed char heroX;
    signed char yTown;
    signed char xTown;
    int heroIdx;
    int i;
    int j;
    signed char townId;
    signed char used[GAME_TOWN_COUNT];
    int k;
    int ultimateSpread;
    signed char allNeutral;
    int difficulty;

    gbInNewGameSetup = 1;
    giCurPlayer = 0;
    gpCurPlayer = &gpGame->m_players[giCurPlayer];
    giCurPlayerBit = 1 << giCurPlayer;
    giCurWatchPlayerBit = giCurPlayerBit;
    giCurPlayerHighBit = 1 << (giCurPlayer + 4);
    giCurWatchPlayerHighBit = 1 << (giCurPlayer + 4);
    giCurWatchPlayer = giCurPlayer;
    for (i = 0; i < m_playerCount; i++) {
        m_players[i].m_townCount = 0;
        m_players[i].m_townLocatorPage = 0;
        m_players[i].m_currentTown = -1;
        m_players[i].m_heroCount = 0;
        m_players[i].m_heroLocatorPage = 0;
        m_players[i].m_currentHero = -1;
    }
    memset(m_mapExtra, 0, sizeof(m_mapExtra));
    memset(mapVisited, 0, sizeof(mapVisited));
    RandomizeHeroPool();
    strcpy(gMapName, mapName);
    LoadMap(gMapName);
    RandomizeTerrainTiles();
    RandomizePlayerCrests();
    ProcessMapExtra();
    allNeutral = SetupTowns();
    ProcessRandomObjects(1);
    ProcessRandomObjects(0);
    RandomizeEvents();
    m_deadPlayerCount = 0;
    for (i = m_playerCount; i < GAME_PLAYER_COUNT; i++)
        m_playerDead[i] = 1;
    for (i = 0; i < m_playerCount; i++) {
        m_players[i].m_ultimateArtifactHintChance = 0;
        m_players[i].m_ultimateArtifactHintX = -1;
        m_players[i].m_ultimateArtifactHintY = -1;
        heroIdx = 0;
        if (allNeutral) {
            if (m_campaignType <= 0 || m_campaignScenario < 4 || m_campaignScenario > 7) {
                if (m_campaignType > 0) {
                    for (j = 0; j < 4; j++) {
                        if (gCrestTownTypes[m_players[i].m_color] == GetTown(j)->m_type) {
                            SetupTown(j, !gbHumanPlayer[i]);
                            ClaimTown(j, i);
                        }
                    }
                } else {
                    townId = RandomScan(m_townOwners, 0, 4, 8);
                    if (townId == -1)
                        townId = Scan(m_townOwners, 0, 4);
                    SetupTown(townId, !gbHumanPlayer[i]);
                    ClaimTown(townId, i);
                }
            }
        } else {
            for (j = 0; j < GAME_TOWN_COUNT; j++) {
                if (m_castleRecs[j].m_owner == i)
                    SetupTown(j, !gbHumanPlayer[i]);
            }
        }
        if (m_noMapHeroes
            || (m_campaignType > 0 && m_campaignScenario >= 4 && m_campaignScenario <= 7 && i == 0)) {
            m_players[i].m_heroCount = 1;
            if (m_campaignType > 0)
                m_players[i].m_heroIds[0] = GetNewHeroId(gCrestHeroClass[m_players[i].m_color]);
            else
                m_players[i].m_heroIds[0] =
                    GetNewHeroId(gTownHeroClass[m_castleRecs[m_players[i].m_townIds[0]].m_type]);
            m_availableHeroes[m_players[i].m_heroIds[0]] = i;
            m_heroRecs[m_players[i].m_heroIds[0]].m_owner = i;
            m_heroRecs[m_players[i].m_heroIds[0]].m_x = m_castleRecs[m_players[i].m_townIds[0]].m_x;
            m_heroRecs[m_players[i].m_heroIds[0]].m_y = m_castleRecs[m_players[i].m_townIds[0]].m_y;
            m_castleRecs[m_players[i].m_townIds[0]].m_occupyingHeroId = m_players[i].m_heroIds[0];
            SetVisibility(
                m_heroRecs[m_players[i].m_heroIds[0]].m_x,
                m_heroRecs[m_players[i].m_heroIds[0]].m_y,
                i,
                gHeroScoutRadius[m_heroRecs[m_players[i].m_heroIds[0]].m_heroClass]
            );
        }
        if (m_campaignType > 0)
            k = gCrestHeroClass[m_players[i].m_color];
        else
            k = Random(0, 3);
        m_players[i].m_availableHeroIds[0] = GetNewHeroId(k);
        m_availableHeroes[m_players[i].m_availableHeroIds[0]] = 0x40;
        k = (Random(1, 3) + k) % 4;
        m_players[i].m_availableHeroIds[1] = GetNewHeroId(k);
        m_availableHeroes[m_players[i].m_availableHeroIds[1]] = 0x40;
    }
    if (!m_noMapHeroes)
        ProcessOnMapHeroes();
    if (m_campaignType <= 0) {
        for (k = 0; k < 4; k++) {
            if (allNeutral && m_townOwners[k] == -1) {
                xTown = m_castleRecs[k].m_x;
                yTown = m_castleRecs[k].m_y;
                for (i = 0; i < 4; i++) {
                    m_map[xTown - 2 + i][yTown - 2].m_overlayIndex -= 12;
                    m_map[xTown - 2 + i][yTown - 1].m_objectIndex -= 12;
                    m_map[xTown - 2 + i][yTown].m_objectIndex -= 12;
                }
                m_castleRecs[k].m_buildings = 0x20;
                if (m_castleRecs[k].m_type == 2)
                    m_castleRecs[k].m_buildings |= 0x2000;
                SetupTown(k, 0);
            }
        }
    }
    for (i = 0; i < m_playerCount; i++) {
        for (j = 0; j < m_players[i].m_heroCount; j++) {
            heroX = m_heroRecs[m_players[i].m_heroIds[j]].m_x;
            heroY = m_heroRecs[m_players[i].m_heroIds[j]].m_y;
            m_heroRecs[m_players[i].m_heroIds[j]].m_locationType = m_map[heroX][heroY].m_triggerType;
            m_heroRecs[m_players[i].m_heroIds[j]].m_occupiedTown = m_map[heroX][heroY].m_objectMetadata;
            m_map[heroX][heroY].m_triggerType = (MAP_TRIGGER_EVENT | MAP_OBJECT_HERO);
            m_map[heroX][heroY].m_objectMetadata = m_players[i].m_heroIds[j];
        }
        if (m_players[i].m_heroCount > 0)
            m_players[i].m_currentHero = m_players[i].m_heroIds[0];
        else if (m_players[i].m_townCount > 0)
            m_players[i].m_currentTown = m_players[i].m_townIds[0];
    }
    i = Random(9, 62);
    j = Random(9, 62);
    ultimateSpread = Random(1, 20) + Random(1, 20) + Random(1, 30);
    while (m_map[i][j].m_objectIndex != 0xff || m_map[i][j].m_overlayIndex != 0xff || m_map[i][j].m_tileIndex < 20
           || (giNumHumanPlayers == 1
               && abs(i - m_heroRecs[m_players[0].m_heroIds[0]].m_x)
                          + abs(j - m_heroRecs[m_players[0].m_heroIds[0]].m_y)
                      <= ultimateSpread)) {
        ultimateSpread = Random(1, 20) + Random(1, 20) + Random(1, 30);
        i = Random(9, 62);
        j = Random(9, 62);
    }
    m_ultimateArtifactX = i;
    m_ultimateArtifactY = j;
    m_ultimateArtifactId = Random(0, 3);
    for (i = 0; i < m_playerCount; i++) {
        if (gbHumanPlayer[i]) {
            if (i == 0)
                difficulty = m_difficulty;
            else
                difficulty = m_players[i].m_difficulty - 1;
        } else {
            difficulty = 0;
        }
        memcpy(m_players[i].m_resources, gStartingResources[difficulty], sizeof(m_players[i].m_resources));
    }
    memset(used, -1, sizeof(used));
    for (i = 0; i < GAME_TOWN_COUNT; i++) {
        nextThreat = 0;
        anyFree = Scan(used, 0, GAME_TOWN_COUNT);
        if (anyFree != -1)
            nextThreat = RandomScan(used, 0, GAME_TOWN_COUNT, GAME_TOWN_COUNT);
        anyFree = nextThreat;
        m_castleRecs[i].m_threat = anyFree;
        used[anyFree] = 0;
    }
    for (i = 0; i < 4; i++)
        GiveTroopsToNeutralTowns();
    SetupAdjacentMons();
    gpPhilAI->GetGameAIVars();
    gbInNewGameSetup = 0;
}

// HoMM1 groups the multi-cell object triggers 0x34-0x37 and 0x38-0x3c by
// their first trigger so neighbouring halves can be compared.
VA(0x0043d454, 0x6f)
int GetObjectFamily(int trigger) {
    switch (trigger) {
        case MAP_OBJECT_MOUNTAINS:
        case MAP_OBJECT_MOUNTAINS_2:
        case MAP_OBJECT_MOUNTAINS_3:
        case MAP_OBJECT_MOUNTAINS_4:
            return MAP_OBJECT_MOUNTAINS;
        case MAP_OBJECT_TREES:
        case MAP_OBJECT_TREES_2:
        case MAP_OBJECT_TREES_3:
        case MAP_OBJECT_TREES_4:
        case MAP_OBJECT_TREES_5:
            return MAP_OBJECT_TREES;
        default:
            return trigger;
    }
}

// HoMM1: once a cell's object frame is gone, its overlay drops into the
// object slot unless the eastern neighbour continues the same object.
VA(0x0043d4c3, 0x1e4)
void game::SettleOverlay(int x, int y) {
    mapCell* cell;
    mapCell* cellEast;
    cell = &m_map[x][y];
    if (cell->m_objectIndex == 0xff && cell->m_overlayIndex != 0xff) {
        switch (cell->m_triggerType) {
            case MAP_OBJECT_MOUNTAINS_2:
            case MAP_OBJECT_TREES_2:
                if (x + 1 < MAP_CELL_GRID_SIZE) {
                    cellEast = &m_map[x + 1][y];
                    if (GetObjectFamily(cellEast->m_triggerType) == GetObjectFamily(cell->m_triggerType)) {
                        cell->m_secondaryTrigger |= 0x80;
                    } else {
                        cell->m_objectIndex = cell->m_overlayIndex;
                        cell->m_objectTileset = cell->m_overlayTileset;
                        cell->m_overlayTileset = 0;
                        cell->m_overlayIndex = 0xff;
                    }
                }
                break;
            case MAP_OBJECT_MOUNTAINS_4:
            case MAP_OBJECT_TREES_4:
                if (x + 1 < MAP_CELL_GRID_SIZE) {
                    cellEast = &m_map[x + 1][y];
                    if (GetObjectFamily(cellEast->m_triggerType) == GetObjectFamily(cell->m_triggerType)) {
                        cell->m_objectIndex = cell->m_overlayIndex;
                        cell->m_objectTileset = cell->m_overlayTileset;
                        cell->m_overlayTileset = 0;
                        cell->m_overlayIndex = 0xff;
                    } else {
                        cell->m_secondaryTrigger |= 0x80;
                    }
                }
                break;
            default:
                break;
        }
    }
}

// Buka 2.1 game::RandomizeEvents for HoMM1's map objects: numbers sites
// and obelisks, rolls each event's contents, files town and mine ids into
// their footprints, then settles overlays and the passive trigger bits.
VA(0x0043d6a7, 0xc63)
void game::RandomizeEvents(void) {
    unsigned char overlayTileset;
    unsigned char objTileset;
    short y;
    short i;
    short j;
    signed char id;
    short x;
    mapCell* cell;
    int siteNum;
    signed char obeliskId;

    obeliskId = 1;
    siteNum = 1;
    for (y = 0; y < MAP_CELL_GRID_SIZE; y++) {
        for (x = 0; x < MAP_CELL_GRID_SIZE; x++) {
            cell = &m_map[x][y];
            switch (cell->m_triggerType) {
                case MAP_TRIGGER_EVENT | MAP_OBJECT_GAZEBO:
                    cell->m_objectMetadata = siteNum;
                    siteNum++;
                    break;
                case MAP_OBJECT_WHIRLPOOL:
                    cell->m_triggerType |= MAP_TRIGGER_EVENT;
                    break;
                case MAP_TRIGGER_EVENT | MAP_OBJECT_OBELISK:
                    cell->m_objectMetadata = obeliskId;
                    obeliskId++;
                    break;
                case MAP_TRIGGER_EVENT | MAP_OBJECT_STATUE:
                    cell->m_objectMetadata = 1;
                    break;
                case MAP_TRIGGER_EVENT | MAP_OBJECT_SKELETON:
                    if (Random(0, 9) == 3)
                        cell->m_objectMetadata = 2;
                    else
                        cell->m_objectMetadata = 1;
                    break;
                case MAP_TRIGGER_EVENT | MAP_OBJECT_DAEMON_CAVE:
                    switch (Random(0, 99) % 10) {
                        case 0:
                        case 1:
                        case 2:
                            cell->m_objectMetadata = 2;
                            break;
                        case 3:
                            cell->m_objectMetadata = 3;
                            break;
                        case 4:
                        case 5:
                        case 6:
                            cell->m_objectMetadata = 4;
                            break;
                        case 7:
                        case 8:
                        case 9:
                            cell->m_objectMetadata = 5;
                            break;
                    }
                    break;
                case MAP_TRIGGER_EVENT | MAP_OBJECT_TREASURE_CHEST:
                    cell->m_objectMetadata = Random(2, 4);
                    break;
                case MAP_TRIGGER_EVENT | MAP_OBJECT_CAMPFIRE:
                    cell->m_objectMetadata = Random(4, 6) << 4;
                    cell->m_objectMetadata |= static_cast<signed char>(Random(0, 5));
                    break;
                case MAP_TRIGGER_EVENT | MAP_OBJECT_ANCIENT_LAMP:
                    cell->m_objectMetadata = Random(0, 3) + 2;
                    break;
                case MAP_TRIGGER_EVENT | MAP_OBJECT_SHIPWRECK:
                    if (x <= 0 || x >= MAP_CELL_GRID_SIZE - 1 || (m_map[x - 1][y].m_triggerType & MAP_TRIGGER_TYPE_MASK) != 0x23) {
                        cell->m_triggerType &= MAP_TRIGGER_TYPE_MASK;
                        break;
                    }
                    goto treasure;
                case MAP_TRIGGER_EVENT | MAP_OBJECT_GRAVEYARD:
                treasure:
                    switch (Random(0, 99) % 10) {
                        case 0:
                        case 1:
                        case 2:
                            cell->m_objectMetadata = 2;
                            break;
                        case 3:
                        case 4:
                        case 5:
                            cell->m_objectMetadata = 3;
                            break;
                        case 6:
                        case 7:
                        case 8:
                            cell->m_objectMetadata = 4;
                            break;
                        case 9:
                            cell->m_objectMetadata = 5;
                            break;
                    }
                    break;
                case MAP_TRIGGER_EVENT | MAP_OBJECT_STRAW_HUT:
                    cell->m_objectMetadata = Random(10, 30);
                    break;
                case MAP_TRIGGER_EVENT | MAP_OBJECT_HOUSE:
                    cell->m_objectMetadata = Random(20, 50);
                    break;
                case MAP_TRIGGER_EVENT | MAP_OBJECT_CABIN:
                    cell->m_objectMetadata = Random(0, 127) % 4 + 1;
                    break;
                case MAP_TRIGGER_EVENT | MAP_OBJECT_DWARF_LOG_CABIN:
                    cell->m_objectMetadata = Random(0, 98) % 3 + 1;
                    break;
                case MAP_TRIGGER_EVENT | MAP_OBJECT_PEASANT_LOG_CABIN:
                    cell->m_objectMetadata = Random(20, 50);
                    break;
                case MAP_TRIGGER_EVENT | MAP_OBJECT_WATERWHEEL:
                    cell->m_objectMetadata = 1;
                    break;
                case MAP_TRIGGER_EVENT | MAP_OBJECT_MONSTER:
                    if (!static_cast<unsigned char>(cell->m_objectMetadata)) {
                        cell->m_objectMetadata = GetRandomNumTroops(cell->m_objectIndex);
                        if (Random(0, 99) <= 25 && cell->m_objectIndex != 26)
                            cell->m_objectMetadata = static_cast<unsigned char>(cell->m_objectMetadata) | 0x80;
                    }
                    break;
                case MAP_TRIGGER_EVENT | MAP_OBJECT_RESOURCE:
                    cell->m_objectMetadata = cell->m_objectIndex;
                    if (cell->m_objectIndex > 4)
                        cell->m_objectMetadata = static_cast<unsigned char>(cell->m_objectMetadata) - 61;
                    switch (static_cast<unsigned char>(cell->m_objectMetadata)) {
                        case 0:
                        case 2:
                            cell->m_objectMetadata = Random(8, 16);
                            break;
                        case 6:
                            cell->m_objectMetadata = Random(5, 10);
                            break;
                        default:
                            cell->m_objectMetadata = Random(3, 7);
                            break;
                    }
                    break;
                case MAP_TRIGGER_EVENT | MAP_OBJECT_SPELL_SHRINE:
                    switch (Random(0, 9)) {
                        case 0:
                        case 1:
                        case 2:
                        case 3:
                            cell->m_objectMetadata = gMageGuildSpellPool[0][Random(0, 7)] + 1;
                            break;
                        case 4:
                        case 5:
                        case 6:
                        case 7:
                            cell->m_objectMetadata = gMageGuildSpellPool[1][Random(0, 7)] + 1;
                            break;
                        default:
                            cell->m_objectMetadata = gMageGuildSpellPool[2][Random(0, 7)] + 1;
                            break;
                    }
                    break;
                case MAP_TRIGGER_EVENT | MAP_OBJECT_DESERT_TENT:
                    cell->m_objectMetadata = Random(10, 20);
                    break;
                case MAP_TRIGGER_EVENT | MAP_OBJECT_WAGON_CAMP:
                    if (x <= 0 || x >= MAP_CELL_GRID_SIZE - 1 || (m_map[x - 1][y].m_triggerType & MAP_TRIGGER_TYPE_MASK) != 0x2a
                        || (m_map[x + 1][y].m_triggerType & MAP_TRIGGER_TYPE_MASK) != 0x2a) {
                        cell->m_triggerType &= MAP_TRIGGER_TYPE_MASK;
                        break;
                    }
                    cell->m_objectMetadata = Random(30, 50);
                    break;
                case MAP_TRIGGER_EVENT | MAP_OBJECT_ARTIFACT:
                    switch (Random(0, 99) % 10) {
                        case 0:
                        case 1:
                        case 2:
                        case 3:
                        case 4:
                        case 5:
                            cell->m_objectMetadata = 1;
                            break;
                        case 6:
                        case 7:
                            cell->m_objectMetadata = 2;
                            break;
                        case 8:
                        case 9:
                            cell->m_objectMetadata = 3;
                            break;
                    }
                    break;
                case MAP_TRIGGER_EVENT | MAP_OBJECT_TOWN:
                    id = GetTownId(x, y);
                    for (j = 0; j < 3; j++) {
                        for (i = 0; i < 4; i++) {
                            if (!static_cast<unsigned char>(m_map[x - 2 + i][y - 2 + j].m_objectMetadata))
                                m_map[x - 2 + i][y - 2 + j].m_objectMetadata = id;
                        }
                    }
                    SetupTown(id, 0);
                    break;
                case MAP_TRIGGER_EVENT | MAP_OBJECT_ALCHEMIST_LAB:
                case MAP_TRIGGER_EVENT | MAP_OBJECT_MINE:
                case MAP_TRIGGER_EVENT | MAP_OBJECT_SAWMILL:
                    id = GetMineId(x, y);
                    for (j = 0; j < 2; j++) {
                        for (i = 0; i < 2; i++) {
                            if (!static_cast<unsigned char>(m_map[x + i][y - j].m_objectMetadata)
                                || (cell->m_triggerType & MAP_TRIGGER_TYPE_MASK) == (m_map[x + i][y - j].m_triggerType & MAP_TRIGGER_TYPE_MASK))
                                m_map[x + i][y - j].m_objectMetadata = id;
                        }
                    }
                    break;
                case MAP_TRIGGER_EVENT | MAP_OBJECT_WINDMILL:
                    cell->m_objectMetadata = Random(1, 5);
                    break;
            }
        }
    }
    for (y = 0; y < MAP_CELL_GRID_SIZE; y++) {
        for (x = 0; x < MAP_CELL_GRID_SIZE; x++) {
            cell = &m_map[x][y];
            if (cell->m_objectIndex != 0xff && cell->m_overlayIndex != 0xff) {
                objTileset = cell->m_objectTileset & 0xf;
                overlayTileset = cell->m_overlayTileset & 0xf;
                if ((objTileset == 8 || objTileset == 9) && (overlayTileset == 8 || overlayTileset == 9))
                    cell->m_secondaryTrigger |= 0x80;
            }
            SettleOverlay(x, y);
            if (x == 0 || y == 0 || x == MAP_CELL_GRID_SIZE - 1 || y == MAP_CELL_GRID_SIZE - 1) {
                switch (cell->m_triggerType) {
                    case MAP_OBJECT_MOUNTAINS:
                    case MAP_OBJECT_MOUNTAINS_2:
                    case MAP_OBJECT_MOUNTAINS_3:
                    case MAP_OBJECT_MOUNTAINS_4:
                    case MAP_OBJECT_TREES:
                    case MAP_OBJECT_TREES_2:
                    case MAP_OBJECT_TREES_3:
                    case MAP_OBJECT_TREES_4:
                    case MAP_OBJECT_TREES_5:
                        cell->m_secondaryTrigger |= 0x80;
                        break;
                }
            }
        }
    }
    for (y = 0; y < MAP_CELL_GRID_SIZE; y++) {
        for (x = 0; x < MAP_CELL_GRID_SIZE; x++) {
            cell = &m_map[x][y];
            if (cell->m_triggerType == 0x32)
                cell->m_flags |= 0x80;
            if (cell->m_triggerType & MAP_TRIGGER_EVENT) {
                switch (cell->m_triggerType & MAP_TRIGGER_TYPE_MASK) {
                    case 0x1:
                    case MAP_OBJECT_SIGNPOST:
                    case MAP_OBJECT_BUOY:
                    case MAP_OBJECT_SKELETON:
                    case MAP_OBJECT_DAEMON_CAVE:
                    case MAP_OBJECT_TREASURE_CHEST:
                    case MAP_OBJECT_FAERIE_RING:
                    case MAP_OBJECT_CAMPFIRE:
                    case MAP_OBJECT_FOUNTAIN:
                    case MAP_OBJECT_GAZEBO:
                    case MAP_OBJECT_ANCIENT_LAMP:
                    case MAP_OBJECT_GRAVEYARD:
                    case MAP_OBJECT_STRAW_HUT:
                    case MAP_OBJECT_HOUSE:
                    case MAP_OBJECT_CABIN:
                    case MAP_OBJECT_DWARF_LOG_CABIN:
                    case MAP_OBJECT_PEASANT_LOG_CABIN:
                    case MAP_OBJECT_INN_1:
                    case MAP_OBJECT_INN_2:
                    case MAP_OBJECT_INN_3:
                    case MAP_OBJECT_INN_4:
                    case MAP_OBJECT_DRAGON_CITY:
                    case MAP_OBJECT_LIGHTHOUSE:
                    case MAP_OBJECT_WATERWHEEL:
                    case MAP_OBJECT_MINE:
                    case MAP_OBJECT_MONSTER:
                    case MAP_OBJECT_OBELISK:
                    case MAP_OBJECT_OASIS:
                    case MAP_OBJECT_RESOURCE:
                    case MAP_OBJECT_COAST:
                    case MAP_OBJECT_SAWMILL:
                    case MAP_OBJECT_RANKING_SHRINE:
                    case MAP_OBJECT_SPELL_SHRINE:
                    case MAP_OBJECT_SHIPWRECK:
                    case MAP_OBJECT_STATUE:
                    case MAP_OBJECT_SWAN_POND:
                    case MAP_OBJECT_DESERT_TENT:
                    case MAP_OBJECT_TOWN:
                    case MAP_OBJECT_STONE_LITHS:
                    case MAP_OBJECT_WAGON_CAMP:
                    case MAP_OBJECT_WELL:
                    case MAP_OBJECT_WHIRLPOOL:
                    case MAP_OBJECT_WINDMILL:
                    case MAP_OBJECT_MEGALITH:
                    case MAP_OBJECT_ARTIFACT:
                    case MAP_OBJECT_NOTHING_HERE:
                    case MAP_OBJECT_HERO:
                    case MAP_OBJECT_SHIP:
                    case 0x3f:
                        break;
                    default:
                        cell->m_triggerType -= MAP_TRIGGER_EVENT;
                        break;
                }
            }
        }
    }
}

// Buka 2.1 game::LoadMap for HoMM1's .MAP files: an optional old header,
// the world map, town and mine records, artifacts, obelisks, sounds and
// (from version 1112) the map extras.
VA(0x0043e30a, 0x43a)
short game::LoadMap(char* filename) {
    void* buf;
    short width;
    signed char y;
    short height;
    short i;
    int handle;
    signed char x;
    signed char type;
    int unused;
    short version;

    sprintf(gText, "%s%s", ".\\MAPS\\", filename);
    handle = open(gText, O_BINARY);
    if (handle == -1)
        FileError(gText);
    read(handle, &version, 2);
    if (version == 1000) {
        buf = malloc(0x554);
        read(handle, buf, 0x552);
        read(handle, &version, 2);
        free(buf);
    }
    read(handle, &width, 2);
    read(handle, &height, 2);
    ReadWorldMap(handle);
    for (i = 0; i < GAME_TOWN_COUNT; i++) {
        read(handle, &x, 1);
        read(handle, &y, 1);
        read(handle, &type, 1);
        if (x >= 0) {
            m_castleRecs[i].m_x = x;
            m_castleRecs[i].m_y = y;
            m_castleRecs[i].m_type = type & 0x7f;
            if ((type & 0x7f) == 2)
                m_castleRecs[i].m_buildings |= 0x2000;
            if (type < 0)
                m_castleRecs[i].m_buildings |= (1 << BUILDING_SLOT_CASTLE);
            else
                m_castleRecs[i].m_buildings |= (1 << BUILDING_SLOT_TENT);
        }
    }
    for (i = 0; i < GAME_MINE_COUNT; i++) {
        read(handle, &x, 1);
        read(handle, &y, 1);
        read(handle, &type, 1);
        if (x >= 0) {
            m_mines[i].x = x;
            m_mines[i].y = y;
            m_mines[i].type = type;
        }
    }
    read(handle, m_randomArtifacts, sizeof(m_randomArtifacts));
    read(handle, &m_obeliskCount, 1);
    read(handle, m_mapSounds, sizeof(m_mapSounds));
    if (version >= 1112) {
        read(handle, &iMaxMapExtra, 4);
        for (i = 1; i < iMaxMapExtra; i++) {
            read(handle, &pwSizeOfMapExtra[i], 4);
            ppMapExtra[i] = malloc(pwSizeOfMapExtra[i]);
            read(handle, ppMapExtra[i], pwSizeOfMapExtra[i]);
        }
    } else {
        iMaxMapExtra = 1;
    }
    close(handle);
    return 0;
}

// donor PoL RVA 0x00078fea; preferred Buka symbol ?ClaimTown@game@@QAEXHHH@Z
// donor Buka TU SOURCE/GAME; HoMM1 owner inferred from contiguous order
// evidence: graph:4;base=0.415111;margin=0.758393;shape=0.164;size=0.968;calls=0.500;alternate=pol20:void game::ClaimTown(int, int, int)@0x00078fea
VA(0x0043e744, 0x321)
void game::ClaimTown(signed char townId, signed char player) {
    int i;
    town* townRec;
    mapCell* cell;

    townRec = &m_castleRecs[townId];
    if (townRec->m_owner == player)
        return;
    if (m_townOwners[townId] != -1)
        gpGame->GetTown(townId)->Deallocate();
    for (i = 0; i < ARMY_GROUP_SLOT_COUNT; ++i) {
        townRec->m_army.m_creatureTypes[i] = CREATURE_NONE;
        townRec->m_army.m_creatureCounts[i] = 0;
    }
    if (m_castleRecs[townId].m_owner == -1)
        m_castleRecs[townId].m_turnsOwned = 2;
    else
        m_castleRecs[townId].m_turnsOwned = 0;
    m_castleRecs[townId].m_owner = player;
    m_townOwners[townId] = player;
    m_players[player].m_townIds[m_players[player].m_townCount] = townId;
    m_players[player].m_townCount++;

    cell = &m_map[m_castleRecs[townId].m_x - 1][m_castleRecs[townId].m_y];
    cell->m_flags |= 0x10;
    cell->m_objectTileset |= 0xe0;
    cell->m_extraFrame = m_players[player].Color() * 2;
    cell = &m_map[m_castleRecs[townId].m_x + 1][m_castleRecs[townId].m_y];
    cell->m_flags |= 0x10;
    cell->m_objectTileset |= 0xe0;
    cell->m_extraFrame = m_players[player].Color() * 2 + 1;
    SetVisibility(m_castleRecs[townId].m_x, m_castleRecs[townId].m_y, player, giVisRangeTown);
    CheckEndGame(0);
}

// Buka 2.1 game::ClaimMine reduced to HoMM1's flag placement: the flag cell
// sits beside the mine by type and shows the owner's colour frame.
VA(0x0043ea65, 0x2c9)
void game::ClaimMine(signed char mineId, signed char player) {
    short frame;
    mapCell* cell;
    m_mines[mineId].owner = player;
    m_mineOwners[mineId] = player;
    switch (m_mines[mineId].type) {
        case 0x16:
            frame = 0x14;
            break;
        case 0x17:
            frame = 0x18;
            break;
        case 0:
            frame = 0x10;
            break;
        case 1:
            frame = 0xc;
            break;
        default:
            frame = 8;
            break;
    }
    switch (m_mines[mineId].type) {
        case 1:
            cell = &m_map[m_mines[mineId].x][m_mines[mineId].y - 2];
            break;
        case 0x16:
            cell = &m_map[m_mines[mineId].x - 1][m_mines[mineId].y - 3];
            break;
        case 0x17:
            cell = &m_map[m_mines[mineId].x - 2][m_mines[mineId].y];
            break;
        default:
            cell = &m_map[m_mines[mineId].x][m_mines[mineId].y - 1];
            break;
    }
    if (player == -1) {
        cell->m_flags ^= 0x20;
    } else {
        cell->m_flags |= 0x20;
        cell->m_overlayTileset |= 0xe0;
        cell->m_extraFrame = m_players[player].Color() + frame;
    }
}

// Buka 2.1 game::ViewSpells for HoMM1's spell book: combat (0) and
// adventure (1) books each have their own window position; type 2 shows
// both tabs.
VA(0x0043ed2e, 0x297)
signed char game::ViewSpells(
    class hero* spellHero,
    signed char spellType,
    short (*callback)(struct tag_message&),
    signed char readOnly
) {
    tag_message message;

    m_viewSpell = SPELL_NONE;
    short winX[3] = {177, 97, 177};
    short winY[3] = {100, 47, 100};
    if (!spellHero->GetNumSpells(spellType)) {
        NormalDialog("No spells to cast.", NORMAL_DIALOG_TYPE_OK, -1, -1, NORMAL_DIALOG_NO_RESOURCE, 0, NORMAL_DIALOG_NO_RESOURCE, 0, NORMAL_DIALOG_NO_OR_TEXT);
    } else {
        m_viewSpellsCallback = callback;
        m_viewSpellsReadOnly = readOnly;
        m_viewSpellsHero = spellHero;
        SetupSpellRange(spellType);
        m_viewSpellsTop = m_spellFirst;
        if (spellType == 2 || spellType == 0) {
            m_viewSpellsWindow = new heroWindow(146, 145, "spellwin.bin");
            if (!m_viewSpellsWindow)
                MemError();
        } else {
            m_viewSpellsWindow = new heroWindow(97, 145, "spellwin.bin");
            if (!m_viewSpellsWindow)
                MemError();
        }
        if (spellType != 2) {
            message.type = MESSAGE_WIDGET;
            message.command = WIDGET_COMMAND_CLEAR_FLAGS;
            if (spellType == 0)
                message.id = 4;
            else
                message.id = 5;
            message.value = WIDGET_FLAG_ENABLED | WIDGET_FLAG_DRAW;
            m_viewSpellsWindow->BroadcastMessage(message);
        }
        UpdateSpellWidgets();
        gpWindowManager->DoDialog(m_viewSpellsWindow, ViewSpellsHandler, 0);
        delete m_viewSpellsWindow;
    }
    return m_viewSpell;
}

// HoMM1: combat spells fill hero slots 0..18 and adventure spells 19..28;
// the page ends at the last memorized slot.
VA(0x0043efc5, 0xbb)
void game::SetupSpellRange(short spellType) {
    switch (spellType) {
        case 0:
            m_spellFirst = 0;
            m_spellLast = m_spellFirst + 18;
            break;
        default:
            m_spellFirst = 19;
            m_spellLast = m_spellFirst + 9;
            break;
    }
    while (m_viewSpellsHero->m_spellCharges[m_spellLast] < 1)
        m_spellLast--;
}

// Buka 2.1 game::UpdateSpellWidgets for HoMM1's four-spell page: each slot
// shows the spell icon and its name with the remaining casts.
VA(0x0043f080, 0x1e0)
void game::UpdateSpellWidgets(void) {
    tag_message message;
    short i;

    message.type = MESSAGE_WIDGET;
    for (i = 0; i < 4; i++) {
        if (m_viewSpellsTop + i > m_spellLast) {
            message.command = WIDGET_COMMAND_CLEAR_FLAGS;
            message.id = i + SPELL_BOOK_ENTRY_FIRST;
            message.value = WIDGET_FLAG_ENABLED | WIDGET_FLAG_DRAW;
            m_viewSpellsWindow->BroadcastMessage(message);
            message.id = i + SPELL_BOOK_LABEL_FIRST;
            m_viewSpellsWindow->BroadcastMessage(message);
        } else {
            message.command = WIDGET_COMMAND_SET_FLAGS;
            message.id = i + SPELL_BOOK_LABEL_FIRST;
            message.value = WIDGET_FLAG_ENABLED | WIDGET_FLAG_DRAW;
            m_viewSpellsWindow->BroadcastMessage(message);
            message.id = i + SPELL_BOOK_ENTRY_FIRST;
            m_viewSpellsWindow->BroadcastMessage(message);
            if (m_viewSpellsReadOnly) {
                message.command = WIDGET_COMMAND_SET_FLAGS;
                message.value = WIDGET_FLAG_ENABLED;
                m_viewSpellsWindow->BroadcastMessage(message);
            }
            message.command = WIDGET_COMMAND_SET_FRAME;
            message.value = m_viewSpellsHero->m_spells[m_viewSpellsTop + i];
            m_viewSpellsWindow->BroadcastMessage(message);
            sprintf(
                gText,
                "%s[%d]",
                gSpellNames[m_viewSpellsHero->m_spells[m_viewSpellsTop + i]],
                m_viewSpellsHero->m_spellCharges[m_viewSpellsTop + i]
            );
            message.command = WIDGET_COMMAND_SET_TEXT;
            message.id = i + SPELL_BOOK_LABEL_FIRST;
            message.text = gText;
            m_viewSpellsWindow->BroadcastMessage(message);
        }
    }
}

// Buka 2.1 ViewSpellsHandler: right clicks describe a spell or control,
// left clicks page, switch books or pick the spell to cast.
VA(0x0043f260, 0x4f8)
short ViewSpellsHandler(tag_message& message) {
    int spell;
    if (message.type == MESSAGE_WIDGET) {
        switch (message.command) {
            case WIDGET_NOTIFY_SELECT:
            case WIDGET_NOTIFY_RIGHT_CLICK:
                if (message.command == WIDGET_NOTIFY_RIGHT_CLICK
                    || (message.modifiers & MESSAGE_MODIFIER_RIGHT_BUTTON)) {
                    switch (message.id) {
                        case SPELL_BOOK_ENTRY_FIRST:
                        case SPELL_BOOK_ENTRY_FIRST + 1:
                        case SPELL_BOOK_ENTRY_FIRST + 2:
                        case SPELL_BOOK_ENTRY_LAST:
                            spell = gpGame->m_viewSpellsHero
                                        ->m_spells[message.id - SPELL_BOOK_ENTRY_FIRST + gpGame->m_viewSpellsTop];
                            NormalDialog(gSpellDesc[spell], NORMAL_DIALOG_TYPE_QUICK_VIEW, -1, -1, NORMAL_DIALOG_SPELL, spell, NORMAL_DIALOG_NO_RESOURCE, 0, NORMAL_DIALOG_NO_OR_TEXT);
                            break;
                        case SPELL_BOOK_PREVIOUS_PAGE:
                            NormalDialog(cSpellHelp[0], NORMAL_DIALOG_TYPE_QUICK_VIEW, -1, -1, NORMAL_DIALOG_NO_RESOURCE, 0, NORMAL_DIALOG_NO_RESOURCE, 0, NORMAL_DIALOG_NO_OR_TEXT);
                            break;
                        case SPELL_BOOK_NEXT_PAGE:
                            NormalDialog(cSpellHelp[1], NORMAL_DIALOG_TYPE_QUICK_VIEW, -1, -1, NORMAL_DIALOG_NO_RESOURCE, 0, NORMAL_DIALOG_NO_RESOURCE, 0, NORMAL_DIALOG_NO_OR_TEXT);
                            break;
                        case SPELL_BOOK_ADVENTURE_SPELLS:
                            NormalDialog(cSpellHelp[2], NORMAL_DIALOG_TYPE_QUICK_VIEW, -1, -1, NORMAL_DIALOG_NO_RESOURCE, 0, NORMAL_DIALOG_NO_RESOURCE, 0, NORMAL_DIALOG_NO_OR_TEXT);
                            break;
                        case SPELL_BOOK_COMBAT_SPELLS:
                            NormalDialog(cSpellHelp[3], NORMAL_DIALOG_TYPE_QUICK_VIEW, -1, -1, NORMAL_DIALOG_NO_RESOURCE, 0, NORMAL_DIALOG_NO_RESOURCE, 0, NORMAL_DIALOG_NO_OR_TEXT);
                            break;
                    }
                } else {
                    switch (message.id) {
                        case SPELL_BOOK_ENTRY_FIRST:
                        case SPELL_BOOK_ENTRY_FIRST + 1:
                        case SPELL_BOOK_ENTRY_FIRST + 2:
                        case SPELL_BOOK_ENTRY_LAST:
                            if (gpGame->m_viewSpellsReadOnly) {
                                spell = gpGame->m_viewSpellsHero
                                            ->m_spells[message.id - SPELL_BOOK_ENTRY_FIRST + gpGame->m_viewSpellsTop];
                                NormalDialog(gSpellDesc[spell], NORMAL_DIALOG_TYPE_OK, -1, -1, NORMAL_DIALOG_SPELL, spell, NORMAL_DIALOG_NO_RESOURCE, 0, NORMAL_DIALOG_NO_OR_TEXT);
                                return MESSAGE_DISPATCH_CONSUME;
                            }
                            gpGame->m_viewSpell = gpGame->m_viewSpellsHero
                                                      ->m_spells[message.id - SPELL_BOOK_ENTRY_FIRST + gpGame->m_viewSpellsTop];
                            message.command = WIDGET_COMMAND_DIALOG_SELECT;
                            return MESSAGE_DISPATCH_FORWARD;
                        case SPELL_BOOK_PREVIOUS_PAGE:
                            if (gpGame->m_viewSpellsTop == gpGame->m_spellFirst)
                                break;
                            gpGame->m_viewSpellsTop -= 4;
                            if (gpGame->m_viewSpellsTop < gpGame->m_spellFirst)
                                gpGame->m_viewSpellsTop = gpGame->m_spellFirst;
                            gpGame->UpdateSpellWidgets();
                            gpGame->m_viewSpellsWindow->MoveWindow(0, 0);
                            break;
                        case SPELL_BOOK_NEXT_PAGE:
                            if (gpGame->m_viewSpellsTop + 4 <= gpGame->m_spellLast)
                                gpGame->m_viewSpellsTop += 4;
                            if (gpGame->m_viewSpellsTop < gpGame->m_spellFirst)
                                gpGame->m_viewSpellsTop = gpGame->m_spellFirst;
                            gpGame->UpdateSpellWidgets();
                            gpGame->m_viewSpellsWindow->MoveWindow(0, 0);
                            break;
                        case SPELL_BOOK_ADVENTURE_SPELLS:
                            gpGame->SetupSpellRange(1);
                            gpGame->m_viewSpellsTop = gpGame->m_spellFirst;
                            gpGame->UpdateSpellWidgets();
                            gpGame->m_viewSpellsWindow->MoveWindow(0, 0);
                            break;
                        case SPELL_BOOK_COMBAT_SPELLS:
                            gpGame->SetupSpellRange(0);
                            gpGame->m_viewSpellsTop = gpGame->m_spellFirst;
                            gpGame->UpdateSpellWidgets();
                            gpGame->m_viewSpellsWindow->MoveWindow(0, 0);
                            break;
                    }
                }
                break;
            case WIDGET_COMMAND_HOVER:
                if (message.id == gpWindowManager->m_lastHoverId)
                    return MESSAGE_DISPATCH_CONSUME;
                else
                    return gpGame->m_viewSpellsCallback(message);
                break;
        }
        if (message.id == WIDGET_COMMAND_DIALOG_SELECT) {
            message.command = message.id;
            return MESSAGE_DISPATCH_FORWARD;
        }
    }
    return MESSAGE_DISPATCH_CONSUME;
}

// Buka 2.1 ViewSpecialHandler: hovering a spell-book control shows its
// help line in the hero screen's status bar.
VA(0x0043f758, 0x175)
short ViewSpecialHandler(tag_message& message) {
    if (message.type == MESSAGE_WIDGET) {
        switch (message.command) {
            case WIDGET_COMMAND_HOVER:
                if (message.id == gpWindowManager->m_lastHoverId)
                    return MESSAGE_DISPATCH_CONSUME;
                gpWindowManager->m_lastHoverId = message.id;
                switch (message.id) {
                    case SPELL_BOOK_PREVIOUS_PAGE:
                        strcpy(gText, cSpellHelp[0]);
                        break;
                    case SPELL_BOOK_NEXT_PAGE:
                        strcpy(gText, cSpellHelp[1]);
                        break;
                    case SPELL_BOOK_ADVENTURE_SPELLS:
                        strcpy(gText, cSpellHelp[2]);
                        break;
                    case SPELL_BOOK_COMBAT_SPELLS:
                        strcpy(gText, cSpellHelp[3]);
                        break;
                    case DIALOG_BUTTON_0:
                        strcpy(gText, cSpellHelp[4]);
                        break;
                    default:
                        strcpy(gText, cSpellHelp[5]);
                        break;
                }
                HeroMessageUpdate(gText);
                return MESSAGE_DISPATCH_CONSUME;
        }
    }
    return MESSAGE_DISPATCH_CONSUME;
}

// donor PoL RVA 0x0007a649; preferred Buka symbol ?ViewArmy@game@@QAEXHHHHPAVtown@@HHHPAVhero@@PAVarmy@@PAVarmyGroup@@H@Z
// donor Buka TU SOURCE/GAME; HoMM1 owner inferred from contiguous order
// evidence: graph:5;base=0.612909;margin=0.340762;shape=0.385;size=0.681;calls=0.829;strings= (%d)|%s%d|armywin.bin;alternate=pol20:void game::ViewArmy(int, int, int, int, class town *, int, int, int, class hero *, class army *, class armyGroup *, int)@0x0007a649
VA(0x0043f8cd, 0x8e1)
void game::ViewArmy(
    short x,
    short y,
    signed char monsterType,
    short numTroops,
    class town* castle,
    signed char disableDismiss,
    signed char facing,
    signed char quickView,
    class hero* theHero,
    class army* theArmy,
    class armyGroup* theGroup
) {
    char numText[12];
    int shotCount;
    short baseX;
    short spacing;
    short topY;
    short animId;
    int morale;
    tag_monsterInfo* monsterInfo;
    short numId;
    int i;
    char* statText;
    int luck;
    tag_message message;
    char iconName[16];
    iconWidget* monsterWidget;
    short statsMessage;
    short titleLabel;
    int mod;
    short blankBtn;
    char fileName[13];

    baseX = 86;
    topY = 164;
    blankBtn = 1;
    numId = 2;
    titleLabel = 3;
    statsMessage = 4;
    animId = 5;
    message.type = MESSAGE_WIDGET;

    if (monsterType != CREATURE_SWORDSMAN)
        strcpy(iconName, gArmyNames[monsterType]);
    else
        strcpy(iconName, "swrdsman");
    monsterInfo = &gMonsterDatabase[monsterType];
    m_viewArmyWindow = new heroWindow(x, y, "armywin.bin");
    if (!m_viewArmyWindow)
        MemError();
    spacing = 30;
    if (monsterInfo->stats.attributes & 1) {
        switch (facing) {
            case 0:
                spacing += 43;
                break;
            case 1:
                spacing += 119;
                break;
        }
    } else if (facing == 1) {
        spacing += 76;
    } else {
        spacing += 86;
    }
    if (monsterInfo->stats.attributes & 2)
        sprintf(fileName, "%s.wlk", iconName);
    else
        sprintf(fileName, "%s.wip", iconName);
    monsterWidget = new iconWidget(spacing, 164, 86, 149, fileName, 0, facing == 1, 5, ICON_WIDGET_DRAW, 1);
    if (!monsterWidget)
        MemError();
    m_viewArmyWindow->AddWidget(monsterWidget, -1);

    strcpy(fileName, gArmyNames[monsterType]);
    fileName[0] -= 32;
    message.command = WIDGET_COMMAND_SET_TEXT;
    message.id = 3;
    message.text = fileName;
    m_viewArmyWindow->BroadcastMessage(message);

    statText = static_cast<char*>(malloc(550));
    if (theGroup)
        morale = theGroup->GetMorale(theHero, castle);
    else
        morale = 0;
    sprintf(statText, "");

    mod = 0;
    sprintf(gText, "%s%d", gArmyStatText[0], monsterInfo->stats.attack);
    strcat(statText, gText);
    if (theHero)
        mod += theHero->m_primaryStats[HERO_PRIMARY_ATTACK];
    if (mod) {
        sprintf(gText, " (%d)", monsterInfo->stats.attack + mod);
        strcat(statText, gText);
    }

    mod = 0;
    sprintf(gText, "\n%s%d", gArmyStatText[1], monsterInfo->stats.defense);
    strcat(statText, gText);
    if (theHero)
        mod += theHero->m_primaryStats[HERO_PRIMARY_DEFENSE];
    if (theArmy && theArmy->m_spellEffect == SPELL_PROTECTION)
        mod += 3;
    if (mod) {
        sprintf(gText, " (%d)", monsterInfo->stats.defense + mod);
        strcat(statText, gText);
    }

    if (monsterInfo->stats.attributes & 4) {
        if (theArmy)
            shotCount = theArmy->m_stats.shots;
        else
            shotCount = monsterInfo->stats.shots;
        if (shotCount > 0) {
            if (gpCombatManager->m_active == 1)
                sprintf(gText, "\n%s%d", gArmyStatText[2], shotCount);
            else
                sprintf(gText, "\n%s%d", gArmyStatText[8], shotCount);
            strcat(statText, gText);
        }
    }

    sprintf(gText, "\n%s%d", gArmyStatText[3], monsterInfo->stats.damageMin);
    strcat(statText, gText);
    if (monsterInfo->stats.damageMin != monsterInfo->stats.damageMax) {
        sprintf(gText, "-%d", monsterInfo->stats.damageMax);
        strcat(statText, gText);
    }
    sprintf(gText, "\n%s%d", gArmyStatText[4], static_cast<unsigned char>(monsterInfo->stats.hitPoints));
    strcat(statText, gText);
    sprintf(gText, "\n%s%s", gArmyStatText[5], gSpeedText[monsterInfo->stats.speed]);
    strcat(statText, gText);
    sprintf(gText, "\n%s%s", gArmyStatText[6], gMoraleText[morale + 3]);
    strcat(statText, gText);
    luck = GetLuck(theHero, theArmy);
    sprintf(gText, "\n%s%s", gArmyStatText[7], gLuckText[luck + 3]);
    strcat(statText, gText);

    message.id = 4;
    message.text = statText;
    m_viewArmyWindow->BroadcastMessage(message);
    if (disableDismiss) {
        message.command = WIDGET_COMMAND_CLEAR_FLAGS;
        message.value = WIDGET_FLAG_ENABLED | WIDGET_FLAG_DRAW;
        message.id = DIALOG_BUTTON_3;
        m_viewArmyWindow->BroadcastMessage(message);
    }
    if (quickView) {
        message.command = WIDGET_COMMAND_CLEAR_FLAGS;
        message.value = WIDGET_FLAG_ENABLED | WIDGET_FLAG_DRAW;
        message.id = DIALOG_BUTTON_0;
        m_viewArmyWindow->BroadcastMessage(message);
    }
    if (numTroops < 1) {
        message.command = WIDGET_COMMAND_CLEAR_FLAGS;
        message.value = WIDGET_FLAG_ENABLED | WIDGET_FLAG_DRAW;
        message.id = 1;
        m_viewArmyWindow->BroadcastMessage(message);
        message.id = 2;
        m_viewArmyWindow->BroadcastMessage(message);
    } else {
        sprintf(numText, "%d", numTroops);
        message.command = WIDGET_COMMAND_SET_TEXT;
        message.id = 2;
        message.text = numText;
        m_viewArmyWindow->BroadcastMessage(message);
    }
    glTimers[0] = KBTickCount() + 90;
    m_viewArmyResult = 0;
    if (quickView) {
        gpMouseManager->ReallyHidePointer();
        gpWindowManager->AddWindow(m_viewArmyWindow, -1, 1);
        QuickViewWait();
        gpWindowManager->RemoveWindow(m_viewArmyWindow);
        gpMouseManager->ReallyShowPointer();
    } else {
        gpWindowManager->DoDialog(m_viewArmyWindow, ViewArmyHandler, 0);
        if (gbDismissArmy && theGroup) {
            for (i = 0; i < ARMY_GROUP_SLOT_COUNT; i++) {
                if (theGroup->m_creatureTypes[i] == monsterType) {
                    theGroup->m_creatureTypes[i] = CREATURE_NONE;
                    theGroup->m_creatureCounts[i] = 0;
                }
            }
        }
    }
    free(statText);
    delete m_viewArmyWindow;
}

// donor PoL RVA 0x0007b2cf; preferred Buka symbol ?ViewArmyHandler@@YIHAAUtag_message@@@Z
// donor Buka TU SOURCE/GAME; HoMM1 owner inferred from contiguous order
// evidence: graph:3;base=0.308927;margin=0.170943;shape=0.232;size=0.493;calls=0.556;alternate=pol20:int ViewArmyHandler(struct tag_message &)@0x0007b2cf
VA(0x004401ae, 0x1b8)
short ViewArmyHandler(tag_message& message) {
    short frameDelay;
    short offset;
    gbDismissArmy = 0;
    frameDelay = 5;
    if (message.type == MESSAGE_WIDGET) {
        switch (message.command) {
            case WIDGET_NOTIFY_DESELECT:
                switch (message.id) {
                    case DIALOG_BUTTON_0:
                    case DIALOG_BUTTON_1:
                        gpWindowManager->m_dialogResult = message.id;
                        message.command = message.id =
                            WIDGET_COMMAND_DIALOG_SELECT;
                        return MESSAGE_DISPATCH_FORWARD;
                    case DIALOG_BUTTON_3:
                        NormalDialog("Are you sure you want to dismiss this army?", NORMAL_DIALOG_TYPE_YES_NO, 0xb1, 0x36, NORMAL_DIALOG_NO_RESOURCE, 0, NORMAL_DIALOG_NO_RESOURCE, 0, NORMAL_DIALOG_NO_OR_TEXT);
                        if (gpWindowManager->m_dialogResult == NORMAL_DIALOG_CONFIRM) {
                            gbDismissArmy = 1;
                            message.command = message.id =
                                WIDGET_COMMAND_DIALOG_SELECT;
                            return MESSAGE_DISPATCH_FORWARD;
                        }
                        break;
                    default:
                        break;
                }
                break;
            default:
                break;
        }
    }
    if (KBTickCount() > glTimers[0]) {
        message.type = MESSAGE_WIDGET;
        message.command = WIDGET_COMMAND_SET_FRAME;
        message.id = 5;
        gpGame->m_viewArmyResult++;
        message.value = gpGame->m_viewArmyResult % 6;
        gpGame->m_viewArmyWindow->BroadcastMessage(message);
        gpGame->m_viewArmyWindow->DrawWindow();
        glTimers[0] = KBTickCount() + 90;
    }
    return MESSAGE_DISPATCH_CONSUME;
}

// Kingdom overview: heroes by class, castles and towns by type and mines by
// resource drawn onto the backdrop, then the date, income and resources.
VA(0x00440366, 0xbf2)
void game::Overview(void) {
    short unusedGY;
    short townTop;
    short unusedFVal;
    short heroNumYW;
    short incomeWidgetY;
    font* smallFont;
    short heroTextH;
    short townTextWPos;
    short spacing;
    short heroTextW;
    short castleFrameY;
    short limitYOff;
    short dayIdY;
    short left;
    short castleIconY;
    signed char mineNums[7];
    short fieldH;
    font* bigFont;
    short textW;
    short mineRowY;
    short i;
    short badgeY;
    short lineH;
    short mineW;
    signed char redraw;
    tag_message message;
    short totals[4];
    short numMines;
    short numCastles;
    short numTowns;
    heroWindow* win;
    short firstTown;
    short classCountY;
    icon* ovIcon;
    short heroFrame;
    short mineBase;
    short badge;
    short spare1;
    short heroRowY;
    short shieldDXX;
    short one;
    short nextType;

    gpAdvManager->TrimLoopingSounds(8);
    gbOverviewShowing = 1;
    unusedGY = 82;
    shieldDXX = 49;
    spare1 = 73;
    heroTextW = 33;
    heroTextH = 38;
    textW = 132;
    fieldH = 80;
    townTextWPos = 132;
    unusedFVal = 80;
    mineW = 72;
    badgeY = 66;
    heroRowY = 32;
    heroNumYW = 67;
    castleIconY = 113;
    townTop = 201;
    mineRowY = 289;
    heroFrame = 0;
    castleFrameY = 4;
    firstTown = 8;
    mineBase = 12;
    badge = 15;
    lineH = 16;
    limitYOff = 544;
    redraw = 1;
    one = 1;
    dayIdY = 64;
    incomeWidgetY = 65;

    gpMouseManager->SetPointer("advmice.mse", ADVENTURE_POINTER_DEFAULT);
    bigFont = gpResourceManager->GetFont("bigfont.fnt");
    smallFont = gpResourceManager->GetFont("smalfont.fnt");
    gpWindowManager->FadeScreen(WINDOW_FADE_OUT, WINDOW_MANAGER_DIALOG_FADE_STEP, NULL);
    gpResourceManager->GetBackdropAtLoc("overmain.bmp", gpWindowManager->m_screen, 96, 0);
    sprintf(gText, "overban%01d.bmp", gpCurPlayer->m_color);
    gpResourceManager->GetBackdropAtLoc(gText, gpWindowManager->m_screen, 0, 0);
    ovIcon = gpResourceManager->GetIcon("overview.icn");

    memset(totals, 0, sizeof(totals));
    for (i = 0; i < gpCurPlayer->m_heroCount; i++)
        totals[m_heroRecs[gpCurPlayer->m_heroIds[i]].m_heroClass]++;
    classCountY = 0;
    for (i = 0; i < 4; i++) {
        if (totals[i])
            classCountY++;
    }
    spacing = 136;
    left = 121;
    nextType = 0;
    for (i = 0; i < classCountY; i++) {
        while (!totals[nextType])
            nextType++;
        ovIcon->DrawToBuffer(spacing * i + left, 32, nextType, ICON_DRAW_NORMAL, 0);
        ovIcon->DrawToBuffer(spacing * i + left + 49, 67, 15, ICON_DRAW_NORMAL, 0);
        sprintf(gText, "%d", totals[nextType]);
        bigFont->DrawBoundedString(gText, spacing * i + left + 48, 77, 33, 16, 1, 1);
        nextType++;
    }

    memset(totals, 0, sizeof(totals));
    for (i = 0; i < gpCurPlayer->m_townCount; i++) {
        if (m_castleRecs[gpCurPlayer->m_townIds[i]].m_buildings & 0x40)
            totals[m_castleRecs[gpCurPlayer->m_townIds[i]].m_type]++;
    }
    numCastles = 0;
    for (i = 0; i < 4; i++) {
        if (totals[i])
            numCastles++;
    }
    if (numCastles) {
        spacing = 136;
        left = 100;
        nextType = 0;
        for (i = 0; i < numCastles; i++) {
            while (!totals[nextType])
                nextType++;
            ovIcon->DrawToBuffer(spacing * i + left, 113, nextType + 4, ICON_DRAW_NORMAL, 0);
            sprintf(gText, "%d", totals[nextType]);
            bigFont->DrawBoundedString(gText, spacing * i + left, 173, 132, 16, 1, 1);
            nextType++;
        }
    }

    memset(totals, 0, sizeof(totals));
    for (i = 0; i < gpCurPlayer->m_townCount; i++) {
        if (!(m_castleRecs[gpCurPlayer->m_townIds[i]].m_buildings & 0x40))
            totals[m_castleRecs[gpCurPlayer->m_townIds[i]].m_type]++;
    }
    numTowns = 0;
    for (i = 0; i < 4; i++) {
        if (totals[i])
            numTowns++;
    }
    if (numTowns) {
        spacing = 136;
        left = 100;
        nextType = 0;
        for (i = 0; i < numTowns; i++) {
            while (!totals[nextType])
                nextType++;
            ovIcon->DrawToBuffer(spacing * i + left, 201, nextType + 8, ICON_DRAW_NORMAL, 0);
            sprintf(gText, "%d", totals[nextType]);
            bigFont->DrawBoundedString(gText, spacing * i + left, 261, 132, 16, 1, 1);
            nextType++;
        }
    }

    memset(mineNums, 0, sizeof(mineNums));
    for (i = 2; i < GAME_MINE_COUNT; i++) {
        if (m_mineOwners[i] == giCurPlayer)
            mineNums[m_mines[i].type]++;
    }
    numMines = 0;
    for (i = 0; i < 7; i++) {
        if (mineNums[i])
            numMines++;
    }
    if (numMines) {
        spacing = 77;
        left = 100;
        nextType = 0;
        for (i = 0; i < numMines; i++) {
            while (!mineNums[nextType])
                nextType++;
            ovIcon->DrawToBuffer(spacing * i + left, 289, (nextType < 2 ? nextType : 2) + 12, ICON_DRAW_NORMAL, 0);
            if (nextType >= 2)
                ovIcon->DrawToBuffer(spacing * i + left, 289, nextType + 14, ICON_DRAW_NORMAL, 0);
            sprintf(gText, "%d", mineNums[nextType]);
            bigFont->DrawBoundedString(gText, spacing * i + left, 355, 72, 16, 1, 1);
            nextType++;
        }
    }

    gpWindowManager->UpdateScreenRegion(0, 0, 640, 480);
    win = new heroWindow(0, 0, "overwind.bin");
    if (!win)
        MemError();
    SetWinText(win, 8);
    message.type = MESSAGE_WIDGET;
    message.command = WIDGET_COMMAND_SET_TEXT;
    message.id = 64;
    sprintf(gText, gOverviewText[0], m_month, m_week, m_day);
    message.text = gText;
    win->BroadcastMessage(message);
    message.id = 65;
    sprintf(gText, "%d", ComputeDailyGold(giCurPlayer));
    win->BroadcastMessage(message);
    for (i = 0; i < 7; i++) {
        sprintf(gText, "%d", gpCurPlayer->m_resources[i]);
        message.id = i + 1;
        win->BroadcastMessage(message);
    }
    gpWindowManager->AddWindow(win, -1, 1);
    win->DrawWindow();
    gText[0] = 0;
    if (m_mineOwners[0] == giCurPlayer) {
        strcpy(gText, gOverviewText[1]);
        smallFont->DrawBoundedString(gText, 100, 450, 400, 12, 1, 0);
        gpWindowManager->UpdateScreenRegion(100, 450, 400, 12);
    }
    if (m_mineOwners[1] == giCurPlayer) {
        strcpy(gText, gOverviewText[2]);
        smallFont->DrawBoundedString(gText, 100, 465, 400, 12, 1, 0);
        gpWindowManager->UpdateScreenRegion(100, 465, 400, 12);
    }
    gpWindowManager->FadeScreen(WINDOW_FADE_IN, WINDOW_MANAGER_DIALOG_FADE_STEP, NULL);
    gpWindowManager->DoDialog(win, TrueFalseDialogHandler, 0);
    delete win;
    gpWindowManager->FadeScreen(WINDOW_FADE_OUT, WINDOW_MANAGER_DIALOG_FADE_STEP, NULL);
    gpResourceManager->Dispose(ovIcon);
    gpResourceManager->Dispose(smallFont);
    gpResourceManager->Dispose(bigFont);
    gbOverviewShowing = 0;
}

// Buka 2.1 game::GetRandomNumTroops with HoMM1's 28 creatures.
VA(0x00440f58, 0x28b)
signed char game::GetRandomNumTroops(signed char monsterType) {
    switch (monsterType) {
        case CREATURE_PEASANT:
            return Random(30, 80);
        case CREATURE_ARCHER:
            return Random(20, 30);
        case CREATURE_PIKEMAN:
            return Random(20, 30);
        case CREATURE_SWORDSMAN:
            return Random(12, 25);
        case CREATURE_CAVALRY:
            return Random(8, 16);
        case CREATURE_PALADIN:
            return Random(6, 12);
        case CREATURE_GOBLIN:
            return Random(25, 40);
        case CREATURE_ORC:
            return Random(15, 30);
        case CREATURE_WOLF:
            return Random(20, 35);
        case CREATURE_OGRE:
            return Random(10, 20);
        case CREATURE_TROLL:
            return Random(7, 10);
        case CREATURE_CYCLOPS:
            return Random(5, 7);
        case CREATURE_SPRITE:
            return Random(20, 40);
        case CREATURE_DWARF:
            return Random(10, 25);
        case CREATURE_ELF:
            return Random(15, 30);
        case CREATURE_DRUID:
            return Random(10, 25);
        case CREATURE_UNICORN:
            return Random(8, 15);
        case CREATURE_PHOENIX:
            return Random(7, 12);
        case CREATURE_CENTAUR:
            return Random(20, 50);
        case CREATURE_GARGOYLE:
            return Random(15, 30);
        case CREATURE_GRIFFIN:
            return Random(10, 25);
        case CREATURE_MINOTAUR:
            return Random(10, 16);
        case CREATURE_HYDRA:
            return Random(6, 8);
        case CREATURE_DRAGON:
            return Random(3, 7);
        case CREATURE_ROGUE:
            return Random(20, 40);
        case CREATURE_NOMAD:
            return Random(12, 25);
        case CREATURE_GHOST:
            return Random(10, 20);
        case CREATURE_GENIE:
            return Random(4, 9);
        default:
            return 3;
    }
}

// Buka 2.1 game::TurnOnAIMusic.
VA(0x004411e3, 0x3d)
void game::TurnOnAIMusic(void) {
    gpSoundManager->StopAllSamples();
    gpSoundManager->SwitchAmbientMusic(MUSIC_TRACK_AI_TURN);
    gpSoundManager->m_musicReady = 0;
}

// Buka 2.1 game::TurnOffAIMusic.
VA(0x00441220, 0x25)
void game::TurnOffAIMusic(void) {
    gpSoundManager->m_musicReady = 1;
}

// Buka 2.1 game::NextPlayer for HoMM1: autosaves, advances to the next
// living player (a new day after the last), restores hero movement (none
// on the campaign's goal town) and hands the turn to the computer or the
// human.
VA(0x00441245, 0x4e1)
void game::NextPlayer(void) {
    hero* currentHero;
    int numHumans;
    int i;
    int remote;
    // Retail reserves 0x14 unused bytes above the named locals.
    char unused[20];

    iCurHourGlassPhase = 0;
    if (gbThisNetHumanPlayer[giCurPlayer] && gConfig.autosave) {
        numHumans = 0;
        for (i = 0; i < GAME_PLAYER_COUNT; i++) {
            if (!m_playerDead[i] && gbHumanPlayer[i])
                numHumans++;
        }
        SaveGame("AUTOSAVE", 1);
    }
    if (gpGame->m_players[giCurPlayer].m_daysLeft > 0)
        gpGame->m_players[giCurPlayer].m_daysLeft--;
    CheckEndGame(0);
    gpAdvManager->DeactivateCurrTown();
    gpAdvManager->DeactivateCurrHero();
    do {
        giCurPlayer++;
        if (giCurPlayer >= m_playerCount) {
            giCurPlayer = 0;
            PerDay();
        }
    } while (gpGame->m_playerDead[giCurPlayer]);
    gpCurPlayer = &gpGame->m_players[giCurPlayer];
    giCurPlayerBit = 1 << giCurPlayer;
    giCurPlayerHighBit = 1 << (giCurPlayer + 4);
    for (i = 0; i < m_players[giCurPlayer].m_heroCount; i++) {
        currentHero = &m_heroRecs[m_players[giCurPlayer].m_heroIds[i]];
        currentHero->m_mobility = currentHero->CalcMobility();
        if (m_campaignType > 0 && gCampaignScenarios[m_campaignScenario].victoryTownX == currentHero->m_x
            && gCampaignScenarios[m_campaignScenario].victoryTownY == currentHero->m_y)
            currentHero->m_mobility = 0;
        currentHero->m_remainingMobility = currentHero->m_mobility;
    }
    if (!gbThisNetHumanPlayer[giCurPlayer]) {
        gpMouseManager->SetPointer(ADVENTURE_POINTER_WAIT);
        gpAdvManager->HideRoute(1, 0, 1);
        gpAdvManager->CheckDimNextHeroBut();
        TurnOnAIMusic();
        SetNoDialogMenus(0);
        giBottomViewOverride = 6;
        ShowComputerScreen();
        bShowIt = 0;
        if (gbRemoteOn && (gbHumanPlayer[giCurPlayer] || giHostGamePos != giThisGamePos)) {
            if (!gbHumanPlayer[giCurPlayer])
                remote = giHostGamePos;
            else
                remote = giCurPlayer;
            if (!gpGame->TransmitSaveGame(remote, 0))
                ShutDown(NULL);
        }
        if (giBottomViewOverride == 6)
            giBottomViewOverride = 0;
    } else {
        SetNoDialogMenus(1);
        gpInputManager->Flush();
        if (gbBlackoutPlayer && giNumHumanPlayers > 1) {
            sprintf(gText, "%s player turn.", gColorNames[gpGame->m_players[giCurPlayer].m_color]);
            gText[0] -= 32;
            WaitForPlayer(gText, giCurPlayer);
        }
        if (gbThisNetHumanPlayer[giCurPlayer])
            CancelComputerScreen();
        giCurWatchPlayerBit = giCurPlayerBit;
        giCurWatchPlayer = giCurPlayer;
        giCurWatchPlayerHighBit = 1 << (giCurPlayer + 4);
    }
    DoNewTurn();
    gpMouseManager->ReallyShowPointer();
    CheckEndGame(0);
    if (gbThisNetHumanPlayer[giCurPlayer] && gbRemoteOn && m_day != 1 && giForceSwitchMusic == -1) {
        gpSoundManager->SwitchAmbientMusic(15);
        giForceSwitchMusic = KBTickCount();
    }
}

// Buka 2.1 game::ComputeDailyGold for HoMM1: the first mine and gold mines
// pay 1000, towns 250 (castles 1000), three treasure artifacts add more,
// and computer players' gold scales with their level.
VA(0x00441726, 0x280)
int game::ComputeDailyGold(int player) {
    int gold;
    int i;
    gold = 0;
    if (m_mines[0].owner == player)
        gold += 1000;
    for (i = 2; i < GAME_MINE_COUNT; i++) {
        if (m_mines[i].owner == player && m_mines[i].type == 6)
            gold += 1000;
    }
    for (i = 0; i < GAME_TOWN_COUNT; i++) {
        if (m_castleRecs[i].m_owner == player) {
            if (m_castleRecs[i].m_buildings & (1 << BUILDING_SLOT_TENT))
                gold += 250;
            else
                gold += 1000;
        }
    }
    gold += m_players[player].NumOfGivenArtifact(ARTIFACT_ENDLESS_SACK_OF_GOLD) * 1000;
    gold += m_players[player].NumOfGivenArtifact(ARTIFACT_ENDLESS_BAG_OF_GOLD) * 750;
    gold += m_players[player].NumOfGivenArtifact(ARTIFACT_ENDLESS_PURSE_OF_GOLD) * 500;
    if (!gbHumanPlayer[player]) {
        if (gpGame->m_players[player].m_difficulty == 1)
            gold = gold * 0.75;
        if (gpGame->m_players[player].m_difficulty == 2) {
        }
        if (gpGame->m_players[player].m_difficulty == 3)
            gold = gold * 1.29;
        if (gpGame->m_players[player].m_difficulty == 4)
            gold = gold * 1.45;
    }
    return gold;
}

// Buka 2.1 game::PerDay for HoMM1: records each player's income, pays the
// mines, towns and computer bonuses, then advances the calendar.
VA(0x004419a6, 0x463)
void game::PerDay(void) {
    short i;
    short production;
    // Retail reserves one more unused slot between the counters.
    short k;
    short j;
    signed char resource;

    for (i = 0; i < gpGame->m_playerCount; i++) {
        for (j = 0; j < PLAYER_RESOURCE_COUNT; j++)
            gpGame->m_players[i].m_aiData.m_income[j] = -m_players[i].m_resources[j];
    }
    memset(m_townBuiltToday, 0, sizeof(m_townBuiltToday));
    gpAdvManager->m_identifyHeroActive = 0;
    for (i = 2; i < GAME_MINE_COUNT; i++) {
        if (m_mines[i].owner != -1) {
            resource = m_mines[i].type;
            production = 0;
            if (resource == RESOURCE_ORE)
                production = 2;
            else if (resource == RESOURCE_WOOD)
                production = 2;
            else if (resource != RESOURCE_GOLD)
                production = 1;
            if (resource != RESOURCE_GOLD)
                m_players[m_mines[i].owner].m_resources[resource] += production;
        }
    }
    for (i = 0; i < GAME_TOWN_COUNT; i++)
        m_castleRecs[i].m_turnsOwned++;
    for (i = 0; i < m_playerCount; i++)
        m_players[i].m_resources[RESOURCE_GOLD] += ComputeDailyGold(i);
    for (i = 0; i < m_playerCount; i++) {
        if (!gbHumanPlayer[i]) {
            if (gpGame->m_players[i].m_difficulty > 2) {
                m_players[i].m_resources[RESOURCE_WOOD]++;
                m_players[i].m_resources[RESOURCE_ORE]++;
            }
            if (gpGame->m_players[i].m_difficulty > 3 && m_day >= 1 && m_day <= 6)
                m_players[i].m_resources[m_day - 1]++;
        }
    }
    m_day++;
    giCurTurn = (m_month - 1) * 28 + (m_week - 1) * 7 + m_day;
    if (m_day > 7) {
        m_day = 1;
        PerWeek();
    }
    if (m_week > 4) {
        m_week = 1;
        PerMonth();
    }
    for (i = 0; i < gpGame->m_playerCount; i++) {
        for (j = 0; j < PLAYER_RESOURCE_COUNT; j++)
            gpGame->m_players[i].m_aiData.m_income[j] += m_players[i].m_resources[j];
    }
}

// Buka 2.1 game::PerWeek for HoMM1: rolls the week, grows every dwelling
// (computer towns grow faster), refreshes the tavern heroes and restocks the
// map's renewable sites.
VA(0x00441e09, 0x84b)
void game::PerWeek(void) {
    short posY;
    short posX;
    short gain;
    town* townPointer;
    short j;
    short i;
    int heroClass = 0;

    giWeekType = 0;
    giWeekSpecial = Random(0, 14);
    if (m_week != 4) {
        i = Random(1, 4);
        if (i == 1) {
            giWeekType = 1;
            giWeekSpecial = Random(0, 23);
        }
    }
    for (i = 0; i < GAME_TOWN_COUNT; i++) {
        townPointer = GetTown(i);
        for (j = 7; j <= 12; j++) {
            if (townPointer->m_buildings & (1 << j)) {
                gain = gMonsterDatabase[gDwellingType[townPointer->m_type][j - 7]].growth;
                if (townPointer->m_buildings & 0x10)
                    gain += 2;
                if (townPointer->m_owner >= 0 && !gbHumanPlayer[townPointer->m_owner]) {
                    if (gpGame->m_players[townPointer->m_owner].m_difficulty == 3)
                        gain = gain * 1.24;
                    if (gpGame->m_players[townPointer->m_owner].m_difficulty == 4)
                        gain = gain * 1.36;
                }
                if (giWeekType == 1 && gDwellingType[townPointer->m_type][j - 7] == giWeekSpecial)
                    gain += 5;
                townPointer->m_garrison[j - 7] += gain;
            }
        }
    }
    for (i = 0; i < GAME_PLAYER_COUNT; i++) {
        for (j = 0; j < 2; j++) {
            heroClass = (Random(1, 3) + heroClass) % 4;
            if (gpGame->m_availableHeroes[gpGame->m_players[i].m_availableHeroIds[j]] == 0x40)
                gpGame->m_availableHeroes[gpGame->m_players[i].m_availableHeroIds[j]] = -1;
            gpGame->m_players[i].m_availableHeroIds[j] = gpGame->GetNewHeroId(heroClass);
        }
    }
    for (posY = 0; posY < MAP_CELL_GRID_SIZE; posY++) {
        for (posX = 0; posX < MAP_CELL_GRID_SIZE; posX++) {
            switch (m_map[posX][posY].m_triggerType) {
                case MAP_TRIGGER_EVENT | MAP_OBJECT_WATERWHEEL:
                    if (static_cast<unsigned char>(m_map[posX][posY].m_objectMetadata) != 0xff)
                        m_map[posX][posY].m_objectMetadata = 2;
                    break;
                case MAP_TRIGGER_EVENT | MAP_OBJECT_WINDMILL:
                    m_map[posX][posY].m_objectMetadata = Random(1, 5);
                    break;
                case MAP_TRIGGER_EVENT | MAP_OBJECT_STRAW_HUT:
                    if (static_cast<unsigned char>(m_map[posX][posY].m_objectMetadata) < 100)
                        m_map[posX][posY].m_objectMetadata = static_cast<unsigned char>(m_map[posX][posY].m_objectMetadata) + Random(3, 6);
                    break;
                case MAP_TRIGGER_EVENT | MAP_OBJECT_HOUSE:
                    if (static_cast<unsigned char>(m_map[posX][posY].m_objectMetadata) < 100)
                        m_map[posX][posY].m_objectMetadata = static_cast<unsigned char>(m_map[posX][posY].m_objectMetadata) + Random(5, 10);
                    break;
                case MAP_TRIGGER_EVENT | MAP_OBJECT_CABIN:
                    if (static_cast<unsigned char>(m_map[posX][posY].m_objectMetadata) < 100)
                        m_map[posX][posY].m_objectMetadata = static_cast<unsigned char>(m_map[posX][posY].m_objectMetadata) + Random(2, 4);
                    break;
                case MAP_TRIGGER_EVENT | MAP_OBJECT_DWARF_LOG_CABIN:
                    if (static_cast<unsigned char>(m_map[posX][posY].m_objectMetadata) < 100)
                        m_map[posX][posY].m_objectMetadata = static_cast<unsigned char>(m_map[posX][posY].m_objectMetadata) + Random(2, 4);
                    break;
                case MAP_TRIGGER_EVENT | MAP_OBJECT_PEASANT_LOG_CABIN:
                    if (static_cast<unsigned char>(m_map[posX][posY].m_objectMetadata) < 100)
                        m_map[posX][posY].m_objectMetadata = static_cast<unsigned char>(m_map[posX][posY].m_objectMetadata) + Random(5, 10);
                    break;
                case MAP_TRIGGER_EVENT | MAP_OBJECT_DESERT_TENT:
                    if (static_cast<unsigned char>(m_map[posX][posY].m_objectMetadata) < 100)
                        m_map[posX][posY].m_objectMetadata = static_cast<unsigned char>(m_map[posX][posY].m_objectMetadata) + Random(1, 3);
                    break;
                case MAP_TRIGGER_EVENT | MAP_OBJECT_WAGON_CAMP:
                    if (static_cast<unsigned char>(m_map[posX][posY].m_objectMetadata) < 100)
                        m_map[posX][posY].m_objectMetadata = static_cast<unsigned char>(m_map[posX][posY].m_objectMetadata) + Random(3, 6);
                    break;
                default:
                    break;
            }
        }
    }
    m_week++;
    GiveTroopsToNeutralTowns();
}

// Buka 2.1 game::PerMonth for HoMM1's six dwellings: a normal, creature or
// plague month, the creature month also seeding wandering monsters.
VA(0x00442654, 0x2e1)
void game::PerMonth(void) {
    town* townPointer;
    short growth;
    short j;
    short i;
    int y;
    int x;
    mapCell* spot;

    m_month++;
    i = Random(1, 10);
    if (i <= 5) {
        giMonthType = 0;
        giMonthSpecial = Random(0, 9);
    } else if (i <= 9) {
        giMonthType = 1;
        giMonthSpecial = giMonType[Random(0, 11)];
    } else {
        giMonthType = 2;
    }
    for (i = 0; i < GAME_TOWN_COUNT; i++) {
        for (j = 7; j <= 12; j++) {
            townPointer = GetTown(i);
            if (townPointer->m_buildings & (1 << j)) {
                growth = gMonsterDatabase[gDwellingType[townPointer->m_type][j - 7]].growth;
                if (townPointer->m_buildings & (1 << BUILDING_SLOT_WELL))
                    growth += 2;
                if (giMonthType == 1 && gDwellingType[townPointer->m_type][j - 7] == giMonthSpecial)
                    townPointer->m_garrison[j - 7] *= 2;
                if (giMonthType == 2) {
                    townPointer->m_garrison[j - 7] -= growth;
                    if (townPointer->m_garrison[j - 7] < 0)
                        townPointer->m_garrison[j - 7] = 0;
                    townPointer->m_garrison[j - 7] = townPointer->m_garrison[j - 7] >> 1;
                }
            }
        }
    }
    if (giMonthType == 1) {
        for (x = 0; x < MAP_CELL_GRID_SIZE; x++) {
            for (y = 0; y < MAP_CELL_GRID_SIZE; y++) {
                spot = gpAdvManager->GetCell(x, y);
                if (!spot->m_triggerType && giGroundToTerrain[spot->m_tileIndex]) {
                    if (Random(0, 360) == 10) {
                        spot->m_triggerType = (MAP_TRIGGER_EVENT | MAP_OBJECT_MONSTER);
                        spot->m_objectTileset = 0xc;
                        spot->m_objectIndex = giMonthSpecial;
                        spot->m_objectMetadata = GetRandomNumTroops(giMonthSpecial);
                    }
                }
            }
        }
    }
    gpAdvManager->CompleteDraw(0);
}

// Buka 2.1 game::RandomizeTown for HoMM1's 4x3 town footprint: the town
// type comes from the campaign crest, a distinct roll for the first four
// towns or a plain roll, and shifts every town frame to that type.
VA(0x00442935, 0x67f)
void game::RandomizeTown(signed char x, signed char y, signed char isCastle) {
    signed char j;
    signed char unique;
    town* town;
    signed char i;
    unsigned char frameShift;
    signed char townNum;
    signed char race;
    signed char plain;

    townNum = GetTownId(x, y);
    for (j = 0; j < 3; j++) {
        for (i = 0; i < 4; i++) {
            if ((m_map[x - 2 + i][y - 2 + j].m_triggerType & MAP_TRIGGER_TYPE_MASK) > 0
                && (m_map[x - 2 + i][y - 2 + j].m_triggerType & MAP_TRIGGER_TYPE_MASK) <= 0x30) {
                m_map[x - 2 + i][y - 2 + j].m_secondaryTrigger |= 0x28;
            } else {
                m_map[x - 2 + i][y - 2 + j].m_triggerType = MAP_OBJECT_TOWN;
                m_map[x - 2 + i][y - 2 + j].m_objectMetadata = townNum;
            }
        }
    }
    m_map[x][y].m_triggerType |= MAP_TRIGGER_EVENT;
    town = GetTown(townNum);
    town->m_turnsOwned = 10;
    if (m_campaignType > 0 && m_campaignScenario >= 4 && m_campaignScenario <= 7 && town->m_owner == 0) {
        race = gCrestTownTypes[m_players[0].m_color];
    } else if (townNum < 4) {
        unique = 0;
        race = 0;
        while (!unique) {
            race = Random(0, 3);
            unique = 1;
            for (i = 0; i < 4; i++) {
                if (gRandomTownTypes[i] == race)
                    unique = 0;
            }
        }
        gRandomTownTypes[townNum] = race;
    } else {
        race = Random(0, 3);
    }
    frameShift = (4 - race) * 24;
    for (i = 0; i < 4; i++) {
        m_map[x - 2 + i][y - 2].m_overlayIndex -= frameShift;
        m_map[x - 2 + i][y - 1].m_objectIndex -= frameShift;
        m_map[x - 2 + i][y].m_objectIndex -= frameShift;
    }
    m_castleRecs[townNum].m_type = race;
    plain = 1;
    if (town->m_extraIndex >= 1 && static_cast<mapTownExtra*>(ppMapExtra[town->m_extraIndex])->customized)
        plain = 0;
    if (plain) {
        if (race == 2)
            m_castleRecs[townNum].m_buildings = 0x2000;
        else
            m_castleRecs[townNum].m_buildings = 0;
    }
    if (isCastle) {
        m_castleRecs[townNum].m_buildings |= ((1 << BUILDING_SLOT_CASTLE) | (1 << BUILDING_SLOT_DWELLING_1));
        m_castleRecs[townNum].m_garrison[0] = gMonsterDatabase[gDwellingType[race][0]].growth;
        if (m_castleRecs[townNum].m_buildings & (1 << BUILDING_SLOT_TENT))
            m_castleRecs[townNum].m_buildings -= (1 << BUILDING_SLOT_TENT);
    } else {
        m_castleRecs[townNum].m_buildings |= (1 << BUILDING_SLOT_TENT);
        if (m_castleRecs[townNum].m_buildings & (1 << BUILDING_SLOT_CASTLE))
            m_castleRecs[townNum].m_buildings -= (1 << BUILDING_SLOT_CASTLE);
        SetupTown(townNum, 0);
    }
}

// Buka 2.1 game::SetupTowns' per-town tail: default dwellings for towns the
// map leaves uncustomized, then nine distinct mage-guild spells; computer
// owners favour the stronger spells.
VA(0x00442fb4, 0x3b4)
void game::SetupTown(signed char townId, signed char aiOwned) {
    short dwellingCount;
    char dwellingRoll[10];
    int k;
    signed char used[29];
    signed char townType;
    short newSpell;
    short spellValue;
    int spellLevel;

    dwellingRoll[0] = 1;
    dwellingRoll[1] = 1;
    dwellingRoll[2] = 1;
    dwellingRoll[3] = 2;
    dwellingRoll[4] = 1;
    dwellingRoll[5] = 1;
    dwellingRoll[6] = 1;
    dwellingRoll[7] = 2;
    dwellingRoll[8] = 1;
    dwellingRoll[9] = 3;
    dwellingCount = dwellingRoll[Random(0, 99) / 10];
    townType = m_castleRecs[townId].m_type;
    if (m_castleRecs[townId].m_customized) {
        for (k = 0; k < 6; k++) {
            if (m_castleRecs[townId].m_buildings & (1 << (k + 7)))
                m_castleRecs[townId].m_garrison[k] = gMonsterDatabase[gDwellingType[townType][k]].growth;
        }
    }
    if (!m_castleRecs[townId].m_customized) {
        m_castleRecs[townId].m_buildings |= (1 << BUILDING_SLOT_DWELLING_1);
        m_castleRecs[townId].m_garrison[0] = gMonsterDatabase[gDwellingType[townType][0]].growth;
        if (aiOwned && dwellingCount == 1 && Random(1, 10) < 4)
            dwellingCount++;
        if (--dwellingCount) {
            m_castleRecs[townId].m_buildings |= 0x100;
            m_castleRecs[townId].m_garrison[1] = gMonsterDatabase[gDwellingType[townType][1]].growth;
            dwellingCount--;
        }
    }
    memset(used, 0, 29);
    for (k = 0; k < 9; k++) {
        if (k <= 2)
            spellLevel = 0;
        else if (k <= 4)
            spellLevel = 1;
        else if (k <= 6)
            spellLevel = 2;
        else
            spellLevel = 3;
        do {
            newSpell = gMageGuildSpellPool[spellLevel][Random(0, 7)];
            if (aiOwned)
                spellValue = giSpellAIValue[newSpell] * (gcSpellAIFlags[newSpell] & 1 ? 4 : 1) + 50;
            else
                spellValue = 1500;
            if (newSpell == SPELL_DIMENSION_DOOR)
                spellValue = 1500;
        } while (used[newSpell] || Random(1, 1500) >= spellValue);
        m_castleRecs[townId].m_mageGuildSpells[k] = newSpell;
        used[newSpell] = 1;
    }
}

// Buka 2.1 game::RandomizeMine for HoMM1's 2x2 mines: the terrain picks
// the mine type (unused types first) and the object and shadow frames.
VA(0x00443368, 0x659)
void game::RandomizeMine(signed char x, signed char y) {
    unsigned char bits;
    unsigned char upFrame;
    signed char k;
    int tries;
    signed char j;
    signed char terrain;
    signed char type;
    signed char mineIdx;
    unsigned char objFrame;

    terrain = giGroundToTerrain[m_map[x][y].m_tileIndex];
    for (tries = 0; tries < 30; tries++) {
        switch (terrain) {
            case TERRAIN_GRASS:
            case TERRAIN_DIRT:
                type = Random(1, 6);
                if (type == RESOURCE_MERCURY)
                    type = RESOURCE_WOOD;
                break;
            case TERRAIN_SNOW:
                type = Random(2, 6);
                break;
            case TERRAIN_SWAMP:
                type = Random(0, 6);
                break;
            case TERRAIN_LAVA:
                type = RESOURCE_MERCURY;
                break;
            case TERRAIN_DESERT:
            default:
                type = Random(1, 6);
                break;
        }
        if (!giMineTypeCount[type])
            tries = 30;
    }
    giMineTypeCount[type]++;
    switch (type) {
        case RESOURCE_WOOD:
            upFrame = 5;
            break;
        case RESOURCE_MERCURY:
            upFrame = 0x19;
            break;
        default:
            switch (terrain) {
                case TERRAIN_GRASS:
                    upFrame = 0xf;
                    break;
                case TERRAIN_SNOW:
                    upFrame = 0x13;
                    break;
                default:
                    upFrame = 9;
                    break;
            }
            break;
    }
    switch (type) {
        case RESOURCE_WOOD:
            objFrame = 7;
            break;
        case RESOURCE_MERCURY:
            switch (terrain) {
                case TERRAIN_SWAMP:
                    objFrame = 0x2b;
                    break;
                case TERRAIN_LAVA:
                    objFrame = 0x23;
                    break;
                default:
                    objFrame = 0x1b;
                    break;
            }
            break;
        default:
            switch (terrain) {
                case TERRAIN_GRASS:
                    objFrame = 0x11;
                    break;
                case TERRAIN_SNOW:
                    objFrame = 0x15;
                    break;
                case TERRAIN_SWAMP:
                    objFrame = 0x17;
                    break;
                case TERRAIN_DESERT:
                    objFrame = 0xd;
                    break;
                default:
                    objFrame = 0xb;
                    break;
            }
            break;
    }
    m_map[x][y].m_objectIndex = objFrame;
    m_map[x + 1][y].m_objectIndex = objFrame + 1;
    m_map[x][y - 1].m_overlayIndex = upFrame;
    m_map[x + 1][y - 1].m_overlayIndex = upFrame + 1;
    if (type == RESOURCE_MERCURY) {
        m_map[x + 1][y].m_flags |= 4;
        bits = MAP_OBJECT_ALCHEMIST_LAB;
    } else if (type == RESOURCE_WOOD) {
        bits = MAP_OBJECT_SAWMILL;
    } else {
        m_map[x + 1][y].m_flags |= 0x10;
        m_map[x + 1][y].m_objectTileset |= 0xb0;
        m_map[x + 1][y].m_extraFrame = type - 2;
        bits = MAP_OBJECT_MINE;
    }
    mineIdx = GetMineId(x, y);
    for (k = 0; k < 2; k++) {
        for (j = 0; j < 2; j++) {
            if ((m_map[x + j][y - k].m_triggerType & MAP_TRIGGER_TYPE_MASK) > 0
                && (m_map[x + j][y - k].m_triggerType & MAP_TRIGGER_TYPE_MASK) <= 0x30) {
                m_map[x + j][y - k].m_secondaryTrigger |= bits;
            } else {
                m_map[x + j][y - k].m_objectMetadata = mineIdx;
                m_map[x + j][y - k].m_triggerType = bits;
            }
        }
    }
    m_map[x][y].m_triggerType |= MAP_TRIGGER_EVENT;
    m_mines[mineIdx].type = type;
}

// HoMM1 picks an unused random artifact (ids 4..36), else the first free one.
VA(0x004439c1, 0x79)
signed char game::GetRandomArtifactId(void) {
    signed char freeSlot = Scan(m_randomArtifacts, 4, 33);
    if (freeSlot == -1)
        return -1;
    signed char artifact = RandomScan(m_randomArtifacts, 4, 33, 37);
    if (artifact == ARTIFACT_NONE)
        return freeSlot;
    else
        return artifact;
}

// Buka 2.1 game::RandomizeHeroPool without HoMM2's starting spells.
VA(0x00443a3a, 0x102)
void game::RandomizeHeroPool(void) {
    short heroId;
    for (heroId = 0; heroId < GAME_HERO_COUNT; heroId++) {
        m_heroRecs[heroId].m_experience = Random(0, 50) + 40;
        SetRandomHeroArmies(heroId, 0);
        m_heroRecs[heroId].m_remainingMobility = m_heroRecs[heroId].CalcMobility();
        m_heroRecs[heroId].m_mobility = m_heroRecs[heroId].m_remainingMobility;
        m_heroRecs[heroId].m_randomSeed = Random(1, 16000);
    }
}

// Buka 2.1 game::SetRandomHeroArmies: HoMM1 has four classes and draws
// only from the first two stacks of each class table.
VA(0x00443b3c, 0x2e0)
void game::SetRandomHeroArmies(short heroId, int strongArmy) {
    armyGroup* army = &m_heroRecs[heroId].m_army;
    short slot = 0;
    short armyTable[4][3][3] = {
        {{0, 30, 50}, {1, 3, 5}, {2, 2, 4}},
        {{6, 15, 25}, {7, 3, 5}, {8, 2, 3}},
        {{12, 10, 20}, {13, 2, 4}, {14, 1, 2}},
        {{18, 6, 10}, {19, 2, 4}, {20, 1, 2}}
    };
    int present[3];
    int i;
    int max;
    int minNum;

    present[0] = 1;
    present[1] = Random(0, 99) < 50 + (strongArmy ? 30 : 0);
    present[2] = Random(0, 99) < 25 + (strongArmy ? 40 : 0);
    if (!present[2])
        present[1] = 1;
    for (i = 0; i < ARMY_GROUP_SLOT_COUNT; i++) {
        army->m_creatureTypes[i] = CREATURE_NONE;
        army->m_creatureCounts[i] = -1;
    }
    for (i = 0; i < 2; i++) {
        if (present[i]) {
            army->m_creatureTypes[slot] = armyTable[m_heroRecs[heroId].m_heroClass][i][0];
            minNum = armyTable[m_heroRecs[heroId].m_heroClass][i][1] * 10;
            max = armyTable[m_heroRecs[heroId].m_heroClass][i][2] * 10 + 9;
            if (strongArmy)
                minNum = (minNum + max) / 2;
            army->m_creatureCounts[slot] = Random(minNum, max) / 10;
            slot++;
        }
    }
}

// Buka 2.1 game::ProcessRandomObjects for HoMM1's random towns, castles,
// monsters by strength band, resources, artifacts and mines; NewMap runs
// the castles-only pass first.
VA(0x00443e1c, 0x2cd)
void game::ProcessRandomObjects(int castlesOnly) {
    mapCell* cellPtr;
    int lowFV;
    int y;
    int i;
    int x;
    int highFV;

    for (i = 0; i < 7; i++)
        giMineTypeCount[i] = 0;
    for (i = 0; i < 4; i++)
        gRandomTownTypes[i] = -1;
    for (y = 0; y < MAP_CELL_GRID_SIZE; y++) {
        for (x = 0; x < MAP_CELL_GRID_SIZE; x++) {
            cellPtr = &m_map[x][y];
            if (!castlesOnly || cellPtr->m_triggerType == (MAP_TRIGGER_EVENT | MAP_FILE_OBJECT_RANDOM_CASTLE)) {
                switch (cellPtr->m_triggerType) {
                    case MAP_TRIGGER_EVENT | MAP_FILE_OBJECT_RANDOM_TOWN:
                        RandomizeTown(x, y, 0);
                        break;
                    case MAP_TRIGGER_EVENT | MAP_FILE_OBJECT_RANDOM_CASTLE:
                        RandomizeTown(x, y, 1);
                        break;
                    case MAP_TRIGGER_EVENT | MAP_FILE_OBJECT_RANDOM_MONSTER:
                        lowFV = 80;
                        highFV = 2000;
                        goto pickMonster;
                    case MAP_TRIGGER_EVENT | MAP_FILE_OBJECT_RANDOM_MONSTER_WEAK:
                        lowFV = 0;
                        highFV = 400;
                        goto pickMonster;
                    case MAP_TRIGGER_EVENT | MAP_FILE_OBJECT_RANDOM_MONSTER_MEDIUM:
                        lowFV = 80;
                        highFV = 1000;
                        goto pickMonster;
                    case MAP_TRIGGER_EVENT | MAP_FILE_OBJECT_RANDOM_MONSTER_STRONG:
                        lowFV = 500;
                        highFV = 2500;
                        goto pickMonster;
                    case MAP_TRIGGER_EVENT | MAP_FILE_OBJECT_RANDOM_MONSTER_VERY_STRONG:
                        lowFV = 2000;
                        highFV = 100000;
                        goto pickMonster;
                    pickMonster:
                        cellPtr->m_triggerType = (MAP_TRIGGER_EVENT | MAP_OBJECT_MONSTER);
                        cellPtr->m_objectIndex = Random(0, 27);
                        while (gMonsterDatabase[cellPtr->m_objectIndex].fightValue <= lowFV
                               || gMonsterDatabase[cellPtr->m_objectIndex].fightValue >= highFV)
                            cellPtr->m_objectIndex = Random(0, 27);
                        break;
                    case MAP_TRIGGER_EVENT | MAP_FILE_OBJECT_RANDOM_RESOURCE:
                        cellPtr->m_triggerType = (MAP_TRIGGER_EVENT | MAP_OBJECT_RESOURCE);
                        cellPtr->m_objectIndex = Random(61, 67);
                        break;
                    case MAP_TRIGGER_EVENT | MAP_FILE_OBJECT_RANDOM_ARTIFACT:
                        cellPtr->m_triggerType = (MAP_TRIGGER_EVENT | MAP_OBJECT_ARTIFACT);
                        cellPtr->m_objectIndex = GetRandomArtifactId();
                        m_randomArtifacts[cellPtr->m_objectIndex] = 36;
                        break;
                    case MAP_TRIGGER_EVENT | MAP_FILE_OBJECT_RANDOM_MINE:
                        RandomizeMine(x, y);
                        break;
                }
            }
        }
    }
}

// donor PoL RVA 0x00080b64; preferred Buka symbol ?SetVisibility@game@@QAEXHHHH@Z
// donor Buka TU SOURCE/GAME; HoMM1 owner inferred from contiguous order
// evidence: graph:3;base=0.449662;margin=0.505159;shape=0.181;size=0.875;calls=1.000;alternate=pol20:void game::SetVisibility(int, int, int, int)@0x00080b64
VA(0x004440e9, 0x259)
void game::SetVisibility(short x, short y, short player, short radius) {
    int i;
    int j;
    int rangeLeft;
    int cutoff;
    unsigned char viewMask = 1 << player;
    unsigned char outerMask = 1 << (player + 4);

    if (radius >= 5)
        cutoff = 3;
    else
        cutoff = 2;

    for (j = y - radius; j <= y + radius; ++j) {
        for (i = x - radius; i <= x + radius; ++i) {
            rangeLeft = radius - abs(y - j) + radius - abs(x - i);
            if (rangeLeft >= cutoff && i >= 0 && j >= 0 && i < MAP_CELL_GRID_SIZE
                && j < MAP_CELL_GRID_SIZE)
                gpGame->m_mapExtra[i][j] |= viewMask;
        }
    }
    if (gbHumanPlayer[player]) {
        for (j = y - radius - 1; j <= y + radius + 1; ++j) {
            for (i = x - radius - 1; i <= x + radius + 1; ++i) {
                rangeLeft = radius - abs(y - j) + radius - abs(x - i);
                if (rangeLeft + 1 >= cutoff && i >= 0 && j >= 0 && i < MAP_CELL_GRID_SIZE
                    && j < MAP_CELL_GRID_SIZE)
                    gpGame->m_mapExtra[i][j] |= outerMask;
            }
        }
    }
}

// donor PoL RVA 0x00080e6c; preferred Buka symbol ?GiveArmy@game@@QAEXPAVarmyGroup@@HHH@Z
// donor Buka TU SOURCE/GAME; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.542661;margin=0.479075;shape=0.407;size=0.794;calls=1.000;alternate=pol20:void game::GiveArmy(class armyGroup *, int, int, int)@0x00080e6c
VA(0x00444342, 0xfc)
void game::GiveArmy(armyGroup* group, int type, int count, int slot) {
    int swap;
    int i;
    if (slot >= 0) {
        i = slot;
        group->m_creatureTypes[i] = type;
        group->m_creatureCounts[i] = 0;
    } else {
        for (i = 0; i < ARMY_GROUP_SLOT_COUNT; ++i) {
            if (group->m_creatureTypes[i] == type)
                break;
        }
        if (i >= ARMY_GROUP_SLOT_COUNT) {
            for (i = 0; i < ARMY_GROUP_SLOT_COUNT; ++i) {
                if (group->m_creatureTypes[i] < 0) {
                    group->m_creatureCounts[i] = 0;
                    break;
                }
            }
        }
        if (i >= ARMY_GROUP_SLOT_COUNT)
            return;
    }
    group->m_creatureTypes[i] = type;
    group->m_creatureCounts[i] += count;
}

// donor PoL RVA 0x00080f68; preferred Buka symbol ?ExperienceValueOfStack@game@@QAEHPAVarmyGroup@@PAVhero@@@Z
// donor Buka TU SOURCE/GAME; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.491573;margin=0.501516;shape=0.250;size=0.900;calls=1.000;alternate=pol20:int game::ExperienceValueOfStack(class armyGroup *, class hero *)@0x00080f68
VA(0x0044443e, 0x8c)
int game::ExperienceValueOfStack(armyGroup* group, hero* h) {
    int expValue = 0;
    int i;
    for (i = 0; i < ARMY_GROUP_SLOT_COUNT; ++i) {
        if (group->m_creatureCounts[i] > 0)
            expValue += group->m_creatureCounts[i] * gMonsterDatabase[group->m_creatureTypes[i]].hitPoints;
    }
    if (h)
        expValue += 500;
    return expValue;
}

// Buka 2.1 MiscRuntime seeded generator; HoMM1 keeps it in this TU.
VA(0x004444ca, 0x92)
int SGenRand(void)
{
    int bitMask;
    int value = 0;
    int i;
    iLastSeed &= 0xfff;
    iLastSeed *= 7;
    iLastSeed += (iLastSeed & 0xff0) >> 4;
    for (i = 31; i >= 0; --i) {
        bitMask = 1 << i;
        if (iLastSeed & bitMask)
            value |= 1 << i;
    }
    return value;
}

VA(0x0044455c, 0x52)
int SRandom(int low, int high)
{
    int result;
    SIncRandomize(low, high);
    result = SGenRand();
    iLastSeed += low;
    iLastSeed += high * 8;
    return result % (high - low + 1) + low;
}

VA(0x004445ae, 0x89)
void SIncRandomize(int x, int y)
{
    int feedback;
    x *= 13;
    y *= 13;
    x &= 0xff;
    y &= 0xff;
    iLastSeed += y << 5;
    iLastSeed += x * 13233;
    iLastSeed += y;
    feedback = iLastSeed & 0x3f;
    iLastSeed += feedback << 8;
}

VA(0x00444637, 0x24)
void SRand(int seed)
{
    iLastSeed = seed;
    srand(seed);
}

// donor PoL RVA 0x00080ff9; preferred Buka symbol ?GetLuck@game@@QAEHPAVhero@@PAVarmy@@PAVtown@@@Z
// donor Buka TU SOURCE/GAME; HoMM1 owner inferred from contiguous order
// evidence: graph:3;base=0.340271;margin=0.529148;shape=0.188;size=0.654;calls=0.667;alternate=pol20:int game::GetLuck(class hero *, class army *, class town *)@0x00080ff9
VA(0x0044465b, 0xbf)
int game::GetLuck(hero* h, army*) {
    int luck;

    if (!h)
    return 0;
    luck = 0;
    if (h->HasArtifact(ARTIFACT_LUCKY_RABBITS_FOOT))
        luck++;
    if (h->HasArtifact(ARTIFACT_GOLDEN_HORSESHOE))
        luck++;
    if (h->HasArtifact(ARTIFACT_GAMBLERS_LUCKY_COIN))
        luck++;
    if (h->HasArtifact(ARTIFACT_FOUR_LEAF_CLOVER))
        luck++;
    luck += h->m_luck;
    if (luck < -3)
        luck = -3;
    if (luck > 3)
        luck = 3;
    return luck;
}

// Buka 2.1 keeps the scan cursor in file statics.
DATA(0x004c50fc) static int s_adjacentMonsterEndX;
DATA(0x004c5100) static int s_adjacentMonsterEndY;
DATA(0x004c50d4) static int s_adjacentMonsterX;
DATA(0x004c50d8) static int s_adjacentMonsterY;
DATA(0x004c50ec) static int s_adjacentMonsterMinX;
DATA(0x004c50f0) static int s_adjacentMonsterMinY;

// donor PoL RVA 0x00069bef; preferred Buka symbol ?FindAdjacentMonster@advManager@@QAEHHHPAH0HH@Z
// donor Buka TU SOURCE/ADVMGR; HoMM1 owner inferred from contiguous order
// evidence: graph:3;base=0.502628;margin=0.574839;shape=0.364;size=0.953;calls=0.667;alternate=pol20:int advManager::FindAdjacentMonster(int, int, int *, int *, int, int)@0x00069bef
// HoMM1 retail returns the found flag in AL (xor al,al / mov al,1).
VA(0x0044471a, 0x350)
signed char advManager::FindAdjacentMonster(
    int originX, int originY, int* monsterX, int* monsterY, int excludedX, int excludedY
) {
    s_adjacentMonsterEndX = originX + 2;
    s_adjacentMonsterEndY = originY + 2;

    if (originX > 0 && originY > 0 && originX < MAP_CELL_GRID_SIZE - 1
        && originY < MAP_CELL_GRID_SIZE - 1) {
        for (s_adjacentMonsterX = originX - 1; s_adjacentMonsterX < s_adjacentMonsterEndX;
             ++s_adjacentMonsterX) {
            for (s_adjacentMonsterY = originY - 1; s_adjacentMonsterY < s_adjacentMonsterEndY;
                 ++s_adjacentMonsterY) {
                if (m_mapData[s_adjacentMonsterX][s_adjacentMonsterY].m_triggerType == (MAP_TRIGGER_EVENT | MAP_OBJECT_MONSTER)) {
                    if (s_adjacentMonsterY < originY) {
                        if ((GetCell(originX, originY)->m_objectIndex == 0xff
                             || (GetCell(originX, originY)->m_flags & 0x80))
                            && (s_adjacentMonsterX != excludedX
                                || s_adjacentMonsterY != excludedY))
                            goto foundAdjacentMonster;
                    } else if (s_adjacentMonsterX != excludedX || s_adjacentMonsterY != excludedY) {
                        goto foundAdjacentMonster;
                    }
                }
            }
        }
    } else {
        if (originX == MAP_CELL_GRID_SIZE - 1)
            s_adjacentMonsterEndX = originX + 1;
        if (originY == MAP_CELL_GRID_SIZE - 1)
            s_adjacentMonsterEndY = originY + 1;
        s_adjacentMonsterMinX = originX == 0 ? 0 : originX - 1;
        s_adjacentMonsterMinY = originY == 0 ? 0 : originY - 1;

        for (s_adjacentMonsterX = s_adjacentMonsterMinX; s_adjacentMonsterX < s_adjacentMonsterEndX;
             ++s_adjacentMonsterX) {
            for (s_adjacentMonsterY = s_adjacentMonsterMinY;
                 s_adjacentMonsterY < s_adjacentMonsterEndY;
                 ++s_adjacentMonsterY) {
                if (m_mapData[s_adjacentMonsterX][s_adjacentMonsterY].m_triggerType == (MAP_TRIGGER_EVENT | MAP_OBJECT_MONSTER)) {
                    if (s_adjacentMonsterY < originY) {
                        if ((GetCell(originX, originY)->m_objectIndex == 0xff
                             || (GetCell(originX, originY)->m_flags & 0x80))
                            && (s_adjacentMonsterX != excludedX
                                || s_adjacentMonsterY != excludedY))
                            goto foundAdjacentMonster;
                    } else if (s_adjacentMonsterX != excludedX || s_adjacentMonsterY != excludedY) {
                        goto foundAdjacentMonster;
                    }
                }
            }
        }
    }
    return 0;

foundAdjacentMonster:
    *monsterX = s_adjacentMonsterX;
    *monsterY = s_adjacentMonsterY;
    return 1;
}

// donor PoL RVA 0x0008111f; preferred Buka symbol ?SetupAdjacentMons@game@@QAEXXZ
// donor Buka TU SOURCE/GAME; HoMM1 owner inferred from contiguous order
// evidence: graph:3;base=0.519474;margin=0.741945;shape=0.279;size=0.996;calls=1.000;alternate=pol20:void game::SetupAdjacentMons(void)@0x0008111f
VA(0x00444a6a, 0xde)
void game::SetupAdjacentMons(void) {
    int x;
    int y;
    int mask = 0x7f;
    int monY;
    int monX;

    for (x = 0; x < MAP_CELL_GRID_SIZE; ++x) {
        for (y = 0; y < MAP_CELL_GRID_SIZE; ++y) {
            if (gpAdvManager->FindAdjacentMonster(x, y, &monX, &monY, -1, -1))
                mapExtra[x][y] |= 0x80;
            else
                mapExtra[x][y] &= mask;
        }
    }
}

// donor PoL RVA 0x00081210; preferred Buka symbol ?CancelComputerScreen@game@@QAEXXZ
// donor Buka TU SOURCE/GAME; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.526467;margin=0.434153;shape=0.371;size=0.866;calls=1.000;alternate=pol20:void game::CancelComputerScreen(void)@0x00081210
VA(0x00444b48, 0x61)
void game::CancelComputerScreen(void) {
    TurnOffAIMusic();
    bShowIt = 1;
    int i;
    for (i = 1; i <= 6; ++i)
        gpWindowManager->BroadcastMessage(
            MESSAGE_WIDGET,
            WIDGET_COMMAND_CLEAR_FLAGS,
            i,
            WIDGET_FLAG_UPDATE | WIDGET_FLAG_DIMMED
        );
}

// donor PoL RVA 0x00081271; preferred Buka symbol ?ShowComputerScreen@game@@QAEXXZ
// donor Buka TU SOURCE/GAME; HoMM1 owner inferred from contiguous order
// evidence: graph:3;base=0.481260;margin=0.673888;shape=0.345;size=0.812;calls=0.778;alternate=pol20:void game::ShowComputerScreen(void)@0x00081271
VA(0x00444ba9, 0x115)
void game::ShowComputerScreen(void) {
    if (gConfig.blackoutComputer || gbRemoteOn) {
        int saved = gbThisNetHumanPlayer[giCurPlayer];
        gbThisNetHumanPlayer[giCurPlayer] = 1;
        int i;
        for (i = 1; i <= 6; ++i)
            gpWindowManager->BroadcastMessage(
                MESSAGE_WIDGET,
                WIDGET_COMMAND_SET_FLAGS,
                i,
                WIDGET_FLAG_UPDATE | WIDGET_FLAG_DIMMED
            );
        gpMouseManager->ReallyHidePointer();
        gbAllBlack = 1;
        gpAdvManager->CompleteDraw(1);
        gpAdvManager->UpdateHeroLocators(1, 1);
        gpAdvManager->UpdateTownLocators(1, 1);
        gpAdvManager->UpdBottomView(1, 1, 1);
        gpAdvManager->UpdateScreen(0, 1);
        gbAllBlack = 0;
        gbThisNetHumanPlayer[giCurPlayer] = saved;
        gpMouseManager->ReallyShowPointer();
    }
    ShowHeroesLogo();
}

// Buka 2.1 game::ShowHeroesLogo; HoMM1 draws the logo from a tileset.
VA(0x00444cbe, 0xa8)
void game::ShowHeroesLogo(void) {
    tileset* logo;
    if (!gpAdvManager->m_openState) {
        gpMouseManager->ReallyHidePointer();
        gpAdvManager->m_openState = 1;
        logo = gpResourceManager->GetTileset("herologo.til");
        TileToBitmap(logo, 0, gpWindowManager->m_screen, 480, 16);
        gpWindowManager->UpdateScreenRegion(480, 16, 144, 144);
        gpResourceManager->Dispose(logo);
        gpMouseManager->ReallyShowPointer();
    }
}

// donor PoL RVA 0x000813fe; preferred Buka symbol ?WaitForPlayer@game@@QAEXPADH@Z
// donor Buka TU SOURCE/GAME; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.547163;margin=0.571086;shape=0.476;size=0.842;calls=0.833;alternate=pol20:void game::WaitForPlayer(char *, int)@0x000813fe
VA(0x00444d66, 0x155)
void game::WaitForPlayer(char* text, int player) {
    if (gbBlackoutPlayer && giNumHumanPlayers > 1 && !gbRemoteOn) {
        gpMouseManager->SetPointer(ADVENTURE_POINTER_DEFAULT);
        gbAllBlack = 1;
        giBottomViewOverrideEndTime = KBTickCount() + 9999999;
        if (gbThisNetHumanPlayer[giCurPlayer])
            giBottomViewOverride = 1;
        else
            giBottomViewOverride = 0;
        gpSoundManager->m_musicReady = 1;
        gpSoundManager->SwitchAmbientMusic(15);
        gpMouseManager->ReallyHidePointer();
        gpAdvManager->CompleteDraw(1);
        gpAdvManager->UpdateHeroLocators(1, 1);
        gpAdvManager->UpdateTownLocators(1, 1);
        gpAdvManager->UpdateScreen(0, 1);
        ShowHeroesLogo();
        gbAllBlack = 0;
        gpMouseManager->ReallyShowPointer();
        NormalDialog(text, NORMAL_DIALOG_TYPE_OK, 0x61, -1, NORMAL_DIALOG_CREST, gpGame->m_players[player].m_color, NORMAL_DIALOG_NO_RESOURCE, 0, NORMAL_DIALOG_NO_OR_TEXT);
        gpSoundManager->SwitchAmbientMusic(MUSIC_TRACK_NONE);
    }
}

// HoMM1 rerolls the variant within each four-tile group, past the first
// four tiles of every twenty-tile terrain block.
VA(0x00444ebb, 0xb2)
void game::RandomizeTerrainTiles(void) {
    mapCell* cellPtr;
    // Retail reserves an unused slot above the loop counters.
    int tile;
    int x;
    int y;
    for (y = 0; y < MAP_CELL_GRID_SIZE; y++) {
        for (x = 0; x < MAP_CELL_GRID_SIZE; x++) {
            cellPtr = &m_map[x][y];
            if (cellPtr->m_tileIndex % 20 >= 4)
                cellPtr->m_tileIndex = cellPtr->m_tileIndex / 4 * 4 + Random(0, 3);
        }
    }
}

// Buka 2.1 game::ProcessMapExtra reduced to HoMM1's town extras; HoMM1 has
// no late overlays.
VA(0x00444f6d, 0x129)
void game::ProcessMapExtra(void) {
    int y;
    mapCell* cellPtr;
    int x;
    signed char townNum;
    for (y = 0; y < MAP_CELL_GRID_SIZE; y++) {
        for (x = 0; x < MAP_CELL_GRID_SIZE; x++) {
            cellPtr = &m_map[x][y];
            switch (cellPtr->m_triggerType) {
                case MAP_TRIGGER_EVENT | MAP_OBJECT_TOWN:
                case MAP_TRIGGER_EVENT | MAP_FILE_OBJECT_RANDOM_TOWN:
                case MAP_TRIGGER_EVENT | MAP_FILE_OBJECT_RANDOM_CASTLE:
                    townNum = GetTownId(x, y);
                    m_castleRecs[townNum].m_extraIndex = cellPtr->m_objectMetadata;
                    cellPtr->m_objectMetadata = townNum;
                    break;
                case MAP_TRIGGER_EVENT | MAP_FILE_OBJECT_HERO:
                    m_noMapHeroes = 0;
                    break;
            }
        }
    }
}

// Buka 2.1 game::SetupTowns reduced to HoMM1's owners, garrisons and
// buildings; a map whose towns all lack owners leaves the placeholder -2.
VA(0x00445096, 0x213)
signed char game::SetupTowns(void) {
    mapTownExtra* extra;
    int own;
    signed char noOwners;
    town* town;
    int j;
    int i;
    int mask;
    noOwners = 1;
    mask = 0x1f9f;
    for (i = 0; i < GAME_TOWN_COUNT; i++) {
        town = GetTown(i);
        town->m_customized = 0;
        if (town->m_extraIndex >= 1) {
            extra = static_cast<mapTownExtra*>(ppMapExtra[town->m_extraIndex]);
            if (extra->customized && extra->owner != -2) {
                if (gpGame->m_playerCount <= extra->owner)
                    own = gpGame->m_playerCount - 1;
                else
                    own = extra->owner;
                noOwners = 0;
                if (own != -1)
                    ClaimTown(i, own);
            }
            if (extra->customized) {
                town->m_customized = 1;
                for (j = 0; j < ARMY_GROUP_SLOT_COUNT; j++) {
                    town->m_army.m_creatureCounts[j] = extra->troopCounts[j];
                    if (town->m_army.m_creatureCounts[j] > 0)
                        town->m_army.m_creatureTypes[j] = extra->troopTypes[j];
                    else
                        town->m_army.m_creatureTypes[j] = CREATURE_NONE;
                }
                town->m_buildState = extra->buildState;
                town->m_buildings = town->m_buildings - (town->m_buildings & mask) + (extra->buildings & mask);
            }
        }
    }
    if (!noOwners) {
        for (i = 0; i < GAME_TOWN_COUNT; i++) {
            town = GetTown(i);
            if (town->m_owner == -2)
                town->m_owner = -1;
        }
    }
    return noOwners;
}

// Buka 2.1 game::ProcessOnMapHeroes for HoMM1: each placed hero takes its
// map-extra garrison, artifacts, experience and owner; a hero standing at a
// town gate occupies the town.
VA(0x004452a9, 0x347)
void game::ProcessOnMapHeroes(void) {
    int mapY;
    mapHeroExtra* extra;
    town* town;
    int townId;
    int iPlayer;
    int k;
    int j;
    int mapX;
    mapCell* north;
    mapCell* cell;
    hero* theHero;

    for (mapY = 0; mapY < MAP_CELL_GRID_SIZE; mapY++) {
        for (mapX = 0; mapX < MAP_CELL_GRID_SIZE; mapX++) {
            cell = &m_map[mapX][mapY];
            if ((cell->m_triggerType & MAP_TRIGGER_TYPE_MASK) == 0x47) {
                extra = static_cast<mapHeroExtra*>(ppMapExtra[static_cast<unsigned char>(cell->m_objectMetadata)]);
                theHero = GetHero(extra->heroId);
                for (k = 0; k < ARMY_GROUP_SLOT_COUNT; k++) {
                    theHero->m_army.m_creatureCounts[k] = extra->troopCounts[k];
                    if (theHero->m_army.m_creatureCounts[k] > 0)
                        theHero->m_army.m_creatureTypes[k] = extra->troopTypes[k];
                    else
                        theHero->m_army.m_creatureTypes[k] = CREATURE_NONE;
                }
                for (j = 0; j < 4; j++) {
                    if (extra->artifacts[j] >= 0)
                        gpAdvManager->GiveArtifact(theHero, extra->artifacts[j]);
                }
                theHero->m_experience = 0;
                gpAdvManager->GiveExperience(theHero, extra->experience, 1);
                theHero->CheckLevel();
                theHero->m_x = mapX;
                theHero->m_y = mapY;
                if (gpGame->m_playerCount <= extra->owner)
                    iPlayer = gpGame->m_playerCount - 1;
                else
                    iPlayer = extra->owner;
                theHero->m_owner = iPlayer;
                m_availableHeroes[extra->heroId] = iPlayer;
                m_players[theHero->m_owner].m_heroIds[m_players[theHero->m_owner].m_heroCount] = theHero->m_id;
                m_players[theHero->m_owner].m_heroCount++;
                if (mapY > 0) {
                    north = &m_map[mapX][mapY - 1];
                    if (north->m_triggerType == (MAP_TRIGGER_EVENT | MAP_OBJECT_TOWN)) {
                        theHero->m_y--;
                        townId = GetTownId(mapX, mapY - 1);
                        town = GetTown(townId);
                        town->m_occupyingHeroId = theHero->m_id;
                    }
                }
                cell->m_objectTileset = 0;
                cell->m_objectIndex = 0xff;
                cell->m_overlayTileset = 0;
                cell->m_overlayIndex = 0xff;
                cell->m_objectMetadata = 0;
                cell->m_triggerType = MAP_OBJECT_NONE;
                SetVisibility(theHero->m_x, theHero->m_y, theHero->m_owner, gHeroScoutRadius[theHero->m_heroClass]);
            }
        }
    }
    CheckHeroConsistency();
}

// Buka 2.1 game::CheckHeroConsistency for HoMM1: replaces tavern heroes
// some player already owns, clears map heroes that lost their owner and
// zeroes the counts of empty or negative stacks.
VA(0x004455f0, 0x3b5)
void game::CheckHeroConsistency(void) {
    town* town;
    int j;
    int i;
    int y;
    int x;
    mapCell* cell;
    hero* theHero;

    for (i = 0; i < m_playerCount; i++) {
        if (!m_playerDead[i]) {
            for (j = 0; j < 2; j++) {
                if (m_availableHeroes[m_players[i].m_availableHeroIds[j]] >= 0
                    && m_availableHeroes[m_players[i].m_availableHeroIds[j]] <= 3) {
                    m_players[i].m_availableHeroIds[j] = GetNewHeroId(0);
                    m_availableHeroes[m_players[i].m_availableHeroIds[j]] = 0x40;
                }
            }
        }
    }
    for (x = 0; x < MAP_CELL_GRID_SIZE; x++) {
        for (y = 0; y < MAP_CELL_GRID_SIZE; y++) {
            cell = gpAdvManager->GetCell(x, y);
            if (cell->m_triggerType == (MAP_TRIGGER_EVENT | MAP_OBJECT_HERO)) {
                if (static_cast<unsigned char>(cell->m_objectMetadata) >= 0 && static_cast<unsigned char>(cell->m_objectMetadata) < GAME_HERO_COUNT) {
                    theHero = GetHero(cell->m_objectMetadata);
                    if (theHero->m_owner < 0 || theHero->m_owner > 3) {
                        if (theHero->m_locationType == (MAP_TRIGGER_EVENT | MAP_OBJECT_TOWN)) {
                            town = gpGame->GetTown(theHero->m_occupiedTown);
                            town->m_occupyingHeroId = TOWN_OCCUPYING_HERO_NONE;
                        }
                        RestoreCell(theHero->m_x, theHero->m_y, theHero->m_locationType, theHero->m_occupiedTown, NULL, 1);
                    }
                } else {
                    cell->m_triggerType = MAP_OBJECT_NONE;
                }
            }
        }
    }
    for (i = 0; i < GAME_HERO_COUNT; i++) {
        for (j = 0; j < ARMY_GROUP_SLOT_COUNT; j++) {
            if (m_heroRecs[i].m_army.m_creatureTypes[j] == CREATURE_NONE || m_heroRecs[i].m_army.m_creatureCounts[j] < 0)
                m_heroRecs[i].m_army.m_creatureCounts[j] = 0;
        }
    }
    for (i = 0; i < GAME_TOWN_COUNT; i++) {
        for (j = 0; j < ARMY_GROUP_SLOT_COUNT; j++) {
            if (m_castleRecs[i].m_army.m_creatureTypes[j] == CREATURE_NONE || m_castleRecs[i].m_army.m_creatureCounts[j] < 0)
                m_castleRecs[i].m_army.m_creatureCounts[j] = 0;
        }
    }
}

// donor PoL RVA 0x00083219; preferred Buka symbol ?TransmitSaveGame@game@@QAEHHHH@Z
// donor Buka TU SOURCE/GAME; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.660125;margin=0.426397;shape=0.321;size=0.898;calls=0.886;strings=%s%s|.\DATA\|PostWait;alternate=pol20:int game::TransmitSaveGame(int, int, int)@0x00083219

// Saves REMOTE.GAM, optionally LZH-encodes it, then sends it in 200-byte
// segments, 100 segments per acknowledged block.
VA(0x004459a5, 0x6e9)
int game::TransmitSaveGame(int remotePlayer, int playerExited) {
    int okay;
    char pathname[456];
    char* outData;
    int block;
    int prevReady;
    int numBlocks;
    int junk3;
    int oldTrack;
    char* sendPacket;
    int blockSize;
    int sendPacketIndex;
    int segCount;
    int fileHandle;
    int junk2;
    char* incoming;
    char acked[500];
    int junk1;
    int status;
    int unk;
    int fileSize;
    int len;
    char* fileData;
    char finished;

    gpAdvManager->TrimLoopingSounds(8);
    okay = 0;
    status = 0;
    oldTrack = -1;
    prevReady = gpSoundManager->m_musicReady;
    gpSoundManager->m_musicReady = 1;
    oldTrack = gpSoundManager->m_currentTrack;
    gpSoundManager->SwitchAmbientMusic(MUSIC_TRACK_NONE);
    gpSoundManager->m_musicReady = prevReady;

    LogStr("Transmit Game Start");
    if (gpAdvManager->m_active == 1)
        BVResMsg("Sending Data", -1, 0);
    while (!gbHeartbeatSeen) {
        PollSound();
        Process1WindowsMessage();
    }
    AiPrint("Transmit Start");
    memset(acked, 0, sizeof(acked));
    SaveGame("REMOTE.GAM", 0);
    extern char gcDataPath[];
    sprintf(pathname, "%s%s", gcDataPath, "REMOTE.GAM");
    fileSize = FileSize(pathname);
    sendPacket = static_cast<char*>(malloc(0x100));
    if (!iMPBaseType || (iMPBaseType == MULTIPLAYER_BASE_NETWORK && gbRemoteReady))
        outData = static_cast<char*>(malloc(fileSize));
    fileData = static_cast<char*>(malloc(fileSize));
    fileHandle = open(pathname, O_BINARY);
    if (fileHandle == -1)
        FileError(pathname);
    if (fileHandle == -1) {
        goto cleanup;
    }
    {
        read(fileHandle, fileData, fileSize);
        close(fileHandle);
        if (!iMPBaseType || (iMPBaseType == MULTIPLAYER_BASE_NETWORK && gbRemoteReady))
            fileSize = EncodeData(outData, fileData, fileSize);
        else
            outData = fileData;

        ((int*)sendPacket)[0] = fileSize;
        ((int*)sendPacket)[1] = playerExited;
        status = TransmitAndWait(sendPacket, remotePlayer, 8, 1, 2, &incoming);
        if (!status)
            ShutDown(NULL);

        segCount = (fileSize - 1) / 200 + 1;
        numBlocks = (segCount - 1) / 100 + 1;
        for (block = 0; block < numBlocks; block++) {
            LogInt("Start Seg #", block);
            if (block + 1 == numBlocks)
                blockSize = segCount - block * 100;
            else
                blockSize = 100;
            finished = 0;
            while (!finished) {
                for (sendPacketIndex = block * 100; sendPacketIndex < block * 100 + blockSize; sendPacketIndex++) {
                    PollSound();
                    CheckDoMain(0, 1);
                    if (!acked[sendPacketIndex]) {
                        if (sendPacketIndex + 1 == segCount)
                            len = fileSize - sendPacketIndex * 200;
                        else
                            len = 200;
                        *(short*)sendPacket = static_cast<short>(sendPacketIndex);
                        memcpy(sendPacket + 2, outData + sendPacketIndex * 200, len);
                        status = TransmitRemoteData(sendPacket, remotePlayer, len + 2, 3, 0, 1, -1, 1);
                        if (!status)
                            ShutDown(NULL);
                    }
                }
                LogStr("PreWait");
                *(short*)sendPacket = static_cast<short>(block * 100);
                status = TransmitAndWait(sendPacket, remotePlayer, 2, 4, 5, &incoming);
                LogStr("PostWait");
                if (!status)
                    ShutDown(NULL);
                for (sendPacketIndex = 0; sendPacketIndex < blockSize; sendPacketIndex++) {
                    if (reinterpret_cast<RemoteMessage*>(incoming)->payload.data[sendPacketIndex] > 0) // API-forced: char* record.
                        acked[block * 100 + sendPacketIndex] = 1;
                }
                finished = 1;
                for (sendPacketIndex = block * 100; sendPacketIndex < block * 100 + blockSize; sendPacketIndex++) {
                    if (!acked[sendPacketIndex])
                        finished = 0;
                }
            }
        }
        status = TransmitRemoteData(NULL, remotePlayer, 0, 6, 1, 1, -1, 1);
        if (!status)
            ShutDown(NULL);
        okay = 1;
    }

cleanup:
    free(sendPacket);
    if (!iMPBaseType || (iMPBaseType == MULTIPLAYER_BASE_NETWORK && gbRemoteReady))
        free(outData);
    free(fileData);
    AiPrint("Transmit End");
    if (gpAdvManager->m_active == 1) {
        giBottomViewOverride = 0;
        gpAdvManager->UpdBottomView(1, 1, 1);
    }
    if (oldTrack != -1) {
        prevReady = gpSoundManager->m_musicReady;
        gpSoundManager->m_musicReady = 1;
        gpSoundManager->SwitchAmbientMusic(oldTrack);
        gpSoundManager->m_musicReady = prevReady;
    }
    return okay;
}

// donor PoL RVA 0x00083937; preferred Buka symbol ?ReceiveSaveGame@game@@QAEHHHHH@Z
// donor Buka TU SOURCE/GAME; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.655741;margin=0.222523;shape=0.420;size=0.807;calls=0.714;strings=%s%s|.\DATA\|Receive End;alternate=pol20:int game::ReceiveSaveGame(int, int, int, int)@0x00083937
// Collects the remote save in 200-byte segments, acknowledging each block
// of 100, then decodes it and writes REMOTE.GAM.
VA(0x0044608e, 0x579)
int game::ReceiveSaveGame(int dataSize, int remotePlayer) {
    int unused1;
    int okay;
    int prevReady;
    char pathname[452];
    char* inData;
    int k;
    int oldTrack;
    int fileHandle;
    char done;
    char* sendPacket;
    RemoteMessage* receivedPacket;
    int result;
    long lastPacketTime;
    char gotIt[500];
    int packetStart;
    char* decodedData;

    gpAdvManager->TrimLoopingSounds(8);
    fileHandle = 0;
    done = 0;
    unused1 = 0;
    okay = 0;
    oldTrack = -1;
    if (gpAdvManager->m_active == 1)
        BVResMsg("Receiving Data", -1, 0);
    prevReady = gpSoundManager->m_musicReady;
    oldTrack = gpSoundManager->m_currentTrack;
    gpSoundManager->m_musicReady = 1;
    gpSoundManager->SwitchAmbientMusic(MUSIC_TRACK_NONE);
    gpSoundManager->m_musicReady = prevReady;
    while (!gbHeartbeatSeen) {
        PollSound();
        Process1WindowsMessage();
    }
    result = TransmitRemoteData(NULL, remotePlayer, 0, 2, 1, 1, -1, 1);
    if (!result)
        ShutDown(NULL);
    memset(gotIt, 0, sizeof(gotIt));
    if (!iMPBaseType || (iMPBaseType == MULTIPLAYER_BASE_NETWORK && gbRemoteReady))
        decodedData = static_cast<char*>(malloc(0x130b0));
    sendPacket = static_cast<char*>(malloc(0x100));
    inData = static_cast<char*>(malloc(dataSize + 500));
    lastPacketTime = KBTickCount();
    while (!done) {
        PollSound();
        CheckDoMain(0, 1);
        if (lastPacketTime + 20000 < KBTickCount()) {
            NormalDialog("Error receiving data.  Keep trying??", NORMAL_DIALOG_TYPE_YES_NO, -1, -1, NORMAL_DIALOG_NO_RESOURCE, 0, NORMAL_DIALOG_NO_RESOURCE, 0, NORMAL_DIALOG_NO_OR_TEXT);
            if (gpWindowManager->m_dialogResult == NORMAL_DIALOG_CONFIRM)
                lastPacketTime = KBTickCount();
            else
                ShutDown(NULL);
        }
        receivedPacket = reinterpret_cast<RemoteMessage*>(GetRemoteData(1)); // API-forced: char* record.
        if (receivedPacket && (receivedPacket->type == 2 || receivedPacket->type == 3)) {
            lastPacketTime = KBTickCount();
            switch (receivedPacket->command) {
                case 3:
                    packetStart = receivedPacket->payload.segment.index;
                    gotIt[packetStart] = 1;
                    memcpy(inData + packetStart * 200, receivedPacket->payload.segment.data,
                           receivedPacket->payloadSize - 2);
                    break;
                case 4:
                    packetStart = receivedPacket->payload.segment.index;
                    for (k = packetStart; k < packetStart + 100; k++)
                        *(sendPacket + k - packetStart) = gotIt[k];
                    result = TransmitRemoteData(sendPacket, remotePlayer, 200, 5, 1, 1, -1, 1);
                    if (!result)
                        ShutDown(NULL);
                    break;
                case 6:
                    done = 1;
                    break;
            }
        }
    }
    if (!iMPBaseType || (iMPBaseType == MULTIPLAYER_BASE_NETWORK && gbRemoteReady))
        dataSize = DecodeData(decodedData, inData);
    else
        decodedData = inData;
    extern char gcDataPath[];
    sprintf(pathname, "%s%s", gcDataPath, "REMOTE.GAM");
    fileHandle = open(pathname, O_WRONLY | O_CREAT | O_TRUNC | O_BINARY, S_IWRITE);
    if (fileHandle == -1)
        FileError(pathname);
    write(fileHandle, decodedData, dataSize);
    close(fileHandle);
    okay = 1;
    free(sendPacket);
    free(inData);
    if (!iMPBaseType || (iMPBaseType == MULTIPLAYER_BASE_NETWORK && gbRemoteReady))
        free(decodedData);
    AiPrint("Receive End");
    if (gpAdvManager->m_active == 1) {
        giBottomViewOverride = 0;
        gpAdvManager->UpdBottomView(1, 1, 1);
    }
    if (oldTrack != -1) {
        prevReady = gpSoundManager->m_musicReady;
        gpSoundManager->m_musicReady = 1;
        gpSoundManager->SwitchAmbientMusic(oldTrack);
        gpSoundManager->m_musicReady = prevReady;
    }
    return okay;
}

// donor PoL RVA 0x00083fc4; preferred Buka symbol ?DoNewTurn@game@@QAEXXZ
// donor Buka TU SOURCE/GAME; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.487826;margin=0.297906;shape=0.344;size=0.836;calls=0.867;alternate=pol20:void game::DoNewTurn(void)@0x00083fc4
VA(0x00446607, 0x42b)
void game::DoNewTurn(void) {
    int track;
    char monsterName[52];

    if (!gbThisNetHumanPlayer[giCurPlayer]) {
        CheckEndGame(0);
        return;
    }
    giBottomViewOverrideEndTime = KBTickCount() + 3000;
    giBottomViewOverride = 1;
    gpAdvManager->UpdBottomView(1, 1, 1);
    gpAdvManager->SetInitialMapOrigin();
    gpAdvManager->CompleteDraw(0);
    gpAdvManager->UpdateScreen(0, 0);
    CheckEndGame(0);
    if (gpCurPlayer->m_daysLeft >= 0) {
        if (gpCurPlayer->m_daysLeft == 1) {
            sprintf(gText, gNewTurnText[1], gColorNames[gpGame->m_players[giCurPlayer].Color()]);
            gText[0] -= 32;
        } else {
            sprintf(
                gText,
                gNewTurnText[0],
                gColorNames[gpGame->m_players[giCurPlayer].Color()],
                gpCurPlayer->m_daysLeft
            );
            gText[0] -= 32;
        }
        NormalDialog(gText, NORMAL_DIALOG_TYPE_OK, 0x61, -1, NORMAL_DIALOG_CREST, gpGame->m_players[giCurPlayer].Color(), NORMAL_DIALOG_NO_RESOURCE, 0, NORMAL_DIALOG_NO_OR_TEXT);
    }
    if (gpCurPlayer->m_heroCount > 0)
        gpAdvManager->SetHeroContext(gpCurPlayer->NextHero(0), 0);
    else if (gpCurPlayer->m_townCount > 0)
        gpAdvManager->SetTownContext(gpCurPlayer->m_townIds[0]);
    gpAdvManager->CheckDimNextHeroBut();
    gpSoundManager->SwitchAmbientMusic(gpAdvManager->m_currentTerrain);
    if (m_day == 1) {
        if ((m_month != 1 || m_week != 1 || m_day != 1) && giWeekType != -1) {
            track = -1;
            if (m_week == 1) {
                track = 0x33;
                if (giMonthType == 0) {
                    sprintf(gText, gNewTurnText[2], gMonthNames[giMonthSpecial]);
                } else if (giMonthType == 1) {
                    strcpy(monsterName, gArmyNames[giMonthSpecial]);
                    monsterName[0] -= 32;
                    sprintf(gText, gNewTurnText[3], gArmyNames[giMonthSpecial], monsterName);
                } else {
                    sprintf(gText, gNewTurnText[4]);
                }
            } else {
                track = 0x32;
                if (giWeekType == 0) {
                    sprintf(gText, gNewTurnText[5], gWeekNames[giWeekSpecial]);
                } else {
                    strcpy(monsterName, gArmyNames[giWeekSpecial]);
                    monsterName[0] -= 32;
                    sprintf(gText, gNewTurnText[6], gArmyNames[giWeekSpecial], monsterName);
                }
            }
            gpSoundManager->SwitchAmbientMusic(track);
            gpMouseManager->SetPointer(ADVENTURE_POINTER_DEFAULT);
            NormalDialog(gText, NORMAL_DIALOG_TYPE_OK, 0x61, -1, NORMAL_DIALOG_NO_RESOURCE, 0, NORMAL_DIALOG_NO_RESOURCE, 0, NORMAL_DIALOG_NO_OR_TEXT);
            gpSoundManager->SwitchAmbientMusic(gpAdvManager->m_currentTerrain);
        }
    }
}

// Buka 2.1 game::GetBoatsBuilt.
VA(0x00446a32, 0x58)
int game::GetBoatsBuilt(void) {
    int count = 0;
    int i;
    for (i = 0; i < GAME_BOAT_COUNT; ++i) {
        if (m_boatSlots[i] != -1)
            ++count;
    }
    return count;
}

DATA(0x00490abc)
signed char gbShowMapInfo = 0;

// donor PoL RVA 0x000b6f40; preferred Buka symbol ?GetMap@game@@QAEXXZ
// donor Buka TU SOURCE/Newgame; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.547944;margin=0.077231;shape=0.356;size=0.530;calls=0.607;strings=.\MAPS\;alternate=pol20:void game::GetMap(void)@0x000b6f40
// Buka 2.1 GetMap with HoMM1's player-count file masks and the reqextra.bin
// map-info window; a cancelled pick restores the previous map's texts.
VA(0x00446a8a, 0x36f)
void game::GetMap(void) {
    char saveFullName[20];
    char oldDescription[124];
    char oldMapName[16];
    char mask[16];
    short result;
    fileRequester* request;

    strcpy(oldMapName, gMapName);
    strcpy(saveFullName, gFullMapName);
    strcpy(oldDescription, gMapDescription);
    gbShowMapInfo = 1;
    strcpy(gcCurMapName, "");
    gpReqExtraWindow = new heroWindow(310, 332, "reqextra.bin");
    if (!gpReqExtraWindow)
        MemError();
    if (giNumHumanPlayers == 1)
        sprintf(mask, "????1???.MAP");
    else if (giNumHumanPlayers == 2)
        sprintf(mask, "?????2??.MAP");
    else if (giNumHumanPlayers == 3)
        sprintf(mask, "??????3?.MAP");
    else if (giNumHumanPlayers == 4)
        sprintf(mask, "???????4.MAP");
    request = new fileRequester(310, 14, 0, mask, ".\\MAPS\\", ".MAP");
    if (!request)
        MemError();
    request->ShowMapInfo();
    result = gpExec->DoDialog(request);
    gpWindowManager->RemoveWindow(gpReqExtraWindow);
    if (result == DIALOG_BUTTON_2) {
        strcpy(gMapName, gLastFilename);
        delete request;
    } else {
        strcpy(gMapName, oldMapName);
        strcpy(gFullMapName, saveFullName);
        strcpy(gMapDescription, oldDescription);
        delete request;
    }
    delete gpReqExtraWindow;
    gbShowMapInfo = 0;
}

// Buka 2.1 game::GetNumThievesGuilds.
VA(0x00446df9, 0x98)
int game::GetNumThievesGuilds(int color) {
    int numGuilds = 0;
    int i;
    for (i = 0; i < m_players[color].m_townCount; ++i) {
        if (gpGame->m_castleRecs[m_players[color].m_townIds[i]].m_buildings & 2)
            ++numGuilds;
    }
    return numGuilds;
}

// Buka 2.1 game::CalcDifficultyRating for HoMM1: difficulty, opponents
// (human seats by handicap, computers by level), King of the Hill, map
// size and map difficulty.
VA(0x00446e91, 0x30f)
int game::CalcDifficultyRating(void) {
    int i;
    int total;

    total = 0;
    if (m_difficulty == 0) {
    } else if (m_difficulty == 1) {
        total += 10;
    } else if (m_difficulty == 2) {
        total += 20;
    } else if (m_difficulty == 3) {
        total += 30;
    }
    for (i = 1; i < 4; i++) {
        if (i < giNumHumanPlayers)
            total += (m_players[i].m_difficulty - 1) * 10;
        else if (m_players[i].m_difficulty == 0)
            total -= 10;
        else if (m_players[i].m_difficulty == 1)
            total += 5;
        else if (m_players[i].m_difficulty == 2)
            total += 10;
        else if (m_players[i].m_difficulty == 3)
            total += 15;
        else if (m_players[i].m_difficulty == 4)
            total += 20;
    }
    gpGame->m_playerCount = 0;
    for (i = 0; i < 4; i++) {
        if (gpGame->m_players[i].m_difficulty > 0)
            gpGame->m_playerCount++;
    }
    if (gbKingOfTheHill) {
        if (m_playerCount - giNumHumanPlayers == 0) {
        } else if (m_playerCount - giNumHumanPlayers == 1) {
        } else if (m_playerCount - giNumHumanPlayers == 2) {
            total += 5;
        } else if (m_playerCount - giNumHumanPlayers == 3) {
            total += 10;
        }
    }
    if (giMapSize == 0) {
    } else if (giMapSize == 1) {
        total += 10;
    } else if (giMapSize == 2) {
        total += 20;
    }
    if (giMapDifficulty == 0)
        total += 20;
    else if (giMapDifficulty == 1)
        total += 30;
    else if (giMapDifficulty == 2)
        total += 40;
    else if (giMapDifficulty == 3)
        total += 50;
    return total;
}

// ShowCongrats' base score: 200 less a day per day for two months, then a
// half, a quarter and an eighth per day, never below 20.
VA(0x004471a0, 0x138)
int GetBaseScore(int days) {
    int score;

    score = 200;
    if (days <= 60) {
        score -= days;
        goto done;
    } else {
        score -= 60;
    }
    if (days <= 120) {
        score -= (days - 60) * 0.5;
        goto done;
    } else {
        score -= 30.0;
    }
    if (days <= 360) {
        score -= (days - 120) * 0.25;
        goto done;
    } else {
        score -= 60.0;
    }
    score -= (days - 360) * 0.125;
done:
    if (score < 20)
        score = 20;
    return score;
}

// Retail loads sceninfo.bin and is called on gpGame with no arguments:
// Buka's game::ShowScenInfo, not the adventure-map ViewWorld (0x431507).

VA(0x004472d8, 0x44e)
void game::ShowScenInfo(void) {
    short i;
    const char sizeId = 100;
    const char mapLevelId = 101;
    const char mapDescId = 102;
    const char crestId = 103;
    const char nameId = 104;
    const char levelId = 105;
    const char playersId = 106;
    const char kingOfHillId = 107;
    const char ratingId = 108;
    char line1[20];
    int difficulty;
    heroWindow* scenWindow;
    tag_message message;
    short idx;
    // Retail reserves one unused slot between the seat counters.
    int pad;

    gpMouseManager->SetPointer(ADVENTURE_POINTER_DEFAULT);
    scenWindow = new heroWindow(159, 14, "sceninfo.bin");
    if (!scenWindow)
        MemError();
    message.type = MESSAGE_WIDGET;
    message.command = WIDGET_COMMAND_SET_TEXT;
    message.id = nameId;
    message.text = m_mapName;
    scenWindow->BroadcastMessage(message);
    difficulty = m_difficulty;
    if (giCurPlayer > 0)
        difficulty = gpCurPlayer->m_difficulty - 1;
    message.id = levelId;
    message.text = gDifficultyNames[difficulty];
    scenWindow->BroadcastMessage(message);
    message.id = playersId;
    message.text = gText;
    sprintf(gText, "");
    for (i = 1; i < 4; i++) {
        if (giCurPlayer == 0) {
            sprintf(line1, "%s\n",
                    gbHumanPlayer[i] ? gHandicapNames[m_players[i].m_difficulty] : gPlayerTypeNames[m_players[i].m_difficulty]);
        } else if (i == 1) {
            sprintf(line1, "%s\n", gHandicapNames[m_difficulty + 1]);
        } else {
            if (i - 1 >= giCurPlayer)
                idx = i;
            else
                idx = i - 1;
            sprintf(line1, "%s\n",
                    gbHumanPlayer[idx] ? gHandicapNames[m_players[idx].m_difficulty] : gPlayerTypeNames[m_players[idx].m_difficulty]);
        }
        strcat(gText, line1);
    }
    scenWindow->BroadcastMessage(message);
    message.id = kingOfHillId;
    message.text = gText;
    sprintf(gText, gbKingOfTheHill ? "Yes" : "No");
    scenWindow->BroadcastMessage(message);
    message.id = ratingId;
    sprintf(gText, "%d%%", gpGame->m_difficultyRating);
    message.text = gText;
    scenWindow->BroadcastMessage(message);
    message.id = sizeId;
    message.text = gMapSizeNames[m_mapSize];
    scenWindow->BroadcastMessage(message);
    message.id = mapLevelId;
    message.text = gMapDifficultyNames[m_mapDifficulty];
    scenWindow->BroadcastMessage(message);
    message.id = mapDescId;
    message.text = m_mapDescription;
    scenWindow->BroadcastMessage(message);
    message.command = WIDGET_COMMAND_SET_FRAME;
    if (m_players[giCurPlayer].m_color != -1) {
        message.id = crestId;
        message.value = m_players[giCurPlayer].m_color * 2 + 11;
        scenWindow->BroadcastMessage(message);
    }
    gpWindowManager->DoDialog(scenWindow, EventWindowHandler, 0);
}

// HoMM1 keeps the human's crest and gives each opponent a free one: the
// campaign scenario's crest when it names one, else a random draw.
VA(0x00447726, 0x14f)
void game::RandomizePlayerCrests(void) {
    int i;
    signed char taken[4];
    taken[0] = 0;
    taken[1] = 0;
    taken[2] = 0;
    taken[3] = 0;
    taken[m_players[0].m_color] = 1;
    for (i = 1; i < m_playerCount; i++) {
        do {
            if (m_campaignType > 0 && gCampaignScenarios[m_campaignScenario].playerCrests[i] < 4
                && gCampaignScenarios[m_campaignScenario].playerCrests[i] >= 0)
                m_players[i].m_color = gCampaignScenarios[m_campaignScenario].playerCrests[i];
            else
                m_players[i].m_color = Random(0, 3);
        } while (taken[m_players[i].m_color] == 1);
        taken[m_players[i].m_color] = 1;
    }
}

// donor PoL RVA 0x0008480a; preferred Buka symbol ?RestoreCell@game@@QAEXHHHHPAVmapCell@@H@Z
// donor Buka TU SOURCE/GAME; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.572404;margin=0.482531;shape=0.423;size=0.977;calls=1.000;alternate=pol20:void game::RestoreCell(int, int, int, int, class mapCell *, int)@0x0008480a
VA(0x00447875, 0xa1)
void game::RestoreCell(int x, int y, int obj, int barrier, mapCell* passedCell, int) {
    mapCell* cell;
    if (passedCell)
        cell = passedCell;
    else
        cell = gpAdvManager->GetCell(x, y);
    if (y > 0 && obj == 0xa8 && gpAdvManager->GetCell(x, y - 1)->m_triggerType != MAP_OBJECT_TOWN) {
        cell->m_triggerType = MAP_OBJECT_NONE;
        cell->m_objectMetadata = 0;
        return;
    }
    cell->m_triggerType = obj;
    cell->m_objectMetadata = barrier;
}

// GAME owns retail .data 0x00490600-0x00490b4b and .bss 0x004c50d0-0x004c512f.
// Retail emits gbNewGameSettingsSaved, giMonType and iLastSeed among the
// literals of their users; gbShowMapInfo is defined above GetMap.
DATA(0x00490600)
int gbGameOver = 0;
DATA(0x004906c0)
signed char gbNewGameSettingsSaved = 0;
DATA(0x004909c0)
signed char giMonType[12] = {0, 6, 13, 14, 9, 15, 7, 8, 18, 19, 16, 20};
DATA(0x004909cc)
unsigned long iLastSeed = 135621123;
DATA(0x004c50d0)
signed char gSaveCurPlayer;
DATA(0x004c50dc)
signed char gcSavedCrest;
DATA(0x004c50e0)
signed char gcSavedDifficulty;
DATA(0x004c50e4)
int giEndSequence;
DATA(0x004c50e8)
signed char gbDismissArmy;
DATA(0x004c50f4)
heroWindow* gpReqExtraWindow;
DATA(0x004c50f8)
signed char gcSavedPlayerTypes[4];
DATA(0x004c5108)
short giMineTypeCount[RESOURCE_COUNT];
DATA(0x004c5118)
char gcCurMapName[16];
DATA(0x004c5128)
signed char gbSavedKingOfTheHill;
DATA(0x004c512c)
signed char gRandomTownTypes[4];

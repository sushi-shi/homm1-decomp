// Located from HoMM2 Buka 2.1; PoL 2.0 supplies the VC4 declaration.

#include <match.h>

#include <BASE/BITS.h>
#include <BASE/executive.h>
#include <BASE/font.h>
#include <BASE/heroWindow.h>
#include <BASE/heroWindowManager.h>
#include <BASE/icon.h>
#include <BASE/iconWidget.h>
#include <BASE/inputManager.h>
#include <BASE/LZHUF.h>
#include <BASE/message.h>
#include <BASE/Misc.h>
#include <BASE/miscwin.h>
#include <BASE/mouseManager.h>
#include <BASE/resource.h>
#include <BASE/resourceManager.h>
#include <BASE/audio.h>
#include <BASE/TILE.h>
#include <BASE/tileset.h>
#include <BASE/widget.h>
#include <SOURCE/advManager.h>
#include <SOURCE/army.h>
#include <SOURCE/armyGroup.h>
#include <SOURCE/artifactTypes.h>
#include <SOURCE/campaignTypes.h>
#include <SOURCE/combatManager.h>
#include <SOURCE/combatTypes.h>
#include <SOURCE/EVENTS.h>
#include <SOURCE/fileRequester.h>
#include <SOURCE/FINDPATH.h>
#include <SOURCE/game.h>
#include <SOURCE/hero.h>
#include <SOURCE/KB.h>
#include <SOURCE/kbwin.h>
#include <SOURCE/mapCell.h>
#include <SOURCE/mapObjectTypes.h>
#include <SOURCE/philAI.h>
#include <SOURCE/playerData.h>
#include <SOURCE/REMOTE.h>
#include <SOURCE/town.h>

#include <fcntl.h>
#include <io.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

// donor PoL RVA 0x000708b0; preferred Buka symbol ?Write@playerData@@QAEXH@Z
// donor Buka TU SOURCE/GAME; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.582104;margin=0.473963;shape=0.500;size=0.883;calls=0.885;alternate=pol20:void playerData::Write(int)@0x000708b0
VA(0x0042b400, 0x1f2)
void playerData::Write(i32 file) {
    char unused[52];

    write(file, m_unknown00, sizeof(m_unknown00));
    write(file, &m_color, 1);
    write(file, &m_difficulty, 1);
    write(file, &m_heroCount, 1);
    write(file, &m_currentHero, 1);
    write(file, &m_heroLocatorPage, 1);
    write(file, m_heroIds, sizeof(m_heroIds));
    write(file, m_availableHeroIds, sizeof(m_availableHeroIds));
    memset(unused, 0, PLAYER_SAVE_PAD_SIZE);
    write(file, unused, PLAYER_SAVE_PAD_SIZE);
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
VA(0x0042b5f2, 0x1e1)
void playerData::Read(i32 file) {
    char unused[52];

    read(file, m_unknown00, sizeof(m_unknown00));
    read(file, &m_color, 1);
    read(file, &m_difficulty, 1);
    read(file, &m_heroCount, 1);
    read(file, &m_currentHero, 1);
    read(file, &m_heroLocatorPage, 1);
    read(file, m_heroIds, sizeof(m_heroIds));
    read(file, m_availableHeroIds, sizeof(m_availableHeroIds));
    read(file, unused, PLAYER_SAVE_PAD_SIZE);
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
VA(0x0042b7d3, 0x100)
i8 playerData::NextHero(i32) {
    i32 curHero = -1;
    i32 i;

    if (gpCurPlayer->m_currentHero != GAME_HERO_NONE) {
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
    return GAME_HERO_NONE;
}

// Buka 2.1 playerData::HasMobileHero.
VA(0x0042b8d3, 0x56)
i8 playerData::HasMobileHero(void) {
    for (i16 i = 0; i < m_heroCount; ++i) {
        if (gpGame->IsMobile(m_heroIds[i]))
            return 1;
    }
    return 0;
}

// HoMM1 counts this player's visited-obelisk bits.
VA(0x0042b929, 0x56)
i8 playerData::CountVisitedObelisks(void) {
    i8 count = 0;
    for (i16 i = 0; i < PLAYER_PUZZLE_PIECE_COUNT; ++i) {
        if (BitTest(m_obelisksVisited, i))
            ++count;
    }
    return count;
}

// Buka 2.1 playerData::BuildingsOwned; slot 0 is the mage guild.
VA(0x0042b97f, 0xb6)
i32 playerData::BuildingsOwned(i32 townType, i32 buildingIndex, i32 buildState) {
    i32 count = 0;
    i32 i;
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
VA(0x0042ba35, 0x84)
i32 playerData::NumOfGivenArtifact(i32 artifact) {
    i32 count = 0;
    i32 i;
    i32 j;
    for (i = 0; i < m_heroCount; i++) {
        for (j = 0; j < HERO_ARTIFACT_SLOT_COUNT; j++) {
            if (gpGame->m_heroRecs[m_heroIds[i]].m_artifacts[j] == artifact)
                count++;
        }
    }
    return count;
}

// Buka 2.1 ComputeUALoc: HoMM1 needs eleven obelisks (four percent each over
// ten) and skips player 0's hint.
VA(0x0042bab9, 0x321)
void ComputeUALoc(i32 player) {
    i32 tries;
    i32 x;
    i32 y;
    i32 heading;
    i32 numObelisks;

    if (player > 0) {
        numObelisks = gpGame->m_players[player].CountVisitedObelisks();
        if (numObelisks < ULTIMATE_HINT_OBELISK_MIN
            || gpGame->m_ultimateArtifactId == ARTIFACT_NONE) {
            gpGame->m_players[player].m_ultimateArtifactHintChance = 0;
            gpGame->m_players[player].m_ultimateArtifactHintX = PLAYER_ULTIMATE_HINT_NONE;
            gpGame->m_players[player].m_ultimateArtifactHintY = PLAYER_ULTIMATE_HINT_NONE;
        } else {
            gpGame->m_players[player].m_ultimateArtifactHintChance =
                (numObelisks - ULTIMATE_HINT_OBELISK_MIN) * ULTIMATE_HINT_PERCENT_PER_OBELISK;
            if (gpGame->m_players[player].m_ultimateArtifactHintChance >= Random(1, 100)) {
                gpGame->m_players[player].m_ultimateArtifactHintX = gpGame->m_ultimateArtifactX;
                gpGame->m_players[player].m_ultimateArtifactHintY = gpGame->m_ultimateArtifactY;
            } else {
                x = PLAYER_ULTIMATE_HINT_NONE;
                y = PLAYER_ULTIMATE_HINT_NONE;
                heading = 0;
                tries = 0;
                while (
                    !(x >= 0 && x < MAP_CELL_GRID_SIZE && y >= 0 && y < MAP_CELL_GRID_SIZE
                      && gpGame->m_map[x][y].m_triggerType == MAP_OBJECT_NONE
                      && gpGame->m_map[x][y].m_objectIndex == MAP_CELL_NO_FRAME
                      && gpGame->m_map[x][y].m_overlayIndex == MAP_CELL_NO_FRAME
                      && gpGame->m_map[x][y].m_tileIndex >= MAP_CELL_TILES_PER_TERRAIN)
                ) {
                    tries++;
                    heading = 0;
                    while (heading == 0)
                        heading =
                            ULTIMATE_HINT_SCATTER - Random(0, 2) - Random(0, 2) - Random(0, 2);
                    x = gpGame->m_ultimateArtifactX + heading;
                    heading = 0;
                    while (heading == 0)
                        heading =
                            ULTIMATE_HINT_SCATTER - Random(0, 2) - Random(0, 2) - Random(0, 2);
                    y = gpGame->m_ultimateArtifactY + heading;
                    if (tries >= ULTIMATE_HINT_PLACE_TRIES) {
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
VA(0x0042bdda, 0x17c)
void game::VisitObelisk(i8 player) {
    i16 attempts;
    i8 visited;
    i8 fallback;
    i8 piece;
    i32 pieces;
    i16 numRemoved;
    i32 removeCount;

    pieces = PLAYER_PUZZLE_PIECE_COUNT;
    removeCount = pieces / m_obeliskCount;
    if (removeCount < 1)
        removeCount = 1;
    for (numRemoved = 0; numRemoved < removeCount; numRemoved++) {
        visited = m_players[player].CountVisitedObelisks();
        for (piece = 0; piece < pieces; piece += Random(1, 5)) {
            if (!BitTest(m_players[player].m_obelisksVisited, piece))
                break;
        }
        for (attempts = 0; attempts < OBELISK_PIECE_PICK_TRIES; attempts++) {
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
VA(0x0042bf56, 0x98)
i8 game::IsMobile(i8 heroId) {
    if (heroId == GAME_HERO_NONE)
        return 0;
    hero* mobileHero = &m_heroRecs[heroId];
    i32 terrain = CELL_TERRAIN(gpAdvManager->GetCell(mobileHero->m_x, mobileHero->m_y));
    return mobileHero->m_remainingMobility >= CalcTerrainCost(
               terrain,
               mobileHero->m_direction & MAP_DIRECTION_DIAGONAL_BIT,
               mobileHero->m_remainingMobility,
               mobileHero->m_heroClass
           );
}

// Buka 2.1 game::GetWorldMapData.
VA(0x0042bfee, 0x13)
mapCell (*game::GetWorldMapData(void)) [MAP_CELL_GRID_SIZE] { return m_map; }

// Buka 2.1 game::CreateBoat without the network map-change notice.
VA(0x0042c001, 0xce)
i8 game::CreateBoat(i8 x, i8 y) {
    i8 boatIdx = Scan(m_boatSlots, 0, GAME_BOAT_COUNT);
    if (boatIdx != GAME_TABLE_FREE) {
        m_boatSlots[boatIdx] = boatIdx;
        boatRecord* boat = &m_boats[boatIdx];
        boat->id = boatIdx;
        boat->x = x;
        boat->y = y;
        boat->direction = MAP_DIRECTION_EAST;
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
VA(0x0042c0cf, 0x4b)
i8 game::Scan(i8* array, i8 start, i8 length) {
    i8 i;
    for (i = start; i < start + length; ++i) {
        if (array[i] == GAME_TABLE_FREE)
            return i;
    }
    return GAME_TABLE_FREE;
}

// Buka 2.1 game::RandomScan; HoMM1 always looks for a free (-1) entry.
VA(0x0042c11a, 0x64)
i8 game::RandomScan(i8* array, i8 start, i8 range, i32) {
    i8 index = GAME_TABLE_FREE;
    i32 i;
    for (i = 0; i < GAME_RANDOM_SCAN_TRIES; ++i) {
        index = start + Random(0, range - 1);
        if (array[index] == GAME_TABLE_FREE)
            return index;
    }
    return GAME_TABLE_FREE;
}

// HoMM1 nine heroes per class; a 0x40 entry is the fallback pick.
VA(0x0042c17e, 0xe5)
i8 game::GetNewHeroId(i8 heroClass) {
    i8 freeSlot = GAME_TABLE_FREE;
    i8 id = GAME_HERO_NONE;
    i16 first = heroClass * HERO_PER_CLASS_COUNT;
    i32 i;
    freeSlot = Scan(m_availableHeroes, first, HERO_PER_CLASS_COUNT);
    if (freeSlot != GAME_TABLE_FREE) {
        id = RandomScan(m_availableHeroes, first, HERO_PER_CLASS_COUNT, HERO_PER_CLASS_COUNT);
    } else {
        freeSlot = Scan(m_availableHeroes, 0, GAME_HERO_COUNT);
        if (freeSlot != GAME_TABLE_FREE) {
            id = RandomScan(m_availableHeroes, 0, GAME_HERO_COUNT, GAME_HERO_COUNT);
        } else {
            for (i = 0; i < GAME_HERO_COUNT; ++i) {
                if (m_availableHeroes[i] == HERO_AVAILABILITY_RETREATED)
                    id = i;
            }
        }
    }
    if (id != GAME_HERO_NONE)
        return id;
    else
        return 0;
}

// Buka 2.1 game::GetTownId.
VA(0x0042c263, 0x69)
i8 game::GetTownId(i8 x, i8 y) {
    for (i16 i = 0; i < GAME_TOWN_COUNT; ++i) {
        if (m_castleRecs[i].m_x == x && m_castleRecs[i].m_y == y)
            return i;
    }
    return GAME_TOWN_NONE;
}

// Buka 2.1 game::GetMineId.
VA(0x0042c2cc, 0x69)
i8 game::GetMineId(i8 x, i8 y) {
    for (i16 i = 0; i < GAME_MINE_COUNT; ++i) {
        if (m_mines[i].x == x && m_mines[i].y == y)
            return i;
    }
    return GAME_MINE_NONE;
}

// donor PoL RVA 0x00071d89; preferred Buka symbol ?GenerateStandardFileName@@YIXPAD0@Z
// donor Buka TU SOURCE/GAME; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.427111;margin=0.052277;shape=0.267;size=0.694;calls=1.000;alternate=pol20:void GenerateStandardFileName(char *, char *)@0x00071d89
VA(0x0042c335, 0x1a8)
void GenerateStandardFileName(char* source, char* destination) {
    char* extension;
    i32 indexOut;
    i32 idx;
    i32 size;
    char character;

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
        if (indexOut >= SAVE_FILE_BASE_NAME_LENGTH)
            idx = SAVE_FILE_NAME_SCAN_STOP;
    }
    *extension = '.';
    strcpy(destination + indexOut, extension);
}

// donor PoL RVA 0x00071eb7; preferred Buka symbol ?SaveGame@game@@QAEHPADHC@Z
// donor Buka TU SOURCE/GAME; HoMM1 owner inferred from contiguous order
// evidence: graph:3;base=0.606443;margin=0.348788;shape=0.410;size=0.678;calls=0.698;strings=%s%s|%s.%s|%s.GM%d;alternate=pol20:int game::SaveGame(char *, int, signed char)@0x00071eb7
inline void game::ReadWorldMap(i32 fd) {
    read(fd, m_map, sizeof(m_map));
}

inline void game::WriteWorldMap(i32 fd) {
    write(fd, m_map, sizeof(m_map));
}

// Buka 2.1 game::SaveGame for HoMM1's single save layout: name, globals,
// campaign state, map header, players, world map, records and visibility.
VA(0x0042c4dd, 0x796)
i16 game::SaveGame(char* filename, i8 generateName) {
    i32 nHumans;
    i32 saveFlag;
    char human[GAME_PLAYER_COUNT];
    i32 iFile;
    i32 file;
    i32 junk[4];
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
        extern char gDataPath[];
        sprintf(filePath, "%s%s", gDataPath, fileName);
    } else {
        extern char gGamePath[];
        sprintf(filePath, "%s%s", gGamePath, fileName);
        if (strnicmp(fileName, "AUTOSAVE", SAVE_FILE_BASE_NAME_LENGTH)
            && strnicmp(fileName, "PLYREXIT", SAVE_FILE_BASE_NAME_LENGTH))
            strcpy(gpGame->m_saveName, filename);
    }
    file = open(filePath, O_WRONLY | O_CREAT | O_TRUNC | O_BINARY, S_IWRITE);
    if (file == -1)
        FileError(filePath);
    write(file, &gbIAmGreatest, 1);
    write(file, this, 2);
    write(file, &giMonthType, 1);
    write(file, &giMonthTypeExtra, 1);
    write(file, &giWeekType, 1);
    write(file, &giWeekTypeExtra, 1);
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
// Buka 2.1 game::LoadGame for HoMM1's save layout; origdata.bin restores
// the default hero names and blank visibility, and the seats are re-dealt
// to this session's human players.
VA(0x0042cc73, 0x87f)
i16 game::LoadGame(char* filename, i32 origData, i32) {
    i32 junk2;
    i32 numHumans;
    i32 i;
    i32 handle;
    char pathName[452];
    i8 humans[GAME_PLAYER_COUNT];
    i32 junk;
    char buffer[0x2c];

    numHumans = 0;
    gGameOver = 0;
    m_noMapHeroes = 1;
    extern char gDataPath[];
    extern char gGamePath[];
    if (origData || !strcmp(filename, "REMOTE.GAM"))
        sprintf(pathName, "%s%s", gDataPath, filename);
    else
        sprintf(pathName, "%s%s", gGamePath, filename);
    handle = open(pathName, O_BINARY);
    if (handle == -1)
        FileError(pathName);
    ClearMapExtra();
    read(handle, &gbIAmGreatest, 1);
    read(handle, this, 2);
    read(handle, &giMonthType, 1);
    read(handle, &giMonthTypeExtra, 1);
    read(handle, &giWeekType, 1);
    read(handle, &giWeekTypeExtra, 1);
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
            if (!gRemoteOn || i == giThisGamePos)
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
    giCurTurn =
        (m_month - 1) * CALENDAR_DAYS_PER_MONTH + (m_week - 1) * CALENDAR_DAYS_PER_WEEK + m_day;
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
        memset(m_mapSounds, MAP_SOUND_NONE, sizeof(m_mapSounds));
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
    giCurPlayerHighBit = 1 << (giCurPlayer + GAME_PLAYER_HIGH_BIT_SHIFT);
    gCurWatchPlayerHighBit = 1 << (giCurWatchPlayer + GAME_PLAYER_HIGH_BIT_SHIFT);
    bShowIt = gbThisNetHumanPlayer[giCurPlayer];
    memset(mapExtra, 0, sizeof(mapExtra));
    if (!origData)
        SetupAdjacentMons();
    return 1;
}

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
    NEW_GAME_SCENARIO_PANEL = 0x12,
    NEW_GAME_KING_OF_THE_HILL = 0x13,
    NEW_GAME_RATING = 0x14,
    NEW_GAME_CANCEL = DIALOG_BUTTON_1,
    NEW_GAME_OK = DIALOG_BUTTON_2,
    // Player p's type toggle is p + TOGGLE_BASE (ids 2..4) and its type label
    // p + LABEL_BASE (ids 5..7).
    NEW_GAME_OPPONENT_TOGGLE_BASE = 1,
    NEW_GAME_OPPONENT_LABEL_BASE = 4
H1_ENUM_END(NewGameControl)

// newgame.icn frames UpdateNewGameWindow selects: the human-opponent face,
// the computer-type faces (type + base), the crests (two per color) and
// the King of the Hill toggle (flag + base).
H1_ENUM_CONST_BEGIN(NewGameFrame)
    NEW_GAME_FRAME_COMPUTER_TYPE_BASE = 5,
    NEW_GAME_FRAME_CREST_BASE = 11,
    NEW_GAME_FRAME_CREST_STRIDE = 2,
    NEW_GAME_FRAME_HUMAN_OPPONENT = 0x1a,
    NEW_GAME_FRAME_KING_OF_THE_HILL_BASE = 27
H1_ENUM_CONST_END(NewGameFrame)

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

// Buka 2.1 NewGameHandler without HoMM2's remote chat and player races:
// right clicks show help, the player toggles cycle the opponents and OK
// packs the chosen opponents before closing the dialog.
VA(0x0042d4f2, 0x4f3)
i16 NewGameHandler(tag_message& message) {
    i32 iPlayer;
    i32 i;
    i32 helpIndex;
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
                    case NEW_GAME_SCENARIO_PANEL:
                        helpIndex = NEW_GAME_HELP_SCENARIO;
                        break;
                    case NEW_GAME_SCENARIO_SELECT:
                        helpIndex = NEW_GAME_HELP_SCENARIO;
                        break;
                    case NEW_GAME_DIFFICULTY_FIRST:
                        helpIndex = NEW_GAME_HELP_DIFFICULTY;
                        break;
                    case NEW_GAME_DIFFICULTY_FIRST + DIFFICULTY_NORMAL:
                        helpIndex = NEW_GAME_HELP_DIFFICULTY;
                        break;
                    case NEW_GAME_DIFFICULTY_FIRST + DIFFICULTY_HARD:
                        helpIndex = NEW_GAME_HELP_DIFFICULTY;
                        break;
                    case NEW_GAME_DIFFICULTY_LAST:
                        helpIndex = NEW_GAME_HELP_DIFFICULTY;
                        break;
                    case NEW_GAME_OPPONENT_FIRST:
                    case NEW_GAME_OPPONENT_FIRST + 1:
                    case NEW_GAME_OPPONENT_LAST:
                        if (message.id - NEW_GAME_OPPONENT_TOGGLE_BASE < giNumHumanPlayers)
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
                    NormalDialog(
                        gNewGameHelp[helpIndex],
                        NORMAL_DIALOG_TYPE_QUICK_VIEW,
                        -1,
                        -1,
                        NORMAL_DIALOG_NO_RESOURCE,
                        0,
                        NORMAL_DIALOG_NO_RESOURCE,
                        0,
                        NORMAL_DIALOG_NO_OR_TEXT
                    );
            }
        } else {
            switch (message.command) {
                case WIDGET_NOTIFY_DESELECT:
                    switch (message.id) {
                        case NEW_GAME_OK:
                            gpGame->m_playerCount = 0;
                            for (i = 0; i < GAME_PLAYER_COUNT; i++) {
                                if (gpGame->m_players[i].m_difficulty > PLAYER_TYPE_NONE)
                                    gpGame->m_playerCount++;
                            }
                            if (gpGame->m_playerCount < GAME_MIN_PLAYER_COUNT) {
                                NormalDialog(
                                    "A game requires at least one opponent.",
                                    NORMAL_DIALOG_TYPE_OK,
                                    0xb1,
                                    0x3c,
                                    NORMAL_DIALOG_NO_RESOURCE,
                                    0,
                                    NORMAL_DIALOG_NO_RESOURCE,
                                    0,
                                    NORMAL_DIALOG_NO_OR_TEXT
                                );
                                break;
                            } else {
                                if (!gpGame->m_players[1].m_difficulty) {
                                    if (gpGame->m_players[2].m_difficulty) {
                                        gpGame->m_players[1].m_difficulty =
                                            gpGame->m_players[2].m_difficulty;
                                        gpGame->m_players[2].m_difficulty = PLAYER_TYPE_NONE;
                                    } else {
                                        gpGame->m_players[1].m_difficulty =
                                            gpGame->m_players[3].m_difficulty;
                                        gpGame->m_players[3].m_difficulty = PLAYER_TYPE_NONE;
                                    }
                                }
                                if (!gpGame->m_players[2].m_difficulty
                                    && gpGame->m_players[3].m_difficulty) {
                                    gpGame->m_players[2].m_difficulty =
                                        gpGame->m_players[3].m_difficulty;
                                    gpGame->m_players[3].m_difficulty = PLAYER_TYPE_NONE;
                                }
                            }
                        case NEW_GAME_CANCEL:
                            FINISH_DIALOG_MESSAGE(message);
                            return MESSAGE_DISPATCH_FORWARD;
                        default:
                            break;
                    }
                    break;
                case WIDGET_NOTIFY_SELECT:
                    switch (message.id) {
                        case NEW_GAME_DIFFICULTY_FIRST:
                        case NEW_GAME_DIFFICULTY_FIRST + DIFFICULTY_NORMAL:
                        case NEW_GAME_DIFFICULTY_FIRST + DIFFICULTY_HARD:
                        case NEW_GAME_DIFFICULTY_LAST:
                            gpGame->m_difficulty = message.id - NEW_GAME_DIFFICULTY_FIRST;
                            break;
                        case NEW_GAME_OPPONENT_FIRST:
                        case NEW_GAME_OPPONENT_FIRST + 1:
                        case NEW_GAME_OPPONENT_LAST:
                            iPlayer = message.id - NEW_GAME_OPPONENT_TOGGLE_BASE;
                            gpGame->m_players[iPlayer].m_difficulty++;
                            gpGame->m_players[iPlayer].m_difficulty %= PLAYER_TYPE_COUNT;
                            if (giNumHumanPlayers > iPlayer
                                && !gpGame->m_players[iPlayer].m_difficulty)
                                gpGame->m_players[iPlayer].m_difficulty = HUMAN_HANDICAP_EASY;
                            break;
                        case NEW_GAME_COLOR:
                            gpGame->m_players[0].m_color =
                                (gpGame->m_players[0].m_color + 1) % GAME_PLAYER_COUNT;
                            break;
                        case NEW_GAME_KING_OF_THE_HILL:
                            gbIAmGreatest = 1 - gbIAmGreatest;
                            break;
                        case NEW_GAME_SCENARIO_SELECT:
                        case NEW_GAME_SCENARIO_NAME:
                        case NEW_GAME_SCENARIO_PANEL:
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
VA(0x0042d9e5, 0x293)
void game::UpdateNewGameWindow(void) {
    tag_message message;
    i16 i;
    char* period;

    strcpy(gText, gFullMapName);
    period = strchr(gText, '.');
    if (period)
        *period = 0;
    SET_WIDGET_MESSAGE(message, WIDGET_COMMAND_SET_TEXT, NEW_GAME_SCENARIO_NAME);
    message.text = gText;
    m_newGameWindow->BroadcastMessage(message);
    message.command = WIDGET_COMMAND_CLEAR_FLAGS;
    message.value = WIDGET_FLAG_DRAW;
    for (i = 0; i < DIFFICULTY_COUNT; i++) {
        message.id = i + NEW_GAME_DIFFICULTY_FIRST;
        m_newGameWindow->BroadcastMessage(message);
    }
    message.command = WIDGET_COMMAND_SET_FLAGS;
    message.id = m_difficulty + NEW_GAME_DIFFICULTY_FIRST;
    m_newGameWindow->BroadcastMessage(message);
    message.command = WIDGET_COMMAND_SET_FRAME;
    for (i = 1; i < GAME_PLAYER_COUNT; i++) {
        message.id = i + NEW_GAME_OPPONENT_TOGGLE_BASE;
        if (i < giNumHumanPlayers)
            message.value = NEW_GAME_FRAME_HUMAN_OPPONENT;
        else
            message.value = m_players[i].m_difficulty + NEW_GAME_FRAME_COMPUTER_TYPE_BASE;
        m_newGameWindow->BroadcastMessage(message);
    }
    message.command = WIDGET_COMMAND_SET_TEXT;
    for (i = 1; i < GAME_PLAYER_COUNT; i++) {
        message.id = i + NEW_GAME_OPPONENT_LABEL_BASE;
        if (i < giNumHumanPlayers)
            message.text = gHumanPlayerTypeNames[m_players[i].m_difficulty];
        else
            message.text = gPlayerTypeNames[m_players[i].m_difficulty];
        m_newGameWindow->BroadcastMessage(message);
    }
    gpGame->m_difficultyRating = CalcDifficultyRating();
    message.id = NEW_GAME_RATING;
    sprintf(gText, "%s %d%%", localization::Tr("ui.new_game.difficulty_rating"), gpGame->m_difficultyRating);
    message.text = gText;
    m_newGameWindow->BroadcastMessage(message);
    message.command = WIDGET_COMMAND_SET_FRAME;
    if (m_players[0].m_color != PLAYER_COLOR_NONE) {
        message.id = NEW_GAME_COLOR;
        message.value =
            m_players[0].m_color * NEW_GAME_FRAME_CREST_STRIDE + NEW_GAME_FRAME_CREST_BASE;
        m_newGameWindow->BroadcastMessage(message);
    }
    message.command = WIDGET_COMMAND_SET_FRAME;
    message.id = NEW_GAME_KING_OF_THE_HILL;
    message.value = gbIAmGreatest + NEW_GAME_FRAME_KING_OF_THE_HILL_BASE;
    m_newGameWindow->BroadcastMessage(message);
}

// GiveTroopsToNeutralTowns (Buka NeutralTownReinforcementConstant names): a
// 1..15 roll picks the tier, whose key plus the town type selects the
// recruit and whose range the count.
H1_ENUM_CONST_BEGIN(NeutralTownReinforcementConstant)
    REINFORCEMENT_ROLL_MIN = 1,
    REINFORCEMENT_ROLL_MAX = 15,
    REINFORCEMENT_TIER_ONE_THRESHOLD = 5,
    REINFORCEMENT_TIER_TWO_THRESHOLD = 10,
    REINFORCEMENT_TIER_THREE_THRESHOLD = 13,
    REINFORCEMENT_TIER_ONE_KEY = 10,
    REINFORCEMENT_TIER_TWO_KEY = 20,
    REINFORCEMENT_TIER_THREE_KEY = 30,
    REINFORCEMENT_TIER_FOUR_KEY = 40,
    REINFORCEMENT_TIER_ONE_COUNT_MIN = 8,
    REINFORCEMENT_TIER_ONE_COUNT_MAX = 15,
    REINFORCEMENT_TIER_TWO_COUNT_MIN = 5,
    REINFORCEMENT_TIER_TWO_COUNT_MAX = 7,
    REINFORCEMENT_TIER_THREE_COUNT_MIN = 3,
    REINFORCEMENT_TIER_THREE_COUNT_MAX = 5,
    REINFORCEMENT_TIER_FOUR_COUNT_MIN = 1,
    REINFORCEMENT_TIER_FOUR_COUNT_MAX = 3
H1_ENUM_CONST_END(NeutralTownReinforcementConstant)

// Buka 2.1 game::GiveTroopsToNeutralTown inlined over every town: an
// unowned town on the map gains a random tier of its own creatures.
VA(0x0042dc78, 0x24c)
void game::GiveTroopsToNeutralTowns(void) {
    i32 howMany;
    i32 die;
    i32 i;
    i32 tier;
    i32 monster;
    for (i = 0; i < GAME_TOWN_COUNT; i++) {
        if ((m_castleRecs[i].m_x > 0 || m_castleRecs[i].m_y > 0) && m_castleRecs[i].m_owner < 0) {
            die = Random(REINFORCEMENT_ROLL_MIN, REINFORCEMENT_ROLL_MAX);
            if (die <= REINFORCEMENT_TIER_ONE_THRESHOLD) {
                tier = REINFORCEMENT_TIER_ONE_KEY;
                howMany =
                    Random(REINFORCEMENT_TIER_ONE_COUNT_MIN, REINFORCEMENT_TIER_ONE_COUNT_MAX);
            } else if (die <= REINFORCEMENT_TIER_TWO_THRESHOLD) {
                tier = REINFORCEMENT_TIER_TWO_KEY;
                howMany =
                    Random(REINFORCEMENT_TIER_TWO_COUNT_MIN, REINFORCEMENT_TIER_TWO_COUNT_MAX);
            } else if (die <= REINFORCEMENT_TIER_THREE_THRESHOLD) {
                tier = REINFORCEMENT_TIER_THREE_KEY;
                howMany =
                    Random(REINFORCEMENT_TIER_THREE_COUNT_MIN, REINFORCEMENT_TIER_THREE_COUNT_MAX);
            } else {
                tier = REINFORCEMENT_TIER_FOUR_KEY;
                howMany =
                    Random(REINFORCEMENT_TIER_FOUR_COUNT_MIN, REINFORCEMENT_TIER_FOUR_COUNT_MAX);
            }
            switch (m_castleRecs[i].m_type + tier) {
                case REINFORCEMENT_TIER_ONE_KEY + TOWN_TYPE_KNIGHT:
                    monster = CREATURE_PEASANT;
                    break;
                case REINFORCEMENT_TIER_TWO_KEY + TOWN_TYPE_KNIGHT:
                    monster = CREATURE_ARCHER;
                    break;
                case REINFORCEMENT_TIER_THREE_KEY + TOWN_TYPE_KNIGHT:
                    monster = CREATURE_PIKEMAN;
                    break;
                case REINFORCEMENT_TIER_FOUR_KEY + TOWN_TYPE_KNIGHT:
                    monster = CREATURE_SWORDSMAN;
                    break;
                case REINFORCEMENT_TIER_ONE_KEY + TOWN_TYPE_BARBARIAN:
                    monster = CREATURE_GOBLIN;
                    break;
                case REINFORCEMENT_TIER_TWO_KEY + TOWN_TYPE_BARBARIAN:
                    monster = CREATURE_ORC;
                    break;
                case REINFORCEMENT_TIER_THREE_KEY + TOWN_TYPE_BARBARIAN:
                    monster = CREATURE_WOLF;
                    break;
                case REINFORCEMENT_TIER_FOUR_KEY + TOWN_TYPE_BARBARIAN:
                    monster = CREATURE_OGRE;
                    break;
                case REINFORCEMENT_TIER_ONE_KEY + TOWN_TYPE_SORCERESS:
                    monster = CREATURE_SPRITE;
                    break;
                case REINFORCEMENT_TIER_TWO_KEY + TOWN_TYPE_SORCERESS:
                    monster = CREATURE_DWARF;
                    break;
                case REINFORCEMENT_TIER_THREE_KEY + TOWN_TYPE_SORCERESS:
                    monster = CREATURE_ELF;
                    break;
                case REINFORCEMENT_TIER_FOUR_KEY + TOWN_TYPE_SORCERESS:
                    monster = CREATURE_DRUID;
                    break;
                case REINFORCEMENT_TIER_ONE_KEY + TOWN_TYPE_WARLOCK:
                    monster = CREATURE_CENTAUR;
                    break;
                case REINFORCEMENT_TIER_TWO_KEY + TOWN_TYPE_WARLOCK:
                    monster = CREATURE_GARGOYLE;
                    break;
                case REINFORCEMENT_TIER_THREE_KEY + TOWN_TYPE_WARLOCK:
                    monster = CREATURE_GRIFFIN;
                    break;
                case REINFORCEMENT_TIER_FOUR_KEY + TOWN_TYPE_WARLOCK:
                    monster = CREATURE_MINOTAUR;
                    break;
            }
            GiveArmy(&m_castleRecs[i].m_army, monster, howMany, ARMY_GROUP_EMPTY_SLOT);
        }
    }
}

// Buka 2.1 game::NewGame: HoMM1 starts campaigns directly, restores the
// previous setup choices and falls back to a default map when the remembered
// one does not fit the human player count.
VA(0x0042dec4, 0x3e9)
i8 game::NewGame(void) {
    DATA(0x004909e0)
    static i8 gNewGameSettingsSaved = 0;
    i32 player;
    if (!SetupGame(1))
        return 0;
    if (gCampaignChoice > 0) {
        InitEntireCampaign(gCampaignChoice);
        return 1;
    }
    if (gbWaitForRemoteReceive)
        return 1;
    LoadGame("origdata.bin", 1, 0);
    m_newGameWindow = new heroWindow(310, 14, "newgame.bin");
    if (!m_newGameWindow)
        MemError();
    SetWinText(m_newGameWindow, WINDOW_TEXT_NEW_GAME);
    if (gNewGameSettingsSaved) {
        gpGame->m_difficulty = gSavedDifficulty;
        m_players[1].m_difficulty = gSavedPlayerTypes[1];
        m_players[2].m_difficulty = gSavedPlayerTypes[2];
        m_players[3].m_difficulty = gSavedPlayerTypes[3];
        gbIAmGreatest = gSavedKingOfTheHill;
        m_players[0].m_color = gSavedCrest;
        for (player = 1; giNumHumanPlayers > player; player++) {
            if (m_players[player].m_difficulty == HUMAN_HANDICAP_NONE)
                m_players[player].m_difficulty = m_players[0].m_difficulty;
        }
    }
    if (!strnicmp(gMapName, "camp", 4) || (giNumHumanPlayers == 1 && gMapName[4] != '1')
        || (giNumHumanPlayers == 2 && gMapName[5] != '2')
        || (giNumHumanPlayers == 3 && gMapName[6] != '3')
        || (giNumHumanPlayers == 4 && gMapName[7] != '4')) {
        if (giNumHumanPlayers == 1) {
            strcpy(gMapName, "AES31000.map");
            strcpy(gFullMapName, "Claw ( Easy )");
            strcpy(
                gMapDescription,
                "The Griffons will protect you until you are ready to make your move."
            );
            gMapSize = MAP_SIZE_SMALL;
            gMapDifficulty = MAP_DIFFICULTY_EASY;
        } else {
            strcpy(gMapName, "CNM51234.map");
            strcpy(gFullMapName, "Around the Bay");
            strcpy(gMapDescription, "A large island of tight passes with a circular feel.");
            gMapSize = MAP_SIZE_MEDIUM;
            gMapDifficulty = MAP_DIFFICULTY_NORMAL;
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
    m_mapSize = gMapSize;
    m_mapDifficulty = gMapDifficulty;
    strcpy(m_mapName, gFullMapName);
    gNewGameSettingsSaved = 1;
    gSavedDifficulty = gpGame->m_difficulty;
    gSavedPlayerTypes[1] = m_players[1].m_difficulty;
    gSavedPlayerTypes[2] = m_players[2].m_difficulty;
    gSavedPlayerTypes[3] = m_players[3].m_difficulty;
    gSavedKingOfTheHill = gbIAmGreatest;
    gSavedCrest = m_players[0].m_color;
    NewMap(gMapName);
    return 1;
}

// HoMM1 identity: advManager::ControlPanel calls it on gpGame with three
// arguments and the callee returns with `ret 0xc` (Buka game::ShowCampaignInfo).

VA(0x0042e2ad, 0x274)
void game::ShowCampaignInfo(i32 scenario, i32 fromMenu, i32) {
    heroWindow* window;
    tag_message message;

    gpMouseManager->SetPointer("advmice.mse", ADVENTURE_POINTER_DEFAULT);
    window = new heroWindow(105, 96, "campaign.bin");
    if (!window)
        MemError();
    SET_WIDGET_MESSAGE(message, WIDGET_COMMAND_SET_TEXT, CAMPAIGN_INFO_NAME);
    strcpy(gText, gCampaignScenarioNames[scenario]);
    message.text = gText;
    window->BroadcastMessage(message);
    message.id = CAMPAIGN_INFO_TEXT;
    strcpy(gText, gCampaignScenarioText[scenario]);
    message.text = gText;
    window->BroadcastMessage(message);
    message.text = gText;
    window->BroadcastMessage(message);
    message.type = MESSAGE_WIDGET;
    message.id = CAMPAIGN_INFO_PROGRESS;
    message.command = WIDGET_COMMAND_SET_FRAME;
    message.value = gpGame->m_campaignScenariosWon + CAMPAIGN_INFO_PROGRESS_FRAME_BASE;
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
        message.id = CAMPAIGN_INFO_RESTART;
        window->BroadcastMessage(message);
    }
    if (!fromMenu)
        PlayMusic(MUSIC_TRACK_MAIN_MENU);
    gpWindowManager->DoDialog(window, EventWindowHandler, 0);
    delete window;
    if (gpWindowManager->m_dialogResult == CAMPAIGN_INFO_RESTART) {
        NormalDialog(
            localization::Tr("campaign.restart.confirm"),
            NORMAL_DIALOG_TYPE_YES_NO,
            -1,
            -1,
            NORMAL_DIALOG_NO_RESOURCE,
            0,
            NORMAL_DIALOG_NO_RESOURCE,
            0,
            NORMAL_DIALOG_NO_OR_TEXT
        );
        if (gpWindowManager->m_dialogResult == NORMAL_DIALOG_CONFIRM) {
            InitCampaignMap(m_campaignScenario, 0);
            gpAdvManager->m_routeShown = 0;
            giBottomViewOverride = BOTTOM_VIEW_NONE;
            gpWindowManager->FadeScreen(WINDOW_FADE_OUT, WINDOW_FADE_STEPS_SHORT, gPalette);
            gpAdvManager->SetInitialMapOrigin();
            gpAdvManager->RedrawAdvScreen(1);
            gpWindowManager->FadeScreen(WINDOW_FADE_IN, WINDOW_FADE_STEPS_SHORT, gPalette);
        }
    }
}

// Buka 2.1 game::InitEntireCampaign; HoMM1 reloads origdata.bin first and
// starts the campaign calendar on day 1.
VA(0x0042e521, 0x74)
void game::InitEntireCampaign(i32 side) {
    LoadGame("origdata.bin", 1, 0);
    strcpy(gFullMapName, "");
    gpGame->m_difficulty = DIFFICULTY_EXPERT;
    m_campaignType = side;
    m_campaignScenario = 0;
    m_campaignScenariosWon = 0;
    m_campaignDay = 1;
    InitCampaignMap(m_campaignScenario, 0);
}

// Buka 2.1 game::InitCampaignMap reduced to HoMM1's CAMP%d.CMP maps: the
// calendar continues from m_campaignDay and the scenario table seeds the
// opponents and every player's resources.
VA(0x0042e595, 0x25b)
void game::InitCampaignMap(i32 scenario, i32) {
    i32 saveType;
    i32 savedScenario;
    i32 i;
    i32 j;
    i32 savedState;
    i32 savedDay;

    saveType = m_campaignType;
    savedScenario = m_campaignScenario;
    savedState = m_campaignScenariosWon;
    savedDay = m_campaignDay;
    LoadGame("origdata.bin", 1, 0);
    m_campaignType = saveType;
    m_campaignScenario = savedScenario;
    m_campaignScenariosWon = savedState;
    m_campaignDay = savedDay;
    m_month = (m_campaignDay - 1) / CALENDAR_DAYS_PER_MONTH + 1;
    m_week =
        (m_campaignDay - 1 - (m_month - 1) * CALENDAR_DAYS_PER_MONTH) / CALENDAR_DAYS_PER_WEEK + 1;
    m_day = (m_campaignDay - 1) % CALENDAR_DAYS_PER_WEEK + 1;
    giCurTurn =
        (m_month - 1) * CALENDAR_DAYS_PER_MONTH + (m_week - 1) * CALENDAR_DAYS_PER_WEEK + m_day;
    gbIAmGreatest = gCampaignScenarios[scenario].kingOfTheHill;
    giNumHumanPlayers = 0;
    m_players[0].m_difficulty = HUMAN_HANDICAP_EXPERT;
    m_players[0].m_color = gCampaignSideCrests[m_campaignType - 1][0];
    m_playerCount = 1;
    for (i = 1; i < GAME_PLAYER_COUNT; i++) {
        m_players[i].m_difficulty = gCampaignScenarios[scenario].playerTypes[i];
        if (m_players[i].m_difficulty)
            m_playerCount++;
    }
    giNumHumanPlayers = 1;
    sprintf(gMapName, "CAMP%d.CMP", scenario + 1);
    NewMap(gMapName);
    for (i = 0; i < GAME_PLAYER_COUNT; i++) {
        for (j = 0; j < RESOURCE_COUNT; j++)
            m_players[i].m_resources[j] = gCampaignScenarios[scenario].resources[i][j];
    }
}

// Buka 2.1 game::NewMap for HoMM1: map setup helpers, a starting town and
// hero per player (campaign crests pick them), two tavern heroes, the
// ultimate artifact site, starting resources, town threat ranks and the
// first neutral garrisons.
VA(0x0042e7f0, 0xe31)
void game::NewMap(char* mapName) {
    i32 nextThreat;
    i32 anyFree;
    i8 heroY;
    i8 heroX;
    i8 yTown;
    i8 xTown;
    i32 heroIdx;
    i32 i;
    i32 j;
    i8 townId;
    i8 used[GAME_TOWN_COUNT];
    i32 k;
    i32 ultimateSpread;
    i8 allNeutral;
    i32 difficulty;

    gbInNewGameSetup = 1;
    giCurPlayer = 0;
    gpCurPlayer = &gpGame->m_players[giCurPlayer];
    giCurPlayerBit = 1 << giCurPlayer;
    giCurWatchPlayerBit = giCurPlayerBit;
    giCurPlayerHighBit = 1 << (giCurPlayer + GAME_PLAYER_HIGH_BIT_SHIFT);
    gCurWatchPlayerHighBit = 1 << (giCurPlayer + GAME_PLAYER_HIGH_BIT_SHIFT);
    giCurWatchPlayer = giCurPlayer;
    for (i = 0; i < m_playerCount; i++) {
        m_players[i].m_townCount = 0;
        m_players[i].m_townLocatorPage = 0;
        m_players[i].m_currentTown = GAME_TOWN_NONE;
        m_players[i].m_heroCount = 0;
        m_players[i].m_heroLocatorPage = 0;
        m_players[i].m_currentHero = GAME_HERO_NONE;
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
        m_players[i].m_ultimateArtifactHintX = PLAYER_ULTIMATE_HINT_NONE;
        m_players[i].m_ultimateArtifactHintY = PLAYER_ULTIMATE_HINT_NONE;
        heroIdx = 0;
        if (allNeutral) {
            if (m_campaignType <= 0 || m_campaignScenario < CAMPAIGN_SCENARIO_LORD_FIRST
                || m_campaignScenario > CAMPAIGN_SCENARIO_LORD_LAST) {
                if (m_campaignType > 0) {
                    for (j = 0; j < GAME_PLAYER_COUNT; j++) {
                        if (gCrestTownTypes[m_players[i].m_color] == GetTown(j)->m_type) {
                            SetupTown(j, !gbHumanPlayer[i]);
                            ClaimTown(j, i);
                        }
                    }
                } else {
                    townId = RandomScan(m_townOwners, 0, GAME_PLAYER_COUNT, 8);
                    if (townId == GAME_TABLE_FREE)
                        townId = Scan(m_townOwners, 0, GAME_PLAYER_COUNT);
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
            || (m_campaignType > 0 && m_campaignScenario >= CAMPAIGN_SCENARIO_LORD_FIRST
                && m_campaignScenario <= CAMPAIGN_SCENARIO_LORD_LAST && i == 0)) {
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
        m_availableHeroes[m_players[i].m_availableHeroIds[0]] = HERO_AVAILABILITY_RETREATED;
        k = (Random(1, 3) + k) % HERO_CLASS_COUNT;
        m_players[i].m_availableHeroIds[1] = GetNewHeroId(k);
        m_availableHeroes[m_players[i].m_availableHeroIds[1]] = HERO_AVAILABILITY_RETREATED;
    }
    if (!m_noMapHeroes)
        ProcessOnMapHeroes();
    if (m_campaignType <= 0) {
        for (k = 0; k < GAME_PLAYER_COUNT; k++) {
            if (allNeutral && m_townOwners[k] == GAME_PLAYER_NONE) {
                xTown = m_castleRecs[k].m_x;
                yTown = m_castleRecs[k].m_y;
                for (i = 0; i < TOWN_FOOTPRINT_WIDTH; i++) {
                    m_map[xTown - TOWN_FOOTPRINT_LEFT + i][yTown - TOWN_FOOTPRINT_TOP]
                        .m_overlayIndex -= TOWN_CASTLE_FRAME_OFFSET;
                    m_map[xTown - TOWN_FOOTPRINT_LEFT + i][yTown - 1].m_objectIndex -=
                        TOWN_CASTLE_FRAME_OFFSET;
                    m_map[xTown - TOWN_FOOTPRINT_LEFT + i][yTown].m_objectIndex -=
                        TOWN_CASTLE_FRAME_OFFSET;
                }
                m_castleRecs[k].m_buildings = 1 << BUILDING_SLOT_TENT;
                if (m_castleRecs[k].m_type == TOWN_TYPE_BARBARIAN)
                    m_castleRecs[k].m_buildings |= 1 << BUILDING_SLOT_SPECIAL;
                SetupTown(k, 0);
            }
        }
    }
    for (i = 0; i < m_playerCount; i++) {
        for (j = 0; j < m_players[i].m_heroCount; j++) {
            heroX = m_heroRecs[m_players[i].m_heroIds[j]].m_x;
            heroY = m_heroRecs[m_players[i].m_heroIds[j]].m_y;
            m_heroRecs[m_players[i].m_heroIds[j]].m_locationType =
                m_map[heroX][heroY].m_triggerType;
            m_heroRecs[m_players[i].m_heroIds[j]].m_occupiedTown =
                m_map[heroX][heroY].m_objectMetadata;
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
    // 1.1 consumes the 30-range roll first; sequence it before the two 20-range rolls.
    ultimateSpread = Random(1, 30);
    ultimateSpread += Random(1, 20) + Random(1, 20);
    while (m_map[i][j].m_objectIndex != MAP_CELL_NO_FRAME
           || m_map[i][j].m_overlayIndex != MAP_CELL_NO_FRAME
           || m_map[i][j].m_tileIndex < MAP_CELL_TILES_PER_TERRAIN
           || (giNumHumanPlayers == 1
               && abs(i - m_heroRecs[m_players[0].m_heroIds[0]].m_x)
                          + abs(j - m_heroRecs[m_players[0].m_heroIds[0]].m_y)
                      <= ultimateSpread)) {
        ultimateSpread = Random(1, 30);
        ultimateSpread += Random(1, 20) + Random(1, 20);
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
        memcpy(
            m_players[i].m_resources,
            gStartingResources[difficulty],
            sizeof(m_players[i].m_resources)
        );
    }
    memset(used, -1, sizeof(used));
    for (i = 0; i < GAME_TOWN_COUNT; i++) {
        nextThreat = 0;
        anyFree = Scan(used, 0, GAME_TOWN_COUNT);
        if (anyFree != GAME_TABLE_FREE)
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
VA(0x0042f621, 0x51)
i32 GetObjectFamily(i32 trigger) {
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
VA(0x0042f672, 0x1be)
void game::SettleOverlay(i32 x, i32 y) {
    mapCell* cell;
    mapCell* cellEast;
    cell = &m_map[x][y];
    if (cell->m_objectIndex == MAP_CELL_NO_FRAME && cell->m_overlayIndex != MAP_CELL_NO_FRAME) {
        switch (cell->m_triggerType) {
            case MAP_OBJECT_MOUNTAINS_2:
            case MAP_OBJECT_TREES_2:
                if (x + 1 < MAP_CELL_GRID_SIZE) {
                    cellEast = &m_map[x + 1][y];
                    if (GetObjectFamily(cellEast->m_triggerType)
                        == GetObjectFamily(cell->m_triggerType)) {
                        cell->m_secondaryTrigger |= MAP_CELL_SECONDARY_BLOCKED;
                    } else {
                        cell->m_objectIndex = cell->m_overlayIndex;
                        cell->m_objectTileset = cell->m_overlayTileset;
                        cell->m_overlayTileset = 0;
                        cell->m_overlayIndex = MAP_CELL_NO_FRAME;
                    }
                }
                break;
            case MAP_OBJECT_MOUNTAINS_4:
            case MAP_OBJECT_TREES_4:
                if (x + 1 < MAP_CELL_GRID_SIZE) {
                    cellEast = &m_map[x + 1][y];
                    if (GetObjectFamily(cellEast->m_triggerType)
                        == GetObjectFamily(cell->m_triggerType)) {
                        cell->m_objectIndex = cell->m_overlayIndex;
                        cell->m_objectTileset = cell->m_overlayTileset;
                        cell->m_overlayTileset = 0;
                        cell->m_overlayIndex = MAP_CELL_NO_FRAME;
                    } else {
                        cell->m_secondaryTrigger |= MAP_CELL_SECONDARY_BLOCKED;
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
VA(0x0042f830, 0xb14)
void game::RandomizeEvents(void) {
    u8 overlayTileset;
    u8 objTileset;
    i16 y;
    i16 i;
    i16 j;
    i8 id;
    i32 siteNum;
    i16 x;
    mapCell* cell;
    i8 obeliskId;

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
                    cell->m_objectMetadata = MAP_EVENT_DATA_AVAILABLE;
                    break;
                case MAP_TRIGGER_EVENT | MAP_OBJECT_SKELETON:
                    if (Random(0, 9) == 3)
                        cell->m_objectMetadata = SKELETON_ARTIFACT;
                    else
                        cell->m_objectMetadata = SKELETON_EMPTY;
                    break;
                case MAP_TRIGGER_EVENT | MAP_OBJECT_DAEMON_CAVE:
                    switch (Random(0, 99) % 10) {
                        case 0:
                        case 1:
                        case 2:
                            cell->m_objectMetadata = DAEMON_REWARD_EXPERIENCE;
                            break;
                        case 3:
                            cell->m_objectMetadata = DAEMON_REWARD_ARTIFACT;
                            break;
                        case 4:
                        case 5:
                        case 6:
                            cell->m_objectMetadata = DAEMON_REWARD_EXPERIENCE_GOLD;
                            break;
                        case 7:
                        case 8:
                        case 9:
                            cell->m_objectMetadata = DAEMON_REWARD_RANSOM;
                            break;
                    }
                    break;
                case MAP_TRIGGER_EVENT | MAP_OBJECT_TREASURE_CHEST:
                    cell->m_objectMetadata = Random(2, 4);
                    break;
                case MAP_TRIGGER_EVENT | MAP_OBJECT_CAMPFIRE:
                    cell->m_objectMetadata = Random(4, 6) << CAMPFIRE_AMOUNT_SHIFT;
                    cell->m_objectMetadata |= static_cast<i8>(Random(0, 5));
                    break;
                case MAP_TRIGGER_EVENT | MAP_OBJECT_ANCIENT_LAMP:
                    cell->m_objectMetadata = Random(0, 3) + 2;
                    break;
                case MAP_TRIGGER_EVENT | MAP_OBJECT_SHIPWRECK:
                    if (x <= 0 || x >= MAP_CELL_GRID_SIZE - 1
                        || (m_map[x - 1][y].m_triggerType & MAP_TRIGGER_TYPE_MASK)
                               != MAP_OBJECT_SHIPWRECK) {
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
                            cell->m_objectMetadata = GHOST_SITE_SMALL;
                            break;
                        case 3:
                        case 4:
                        case 5:
                            cell->m_objectMetadata = GHOST_SITE_MEDIUM;
                            break;
                        case 6:
                        case 7:
                        case 8:
                            cell->m_objectMetadata = GHOST_SITE_LARGE;
                            break;
                        case 9:
                            cell->m_objectMetadata = GHOST_SITE_HUGE;
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
                    cell->m_objectMetadata = MAP_EVENT_DATA_AVAILABLE;
                    break;
                case MAP_TRIGGER_EVENT | MAP_OBJECT_MONSTER:
                    if (!static_cast<u8>(cell->m_objectMetadata)) {
                        cell->m_objectMetadata = GetRandomNumTroops(cell->m_objectIndex);
                        if (Random(0, 99) <= 25 && cell->m_objectIndex != CREATURE_GHOST)
                            cell->m_objectMetadata =
                                static_cast<u8>(cell->m_objectMetadata) | MONSTER_WILLING_FLAG;
                    }
                    break;
                case MAP_TRIGGER_EVENT | MAP_OBJECT_RESOURCE:
                    cell->m_objectMetadata = cell->m_objectIndex;
                    if (cell->m_objectIndex > 4)
                        cell->m_objectMetadata =
                            static_cast<u8>(cell->m_objectMetadata) - RESOURCE_PILE_OBJECT_BASE;
                    switch (static_cast<u8>(cell->m_objectMetadata)) {
                        case RESOURCE_WOOD:
                        case RESOURCE_ORE:
                            cell->m_objectMetadata = Random(8, 16);
                            break;
                        case RESOURCE_GOLD:
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
                            cell->m_objectMetadata =
                                gMageGuildSpellPool[MAGE_GUILD_STATE_LEVEL_1][Random(0, 7)] + 1;
                            break;
                        case 4:
                        case 5:
                        case 6:
                        case 7:
                            cell->m_objectMetadata =
                                gMageGuildSpellPool[MAGE_GUILD_STATE_LEVEL_2][Random(0, 7)] + 1;
                            break;
                        default:
                            cell->m_objectMetadata =
                                gMageGuildSpellPool[MAGE_GUILD_STATE_LEVEL_3][Random(0, 7)] + 1;
                            break;
                    }
                    break;
                case MAP_TRIGGER_EVENT | MAP_OBJECT_DESERT_TENT:
                    cell->m_objectMetadata = Random(10, 20);
                    break;
                case MAP_TRIGGER_EVENT | MAP_OBJECT_WAGON_CAMP:
                    if (x <= 0 || x >= MAP_CELL_GRID_SIZE - 1
                        || (m_map[x - 1][y].m_triggerType & MAP_TRIGGER_TYPE_MASK)
                               != MAP_OBJECT_WAGON_CAMP
                        || (m_map[x + 1][y].m_triggerType & MAP_TRIGGER_TYPE_MASK)
                               != MAP_OBJECT_WAGON_CAMP) {
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
                            cell->m_objectMetadata = ARTIFACT_EVENT_MODE_PICKUP;
                            break;
                        case 6:
                        case 7:
                            cell->m_objectMetadata = ARTIFACT_EVENT_MODE_GUARDED;
                            break;
                        case 8:
                        case 9:
                            cell->m_objectMetadata = ARTIFACT_EVENT_MODE_GOLD;
                            break;
                    }
                    break;
                case MAP_TRIGGER_EVENT | MAP_OBJECT_TOWN:
                    id = GetTownId(x, y);
                    for (j = 0; j < TOWN_FOOTPRINT_HEIGHT; j++) {
                        for (i = 0; i < TOWN_FOOTPRINT_WIDTH; i++) {
                            if (!static_cast<u8>(
                                    m_map[x - TOWN_FOOTPRINT_LEFT + i][y - TOWN_FOOTPRINT_TOP + j]
                                        .m_objectMetadata
                                ))
                                m_map[x - TOWN_FOOTPRINT_LEFT + i][y - TOWN_FOOTPRINT_TOP + j]
                                    .m_objectMetadata = id;
                        }
                    }
                    SetupTown(id, 0);
                    break;
                case MAP_TRIGGER_EVENT | MAP_OBJECT_ALCHEMIST_LAB:
                case MAP_TRIGGER_EVENT | MAP_OBJECT_MINE:
                case MAP_TRIGGER_EVENT | MAP_OBJECT_SAWMILL:
                    id = GetMineId(x, y);
                    for (j = 0; j < MINE_FOOTPRINT_HEIGHT; j++) {
                        for (i = 0; i < MINE_FOOTPRINT_WIDTH; i++) {
                            if (!static_cast<u8>(m_map[x + i][y - j].m_objectMetadata)
                                || (cell->m_triggerType & MAP_TRIGGER_TYPE_MASK)
                                       == (m_map[x + i][y - j].m_triggerType
                                           & MAP_TRIGGER_TYPE_MASK))
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
            if (cell->m_objectIndex != MAP_CELL_NO_FRAME
                && cell->m_overlayIndex != MAP_CELL_NO_FRAME) {
                objTileset = cell->m_objectTileset & MAP_CELL_TILESET_MASK;
                overlayTileset = cell->m_overlayTileset & MAP_CELL_TILESET_MASK;
                if ((objTileset == TILESET_MTN32 || objTileset == TILESET_TREE32)
                    && (overlayTileset == TILESET_MTN32 || overlayTileset == TILESET_TREE32))
                    cell->m_secondaryTrigger |= MAP_CELL_SECONDARY_BLOCKED;
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
                        cell->m_secondaryTrigger |= MAP_CELL_SECONDARY_BLOCKED;
                        break;
                }
            }
        }
    }
    for (y = 0; y < MAP_CELL_GRID_SIZE; y++) {
        for (x = 0; x < MAP_CELL_GRID_SIZE; x++) {
            cell = &m_map[x][y];
            if (cell->m_triggerType == MAP_OBJECT_SHADOW)
                cell->m_flags |= MAP_CELL_OBJECT_SHADOW_ONLY;
            if (cell->m_triggerType & MAP_TRIGGER_EVENT) {
                switch (cell->m_triggerType & MAP_TRIGGER_TYPE_MASK) {
                    case MAP_OBJECT_ALCHEMIST_LAB:
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
                    case MAP_OBJECT_ULTIMATE_ARTIFACT:
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
VA(0x00430344, 0x3c6)
i16 game::LoadMap(char* filename) {
    void* buf;
    i16 width;
    i8 y;
    i16 height;
    i16 i;
    i32 handle;
    i8 x;
    i8 type;
    i32 unused;
    i16 version;

    extern char gMapPath[];
    sprintf(gText, "%s%s", gMapPath, filename);
    handle = open(gText, O_BINARY);
    if (handle == -1)
        FileError(gText);
    read(handle, &version, 2);
    if (version == MAP_HEADER_ID) {
        buf = malloc(sizeof(SMapHeader));
        read(handle, buf, sizeof(SMapHeader) - sizeof(version));
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
            m_castleRecs[i].m_type = type & MAP_TOWN_TYPE_MASK;
            if ((type & MAP_TOWN_TYPE_MASK) == TOWN_TYPE_BARBARIAN)
                m_castleRecs[i].m_buildings |= 1 << BUILDING_SLOT_SPECIAL;
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
    if (version >= MAP_EXTRA_VERSION) {
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
VA(0x0043070a, 0x29b)
void game::ClaimTown(i8 townId, i8 player) {
    i32 i;
    town* townRec;
    mapCell* cell;

    townRec = &m_castleRecs[townId];
    if (townRec->m_owner == player)
        return;
    if (m_townOwners[townId] != GAME_PLAYER_NONE)
        gpGame->GetTown(townId)->Deallocate();
    for (i = 0; i < ARMY_GROUP_SLOT_COUNT; ++i) {
        townRec->m_army.m_creatureTypes[i] = CREATURE_NONE;
        townRec->m_army.m_creatureCounts[i] = 0;
    }
    if (m_castleRecs[townId].m_owner == GAME_PLAYER_NONE)
        m_castleRecs[townId].m_turnsOwned = 2;
    else
        m_castleRecs[townId].m_turnsOwned = 0;
    m_castleRecs[townId].m_owner = player;
    m_townOwners[townId] = player;
    m_players[player].m_townIds[m_players[player].m_townCount] = townId;
    m_players[player].m_townCount++;

    cell = &m_map[m_castleRecs[townId].m_x - 1][m_castleRecs[townId].m_y];
    cell->m_flags |= MAP_CELL_OBJECT_EXTRA;
    cell->m_objectTileset |= TILESET_FLAG32 << MAP_CELL_EXTRA_TILESET_SHIFT;
    cell->m_extraFrame = m_players[player].Color() * 2;
    cell = &m_map[m_castleRecs[townId].m_x + 1][m_castleRecs[townId].m_y];
    cell->m_flags |= MAP_CELL_OBJECT_EXTRA;
    cell->m_objectTileset |= TILESET_FLAG32 << MAP_CELL_EXTRA_TILESET_SHIFT;
    cell->m_extraFrame = m_players[player].Color() * 2 + 1;
    SetVisibility(m_castleRecs[townId].m_x, m_castleRecs[townId].m_y, player, gVisRangeTown);
    CheckEndGame(0);
}

// Buka 2.1 game::ClaimMine reduced to HoMM1's flag placement: the flag cell
// sits beside the mine by type and shows the owner's colour frame.
VA(0x004309a5, 0x24a)
void game::ClaimMine(i8 mineId, i8 player) {
    i16 frame;
    mapCell* cell;
    m_mines[mineId].owner = player;
    m_mineOwners[mineId] = player;
    switch (m_mines[mineId].type) {
        case MAP_OBJECT_DRAGON_CITY:
            frame = 0x14;
            break;
        case MAP_OBJECT_LIGHTHOUSE:
            frame = 0x18;
            break;
        case RESOURCE_WOOD:
            frame = 0x10;
            break;
        case RESOURCE_MERCURY:
            frame = 0xc;
            break;
        default:
            frame = 8;
            break;
    }
    switch (m_mines[mineId].type) {
        case RESOURCE_MERCURY:
            cell = &m_map[m_mines[mineId].x][m_mines[mineId].y - 2];
            break;
        case MAP_OBJECT_DRAGON_CITY:
            cell = &m_map[m_mines[mineId].x - 1][m_mines[mineId].y - 3];
            break;
        case MAP_OBJECT_LIGHTHOUSE:
            cell = &m_map[m_mines[mineId].x - 2][m_mines[mineId].y];
            break;
        default:
            cell = &m_map[m_mines[mineId].x][m_mines[mineId].y - 1];
            break;
    }
    if (player == GAME_PLAYER_NONE) {
        cell->m_flags ^= MAP_CELL_OVERLAY_EXTRA;
    } else {
        cell->m_flags |= MAP_CELL_OVERLAY_EXTRA;
        cell->m_overlayTileset |= TILESET_FLAG32 << MAP_CELL_EXTRA_TILESET_SHIFT;
        cell->m_extraFrame = m_players[player].Color() + frame;
    }
}

// Buka 2.1 game::ViewSpells for HoMM1's spell book: combat (0) and
// adventure (1) books each have their own window position; type 2 shows
// both tabs.
VA(0x00430bef, 0x23b)
i8 game::ViewSpells(
    class hero* spellHero,
    H1_ENUM_PARAM(HeroSpellType, i8) spellType,
    i16 (*callback)(struct tag_message&),
    i8 readOnly
) {
    tag_message message;

    m_viewSpell = SPELL_NONE;
    i16 winX[3] = {177, 97, 177};
    i16 winY[3] = {100, 47, 100};
    if (!spellHero->GetNumSpells(spellType)) {
        NormalDialog(
            "No spells to cast.",
            NORMAL_DIALOG_TYPE_OK,
            -1,
            -1,
            NORMAL_DIALOG_NO_RESOURCE,
            0,
            NORMAL_DIALOG_NO_RESOURCE,
            0,
            NORMAL_DIALOG_NO_OR_TEXT
        );
    } else {
        m_viewSpellsCallback = callback;
        m_viewSpellsReadOnly = readOnly;
        m_viewSpellsHero = spellHero;
        SetupSpellRange(spellType);
        m_viewSpellsTop = m_spellFirst;
        if (spellType == SPELL_TYPE_ALL || spellType == SPELL_TYPE_COMBAT) {
            m_viewSpellsWindow = new heroWindow(146, 145, "spellwin.bin");
            if (!m_viewSpellsWindow)
                MemError();
        } else {
            m_viewSpellsWindow = new heroWindow(97, 145, "spellwin.bin");
            if (!m_viewSpellsWindow)
                MemError();
        }
        if (spellType != SPELL_TYPE_ALL) {
            message.type = MESSAGE_WIDGET;
            message.command = WIDGET_COMMAND_CLEAR_FLAGS;
            if (spellType == SPELL_TYPE_COMBAT)
                message.id = SPELL_BOOK_ADVENTURE_SPELLS;
            else
                message.id = SPELL_BOOK_COMBAT_SPELLS;
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
VA(0x00430e2a, 0xa0)
void game::SetupSpellRange(H1_ENUM_PARAM(HeroSpellType, i16) spellType) {
    switch (spellType) {
        case SPELL_TYPE_COMBAT:
            m_spellFirst = 0;
            m_spellLast = m_spellFirst + HERO_COMBAT_SPELL_SLOT_COUNT - 1;
            break;
        default:
            m_spellFirst = HERO_COMBAT_SPELL_SLOT_COUNT;
            m_spellLast = m_spellFirst + HERO_SPELL_SLOT_COUNT - HERO_COMBAT_SPELL_SLOT_COUNT - 1;
            break;
    }
    while (m_viewSpellsHero->m_spellCharges[m_spellLast] < 1)
        m_spellLast--;
}

// Buka 2.1 game::UpdateSpellWidgets for HoMM1's four-spell page: each slot
// shows the spell icon and its name with the remaining casts.
VA(0x00430eca, 0x1d4)
void game::UpdateSpellWidgets(void) {
    tag_message message;
    i16 i;

    message.type = MESSAGE_WIDGET;
    for (i = 0; i < SPELL_BOOK_PAGE_SIZE; i++) {
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
VA(0x0043109e, 0x494)
i16 ViewSpellsHandler(tag_message& message) {
    i32 spell;
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
                            spell =
                                gpGame->m_viewSpellsHero->m_spells
                                    [message.id - SPELL_BOOK_ENTRY_FIRST + gpGame->m_viewSpellsTop];
                            NormalDialog(
                                gSpellDesc[spell],
                                NORMAL_DIALOG_TYPE_QUICK_VIEW,
                                -1,
                                -1,
                                NORMAL_DIALOG_SPELL,
                                spell,
                                NORMAL_DIALOG_NO_RESOURCE,
                                0,
                                NORMAL_DIALOG_NO_OR_TEXT
                            );
                            break;
                        case SPELL_BOOK_PREVIOUS_PAGE:
                            NormalDialog(
                                gSpellHelp[SPELL_HELP_PREVIOUS_PAGE],
                                NORMAL_DIALOG_TYPE_QUICK_VIEW,
                                -1,
                                -1,
                                NORMAL_DIALOG_NO_RESOURCE,
                                0,
                                NORMAL_DIALOG_NO_RESOURCE,
                                0,
                                NORMAL_DIALOG_NO_OR_TEXT
                            );
                            break;
                        case SPELL_BOOK_NEXT_PAGE:
                            NormalDialog(
                                gSpellHelp[SPELL_HELP_NEXT_PAGE],
                                NORMAL_DIALOG_TYPE_QUICK_VIEW,
                                -1,
                                -1,
                                NORMAL_DIALOG_NO_RESOURCE,
                                0,
                                NORMAL_DIALOG_NO_RESOURCE,
                                0,
                                NORMAL_DIALOG_NO_OR_TEXT
                            );
                            break;
                        case SPELL_BOOK_ADVENTURE_SPELLS:
                            NormalDialog(
                                gSpellHelp[SPELL_HELP_ADVENTURE_SPELLS],
                                NORMAL_DIALOG_TYPE_QUICK_VIEW,
                                -1,
                                -1,
                                NORMAL_DIALOG_NO_RESOURCE,
                                0,
                                NORMAL_DIALOG_NO_RESOURCE,
                                0,
                                NORMAL_DIALOG_NO_OR_TEXT
                            );
                            break;
                        case SPELL_BOOK_COMBAT_SPELLS:
                            NormalDialog(
                                gSpellHelp[SPELL_HELP_COMBAT_SPELLS],
                                NORMAL_DIALOG_TYPE_QUICK_VIEW,
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
                } else {
                    switch (message.id) {
                        case SPELL_BOOK_ENTRY_FIRST:
                        case SPELL_BOOK_ENTRY_FIRST + 1:
                        case SPELL_BOOK_ENTRY_FIRST + 2:
                        case SPELL_BOOK_ENTRY_LAST:
                            if (gpGame->m_viewSpellsReadOnly) {
                                spell = gpGame->m_viewSpellsHero->m_spells
                                            [message.id - SPELL_BOOK_ENTRY_FIRST
                                             + gpGame->m_viewSpellsTop];
                                NormalDialog(
                                    gSpellDesc[spell],
                                    NORMAL_DIALOG_TYPE_OK,
                                    -1,
                                    -1,
                                    NORMAL_DIALOG_SPELL,
                                    spell,
                                    NORMAL_DIALOG_NO_RESOURCE,
                                    0,
                                    NORMAL_DIALOG_NO_OR_TEXT
                                );
                                return MESSAGE_DISPATCH_CONSUME;
                            }
                            gpGame->m_viewSpell =
                                gpGame->m_viewSpellsHero->m_spells
                                    [message.id - SPELL_BOOK_ENTRY_FIRST + gpGame->m_viewSpellsTop];
                            message.command = WIDGET_COMMAND_DIALOG_SELECT;
                            return MESSAGE_DISPATCH_FORWARD;
                        case SPELL_BOOK_PREVIOUS_PAGE:
                            if (gpGame->m_viewSpellsTop == gpGame->m_spellFirst)
                                break;
                            gpGame->m_viewSpellsTop -= SPELL_BOOK_PAGE_SIZE;
                            if (gpGame->m_viewSpellsTop < gpGame->m_spellFirst)
                                gpGame->m_viewSpellsTop = gpGame->m_spellFirst;
                            gpGame->UpdateSpellWidgets();
                            gpGame->m_viewSpellsWindow->MoveWindow(0, 0);
                            break;
                        case SPELL_BOOK_NEXT_PAGE:
                            if (gpGame->m_viewSpellsTop + SPELL_BOOK_PAGE_SIZE
                                <= gpGame->m_spellLast)
                                gpGame->m_viewSpellsTop += SPELL_BOOK_PAGE_SIZE;
                            if (gpGame->m_viewSpellsTop < gpGame->m_spellFirst)
                                gpGame->m_viewSpellsTop = gpGame->m_spellFirst;
                            gpGame->UpdateSpellWidgets();
                            gpGame->m_viewSpellsWindow->MoveWindow(0, 0);
                            break;
                        case SPELL_BOOK_ADVENTURE_SPELLS:
                            gpGame->SetupSpellRange(SPELL_TYPE_ADVENTURE);
                            gpGame->m_viewSpellsTop = gpGame->m_spellFirst;
                            gpGame->UpdateSpellWidgets();
                            gpGame->m_viewSpellsWindow->MoveWindow(0, 0);
                            break;
                        case SPELL_BOOK_COMBAT_SPELLS:
                            gpGame->SetupSpellRange(SPELL_TYPE_COMBAT);
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
VA(0x00431532, 0x129)
i16 ViewSpecialHandler(tag_message& message) {
    if (message.type == MESSAGE_WIDGET) {
        switch (message.command) {
            case WIDGET_COMMAND_HOVER:
                if (message.id == gpWindowManager->m_lastHoverId)
                    return MESSAGE_DISPATCH_CONSUME;
                gpWindowManager->m_lastHoverId = message.id;
                switch (message.id) {
                    case SPELL_BOOK_PREVIOUS_PAGE:
                        strcpy(gText, gSpellHelp[SPELL_HELP_PREVIOUS_PAGE]);
                        break;
                    case SPELL_BOOK_NEXT_PAGE:
                        strcpy(gText, gSpellHelp[SPELL_HELP_NEXT_PAGE]);
                        break;
                    case SPELL_BOOK_ADVENTURE_SPELLS:
                        strcpy(gText, gSpellHelp[SPELL_HELP_ADVENTURE_SPELLS]);
                        break;
                    case SPELL_BOOK_COMBAT_SPELLS:
                        strcpy(gText, gSpellHelp[SPELL_HELP_COMBAT_SPELLS]);
                        break;
                    case DIALOG_BUTTON_0:
                        strcpy(gText, gSpellHelp[SPELL_HELP_CLOSE]);
                        break;
                    default:
                        strcpy(gText, gSpellHelp[SPELL_HELP_VIEW_SPELLS]);
                        break;
                }
                HeroMessageUpdate(gText);
                return MESSAGE_DISPATCH_CONSUME;
        }
    }
    return MESSAGE_DISPATCH_CONSUME;
}

// armywin.bin widget ids; Buka ViewArmyControlId names the dismiss (DIALOG_BUTTON_3)
// and close (DIALOG_BUTTON_0) buttons. The animation icon cycles
// VIEW_ARMY_ANIMATION_FRAMES frames every VIEW_ARMY_FRAME_DELAY ticks.
H1_ENUM_BEGIN(ViewArmyControl)
    VIEW_ARMY_COUNT_FRAME = 1,
    VIEW_ARMY_COUNT_TEXT = 2,
    VIEW_ARMY_TITLE = 3,
    VIEW_ARMY_STATS = 4,
    VIEW_ARMY_ANIMATION = 5,
    VIEW_ARMY_DISMISS = DIALOG_BUTTON_3,
    VIEW_ARMY_CLOSE = DIALOG_BUTTON_0
H1_ENUM_END(ViewArmyControl)

H1_ENUM_CONST_BEGIN(ViewArmyConstant)
    VIEW_ARMY_ANIMATION_FRAMES = 6,
    VIEW_ARMY_FRAME_DELAY = 90,
    VIEW_ARMY_STAT_TEXT_SIZE = 550,
    // glTimers slot the army window's animation runs on.
    VIEW_ARMY_TIMER_SLOT = 0
H1_ENUM_CONST_END(ViewArmyConstant)

// donor PoL RVA 0x0007a649; preferred Buka symbol ?ViewArmy@game@@QAEXHHHHPAVtown@@HHHPAVhero@@PAVarmy@@PAVarmyGroup@@H@Z
// donor Buka TU SOURCE/GAME; HoMM1 owner inferred from contiguous order
// evidence: graph:5;base=0.612909;margin=0.340762;shape=0.385;size=0.681;calls=0.829;strings= (%d)|%s%d|armywin.bin;alternate=pol20:void game::ViewArmy(int, int, int, int, class town *, int, int, int, class hero *, class army *, class armyGroup *, int)@0x0007a649
VA(0x0043165b, 0x8af)
void game::ViewArmy(
    i16 x,
    i16 y,
    i8 monsterType,
    i16 numTroops,
    class town* castle,
    i8 disableDismiss,
    i8 facing,
    i8 quickView,
    class hero* theHero,
    class army* theArmy,
    class armyGroup* theGroup
) {
    char numText[12];
    i32 shotCount;
    i16 baseX;
    i16 spacing;
    i16 topY;
    i16 animId;
    i32 morale;
    tag_monsterInfo* monsterInfo;
    i16 numId;
    i32 i;
    char* statText;
    i32 luck;
    tag_message message;
    char iconName[16];
    iconWidget* monsterWidget;
    i16 statsMessage;
    i16 titleLabel;
    i32 mod;
    i16 blankBtn;
    char fileName[13];

    baseX = 86;
    topY = 164;
    blankBtn = VIEW_ARMY_COUNT_FRAME;
    numId = VIEW_ARMY_COUNT_TEXT;
    titleLabel = VIEW_ARMY_TITLE;
    statsMessage = VIEW_ARMY_STATS;
    animId = VIEW_ARMY_ANIMATION;
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
    if (monsterInfo->stats.attributes & MONSTER_FLAGS_WIDE) {
        switch (facing) {
            case ARMY_FACING_RIGHT:
                spacing += 43;
                break;
            case ARMY_FACING_LEFT:
                spacing += 119;
                break;
        }
    } else if (facing == ARMY_FACING_LEFT) {
        spacing += 76;
    } else {
        spacing += 86;
    }
    if (monsterInfo->stats.attributes & MONSTER_FLAGS_FLYING)
        sprintf(fileName, "%s.wlk", iconName);
    else
        sprintf(fileName, "%s.wip", iconName);
    monsterWidget = new iconWidget(
        spacing,
        164,
        86,
        149,
        fileName,
        0,
        facing == ARMY_FACING_LEFT,
        VIEW_ARMY_ANIMATION,
        ICON_WIDGET_DRAW,
        1
    );
    if (!monsterWidget)
        MemError();
    m_viewArmyWindow->AddWidget(monsterWidget, WINDOW_Z_ORDER_APPEND);

    strcpy(fileName, gArmyNames[monsterType]);
    fileName[0] -= 'a' - 'A';
    message.command = WIDGET_COMMAND_SET_TEXT;
    message.id = VIEW_ARMY_TITLE;
    message.text = fileName;
    m_viewArmyWindow->BroadcastMessage(message);

    statText = static_cast<char*>(malloc(VIEW_ARMY_STAT_TEXT_SIZE));
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

    if (monsterInfo->stats.attributes & MONSTER_FLAGS_SHOOTER) {
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
    sprintf(gText, "\n%s%d", gArmyStatText[4], static_cast<u8>(monsterInfo->stats.hitPoints));
    strcat(statText, gText);
    sprintf(gText, "\n%s%s", gArmyStatText[5], gSpeedText[monsterInfo->stats.speed]);
    strcat(statText, gText);
    sprintf(gText, "\n%s%s", gArmyStatText[6], gMoraleText[morale + 3]);
    strcat(statText, gText);
    luck = GetLuck(theHero, theArmy);
    sprintf(gText, "\n%s%s", gArmyStatText[7], gLuckText[luck + 3]);
    strcat(statText, gText);

    message.id = VIEW_ARMY_STATS;
    message.text = statText;
    m_viewArmyWindow->BroadcastMessage(message);
    if (disableDismiss) {
        message.command = WIDGET_COMMAND_CLEAR_FLAGS;
        message.value = WIDGET_FLAG_ENABLED | WIDGET_FLAG_DRAW;
        message.id = VIEW_ARMY_DISMISS;
        m_viewArmyWindow->BroadcastMessage(message);
    }
    if (quickView) {
        message.command = WIDGET_COMMAND_CLEAR_FLAGS;
        message.value = WIDGET_FLAG_ENABLED | WIDGET_FLAG_DRAW;
        message.id = VIEW_ARMY_CLOSE;
        m_viewArmyWindow->BroadcastMessage(message);
    }
    if (numTroops < 1) {
        message.command = WIDGET_COMMAND_CLEAR_FLAGS;
        message.value = WIDGET_FLAG_ENABLED | WIDGET_FLAG_DRAW;
        message.id = VIEW_ARMY_COUNT_FRAME;
        m_viewArmyWindow->BroadcastMessage(message);
        message.id = VIEW_ARMY_COUNT_TEXT;
        m_viewArmyWindow->BroadcastMessage(message);
    } else {
        sprintf(numText, "%d", numTroops);
        message.command = WIDGET_COMMAND_SET_TEXT;
        message.id = VIEW_ARMY_COUNT_TEXT;
        message.text = numText;
        m_viewArmyWindow->BroadcastMessage(message);
    }
    glTimers[VIEW_ARMY_TIMER_SLOT] = KBTickCount() + VIEW_ARMY_FRAME_DELAY;
    m_viewArmyResult = 0;
    if (quickView) {
        gpMouseManager->ReallyHidePointer();
        gpWindowManager->AddWindow(m_viewArmyWindow, WINDOW_Z_ORDER_APPEND, 1);
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
VA(0x00431f0a, 0x17f)
i16 ViewArmyHandler(tag_message& message) {
    i16 frameDelay;
    i16 offset;
    gbDismissArmy = 0;
    frameDelay = 5;
    if (message.type == MESSAGE_WIDGET) {
        switch (message.command) {
            case WIDGET_NOTIFY_DESELECT:
                switch (message.id) {
                    case DIALOG_BUTTON_0:
                    case DIALOG_BUTTON_1:
                        FINISH_DIALOG_MESSAGE(message);
                        return MESSAGE_DISPATCH_FORWARD;
                    case VIEW_ARMY_DISMISS:
                        NormalDialog(
                            "Are you sure you want to dismiss this army?",
                            NORMAL_DIALOG_TYPE_YES_NO,
                            0xb1,
                            0x36,
                            NORMAL_DIALOG_NO_RESOURCE,
                            0,
                            NORMAL_DIALOG_NO_RESOURCE,
                            0,
                            NORMAL_DIALOG_NO_OR_TEXT
                        );
                        if (gpWindowManager->m_dialogResult == NORMAL_DIALOG_CONFIRM) {
                            gbDismissArmy = 1;
                            message.command = message.id = WIDGET_COMMAND_DIALOG_SELECT;
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
    if (KBTickCount() > glTimers[VIEW_ARMY_TIMER_SLOT]) {
        SET_WIDGET_MESSAGE(message, WIDGET_COMMAND_SET_FRAME, VIEW_ARMY_ANIMATION);
        gpGame->m_viewArmyResult++;
        message.value = gpGame->m_viewArmyResult % VIEW_ARMY_ANIMATION_FRAMES;
        gpGame->m_viewArmyWindow->BroadcastMessage(message);
        gpGame->m_viewArmyWindow->DrawWindow();
        glTimers[VIEW_ARMY_TIMER_SLOT] = KBTickCount() + VIEW_ARMY_FRAME_DELAY;
    }
    return MESSAGE_DISPATCH_CONSUME;
}

// overwind.bin widget ids: resource r's count is RESOURCE_BASE + r.
H1_ENUM_BEGIN(OverviewControl)
    OVERVIEW_RESOURCE_BASE = 1,
    OVERVIEW_DATE = 64,
    OVERVIEW_DAILY_GOLD = 65
H1_ENUM_END(OverviewControl)

// Kingdom overview: heroes by class, castles and towns by type and mines by
// resource drawn onto the backdrop, then the date, income and resources.
VA(0x00432089, 0xc81)
void game::Overview(void) {
    i16 unusedGY;
    i16 townTop;
    i16 unusedFVal;
    i16 heroNumYW;
    i16 incomeWidgetY;
    font* smallFont;
    i16 heroTextH;
    i16 townTextWPos;
    i16 spacing;
    i16 heroTextW;
    i16 castleFrameY;
    i16 limitYOff;
    i16 dayIdY;
    i16 left;
    i16 castleIconY;
    i8 mineNums[RESOURCE_COUNT];
    i16 fieldH;
    font* bigFont;
    i16 textW;
    i16 mineRowY;
    i16 i;
    i16 badgeY;
    i16 lineH;
    i16 mineW;
    i8 redraw;
    tag_message message;
    i16 totals[TOWN_TYPE_COUNT];
    i16 numMines;
    i16 numCastles;
    i16 numTowns;
    heroWindow* win;
    i16 firstTown;
    i16 classCountY;
    icon* ovIcon;
    i16 heroFrame;
    i16 mineBase;
    i16 badge;
    i16 spare1;
    i16 heroRowY;
    i16 shieldDXX;
    i16 one;
    i16 nextType;

    gpAdvManager->TrimLoopingSounds(8);
    gOverviewShowing = 1;
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
    gpWindowManager->FadeScreen(WINDOW_FADE_OUT, WINDOW_FADE_STEPS_SHORT, NULL);
    gpResourceManager->GetBackdropAtLoc("overmain.bmp", gpWindowManager->m_screen, 96, 0);
    sprintf(gText, "overban%01d.bmp", gpCurPlayer->m_color);
    gpResourceManager->GetBackdropAtLoc(gText, gpWindowManager->m_screen, 0, 0);
    ovIcon = gpResourceManager->GetIcon("overview.icn");

    memset(totals, 0, sizeof(totals));
    for (i = 0; i < gpCurPlayer->m_heroCount; i++)
        totals[m_heroRecs[gpCurPlayer->m_heroIds[i]].m_heroClass]++;
    classCountY = 0;
    for (i = 0; i < HERO_CLASS_COUNT; i++) {
        if (totals[i])
            classCountY++;
    }
    spacing = 136;
    left = 121;
    nextType = 0;
    for (i = 0; i < classCountY; i++) {
        while (!totals[nextType])
            nextType++;
        ovIcon->DrawToBuffer(
            spacing * i + left,
            32,
            nextType,
            ICON_DRAW_NORMAL,
            ICON_DRAW_OFFSET_FULL
        );
        ovIcon->DrawToBuffer(
            spacing * i + left + 49,
            67,
            15,
            ICON_DRAW_NORMAL,
            ICON_DRAW_OFFSET_FULL
        );
        sprintf(gText, "%d", totals[nextType]);
        bigFont
            ->DrawBoundedString(gText, spacing * i + left + 48, 77, 33, 16, 1, FONT_ALIGN_CENTER);
        nextType++;
    }

    memset(totals, 0, sizeof(totals));
    for (i = 0; i < gpCurPlayer->m_townCount; i++) {
        if (m_castleRecs[gpCurPlayer->m_townIds[i]].m_buildings & (1 << BUILDING_SLOT_CASTLE))
            totals[m_castleRecs[gpCurPlayer->m_townIds[i]].m_type]++;
    }
    numCastles = 0;
    for (i = 0; i < TOWN_TYPE_COUNT; i++) {
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
            ovIcon->DrawToBuffer(
                spacing * i + left,
                113,
                nextType + 4,
                ICON_DRAW_NORMAL,
                ICON_DRAW_OFFSET_FULL
            );
            sprintf(gText, "%d", totals[nextType]);
            bigFont
                ->DrawBoundedString(gText, spacing * i + left, 173, 132, 16, 1, FONT_ALIGN_CENTER);
            nextType++;
        }
    }

    memset(totals, 0, sizeof(totals));
    for (i = 0; i < gpCurPlayer->m_townCount; i++) {
        if (!(m_castleRecs[gpCurPlayer->m_townIds[i]].m_buildings & (1 << BUILDING_SLOT_CASTLE)))
            totals[m_castleRecs[gpCurPlayer->m_townIds[i]].m_type]++;
    }
    numTowns = 0;
    for (i = 0; i < TOWN_TYPE_COUNT; i++) {
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
            ovIcon->DrawToBuffer(
                spacing * i + left,
                201,
                nextType + 8,
                ICON_DRAW_NORMAL,
                ICON_DRAW_OFFSET_FULL
            );
            sprintf(gText, "%d", totals[nextType]);
            bigFont
                ->DrawBoundedString(gText, spacing * i + left, 261, 132, 16, 1, FONT_ALIGN_CENTER);
            nextType++;
        }
    }

    memset(mineNums, 0, sizeof(mineNums));
    for (i = MINE_SLOT_STANDARD_FIRST; i < GAME_MINE_COUNT; i++) {
        if (m_mineOwners[i] == giCurPlayer)
            mineNums[m_mines[i].type]++;
    }
    numMines = 0;
    for (i = 0; i < RESOURCE_COUNT; i++) {
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
            ovIcon->DrawToBuffer(
                spacing * i + left,
                289,
                __min(nextType, 2) + 12,
                ICON_DRAW_NORMAL,
                ICON_DRAW_OFFSET_FULL
            );
            if (nextType >= 2)
                ovIcon->DrawToBuffer(
                    spacing * i + left,
                    289,
                    nextType + 14,
                    ICON_DRAW_NORMAL,
                    ICON_DRAW_OFFSET_FULL
                );
            sprintf(gText, "%d", mineNums[nextType]);
            bigFont
                ->DrawBoundedString(gText, spacing * i + left, 355, 72, 16, 1, FONT_ALIGN_CENTER);
            nextType++;
        }
    }

    gpWindowManager->UpdateScreenRegion(0, 0, 640, 480);
    win = new heroWindow(0, 0, "overwind.bin");
    if (!win)
        MemError();
    SetWinText(win, WINDOW_TEXT_OVERVIEW);
    SET_WIDGET_MESSAGE(message, WIDGET_COMMAND_SET_TEXT, OVERVIEW_DATE);
    sprintf(gText, gOverviewText[0], m_month, m_week, m_day);
    message.text = gText;
    win->BroadcastMessage(message);
    message.id = OVERVIEW_DAILY_GOLD;
    sprintf(gText, "%d", ComputeDailyGold(giCurPlayer));
    win->BroadcastMessage(message);
    for (i = 0; i < RESOURCE_COUNT; i++) {
        sprintf(gText, "%d", gpCurPlayer->m_resources[i]);
        message.id = i + OVERVIEW_RESOURCE_BASE;
        win->BroadcastMessage(message);
    }
    gpWindowManager->AddWindow(win, WINDOW_Z_ORDER_APPEND, 1);
    win->DrawWindow();
    gText[0] = 0;
    if (m_mineOwners[MINE_SLOT_DRAGON_CITY] == giCurPlayer) {
        strcpy(gText, gOverviewText[1]);
        smallFont->DrawBoundedString(gText, 100, 450, 400, 12, 1, FONT_ALIGN_LEFT);
        gpWindowManager->UpdateScreenRegion(100, 450, 400, 12);
    }
    if (m_mineOwners[MINE_SLOT_LIGHTHOUSE] == giCurPlayer) {
        strcpy(gText, gOverviewText[2]);
        smallFont->DrawBoundedString(gText, 100, 465, 400, 12, 1, FONT_ALIGN_LEFT);
        gpWindowManager->UpdateScreenRegion(100, 465, 400, 12);
    }
    gpWindowManager->FadeScreen(WINDOW_FADE_IN, WINDOW_FADE_STEPS_SHORT, NULL);
    gpWindowManager->DoDialog(win, TrueFalseDialogHandler, 0);
    delete win;
    gpWindowManager->FadeScreen(WINDOW_FADE_OUT, WINDOW_FADE_STEPS_SHORT, NULL);
    gpResourceManager->Dispose(ovIcon);
    gpResourceManager->Dispose(smallFont);
    gpResourceManager->Dispose(bigFont);
    gOverviewShowing = 0;
}

// Buka 2.1 game::GetRandomNumTroops with HoMM1's 28 creatures.
VA(0x00432d0a, 0x25d)
i8 game::GetRandomNumTroops(i8 monsterType) {
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

VA(0x00432f67, 0x1a)
void game::TurnOnAIMusic(void) {
    StopAllAudio();
    PlayMusic(MUSIC_TRACK_AI_TURN);
}

VA(0x00432f81, 0xb)
// Retail retains only the ordinary empty member-function prologue/epilogue.
void game::TurnOffAIMusic(void) {}

// Buka 2.1 game::NextPlayer for HoMM1: autosaves, advances to the next
// living player (a new day after the last), restores hero movement (none
// on the campaign's goal town) and hands the turn to the computer or the
// human.
VA(0x00432f8c, 0x520)
void game::NextPlayer(void) {
    hero* currentHero;
    i32 numHumans;
    i32 i;
    i32 remote;
    // Retail reserves 0x14 unused bytes above the named locals.
    char unused[20];

    gCurHourGlassPhase = 0;
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
    gpAdvManager->m_identifyHeroActive = 0;
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
    giCurPlayerHighBit = 1 << (giCurPlayer + GAME_PLAYER_HIGH_BIT_SHIFT);
    for (i = 0; i < m_players[giCurPlayer].m_heroCount; i++) {
        currentHero = &m_heroRecs[m_players[giCurPlayer].m_heroIds[i]];
        currentHero->m_mobility = currentHero->CalcMobility();
        if (m_campaignType > 0
            && gCampaignScenarios[m_campaignScenario].victoryTownX == currentHero->m_x
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
        giBottomViewOverride = BOTTOM_VIEW_OVERRIDE_DISABLED;
        ShowComputerScreen();
        bShowIt = 0;
        if (gRemoteOn && (gbHumanPlayer[giCurPlayer] || giHostGamePos != giThisGamePos)) {
            if (!gbHumanPlayer[giCurPlayer])
                remote = giHostGamePos;
            else
                remote = giCurPlayer;
            if (!gpGame->TransmitSaveGame(remote, 0))
                ShutDown(NULL);
        }
        if (giBottomViewOverride == BOTTOM_VIEW_OVERRIDE_DISABLED)
            giBottomViewOverride = BOTTOM_VIEW_NONE;
    } else {
        SetNoDialogMenus(1);
        gpInputManager->Flush();
        if (gbBlackoutPlayer && giNumHumanPlayers > 1) {
            sprintf(gText, "%s player turn.", gColorNames[gpGame->m_players[giCurPlayer].m_color]);
            gText[0] -= 'a' - 'A';
            WaitForPlayer(gText, giCurPlayer);
        }
        if (gbThisNetHumanPlayer[giCurPlayer])
            CancelComputerScreen();
        giCurWatchPlayerBit = giCurPlayerBit;
        giCurWatchPlayer = giCurPlayer;
        gCurWatchPlayerHighBit = 1 << (giCurPlayer + GAME_PLAYER_HIGH_BIT_SHIFT);
    }
    DoNewTurn();
    gpMouseManager->ReallyShowPointer();
    CheckEndGame(0);
    if (gbThisNetHumanPlayer[giCurPlayer] && gRemoteOn && m_day != 1 && gForceSwitchMusic == -1) {
        PlayMusic(MUSIC_TRACK_NETWORK_TURN);
        gForceSwitchMusic = KBTickCount();
    }
    if (gbThisNetHumanPlayer[giCurPlayer])
        gpAdvManager->ForceNewHover();
}

// Buka 2.1 game::ComputeDailyGold for HoMM1: the first mine and gold mines
// pay 1000, towns 250 (castles 1000), three treasure artifacts add more,
// and computer players' gold scales with their level.
VA(0x004334ac, 0x222)
i32 game::ComputeDailyGold(i32 player) {
    i32 gold;
    i32 i;
    gold = 0;
    if (m_mines[MINE_SLOT_DRAGON_CITY].owner == player)
        gold += DAILY_GOLD_DRAGON_CITY;
    for (i = MINE_SLOT_STANDARD_FIRST; i < GAME_MINE_COUNT; i++) {
        if (m_mines[i].owner == player && m_mines[i].type == RESOURCE_GOLD)
            gold += DAILY_GOLD_MINE;
    }
    for (i = 0; i < GAME_TOWN_COUNT; i++) {
        if (m_castleRecs[i].m_owner == player) {
            if (m_castleRecs[i].m_buildings & (1 << BUILDING_SLOT_TENT))
                gold += DAILY_GOLD_TOWN;
            else
                gold += DAILY_GOLD_CASTLE;
        }
    }
    gold += m_players[player].NumOfGivenArtifact(ARTIFACT_ENDLESS_SACK_OF_GOLD)
            * DAILY_GOLD_ENDLESS_SACK;
    gold +=
        m_players[player].NumOfGivenArtifact(ARTIFACT_ENDLESS_BAG_OF_GOLD) * DAILY_GOLD_ENDLESS_BAG;
    gold += m_players[player].NumOfGivenArtifact(ARTIFACT_ENDLESS_PURSE_OF_GOLD)
            * DAILY_GOLD_ENDLESS_PURSE;
    if (!gbHumanPlayer[player]) {
        if (gpGame->m_players[player].m_difficulty == PLAYER_TYPE_DUMB)
            gold = gold * 0.75;
        if (gpGame->m_players[player].m_difficulty == PLAYER_TYPE_AVERAGE) {
        }
        if (gpGame->m_players[player].m_difficulty == PLAYER_TYPE_SMART)
            gold = gold * 1.29;
        if (gpGame->m_players[player].m_difficulty == PLAYER_TYPE_GENIUS)
            gold = gold * 1.45;
    }
    return gold;
}

// Buka 2.1 game::PerDay for HoMM1: records each player's income, pays the
// mines, towns and computer bonuses, then advances the calendar.
VA(0x004336ce, 0x4ce)
void game::PerDay(void) {
    i16 i;
    i16 production;
    // Retail reserves one more unused slot between the counters.
    i16 k;
    i16 j;
    i8 resource;

    for (i = 0; i < gpGame->m_playerCount; i++) {
        for (j = 0; j < RESOURCE_COUNT; j++)
            gpGame->m_players[i].m_aiData.m_income[j] = -m_players[i].m_resources[j];
    }
    memset(m_townBuiltToday, 0, sizeof(m_townBuiltToday));
    for (i = MINE_SLOT_STANDARD_FIRST; i < GAME_MINE_COUNT; i++) {
        if (m_mines[i].owner != GAME_PLAYER_NONE) {
            resource = m_mines[i].type;
            production = 0;
            if (resource == RESOURCE_ORE)
                production = DAILY_MINE_YIELD_WOOD_ORE;
            else if (resource == RESOURCE_WOOD)
                production = DAILY_MINE_YIELD_WOOD_ORE;
            else if (resource != RESOURCE_GOLD)
                production = DAILY_MINE_YIELD_OTHER;
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
            if (gpGame->m_players[i].m_difficulty > PLAYER_TYPE_NO_WOOD_ORE_BONUS_LAST) {
                m_players[i].m_resources[RESOURCE_WOOD]++;
                m_players[i].m_resources[RESOURCE_ORE]++;
            }
            if (gpGame->m_players[i].m_difficulty > PLAYER_TYPE_NO_WEEKDAY_BONUS_LAST && m_day >= 1
                && m_day <= RESOURCE_NON_GOLD_END)
                m_players[i].m_resources[m_day - 1]++;
        }
    }
    m_day++;
    giCurTurn =
        (m_month - 1) * CALENDAR_DAYS_PER_MONTH + (m_week - 1) * CALENDAR_DAYS_PER_WEEK + m_day;
    if (m_day > CALENDAR_DAYS_PER_WEEK) {
        m_day = 1;
        PerWeek();
    }
    if (m_week > CALENDAR_WEEKS_PER_MONTH) {
        m_week = 1;
        PerMonth();
    }
    for (i = 0; i < gpGame->m_playerCount; i++) {
        for (j = 0; j < RESOURCE_COUNT; j++)
            gpGame->m_players[i].m_aiData.m_income[j] += m_players[i].m_resources[j];
    }
}

// Calendar draws: gWeekNames / gMonthNames sizes, the creature tables a
// creature week or month picks from, and gNewTurnText's announcement rows.
H1_ENUM_CONST_BEGIN(CalendarConstant)
    CALENDAR_WEEK_NAME_COUNT = 15,
    CALENDAR_WEEK_CREATURE_COUNT = 24,
    CALENDAR_MONTH_NAME_COUNT = 10,
    CALENDAR_MONTH_CREATURE_COUNT = 12,
    NEW_TURN_TEXT_DAYS_LEFT = 0,
    NEW_TURN_TEXT_LAST_DAY = 1,
    NEW_TURN_TEXT_MONTH_NORMAL = 2,
    NEW_TURN_TEXT_MONTH_CREATURE = 3,
    NEW_TURN_TEXT_MONTH_PLAGUE = 4,
    NEW_TURN_TEXT_WEEK_NORMAL = 5,
    NEW_TURN_TEXT_WEEK_CREATURE = 6
H1_ENUM_CONST_END(CalendarConstant)

// Buka 2.1 game::PerWeek for HoMM1: rolls the week, grows every dwelling
// (computer towns grow faster), refreshes the tavern heroes and restocks the
// map's renewable sites.
VA(0x00433b9c, 0x7c2)
void game::PerWeek(void) {
    i16 posY;
    i16 posX;
    i16 gain;
    town* townPointer;
    i16 j;
    i16 i;
    i32 heroClass = 0;

    giWeekType = CALENDAR_PERIOD_NORMAL;
    giWeekTypeExtra = Random(0, CALENDAR_WEEK_NAME_COUNT - 1);
    if (m_week != CALENDAR_WEEKS_PER_MONTH) {
        i = Random(1, 4);
        if (i == 1) {
            giWeekType = CALENDAR_PERIOD_CREATURE;
            giWeekTypeExtra = Random(0, CALENDAR_WEEK_CREATURE_COUNT - 1);
        }
    }
    for (i = 0; i < GAME_TOWN_COUNT; i++) {
        townPointer = GetTown(i);
        for (j = BUILDING_SLOT_DWELLING_FIRST; j <= BUILDING_SLOT_DWELLING_LAST; j++) {
            if (townPointer->m_buildings & (1 << j)) {
                gain = gMonsterDatabase[gDwellingType[townPointer->m_type]
                                                     [j - BUILDING_SLOT_DWELLING_FIRST]]
                           .growth;
                if (townPointer->m_buildings & (1 << BUILDING_SLOT_WELL))
                    gain += WEEKLY_WELL_GROWTH_BONUS;
                if (townPointer->m_owner >= 0 && !gbHumanPlayer[townPointer->m_owner]) {
                    if (gpGame->m_players[townPointer->m_owner].m_difficulty == PLAYER_TYPE_SMART)
                        gain = gain * 1.24;
                    if (gpGame->m_players[townPointer->m_owner].m_difficulty == PLAYER_TYPE_GENIUS)
                        gain = gain * 1.36;
                }
                if (giWeekType == CALENDAR_PERIOD_CREATURE
                    && gDwellingType[townPointer->m_type][j - BUILDING_SLOT_DWELLING_FIRST]
                           == giWeekTypeExtra)
                    gain += WEEKLY_CREATURE_GROWTH_BONUS;
                townPointer->m_garrison[j - BUILDING_SLOT_DWELLING_FIRST] += gain;
            }
        }
    }
    for (i = 0; i < GAME_PLAYER_COUNT; i++) {
        for (j = 0; j < PLAYER_TAVERN_HERO_COUNT; j++) {
            heroClass = (Random(1, 3) + heroClass) % HERO_CLASS_COUNT;
            if (gpGame->m_availableHeroes[gpGame->m_players[i].m_availableHeroIds[j]]
                == HERO_AVAILABILITY_RETREATED)
                gpGame->m_availableHeroes[gpGame->m_players[i].m_availableHeroIds[j]] =
                    HERO_AVAILABILITY_UNAVAILABLE;
            gpGame->m_players[i].m_availableHeroIds[j] = gpGame->GetNewHeroId(heroClass);
        }
    }
    for (posY = 0; posY < MAP_CELL_GRID_SIZE; posY++) {
        for (posX = 0; posX < MAP_CELL_GRID_SIZE; posX++) {
            switch (m_map[posX][posY].m_triggerType) {
                case MAP_TRIGGER_EVENT | MAP_OBJECT_WATERWHEEL:
                    if (static_cast<u8>(m_map[posX][posY].m_objectMetadata)
                        != WEEKLY_WATER_WHEEL_EMPTY)
                        m_map[posX][posY].m_objectMetadata = WEEKLY_WATER_WHEEL_GOLD;
                    break;
                case MAP_TRIGGER_EVENT | MAP_OBJECT_WINDMILL:
                    m_map[posX][posY].m_objectMetadata = Random(1, 5);
                    break;
                case MAP_TRIGGER_EVENT | MAP_OBJECT_STRAW_HUT:
                    if (static_cast<u8>(m_map[posX][posY].m_objectMetadata)
                        < WEEKLY_SITE_STOCK_LIMIT)
                        m_map[posX][posY].m_objectMetadata =
                            static_cast<u8>(m_map[posX][posY].m_objectMetadata) + Random(3, 6);
                    break;
                case MAP_TRIGGER_EVENT | MAP_OBJECT_HOUSE:
                    if (static_cast<u8>(m_map[posX][posY].m_objectMetadata)
                        < WEEKLY_SITE_STOCK_LIMIT)
                        m_map[posX][posY].m_objectMetadata =
                            static_cast<u8>(m_map[posX][posY].m_objectMetadata) + Random(5, 10);
                    break;
                case MAP_TRIGGER_EVENT | MAP_OBJECT_CABIN:
                    if (static_cast<u8>(m_map[posX][posY].m_objectMetadata)
                        < WEEKLY_SITE_STOCK_LIMIT)
                        m_map[posX][posY].m_objectMetadata =
                            static_cast<u8>(m_map[posX][posY].m_objectMetadata) + Random(2, 4);
                    break;
                case MAP_TRIGGER_EVENT | MAP_OBJECT_DWARF_LOG_CABIN:
                    if (static_cast<u8>(m_map[posX][posY].m_objectMetadata)
                        < WEEKLY_SITE_STOCK_LIMIT)
                        m_map[posX][posY].m_objectMetadata =
                            static_cast<u8>(m_map[posX][posY].m_objectMetadata) + Random(2, 4);
                    break;
                case MAP_TRIGGER_EVENT | MAP_OBJECT_PEASANT_LOG_CABIN:
                    if (static_cast<u8>(m_map[posX][posY].m_objectMetadata)
                        < WEEKLY_SITE_STOCK_LIMIT)
                        m_map[posX][posY].m_objectMetadata =
                            static_cast<u8>(m_map[posX][posY].m_objectMetadata) + Random(5, 10);
                    break;
                case MAP_TRIGGER_EVENT | MAP_OBJECT_DESERT_TENT:
                    if (static_cast<u8>(m_map[posX][posY].m_objectMetadata)
                        < WEEKLY_SITE_STOCK_LIMIT)
                        m_map[posX][posY].m_objectMetadata =
                            static_cast<u8>(m_map[posX][posY].m_objectMetadata) + Random(1, 3);
                    break;
                case MAP_TRIGGER_EVENT | MAP_OBJECT_WAGON_CAMP:
                    if (static_cast<u8>(m_map[posX][posY].m_objectMetadata)
                        < WEEKLY_SITE_STOCK_LIMIT)
                        m_map[posX][posY].m_objectMetadata =
                            static_cast<u8>(m_map[posX][posY].m_objectMetadata) + Random(3, 6);
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
VA(0x0043435e, 0x2c0)
void game::PerMonth(void) {
    DATA(0x00490ce0)
    static i8 gMonType[12] = {0, 6, 13, 14, 9, 15, 7, 8, 18, 19, 16, 20};
    town* townPointer;
    i16 growth;
    i16 j;
    i16 i;
    i32 y;
    i32 x;
    mapCell* spot;

    m_month++;
    i = Random(1, 10);
    if (i <= 5) {
        giMonthType = CALENDAR_PERIOD_NORMAL;
        giMonthTypeExtra = Random(0, CALENDAR_MONTH_NAME_COUNT - 1);
    } else if (i <= 9) {
        giMonthType = CALENDAR_PERIOD_CREATURE;
        giMonthTypeExtra = gMonType[Random(0, CALENDAR_MONTH_CREATURE_COUNT - 1)];
    } else {
        giMonthType = CALENDAR_PERIOD_PLAGUE;
    }
    for (i = 0; i < GAME_TOWN_COUNT; i++) {
        for (j = BUILDING_SLOT_DWELLING_FIRST; j <= BUILDING_SLOT_DWELLING_LAST; j++) {
            townPointer = GetTown(i);
            if (townPointer->m_buildings & (1 << j)) {
                growth = gMonsterDatabase[gDwellingType[townPointer->m_type]
                                                       [j - BUILDING_SLOT_DWELLING_FIRST]]
                             .growth;
                if (townPointer->m_buildings & (1 << BUILDING_SLOT_WELL))
                    growth += WEEKLY_WELL_GROWTH_BONUS;
                if (giMonthType == CALENDAR_PERIOD_CREATURE
                    && gDwellingType[townPointer->m_type][j - BUILDING_SLOT_DWELLING_FIRST]
                           == giMonthTypeExtra)
                    townPointer->m_garrison[j - BUILDING_SLOT_DWELLING_FIRST] *=
                        MONTHLY_CREATURE_GROWTH_FACTOR;
                if (giMonthType == CALENDAR_PERIOD_PLAGUE) {
                    townPointer->m_garrison[j - BUILDING_SLOT_DWELLING_FIRST] -= growth;
                    if (townPointer->m_garrison[j - BUILDING_SLOT_DWELLING_FIRST] < 0)
                        townPointer->m_garrison[j - BUILDING_SLOT_DWELLING_FIRST] = 0;
                    townPointer->m_garrison[j - BUILDING_SLOT_DWELLING_FIRST] =
                        townPointer->m_garrison[j - BUILDING_SLOT_DWELLING_FIRST] >> 1;
                }
            }
        }
    }
    if (giMonthType == CALENDAR_PERIOD_CREATURE) {
        for (x = 0; x < MAP_CELL_GRID_SIZE; x++) {
            for (y = 0; y < MAP_CELL_GRID_SIZE; y++) {
                spot = gpAdvManager->GetCell(x, y);
                if (!spot->m_triggerType && CELL_TERRAIN(spot)) {
                    if (Random(0, 360) == 10) {
                        spot->m_triggerType = (MAP_TRIGGER_EVENT | MAP_OBJECT_MONSTER);
                        spot->m_objectTileset = TILESET_MONS32;
                        spot->m_objectIndex = giMonthTypeExtra;
                        spot->m_objectMetadata = GetRandomNumTroops(giMonthTypeExtra);
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
VA(0x0043461e, 0x569)
void game::RandomizeTown(i8 x, i8 y, i8 isCastle) {
    i8 j;
    i8 unique;
    town* town;
    i8 i;
    u8 frameShift;
    i8 townNum;
    i8 race;
    i8 plain;

    townNum = GetTownId(x, y);
    for (j = 0; j < TOWN_FOOTPRINT_HEIGHT; j++) {
        for (i = 0; i < TOWN_FOOTPRINT_WIDTH; i++) {
            if ((m_map[x - TOWN_FOOTPRINT_LEFT + i][y - TOWN_FOOTPRINT_TOP + j].m_triggerType
                 & MAP_TRIGGER_TYPE_MASK)
                    > 0
                && (m_map[x - TOWN_FOOTPRINT_LEFT + i][y - TOWN_FOOTPRINT_TOP + j].m_triggerType
                    & MAP_TRIGGER_TYPE_MASK)
                       <= MAP_OBJECT_EVENT_LAST) {
                m_map[x - TOWN_FOOTPRINT_LEFT + i][y - TOWN_FOOTPRINT_TOP + j].m_secondaryTrigger |=
                    MAP_OBJECT_TOWN;
            } else {
                m_map[x - TOWN_FOOTPRINT_LEFT + i][y - TOWN_FOOTPRINT_TOP + j].m_triggerType =
                    MAP_OBJECT_TOWN;
                m_map[x - TOWN_FOOTPRINT_LEFT + i][y - TOWN_FOOTPRINT_TOP + j].m_objectMetadata =
                    townNum;
            }
        }
    }
    m_map[x][y].m_triggerType |= MAP_TRIGGER_EVENT;
    town = GetTown(townNum);
    town->m_turnsOwned = TOWN_RANDOM_AGE;
    if (m_campaignType > 0 && m_campaignScenario >= CAMPAIGN_SCENARIO_LORD_FIRST
        && m_campaignScenario <= CAMPAIGN_SCENARIO_LORD_LAST && town->m_owner == 0) {
        race = gCrestTownTypes[m_players[0].m_color];
    } else if (townNum < GAME_PLAYER_COUNT) {
        unique = 0;
        race = 0;
        while (!unique) {
            race = Random(0, 3);
            unique = 1;
            for (i = 0; i < GAME_PLAYER_COUNT; i++) {
                if (gRandomTownTypes[i] == race)
                    unique = 0;
            }
        }
        gRandomTownTypes[townNum] = race;
    } else {
        race = Random(0, 3);
    }
    frameShift = (TOWN_TYPE_COUNT - race) * TOWN_RACE_FRAME_STRIDE;
    for (i = 0; i < TOWN_FOOTPRINT_WIDTH; i++) {
        m_map[x - TOWN_FOOTPRINT_LEFT + i][y - TOWN_FOOTPRINT_TOP].m_overlayIndex -= frameShift;
        m_map[x - TOWN_FOOTPRINT_LEFT + i][y - 1].m_objectIndex -= frameShift;
        m_map[x - TOWN_FOOTPRINT_LEFT + i][y].m_objectIndex -= frameShift;
    }
    m_castleRecs[townNum].m_type = race;
    plain = 1;
    if (town->m_extraIndex >= 1
        && static_cast<mapTownExtra*>(ppMapExtra[town->m_extraIndex])->customized)
        plain = 0;
    if (plain) {
        if (race == TOWN_TYPE_BARBARIAN)
            m_castleRecs[townNum].m_buildings = 1 << BUILDING_SLOT_SPECIAL;
        else
            m_castleRecs[townNum].m_buildings = 0;
    }
    if (isCastle) {
        m_castleRecs[townNum].m_buildings |=
            ((1 << BUILDING_SLOT_CASTLE) | (1 << BUILDING_SLOT_DWELLING_1));
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
VA(0x00434b87, 0x318)
void game::SetupTown(i8 townId, i8 aiOwned) {
    i16 dwellingCount;
    char dwellingRoll[10];
    i32 k;
    i8 used[29];
    i8 townType;
    i16 newSpell;
    i16 spellValue;
    i32 spellLevel;

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
        for (k = 0; k < BUILDING_SLOT_DWELLING_COUNT; k++) {
            if (m_castleRecs[townId].m_buildings & (1 << (k + BUILDING_SLOT_DWELLING_FIRST)))
                m_castleRecs[townId].m_garrison[k] =
                    gMonsterDatabase[gDwellingType[townType][k]].growth;
        }
    }
    if (!m_castleRecs[townId].m_customized) {
        m_castleRecs[townId].m_buildings |= (1 << BUILDING_SLOT_DWELLING_1);
        m_castleRecs[townId].m_garrison[0] = gMonsterDatabase[gDwellingType[townType][0]].growth;
        if (aiOwned && dwellingCount == 1 && Random(1, 10) < 4)
            dwellingCount++;
        if (--dwellingCount) {
            m_castleRecs[townId].m_buildings |= (1 << BUILDING_SLOT_DWELLING_2);
            m_castleRecs[townId].m_garrison[1] =
                gMonsterDatabase[gDwellingType[townType][1]].growth;
            dwellingCount--;
        }
    }
    memset(used, 0, 29);
    for (k = 0; k < TOWN_MAGE_GUILD_SPELL_COUNT; k++) {
        if (k <= MAGE_GUILD_LEVEL_1_LAST_SLOT)
            spellLevel = MAGE_GUILD_STATE_LEVEL_1;
        else if (k <= MAGE_GUILD_LEVEL_2_LAST_SLOT)
            spellLevel = MAGE_GUILD_STATE_LEVEL_2;
        else if (k <= MAGE_GUILD_LEVEL_3_LAST_SLOT)
            spellLevel = MAGE_GUILD_STATE_LEVEL_3;
        else
            spellLevel = MAGE_GUILD_STATE_LEVEL_4;
        do {
            newSpell = gMageGuildSpellPool[spellLevel][Random(0, 7)];
            if (aiOwned)
                spellValue =
                    gSpellAIValue[newSpell]
                        * (gSpellAIFlags[newSpell] & SPELL_AI_FLAG_SCALES_WITH_POWER ? 4 : 1)
                    + 50;
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
VA(0x00434e9f, 0x586)
void game::RandomizeMine(i8 x, i8 y) {
    u8 bits;
    u8 upFrame;
    i8 k;
    i32 tries;
    i8 j;
    i8 terrain;
    i8 type;
    i8 mineIdx;
    u8 objFrame;

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
        if (!gMineTypeCount[type])
            tries = 30;
    }
    gMineTypeCount[type]++;
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
        m_map[x + 1][y].m_flags |= MAP_CELL_OBJECT_ANIMATED;
        bits = MAP_OBJECT_ALCHEMIST_LAB;
    } else if (type == RESOURCE_WOOD) {
        bits = MAP_OBJECT_SAWMILL;
    } else {
        m_map[x + 1][y].m_flags |= MAP_CELL_OBJECT_EXTRA;
        m_map[x + 1][y].m_objectTileset |= TILESET_RSRC32 << MAP_CELL_EXTRA_TILESET_SHIFT;
        m_map[x + 1][y].m_extraFrame = type - RESOURCE_ORE;
        bits = MAP_OBJECT_MINE;
    }
    mineIdx = GetMineId(x, y);
    for (k = 0; k < MINE_FOOTPRINT_HEIGHT; k++) {
        for (j = 0; j < MINE_FOOTPRINT_WIDTH; j++) {
            if ((m_map[x + j][y - k].m_triggerType & MAP_TRIGGER_TYPE_MASK) > 0
                && (m_map[x + j][y - k].m_triggerType & MAP_TRIGGER_TYPE_MASK)
                       <= MAP_OBJECT_EVENT_LAST) {
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
VA(0x00435425, 0x5e)
i8 game::GetRandomArtifactId(void) {
    i8 freeSlot = Scan(
        m_randomArtifacts,
        ARTIFACT_REGULAR_FIRST,
        ARTIFACT_REGULAR_END - ARTIFACT_REGULAR_FIRST
    );
    if (freeSlot == GAME_TABLE_FREE)
        return ARTIFACT_NONE;
    i8 artifact = RandomScan(
        m_randomArtifacts,
        ARTIFACT_REGULAR_FIRST,
        ARTIFACT_REGULAR_END - ARTIFACT_REGULAR_FIRST,
        ARTIFACT_REGULAR_END
    );
    if (artifact == ARTIFACT_NONE)
        return freeSlot;
    else
        return artifact;
}

// Buka 2.1 game::RandomizeHeroPool without HoMM2's starting spells.
VA(0x00435483, 0xe3)
void game::RandomizeHeroPool(void) {
    i16 heroId;
    for (heroId = 0; heroId < GAME_HERO_COUNT; heroId++) {
        m_heroRecs[heroId].m_experience = Random(0, 50) + RANDOM_HERO_EXPERIENCE_BASE;
        SetRandomHeroArmies(heroId, RANDOM_HERO_NORMAL_ARMY);
        m_heroRecs[heroId].m_remainingMobility = m_heroRecs[heroId].CalcMobility();
        m_heroRecs[heroId].m_mobility = m_heroRecs[heroId].m_remainingMobility;
        m_heroRecs[heroId].m_randomSeed = Random(1, 16000);
    }
}

// Buka 2.1 game::SetRandomHeroArmies: HoMM1 has four classes and draws
// only from the first two stacks of each class table.
VA(0x00435566, 0x28f)
void game::SetRandomHeroArmies(i16 heroId, i32 strongArmy) {
    armyGroup* army = &m_heroRecs[heroId].m_army;
    i16 slot = 0;
    i16 armyTable[HERO_CLASS_COUNT][RANDOM_HERO_ARMY_OPTION_COUNT][RANDOM_HERO_ARMY_FIELD_COUNT] = {
        {{CREATURE_PEASANT, 30, 50}, {CREATURE_ARCHER, 3, 5}, {CREATURE_PIKEMAN, 2, 4}},
        {{CREATURE_GOBLIN, 15, 25}, {CREATURE_ORC, 3, 5}, {CREATURE_WOLF, 2, 3}},
        {{CREATURE_SPRITE, 10, 20}, {CREATURE_DWARF, 2, 4}, {CREATURE_ELF, 1, 2}},
        {{CREATURE_CENTAUR, 6, 10}, {CREATURE_GARGOYLE, 2, 4}, {CREATURE_GRIFFIN, 1, 2}}
    };
    i32 present[RANDOM_HERO_ARMY_OPTION_COUNT];
    i32 i;
    i32 max;
    i32 minNum;

    present[0] = 1;
    present[1] = Random(0, 99) < RANDOM_HERO_FIRST_STACK_CHANCE
                                     + (strongArmy ? RANDOM_HERO_FIRST_STACK_BONUS_CHANCE : 0);
    present[2] = Random(0, 99) < RANDOM_HERO_SECOND_STACK_CHANCE
                                     + (strongArmy ? RANDOM_HERO_SECOND_STACK_BONUS_CHANCE : 0);
    if (!present[2])
        present[1] = 1;
    for (i = 0; i < ARMY_GROUP_SLOT_COUNT; i++) {
        army->m_creatureTypes[i] = CREATURE_NONE;
        army->m_creatureCounts[i] = RANDOM_HERO_EMPTY_COUNT;
    }
    for (i = 0; i < RANDOM_HERO_ARMY_SELECTION_COUNT; i++) {
        if (present[i]) {
            army->m_creatureTypes[slot] =
                armyTable[m_heroRecs[heroId].m_heroClass][i][RANDOM_HERO_ARMY_FIELD_CREATURE];
            minNum = armyTable[m_heroRecs[heroId].m_heroClass][i][RANDOM_HERO_ARMY_FIELD_MIN]
                     * RANDOM_HERO_COUNT_SCALE;
            max = armyTable[m_heroRecs[heroId].m_heroClass][i][RANDOM_HERO_ARMY_FIELD_MAX]
                      * RANDOM_HERO_COUNT_SCALE
                  + RANDOM_HERO_COUNT_ROUNDING;
            if (strongArmy)
                minNum = (minNum + max) / 2;
            army->m_creatureCounts[slot] = Random(minNum, max) / RANDOM_HERO_COUNT_SCALE;
            slot++;
        }
    }
}

// Buka 2.1 game::ProcessRandomObjects for HoMM1's random towns, castles,
// monsters by strength band, resources, artifacts and mines; NewMap runs
// the castles-only pass first.
VA(0x004357f5, 0x27f)
void game::ProcessRandomObjects(i32 castlesOnly) {
    mapCell* cellPtr;
    i32 lowFV;
    i32 y;
    i32 x;
    i32 i;
    i32 highFV;

    for (i = 0; i < RESOURCE_COUNT; i++)
        gMineTypeCount[i] = 0;
    for (i = 0; i < GAME_PLAYER_COUNT; i++)
        gRandomTownTypes[i] = TOWN_TYPE_NONE;
    for (y = 0; y < MAP_CELL_GRID_SIZE; y++) {
        for (x = 0; x < MAP_CELL_GRID_SIZE; x++) {
            cellPtr = &m_map[x][y];
            if (!castlesOnly
                || cellPtr->m_triggerType == (MAP_TRIGGER_EVENT | MAP_FILE_OBJECT_RANDOM_CASTLE)) {
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
                        m_randomArtifacts[cellPtr->m_objectIndex] = GAME_ARTIFACT_ON_MAP;
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
VA(0x00435a74, 0x22f)
void game::SetVisibility(i16 x, i16 y, i16 player, i16 radius) {
    i32 i;
    i32 j;
    i32 rangeLeft;
    i32 cutoff;
    u8 viewMask = 1 << player;
    u8 outerMask = 1 << (player + GAME_PLAYER_HIGH_BIT_SHIFT);

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
VA(0x00435ca3, 0xc8)
void game::GiveArmy(armyGroup* group, i32 type, i32 count, i32 slot) {
    i32 swap;
    i32 i;
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
VA(0x00435d6b, 0x7a)
i32 game::ExperienceValueOfStack(armyGroup* group, hero* h) {
    i32 expValue = 0;
    i32 i;
    for (i = 0; i < ARMY_GROUP_SLOT_COUNT; ++i) {
        if (group->m_creatureCounts[i] > 0)
            expValue +=
                group->m_creatureCounts[i] * gMonsterDatabase[group->m_creatureTypes[i]].hitPoints;
    }
    if (h)
        expValue += 500;
    return expValue;
}

// Buka 2.1 MiscRuntime seeded generator; HoMM1 keeps it in this TU.
VA(0x00435de5, 0x92)
i32 SGenRand(void) {
    i32 bitMask;
    i32 value = 0;
    i32 i;
    gLastSeed &= 0xfff;
    gLastSeed *= 7;
    gLastSeed += (gLastSeed & 0xff0) >> 4;
    for (i = 31; i >= 0; --i) {
        bitMask = 1 << i;
        if (gLastSeed & bitMask)
            value |= 1 << i;
    }
    return value;
}

VA(0x00435e77, 0x55)
i32 SRandom(i32 low, i32 high) {
    i32 result;
    SIncRandomize(low, high);
    result = SGenRand();
    gLastSeed += low;
    gLastSeed += high * 8;
    return result % (high - low + 1) + low;
}

VA(0x00435ecc, 0x8a)
void SIncRandomize(i32 x, i32 y) {
    i32 feedback;
    x *= 13;
    y *= 13;
    x &= 0xff;
    y &= 0xff;
    gLastSeed += y << 5;
    gLastSeed += x * 13233;
    gLastSeed += y;
    feedback = gLastSeed & 0x3f;
    gLastSeed += feedback << 8;
}

VA(0x00435f56, 0x19)
void SRand(i32 seed) {
    gLastSeed = seed;
    srand(seed);
}

// donor PoL RVA 0x00080ff9; preferred Buka symbol ?GetLuck@game@@QAEHPAVhero@@PAVarmy@@PAVtown@@@Z
// donor Buka TU SOURCE/GAME; HoMM1 owner inferred from contiguous order
// evidence: graph:3;base=0.340271;margin=0.529148;shape=0.188;size=0.654;calls=0.667;alternate=pol20:int game::GetLuck(class hero *, class army *, class town *)@0x00080ff9
VA(0x00435f6f, 0xb7)
i32 game::GetLuck(hero* h, army*) {
    i32 luck;

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
    if (luck < GAME_LUCK_MIN)
        luck = GAME_LUCK_MIN;
    if (luck > GAME_LUCK_MAX)
        luck = GAME_LUCK_MAX;
    return luck;
}

// Buka 2.1 keeps the scan cursor in file statics.
DATA(0x004a7314)
static i32 s_adjacentMonsterEndX;
DATA(0x004a7310)
static i32 s_adjacentMonsterEndY;
DATA(0x004a72dc)
static i32 s_adjacentMonsterX;
DATA(0x004a72e0)
static i32 s_adjacentMonsterY;
DATA(0x004a72f0)
static i32 s_adjacentMonsterMinX;
DATA(0x004a72f4)
static i32 s_adjacentMonsterMinY;

// donor PoL RVA 0x00069bef; preferred Buka symbol ?FindAdjacentMonster@advManager@@QAEHHHPAH0HH@Z
// donor Buka TU SOURCE/ADVMGR; HoMM1 owner inferred from contiguous order
// evidence: graph:3;base=0.502628;margin=0.574839;shape=0.364;size=0.953;calls=0.667;alternate=pol20:int advManager::FindAdjacentMonster(int, int, int *, int *, int, int)@0x00069bef
// HoMM1 retail returns the found flag in AL (xor al,al / mov al,1).
VA(0x00436026, 0x2fa)
i8 advManager::FindAdjacentMonster(
    i32 originX,
    i32 originY,
    i32* monsterX,
    i32* monsterY,
    i32 excludedX,
    i32 excludedY
) {
    s_adjacentMonsterEndX = originX + 2;
    s_adjacentMonsterEndY = originY + 2;

    if (originX > 0 && originY > 0 && originX < MAP_CELL_GRID_SIZE - 1
        && originY < MAP_CELL_GRID_SIZE - 1) {
        for (s_adjacentMonsterX = originX - 1; s_adjacentMonsterX < s_adjacentMonsterEndX;
             ++s_adjacentMonsterX) {
            for (s_adjacentMonsterY = originY - 1; s_adjacentMonsterY < s_adjacentMonsterEndY;
                 ++s_adjacentMonsterY) {
                if (m_mapData[s_adjacentMonsterX][s_adjacentMonsterY].m_triggerType
                    == (MAP_TRIGGER_EVENT | MAP_OBJECT_MONSTER)) {
                    if (s_adjacentMonsterY < originY) {
                        if ((GetCell(originX, originY)->m_objectIndex == MAP_CELL_NO_FRAME
                             || (GetCell(originX, originY)->m_flags & MAP_CELL_OBJECT_SHADOW_ONLY))
                            && (s_adjacentMonsterX != excludedX || s_adjacentMonsterY != excludedY))
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
                if (m_mapData[s_adjacentMonsterX][s_adjacentMonsterY].m_triggerType
                    == (MAP_TRIGGER_EVENT | MAP_OBJECT_MONSTER)) {
                    if (s_adjacentMonsterY < originY) {
                        if ((GetCell(originX, originY)->m_objectIndex == MAP_CELL_NO_FRAME
                             || (GetCell(originX, originY)->m_flags & MAP_CELL_OBJECT_SHADOW_ONLY))
                            && (s_adjacentMonsterX != excludedX || s_adjacentMonsterY != excludedY))
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
VA(0x00436320, 0xbc)
void game::SetupAdjacentMons(void) {
    i32 x;
    i32 y;
    i32 mask = 0x7f;
    i32 monY;
    i32 monX;

    for (x = 0; x < MAP_CELL_GRID_SIZE; ++x) {
        for (y = 0; y < MAP_CELL_GRID_SIZE; ++y) {
            if (gpAdvManager->FindAdjacentMonster(x, y, &monX, &monY, -1, -1))
                mapExtra[x][y] |= MAP_EXTRA_MONSTER_ADJACENT;
            else
                mapExtra[x][y] &= mask;
        }
    }
}

// donor PoL RVA 0x00081210; preferred Buka symbol ?CancelComputerScreen@game@@QAEXXZ
// donor Buka TU SOURCE/GAME; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.526467;margin=0.434153;shape=0.371;size=0.866;calls=1.000;alternate=pol20:void game::CancelComputerScreen(void)@0x00081210
VA(0x004363dc, 0x55)
void game::CancelComputerScreen(void) {
    TurnOffAIMusic();
    bShowIt = 1;
    i32 i;
    for (i = ADVMGR_PANEL_BUTTON_FIRST; i <= ADVMGR_PANEL_BUTTON_LAST; ++i)
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
VA(0x00436431, 0x105)
void game::ShowComputerScreen(void) {
    if (gConfig.blackoutComputer || gRemoteOn) {
        i32 saved = gbThisNetHumanPlayer[giCurPlayer];
        gbThisNetHumanPlayer[giCurPlayer] = 1;
        i32 i;
        for (i = ADVMGR_PANEL_BUTTON_FIRST; i <= ADVMGR_PANEL_BUTTON_LAST; ++i)
            gpWindowManager->BroadcastMessage(
                MESSAGE_WIDGET,
                WIDGET_COMMAND_SET_FLAGS,
                i,
                WIDGET_FLAG_UPDATE | WIDGET_FLAG_DIMMED
            );
        gpMouseManager->ReallyHidePointer();
        gAllBlack = 1;
        gpAdvManager->CompleteDraw(1);
        gpAdvManager->UpdateHeroLocators(1, 1);
        gpAdvManager->UpdateTownLocators(1, 1);
        gpAdvManager->UpdBottomView(1, 1, 1);
        gpAdvManager->UpdateScreen(0, 1);
        gAllBlack = 0;
        gbThisNetHumanPlayer[giCurPlayer] = saved;
        gpMouseManager->ReallyShowPointer();
    }
    ShowHeroesLogo();
}

// Buka 2.1 game::ShowHeroesLogo; HoMM1 draws the logo from a tileset.
VA(0x00436536, 0x9c)
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
VA(0x004365d2, 0x124)
void game::WaitForPlayer(char* text, i32 player) {
    if (gbBlackoutPlayer && giNumHumanPlayers > 1 && !gRemoteOn) {
        gpMouseManager->SetPointer(ADVENTURE_POINTER_DEFAULT);
        gAllBlack = 1;
        giBottomViewOverrideEndTime = KBTickCount() + 9999999;
        if (gbThisNetHumanPlayer[giCurPlayer])
            giBottomViewOverride = BOTTOM_VIEW_NEW_TURN;
        else
            giBottomViewOverride = BOTTOM_VIEW_NONE;
        PlayMusic(MUSIC_TRACK_NETWORK_TURN);
        gpMouseManager->ReallyHidePointer();
        gpAdvManager->CompleteDraw(1);
        gpAdvManager->UpdateHeroLocators(1, 1);
        gpAdvManager->UpdateTownLocators(1, 1);
        gpAdvManager->UpdateScreen(0, 1);
        ShowHeroesLogo();
        gAllBlack = 0;
        gpMouseManager->ReallyShowPointer();
        NormalDialog(
            text,
            NORMAL_DIALOG_TYPE_OK,
            0x61,
            -1,
            NORMAL_DIALOG_CREST,
            gpGame->m_players[player].m_color,
            NORMAL_DIALOG_NO_RESOURCE,
            0,
            NORMAL_DIALOG_NO_OR_TEXT
        );
        StopMusic();
    }
}

// Ground tiles come in groups of four interchangeable variants;
// RandomizeTerrainTiles rerolls the variant within its group.
H1_ENUM_CONST_BEGIN(TerrainTileConstant)
    TERRAIN_TILE_VARIANT_COUNT = 4
H1_ENUM_CONST_END(TerrainTileConstant)

// HoMM1 rerolls the variant within each four-tile group, past the first
// four tiles of every twenty-tile terrain block.
VA(0x004366f6, 0xa0)
void game::RandomizeTerrainTiles(void) {
    mapCell* cellPtr;
    // Retail reserves an unused slot above the loop counters.
    i32 tile;
    i32 x;
    i32 y;
    for (y = 0; y < MAP_CELL_GRID_SIZE; y++) {
        for (x = 0; x < MAP_CELL_GRID_SIZE; x++) {
            cellPtr = &m_map[x][y];
            if (cellPtr->m_tileIndex % MAP_CELL_TILES_PER_TERRAIN >= TERRAIN_TILE_VARIANT_COUNT)
                cellPtr->m_tileIndex =
                    cellPtr->m_tileIndex / TERRAIN_TILE_VARIANT_COUNT * TERRAIN_TILE_VARIANT_COUNT
                    + Random(0, TERRAIN_TILE_VARIANT_COUNT - 1);
        }
    }
}

// Buka 2.1 game::ProcessMapExtra reduced to HoMM1's town extras; HoMM1 has
// no late overlays.
VA(0x00436796, 0x107)
void game::ProcessMapExtra(void) {
    i32 y;
    mapCell* cellPtr;
    i32 x;
    i8 townNum;
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
VA(0x0043689d, 0x1cf)
i8 game::SetupTowns(void) {
    mapTownExtra* extra;
    i32 own;
    i8 noOwners;
    town* town;
    i32 j;
    i32 i;
    i32 mask;
    noOwners = 1;
    mask = MAP_TOWN_EXTRA_BUILDING_MASK;
    for (i = 0; i < GAME_TOWN_COUNT; i++) {
        town = GetTown(i);
        town->m_customized = 0;
        if (town->m_extraIndex >= 1) {
            extra = static_cast<mapTownExtra*>(ppMapExtra[town->m_extraIndex]);
            if (extra->customized && extra->owner != MAP_TOWN_OWNER_UNSET) {
                if (gpGame->m_playerCount <= extra->owner)
                    own = gpGame->m_playerCount - 1;
                else
                    own = extra->owner;
                noOwners = 0;
                if (own != GAME_PLAYER_NONE)
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
                town->m_buildings =
                    town->m_buildings - (town->m_buildings & mask) + (extra->buildings & mask);
            }
        }
    }
    if (!noOwners) {
        for (i = 0; i < GAME_TOWN_COUNT; i++) {
            town = GetTown(i);
            if (town->m_owner == MAP_TOWN_OWNER_UNSET)
                town->m_owner = GAME_PLAYER_NONE;
        }
    }
    return noOwners;
}

// Buka 2.1 game::ProcessOnMapHeroes for HoMM1: each placed hero takes its
// map-extra garrison, artifacts, experience and owner; a hero standing at a
// town gate occupies the town.
VA(0x00436a6c, 0x325)
void game::ProcessOnMapHeroes(void) {
    i32 mapY;
    mapHeroExtra* extra;
    town* town;
    i32 townId;
    i32 iPlayer;
    i32 k;
    i32 j;
    i32 mapX;
    mapCell* north;
    mapCell* cell;
    hero* theHero;

    for (mapY = 0; mapY < MAP_CELL_GRID_SIZE; mapY++) {
        for (mapX = 0; mapX < MAP_CELL_GRID_SIZE; mapX++) {
            cell = &m_map[mapX][mapY];
            if ((cell->m_triggerType & MAP_TRIGGER_TYPE_MASK) == MAP_FILE_OBJECT_HERO) {
                extra =
                    static_cast<mapHeroExtra*>(ppMapExtra[static_cast<u8>(cell->m_objectMetadata)]);
                theHero = GetHero(extra->heroId);
                for (k = 0; k < ARMY_GROUP_SLOT_COUNT; k++) {
                    theHero->m_army.m_creatureCounts[k] = extra->troopCounts[k];
                    if (theHero->m_army.m_creatureCounts[k] > 0)
                        theHero->m_army.m_creatureTypes[k] = extra->troopTypes[k];
                    else
                        theHero->m_army.m_creatureTypes[k] = CREATURE_NONE;
                }
                for (j = 0; j < MAP_HERO_EXTRA_ARTIFACT_COUNT; j++) {
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
                m_players[theHero->m_owner].m_heroIds[m_players[theHero->m_owner].m_heroCount] =
                    theHero->m_id;
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
                cell->m_objectIndex = MAP_CELL_NO_FRAME;
                cell->m_overlayTileset = 0;
                cell->m_overlayIndex = MAP_CELL_NO_FRAME;
                cell->m_objectMetadata = 0;
                cell->m_triggerType = MAP_OBJECT_NONE;
                SetVisibility(
                    theHero->m_x,
                    theHero->m_y,
                    theHero->m_owner,
                    gHeroScoutRadius[theHero->m_heroClass]
                );
            }
        }
    }
    CheckHeroConsistency();
}

// Buka 2.1 game::CheckHeroConsistency for HoMM1: replaces tavern heroes
// some player already owns, clears map heroes that lost their owner and
// zeroes the counts of empty or negative stacks.
VA(0x00436d91, 0x341)
void game::CheckHeroConsistency(void) {
    town* town;
    i32 j;
    i32 i;
    i32 y;
    i32 x;
    mapCell* cell;
    hero* theHero;

    for (i = 0; i < m_playerCount; i++) {
        if (!m_playerDead[i]) {
            for (j = 0; j < PLAYER_TAVERN_HERO_COUNT; j++) {
                if (m_availableHeroes[m_players[i].m_availableHeroIds[j]] >= 0
                    && m_availableHeroes[m_players[i].m_availableHeroIds[j]]
                           <= GAME_PLAYER_COUNT - 1) {
                    m_players[i].m_availableHeroIds[j] = GetNewHeroId(0);
                    m_availableHeroes[m_players[i].m_availableHeroIds[j]] =
                        HERO_AVAILABILITY_RETREATED;
                }
            }
        }
    }
    for (x = 0; x < MAP_CELL_GRID_SIZE; x++) {
        for (y = 0; y < MAP_CELL_GRID_SIZE; y++) {
            cell = gpAdvManager->GetCell(x, y);
            if (cell->m_triggerType == (MAP_TRIGGER_EVENT | MAP_OBJECT_HERO)) {
                if (static_cast<u8>(cell->m_objectMetadata) >= 0
                    && static_cast<u8>(cell->m_objectMetadata) < GAME_HERO_COUNT) {
                    theHero = GetHero(cell->m_objectMetadata);
                    if (theHero->m_owner < 0 || theHero->m_owner > GAME_PLAYER_COUNT - 1) {
                        if (theHero->m_locationType == (MAP_TRIGGER_EVENT | MAP_OBJECT_TOWN)) {
                            town = gpGame->GetTown(theHero->m_occupiedTown);
                            town->m_occupyingHeroId = TOWN_OCCUPYING_HERO_NONE;
                        }
                        RestoreCell(
                            theHero->m_x,
                            theHero->m_y,
                            theHero->m_locationType,
                            theHero->m_occupiedTown,
                            NULL,
                            1
                        );
                    }
                } else {
                    cell->m_triggerType = MAP_OBJECT_NONE;
                }
            }
        }
    }
    for (i = 0; i < GAME_HERO_COUNT; i++) {
        for (j = 0; j < ARMY_GROUP_SLOT_COUNT; j++) {
            if (m_heroRecs[i].m_army.m_creatureTypes[j] == CREATURE_NONE
                || m_heroRecs[i].m_army.m_creatureCounts[j] < 0)
                m_heroRecs[i].m_army.m_creatureCounts[j] = 0;
        }
    }
    for (i = 0; i < GAME_TOWN_COUNT; i++) {
        for (j = 0; j < ARMY_GROUP_SLOT_COUNT; j++) {
            if (m_castleRecs[i].m_army.m_creatureTypes[j] == CREATURE_NONE
                || m_castleRecs[i].m_army.m_creatureCounts[j] < 0)
                m_castleRecs[i].m_army.m_creatureCounts[j] = 0;
        }
    }
}

// REMOTE.GAM transfer (Buka RemoteSaveConstant): the sender announces the
// size (BOX_REMOTE_SAVE, answered by REMOTE_COMMAND_SAVE_INIT_RESPONSE),
// streams SEGMENT_SIZE-byte segments (SAVE_DATA), asks for each
// BATCH_SIZE-segment block's acknowledgement map (SAVE_ACK_REQUEST /
// SAVE_ACK_RESPONSE) and closes with SAVE_FINISH (RemoteCommand).
H1_ENUM_CONST_BEGIN(RemoteSaveConstant)
    REMOTE_SAVE_SEGMENT_SIZE = 200,
    REMOTE_SAVE_BATCH_SIZE = 100,
    REMOTE_SAVE_HEADER_SIZE = 8,
    REMOTE_SAVE_ACK_MAP_SIZE = 200,
    REMOTE_SAVE_INDEX_SIZE = 2,
    REMOTE_SAVE_RECEIVE_TIMEOUT = 20000,
    REMOTE_SAVE_BUFFER_EXTRA = 500,
    REMOTE_SAVE_DECODE_BUFFER_SIZE = 0x130b0,
    REMOTE_SAVE_TRANSFER_SOUNDS = 8
H1_ENUM_CONST_END(RemoteSaveConstant)

// donor PoL RVA 0x00083219; preferred Buka symbol ?TransmitSaveGame@game@@QAEHHHH@Z
// donor Buka TU SOURCE/GAME; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.660125;margin=0.426397;shape=0.321;size=0.898;calls=0.886;strings=%s%s|.\DATA\|PostWait;alternate=pol20:int game::TransmitSaveGame(int, int, int)@0x00083219

// Saves REMOTE.GAM, optionally LZH-encodes it, then sends it in 200-byte
// segments, 100 segments per acknowledged block.
VA(0x004370d2, 0x5fd)
i32 game::TransmitSaveGame(i32 remotePlayer, i32 playerExited) {
    i32 okay;
    char pathname[456];
    char* outData;
    i32 block;
    i32 numBlocks;
    i32 junk3;
    i32 oldTrack;
    char* sendPacket;
    i32 blockSize;
    i32 sendPacketIndex;
    i32 segCount;
    i32 fileHandle;
    i32 junk2;
    char* incoming;
    char acked[500];
    i32 junk1;
    i32 status;
    i32 unk;
    i32 fileSize;
    i32 len;
    char* fileData;
    i8 finished;

    gpAdvManager->TrimLoopingSounds(REMOTE_SAVE_TRANSFER_SOUNDS);
    okay = 0;
    status = 0;
    oldTrack = MUSIC_TRACK_NONE;
    oldTrack = GetCurrentTrack();
    StopMusic();

    LogStr("Transmit Game Start");
    if (gpAdvManager->m_active == 1)
        BVResMsg("Sending Data", RESOURCE_NONE, 0);
    while (!gHeartbeatSeen) {
        PollSound();
        Process1WindowsMessage();
    }
    AiPrint("Transmit Start");
    memset(acked, 0, sizeof(acked));
    SaveGame("REMOTE.GAM", 0);
    extern char gDataPath[];
    sprintf(pathname, "%s%s", gDataPath, "REMOTE.GAM");
    fileSize = FileSize(pathname);
    sendPacket = static_cast<char*>(malloc(REMOTE_MESSAGE_SIZE));
    if (!iMPBaseType || (iMPBaseType == MULTIPLAYER_BASE_NETWORK && gRemoteReady))
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
        if (!iMPBaseType || (iMPBaseType == MULTIPLAYER_BASE_NETWORK && gRemoteReady))
            fileSize = EncodeData(outData, fileData, fileSize);
        else
            outData = fileData;

        reinterpret_cast<i32*>(sendPacket)[0] =
            fileSize; // byte-evidenced: the save-transfer packet header words
        reinterpret_cast<i32*>(sendPacket)[1] =
            playerExited; // byte-evidenced: the save-transfer packet header words
        status = TransmitAndWait(
            sendPacket,
            remotePlayer,
            REMOTE_SAVE_HEADER_SIZE,
            BOX_REMOTE_SAVE,
            REMOTE_COMMAND_SAVE_INIT_RESPONSE,
            &incoming
        );
        if (!status)
            ShutDown(NULL);

        segCount = (fileSize - 1) / REMOTE_SAVE_SEGMENT_SIZE + 1;
        numBlocks = (segCount - 1) / REMOTE_SAVE_BATCH_SIZE + 1;
        for (block = 0; block < numBlocks; block++) {
            LogInt("Start Seg #", block);
            if (block + 1 == numBlocks)
                blockSize = segCount - block * REMOTE_SAVE_BATCH_SIZE;
            else
                blockSize = REMOTE_SAVE_BATCH_SIZE;
            finished = 0;
            while (!finished) {
                for (sendPacketIndex = block * REMOTE_SAVE_BATCH_SIZE;
                     sendPacketIndex < block * REMOTE_SAVE_BATCH_SIZE + blockSize;
                     sendPacketIndex++) {
                    PollSound();
                    CheckDoMain(0, 1);
                    if (!acked[sendPacketIndex]) {
                        if (sendPacketIndex + 1 == segCount)
                            len = fileSize - sendPacketIndex * REMOTE_SAVE_SEGMENT_SIZE;
                        else
                            len = REMOTE_SAVE_SEGMENT_SIZE;
                        *reinterpret_cast<i16*>(sendPacket) = static_cast<i16>(
                            sendPacketIndex
                        ); // byte-evidenced: the save-transfer packet header words
                        memcpy(
                            sendPacket + REMOTE_SAVE_INDEX_SIZE,
                            outData + sendPacketIndex * REMOTE_SAVE_SEGMENT_SIZE,
                            len
                        );
                        status = TransmitRemoteData(
                            sendPacket,
                            remotePlayer,
                            len + REMOTE_SAVE_INDEX_SIZE,
                            REMOTE_COMMAND_SAVE_DATA,
                            0,
                            1,
                            REMOTE_MESSAGE_DEFAULT,
                            1
                        );
                        if (!status)
                            ShutDown(NULL);
                    }
                }
                LogStr("PreWait");
                *reinterpret_cast<i16*>(sendPacket) = static_cast<i16>(
                    block * REMOTE_SAVE_BATCH_SIZE
                ); // byte-evidenced: the save-transfer packet header words
                status = TransmitAndWait(
                    sendPacket,
                    remotePlayer,
                    REMOTE_SAVE_INDEX_SIZE,
                    REMOTE_COMMAND_SAVE_ACK_REQUEST,
                    REMOTE_COMMAND_SAVE_ACK_RESPONSE,
                    &incoming
                );
                LogStr("PostWait");
                if (!status)
                    ShutDown(NULL);
                for (sendPacketIndex = 0; sendPacketIndex < blockSize; sendPacketIndex++) {
                    if (reinterpret_cast<RemoteMessage*>(incoming)->payload.data[sendPacketIndex]
                        > 0) // API-forced: char* record.
                        acked[block * REMOTE_SAVE_BATCH_SIZE + sendPacketIndex] = 1;
                }
                finished = 1;
                for (sendPacketIndex = block * REMOTE_SAVE_BATCH_SIZE;
                     sendPacketIndex < block * REMOTE_SAVE_BATCH_SIZE + blockSize;
                     sendPacketIndex++) {
                    if (!acked[sendPacketIndex])
                        finished = 0;
                }
            }
        }
        status = TransmitRemoteData(
            NULL,
            remotePlayer,
            0,
            REMOTE_COMMAND_SAVE_FINISH,
            1,
            1,
            REMOTE_MESSAGE_DEFAULT,
            1
        );
        if (!status)
            ShutDown(NULL);
        okay = 1;
    }

cleanup:
    free(sendPacket);
    if (!iMPBaseType || (iMPBaseType == MULTIPLAYER_BASE_NETWORK && gRemoteReady))
        free(outData);
    free(fileData);
    AiPrint("Transmit End");
    if (gpAdvManager->m_active == 1) {
        giBottomViewOverride = BOTTOM_VIEW_NONE;
        gpAdvManager->UpdBottomView(1, 1, 1);
    }
    if (oldTrack != MUSIC_TRACK_NONE) {
        PlayMusic(oldTrack);
    }
    return okay;
}

// donor PoL RVA 0x00083937; preferred Buka symbol ?ReceiveSaveGame@game@@QAEHHHHH@Z
// donor Buka TU SOURCE/GAME; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.655741;margin=0.222523;shape=0.420;size=0.807;calls=0.714;strings=%s%s|.\DATA\|Receive End;alternate=pol20:int game::ReceiveSaveGame(int, int, int, int)@0x00083937
// Collects the remote save in 200-byte segments, acknowledging each block
// of 100, then decodes it and writes REMOTE.GAM.
VA(0x004376cf, 0x4b0)
i32 game::ReceiveSaveGame(i32 dataSize, i32 remotePlayer) {
    i32 unused1;
    i32 okay;
    char pathname[452];
    char* inData;
    i32 k;
    i32 oldTrack;
    i32 fileHandle;
    i8 done;
    char* sendPacket;
    RemoteMessage* receivedPacket;
    i32 result;
    i32 lastPacketTime;
    char gotIt[500];
    i32 packetStart;
    char* decodedData;

    gpAdvManager->TrimLoopingSounds(REMOTE_SAVE_TRANSFER_SOUNDS);
    fileHandle = 0;
    done = 0;
    unused1 = 0;
    okay = 0;
    oldTrack = MUSIC_TRACK_NONE;
    if (gpAdvManager->m_active == 1)
        BVResMsg("Receiving Data", RESOURCE_NONE, 0);
    oldTrack = GetCurrentTrack();
    StopMusic();
    while (!gHeartbeatSeen) {
        PollSound();
        Process1WindowsMessage();
    }
    result = TransmitRemoteData(
        NULL,
        remotePlayer,
        0,
        REMOTE_COMMAND_SAVE_INIT_RESPONSE,
        1,
        1,
        REMOTE_MESSAGE_DEFAULT,
        1
    );
    if (!result)
        ShutDown(NULL);
    memset(gotIt, 0, sizeof(gotIt));
    if (!iMPBaseType || (iMPBaseType == MULTIPLAYER_BASE_NETWORK && gRemoteReady))
        decodedData = static_cast<char*>(malloc(REMOTE_SAVE_DECODE_BUFFER_SIZE));
    sendPacket = static_cast<char*>(malloc(REMOTE_MESSAGE_SIZE));
    inData = static_cast<char*>(malloc(dataSize + REMOTE_SAVE_BUFFER_EXTRA));
    lastPacketTime = KBTickCount();
    while (!done) {
        PollSound();
        CheckDoMain(0, 1);
        if (lastPacketTime + REMOTE_SAVE_RECEIVE_TIMEOUT < KBTickCount()) {
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
                ShutDown(NULL);
        }
        receivedPacket =
            reinterpret_cast<RemoteMessage*>(GetRemoteData(1)); // API-forced: char* record.
        if (receivedPacket
            && (receivedPacket->type == REMOTE_MESSAGE_RELIABLE
                || receivedPacket->type == REMOTE_MESSAGE_UNRELIABLE)) {
            lastPacketTime = KBTickCount();
            switch (receivedPacket->command) {
                case REMOTE_COMMAND_SAVE_DATA:
                    packetStart = receivedPacket->payload.segment.index;
                    gotIt[packetStart] = 1;
                    memcpy(
                        inData + packetStart * REMOTE_SAVE_SEGMENT_SIZE,
                        receivedPacket->payload.segment.data,
                        receivedPacket->payloadSize - REMOTE_SAVE_INDEX_SIZE
                    );
                    break;
                case REMOTE_COMMAND_SAVE_ACK_REQUEST:
                    packetStart = receivedPacket->payload.segment.index;
                    for (k = packetStart; k < packetStart + REMOTE_SAVE_BATCH_SIZE; k++)
                        *(sendPacket + k - packetStart) = gotIt[k];
                    result = TransmitRemoteData(
                        sendPacket,
                        remotePlayer,
                        REMOTE_SAVE_ACK_MAP_SIZE,
                        REMOTE_COMMAND_SAVE_ACK_RESPONSE,
                        1,
                        1,
                        REMOTE_MESSAGE_DEFAULT,
                        1
                    );
                    if (!result)
                        ShutDown(NULL);
                    break;
                case REMOTE_COMMAND_SAVE_FINISH:
                    done = 1;
                    break;
            }
        }
    }
    if (!iMPBaseType || (iMPBaseType == MULTIPLAYER_BASE_NETWORK && gRemoteReady))
        dataSize = DecodeData(decodedData, inData);
    else
        decodedData = inData;
    extern char gDataPath[];
    sprintf(pathname, "%s%s", gDataPath, "REMOTE.GAM");
    fileHandle = open(pathname, O_WRONLY | O_CREAT | O_TRUNC | O_BINARY, S_IWRITE);
    if (fileHandle == -1)
        FileError(pathname);
    write(fileHandle, decodedData, dataSize);
    close(fileHandle);
    okay = 1;
    free(sendPacket);
    free(inData);
    if (!iMPBaseType || (iMPBaseType == MULTIPLAYER_BASE_NETWORK && gRemoteReady))
        free(decodedData);
    AiPrint("Receive End");
    if (gpAdvManager->m_active == 1) {
        giBottomViewOverride = BOTTOM_VIEW_NONE;
        gpAdvManager->UpdBottomView(1, 1, 1);
    }
    if (oldTrack != MUSIC_TRACK_NONE) {
        PlayMusic(oldTrack);
    }
    return okay;
}

// donor PoL RVA 0x00083fc4; preferred Buka symbol ?DoNewTurn@game@@QAEXXZ
// donor Buka TU SOURCE/GAME; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.487826;margin=0.297906;shape=0.344;size=0.836;calls=0.867;alternate=pol20:void game::DoNewTurn(void)@0x00083fc4
VA(0x00437b7f, 0x5b5)
void game::DoNewTurn(void) {
    i32 track;
    char monsterName[52];

    if (!gbThisNetHumanPlayer[giCurPlayer]) {
        CheckEndGame(0);
        return;
    }
    giBottomViewOverrideEndTime = KBTickCount() + 3000;
    giBottomViewOverride = BOTTOM_VIEW_NEW_TURN;
    gpAdvManager->UpdBottomView(1, 1, 1);
    gpAdvManager->SetInitialMapOrigin();
    gpAdvManager->CompleteDraw(0);
    gpAdvManager->UpdateScreen(0, 0);
    CheckEndGame(0);
    if (gpCurPlayer->m_daysLeft >= 0) {
        if (gpCurPlayer->m_daysLeft == 1) {
            sprintf(
                gText,
                gNewTurnText[NEW_TURN_TEXT_LAST_DAY],
                gColorNames[gpGame->m_players[giCurPlayer].Color()]
            );
            gText[0] -= 'a' - 'A';
        } else {
            sprintf(
                gText,
                gNewTurnText[NEW_TURN_TEXT_DAYS_LEFT],
                gColorNames[gpGame->m_players[giCurPlayer].Color()],
                gpCurPlayer->m_daysLeft
            );
            gText[0] -= 'a' - 'A';
        }
        NormalDialog(
            gText,
            NORMAL_DIALOG_TYPE_OK,
            0x61,
            -1,
            NORMAL_DIALOG_CREST,
            gpGame->m_players[giCurPlayer].Color(),
            NORMAL_DIALOG_NO_RESOURCE,
            0,
            NORMAL_DIALOG_NO_OR_TEXT
        );
    }
    if (gpCurPlayer->m_heroCount > 0)
        gpAdvManager->SetHeroContext(gpCurPlayer->NextHero(0), 0);
    else if (gpCurPlayer->m_townCount > 0)
        gpAdvManager->SetTownContext(gpCurPlayer->m_townIds[0]);
    gpAdvManager->CheckDimNextHeroBut();
    PlayMusic(gpAdvManager->m_currentTerrain);
    if (m_day == 1) {
        if ((m_month != 1 || m_week != 1 || m_day != 1) && giWeekType != CALENDAR_PERIOD_NONE) {
            track = MUSIC_TRACK_NONE;
            if (m_week == 1) {
                track = MUSIC_TRACK_NEW_MONTH;
                if (giMonthType == CALENDAR_PERIOD_NORMAL) {
                    sprintf(
                        gText,
                        gNewTurnText[NEW_TURN_TEXT_MONTH_NORMAL],
                        gMonthNames[giMonthTypeExtra]
                    );
                } else if (giMonthType == CALENDAR_PERIOD_CREATURE) {
                    strcpy(monsterName, gArmyNames[giMonthTypeExtra]);
                    monsterName[0] -= 'a' - 'A';
                    sprintf(
                        gText,
                        gNewTurnText[NEW_TURN_TEXT_MONTH_CREATURE],
                        gArmyNames[giMonthTypeExtra],
                        monsterName
                    );
                } else {
                    sprintf(gText, gNewTurnText[NEW_TURN_TEXT_MONTH_PLAGUE]);
                }
            } else {
                track = MUSIC_TRACK_NEW_WEEK;
                if (giWeekType == CALENDAR_PERIOD_NORMAL) {
                    sprintf(
                        gText,
                        gNewTurnText[NEW_TURN_TEXT_WEEK_NORMAL],
                        gWeekNames[giWeekTypeExtra]
                    );
                } else {
                    strcpy(monsterName, gArmyNames[giWeekTypeExtra]);
                    monsterName[0] -= 'a' - 'A';
                    sprintf(
                        gText,
                        gNewTurnText[NEW_TURN_TEXT_WEEK_CREATURE],
                        gArmyNames[giWeekTypeExtra],
                        monsterName
                    );
                }
            }
            PlayMusic(track);
            gpMouseManager->SetPointer(ADVENTURE_POINTER_DEFAULT);
            NormalDialog(
                gText,
                NORMAL_DIALOG_TYPE_OK,
                0x61,
                -1,
                NORMAL_DIALOG_NO_RESOURCE,
                0,
                NORMAL_DIALOG_NO_RESOURCE,
                0,
                NORMAL_DIALOG_NO_OR_TEXT
            );
            PlayMusic(gpAdvManager->m_currentTerrain);
        }
    }
}

// Buka 2.1 game::GetBoatsBuilt.
VA(0x00438134, 0x4c)
i32 game::GetBoatsBuilt(void) {
    i32 count = 0;
    i32 i;
    for (i = 0; i < GAME_BOAT_COUNT; ++i) {
        if (m_boatSlots[i] != GAME_TABLE_FREE)
            ++count;
    }
    return count;
}

DATA(0x004a6c29)
i8 gShowMapInfo = 0;

// donor PoL RVA 0x000b6f40; preferred Buka symbol ?GetMap@game@@QAEXXZ
// donor Buka TU SOURCE/Newgame; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.547944;margin=0.077231;shape=0.356;size=0.530;calls=0.607;strings=.\MAPS\;alternate=pol20:void game::GetMap(void)@0x000b6f40
// Buka 2.1 GetMap with HoMM1's player-count file masks and the reqextra.bin
// map-info window; a cancelled pick restores the previous map's texts.
VA(0x00438180, 0x373)
void game::GetMap(void) {
    char saveFullName[20];
    char oldDescription[124];
    char oldMapName[16];
    char mask[16];
    i16 result;
    fileRequester* request;

    strcpy(oldMapName, gMapName);
    strcpy(saveFullName, gFullMapName);
    strcpy(oldDescription, gMapDescription);
    gShowMapInfo = 1;
    strcpy(gCurMapName, "");
    gReqExtraWindow = new heroWindow(310, 332, "reqextra.bin");
    if (!gReqExtraWindow)
        MemError();
    if (giNumHumanPlayers == 1)
        sprintf(mask, "????1???.MAP");
    else if (giNumHumanPlayers == 2)
        sprintf(mask, "?????2??.MAP");
    else if (giNumHumanPlayers == 3)
        sprintf(mask, "??????3?.MAP");
    else if (giNumHumanPlayers == 4)
        sprintf(mask, "???????4.MAP");
    extern char gMapPath[];
    request = new fileRequester(310, 14, FILE_REQUESTER_LOAD, mask, gMapPath, ".MAP");
    if (!request)
        MemError();
    request->ShowMapInfo();
    result = gpExec->DoDialog(request);
    gpWindowManager->RemoveWindow(gReqExtraWindow);
    if (result == DIALOG_BUTTON_2) {
        strcpy(gMapName, gLastFilename);
        delete request;
    } else {
        strcpy(gMapName, oldMapName);
        strcpy(gFullMapName, saveFullName);
        strcpy(gMapDescription, oldDescription);
        delete request;
    }
    delete gReqExtraWindow;
    gShowMapInfo = 0;
}

// Buka 2.1 game::GetNumThievesGuilds.
VA(0x004384f3, 0x80)
i32 game::GetNumThievesGuilds(i32 color) {
    i32 numGuilds = 0;
    i32 i;
    for (i = 0; i < m_players[color].m_townCount; ++i) {
        if (gpGame->m_castleRecs[m_players[color].m_townIds[i]].m_buildings
            & (1 << BUILDING_SLOT_THIEVES_GUILD))
            ++numGuilds;
    }
    return numGuilds;
}

// Buka 2.1 game::CalcDifficultyRating for HoMM1: difficulty, opponents
// (human seats by handicap, computers by level), King of the Hill, map
// size and map difficulty.
VA(0x00438573, 0x2e3)
i32 game::CalcDifficultyRating(void) {
    i32 i;
    i32 total;

    total = 0;
    if (m_difficulty == DIFFICULTY_EASY) {
    } else if (m_difficulty == DIFFICULTY_NORMAL) {
        total += 10;
    } else if (m_difficulty == DIFFICULTY_HARD) {
        total += 20;
    } else if (m_difficulty == DIFFICULTY_EXPERT) {
        total += 30;
    }
    for (i = 1; i < GAME_PLAYER_COUNT; i++) {
        if (i < giNumHumanPlayers)
            total += (m_players[i].m_difficulty - 1) * 10;
        else if (m_players[i].m_difficulty == PLAYER_TYPE_NONE)
            total -= 10;
        else if (m_players[i].m_difficulty == PLAYER_TYPE_DUMB)
            total += 5;
        else if (m_players[i].m_difficulty == PLAYER_TYPE_AVERAGE)
            total += 10;
        else if (m_players[i].m_difficulty == PLAYER_TYPE_SMART)
            total += 15;
        else if (m_players[i].m_difficulty == PLAYER_TYPE_GENIUS)
            total += 20;
    }
    gpGame->m_playerCount = 0;
    for (i = 0; i < GAME_PLAYER_COUNT; i++) {
        if (gpGame->m_players[i].m_difficulty > PLAYER_TYPE_NONE)
            gpGame->m_playerCount++;
    }
    if (gbIAmGreatest) {
        if (m_playerCount - giNumHumanPlayers == 0) {
        } else if (m_playerCount - giNumHumanPlayers == 1) {
        } else if (m_playerCount - giNumHumanPlayers == 2) {
            total += 5;
        } else if (m_playerCount - giNumHumanPlayers == 3) {
            total += 10;
        }
    }
    if (gMapSize == MAP_SIZE_SMALL) {
    } else if (gMapSize == MAP_SIZE_MEDIUM) {
        total += 10;
    } else if (gMapSize == MAP_SIZE_LARGE) {
        total += 20;
    }
    if (gMapDifficulty == MAP_DIFFICULTY_EASY)
        total += 20;
    else if (gMapDifficulty == MAP_DIFFICULTY_NORMAL)
        total += 30;
    else if (gMapDifficulty == MAP_DIFFICULTY_TOUGH)
        total += 40;
    else if (gMapDifficulty == MAP_DIFFICULTY_IMPOSSIBLE)
        total += 50;
    return total;
}

// ShowCongrats' base score: 200 less a day per day for two months, then a
// half, a quarter and an eighth per day, never below 20.
VA(0x00438856, 0xde)
i32 GetBaseScore(i32 days) {
    i32 score;

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

VA(0x00438934, 0x410)
void game::ShowScenInfo(void) {
    i16 i;
    const i8 sizeId = 100;
    const i8 mapLevelId = 101;
    const i8 mapDescId = 102;
    const i8 crestId = 103;
    const i8 nameId = 104;
    const i8 levelId = 105;
    const i8 playersId = 106;
    const i8 kingOfHillId = 107;
    const i8 ratingId = 108;
    char line1[20];
    i32 difficulty;
    heroWindow* scenWindow;
    tag_message message;
    i16 idx;
    // Retail reserves one unused slot between the seat counters.
    i32 pad;

    gpMouseManager->SetPointer(ADVENTURE_POINTER_DEFAULT);
    scenWindow = new heroWindow(159, 14, "sceninfo.bin");
    if (!scenWindow)
        MemError();
    SET_WIDGET_MESSAGE(message, WIDGET_COMMAND_SET_TEXT, nameId);
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
    for (i = 1; i < GAME_PLAYER_COUNT; i++) {
        if (giCurPlayer == 0) {
            sprintf(
                line1,
                "%s\n",
                gbHumanPlayer[i] ? gHandicapNames[m_players[i].m_difficulty]
                                 : gPlayerTypeNames[m_players[i].m_difficulty]
            );
        } else if (i == 1) {
            sprintf(line1, "%s\n", gHandicapNames[m_difficulty + 1]);
        } else {
            if (i - 1 >= giCurPlayer)
                idx = i;
            else
                idx = i - 1;
            sprintf(
                line1,
                "%s\n",
                gbHumanPlayer[idx] ? gHandicapNames[m_players[idx].m_difficulty]
                                   : gPlayerTypeNames[m_players[idx].m_difficulty]
            );
        }
        strcat(gText, line1);
    }
    scenWindow->BroadcastMessage(message);
    message.id = kingOfHillId;
    message.text = gText;
    sprintf(gText, gbIAmGreatest ? "Yes" : "No");
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
    if (m_players[giCurPlayer].m_color != PLAYER_COLOR_NONE) {
        message.id = crestId;
        message.value = m_players[giCurPlayer].m_color * 2 + 11;
        scenWindow->BroadcastMessage(message);
    }
    gpWindowManager->DoDialog(scenWindow, EventWindowHandler, 0);
}

// HoMM1 keeps the human's crest and gives each opponent a free one: the
// campaign scenario's crest when it names one, else a random draw.
VA(0x00438d44, 0x114)
void game::RandomizePlayerCrests(void) {
    i32 i;
    i8 taken[PLAYER_COLOR_COUNT];
    taken[PLAYER_COLOR_BLUE] = 0;
    taken[PLAYER_COLOR_GREEN] = 0;
    taken[PLAYER_COLOR_RED] = 0;
    taken[PLAYER_COLOR_YELLOW] = 0;
    taken[m_players[0].m_color] = 1;
    for (i = 1; i < m_playerCount; i++) {
        do {
            if (m_campaignType > 0
                && gCampaignScenarios[m_campaignScenario].playerCrests[i] < PLAYER_COLOR_COUNT
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
VA(0x00438e58, 0x87)
void game::RestoreCell(i32 x, i32 y, i32 obj, i32 barrier, mapCell* passedCell, i32) {
    mapCell* cell;
    if (passedCell)
        cell = passedCell;
    else
        cell = gpAdvManager->GetCell(x, y);
    if (y > 0 && obj == (MAP_TRIGGER_EVENT | MAP_OBJECT_TOWN)
        && gpAdvManager->GetCell(x, y - 1)->m_triggerType != MAP_OBJECT_TOWN) {
        cell->m_triggerType = MAP_OBJECT_NONE;
        cell->m_objectMetadata = 0;
        return;
    }
    cell->m_triggerType = obj;
    cell->m_objectMetadata = barrier;
}

// GAME owns retail .data 0x00490920-0x00490b4b and .bss 0x004c50d0-0x004c512f.
// Retail emits gNewGameSettingsSaved, gMonType and gLastSeed among the
// literals of their users; gShowMapInfo is defined above GetMap.
DATA(0x004a6c24)
i32 gGameOver = 0;
DATA(0x0048fcc8)
u32 gLastSeed = 135621123;
DATA(0x004a7318)
i8 gSaveCurPlayer;
DATA(0x004a72e4)
i8 gSavedCrest;
DATA(0x004a72f8)
i8 gSavedDifficulty;
DATA(0x004a7338)
i32 gEndSequence;
DATA(0x004a731c)
i8 gbDismissArmy;
DATA(0x004a6be4)
heroWindow* gReqExtraWindow;
DATA(0x004a72d8)
i8 gSavedPlayerTypes[4];
DATA(0x004a7300)
i16 gMineTypeCount[RESOURCE_COUNT];
DATA(0x004a6c10)
char gCurMapName[16];
DATA(0x004a7320)
i8 gSavedKingOfTheHill;
DATA(0x004a72e8)
i8 gRandomTownTypes[4];

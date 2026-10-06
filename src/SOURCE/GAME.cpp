#include <match.h>

#include <BASE/audio.h>
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

VA(0x0042b400, 0x1f2)
void playerData::Write(i32 file) {
    char unused[52];

    write(file, m_unknown00, sizeof(m_unknown00));
    WRITE_FILE_VALUE(file, m_color);
    WRITE_FILE_VALUE(file, m_difficulty);
    WRITE_FILE_VALUE(file, m_heroCount);
    WRITE_FILE_VALUE(file, m_currentHero);
    WRITE_FILE_VALUE(file, m_heroLocatorPage);
    write(file, m_heroIds, sizeof(m_heroIds));
    write(file, m_availableHeroIds, sizeof(m_availableHeroIds));
    memset(unused, 0, PLAYER_SAVE_PAD_SIZE);
    write(file, unused, PLAYER_SAVE_PAD_SIZE);
    WRITE_FILE_VALUE(file, m_ultimateArtifactHintChance);
    WRITE_FILE_VALUE(file, m_ultimateArtifactHintX);
    WRITE_FILE_VALUE(file, m_ultimateArtifactHintY);
    WRITE_FILE_VALUE(file, m_daysLeft);
    WRITE_FILE_VALUE(file, m_townCount);
    WRITE_FILE_VALUE(file, m_currentTown);
    WRITE_FILE_VALUE(file, m_townLocatorPage);
    write(file, m_townIds, sizeof(m_townIds));
    write(file, m_resources, sizeof(m_resources));
    write(file, m_aiData.m_income, sizeof(m_aiData.m_income));
    WRITE_FILE_VALUE(file, m_unknown99[1]);
    WRITE_FILE_VALUE(file, m_unknown99[1]);
    write(file, m_obelisksVisited, sizeof(m_obelisksVisited));
}

VA(0x0042b5f2, 0x1e1)
void playerData::Read(i32 file) {
    char unused[52];

    read(file, m_unknown00, sizeof(m_unknown00));
    READ_FILE_VALUE(file, m_color);
    READ_FILE_VALUE(file, m_difficulty);
    READ_FILE_VALUE(file, m_heroCount);
    READ_FILE_VALUE(file, m_currentHero);
    READ_FILE_VALUE(file, m_heroLocatorPage);
    read(file, m_heroIds, sizeof(m_heroIds));
    read(file, m_availableHeroIds, sizeof(m_availableHeroIds));
    read(file, unused, PLAYER_SAVE_PAD_SIZE);
    READ_FILE_VALUE(file, m_ultimateArtifactHintChance);
    READ_FILE_VALUE(file, m_ultimateArtifactHintX);
    READ_FILE_VALUE(file, m_ultimateArtifactHintY);
    READ_FILE_VALUE(file, m_daysLeft);
    READ_FILE_VALUE(file, m_townCount);
    READ_FILE_VALUE(file, m_currentTown);
    READ_FILE_VALUE(file, m_townLocatorPage);
    read(file, m_townIds, sizeof(m_townIds));
    read(file, m_resources, sizeof(m_resources));
    read(file, m_aiData.m_income, sizeof(m_aiData.m_income));
    READ_FILE_VALUE(file, m_unknown99[1]);
    READ_FILE_VALUE(file, m_unknown99[1]);
    read(file, m_obelisksVisited, sizeof(m_obelisksVisited));
}

VA(0x0042b7d3, 0x100)
i8 playerData::NextHero(i32) {
    i32 curHero = -1;
    i32 i;

    if (gpCurPlayer->m_currentHero != HERO_ID_NONE) {
        for (i = 0; i < gpCurPlayer->m_heroCount; ++i) {
            if (gpCurPlayer->m_currentHero == gpCurPlayer->m_heroIds[i])
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
    return HERO_ID_NONE;
}

VA(0x0042b8d3, 0x56)
i8 playerData::HasMobileHero(void) {
    for (i16 i = 0; i < m_heroCount; ++i) {
        if (gpGame->IsMobile(m_heroIds[i]))
            return 1;
    }
    return 0;
}

// Counts this player's visited-obelisk bits.
VA(0x0042b929, 0x56)
i8 playerData::CountVisitedObelisks(void) {
    i8 count = 0;
    for (i16 idx = 0; idx < PLAYER_PUZZLE_PIECE_COUNT; ++idx) {
        if (BitTest(m_obelisksVisited, idx))
            ++count;
    }
    return count;
}

// Slot 0 is the mage guild.
VA(0x0042b97f, 0xb6)
i32 playerData::BuildingsOwned(
    H1_ENUM_PARAM(TownType, i32) townType,
    H1_ENUM_PARAM(BuildingSlotType, i32) buildingIndex,
    i32 buildState
) {
    i32 count = 0;
    i32 i;
    for (i = 0; i < m_townCount; ++i) {
        town* ownedTown = &gpGame->m_castleRecs[m_townIds[i]];
        if (buildingIndex < BUILDING_SLOT_DWELLING_FIRST || ownedTown->m_type == townType) {
            if (buildingIndex == BUILDING_SLOT_MAGE_GUILD) {
                if (ownedTown->m_buildings & H1_ENUM_BIT(BuildingSlotType, BUILDING_SLOT_MAGE_GUILD)) {
                    if (ownedTown->m_buildState == buildState)
                        ++count;
                }
            } else {
                if (ownedTown->m_buildings & H1_ENUM_BIT(BuildingSlotType, buildingIndex))
                    ++count;
            }
        }
    }
    return count;
}

// Scans the fourteen hero slots.
VA(0x0042ba35, 0x84)
i32 playerData::NumOfGivenArtifact(H1_ENUM_PARAM(ArtifactType, i32) artifact) {
    i32 count = 0;
    i32 i;
    i32 jj;
    for (i = 0; i < m_heroCount; i++) {
        for (jj = 0; jj < HERO_ARTIFACT_SLOT_COUNT; jj++) {
            if (gpGame->m_heroRecs[m_heroIds[i]].m_artifacts[jj] == artifact)
                count++;
        }
    }
    return count;
}

// Needs eleven obelisks (four percent each over ten) and skips player 0's
// hint.
VA(0x0042bab9, 0x321)
void ComputeUALoc(i32 player) {
    i32 triesCount;
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
            if (Random(1, 100) <= gpGame->m_players[player].m_ultimateArtifactHintChance) {
                gpGame->m_players[player].m_ultimateArtifactHintX = gpGame->m_ultimateArtifactX;
                gpGame->m_players[player].m_ultimateArtifactHintY = gpGame->m_ultimateArtifactY;
            } else {
                x = PLAYER_ULTIMATE_HINT_NONE;
                y = PLAYER_ULTIMATE_HINT_NONE;
                heading = 0;
                triesCount = 0;
                while (
                    !(x >= 0 && x < MAP_CELL_GRID_SIZE && y >= 0 && y < MAP_CELL_GRID_SIZE
                      && gpGame->m_map[x][y].m_triggerType == MAP_OBJECT_TRIGGER(MAP_OBJECT_NONE)
                      && gpGame->m_map[x][y].m_objectIndex == MAP_CELL_NO_FRAME
                      && gpGame->m_map[x][y].m_overlayIndex == MAP_CELL_NO_FRAME
                      && gpGame->m_map[x][y].m_tileIndex >= MAP_CELL_TILES_PER_TERRAIN)
                ) {
                    triesCount++;
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
                    if (triesCount >= ULTIMATE_HINT_PLACE_TRIES) {
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
// pieces, then re-roll the hint.
VA(0x0042bdda, 0x17c)
void game::VisitObelisk(i8 player) {
    i16 curAttempts;
    i8 visitedFlag;
    i8 fallback;
    i8 thePiece;
    i32 newPieces;
    i16 myRemovedNo;
    i32 removeCount;

    newPieces = PLAYER_PUZZLE_PIECE_COUNT;
    removeCount = newPieces / m_obeliskCount;
    if (removeCount < 1)
        removeCount = 1;
    for (myRemovedNo = 0; myRemovedNo < removeCount; myRemovedNo++) {
        visitedFlag = m_players[player].CountVisitedObelisks();
        for (thePiece = 0; thePiece < newPieces; thePiece += Random(1, 5)) {
            if (!BitTest(m_players[player].m_obelisksVisited, thePiece))
                break;
        }
        for (curAttempts = 0; curAttempts < OBELISK_PIECE_PICK_TRIES; curAttempts++) {
            fallback = Random(0, newPieces - 1);
            if (!BitTest(m_players[player].m_obelisksVisited, fallback))
                break;
        }
        if (thePiece < newPieces)
            BitSet(m_players[player].m_obelisksVisited, thePiece);
        else
            BitSet(m_players[player].m_obelisksVisited, fallback);
    }
    ComputeUALoc(player);
}

VA(0x0042bf56, 0x98)
i8 game::IsMobile(i8 heroId) {
    if (heroId == HERO_ID_NONE)
        return 0;
    hero* mobileHero = &m_heroRecs[heroId];
    H1_ENUM_LOCAL(TerrainType, i32) terrainValue =
        CELL_TERRAIN(gpAdvManager->GetCell(mobileHero->m_x, mobileHero->m_y));
    return mobileHero->m_remainingMobility >= CalcTerrainCost(
               H1_ENUM_ENCODE(TerrainType, terrainValue),
               H1_ENUM_ENCODE(MapDirection, mobileHero->m_direction) & MAP_DIRECTION_DIAGONAL_BIT,
               mobileHero->m_remainingMobility,
               mobileHero->m_heroClass
           );
}

VA(0x0042bfee, 0x13)
mapCell (*game::GetWorldMapData(void)) [MAP_CELL_GRID_SIZE] { return m_map; }

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
        square->m_triggerType = MAP_EVENT_TRIGGER(MAP_OBJECT_SHIP);
        square->m_objectMetadata = boatIdx;
    }
    return boatIdx;
}

VA(0x0042c0cf, 0x4b)
i8 game::Scan(i8* array, i8 start, i8 length) {
    i8 i;
    for (i = start; i < start + length; ++i) {
        if (array[i] == GAME_TABLE_FREE)
            return i;
    }
    return GAME_TABLE_FREE;
}

// Always looks for a free (-1) entry.
VA(0x0042c11a, 0x64)
i8 game::RandomScan(i8* array, i8 start, i8 range, i32) {
    i8 idx = GAME_TABLE_FREE;
    i32 i;
    for (i = 0; i < GAME_RANDOM_SCAN_TRIES; ++i) {
        idx = start + Random(0, range - 1);
        if (array[idx] == GAME_TABLE_FREE)
            return idx;
    }
    return GAME_TABLE_FREE;
}

// Nine heroes per class; a 0x40 entry is the fallback pick.
VA(0x0042c17e, 0xe5)
i8 game::GetNewHeroId(i8 heroClass) {
    i8 freeSlot = GAME_TABLE_FREE;
    i8 idx = HERO_ID_NONE;
    i16 first = heroClass * HERO_PER_CLASS_COUNT;
    i32 ix;
    freeSlot = Scan(m_availableHeroes, first, HERO_PER_CLASS_COUNT);
    if (freeSlot != GAME_TABLE_FREE) {
        idx = RandomScan(m_availableHeroes, first, HERO_PER_CLASS_COUNT, HERO_PER_CLASS_COUNT);
    } else {
        freeSlot = Scan(m_availableHeroes, 0, GAME_HERO_COUNT);
        if (freeSlot != GAME_TABLE_FREE) {
            idx = RandomScan(m_availableHeroes, 0, GAME_HERO_COUNT, GAME_HERO_COUNT);
        } else {
            for (ix = 0; ix < GAME_HERO_COUNT; ++ix) {
                if (m_availableHeroes[ix] == HERO_AVAILABILITY_RETREATED)
                    idx = ix;
            }
        }
    }
    if (idx != HERO_ID_NONE)
        return idx;
    else
        return 0;
}

VA(0x0042c263, 0x69)
i8 game::GetTownId(i8 x, i8 y) {
    for (i16 i = 0; i < GAME_TOWN_COUNT; ++i) {
        if (m_castleRecs[i].m_x == x && m_castleRecs[i].m_y == y)
            return i;
    }
    return GAME_TOWN_NONE;
}

VA(0x0042c2cc, 0x69)
i8 game::GetMineId(i8 x, i8 y) {
    for (i16 i = 0; i < GAME_MINE_COUNT; ++i) {
        if (m_mines[i].x == x && m_mines[i].y == y)
            return i;
    }
    return GAME_MINE_NONE;
}

VA(0x0042c335, 0x1a8)
void GenerateStandardFileName(char* source, char* destination) {
    char* ext;
    i32 indexOut;
    i32 length;
    i32 i;
    u8 chr;

    ext = FindLastToken(source, '.');
    if (!ext) {
        strcpy(destination, source);
        return;
    }
    *ext = 0;
    indexOut = 0;
    length = strlen(source);
    for (i = 0; i < length; i++) {
        chr = source[i];
        if (chr >= 'a' && chr <= 'z')
            chr = chr - ('a' - 'A');
        else if (chr >= CYRILLIC_SMALL_A && chr <= CYRILLIC_SMALL_YA)
            chr = chr - CYRILLIC_CASE_OFFSET;
        else if (chr == CYRILLIC_SMALL_YO)
            chr = CYRILLIC_CAPITAL_YO;
        else
            chr = chr;
        if ((chr >= 'A' && chr <= 'Z') || (chr >= CYRILLIC_CAPITAL_A && chr <= CYRILLIC_CAPITAL_YA)
            || (chr >= '0' && chr <= '9') || chr == CYRILLIC_CAPITAL_YO || chr == '_') {
            destination[indexOut] = chr;
            indexOut++;
        }
        if (indexOut >= SAVE_FILE_BASE_NAME_LENGTH)
            i = SAVE_FILE_NAME_SCAN_STOP;
    }
    *ext = '.';
    strcpy(destination + indexOut, ext);
}

inline void game::ReadWorldMap(i32 fd) {
    read(fd, m_map, sizeof(m_map));
}

inline void game::WriteWorldMap(i32 fd) {
    write(fd, m_map, sizeof(m_map));
}

// Single save layout: name, globals, campaign state, map header, players,
// world map, records and visibility.
VA(0x0042c4dd, 0x796)
i16 game::SaveGame(char* filename, i8 generateName) {
    i32 unusedIndex;
    i32 nHuman;
    i32 unusedName;
    i32 mySaveFlag;
    char humans[GAME_PLAYER_COUNT];
    i32 iFile;
    i32 outFile;
    i32 scratchVals[4];
    char savePath[452];
    char genName[452];
    char buffer[100];

    gpAdvManager->DemobilizeCurrHero();
    if (generateName) {
        if (m_campaignType > 0) {
            sprintf(genName, "%s.%s", filename, "CGM");
        } else {
            nHuman = 0;
            for (iFile = 0; iFile < GAME_PLAYER_COUNT; iFile++) {
                if (!m_playerDead[iFile] && gbHumanPlayer[iFile])
                    nHuman++;
            }
            sprintf(genName, "%s.GM%d", filename, nHuman);
        }
    } else {
        sprintf(genName, filename);
    }
    if (!strcmpi(genName, "REMOTE.GAM")) {
        sprintf(savePath, "%s%s", gDataPath, genName);
    } else {
        sprintf(savePath, "%s%s", gGamePath, genName);
        if (strnicmp(genName, localization::Tr("save.name.autosave"), SAVE_FILE_BASE_NAME_LENGTH)
            && strnicmp(
                genName,
                localization::Tr("save.name.player_exit"),
                SAVE_FILE_BASE_NAME_LENGTH
            ))
            strcpy(gpGame->m_saveName, filename);
    }
    outFile = open(savePath, O_WRONLY | O_CREAT | O_TRUNC | O_BINARY, S_IWRITE);
    if (outFile == -1)
        FileError(savePath);
    WRITE_FILE_VALUE(outFile, gbIAmGreatest);
    write(outFile, this, 2);
    WRITE_FILE_VALUE(outFile, giMonthType);
    WRITE_FILE_VALUE(outFile, giMonthTypeExtra);
    WRITE_FILE_VALUE(outFile, giWeekType);
    WRITE_FILE_VALUE(outFile, giWeekTypeExtra);
    WRITE_FILE_VALUE(outFile, m_campaignType);
    WRITE_FILE_VALUE(outFile, m_campaignScenario);
    WRITE_FILE_VALUE(outFile, m_campaignDay);
    WRITE_FILE_VALUE(outFile, m_campaignScenariosWon);
    memset(buffer, 0, 0x2c);
    write(outFile, buffer, 0x2c);
    write(outFile, m_mapDescription, sizeof(m_mapDescription));
    WRITE_FILE_VALUE(outFile, m_mapSize);
    WRITE_FILE_VALUE(outFile, m_mapDifficulty);
    write(outFile, m_mapName, sizeof(m_mapName));
    GenerateStandardFileName(m_saveName, buffer);
    write(outFile, buffer, 0x11);
    WRITE_FILE_VALUE(outFile, m_difficulty);
    WRITE_FILE_VALUE(outFile, m_playerCount);
    gSaveCurPlayer = giCurPlayer;
    WRITE_FILE_VALUE(outFile, gSaveCurPlayer);
    WRITE_FILE_VALUE(outFile, m_deadPlayerCount);
    write(outFile, m_playerDead, sizeof(m_playerDead));
    for (iFile = 0; iFile < GAME_PLAYER_COUNT; iFile++) {
        humans[iFile] = gbHumanPlayer[iFile];
        if (m_playerDead[iFile])
            humans[iFile] = 0;
    }
    write(outFile, humans, GAME_PLAYER_COUNT);
    WRITE_FILE_VALUE(outFile, m_day);
    WRITE_FILE_VALUE(outFile, m_week);
    WRITE_FILE_VALUE(outFile, m_month);
    for (iFile = 0; iFile < GAME_PLAYER_COUNT; iFile++)
        m_players[iFile].Write(outFile);
    WriteWorldMap(outFile);
    WRITE_FILE_VALUE(outFile, m_obeliskCount);
    write(outFile, m_heroRecs, sizeof(m_heroRecs));
    write(outFile, m_availableHeroes, sizeof(m_availableHeroes));
    write(outFile, m_castleRecs, sizeof(m_castleRecs));
    write(outFile, m_townOwners, sizeof(m_townOwners));
    write(outFile, m_townBuiltToday, sizeof(m_townBuiltToday));
    write(outFile, m_mines, sizeof(m_mines));
    write(outFile, m_mineOwners, sizeof(m_mineOwners));
    write(outFile, m_randomArtifacts, sizeof(m_randomArtifacts));
    write(outFile, m_boats, sizeof(m_boats));
    write(outFile, m_boatSlots, sizeof(m_boatSlots));
    write(outFile, m_obeliskVisitors, sizeof(m_obeliskVisitors));
    WRITE_FILE_VALUE(outFile, m_ultimateArtifactX);
    WRITE_FILE_VALUE(outFile, m_ultimateArtifactY);
    WRITE_FILE_VALUE(outFile, m_ultimateArtifactId);
    write(outFile, m_mapSounds, sizeof(m_mapSounds));
    write(outFile, m_mapExtra, sizeof(m_mapExtra));
    write(outFile, mapVisited, sizeof(mapVisited));
    close(outFile);
    return 1;
}

// origdata.bin restores the default hero names and blank visibility, and
// the seats are re-dealt to this session's human players.
VA(0x0042cc73, 0x87f)
i16 game::LoadGame(char* filename, i32 origData, i32) {
    i32 junk2;
    i32 numHumans;
    i32 ix;
    i32 oldHandle;
    char pathName[452];
    i8 theHumans[GAME_PLAYER_COUNT];
    i32 nextJunk;
    char buffer[0x2c];

    numHumans = 0;
    gGameOver = 0;
    m_noMapHeroes = 1;
    if (origData || !strcmp(filename, "REMOTE.GAM"))
        sprintf(pathName, "%s%s", gDataPath, filename);
    else
        sprintf(pathName, "%s%s", gGamePath, filename);
    oldHandle = open(pathName, O_BINARY);
    if (oldHandle == -1)
        FileError(pathName);
    ClearMapExtra();
    READ_FILE_VALUE(oldHandle, gbIAmGreatest);
    read(oldHandle, this, 2);
    READ_FILE_VALUE(oldHandle, giMonthType);
    READ_FILE_VALUE(oldHandle, giMonthTypeExtra);
    READ_FILE_VALUE(oldHandle, giWeekType);
    READ_FILE_VALUE(oldHandle, giWeekTypeExtra);
    READ_FILE_VALUE(oldHandle, m_campaignType);
    READ_FILE_VALUE(oldHandle, m_campaignScenario);
    READ_FILE_VALUE(oldHandle, m_campaignDay);
    READ_FILE_VALUE(oldHandle, m_campaignScenariosWon);
    read(oldHandle, buffer, 0x2c);
    read(oldHandle, m_mapDescription, sizeof(m_mapDescription));
    READ_FILE_VALUE(oldHandle, m_mapSize);
    READ_FILE_VALUE(oldHandle, m_mapDifficulty);
    read(oldHandle, m_mapName, sizeof(m_mapName));
    read(oldHandle, m_saveName, 0x11);
    sprintf(m_saveName, filename);
    READ_FILE_VALUE(oldHandle, m_difficulty);
    READ_FILE_VALUE(oldHandle, m_playerCount);
    READ_FILE_VALUE(oldHandle, gSaveCurPlayer);
    giCurPlayer = gSaveCurPlayer;
    READ_FILE_VALUE(oldHandle, m_deadPlayerCount);
    read(oldHandle, m_playerDead, sizeof(m_playerDead));
    read(oldHandle, theHumans, GAME_PLAYER_COUNT);
    for (ix = 0; ix < GAME_PLAYER_COUNT; ix++) {
        if ((theHumans[ix] || giDebugLevel >= GAME_DEBUG_LEVEL_ALL_HUMAN_MIN) && numHumans < giNumHumanPlayers) {
            numHumans++;
            gbHumanPlayer[ix] = 1;
        } else {
            gbHumanPlayer[ix] = 0;
        }
    }
    for (ix = 0; ix < GAME_PLAYER_COUNT; ix++) {
        if (gbHumanPlayer[ix]) {
            if (!gRemoteOn || ix == giThisGamePos)
                gbThisNetHumanPlayer[ix] = 1;
            else
                gbThisNetHumanPlayer[ix] = 0;
        } else {
            gbThisNetHumanPlayer[ix] = 0;
        }
    }
    READ_FILE_VALUE(oldHandle, m_day);
    READ_FILE_VALUE(oldHandle, m_week);
    READ_FILE_VALUE(oldHandle, m_month);
    giCurTurn = GAME_DAY_NUMBER(*this);
    for (ix = 0; ix < GAME_PLAYER_COUNT; ix++)
        m_players[ix].Read(oldHandle);
    ReadWorldMap(oldHandle);
    READ_FILE_VALUE(oldHandle, m_obeliskCount);
    read(oldHandle, m_heroRecs, sizeof(m_heroRecs));
    if (origData) {
        for (ix = 0; ix < GAME_HERO_COUNT; ix++) {
            strcpy(m_heroRecs[ix].m_name, gHeroNames[ix][0]);
            strcpy(m_heroRecs[ix].m_shortName, gHeroNames[ix][1]);
        }
    }
    read(oldHandle, m_availableHeroes, sizeof(m_availableHeroes));
    read(oldHandle, m_castleRecs, sizeof(m_castleRecs));
    read(oldHandle, m_townOwners, sizeof(m_townOwners));
    read(oldHandle, m_townBuiltToday, sizeof(m_townBuiltToday));
    read(oldHandle, m_mines, sizeof(m_mines));
    read(oldHandle, m_mineOwners, sizeof(m_mineOwners));
    read(oldHandle, m_randomArtifacts, sizeof(m_randomArtifacts));
    read(oldHandle, m_boats, sizeof(m_boats));
    read(oldHandle, m_boatSlots, sizeof(m_boatSlots));
    read(oldHandle, m_obeliskVisitors, sizeof(m_obeliskVisitors));
    READ_FILE_VALUE(oldHandle, m_ultimateArtifactX);
    READ_FILE_VALUE(oldHandle, m_ultimateArtifactY);
    READ_FILE_VALUE(oldHandle, m_ultimateArtifactId);
    if (origData) {
        memset(m_mapSounds, MAP_SOUND_NONE, sizeof(m_mapSounds));
        memset(m_mapExtra, 0, sizeof(m_mapExtra));
        memset(mapVisited, 0, sizeof(mapVisited));
        strcpy(gpGame->m_saveName, localization::Tr("save.name.new_game"));
    } else {
        read(oldHandle, m_mapSounds, sizeof(m_mapSounds));
        read(oldHandle, m_mapExtra, sizeof(m_mapExtra));
        read(oldHandle, mapVisited, sizeof(mapVisited));
        if (strcmp(filename, "REMOTE.GAM"))
            strcpy(gpGame->m_saveName, filename);
    }
    close(oldHandle);
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

// Right clicks show help, the player toggles cycle the opponents and OK
// packs the chosen opponents before closing the dialog.
VA(0x0042d4f2, 0x4f3)
H1_ENUM_RETURN(MessageDispatchResult, i16) NewGameHandler(tag_message& message) {
    i32 iPlayer;
    i32 i;
    H1_ENUM_LOCAL(NewGameHelp, i32) helpIndex;
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
                    case NEW_GAME_DIFFICULTY_FIRST + H1_ENUM_ENCODE(GameDifficulty, DIFFICULTY_NORMAL):
                        helpIndex = NEW_GAME_HELP_DIFFICULTY;
                        break;
                    case NEW_GAME_DIFFICULTY_FIRST + H1_ENUM_ENCODE(GameDifficulty, DIFFICULTY_HARD):
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
                if (helpIndex >= NEW_GAME_HELP_FIRST)
                    NormalDialog(gNewGameHelp[helpIndex], NORMAL_DIALOG_TYPE_QUICK_VIEW);
            }
        } else {
            switch (message.command) {
                case WIDGET_NOTIFY_DESELECT:
                    switch (message.id) {
                        case NEW_GAME_OK:
                            gpGame->m_playerCount = 0;
                            for (i = 0; i < GAME_PLAYER_COUNT; i++) {
                                if (H1_ENUM_DECODE(ComputerPlayerType, gpGame->m_players[i].m_difficulty) > PLAYER_TYPE_NONE)
                                    gpGame->m_playerCount++;
                            }
                            if (gpGame->m_playerCount < GAME_MIN_PLAYER_COUNT) {
                                NormalDialog(
                                    localization::Tr("game.opponent.required"),
                                    NORMAL_DIALOG_TYPE_OK,
                                    0xb1,
                                    0x3c
                                );
                                break;
                            } else {
                                if (!gpGame->m_players[1].m_difficulty) {
                                    if (gpGame->m_players[2].m_difficulty) {
                                        gpGame->m_players[1].m_difficulty =
                                            gpGame->m_players[2].m_difficulty;
                                        gpGame->m_players[2].m_difficulty = H1_ENUM_ENCODE(ComputerPlayerType, PLAYER_TYPE_NONE);
                                    } else {
                                        gpGame->m_players[1].m_difficulty =
                                            gpGame->m_players[3].m_difficulty;
                                        gpGame->m_players[3].m_difficulty = H1_ENUM_ENCODE(ComputerPlayerType, PLAYER_TYPE_NONE);
                                    }
                                }
                                if (!gpGame->m_players[2].m_difficulty
                                    && gpGame->m_players[3].m_difficulty) {
                                    gpGame->m_players[2].m_difficulty =
                                        gpGame->m_players[3].m_difficulty;
                                    gpGame->m_players[3].m_difficulty = H1_ENUM_ENCODE(ComputerPlayerType, PLAYER_TYPE_NONE);
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
                        case NEW_GAME_DIFFICULTY_FIRST + H1_ENUM_ENCODE(GameDifficulty, DIFFICULTY_NORMAL):
                        case NEW_GAME_DIFFICULTY_FIRST + H1_ENUM_ENCODE(GameDifficulty, DIFFICULTY_HARD):
                        case NEW_GAME_DIFFICULTY_LAST:
                            gpGame->m_difficulty = H1_ENUM_DECODE(GameDifficulty, message.id - NEW_GAME_DIFFICULTY_FIRST);
                            break;
                        case NEW_GAME_OPPONENT_FIRST:
                        case NEW_GAME_OPPONENT_FIRST + 1:
                        case NEW_GAME_OPPONENT_LAST:
                            iPlayer = message.id - NEW_GAME_OPPONENT_TOGGLE_BASE;
                            gpGame->m_players[iPlayer].m_difficulty++;
                            gpGame->m_players[iPlayer].m_difficulty %= H1_ENUM_ENCODE(ComputerPlayerType, PLAYER_TYPE_COUNT);
                            if (iPlayer < giNumHumanPlayers
                                && !gpGame->m_players[iPlayer].m_difficulty)
                                gpGame->m_players[iPlayer].m_difficulty = H1_ENUM_ENCODE(HumanHandicap, HUMAN_HANDICAP_EASY);
                            break;
                        case NEW_GAME_COLOR:
                            gpGame->m_players[0].m_color = H1_ENUM_DECODE(
                                PlayerColor,
                                (H1_ENUM_ENCODE(PlayerColor, gpGame->m_players[0].m_color) + 1) % GAME_PLAYER_COUNT
                            );
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

// New-game screen: map name, difficulty, opponent types and labels,
// rating, crest and King of the Hill.
VA(0x0042d9e5, 0x293)
void game::UpdateNewGameWindow(void) {
    tag_message message;
    // Also the opponent seat of the frame and label loops.
    H1_ENUM_SHARED(GameDifficulty, i16) i;
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
    for (i = DIFFICULTY_EASY; i < DIFFICULTY_COUNT; i++) {
        message.id = H1_ENUM_ENCODE(GameDifficulty, i) + NEW_GAME_DIFFICULTY_FIRST;
        m_newGameWindow->BroadcastMessage(message);
    }
    message.command = WIDGET_COMMAND_SET_FLAGS;
    message.id = H1_ENUM_ENCODE(GameDifficulty, m_difficulty) + NEW_GAME_DIFFICULTY_FIRST;
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
    sprintf(
        gText,
        "%s %d%%",
        localization::Tr("ui.new_game.difficulty_rating"),
        gpGame->m_difficultyRating
    );
    message.text = gText;
    m_newGameWindow->BroadcastMessage(message);
    message.command = WIDGET_COMMAND_SET_FRAME;
    if (m_players[0].m_color != PLAYER_COLOR_NONE) {
        message.id = NEW_GAME_COLOR;
        message.value =
            H1_ENUM_ENCODE(PlayerColor, m_players[0].m_color) * NEW_GAME_FRAME_CREST_STRIDE + NEW_GAME_FRAME_CREST_BASE;
        m_newGameWindow->BroadcastMessage(message);
    }
    message.command = WIDGET_COMMAND_SET_FRAME;
    message.id = NEW_GAME_KING_OF_THE_HILL;
    message.value = gbIAmGreatest + NEW_GAME_FRAME_KING_OF_THE_HILL_BASE;
    m_newGameWindow->BroadcastMessage(message);
}

// Every unowned town on the map gains a random tier of its own creatures.
VA(0x0042dc78, 0x24c)
void game::GiveTroopsToNeutralTowns(void) {
    i32 howMany;
    i32 die;
    i32 i;
    i32 tierValue;
    H1_ENUM_LOCAL(CreatureType, i32) monster;
    for (i = 0; i < GAME_TOWN_COUNT; i++) {
        if ((m_castleRecs[i].m_x > 0 || m_castleRecs[i].m_y > 0) && m_castleRecs[i].m_owner < 0) {
            die = Random(REINFORCEMENT_ROLL_MIN, REINFORCEMENT_ROLL_MAX);
            if (die <= REINFORCEMENT_TIER_ONE_THRESHOLD) {
                tierValue = REINFORCEMENT_TIER_ONE_KEY;
                howMany =
                    Random(REINFORCEMENT_TIER_ONE_COUNT_MIN, REINFORCEMENT_TIER_ONE_COUNT_MAX);
            } else if (die <= REINFORCEMENT_TIER_TWO_THRESHOLD) {
                tierValue = REINFORCEMENT_TIER_TWO_KEY;
                howMany =
                    Random(REINFORCEMENT_TIER_TWO_COUNT_MIN, REINFORCEMENT_TIER_TWO_COUNT_MAX);
            } else if (die <= REINFORCEMENT_TIER_THREE_THRESHOLD) {
                tierValue = REINFORCEMENT_TIER_THREE_KEY;
                howMany =
                    Random(REINFORCEMENT_TIER_THREE_COUNT_MIN, REINFORCEMENT_TIER_THREE_COUNT_MAX);
            } else {
                tierValue = REINFORCEMENT_TIER_FOUR_KEY;
                howMany =
                    Random(REINFORCEMENT_TIER_FOUR_COUNT_MIN, REINFORCEMENT_TIER_FOUR_COUNT_MAX);
            }
            switch (tierValue + H1_ENUM_ENCODE(TownType, m_castleRecs[i].m_type)) {
                case REINFORCEMENT_TIER_ONE_KEY + H1_ENUM_ENCODE(TownType, TOWN_TYPE_KNIGHT):
                    monster = CREATURE_PEASANT;
                    break;
                case REINFORCEMENT_TIER_TWO_KEY + H1_ENUM_ENCODE(TownType, TOWN_TYPE_KNIGHT):
                    monster = CREATURE_ARCHER;
                    break;
                case REINFORCEMENT_TIER_THREE_KEY + H1_ENUM_ENCODE(TownType, TOWN_TYPE_KNIGHT):
                    monster = CREATURE_PIKEMAN;
                    break;
                case REINFORCEMENT_TIER_FOUR_KEY + H1_ENUM_ENCODE(TownType, TOWN_TYPE_KNIGHT):
                    monster = CREATURE_SWORDSMAN;
                    break;
                case REINFORCEMENT_TIER_ONE_KEY + H1_ENUM_ENCODE(TownType, TOWN_TYPE_BARBARIAN):
                    monster = CREATURE_GOBLIN;
                    break;
                case REINFORCEMENT_TIER_TWO_KEY + H1_ENUM_ENCODE(TownType, TOWN_TYPE_BARBARIAN):
                    monster = CREATURE_ORC;
                    break;
                case REINFORCEMENT_TIER_THREE_KEY + H1_ENUM_ENCODE(TownType, TOWN_TYPE_BARBARIAN):
                    monster = CREATURE_WOLF;
                    break;
                case REINFORCEMENT_TIER_FOUR_KEY + H1_ENUM_ENCODE(TownType, TOWN_TYPE_BARBARIAN):
                    monster = CREATURE_OGRE;
                    break;
                case REINFORCEMENT_TIER_ONE_KEY + H1_ENUM_ENCODE(TownType, TOWN_TYPE_SORCERESS):
                    monster = CREATURE_SPRITE;
                    break;
                case REINFORCEMENT_TIER_TWO_KEY + H1_ENUM_ENCODE(TownType, TOWN_TYPE_SORCERESS):
                    monster = CREATURE_DWARF;
                    break;
                case REINFORCEMENT_TIER_THREE_KEY + H1_ENUM_ENCODE(TownType, TOWN_TYPE_SORCERESS):
                    monster = CREATURE_ELF;
                    break;
                case REINFORCEMENT_TIER_FOUR_KEY + H1_ENUM_ENCODE(TownType, TOWN_TYPE_SORCERESS):
                    monster = CREATURE_DRUID;
                    break;
                case REINFORCEMENT_TIER_ONE_KEY + H1_ENUM_ENCODE(TownType, TOWN_TYPE_WARLOCK):
                    monster = CREATURE_CENTAUR;
                    break;
                case REINFORCEMENT_TIER_TWO_KEY + H1_ENUM_ENCODE(TownType, TOWN_TYPE_WARLOCK):
                    monster = CREATURE_GARGOYLE;
                    break;
                case REINFORCEMENT_TIER_THREE_KEY + H1_ENUM_ENCODE(TownType, TOWN_TYPE_WARLOCK):
                    monster = CREATURE_GRIFFIN;
                    break;
                case REINFORCEMENT_TIER_FOUR_KEY + H1_ENUM_ENCODE(TownType, TOWN_TYPE_WARLOCK):
                    monster = CREATURE_MINOTAUR;
                    break;
            }
            GiveArmy(&m_castleRecs[i].m_army, monster, howMany, ARMY_GROUP_EMPTY_SLOT);
        }
    }
}

// Starts campaigns directly, restores the previous setup choices and falls
// back to a default map when the remembered one does not fit the human
// player count.
VA(0x0042dec4, 0x3e9)
i8 game::NewGame(void) {
    DATA(0x004a6c28)
    static i8 gNewGameSettingsSaved = 0;
    i32 player;
    if (!SetupGame(1))
        return 0;
    if (gCampaignChoice > CAMPAIGN_NONE) {
        InitEntireCampaign(H1_ENUM_ENCODE(CampaignChoice, gCampaignChoice));
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
        for (player = 1; player < giNumHumanPlayers; player++) {
            if (H1_ENUM_DECODE(HumanHandicap, m_players[player].m_difficulty) == HUMAN_HANDICAP_NONE)
                m_players[player].m_difficulty = m_players[0].m_difficulty;
        }
    }
    if (!strnicmp(gMapName, "camp", 4) || (giNumHumanPlayers == 1 && gMapName[4] != '1')
        || (giNumHumanPlayers == GAME_PLAYERS_TWO && gMapName[5] != '2')
        || (giNumHumanPlayers == GAME_PLAYERS_THREE && gMapName[6] != '3')
        || (giNumHumanPlayers == GAME_PLAYERS_FOUR && gMapName[7] != '4')) {
        if (giNumHumanPlayers == 1) {
            strcpy(gMapName, "AES31000.map");
            strcpy(gFullMapName, localization::Tr("scenario.claw.name"));
            strcpy(gMapDescription, localization::Tr("scenario.claw.description"));
            gMapSize = MAP_SIZE_SMALL;
            gMapDifficulty = MAP_DIFFICULTY_EASY;
        } else {
            strcpy(gMapName, "CNM51234.map");
            strcpy(gFullMapName, localization::Tr("scenario.bay.name"));
            strcpy(gMapDescription, localization::Tr("scenario.bay.description"));
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
        NormalDialog(localization::Tr("campaign.restart.confirm"), NORMAL_DIALOG_TYPE_YES_NO);
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

// Reloads origdata.bin first and starts the campaign calendar on day 1.
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

// CAMP%d.CMP maps: the calendar continues from m_campaignDay and the
// scenario table seeds the opponents and every player's resources.
VA(0x0042e595, 0x25b)
void game::InitCampaignMap(i32 scenario, i32) {
    i32 saveTypeValue;
    i32 firstSavedScenario;
    i32 i;
    H1_ENUM_LOCAL(ResourceType, i32) jId;
    i32 savedState;
    i32 savedDay;

    saveTypeValue = m_campaignType;
    firstSavedScenario = m_campaignScenario;
    savedState = m_campaignScenariosWon;
    savedDay = m_campaignDay;
    LoadGame("origdata.bin", 1, 0);
    m_campaignType = saveTypeValue;
    m_campaignScenario = firstSavedScenario;
    m_campaignScenariosWon = savedState;
    m_campaignDay = savedDay;
    m_month = (m_campaignDay - 1) / CALENDAR_DAYS_PER_MONTH + 1;
    m_week =
        (m_campaignDay - 1 - (m_month - 1) * CALENDAR_DAYS_PER_MONTH) / CALENDAR_DAYS_PER_WEEK + 1;
    m_day = (m_campaignDay - 1) % CALENDAR_DAYS_PER_WEEK + 1;
    giCurTurn = GAME_DAY_NUMBER(*this);
    gbIAmGreatest = gCampaignScenarios[scenario].kingOfTheHill;
    giNumHumanPlayers = 0;
    m_players[0].m_difficulty = H1_ENUM_ENCODE(HumanHandicap, HUMAN_HANDICAP_EXPERT);
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
        for (jId = RESOURCE_FIRST; jId < RESOURCE_COUNT; jId++)
            m_players[i].m_resources[jId] = gCampaignScenarios[scenario].resources[i][jId];
    }
}

// Map setup helpers, a starting town and hero per player (campaign crests
// pick them), two tavern heroes, the ultimate artifact site, starting
// resources, town threat ranks and the first neutral garrisons.
VA(0x0042e7f0, 0xe31)
void game::NewMap(char* mapName) {
    i32 nextThreatVal;
    i32 anyFreeValue;
    i8 savedHeroY;
    i8 myPosX;
    i8 yTown;
    i8 xTownVal;
    i32 prevHeroNo;
    i32 i;
    i32 j;
    i8 theTownId;
    i8 theUsed[GAME_TOWN_COUNT];
    i32 k;
    i32 ultimateSpread;
    i8 allNeutralVal;
    // A human seat's handicap less one picks its starting resources.
    H1_ENUM_LOCAL(GameDifficulty, i32) curDifficulty;

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
        m_players[i].m_currentHero = HERO_ID_NONE;
    }
    memset(m_mapExtra, 0, sizeof(m_mapExtra));
    memset(mapVisited, 0, sizeof(mapVisited));
    RandomizeHeroPool();
    strcpy(gMapName, mapName);
    LoadMap(gMapName);
    RandomizeTerrainTiles();
    RandomizePlayerCrests();
    ProcessMapExtra();
    allNeutralVal = SetupTowns();
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
        prevHeroNo = 0;
        if (allNeutralVal) {
            if (m_campaignType <= 0 || m_campaignScenario < CAMPAIGN_SCENARIO_LORD_FIRST
                || m_campaignScenario > CAMPAIGN_SCENARIO_LORD_LAST) {
                if (m_campaignType > 0) {
                    for (j = 0; j < GAME_PLAYER_COUNT; j++) {
                        if (GetTown(j)->m_type == gCrestTownTypes[m_players[i].m_color]) {
                            SetupTown(j, !gbHumanPlayer[i]);
                            ClaimTown(j, i);
                        }
                    }
                } else {
                    theTownId = RandomScan(m_townOwners, 0, GAME_PLAYER_COUNT, 8);
                    if (theTownId == GAME_TABLE_FREE)
                        theTownId = Scan(m_townOwners, 0, GAME_PLAYER_COUNT);
                    SetupTown(theTownId, !gbHumanPlayer[i]);
                    ClaimTown(theTownId, i);
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
                    GetNewHeroId(gTownHeroClass[H1_ENUM_ENCODE(TownType, m_castleRecs[m_players[i].m_townIds[0]].m_type)]);
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
        k = (k + Random(1, 3)) % HERO_CLASS_COUNT;
        m_players[i].m_availableHeroIds[1] = GetNewHeroId(k);
        m_availableHeroes[m_players[i].m_availableHeroIds[1]] = HERO_AVAILABILITY_RETREATED;
    }
    if (!m_noMapHeroes)
        ProcessOnMapHeroes();
    if (m_campaignType <= 0) {
        for (k = 0; k < GAME_PLAYER_COUNT; k++) {
            if (allNeutralVal && m_townOwners[k] == GAME_PLAYER_NONE) {
                xTownVal = m_castleRecs[k].m_x;
                yTown = m_castleRecs[k].m_y;
                for (i = 0; i < TOWN_FOOTPRINT_WIDTH; i++) {
                    m_map[xTownVal - TOWN_FOOTPRINT_LEFT + i][yTown - TOWN_FOOTPRINT_TOP]
                        .m_overlayIndex -= TOWN_CASTLE_FRAME_OFFSET;
                    m_map[xTownVal - TOWN_FOOTPRINT_LEFT + i][yTown - 1].m_objectIndex -=
                        TOWN_CASTLE_FRAME_OFFSET;
                    m_map[xTownVal - TOWN_FOOTPRINT_LEFT + i][yTown].m_objectIndex -=
                        TOWN_CASTLE_FRAME_OFFSET;
                }
                m_castleRecs[k].m_buildings = H1_ENUM_BIT(BuildingSlotType, BUILDING_SLOT_TENT);
                if (m_castleRecs[k].m_type == TOWN_TYPE_BARBARIAN)
                    m_castleRecs[k].m_buildings |= H1_ENUM_BIT(BuildingSlotType, BUILDING_SLOT_SPECIAL);
                SetupTown(k, 0);
            }
        }
    }
    for (i = 0; i < m_playerCount; i++) {
        for (j = 0; j < m_players[i].m_heroCount; j++) {
            myPosX = m_heroRecs[m_players[i].m_heroIds[j]].m_x;
            savedHeroY = m_heroRecs[m_players[i].m_heroIds[j]].m_y;
            m_heroRecs[m_players[i].m_heroIds[j]].m_locationType =
                m_map[myPosX][savedHeroY].m_triggerType;
            m_heroRecs[m_players[i].m_heroIds[j]].m_occupiedTown =
                m_map[myPosX][savedHeroY].m_objectMetadata;
            m_map[myPosX][savedHeroY].m_triggerType = MAP_EVENT_TRIGGER(MAP_OBJECT_HERO);
            m_map[myPosX][savedHeroY].m_objectMetadata = m_players[i].m_heroIds[j];
        }
        if (m_players[i].m_heroCount > 0)
            m_players[i].m_currentHero = m_players[i].m_heroIds[0];
        else if (m_players[i].m_townCount > 0)
            m_players[i].m_currentTown = m_players[i].m_townIds[0];
    }
    i = Random(9, 62);
    j = Random(9, 62);
    ultimateSpread = Random(1, 20) + Random(1, 20) + Random(1, 30);
    while (m_map[i][j].m_objectIndex != MAP_CELL_NO_FRAME
           || m_map[i][j].m_overlayIndex != MAP_CELL_NO_FRAME
           || m_map[i][j].m_tileIndex < MAP_CELL_TILES_PER_TERRAIN
           || (giNumHumanPlayers == 1
               && ultimateSpread >= MANHATTAN_LENGTH(
                      i - m_heroRecs[m_players[0].m_heroIds[0]].m_x,
                      j - m_heroRecs[m_players[0].m_heroIds[0]].m_y
                  ))) {
        ultimateSpread = Random(1, 20) + Random(1, 20) + Random(1, 30);
        i = Random(9, 62);
        j = Random(9, 62);
    }
    m_ultimateArtifactX = i;
    m_ultimateArtifactY = j;
    m_ultimateArtifactId = H1_ENUM_DECODE(
        ArtifactType,
        Random(H1_ENUM_ENCODE(ArtifactType, ARTIFACT_ULTIMATE_BOOK), H1_ENUM_ENCODE(ArtifactType, ARTIFACT_ULTIMATE_LAST))
    );
    for (i = 0; i < m_playerCount; i++) {
        if (gbHumanPlayer[i]) {
            if (i == 0)
                curDifficulty = m_difficulty;
            else
                curDifficulty = H1_ENUM_DECODE(GameDifficulty, m_players[i].m_difficulty - 1);
        } else {
            curDifficulty = DIFFICULTY_EASY;
        }
        memcpy(
            m_players[i].m_resources,
            gStartingResources[curDifficulty],
            sizeof(m_players[i].m_resources)
        );
    }
    memset(theUsed, -1, sizeof(theUsed));
    for (i = 0; i < GAME_TOWN_COUNT; i++) {
        nextThreatVal = 0;
        anyFreeValue = Scan(theUsed, 0, GAME_TOWN_COUNT);
        if (anyFreeValue != GAME_TABLE_FREE)
            nextThreatVal = RandomScan(theUsed, 0, GAME_TOWN_COUNT, GAME_TOWN_COUNT);
        anyFreeValue = nextThreatVal;
        m_castleRecs[i].m_threat = anyFreeValue;
        theUsed[anyFreeValue] = 0;
    }
    for (i = 0; i < 4; i++)
        GiveTroopsToNeutralTowns();
    SetupAdjacentMons();
    gpPhilAI->GetGameAIVars();
    gbInNewGameSetup = 0;
}

// Groups the multi-cell object triggers 0x34-0x37 and 0x38-0x3c by their
// first trigger so neighbouring halves can be compared.
VA(0x0042f621, 0x51)
H1_ENUM_RETURN(MapObjectType, i32) GetObjectFamily(H1_ENUM_PARAM(MapObjectType, i32) trigger) {
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

// Once a cell's object frame is gone, its overlay drops into the object
// slot unless the eastern neighbour continues the same object.
VA(0x0042f672, 0x1be)
void game::SettleOverlay(i32 x, i32 y) {
    mapCell* cell;
    mapCell* cellEast;
    cell = &m_map[x][y];
    if (cell->m_objectIndex == MAP_CELL_NO_FRAME && cell->m_overlayIndex != MAP_CELL_NO_FRAME) {
        switch (cell->m_triggerType) {
            case MAP_OBJECT_TRIGGER(MAP_OBJECT_MOUNTAINS_2):
            case MAP_OBJECT_TRIGGER(MAP_OBJECT_TREES_2):
                if (x + 1 < MAP_CELL_GRID_SIZE) {
                    cellEast = &m_map[x + 1][y];
                    if (GetObjectFamily(MAP_PASSIVE_OBJECT(cellEast->m_triggerType))
                        == GetObjectFamily(MAP_PASSIVE_OBJECT(cell->m_triggerType))) {
                        cell->m_secondaryTrigger |= MAP_CELL_SECONDARY_BLOCKED;
                    } else {
                        cell->m_objectIndex = cell->m_overlayIndex;
                        cell->m_objectTileset = cell->m_overlayTileset;
                        cell->m_overlayTileset = 0;
                        cell->m_overlayIndex = MAP_CELL_NO_FRAME;
                    }
                }
                break;
            case MAP_OBJECT_TRIGGER(MAP_OBJECT_MOUNTAINS_4):
            case MAP_OBJECT_TRIGGER(MAP_OBJECT_TREES_4):
                if (x + 1 < MAP_CELL_GRID_SIZE) {
                    cellEast = &m_map[x + 1][y];
                    if (GetObjectFamily(MAP_PASSIVE_OBJECT(cellEast->m_triggerType))
                        == GetObjectFamily(MAP_PASSIVE_OBJECT(cell->m_triggerType))) {
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

// Numbers sites and obelisks, rolls each event's contents, files town and
// mine ids into their footprints, then settles overlays and the passive
// trigger bits.
VA(0x0042f830, 0xb14)
void game::RandomizeEvents(void) {
    H1_ENUM_LOCAL(MapTileset, u8) theTileset;
    H1_ENUM_LOCAL(MapTileset, u8) nextObjTileset;
    i16 y;
    i16 i;
    i16 j;
    i8 id;
    i32 siteNumIdx;
    i16 x;
    mapCell* nextCell;
    i8 obeliskIdNum;

    obeliskIdNum = 1;
    siteNumIdx = 1;
    for (y = 0; y < MAP_CELL_GRID_SIZE; y++) {
        for (x = 0; x < MAP_CELL_GRID_SIZE; x++) {
            nextCell = &m_map[x][y];
            switch (nextCell->m_triggerType) {
                case MAP_EVENT_TRIGGER(MAP_OBJECT_GAZEBO):
                    nextCell->m_objectMetadata = siteNumIdx;
                    siteNumIdx++;
                    break;
                case MAP_OBJECT_TRIGGER(MAP_OBJECT_WHIRLPOOL):
                    nextCell->m_triggerType |= MAP_TRIGGER_EVENT;
                    break;
                case MAP_EVENT_TRIGGER(MAP_OBJECT_OBELISK):
                    nextCell->m_objectMetadata = obeliskIdNum;
                    obeliskIdNum++;
                    break;
                case MAP_EVENT_TRIGGER(MAP_OBJECT_STATUE):
                    nextCell->m_objectMetadata = MAP_EVENT_DATA_AVAILABLE;
                    break;
                case MAP_EVENT_TRIGGER(MAP_OBJECT_SKELETON):
                    nextCell->m_objectMetadata =
                        Random(0, 9) == RANDOM_DECILE_3 ? SKELETON_ARTIFACT : SKELETON_EMPTY;
                    break;
                case MAP_EVENT_TRIGGER(MAP_OBJECT_DAEMON_CAVE):
                    switch (Random(0, 99) % 10) {
                        case RANDOM_DECILE_0:
                        case RANDOM_DECILE_1:
                        case RANDOM_DECILE_2:
                            nextCell->m_objectMetadata = DAEMON_REWARD_EXPERIENCE;
                            break;
                        case RANDOM_DECILE_3:
                            nextCell->m_objectMetadata = DAEMON_REWARD_ARTIFACT;
                            break;
                        case RANDOM_DECILE_4:
                        case RANDOM_DECILE_5:
                        case RANDOM_DECILE_6:
                            nextCell->m_objectMetadata = DAEMON_REWARD_EXPERIENCE_GOLD;
                            break;
                        case RANDOM_DECILE_7:
                        case RANDOM_DECILE_8:
                        case RANDOM_DECILE_9:
                            nextCell->m_objectMetadata = DAEMON_REWARD_RANSOM;
                            break;
                    }
                    break;
                case MAP_EVENT_TRIGGER(MAP_OBJECT_TREASURE_CHEST):
                    nextCell->m_objectMetadata = Random(2, 4);
                    break;
                case MAP_EVENT_TRIGGER(MAP_OBJECT_CAMPFIRE):
                    nextCell->m_objectMetadata = Random(4, 6) << CAMPFIRE_AMOUNT_SHIFT;
                    nextCell->m_objectMetadata |= static_cast<i8>(Random(0, 5));
                    break;
                case MAP_EVENT_TRIGGER(MAP_OBJECT_ANCIENT_LAMP):
                    nextCell->m_objectMetadata = Random(0, 3) + 2;
                    break;
                case MAP_EVENT_TRIGGER(MAP_OBJECT_SHIPWRECK):
                    if (x <= 0 || x >= MAP_CELL_GRID_SIZE - 1
                        || MAP_TRIGGER_OBJECT(m_map[x - 1][y].m_triggerType)
                               != MAP_OBJECT_SHIPWRECK) {
                        nextCell->m_triggerType &= MAP_TRIGGER_TYPE_MASK;
                        break;
                    }
                    goto treasure;
                case MAP_EVENT_TRIGGER(MAP_OBJECT_GRAVEYARD):
                treasure:
                    switch (Random(0, 99) % 10) {
                        case RANDOM_DECILE_0:
                        case RANDOM_DECILE_1:
                        case RANDOM_DECILE_2:
                            nextCell->m_objectMetadata = GHOST_SITE_SMALL;
                            break;
                        case RANDOM_DECILE_3:
                        case RANDOM_DECILE_4:
                        case RANDOM_DECILE_5:
                            nextCell->m_objectMetadata = GHOST_SITE_MEDIUM;
                            break;
                        case RANDOM_DECILE_6:
                        case RANDOM_DECILE_7:
                        case RANDOM_DECILE_8:
                            nextCell->m_objectMetadata = GHOST_SITE_LARGE;
                            break;
                        case RANDOM_DECILE_9:
                            nextCell->m_objectMetadata = GHOST_SITE_HUGE;
                            break;
                    }
                    break;
                case MAP_EVENT_TRIGGER(MAP_OBJECT_STRAW_HUT):
                    nextCell->m_objectMetadata = Random(10, 30);
                    break;
                case MAP_EVENT_TRIGGER(MAP_OBJECT_HOUSE):
                    nextCell->m_objectMetadata = Random(20, 50);
                    break;
                case MAP_EVENT_TRIGGER(MAP_OBJECT_CABIN):
                    nextCell->m_objectMetadata = Random(0, 127) % 4 + 1;
                    break;
                case MAP_EVENT_TRIGGER(MAP_OBJECT_DWARF_LOG_CABIN):
                    nextCell->m_objectMetadata = Random(0, 98) % 3 + 1;
                    break;
                case MAP_EVENT_TRIGGER(MAP_OBJECT_PEASANT_LOG_CABIN):
                    nextCell->m_objectMetadata = Random(20, 50);
                    break;
                case MAP_EVENT_TRIGGER(MAP_OBJECT_WATERWHEEL):
                    nextCell->m_objectMetadata = MAP_EVENT_DATA_AVAILABLE;
                    break;
                case MAP_EVENT_TRIGGER(MAP_OBJECT_MONSTER):
                    if (!nextCell->m_objectMetadata) {
                        nextCell->m_objectMetadata =
                            GetRandomNumTroops(H1_ENUM_DECODE(CreatureType, nextCell->m_objectIndex));
                        if (Random(0, 99) <= 25
                            && H1_ENUM_DECODE(CreatureType, nextCell->m_objectIndex) != CREATURE_GHOST)
                            nextCell->m_objectMetadata |= MONSTER_WILLING_FLAG;
                    }
                    break;
                case MAP_EVENT_TRIGGER(MAP_OBJECT_RESOURCE):
                    nextCell->m_objectMetadata = nextCell->m_objectIndex;
                    if (nextCell->m_objectIndex > 4)
                        nextCell->m_objectMetadata -= RESOURCE_PILE_OBJECT_BASE;
                    switch (H1_ENUM_DECODE(ResourceType, nextCell->m_objectMetadata)) {
                        case RESOURCE_WOOD:
                        case RESOURCE_ORE:
                            nextCell->m_objectMetadata = Random(8, 16);
                            break;
                        case RESOURCE_GOLD:
                            nextCell->m_objectMetadata = Random(5, 10);
                            break;
                        default:
                            nextCell->m_objectMetadata = Random(3, 7);
                            break;
                    }
                    break;
                case MAP_EVENT_TRIGGER(MAP_OBJECT_SPELL_SHRINE):
                    switch (Random(0, 9)) {
                        case RANDOM_DECILE_0:
                        case RANDOM_DECILE_1:
                        case RANDOM_DECILE_2:
                        case RANDOM_DECILE_3:
                            nextCell->m_objectMetadata =
                                H1_ENUM_ENCODE(SpellType, gMageGuildSpellPool[MAGE_GUILD_STATE_LEVEL_1][Random(0, 7)]) + 1;
                            break;
                        case RANDOM_DECILE_4:
                        case RANDOM_DECILE_5:
                        case RANDOM_DECILE_6:
                        case RANDOM_DECILE_7:
                            nextCell->m_objectMetadata =
                                H1_ENUM_ENCODE(SpellType, gMageGuildSpellPool[MAGE_GUILD_STATE_LEVEL_2][Random(0, 7)]) + 1;
                            break;
                        default:
                            nextCell->m_objectMetadata =
                                H1_ENUM_ENCODE(SpellType, gMageGuildSpellPool[MAGE_GUILD_STATE_LEVEL_3][Random(0, 7)]) + 1;
                            break;
                    }
                    break;
                case MAP_EVENT_TRIGGER(MAP_OBJECT_DESERT_TENT):
                    nextCell->m_objectMetadata = Random(10, 20);
                    break;
                case MAP_EVENT_TRIGGER(MAP_OBJECT_WAGON_CAMP):
                    if (x <= 0 || x >= MAP_CELL_GRID_SIZE - 1
                        || MAP_TRIGGER_OBJECT(m_map[x - 1][y].m_triggerType)
                               != MAP_OBJECT_WAGON_CAMP
                        || MAP_TRIGGER_OBJECT(m_map[x + 1][y].m_triggerType)
                               != MAP_OBJECT_WAGON_CAMP) {
                        nextCell->m_triggerType &= MAP_TRIGGER_TYPE_MASK;
                        break;
                    }
                    nextCell->m_objectMetadata = Random(30, 50);
                    break;
                case MAP_EVENT_TRIGGER(MAP_OBJECT_ARTIFACT):
                    switch (Random(0, 99) % 10) {
                        case RANDOM_DECILE_0:
                        case RANDOM_DECILE_1:
                        case RANDOM_DECILE_2:
                        case RANDOM_DECILE_3:
                        case RANDOM_DECILE_4:
                        case RANDOM_DECILE_5:
                            nextCell->m_objectMetadata = ARTIFACT_EVENT_MODE_PICKUP;
                            break;
                        case RANDOM_DECILE_6:
                        case RANDOM_DECILE_7:
                            nextCell->m_objectMetadata = ARTIFACT_EVENT_MODE_GUARDED;
                            break;
                        case RANDOM_DECILE_8:
                        case RANDOM_DECILE_9:
                            nextCell->m_objectMetadata = ARTIFACT_EVENT_MODE_GOLD;
                            break;
                    }
                    break;
                case MAP_EVENT_TRIGGER(MAP_OBJECT_TOWN):
                    id = GetTownId(x, y);
                    for (j = 0; j < TOWN_FOOTPRINT_HEIGHT; j++) {
                        for (i = 0; i < TOWN_FOOTPRINT_WIDTH; i++) {
                            if (!m_map[x - TOWN_FOOTPRINT_LEFT + i][y - TOWN_FOOTPRINT_TOP + j]
                                     .m_objectMetadata)
                                m_map[x - TOWN_FOOTPRINT_LEFT + i][y - TOWN_FOOTPRINT_TOP + j]
                                    .m_objectMetadata = id;
                        }
                    }
                    SetupTown(id, 0);
                    break;
                case MAP_EVENT_TRIGGER(MAP_OBJECT_ALCHEMIST_LAB):
                case MAP_EVENT_TRIGGER(MAP_OBJECT_MINE):
                case MAP_EVENT_TRIGGER(MAP_OBJECT_SAWMILL):
                    id = GetMineId(x, y);
                    for (j = 0; j < MINE_FOOTPRINT_HEIGHT; j++) {
                        for (i = 0; i < MINE_FOOTPRINT_WIDTH; i++) {
                            if (!m_map[x + i][y - j].m_objectMetadata
                                || (m_map[x + i][y - j].m_triggerType & MAP_TRIGGER_TYPE_MASK)
                                       == (nextCell->m_triggerType & MAP_TRIGGER_TYPE_MASK))
                                m_map[x + i][y - j].m_objectMetadata = id;
                        }
                    }
                    break;
                case MAP_EVENT_TRIGGER(MAP_OBJECT_WINDMILL):
                    nextCell->m_objectMetadata = Random(1, 5);
                    break;
            }
        }
    }
    for (y = 0; y < MAP_CELL_GRID_SIZE; y++) {
        for (x = 0; x < MAP_CELL_GRID_SIZE; x++) {
            nextCell = &m_map[x][y];
            if (nextCell->m_objectIndex != MAP_CELL_NO_FRAME
                && nextCell->m_overlayIndex != MAP_CELL_NO_FRAME) {
                nextObjTileset = H1_ENUM_DECODE(MapTileset, nextCell->m_objectTileset & MAP_CELL_TILESET_MASK);
                theTileset = H1_ENUM_DECODE(MapTileset, nextCell->m_overlayTileset & MAP_CELL_TILESET_MASK);
                if ((nextObjTileset == TILESET_MTN32 || nextObjTileset == TILESET_TREE32)
                    && (theTileset == TILESET_MTN32 || theTileset == TILESET_TREE32))
                    nextCell->m_secondaryTrigger |= MAP_CELL_SECONDARY_BLOCKED;
            }
            SettleOverlay(x, y);
            if (x == 0 || y == 0 || x == MAP_CELL_GRID_SIZE - 1 || y == MAP_CELL_GRID_SIZE - 1) {
                switch (nextCell->m_triggerType) {
                    case MAP_OBJECT_TRIGGER(MAP_OBJECT_MOUNTAINS):
                    case MAP_OBJECT_TRIGGER(MAP_OBJECT_MOUNTAINS_2):
                    case MAP_OBJECT_TRIGGER(MAP_OBJECT_MOUNTAINS_3):
                    case MAP_OBJECT_TRIGGER(MAP_OBJECT_MOUNTAINS_4):
                    case MAP_OBJECT_TRIGGER(MAP_OBJECT_TREES):
                    case MAP_OBJECT_TRIGGER(MAP_OBJECT_TREES_2):
                    case MAP_OBJECT_TRIGGER(MAP_OBJECT_TREES_3):
                    case MAP_OBJECT_TRIGGER(MAP_OBJECT_TREES_4):
                    case MAP_OBJECT_TRIGGER(MAP_OBJECT_TREES_5):
                        nextCell->m_secondaryTrigger |= MAP_CELL_SECONDARY_BLOCKED;
                        break;
                }
            }
        }
    }
    for (y = 0; y < MAP_CELL_GRID_SIZE; y++) {
        for (x = 0; x < MAP_CELL_GRID_SIZE; x++) {
            nextCell = &m_map[x][y];
            if (nextCell->m_triggerType == MAP_OBJECT_TRIGGER(MAP_OBJECT_SHADOW))
                nextCell->m_flags |= MAP_CELL_OBJECT_SHADOW_ONLY;
            if (nextCell->m_triggerType & MAP_TRIGGER_EVENT) {
                switch (MAP_TRIGGER_OBJECT(nextCell->m_triggerType)) {
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
                        nextCell->m_triggerType -= MAP_TRIGGER_EVENT;
                        break;
                }
            }
        }
    }
}

// .MAP files: an optional old header, the world map, town and mine
// records, artifacts, obelisks, sounds and (from version 1112) the map
// extras.
VA(0x00430344, 0x3c6)
i16 game::LoadMap(char* filename) {
    void* msg;
    i16 width;
    i8 y;
    i16 height;
    i16 i;
    i32 handle;
    i8 x;
    i8 type;
    i32 wasReserved;
    i16 theVersion;

    sprintf(gText, "%s%s", gMapPath, filename);
    handle = open(gText, O_BINARY);
    if (handle == -1)
        FileError(gText);
    READ_FILE_VALUE(handle, theVersion);
    if (theVersion == MAP_HEADER_ID) {
        msg = malloc(sizeof(SMapHeader));
        read(handle, msg, sizeof(SMapHeader) - sizeof(theVersion));
        READ_FILE_VALUE(handle, theVersion);
        free(msg);
    }
    READ_FILE_VALUE(handle, width);
    READ_FILE_VALUE(handle, height);
    ReadWorldMap(handle);
    for (i = 0; i < GAME_TOWN_COUNT; i++) {
        READ_FILE_VALUE(handle, x);
        READ_FILE_VALUE(handle, y);
        READ_FILE_VALUE(handle, type);
        if (x >= 0) {
            m_castleRecs[i].m_x = x;
            m_castleRecs[i].m_y = y;
            m_castleRecs[i].m_type = H1_ENUM_DECODE(TownType, type & MAP_TOWN_TYPE_MASK);
            if (H1_ENUM_DECODE(TownType, type & MAP_TOWN_TYPE_MASK) == TOWN_TYPE_BARBARIAN)
                m_castleRecs[i].m_buildings |= H1_ENUM_BIT(BuildingSlotType, BUILDING_SLOT_SPECIAL);
            if (type < 0)
                m_castleRecs[i].m_buildings |= H1_ENUM_BIT(BuildingSlotType, BUILDING_SLOT_CASTLE);
            else
                m_castleRecs[i].m_buildings |= H1_ENUM_BIT(BuildingSlotType, BUILDING_SLOT_TENT);
        }
    }
    for (i = 0; i < GAME_MINE_COUNT; i++) {
        READ_FILE_VALUE(handle, x);
        READ_FILE_VALUE(handle, y);
        READ_FILE_VALUE(handle, type);
        if (x >= 0) {
            m_mines[i].x = x;
            m_mines[i].y = y;
            m_mines[i].type = H1_ENUM_DECODE(ResourceType, type);
        }
    }
    read(handle, m_randomArtifacts, sizeof(m_randomArtifacts));
    READ_FILE_VALUE(handle, m_obeliskCount);
    read(handle, m_mapSounds, sizeof(m_mapSounds));
    if (theVersion >= MAP_EXTRA_VERSION) {
        READ_FILE_VALUE(handle, iMaxMapExtra);
        for (i = 1; i < iMaxMapExtra; i++) {
            READ_FILE_VALUE(handle, pwSizeOfMapExtra[i]);
            ppMapExtra[i] = malloc(pwSizeOfMapExtra[i]);
            read(handle, ppMapExtra[i], pwSizeOfMapExtra[i]);
        }
    } else {
        iMaxMapExtra = 1;
    }
    close(handle);
    return 0;
}

VA(0x0043070a, 0x29b)
void game::ClaimTown(i8 townId, i8 player) {
    i32 j;
    town* townRec;
    mapCell* cell;

    townRec = &m_castleRecs[townId];
    if (townRec->m_owner == player)
        return;
    if (m_townOwners[townId] != GAME_PLAYER_NONE)
        gpGame->GetTown(townId)->Deallocate();
    for (j = 0; j < ARMY_GROUP_SLOT_COUNT; ++j) {
        townRec->m_army.m_creatureTypes[j] = CREATURE_NONE;
        townRec->m_army.m_creatureCounts[j] = 0;
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
    cell->m_objectTileset |= H1_ENUM_ENCODE(MapTileset, TILESET_FLAG32) << MAP_CELL_EXTRA_TILESET_SHIFT;
    cell->m_extraFrame = H1_ENUM_ENCODE(PlayerColor, m_players[player].Color()) * 2;
    cell = &m_map[m_castleRecs[townId].m_x + 1][m_castleRecs[townId].m_y];
    cell->m_flags |= MAP_CELL_OBJECT_EXTRA;
    cell->m_objectTileset |= H1_ENUM_ENCODE(MapTileset, TILESET_FLAG32) << MAP_CELL_EXTRA_TILESET_SHIFT;
    cell->m_extraFrame = H1_ENUM_ENCODE(PlayerColor, m_players[player].Color()) * 2 + 1;
    SetVisibility(m_castleRecs[townId].m_x, m_castleRecs[townId].m_y, player, gVisRangeTown);
    CheckEndGame(0);
}

// Flag placement: the flag cell sits beside the mine by type and shows the
// owner's colour frame.
VA(0x004309a5, 0x24a)
void game::ClaimMine(i8 mineId, i8 player) {
    i16 frame;
    mapCell* cellPtr;
    m_mines[mineId].owner = player;
    m_mineOwners[mineId] = player;
    switch (H1_ENUM_ENCODE(ResourceType, m_mines[mineId].type)) {
        case H1_ENUM_ENCODE(MapObjectType, MAP_OBJECT_DRAGON_CITY):
            frame = 0x14;
            break;
        case H1_ENUM_ENCODE(MapObjectType, MAP_OBJECT_LIGHTHOUSE):
            frame = 0x18;
            break;
        case H1_ENUM_ENCODE(ResourceType, RESOURCE_WOOD):
            frame = 0x10;
            break;
        case H1_ENUM_ENCODE(ResourceType, RESOURCE_MERCURY):
            frame = 0xc;
            break;
        default:
            frame = 8;
            break;
    }
    switch (H1_ENUM_ENCODE(ResourceType, m_mines[mineId].type)) {
        case H1_ENUM_ENCODE(ResourceType, RESOURCE_MERCURY):
            cellPtr = &m_map[m_mines[mineId].x][m_mines[mineId].y - 2];
            break;
        case H1_ENUM_ENCODE(MapObjectType, MAP_OBJECT_DRAGON_CITY):
            cellPtr = &m_map[m_mines[mineId].x - 1][m_mines[mineId].y - 3];
            break;
        case H1_ENUM_ENCODE(MapObjectType, MAP_OBJECT_LIGHTHOUSE):
            cellPtr = &m_map[m_mines[mineId].x - 2][m_mines[mineId].y];
            break;
        default:
            cellPtr = &m_map[m_mines[mineId].x][m_mines[mineId].y - 1];
            break;
    }
    if (player == GAME_PLAYER_NONE) {
        cellPtr->m_flags ^= MAP_CELL_OVERLAY_EXTRA;
    } else {
        cellPtr->m_flags |= MAP_CELL_OVERLAY_EXTRA;
        cellPtr->m_overlayTileset |= H1_ENUM_ENCODE(MapTileset, TILESET_FLAG32) << MAP_CELL_EXTRA_TILESET_SHIFT;
        cellPtr->m_extraFrame = frame + H1_ENUM_ENCODE(PlayerColor, m_players[player].Color());
    }
}

// Combat (0) and adventure (1) books each have their own window position;
// type 2 shows both tabs.
VA(0x00430bef, 0x23b)
H1_ENUM_RETURN(SpellType, i8) game::ViewSpells(
    class hero* spellHero,
    H1_ENUM_PARAM(HeroSpellType, i8) spellType,
    H1_ENUM_RETURN(MessageDispatchResult, i16) (*callback)(struct tag_message&),
    i8 readOnly
) {
    tag_message msg;

    m_viewSpell = SPELL_NONE;
    i16 winX[3] = {177, 97, 177};
    i16 winY[3] = {100, 47, 100};
    if (!spellHero->GetNumSpells(spellType)) {
        NormalDialog(localization::Tr("spell.none.available"), NORMAL_DIALOG_TYPE_OK);
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
            SET_WIDGET_MESSAGE(
                msg,
                WIDGET_COMMAND_CLEAR_FLAGS,
                spellType == SPELL_TYPE_COMBAT ? SPELL_BOOK_ADVENTURE_SPELLS
                                               : SPELL_BOOK_COMBAT_SPELLS
            );
            msg.value = WIDGET_FLAG_ENABLED | WIDGET_FLAG_DRAW;
            m_viewSpellsWindow->BroadcastMessage(msg);
        }
        UpdateSpellWidgets();
        gpWindowManager->DoDialog(m_viewSpellsWindow, ViewSpellsHandler, 0);
        delete m_viewSpellsWindow;
    }
    return m_viewSpell;
}

// Combat spells fill hero slots 0..18 and adventure spells 19..28; the
// page ends at the last memorized slot.
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

// Four-spell page: each slot shows the spell icon and its name with the
// remaining casts.
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
            message.value = H1_ENUM_ENCODE(SpellType, m_viewSpellsHero->m_spells[m_viewSpellsTop + i]);
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

// Right clicks describe a spell or control, left clicks page, switch books
// or pick the spell to cast.
VA(0x0043109e, 0x494)
H1_ENUM_RETURN(MessageDispatchResult, i16) ViewSpellsHandler(tag_message& message) {
    H1_ENUM_LOCAL(SpellType, i32) spell;
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
                            spell = gpGame->m_viewSpellsHero->m_spells
                                        [gpGame->m_viewSpellsTop
                                         + (message.id - SPELL_BOOK_ENTRY_FIRST)];
                            NormalDialog(
                                gSpellDesc[spell],
                                NORMAL_DIALOG_TYPE_QUICK_VIEW,
                                -1,
                                -1,
                                NORMAL_DIALOG_SPELL,
                                H1_ENUM_ENCODE(SpellType, spell)
                            );
                            break;
                        case SPELL_BOOK_PREVIOUS_PAGE:
                            NormalDialog(
                                gSpellHelp[SPELL_HELP_PREVIOUS_PAGE],
                                NORMAL_DIALOG_TYPE_QUICK_VIEW
                            );
                            break;
                        case SPELL_BOOK_NEXT_PAGE:
                            NormalDialog(
                                gSpellHelp[SPELL_HELP_NEXT_PAGE],
                                NORMAL_DIALOG_TYPE_QUICK_VIEW
                            );
                            break;
                        case SPELL_BOOK_ADVENTURE_SPELLS:
                            NormalDialog(
                                gSpellHelp[SPELL_HELP_ADVENTURE_SPELLS],
                                NORMAL_DIALOG_TYPE_QUICK_VIEW
                            );
                            break;
                        case SPELL_BOOK_COMBAT_SPELLS:
                            NormalDialog(
                                gSpellHelp[SPELL_HELP_COMBAT_SPELLS],
                                NORMAL_DIALOG_TYPE_QUICK_VIEW
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
                                            [gpGame->m_viewSpellsTop
                                             + (message.id - SPELL_BOOK_ENTRY_FIRST)];
                                NormalDialog(
                                    gSpellDesc[spell],
                                    NORMAL_DIALOG_TYPE_OK,
                                    -1,
                                    -1,
                                    NORMAL_DIALOG_SPELL,
                                    H1_ENUM_ENCODE(SpellType, spell)
                                );
                                return MESSAGE_DISPATCH_CONSUME;
                            }
                            gpGame->m_viewSpell = gpGame->m_viewSpellsHero->m_spells
                                                      [gpGame->m_viewSpellsTop
                                                       + (message.id - SPELL_BOOK_ENTRY_FIRST)];
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
                if (gpWindowManager->m_lastHoverId == message.id)
                    return MESSAGE_DISPATCH_CONSUME;
                else
                    return gpGame->m_viewSpellsCallback(message);
                break;
        }
        if (message.id == H1_ENUM_ENCODE(BaseWidgetCommand, WIDGET_COMMAND_DIALOG_SELECT)) {
            message.command = H1_ENUM_DECODE(BaseWidgetCommand, message.id);
            return MESSAGE_DISPATCH_FORWARD;
        }
    }
    return MESSAGE_DISPATCH_CONSUME;
}

// Hovering a spell-book control shows its help line in the hero screen's
// status bar.
VA(0x00431532, 0x129)
H1_ENUM_RETURN(MessageDispatchResult, i16) ViewSpecialHandler(tag_message& message) {
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

VA(0x0043165b, 0x8af)
void game::ViewArmy(
    i16 x,
    i16 y,
    H1_ENUM_PARAM(CreatureType, i8) monsterType,
    i16 numTroops,
    class town* castle,
    i8 disableDismiss,
    H1_ENUM_PARAM(ArmyFacing, i8) facing,
    i8 quickView,
    class hero* theHero,
    class army* theArmy,
    class armyGroup* theGroup
) {
    char localText[12];
    i32 shotCountNo;
    i16 baseX;
    i16 spacing;
    i16 posY;
    i16 activeId;
    i32 myMorale;
    tag_monsterInfo* monsterInfoObj;
    i16 idIndex;
    i32 m;
    char* statText;
    i32 savedLuck;
    tag_message origMessage;
    char iconNameBuffer[16];
    iconWidget* theMonsterWidget;
    i16 note;
    i16 curTitleLabel;
    i32 oldMod;
    i16 blankBtn;
    char theName[13];

    baseX = 86;
    posY = 164;
    blankBtn = VIEW_ARMY_COUNT_FRAME;
    idIndex = VIEW_ARMY_COUNT_TEXT;
    curTitleLabel = VIEW_ARMY_TITLE;
    note = VIEW_ARMY_STATS;
    activeId = VIEW_ARMY_ANIMATION;
    origMessage.type = MESSAGE_WIDGET;

    if (monsterType != CREATURE_SWORDSMAN)
        strcpy(iconNameBuffer, gArmySpriteNames[monsterType]);
    else
        strcpy(iconNameBuffer, "swrdsman");
    monsterInfoObj = &gMonsterDatabase[monsterType];
    m_viewArmyWindow = new heroWindow(x, y, "armywin.bin");
    if (!m_viewArmyWindow)
        MemError();
    spacing = 30;
    if (monsterInfoObj->stats.attributes & MONSTER_FLAGS_WIDE) {
        switch (facing) {
            case ARMY_FACING_RIGHT:
                spacing += 43;
                break;
            case ARMY_FACING_LEFT:
                spacing += 119;
                break;
        }
    } else {
        spacing += facing == ARMY_FACING_LEFT ? 76 : 86;
    }
    if (monsterInfoObj->stats.attributes & MONSTER_FLAGS_FLYING)
        sprintf(theName, "%s.wlk", iconNameBuffer);
    else
        sprintf(theName, "%s.wip", iconNameBuffer);
    theMonsterWidget = new iconWidget(
        spacing,
        164,
        86,
        149,
        theName,
        0,
        H1_ENUM_DECODE(IconDrawOrientation, facing == ARMY_FACING_LEFT),
        VIEW_ARMY_ANIMATION,
        ICON_WIDGET_DRAW,
        1
    );
    if (!theMonsterWidget)
        MemError();
    m_viewArmyWindow->AddWidget(theMonsterWidget, WINDOW_Z_ORDER_APPEND);

    strcpy(theName, gArmyNames[monsterType]);
    theName[0] = CyrillicToUpper(theName[0]);
    origMessage.command = WIDGET_COMMAND_SET_TEXT;
    origMessage.id = VIEW_ARMY_TITLE;
    origMessage.text = theName;
    m_viewArmyWindow->BroadcastMessage(origMessage);

    statText = static_cast<char*>(malloc(VIEW_ARMY_STAT_TEXT_SIZE));
    if (theGroup)
        myMorale = theGroup->GetMorale(theHero, castle);
    else
        myMorale = 0;
    sprintf(statText, "");

    oldMod = 0;
    sprintf(gText, "%s%d", gArmyStatText[0], monsterInfoObj->stats.attack);
    strcat(statText, gText);
    if (theHero)
        oldMod += theHero->m_primaryStats[HERO_PRIMARY_ATTACK];
    if (oldMod) {
        sprintf(gText, " (%d)", monsterInfoObj->stats.attack + oldMod);
        strcat(statText, gText);
    }

    oldMod = 0;
    sprintf(gText, "\n%s%d", gArmyStatText[1], monsterInfoObj->stats.defense);
    strcat(statText, gText);
    if (theHero)
        oldMod += theHero->m_primaryStats[HERO_PRIMARY_DEFENSE];
    if (theArmy && theArmy->m_spellEffect == SPELL_PROTECTION)
        oldMod += 3;
    if (oldMod) {
        sprintf(gText, " (%d)", monsterInfoObj->stats.defense + oldMod);
        strcat(statText, gText);
    }

    if (monsterInfoObj->stats.attributes & MONSTER_FLAGS_SHOOTER) {
        if (theArmy)
            shotCountNo = theArmy->m_stats.shots;
        else
            shotCountNo = monsterInfoObj->stats.shots;
        if (shotCountNo > 0) {
            if (gpCombatManager->m_active == 1)
                sprintf(gText, "\n%s%d", gArmyStatText[2], shotCountNo);
            else
                sprintf(gText, "\n%s%d", gArmyStatText[8], shotCountNo);
            strcat(statText, gText);
        }
    }

    sprintf(gText, "\n%s%d", gArmyStatText[3], monsterInfoObj->stats.damageMin);
    strcat(statText, gText);
    if (monsterInfoObj->stats.damageMin != monsterInfoObj->stats.damageMax) {
        sprintf(gText, "-%d", monsterInfoObj->stats.damageMax);
        strcat(statText, gText);
    }
    sprintf(gText, "\n%s%d", gArmyStatText[4], monsterInfoObj->stats.hitPoints);
    strcat(statText, gText);
    sprintf(gText, "\n%s%s", gArmyStatText[5], gSpeedText[monsterInfoObj->stats.speed]);
    strcat(statText, gText);
    sprintf(gText, "\n%s%s", gArmyStatText[6], gMoraleText[myMorale + 3]);
    strcat(statText, gText);
    savedLuck = GetLuck(theHero, theArmy);
    sprintf(gText, "\n%s%s", gArmyStatText[7], gLuckText[savedLuck + 3]);
    strcat(statText, gText);

    origMessage.id = VIEW_ARMY_STATS;
    origMessage.text = statText;
    m_viewArmyWindow->BroadcastMessage(origMessage);
    if (disableDismiss) {
        origMessage.command = WIDGET_COMMAND_CLEAR_FLAGS;
        origMessage.value = WIDGET_FLAG_ENABLED | WIDGET_FLAG_DRAW;
        origMessage.id = VIEW_ARMY_DISMISS;
        m_viewArmyWindow->BroadcastMessage(origMessage);
    }
    if (quickView) {
        origMessage.command = WIDGET_COMMAND_CLEAR_FLAGS;
        origMessage.value = WIDGET_FLAG_ENABLED | WIDGET_FLAG_DRAW;
        origMessage.id = VIEW_ARMY_CLOSE;
        m_viewArmyWindow->BroadcastMessage(origMessage);
    }
    if (numTroops < 1) {
        origMessage.command = WIDGET_COMMAND_CLEAR_FLAGS;
        origMessage.value = WIDGET_FLAG_ENABLED | WIDGET_FLAG_DRAW;
        origMessage.id = VIEW_ARMY_COUNT_FRAME;
        m_viewArmyWindow->BroadcastMessage(origMessage);
        origMessage.id = VIEW_ARMY_COUNT_TEXT;
        m_viewArmyWindow->BroadcastMessage(origMessage);
    } else {
        sprintf(localText, "%d", numTroops);
        origMessage.command = WIDGET_COMMAND_SET_TEXT;
        origMessage.id = VIEW_ARMY_COUNT_TEXT;
        origMessage.text = localText;
        m_viewArmyWindow->BroadcastMessage(origMessage);
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
            for (m = 0; m < ARMY_GROUP_SLOT_COUNT; m++) {
                if (theGroup->m_creatureTypes[m] == monsterType) {
                    theGroup->m_creatureTypes[m] = CREATURE_NONE;
                    theGroup->m_creatureCounts[m] = 0;
                }
            }
        }
    }
    free(statText);
    delete m_viewArmyWindow;
}

VA(0x00431f0a, 0x17f)
H1_ENUM_RETURN(MessageDispatchResult, i16) ViewArmyHandler(tag_message& message) {
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
                            localization::Tr("army.dismiss.confirm"),
                            NORMAL_DIALOG_TYPE_YES_NO,
                            0xb1,
                            0x36
                        );
                        if (gpWindowManager->m_dialogResult == NORMAL_DIALOG_CONFIRM) {
                            gbDismissArmy = 1;
                            message.command = H1_ENUM_DECODE(
                                BaseWidgetCommand,
                                message.id = H1_ENUM_ENCODE(BaseWidgetCommand, WIDGET_COMMAND_DIALOG_SELECT)
                            );
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
    if (glTimers[VIEW_ARMY_TIMER_SLOT] < KBTickCount()) {
        SET_WIDGET_MESSAGE(message, WIDGET_COMMAND_SET_FRAME, VIEW_ARMY_ANIMATION);
        gpGame->m_viewArmyResult++;
        message.value = gpGame->m_viewArmyResult % VIEW_ARMY_ANIMATION_FRAMES;
        gpGame->m_viewArmyWindow->BroadcastMessage(message);
        gpGame->m_viewArmyWindow->DrawWindow();
        glTimers[VIEW_ARMY_TIMER_SLOT] = KBTickCount() + VIEW_ARMY_FRAME_DELAY;
    }
    return MESSAGE_DISPATCH_CONSUME;
}

// Kingdom overview: heroes by class, castles and towns by type and mines by
// resource drawn onto the backdrop, then the date, income and resources.
VA(0x00432089, 0xc81)
void game::Overview(void) {
    i16 activeTextW;
    i16 mineRowYVal;
    i8 redrawRequested;
    i16 firstIncomeWidget;
    i16 numTowns;
    i16 heroRowY;
    i16 theFrame;
    i16 localFirstTown;
    i16 left;
    i16 nextMineW;
    heroWindow* baseWin;
    tag_message activeMessage;
    i16 oldGy;
    i16 oldBadgeY;
    i16 numCastlesVal;
    i16 curHeroTextW;
    i16 mineBaseValue;
    i16 activeBadge;
    i16 lineH;
    icon* ovIconRef;
    i16 newHeroTextH;
    i16 townTop;
    // Counts heroes, hero classes, towns, town types, mines and resources in
    // turn.
    i16 j;
    font* smallFontItem;
    // Per hero class, then per town type.
    i16 theTotals[H1_ENUM_ENCODE(TownType, TOWN_TYPE_COUNT)];
    i16 activeTownTextWPos;
    i16 dayIdYValue;
    i16 spacing;
    i16 theVal;
    i16 one;
    font* bigFont;
    H1_ENUM_ARRAY(i8, mineNumsBuffer, ResourceType, RESOURCE_COUNT);
    i16 spare1;
    i16 newHeroNumYW;
    i16 allLimitYOff;
    i16 shieldDXXVal;
    i16 classCountYVal;
    i16 oldNextType;
    i16 selCastleFrameY;
    i16 nextCastleIconY;
    i16 numMines;
    i16 savedFieldH;

    gpAdvManager->TrimLoopingSounds(8);
    gOverviewShowing = 1;
    oldGy = 82;
    shieldDXXVal = 49;
    spare1 = 73;
    curHeroTextW = 33;
    newHeroTextH = 38;
    activeTextW = 132;
    savedFieldH = 80;
    activeTownTextWPos = 132;
    theVal = 80;
    nextMineW = 72;
    oldBadgeY = 66;
    heroRowY = 32;
    newHeroNumYW = 67;
    nextCastleIconY = 113;
    townTop = 201;
    mineRowYVal = 289;
    theFrame = 0;
    selCastleFrameY = 4;
    localFirstTown = 8;
    mineBaseValue = 12;
    activeBadge = 15;
    lineH = 16;
    allLimitYOff = 544;
    redrawRequested = 1;
    one = 1;
    dayIdYValue = 64;
    firstIncomeWidget = 65;

    gpMouseManager->SetPointer("advmice.mse", ADVENTURE_POINTER_DEFAULT);
    bigFont = gpResourceManager->GetFont("bigfont.fnt");
    smallFontItem = gpResourceManager->GetFont("smalfont.fnt");
    gpWindowManager->FadeScreen(WINDOW_FADE_OUT, WINDOW_FADE_STEPS_SHORT, NULL);
    gpResourceManager->GetBackdropAtLoc("overmain.bmp", gpWindowManager->m_screen, 96, 0);
    sprintf(gText, "overban%01d.bmp", gpCurPlayer->m_color);
    gpResourceManager->GetBackdropAtLoc(gText, gpWindowManager->m_screen, 0, 0);
    ovIconRef = gpResourceManager->GetIcon("overview.icn");

    memset(theTotals, 0, sizeof(theTotals));
    for (j = 0; j < gpCurPlayer->m_heroCount; j++)
        theTotals[m_heroRecs[gpCurPlayer->m_heroIds[j]].m_heroClass]++;
    classCountYVal = 0;
    for (j = 0; j < HERO_CLASS_COUNT; j++) {
        if (theTotals[j])
            classCountYVal++;
    }
    spacing = 136;
    left = 121;
    oldNextType = 0;
    for (j = 0; j < classCountYVal; j++) {
        while (!theTotals[oldNextType])
            oldNextType++;
        ovIconRef->DrawToBuffer(
            left + spacing * j,
            32,
            oldNextType,
            ICON_DRAW_NORMAL,
            ICON_DRAW_OFFSET_FULL
        );
        ovIconRef->DrawToBuffer(
            left + spacing * j + 49,
            67,
            15,
            ICON_DRAW_NORMAL,
            ICON_DRAW_OFFSET_FULL
        );
        sprintf(gText, "%d", theTotals[oldNextType]);
        bigFont
            ->DrawBoundedString(gText, left + spacing * j + 48, 77, 33, 16, 1, FONT_ALIGN_CENTER);
        oldNextType++;
    }

    memset(theTotals, 0, sizeof(theTotals));
    for (j = 0; j < gpCurPlayer->m_townCount; j++) {
        if (m_castleRecs[gpCurPlayer->m_townIds[j]].m_buildings & H1_ENUM_BIT(BuildingSlotType, BUILDING_SLOT_CASTLE))
            theTotals[H1_ENUM_ENCODE(TownType, m_castleRecs[gpCurPlayer->m_townIds[j]].m_type)]++;
    }
    numCastlesVal = 0;
    for (j = 0; j < H1_ENUM_ENCODE(TownType, TOWN_TYPE_COUNT); j++) {
        if (theTotals[j])
            numCastlesVal++;
    }
    if (numCastlesVal) {
        spacing = 136;
        left = 100;
        oldNextType = 0;
        for (j = 0; j < numCastlesVal; j++) {
            while (!theTotals[oldNextType])
                oldNextType++;
            ovIconRef->DrawToBuffer(
                left + spacing * j,
                113,
                oldNextType + 4,
                ICON_DRAW_NORMAL,
                ICON_DRAW_OFFSET_FULL
            );
            sprintf(gText, "%d", theTotals[oldNextType]);
            bigFont
                ->DrawBoundedString(gText, left + spacing * j, 173, 132, 16, 1, FONT_ALIGN_CENTER);
            oldNextType++;
        }
    }

    memset(theTotals, 0, sizeof(theTotals));
    for (j = 0; j < gpCurPlayer->m_townCount; j++) {
        if (!(m_castleRecs[gpCurPlayer->m_townIds[j]].m_buildings & H1_ENUM_BIT(BuildingSlotType, BUILDING_SLOT_CASTLE)))
            theTotals[H1_ENUM_ENCODE(TownType, m_castleRecs[gpCurPlayer->m_townIds[j]].m_type)]++;
    }
    numTowns = 0;
    for (j = 0; j < H1_ENUM_ENCODE(TownType, TOWN_TYPE_COUNT); j++) {
        if (theTotals[j])
            numTowns++;
    }
    if (numTowns) {
        spacing = 136;
        left = 100;
        oldNextType = 0;
        for (j = 0; j < numTowns; j++) {
            while (!theTotals[oldNextType])
                oldNextType++;
            ovIconRef->DrawToBuffer(
                left + spacing * j,
                201,
                oldNextType + 8,
                ICON_DRAW_NORMAL,
                ICON_DRAW_OFFSET_FULL
            );
            sprintf(gText, "%d", theTotals[oldNextType]);
            bigFont
                ->DrawBoundedString(gText, left + spacing * j, 261, 132, 16, 1, FONT_ALIGN_CENTER);
            oldNextType++;
        }
    }

    memset(mineNumsBuffer, 0, sizeof(mineNumsBuffer));
    for (j = MINE_SLOT_STANDARD_FIRST; j < GAME_MINE_COUNT; j++) {
        if (m_mineOwners[j] == giCurPlayer)
            mineNumsBuffer[m_mines[j].type]++;
    }
    numMines = 0;
    for (j = 0; j < H1_ENUM_ENCODE(ResourceType, RESOURCE_COUNT); j++) {
        if (mineNumsBuffer[H1_ENUM_DECODE(ResourceType, j)])
            numMines++;
    }
    if (numMines) {
        spacing = 77;
        left = 100;
        oldNextType = 0;
        for (j = 0; j < numMines; j++) {
            while (!mineNumsBuffer[H1_ENUM_DECODE(ResourceType, oldNextType)])
                oldNextType++;
            ovIconRef->DrawToBuffer(
                left + spacing * j,
                289,
                __min(oldNextType, 2) + 12,
                ICON_DRAW_NORMAL,
                ICON_DRAW_OFFSET_FULL
            );
            if (oldNextType >= 2)
                ovIconRef->DrawToBuffer(
                    left + spacing * j,
                    289,
                    oldNextType + 14,
                    ICON_DRAW_NORMAL,
                    ICON_DRAW_OFFSET_FULL
                );
            sprintf(gText, "%d", mineNumsBuffer[H1_ENUM_DECODE(ResourceType, oldNextType)]);
            bigFont
                ->DrawBoundedString(gText, left + spacing * j, 355, 72, 16, 1, FONT_ALIGN_CENTER);
            oldNextType++;
        }
    }

    gpWindowManager->UpdateScreenRegion(0, 0, LOGICAL_SCREEN_WIDTH, LOGICAL_SCREEN_HEIGHT);
    baseWin = new heroWindow(0, 0, "overwind.bin");
    if (!baseWin)
        MemError();
    SetWinText(baseWin, WINDOW_TEXT_OVERVIEW);
    SET_WIDGET_MESSAGE(activeMessage, WIDGET_COMMAND_SET_TEXT, OVERVIEW_DATE);
    sprintf(gText, gOverviewText[0], m_month, m_week, m_day);
    activeMessage.text = gText;
    baseWin->BroadcastMessage(activeMessage);
    activeMessage.id = OVERVIEW_DAILY_GOLD;
    sprintf(gText, "%d", ComputeDailyGold(giCurPlayer));
    baseWin->BroadcastMessage(activeMessage);
    for (j = 0; j < H1_ENUM_ENCODE(ResourceType, RESOURCE_COUNT); j++) {
        sprintf(gText, "%d", gpCurPlayer->m_resources[H1_ENUM_DECODE(ResourceType, j)]);
        activeMessage.id = j + OVERVIEW_RESOURCE_BASE;
        baseWin->BroadcastMessage(activeMessage);
    }
    gpWindowManager->AddWindow(baseWin, WINDOW_Z_ORDER_APPEND, 1);
    baseWin->DrawWindow();
    gText[0] = 0;
    if (m_mineOwners[MINE_SLOT_DRAGON_CITY] == giCurPlayer) {
        strcpy(gText, gOverviewText[1]);
        smallFontItem->DrawBoundedString(gText, 100, 450, 400, 12, 1, FONT_ALIGN_LEFT);
        gpWindowManager->UpdateScreenRegion(100, 450, 400, 12);
    }
    if (m_mineOwners[MINE_SLOT_LIGHTHOUSE] == giCurPlayer) {
        strcpy(gText, gOverviewText[2]);
        smallFontItem->DrawBoundedString(gText, 100, 465, 400, 12, 1, FONT_ALIGN_LEFT);
        gpWindowManager->UpdateScreenRegion(100, 465, 400, 12);
    }
    gpWindowManager->FadeScreen(WINDOW_FADE_IN, WINDOW_FADE_STEPS_SHORT, NULL);
    gpWindowManager->DoDialog(baseWin, TrueFalseDialogHandler, 0);
    delete baseWin;
    gpWindowManager->FadeScreen(WINDOW_FADE_OUT, WINDOW_FADE_STEPS_SHORT, NULL);
    gpResourceManager->Dispose(ovIconRef);
    gpResourceManager->Dispose(smallFontItem);
    gpResourceManager->Dispose(bigFont);
    gOverviewShowing = 0;
}

// Covers the 28 creatures.
VA(0x00432d0a, 0x25d)
i8 game::GetRandomNumTroops(H1_ENUM_PARAM(CreatureType, i8) monsterType) {
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
void game::TurnOffAIMusic(void) {}

// Autosaves, advances to the next living player (a new day after the
// last), restores hero movement (none on the campaign's goal town) and hands
// the turn to the computer or the human.
VA(0x00432f8c, 0x520)
void game::NextPlayer(void) {
    hero* currentHero;
    i32 numHumans;
    i32 ii;
    i32 remoteVal;
    char unused[20];

    gCurHourGlassPhase = 0;
    if (gbThisNetHumanPlayer[giCurPlayer] && gConfig.autosave) {
        numHumans = 0;
        for (ii = 0; ii < GAME_PLAYER_COUNT; ii++) {
            if (!m_playerDead[ii] && gbHumanPlayer[ii])
                numHumans++;
        }
        SaveGame(localization::Tr("save.name.autosave"), 1);
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
    for (ii = 0; ii < m_players[giCurPlayer].m_heroCount; ii++) {
        currentHero = &m_heroRecs[m_players[giCurPlayer].m_heroIds[ii]];
        currentHero->m_mobility = currentHero->CalcMobility();
        if (m_campaignType > 0
            && currentHero->m_x == gCampaignScenarios[m_campaignScenario].victoryTownX
            && currentHero->m_y == gCampaignScenarios[m_campaignScenario].victoryTownY)
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
        if (gRemoteOn && (gbHumanPlayer[giCurPlayer] || giThisGamePos != giHostGamePos)) {
            if (!gbHumanPlayer[giCurPlayer])
                remoteVal = giHostGamePos;
            else
                remoteVal = giCurPlayer;
            if (!gpGame->TransmitSaveGame(remoteVal, 0))
                ShutDown(NULL);
        }
        if (giBottomViewOverride == BOTTOM_VIEW_OVERRIDE_DISABLED)
            giBottomViewOverride = BOTTOM_VIEW_NONE;
    } else {
        SetNoDialogMenus(1);
        gpInputManager->Flush();
        if (gbBlackoutPlayer && giNumHumanPlayers > 1) {
            sprintf(
                gText,
                localization::Tr("turn.player.prompt"),
                gColorNames[gpGame->m_players[giCurPlayer].m_color]
            );
            gText[0] = CyrillicToUpper(gText[0]);
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

// The first mine and gold mines pay 1000, towns 250 (castles 1000), three
// treasure artifacts add more, and computer players' gold scales with their
// level.
VA(0x004334ac, 0x222)
i32 game::ComputeDailyGold(i32 player) {
    i32 dailyGold;
    i32 index;
    dailyGold = 0;
    if (m_mines[MINE_SLOT_DRAGON_CITY].owner == player)
        dailyGold += DAILY_GOLD_DRAGON_CITY;
    for (index = MINE_SLOT_STANDARD_FIRST; index < GAME_MINE_COUNT; index++) {
        if (m_mines[index].owner == player && m_mines[index].type == RESOURCE_GOLD)
            dailyGold += DAILY_GOLD_MINE;
    }
    for (index = 0; index < GAME_TOWN_COUNT; index++) {
        if (m_castleRecs[index].m_owner == player) {
            dailyGold += static_cast<i16>(
                (m_castleRecs[index].m_buildings & H1_ENUM_BIT(BuildingSlotType, BUILDING_SLOT_TENT)) ? DAILY_GOLD_TOWN
                                                                              : DAILY_GOLD_CASTLE
            );
        }
    }
    dailyGold += m_players[player].NumOfGivenArtifact(ARTIFACT_ENDLESS_SACK_OF_GOLD)
                 * DAILY_GOLD_ENDLESS_SACK;
    dailyGold +=
        m_players[player].NumOfGivenArtifact(ARTIFACT_ENDLESS_BAG_OF_GOLD) * DAILY_GOLD_ENDLESS_BAG;
    dailyGold += m_players[player].NumOfGivenArtifact(ARTIFACT_ENDLESS_PURSE_OF_GOLD)
                 * DAILY_GOLD_ENDLESS_PURSE;
    if (!gbHumanPlayer[player]) {
        if (H1_ENUM_DECODE(ComputerPlayerType, gpGame->m_players[player].m_difficulty) == PLAYER_TYPE_DUMB)
            dailyGold = dailyGold * 0.75;
        if (H1_ENUM_DECODE(ComputerPlayerType, gpGame->m_players[player].m_difficulty) == PLAYER_TYPE_AVERAGE)
            dailyGold = dailyGold * 1.0;
        if (H1_ENUM_DECODE(ComputerPlayerType, gpGame->m_players[player].m_difficulty) == PLAYER_TYPE_SMART)
            dailyGold = dailyGold * 1.29;
        if (H1_ENUM_DECODE(ComputerPlayerType, gpGame->m_players[player].m_difficulty) == PLAYER_TYPE_GENIUS)
            dailyGold = dailyGold * 1.45;
    }
    return dailyGold;
}

// Records each player's income, pays the mines, towns and computer
// bonuses, then advances the calendar.
VA(0x004336ce, 0x4ce)
void game::PerDay(void) {
    i16 i;
    i16 theProduction;
    i16 ii;
    H1_ENUM_LOCAL(ResourceType, i16) j;
    H1_ENUM_LOCAL(ResourceType, i8) curResource;

    for (i = 0; i < gpGame->m_playerCount; i++) {
        for (j = RESOURCE_FIRST; j < RESOURCE_COUNT; j++)
            gpGame->m_players[i].m_aiData.m_income[j] = -m_players[i].m_resources[j];
    }
    memset(m_townBuiltToday, 0, sizeof(m_townBuiltToday));
    for (i = MINE_SLOT_STANDARD_FIRST; i < GAME_MINE_COUNT; i++) {
        if (m_mines[i].owner != GAME_PLAYER_NONE) {
            curResource = m_mines[i].type;
            theProduction = 0;
            if (curResource == RESOURCE_ORE)
                theProduction = DAILY_MINE_YIELD_WOOD_ORE;
            else if (curResource == RESOURCE_WOOD)
                theProduction = DAILY_MINE_YIELD_WOOD_ORE;
            else if (curResource != RESOURCE_GOLD)
                theProduction = DAILY_MINE_YIELD_OTHER;
            if (curResource != RESOURCE_GOLD)
                m_players[m_mines[i].owner].m_resources[curResource] += theProduction;
        }
    }
    for (i = 0; i < GAME_TOWN_COUNT; i++)
        m_castleRecs[i].m_turnsOwned++;
    for (i = 0; i < m_playerCount; i++)
        m_players[i].m_resources[RESOURCE_GOLD] += ComputeDailyGold(i);
    for (i = 0; i < m_playerCount; i++) {
        if (!gbHumanPlayer[i]) {
            if (H1_ENUM_DECODE(ComputerPlayerType, gpGame->m_players[i].m_difficulty) > PLAYER_TYPE_NO_WOOD_ORE_BONUS_LAST) {
                m_players[i].m_resources[RESOURCE_WOOD]++;
                m_players[i].m_resources[RESOURCE_ORE]++;
            }
            if (H1_ENUM_DECODE(ComputerPlayerType, gpGame->m_players[i].m_difficulty) > PLAYER_TYPE_NO_WEEKDAY_BONUS_LAST
                && m_day >= 1 && m_day <= H1_ENUM_ENCODE(ResourceType, RESOURCE_NON_GOLD_END))
                m_players[i].m_resources[H1_ENUM_DECODE(ResourceType, m_day - 1)] += 1;
        }
    }
    m_day++;
    giCurTurn = GAME_DAY_NUMBER(*this);
    if (m_day > CALENDAR_DAYS_PER_WEEK) {
        m_day = 1;
        PerWeek();
    }
    if (m_week > CALENDAR_WEEKS_PER_MONTH) {
        m_week = 1;
        PerMonth();
    }
    for (i = 0; i < gpGame->m_playerCount; i++) {
        for (j = RESOURCE_FIRST; j < RESOURCE_COUNT; j++)
            gpGame->m_players[i].m_aiData.m_income[j] += m_players[i].m_resources[j];
    }
}

// Rolls the week, grows every dwelling (computer towns grow faster),
// refreshes the tavern heroes and restocks the map's renewable sites.
VA(0x00433b9c, 0x7c2)
void game::PerWeek(void) {
    i16 posY;
    i16 posX;
    town* townPointer;
    // Also the tavern-hero slot of the hero refresh.
    H1_ENUM_SHARED(BuildingSlotType, i16) j;
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
            if (townPointer->m_buildings & H1_ENUM_BIT(BuildingSlotType, j)) {
                i16 gain = gMonsterDatabase[gDwellingType[townPointer->m_type]
                                                         [j - BUILDING_SLOT_DWELLING_FIRST]]
                               .growth;
                if (townPointer->m_buildings & H1_ENUM_BIT(BuildingSlotType, BUILDING_SLOT_WELL))
                    gain += WEEKLY_WELL_GROWTH_BONUS;
                if (townPointer->m_owner >= 0 && !gbHumanPlayer[townPointer->m_owner]) {
                    if (H1_ENUM_DECODE(ComputerPlayerType, gpGame->m_players[townPointer->m_owner].m_difficulty) == PLAYER_TYPE_SMART)
                        gain = gain * 1.24;
                    if (H1_ENUM_DECODE(ComputerPlayerType, gpGame->m_players[townPointer->m_owner].m_difficulty) == PLAYER_TYPE_GENIUS)
                        gain = gain * 1.36;
                }
                if (giWeekType == CALENDAR_PERIOD_CREATURE
                    && gDwellingType[townPointer->m_type][j - BUILDING_SLOT_DWELLING_FIRST]
                           == H1_ENUM_DECODE(CreatureType, giWeekTypeExtra))
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
                case MAP_EVENT_TRIGGER(MAP_OBJECT_WATERWHEEL):
                    if (m_map[posX][posY].m_objectMetadata != WEEKLY_WATER_WHEEL_EMPTY)
                        m_map[posX][posY].m_objectMetadata = WEEKLY_WATER_WHEEL_GOLD;
                    break;
                case MAP_EVENT_TRIGGER(MAP_OBJECT_WINDMILL):
                    m_map[posX][posY].m_objectMetadata = Random(1, 5);
                    break;
                case MAP_EVENT_TRIGGER(MAP_OBJECT_STRAW_HUT):
                    if (m_map[posX][posY].m_objectMetadata < WEEKLY_SITE_STOCK_LIMIT)
                        m_map[posX][posY].m_objectMetadata += Random(3, 6);
                    break;
                case MAP_EVENT_TRIGGER(MAP_OBJECT_HOUSE):
                    if (m_map[posX][posY].m_objectMetadata < WEEKLY_SITE_STOCK_LIMIT)
                        m_map[posX][posY].m_objectMetadata += Random(5, 10);
                    break;
                case MAP_EVENT_TRIGGER(MAP_OBJECT_CABIN):
                    if (m_map[posX][posY].m_objectMetadata < WEEKLY_SITE_STOCK_LIMIT)
                        m_map[posX][posY].m_objectMetadata += Random(2, 4);
                    break;
                case MAP_EVENT_TRIGGER(MAP_OBJECT_DWARF_LOG_CABIN):
                    if (m_map[posX][posY].m_objectMetadata < WEEKLY_SITE_STOCK_LIMIT)
                        m_map[posX][posY].m_objectMetadata += Random(2, 4);
                    break;
                case MAP_EVENT_TRIGGER(MAP_OBJECT_PEASANT_LOG_CABIN):
                    if (m_map[posX][posY].m_objectMetadata < WEEKLY_SITE_STOCK_LIMIT)
                        m_map[posX][posY].m_objectMetadata += Random(5, 10);
                    break;
                case MAP_EVENT_TRIGGER(MAP_OBJECT_DESERT_TENT):
                    if (m_map[posX][posY].m_objectMetadata < WEEKLY_SITE_STOCK_LIMIT)
                        m_map[posX][posY].m_objectMetadata += Random(1, 3);
                    break;
                case MAP_EVENT_TRIGGER(MAP_OBJECT_WAGON_CAMP):
                    if (m_map[posX][posY].m_objectMetadata < WEEKLY_SITE_STOCK_LIMIT)
                        m_map[posX][posY].m_objectMetadata += Random(3, 6);
                    break;
                default:
                    break;
            }
        }
    }
    m_week++;
    GiveTroopsToNeutralTowns();
}

// Six dwellings: a normal, creature or plague month, the creature month
// also seeding wandering monsters.
VA(0x0043435e, 0x2c0)
void game::PerMonth(void) {
    DATA(0x0048fcbc)
    static H1_ENUM_STORAGE(CreatureType, i8) gMonType[12] = {
        CREATURE_PEASANT, CREATURE_GOBLIN, CREATURE_DWARF, CREATURE_ELF, CREATURE_OGRE, CREATURE_DRUID,
        CREATURE_ORC, CREATURE_WOLF, CREATURE_CENTAUR, CREATURE_GARGOYLE, CREATURE_UNICORN, CREATURE_GRIFFIN
    };
    mapCell* spot;
    i32 x;
    i16 i;
    i32 y;
    i16 growth;
    town* townPointer;
    H1_ENUM_LOCAL(BuildingSlotType, i16) j;

    m_month++;
    i = Random(1, 10);
    if (i <= 5) {
        giMonthType = CALENDAR_PERIOD_NORMAL;
        giMonthTypeExtra = Random(0, CALENDAR_MONTH_NAME_COUNT - 1);
    } else if (i <= 9) {
        giMonthType = CALENDAR_PERIOD_CREATURE;
        giMonthTypeExtra = H1_ENUM_ENCODE(CreatureType, gMonType[Random(0, CALENDAR_MONTH_CREATURE_COUNT - 1)]);
    } else {
        giMonthType = CALENDAR_PERIOD_PLAGUE;
    }
    for (i = 0; i < GAME_TOWN_COUNT; i++) {
        for (j = BUILDING_SLOT_DWELLING_FIRST; j <= BUILDING_SLOT_DWELLING_LAST; j++) {
            townPointer = GetTown(i);
            if (townPointer->m_buildings & H1_ENUM_BIT(BuildingSlotType, j)) {
                growth = gMonsterDatabase[gDwellingType[townPointer->m_type]
                                                       [j - BUILDING_SLOT_DWELLING_FIRST]]
                             .growth;
                if (townPointer->m_buildings & H1_ENUM_BIT(BuildingSlotType, BUILDING_SLOT_WELL))
                    growth += WEEKLY_WELL_GROWTH_BONUS;
                if (giMonthType == CALENDAR_PERIOD_CREATURE
                    && gDwellingType[townPointer->m_type][j - BUILDING_SLOT_DWELLING_FIRST]
                           == H1_ENUM_DECODE(CreatureType, giMonthTypeExtra))
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
                if (!spot->m_triggerType && TERRAIN_IS_LAND(CELL_TERRAIN(spot))) {
                    if (Random(0, MONTH_CREATURE_SPAWN_ROLL_MAX) == MONTH_CREATURE_SPAWN_ROLL_HIT) {
                        spot->m_triggerType = MAP_EVENT_TRIGGER(MAP_OBJECT_MONSTER);
                        spot->m_objectTileset = H1_ENUM_ENCODE(MapTileset, TILESET_MONS32);
                        spot->m_objectIndex = giMonthTypeExtra;
                        spot->m_objectMetadata = GetRandomNumTroops(H1_ENUM_DECODE(CreatureType, giMonthTypeExtra));
                    }
                }
            }
        }
    }
    gpAdvManager->CompleteDraw(0);
}

// 4x3 town footprint: the town type comes from the campaign crest, a
// distinct roll for the first four towns or a plain roll, and shifts every
// town frame to that type.
VA(0x0043461e, 0x569)
void game::RandomizeTown(i8 x, i8 y, i8 isCastle) {
    i8 j;
    i8 curUnique;
    town* town;
    i8 i;
    u8 activeFrameShift;
    i8 townNum;
    H1_ENUM_LOCAL(TownType, i8) race;
    i8 plain;

    townNum = GetTownId(x, y);
    for (j = 0; j < TOWN_FOOTPRINT_HEIGHT; j++) {
        for (i = 0; i < TOWN_FOOTPRINT_WIDTH; i++) {
            if (MAP_TRIGGER_OBJECT(m_map[x - TOWN_FOOTPRINT_LEFT + i][y - TOWN_FOOTPRINT_TOP + j].m_triggerType)
                    > MAP_OBJECT_NONE
                && MAP_TRIGGER_OBJECT(m_map[x - TOWN_FOOTPRINT_LEFT + i][y - TOWN_FOOTPRINT_TOP + j].m_triggerType)
                       <= MAP_OBJECT_EVENT_LAST) {
                m_map[x - TOWN_FOOTPRINT_LEFT + i][y - TOWN_FOOTPRINT_TOP + j].m_secondaryTrigger |=
                    MAP_OBJECT_TRIGGER(MAP_OBJECT_TOWN);
            } else {
                m_map[x - TOWN_FOOTPRINT_LEFT + i][y - TOWN_FOOTPRINT_TOP + j].m_triggerType =
                    MAP_OBJECT_TRIGGER(MAP_OBJECT_TOWN);
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
        curUnique = 0;
        race = TOWN_TYPE_KNIGHT;
        while (!curUnique) {
            race = H1_ENUM_DECODE(
                TownType,
                Random(H1_ENUM_ENCODE(TownType, TOWN_TYPE_KNIGHT), H1_ENUM_ENCODE(TownType, TOWN_TYPE_WARLOCK))
            );
            curUnique = 1;
            for (i = 0; i < GAME_PLAYER_COUNT; i++) {
                if (gRandomTownTypes[i] == race)
                    curUnique = 0;
            }
        }
        gRandomTownTypes[townNum] = race;
    } else {
        race = H1_ENUM_DECODE(
            TownType,
            Random(H1_ENUM_ENCODE(TownType, TOWN_TYPE_KNIGHT), H1_ENUM_ENCODE(TownType, TOWN_TYPE_WARLOCK))
        );
    }
    activeFrameShift = (TOWN_TYPE_COUNT - race) * TOWN_RACE_FRAME_STRIDE;
    for (i = 0; i < TOWN_FOOTPRINT_WIDTH; i++) {
        m_map[x - TOWN_FOOTPRINT_LEFT + i][y - TOWN_FOOTPRINT_TOP].m_overlayIndex -=
            activeFrameShift;
        m_map[x - TOWN_FOOTPRINT_LEFT + i][y - 1].m_objectIndex -= activeFrameShift;
        m_map[x - TOWN_FOOTPRINT_LEFT + i][y].m_objectIndex -= activeFrameShift;
    }
    m_castleRecs[townNum].m_type = race;
    plain = 1;
    if (town->m_extraIndex >= 1
        && static_cast<mapTownExtra*>(ppMapExtra[town->m_extraIndex])->customized)
        plain = 0;
    if (plain) {
        m_castleRecs[townNum].m_buildings =
            race == TOWN_TYPE_BARBARIAN ? H1_ENUM_BIT(BuildingSlotType, BUILDING_SLOT_SPECIAL) : 0;
    }
    if (isCastle) {
        m_castleRecs[townNum].m_buildings |=
            (H1_ENUM_BIT(BuildingSlotType, BUILDING_SLOT_CASTLE) | H1_ENUM_BIT(BuildingSlotType, BUILDING_SLOT_DWELLING_1));
        m_castleRecs[townNum].m_garrison[0] = gMonsterDatabase[gDwellingType[race][0]].growth;
        if (m_castleRecs[townNum].m_buildings & H1_ENUM_BIT(BuildingSlotType, BUILDING_SLOT_TENT))
            m_castleRecs[townNum].m_buildings -= H1_ENUM_BIT(BuildingSlotType, BUILDING_SLOT_TENT);
    } else {
        m_castleRecs[townNum].m_buildings |= H1_ENUM_BIT(BuildingSlotType, BUILDING_SLOT_TENT);
        if (m_castleRecs[townNum].m_buildings & H1_ENUM_BIT(BuildingSlotType, BUILDING_SLOT_CASTLE))
            m_castleRecs[townNum].m_buildings -= H1_ENUM_BIT(BuildingSlotType, BUILDING_SLOT_CASTLE);
        SetupTown(townNum, 0);
    }
}

// SetupTowns' per-town tail: default dwellings for towns the map leaves
// uncustomized, then nine distinct mage-guild spells; computer owners
// favour the stronger spells.
VA(0x00434b87, 0x318)
void game::SetupTown(i8 townId, i8 aiOwned) {
    i16 dwellingCount;
    char rollList[10];
    i32 n;
    H1_ENUM_ARRAY(i8, nextUsed, SpellType, SPELL_COUNT);
    H1_ENUM_LOCAL(TownType, i8) curTownType;
    H1_ENUM_LOCAL(SpellType, i16) newSpell;
    i16 spellValue;
    i32 spellLevel;

    rollList[0] = 1;
    rollList[1] = 1;
    rollList[2] = 1;
    rollList[3] = 2;
    rollList[4] = 1;
    rollList[5] = 1;
    rollList[6] = 1;
    rollList[7] = 2;
    rollList[8] = 1;
    rollList[9] = 3;
    dwellingCount = rollList[Random(0, 99) / 10];
    curTownType = m_castleRecs[townId].m_type;
    if (m_castleRecs[townId].m_customized) {
        for (n = 0; n < BUILDING_SLOT_DWELLING_COUNT; n++) {
            if (m_castleRecs[townId].m_buildings & H1_ENUM_BIT(BuildingSlotType, n + BUILDING_SLOT_DWELLING_FIRST))
                m_castleRecs[townId].m_garrison[n] =
                    gMonsterDatabase[gDwellingType[curTownType][n]].growth;
        }
    }
    if (!m_castleRecs[townId].m_customized) {
        m_castleRecs[townId].m_buildings |= H1_ENUM_BIT(BuildingSlotType, BUILDING_SLOT_DWELLING_1);
        m_castleRecs[townId].m_garrison[0] = gMonsterDatabase[gDwellingType[curTownType][0]].growth;
        if (aiOwned && dwellingCount == 1 && Random(1, 10) < 4)
            dwellingCount++;
        if (--dwellingCount) {
            m_castleRecs[townId].m_buildings |= H1_ENUM_BIT(BuildingSlotType, BUILDING_SLOT_DWELLING_2);
            m_castleRecs[townId].m_garrison[1] =
                gMonsterDatabase[gDwellingType[curTownType][1]].growth;
            dwellingCount--;
        }
    }
    memset(nextUsed, 0, 29);
    for (n = 0; n < TOWN_MAGE_GUILD_SPELL_COUNT; n++) {
        if (n <= MAGE_GUILD_LEVEL_1_LAST_SLOT)
            spellLevel = MAGE_GUILD_STATE_LEVEL_1;
        else if (n <= MAGE_GUILD_LEVEL_2_LAST_SLOT)
            spellLevel = MAGE_GUILD_STATE_LEVEL_2;
        else if (n <= MAGE_GUILD_LEVEL_3_LAST_SLOT)
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
        } while (nextUsed[newSpell] || Random(1, 1500) >= spellValue);
        m_castleRecs[townId].m_mageGuildSpells[n] = newSpell;
        nextUsed[newSpell] = 1;
    }
}

// 2x2 mines: the terrain picks the mine type (unused types first) and the
// object and shadow frames.
VA(0x00434e9f, 0x586)
void game::RandomizeMine(i8 x, i8 y) {
    u8 objFrame;
    i8 mineIdx;
    i8 iRow;
    i32 count;
    i8 iCol;
    u8 trigger;
    H1_ENUM_LOCAL(ResourceType, i8) resType;
    H1_ENUM_LOCAL(TerrainType, i8) terrain;
    u8 mineFrame;

    terrain = giGroundToTerrain[m_map[x][y].m_tileIndex];
    for (count = 0; count < 30; count++) {
        switch (terrain) {
            case TERRAIN_GRASS:
            case TERRAIN_DIRT:
                resType = H1_ENUM_DECODE(
                    ResourceType,
                    Random(H1_ENUM_ENCODE(ResourceType, RESOURCE_MERCURY), H1_ENUM_ENCODE(ResourceType, RESOURCE_GOLD))
                );
                if (resType == RESOURCE_MERCURY)
                    resType = RESOURCE_WOOD;
                break;
            case TERRAIN_SNOW:
                resType = H1_ENUM_DECODE(
                    ResourceType,
                    Random(H1_ENUM_ENCODE(ResourceType, RESOURCE_ORE), H1_ENUM_ENCODE(ResourceType, RESOURCE_GOLD))
                );
                break;
            case TERRAIN_SWAMP:
                resType = H1_ENUM_DECODE(
                    ResourceType,
                    Random(H1_ENUM_ENCODE(ResourceType, RESOURCE_WOOD), H1_ENUM_ENCODE(ResourceType, RESOURCE_GOLD))
                );
                break;
            case TERRAIN_LAVA:
                resType = RESOURCE_MERCURY;
                break;
            case TERRAIN_DESERT:
            default:
                resType = H1_ENUM_DECODE(
                    ResourceType,
                    Random(H1_ENUM_ENCODE(ResourceType, RESOURCE_MERCURY), H1_ENUM_ENCODE(ResourceType, RESOURCE_GOLD))
                );
                break;
        }
        if (!gMineTypeCount[resType])
            count = 30;
    }
    gMineTypeCount[resType]++;
    switch (resType) {
        case RESOURCE_WOOD:
            mineFrame = 5;
            break;
        case RESOURCE_MERCURY:
            mineFrame = 0x19;
            break;
        default:
            switch (terrain) {
                case TERRAIN_GRASS:
                    mineFrame = 0xf;
                    break;
                case TERRAIN_SNOW:
                    mineFrame = 0x13;
                    break;
                default:
                    mineFrame = 9;
                    break;
            }
            break;
    }
    switch (resType) {
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
    m_map[x][y - 1].m_overlayIndex = mineFrame;
    m_map[x + 1][y - 1].m_overlayIndex = mineFrame + 1;
    if (resType == RESOURCE_MERCURY) {
        m_map[x + 1][y].m_flags |= MAP_CELL_OBJECT_ANIMATED;
        trigger = MAP_OBJECT_TRIGGER(MAP_OBJECT_ALCHEMIST_LAB);
    } else if (resType == RESOURCE_WOOD) {
        trigger = MAP_OBJECT_TRIGGER(MAP_OBJECT_SAWMILL);
    } else {
        m_map[x + 1][y].m_flags |= MAP_CELL_OBJECT_EXTRA;
        m_map[x + 1][y].m_objectTileset |= H1_ENUM_ENCODE(MapTileset, TILESET_RSRC32) << MAP_CELL_EXTRA_TILESET_SHIFT;
        m_map[x + 1][y].m_extraFrame = resType - RESOURCE_ORE;
        trigger = MAP_OBJECT_TRIGGER(MAP_OBJECT_MINE);
    }
    mineIdx = GetMineId(x, y);
    for (iRow = 0; iRow < MINE_FOOTPRINT_HEIGHT; iRow++) {
        for (iCol = 0; iCol < MINE_FOOTPRINT_WIDTH; iCol++) {
            if (MAP_TRIGGER_OBJECT(m_map[x + iCol][y - iRow].m_triggerType) > MAP_OBJECT_NONE
                && MAP_TRIGGER_OBJECT(m_map[x + iCol][y - iRow].m_triggerType)
                       <= MAP_OBJECT_EVENT_LAST) {
                m_map[x + iCol][y - iRow].m_secondaryTrigger |= trigger;
            } else {
                m_map[x + iCol][y - iRow].m_objectMetadata = mineIdx;
                m_map[x + iCol][y - iRow].m_triggerType = trigger;
            }
        }
    }
    m_map[x][y].m_triggerType |= MAP_TRIGGER_EVENT;
    m_mines[mineIdx].type = resType;
}

// Picks an unused random artifact (ids 4..36), else the first free one.
VA(0x00435425, 0x5e)
H1_ENUM_RETURN(ArtifactType, i8) game::GetRandomArtifactId(void) {
    H1_ENUM_LOCAL(ArtifactType, i8) freeSlot = H1_ENUM_DECODE(
        ArtifactType,
        Scan(m_randomArtifacts, H1_ENUM_ENCODE(ArtifactType, ARTIFACT_REGULAR_FIRST), ARTIFACT_REGULAR_END - ARTIFACT_REGULAR_FIRST)
    );
    if (freeSlot == ARTIFACT_NONE)
        return ARTIFACT_NONE;
    H1_ENUM_LOCAL(ArtifactType, i8) artifact = H1_ENUM_DECODE(
        ArtifactType,
        RandomScan(
            m_randomArtifacts,
            H1_ENUM_ENCODE(ArtifactType, ARTIFACT_REGULAR_FIRST),
            ARTIFACT_REGULAR_END - ARTIFACT_REGULAR_FIRST,
            H1_ENUM_ENCODE(ArtifactType, ARTIFACT_REGULAR_END)
        )
    );
    if (artifact == ARTIFACT_NONE)
        return freeSlot;
    else
        return artifact;
}

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

// Four classes; draws only from the first two stacks of each class table.
VA(0x00435566, 0x28f)
void game::SetRandomHeroArmies(i16 heroId, i32 strongArmy) {
    armyGroup* curArmy = &m_heroRecs[heroId].m_army;
    i16 curSlot = 0;
    i16 armies[HERO_CLASS_COUNT][RANDOM_HERO_ARMY_OPTION_COUNT][RANDOM_HERO_ARMY_FIELD_COUNT] = {
        {{H1_ENUM_ENCODE(CreatureType, CREATURE_PEASANT), 30, 50},
         {H1_ENUM_ENCODE(CreatureType, CREATURE_ARCHER), 3, 5},
         {H1_ENUM_ENCODE(CreatureType, CREATURE_PIKEMAN), 2, 4}},
        {{H1_ENUM_ENCODE(CreatureType, CREATURE_GOBLIN), 15, 25},
         {H1_ENUM_ENCODE(CreatureType, CREATURE_ORC), 3, 5},
         {H1_ENUM_ENCODE(CreatureType, CREATURE_WOLF), 2, 3}},
        {{H1_ENUM_ENCODE(CreatureType, CREATURE_SPRITE), 10, 20},
         {H1_ENUM_ENCODE(CreatureType, CREATURE_DWARF), 2, 4},
         {H1_ENUM_ENCODE(CreatureType, CREATURE_ELF), 1, 2}},
        {{H1_ENUM_ENCODE(CreatureType, CREATURE_CENTAUR), 6, 10},
         {H1_ENUM_ENCODE(CreatureType, CREATURE_GARGOYLE), 2, 4},
         {H1_ENUM_ENCODE(CreatureType, CREATURE_GRIFFIN), 1, 2}}
    };
    i32 curPresent[RANDOM_HERO_ARMY_OPTION_COUNT];
    i32 i;
    i32 curMax;
    i32 minNum;

    curPresent[0] = 1;
    curPresent[1] = Random(0, 99) < RANDOM_HERO_FIRST_STACK_CHANCE
                                        + (strongArmy ? RANDOM_HERO_FIRST_STACK_BONUS_CHANCE : 0);
    curPresent[2] = Random(0, 99) < RANDOM_HERO_SECOND_STACK_CHANCE
                                        + (strongArmy ? RANDOM_HERO_SECOND_STACK_BONUS_CHANCE : 0);
    if (!curPresent[2])
        curPresent[1] = 1;
    for (i = 0; i < ARMY_GROUP_SLOT_COUNT; i++) {
        curArmy->m_creatureTypes[i] = CREATURE_NONE;
        curArmy->m_creatureCounts[i] = RANDOM_HERO_EMPTY_COUNT;
    }
    for (i = 0; i < RANDOM_HERO_ARMY_SELECTION_COUNT; i++) {
        if (curPresent[i]) {
            curArmy->m_creatureTypes[curSlot] = H1_ENUM_DECODE(
                CreatureType,
                armies[m_heroRecs[heroId].m_heroClass][i][RANDOM_HERO_ARMY_FIELD_CREATURE]
            );
            minNum = armies[m_heroRecs[heroId].m_heroClass][i][RANDOM_HERO_ARMY_FIELD_MIN]
                     * RANDOM_HERO_COUNT_SCALE;
            curMax = armies[m_heroRecs[heroId].m_heroClass][i][RANDOM_HERO_ARMY_FIELD_MAX]
                         * RANDOM_HERO_COUNT_SCALE
                     + RANDOM_HERO_COUNT_ROUNDING;
            if (strongArmy)
                minNum = (minNum + curMax) / 2;
            curArmy->m_creatureCounts[curSlot] = Random(minNum, curMax) / RANDOM_HERO_COUNT_SCALE;
            curSlot++;
        }
    }
}

// Random towns, castles, monsters by strength band, resources, artifacts
// and mines; NewMap runs the castles-only pass first.
VA(0x004357f5, 0x27f)
void game::ProcessRandomObjects(i32 castlesOnly) {
    mapCell* cellPtrItem;
    i32 lowFVVal;
    i32 y;
    i32 x;
    // Also the player of the random-town reset.
    H1_ENUM_SHARED(ResourceType, i32) i;
    i32 highFVNum;

    for (i = RESOURCE_FIRST; i < RESOURCE_COUNT; i++)
        gMineTypeCount[i] = 0;
    for (i = 0; i < GAME_PLAYER_COUNT; i++)
        gRandomTownTypes[i] = TOWN_TYPE_NONE;
    for (y = 0; y < MAP_CELL_GRID_SIZE; y++) {
        for (x = 0; x < MAP_CELL_GRID_SIZE; x++) {
            cellPtrItem = &m_map[x][y];
            if (!castlesOnly
                || cellPtrItem->m_triggerType
                       == MAP_EVENT_TRIGGER(MAP_FILE_OBJECT_RANDOM_CASTLE)) {
                switch (cellPtrItem->m_triggerType) {
                    case MAP_EVENT_TRIGGER(MAP_FILE_OBJECT_RANDOM_TOWN):
                        RandomizeTown(x, y, 0);
                        break;
                    case MAP_EVENT_TRIGGER(MAP_FILE_OBJECT_RANDOM_CASTLE):
                        RandomizeTown(x, y, 1);
                        break;
                    case MAP_EVENT_TRIGGER(MAP_FILE_OBJECT_RANDOM_MONSTER):
                        lowFVVal = 80;
                        highFVNum = 2000;
                        goto pickMonster;
                    case MAP_EVENT_TRIGGER(MAP_FILE_OBJECT_RANDOM_MONSTER_WEAK):
                        lowFVVal = 0;
                        highFVNum = 400;
                        goto pickMonster;
                    case MAP_EVENT_TRIGGER(MAP_FILE_OBJECT_RANDOM_MONSTER_MEDIUM):
                        lowFVVal = 80;
                        highFVNum = 1000;
                        goto pickMonster;
                    case MAP_EVENT_TRIGGER(MAP_FILE_OBJECT_RANDOM_MONSTER_STRONG):
                        lowFVVal = 500;
                        highFVNum = 2500;
                        goto pickMonster;
                    case MAP_EVENT_TRIGGER(MAP_FILE_OBJECT_RANDOM_MONSTER_VERY_STRONG):
                        lowFVVal = 2000;
                        highFVNum = 100000;
                        goto pickMonster;
                    pickMonster:
                        cellPtrItem->m_triggerType = MAP_EVENT_TRIGGER(MAP_OBJECT_MONSTER);
                        cellPtrItem->m_objectIndex = Random(0, 27);
                        while (gMonsterDatabase[H1_ENUM_DECODE(CreatureType, cellPtrItem->m_objectIndex)].fightValue
                                   <= lowFVVal
                               || gMonsterDatabase[H1_ENUM_DECODE(CreatureType, cellPtrItem->m_objectIndex)].fightValue
                                      >= highFVNum)
                            cellPtrItem->m_objectIndex = Random(0, 27);
                        break;
                    case MAP_EVENT_TRIGGER(MAP_FILE_OBJECT_RANDOM_RESOURCE):
                        cellPtrItem->m_triggerType = MAP_EVENT_TRIGGER(MAP_OBJECT_RESOURCE);
                        cellPtrItem->m_objectIndex = Random(61, 67);
                        break;
                    case MAP_EVENT_TRIGGER(MAP_FILE_OBJECT_RANDOM_ARTIFACT):
                        cellPtrItem->m_triggerType = MAP_EVENT_TRIGGER(MAP_OBJECT_ARTIFACT);
                        cellPtrItem->m_objectIndex = H1_ENUM_ENCODE(ArtifactType, GetRandomArtifactId());
                        m_randomArtifacts[H1_ENUM_DECODE(ArtifactType, cellPtrItem->m_objectIndex)] = GAME_ARTIFACT_ON_MAP;
                        break;
                    case MAP_EVENT_TRIGGER(MAP_FILE_OBJECT_RANDOM_MINE):
                        RandomizeMine(x, y);
                        break;
                }
            }
        }
    }
}

VA(0x00435a74, 0x22f)
void game::SetVisibility(i16 x, i16 y, i16 player, i16 radius) {
    i32 i;
    i32 j;
    i32 rangeLeft;
    i32 cutoff;
    u8 viewMask = 1 << player;
    u8 outerMaskVal = 1 << (player + GAME_PLAYER_HIGH_BIT_SHIFT);

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
                    gpGame->m_mapExtra[i][j] |= outerMaskVal;
            }
        }
    }
}

VA(0x00435ca3, 0xc8)
void game::GiveArmy(armyGroup* group, H1_ENUM_PARAM(CreatureType, i32) type, i32 count, i32 slot) {
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
                if (group->m_creatureTypes[i] < CREATURE_FIRST) {
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

// Seeded random generator.
VA(0x00435de5, 0x92)
i32 SGenRand(void) {
    i32 bitMask;
    i32 ret = 0;
    i32 i;
    gLastSeed &= 0xfff;
    gLastSeed *= 7;
    gLastSeed += (gLastSeed & 0xff0) >> 4;
    for (i = 31; i >= 0; --i) {
        bitMask = 1 << i;
        if (gLastSeed & bitMask)
            ret |= 1 << i;
    }
    return ret;
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

// The scan cursor lives in file statics.
DATA(0x004a6c08)
static i32 s_adjacentMonsterEndX;
DATA(0x004a6c04)
static i32 s_adjacentMonsterEndY;
DATA(0x004a6bd4)
static i32 s_adjacentMonsterX;
DATA(0x004a6bd8)
static i32 s_adjacentMonsterY;
DATA(0x004a6be8)
static i32 s_adjacentMonsterMinX;
DATA(0x004a6bec)
static i32 s_adjacentMonsterMinY;

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
                    == MAP_EVENT_TRIGGER(MAP_OBJECT_MONSTER)) {
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
        if (originX == 0)
            s_adjacentMonsterMinX = 0;
        else
            s_adjacentMonsterMinX = originX - 1;
        if (originY == 0)
            s_adjacentMonsterMinY = 0;
        else
            s_adjacentMonsterMinY = originY - 1;

        for (s_adjacentMonsterX = s_adjacentMonsterMinX; s_adjacentMonsterX < s_adjacentMonsterEndX;
             ++s_adjacentMonsterX) {
            for (s_adjacentMonsterY = s_adjacentMonsterMinY;
                 s_adjacentMonsterY < s_adjacentMonsterEndY;
                 ++s_adjacentMonsterY) {
                if (m_mapData[s_adjacentMonsterX][s_adjacentMonsterY].m_triggerType
                    == MAP_EVENT_TRIGGER(MAP_OBJECT_MONSTER)) {
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

VA(0x00436320, 0xbc)
void game::SetupAdjacentMons(void) {
    i32 x;
    i32 y;
    i32 oldMask = 0x7f;
    i32 monY;
    i32 monX;

    for (x = 0; x < MAP_CELL_GRID_SIZE; ++x) {
        for (y = 0; y < MAP_CELL_GRID_SIZE; ++y) {
            if (gpAdvManager->FindAdjacentMonster(x, y, &monX, &monY, -1, -1))
                mapExtra[x][y] |= MAP_EXTRA_MONSTER_ADJACENT;
            else
                mapExtra[x][y] &= oldMask;
        }
    }
}

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

// Draws the logo from a tileset.
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
            H1_ENUM_ENCODE(PlayerColor, gpGame->m_players[player].m_color)
        );
        StopMusic();
    }
}

// Rerolls the variant within each four-tile group, past the first four
// tiles of every twenty-tile terrain block.
VA(0x004366f6, 0xa0)
void game::RandomizeTerrainTiles(void) {
    mapCell* cell;
    i32 tile;
    i32 x;
    i32 y;
    for (y = 0; y < MAP_CELL_GRID_SIZE; y++) {
        for (x = 0; x < MAP_CELL_GRID_SIZE; x++) {
            cell = &m_map[x][y];
            if (cell->m_tileIndex % MAP_CELL_TILES_PER_TERRAIN >= TERRAIN_TILE_VARIANT_COUNT)
                cell->m_tileIndex =
                    cell->m_tileIndex / TERRAIN_TILE_VARIANT_COUNT * TERRAIN_TILE_VARIANT_COUNT
                    + Random(0, TERRAIN_TILE_VARIANT_COUNT - 1);
        }
    }
}

// Town extras only; there are no late overlays.
VA(0x00436796, 0x107)
void game::ProcessMapExtra(void) {
    i32 y;
    mapCell* cell;
    i32 x;
    i8 townNum;
    for (y = 0; y < MAP_CELL_GRID_SIZE; y++) {
        for (x = 0; x < MAP_CELL_GRID_SIZE; x++) {
            cell = &m_map[x][y];
            switch (cell->m_triggerType) {
                case MAP_EVENT_TRIGGER(MAP_OBJECT_TOWN):
                case MAP_EVENT_TRIGGER(MAP_FILE_OBJECT_RANDOM_TOWN):
                case MAP_EVENT_TRIGGER(MAP_FILE_OBJECT_RANDOM_CASTLE):
                    townNum = GetTownId(x, y);
                    m_castleRecs[townNum].m_extraIndex = cell->m_objectMetadata;
                    cell->m_objectMetadata = townNum;
                    break;
                case MAP_EVENT_TRIGGER(MAP_FILE_OBJECT_HERO):
                    m_noMapHeroes = 0;
                    break;
            }
        }
    }
}

// Owners, garrisons and buildings; a map whose towns all lack owners
// leaves the placeholder -2.
VA(0x0043689d, 0x1cf)
i8 game::SetupTowns(void) {
    mapTownExtra* newExtra;
    i32 curOwn;
    i8 isUnowned;
    town* town;
    i32 j;
    i32 i;
    i32 mask;
    isUnowned = 1;
    mask = MAP_TOWN_EXTRA_BUILDING_MASK;
    for (i = 0; i < GAME_TOWN_COUNT; i++) {
        town = GetTown(i);
        town->m_customized = 0;
        if (town->m_extraIndex >= 1) {
            newExtra = static_cast<mapTownExtra*>(ppMapExtra[town->m_extraIndex]);
            if (newExtra->customized && newExtra->owner != MAP_TOWN_OWNER_UNSET) {
                if (newExtra->owner >= gpGame->m_playerCount)
                    curOwn = gpGame->m_playerCount - 1;
                else
                    curOwn = newExtra->owner;
                isUnowned = 0;
                if (curOwn != GAME_PLAYER_NONE)
                    ClaimTown(i, curOwn);
            }
            if (newExtra->customized) {
                town->m_customized = 1;
                for (j = 0; j < ARMY_GROUP_SLOT_COUNT; j++) {
                    town->m_army.m_creatureCounts[j] = newExtra->troopCounts[j];
                    if (town->m_army.m_creatureCounts[j] > 0)
                        town->m_army.m_creatureTypes[j] = newExtra->troopTypes[j];
                    else
                        town->m_army.m_creatureTypes[j] = CREATURE_NONE;
                }
                town->m_buildState = newExtra->buildState;
                town->m_buildings =
                    town->m_buildings - (town->m_buildings & mask) + (newExtra->buildings & mask);
            }
        }
    }
    if (!isUnowned) {
        for (i = 0; i < GAME_TOWN_COUNT; i++) {
            town = GetTown(i);
            if (town->m_owner == MAP_TOWN_OWNER_UNSET)
                town->m_owner = GAME_PLAYER_NONE;
        }
    }
    return isUnowned;
}

// Each placed hero takes its map-extra garrison, artifacts, experience and
// owner; a hero standing at a town gate occupies the town.
VA(0x00436a6c, 0x325)
void game::ProcessOnMapHeroes(void) {
    i32 posY;
    mapHeroExtra* extra;
    town* curTown;
    i32 townId;
    i32 iPlayer;
    i32 i;
    i32 jx;
    i32 posX;
    mapCell* north;
    mapCell* loc;
    hero* theHeroEntry;

    for (posY = 0; posY < MAP_CELL_GRID_SIZE; posY++) {
        for (posX = 0; posX < MAP_CELL_GRID_SIZE; posX++) {
            loc = &m_map[posX][posY];
            if ((loc->m_triggerType & MAP_TRIGGER_TYPE_MASK) == MAP_OBJECT_TRIGGER(MAP_FILE_OBJECT_HERO)) {
                extra = static_cast<mapHeroExtra*>(ppMapExtra[loc->m_objectMetadata]);
                theHeroEntry = GetHero(extra->heroId);
                for (i = 0; i < ARMY_GROUP_SLOT_COUNT; i++) {
                    theHeroEntry->m_army.m_creatureCounts[i] = extra->troopCounts[i];
                    if (theHeroEntry->m_army.m_creatureCounts[i] > 0)
                        theHeroEntry->m_army.m_creatureTypes[i] = extra->troopTypes[i];
                    else
                        theHeroEntry->m_army.m_creatureTypes[i] = CREATURE_NONE;
                }
                for (jx = 0; jx < MAP_HERO_EXTRA_ARTIFACT_COUNT; jx++) {
                    if (extra->artifacts[jx] >= ARTIFACT_FIRST)
                        gpAdvManager->GiveArtifact(theHeroEntry, extra->artifacts[jx]);
                }
                theHeroEntry->m_experience = 0;
                gpAdvManager->GiveExperience(theHeroEntry, extra->experience, 1);
                theHeroEntry->CheckLevel();
                theHeroEntry->m_x = posX;
                theHeroEntry->m_y = posY;
                if (extra->owner >= gpGame->m_playerCount)
                    iPlayer = gpGame->m_playerCount - 1;
                else
                    iPlayer = extra->owner;
                theHeroEntry->m_owner = iPlayer;
                m_availableHeroes[extra->heroId] = iPlayer;
                m_players[theHeroEntry->m_owner]
                    .m_heroIds[m_players[theHeroEntry->m_owner].m_heroCount] = theHeroEntry->m_id;
                m_players[theHeroEntry->m_owner].m_heroCount++;
                if (posY > 0) {
                    north = &m_map[posX][posY - 1];
                    if (north->m_triggerType == MAP_EVENT_TRIGGER(MAP_OBJECT_TOWN)) {
                        theHeroEntry->m_y--;
                        townId = GetTownId(posX, posY - 1);
                        curTown = GetTown(townId);
                        curTown->m_occupyingHeroId = theHeroEntry->m_id;
                    }
                }
                loc->m_objectTileset = 0;
                loc->m_objectIndex = MAP_CELL_NO_FRAME;
                loc->m_overlayTileset = 0;
                loc->m_overlayIndex = MAP_CELL_NO_FRAME;
                loc->m_objectMetadata = 0;
                loc->m_triggerType = MAP_OBJECT_TRIGGER(MAP_OBJECT_NONE);
                SetVisibility(
                    theHeroEntry->m_x,
                    theHeroEntry->m_y,
                    theHeroEntry->m_owner,
                    gHeroScoutRadius[theHeroEntry->m_heroClass]
                );
            }
        }
    }
    CheckHeroConsistency();
}

// Replaces tavern heroes some player already owns, clears map heroes that
// lost their owner and zeroes the counts of empty or negative stacks.
VA(0x00436d91, 0x341)
void game::CheckHeroConsistency(void) {
    town* townOccupied;
    i32 j;
    i32 i;
    i32 y;
    i32 x;
    mapCell* cell;
    hero* boardHro;

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
            if (cell->m_triggerType == MAP_EVENT_TRIGGER(MAP_OBJECT_HERO)) {
                if (cell->m_objectMetadata >= 0 && cell->m_objectMetadata < GAME_HERO_COUNT) {
                    boardHro = GetHero(cell->m_objectMetadata);
                    if (boardHro->m_owner < 0 || boardHro->m_owner > GAME_PLAYER_COUNT - 1) {
                        if (boardHro->m_locationType == MAP_EVENT_TRIGGER(MAP_OBJECT_TOWN)) {
                            townOccupied = gpGame->GetTown(boardHro->m_occupiedTown);
                            townOccupied->m_occupyingHeroId = TOWN_OCCUPYING_HERO_NONE;
                        }
                        RestoreCell(
                            boardHro->m_x,
                            boardHro->m_y,
                            boardHro->m_locationType,
                            boardHro->m_occupiedTown,
                            NULL,
                            1
                        );
                    }
                } else {
                    cell->m_triggerType = MAP_OBJECT_TRIGGER(MAP_OBJECT_NONE);
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

// Saves REMOTE.GAM, optionally LZH-encodes it, then sends it in 200-byte
// segments, 100 segments per acknowledged block.
VA(0x004370d2, 0x5fd)
i32 game::TransmitSaveGame(i32 remotePlayer, i32 playerExited) {
    i32 okay;
    char curPathname[452];
    i32 unusedSum;
    char* mainOutData;
    i32 block;
    i32 blocksCount;
    i32 unusedData;
    H1_ENUM_LOCAL(MusicTrack, i32) oldTrackVal;
    RemotePayload* sendPacket;
    i32 sizeVal;
    i32 entry;
    i32 segCountPos;
    i32 mainFile;
    i32 unusedOffset;
    char* incomingNow;
    char ackedArray[500];
    i32 unusedY;
    i32 replyState;
    i32 unusedSeq;
    i32 prevSize;
    i32 length;
    char* dataObj;
    i8 wasFinished;

    gpAdvManager->TrimLoopingSounds(REMOTE_SAVE_TRANSFER_SOUNDS);
    okay = 0;
    replyState = 0;
    oldTrackVal = MUSIC_TRACK_NONE;
    oldTrackVal = GetCurrentTrack();
    StopMusic();

    if (gpAdvManager->m_active == 1)
        BVResMsg(localization::Tr("network.send.title"), RESOURCE_NONE, 0);
    while (!gHeartbeatSeen) {
        PollSound();
        Process1WindowsMessage();
    }
    AiPrint("Transmit Start");
    memset(ackedArray, 0, sizeof(ackedArray));
    SaveGame("REMOTE.GAM", 0);
    sprintf(curPathname, "%s%s", gDataPath, "REMOTE.GAM");
    prevSize = FileSize(curPathname);
    sendPacket = static_cast<RemotePayload*>(malloc(REMOTE_MESSAGE_SIZE));
    if (!H1_ENUM_ENCODE(MultiplayerBaseType, iMPBaseType) || (iMPBaseType == MULTIPLAYER_BASE_NETWORK && gRemoteReady))
        mainOutData = static_cast<char*>(malloc(prevSize));
    dataObj = static_cast<char*>(malloc(prevSize));
    mainFile = open(curPathname, O_BINARY);
    if (mainFile == -1)
        FileError(curPathname);
    if (mainFile == -1) {
        goto cleanup;
    }
    {
        read(mainFile, dataObj, prevSize);
        close(mainFile);
        if (!H1_ENUM_ENCODE(MultiplayerBaseType, iMPBaseType) || (iMPBaseType == MULTIPLAYER_BASE_NETWORK && gRemoteReady))
            prevSize = EncodeData(mainOutData, dataObj, prevSize);
        else
            mainOutData = dataObj;

        sendPacket->saveSize = prevSize;
        sendPacket->playerExited = playerExited;
        replyState = TransmitAndWait(
            sendPacket->data,
            remotePlayer,
            REMOTE_SAVE_HEADER_SIZE,
            BOX_REMOTE_SAVE,
            REMOTE_COMMAND_SAVE_INIT_RESPONSE,
            &incomingNow
        );
        if (!replyState)
            ShutDown(NULL);

        segCountPos = (prevSize - 1) / REMOTE_SAVE_SEGMENT_SIZE + 1;
        blocksCount = (segCountPos - 1) / REMOTE_SAVE_BATCH_SIZE + 1;
        for (block = 0; block < blocksCount; block++) {
            if (block + 1 == blocksCount)
                sizeVal = segCountPos - block * REMOTE_SAVE_BATCH_SIZE;
            else
                sizeVal = REMOTE_SAVE_BATCH_SIZE;
            wasFinished = 0;
            while (!wasFinished) {
                for (entry = block * REMOTE_SAVE_BATCH_SIZE;
                     entry < block * REMOTE_SAVE_BATCH_SIZE + sizeVal;
                     entry++) {
                    PollSound();
                    CheckDoMain(0, 1);
                    if (!ackedArray[entry]) {
                        if (entry + 1 == segCountPos)
                            length = prevSize - entry * REMOTE_SAVE_SEGMENT_SIZE;
                        else
                            length = REMOTE_SAVE_SEGMENT_SIZE;
                        sendPacket->segment.index = static_cast<i16>(entry);
                        memcpy(
                            sendPacket->segment.data,
                            mainOutData + entry * REMOTE_SAVE_SEGMENT_SIZE,
                            length
                        );
                        replyState = TransmitRemoteData(
                            sendPacket->data,
                            remotePlayer,
                            length + REMOTE_SAVE_INDEX_SIZE,
                            REMOTE_COMMAND_SAVE_DATA,
                            0
                        );
                        if (!replyState)
                            ShutDown(NULL);
                    }
                }
                sendPacket->segment.index = static_cast<i16>(block * REMOTE_SAVE_BATCH_SIZE);
                replyState = TransmitAndWait(
                    sendPacket->data,
                    remotePlayer,
                    REMOTE_SAVE_INDEX_SIZE,
                    REMOTE_COMMAND_SAVE_ACK_REQUEST,
                    REMOTE_COMMAND_SAVE_ACK_RESPONSE,
                    &incomingNow
                );
                if (!replyState)
                    ShutDown(NULL);
                for (entry = 0; entry < sizeVal; entry++) {
                    if (reinterpret_cast<RemoteMessage*>(incomingNow)->payload.data[entry]
                        > 0) // API-forced: char* record.
                        ackedArray[entry + block * REMOTE_SAVE_BATCH_SIZE] = 1;
                }
                wasFinished = 1;
                for (entry = block * REMOTE_SAVE_BATCH_SIZE;
                     entry < block * REMOTE_SAVE_BATCH_SIZE + sizeVal;
                     entry++) {
                    if (!ackedArray[entry])
                        wasFinished = 0;
                }
            }
        }
        replyState = TransmitRemoteData(NULL, remotePlayer, 0, REMOTE_COMMAND_SAVE_FINISH, 1);
        if (!replyState)
            ShutDown(NULL);
        okay = 1;
    }

cleanup:
    free(sendPacket);
    if (!H1_ENUM_ENCODE(MultiplayerBaseType, iMPBaseType) || (iMPBaseType == MULTIPLAYER_BASE_NETWORK && gRemoteReady))
        free(mainOutData);
    free(dataObj);
    AiPrint("Transmit End");
    if (gpAdvManager->m_active == 1) {
        giBottomViewOverride = BOTTOM_VIEW_NONE;
        gpAdvManager->UpdBottomView(1, 1, 1);
    }
    if (oldTrackVal != MUSIC_TRACK_NONE) {
        PlayMusic(oldTrackVal);
    }
    return okay;
}

// Collects the remote save in 200-byte segments, acknowledging each block
// of 100, then decodes it and writes REMOTE.GAM.
VA(0x004376cf, 0x4b0)
i32 game::ReceiveSaveGame(i32 dataSize, i32 remotePlayer) {
    i32 oldUnused1;
    i32 okay;
    char pathname[452];
    char* curInData;
    i32 i;
    H1_ENUM_LOCAL(MusicTrack, i32) trackOld;
    i32 handleValue;
    i8 done;
    char* sendPacket;
    RemoteMessage* receivedPacketObj;
    i32 curRet;
    i32 lastPacketTimeNum;
    char myGotIt[500];
    i32 packetStartValue;
    char* decodedData;

    gpAdvManager->TrimLoopingSounds(REMOTE_SAVE_TRANSFER_SOUNDS);
    handleValue = 0;
    done = 0;
    oldUnused1 = 0;
    okay = 0;
    trackOld = MUSIC_TRACK_NONE;
    if (gpAdvManager->m_active == 1)
        BVResMsg(localization::Tr("network.receive.title"), RESOURCE_NONE, 0);
    trackOld = GetCurrentTrack();
    StopMusic();
    while (!gHeartbeatSeen) {
        PollSound();
        Process1WindowsMessage();
    }
    curRet = TransmitRemoteData(NULL, remotePlayer, 0, REMOTE_COMMAND_SAVE_INIT_RESPONSE, 1);
    if (!curRet)
        ShutDown(NULL);
    memset(myGotIt, 0, sizeof(myGotIt));
    if (!H1_ENUM_ENCODE(MultiplayerBaseType, iMPBaseType) || (iMPBaseType == MULTIPLAYER_BASE_NETWORK && gRemoteReady))
        decodedData = static_cast<char*>(malloc(REMOTE_SAVE_DECODE_BUFFER_SIZE));
    sendPacket = static_cast<char*>(malloc(REMOTE_MESSAGE_SIZE));
    curInData = static_cast<char*>(malloc(dataSize + REMOTE_SAVE_BUFFER_EXTRA));
    lastPacketTimeNum = KBTickCount();
    while (!done) {
        PollSound();
        CheckDoMain(0, 1);
        if (lastPacketTimeNum + REMOTE_WAIT_TIMEOUT < KBTickCount()) {
            NormalDialog(
                localization::Tr("combat.network.receive_error"),
                NORMAL_DIALOG_TYPE_YES_NO
            );
            if (gpWindowManager->m_dialogResult == NORMAL_DIALOG_CONFIRM)
                lastPacketTimeNum = KBTickCount();
            else
                ShutDown(NULL);
        }
        receivedPacketObj =
            reinterpret_cast<RemoteMessage*>(GetRemoteData(1)); // API-forced: char* record.
        if (receivedPacketObj
            && (receivedPacketObj->type == REMOTE_MESSAGE_RELIABLE
                || receivedPacketObj->type == REMOTE_MESSAGE_UNRELIABLE)) {
            lastPacketTimeNum = KBTickCount();
            switch (receivedPacketObj->command) {
                case REMOTE_COMMAND_SAVE_DATA:
                    packetStartValue = receivedPacketObj->payload.segment.index;
                    myGotIt[packetStartValue] = 1;
                    memcpy(
                        curInData + packetStartValue * REMOTE_SAVE_SEGMENT_SIZE,
                        receivedPacketObj->payload.segment.data,
                        receivedPacketObj->payloadSize - REMOTE_SAVE_INDEX_SIZE
                    );
                    break;
                case REMOTE_COMMAND_SAVE_ACK_REQUEST:
                    packetStartValue = receivedPacketObj->payload.segment.index;
                    for (i = packetStartValue; i < packetStartValue + REMOTE_SAVE_BATCH_SIZE; i++)
                        *(sendPacket + i - packetStartValue) = myGotIt[i];
                    curRet = TransmitRemoteData(
                        sendPacket,
                        remotePlayer,
                        REMOTE_SAVE_ACK_MAP_SIZE,
                        REMOTE_COMMAND_SAVE_ACK_RESPONSE,
                        1
                    );
                    if (!curRet)
                        ShutDown(NULL);
                    break;
                case REMOTE_COMMAND_SAVE_FINISH:
                    done = 1;
                    break;
            }
        }
    }
    if (!H1_ENUM_ENCODE(MultiplayerBaseType, iMPBaseType) || (iMPBaseType == MULTIPLAYER_BASE_NETWORK && gRemoteReady))
        dataSize = DecodeData(decodedData, curInData);
    else
        decodedData = curInData;
    sprintf(pathname, "%s%s", gDataPath, "REMOTE.GAM");
    handleValue = open(pathname, O_WRONLY | O_CREAT | O_TRUNC | O_BINARY, S_IWRITE);
    if (handleValue == -1)
        FileError(pathname);
    write(handleValue, decodedData, dataSize);
    close(handleValue);
    okay = 1;
    free(sendPacket);
    free(curInData);
    if (!H1_ENUM_ENCODE(MultiplayerBaseType, iMPBaseType) || (iMPBaseType == MULTIPLAYER_BASE_NETWORK && gRemoteReady))
        free(decodedData);
    AiPrint("Receive End");
    if (gpAdvManager->m_active == 1) {
        giBottomViewOverride = BOTTOM_VIEW_NONE;
        gpAdvManager->UpdBottomView(1, 1, 1);
    }
    if (trackOld != MUSIC_TRACK_NONE) {
        PlayMusic(trackOld);
    }
    return okay;
}

VA(0x00437b7f, 0x5b5)
void game::DoNewTurn(void) {
    H1_ENUM_LOCAL(MusicTrack, i32) track;
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
            gText[0] = CyrillicToUpper(gText[0]);
        } else {
            sprintf(
                gText,
                gNewTurnText[NEW_TURN_TEXT_DAYS_LEFT],
                gColorNames[gpGame->m_players[giCurPlayer].Color()],
                gpCurPlayer->m_daysLeft
            );
            gText[0] = CyrillicToUpper(gText[0]);
        }
        NormalDialog(
            gText,
            NORMAL_DIALOG_TYPE_OK,
            0x61,
            -1,
            NORMAL_DIALOG_CREST,
            H1_ENUM_ENCODE(PlayerColor, gpGame->m_players[giCurPlayer].Color())
        );
    }
    if (gpCurPlayer->m_heroCount > 0)
        gpAdvManager->SetHeroContext(gpCurPlayer->NextHero(0), 0);
    else if (gpCurPlayer->m_townCount > 0)
        gpAdvManager->SetTownContext(gpCurPlayer->m_townIds[0]);
    gpAdvManager->CheckDimNextHeroBut();
    PlayMusic(TERRAIN_MUSIC_TRACK(gpAdvManager->m_currentTerrain));
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
                    strcpy(monsterName, gArmyNamesPlural[H1_ENUM_DECODE(CreatureType, giMonthTypeExtra)]);
                    monsterName[0] = CyrillicToLower(monsterName[0]);
                    sprintf(
                        gText,
                        gNewTurnText[NEW_TURN_TEXT_MONTH_CREATURE],
                        gArmyNamesPlural[H1_ENUM_DECODE(CreatureType, giMonthTypeExtra)],
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
                    strcpy(monsterName, gArmyNamesPlural[H1_ENUM_DECODE(CreatureType, giWeekTypeExtra)]);
                    monsterName[0] = CyrillicToLower(monsterName[0]);
                    sprintf(
                        gText,
                        gNewTurnText[NEW_TURN_TEXT_WEEK_CREATURE],
                        gArmyNamesPlural[H1_ENUM_DECODE(CreatureType, giWeekTypeExtra)],
                        monsterName
                    );
                }
            }
            PlayMusic(track);
            gpMouseManager->SetPointer(ADVENTURE_POINTER_DEFAULT);
            NormalDialog(gText, NORMAL_DIALOG_TYPE_OK, 0x61);
            PlayMusic(TERRAIN_MUSIC_TRACK(gpAdvManager->m_currentTerrain));
        }
    }
}

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

// Player-count file masks and the reqextra.bin map-info window; a
// cancelled pick restores the previous map's texts.
VA(0x00438180, 0x373)
void game::GetMap(void) {
    char saveFullName[20];
    char oldDescription[124];
    char oldMapName[16];
    char mask[16];
    i16 code;
    fileRequester* theRequest;

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
    else if (giNumHumanPlayers == GAME_PLAYERS_TWO)
        sprintf(mask, "?????2??.MAP");
    else if (giNumHumanPlayers == GAME_PLAYERS_THREE)
        sprintf(mask, "??????3?.MAP");
    else if (giNumHumanPlayers == GAME_PLAYERS_FOUR)
        sprintf(mask, "???????4.MAP");
    theRequest = new fileRequester(310, 14, FILE_REQUESTER_LOAD, mask, gMapPath, ".MAP");
    if (!theRequest)
        MemError();
    theRequest->ShowMapInfo();
    code = gpExec->DoDialog(theRequest);
    gpWindowManager->RemoveWindow(gReqExtraWindow);
    if (code == DIALOG_BUTTON_2) {
        strcpy(gMapName, gLastFilename);
        delete theRequest;
    } else {
        strcpy(gMapName, oldMapName);
        strcpy(gFullMapName, saveFullName);
        strcpy(gMapDescription, oldDescription);
        delete theRequest;
    }
    delete gReqExtraWindow;
    gShowMapInfo = 0;
}

VA(0x004384f3, 0x80)
i32 game::GetNumThievesGuilds(i32 color) {
    i32 numGuilds = 0;
    i32 i;
    for (i = 0; i < m_players[color].m_townCount; ++i) {
        if (gpGame->m_castleRecs[m_players[color].m_townIds[i]].m_buildings
            & H1_ENUM_BIT(BuildingSlotType, BUILDING_SLOT_THIEVES_GUILD))
            ++numGuilds;
    }
    return numGuilds;
}

// Difficulty, opponents (human seats by handicap, computers by level), King
// of the Hill, map size and map difficulty. Zero bonuses are still added.
VA(0x00438573, 0x2e3)
i32 game::CalcDifficultyRating(void) {
    i32 i;
    i32 total;

    total = 0;
    if (m_difficulty == DIFFICULTY_EASY) {
        total += 0;
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
        else if (H1_ENUM_DECODE(ComputerPlayerType, m_players[i].m_difficulty) == PLAYER_TYPE_NONE)
            total -= 10;
        else if (H1_ENUM_DECODE(ComputerPlayerType, m_players[i].m_difficulty) == PLAYER_TYPE_DUMB)
            total += 5;
        else if (H1_ENUM_DECODE(ComputerPlayerType, m_players[i].m_difficulty) == PLAYER_TYPE_AVERAGE)
            total += 10;
        else if (H1_ENUM_DECODE(ComputerPlayerType, m_players[i].m_difficulty) == PLAYER_TYPE_SMART)
            total += 15;
        else if (H1_ENUM_DECODE(ComputerPlayerType, m_players[i].m_difficulty) == PLAYER_TYPE_GENIUS)
            total += 20;
    }
    gpGame->m_playerCount = 0;
    for (i = 0; i < GAME_PLAYER_COUNT; i++) {
        if (H1_ENUM_DECODE(ComputerPlayerType, gpGame->m_players[i].m_difficulty) > PLAYER_TYPE_NONE)
            gpGame->m_playerCount++;
    }
    if (gbIAmGreatest) {
        if (m_playerCount - giNumHumanPlayers == 0) {
            total += 0;
        } else if (m_playerCount - giNumHumanPlayers == 1) {
            total += 0;
        } else if (m_playerCount - giNumHumanPlayers == GAME_PLAYERS_TWO) {
            total += 5;
        } else if (m_playerCount - giNumHumanPlayers == GAME_PLAYERS_THREE) {
            total += 10;
        }
    }
    if (gMapSize == MAP_SIZE_SMALL) {
        total += 0;
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

VA(0x00438934, 0x410)
void game::ShowScenInfo(void) {
    i16 jj;
    const i8 sizeIdNo = 100;
    const i8 selLevelId = 101;
    const i8 selDescId = 102;
    const i8 crestId = 103;
    const i8 nameIdIndex = 104;
    const i8 levelIdIdx = 105;
    const i8 playersIdPos = 106;
    const i8 kingOfHillId = 107;
    const i8 ratingId = 108;
    char line1Buf[20];
    // The game's difficulty, or a human seat's handicap less one.
    H1_ENUM_LOCAL(GameDifficulty, i32) baseDifficulty;
    heroWindow* scenWindow;
    tag_message packet;
    i16 startIdx;
    i32 pad;

    gpMouseManager->SetPointer(ADVENTURE_POINTER_DEFAULT);
    scenWindow = new heroWindow(159, 14, "sceninfo.bin");
    if (!scenWindow)
        MemError();
    SET_WIDGET_MESSAGE(packet, WIDGET_COMMAND_SET_TEXT, nameIdIndex);
    packet.text = m_mapName;
    scenWindow->BroadcastMessage(packet);
    baseDifficulty = m_difficulty;
    if (giCurPlayer > 0)
        baseDifficulty = H1_ENUM_DECODE(GameDifficulty, gpCurPlayer->m_difficulty - 1);
    packet.id = levelIdIdx;
    packet.text = gDifficultyNames[baseDifficulty];
    scenWindow->BroadcastMessage(packet);
    packet.id = playersIdPos;
    packet.text = gText;
    sprintf(gText, "");
    for (jj = 1; jj < GAME_PLAYER_COUNT; jj++) {
        if (giCurPlayer == 0) {
            sprintf(
                line1Buf,
                "%s\n",
                gbHumanPlayer[jj] ? gHandicapNames[m_players[jj].m_difficulty]
                                  : gPlayerTypeNames[m_players[jj].m_difficulty]
            );
        } else if (jj == 1) {
            sprintf(line1Buf, "%s\n", gHandicapNames[H1_ENUM_ENCODE(GameDifficulty, m_difficulty) + 1]);
        } else {
            startIdx = jj - 1 < giCurPlayer ? jj - 1 : jj;
            sprintf(
                line1Buf,
                "%s\n",
                gbHumanPlayer[startIdx] ? gHandicapNames[m_players[startIdx].m_difficulty]
                                        : gPlayerTypeNames[m_players[startIdx].m_difficulty]
            );
        }
        strcat(gText, line1Buf);
    }
    scenWindow->BroadcastMessage(packet);
    packet.id = kingOfHillId;
    packet.text = gText;
    sprintf(
        gText,
        gbIAmGreatest ? localization::Tr("scenario.info.yes") : localization::Tr("scenario.info.no")
    );
    scenWindow->BroadcastMessage(packet);
    packet.id = ratingId;
    sprintf(gText, "%d%%", gpGame->m_difficultyRating);
    packet.text = gText;
    scenWindow->BroadcastMessage(packet);
    packet.id = sizeIdNo;
    packet.text = gMapSizeNames[m_mapSize];
    scenWindow->BroadcastMessage(packet);
    packet.id = selLevelId;
    packet.text = gMapDifficultyNames[m_mapDifficulty];
    scenWindow->BroadcastMessage(packet);
    packet.id = selDescId;
    packet.text = m_mapDescription;
    scenWindow->BroadcastMessage(packet);
    packet.command = WIDGET_COMMAND_SET_FRAME;
    if (m_players[giCurPlayer].m_color != PLAYER_COLOR_NONE) {
        packet.id = crestId;
        packet.value = H1_ENUM_ENCODE(PlayerColor, m_players[giCurPlayer].m_color) * 2 + 11;
        scenWindow->BroadcastMessage(packet);
    }
    gpWindowManager->DoDialog(scenWindow, EventWindowHandler, 0);
}

// Keeps the human's crest and gives each opponent a free one: the campaign
// scenario's crest when it names one, else a random draw.
VA(0x00438d44, 0x114)
void game::RandomizePlayerCrests(void) {
    i32 i;
    H1_ENUM_ARRAY(i8, taken, PlayerColor, PLAYER_COLOR_COUNT);
    taken[PLAYER_COLOR_BLUE] = 0;
    taken[PLAYER_COLOR_GREEN] = 0;
    taken[PLAYER_COLOR_RED] = 0;
    taken[PLAYER_COLOR_YELLOW] = 0;
    taken[m_players[0].m_color] = 1;
    for (i = 1; i < m_playerCount; i++) {
        do {
            if (m_campaignType > 0
                && H1_ENUM_DECODE(PlayerColor, gCampaignScenarios[m_campaignScenario].playerCrests[i]) < PLAYER_COLOR_COUNT
                && gCampaignScenarios[m_campaignScenario].playerCrests[i] >= 0)
                m_players[i].m_color = H1_ENUM_DECODE(PlayerColor, gCampaignScenarios[m_campaignScenario].playerCrests[i]);
            else
                m_players[i].m_color = H1_ENUM_DECODE(
                    PlayerColor,
                    Random(H1_ENUM_ENCODE(PlayerColor, PLAYER_COLOR_BLUE), H1_ENUM_ENCODE(PlayerColor, PLAYER_COLOR_YELLOW))
                );
        } while (taken[m_players[i].m_color] == 1);
        taken[m_players[i].m_color] = 1;
    }
}

VA(0x00438e58, 0x87)
void game::RestoreCell(i32 x, i32 y, i32 obj, i32 barrier, mapCell* passedCell, i32) {
    mapCell* cell;
    if (passedCell)
        cell = passedCell;
    else
        cell = gpAdvManager->GetCell(x, y);
    if (y > 0 && obj == MAP_EVENT_TRIGGER(MAP_OBJECT_TOWN)
        && gpAdvManager->GetCell(x, y - 1)->m_triggerType != MAP_OBJECT_TRIGGER(MAP_OBJECT_TOWN)) {
        cell->m_triggerType = MAP_OBJECT_TRIGGER(MAP_OBJECT_NONE);
        cell->m_objectMetadata = 0;
        return;
    }
    cell->m_triggerType = obj;
    cell->m_objectMetadata = barrier;
}

// GAME globals. gNewGameSettingsSaved is local to NewGame; gMonType is
// local to PerMonth, and gShowMapInfo is defined above GetMap.
DATA(0x004a6c24)
i32 gGameOver = 0;
DATA(0x0048fcc8)
i32 gLastSeed = 135621123;
DATA(0x004a6c0c)
i8 gSaveCurPlayer;
DATA(0x004a6bdc)
H1_ENUM_STORAGE(PlayerColor, i8) gSavedCrest;
DATA(0x004a6bf0)
H1_ENUM_STORAGE(GameDifficulty, i8) gSavedDifficulty;
DATA(0x004a6c20)
H1_ENUM_STORAGE(GameEndSequence, i32) gEndSequence;
DATA(0x004a6c0d)
i8 gbDismissArmy;
DATA(0x004a6be4)
heroWindow* gReqExtraWindow;
DATA(0x004a6bd0)
i8 gSavedPlayerTypes[4];
DATA(0x004a6bf4)
H1_ENUM_ARRAY(i16, gMineTypeCount, ResourceType, RESOURCE_COUNT);
DATA(0x004a6c10)
char gCurMapName[16];
DATA(0x004a6c0e)
i8 gSavedKingOfTheHill;
DATA(0x004a6be0)
H1_ENUM_STORAGE(TownType, i8) gRandomTownTypes[4];

#include <H1/Ints.h>

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

void playerData::Write(i32 file) {
    char unused[52];

    write(file, m_unused00, sizeof(m_unused00));
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
    WRITE_FILE_VALUE(file, m_unused9a);
    WRITE_FILE_VALUE(file, m_unused9a);
    write(file, m_puzzlePiecesRemoved, sizeof(m_puzzlePiecesRemoved));
}

void playerData::Read(i32 file) {
    char unused[52];

    read(file, m_unused00, sizeof(m_unused00));
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
    READ_FILE_VALUE(file, m_unused9a);
    READ_FILE_VALUE(file, m_unused9a);
    read(file, m_puzzlePiecesRemoved, sizeof(m_puzzlePiecesRemoved));
}

i8 playerData::NextHero(i32) {
    i32 curHero = -1;
    i32 i;

    if (gCurPlayerData->m_currentHero != HERO_ID_NONE) {
        for (i = 0; i < gCurPlayerData->m_heroCount; ++i) {
            if (gCurPlayerData->m_currentHero == gCurPlayerData->m_heroIds[i])
                curHero = i;
        }
    }

    for (i = curHero + 1; i < gCurPlayerData->m_heroCount; ++i) {
        if (gGame->IsMobile(gCurPlayerData->m_heroIds[i]))
            return m_heroIds[i];
    }
    for (i = 0; i < curHero + 1; ++i) {
        if (gGame->IsMobile(gCurPlayerData->m_heroIds[i]))
            return m_heroIds[i];
    }
    return HERO_ID_NONE;
}

i8 playerData::HasMobileHero(void) {
    for (i16 i = 0; i < m_heroCount; ++i) {
        if (gGame->IsMobile(m_heroIds[i]))
            return 1;
    }
    return 0;
}

i8 playerData::CountPuzzlePiecesRemoved(void) {
    i8 count = 0;
    for (i16 idx = 0; idx < PLAYER_PUZZLE_PIECE_COUNT; ++idx) {
        if (BitTest(m_puzzlePiecesRemoved, idx))
            ++count;
    }
    return count;
}

i32 playerData::BuildingsOwned(
    i32 townType,
    i32 buildingIndex,
    i32 buildState
) {
    i32 count = 0;
    i32 i;
    for (i = 0; i < m_townCount; ++i) {
        town* ownedTown = &gGame->m_castleRecs[m_townIds[i]];
        if (buildingIndex < BUILDING_SLOT_DWELLING_FIRST || ownedTown->m_type == townType) {
            if (buildingIndex == BUILDING_SLOT_MAGE_GUILD) {
                if (ownedTown->m_buildings
                    & (1 << BUILDING_SLOT_MAGE_GUILD)) {
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

i32 playerData::NumOfGivenArtifact(i32 artifact) {
    i32 count = 0;
    i32 i;
    i32 artSlot;
    for (i = 0; i < m_heroCount; i++) {
        for (artSlot = 0; artSlot < HERO_ARTIFACT_SLOT_COUNT; artSlot++) {
            if (gGame->m_heroRecs[m_heroIds[i]].m_artifacts[artSlot] == artifact)
                count++;
        }
    }
    return count;
}

void ComputeUALoc(i32 player) {
    i32 triesCount;
    i32 x;
    i32 y;
    i32 heading;
    i32 nRemovedPieces;

    if (player >= 0) {
        nRemovedPieces = gGame->m_players[player].CountPuzzlePiecesRemoved();
        if (nRemovedPieces < ULTIMATE_HINT_PIECE_MIN
            || gGame->m_ultimateArtifactId == ARTIFACT_NONE) {
            gGame->m_players[player].m_ultimateArtifactHintChance = 0;
            gGame->m_players[player].m_ultimateArtifactHintX = PLAYER_ULTIMATE_HINT_NONE;
            gGame->m_players[player].m_ultimateArtifactHintY = PLAYER_ULTIMATE_HINT_NONE;
        } else {
            gGame->m_players[player].m_ultimateArtifactHintChance =
                (nRemovedPieces - ULTIMATE_HINT_PIECE_MIN) * ULTIMATE_HINT_PERCENT_PER_PIECE;
            if (Random(1, 100) <= gGame->m_players[player].m_ultimateArtifactHintChance) {
                gGame->m_players[player].m_ultimateArtifactHintX = gGame->m_ultimateArtifactX;
                gGame->m_players[player].m_ultimateArtifactHintY = gGame->m_ultimateArtifactY;
            } else {
                x = PLAYER_ULTIMATE_HINT_NONE;
                y = PLAYER_ULTIMATE_HINT_NONE;
                heading = 0;
                triesCount = 0;
                while (
                    !(MAP_CELL_IN_BOUNDS(x, y)
                      && gGame->m_map[x][y].m_triggerType == MAP_OBJECT_TRIGGER(MAP_OBJECT_NONE)
                      && gGame->m_map[x][y].m_objectIndex == MAP_CELL_NO_FRAME
                      && gGame->m_map[x][y].m_overlayIndex == MAP_CELL_NO_FRAME
                      && gGame->m_map[x][y].m_tileIndex >= MAP_CELL_TILES_PER_TERRAIN)
                ) {
                    triesCount++;
                    heading = 0;
                    while (heading == 0)
                        heading =
                            ULTIMATE_HINT_SCATTER - Random(0, 2) - Random(0, 2) - Random(0, 2);
                    x = gGame->m_ultimateArtifactX + heading;
                    heading = 0;
                    while (heading == 0)
                        heading =
                            ULTIMATE_HINT_SCATTER - Random(0, 2) - Random(0, 2) - Random(0, 2);
                    y = gGame->m_ultimateArtifactY + heading;
                    if (triesCount >= ULTIMATE_HINT_PLACE_TRIES) {
                        x = gGame->m_ultimateArtifactX;
                        y = gGame->m_ultimateArtifactY;
                        goto saveLocation;
                    }
                }
            saveLocation:
                gGame->m_players[player].m_ultimateArtifactHintX = x;
                gGame->m_players[player].m_ultimateArtifactHintY = y;
            }
        }
    }
}

i32 game::CountObelisksVisitedBy(i8 player) {
    i32 obelisk;
    i32 count;

    count = 0;
    for (obelisk = 0; obelisk < m_obeliskCount; obelisk++) {
        if (m_obeliskVisitors[obelisk] & (1 << player))
            count++;
    }
    return count;
}

void game::VisitObelisk(i8 player) {
    i16 curAttempts;
    i8 piecesRemoved;
    i8 fallback;
    i8 thePiece;
    i32 piecesTotal;
    i16 myRemovedNo;
    i32 removeCount;

    piecesTotal = PLAYER_PUZZLE_PIECE_COUNT;
    // The pieces that do not divide evenly go to the first obelisks the
    // player visits, so visiting every obelisk completes the puzzle.
    removeCount = piecesTotal / m_obeliskCount;
    if (CountObelisksVisitedBy(player) < piecesTotal % m_obeliskCount)
        removeCount++;
    if (removeCount < 1)
        removeCount = 1;
    for (myRemovedNo = 0; myRemovedNo < removeCount; myRemovedNo++) {
        piecesRemoved = m_players[player].CountPuzzlePiecesRemoved();
        for (thePiece = 0; thePiece < piecesTotal; thePiece += Random(1, 5)) {
            if (!BitTest(m_players[player].m_puzzlePiecesRemoved, thePiece))
                break;
        }
        for (curAttempts = 0; curAttempts < OBELISK_PIECE_PICK_TRIES; curAttempts++) {
            fallback = Random(0, piecesTotal - 1);
            if (!BitTest(m_players[player].m_puzzlePiecesRemoved, fallback))
                break;
        }
        if (thePiece < piecesTotal)
            BitSet(m_players[player].m_puzzlePiecesRemoved, thePiece);
        else
            BitSet(m_players[player].m_puzzlePiecesRemoved, fallback);
    }
    ComputeUALoc(player);
}

b8 game::IsMobile(i8 heroId) {
    if (heroId == HERO_ID_NONE)
        return false;
    hero* mobileHero = &m_heroRecs[heroId];
    i32
    terrainValue = CELL_TERRAIN(gAdvManager->GetCell(mobileHero->m_x, mobileHero->m_y));
    return mobileHero->m_remainingMobility >= CalcTerrainCost(
               terrainValue,
               mobileHero->m_direction & MAP_DIRECTION_DIAGONAL_BIT,
               mobileHero->m_remainingMobility,
               mobileHero->m_heroClass
           );
}

mapCell (*game::GetWorldMapData(void)) [MAP_CELL_GRID_SIZE] { return m_map; }

i8 game::CreateBoat(i8 x, i8 y) {
    i8 boatIdx = Scan(m_boatSlots, 0, GAME_BOAT_COUNT);
    if (boatIdx != GAME_TABLE_FREE) {
        m_boatSlots[boatIdx] = boatIdx;
        boatRecord* boat = &m_boats[boatIdx];
        boat->id = boatIdx;
        boat->x = x;
        boat->y = y;
        boat->direction = MAP_DIRECTION_EAST;
        boat->owner = gCurPlayer;
        mapCell* square = &m_map[x][y];
        boat->savedTriggerType = square->m_triggerType;
        boat->savedEventData = square->m_objectMetadata;
        square->m_triggerType = MAP_EVENT_TRIGGER(MAP_OBJECT_SHIP);
        square->m_objectMetadata = boatIdx;
    }
    return boatIdx;
}

i8 game::Scan(i8* array, i8 start, i8 length) {
    i8 i;
    for (i = start; i < start + length; ++i) {
        if (array[i] == GAME_TABLE_FREE)
            return i;
    }
    return GAME_TABLE_FREE;
}

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
                if (m_availableHeroes[ix] == HERO_AVAILABILITY_IN_TAVERN)
                    idx = ix;
            }
        }
    }
    if (idx != HERO_ID_NONE)
        return idx;
    else
        return 0;
}

i8 game::GetTownId(i8 x, i8 y) {
    for (i16 i = 0; i < GAME_TOWN_COUNT; ++i) {
        if (m_castleRecs[i].m_x == x && m_castleRecs[i].m_y == y)
            return i;
    }
    return GAME_TOWN_NONE;
}

i8 game::GetMineId(i8 x, i8 y) {
    for (i16 i = 0; i < GAME_MINE_COUNT; ++i) {
        if (m_mines[i].x == x && m_mines[i].y == y)
            return i;
    }
    return GAME_MINE_NONE;
}

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
    *ext = '\0';
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

i16 game::SaveGame(char* filename, b8 generateName) {
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
    SaveHeaderReserved reserved;

    gAdvManager->DemobilizeCurrHero();
    if (generateName) {
        if (m_campaignType > 0) {
            sprintf(genName, "%s.%s", filename, "CGM");
        } else {
            nHuman = 0;
            for (iFile = 0; iFile < GAME_PLAYER_COUNT; iFile++) {
                if (!m_playerDead[iFile] && gHumanPlayer[iFile])
                    nHuman++;
            }
            sprintf(genName, "%s.GM%d", filename, nHuman);
        }
    } else {
        sprintf(genName, filename);
    }
    if (!stricmp(genName, "REMOTE.GAM")) {
        sprintf(savePath, "%s%s", gDataPath, genName);
    } else {
        sprintf(savePath, "%s%s", gGamePath, genName);
        if (strnicmp(genName, localization::Tr("save.name.autosave"), SAVE_FILE_BASE_NAME_LENGTH)
            && strnicmp(
                genName,
                localization::Tr("save.name.player_exit"),
                SAVE_FILE_BASE_NAME_LENGTH
            ))
            strcpy(gGame->m_saveName, filename);
    }
    outFile = open(savePath, O_WRONLY | O_CREAT | O_TRUNC | O_BINARY, S_IWRITE);
    if (outFile == FILE_DESCRIPTOR_INVALID)
        FileError(savePath);
    WRITE_FILE_VALUE(outFile, gIAmGreatest);
    write(outFile, this, 2);
    WRITE_FILE_VALUE(outFile, gMonthType);
    WRITE_FILE_VALUE(outFile, gMonthTypeExtra);
    WRITE_FILE_VALUE(outFile, gWeekType);
    WRITE_FILE_VALUE(outFile, gWeekTypeExtra);
    WRITE_FILE_VALUE(outFile, m_campaignType);
    WRITE_FILE_VALUE(outFile, m_campaignScenario);
    WRITE_FILE_VALUE(outFile, m_campaignDay);
    WRITE_FILE_VALUE(outFile, m_campaignScenariosWon);
    memset(&reserved, 0, sizeof(reserved));
    memcpy(reserved.format.signature, SAVE_FORMAT_SIGNATURE, sizeof(reserved.format.signature));
    reserved.format.version = SAVE_FORMAT_CURRENT;
    write(outFile, &reserved, sizeof(reserved));
    write(outFile, m_mapDescription, sizeof(m_mapDescription));
    WRITE_FILE_VALUE(outFile, m_mapSize);
    WRITE_FILE_VALUE(outFile, m_mapDifficulty);
    write(outFile, m_mapName, sizeof(m_mapName));
    GenerateStandardFileName(m_saveName, buffer);
    write(outFile, buffer, 0x11);
    WRITE_FILE_VALUE(outFile, m_difficulty);
    WRITE_FILE_VALUE(outFile, m_playerCount);
    gSavedCurPlayer = gCurPlayer;
    WRITE_FILE_VALUE(outFile, gSavedCurPlayer);
    WRITE_FILE_VALUE(outFile, m_deadPlayerCount);
    write(outFile, m_playerDead, sizeof(m_playerDead));
    for (iFile = 0; iFile < GAME_PLAYER_COUNT; iFile++) {
        humans[iFile] = gHumanPlayer[iFile];
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
    write(outFile, gMapVisitFlags, sizeof(gMapVisitFlags));
    close(outFile);
    return 1;
}

// A spell's description; damage and healing spells state what the caster's
// spell power achieves (per point of spell power without a caster).
char* game::SpellDescription(i32 spell, hero* caster) {
    i32 multiplier;
    i32 power;

    switch (spell) {
        case SPELL_FIREBALL:
            multiplier = 10;
            break;
        case SPELL_LIGHTNING_BOLT:
        case SPELL_STORM:
        case SPELL_METEOR_SHOWER:
            multiplier = 25;
            break;
        case SPELL_RESURRECT:
        case SPELL_ARMAGEDDON:
            multiplier = 50;
            break;
        default:
            return gSpellDesc[spell];
    }
    power = caster ? caster->m_primaryStats[HERO_PRIMARY_SPELL_POWER] : 1;
    sprintf(gText, gSpellDesc[spell], power * multiplier);
    return gText;
}

// The quick save's file name, as SaveGame generates it: one per campaign,
// otherwise numbered by the human players still in the game.
void game::QuickSaveName(char* name) {
    i32 humans;
    i32 i;

    if (m_campaignType > 0) {
        sprintf(name, "%s.%s", "QUICKSAVE", "CGM");
    } else {
        humans = 0;
        for (i = 0; i < GAME_PLAYER_COUNT; i++) {
            if (!m_playerDead[i] && gHumanPlayer[i])
                humans++;
        }
        sprintf(name, "%s.GM%d", "QUICKSAVE", humans);
    }
}

// The original game did not reserve the heroes it drew for the taverns each
// week; reserve those on offer now, so they stay out of other taverns.
void game::UpgradeOriginalSave(void) {
    i32 player;
    i32 slot;
    i8 heroId;

    for (player = 0; player < GAME_PLAYER_COUNT; player++) {
        if (m_playerDead[player])
            continue;
        for (slot = 0; slot < PLAYER_TAVERN_HERO_COUNT; slot++) {
            heroId = m_players[player].m_availableHeroIds[slot];
            if (heroId >= 0 && heroId < GAME_HERO_COUNT
                && m_availableHeroes[heroId] == HERO_AVAILABILITY_UNAVAILABLE)
                m_availableHeroes[heroId] = HERO_AVAILABILITY_IN_TAVERN;
        }
    }
}

i16 game::LoadGame(char* filename, b32 origData, b32) {
    i32 saveFormat;
    i32 junk2;
    i32 numHumans;
    i32 ix;
    i32 theLoadHandle;
    char pathName[452];
    i8 theHumans[GAME_PLAYER_COUNT];
    i32 nextJunk;
    SaveHeaderReserved reserved;

    numHumans = 0;
    gGameOver = false;
    m_noMapHeroes = true;
    if (origData || !strcmp(filename, "REMOTE.GAM"))
        sprintf(pathName, "%s%s", gDataPath, filename);
    else
        sprintf(pathName, "%s%s", gGamePath, filename);
    theLoadHandle = open(pathName, O_BINARY);
    if (theLoadHandle == FILE_DESCRIPTOR_INVALID)
        FileError(pathName);
    ClearMapExtra();
    READ_FILE_VALUE(theLoadHandle, gIAmGreatest);
    read(theLoadHandle, this, 2);
    READ_FILE_VALUE(theLoadHandle, gMonthType);
    READ_FILE_VALUE(theLoadHandle, gMonthTypeExtra);
    READ_FILE_VALUE(theLoadHandle, gWeekType);
    READ_FILE_VALUE(theLoadHandle, gWeekTypeExtra);
    READ_FILE_VALUE(theLoadHandle, m_campaignType);
    READ_FILE_VALUE(theLoadHandle, m_campaignScenario);
    READ_FILE_VALUE(theLoadHandle, m_campaignDay);
    READ_FILE_VALUE(theLoadHandle, m_campaignScenariosWon);
    read(theLoadHandle, &reserved, sizeof(reserved));
    if (memcmp(reserved.format.signature, SAVE_FORMAT_SIGNATURE, sizeof(reserved.format.signature))
        == 0)
        saveFormat = reserved.format.version;
    else
        saveFormat = SAVE_FORMAT_ORIGINAL;
    // A network partner running another version sends a game this one
    // cannot play in step with.
    if (!strcmp(filename, "REMOTE.GAM") && saveFormat != SAVE_FORMAT_CURRENT)
        ShutDown(localization::Tr("network.version.mismatch"));
    read(theLoadHandle, m_mapDescription, sizeof(m_mapDescription));
    READ_FILE_VALUE(theLoadHandle, m_mapSize);
    READ_FILE_VALUE(theLoadHandle, m_mapDifficulty);
    read(theLoadHandle, m_mapName, sizeof(m_mapName));
    read(theLoadHandle, m_saveName, 0x11);
    sprintf(m_saveName, filename);
    READ_FILE_VALUE(theLoadHandle, m_difficulty);
    READ_FILE_VALUE(theLoadHandle, m_playerCount);
    READ_FILE_VALUE(theLoadHandle, gSavedCurPlayer);
    gCurPlayer = gSavedCurPlayer;
    READ_FILE_VALUE(theLoadHandle, m_deadPlayerCount);
    read(theLoadHandle, m_playerDead, sizeof(m_playerDead));
    read(theLoadHandle, theHumans, GAME_PLAYER_COUNT);
    for (ix = 0; ix < GAME_PLAYER_COUNT; ix++) {
        if ((theHumans[ix] || gDebugLevel >= GAME_DEBUG_LEVEL_ALL_HUMAN_MIN)
            && numHumans < gNumHumanPlayers) {
            numHumans++;
            gHumanPlayer[ix] = true;
        } else {
            gHumanPlayer[ix] = false;
        }
    }
    for (ix = 0; ix < GAME_PLAYER_COUNT; ix++) {
        if (gHumanPlayer[ix]) {
            if (!gRemoteOn || ix == gThisGamePos)
                gThisNetHumanPlayer[ix] = true;
            else
                gThisNetHumanPlayer[ix] = false;
        } else {
            gThisNetHumanPlayer[ix] = false;
        }
    }
    READ_FILE_VALUE(theLoadHandle, m_day);
    READ_FILE_VALUE(theLoadHandle, m_week);
    READ_FILE_VALUE(theLoadHandle, m_month);
    gCurTurn = GAME_DAY_NUMBER(*this);
    for (ix = 0; ix < GAME_PLAYER_COUNT; ix++)
        m_players[ix].Read(theLoadHandle);
    ReadWorldMap(theLoadHandle);
    READ_FILE_VALUE(theLoadHandle, m_obeliskCount);
    read(theLoadHandle, m_heroRecs, sizeof(m_heroRecs));
    if (origData) {
        for (ix = 0; ix < GAME_HERO_COUNT; ix++) {
            strcpy(m_heroRecs[ix].m_name, gHeroNames[ix][0]);
            strcpy(m_heroRecs[ix].m_shortName, gHeroNames[ix][1]);
        }
    }
    read(theLoadHandle, m_availableHeroes, sizeof(m_availableHeroes));
    read(theLoadHandle, m_castleRecs, sizeof(m_castleRecs));
    read(theLoadHandle, m_townOwners, sizeof(m_townOwners));
    memset(m_townBuiltToday, 0, sizeof(m_townBuiltToday));
    if (saveFormat < SAVE_FORMAT_TOWN_FLAGS) {
        read(theLoadHandle, m_townBuiltToday, GAME_TOWN_FLAG_BYTES_ORIGINAL);
        // Towns 32-35 set their flags in the first hero's id.
        m_heroRecs[0].m_id = 0;
    } else {
        read(theLoadHandle, m_townBuiltToday, sizeof(m_townBuiltToday));
    }
    read(theLoadHandle, m_mines, sizeof(m_mines));
    read(theLoadHandle, m_mineOwners, sizeof(m_mineOwners));
    read(theLoadHandle, m_randomArtifacts, sizeof(m_randomArtifacts));
    read(theLoadHandle, m_boats, sizeof(m_boats));
    read(theLoadHandle, m_boatSlots, sizeof(m_boatSlots));
    read(theLoadHandle, m_obeliskVisitors, sizeof(m_obeliskVisitors));
    READ_FILE_VALUE(theLoadHandle, m_ultimateArtifactX);
    READ_FILE_VALUE(theLoadHandle, m_ultimateArtifactY);
    READ_FILE_VALUE(theLoadHandle, m_ultimateArtifactId);
    if (origData) {
        memset(m_mapSounds, MAP_SOUND_NONE, sizeof(m_mapSounds));
        memset(m_mapExtra, 0, sizeof(m_mapExtra));
        memset(gMapVisitFlags, 0, sizeof(gMapVisitFlags));
        strcpy(gGame->m_saveName, localization::Tr("save.name.new_game"));
    } else {
        read(theLoadHandle, m_mapSounds, sizeof(m_mapSounds));
        read(theLoadHandle, m_mapExtra, sizeof(m_mapExtra));
        read(theLoadHandle, gMapVisitFlags, sizeof(gMapVisitFlags));
        if (strcmp(filename, "REMOTE.GAM"))
            strcpy(gGame->m_saveName, filename);
    }
    close(theLoadHandle);
    if (!origData && saveFormat == SAVE_FORMAT_ORIGINAL)
        UpgradeOriginalSave();
    gAdvManager->m_heroContextLocked = false;
    gCurPlayerData = &gGame->m_players[gCurPlayer];
    gCurPlayerBit = 1 << gCurPlayer;
    gCurWatchPlayer = gCurPlayer;
    while (!gThisNetHumanPlayer[gCurWatchPlayer])
        gCurWatchPlayer = (gCurWatchPlayer + 1) % m_playerCount;
    gCurWatchPlayerBit = 1 << gCurWatchPlayer;
    gCurPlayerHighBit = 1 << (gCurPlayer + GAME_PLAYER_HIGH_BIT_SHIFT);
    gCurWatchPlayerHighBit = 1 << (gCurWatchPlayer + GAME_PLAYER_HIGH_BIT_SHIFT);
    gShowIt = gThisNetHumanPlayer[gCurPlayer];
    memset(gMapExtra, 0, sizeof(gMapExtra));
    if (!origData)
        SetupAdjacentMons();
    return 1;
}

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
                    case NEW_GAME_DIFFICULTY_FIRST
                        + DIFFICULTY_NORMAL:
                        helpIndex = NEW_GAME_HELP_DIFFICULTY;
                        break;
                    case NEW_GAME_DIFFICULTY_FIRST
                        + DIFFICULTY_HARD:
                        helpIndex = NEW_GAME_HELP_DIFFICULTY;
                        break;
                    case NEW_GAME_DIFFICULTY_LAST:
                        helpIndex = NEW_GAME_HELP_DIFFICULTY;
                        break;
                    case NEW_GAME_OPPONENT_FIRST:
                    case NEW_GAME_OPPONENT_FIRST + 1:
                    case NEW_GAME_OPPONENT_LAST:
                        if (message.id - NEW_GAME_OPPONENT_TOGGLE_BASE < gNumHumanPlayers)
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
                            gGame->m_playerCount = 0;
                            for (i = 0; i < GAME_PLAYER_COUNT; i++) {
                                if ((gGame->m_players[i].m_difficulty)
                                    > PLAYER_TYPE_NONE)
                                    gGame->m_playerCount++;
                            }
                            if (gGame->m_playerCount < GAME_MIN_PLAYER_COUNT) {
                                NormalDialog(
                                    localization::Tr("game.opponent.required"),
                                    NORMAL_DIALOG_TYPE_OK,
                                    0xb1,
                                    0x3c
                                );
                                break;
                            } else {
                                if (!gGame->m_players[1].m_difficulty) {
                                    if (gGame->m_players[2].m_difficulty) {
                                        gGame->m_players[1].m_difficulty =
                                            gGame->m_players[2].m_difficulty;
                                        gGame->m_players[2].m_difficulty =
                                            PLAYER_TYPE_NONE;
                                    } else {
                                        gGame->m_players[1].m_difficulty =
                                            gGame->m_players[3].m_difficulty;
                                        gGame->m_players[3].m_difficulty =
                                            PLAYER_TYPE_NONE;
                                    }
                                }
                                if (!gGame->m_players[2].m_difficulty
                                    && gGame->m_players[3].m_difficulty) {
                                    gGame->m_players[2].m_difficulty =
                                        gGame->m_players[3].m_difficulty;
                                    gGame->m_players[3].m_difficulty =
                                        PLAYER_TYPE_NONE;
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
                        case NEW_GAME_DIFFICULTY_FIRST
                            + DIFFICULTY_NORMAL:
                        case NEW_GAME_DIFFICULTY_FIRST
                            + DIFFICULTY_HARD:
                        case NEW_GAME_DIFFICULTY_LAST:
                            gGame->m_difficulty = (message.id - NEW_GAME_DIFFICULTY_FIRST);
                            break;
                        case NEW_GAME_OPPONENT_FIRST:
                        case NEW_GAME_OPPONENT_FIRST + 1:
                        case NEW_GAME_OPPONENT_LAST:
                            iPlayer = message.id - NEW_GAME_OPPONENT_TOGGLE_BASE;
                            gGame->m_players[iPlayer].m_difficulty++;
                            gGame->m_players[iPlayer].m_difficulty %=
                                PLAYER_TYPE_COUNT;
                            if (iPlayer < gNumHumanPlayers
                                && !gGame->m_players[iPlayer].m_difficulty)
                                gGame->m_players[iPlayer].m_difficulty =
                                    HUMAN_HANDICAP_EASY;
                            break;
                        case NEW_GAME_COLOR:
                            gGame->m_players[0].m_color = (((gGame->m_players[0].m_color) + 1) % GAME_PLAYER_COUNT);
                            break;
                        case NEW_GAME_KING_OF_THE_HILL:
                            gIAmGreatest = 1 - gIAmGreatest;
                            break;
                        case NEW_GAME_SCENARIO_SELECT:
                        case NEW_GAME_SCENARIO_NAME:
                        case NEW_GAME_SCENARIO_PANEL:
                            game::GetMap();
                            break;
                        default:
                            break;
                    }
                    gGame->UpdateNewGameWindow();
                    gGame->m_newGameWindow->DrawWindow();
                    break;
                default:
                    break;
            }
        }
    }
    return MESSAGE_DISPATCH_CONSUME;
}

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
    for (i = DIFFICULTY_EASY; i < DIFFICULTY_COUNT; i++) {
        message.id = i + NEW_GAME_DIFFICULTY_FIRST;
        m_newGameWindow->BroadcastMessage(message);
    }
    message.command = WIDGET_COMMAND_SET_FLAGS;
    message.id = m_difficulty + NEW_GAME_DIFFICULTY_FIRST;
    m_newGameWindow->BroadcastMessage(message);
    message.command = WIDGET_COMMAND_SET_FRAME;
    for (i = 1; i < GAME_PLAYER_COUNT; i++) {
        message.id = i + NEW_GAME_OPPONENT_TOGGLE_BASE;
        if (i < gNumHumanPlayers)
            message.value = NEW_GAME_FRAME_HUMAN_OPPONENT;
        else
            message.value = m_players[i].m_difficulty + NEW_GAME_FRAME_COMPUTER_TYPE_BASE;
        m_newGameWindow->BroadcastMessage(message);
    }
    message.command = WIDGET_COMMAND_SET_TEXT;
    for (i = 1; i < GAME_PLAYER_COUNT; i++) {
        message.id = i + NEW_GAME_OPPONENT_LABEL_BASE;
        if (i < gNumHumanPlayers)
            message.text = gHumanPlayerTypeNames[m_players[i].m_difficulty];
        else
            message.text = gPlayerTypeNames[m_players[i].m_difficulty];
        m_newGameWindow->BroadcastMessage(message);
    }
    gGame->m_difficultyRating = CalcDifficultyRating();
    message.id = NEW_GAME_RATING;
    sprintf(
        gText,
        "%s %d%%",
        localization::Tr("ui.new_game.difficulty_rating"),
        gGame->m_difficultyRating
    );
    message.text = gText;
    m_newGameWindow->BroadcastMessage(message);
    message.command = WIDGET_COMMAND_SET_FRAME;
    if (m_players[0].m_color != PLAYER_COLOR_NONE) {
        message.id = NEW_GAME_COLOR;
        message.value =
            (m_players[0].m_color) * NEW_GAME_FRAME_CREST_STRIDE
            + NEW_GAME_FRAME_CREST_BASE;
        m_newGameWindow->BroadcastMessage(message);
    }
    message.command = WIDGET_COMMAND_SET_FRAME;
    message.id = NEW_GAME_KING_OF_THE_HILL;
    message.value = gIAmGreatest + NEW_GAME_FRAME_KING_OF_THE_HILL_BASE;
    m_newGameWindow->BroadcastMessage(message);
}

void game::GiveTroopsToNeutralTowns(void) {
    i32 howMany;
    i32 die;
    i32 i;
    i32 tierValue;
    i32 monster;
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
            switch (tierValue + (m_castleRecs[i].m_type)) {
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

char gCurMapName[16] = "";
i32 gEndSequence = GAME_END_LOST;
b32 gGameOver = false;

i8 game::NewGame(void) {
    static b8 gNewGameSettingsSaved = false;
    i32 player;
    if (!SetupGame(true))
        return 0;
    if (gCampaignChoice > CAMPAIGN_NONE) {
        InitEntireCampaign(gCampaignChoice);
        return 1;
    }
    if (gWaitForRemoteReceive)
        return 1;
    LoadGame("origdata.bin", true, false);
    m_newGameWindow = new heroWindow(310, 14, "newgame.bin");
    if (!m_newGameWindow)
        MemError();
    SetWinText(m_newGameWindow, WINDOW_TEXT_NEW_GAME);
    if (gNewGameSettingsSaved) {
        gGame->m_difficulty = gSavedDifficulty;
        m_players[1].m_difficulty = gSavedDifficulties[1];
        m_players[2].m_difficulty = gSavedDifficulties[2];
        m_players[3].m_difficulty = gSavedDifficulties[3];
        gIAmGreatest = gSavedKingOfTheHill;
        m_players[0].m_color = gSavedCrest;
        for (player = 1; player < gNumHumanPlayers; player++) {
            if ((m_players[player].m_difficulty)
                == HUMAN_HANDICAP_NONE)
                m_players[player].m_difficulty = m_players[0].m_difficulty;
        }
    }
    if (!strnicmp(gMapName, "camp", 4) || (gNumHumanPlayers == 1 && gMapName[4] != '1')
        || (gNumHumanPlayers == GAME_PLAYERS_TWO && gMapName[5] != '2')
        || (gNumHumanPlayers == GAME_PLAYERS_THREE && gMapName[6] != '3')
        || (gNumHumanPlayers == GAME_PLAYERS_FOUR && gMapName[7] != '4')) {
        if (gNumHumanPlayers == 1) {
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
    gMouseManager->ReallyShowPointer();
    gWindowManager->DoDialog(m_newGameWindow, NewGameHandler, false);
    delete m_newGameWindow;
    if (gWindowManager->m_dialogResult == DIALOG_BUTTON_1)
        return 0;
    strcpy(m_mapName, gFullMapName);
    strcpy(m_mapDescription, gMapDescription);
    m_mapSize = gMapSize;
    m_mapDifficulty = gMapDifficulty;
    strcpy(m_mapName, gFullMapName);
    gNewGameSettingsSaved = true;
    gSavedDifficulty = gGame->m_difficulty;
    gSavedDifficulties[1] = m_players[1].m_difficulty;
    gSavedDifficulties[2] = m_players[2].m_difficulty;
    gSavedDifficulties[3] = m_players[3].m_difficulty;
    gSavedKingOfTheHill = gIAmGreatest;
    gSavedCrest = m_players[0].m_color;
    NewMap(gMapName);
    return 1;
}

void game::ShowCampaignInfo(i32 scenario, b32 viewOnly, i32) {
    heroWindow* window;
    tag_message message;

    gMouseManager->SetPointer("advmice.mse", ADVENTURE_POINTER_DEFAULT);
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
    message.value = gGame->m_campaignScenariosWon + CAMPAIGN_INFO_PROGRESS_FRAME_BASE;
    window->BroadcastMessage(message);
    message.type = MESSAGE_WIDGET;
    message.command = WIDGET_COMMAND_CLEAR_FLAGS;
    message.value = WIDGET_FLAG_ENABLED | WIDGET_FLAG_DRAW;
    if (viewOnly) {
        message.id = DIALOG_BUTTON_2;
        window->BroadcastMessage(message);
    } else {
        message.id = DIALOG_BUTTON_0;
        window->BroadcastMessage(message);
        message.id = CAMPAIGN_INFO_RESTART;
        window->BroadcastMessage(message);
    }
    if (!viewOnly)
        PlayMusic(MUSIC_TRACK_MAIN_MENU);
    gWindowManager->DoDialog(window, EventWindowHandler, false);
    delete window;
    if (gWindowManager->m_dialogResult == CAMPAIGN_INFO_RESTART) {
        NormalDialog(localization::Tr("campaign.restart.confirm"), NORMAL_DIALOG_TYPE_YES_NO);
        if (gWindowManager->m_dialogResult == NORMAL_DIALOG_CONFIRM) {
            InitCampaignMap(m_campaignScenario, 0);
            gAdvManager->m_routeShown = false;
            gBottomViewOverride = BOTTOM_VIEW_NONE;
            gWindowManager->FadeScreen(WINDOW_FADE_OUT, WINDOW_FADE_SHORT, gPalette);
            gAdvManager->SetInitialMapOrigin();
            gAdvManager->RedrawAdvScreen(true);
            gWindowManager->FadeScreen(WINDOW_FADE_IN, WINDOW_FADE_SHORT, gPalette);
        }
    }
}

void game::InitEntireCampaign(i32 side) {
    LoadGame("origdata.bin", true, false);
    strcpy(gFullMapName, "");
    gGame->m_difficulty = DIFFICULTY_EXPERT;
    m_campaignType = side;
    m_campaignScenario = 0;
    m_campaignScenariosWon = 0;
    m_campaignDay = 1;
    InitCampaignMap(m_campaignScenario, 0);
}

void game::InitCampaignMap(i32 scenario, i32) {
    i32 saveTypeValue;
    i32 firstSavedScenario;
    i32 i;
    i32 resourceIdx;
    i32 savedWon;
    i32 savedDay;

    saveTypeValue = m_campaignType;
    firstSavedScenario = m_campaignScenario;
    savedWon = m_campaignScenariosWon;
    savedDay = m_campaignDay;
    LoadGame("origdata.bin", true, false);
    m_campaignType = saveTypeValue;
    m_campaignScenario = firstSavedScenario;
    m_campaignScenariosWon = savedWon;
    m_campaignDay = savedDay;
    m_month = (m_campaignDay - 1) / CALENDAR_DAYS_PER_MONTH + 1;
    m_week =
        (m_campaignDay - 1 - (m_month - 1) * CALENDAR_DAYS_PER_MONTH) / CALENDAR_DAYS_PER_WEEK + 1;
    m_day = (m_campaignDay - 1) % CALENDAR_DAYS_PER_WEEK + 1;
    gCurTurn = GAME_DAY_NUMBER(*this);
    gIAmGreatest = gCampaignScenarios[scenario].kingOfTheHill;
    gNumHumanPlayers = 0;
    m_players[0].m_difficulty = HUMAN_HANDICAP_EXPERT;
    m_players[0].m_color = gCampaignSideCrests[m_campaignType - 1][0];
    m_playerCount = 1;
    for (i = 1; i < GAME_PLAYER_COUNT; i++) {
        m_players[i].m_difficulty = gCampaignScenarios[scenario].playerTypes[i];
        if (m_players[i].m_difficulty)
            m_playerCount++;
    }
    gNumHumanPlayers = 1;
    sprintf(gMapName, "CAMP%d.CMP", scenario + 1);
    NewMap(gMapName);
    for (i = 0; i < GAME_PLAYER_COUNT; i++) {
        for (resourceIdx = RESOURCE_FIRST; resourceIdx < RESOURCE_COUNT; resourceIdx++)
            m_players[i].m_resources[resourceIdx] =
                gCampaignScenarios[scenario].resources[i][resourceIdx];
    }
}

void game::NewMap(char* mapName) {
    i32 nameId;
    i32 anyFreeValue;
    i8 savedHeroY;
    i8 myPosX;
    i8 yTown;
    i8 xTownVal;
    b32 prevHeroNo;
    i32 i;
    i32 j;
    i8 theTownId;
    i8 theUsed[GAME_TOWN_COUNT];
    i32 k;
    i32 ultimateSpread;
    i8 allNeutralVal;
    i32 curDifficulty;

    gInNewGameSetup = true;
    gCurPlayer = 0;
    gCurPlayerData = &gGame->m_players[gCurPlayer];
    gCurPlayerBit = 1 << gCurPlayer;
    gCurWatchPlayerBit = gCurPlayerBit;
    gCurPlayerHighBit = 1 << (gCurPlayer + GAME_PLAYER_HIGH_BIT_SHIFT);
    gCurWatchPlayerHighBit = 1 << (gCurPlayer + GAME_PLAYER_HIGH_BIT_SHIFT);
    gCurWatchPlayer = gCurPlayer;
    for (i = 0; i < m_playerCount; i++) {
        m_players[i].m_townCount = 0;
        m_players[i].m_townLocatorPage = 0;
        m_players[i].m_currentTown = GAME_TOWN_NONE;
        m_players[i].m_heroCount = 0;
        m_players[i].m_heroLocatorPage = 0;
        m_players[i].m_currentHero = HERO_ID_NONE;
    }
    memset(m_mapExtra, 0, sizeof(m_mapExtra));
    memset(gMapVisitFlags, 0, sizeof(gMapVisitFlags));
    RandomizeHeroPool();
    strcpy(gMapName, mapName);
    LoadMap(gMapName);
    RandomizeTerrainTiles();
    RandomizePlayerCrests();
    ProcessMapExtra();
    allNeutralVal = SetupTowns();
    ProcessRandomObjects(true);
    ProcessRandomObjects(false);
    RandomizeEvents();
    m_deadPlayerCount = 0;
    for (i = m_playerCount; i < GAME_PLAYER_COUNT; i++)
        m_playerDead[i] = true;
    for (i = 0; i < m_playerCount; i++) {
        m_players[i].m_ultimateArtifactHintChance = 0;
        m_players[i].m_ultimateArtifactHintX = PLAYER_ULTIMATE_HINT_NONE;
        m_players[i].m_ultimateArtifactHintY = PLAYER_ULTIMATE_HINT_NONE;
        prevHeroNo = false;
        if (allNeutralVal) {
            if (m_campaignType <= 0 || m_campaignScenario < CAMPAIGN_SCENARIO_LORD_FIRST
                || m_campaignScenario > CAMPAIGN_SCENARIO_LORD_LAST) {
                if (m_campaignType > 0) {
                    for (j = 0; j < GAME_PLAYER_COUNT; j++) {
                        if (GetTown(j)->m_type == gCrestTownTypes[m_players[i].m_color]) {
                            SetupTown(j, !gHumanPlayer[i]);
                            ClaimTown(j, i);
                        }
                    }
                } else {
                    theTownId = RandomScan(m_townOwners, 0, GAME_PLAYER_COUNT, 8);
                    if (theTownId == GAME_TABLE_FREE)
                        theTownId = Scan(m_townOwners, 0, GAME_PLAYER_COUNT);
                    SetupTown(theTownId, !gHumanPlayer[i]);
                    ClaimTown(theTownId, i);
                }
            }
        } else {
            for (j = 0; j < GAME_TOWN_COUNT; j++) {
                if (m_castleRecs[j].m_owner == i)
                    SetupTown(j, !gHumanPlayer[i]);
            }
        }
        if (m_noMapHeroes
            || (m_campaignType > 0 && m_campaignScenario >= CAMPAIGN_SCENARIO_LORD_FIRST
                && m_campaignScenario <= CAMPAIGN_SCENARIO_LORD_LAST && i == 0)) {
            m_players[i].m_heroCount = 1;
            if (m_campaignType > 0)
                m_players[i].m_heroIds[0] = GetNewHeroId(gCrestHeroClass[m_players[i].m_color]);
            else
                m_players[i].m_heroIds[0] = GetNewHeroId(
                    gTownHeroClass
                        [(m_castleRecs[m_players[i].m_townIds[0]].m_type)]
                );
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
        m_availableHeroes[m_players[i].m_availableHeroIds[0]] = HERO_AVAILABILITY_IN_TAVERN;
        k = (k + Random(1, 3)) % HERO_CLASS_COUNT;
        m_players[i].m_availableHeroIds[1] = GetNewHeroId(k);
        m_availableHeroes[m_players[i].m_availableHeroIds[1]] = HERO_AVAILABILITY_IN_TAVERN;
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
                m_castleRecs[k].m_buildings = (1 << BUILDING_SLOT_TENT);
                if (m_castleRecs[k].m_type == TOWN_TYPE_BARBARIAN)
                    m_castleRecs[k].m_buildings |=
                        (1 << BUILDING_SLOT_SPECIAL);
                SetupTown(k, false);
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
           || (gNumHumanPlayers == 1
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
    m_ultimateArtifactId = (Random( ARTIFACT_ULTIMATE_BOOK, ARTIFACT_ULTIMATE_LAST ));
    for (i = 0; i < m_playerCount; i++) {
        if (gHumanPlayer[i]) {
            if (i == 0)
                curDifficulty = m_difficulty;
            else
                curDifficulty = (m_players[i].m_difficulty - 1);
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
        nameId = 0;
        anyFreeValue = Scan(theUsed, 0, GAME_TOWN_COUNT);
        if (anyFreeValue != GAME_TABLE_FREE)
            nameId = RandomScan(theUsed, 0, GAME_TOWN_COUNT, GAME_TOWN_COUNT);
        anyFreeValue = nameId;
        m_castleRecs[i].m_nameIndex = anyFreeValue;
        theUsed[anyFreeValue] = 0;
    }
    for (i = 0; i < 4; i++)
        GiveTroopsToNeutralTowns();
    SetupAdjacentMons();
    gPhilAI->GetGameAIVars();
    gInNewGameSetup = false;
}

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

void game::RandomizeEvents(void) {
    u8 curOverTileset;
    u8 nextObjTileset;
    i16 y;
    i16 i;
    i16 j;
    i8 id;
    i32 siteNumIdx;
    i16 x;
    mapCell* myCell;
    i8 obeliskIdNum;

    obeliskIdNum = 1;
    siteNumIdx = 1;
    for (y = 0; y < MAP_CELL_GRID_SIZE; y++) {
        for (x = 0; x < MAP_CELL_GRID_SIZE; x++) {
            myCell = &m_map[x][y];
            switch (myCell->m_triggerType) {
                case MAP_EVENT_TRIGGER(MAP_OBJECT_GAZEBO):
                    myCell->m_objectMetadata = siteNumIdx;
                    siteNumIdx++;
                    break;
                case MAP_OBJECT_TRIGGER(MAP_OBJECT_WHIRLPOOL):
                    myCell->m_triggerType |= MAP_TRIGGER_EVENT;
                    break;
                case MAP_EVENT_TRIGGER(MAP_OBJECT_OBELISK):
                    myCell->m_objectMetadata = obeliskIdNum;
                    obeliskIdNum++;
                    break;
                case MAP_EVENT_TRIGGER(MAP_OBJECT_STATUE):
                    myCell->m_objectMetadata = MAP_EVENT_DATA_AVAILABLE;
                    break;
                case MAP_EVENT_TRIGGER(MAP_OBJECT_SKELETON):
                    myCell->m_objectMetadata =
                        Random(0, 9) == RANDOM_DECILE_3 ? SKELETON_ARTIFACT : SKELETON_EMPTY;
                    break;
                case MAP_EVENT_TRIGGER(MAP_OBJECT_DAEMON_CAVE):
                    switch (Random(0, 99) % 10) {
                        case RANDOM_DECILE_0:
                        case RANDOM_DECILE_1:
                        case RANDOM_DECILE_2:
                            myCell->m_objectMetadata = DAEMON_REWARD_EXPERIENCE;
                            break;
                        case RANDOM_DECILE_3:
                            myCell->m_objectMetadata = DAEMON_REWARD_ARTIFACT;
                            break;
                        case RANDOM_DECILE_4:
                        case RANDOM_DECILE_5:
                        case RANDOM_DECILE_6:
                            myCell->m_objectMetadata = DAEMON_REWARD_EXPERIENCE_GOLD;
                            break;
                        case RANDOM_DECILE_7:
                        case RANDOM_DECILE_8:
                        case RANDOM_DECILE_9:
                            myCell->m_objectMetadata = DAEMON_REWARD_RANSOM;
                            break;
                    }
                    break;
                case MAP_EVENT_TRIGGER(MAP_OBJECT_TREASURE_CHEST):
                    myCell->m_objectMetadata = Random(2, 4);
                    break;
                case MAP_EVENT_TRIGGER(MAP_OBJECT_CAMPFIRE):
                    myCell->m_objectMetadata = Random(4, 6) << CAMPFIRE_AMOUNT_SHIFT;
                    myCell->m_objectMetadata |= static_cast<i8>(Random(0, 5));
                    break;
                case MAP_EVENT_TRIGGER(MAP_OBJECT_ANCIENT_LAMP):
                    myCell->m_objectMetadata = Random(0, 3) + 2;
                    break;
                case MAP_EVENT_TRIGGER(MAP_OBJECT_SHIPWRECK):
                    if (x <= 0 || x >= MAP_CELL_GRID_SIZE - 1
                        || MAP_TRIGGER_OBJECT(m_map[x - 1][y].m_triggerType)
                               != MAP_OBJECT_SHIPWRECK) {
                        myCell->m_triggerType &= MAP_TRIGGER_TYPE_MASK;
                        break;
                    }
                    goto treasure;
                case MAP_EVENT_TRIGGER(MAP_OBJECT_GRAVEYARD):
                treasure:
                    switch (Random(0, 99) % 10) {
                        case RANDOM_DECILE_0:
                        case RANDOM_DECILE_1:
                        case RANDOM_DECILE_2:
                            myCell->m_objectMetadata = GHOST_SITE_SMALL;
                            break;
                        case RANDOM_DECILE_3:
                        case RANDOM_DECILE_4:
                        case RANDOM_DECILE_5:
                            myCell->m_objectMetadata = GHOST_SITE_MEDIUM;
                            break;
                        case RANDOM_DECILE_6:
                        case RANDOM_DECILE_7:
                        case RANDOM_DECILE_8:
                            myCell->m_objectMetadata = GHOST_SITE_LARGE;
                            break;
                        case RANDOM_DECILE_9:
                            myCell->m_objectMetadata = GHOST_SITE_HUGE;
                            break;
                    }
                    break;
                case MAP_EVENT_TRIGGER(MAP_OBJECT_STRAW_HUT):
                    myCell->m_objectMetadata = Random(10, 30);
                    break;
                case MAP_EVENT_TRIGGER(MAP_OBJECT_HOUSE):
                    myCell->m_objectMetadata = Random(20, 50);
                    break;
                case MAP_EVENT_TRIGGER(MAP_OBJECT_CABIN):
                    myCell->m_objectMetadata = Random(0, 127) % 4 + 1;
                    break;
                case MAP_EVENT_TRIGGER(MAP_OBJECT_DWARF_LOG_CABIN):
                    myCell->m_objectMetadata = Random(0, 98) % 3 + 1;
                    break;
                case MAP_EVENT_TRIGGER(MAP_OBJECT_PEASANT_LOG_CABIN):
                    myCell->m_objectMetadata = Random(20, 50);
                    break;
                case MAP_EVENT_TRIGGER(MAP_OBJECT_WATERWHEEL):
                    myCell->m_objectMetadata = MAP_EVENT_DATA_AVAILABLE;
                    break;
                case MAP_EVENT_TRIGGER(MAP_OBJECT_MONSTER):
                    if (!myCell->m_objectMetadata) {
                        myCell->m_objectMetadata =
                            GetRandomNumTroops(myCell->m_objectIndex);
                        if (Random(0, 99) <= 25
                            && myCell->m_objectIndex
                                   != CREATURE_GHOST)
                            myCell->m_objectMetadata |= MONSTER_WILLING_FLAG;
                    }
                    break;
                case MAP_EVENT_TRIGGER(MAP_OBJECT_RESOURCE):
                    myCell->m_objectMetadata = myCell->m_objectIndex;
                    if (myCell->m_objectIndex > 4)
                        myCell->m_objectMetadata -= RESOURCE_PILE_OBJECT_BASE;
                    switch (myCell->m_objectMetadata) {
                        case RESOURCE_WOOD:
                        case RESOURCE_ORE:
                            myCell->m_objectMetadata = Random(8, 16);
                            break;
                        case RESOURCE_GOLD:
                            myCell->m_objectMetadata = Random(5, 10);
                            break;
                        default:
                            myCell->m_objectMetadata = Random(3, 7);
                            break;
                    }
                    break;
                case MAP_EVENT_TRIGGER(MAP_OBJECT_SPELL_SHRINE):
                    switch (Random(0, 9)) {
                        case RANDOM_DECILE_0:
                        case RANDOM_DECILE_1:
                        case RANDOM_DECILE_2:
                        case RANDOM_DECILE_3:
                            myCell->m_objectMetadata =
                                gMageGuildSpellPool[MAGE_GUILD_STATE_LEVEL_1][Random(0, 7)]
                                + 1;
                            break;
                        case RANDOM_DECILE_4:
                        case RANDOM_DECILE_5:
                        case RANDOM_DECILE_6:
                        case RANDOM_DECILE_7:
                            myCell->m_objectMetadata =
                                gMageGuildSpellPool[MAGE_GUILD_STATE_LEVEL_2][Random(0, 7)]
                                + 1;
                            break;
                        default:
                            myCell->m_objectMetadata =
                                gMageGuildSpellPool[MAGE_GUILD_STATE_LEVEL_3][Random(0, 7)]
                                + 1;
                            break;
                    }
                    break;
                case MAP_EVENT_TRIGGER(MAP_OBJECT_DESERT_TENT):
                    myCell->m_objectMetadata = Random(10, 20);
                    break;
                case MAP_EVENT_TRIGGER(MAP_OBJECT_WAGON_CAMP):
                    if (x <= 0 || x >= MAP_CELL_GRID_SIZE - 1
                        || MAP_TRIGGER_OBJECT(m_map[x - 1][y].m_triggerType)
                               != MAP_OBJECT_WAGON_CAMP
                        || MAP_TRIGGER_OBJECT(m_map[x + 1][y].m_triggerType)
                               != MAP_OBJECT_WAGON_CAMP) {
                        myCell->m_triggerType &= MAP_TRIGGER_TYPE_MASK;
                        break;
                    }
                    myCell->m_objectMetadata = Random(30, 50);
                    break;
                case MAP_EVENT_TRIGGER(MAP_OBJECT_ARTIFACT):
                    switch (Random(0, 99) % 10) {
                        case RANDOM_DECILE_0:
                        case RANDOM_DECILE_1:
                        case RANDOM_DECILE_2:
                        case RANDOM_DECILE_3:
                        case RANDOM_DECILE_4:
                        case RANDOM_DECILE_5:
                            myCell->m_objectMetadata = ARTIFACT_EVENT_MODE_PICKUP;
                            break;
                        case RANDOM_DECILE_6:
                        case RANDOM_DECILE_7:
                            myCell->m_objectMetadata = ARTIFACT_EVENT_MODE_GUARDED;
                            break;
                        case RANDOM_DECILE_8:
                        case RANDOM_DECILE_9:
                            myCell->m_objectMetadata = ARTIFACT_EVENT_MODE_GOLD;
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
                    SetupTown(id, false);
                    break;
                case MAP_EVENT_TRIGGER(MAP_OBJECT_ALCHEMIST_LAB):
                case MAP_EVENT_TRIGGER(MAP_OBJECT_MINE):
                case MAP_EVENT_TRIGGER(MAP_OBJECT_SAWMILL):
                    id = GetMineId(x, y);
                    for (j = 0; j < MINE_FOOTPRINT_HEIGHT; j++) {
                        for (i = 0; i < MINE_FOOTPRINT_WIDTH; i++) {
                            if (!m_map[x + i][y - j].m_objectMetadata
                                || MAP_TRIGGER_OBJECT(m_map[x + i][y - j].m_triggerType)
                                       == MAP_TRIGGER_OBJECT(myCell->m_triggerType))
                                m_map[x + i][y - j].m_objectMetadata = id;
                        }
                    }
                    break;
                case MAP_EVENT_TRIGGER(MAP_OBJECT_WINDMILL):
                    myCell->m_objectMetadata = Random(1, 5);
                    break;
            }
        }
    }
    for (y = 0; y < MAP_CELL_GRID_SIZE; y++) {
        for (x = 0; x < MAP_CELL_GRID_SIZE; x++) {
            myCell = &m_map[x][y];
            if (myCell->m_objectIndex != MAP_CELL_NO_FRAME
                && myCell->m_overlayIndex != MAP_CELL_NO_FRAME) {
                nextObjTileset =
                    (myCell->m_objectTileset & MAP_CELL_TILESET_MASK);
                curOverTileset =
                    (myCell->m_overlayTileset & MAP_CELL_TILESET_MASK);
                if ((nextObjTileset == TILESET_MTN32 || nextObjTileset == TILESET_TREE32)
                    && (curOverTileset == TILESET_MTN32 || curOverTileset == TILESET_TREE32))
                    myCell->m_secondaryTrigger |= MAP_CELL_SECONDARY_BLOCKED;
            }
            SettleOverlay(x, y);
            if (x == 0 || y == 0 || x == MAP_CELL_GRID_SIZE - 1 || y == MAP_CELL_GRID_SIZE - 1) {
                switch (myCell->m_triggerType) {
                    case MAP_OBJECT_TRIGGER(MAP_OBJECT_MOUNTAINS):
                    case MAP_OBJECT_TRIGGER(MAP_OBJECT_MOUNTAINS_2):
                    case MAP_OBJECT_TRIGGER(MAP_OBJECT_MOUNTAINS_3):
                    case MAP_OBJECT_TRIGGER(MAP_OBJECT_MOUNTAINS_4):
                    case MAP_OBJECT_TRIGGER(MAP_OBJECT_TREES):
                    case MAP_OBJECT_TRIGGER(MAP_OBJECT_TREES_2):
                    case MAP_OBJECT_TRIGGER(MAP_OBJECT_TREES_3):
                    case MAP_OBJECT_TRIGGER(MAP_OBJECT_TREES_4):
                    case MAP_OBJECT_TRIGGER(MAP_OBJECT_TREES_5):
                        myCell->m_secondaryTrigger |= MAP_CELL_SECONDARY_BLOCKED;
                        break;
                }
            }
        }
    }
    for (y = 0; y < MAP_CELL_GRID_SIZE; y++) {
        for (x = 0; x < MAP_CELL_GRID_SIZE; x++) {
            myCell = &m_map[x][y];
            if (myCell->m_triggerType == MAP_OBJECT_TRIGGER(MAP_OBJECT_SHADOW))
                myCell->m_flags |= MAP_CELL_OBJECT_SHADOW_ONLY;
            if (myCell->m_triggerType & MAP_TRIGGER_EVENT) {
                switch (MAP_TRIGGER_OBJECT(myCell->m_triggerType)) {
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
                        myCell->m_triggerType -= MAP_TRIGGER_EVENT;
                        break;
                }
            }
        }
    }
}

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
    if (handle == FILE_DESCRIPTOR_INVALID)
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
            m_castleRecs[i].m_type = (type & MAP_TOWN_TYPE_MASK);
            if ((type & MAP_TOWN_TYPE_MASK) == TOWN_TYPE_BARBARIAN)
                m_castleRecs[i].m_buildings |= (1 << BUILDING_SLOT_SPECIAL);
            if (type < 0)
                m_castleRecs[i].m_buildings |= (1 << BUILDING_SLOT_CASTLE);
            else
                m_castleRecs[i].m_buildings |= (1 << BUILDING_SLOT_TENT);
        }
    }
    for (i = 0; i < GAME_MINE_COUNT; i++) {
        READ_FILE_VALUE(handle, x);
        READ_FILE_VALUE(handle, y);
        READ_FILE_VALUE(handle, type);
        if (x >= 0) {
            m_mines[i].x = x;
            m_mines[i].y = y;
            m_mines[i].type = type;
        }
    }
    read(handle, m_randomArtifacts, sizeof(m_randomArtifacts));
    READ_FILE_VALUE(handle, m_obeliskCount);
    read(handle, m_mapSounds, sizeof(m_mapSounds));
    if (theVersion >= MAP_EXTRA_VERSION) {
        READ_FILE_VALUE(handle, gMaxMapExtra);
        for (i = 1; i < gMaxMapExtra; i++) {
            READ_FILE_VALUE(handle, gMapExtraSizes[i]);
            gMapExtraBlocks[i] = malloc(gMapExtraSizes[i]);
            read(handle, gMapExtraBlocks[i], gMapExtraSizes[i]);
        }
    } else {
        gMaxMapExtra = 1;
    }
    close(handle);
    return 0;
}

void game::ClaimTown(i8 townId, i8 player) {
    i32 j;
    town* townRec;
    mapCell* cell;

    townRec = &m_castleRecs[townId];
    if (townRec->m_owner == player)
        return;
    if (m_townOwners[townId] != GAME_PLAYER_NONE)
        gGame->GetTown(townId)->Deallocate();
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
    cell->m_objectTileset |= TILESET_FLAG32
                             << MAP_CELL_EXTRA_TILESET_SHIFT;
    cell->m_extraFrame = (m_players[player].Color()) * 2;
    cell = &m_map[m_castleRecs[townId].m_x + 1][m_castleRecs[townId].m_y];
    cell->m_flags |= MAP_CELL_OBJECT_EXTRA;
    cell->m_objectTileset |= TILESET_FLAG32
                             << MAP_CELL_EXTRA_TILESET_SHIFT;
    cell->m_extraFrame = (m_players[player].Color()) * 2 + 1;
    SetVisibility(m_castleRecs[townId].m_x, m_castleRecs[townId].m_y, player, gVisRangeTown);
    CheckEndGame(false);
}

void game::ClaimMine(i8 mineId, i8 player) {
    i16 frame;
    mapCell* cellPtr;
    m_mines[mineId].owner = player;
    m_mineOwners[mineId] = player;
    switch ((m_mines[mineId].type)) {
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
    switch ((m_mines[mineId].type)) {
        case RESOURCE_MERCURY:
            cellPtr = &m_map[m_mines[mineId].x][m_mines[mineId].y - 2];
            break;
        case MAP_OBJECT_DRAGON_CITY:
            cellPtr = &m_map[m_mines[mineId].x - 1][m_mines[mineId].y - 3];
            break;
        case MAP_OBJECT_LIGHTHOUSE:
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
        cellPtr->m_overlayTileset |= TILESET_FLAG32
                                     << MAP_CELL_EXTRA_TILESET_SHIFT;
        cellPtr->m_extraFrame = frame + (m_players[player].Color());
    }
}

i8 game::ViewSpells(
    class hero* spellHero,
    i8 spellType,
    i16 (*callback)(struct tag_message&),
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
        gWindowManager->DoDialog(m_viewSpellsWindow, ViewSpellsHandler, false);
        delete m_viewSpellsWindow;
    }
    return m_viewSpell;
}

void game::SetupSpellRange(i16 spellType) {
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
            message.value =
                m_viewSpellsHero->m_spells[m_viewSpellsTop + i];
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
                            spell = gGame->m_viewSpellsHero->m_spells
                                        [gGame->m_viewSpellsTop
                                         + (message.id - SPELL_BOOK_ENTRY_FIRST)];
                            NormalDialog(
                                gGame->SpellDescription(spell, gGame->m_viewSpellsHero),
                                NORMAL_DIALOG_TYPE_QUICK_VIEW,
                                -1,
                                -1,
                                NORMAL_DIALOG_SPELL,
                                spell
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
                            if (gGame->m_viewSpellsReadOnly) {
                                spell = gGame->m_viewSpellsHero->m_spells
                                            [gGame->m_viewSpellsTop
                                             + (message.id - SPELL_BOOK_ENTRY_FIRST)];
                                NormalDialog(
                                    gGame->SpellDescription(spell, gGame->m_viewSpellsHero),
                                    NORMAL_DIALOG_TYPE_OK,
                                    -1,
                                    -1,
                                    NORMAL_DIALOG_SPELL,
                                    spell
                                );
                                return MESSAGE_DISPATCH_CONSUME;
                            }
                            gGame->m_viewSpell = gGame->m_viewSpellsHero->m_spells
                                                     [gGame->m_viewSpellsTop
                                                      + (message.id - SPELL_BOOK_ENTRY_FIRST)];
                            message.command = WIDGET_COMMAND_DIALOG_SELECT;
                            return MESSAGE_DISPATCH_FORWARD;
                        case SPELL_BOOK_PREVIOUS_PAGE:
                            if (gGame->m_viewSpellsTop == gGame->m_spellFirst)
                                break;
                            gGame->m_viewSpellsTop -= SPELL_BOOK_PAGE_SIZE;
                            if (gGame->m_viewSpellsTop < gGame->m_spellFirst)
                                gGame->m_viewSpellsTop = gGame->m_spellFirst;
                            gGame->UpdateSpellWidgets();
                            gGame->m_viewSpellsWindow->MoveWindow(0, 0);
                            break;
                        case SPELL_BOOK_NEXT_PAGE:
                            if (gGame->m_viewSpellsTop + SPELL_BOOK_PAGE_SIZE <= gGame->m_spellLast)
                                gGame->m_viewSpellsTop += SPELL_BOOK_PAGE_SIZE;
                            if (gGame->m_viewSpellsTop < gGame->m_spellFirst)
                                gGame->m_viewSpellsTop = gGame->m_spellFirst;
                            gGame->UpdateSpellWidgets();
                            gGame->m_viewSpellsWindow->MoveWindow(0, 0);
                            break;
                        case SPELL_BOOK_ADVENTURE_SPELLS:
                            gGame->SetupSpellRange(SPELL_TYPE_ADVENTURE);
                            gGame->m_viewSpellsTop = gGame->m_spellFirst;
                            gGame->UpdateSpellWidgets();
                            gGame->m_viewSpellsWindow->MoveWindow(0, 0);
                            break;
                        case SPELL_BOOK_COMBAT_SPELLS:
                            gGame->SetupSpellRange(SPELL_TYPE_COMBAT);
                            gGame->m_viewSpellsTop = gGame->m_spellFirst;
                            gGame->UpdateSpellWidgets();
                            gGame->m_viewSpellsWindow->MoveWindow(0, 0);
                            break;
                    }
                }
                break;
            case WIDGET_COMMAND_HOVER:
                if (gWindowManager->m_lastHoverId == message.id)
                    return MESSAGE_DISPATCH_CONSUME;
                else
                    return gGame->m_viewSpellsCallback(message);
                break;
        }
    }
    return MESSAGE_DISPATCH_CONSUME;
}

i16 ViewSpecialHandler(tag_message& message) {
    if (message.type == MESSAGE_WIDGET) {
        switch (message.command) {
            case WIDGET_COMMAND_HOVER:
                if (message.id == gWindowManager->m_lastHoverId)
                    return MESSAGE_DISPATCH_CONSUME;
                gWindowManager->m_lastHoverId = message.id;
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

void game::ViewArmy(
    i16 x,
    i16 y,
    i8 monsterType,
    i16 numTroops,
    class town* castle,
    b8 disableDismiss,
    i8 facing,
    i8 quickView,
    class hero* theHero,
    class army* theArmy,
    class armyGroup* theGroup
) {
    char localText[12];
    i32 shotCountNo;
    i16 baseX;
    i16 monsterLeft;
    i16 posY;
    i16 animWidgetId;
    i32 myMorale;
    tag_monsterInfo* monsterInfoObj;
    i16 countId;
    i32 m;
    char* statText;
    i32 savedLuck;
    tag_message origMessage;
    char iconNameBuffer[16];
    iconWidget* theMonsterWidget;
    i16 statLabel;
    i16 curTitleLabel;
    i32 stackModifier;
    i16 blankBtn;
    char theName[13];

    baseX = 86;
    posY = 164;
    blankBtn = VIEW_ARMY_COUNT_FRAME;
    countId = VIEW_ARMY_COUNT_TEXT;
    curTitleLabel = VIEW_ARMY_TITLE;
    statLabel = VIEW_ARMY_STATS;
    animWidgetId = VIEW_ARMY_ANIMATION;
    origMessage.type = MESSAGE_WIDGET;

    if (monsterType != CREATURE_SWORDSMAN)
        strcpy(iconNameBuffer, gArmySpriteNames[monsterType]);
    else
        strcpy(iconNameBuffer, "swrdsman");
    monsterInfoObj = &gMonsterDatabase[monsterType];
    m_viewArmyWindow = new heroWindow(x, y, "armywin.bin");
    if (!m_viewArmyWindow)
        MemError();
    monsterLeft = 30;
    if (monsterInfoObj->stats.attributes & MONSTER_FLAGS_WIDE) {
        switch (facing) {
            case ARMY_FACING_RIGHT:
                monsterLeft += 43;
                break;
            case ARMY_FACING_LEFT:
                monsterLeft += 119;
                break;
        }
    } else {
        monsterLeft += facing == ARMY_FACING_LEFT ? 76 : 86;
    }
    if (monsterInfoObj->stats.attributes & MONSTER_FLAGS_FLYING)
        sprintf(theName, "%s.wlk", iconNameBuffer);
    else
        sprintf(theName, "%s.wip", iconNameBuffer);
    theMonsterWidget = new iconWidget(
        monsterLeft,
        164,
        86,
        149,
        theName,
        0,
        (facing == ARMY_FACING_LEFT),
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

    stackModifier = 0;
    sprintf(gText, "%s%d", gArmyStatText[0], monsterInfoObj->stats.attack);
    strcat(statText, gText);
    if (theHero)
        stackModifier += theHero->m_primaryStats[HERO_PRIMARY_ATTACK];
    if (stackModifier) {
        sprintf(gText, " (%d)", monsterInfoObj->stats.attack + stackModifier);
        strcat(statText, gText);
    }

    stackModifier = 0;
    sprintf(gText, "\n%s%d", gArmyStatText[1], monsterInfoObj->stats.defense);
    strcat(statText, gText);
    if (theHero)
        stackModifier += theHero->m_primaryStats[HERO_PRIMARY_DEFENSE];
    if (theArmy && theArmy->m_spellEffect == SPELL_PROTECTION)
        stackModifier += ARMY_PROTECTION_DEFENSE_BONUS;
    if (stackModifier) {
        sprintf(gText, " (%d)", monsterInfoObj->stats.defense + stackModifier);
        strcat(statText, gText);
    }

    if (monsterInfoObj->stats.attributes & MONSTER_FLAGS_SHOOTER) {
        if (theArmy)
            shotCountNo = theArmy->m_stats.shots;
        else
            shotCountNo = monsterInfoObj->stats.shots;
        if (shotCountNo > 0) {
            if (gCombatManager->m_active == 1)
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
    // In combat the line also shows what is left of the top creature.
    if (gInCombat && theArmy)
        sprintf(
            gText,
            "\n%s%d (%d)",
            gArmyStatText[4],
            theArmy->m_stats.hitPoints - theArmy->m_hitPointsLost,
            monsterInfoObj->stats.hitPoints
        );
    else
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
    gTimers[VIEW_ARMY_TIMER_SLOT] = KBTickCount() + VIEW_ARMY_FRAME_DELAY;
    m_dialogAnimationCounter = 0;
    if (quickView) {
        gMouseManager->ReallyHidePointer();
        gWindowManager->AddWindow(m_viewArmyWindow, WINDOW_Z_ORDER_APPEND, 1);
        QuickViewWait();
        gWindowManager->RemoveWindow(m_viewArmyWindow);
        gMouseManager->ReallyShowPointer();
    } else {
        gWindowManager->DoDialog(m_viewArmyWindow, ViewArmyHandler, false);
        if (gDismissArmy && theGroup) {
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

i16 ViewArmyHandler(tag_message& message) {
    i16 frameDelay;
    i16 offset;
    gDismissArmy = false;
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
                        if (gWindowManager->m_dialogResult == NORMAL_DIALOG_CONFIRM) {
                            gDismissArmy = true;
                            message.command = (message.id = WIDGET_COMMAND_DIALOG_SELECT);
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
    if (gTimers[VIEW_ARMY_TIMER_SLOT] < KBTickCount()) {
        SET_WIDGET_MESSAGE(message, WIDGET_COMMAND_SET_FRAME, VIEW_ARMY_ANIMATION);
        gGame->m_dialogAnimationCounter++;
        message.value = gGame->m_dialogAnimationCounter % VIEW_ARMY_ANIMATION_FRAMES;
        gGame->m_viewArmyWindow->BroadcastMessage(message);
        gGame->m_viewArmyWindow->DrawWindow();
        gTimers[VIEW_ARMY_TIMER_SLOT] = KBTickCount() + VIEW_ARMY_FRAME_DELAY;
    }
    return MESSAGE_DISPATCH_CONSUME;
}

void game::Overview(void) {
    i16 activeTextW;
    i16 mineRowYVal;
    b8 redrawRequested;
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
    i16 j;
    font* smallFontItem;
    i16 theTotals[TOWN_TYPE_COUNT];
    i16 activeTownTextWPos;
    i16 dayIdYValue;
    i16 spacing;
    i16 theVal;
    i16 one;
    font* bigFont;
    i8 mineNumsBuffer[RESOURCE_COUNT];
    i16 spare1;
    i16 newHeroNumYW;
    i16 allLimitYOff;
    i16 shieldDXXVal;
    i16 heroClassCount;
    i16 oldNextType;
    i16 selCastleFrameY;
    i16 nextCastleIconY;
    i16 numMines;
    i16 savedFieldH;

    gAdvManager->TrimLoopingSounds(8);
    gOverviewShowing = true;
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
    redrawRequested = true;
    one = 1;
    dayIdYValue = 64;
    firstIncomeWidget = 65;

    gMouseManager->SetPointer("advmice.mse", ADVENTURE_POINTER_DEFAULT);
    bigFont = gResourceManager->GetFont("bigfont.fnt");
    smallFontItem = gResourceManager->GetFont("smalfont.fnt");
    gWindowManager->FadeScreen(WINDOW_FADE_OUT, WINDOW_FADE_SHORT, NULL);
    gResourceManager->GetBackdropAtLoc("overmain.bmp", gWindowManager->m_screen, 96, 0);
    sprintf(gText, "overban%01d.bmp", gCurPlayerData->m_color);
    gResourceManager->GetBackdropAtLoc(gText, gWindowManager->m_screen, 0, 0);
    ovIconRef = gResourceManager->GetIcon("overview.icn");

    memset(theTotals, 0, sizeof(theTotals));
    for (j = 0; j < gCurPlayerData->m_heroCount; j++)
        theTotals[m_heroRecs[gCurPlayerData->m_heroIds[j]].m_heroClass]++;
    heroClassCount = 0;
    for (j = 0; j < HERO_CLASS_COUNT; j++) {
        if (theTotals[j])
            heroClassCount++;
    }
    spacing = 136;
    left = 121;
    oldNextType = 0;
    for (j = 0; j < heroClassCount; j++) {
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
    for (j = 0; j < gCurPlayerData->m_townCount; j++) {
        if (m_castleRecs[gCurPlayerData->m_townIds[j]].m_buildings
            & (1 << BUILDING_SLOT_CASTLE))
            theTotals
                [(m_castleRecs[gCurPlayerData->m_townIds[j]].m_type)]++;
    }
    numCastlesVal = 0;
    for (j = 0; j < TOWN_TYPE_COUNT; j++) {
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
    for (j = 0; j < gCurPlayerData->m_townCount; j++) {
        if (!(m_castleRecs[gCurPlayerData->m_townIds[j]].m_buildings
              & (1 << BUILDING_SLOT_CASTLE)))
            theTotals
                [(m_castleRecs[gCurPlayerData->m_townIds[j]].m_type)]++;
    }
    numTowns = 0;
    for (j = 0; j < TOWN_TYPE_COUNT; j++) {
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
        if (m_mineOwners[j] == gCurPlayer)
            mineNumsBuffer[m_mines[j].type]++;
    }
    numMines = 0;
    for (j = 0; j < RESOURCE_COUNT; j++) {
        if (mineNumsBuffer[j])
            numMines++;
    }
    if (numMines) {
        spacing = 77;
        left = 100;
        oldNextType = 0;
        for (j = 0; j < numMines; j++) {
            while (!mineNumsBuffer[oldNextType])
                oldNextType++;
            ovIconRef->DrawToBuffer(
                left + spacing * j,
                289,
                __min(oldNextType, 2) + 12,
                ICON_DRAW_NORMAL,
                ICON_DRAW_OFFSET_FULL
            );
            if (oldNextType >= RESOURCE_ORE)
                ovIconRef->DrawToBuffer(
                    left + spacing * j,
                    289,
                    oldNextType + 14,
                    ICON_DRAW_NORMAL,
                    ICON_DRAW_OFFSET_FULL
                );
            sprintf(gText, "%d", mineNumsBuffer[oldNextType]);
            bigFont
                ->DrawBoundedString(gText, left + spacing * j, 355, 72, 16, 1, FONT_ALIGN_CENTER);
            oldNextType++;
        }
    }

    gWindowManager->UpdateScreenRegion(0, 0, LOGICAL_SCREEN_WIDTH, LOGICAL_SCREEN_HEIGHT);
    baseWin = new heroWindow(0, 0, "overwind.bin");
    if (!baseWin)
        MemError();
    SetWinText(baseWin, WINDOW_TEXT_OVERVIEW);
    SET_WIDGET_MESSAGE(activeMessage, WIDGET_COMMAND_SET_TEXT, OVERVIEW_DATE);
    sprintf(gText, gOverviewText[0], m_month, m_week, m_day);
    activeMessage.text = gText;
    baseWin->BroadcastMessage(activeMessage);
    activeMessage.id = OVERVIEW_DAILY_GOLD;
    sprintf(gText, "%d", ComputeDailyGold(gCurPlayer));
    baseWin->BroadcastMessage(activeMessage);
    for (j = 0; j < RESOURCE_COUNT; j++) {
        sprintf(gText, "%d", gCurPlayerData->m_resources[j]);
        activeMessage.id = j + OVERVIEW_RESOURCE_BASE;
        baseWin->BroadcastMessage(activeMessage);
    }
    gWindowManager->AddWindow(baseWin, WINDOW_Z_ORDER_APPEND, 1);
    baseWin->DrawWindow();
    gText[0] = 0;
    if (m_mineOwners[MINE_SLOT_DRAGON_CITY] == gCurPlayer) {
        strcpy(gText, gOverviewText[1]);
        smallFontItem->DrawBoundedString(gText, 100, 450, 400, 12, 1, FONT_ALIGN_LEFT);
        gWindowManager->UpdateScreenRegion(100, 450, 400, 12);
    }
    if (m_mineOwners[MINE_SLOT_LIGHTHOUSE] == gCurPlayer) {
        strcpy(gText, gOverviewText[2]);
        smallFontItem->DrawBoundedString(gText, 100, 465, 400, 12, 1, FONT_ALIGN_LEFT);
        gWindowManager->UpdateScreenRegion(100, 465, 400, 12);
    }
    gWindowManager->FadeScreen(WINDOW_FADE_IN, WINDOW_FADE_SHORT, NULL);
    gWindowManager->DoDialog(baseWin, TrueFalseDialogHandler, false);
    delete baseWin;
    gWindowManager->FadeScreen(WINDOW_FADE_OUT, WINDOW_FADE_SHORT, NULL);
    gResourceManager->Dispose(ovIconRef);
    gResourceManager->Dispose(smallFontItem);
    gResourceManager->Dispose(bigFont);
    gOverviewShowing = false;
}

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

void game::TurnOnAIMusic(void) {
    StopAllAudio();
    PlayMusic(MUSIC_TRACK_AI_TURN);
}

void game::TurnOffAIMusic(void) {}

void game::NextPlayer(void) {
    hero* currentHero;
    i32 numHumans;
    i32 ii;
    i32 remoteVal;
    char unused[20];

    gCurHourGlassPhase = 0;
    if (gThisNetHumanPlayer[gCurPlayer] && gConfig.autosave) {
        numHumans = 0;
        for (ii = 0; ii < GAME_PLAYER_COUNT; ii++) {
            if (!m_playerDead[ii] && gHumanPlayer[ii])
                numHumans++;
        }
        SaveGame(localization::Tr("save.name.autosave"), true);
    }
    if (gGame->m_players[gCurPlayer].m_daysLeft > 0)
        gGame->m_players[gCurPlayer].m_daysLeft--;
    CheckEndGame(false);
    gAdvManager->m_identifyHeroActive = false;
    gAdvManager->DeactivateCurrTown();
    gAdvManager->DeactivateCurrHero();
    do {
        gCurPlayer++;
        if (gCurPlayer >= m_playerCount) {
            gCurPlayer = 0;
            PerDay();
        }
    } while (gGame->m_playerDead[gCurPlayer]);
    gCurPlayerData = &gGame->m_players[gCurPlayer];
    gCurPlayerBit = 1 << gCurPlayer;
    gCurPlayerHighBit = 1 << (gCurPlayer + GAME_PLAYER_HIGH_BIT_SHIFT);
    for (ii = 0; ii < m_players[gCurPlayer].m_heroCount; ii++) {
        currentHero = &m_heroRecs[m_players[gCurPlayer].m_heroIds[ii]];
        currentHero->m_mobility = currentHero->CalcMobility();
        if (m_campaignType > 0
            && currentHero->m_x == gCampaignScenarios[m_campaignScenario].victoryTownX
            && currentHero->m_y == gCampaignScenarios[m_campaignScenario].victoryTownY)
            currentHero->m_mobility = 0;
        currentHero->m_remainingMobility = currentHero->m_mobility;
        if (gCheatUnlimitedMovement[gCurPlayer][currentHero->m_id])
            currentHero->m_remainingMobility = currentHero->m_mobility = CHEAT_UNLIMITED_MOBILITY;
    }
    if (!gThisNetHumanPlayer[gCurPlayer]) {
        gMouseManager->SetPointer(ADVENTURE_POINTER_WAIT);
        gAdvManager->HideRoute(true, false, true);
        gAdvManager->CheckDimNextHeroBut();
        TurnOnAIMusic();
        SetNoDialogMenus(0);
        gBottomViewOverride = BOTTOM_VIEW_OVERRIDE_DISABLED;
        ShowComputerScreen();
        gShowIt = false;
        if (gRemoteOn && (gHumanPlayer[gCurPlayer] || gThisGamePos != gHostGamePos)) {
            if (!gHumanPlayer[gCurPlayer])
                remoteVal = gHostGamePos;
            else
                remoteVal = gCurPlayer;
            if (!gGame->TransmitSaveGame(remoteVal, 0))
                ShutDown(NULL);
        }
        if (gBottomViewOverride == BOTTOM_VIEW_OVERRIDE_DISABLED)
            gBottomViewOverride = BOTTOM_VIEW_NONE;
    } else {
        SetNoDialogMenus(1);
        gInputManager->Flush();
        if (gBlackoutPlayer && gNumHumanPlayers > 1) {
            sprintf(
                gText,
                localization::Tr("turn.player.prompt"),
                gColorNames[gGame->m_players[gCurPlayer].m_color]
            );
            gText[0] = CyrillicToUpper(gText[0]);
            WaitForPlayer(gText, gCurPlayer);
        }
        if (gThisNetHumanPlayer[gCurPlayer])
            CancelComputerScreen();
        gCurWatchPlayerBit = gCurPlayerBit;
        gCurWatchPlayer = gCurPlayer;
        gCurWatchPlayerHighBit = 1 << (gCurPlayer + GAME_PLAYER_HIGH_BIT_SHIFT);
    }
    DoNewTurn();
    gMouseManager->ReallyShowPointer();
    CheckEndGame(false);
    if (gThisNetHumanPlayer[gCurPlayer] && gRemoteOn && m_day != 1
        && gForceSwitchMusic == FORCED_MUSIC_IDLE) {
        PlayMusic(MUSIC_TRACK_NETWORK_TURN);
        gForceSwitchMusic = KBTickCount();
    }
    if (gThisNetHumanPlayer[gCurPlayer])
        gAdvManager->ForceNewHover();
}

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
                (m_castleRecs[index].m_buildings
                 & (1 << BUILDING_SLOT_TENT))
                    ? DAILY_GOLD_TOWN
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
    if (!gHumanPlayer[player]) {
        if ((gGame->m_players[player].m_difficulty)
            == PLAYER_TYPE_DUMB)
            dailyGold = dailyGold * 0.75;
        if ((gGame->m_players[player].m_difficulty)
            == PLAYER_TYPE_AVERAGE)
            dailyGold = dailyGold * 1.0;
        if ((gGame->m_players[player].m_difficulty)
            == PLAYER_TYPE_SMART)
            dailyGold = dailyGold * 1.29;
        if ((gGame->m_players[player].m_difficulty)
            == PLAYER_TYPE_GENIUS)
            dailyGold = dailyGold * 1.45;
    }
    return dailyGold;
}

void game::PerDay(void) {
    i16 i;
    i16 theProduction;
    i16 ii;
    i16 j;
    i8 curResource;

    for (i = 0; i < gGame->m_playerCount; i++) {
        for (j = RESOURCE_FIRST; j < RESOURCE_COUNT; j++)
            gGame->m_players[i].m_aiData.m_income[j] = -m_players[i].m_resources[j];
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
        if (!gHumanPlayer[i]) {
            if ((gGame->m_players[i].m_difficulty)
                > PLAYER_TYPE_NO_WOOD_ORE_BONUS_LAST) {
                m_players[i].m_resources[RESOURCE_WOOD]++;
                m_players[i].m_resources[RESOURCE_ORE]++;
            }
            if ((gGame->m_players[i].m_difficulty)
                    > PLAYER_TYPE_NO_WEEKDAY_BONUS_LAST
                && m_day >= 1 && m_day <= RESOURCE_NON_GOLD_END)
                m_players[i].m_resources[(m_day - 1)] += 1;
        }
    }
    for (i = 0; i < GAME_HERO_COUNT; i++)
        m_heroRecs[i].m_fledState = HERO_FLED_NONE;
    m_day++;
    gCurTurn = GAME_DAY_NUMBER(*this);
    if (m_day > CALENDAR_DAYS_PER_WEEK) {
        m_day = 1;
        PerWeek();
    }
    if (m_week > CALENDAR_WEEKS_PER_MONTH) {
        m_week = 1;
        PerMonth();
    }
    for (i = 0; i < gGame->m_playerCount; i++) {
        for (j = RESOURCE_FIRST; j < RESOURCE_COUNT; j++)
            gGame->m_players[i].m_aiData.m_income[j] += m_players[i].m_resources[j];
    }
}

void game::PerWeek(void) {
    i16 posY;
    i16 posX;
    town* townPointer;
    i16 j;
    i16 i;
    i32 heroClass = 0;
    i8 previousHeroIds[PLAYER_TAVERN_HERO_COUNT];

    gWeekType = CALENDAR_PERIOD_NORMAL;
    gWeekTypeExtra = Random(0, CALENDAR_WEEK_NAME_COUNT - 1);
    if (m_week != CALENDAR_WEEKS_PER_MONTH) {
        i = Random(1, 4);
        if (i == 1) {
            gWeekType = CALENDAR_PERIOD_CREATURE;
            gWeekTypeExtra = Random(0, CALENDAR_WEEK_CREATURE_COUNT - 1);
        }
    }
    for (i = 0; i < GAME_TOWN_COUNT; i++) {
        townPointer = GetTown(i);
        for (j = BUILDING_SLOT_DWELLING_FIRST; j <= BUILDING_SLOT_DWELLING_LAST; j++) {
            if (townPointer->m_buildings & (1 << j)) {
                i16 gain = gMonsterDatabase[gDwellingType[townPointer->m_type]
                                                         [j - BUILDING_SLOT_DWELLING_FIRST]]
                               .growth;
                if (townPointer->m_buildings & (1 << BUILDING_SLOT_WELL))
                    gain += WEEKLY_WELL_GROWTH_BONUS;
                if (townPointer->m_owner >= 0 && !gHumanPlayer[townPointer->m_owner]) {
                    if ((gGame->m_players[townPointer->m_owner].m_difficulty)
                        == PLAYER_TYPE_SMART)
                        gain = gain * 1.24;
                    if ((gGame->m_players[townPointer->m_owner].m_difficulty)
                        == PLAYER_TYPE_GENIUS)
                        gain = gain * 1.36;
                }
                if (gWeekType == CALENDAR_PERIOD_CREATURE
                    && gDwellingType[townPointer->m_type][j - BUILDING_SLOT_DWELLING_FIRST]
                           == gWeekTypeExtra)
                    gain += WEEKLY_CREATURE_GROWTH_BONUS;
                townPointer->m_dwellingAvailable[j - BUILDING_SLOT_DWELLING_FIRST] += gain;
            }
        }
    }
    // Every hero drawn for a tavern is reserved there, so no hero is offered
    // in two taverns at once; last week's pair goes back to the pool only
    // after both slots are redrawn, so a tavern never offers it again
    // straight away. A dead player's tavern reserves nobody.
    for (i = 0; i < GAME_PLAYER_COUNT; i++) {
        for (j = 0; j < PLAYER_TAVERN_HERO_COUNT; j++) {
            heroClass = (Random(1, 3) + heroClass) % HERO_CLASS_COUNT;
            previousHeroIds[j] = gGame->m_players[i].m_availableHeroIds[j];
            gGame->m_players[i].m_availableHeroIds[j] = gGame->GetNewHeroId(heroClass);
            gGame->m_availableHeroes[gGame->m_players[i].m_availableHeroIds[j]] =
                m_playerDead[i] ? HERO_AVAILABILITY_UNAVAILABLE : HERO_AVAILABILITY_IN_TAVERN;
        }
        if (m_playerDead[i])
            continue;
        for (j = 0; j < PLAYER_TAVERN_HERO_COUNT; j++) {
            if (previousHeroIds[j] >= 0
                && previousHeroIds[j] != gGame->m_players[i].m_availableHeroIds[0]
                && previousHeroIds[j] != gGame->m_players[i].m_availableHeroIds[1]
                && gGame->m_availableHeroes[previousHeroIds[j]] == HERO_AVAILABILITY_IN_TAVERN)
                gGame->m_availableHeroes[previousHeroIds[j]] = HERO_AVAILABILITY_UNAVAILABLE;
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

void game::PerMonth(void) {
    static i8 gMonType[12] = {
        CREATURE_PEASANT,
        CREATURE_GOBLIN,
        CREATURE_DWARF,
        CREATURE_ELF,
        CREATURE_OGRE,
        CREATURE_DRUID,
        CREATURE_ORC,
        CREATURE_WOLF,
        CREATURE_CENTAUR,
        CREATURE_GARGOYLE,
        CREATURE_UNICORN,
        CREATURE_GRIFFIN
    };
    mapCell* spot;
    i32 x;
    i16 i;
    i32 y;
    i16 growth;
    town* townPointer;
    i16 j;

    m_month++;
    i = Random(1, 10);
    if (i <= 5) {
        gMonthType = CALENDAR_PERIOD_NORMAL;
        gMonthTypeExtra = Random(0, CALENDAR_MONTH_NAME_COUNT - 1);
    } else if (i <= 9) {
        gMonthType = CALENDAR_PERIOD_CREATURE;
        gMonthTypeExtra =
            gMonType[Random(0, CALENDAR_MONTH_CREATURE_COUNT - 1)];
    } else {
        gMonthType = CALENDAR_PERIOD_PLAGUE;
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
                if (gMonthType == CALENDAR_PERIOD_CREATURE
                    && gDwellingType[townPointer->m_type][j - BUILDING_SLOT_DWELLING_FIRST]
                           == gMonthTypeExtra)
                    townPointer->m_dwellingAvailable[j - BUILDING_SLOT_DWELLING_FIRST] *=
                        MONTHLY_CREATURE_GROWTH_FACTOR;
                if (gMonthType == CALENDAR_PERIOD_PLAGUE) {
                    townPointer->m_dwellingAvailable[j - BUILDING_SLOT_DWELLING_FIRST] -= growth;
                    if (townPointer->m_dwellingAvailable[j - BUILDING_SLOT_DWELLING_FIRST] < 0)
                        townPointer->m_dwellingAvailable[j - BUILDING_SLOT_DWELLING_FIRST] = 0;
                    townPointer->m_dwellingAvailable[j - BUILDING_SLOT_DWELLING_FIRST] =
                        townPointer->m_dwellingAvailable[j - BUILDING_SLOT_DWELLING_FIRST] >> 1;
                }
            }
        }
    }
    if (gMonthType == CALENDAR_PERIOD_CREATURE) {
        for (x = 0; x < MAP_CELL_GRID_SIZE; x++) {
            for (y = 0; y < MAP_CELL_GRID_SIZE; y++) {
                spot = gAdvManager->GetCell(x, y);
                if (!spot->m_triggerType && TERRAIN_IS_LAND(CELL_TERRAIN(spot))) {
                    if (Random(0, MONTH_CREATURE_SPAWN_ROLL_MAX) == MONTH_CREATURE_SPAWN_ROLL_HIT) {
                        spot->m_triggerType = MAP_EVENT_TRIGGER(MAP_OBJECT_MONSTER);
                        spot->m_objectTileset = TILESET_MONS32;
                        spot->m_objectIndex = gMonthTypeExtra;
                        spot->m_objectMetadata =
                            GetRandomNumTroops(gMonthTypeExtra);
                    }
                }
            }
        }
    }
    gAdvManager->CompleteDraw(false);
}

void game::RandomizeTown(i8 x, i8 y, b8 isCastle) {
    i8 j;
    b8 curUnique;
    town* town;
    i8 i;
    u8 activeFrameShift;
    i8 townNum;
    i8 race;
    b8 plain;

    townNum = GetTownId(x, y);
    for (j = 0; j < TOWN_FOOTPRINT_HEIGHT; j++) {
        for (i = 0; i < TOWN_FOOTPRINT_WIDTH; i++) {
            if (MAP_TRIGGER_OBJECT(
                    m_map[x - TOWN_FOOTPRINT_LEFT + i][y - TOWN_FOOTPRINT_TOP + j].m_triggerType
                ) > MAP_OBJECT_NONE
                && MAP_TRIGGER_OBJECT(
                       m_map[x - TOWN_FOOTPRINT_LEFT + i][y - TOWN_FOOTPRINT_TOP + j].m_triggerType
                   ) <= MAP_OBJECT_EVENT_LAST) {
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
        curUnique = false;
        race = TOWN_TYPE_KNIGHT;
        while (!curUnique) {
            race = (Random( TOWN_TYPE_KNIGHT, TOWN_TYPE_WARLOCK ));
            curUnique = true;
            for (i = 0; i < GAME_PLAYER_COUNT; i++) {
                if (gRandomTownTypes[i] == race)
                    curUnique = false;
            }
        }
        gRandomTownTypes[townNum] = race;
    } else {
        race = (Random( TOWN_TYPE_KNIGHT, TOWN_TYPE_WARLOCK ));
    }
    activeFrameShift = (TOWN_TYPE_COUNT - race) * TOWN_RACE_FRAME_STRIDE;
    for (i = 0; i < TOWN_FOOTPRINT_WIDTH; i++) {
        m_map[x - TOWN_FOOTPRINT_LEFT + i][y - TOWN_FOOTPRINT_TOP].m_overlayIndex -=
            activeFrameShift;
        m_map[x - TOWN_FOOTPRINT_LEFT + i][y - 1].m_objectIndex -= activeFrameShift;
        m_map[x - TOWN_FOOTPRINT_LEFT + i][y].m_objectIndex -= activeFrameShift;
    }
    m_castleRecs[townNum].m_type = race;
    plain = true;
    if (town->m_extraIndex >= 1
        && static_cast<mapTownExtra*>(gMapExtraBlocks[town->m_extraIndex])->customized)
        plain = false;
    if (plain) {
        m_castleRecs[townNum].m_buildings =
            race == TOWN_TYPE_BARBARIAN ? (1 << BUILDING_SLOT_SPECIAL) : 0;
    }
    if (isCastle) {
        // A random castle gets only its castle; dwellings follow the map's
        // customization or SetupTown's defaults.
        m_castleRecs[townNum].m_buildings |= (1 << BUILDING_SLOT_CASTLE);
        if (m_castleRecs[townNum].m_buildings & (1 << BUILDING_SLOT_TENT))
            m_castleRecs[townNum].m_buildings -= (1 << BUILDING_SLOT_TENT);
    } else {
        m_castleRecs[townNum].m_buildings |= (1 << BUILDING_SLOT_TENT);
        if (m_castleRecs[townNum].m_buildings & (1 << BUILDING_SLOT_CASTLE))
            m_castleRecs[townNum].m_buildings -=
                (1 << BUILDING_SLOT_CASTLE);
        SetupTown(townNum, false);
    }
}

void game::SetupTown(i8 townId, b8 aiOwned) {
    i16 dwellingCount;
    i32 x;
    i32 y;
    char rollList[10];
    i32 n;
    b8 nextUsed[SPELL_COUNT];
    i8 curTownType;
    i16 newSpell;
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
            if (m_castleRecs[townId].m_buildings
                & (1 << (n + BUILDING_SLOT_DWELLING_FIRST)))
                m_castleRecs[townId].m_dwellingAvailable[n] =
                    gMonsterDatabase[gDwellingType[curTownType][n]].growth;
        }
    }
    if (!m_castleRecs[townId].m_customized) {
        m_castleRecs[townId].m_buildings |= (1 << BUILDING_SLOT_DWELLING_1);
        m_castleRecs[townId].m_dwellingAvailable[0] =
            gMonsterDatabase[gDwellingType[curTownType][0]].growth;
        if (aiOwned && dwellingCount == 1 && Random(1, 10) < 4)
            dwellingCount++;
        if (--dwellingCount) {
            m_castleRecs[townId].m_buildings |=
                (1 << BUILDING_SLOT_DWELLING_2);
            m_castleRecs[townId].m_dwellingAvailable[1] =
                gMonsterDatabase[gDwellingType[curTownType][1]].growth;
            dwellingCount--;
        }
    }
    // Every cell of the town's footprint names the town, so clicks, quick
    // views and the radar find it from any of them; objects placed inside
    // the footprint keep their own data.
    for (x = m_castleRecs[townId].m_x - TOWN_FOOTPRINT_LEFT;
         x < m_castleRecs[townId].m_x - TOWN_FOOTPRINT_LEFT + TOWN_FOOTPRINT_WIDTH;
         x++) {
        for (y = m_castleRecs[townId].m_y - TOWN_FOOTPRINT_TOP;
             y < m_castleRecs[townId].m_y - TOWN_FOOTPRINT_TOP + TOWN_FOOTPRINT_HEIGHT;
             y++) {
            if (x < 0 || y < 0 || x >= MAP_CELL_GRID_SIZE || y >= MAP_CELL_GRID_SIZE)
                continue;
            if ((m_map[x][y].m_triggerType & MAP_TRIGGER_EVENT)
                && (m_map[x][y].m_triggerType & MAP_TRIGGER_TYPE_MASK) != MAP_OBJECT_TOWN)
                continue;
            m_map[x][y].m_objectMetadata = townId;
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
        nextUsed[newSpell] = true;
    }
}

void game::RandomizeMine(i8 x, i8 y) {
    u8 objFrame;
    i8 mineIdx;
    i8 iRow;
    i32 count;
    i8 iCol;
    u8 trigger;
    i8 resType;
    i8 terrain;
    u8 mineFrame;

    terrain = gGroundToTerrain[m_map[x][y].m_tileIndex];
    for (count = 0; count < RANDOM_MINE_TYPE_ROLLS; count++) {
        switch (terrain) {
            case TERRAIN_GRASS:
            case TERRAIN_DIRT:
                resType = (Random( RESOURCE_MERCURY, RESOURCE_GOLD ));
                if (resType == RESOURCE_MERCURY)
                    resType = RESOURCE_WOOD;
                break;
            case TERRAIN_SNOW:
                resType = (Random( RESOURCE_ORE, RESOURCE_GOLD ));
                break;
            case TERRAIN_SWAMP:
                resType = (Random( RESOURCE_WOOD, RESOURCE_GOLD ));
                break;
            case TERRAIN_LAVA:
                resType = RESOURCE_MERCURY;
                break;
            case TERRAIN_DESERT:
            default:
                resType = (Random( RESOURCE_MERCURY, RESOURCE_GOLD ));
                break;
        }
        if (!gMineTypeCount[resType])
            count = RANDOM_MINE_TYPE_ROLLS;
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
        m_map[x + 1][y].m_objectTileset |= TILESET_RSRC32
                                           << MAP_CELL_EXTRA_TILESET_SHIFT;
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

i8 game::GetRandomArtifactId(void) {
    i8
    freeSlot = (Scan( m_randomArtifacts, ARTIFACT_REGULAR_FIRST, ARTIFACT_REGULAR_END - ARTIFACT_REGULAR_FIRST ));
    if (freeSlot == ARTIFACT_NONE)
        return ARTIFACT_NONE;
    i8
    artifact = (RandomScan( m_randomArtifacts, ARTIFACT_REGULAR_FIRST, ARTIFACT_REGULAR_END - ARTIFACT_REGULAR_FIRST, ARTIFACT_REGULAR_END ));
    if (artifact == ARTIFACT_NONE)
        return freeSlot;
    else
        return artifact;
}

// A site's random artifact is chosen from where it lies and the heroes'
// random seeds, so reloading a saved game cannot change it. Artifacts that
// are already in play are skipped; with none left the site pays gold.
i8 game::CellRandomArtifactId(i32 cellIndex) {
    i32 freeCount;
    i32 pick;
    i8 artifact;

    freeCount = 0;
    for (artifact = ARTIFACT_REGULAR_FIRST; artifact < ARTIFACT_REGULAR_END; artifact++) {
        if (m_randomArtifacts[artifact] == GAME_TABLE_FREE)
            freeCount++;
    }
    if (freeCount == 0)
        return ARTIFACT_NONE;
    pick = static_cast<u8>(m_heroRecs
                               [(cellIndex + static_cast<u8>(m_heroRecs[0].m_randomSeed))
                                % GAME_ARTIFACT_SEED_HERO_COUNT]
                                   .m_randomSeed)
           % freeCount;
    for (artifact = ARTIFACT_REGULAR_FIRST; artifact < ARTIFACT_REGULAR_END; artifact++) {
        if (m_randomArtifacts[artifact] == GAME_TABLE_FREE) {
            if (pick == 0)
                return artifact;
            pick--;
        }
    }
    return ARTIFACT_NONE;
}

void game::RandomizeHeroPool(void) {
    i16 heroId;
    for (heroId = 0; heroId < GAME_HERO_COUNT; heroId++) {
        // Heroes start without experience; the draw is kept so the rest of
        // the game's random sequence stays as before.
        Random(0, 50);
        m_heroRecs[heroId].m_experience = 0;
        SetRandomHeroArmies(heroId, RANDOM_HERO_NORMAL_ARMY);
        m_heroRecs[heroId].m_remainingMobility = m_heroRecs[heroId].CalcMobility();
        m_heroRecs[heroId].m_mobility = m_heroRecs[heroId].m_remainingMobility;
        m_heroRecs[heroId].m_randomSeed = Random(1, 16000);
    }
}

void game::SetRandomHeroArmies(i16 heroId, i32 strongArmy) {
    armyGroup* curArmy = &m_heroRecs[heroId].m_army;
    i16 curSlot = 0;
    i16 armies[HERO_CLASS_COUNT][RANDOM_HERO_ARMY_OPTION_COUNT][RANDOM_HERO_ARMY_FIELD_COUNT] = {
        {{CREATURE_PEASANT, 30, 50},
         {CREATURE_ARCHER, 3, 5},
         {CREATURE_PIKEMAN, 2, 4}},
        {{CREATURE_GOBLIN, 15, 25},
         {CREATURE_ORC, 3, 5},
         {CREATURE_WOLF, 2, 3}},
        {{CREATURE_SPRITE, 10, 20},
         {CREATURE_DWARF, 2, 4},
         {CREATURE_ELF, 1, 2}},
        {{CREATURE_CENTAUR, 6, 10},
         {CREATURE_GARGOYLE, 2, 4},
         {CREATURE_GRIFFIN, 1, 2}}
    };
    b32 curPresent[RANDOM_HERO_ARMY_OPTION_COUNT];
    i32 i;
    i32 curMax;
    i32 minNum;

    curPresent[RANDOM_HERO_ARMY_OPTION_SURE] = true;
    curPresent[RANDOM_HERO_ARMY_OPTION_FIRST_ROLL] =
        Random(0, 99)
        < RANDOM_HERO_FIRST_STACK_CHANCE + (strongArmy ? RANDOM_HERO_FIRST_STACK_BONUS_CHANCE : 0);
    curPresent[RANDOM_HERO_ARMY_OPTION_SECOND_ROLL] =
        Random(0, 99) < RANDOM_HERO_SECOND_STACK_CHANCE
                            + (strongArmy ? RANDOM_HERO_SECOND_STACK_BONUS_CHANCE : 0);
    if (!curPresent[RANDOM_HERO_ARMY_OPTION_SECOND_ROLL])
        curPresent[RANDOM_HERO_ARMY_OPTION_FIRST_ROLL] = true;
    for (i = 0; i < ARMY_GROUP_SLOT_COUNT; i++) {
        curArmy->m_creatureTypes[i] = CREATURE_NONE;
        curArmy->m_creatureCounts[i] = RANDOM_HERO_EMPTY_COUNT;
    }
    for (i = 0; i < RANDOM_HERO_ARMY_SELECTION_COUNT; i++) {
        if (curPresent[i]) {
            curArmy->m_creatureTypes[curSlot] = (armies[m_heroRecs[heroId].m_heroClass][i][RANDOM_HERO_ARMY_FIELD_CREATURE]);
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

void game::ProcessRandomObjects(b32 castlesOnly) {
    mapCell* cellPtrItem;
    i32 lowFVVal;
    i32 y;
    i32 x;
    i32 i;
    i32 highFVNum;

    for (i = RESOURCE_FIRST; i < RESOURCE_COUNT; i++)
        gMineTypeCount[i] = 0;
    for (i = 0; i < GAME_PLAYER_COUNT; i++)
        gRandomTownTypes[i] = TOWN_TYPE_NONE;
    for (y = 0; y < MAP_CELL_GRID_SIZE; y++) {
        for (x = 0; x < MAP_CELL_GRID_SIZE; x++) {
            cellPtrItem = &m_map[x][y];
            if (!castlesOnly
                || cellPtrItem->m_triggerType == MAP_EVENT_TRIGGER(MAP_FILE_OBJECT_RANDOM_CASTLE)) {
                switch (cellPtrItem->m_triggerType) {
                    case MAP_EVENT_TRIGGER(MAP_FILE_OBJECT_RANDOM_TOWN):
                        RandomizeTown(x, y, false);
                        break;
                    case MAP_EVENT_TRIGGER(MAP_FILE_OBJECT_RANDOM_CASTLE):
                        RandomizeTown(x, y, true);
                        break;
                    case MAP_EVENT_TRIGGER(MAP_FILE_OBJECT_RANDOM_MONSTER):
                        lowFVVal = RANDOM_MONSTER_ANY_LOW;
                        highFVNum = RANDOM_MONSTER_ANY_HIGH;
                        goto pickMonster;
                    case MAP_EVENT_TRIGGER(MAP_FILE_OBJECT_RANDOM_MONSTER_WEAK):
                        lowFVVal = RANDOM_MONSTER_WEAK_LOW;
                        highFVNum = RANDOM_MONSTER_WEAK_HIGH;
                        goto pickMonster;
                    case MAP_EVENT_TRIGGER(MAP_FILE_OBJECT_RANDOM_MONSTER_MEDIUM):
                        lowFVVal = RANDOM_MONSTER_MEDIUM_LOW;
                        highFVNum = RANDOM_MONSTER_MEDIUM_HIGH;
                        goto pickMonster;
                    case MAP_EVENT_TRIGGER(MAP_FILE_OBJECT_RANDOM_MONSTER_STRONG):
                        lowFVVal = RANDOM_MONSTER_STRONG_LOW;
                        highFVNum = RANDOM_MONSTER_STRONG_HIGH;
                        goto pickMonster;
                    case MAP_EVENT_TRIGGER(MAP_FILE_OBJECT_RANDOM_MONSTER_VERY_STRONG):
                        lowFVVal = RANDOM_MONSTER_VERY_STRONG_LOW;
                        highFVNum = RANDOM_MONSTER_VERY_STRONG_HIGH;
                        goto pickMonster;
                    pickMonster:
                        cellPtrItem->m_triggerType = MAP_EVENT_TRIGGER(MAP_OBJECT_MONSTER);
                        cellPtrItem->m_objectIndex = Random(0, 27);
                        while (gMonsterDatabase
                                       [cellPtrItem->m_objectIndex]
                                           .fightValue
                                   <= lowFVVal
                               || gMonsterDatabase
                                          [cellPtrItem->m_objectIndex]
                                              .fightValue
                                      >= highFVNum)
                            cellPtrItem->m_objectIndex = Random(0, 27);
                        break;
                    case MAP_EVENT_TRIGGER(MAP_FILE_OBJECT_RANDOM_RESOURCE):
                        cellPtrItem->m_triggerType = MAP_EVENT_TRIGGER(MAP_OBJECT_RESOURCE);
                        cellPtrItem->m_objectIndex = Random(61, 67);
                        break;
                    case MAP_EVENT_TRIGGER(MAP_FILE_OBJECT_RANDOM_ARTIFACT):
                        cellPtrItem->m_triggerType = MAP_EVENT_TRIGGER(MAP_OBJECT_ARTIFACT);
                        cellPtrItem->m_objectIndex =
                            (GetRandomArtifactId());
                        m_randomArtifacts
                            [cellPtrItem->m_objectIndex] =
                                GAME_ARTIFACT_ON_MAP;
                        break;
                    case MAP_EVENT_TRIGGER(MAP_FILE_OBJECT_RANDOM_MINE):
                        RandomizeMine(x, y);
                        break;
                }
            }
        }
    }
}

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
                gGame->m_mapExtra[i][j] |= viewMask;
        }
    }
    if (gHumanPlayer[player]) {
        for (j = y - radius - 1; j <= y + radius + 1; ++j) {
            for (i = x - radius - 1; i <= x + radius + 1; ++i) {
                rangeLeft = radius - abs(y - j) + radius - abs(x - i);
                if (rangeLeft + 1 >= cutoff && i >= 0 && j >= 0 && i < MAP_CELL_GRID_SIZE
                    && j < MAP_CELL_GRID_SIZE)
                    gGame->m_mapExtra[i][j] |= outerMaskVal;
            }
        }
    }
}

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

i32 game::ExperienceValueOfStack(armyGroup* group, hero* heroPointer) {
    i32 expValue = 0;
    i32 i;
    for (i = 0; i < ARMY_GROUP_SLOT_COUNT; ++i) {
        if (group->m_creatureCounts[i] > 0)
            expValue +=
                group->m_creatureCounts[i] * gMonsterDatabase[group->m_creatureTypes[i]].hitPoints;
    }
    if (heroPointer)
        expValue += 500;
    return expValue;
}

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

i32 SRandom(i32 low, i32 high) {
    i32 result;
    SIncRandomize(low, high);
    result = SGenRand();
    gLastSeed += low;
    gLastSeed += high * 8;
    return result % (high - low + 1) + low;
}

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

void SRand(i32 seed) {
    gLastSeed = seed;
    srand(seed);
}

i32 game::GetLuck(hero* heroPointer, army*) {
    i32 luck;

    if (!heroPointer)
        return 0;
    luck = 0;
    if (heroPointer->HasArtifact(ARTIFACT_FIZBIN_OF_MISFORTUNE))
        luck -= GAME_FIZBIN_LUCK_PENALTY;
    if (heroPointer->HasArtifact(ARTIFACT_LUCKY_RABBITS_FOOT))
        luck++;
    if (heroPointer->HasArtifact(ARTIFACT_GOLDEN_HORSESHOE))
        luck++;
    if (heroPointer->HasArtifact(ARTIFACT_GAMBLERS_LUCKY_COIN))
        luck++;
    if (heroPointer->HasArtifact(ARTIFACT_FOUR_LEAF_CLOVER))
        luck++;
    luck += heroPointer->m_luck;
    if (luck < GAME_LUCK_MIN)
        luck = GAME_LUCK_MIN;
    if (luck > GAME_LUCK_MAX)
        luck = GAME_LUCK_MAX;
    return luck;
}

static i32 s_adjacentMonsterEndX;
static i32 s_adjacentMonsterEndY;
static i32 s_adjacentMonsterX;
static i32 s_adjacentMonsterY;
static i32 s_adjacentMonsterMinX;
static i32 s_adjacentMonsterMinY;

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

void game::SetupAdjacentMons(void) {
    i32 x;
    i32 y;
    i32 oldMask = 0x7f;
    i32 monY;
    i32 monX;

    for (x = 0; x < MAP_CELL_GRID_SIZE; ++x) {
        for (y = 0; y < MAP_CELL_GRID_SIZE; ++y) {
            if (gAdvManager->FindAdjacentMonster(x, y, &monX, &monY, -1, -1))
                gMapExtra[x][y] |= MAP_EXTRA_MONSTER_ADJACENT;
            else
                gMapExtra[x][y] &= oldMask;
        }
    }
}

void game::CancelComputerScreen(void) {
    TurnOffAIMusic();
    gShowIt = true;
    i32 i;
    for (i = ADVMGR_PANEL_BUTTON_FIRST; i <= ADVMGR_PANEL_BUTTON_LAST; ++i)
        gWindowManager->BroadcastMessage(
            MESSAGE_WIDGET,
            WIDGET_COMMAND_CLEAR_FLAGS,
            i,
            WIDGET_FLAG_UPDATE | WIDGET_FLAG_DIMMED
        );
}

void game::ShowComputerScreen(void) {
    if (gConfig.blackoutComputer || gRemoteOn) {
        b32 saved = gThisNetHumanPlayer[gCurPlayer];
        gThisNetHumanPlayer[gCurPlayer] = true;
        i32 i;
        for (i = ADVMGR_PANEL_BUTTON_FIRST; i <= ADVMGR_PANEL_BUTTON_LAST; ++i)
            gWindowManager->BroadcastMessage(
                MESSAGE_WIDGET,
                WIDGET_COMMAND_SET_FLAGS,
                i,
                WIDGET_FLAG_UPDATE | WIDGET_FLAG_DIMMED
            );
        gMouseManager->ReallyHidePointer();
        gAllBlack = true;
        gAdvManager->CompleteDraw(true);
        gAdvManager->UpdateHeroLocators(true, 1);
        gAdvManager->UpdateTownLocators(true, 1);
        gAdvManager->UpdBottomView(true, true, true);
        gAdvManager->UpdateScreen(false, true);
        gAllBlack = false;
        gThisNetHumanPlayer[gCurPlayer] = saved;
        gMouseManager->ReallyShowPointer();
    }
    ShowHeroesLogo();
}

void game::ShowHeroesLogo(void) {
    tileset* logo;
    if (!gAdvManager->m_heroesLogoShown) {
        gMouseManager->ReallyHidePointer();
        gAdvManager->m_heroesLogoShown = true;
        logo = gResourceManager->GetTileset("herologo.til");
        TileToBitmap(logo, 0, gWindowManager->m_screen, 480, 16);
        gWindowManager->UpdateScreenRegion(480, 16, 144, 144);
        gResourceManager->Dispose(logo);
        gMouseManager->ReallyShowPointer();
    }
}

void game::WaitForPlayer(char* text, i32 player) {
    if (gBlackoutPlayer && gNumHumanPlayers > 1 && !gRemoteOn) {
        gMouseManager->SetPointer(ADVENTURE_POINTER_DEFAULT);
        gAllBlack = true;
        gBottomViewOverrideEndTime = KBTickCount() + 9999999;
        if (gThisNetHumanPlayer[gCurPlayer])
            gBottomViewOverride = BOTTOM_VIEW_NEW_TURN;
        else
            gBottomViewOverride = BOTTOM_VIEW_NONE;
        PlayMusic(MUSIC_TRACK_NETWORK_TURN);
        gMouseManager->ReallyHidePointer();
        gAdvManager->CompleteDraw(true);
        gAdvManager->UpdateHeroLocators(true, 1);
        gAdvManager->UpdateTownLocators(true, 1);
        gAdvManager->UpdateScreen(false, true);
        ShowHeroesLogo();
        gAllBlack = false;
        gMouseManager->ReallyShowPointer();
        NormalDialog(
            text,
            NORMAL_DIALOG_TYPE_OK,
            0x61,
            -1,
            NORMAL_DIALOG_CREST,
            (gGame->m_players[player].m_color)
        );
        StopMusic();
    }
}

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

b32 game::OnTownFootprint(i32 x, i32 y) {
    i32 i;

    for (i = 0; i < GAME_TOWN_COUNT; i++) {
        if (x >= m_castleRecs[i].m_x - TOWN_FOOTPRINT_LEFT
            && x < m_castleRecs[i].m_x - TOWN_FOOTPRINT_LEFT + TOWN_FOOTPRINT_WIDTH
            && y >= m_castleRecs[i].m_y - TOWN_FOOTPRINT_TOP && y <= m_castleRecs[i].m_y)
            return true;
    }
    return false;
}

void game::ProcessMapExtra(void) {
    i32 y;
    mapCell* cell;
    i32 x;
    i8 townNum;
    i32 hidden;
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
                    m_noMapHeroes = false;
                    break;
            }
            // Some maps keep a hero, or a town away from every town, under
            // another object. Neither has a record: once the object was
            // gone, the cell showed and selected hero or town 0.
            hidden = MAP_TRIGGER_OBJECT(cell->m_secondaryTrigger);
            if (hidden == MAP_OBJECT_HERO || (hidden == MAP_OBJECT_TOWN && !OnTownFootprint(x, y)))
                cell->m_secondaryTrigger -= hidden;
        }
    }
}

i8 game::SetupTowns(void) {
    mapTownExtra* newExtra;
    i32 curOwn;
    b8 isUnowned;
    town* town;
    i32 j;
    i32 i;
    i32 mask;
    isUnowned = true;
    mask = MAP_TOWN_EXTRA_BUILDING_MASK;
    for (i = 0; i < GAME_TOWN_COUNT; i++) {
        town = GetTown(i);
        town->m_customized = false;
        if (town->m_extraIndex >= 1) {
            newExtra = static_cast<mapTownExtra*>(gMapExtraBlocks[town->m_extraIndex]);
            if (newExtra->customized && newExtra->owner != MAP_TOWN_OWNER_UNSET) {
                if (newExtra->owner >= gGame->m_playerCount)
                    curOwn = gGame->m_playerCount - 1;
                else
                    curOwn = newExtra->owner;
                isUnowned = false;
                if (curOwn != GAME_PLAYER_NONE)
                    ClaimTown(i, curOwn);
            }
            if (newExtra->customized) {
                town->m_customized = true;
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
            if ((loc->m_triggerType & MAP_TRIGGER_TYPE_MASK)
                == MAP_OBJECT_TRIGGER(MAP_FILE_OBJECT_HERO)) {
                extra = static_cast<mapHeroExtra*>(gMapExtraBlocks[loc->m_objectMetadata]);
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
                        gAdvManager->GiveArtifact(theHeroEntry, extra->artifacts[jx]);
                }
                theHeroEntry->m_experience = 0;
                gAdvManager->GiveExperience(theHeroEntry, extra->experience, true);
                theHeroEntry->CheckLevel();
                theHeroEntry->m_x = posX;
                theHeroEntry->m_y = posY;
                if (extra->owner >= gGame->m_playerCount)
                    iPlayer = gGame->m_playerCount - 1;
                else
                    iPlayer = extra->owner;
                theHeroEntry->m_owner = iPlayer;
                // Movement depends on the hero's artifacts and owner, both
                // known only now.
                theHeroEntry->m_mobility = theHeroEntry->CalcMobility();
                theHeroEntry->m_remainingMobility = theHeroEntry->m_mobility;
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
                        HERO_AVAILABILITY_IN_TAVERN;
                }
            }
        }
    }
    for (x = 0; x < MAP_CELL_GRID_SIZE; x++) {
        for (y = 0; y < MAP_CELL_GRID_SIZE; y++) {
            cell = gAdvManager->GetCell(x, y);
            if (cell->m_triggerType == MAP_EVENT_TRIGGER(MAP_OBJECT_HERO)) {
                if (cell->m_objectMetadata >= 0 && cell->m_objectMetadata < GAME_HERO_COUNT) {
                    boardHro = GetHero(cell->m_objectMetadata);
                    if (boardHro->m_owner < 0 || boardHro->m_owner > GAME_PLAYER_COUNT - 1) {
                        if (boardHro->m_locationType == MAP_EVENT_TRIGGER(MAP_OBJECT_TOWN)) {
                            townOccupied = gGame->GetTown(boardHro->m_occupiedTown);
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

i32 game::TransmitSaveGame(i32 remotePlayer, i32 playerExited) {
    b32 okay;
    char curPathname[452];
    i32 unusedSum;
    char* mainOutData;
    i32 block;
    i32 blocksCount;
    i32 unusedData;
    i32 oldTrackVal;
    RemotePayload* sendPacket;
    i32 segmentsInBlock;
    i32 entry;
    i32 totalSegments;
    i32 mainFile;
    i32 unusedOffset;
    RemoteMessage* incomingNow;
    char ackedArray[500];
    i32 unusedY;
    b32 replyState;
    i32 unusedSeq;
    i32 dataSize;
    i32 length;
    char* dataObj;
    b8 wasFinished;

    gAdvManager->TrimLoopingSounds(REMOTE_SAVE_TRANSFER_SOUNDS);
    okay = false;
    replyState = false;
    oldTrackVal = MUSIC_TRACK_NONE;
    oldTrackVal = GetCurrentTrack();
    StopMusic();

    if (gAdvManager->m_active == 1)
        BVResMsg(localization::Tr("network.send.title"), RESOURCE_NONE, 0);
    while (!gHeartbeatSeen) {
        PollSound();
        Process1WindowsMessage();
    }
    AiPrint("Transmit Start");
    memset(ackedArray, 0, sizeof(ackedArray));
    SaveGame("REMOTE.GAM", false);
    sprintf(curPathname, "%s%s", gDataPath, "REMOTE.GAM");
    dataSize = FileSize(curPathname);
    sendPacket = static_cast<RemotePayload*>(malloc(REMOTE_MESSAGE_SIZE));
    if (REMOTE_SAVE_ENCODED())
        mainOutData = static_cast<char*>(malloc(dataSize));
    dataObj = static_cast<char*>(malloc(dataSize));
    mainFile = open(curPathname, O_BINARY);
    if (mainFile == FILE_DESCRIPTOR_INVALID)
        FileError(curPathname);
    if (mainFile == FILE_DESCRIPTOR_INVALID) {
        goto cleanup;
    }
    {
        read(mainFile, dataObj, dataSize);
        close(mainFile);
        if (REMOTE_SAVE_ENCODED())
            dataSize = EncodeData(mainOutData, dataObj, dataSize);
        else
            mainOutData = dataObj;

        sendPacket->saveSize = dataSize;
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

        totalSegments = (dataSize - 1) / REMOTE_SAVE_SEGMENT_SIZE + 1;
        blocksCount = (totalSegments - 1) / REMOTE_SAVE_BATCH_SIZE + 1;
        for (block = 0; block < blocksCount; block++) {
            if (block + 1 == blocksCount)
                segmentsInBlock = totalSegments - block * REMOTE_SAVE_BATCH_SIZE;
            else
                segmentsInBlock = REMOTE_SAVE_BATCH_SIZE;
            wasFinished = false;
            while (!wasFinished) {
                for (entry = block * REMOTE_SAVE_BATCH_SIZE;
                     entry < block * REMOTE_SAVE_BATCH_SIZE + segmentsInBlock;
                     entry++) {
                    PollSound();
                    CheckDoMain(0, true);
                    if (!ackedArray[entry]) {
                        if (entry + 1 == totalSegments)
                            length = dataSize - entry * REMOTE_SAVE_SEGMENT_SIZE;
                        else
                            length = REMOTE_SAVE_SEGMENT_SIZE;
                        sendPacket->segment.index = entry;
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
                            false
                        );
                        if (!replyState)
                            ShutDown(NULL);
                    }
                }
                sendPacket->segment.index = block * REMOTE_SAVE_BATCH_SIZE;
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
                for (entry = 0; entry < segmentsInBlock; entry++) {
                    if (incomingNow->payload.data[entry] > 0)
                        ackedArray[entry + block * REMOTE_SAVE_BATCH_SIZE] = 1;
                }
                wasFinished = true;
                for (entry = block * REMOTE_SAVE_BATCH_SIZE;
                     entry < block * REMOTE_SAVE_BATCH_SIZE + segmentsInBlock;
                     entry++) {
                    if (!ackedArray[entry])
                        wasFinished = false;
                }
            }
        }
        replyState = TransmitRemoteData(NULL, remotePlayer, 0, REMOTE_COMMAND_SAVE_FINISH, true);
        if (!replyState)
            ShutDown(NULL);
        okay = true;
    }

cleanup:
    free(sendPacket);
    if (REMOTE_SAVE_ENCODED())
        free(mainOutData);
    free(dataObj);
    AiPrint("Transmit End");
    if (gAdvManager->m_active == 1) {
        gBottomViewOverride = BOTTOM_VIEW_NONE;
        gAdvManager->UpdBottomView(true, true, true);
    }
    if (oldTrackVal != MUSIC_TRACK_NONE) {
        PlayMusic(oldTrackVal);
    }
    return okay;
}

b32 game::ReceiveSaveGame(i32 dataSize, i32 remotePlayer) {
    b32 oldUnused1;
    b32 okay;
    char pathname[452];
    char* curInData;
    i32 i;
    i32 trackOld;
    i32 handleValue;
    b8 done;
    char* sendPacket;
    RemoteMessage* receivedPacketObj;
    i32 curRet;
    i32 lastPacketTimeNum;
    char myGotIt[500];
    i32 packetStartValue;
    char* decodedData;

    gAdvManager->TrimLoopingSounds(REMOTE_SAVE_TRANSFER_SOUNDS);
    handleValue = 0;
    done = false;
    oldUnused1 = false;
    okay = false;
    trackOld = MUSIC_TRACK_NONE;
    if (gAdvManager->m_active == 1)
        BVResMsg(localization::Tr("network.receive.title"), RESOURCE_NONE, 0);
    trackOld = GetCurrentTrack();
    StopMusic();
    while (!gHeartbeatSeen) {
        PollSound();
        Process1WindowsMessage();
    }
    curRet = TransmitRemoteData(NULL, remotePlayer, 0, REMOTE_COMMAND_SAVE_INIT_RESPONSE, true);
    if (!curRet)
        ShutDown(NULL);
    memset(myGotIt, 0, sizeof(myGotIt));
    if (REMOTE_SAVE_ENCODED())
        decodedData = static_cast<char*>(malloc(REMOTE_SAVE_DECODE_BUFFER_SIZE));
    sendPacket = static_cast<char*>(malloc(REMOTE_MESSAGE_SIZE));
    curInData = static_cast<char*>(malloc(dataSize + REMOTE_SAVE_BUFFER_EXTRA));
    lastPacketTimeNum = KBTickCount();
    while (!done) {
        PollSound();
        CheckDoMain(0, true);
        if (lastPacketTimeNum + REMOTE_WAIT_TIMEOUT < KBTickCount()) {
            NormalDialog(
                localization::Tr("combat.network.receive_error"),
                NORMAL_DIALOG_TYPE_YES_NO
            );
            if (gWindowManager->m_dialogResult == NORMAL_DIALOG_CONFIRM)
                lastPacketTimeNum = KBTickCount();
            else
                ShutDown(NULL);
        }
        receivedPacketObj = GetRemoteData(true);
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
                        true
                    );
                    if (!curRet)
                        ShutDown(NULL);
                    break;
                case REMOTE_COMMAND_SAVE_FINISH:
                    done = true;
                    break;
            }
        }
    }
    if (REMOTE_SAVE_ENCODED())
        dataSize = DecodeData(decodedData, curInData);
    else
        decodedData = curInData;
    sprintf(pathname, "%s%s", gDataPath, "REMOTE.GAM");
    handleValue = open(pathname, O_WRONLY | O_CREAT | O_TRUNC | O_BINARY, S_IWRITE);
    if (handleValue == FILE_DESCRIPTOR_INVALID)
        FileError(pathname);
    write(handleValue, decodedData, dataSize);
    close(handleValue);
    okay = true;
    free(sendPacket);
    free(curInData);
    if (REMOTE_SAVE_ENCODED())
        free(decodedData);
    AiPrint("Receive End");
    if (gAdvManager->m_active == 1) {
        gBottomViewOverride = BOTTOM_VIEW_NONE;
        gAdvManager->UpdBottomView(true, true, true);
    }
    if (trackOld != MUSIC_TRACK_NONE) {
        PlayMusic(trackOld);
    }
    return okay;
}

void game::DoNewTurn(void) {
    i32 track;
    char monsterName[52];

    if (!gThisNetHumanPlayer[gCurPlayer]) {
        CheckEndGame(false);
        return;
    }
    gBottomViewOverrideEndTime = KBTickCount() + 3000;
    gBottomViewOverride = BOTTOM_VIEW_NEW_TURN;
    gAdvManager->UpdBottomView(true, true, true);
    gAdvManager->SetInitialMapOrigin();
    gAdvManager->CompleteDraw(false);
    gAdvManager->UpdateScreen(false, false);
    CheckEndGame(false);
    if (gCurPlayerData->m_daysLeft >= 0) {
        if (gCurPlayerData->m_daysLeft == 1) {
            sprintf(
                gText,
                gNewTurnText[NEW_TURN_TEXT_LAST_DAY],
                gColorNames[gGame->m_players[gCurPlayer].Color()]
            );
            gText[0] = CyrillicToUpper(gText[0]);
        } else {
            sprintf(
                gText,
                gNewTurnText[NEW_TURN_TEXT_DAYS_LEFT],
                gColorNames[gGame->m_players[gCurPlayer].Color()],
                gCurPlayerData->m_daysLeft
            );
            gText[0] = CyrillicToUpper(gText[0]);
        }
        NormalDialog(
            gText,
            NORMAL_DIALOG_TYPE_OK,
            0x61,
            -1,
            NORMAL_DIALOG_CREST,
            (gGame->m_players[gCurPlayer].Color())
        );
    }
    if (gCurPlayerData->m_heroCount > 0)
        gAdvManager->SetHeroContext(gCurPlayerData->NextHero(0), false);
    else if (gCurPlayerData->m_townCount > 0)
        gAdvManager->SetTownContext(gCurPlayerData->m_townIds[0]);
    gAdvManager->CheckDimNextHeroBut();
    PlayMusic(TERRAIN_MUSIC_TRACK(gAdvManager->m_currentTerrain));
    if (m_day == 1) {
        if ((m_month != 1 || m_week != 1 || m_day != 1) && gWeekType != CALENDAR_PERIOD_NONE) {
            track = MUSIC_TRACK_NONE;
            if (m_week == 1) {
                track = MUSIC_TRACK_NEW_MONTH;
                if (gMonthType == CALENDAR_PERIOD_NORMAL) {
                    sprintf(
                        gText,
                        gNewTurnText[NEW_TURN_TEXT_MONTH_NORMAL],
                        gMonthNames[gMonthTypeExtra]
                    );
                } else if (gMonthType == CALENDAR_PERIOD_CREATURE) {
                    strcpy(
                        monsterName,
                        gArmyNamesPlural[gMonthTypeExtra]
                    );
                    monsterName[0] = CyrillicToLower(monsterName[0]);
                    sprintf(
                        gText,
                        gNewTurnText[NEW_TURN_TEXT_MONTH_CREATURE],
                        gArmyNamesPlural[gMonthTypeExtra],
                        monsterName
                    );
                } else {
                    sprintf(gText, gNewTurnText[NEW_TURN_TEXT_MONTH_PLAGUE]);
                }
            } else {
                track = MUSIC_TRACK_NEW_WEEK;
                if (gWeekType == CALENDAR_PERIOD_NORMAL) {
                    sprintf(
                        gText,
                        gNewTurnText[NEW_TURN_TEXT_WEEK_NORMAL],
                        gWeekNames[gWeekTypeExtra]
                    );
                } else {
                    strcpy(
                        monsterName,
                        gArmyNamesPlural[gWeekTypeExtra]
                    );
                    monsterName[0] = CyrillicToLower(monsterName[0]);
                    sprintf(
                        gText,
                        gNewTurnText[NEW_TURN_TEXT_WEEK_CREATURE],
                        gArmyNamesPlural[gWeekTypeExtra],
                        monsterName
                    );
                }
            }
            PlayMusic(track);
            gMouseManager->SetPointer(ADVENTURE_POINTER_DEFAULT);
            NormalDialog(gText, NORMAL_DIALOG_TYPE_OK, 0x61);
            PlayMusic(TERRAIN_MUSIC_TRACK(gAdvManager->m_currentTerrain));
        }
    }
}

i32 game::GetBoatsBuilt(void) {
    i32 count = 0;
    i32 i;
    for (i = 0; i < GAME_BOAT_COUNT; ++i) {
        if (m_boatSlots[i] != GAME_TABLE_FREE)
            ++count;
    }
    return count;
}

b8 gShowMapInfo = false;

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
    gShowMapInfo = true;
    strcpy(gCurMapName, "");
    gReqExtraWindow = new heroWindow(310, 332, "reqextra.bin");
    if (!gReqExtraWindow)
        MemError();
    if (gNumHumanPlayers == 1)
        sprintf(mask, "????1???.MAP");
    else if (gNumHumanPlayers == GAME_PLAYERS_TWO)
        sprintf(mask, "?????2??.MAP");
    else if (gNumHumanPlayers == GAME_PLAYERS_THREE)
        sprintf(mask, "??????3?.MAP");
    else if (gNumHumanPlayers == GAME_PLAYERS_FOUR)
        sprintf(mask, "???????4.MAP");
    theRequest = new fileRequester(310, 14, FILE_REQUESTER_LOAD, mask, gMapPath, ".MAP");
    if (!theRequest)
        MemError();
    theRequest->ShowMapInfo();
    code = gExec->DoDialog(theRequest);
    gWindowManager->RemoveWindow(gReqExtraWindow);
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
    gShowMapInfo = false;
}

i32 game::GetNumThievesGuilds(i32 player) {
    i32 numGuilds = 0;
    i32 i;
    for (i = 0; i < m_players[player].m_townCount; ++i) {
        if (gGame->m_castleRecs[m_players[player].m_townIds[i]].m_buildings
            & (1 << BUILDING_SLOT_THIEVES_GUILD))
            ++numGuilds;
    }
    return numGuilds;
}

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
        if (i < gNumHumanPlayers)
            total += (m_players[i].m_difficulty - 1) * 10;
        else if ((m_players[i].m_difficulty) == PLAYER_TYPE_NONE)
            total -= 10;
        else if ((m_players[i].m_difficulty) == PLAYER_TYPE_DUMB)
            total += 5;
        else if ((m_players[i].m_difficulty)
                 == PLAYER_TYPE_AVERAGE)
            total += 10;
        else if ((m_players[i].m_difficulty) == PLAYER_TYPE_SMART)
            total += 15;
        else if ((m_players[i].m_difficulty)
                 == PLAYER_TYPE_GENIUS)
            total += 20;
    }
    gGame->m_playerCount = 0;
    for (i = 0; i < GAME_PLAYER_COUNT; i++) {
        if ((gGame->m_players[i].m_difficulty) > PLAYER_TYPE_NONE)
            gGame->m_playerCount++;
    }
    if (gIAmGreatest) {
        if (m_playerCount - gNumHumanPlayers == 0) {
            total += 0;
        } else if (m_playerCount - gNumHumanPlayers == 1) {
            total += 0;
        } else if (m_playerCount - gNumHumanPlayers == GAME_PLAYERS_TWO) {
            total += 5;
        } else if (m_playerCount - gNumHumanPlayers == GAME_PLAYERS_THREE) {
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
    i32 baseDifficulty;
    heroWindow* scenWindow;
    tag_message packet;
    i16 startIdx;
    i32 pad;

    gMouseManager->SetPointer(ADVENTURE_POINTER_DEFAULT);
    scenWindow = new heroWindow(159, 14, "sceninfo.bin");
    if (!scenWindow)
        MemError();
    SET_WIDGET_MESSAGE(packet, WIDGET_COMMAND_SET_TEXT, nameIdIndex);
    packet.text = m_mapName;
    scenWindow->BroadcastMessage(packet);
    baseDifficulty = m_difficulty;
    if (gCurPlayer > 0)
        baseDifficulty = (gCurPlayerData->m_difficulty - 1);
    packet.id = levelIdIdx;
    packet.text = gDifficultyNames[baseDifficulty];
    scenWindow->BroadcastMessage(packet);
    packet.id = playersIdPos;
    packet.text = gText;
    sprintf(gText, "");
    for (jj = 1; jj < GAME_PLAYER_COUNT; jj++) {
        if (gCurPlayer == 0) {
            sprintf(
                line1Buf,
                "%s\n",
                gHumanPlayer[jj] ? gHandicapNames[m_players[jj].m_difficulty]
                                 : gPlayerTypeNames[m_players[jj].m_difficulty]
            );
        } else if (jj == 1) {
            sprintf(
                line1Buf,
                "%s\n",
                gHandicapNames[m_difficulty + 1]
            );
        } else {
            startIdx = jj - 1 < gCurPlayer ? jj - 1 : jj;
            sprintf(
                line1Buf,
                "%s\n",
                gHumanPlayer[startIdx] ? gHandicapNames[m_players[startIdx].m_difficulty]
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
        gIAmGreatest ? localization::Tr("scenario.info.yes") : localization::Tr("scenario.info.no")
    );
    scenWindow->BroadcastMessage(packet);
    packet.id = ratingId;
    sprintf(gText, "%d%%", gGame->m_difficultyRating);
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
    if (m_players[gCurPlayer].m_color != PLAYER_COLOR_NONE) {
        packet.id = crestId;
        packet.value = (m_players[gCurPlayer].m_color) * 2 + 11;
        scenWindow->BroadcastMessage(packet);
    }
    gWindowManager->DoDialog(scenWindow, EventWindowHandler, false);
}

void game::RandomizePlayerCrests(void) {
    i32 i;
    i32 crest;
    i8 taken[PLAYER_COLOR_COUNT];
    taken[PLAYER_COLOR_BLUE] = 0;
    taken[PLAYER_COLOR_GREEN] = 0;
    taken[PLAYER_COLOR_RED] = 0;
    taken[PLAYER_COLOR_YELLOW] = 0;
    taken[m_players[0].m_color] = 1;
    for (i = 1; i < m_playerCount; i++) {
        // A campaign scenario names the crests of the players after the
        // first (the enemy lords'); a crest it leaves open, or one already
        // taken, is drawn at random.
        crest = PLAYER_COLOR_NONE;
        if (m_campaignType > 0 && i - 1 < CAMPAIGN_CREST_COUNT)
            crest = gCampaignScenarios[m_campaignScenario].playerCrests[i - 1];
        if (crest < PLAYER_COLOR_BLUE || crest >= PLAYER_COLOR_COUNT || taken[crest] == 1) {
            do
                crest = Random(PLAYER_COLOR_BLUE, PLAYER_COLOR_YELLOW);
            while (taken[crest] == 1);
        }
        m_players[i].m_color = crest;
        taken[crest] = 1;
    }
}

void game::RestoreCell(i32 x, i32 y, i32 objectType, i32 barrier, mapCell* passedCell, i32) {
    mapCell* cell;
    if (passedCell)
        cell = passedCell;
    else
        cell = gAdvManager->GetCell(x, y);
    if (y > 0 && objectType == MAP_EVENT_TRIGGER(MAP_OBJECT_TOWN)
        && gAdvManager->GetCell(x, y - 1)->m_triggerType != MAP_OBJECT_TRIGGER(MAP_OBJECT_TOWN)) {
        cell->m_triggerType = MAP_OBJECT_TRIGGER(MAP_OBJECT_NONE);
        cell->m_objectMetadata = 0;
        return;
    }
    cell->m_triggerType = objectType;
    cell->m_objectMetadata = barrier;
}

i32 gLastSeed = 135621123;
i8 gSavedCurPlayer;
i8 gSavedCrest;
i8 gSavedDifficulty;
b8 gDismissArmy;
heroWindow* gReqExtraWindow;
i8 gSavedDifficulties[4];
i16 gMineTypeCount[RESOURCE_COUNT];
i8 gSavedKingOfTheHill;
i8 gRandomTownTypes[4];

#include <H1/Ints.h>

#include <SOURCE/saveRecords.h>

#include <BASE/IconEntry.h>
#include <BASE/resourceManager.h>
#include <SOURCE/fileRequester.h>
#include <SOURCE/game.h>
#include <SOURCE/highScoreManager.h>
#include <SOURCE/playerData.h>
#ifdef HOMM1_EDITOR
#include <EDITOR/editManager.h>

H1_STATIC_ASSERT(sizeof(editMapRecord) == EDIT_MAP_RECORD_SIZE, "editor map record layout");
H1_STATIC_ASSERT(sizeof(editHeroExtra) == EDIT_EXTRA_RECORD_MAX_SIZE, "editor hero extra layout");
H1_STATIC_ASSERT(sizeof(editTownExtra) <= EDIT_EXTRA_RECORD_MAX_SIZE, "editor town extra layout");
H1_STATIC_ASSERT(sizeof(editMapCellPair) == 4, "editor object owner layout");
#endif

#include <string.h>

// The structures the game still reads straight out of file data in memory
// (icon frame tables, map extra blocks) or keeps as file images must keep the
// original's byte layout.
H1_STATIC_ASSERT(sizeof(IconEntry) == ICON_ENTRY_RECORD_SIZE, "icon frame layout");
H1_STATIC_ASSERT(sizeof(mapTownExtra) == MAP_TOWN_EXTRA_RECORD_SIZE, "map town extra layout");
H1_STATIC_ASSERT(sizeof(mapHeroExtra) == MAP_HERO_EXTRA_RECORD_SIZE, "map hero extra layout");
H1_STATIC_ASSERT(sizeof(SMapHeader) == MAP_HEADER_RECORD_SIZE, "map header layout");
H1_STATIC_ASSERT(sizeof(HighScoreEntry) == HIGH_SCORE_RECORD_SIZE, "high score layout");
H1_STATIC_ASSERT(sizeof(aggEntry) == AGG_ENTRY_RECORD_SIZE, "archive entry layout");
H1_STATIC_ASSERT(sizeof(mapCell) == MAP_CELL_RECORD_SIZE, "map cell layout");
H1_STATIC_ASSERT(sizeof(i8) == 1 && sizeof(i16) == 2 && sizeof(i32) == 4, "integer widths");
H1_STATIC_ASSERT(sizeof(SaveFormatTag) == SAVE_FORMAT_TAG_RECORD_SIZE, "save format tag layout");
H1_STATIC_ASSERT(sizeof(SaveHeaderReserved) == SAVE_HEADER_RESERVED_RECORD_SIZE, "save header block layout");
H1_STATIC_ASSERT(
    static_cast<i32>(SAVE_FILE_RESERVED_SIZE) == static_cast<i32>(SAVE_HEADER_RESERVED_RECORD_SIZE),
    "save header block size"
);
H1_STATIC_ASSERT(sizeof(SAVE_FORMAT_SIGNATURE) == sizeof(((SaveFormatTag*)0)->signature) + 1, "save signature");

void WriteArmyGroup(RecordWriter& out, const armyGroup& group) {
    out.Put(group.m_creatureTypes, ARMY_GROUP_SLOT_COUNT);
    for (i32 slot = 0; slot < ARMY_GROUP_SLOT_COUNT; slot++)
        out.Put(group.m_creatureCounts[slot]);
}

void ReadArmyGroup(RecordReader& in, armyGroup& group) {
    in.Get(group.m_creatureTypes, ARMY_GROUP_SLOT_COUNT);
    for (i32 slot = 0; slot < ARMY_GROUP_SLOT_COUNT; slot++)
        group.m_creatureCounts[slot] = in.GetI16();
}

void playerData::Write(RecordWriter& file) {
    char unused[52];
    i32 index;

    file.Put(m_unused00, sizeof(m_unused00));
    file.Put(m_color);
    file.Put(m_difficulty);
    file.Put(m_heroCount);
    file.Put(m_currentHero);
    file.Put(m_heroLocatorPage);
    file.Put(m_heroIds, sizeof(m_heroIds));
    file.Put(m_availableHeroIds, sizeof(m_availableHeroIds));
    memset(unused, 0, PLAYER_SAVE_PAD_SIZE);
    file.Put(unused, PLAYER_SAVE_PAD_SIZE);
    file.Put(m_ultimateArtifactHintChance);
    file.Put(m_ultimateArtifactHintX);
    file.Put(m_ultimateArtifactHintY);
    file.Put(m_daysLeft);
    file.Put(m_townCount);
    file.Put(m_currentTown);
    file.Put(m_townLocatorPage);
    file.Put(m_townIds, sizeof(m_townIds));
    for (index = 0; index < RESOURCE_COUNT; index++)
        file.Put(m_resources[index]);
    for (index = 0; index < RESOURCE_COUNT; index++)
        file.Put(m_aiData.m_income[index]);
    file.Put(m_unused9a);
    file.Put(m_unused9a);
    file.Put(m_puzzlePiecesRemoved, sizeof(m_puzzlePiecesRemoved));
}

void playerData::Read(RecordReader& file) {
    char unused[52];
    i32 index;

    file.Get(m_unused00, sizeof(m_unused00));
    file.Get(m_color);
    file.Get(m_difficulty);
    file.Get(m_heroCount);
    file.Get(m_currentHero);
    file.Get(m_heroLocatorPage);
    file.Get(m_heroIds, sizeof(m_heroIds));
    file.Get(m_availableHeroIds, sizeof(m_availableHeroIds));
    file.Get(unused, PLAYER_SAVE_PAD_SIZE);
    file.Get(m_ultimateArtifactHintChance);
    file.Get(m_ultimateArtifactHintX);
    file.Get(m_ultimateArtifactHintY);
    file.Get(m_daysLeft);
    file.Get(m_townCount);
    file.Get(m_currentTown);
    file.Get(m_townLocatorPage);
    file.Get(m_townIds, sizeof(m_townIds));
    for (index = 0; index < RESOURCE_COUNT; index++)
        m_resources[index] = file.GetI32();
    for (index = 0; index < RESOURCE_COUNT; index++)
        m_aiData.m_income[index] = file.GetI32();
    file.Get(m_unused9a);
    file.Get(m_unused9a);
    file.Get(m_puzzlePiecesRemoved, sizeof(m_puzzlePiecesRemoved));
}

void WriteHero(RecordWriter& out, const hero& record) {
    out.Put(record.m_id);
    out.Put(record.m_owner);
    out.Put(record.m_name, sizeof(record.m_name));
    out.Put(record.m_shortName, sizeof(record.m_shortName));
    out.Put(record.m_heroClass);
    out.Put(record.m_portrait);
    out.Put(record.m_x);
    out.Put(record.m_y);
    out.Put(record.m_destinationX);
    out.Put(record.m_destinationY);
    out.Put(record.m_direction);
    out.Put(record.m_locationType);
    out.Put(record.m_locationMetadata);
    out.Put(record.m_mobility);
    out.Put(record.m_remainingMobility);
    out.Put(record.m_experience);
    out.Put(record.m_unused2d);
    out.Put(record.m_level);
    out.Put(record.m_primaryStats, HERO_STARTING_STAT_COUNT);
    out.Put(record.m_morale);
    out.Put(record.m_luck);
    out.Put(record.m_cowardice);
    out.Put(record.m_fledState);
    out.Put(record.m_visitedSites);
    out.Put(record.m_randomSeed);
    out.Put(record.m_unused3f, sizeof(record.m_unused3f));
    WriteArmyGroup(out, record.m_army);
    out.Put(record.m_spells, HERO_SPELL_SLOT_COUNT);
    out.Put(record.m_spellCharges, HERO_SPELL_SLOT_COUNT);
    out.Put(record.m_artifacts, HERO_ARTIFACT_SLOT_COUNT);
    out.Put(record.m_eventFlags);
    out.Put(record.m_aiFightValue);
}

void ReadHero(RecordReader& in, hero& record) {
    in.Get(record.m_id);
    in.Get(record.m_owner);
    in.Get(record.m_name, sizeof(record.m_name));
    in.Get(record.m_shortName, sizeof(record.m_shortName));
    in.Get(record.m_heroClass);
    in.Get(record.m_portrait);
    in.Get(record.m_x);
    in.Get(record.m_y);
    in.Get(record.m_destinationX);
    in.Get(record.m_destinationY);
    in.Get(record.m_direction);
    in.Get(record.m_locationType);
    in.Get(record.m_locationMetadata);
    record.m_mobility = in.GetI16();
    record.m_remainingMobility = in.GetI16();
    record.m_experience = in.GetI32();
    in.Get(record.m_unused2d);
    record.m_level = in.GetI16();
    in.Get(record.m_primaryStats, HERO_STARTING_STAT_COUNT);
    in.Get(record.m_morale);
    in.Get(record.m_luck);
    in.Get(record.m_cowardice);
    in.Get(record.m_fledState);
    record.m_visitedSites = in.GetI32();
    record.m_randomSeed = in.GetI16();
    in.Get(record.m_unused3f, sizeof(record.m_unused3f));
    ReadArmyGroup(in, record.m_army);
    in.Get(record.m_spells, HERO_SPELL_SLOT_COUNT);
    in.Get(record.m_spellCharges, HERO_SPELL_SLOT_COUNT);
    in.Get(record.m_artifacts, HERO_ARTIFACT_SLOT_COUNT);
    record.m_eventFlags = in.GetI32();
    record.m_aiFightValue = in.GetF32();
    // Names are fixed-width fields; the game treats them as C strings.
    record.m_name[sizeof(record.m_name) - 1] = '\0';
    record.m_shortName[sizeof(record.m_shortName) - 1] = '\0';
}

void WriteTown(RecordWriter& out, const town& record) {
    out.Put(record.m_id);
    out.Put(record.m_owner);
    out.Put(record.m_nameIndex);
    out.Put(record.m_type);
    out.Put(record.m_x);
    out.Put(record.m_y);
    WriteArmyGroup(out, record.m_army);
    out.Put(record.m_occupyingHeroId);
    out.Put(record.m_buildings);
    out.Put(record.m_mageGuildLevel);
    out.Put(record.m_unused19);
    for (i32 dwelling = 0; dwelling < 6; dwelling++)
        out.Put(record.m_dwellingAvailable[dwelling]);
    out.Put(record.m_extraIndex);
    out.Put(record.m_customized);
    out.Put(record.m_unused28, sizeof(record.m_unused28));
    out.Put(record.m_mageGuildSpells, TOWN_MAGE_GUILD_SPELL_COUNT);
    out.Put(record.m_turnsOwned);
}

void ReadTown(RecordReader& in, town& record) {
    in.Get(record.m_id);
    in.Get(record.m_owner);
    in.Get(record.m_nameIndex);
    in.Get(record.m_type);
    in.Get(record.m_x);
    in.Get(record.m_y);
    ReadArmyGroup(in, record.m_army);
    in.Get(record.m_occupyingHeroId);
    record.m_buildings = in.GetI16();
    in.Get(record.m_mageGuildLevel);
    in.Get(record.m_unused19);
    for (i32 dwelling = 0; dwelling < 6; dwelling++)
        record.m_dwellingAvailable[dwelling] = in.GetI16();
    in.Get(record.m_extraIndex);
    in.Get(record.m_customized);
    in.Get(record.m_unused28, sizeof(record.m_unused28));
    in.Get(record.m_mageGuildSpells, TOWN_MAGE_GUILD_SPELL_COUNT);
    record.m_turnsOwned = in.GetU16();
}

void WriteMapCell(RecordWriter& out, const mapCell& cell) {
    out.Put(cell.m_tileIndex);
    out.Put(cell.m_objectTileset);
    out.Put(cell.m_objectIndex);
    out.Put(cell.m_overlayTileset);
    out.Put(cell.m_overlayIndex);
    out.Put(cell.m_extraFrame);
    out.Put(cell.m_flags);
    out.Put(cell.m_secondaryTrigger);
    out.Put(cell.m_triggerType);
    out.Put(cell.m_objectMetadata);
}

void ReadMapCell(RecordReader& in, mapCell& cell) {
    in.Get(cell.m_tileIndex);
    in.Get(cell.m_objectTileset);
    in.Get(cell.m_objectIndex);
    in.Get(cell.m_overlayTileset);
    in.Get(cell.m_overlayIndex);
    in.Get(cell.m_extraFrame);
    in.Get(cell.m_flags);
    in.Get(cell.m_secondaryTrigger);
    in.Get(cell.m_triggerType);
    in.Get(cell.m_objectMetadata);
}

void WriteMine(RecordWriter& out, const mineRecord& record) {
    out.Put(record.id);
    out.Put(record.owner);
    out.Put(record.type);
    out.Put(record.guardianType);
    out.Put(record.guardianCount);
    out.Put(record.x);
    out.Put(record.y);
}

void ReadMine(RecordReader& in, mineRecord& record) {
    in.Get(record.id);
    in.Get(record.owner);
    in.Get(record.type);
    in.Get(record.guardianType);
    in.Get(record.guardianCount);
    in.Get(record.x);
    in.Get(record.y);
}

void WriteBoat(RecordWriter& out, const boatRecord& record) {
    out.Put(record.id);
    out.Put(record.x);
    out.Put(record.y);
    out.Put(record.direction);
    out.Put(record.savedTriggerType);
    out.Put(record.savedEventData);
    out.Put(record.heroId);
    out.Put(record.owner);
}

void ReadBoat(RecordReader& in, boatRecord& record) {
    in.Get(record.id);
    in.Get(record.x);
    in.Get(record.y);
    in.Get(record.direction);
    in.Get(record.savedTriggerType);
    in.Get(record.savedEventData);
    in.Get(record.heroId);
    in.Get(record.owner);
}

void WriteHighScore(RecordWriter& out, const HighScoreEntry& record) {
    out.Put(record.playerName, sizeof(record.playerName));
    out.Put(record.scenarioName, sizeof(record.scenarioName));
    out.Put(record.score);
    out.Put(record.unused24, sizeof(record.unused24));
}

void ReadHighScore(RecordReader& in, HighScoreEntry& record) {
    in.Get(record.playerName, sizeof(record.playerName));
    in.Get(record.scenarioName, sizeof(record.scenarioName));
    record.score = in.GetI32();
    in.Get(record.unused24, sizeof(record.unused24));
    record.playerName[sizeof(record.playerName) - 1] = '\0';
    record.scenarioName[sizeof(record.scenarioName) - 1] = '\0';
}

void WriteMapHeader(RecordWriter& out, const SMapHeader& header) {
    i32 slot;
    out.Put(header.id);
    out.Put(header.difficulty);
    out.Put(header.size);
    for (slot = 0; slot < MAP_HEADER_SLOT_COUNT; slot++)
        out.Put(header.name[slot], MAP_HEADER_NAME_SIZE);
    for (slot = 0; slot < MAP_HEADER_SLOT_COUNT - 1; slot++)
        out.Put(header.description[slot], MAP_HEADER_DESCRIPTION_SIZE);
    out.Put(header.lastDescription, sizeof(header.lastDescription));
    out.Put(header.format);
}

void ReadMapHeader(RecordReader& in, SMapHeader& header) {
    i32 slot;
    header.id = in.GetI16();
    in.Get(header.difficulty);
    in.Get(header.size);
    for (slot = 0; slot < MAP_HEADER_SLOT_COUNT; slot++)
        in.Get(header.name[slot], MAP_HEADER_NAME_SIZE);
    for (slot = 0; slot < MAP_HEADER_SLOT_COUNT - 1; slot++)
        in.Get(header.description[slot], MAP_HEADER_DESCRIPTION_SIZE);
    in.Get(header.lastDescription, sizeof(header.lastDescription));
    header.format = in.GetI16();
}

void ReadAggEntry(RecordReader& in, aggEntry& entry) {
    entry.id = in.GetI16();
    entry.offset = in.GetI32();
    entry.size = in.GetU32();
}

void WriteSaveHeaderReserved(RecordWriter& out, const SaveHeaderReserved& record) {
    out.Put(record.format.signature, sizeof(record.format.signature));
    out.Put(record.format.version);
    out.Put(record.unused, sizeof(record.unused));
}

void ReadSaveHeaderReserved(RecordReader& in, SaveHeaderReserved& record) {
    in.Get(record.format.signature, sizeof(record.format.signature));
    record.format.version = in.GetI32();
    in.Get(record.unused, sizeof(record.unused));
}

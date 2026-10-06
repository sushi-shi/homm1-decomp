#ifndef HOMM1_SOURCE_SAVERECORDS_H
#define HOMM1_SOURCE_SAVERECORDS_H

// The game's file records - saved games, maps, high scores and the resource
// archive directory - encoded field by field. The sizes are those of the
// original's 32-bit structures, which the files on disk keep; the codecs make
// the files independent of how this build lays the structures out in memory.

#include <H1/Ints.h>

#include <PLATFORM/Records.h>

class armyGroup;
class hero;
class mapCell;
class town;
struct mineRecord;
struct boatRecord;
struct HighScoreEntry;
struct SMapHeader;
struct aggEntry;

enum FileRecordSize {
    ARMY_GROUP_RECORD_SIZE = 15,
    HERO_RECORD_SIZE = 182,
    TOWN_RECORD_SIZE = 55,
    MAP_CELL_RECORD_SIZE = 10,
    MINE_RECORD_SIZE = 7,
    BOAT_RECORD_SIZE = 8,
    HIGH_SCORE_RECORD_SIZE = 87,
    MAP_HEADER_RECORD_SIZE = 1364,
    AGG_ENTRY_RECORD_SIZE = 10,
    ICON_ENTRY_RECORD_SIZE = 12,
    MAP_TOWN_EXTRA_RECORD_SIZE = 20,
    MAP_HERO_EXTRA_RECORD_SIZE = 25,
    // The largest map extra record; blocks are allocated at least this large
    // so that a record read through its structure never leaves the block.
    MAP_EXTRA_RECORD_MAX_SIZE = MAP_HERO_EXTRA_RECORD_SIZE
};

void WriteArmyGroup(RecordWriter& out, const armyGroup& group);
void ReadArmyGroup(RecordReader& in, armyGroup& group);
void WriteHero(RecordWriter& out, const hero& record);
void ReadHero(RecordReader& in, hero& record);
void WriteTown(RecordWriter& out, const town& record);
void ReadTown(RecordReader& in, town& record);
void WriteMapCell(RecordWriter& out, const mapCell& cell);
void ReadMapCell(RecordReader& in, mapCell& cell);
void WriteMine(RecordWriter& out, const mineRecord& record);
void ReadMine(RecordReader& in, mineRecord& record);
void WriteBoat(RecordWriter& out, const boatRecord& record);
void ReadBoat(RecordReader& in, boatRecord& record);
void WriteHighScore(RecordWriter& out, const HighScoreEntry& record);
void ReadHighScore(RecordReader& in, HighScoreEntry& record);
void WriteMapHeader(RecordWriter& out, const SMapHeader& header);
void ReadMapHeader(RecordReader& in, SMapHeader& header);
void ReadAggEntry(RecordReader& in, aggEntry& entry);

#endif

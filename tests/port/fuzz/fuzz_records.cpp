// The record codecs (include/PLATFORM/Records.h, include/SOURCE/saveRecords.h)
// on arbitrary bytes: each record type is decoded from the input as a
// sequence of records, re-encoded, decoded again and re-encoded again; the
// two encodings must be equal (decoding is idempotent after one pass, which
// terminates the text fields). The map header is then copied into the file
// requester's fields as REQUEST.cpp does, and the high score table read as
// HISCORE.cpp and KB.cpp do.

#include "FuzzSupport.h"

#include <BASE/resourceManager.h>
#include <PLATFORM/Records.h>
#include <SOURCE/fileRequester.h>
#include <SOURCE/game.h>
#include <SOURCE/highScoreManager.h>
#include <SOURCE/KB.h>
#include <SOURCE/saveRecords.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>

// The record classes' constructors live in game units this harness does not
// link; the codecs need only the storage.
armyGroup::armyGroup(void) {}
hero::hero(void) {}
town::town(void) {}

namespace {

[[noreturn]] void Fail(const char* what) {
    std::fprintf(stderr, "fuzz_records: %s\n", what);
    std::abort();
}

template <class Record>
std::vector<u8> Transcode(const u8* data, i32 size, void (*read)(RecordReader&, Record&),
                          void (*write)(RecordWriter&, const Record&), bool& complete) {
    RecordReader in(data, size);
    RecordWriter out;
    complete = true;
    while (in.Remaining() > 0) {
        Record record;
        std::memset(static_cast<void*>(&record), 0, sizeof(record));
        read(in, record);
        if (!in.Ok()) {
            complete = false;
            break;
        }
        write(out, record);
    }
    return std::vector<u8>(out.Data(), out.Data() + out.Size());
}

template <class Record>
void CheckCodec(const u8* data, i32 size, int recordSize, void (*read)(RecordReader&, Record&),
                void (*write)(RecordWriter&, const Record&)) {
    bool complete = false;
    std::vector<u8> first = Transcode<Record>(data, size, read, write, complete);
    if (first.size() != static_cast<size_t>(size / recordSize) * static_cast<size_t>(recordSize))
        Fail("encoded size differs from the records read");
    if (complete != (size % recordSize == 0))
        Fail("a partial record was not refused");
    bool again = false;
    std::vector<u8> second =
        Transcode<Record>(first.data(), static_cast<i32>(first.size()), read, write, again);
    std::vector<u8> third =
        Transcode<Record>(second.data(), static_cast<i32>(second.size()), read, write, again);
    if (!again || second != third)
        Fail("re-encoding is not stable");
}

void WritePlayerRecord(RecordWriter& out, const playerData& record) {
    const_cast<playerData&>(record).Write(out);
}

void ReadPlayerRecord(RecordReader& in, playerData& record) {
    record.Read(in);
}

}  // namespace

extern "C" int LLVMFuzzerInitialize(int*, char***) {
    return 0;
}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
    if (size > (1u << 20))
        return 0;
    i32 length = static_cast<i32>(size);
    CheckCodec<armyGroup>(data, length, ARMY_GROUP_RECORD_SIZE, ReadArmyGroup, WriteArmyGroup);
    CheckCodec<hero>(data, length, HERO_RECORD_SIZE, ReadHero, WriteHero);
    CheckCodec<town>(data, length, TOWN_RECORD_SIZE, ReadTown, WriteTown);
    CheckCodec<mapCell>(data, length, MAP_CELL_RECORD_SIZE, ReadMapCell, WriteMapCell);
    CheckCodec<mineRecord>(data, length, MINE_RECORD_SIZE, ReadMine, WriteMine);
    CheckCodec<boatRecord>(data, length, BOAT_RECORD_SIZE, ReadBoat, WriteBoat);
    CheckCodec<HighScoreEntry>(data, length, HIGH_SCORE_RECORD_SIZE, ReadHighScore, WriteHighScore);
    CheckCodec<SMapHeader>(data, length, MAP_HEADER_RECORD_SIZE, ReadMapHeader, WriteMapHeader);
    static const int playerRecordSize = [] {
        playerData empty;
        std::memset(static_cast<void*>(&empty), 0, sizeof(empty));
        RecordWriter out;
        empty.Write(out);
        return out.Size();
    }();
    CheckCodec<playerData>(data, length, playerRecordSize, ReadPlayerRecord, WritePlayerRecord);

    // The archive directory entries, as LoadAggregateHeader decodes them.
    {
        RecordReader in(data, length);
        while (in.Remaining() >= AGG_ENTRY_RECORD_SIZE) {
            aggEntry entry;
            ReadAggEntry(in, entry);
        }
    }

    // The map list: the header's name and description copied into the
    // requester's fields (REQUEST.cpp).
    {
        RecordReader in(data, length);
        SMapHeader header;
        ReadMapHeader(in, header);
        FileRequesterName name;
        FileRequesterMapInfo info;
        CopyTextField(name.text, sizeof(name.text), header.name[0], sizeof(header.name[0]));
        CopyTextField(info.description, sizeof(info.description), header.description[0],
                      sizeof(header.description[0]));
        if (std::strlen(name.text) >= sizeof(name.text)
            || std::strlen(info.description) >= sizeof(info.description))
            Fail("map list text not terminated");
    }

    // The high score table: ten 87-byte entries, names used as C strings.
    {
        RecordReader in(data, length);
        for (i32 rank = 0; rank < HIGH_SCORE_DISPLAY_ENTRY_COUNT; rank++) {
            HighScoreEntry entry;
            ReadHighScore(in, entry);
            if (std::strlen(entry.playerName) >= sizeof(entry.playerName)
                || std::strlen(entry.scenarioName) >= sizeof(entry.scenarioName))
                Fail("high score name not terminated");
        }
    }
    return 0;
}

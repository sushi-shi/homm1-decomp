// File record codecs: every record encodes to the original's size, decodes
// back to the same bytes, and refuses truncated input. With the game data
// ($HOMM1_DATA), every shipped map, saved game, high score table and the
// resource archive directory must parse to exactly its file length and
// re-encode byte for byte. The edition's saves carry a format tag ("H1TE"
// and the format version) in the header block the original game leaves zero;
// every saved game found is also checked with that tag in place.

#include <H1/Ints.h>

#include <BASE/resourceManager.h>
#include <PLATFORM/File.h>
#include <PLATFORM/Records.h>
#include <SOURCE/fileRequester.h>
#include <SOURCE/KB.h>
#include <SOURCE/game.h>
#include <SOURCE/highScoreManager.h>
#include <SOURCE/saveRecords.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <functional>
#include <string>
#include <vector>

// The record classes' constructors live in game units this test does not
// link; the codecs need only the storage.
armyGroup::armyGroup(void) {}
hero::hero(void) {}
town::town(void) {}

namespace {

int gFailures = 0;

void Expect(bool condition, const std::string& what) {
    if (!condition) {
        std::fprintf(stderr, "FAIL: %s\n", what.c_str());
        gFailures++;
    }
}

template <class Record>
void Fill(Record& record, u8 seed) {
    u8* bytes = reinterpret_cast<u8*>(&record);
    for (size_t i = 0; i < sizeof(Record); i++)
        bytes[i] = static_cast<u8>(seed + i * 7);
}

template <class Record>
void CheckCodec(
    const char* name,
    int size,
    void (*write)(RecordWriter&, const Record&),
    void (*read)(RecordReader&, Record&)
) {
    Record original;
    Fill(original, 0x31);
    RecordWriter first;
    write(first, original);
    Expect(first.Size() == size, std::string(name) + ": encoded size");
    Record decoded;
    Fill(decoded, 0x77);
    RecordReader in(first.Data(), first.Size());
    read(in, decoded);
    Expect(in.Ok() && in.Remaining() == 0, std::string(name) + ": decodes exactly");
    RecordWriter second;
    write(second, decoded);
    Expect(second.Size() == first.Size(), std::string(name) + ": re-encoded size");
    // Text fields are terminated on reading; everything else must survive.
    int differences = 0;
    for (int i = 0; i < first.Size() && i < second.Size(); i++)
        differences += first.Data()[i] != second.Data()[i];
    Expect(differences <= 2, std::string(name) + ": round trip");
    RecordReader truncated(first.Data(), first.Size() - 1);
    read(truncated, decoded);
    Expect(!truncated.Ok(), std::string(name) + ": truncated input is refused");
}

void WritePlayerRecord(RecordWriter& out, const playerData& record) {
    const_cast<playerData&>(record).Write(out);
}

void ReadPlayerRecord(RecordReader& in, playerData& record) {
    record.Read(in);
}

// ---------------------------------------------------------------- files

std::vector<u8> ReadHostFile(const std::string& path) {
    std::vector<u8> bytes;
    std::FILE* file = std::fopen(path.c_str(), "rb");
    if (file == nullptr)
        return bytes;
    int c;
    while ((c = std::fgetc(file)) != EOF)
        bytes.push_back(static_cast<u8>(c));
    std::fclose(file);
    return bytes;
}

std::vector<std::string> List(const char* pattern) {
    std::vector<std::string> names;
    FileFindData found;
    i32 find = FileFindFirst(pattern, &found);
    if (find == FILE_INVALID)
        return names;
    do {
        names.push_back(found.name);
    } while (FileFindNext(find, &found));
    FileFindClose(find);
    return names;
}

// Copies a field from the reader to the writer through its codec.
struct Transcoder {
    RecordReader& in;
    RecordWriter& out;
    template <class T>
    void Value() {
        T value{};
        in.Get(value);
        out.Put(value);
    }
    void Bytes(int count) {
        std::vector<u8> bytes(static_cast<size_t>(count));
        in.Get(bytes.data(), count);
        out.Put(bytes.data(), count);
    }
    template <class Record>
    void Records(int count, void (*read)(RecordReader&, Record&),
                 void (*write)(RecordWriter&, const Record&)) {
        for (int i = 0; i < count; i++) {
            Record record;
            std::memset(static_cast<void*>(&record), 0, sizeof(record));
            read(in, record);
            write(out, record);
        }
    }
};

// The layout game::LoadMap reads.
void TranscodeMapFile(RecordReader& in, RecordWriter& out) {
    i16 version;
    in.Get(version);
    out.Put(version);
    if (version == MAP_HEADER_ID) {
        // The header's first field is the version just read.
        std::vector<u8> header(MAP_HEADER_RECORD_SIZE);
        header[0] = static_cast<u8>(version);
        header[1] = static_cast<u8>(version >> 8);
        in.Get(header.data() + 2, MAP_HEADER_RECORD_SIZE - 2);
        RecordReader headerIn(header.data(), MAP_HEADER_RECORD_SIZE);
        SMapHeader decoded;
        ReadMapHeader(headerIn, decoded);
        RecordWriter headerOut;
        WriteMapHeader(headerOut, decoded);
        out.Put(headerOut.Data() + 2, MAP_HEADER_RECORD_SIZE - 2);
        in.Get(version);
        out.Put(version);
    }
    Transcoder t{in, out};
    t.Value<i16>();
    t.Value<i16>();
    t.Records<mapCell>(MAP_CELL_GRID_SIZE * MAP_CELL_GRID_SIZE, ReadMapCell, WriteMapCell);
    t.Bytes(GAME_TOWN_COUNT * 3);
    t.Bytes(GAME_MINE_COUNT * 3);
    t.Bytes(0x25);
    t.Value<i8>();
    t.Bytes(MAP_CELL_GRID_SIZE * MAP_CELL_GRID_SIZE);
    if (version >= MAP_EXTRA_VERSION) {
        i32 count;
        in.Get(count);
        out.Put(count);
        Expect(count >= 1 && count <= MAP_EXTRA_RECORD_CAPACITY, "map extra count in range");
        for (i32 i = 1; i < count && in.Ok(); i++) {
            i32 size;
            in.Get(size);
            out.Put(size);
            Expect(size >= 0 && size <= in.Remaining(), "map extra size in range");
            t.Bytes(size);
        }
    }
    // Maps saved by the scenario editor in its newer format end with the
    // editor's object owner table (two 16-bit values per cell) and its next
    // object id; the game does not read them.
    if (in.Remaining() == MAP_CELL_GRID_SIZE * MAP_CELL_GRID_SIZE * 4 + 2) {
        for (int i = 0; i < MAP_CELL_GRID_SIZE * MAP_CELL_GRID_SIZE * 2 + 1; i++)
            t.Value<i16>();
    }
}

// The layout game::SaveGame writes; ORIGDATA.BIN stops before the map grids.
void TranscodeSavedGame(RecordReader& in, RecordWriter& out, bool original) {
    Transcoder t{in, out};
    t.Value<i8>();
    t.Value<i16>();
    t.Bytes(4);
    for (int i = 0; i < 4; i++)
        t.Value<i32>();
    t.Records<SaveHeaderReserved>(1, ReadSaveHeaderReserved, WriteSaveHeaderReserved);
    t.Bytes(0x79);
    t.Bytes(2);
    t.Bytes(0x11);
    t.Bytes(0x11);
    t.Bytes(4);
    t.Bytes(GAME_PLAYER_COUNT * 2);
    t.Value<u16>();
    t.Value<u16>();
    t.Value<u16>();
    t.Records<playerData>(GAME_PLAYER_COUNT, ReadPlayerRecord, WritePlayerRecord);
    t.Records<mapCell>(MAP_CELL_GRID_SIZE * MAP_CELL_GRID_SIZE, ReadMapCell, WriteMapCell);
    t.Value<i8>();
    t.Records<hero>(GAME_HERO_COUNT, ReadHero, WriteHero);
    t.Bytes(GAME_HERO_COUNT);
    t.Records<town>(GAME_TOWN_COUNT, ReadTown, WriteTown);
    t.Bytes(GAME_TOWN_COUNT);
    t.Bytes(4);
    t.Records<mineRecord>(GAME_MINE_COUNT, ReadMine, WriteMine);
    t.Bytes(GAME_MINE_COUNT);
    t.Bytes(0x25);
    t.Records<boatRecord>(GAME_BOAT_COUNT, ReadBoat, WriteBoat);
    t.Bytes(GAME_BOAT_COUNT);
    t.Bytes(0x30);
    t.Bytes(3);
    if (!original)
        t.Bytes(MAP_CELL_GRID_SIZE * MAP_CELL_GRID_SIZE * 3);
}

// The offset of a saved game's reserved header block: the fields before it.
const int kSaveReservedOffset = 1 + 2 + 4 + 4 * 4;

// The edition's tag: a block written by the codec holds exactly "H1TE", the
// version and zeros, and reads back as such; the original game's zero block
// carries no tag.
void CheckSaveFormatTag() {
    SaveHeaderReserved tagged;
    std::memset(&tagged, 0, sizeof(tagged));
    std::memcpy(tagged.format.signature, SAVE_FORMAT_SIGNATURE, sizeof(tagged.format.signature));
    tagged.format.version = SAVE_FORMAT_CURRENT;
    RecordWriter out;
    WriteSaveHeaderReserved(out, tagged);
    std::vector<u8> expected(SAVE_HEADER_RESERVED_RECORD_SIZE, 0);
    std::memcpy(expected.data(), "H1TE\x01", 5);
    Expect(SAVE_FORMAT_CURRENT == SAVE_FORMAT_FLED_STATE && SAVE_FORMAT_FLED_STATE == 1,
           "save format: the edition's version is 1");
    Expect(out.Size() == SAVE_HEADER_RESERVED_RECORD_SIZE
               && std::memcmp(out.Data(), expected.data(), expected.size()) == 0,
           "save format tag bytes");
    SaveHeaderReserved decoded;
    Fill(decoded, 0x55);
    RecordReader in(out.Data(), out.Size());
    ReadSaveHeaderReserved(in, decoded);
    Expect(in.Ok() && in.Remaining() == 0
               && std::memcmp(decoded.format.signature, SAVE_FORMAT_SIGNATURE, 4) == 0
               && decoded.format.version == SAVE_FORMAT_FLED_STATE,
           "save format tag round trip");
    std::vector<u8> zeros(SAVE_HEADER_RESERVED_RECORD_SIZE, 0);
    RecordReader originalIn(zeros.data(), static_cast<i32>(zeros.size()));
    ReadSaveHeaderReserved(originalIn, decoded);
    Expect(originalIn.Ok()
               && std::memcmp(decoded.format.signature, SAVE_FORMAT_SIGNATURE, 4) != 0
               && decoded.format.version == SAVE_FORMAT_ORIGINAL,
           "the original game's block has no tag");
}

void TranscodeHighScores(RecordReader& in, RecordWriter& out) {
    Transcoder t{in, out};
    t.Records<HighScoreEntry>(HIGH_SCORE_DISPLAY_ENTRY_COUNT, ReadHighScore, WriteHighScore);
}

void CheckTaggedSave(const std::string& gamePath);

// Names in records keep their terminator in the shipped files; bytes after
// it are carried as they are, so a re-encoding is exact.
void CheckFile(const std::string& gamePath, const std::function<void(RecordReader&, RecordWriter&)>& transcode) {
    char resolved[FILE_PATH_CAPACITY];
    if (!FileResolve(gamePath.c_str(), FILE_OPEN_READ, resolved, sizeof(resolved))) {
        Expect(false, gamePath + ": present");
        return;
    }
    std::vector<u8> bytes = ReadHostFile(resolved);
    RecordReader in(bytes.data(), static_cast<i32>(bytes.size()));
    RecordWriter out;
    transcode(in, out);
    Expect(in.Ok(), gamePath + ": parses");
    Expect(in.Remaining() == 0, gamePath + ": parses to its end (" + std::to_string(in.Remaining())
                                    + " bytes left)");
    Expect(out.Size() == static_cast<i32>(bytes.size())
               && std::memcmp(out.Data(), bytes.data(), bytes.size()) == 0,
           gamePath + ": re-encodes byte for byte");
    std::printf("ok %s (%zu bytes)\n", gamePath.c_str(), bytes.size());
}

// A saved game with the edition's tag in its header block parses the same
// and re-encodes byte for byte, tag included.
void CheckTaggedSave(const std::string& gamePath) {
    char resolved[FILE_PATH_CAPACITY];
    if (!FileResolve(gamePath.c_str(), FILE_OPEN_READ, resolved, sizeof(resolved)))
        return;
    std::vector<u8> bytes = ReadHostFile(resolved);
    if (bytes.size() < static_cast<size_t>(kSaveReservedOffset + SAVE_HEADER_RESERVED_RECORD_SIZE))
        return;
    // The block is zero in the original game's saves, tagged in the edition's.
    const u8* block = bytes.data() + kSaveReservedOffset;
    bool zero = true;
    for (int i = 0; i < SAVE_HEADER_RESERVED_RECORD_SIZE; i++)
        zero = zero && block[i] == 0;
    Expect(zero || std::memcmp(block, SAVE_FORMAT_SIGNATURE, 4) == 0,
           gamePath + ": the header block is zero or tagged");
    std::memset(bytes.data() + kSaveReservedOffset, 0, SAVE_HEADER_RESERVED_RECORD_SIZE);
    std::memcpy(bytes.data() + kSaveReservedOffset, "H1TE\x01", 5);
    RecordReader in(bytes.data(), static_cast<i32>(bytes.size()));
    RecordWriter out;
    TranscodeSavedGame(in, out, false);
    Expect(in.Ok() && in.Remaining() == 0 && out.Size() == static_cast<i32>(bytes.size())
               && std::memcmp(out.Data(), bytes.data(), bytes.size()) == 0,
           gamePath + " with the edition's tag: re-encodes byte for byte");
}

void CheckArchive() {
    i32 file = FileOpen("DATA\\HEROES.AGG", FILE_OPEN_READ);
    Expect(file != FILE_INVALID, "archive present");
    if (file == FILE_INVALID)
        return;
    i32 length = FileLength(file);
    u8 countBytes[2];
    Expect(FileReadExact(file, countBytes, 2), "archive count");
    RecordReader countReader(countBytes, 2);
    i16 count;
    countReader.Get(count);
    std::vector<u8> directory(static_cast<size_t>(count) * AGG_ENTRY_RECORD_SIZE);
    Expect(FileReadExact(file, directory.data(), static_cast<i32>(directory.size())),
           "archive directory");
    FileClose(file);
    RecordReader in(directory.data(), static_cast<i32>(directory.size()));
    int valid = 0;
    for (i16 i = 0; i < count; i++) {
        aggEntry entry;
        ReadAggEntry(in, entry);
        valid += entry.offset >= 0 && entry.offset <= length
                 && entry.size <= static_cast<u32>(length - entry.offset);
    }
    Expect(in.Ok() && valid == count, "archive entries lie inside the file");
    std::printf("ok DATA\\HEROES.AGG (%d entries)\n", count);
}

}  // namespace

int main() {
    CheckCodec<armyGroup>("army", ARMY_GROUP_RECORD_SIZE, WriteArmyGroup, ReadArmyGroup);
    CheckCodec<hero>("hero", HERO_RECORD_SIZE, WriteHero, ReadHero);
    CheckCodec<town>("town", TOWN_RECORD_SIZE, WriteTown, ReadTown);
    CheckCodec<mapCell>("map cell", MAP_CELL_RECORD_SIZE, WriteMapCell, ReadMapCell);
    CheckCodec<mineRecord>("mine", MINE_RECORD_SIZE, WriteMine, ReadMine);
    CheckCodec<boatRecord>("boat", BOAT_RECORD_SIZE, WriteBoat, ReadBoat);
    CheckCodec<HighScoreEntry>("high score", HIGH_SCORE_RECORD_SIZE, WriteHighScore, ReadHighScore);
    CheckCodec<SMapHeader>("map header", MAP_HEADER_RECORD_SIZE, WriteMapHeader, ReadMapHeader);
    CheckCodec<SaveHeaderReserved>("save header block", SAVE_HEADER_RESERVED_RECORD_SIZE,
                                   WriteSaveHeaderReserved, ReadSaveHeaderReserved);
    CheckSaveFormatTag();

    const char* data = std::getenv("HOMM1_DATA");
    if (data != nullptr && data[0] != '\0') {
        FileSetRoot(data);
        int maps = 0;
        for (const char* pattern : {"MAPS\\*.MAP", "MAPS\\*.CMP"}) {
            for (const std::string& name : List(pattern)) {
                CheckFile("MAPS\\" + name, TranscodeMapFile);
                maps++;
            }
        }
        Expect(maps > 0, "shipped maps found");
        for (const char* pattern : {"GAMES\\*.GM?", "GAMES\\*.CGM"}) {
            for (const std::string& name : List(pattern)) {
                CheckFile("GAMES\\" + name, [](RecordReader& in, RecordWriter& out) {
                    TranscodeSavedGame(in, out, false);
                });
                CheckTaggedSave("GAMES\\" + name);
            }
        }
        CheckFile("DATA\\REMOTE.GAM", [](RecordReader& in, RecordWriter& out) {
            TranscodeSavedGame(in, out, false);
        });
        CheckFile("DATA\\ORIGDATA.BIN", [](RecordReader& in, RecordWriter& out) {
            TranscodeSavedGame(in, out, true);
        });
        CheckFile("DATA\\STANDARD.HS", TranscodeHighScores);
        CheckFile("DATA\\CAMPAIGN.HS", TranscodeHighScores);
        CheckArchive();
    } else {
        std::printf("HOMM1_DATA not set: shipped files not checked\n");
    }
    if (gFailures != 0) {
        std::fprintf(stderr, "%d failure(s)\n", gFailures);
        return 1;
    }
    std::printf("records: all checks passed\n");
    return 0;
}

// Every shipped map through the scenario editor's own map reader and writer.
//
// The test links the native editor without its entry point, starts it
// headless (SDL's dummy video and audio drivers) on a scratch folder that
// links to the game data, and for each map in MAPS does what the editor's
// Save does after Load: CheckObjects, UpdateTriggers, then the file writer.
// The result is compared with the shipped file region by region. The writer
// legitimately changes only what it derives instead of copying:
//
// - the header's format word: the newer format always writes 1000; an older
//   map is written in the older layout, which (as in the original editor)
//   repeats the 2-byte header id at the start of the header body and drops
//   the body's last two bytes;
// - the cells' trigger bytes, which UpdateTriggers recomputes from the
//   objects on the map;
// - the town, mine, artifact and obelisk tables and the cell sound grid,
//   which the reader skips and the writer rebuilds from the cells;
// - the object owner table of the newer format, which an older map does not
//   carry (some shipped older maps hold one; the editor neither reads nor
//   writes it for them).
//
// Everything else - the header text, the cells' terrain, object and overlay
// bytes, the extra records - must be byte-identical. A second load and save
// of the written file must reproduce it exactly, and the game's map reader
// must accept it.
//
// Needs $HOMM1_DATA; without it the test is skipped (exit code 77).

#include <H1/Ints.h>

#include <BASE/executive.h>
#include <EDITOR/EDITOR.h>
#include <EDITOR/editManager.h>
#include <PLATFORM/File.h>
#include <PLATFORM/Records.h>
#include <SOURCE/KB.h>
#include <SOURCE/fileRequester.h>
#include <SOURCE/game.h>
#include <SOURCE/mapObjectTypes.h>
#include <SOURCE/saveRecords.h>
#include <SOURCE/town.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <random>
#include <string>
#include <system_error>
#include <vector>

bool KBStartHost(const char* dataRoot, const char* gameArguments, i32 fullScreen);

namespace {

int gFailures = 0;

void Expect(bool condition, const std::string& what) {
    if (!condition) {
        std::fprintf(stderr, "FAIL: %s\n", what.c_str());
        gFailures++;
    }
}

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

// Links target at link, or copies it where the host refuses links (Windows
// without the privilege).
bool LinkOrCopy(const std::filesystem::path& target, const std::filesystem::path& link) {
    namespace fs = std::filesystem;
    std::error_code error;
    if (fs::is_directory(target, error)) {
        fs::create_directory_symlink(target, link, error);
        if (error)
            fs::copy(target, link, fs::copy_options::recursive, error);
    } else {
        fs::create_symlink(target, link, error);
        if (error)
            fs::copy_file(target, link, error);
    }
    return !error;
}

void SetVariable(const char* name, const char* value, bool replace) {
    if (!replace && std::getenv(name) != nullptr)
        return;
#if defined(_WIN32)
    _putenv_s(name, value);
#else
    setenv(name, value, 1);
#endif
}

std::string FindEntry(const std::string& directory, const char* name) {
    char resolved[FILE_PATH_CAPACITY];
    std::string saved = FileRoot();
    FileSetRoot(directory.c_str());
    bool found = FileResolve(name, FILE_OPEN_READ, resolved, sizeof(resolved));
    FileSetRoot(saved.c_str());
    return found ? std::string(resolved) : std::string();
}

// The regions of a map file, as game::LoadMap reads it.
struct Layout {
    size_t header = 0;      // header id and body (0 when absent)
    size_t cells = 0;       // offset of the cell grid
    size_t tables = 0;      // town, mine, artifact, obelisk tables
    size_t sounds = 0;
    size_t extras = 0;
    size_t owners = 0;      // object owner table, or the file size
    size_t end = 0;
};

constexpr size_t kCellBytes = MAP_CELL_GRID_SIZE * MAP_CELL_GRID_SIZE * MAP_CELL_RECORD_SIZE;
constexpr size_t kTableBytes = GAME_TOWN_COUNT * 3 + GAME_MINE_COUNT * 3 + 0x25 + 1;
constexpr size_t kSoundBytes = MAP_CELL_GRID_SIZE * MAP_CELL_GRID_SIZE;

Layout Parse(const std::vector<u8>& bytes) {
    Layout layout;
    RecordReader in(bytes.data(), static_cast<i32>(bytes.size()));
    i16 version = in.GetI16();
    if (version == MAP_HEADER_ID) {
        layout.header = MAP_HEADER_RECORD_SIZE;
        in.Skip(MAP_HEADER_RECORD_SIZE - 2);
        version = in.GetI16();
    }
    in.Skip(4);
    layout.cells = static_cast<size_t>(in.Offset());
    in.Skip(static_cast<i32>(kCellBytes));
    layout.tables = static_cast<size_t>(in.Offset());
    in.Skip(static_cast<i32>(kTableBytes));
    layout.sounds = static_cast<size_t>(in.Offset());
    in.Skip(static_cast<i32>(kSoundBytes));
    layout.extras = static_cast<size_t>(in.Offset());
    if (version == EDIT_MAP_VERSION) {
        i32 count = in.GetI32();
        for (i32 i = 1; i < count && in.Ok(); i++)
            in.Skip(in.GetI32());
    }
    layout.owners = static_cast<size_t>(in.Offset());
    layout.end = bytes.size();
    return layout;
}

// The cells CheckObjects clears: the row of the footprint above each random
// town or castle, and in the newer format every cell of an object or overlay
// that has a cell there (the owner table names them).
std::vector<bool> ClearedCells(const std::vector<u8>& bytes, const Layout& layout, bool newFormat) {
    constexpr int kGrid = MAP_CELL_GRID_SIZE;
    std::vector<bool> cleared(static_cast<size_t>(kGrid * kGrid), false);
    const u8* cells = &bytes[layout.cells];
    auto owner = [&](int cell, int which) {
        size_t offset = layout.owners + static_cast<size_t>(cell) * 4 + static_cast<size_t>(which) * 2;
        return static_cast<int>(bytes[offset] | (bytes[offset + 1] << 8));
    };
    for (int x = 0; x < kGrid; x++) {
        for (int y = 0; y < kGrid; y++) {
            u8 trigger = cells[static_cast<size_t>(x * kGrid + y) * MAP_CELL_RECORD_SIZE + 8];
            if ((trigger != (MAP_TRIGGER_EVENT | MAP_FILE_OBJECT_RANDOM_TOWN)
                 && trigger != (MAP_TRIGGER_EVENT | MAP_FILE_OBJECT_RANDOM_CASTLE))
                || x < TOWN_FOOTPRINT_LEFT || y < TOWN_FOOTPRINT_TOP)
                continue;
            for (int column = 0; column < TOWN_FOOTPRINT_WIDTH; column++) {
                int cell = (x - TOWN_FOOTPRINT_LEFT + column) * kGrid + (y - TOWN_FOOTPRINT_TOP);
                cleared[static_cast<size_t>(cell)] = true;
                if (!newFormat || layout.end - layout.owners < static_cast<size_t>(kGrid * kGrid * 4))
                    continue;
                for (int which = 0; which < 2; which++) {
                    int id = owner(cell, which);
                    for (int other = 0; id != 0 && other < kGrid * kGrid; other++)
                        if (owner(other, 0) == id || owner(other, 1) == id)
                            cleared[static_cast<size_t>(other)] = true;
                }
            }
        }
    }
    return cleared;
}

// Cell bytes other than the trigger bytes UpdateTriggers recomputes, outside
// the cells CheckObjects clears.
bool CellsMatch(const u8* a, const u8* b, const std::vector<bool>& cleared, int& changedTriggers,
                int& clearedCells) {
    bool same = true;
    changedTriggers = 0;
    clearedCells = 0;
    for (size_t cell = 0; cell < kCellBytes / MAP_CELL_RECORD_SIZE; cell++) {
        bool cellChanged = false;
        for (size_t field = 0; field < MAP_CELL_RECORD_SIZE; field++) {
            size_t offset = cell * MAP_CELL_RECORD_SIZE + field;
            if (a[offset] == b[offset])
                continue;
            // m_flags (6), m_secondaryTrigger (7), m_triggerType (8) and
            // m_objectMetadata (9) are derived trigger state.
            if (cleared[cell])
                cellChanged = true;
            else if (field >= 6)
                changedTriggers++;
            else
                same = false;
        }
        clearedCells += cellChanged;
    }
    return same;
}

std::vector<u8> LoadAndSave(const std::string& name, bool& loaded) {
    char fileName[FILE_PATH_CAPACITY];
    std::snprintf(fileName, sizeof(fileName), "MAPS\\%s", name.c_str());
    RecordReader reader;
    loaded = reader.LoadFile(fileName);
    if (!loaded)
        return {};
    // As in an editor started afresh for the map: an older map carries no
    // object owner table, so none must linger from the previous map.
    gEditManager->FreeMapExtras();
    std::memset(gEditManager->m_map.cellPairs, 0, sizeof(gEditManager->m_map.cellPairs));
    loaded = gEditManager->ReadMapFile(reader) != 0;
    if (!loaded)
        return {};
    gEditManager->ClearErrors();
    gEditManager->CheckObjects();
    gEditManager->UpdateTriggers();
    RecordWriter writer;
    gEditManager->WriteMapFile(writer);
    gEditManager->ClearErrors();
    return std::vector<u8>(writer.Data(), writer.Data() + writer.Size());
}

void CheckMap(const std::string& root, const std::string& name) {
    std::vector<u8> original = ReadHostFile(root + "/MAPS/" + name);
    bool loaded = false;
    std::vector<u8> saved = LoadAndSave(name, loaded);
    Expect(loaded, name + ": the editor reads it");
    if (!loaded)
        return;
    Layout before = Parse(original);
    Layout after = Parse(saved);
    bool newFormat = before.header != 0
                     && original[MAP_HEADER_RECORD_SIZE - 2] == (MAP_HEADER_ID & 0xff)
                     && original[MAP_HEADER_RECORD_SIZE - 1] == (MAP_HEADER_ID >> 8);

    // The header: the body byte for byte in the newer format; shifted by
    // the repeated id in the older one.
    if (before.header != 0) {
        Expect(after.header == before.header, name + ": header kept");
        size_t body = 2;
        size_t length = offsetof(SMapHeader, format) - body;
        if (newFormat) {
            Expect(std::memcmp(&original[body], &saved[body], length) == 0,
                   name + ": header body unchanged");
        } else {
            Expect(saved[2] == saved[0] && saved[3] == saved[1],
                   name + ": older header repeats its id");
            Expect(std::memcmp(&original[body], &saved[body + 2], length - 2) == 0,
                   name + ": older header body unchanged after the repeated id");
        }
    }
    int triggers = 0;
    int clearedCells = 0;
    Expect(CellsMatch(&original[before.cells], &saved[after.cells],
                      ClearedCells(original, before, newFormat), triggers, clearedCells),
           name + ": terrain, object and overlay bytes unchanged");
    size_t extrasBefore = before.owners - before.extras;
    size_t extrasAfter = after.owners - after.extras;
    Expect(extrasBefore == extrasAfter
               && std::memcmp(&original[before.extras], &saved[after.extras], extrasBefore) == 0,
           name + ": extra records unchanged");
    if (newFormat)
        Expect(before.end - before.owners == after.end - after.owners
                   && std::memcmp(&original[before.owners], &saved[after.owners],
                                  before.end - before.owners)
                          == 0,
               name + ": object owner table unchanged");
    else
        Expect(after.end == after.owners, name + ": older layout written without owner table");
    int tables = 0;
    for (size_t i = 0; i < kTableBytes + kSoundBytes; i++)
        tables += original[before.tables + i] != saved[after.tables + i];

    // Written again, the file stays the same; the game reads it.
    std::string copy = "ROUNDTRP.MAP";
    char path[FILE_PATH_CAPACITY];
    std::snprintf(path, sizeof(path), "MAPS\\%s", copy.c_str());
    Expect(FileReplace(path, saved.data(), static_cast<i32>(saved.size())), name + ": written");
    bool reloaded = false;
    std::vector<u8> again = LoadAndSave(copy, reloaded);
    Expect(reloaded && (newFormat ? again == saved : again.size() == saved.size()),
           name + ": saving the saved map again reproduces it");
    std::printf("ok %s: %zu bytes -> %zu; rebuilt: %d trigger bytes, %d table and sound bytes; "
                "%d cells cleared above random towns%s\n",
                name.c_str(), original.size(), saved.size(), triggers, tables, clearedCells,
                newFormat ? "" : " (older format)");
}

}  // namespace

int main() {
    const char* data = std::getenv("HOMM1_DATA");
    if (data == nullptr || data[0] == '\0') {
        std::printf("skipped: needs HOMM1_DATA\n");
        return 77;
    }
    // A scratch game folder: the data folder linked in, the maps linked
    // one by one into a writable MAPS folder.
    namespace fs = std::filesystem;
    std::error_code error;
    fs::path folder = fs::temp_directory_path(error)
                      / ("homm1-editor-maps-" + std::to_string(std::random_device()()));
    if (error || !fs::create_directories(folder / "MAPS", error)
        || !fs::create_directories(folder / "GAMES", error))
        return 1;
    std::string scratch = folder.string();
    std::string dataDirectory = FindEntry(data, "DATA");
    std::string mapsDirectory = FindEntry(data, "MAPS");
    Expect(!dataDirectory.empty() && !mapsDirectory.empty(), "game data folders");
    if (!LinkOrCopy(dataDirectory, folder / "DATA"))
        return 1;
    std::vector<std::string> maps;
    FileSetRoot(data);
    for (const char* wanted : {"MAPS\\*.MAP"}) {
        FileFindData found;
        i32 find = FileFindFirst(wanted, &found);
        while (find != FILE_INVALID) {
            maps.push_back(found.name);
            if (!FileFindNext(find, &found))
                break;
        }
        FileFindClose(find);
    }
    for (const std::string& name : maps)
        if (!LinkOrCopy(fs::path(mapsDirectory) / name, folder / "MAPS" / name))
            return 1;

    SetVariable("SDL_VIDEODRIVER", "dummy", false);
    SetVariable("SDL_AUDIO_DRIVER", "dummy", false);
    SetVariable("HOMM1_NO_DIALOGS", "1", true);
    SetVariable("HOMM1_CONFIG", (scratch + "/config").c_str(), true);
    if (!KBStartHost(scratch.c_str(), "", 0))
        return 1;
    if (gExec->InitSystem() != 0)
        return 1;
    if (gExec->AddManager(gEditManager, BASE_MANAGER_PRIORITY_UNASSIGNED) != 0)
        return 1;
    for (const std::string& name : maps)
        CheckMap(scratch, name);
    Expect(!maps.empty(), "shipped maps found");

    if (fs::remove_all(folder, error) == 0 || error)
        std::fprintf(stderr, "could not remove %s\n", scratch.c_str());
    if (gFailures != 0) {
        std::fprintf(stderr, "%d failure(s)\n", gFailures);
        std::fflush(stdout);
        std::_Exit(1);
    }
    std::printf("editor maps: all %zu maps round-trip\n", maps.size());
    // The editor's own shutdown path ends the process from deep inside the
    // window manager; the test ends here instead.
    std::fflush(stdout);
    std::_Exit(0);
}

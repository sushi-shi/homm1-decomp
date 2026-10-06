// Regression tests for the retail defects fixed in the scenario editor
// (docs/port/divergences.md). The editor is started headless as in
// editor_maps_test. The placement sweep matters most under the sanitizers,
// where the original wrote outside the map at once.
//
// Needs $HOMM1_DATA; without it the test is skipped (exit code 77).

#include <H1/Ints.h>

#include <BASE/executive.h>
#include <EDITOR/EDITOR.h>
#include <EDITOR/editManager.h>
#include <EDITOR/eventsManager.h>
#include <EDITOR/overlayManager.h>
#include <PLATFORM/Records.h>
#include <SOURCE/KB.h>
#include <SOURCE/mapCell.h>
#include <SOURCE/mapObjectTypes.h>

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>

bool KBStartHost(const char* dataRoot, const char* gameArguments, i32 fullScreen);

namespace {

int gFailures = 0;

void Expect(bool condition, const char* what) {
    std::printf("%s %s\n", condition ? "ok" : "FAIL", what);
    if (!condition)
        gFailures++;
}

// An empty map of one terrain.
void Ground(int terrain) {
    gEditManager->FreeMapExtras();
    gEditManager->ResetArea(0, 0, MAP_CELL_GRID_SIZE, MAP_CELL_GRID_SIZE);
    for (int x = 0; x < MAP_CELL_GRID_SIZE; x++)
        for (int y = 0; y < MAP_CELL_GRID_SIZE; y++)
            gEditManager->m_map.cells[x][y].m_tileIndex =
                static_cast<u8>(terrain * MAP_CELL_TILES_PER_TERRAIN);
}

int FirstTerrain(const overlayType& type) {
    for (int terrain = 0; terrain < EDITOR_TERRAIN_COUNT; terrain++)
        if (type.terrainMask & (1 << terrain))
            return terrain;
    return 1;
}

// Closing the random map dialog balanced the shares against entry -1.
void TerrainShares() {
    double shares[EDITOR_TERRAIN_COUNT] = {10, 20, 30, 15, 5, 10, 10};
    for (int i = 0; i < EDITOR_TERRAIN_COUNT; i++)
        gTerrainPercent[i] = shares[i];
    BalanceTerrainPercents(-1);
    double sum = 0;
    bool finite = true;
    for (int i = 0; i < EDITOR_TERRAIN_COUNT; i++) {
        sum += gTerrainPercent[i];
        finite = finite && std::isfinite(gTerrainPercent[i]);
    }
    Expect(finite && std::fabs(sum - 100.0) < 0.01, "closing the random map dialog keeps the shares");
}

// Each placed town takes an extra record. Records of towns erased (or
// whose placement was undone) were never given back and the 255-entry table
// filled up; the original then overran it. Records neither the map nor its
// undo copy shows are now freed before a new one is taken, and a table still
// full refuses the object.
void ExtraRecordCapacity() {
    const overlayType* town = nullptr;
    for (int i = 0; i < OVERLAY_TYPE_COUNT && town == nullptr; i++)
        if ((gOverlayTypes[i].trigger & MAP_TRIGGER_TYPE_MASK) == MAP_OBJECT_TOWN)
            town = &gOverlayTypes[i];
    Expect(town != nullptr, "a town object type exists");
    if (town == nullptr)
        return;
    Ground(FirstTerrain(*town));
    // A full table, every record named by a town cell along the bottom rows.
    for (int i = MAP_EXTRA_FIRST_RECORD; i < MAP_EXTRA_RECORD_CAPACITY; i++) {
        gEditManager->m_extras[i] = std::calloc(1, sizeof(editTownExtra));
        gEditManager->m_extraSizes[i] = sizeof(editTownExtra);
        mapCell* cell = &gEditManager->m_map.cells[i % MAP_CELL_GRID_SIZE][64 + i / MAP_CELL_GRID_SIZE];
        cell->m_triggerType = MAP_EVENT_TRIGGER(MAP_OBJECT_TOWN);
        cell->m_objectMetadata = static_cast<u8>(i);
    }
    gEditManager->m_extraCount = MAP_EXTRA_RECORD_CAPACITY;
    gEditManager->SaveUndo();
    i32 placed = PlaceOverlay(const_cast<overlayType*>(town), 30, 30);
    Expect(placed == 0 && gEditManager->m_extraCount == MAP_EXTRA_RECORD_CAPACITY,
           "a town is refused when every record is in use");
    // Erase those towns: their records are given back for the next one.
    for (int i = MAP_EXTRA_FIRST_RECORD; i < MAP_EXTRA_RECORD_CAPACITY; i++)
        gEditManager->m_map.cells[i % MAP_CELL_GRID_SIZE][64 + i / MAP_CELL_GRID_SIZE].m_triggerType = 0;
    gEditManager->SaveUndo();
    placed = PlaceOverlay(const_cast<overlayType*>(town), 30, 30);
    Expect(placed != 0 && gEditManager->m_extraCount == MAP_EXTRA_FIRST_RECORD + 1,
           "the records of erased towns are given back");
    // Erased again, but the undo copy still shows it: its record stays.
    gEditManager->SaveUndo();
    gEditManager->ResetArea(0, 0, MAP_CELL_GRID_SIZE, MAP_CELL_GRID_SIZE);
    for (int x = 0; x < MAP_CELL_GRID_SIZE; x++)
        for (int y = 0; y < MAP_CELL_GRID_SIZE; y++)
            gEditManager->m_map.cells[x][y].m_tileIndex =
                static_cast<u8>(FirstTerrain(*town) * MAP_CELL_TILES_PER_TERRAIN);
    placed = PlaceOverlay(const_cast<overlayType*>(town), 40, 40);
    Expect(placed != 0 && gEditManager->m_extraCount == MAP_EXTRA_FIRST_RECORD + 2,
           "a town the undo copy shows keeps its record");
    gEditManager->SaveUndo();
    gEditManager->FreeMapExtras();
}

// The Dragon City and lighthouse coordinates are u8: the test against -1
// never matched, so a map without them wrote (255, 255, type).
void EmptyMineRecords() {
    Ground(2);
    RecordWriter file;
    gEditManager->WriteMines(file);
    bool empty = file.Size() >= 6;
    for (int i = 0; i < 6 && empty; i++)
        empty = file.Data()[i] == EDIT_MAP_NO_RECORD;
    Expect(empty, "a map without Dragon City and lighthouse writes their empty records");
}

// Every object type at every position along the map's edges: the pieces
// that fall outside the map are not written.
void PlacementAtTheEdges() {
    int placements = 0;
    bool outsideUntouched = true;
    for (int i = 0; i < OVERLAY_TYPE_COUNT; i++) {
        overlayType* type = &gOverlayTypes[i];
        if ((type->trigger & MAP_TRIGGER_TYPE_MASK) == MAP_OBJECT_TOWN
            || (type->trigger & MAP_TRIGGER_TYPE_MASK) == MAP_FILE_OBJECT_RANDOM_TOWN
            || (type->trigger & MAP_TRIGGER_TYPE_MASK) == MAP_FILE_OBJECT_RANDOM_CASTLE
            || (type->trigger & MAP_TRIGGER_TYPE_MASK) == MAP_FILE_OBJECT_HERO)
            continue;
        const int spots[][2] = {{-4, 0}, {-2, 1}, {0, 0}, {66, 0}, {68, 2}, {70, 1}, {71, 3},
                                {-3, 71}, {0, 71}, {69, 71}, {71, 71}, {35, 0}, {35, 71}};
        for (const auto& spot : spots) {
            Ground(FirstTerrain(*type));
            editMapCellPair corner = gEditManager->m_map.cellPairs[0][0];
            mapCell bottomLeft = gEditManager->m_map.cells[0][MAP_CELL_GRID_SIZE - 1];
            if (!CanPlaceOverlay(type, static_cast<i16>(spot[0]), static_cast<i16>(spot[1])))
                continue;
            PlaceOverlay(type, static_cast<i16>(spot[0]), static_cast<i16>(spot[1]));
            placements++;
            // Past the last column lies the object id table; above row 0
            // of column x lies the bottom cell of column x - 1.
            if (spot[0] > 2 && (std::memcmp(&corner, &gEditManager->m_map.cellPairs[0][0], sizeof(corner)) != 0))
                outsideUntouched = false;
            if (spot[0] > 2 && spot[0] < 66
                && std::memcmp(&bottomLeft, &gEditManager->m_map.cells[0][MAP_CELL_GRID_SIZE - 1],
                               sizeof(bottomLeft))
                       != 0)
                outsideUntouched = false;
        }
    }
    std::printf("%d placements at the edges\n", placements);
    Expect(placements > 0 && outsideUntouched, "objects at the map's edges stay inside it");
}

}  // namespace

int main() {
    const char* data = std::getenv("HOMM1_DATA");
    if (data == nullptr || data[0] == '\0') {
        std::printf("skipped: needs HOMM1_DATA\n");
        return 77;
    }
    setenv("SDL_VIDEODRIVER", "dummy", 0);
    // SIGTERM ends the program at once (timeout, ctest); SDL would turn it into
    // a quit event that a hung loop never reads.
    setenv("SDL_NO_SIGNAL_HANDLERS", "1", 0);
    setenv("SDL_AUDIO_DRIVER", "dummy", 0);
    setenv("HOMM1_NO_DIALOGS", "1", 1);
    const char* temporary = std::getenv("TMPDIR");
    std::string config = std::string(temporary != nullptr && temporary[0] != '\0' ? temporary : "/tmp")
                         + "/homm1-editor-regressions-XXXXXX";
    if (mkdtemp(config.data()) == nullptr)
        return 1;
    setenv("XDG_CONFIG_HOME", config.c_str(), 1);
    if (!KBStartHost(data, "", 0))
        return 1;
    if (gExec->InitSystem() != 0 || gExec->AddManager(gEditManager, BASE_MANAGER_PRIORITY_UNASSIGNED) != 0)
        return 1;
    TerrainShares();
    ExtraRecordCapacity();
    EmptyMineRecords();
    PlacementAtTheEdges();
    std::string cleanup = "rm -r '" + config + "'";
    if (std::system(cleanup.c_str()) != 0)
        std::fprintf(stderr, "could not remove %s\n", config.c_str());
    std::fflush(stdout);
    std::_Exit(gFailures != 0 ? 1 : 0);
}

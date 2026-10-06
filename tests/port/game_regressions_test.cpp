// Regression tests for the retail out-of-bounds defects fixed in the shared
// game units (docs/port/divergences.md). The test links the native game
// without its entry point and starts it headless (SDL's dummy drivers) on
// the game data, like editor_maps_test. Most cases matter most under the
// sanitizers (ctest in a -DHOMM1_SANITIZERS=ON build), where the original
// code fails at once; each also checks the corrected result.
//
// Needs $HOMM1_DATA; without it the test is skipped (exit code 77).

#include <H1/Ints.h>

#include <BASE/Misc.h>
#include <BASE/executive.h>
#include <BASE/heroWindowManager.h>
#include <BASE/miscwin.h>
#include <BASE/mouseManager.h>
#include <BASE/palette.h>
#include <BASE/resourceManager.h>
#include <PLATFORM/File.h>
#include <PLATFORM/Records.h>
#include <SOURCE/KB.h>
#include <SOURCE/advManager.h>
#include <SOURCE/artifactTypes.h>
#include <SOURCE/creatureTypes.h>
#include <SOURCE/game.h>
#include <SOURCE/hero.h>
#include <SOURCE/kbwin.h>
#include <SOURCE/mapCell.h>
#include <SOURCE/mapObjectTypes.h>
#include <SOURCE/philAI.h>
#include <SOURCE/playerData.h>
#include <SOURCE/resourceTypes.h>
#include <SOURCE/searchArray.h>

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

void StartGame() {
    if (gExec->InitSystem())
        std::_Exit(1);
    gPalette = gResourceManager->GetPalette("kb.pal");
    PostprocessPalette(gPalette->m_data);
    SetPalette(gPalette->m_data, 1);
    gPhilAI->m_debugFont = gResourceManager->GetFont("smalfont.fnt");
    LoadSystemwideIcons();
    memset(gThisNetHumanPlayer, 0, GAME_PLAYER_COUNT);
}

// A new single-player game on a shipped map, as the new game window starts it.
void NewGame(const char* map) {
    gNumHumanPlayers = 1;
    gHumanPlayer[0] = 1;
    gGame->LoadGame(const_cast<char*>("origdata.bin"), 1, 0);
    gGame->m_playerCount = GAME_PLAYER_COUNT;
    for (int i = 1; i < GAME_PLAYER_COUNT; i++)
        gGame->m_players[i].m_difficulty = PLAYER_TYPE_AVERAGE;
    gGame->NewMap(const_cast<char*>(map));
}

// GiveArtifact recorded the spell book's owner one past the random artifact
// table, over the first boat's id.
void SpellBookKeepsBoatTable() {
    hero* bearer = &gGame->m_heroRecs[3];
    memset(bearer->m_artifacts, ARTIFACT_NONE, sizeof(bearer->m_artifacts));
    gGame->m_boats[0].id = 0x55;
    gAdvManager->GiveArtifact(bearer, ARTIFACT_MAGIC_BOOK);
    Expect(gGame->m_boats[0].id == 0x55, "the spell book does not overwrite the boat table");
    Expect(bearer->HasArtifact(ARTIFACT_MAGIC_BOOK) != 0, "the hero has the spell book");
}

// The ultimate artifact's value was read at index -1 once it was dug up.
void ObeliskValueWithoutUltimate() {
    i8 saved = gGame->m_ultimateArtifactId;
    gGame->m_ultimateArtifactId = ARTIFACT_NONE;
    Expect(gPhilAI->TurnValueOfObelisk(1) == 0.0f, "no obelisk value once the artifact is gone");
    gGame->m_ultimateArtifactId = saved;
}

// A site on the top rows put its owner's flag outside the grid.
void MineFlagOnTopRow() {
    u8 before[sizeof(gGame->m_map)];
    memcpy(before, gGame->m_map, sizeof(before));
    i8 slot = GAME_MINE_COUNT - 1;
    mineRecord saved = gGame->m_mines[slot];
    gGame->m_mines[slot].x = 40;
    gGame->m_mines[slot].y = 1;
    gGame->m_mines[slot].type = RESOURCE_MERCURY;
    gGame->ClaimMine(slot, 0);
    Expect(gGame->m_mineOwners[slot] == 0, "a mine on row 1 is claimed");
    Expect(memcmp(before, gGame->m_map, sizeof(before)) == 0, "its flag is not written outside the map");
    gGame->m_mines[slot] = saved;
    gGame->m_mineOwners[slot] = GAME_PLAYER_NONE;
}

// BuildPath followed a stale route out of the grid and kept its steps.
void RouteLeavingTheGrid() {
    gSearchArray->Clear();
    searchNode& node = gSearchArray->m_cells[0][5];
    node.x = 0;
    node.y = 5;
    node.visited = 1;
    node.distance = 1;
    node.direction = MAP_DIRECTION_EAST;  // traced back: west, out of the map
    i32 length = gSearchArray->BuildPath(10, 10, 0, 5, 100);
    Expect(length == 0 && gSearchArray->m_pathLength == 0, "a route traced off the map fails empty");
}

// The map contents are checked when a game starts on them.
void MapChecks() {
    Expect(gGame->MapDataValid() != 0, "the started map is valid");
    mapCell saved = gGame->m_map[10][10];
    gGame->m_map[10][10].m_triggerType = MAP_TRIGGER_EVENT | MAP_OBJECT_MONSTER;
    gGame->m_map[10][10].m_objectIndex = CREATURE_COUNT + 3;
    Expect(gGame->MapDataValid() == 0, "a monster of no creature type is refused");
    gGame->m_map[10][10] = saved;
    gGame->m_map[10][10].m_tileIndex = MAP_CELL_GROUND_TILE_COUNT;
    Expect(gGame->MapDataValid() == 0, "a ground tile past the table is refused");
    gGame->m_map[10][10] = saved;
    gGame->m_map[10][10].m_triggerType = MAP_TRIGGER_EVENT | MAP_OBJECT_TOWN;
    Expect(gGame->MapDataValid() == 0, "a town cell without a town record is refused");
    gGame->m_map[10][10] = saved;
    Expect(gGame->MapDataValid() != 0, "the map is valid again");
}

// Week turns with the empty tavern slots of players not in the game.
void WeekWithAbsentPlayers() {
    gGame->m_players[3].m_availableHeroIds[0] = HERO_ID_NONE;
    gGame->m_players[3].m_availableHeroIds[1] = HERO_ID_NONE;
    gGame->PerWeek();
    Expect(gGame->m_players[3].m_availableHeroIds[0] >= 0, "an absent player's tavern is drawn");
}

// Four-player campaign scenarios read a fourth crest from a three-entry table.
void CampaignCrests() {
    gNumHumanPlayers = 1;
    gHumanPlayer[0] = 1;
    gGame->InitEntireCampaign(1);
    bool distinct = true;
    for (int i = 0; i < gGame->m_playerCount; i++)
        for (int j = i + 1; j < gGame->m_playerCount; j++)
            distinct = distinct && gGame->m_players[i].m_color != gGame->m_players[j].m_color;
    Expect(distinct, "campaign crests are distinct");
}

}  // namespace

int main() {
    const char* data = std::getenv("HOMM1_DATA");
    if (data == nullptr || data[0] == '\0') {
        std::printf("skipped: needs HOMM1_DATA\n");
        return 77;
    }
    setenv("SDL_VIDEODRIVER", "dummy", 0);
    setenv("SDL_AUDIO_DRIVER", "dummy", 0);
    setenv("HOMM1_NO_DIALOGS", "1", 1);
    const char* temporary = std::getenv("TMPDIR");
    std::string config = std::string(temporary != nullptr && temporary[0] != '\0' ? temporary : "/tmp")
                         + "/homm1-game-regressions-XXXXXX";
    if (mkdtemp(config.data()) == nullptr)
        return 1;
    setenv("XDG_CONFIG_HOME", config.c_str(), 1);
    if (!KBStartHost(data, "/I0", 0))
        return 1;
    StartGame();
    NewGame("AES31000.MAP");
    SpellBookKeepsBoatTable();
    ObeliskValueWithoutUltimate();
    MineFlagOnTopRow();
    RouteLeavingTheGrid();
    MapChecks();
    WeekWithAbsentPlayers();
    CampaignCrests();
    std::string cleanup = "rm -r '" + config + "'";
    if (std::system(cleanup.c_str()) != 0)
        std::fprintf(stderr, "could not remove %s\n", config.c_str());
    std::fflush(stdout);
    std::_Exit(gFailures != 0 ? 1 : 0);
}

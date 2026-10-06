// Maps and campaign maps through the game's own readers, in the game's units
// started headless: the map is written to the scratch game folder's MAPS,
// listed by the file requester (which reads every map's header for the list)
// and started as a new game with game::NewMap, which reads it with
// game::LoadMap and sets the game up from it, as the new game dialog does
// after loading the original game data.
//
// The input is the map file followed by one byte with the new game dialog's
// choices:
//   bit 7 clear  a standard game: 2 + (bits 0-1) % 3 players, of whom
//                1 + (bits 2-3) % players are human (hot seat);
//   bit 7 set    a campaign scenario, as InitCampaignMap starts it (the map is
//                written as CAMPn.CMP): side 1 + (bits 0-1), scenario
//                (bits 2-6) % 9.
//
// With HOMM1_FUZZ_LOAD_ONLY=1 the set-up stops after game::LoadMap, to fuzz
// the reader alone.

#include "FuzzSupport.h"

#include <SOURCE/campaignTypes.h>
#include <SOURCE/fileRequester.h>
#include <SOURCE/game.h>
#include <SOURCE/KB.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>

namespace {

std::string gRoot;
bool gLoadOnly = false;

}  // namespace

extern "C" int LLVMFuzzerInitialize(int*, char***) {
    FuzzRequireData("fuzz_map");
    gRoot = FuzzScratchGame();
    FuzzStartHost(gRoot);
    const char* loadOnly = std::getenv("HOMM1_FUZZ_LOAD_ONLY");
    gLoadOnly = loadOnly != nullptr && std::strcmp(loadOnly, "1") == 0;
    return 0;
}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
    if (size < 1 || size > (1u << 20))
        return 0;
    u8 setup = data[size - 1];
    size--;
    bool campaign = (setup & 0x80) != 0;
    i32 scenario = ((setup >> 2) & 0x1f) % CAMPAIGN_SCENARIO_COUNT;
    char name[16];
    if (campaign)
        std::snprintf(name, sizeof(name), "CAMP%d.CMP", scenario + 1);
    else
        std::snprintf(name, sizeof(name), "FUZZ.MAP");
    std::string path = gRoot + "/MAPS/" + name;
    if (!FuzzWriteFile(path, data, size))
        return 0;

    // The scenario list reads each map's header; selecting a map shows its
    // size and difficulty by name (fileRequester::ShowMapInfo).
    RunGame([] {
        gShowMapInfo = 1;
        fileRequester* requester =
            new fileRequester(0, 0, FILE_REQUESTER_LOAD, "*.MAP", ".\\MAPS\\", ".MAP");
        for (i32 i = 0; i < requester->m_fileCount; i++) {
            volatile size_t shown = std::strlen(gMapSizeNames[requester->m_mapInfo[i].size])
                                    + std::strlen(gMapDifficultyNames[requester->m_mapInfo[i].difficulty])
                                    + std::strlen(requester->m_mapNames[i].text)
                                    + std::strlen(requester->m_mapInfo[i].description);
            (void)shown;
        }
        delete requester;
    });

    RunGame([&] {
        gNumHumanPlayers = 1;
        if (campaign && !gLoadOnly) {
            gGame->m_campaignType = 1 + (setup & 3);
            gGame->m_campaignScenario = scenario;
            gGame->m_campaignScenariosWon = 0;
            gGame->m_campaignDay = 1;
            gGame->InitCampaignMap(scenario, 0);
            return;
        }
        i32 players = 2 + (setup & 3) % 3;
        gNumHumanPlayers = 1 + ((setup >> 2) & 3) % players;
        gGame->LoadGame(const_cast<char*>("origdata.bin"), 1, 0);
        gGame->m_campaignType = 0;
        for (i32 i = 0; i < GAME_PLAYER_COUNT; i++) {
            gHumanPlayer[i] = i < gNumHumanPlayers;
            gThisNetHumanPlayer[i] = gHumanPlayer[i];
            if (i >= players)
                gGame->m_players[i].m_difficulty = PLAYER_TYPE_NONE;
            else if (gGame->m_players[i].m_difficulty == PLAYER_TYPE_NONE)
                gGame->m_players[i].m_difficulty = 1;
        }
        gGame->m_playerCount = static_cast<i8>(players);
        if (gLoadOnly)
            gGame->LoadMap(name);
        else
            gGame->NewMap(const_cast<char*>("FUZZ.MAP"));
    });
    std::remove(path.c_str());
    return 0;
}

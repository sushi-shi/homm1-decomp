// Saved games through the game's own loader: the input is written to the
// scratch game folder's GAMES and loaded with game::LoadGame, as the load
// game requester does, in the game's units started headless.
//
// The input is the saved game followed by one byte: the number of human
// players chosen in the set-up dialog, 1 + (byte % 4). The load requester
// lists the folder first; it also holds a file whose name is longer than the
// original's 8.3 names, which the list must leave out.

#include "FuzzSupport.h"

#include <SOURCE/fileRequester.h>
#include <SOURCE/game.h>
#include <SOURCE/KB.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>

namespace {

std::string gRoot;

}  // namespace

extern "C" int LLVMFuzzerInitialize(int*, char***) {
    FuzzRequireData("fuzz_savegame");
    gRoot = FuzzScratchGame();
    FuzzStartHost(gRoot);
    const uint8_t nothing = 0;
    FuzzWriteFile(gRoot + "/GAMES/LONGNAME.GM12345", &nothing, 0);
    return 0;
}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
    if (size < 1 || size > (1u << 20))
        return 0;
    if (!FuzzWriteFile(gRoot + "/GAMES/FUZZ.GM1", data, size - 1))
        return 0;
    gNumHumanPlayers = 1 + data[size - 1] % 4;
    RunGame([] {
        gShowMapInfo = 0;
        fileRequester* requester =
            new fileRequester(0, 0, FILE_REQUESTER_LOAD, "*.GM*", ".\\GAMES\\", ".GM*");
        for (i32 i = 0; i < requester->m_fileCount; i++) {
            if (std::strcmp(requester->m_extensions[i].text, ".GM1") != 0) {
                std::fprintf(stderr, "fuzz_savegame: the list holds %s%s\n",
                             requester->m_fileNames[i].text, requester->m_extensions[i].text);
                std::abort();
            }
        }
        delete requester;
    });
    RunGame([] { gGame->LoadGame(const_cast<char*>("FUZZ.GM1"), 0, 0); });
    return 0;
}

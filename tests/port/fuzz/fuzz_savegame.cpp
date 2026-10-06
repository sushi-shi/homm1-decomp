// Saved games through the game's own loader: the input is written to the
// scratch game folder's GAMES and loaded with game::LoadGame, as the load
// game requester does, in the game's units started headless.
//
// The input is the saved game followed by one byte: the number of human
// players chosen in the set-up dialog, 1 + (byte % 4).

#include "FuzzSupport.h"

#include <SOURCE/game.h>
#include <SOURCE/KB.h>

#include <string>

namespace {

std::string gRoot;

}  // namespace

extern "C" int LLVMFuzzerInitialize(int*, char***) {
    FuzzRequireData("fuzz_savegame");
    gRoot = FuzzScratchGame();
    FuzzStartHost(gRoot);
    return 0;
}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
    if (size < 1 || size > (1u << 20))
        return 0;
    if (!FuzzWriteFile(gRoot + "/GAMES/FUZZ.GM1", data, size - 1))
        return 0;
    gNumHumanPlayers = 1 + data[size - 1] % 4;
    RunGame([] { gGame->LoadGame(const_cast<char*>("FUZZ.GM1"), 0, 0); });
    return 0;
}

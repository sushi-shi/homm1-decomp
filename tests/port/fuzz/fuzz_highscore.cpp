// The high score tables through the game's own reader and display, in the
// game's units started headless: the input is written as both DATA tables
// (STANDARD.HS and CAMPAIGN.HS) and shown by highScoreManager::Update on the
// high score window, once for each table, as the high score screen does.

#include "FuzzSupport.h"

#include <BASE/heroWindow.h>
#include <SOURCE/highScoreManager.h>
#include <SOURCE/KB.h>

#include <string>

namespace {

std::string gRoot;

}  // namespace

extern "C" int LLVMFuzzerInitialize(int*, char***) {
    FuzzRequireData("fuzz_highscore");
    gRoot = FuzzScratchGame();
    FuzzStartHost(gRoot);
    return 0;
}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
    if (size > (1u << 16))
        return 0;
    if (!FuzzWriteFile(gRoot + "/DATA/STANDARD.HS", data, size)
        || !FuzzWriteFile(gRoot + "/DATA/CAMPAIGN.HS", data, size))
        return 0;
    RunGame([] {
        highScoreManager scores;
        scores.m_window = new heroWindow(0, 0, const_cast<char*>("hiscore.bin"));
        for (i32 table = 0; table < 2; table++) {
            scores.m_showCampaignScores = static_cast<i8>(table);
            scores.Update();
        }
        delete scores.m_window;
    });
    return 0;
}

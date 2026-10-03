#ifndef HOMM1_SOURCE_HIGHSCORE_RUNTIME_H
#define HOMM1_SOURCE_HIGHSCORE_RUNTIME_H

#include <Domains.h>

// HighScoreEntry::score of an unused rank (KB AddScoreToHighScore, Update).
H1_ENUM_CONST_BEGIN(HighScoreRuntimeConstant)
    HIGH_SCORE_EMPTY = -1
H1_ENUM_CONST_END(HighScoreRuntimeConstant)

// HoMM1 score files hold 0x57-byte records; Update reads name, scenario and
// score from the fixed prefix.
#pragma pack(push, 1)
                     struct HighScoreEntry {
    char playerName[17];
    char scenarioName[15];
    int score;
    char unknown24[0x33];
};
#pragma pack(pop)


#endif

#ifndef HOMM1_SOURCE_CAMPAIGNTYPES_H
#define HOMM1_SOURCE_CAMPAIGNTYPES_H

#include <SOURCE/game.h>

enum CampaignChoice {
    CAMPAIGN_NONE = 0,
    CAMPAIGN_IRONFIST = 1,
    CAMPAIGN_SLAYER = 2,
    CAMPAIGN_LAMANDA = 3,
    CAMPAIGN_ALAMAR = 4
};

enum CampaignScenarioTableConstant {
    CAMPAIGN_SCENARIO_COUNT = 9
};

#pragma pack(push, 1)
struct campaignScenario {
    i8 kingOfTheHill;
    i8 victoryTownX;
    i8 victoryTownY;
    char victoryTownName[0x10];
    i8 playerTypes[GAME_PLAYER_COUNT];
    i16 playerCrests[3];
    u16 resources[GAME_PLAYER_COUNT][7];
};
#pragma pack(pop)

#endif

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
    CAMPAIGN_SCENARIO_COUNT = 9,
    // Entries of a scenario's crest table, read by player number.
    CAMPAIGN_CREST_COUNT = 3
};

enum CampaignScenarioRow {
    CAMPAIGN_SCENARIO_1 = 0,
    CAMPAIGN_SCENARIO_EYE_OF_GOROS = 2,
    CAMPAIGN_SCENARIO_5 = 4,
    CAMPAIGN_SCENARIO_6 = 5,
    CAMPAIGN_SCENARIO_7 = 6,
    CAMPAIGN_SCENARIO_8 = 7,
    CAMPAIGN_SCENARIO_DRAGON_CITY = 8
};

#pragma pack(push, 1)
struct campaignScenario {
    b8 kingOfTheHill;
    i8 victoryTownX;
    i8 victoryTownY;
    char victoryTownName[0x10];
    i8 playerTypes[GAME_PLAYER_COUNT];
    i16 playerCrests[CAMPAIGN_CREST_COUNT];
    u16 resources[GAME_PLAYER_COUNT][RESOURCE_COUNT];
};
#pragma pack(pop)

#endif

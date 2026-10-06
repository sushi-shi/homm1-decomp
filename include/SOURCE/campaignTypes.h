#ifndef HOMM1_SOURCE_CAMPAIGNTYPES_H
#define HOMM1_SOURCE_CAMPAIGNTYPES_H

#include <SOURCE/game.h>

// gCampaignChoice: the campaign being played (0 = a standalone map). The
// stpcmpgn.bin choices and the New Campaign menu commands select them in the
// gSetupCampaignGameHelp order (Ironfist, Slayer, Lamanda, Alamar).
H1_ENUM_BEGIN(CampaignChoice)
    CAMPAIGN_NONE = 0,
    CAMPAIGN_IRONFIST = 1,
    CAMPAIGN_SLAYER = 2,
    CAMPAIGN_LAMANDA = 3,
    CAMPAIGN_ALAMAR = 4
H1_ENUM_END(CampaignChoice)

// gCampaignScenarios rows: CheckEndGame completes the campaign after the
// last of the COUNT scenarios.
H1_ENUM_CONST_BEGIN(CampaignScenarioTableConstant)
    CAMPAIGN_SCENARIO_COUNT = 9
H1_ENUM_CONST_END(CampaignScenarioTableConstant)

// gCampaignScenarios rows with their own CheckEndGame objective (the player
// sees row + 1). Scenarios 1 and 5-8 are won by holding the victoryTown and
// scenario 1 is lost when an opponent takes it; scenario 3 is the Eye of Goros
// hunt for an ultimate artifact; scenario 9 is lost with Dragon City.
H1_ENUM_CONST_BEGIN(CampaignScenarioRow)
    CAMPAIGN_SCENARIO_1 = 0,
    CAMPAIGN_SCENARIO_EYE_OF_GOROS = 2,
    CAMPAIGN_SCENARIO_5 = 4,
    CAMPAIGN_SCENARIO_6 = 5,
    CAMPAIGN_SCENARIO_7 = 6,
    CAMPAIGN_SCENARIO_8 = 7,
    CAMPAIGN_SCENARIO_DRAGON_CITY = 8
H1_ENUM_CONST_END(CampaignScenarioRow)

// Campaign scenario table: 85-byte records with the King of the Hill
// flag, the town CheckEndGame watches, the three opponents' player types and
// every player's starting resources.
#pragma pack(push, 1)
struct campaignScenario {
    i8 kingOfTheHill;
    i8 victoryTownX;
    i8 victoryTownY;
    // Space-padded name of the campaign town the map renames (not NUL-terminated).
    char victoryTownName[0x10];
    i8 playerTypes[GAME_PLAYER_COUNT];
    // Opponent crests, indexed by player position; PLAYER_COLOR_COUNT leaves
    // the crest to RandomizePlayerCrests' draw.
    i16 playerCrests[3];
    H1_ENUM_ARRAY(u16, resources[GAME_PLAYER_COUNT], ResourceType, RESOURCE_COUNT);
};
#pragma pack(pop)

#endif // HOMM1_SOURCE_CAMPAIGNTYPES_H

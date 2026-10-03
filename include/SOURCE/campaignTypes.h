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

// HoMM1's campaign scenario table: 85-byte records with the King of the Hill
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
    // Opponent crests, indexed by player position.
    i16 playerCrests[3];
    u16 resources[GAME_PLAYER_COUNT][7];
};
#pragma pack(pop)

#endif // HOMM1_SOURCE_CAMPAIGNTYPES_H

#ifndef HOMM1_SOURCE_CAMPAIGNTYPES_H
#define HOMM1_SOURCE_CAMPAIGNTYPES_H

#include <SOURCE/game.h>

// clang-format off
// giCampaignChoice: the campaign being played (0 = a standalone map). The
// stpcmpgn.bin choices and the New Campaign menu commands select them in the
// gSetupCampaignGameHelp order (Ironfist, Slayer, Lamanda, Alamar).
H1_ENUM_BEGIN(CampaignChoice)
    CAMPAIGN_NONE = 0,
    CAMPAIGN_IRONFIST = 1,
    CAMPAIGN_SLAYER = 2,
    CAMPAIGN_LAMANDA = 3,
    CAMPAIGN_ALAMAR = 4
H1_ENUM_END(CampaignChoice)
// clang-format on

// HoMM1's campaign scenario table: 85-byte records with the King of the Hill
// flag, the town CheckEndGame watches, the three opponents' player types and
// every player's starting resources.
#pragma pack(push, 1)
struct campaignScenario {
    signed char kingOfTheHill;
    signed char victoryTownX;
    signed char victoryTownY;
    // Space-padded name of the campaign town the map renames (not NUL-terminated).
    char victoryTownName[0x10];
    signed char playerTypes[GAME_PLAYER_COUNT];
    // Opponent crests, indexed by player position.
    short playerCrests[3];
    unsigned short resources[GAME_PLAYER_COUNT][7];
};
#pragma pack(pop)
extern campaignScenario gCampaignScenarios[];
// New-game "King of the Hill" option; campaign scenarios preset it.
extern signed char gbIAmGreatest;

#endif // HOMM1_SOURCE_CAMPAIGNTYPES_H

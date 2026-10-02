#ifndef HOMM1_SOURCE_CAMPAIGNTYPES_H
#define HOMM1_SOURCE_CAMPAIGNTYPES_H

#include <SOURCE/game.h>

// HoMM1's campaign scenario table: 85-byte records with the King of the Hill
// flag, the town CheckEndGame watches, the three opponents' player types and
// every player's starting resources.
#pragma pack(push, 1)
struct campaignScenario {
    signed char kingOfTheHill;
    signed char victoryTownX;
    signed char victoryTownY;
    char unknown03[0x10];
    signed char playerTypes[GAME_PLAYER_COUNT];
    char unknown17[6];
    unsigned short resources[GAME_PLAYER_COUNT][7];
};
#pragma pack(pop)
extern campaignScenario gCampaignScenarios[];
// New-game "King of the Hill" option; campaign scenarios preset it.
extern signed char gbKingOfTheHill;
// Days elapsed in the current game (week/month calendar flattened).
extern int giCurTurn;

#endif // HOMM1_SOURCE_CAMPAIGNTYPES_H

#ifndef HOMM1_SOURCE_X_GLOBAL_H
#define HOMM1_SOURCE_X_GLOBAL_H

#include <SOURCE/hero.h>
#include <SOURCE/mapCell.h>

class armyGroup;

// Global tables and state as in Buka's X_GLOBAL.h; KBDeclarations-style
// globals stay in SOURCE/KB.h. Most sit in KB's retail data band; the rest wait
// for the data campaign to place them.

// gAdvDisposeLevel while combat runs: how much adventure-screen art the
// resource manager may release (Buka X_GLOBAL.h).
H1_ENUM_BEGIN(AdvDisposeLevel)
    ADV_DISPOSE_NONE = 0,
    ADV_DISPOSE_PARTIAL = 1,
    ADV_DISPOSE_FULL = 2
H1_ENUM_END(AdvDisposeLevel)




extern armyGroup* gpMonsterGroup;

// SaveGame files the current player through this byte.
extern signed char gSaveCurPlayer;
extern char gMapCellScratch[MAP_CELL_GRID_SIZE][MAP_CELL_GRID_SIZE];

// NewGame remembers the last new-game settings for the next setup screen.
extern signed char gbNewGameSettingsSaved;
extern signed char gcSavedDifficulty;
extern signed char gcSavedPlayerTypes[];
extern signed char gbSavedKingOfTheHill;
extern signed char gcSavedCrest;

#pragma pack(push, 1)
struct campaignTownName {
    signed char x;
    signed char y;
    char name[83];
};
#pragma pack(pop)
extern campaignTownName gCampaignTownNames[];

extern signed char gRandomTownTypes[4];
extern signed char gTownTypeHeroClass[];
extern signed char gClassVisionRange[];
extern signed char gSpellAttributes[];
extern short giMineTypeCount[];


extern short giLastMapOriginX;
extern short giLastMapOriginY;
// HoMM1's three-byte exit notice: game position, control hand-off, next player.
#pragma pack(push, 1)
struct playerExitMessage {
    signed char gamePosition;
    signed char takesControl;
    signed char nextPlayer;
};
#pragma pack(pop)
extern playerExitMessage gPlayerExitMessage;


#endif

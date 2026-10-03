#ifndef HOMM1_SOURCE_X_GLOBAL_H
#define HOMM1_SOURCE_X_GLOBAL_H

#include <SOURCE/hero.h>
#include <SOURCE/mapCell.h>

class armyGroup;

// Global tables and state as in Buka's X_GLOBAL.h; KBDeclarations-style
// globals stay in SOURCE/KB.h. Most sit in KB's retail data band; the rest wait
// for the data campaign to place them.

extern int gbEnlargeScreenBlit;
extern int gAdvDisposeLevel;
// gAdvDisposeLevel while combat runs: how much adventure-screen art the
// resource manager may release (Buka X_GLOBAL.h).
H1_ENUM_BEGIN(AdvDisposeLevel)
    ADV_DISPOSE_NONE = 0,
    ADV_DISPOSE_PARTIAL = 1,
    ADV_DISPOSE_FULL = 2
H1_ENUM_END(AdvDisposeLevel)

// Adventure control panel: option labels, then the control-panel and
// adventure-panel help lines.
extern char* onOffText[];
extern char* walkSpeedText[];
extern char* musicQualityText[];
extern char* gCPanelHelp[];
extern char* gAPanelHelp[];

// Combat ground tiles (0x00491058) and obstacle icons (0x00491078) per
// combat terrain.
extern char* cCombatGroundNames[];
extern char* cCombatObstacleNames[];
// Primary stat, morale and luck labels of the general's stats text, and the
// combat command help lines HandleViewGeneral shows (entries 1-5).
extern char* cViewGeneralLabels[];
extern char* cViewGeneralHelp[];

// Hero frame flips for the horse and boat walk cycles.
extern short horseFrameFlip[];
extern short boatFrameFlip[];

// Event texts, player colour names and the wandering-monster group of the
// current encounter.
extern char* gEventText[];
extern char* gColorNames[];
extern armyGroup* gpMonsterGroup;

// Calendar specials: week/month type and the featured creature or name.
extern signed char giWeekType;
extern signed char giMonthType;
extern signed char giWeekTypeExtra;
extern signed char giMonthTypeExtra;
// SaveGame files the current player through this byte.
extern signed char gSaveCurPlayer;
// Morale and luck names, indexed from -3, and their info-window texts.
extern char* gMoraleText[];
extern char* gLuckText[];
extern char* gMoraleInfoText[];
extern char* gLuckInfoText[];
// Default hero names (name, short name) restored with the original data, and
// a per-cell scratch map cleared on every load.
extern char* gHeroNames[][2];
extern char gMapCellScratch[MAP_CELL_GRID_SIZE][MAP_CELL_GRID_SIZE];
// Hero level names and the per-class primary-skill gain table.
extern char* cHeroLevel[];
extern signed char gHeroSkillBonus[][9][HERO_PRIMARY_STAT_COUNT];

// New-game screen: right-click help and the human/computer seat labels.
extern char* gNewGameHelp[];
extern char* gHumanPlayerTypeNames[];
extern char* gPlayerTypeNames[];
// NewGame remembers the last new-game settings for the next setup screen.
extern signed char gcSavedDifficulty;
extern signed char gcSavedPlayerTypes[];
extern signed char gbSavedKingOfTheHill;
extern signed char gcSavedCrest;
// Scenario-info labels: human seat handicap and map difficulty names.
extern char* gHandicapNames[];
extern char* gMapDifficultyNames[];

// Campaign: the lord picked on stpcmpgn.bin (1-4; PickLoadGame filters *.CGM
// on it), scenario titles and briefings, two crest bytes per side (the first
// is the human player's), side names and win texts, and the town a campaign
// map renames at a fixed position (x, y, then the name).
extern signed char giCampaignChoice;
extern char* gCampaignScenarioNames[];
extern char* gCampaignScenarioText[];
extern signed char gCampaignSideCrests[][2];
extern char* gCampaignSideNames[];
extern char* gCampaignWinTexts[];
#pragma pack(push, 1)
struct campaignTownName {
    signed char x;
    signed char y;
    char name[83];
};
#pragma pack(pop)
extern campaignTownName gCampaignTownNames[];
extern char* gTownNames[];

// NewMap: the town type of each crest and the types already given to the
// first four random towns; the starting hero class of each campaign crest and
// of each town type, each hero class's sight radius, the starting resources by
// difficulty, the spell attribute bits and mage-guild pool by spell level, the
// vision radius a claimed town grants and the mines placed per type.
extern short gCrestTownTypes[];
extern signed char gRandomTownTypes[4];
extern short gCrestHeroClass[];
extern signed char gTownTypeHeroClass[];
extern signed char gClassVisionRange[];
extern int gStartingResources[][7];
extern signed char gSpellAttributes[];
extern signed char gMageGuildSpellPool[4][8];
extern signed char giVisRangeTown;
extern short giMineTypeCount[];

// Army info strings: attack, defense, shots (combat), damage, hit points,
// speed, morale, luck, shots (adventure); then the speed names.
extern char* gArmyStatText[];
extern char* gSpeedText[];
// Kingdom overview text: the dated title, then Dragon City and Lighthouse.
extern char* gOverviewText[];
// New-turn texts: days-left and last-day warnings, then the month/week
// banners and names.
extern char* gNewTurnText[];
extern char* gMonthNames[];
extern char* gWeekNames[];

// oldmain's re-entry guard and the intro, end-sequence and remote state it
// shares with the game screens.
extern signed char bKBDone;
extern signed char gbSkipIntro;
extern signed char gbWaitForRemoteReceive;
extern short giLastMapOriginX;
extern short giLastMapOriginY;
extern char gcWinText[];
extern char* gInitMenuHelp[];
// The other side's ready flag and the heartbeat-seen flag (REMOTE).
extern int gbRemoteReady;
extern int gbHeartbeatSeen;
// HoMM1's three-byte exit notice: game position, control hand-off, next player.
#pragma pack(push, 1)
struct playerExitMessage {
    signed char gamePosition;
    signed char takesControl;
    signed char nextPlayer;
};
#pragma pack(pop)
extern playerExitMessage gPlayerExitMessage;
// CheckEndGame's re-entry guard and last offered score, the creature
// alignment names (by type / 6) and the score labels.
extern signed char bInCheckEndGame;
extern int giScore;
extern char* gAlignmentNames[];
extern char* gScoreLabels[];

// Setup screens: the setup-dialog flag kbwin's menus check and the
// right-click help of each setup dialog.
extern int gbInSetupDialog;
extern char* gSetupCampaignGameHelp[];
extern char* gSetupComPortHelp[];
extern char* gSetupDCComPortHelp[];
extern char* gSetupBaudHelp[];
extern char* gSetupDCBaudHelp[];
extern char* gSetupHotSeatGameHelp[];
extern char* gSetupModemGameHelp[];
extern char* gSetupDCGameHelp[];
extern char* gSetupMultiPlayerGameHelp[];
extern char* gSetupNetworkGameHelp[];
extern char* gSetupGameHelp[];

#endif

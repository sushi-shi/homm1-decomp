#ifndef HOMM1_SOURCE_KB_H
#define HOMM1_SOURCE_KB_H

#include <Domains.h>
#include <SOURCE/armyGroup.h>
#include <SOURCE/combatTypes.h>
#include <SOURCE/dialogTypes.h>
#include <SOURCE/FINDPATH.h>
#include <SOURCE/hero.h>
#include <SOURCE/mapCell.h>
#include <SOURCE/terrainTypes.h>

// WaitEndSample's waitTime: a negative wait means the default 4000 ms.
H1_ENUM_CONST_BEGIN(SampleWaitConstant)
    SAMPLE_WAIT_DEFAULT = -1
H1_ENUM_CONST_END(SampleWaitConstant)

// stpmain.bin buttons: InitMenuHandler returns the id as m_dialogResult and
// oldmain dispatches it. gGameCommand re-enters the same switch with the
// adventure screen's new/load/quit commands (advManager::ControlPanel's
// cpanel.bin ids and the N/L/Q hotkeys in advManager::Main), which share
// these values; MAIN_MENU_NO_COMMAND is the idle value. InitMenuHandler
// accepts ids 1..MAIN_MENU_LAST.
H1_ENUM_BEGIN(MainMenuControl)
    MAIN_MENU_NO_COMMAND = -1,
    MAIN_MENU_NEW_GAME = 1,
    MAIN_MENU_LOAD_GAME = 2,
    MAIN_MENU_QUIT = 4,
    MAIN_MENU_HIGH_SCORES = 5,
    MAIN_MENU_CREDITS = 6,
    MAIN_MENU_LAST = MAIN_MENU_CREDITS
H1_ENUM_END(MainMenuControl)

extern char gbInPollSound;
extern char gbNoSound;
extern signed char gbShowHighScore;
// HeroView and the kingdom overview raise these while their screens are up;
// NormalDialog only parks over the adventure map when neither is showing.
extern signed char gbHeroWindShowing;
extern signed char gbOverviewShowing;
extern signed char giHighScoreType;
extern signed char giTerrainCost[FINDPATH_TERRAIN_COUNT][FINDPATH_STEP_COST_COUNT];
// Cell tile index -> terrain type; IsMobile reads it zero-extended.
extern H1_ENUM_STORAGE(TerrainType, signed char) giGroundToTerrain[];
// The terrain type under a map cell (Buka 2.1 KBDeclarations.h).
#define CELL_TERRAIN(cell) (giGroundToTerrain[(cell)->m_tileIndex])
extern int bShowIt;
extern char gText[];
extern char* gArmyNames[];
extern char* gArmyNamesPlural[];
// A creature's name, plural for counts above one (Buka 2.1 KBDeclarations.h; HoMM1
// tests count > 1).
#define CREATURE_DISPLAY_NAME(type, count) ((count) > 1 ? gArmyNamesPlural[type] : gArmyNames[type])
extern struct tag_monsterInfo gMonsterDatabase[];
extern int gbMinimized;
extern signed char gbInMemError;
extern char* gcMemoryErrorTitle;
extern char* gcMemoryRequirements;
extern char* gcExtendedMemoryUnits;
extern char* gcConventionalMemoryUnits;
extern int giRequiredExtendedMemory;
extern int giRequiredConventionalMemory;
extern int gbLoadingMonoIcon;
extern struct configStruct gConfig;
// Retail DoDimensionDoor walks gpSearchArray paths through this delta table.
extern struct tag_tilePoint normalDirTable[];
extern char* DEFAULT_AGGREGATE_NAME;
extern class resourceManager* gpResourceManager;
extern class soundManager* gpSoundManager;
extern class heroWindowManager* gpWindowManager;
extern class mouseManager* gpMouseManager;
extern class heroWindow* pNormalDialogWindow;
extern class advManager* gpAdvManager;
extern signed char gbThisNetHumanPlayer[];
extern class townManager* gpTownManager;
extern class combatManager* gpCombatManager;
extern class executive* gpExec;
extern class game* gpGame;
extern int giHighMemBuffer;
extern int giBottomViewOverride;
extern long giBottomViewOverrideEndTime;
extern int giBottomViewResource;
extern int giBottomViewResourceQty;
extern char gcBottomViewText[];
extern void* hmnuAdv;
extern void* hmnuDflt;
extern void* hmnuCmbt;
extern void* hmnuTown;
extern int gbHeroMoving;
extern int gbRemoteOn;
extern class heroWindow* DataEntryWin;
extern char* cDEDest;
extern int iDEMaxLen;
extern signed char bDataEntryTime;
extern H1_ENUM_STORAGE(DialogWaitType, signed char) giWaitType;
extern signed char gbFunctionComplete;
// Artifact names (0x00493048).
extern char* gArtifactNames[];
extern char* gNeutralBuildingNames[];
extern char* gDwellingNames[];
// BuyBuild's building descriptions (0x00493e78, 0x00493908) and per-dwelling
// prerequisite building masks (0x00491a68); CanBuild reads six masks per
// faction.
extern char* gNeutralBuildingDescriptions[];
extern char* gDwellingDescriptions[];
extern unsigned short gDwellingRequirements[];
extern int gMageBuildingCosts[][7];
extern int gNeutralBuildingCosts[][7];
extern int gDwellingCosts[][7];
extern int gMageBaseResourceValues[];
extern int gNeutralBaseResourceValues[];
extern int gDwellingBaseResourceValues[];
extern char cNetBoxLine[][60];
// ppMapExtra/pwSizeOfMapExtra: the map file's extra records (signs, events,
// town customizations), addressed by a cell's or town's byte index. Record 0
// is never allocated, so iMaxMapExtra restarts at FIRST_RECORD (InitVars,
// ClearMapExtra, game::LoadMap) and ClearMapExtra frees every slot.
H1_ENUM_CONST_BEGIN(MapExtraConstant)
    MAP_EXTRA_FIRST_RECORD = 1,
    MAP_EXTRA_RECORD_CAPACITY = 255
H1_ENUM_CONST_END(MapExtraConstant)
extern void* ppMapExtra[];
extern class icon* gBuyBuildIcons;
extern class icon* gSystemIcons;
extern class font* bigFont;
extern class font* smallFont;
extern short giScoreMon[][2];
extern short giScoreCampaignMon[][2];
// Combat effect icon files by effect (0x004910d8) and the one loaded effect
// icon (0x004c709c) army draws and PowEffect share.
extern char* gCombatFxNames[];
extern class icon* gCurLoadedSpellIcon;

// HoMM1 KB name table accessor (retail 0x004516bf).
char* GetMonsterName(int);
struct SAMPLE2 LoadPlaySample(char*);
void WaitEndSample(struct SAMPLE2, int);
// Empty sample pair copied into locals before LoadPlaySample (0x004c5180).
extern struct SAMPLE2 NULL_SAMPLE2;
extern int glTimers[];
// Shared glTimers slots (Buka KBDeclarations.h numbers the same ones); the
// table ends at giScore (0x004c6a98), six slots. Units keep slots 0 and 1.
H1_ENUM_CONST_BEGIN(GlobalTimerConstant)
    GLOBAL_TIMER_COUNT = 6,
    GLOBAL_BUTTON_REPEAT_TIMER_SLOT = 2,
    GLOBAL_MUSIC_FADE_TIMER_SLOT = 4,
    GLOBAL_POLL_SOUND_TIMER_SLOT = 5
H1_ENUM_CONST_END(GlobalTimerConstant)
void EarlyShutDownSystem();
void QuickViewWait();
signed char CanBuild(class town*, short);
signed char CanBuy(class town*, short);
extern "C" void PollSound();
void ForcePollSound();
char toupper(char);
short NullHandler(struct tag_message&);
char* GetBuildingName(int, short);
void GetBuildingCost(int, short, int* const, int);
char* GetMonsterName(int);
int GetBuildingBaseResourceValue(int, int, int);
void AddNetBoxLine(char*);
void GOut(char*);
extern signed char bEarlySetupDone;
extern int giShowIntro;
extern signed char giScreenScroll;
extern int gbBlackoutPlayer;
extern char gMapName[];
extern char gFullMapName[];
extern char gMapDescription[];
extern char cAggPathName[];
extern int giNumHumanPlayers;
extern int gbHumanPlayer[];
void InitMainClasses(void);
void InitVars(void);
int InterpretCommandLine(void);
void ClearMapExtra(void);
short GetMonType(int, int);
int MemSize(int);
signed char CheckMem(void);
int IsCDDrive(int);
void LoadSystemwideIcons(void);
void UnloadSystemwideIcons(void);
void UpdateSystemOptionsMenu(void);
void CleanUpMenus(void);
void EarlyResizeWindow(int, int, int, int);
void GetDataEntry(char*, char*, int, char*);
short DataEntryWindowHandler(struct tag_message&);
short EventWindowHandler(struct tag_message&);
short TrueFalseDialogHandler(struct tag_message&);
// HoMM1 town-name lookup by town id (retail 0x00455aaf); the inline
// game::GetTown narrows the id, hence retail's movsx after jmp $+5.
char* GetTownName(int);
void ReceiveRemotePlayerExit(signed char, signed char, signed char, signed char);
void ShutDown(char*);
void HandleRemoteDeadPlayerExit(int);
void CheckEndGame(int);
void HandleRemoteSuddenExit(void);
extern signed char gbRetreatWin;
extern signed char gbGameInitialized;
extern H1_ENUM_STORAGE(MainMenuControl, short) gGameCommand;
extern signed char gbCombatSurrender;
// The new-map builder raises this while it claims towns and mines.
extern int gbInNewGameSetup;
extern int bInShutDown;
void DeleteMainClasses(void);
extern class highScoreManager* gpHighScoreManager;
void FileError(char*);
void MemError();
void GetMonsterCost(int, int* const);
// philAI::BuildHero charges this word-sized gold price.
extern short gHeroGoldCost;
void PopNetBox(char*);
// NormalDialog's x/y: AUTO_POSITION lets it place the window (the adventure
// screen's NORMAL_DIALOG_ADVENTURE_X or centred; y centred up to
// NORMAL_DIALOG_MAX_TOP).
H1_ENUM_CONST_BEGIN(NormalDialogPosition)
    NORMAL_DIALOG_AUTO_POSITION = -1
H1_ENUM_CONST_END(NormalDialogPosition)

// Buka 2.1 KBDeclarations.h declares the same trailing defaults (HoMM1 has no
// timeout argument).
void NormalDialog(
    char*,
    H1_ENUM_PARAM(NormalDialogType, int),
    int = NORMAL_DIALOG_AUTO_POSITION,
    int = NORMAL_DIALOG_AUTO_POSITION,
    H1_ENUM_PARAM(NormalDialogResourceType, int) = NORMAL_DIALOG_NO_RESOURCE,
    int = 0,
    H1_ENUM_PARAM(NormalDialogResourceType, int) = NORMAL_DIALOG_NO_RESOURCE,
    int = 0,
    H1_ENUM_PARAM(NormalDialogOrText, int) = NORMAL_DIALOG_NO_OR_TEXT
);
extern char* cTownObjectNames[];
extern char* gSpellDesc[];
extern char* gSpellNames[];
// QuickInfo's name tables.
extern char* gTerrainNames[];
extern char* gResourceNames[];
extern char* gObjectNames[];
// KB's map-extra record count and sizes (Buka KBDeclarations).
extern int iMaxMapExtra;
extern int pwSizeOfMapExtra[];
// KB's adventure status-bar resource message and its menu, wait and victory
// screens.
void BVResMsg(char*, int, int);
short InitMenuHandler(struct tag_message&);
short WaitHandler(struct tag_message&);
void ShowCongrats(void);
void CongratsWait(void);
int AddScoreToHighScore(int, int, char*, char*);

// HoMM1 uses six-word graphics records; HoMM2 adds colorMouseCursor.
struct exeGfxConfig {
    int showMenu;
    int x;
    int y;
    int width;
    int height;
    int fullScreen;
};
// ReadPrefsFromFile reads 0x134 bytes at the owner base. The registry
// readers and writers name every persisted field except the 0x50 interval.
struct configStruct {
    int walkSpeed;
    int musicVolume;
    int soundVolume;
    int autosave;
    int showRoute;
    int blackoutComputer;
    exeGfxConfig gfx[2];
    int firstMapOffset;
    int currentMapOffset;
    char _pad_0x050[0x64];
    int cdOffset;
    int musicSource;
    int comPort[2];
    int baudRate[2];
    char modemInitString[100];
    int slowVideo;
};
struct tag_tilePoint {
    signed char x;
    signed char y;
    short frameOffset;
};
struct SPlayerExit {
    signed char player[7];
};
extern int gbComputeExtent;
extern int gbCurrArmyDrawn;
extern signed char gbIconClipOn;
extern int gbLimitToExtent;
extern int gbSaveBiggestExtent;
extern int giMaxExtentX;
extern int giMaxExtentY;
extern int giMinExtentX;
extern int giMinExtentY;
extern int giMonoIconSkip;
extern unsigned char gMonoColorMap[];
extern class inputManager *gpInputManager;
extern int giCurExe;
extern int giDebugLevel;
extern class palette* gpBufferPalette;
extern int gbColorMice;
extern int gbSpecialMouseMasks;
extern char gcDataPath[];
extern char gcSoundPath[];
extern int gbInDialog;
extern class palette* gPalette;
// Main: right-click help for the six adventure panel buttons, the typed
// cheat-digit sequence and the pending menu command.
extern char* cAdvMenuHelp[];
extern int gbAllBlack;
extern int gbNoBorder;
// Per hero type scouting radius used by TeleportTo.
extern signed char gHeroScoutRadius[];
extern unsigned char giCloudType[];
// GAME stores and reloads it as a dword (retail 0x4c7ca0).
extern int giCurWatchPlayer;
extern long giForceSwitchMusic;
extern int giMenuCommand;
extern short gMapX;
extern short gMapY;
// UpdBottomViewHero's per-creature mons32.icn frame width.
extern signed char gMons32Width[];
extern class searchArray* gpSearchArray;
// UpdateRadar's per-owner and per-terrain radar pixel colours.
extern short gRadarOwnerColor[];
extern short gRadarTerrainColor[];
// Route arrow frame by [next step][this step] path direction.
extern signed char gRouteFrame[][8];
// Damage multipliers for attack minus defense, -20..20 (0x00492470).
extern float gfBattleStat[];
extern short giSpellEffectFrame;
// Pow (impact) effect icons by effect (0x00491098).
extern char* gPowEffectNames[];
extern char* gArmySizeNames[6][2];
// New-game "King of the Hill" option; campaign scenarios preset it.
extern signed char gbIAmGreatest;
extern struct campaignScenario gCampaignScenarios[];
// Victory/defeat window texts (0x00493e48).
extern char* cBattleResults[];
// Combat help lines for the auto-combat, skip and other controls.
extern char* cCombatHelp[];
// Command help lines for CombatMessage(short) (0x00493b38).
extern char* cCombatMessage[];
// Spell-book hover help lines (0x00493a78).
extern char* cSpellHelp[];
// CheckHandleNet hands combat packets back while a battle is running.
extern signed char gbInCombat;
// Neighbour hex per combat hex and direction (0x004911c0), -1 off grid.
extern signed char gCombatAdjacency[45][6];
// The loaded combat effect icon's file id (0x004c6d64).
extern short gCurLoadedSpellFileId;
// ProcessCombatMsg records the hero casting from the combat screen.
extern int giCurGeneral;
// Area spells mark each stack once per cast: [side][army slot].
extern signed char gArmyEffected[COMBAT_SIDE_COUNT][ARMY_GROUP_SLOT_COUNT];
extern char* gDifficultyNames[];
extern int giMapDifficulty;
extern int giMapSize;
extern char gLastFilename[];
extern char gLastMapName[];
extern char* gMapSizeNames[];
extern char* cHeroScreen[];
extern char* gArtifactDesc[];
extern char* gClassNames[];
// Per-class sea mobility multiplier and level thresholds (retail 0x492038,
// 0x492598).
extern float gfClassNavigationMod[];
extern int giHeroScreenSrcIndex;
extern short gMinExpForLevel[][HERO_EXPERIENCE_LEVEL_TABLE_COUNT];
extern class hero* gpHVHero;
extern char* gStatDesc[];
extern char* gStatNames[];
extern class heroWindow* heroWin;
extern signed char giHighScoreRank;
int EarlySetup(void);
int GameUnsaved(void);
extern signed char gbFirstTimeThrough;
extern char gcAnimPath[];
extern char gcRegAppPath[];
extern char gcRegCDRomPath[];
extern long giCurWindowsStyleFlags;
extern struct SMenuEnableStatus gsMenuEnableStatus[];
extern struct WindowTextEntry gWinSetup[];
extern char* gWinSetupText[];
int HandleAppSpecificMenuCommands(int);
int oldmain(void);
void UpdateAppSpecificMenus(void*);
extern int bSpecialHideCursor;
extern int gArtifactBaseRV[];
extern signed char gbDrawSavedCursor;
extern signed char gcSpellAIFlags[];
extern signed char gDwellingType[4][6];
extern float gfSpellCastNumMod[];
// FightValueOfStack's primary-stat power curve, per-spell AI flags and
// values, spell-power duration scale and per-charge cast weights.
extern float gfStatPower[];
// DoAI: the single player the AI may run for, and the places each hero has
// already started from this turn.
extern signed char giLimitPlayer;
extern int giMineIncome[];
extern short giSpellAIValue[];
extern class armyGroup* gpMonGroup;
extern class philAI* gpPhilAI;
extern int gResourceBaseValue[];
// ValueOfBuyingHero: the hero class native to each town type.
extern signed char gTownHeroClass[];
extern int gUltArtifactAvgValue;
// GoodAdjacent skips cells whose adjacency byte carries the monster bit.
extern unsigned char mapExtra[MAP_CELL_GRID_SIZE][MAP_CELL_GRID_SIZE];
extern signed char gbGamePosToNetPos[];
// WaitForOtherPlayer stores the game position of net position zero here
// (0x004c6710). Declared ahead of giThisGamePos (0x004c74a0): only this order
// gives the host/this compares in advManager::Main, game::NextPlayer,
// PollRemote and HandleRemoteSuddenExit retail's load order.
extern int giHostGamePos;
extern int giThisGamePos;
extern int giThisNetPos;
extern signed char iMPBaseType;
signed char NetPosToGamePos(int);
signed char WaitForOtherPlayer(void);
// SeedPosition's seeding state.
extern int giSeedingValid;
// KB-band setup state (Buka X_GLOBAL.h): the direct-connect flag and the
// multiplayer game type. They stay out of X_GLOBAL.h while GAME and KB still
// use these names for other retail objects (see the aliases there).
extern signed char gbDirectConnect;
extern signed char iMPExtendedType;
extern int gbInSmacker;
extern class smackManager* gpSmackManager;
// Spells taught per mage-guild level (retail 0x492514).
extern signed char gMageGuildSpellCount[];
extern char* cCastleInfo[];
extern char* cTownCommand[];
extern struct TownBuildingExtent gTownBuildingExtents[4][16];
// KB's tavern recruit dialog handler (retail 0x0045140e).
short RecruitHeroHandler(struct tag_message&);
extern signed char townTheme[];
extern int gbFullCombatScreenDrawn;
extern int gbLimitedCombatUpdatePalette;
extern int giScrollX;
extern int giScrollY;
// CheckEndGame's re-entry guard and last offered score, the creature
// alignment names (by type / 6) and the score labels.
extern signed char bInCheckEndGame;
extern int giScore;
// oldmain's re-entry guard and the intro, end-sequence and remote state it
// shares with the game screens.
extern signed char bKBDone;
extern short boatFrameFlip[];
// Combat ground tiles (0x00491058) and obstacle icons (0x00491078) per
// combat terrain.
extern char* cCombatGroundNames[];
extern char* cCombatObstacleNames[];
// Hero level names and the per-class primary-skill gain table.
extern char* cHeroLevel[];
extern char* cViewGeneralHelp[];
// Primary stat, morale and luck labels of the general's stats text, and the
// combat command help lines HandleViewGeneral shows (entries 1-5).
extern char* cViewGeneralLabels[];
extern int gAdvDisposeLevel;
extern char* gAlignmentNames[];
extern char* gAPanelHelp[];
// Army info strings: attack, defense, shots (combat), damage, hit points,
// speed, morale, luck, shots (adventure); then the speed names.
extern char* gArmyStatText[];
extern int gbEnlargeScreenBlit;
extern int gbHeartbeatSeen;
// Setup screens: the setup-dialog flag kbwin's menus check and the
// right-click help of each setup dialog.
extern int gbInSetupDialog;
// The other side's ready flag and the heartbeat-seen flag (REMOTE).
extern int gbRemoteReady;
extern signed char gbSkipIntro;
extern signed char gbWaitForRemoteReceive;
extern char* gCampaignScenarioNames[];
extern char* gCampaignScenarioText[];
extern signed char gCampaignSideCrests[][2];
extern char* gCampaignSideNames[];
extern char* gCampaignWinTexts[];
extern char* gColorNames[];
extern char* gCPanelHelp[];
extern short gCrestHeroClass[];
// NewMap: the town type of each crest and the types already given to the
// first four random towns; the starting hero class of each campaign crest and
// of each town type, each hero class's sight radius, the starting resources by
// difficulty, the spell attribute bits and mage-guild pool by spell level, the
// vision radius a claimed town grants and the mines placed per type.
extern short gCrestTownTypes[];
extern char gcWinText[];
// Event texts, player colour names and the wandering-monster group of the
// current encounter.
extern char* gEventText[];
// Scenario-info labels: human seat handicap and map difficulty names.
extern char* gHandicapNames[];
// Default hero names (name, short name) restored with the original data, and
// a per-cell scratch map cleared on every load.
extern char* gHeroNames[][2];
extern signed char gHeroSkillBonus[][9][HERO_PRIMARY_STAT_COUNT];
extern char* gHumanPlayerTypeNames[];
// Campaign: the lord picked on stpcmpgn.bin (1-4; PickLoadGame filters *.CGM
// on it), scenario titles and briefings, two crest bytes per side (the first
// is the human player's), side names and win texts, and the town a campaign
// map renames at a fixed position (x, y, then the name).
extern signed char giCampaignChoice;
extern signed char giMonthType;
extern signed char giMonthTypeExtra;
extern char* gInitMenuHelp[];
extern signed char giVisRangeTown;
// Calendar specials: week/month type and the featured creature or name.
extern signed char giWeekType;
extern signed char giWeekTypeExtra;
extern char* gLuckInfoText[];
extern char* gLuckText[];
extern signed char gMageGuildSpellPool[4][8];
extern char* gMapDifficultyNames[];
extern char* gMonthNames[];
extern char* gMoraleInfoText[];
// Morale and luck names, indexed from -3, and their info-window texts.
extern char* gMoraleText[];
// New-game screen: right-click help and the human/computer seat labels.
extern char* gNewGameHelp[];
// New-turn texts: days-left and last-day warnings, then the month/week
// banners and names.
extern char* gNewTurnText[];
// Kingdom overview text: the dated title, then Dragon City and Lighthouse.
extern char* gOverviewText[];
extern char* gPlayerTypeNames[];
extern char* gScoreLabels[];
extern char* gSetupBaudHelp[];
extern char* gSetupCampaignGameHelp[];
extern char* gSetupComPortHelp[];
extern char* gSetupDCBaudHelp[];
extern char* gSetupDCComPortHelp[];
extern char* gSetupDCGameHelp[];
extern char* gSetupGameHelp[];
extern char* gSetupHotSeatGameHelp[];
extern char* gSetupModemGameHelp[];
extern char* gSetupMultiPlayerGameHelp[];
extern char* gSetupNetworkGameHelp[];
extern char* gSpeedText[];
extern int gStartingResources[][7];
extern char* gTownNames[];
extern char* gWeekNames[];
// Hero frame flips for the horse and boat walk cycles.
extern short horseFrameFlip[];
extern char* musicQualityText[];
// Adventure control panel: option labels, then the control-panel and
// adventure-panel help lines.
extern char* onOffText[];
extern char* walkSpeedText[];

#endif

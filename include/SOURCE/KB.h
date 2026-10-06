#ifndef HOMM1_SOURCE_KB_H
#define HOMM1_SOURCE_KB_H

#include <SOURCE/armyGroup.h>
#include <SOURCE/combatTypes.h>
#include <SOURCE/dialogTypes.h>
#include <SOURCE/FINDPATH.h>
#include <SOURCE/hero.h>
#include <SOURCE/mapCell.h>
#include <SOURCE/terrainTypes.h>

enum SampleWaitConstant {
    SAMPLE_WAIT_DEFAULT = -1
};

enum MainMenuControl {
    MAIN_MENU_NO_COMMAND = -1,
    MAIN_MENU_NEW_GAME = 1,
    MAIN_MENU_LOAD_GAME = 2,
    MAIN_MENU_QUIT = 4,
    MAIN_MENU_HIGH_SCORES = 5,
    MAIN_MENU_CREDITS = 6,
    MAIN_MENU_LAST = MAIN_MENU_CREDITS
};

extern i8 gInPollSound;
extern i8 gbNoSound;
extern i8 gShowHighScore;
extern i8 gHeroWindShowing;
extern i8 gOverviewShowing;
extern i8 giHighScoreType;
extern i8 giTerrainCost[FINDPATH_TERRAIN_COUNT][FINDPATH_STEP_COST_COUNT];
extern i8 giGroundToTerrain[];
#define CELL_TERRAIN(cell) (giGroundToTerrain[(cell)->m_tileIndex])
extern i32 bShowIt;
extern char gText[];
extern char* gArmyNames[];
extern char* gArmyNamesPlural[];
#define CREATURE_DISPLAY_NAME(type, count) ((count) > 1 ? gArmyNamesPlural[type] : gArmyNames[type])
extern struct tag_monsterInfo gMonsterDatabase[];
extern i32 gMinimized;
extern char* gMemoryErrorTitle;
extern char* gMemoryRequirements;
extern char* gExtendedMemoryUnits;
extern char* gConventionalMemoryUnits;
extern i32 gRequiredExtendedMemory;
extern i32 gRequiredConventionalMemory;
extern i32 gLoadingMonoIcon;
extern struct configStruct gConfig;
extern struct tag_tilePoint normalDirTable[];
extern char* DEFAULT_AGGREGATE_NAME;
extern class resourceManager* gpResourceManager;
extern class soundManager* gpSoundManager;
extern class heroWindowManager* gpWindowManager;
extern class mouseManager* gpMouseManager;
extern class heroWindow* gNormalDialogWindow;
extern class advManager* gpAdvManager;
extern i8 gbThisNetHumanPlayer[];
extern class townManager* gpTownManager;
extern class combatManager* gpCombatManager;
extern class executive* gpExec;
extern class game* gpGame;
extern i32 gHighMemBuffer;
extern i32 giBottomViewOverride;
extern i32 giBottomViewOverrideEndTime;
extern i32 giBottomViewResource;
extern i32 giBottomViewResourceQty;
extern char gcBottomViewText[];
extern void* hmnuAdv;
extern void* hmnuDflt;
extern void* hmnuCmbt;
extern void* hmnuTown;
extern i32 gHeroMoving;
extern i32 gRemoteOn;
extern class heroWindow* DataEntryWin;
extern char* cDEDest;
extern i32 iDEMaxLen;
extern i8 bDataEntryTime;
extern i8 giWaitType;
extern i8 gbFunctionComplete;
extern char* gArtifactNames[];
extern char* gNeutralBuildingNames[];
extern char* gDwellingNames[];
extern char* gNeutralBuildingDescriptions[];
extern char* gDwellingDescriptions[];
extern u16 gDwellingRequirements[];
extern i32 gMageBuildingCosts[][7];
extern i32 gNeutralBuildingCosts[][7];
extern i32 gDwellingCosts[][7];
extern i32 gMageBaseResourceValues[];
extern i32 gNeutralBaseResourceValues[];
extern i32 gDwellingBaseResourceValues[];
extern char cNetBoxLine[][60];
enum MapExtraConstant {
    MAP_EXTRA_FIRST_RECORD = 1,
    MAP_EXTRA_RECORD_CAPACITY = 255
};
extern void* ppMapExtra[];
extern class icon* gBuyBuildIcons;
extern class icon* gSystemIcons;
extern class font* bigFont;
extern class font* smallFont;
extern i16 gScoreMon[][2];
extern i16 gScoreCampaignMon[][2];
extern char* gCombatFxNames[];
extern class icon* gCurLoadedSpellIcon;

char* GetMonsterName(i32 monster);
struct SAMPLE2 LoadPlaySample(char* name);
void WaitEndSample(struct SAMPLE2 s, i32 waitTime);
extern struct SAMPLE2 NULL_SAMPLE2;
extern i32 glTimers[];
enum GlobalTimerConstant {
    GLOBAL_TIMER_COUNT = 6,
    GLOBAL_BUTTON_REPEAT_TIMER_SLOT = 2,
    GLOBAL_MUSIC_FADE_TIMER_SLOT = 4,
    GLOBAL_POLL_SOUND_TIMER_SLOT = 5
};
void EarlyShutDownSystem();
void QuickViewWait();
i8 CanBuild(class town* t, i16 building);
i8 CanBuy(class town* t, i16 type);
extern "C" void PollSound();
void ForcePollSound();
char toupper(char character);
i16 NullHandler(struct tag_message&);
char* GetBuildingName(i32 race, i16 building);
void GetBuildingCost(i32 race, i16 building, i32* const destination, i32 mageLevel);
char* GetMonsterName(i32 monster);
i32 GetBuildingBaseResourceValue(i32 race, i32 building, i32 level);
void AddNetBoxLine(char* text);
void GOut(char* text);
extern i32 giShowIntro;
extern i8 giScreenScroll;
extern i32 gbBlackoutPlayer;
extern char gMapName[];
extern char gFullMapName[];
extern char gMapDescription[];
extern char cAggPathName[];
extern i32 giNumHumanPlayers;
extern i32 gbHumanPlayer[];
void InitMainClasses(void);
void InitVars(void);
i32 InterpretCommandLine(void);
void ClearMapExtra(void);
i16 GetMonType(i32 score, i32 highScoreType);
i32 MemSize(i32);
i8 CheckMem(void);
i32 IsCDDrive(i32 driveIndex);
void LoadSystemwideIcons(void);
void UnloadSystemwideIcons(void);
void UpdateSystemOptionsMenu(void);
void CleanUpMenus(void);
void EarlyResizeWindow(i32, i32, i32, i32);
void GetDataEntry(char* prompt, char* destination, i32 maximumLength, char* initialText);
i16 DataEntryWindowHandler(struct tag_message& message);
i16 EventWindowHandler(struct tag_message& message);
i16 TrueFalseDialogHandler(struct tag_message& message);
char* GetTownName(i32 i);
void ReceiveRemotePlayerExit(i8 position, i8, i8 eliminated, i8 timedOut);
void ShutDown(char* message);
void HandleRemoteDeadPlayerExit(i32 position);
void CheckEndGame(i32 forced);
void HandleRemoteSuddenExit(void);
extern i8 gbRetreatWin;
extern i8 gGameInitialized;
extern i16 gGameCommand;
extern i8 gbCombatSurrender;
extern i32 gbInNewGameSetup;
void DeleteMainClasses(void);
extern class highScoreManager* gpHighScoreManager;
void FileError(char* filename);
void MemError();
void GetMonsterCost(i32 monster, i32* const cost);
extern i16 gHeroGoldCost;
void PopNetBox(char* notice);
enum NormalDialogPosition {
    NORMAL_DIALOG_AUTO_POSITION = -1
};

void NormalDialog(
    char* text,
    i32 dialogType,
    i32 x = NORMAL_DIALOG_AUTO_POSITION,
    i32 y = NORMAL_DIALOG_AUTO_POSITION,
    i32 firstResourceType = NORMAL_DIALOG_NO_RESOURCE,
    i32 firstResourceValue = 0,
    i32 secondResourceType = NORMAL_DIALOG_NO_RESOURCE,
    i32 secondResourceValue = 0,
    i32 showOrText = NORMAL_DIALOG_NO_OR_TEXT
);
extern char* gTownObjectNames[];
extern char* gSpellDesc[];
extern char* gSpellNames[];
extern char* gTerrainNames[];
extern char* gResourceNames[];
extern char* gObjectNames[];
extern i32 iMaxMapExtra;
extern i32 pwSizeOfMapExtra[];
void BVResMsg(char* s, i32 res, i32 qty);
i16 InitMenuHandler(struct tag_message& message);
i16 WaitHandler(struct tag_message& message);
void ShowCongrats(void);
void CongratsWait(void);
i32 AddScoreToHighScore(i32 score, i32 standard, char*, char* scenarioName);

struct exeGfxConfig {
    i32 showMenu;
    i32 x;
    i32 y;
    i32 width;
    i32 height;
    i32 fullScreen;
};
struct configStruct {
    i32 walkSpeed;
    i32 musicVolume;
    i32 soundVolume;
    i32 autosave;
    i32 showRoute;
    i32 blackoutComputer;
    exeGfxConfig gfx[2];
    i32 firstMapOffset;
    i32 currentMapOffset;
    char _pad_0x050[0x64];
    i32 cdOffset;
    i32 musicSource;
    i32 comPort[2];
    i32 baudRate[2];
    char modemInitString[100];
    i32 slowVideo;
};
struct tag_tilePoint {
    i8 x;
    i8 y;
    i16 frameOffset;
};
struct SPlayerExit {
    i8 player[7];
};
extern i32 gComputeExtent;
extern i32 gCurrArmyDrawn;
extern i8 gbIconClipOn;
extern i32 gLimitToExtent;
extern i32 gSaveBiggestExtent;
extern i32 giMaxExtentX;
extern i32 giMaxExtentY;
extern i32 giMinExtentX;
extern i32 giMinExtentY;
extern i32 gMonoIconSkip;
extern u8 gMonoColorMap[];
extern class inputManager* gpInputManager;
extern i32 gCurExe;
extern i32 giDebugLevel;
extern class palette* gpBufferPalette;
extern i32 gColorMice;
extern i32 gSpecialMouseMasks;
extern char gDataPath[];
extern char gSoundPath[];
extern i32 gInDialog;
extern class palette* gPalette;
extern char* gAdvMenuHelp[];
extern i32 gAllBlack;
extern i32 gNoBorder;
extern i8 gHeroScoutRadius[];
extern u8 gCloudType[];
extern i32 giCurWatchPlayer;
extern i32 gForceSwitchMusic;
extern i32 gMenuCommand;
extern i16 gMapX;
extern i16 gMapY;
extern i8 gMons32Width[];
extern class searchArray* gpSearchArray;
extern i16 gRadarOwnerColor[];
extern i16 gRadarTerrainColor[];
extern i8 gRouteFrame[][8];
extern float gBattleStat[];
extern i16 gSpellEffectFrame;
extern char* gPowEffectNames[];
extern char* gArmySizeNames[6][2];
extern i8 gbIAmGreatest;
extern struct campaignScenario gCampaignScenarios[];
extern char* gBattleResults[];
extern char* gCombatHelp[];
extern char* gCombatMessage[];
extern char* gSpellHelp[];
extern i8 gInCombat;
extern i8 gCombatAdjacency[45][6];
extern i16 gCurLoadedSpellFileId;
extern i32 giCurGeneral;
extern i8 gArmyEffected[COMBAT_SIDE_COUNT][ARMY_GROUP_SLOT_COUNT];
extern char* gDifficultyNames[];
extern i32 gMapDifficulty;
extern i32 gMapSize;
extern char gLastFilename[];
extern char gLastMapName[];
extern char* gMapSizeNames[];
extern char* gHeroScreen[];
extern char* gArtifactDesc[];
extern char* gClassNames[];
extern float gClassNavigationMod[];
extern i32 giHeroScreenSrcIndex;
extern i16 gMinExpForLevel[][HERO_EXPERIENCE_LEVEL_TABLE_COUNT];
extern class hero* gHVHero;
extern char* gStatDesc[];
extern char* gStatNames[];
extern class heroWindow* heroWin;
extern i8 gHighScoreRank;
i32 EarlySetup(void);
i32 GameUnsaved(void);
extern i8 gFirstTimeThrough;
extern char gAnimPath[];
extern char gcRegAppPath[];
extern char gcRegCDRomPath[];
extern i32 giCurWindowsStyleFlags;
extern struct SMenuEnableStatus gMenuEnableStatus[];
extern struct WindowTextEntry gWinSetup[];
extern char* gWinSetupText[];
i32 HandleAppSpecificMenuCommands(i32 command);
i32 oldmain(void);
void UpdateAppSpecificMenus(void* hMenu);
extern i32 bSpecialHideCursor;
extern i32 gArtifactBaseRV[];
extern i8 gDrawSavedCursor;
extern i8 gSpellAIFlags[];
extern i8 gDwellingType[4][6];
extern float gSpellCastNumMod[];
extern float gStatPower[];
extern i8 giLimitPlayer;
extern i32 gMineIncome[];
extern i16 gSpellAIValue[];
extern class armyGroup* gpMonGroup;
extern class philAI* gpPhilAI;
extern i32 gResourceBaseValue[];
extern i8 gTownHeroClass[];
extern i32 gUltArtifactAvgValue;
extern u8 mapExtra[MAP_CELL_GRID_SIZE][MAP_CELL_GRID_SIZE];
extern i8 gbGamePosToNetPos[];
extern i32 giHostGamePos;
extern i32 giThisGamePos;
extern i32 giThisNetPos;
extern i8 iMPBaseType;
i8 NetPosToGamePos(i32 netPos);
i8 WaitForOtherPlayer(void);
extern i32 giSeedingValid;
extern i8 gDirectConnect;
extern i8 iMPExtendedType;
extern i32 gInSmacker;
extern class smackManager* gpSmackManager;
extern i8 gMageGuildSpellCount[];
extern char* gCastleInfo[];
extern char* gTownCommand[];
extern struct TownBuildingExtent gTownBuildingExtents[4][16];
i16 RecruitHeroHandler(struct tag_message& message);
extern i8 townTheme[];
extern i32 gFullCombatScreenDrawn;
extern i32 gLimitedCombatUpdatePalette;
extern i32 gScrollX;
extern i32 gScrollY;
extern i8 gInCheckEndGame;
extern i32 giScore;
extern i8 gKBDone;
extern i16 boatFrameFlip[];
extern char* gCombatGroundNames[];
extern char* gCombatObstacleNames[];
extern char* gHeroLevel[];
extern char* gViewGeneralHelp[];
extern char* gViewGeneralLabels[];
extern i32 gAdvDisposeLevel;
extern char* gAlignmentNames[];
extern char* gAPanelHelp[];
extern char* gArmyStatText[];
extern i32 gEnlargeScreenBlit;
extern i32 gHeartbeatSeen;
extern i32 gInSetupDialog;
extern i32 gRemoteReady;
extern i8 gSkipIntro;
extern i8 gbWaitForRemoteReceive;
extern char* gCampaignScenarioNames[];
extern char* gCampaignScenarioText[];
extern i8 gCampaignSideCrests[][2];
extern char* gCampaignSideNames[];
extern char* gCampaignWinTexts[];
extern char* gColorNames[];
extern char* gCPanelHelp[];
extern i16 gCrestHeroClass[];
extern i16 gCrestTownTypes[];
extern char gcWinText[];
extern char* gEventText[];
extern char* gHandicapNames[];
extern char* gHeroNames[][2];
extern i8 gHeroSkillBonus[][9][HERO_PRIMARY_STAT_COUNT];
extern char* gHumanPlayerTypeNames[];
extern i8 gCampaignChoice;
extern i8 giMonthType;
extern i8 giMonthTypeExtra;
extern char* gInitMenuHelp[];
extern i8 gVisRangeTown;
extern i8 giWeekType;
extern i8 giWeekTypeExtra;
extern char* gLuckInfoText[];
extern char* gLuckText[];
extern i8 gMageGuildSpellPool[4][8];
extern char* gMapDifficultyNames[];
extern char* gMonthNames[];
extern char* gMoraleInfoText[];
extern char* gMoraleText[];
extern char* gNewGameHelp[];
extern char* gNewTurnText[];
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
extern i32 gStartingResources[][7];
extern char* gTownNames[];
extern char* gWeekNames[];
extern i16 horseFrameFlip[];
extern char* musicQualityText[];
extern char* onOffText[];
extern char* walkSpeedText[];

enum AdvDisposeLevel {
    ADV_DISPOSE_NONE = 0,
    ADV_DISPOSE_PARTIAL = 1,
    ADV_DISPOSE_FULL = 2
};

enum ConfigExecutable {
    CONFIG_EXECUTABLE_GAME = 0,
    CONFIG_EXECUTABLE_EDITOR = 1,
    CONFIG_EXECUTABLE_COUNT = 2
};

#endif

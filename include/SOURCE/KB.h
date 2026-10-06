#ifndef HOMM1_SOURCE_KB_H
#define HOMM1_SOURCE_KB_H

#include <BASE/audioTypes.h>
#include <BASE/message.h>
#include <SOURCE/armyGroup.h>
#include <SOURCE/campaignTypes.h>
#include <SOURCE/combatTypes.h>
#include <SOURCE/cursorTypes.h>
#include <SOURCE/dialogTypes.h>
#include <SOURCE/fileRequester.h>
#include <SOURCE/FINDPATH.h>
#include <SOURCE/game.h>
#include <SOURCE/gameTypes.h>
#include <SOURCE/hero.h>
#include <SOURCE/highScoreManager.h>
#include <SOURCE/mapCell.h>
#include <SOURCE/REMOTE.h>
#include <SOURCE/resourceTypes.h>
#include <SOURCE/SETUP.h>
#include <SOURCE/terrainTypes.h>
#include <SOURCE/VIEW.h>

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
    MAIN_MENU_LAST = MAIN_MENU_CREDITS,
    // Not a menu button: the adventure map asks the main loop to reload the
    // quick save.
    MAIN_MENU_QUICK_LOAD = 0x40
};

                               extern b8 gInPollSound;
extern i8 gNoSound;
extern b8 gShowHighScore;
extern b8 gHeroWindShowing;
extern b8 gOverviewShowing;
extern i8 gHighScoreType;
extern i8 gTerrainCost[FINDPATH_TERRAIN_COUNT][FINDPATH_STEP_COST_COUNT];
#define CELL_TERRAIN(cell) (gGroundToTerrain[(cell)->m_tileIndex])
extern b32 gShowIt;
extern char gText[];
extern char* gArmyNames[CREATURE_COUNT];
extern char* gArmySpriteNames[CREATURE_COUNT];
extern char* gArmyNamesPlural[CREATURE_COUNT];
#define CREATURE_DISPLAY_NAME(type, count)                                                         \
    ((count) <= 1 ? gArmyNames[type] : gArmyNamesPlural[type])
extern struct tag_monsterInfo gMonsterDatabase[CREATURE_COUNT];
extern i32 gMinimized;
extern char* gMemoryErrorTitle;
extern char* gMemoryRequirements;
extern char* gExtendedMemoryUnits;
extern char* gConventionalMemoryUnits;
extern i32 gRequiredExtendedMemory;
extern i32 gRequiredConventionalMemory;
extern b32 gLoadingMonoIcon;
extern struct configStruct gConfig;
extern struct tag_tilePoint gNormalDirTable[];
extern char* gDefaultAggregateName;
extern class resourceManager* gResourceManager;
extern class heroWindowManager* gWindowManager;
extern class mouseManager* gMouseManager;
extern class heroWindow* gNormalDialogWindow;
extern class advManager* gAdvManager;
extern b8 gThisNetHumanPlayer[];
extern class townManager* gTownManager;
extern class combatManager* gCombatManager;
extern class executive* gExec;
extern class game* gGame;
extern i32 gHighMemBuffer;
extern i32 gBottomViewOverrideEndTime;
extern i32 gBottomViewResource;
extern i32 gBottomViewResourceQty;
extern char gBottomViewText[];
extern b32 gHeroMoving;
extern b32 gRemoteOn;
extern class heroWindow* gDataEntryWindow;
extern char* gDataEntryDest;
extern i32 gDataEntryMaxLen;
extern i8 gDataEntryTime;
extern i8 gWaitType;
extern b8 gFunctionComplete;
extern char* gArtifactNames[ARTIFACT_COUNT];
extern char* gNeutralBuildingNames[BUILDING_SLOT_NEUTRAL_COUNT];
extern char* gDwellingNames[];
extern char* gNeutralBuildingDescriptions[BUILDING_SLOT_NEUTRAL_COUNT];
extern char* gDwellingDescriptions[];
extern u16 gDwellingRequirements[];
extern i32 gMageBuildingCosts[][7];
extern i32 gNeutralBuildingCosts[BUILDING_SLOT_NEUTRAL_COUNT][7];
extern i32 gDwellingCosts[][7];
extern i32 gMageBaseResourceValues[];
extern i32 gNeutralBaseResourceValues[BUILDING_SLOT_NEUTRAL_COUNT];
extern i32 gDwellingBaseResourceValues[];
extern char gNetBoxLine[][60];
enum MapExtraConstant {
    MAP_EXTRA_FIRST_RECORD = 1,
    MAP_EXTRA_RECORD_CAPACITY = 255
};
extern void* gMapExtraBlocks[];
extern class icon* gBuyBuildIcons;
extern class icon* gSystemIcons;
extern class font* gBigFont;
extern class font* gSmallFont;
extern i16 gScoreMon[][2];
extern i16 gScoreCampaignMon[][2];
extern char* gCombatFxNames[COMBAT_EFFECT_COUNT];
extern class icon* gCurLoadedSpellIcon;

char* GetMonsterSingularName(i32 monster);
char* GetMonsterName(i32 monster);
class sample* LoadPlaySample(char* name);
enum TimerSlot {
    ADVENTURE_FRAME_TIMER_SLOT = 0,
    COMBAT_FRAME_TIMER_SLOT = 0,
    TOWN_FRAME_TIMER_SLOT = 0,
    VIEW_ARMY_TIMER_SLOT = 0,
    HIGH_SCORE_TIMER_SLOT = 0,
    NET_BOX_BLINK_TIMER_SLOT = 0,
    COMBAT_EFFECT_TIMER_SLOT = 1,
    CURSOR_TURN_TIMER_SLOT = 1,
    DELAY_TICKS_TIMER_SLOT = 1,
    GLOBAL_BUTTON_REPEAT_TIMER_SLOT = 2,
    GLOBAL_MUSIC_FADE_TIMER_SLOT = 4,
    GLOBAL_POLL_SOUND_TIMER_SLOT = 5,
    GLOBAL_TIMER_COUNT = 6
};
extern i32 gTimers[GLOBAL_TIMER_COUNT];
void EarlyShutDownSystem();
void QuickViewWait();
b8 CanBuild(class town* townPointer, i16 building);
b8 CanBuy(class town* townPointer, i16 building);
extern "C" void PollSound();
void ForcePollSound();
#ifndef HOMM1_EDITOR
char toupper(char character);
#endif
enum Cp1251CaseConstant {
    CYRILLIC_CASE_OFFSET = 0x20,
    CYRILLIC_CAPITAL_YO = 0xa8,
    CYRILLIC_SMALL_YO = 0xb8,
    CYRILLIC_CAPITAL_A = 0xc0,
    CYRILLIC_CAPITAL_YA = 0xdf,
    CYRILLIC_SMALL_A = 0xe0,
    CYRILLIC_SMALL_YA = 0xff
};

inline char CyrillicToUpper(char c) {
    if (static_cast<u8>(c) >= 'a' && static_cast<u8>(c) <= 'z')
        return static_cast<u8>(c) - CYRILLIC_CASE_OFFSET;
    if (static_cast<u8>(c) >= CYRILLIC_SMALL_A && static_cast<u8>(c) <= CYRILLIC_SMALL_YA)
        return static_cast<u8>(c) - CYRILLIC_CASE_OFFSET;
    if (static_cast<u8>(c) == CYRILLIC_SMALL_YO)
        return CYRILLIC_CAPITAL_YO;
    return c;
}

inline char CyrillicToLower(char c) {
    if (static_cast<u8>(c) >= 'A' && static_cast<u8>(c) <= 'Z')
        return static_cast<u8>(c) + CYRILLIC_CASE_OFFSET;
    if (static_cast<u8>(c) >= CYRILLIC_CAPITAL_A && static_cast<u8>(c) <= CYRILLIC_CAPITAL_YA)
        return static_cast<u8>(c) + CYRILLIC_CASE_OFFSET;
    if (static_cast<u8>(c) == CYRILLIC_CAPITAL_YO)
        return CYRILLIC_SMALL_YO;
    return c;
}
i16 NullHandler(struct tag_message& message);
char* GetBuildingName(
    i32 race,
    i16 building
);
void GetBuildingCost(
    i32 race,
    i16 building,
    i32* const destination,
    i32 mageLevel
);
char* GetMonsterName(i32 monster);
i32 GetBuildingBaseResourceValue(
    i32 race,
    i32 building,
    i32 level
);
void AddNetBoxLine(char* text);
void GOut(char* text);
extern i32 gShowIntro;
extern b8 gScreenScroll;
extern char gMapName[];
extern b32 gBlackoutPlayer;
extern char gFullMapName[];
extern char gMapDescription[];
extern char gAggPathName[];
extern i32 gNumHumanPlayers;
extern b32 gHumanPlayer[];
void InitMainClasses(void);
void InitVars(void);
b32 InterpretCommandLine(void);
void ClearMapExtra(void);
i16 GetMonType(i32 score, i32 highScoreType);
i32 MemSize(i32);
b8 CheckMem(void);
bool IsCDDrive(i32 driveIndex);
void LoadSystemwideIcons(void);
void UnloadSystemwideIcons(void);
void UpdateSystemOptionsMenu(void);
void CleanUpMenus(void);
void EarlyResizeWindow(i32 x, i32 y, i32 width, i32 height);
void GetDataEntry(char* prompt, char* destination, i32 maximumLength, char* initialText);
i16 DataEntryWindowHandler(struct tag_message& message);
i16 EventWindowHandler(struct tag_message& message);
i16 TrueFalseDialogHandler(struct tag_message& message);
char* GetTownName(i32 townIndex);
void ReceiveRemotePlayerExit(i8 position, i8 hadControl, b8 eliminated, b8 timedOut);
void ShutDown(char* message);
void HandleRemoteDeadPlayerExit(i32 position);
void CheckEndGame(b32 forceWin);
void HandleRemoteSuddenExit(void);
extern b8 gRetreatWin;
extern b8 gGameInitialized;
extern i16 gGameCommand;
extern b8 gCombatSurrender;
extern b32 gInNewGameSetup;
void DeleteMainClasses(void);
extern class highScoreManager* gHighScoreManager;
void FileError(char* filename);
void FormatAbbreviatedCount(char* text, i32 value, i32 thousandsFrom);
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
extern char* gSpellDesc[SPELL_COUNT];
extern char* gSpellNames[SPELL_COUNT];
extern char* gTerrainNames[TERRAIN_COUNT];
extern char* gResourceNames[RESOURCE_COUNT];
extern char* gMineNames[RESOURCE_COUNT];
extern char* gObjectNames[];
extern i32 gMaxMapExtra;
extern i32 gMapExtraSizes[];
void BVResMsg(char* text, i32 resourceType, i32 quantity);
i16 InitMenuHandler(struct tag_message& message);
i16 WaitHandler(struct tag_message& message);
void ShowCongrats(void);
void CongratsWait(void);
i32 AddScoreToHighScore(
    i32 score,
    i32 highScoreType,
    char*,
    char* scenarioName
);

struct exeGfxConfig {
    i32 showMenu;
    i32 x;
    i32 y;
    i32 width;
    i32 height;
    i32 fullScreen;
};
enum ConfigExecutable {
    CONFIG_EXECUTABLE_GAME = 0,
    CONFIG_EXECUTABLE_EDITOR = 1,
    CONFIG_EXECUTABLE_COUNT = 2
};

enum ConfigConnection {
    CONFIG_CONNECTION_MODEM = 0,
    CONFIG_CONNECTION_DIRECT = 1,
    CONFIG_CONNECTION_COUNT = 2
};

// gConfig.cheatMode: the adventure-map digit cheats are off, limited to the
// original map reveal, or extended with the edition's resource, town, hero,
// creature and artifact codes.
enum CheatMode {
    CHEAT_MODE_OFF = 0,
    CHEAT_MODE_ORIGINAL = 1,
    CHEAT_MODE_EXTENDED = 2
};
// gConfig.battleMessageFormat: the combat status bar forecasts the damage of
// an attack or shot, or keeps the classic texts with or without the
// edition's grammar fixes.
enum BattleMessageFormat {
    BATTLE_MESSAGE_FORECAST = 0,
    BATTLE_MESSAGE_CLASSIC_PLUS = 1,
    BATTLE_MESSAGE_CLASSIC = 2
};
struct configStruct {
    i32 walkSpeed;
    i32 musicVolume;
    i32 soundVolume;
    i32 autosave;
    i32 showRoute;
    i32 blackoutComputer;
    exeGfxConfig gfx[CONFIG_EXECUTABLE_COUNT];
    i32 firstMapOffset;
    i32 currentMapOffset;
    char _pad_0x050[0x64];
    i32 musicSource;
    i32 comPort[CONFIG_CONNECTION_COUNT];
    i32 baudRate[CONFIG_CONNECTION_COUNT];
    char modemInitString[100];
    // Edition options, stored beside the preferences above.
    i32 showEnemyMobility;
    i32 softRetreatSurrender;
    i32 slightlyHarderAI;
    i32 cheatMode;
    i32 originalCheatKeys;
    i32 losslessAudio;
    i32 playVideos;
    i32 battleMessageFormat;
};
#define CURRENT_GRAPHICS_CONFIG (gConfig.gfx[gCurExe])
struct tag_tilePoint {
    i8 x;
    i8 y;
    i16 frameOffset;
};
struct SPlayerExit {
    i8 player[7];
};
extern b32 gComputeExtent;
extern b32 gCurrArmyDrawn;
extern b8 gIconClipOn;
extern b32 gLimitToExtent;
extern b32 gSaveBiggestExtent;
extern i32 gMaxExtentX;
extern i32 gMaxExtentY;
extern i32 gMinExtentX;
extern i32 gMinExtentY;
extern i32 gMonoIconSkip;
extern u8 gMonoColorMap[];
extern class inputManager* gInputManager;
extern i32 gCurExe;
enum DebugLevel {
    DEBUG_LEVEL_NONE = 0,
    EVENTS_CELL_EDIT_DEBUG_LEVEL_MIN = 1,
    ADVENTURE_DEBUG_KEYS_LEVEL_MIN = 1,
    FILE_REQUESTER_DEBUG_ALLOW_PLAYER_MISMATCH_MIN = 2,
    GAME_DEBUG_LEVEL_ALL_HUMAN_MIN = 2,
    AI_DEBUG_LEVEL_STATUS_TEXT_MIN = 2,
    KBWIN_TRACE_DEBUG_LEVEL = 4,
    AI_DEBUG_LEVEL_EVENT = 5,
    AI_DEBUG_LEVEL_BATTLE = 9,
    MISC_FORCED_DEBUG_LEVEL = 9
};
extern i32 gDebugLevel;
extern class palette* gBufferPalette;
extern i32 gColorMice;
extern i32 gSpecialMouseMasks;
extern char gDataPath[];
extern char gGamePath[];
extern char* gArtifactEvent[];
extern char gMapPath[];
extern char gSoundPath[];
extern char gTracksPath[];
extern b32 gInDialog;
extern class palette* gPalette;
extern b32 gAllBlack;
extern b32 gNoBorder;
extern i8 gHeroScoutRadius[];
extern u8 gCloudType[];
extern i32 gCurWatchPlayer;
extern i32 gForceSwitchMusic;
extern i32 gMenuCommand;
extern i16 gMapX;
extern i16 gMapY;
extern i8 gMons32Width[CREATURE_COUNT];
extern class searchArray* gSearchArray;
extern i16 gRadarOwnerColor[];
extern i16 gRadarTerrainColor[24];
extern i8 gRouteFrame[][8];
extern float gBattleStat[];
extern i16 gSpellEffectFrame;
extern char* gPowEffectNames[];
extern char* gArmySizeNames[6][2];
extern i8 gIAmGreatest;
extern struct campaignScenario gCampaignScenarios[];
extern char* gSpellHelp[SPELL_HELP_COUNT];
extern b8 gInCombat;
extern i8 gCombatAdjacency[45][COMBAT_DIRECTION_ADJACENT_COUNT];
extern i16 gCurLoadedSpellFileId;

extern i32 gCurGeneral;
extern i8 gArmyEffected[COMBAT_SIDE_COUNT][ARMY_GROUP_SLOT_COUNT];
extern char* gDifficultyNames[DIFFICULTY_COUNT];
extern i32 gMapDifficulty;
extern i32 gMapSize;
extern char gLastFilename[FILE_REQUESTER_NAME_SIZE];
extern i8 gGroundToTerrain[];
extern char gLastMapName[];
extern char* gMapSizeNames[MAP_SIZE_COUNT];
extern char* gHeroScreen[HERO_TEXT_COUNT];
extern char* gArtifactDesc[ARTIFACT_COUNT];
extern char* gClassNames[];
extern float gClassNavigationMod[];
extern i32 gHeroScreenSrcIndex;
extern i16 gMinExpForLevel[][HERO_EXPERIENCE_LEVEL_TABLE_COUNT];
extern class hero* gInfoViewedHero;
extern char* gStatDesc[];
extern char* gStatNames[];
extern class heroWindow* gHeroScreenWindow;
extern i8 gHighScoreRank;
b32 EarlySetup(void);
b32 GameUnsaved(void);
extern b8 gFirstTimeThrough;
extern char gAnimPath[];
extern char gRegAppPath[];
extern char gRegCDRomPath[];
extern i32 gCurWindowsStyleFlags;
extern struct SMenuEnableStatus gMenuEnableStatus[];
extern struct WindowTextEntry gWinSetup[];
extern char* gWinSetupText[];
i32 HandleAppSpecificMenuCommands(i32 command);
i32 oldmain(void);
void UpdateAppSpecificMenus(void* menu);

extern b32 gSpecialHideCursor;
extern i32 gArtifactBaseRV[ARTIFACT_REGULAR_END];
extern b8 gDrawSavedCursor;
extern i8 gSpellAIFlags[SPELL_COUNT];
extern i8 gDwellingType[TOWN_TYPE_COUNT][6];
extern float gSpellCastNumMod[];
extern float gStatPower[];
enum StatCurveConstant {
    STAT_CURVE_OFFSET = 20,
    STAT_CURVE_LAST = 40,
    SPELL_CAST_COUNT_LAST = 20
};

extern i8 gLimitPlayer;
extern i32 gMineIncome[RESOURCE_COUNT];
extern i16 gSpellAIValue[SPELL_COUNT];
extern class armyGroup* gMonGroup;
extern class philAI* gPhilAI;
extern i32 gResourceBaseValue[RESOURCE_COUNT];
extern i8 gTownHeroClass[];
extern i32 gUltArtifactAvgValue;
extern u8 gMapExtra[MAP_CELL_GRID_SIZE][MAP_CELL_GRID_SIZE];
extern i8 gGamePosToNetPos[];

extern i32 gHostGamePos;
extern i32 gThisGamePos;
extern i32 gThisNetPos;
extern i8 gMapBaseType;
i8 NetPosToGamePos(i32 netPos);
b8 WaitForOtherPlayer(void);
extern b32 gSeedingValid;
extern i8 gDirectConnect;

extern i8 gMapExtendedType;
extern b32 gInSmacker;
extern i8 gMageGuildSpellCount[];
extern struct TownBuildingExtent gTownBuildingExtents[TOWN_TYPE_COUNT][BUILDING_SLOT_CAPACITY];
i16 RecruitHeroHandler(struct tag_message& message);
extern i8 gTownTheme[TOWN_TYPE_COUNT];
extern b32 gFullCombatScreenDrawn;
extern b32 gLimitedCombatUpdatePalette;
extern i32 gScrollX;
extern i32 gScrollY;
extern b8 gInCheckEndGame;
extern i32 gScore;
extern b8 gKBDone;
extern i16 gBoatFrameFlip[];
extern char* gCombatGroundNames[TERRAIN_COUNT];
extern char* gCombatObstacleNames[TERRAIN_COUNT];
extern char* gHeroLevel[HERO_LEVEL_TEXT_COUNT];
extern char* gViewGeneralHelp[GENERAL_HOVER_HELP_COUNT];
extern char* gViewGeneralLabels[];
extern char* gAlignmentNames[];
extern char* gAPanelHelp[];
extern char* gArmyStatText[];
extern b32 gEnlargeScreenBlit;
extern b32 gHeartbeatSeen;
extern b32 gInSetupDialog;
extern b32 gRemoteReady;
extern b8 gSkipIntro;
extern b8 gWaitForRemoteReceive;
extern char* gCampaignScenarioNames[];
extern char* gCampaignScenarioText[];
extern i8 gCampaignSideCrests[][2];
extern char* gCampaignSideNames[];
extern char* gCampaignWinTexts[];
extern char* gColorNames[PLAYER_COLOR_COUNT];
extern i16 gCrestHeroClass[PLAYER_COLOR_COUNT];
extern i16 gCrestTownTypes[PLAYER_COLOR_COUNT];
extern char gWinText[];
extern char* gHandicapNames[];
extern char* gHeroNames[][2];
extern char* gHeroNamesAccusative[];
extern char* gHeroNamesGenitive[];
extern char* gClassNamesAccusative[];
extern char* gArmyNamesMoved[];
extern i8 gHeroSkillBonus[4][9][HERO_PRIMARY_STAT_COUNT];
extern char* gHumanPlayerTypeNames[];

extern i8 gCampaignChoice;
extern i8 gMonthType;
extern i8 gMonthTypeExtra;
extern i8 gVisRangeTown;

extern i8 gWeekType;
extern i8 gWeekTypeExtra;
extern char* gLuckText[];
extern i8 gMageGuildSpellPool[4][8];
extern char* gMapDifficultyNames[MAP_DIFFICULTY_COUNT];
extern char* gMonthNames[];
extern char* gMoraleText[];
extern char* gNewGameHelp[NEW_GAME_HELP_COUNT];
extern char* gNewTurnText[];
extern char* gOverviewText[];
extern char* gPlayerTypeNames[];
extern char* gScoreLabels[];
extern char* gSetupBaudHelp[SETUP_BAUD_HELP_COUNT];
extern char* gSetupCampaignGameHelp[SETUP_CAMPAIGN_HELP_COUNT];
extern char* gSetupComPortHelp[SETUP_COM_PORT_HELP_COUNT];
extern char* gSetupDCBaudHelp[SETUP_BAUD_HELP_COUNT];
extern char* gSetupDCComPortHelp[SETUP_COM_PORT_HELP_COUNT];
extern char* gSetupDCGameHelp[SETUP_MODEM_HELP_COUNT];
extern char* gSetupGameHelp[SETUP_GAME_HELP_COUNT];
extern char* gSetupHotSeatGameHelp[SETUP_HOT_SEAT_HELP_COUNT];
extern char* gSetupModemGameHelp[SETUP_MODEM_HELP_COUNT];
extern char* gSetupMultiPlayerGameHelp[SETUP_MULTIPLAYER_HELP_COUNT];
extern char* gSetupNetworkGameHelp[SETUP_NETWORK_HELP_COUNT];
extern char* gSpeedText[];
extern i32 gStartingResources[DIFFICULTY_COUNT][RESOURCE_COUNT];
extern char* gTownNames[];
extern char* gWeekNames[];
extern i16 gHorseFrameFlip[];
extern char* gMusicQualityText[];
extern char* gOnOffText[];
extern char* gWalkSpeedText[WALK_SPEED_COUNT];

enum AdvDisposeLevel {
    ADV_DISPOSE_NONE = 0,
    ADV_DISPOSE_PARTIAL = 1,
    ADV_DISPOSE_OBJECT_ICONS_LAST = ADV_DISPOSE_PARTIAL,
    ADV_DISPOSE_FULL = 2
};
extern i32 gAdvDisposeLevel;

enum MainMenuHelp {
    MAIN_MENU_HELP_NONE = -1,
    MAIN_MENU_HELP_FIRST = 0,
    MAIN_MENU_HELP_NEW_GAME = 0,
    MAIN_MENU_HELP_LOAD_GAME = 1,
    MAIN_MENU_HELP_HIGH_SCORES = 2,
    MAIN_MENU_HELP_CREDITS = 3,
    MAIN_MENU_HELP_QUIT = 4,
    MAIN_MENU_HELP_COUNT = 5
};
extern char* gInitMenuHelp[MAIN_MENU_HELP_COUNT];

enum GameEndSequence {
    GAME_END_LOST = 0,
    GAME_END_WON = 1,
    GAME_END_CAMPAIGN_COMPLETE = 2,
    GAME_END_SEQUENCE_COUNT = 3
};
extern i32 gEndSequence;

enum NetPositionConstant {
    NET_POSITION_NONE = -1,
    NET_POSITION_HOST = 0,
    NET_POSITION_FIRST_GUEST = 1,
    NET_GAME_POSITION_HOST = 0,
    NET_GAME_POSITION_GUEST = 1
};

enum CheckEndGameConstant {
    END_GAME_NO_GRACE_PERIOD = -1,
    END_GAME_GRACE_DAYS = CALENDAR_DAYS_PER_WEEK
};

enum MoraleInfoText {
    MORALE_INFO_GOOD = 0,
    MORALE_INFO_NEUTRAL = 1,
    MORALE_INFO_BAD = 2,
    MORALE_INFO_HEADER = 3,
    MORALE_INFO_KNIGHT = 4,
    MORALE_INFO_ALL_TROOPS = 5,
    MORALE_INFO_THREE_ALIGNMENTS = 6,
    MORALE_INFO_FOUR_ALIGNMENTS = 7,
    MORALE_INFO_MEDAL_OF_VALOR = 8,
    MORALE_INFO_MEDAL_OF_COURAGE = 9,
    MORALE_INFO_MEDAL_OF_HONOR = 10,
    MORALE_INFO_MEDAL_OF_DISTINCTION = 11,
    MORALE_INFO_FIZBIN = 12,
    MORALE_INFO_BUOY = 13,
    MORALE_INFO_OASIS = 14,
    MORALE_INFO_STATUE = 15,
    MORALE_INFO_GRAVEYARD = 16,
    MORALE_INFO_SHIPWRECK = 17,
    MORALE_INFO_COWARDICE = 18,
    MORALE_INFO_NONE = 19,
    MORALE_INFO_FIVE_ALIGNMENTS = 20,
    MORALE_INFO_COUNT = 21
};
extern char* gMoraleInfoText[MORALE_INFO_COUNT];

enum LuckInfoText {
    LUCK_INFO_GOOD = 0,
    LUCK_INFO_NEUTRAL = 1,
    LUCK_INFO_BAD = 2,
    LUCK_INFO_HEADER = 3,
    LUCK_INFO_RABBITS_FOOT = 4,
    LUCK_INFO_HORSESHOE = 5,
    LUCK_INFO_LUCKY_COIN = 6,
    LUCK_INFO_CLOVER = 7,
    LUCK_INFO_FAERIE_RING = 8,
    LUCK_INFO_FOUNTAIN = 9,
    LUCK_INFO_NONE = 10,
    LUCK_INFO_FIZBIN = 11,
    LUCK_INFO_COUNT = 12
};
extern char* gLuckInfoText[LUCK_INFO_COUNT];

enum ScoreMonsterConstant {
    SCORE_MONSTER_COUNT = 28,
    SCORE_MONSTER_THRESHOLD = 0,
    SCORE_MONSTER_TYPE = 1
};

enum NetBoxLineSlot {
    NET_BOX_SLOT_PREVIOUS = 0,
    NET_BOX_SLOT_LATEST = 1,
    NET_BOX_SLOT_COUNT = 2
};

enum NetBoxControl {
NET_BOX_LINE_PREVIOUS = 1, NET_BOX_LINE_LATEST = 2,
                           NET_BOX_INPUT = 3 };

                           enum NetBoxConstant {
    NET_BOX_BLINK_DELAY = 360
};

enum CongratsControl {
CONGRATS_TITLE = 100, CONGRATS_SCORE_LABEL_FIRST = 101, CONGRATS_DAYS = 106,
                      CONGRATS_BASE_SCORE = 107, CONGRATS_DIFFICULTY = 108,
                      CONGRATS_FINAL_SCORE = 109,
                      CONGRATS_RATING = 110 };

                          enum CongratsConstant {
    CONGRATS_SCORE_LABEL_COUNT = 5
};

enum DataEntryStep {
    DATA_ENTRY_STEP_FOCUS = 0,
    DATA_ENTRY_STEP_READ = 1
};

enum DataEntryControl {
DATA_ENTRY_PROMPT = 1, DATA_ENTRY_TEXT = 10 };

#endif

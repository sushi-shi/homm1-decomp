#ifndef HOMM1_SOURCE_KB_H
#define HOMM1_SOURCE_KB_H

#include <Domains.h>
#include <SOURCE/armyGroup.h>
#include <SOURCE/combatTypes.h>
#include <SOURCE/dialogTypes.h>
#include <SOURCE/FINDPATH.h>
#include <SOURCE/gameTypes.h>
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

extern i8 gInPollSound;
extern i8 gbNoSound;
extern i8 gShowHighScore;
// HeroView and the kingdom overview raise these while their screens are up;
// NormalDialog only parks over the adventure map when neither is showing.
extern i8 gHeroWindShowing;
extern i8 gOverviewShowing;
extern i8 giHighScoreType;
extern i8 giTerrainCost[FINDPATH_TERRAIN_COUNT][FINDPATH_STEP_COST_COUNT];
// Cell tile index -> terrain type; IsMobile reads it zero-extended.
extern H1_ENUM_STORAGE(TerrainType, i8) giGroundToTerrain[];
// The terrain type under a map cell.
#define CELL_TERRAIN(cell) (giGroundToTerrain[(cell)->m_tileIndex])
extern i32 bShowIt;
extern char gText[];
extern char* gArmyNames[];
// Locale-independent resource stems; display names stay in the catalog.
extern char* gArmySpriteNames[28];
extern char* gArmyNamesPlural[];
// A creature's name, singular for counts at most one.
#define CREATURE_DISPLAY_NAME(type, count)                                                         \
    ((count) <= 1 ? gArmyNames[type] : gArmyNamesPlural[type])
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
// Retail DoDimensionDoor walks gpSearchArray paths through this delta table.
extern struct tag_tilePoint normalDirTable[];
extern char* DEFAULT_AGGREGATE_NAME;
extern class resourceManager* gpResourceManager;
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
extern i32 gHeroMoving;
extern i32 gRemoteOn;
extern class heroWindow* DataEntryWin;
extern char* cDEDest;
extern i32 iDEMaxLen;
extern i8 bDataEntryTime;
extern H1_ENUM_STORAGE(DialogWaitType, i8) giWaitType;
extern i8 gbFunctionComplete;
// Artifact names (0x00492e60).
extern char* gArtifactNames[];
extern char* gNeutralBuildingNames[];
extern char* gDwellingNames[];
// BuyBuild's building descriptions (0x00493c90, 0x00493720) and per-dwelling
// prerequisite building masks (0x00491880); CanBuild reads six masks per
// faction.
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
extern i16 gScoreMon[][2];
extern i16 gScoreCampaignMon[][2];
// Combat effect icon files by effect (0x00490ef0) and the one loaded effect
// icon (0x004c709c) army draws and PowEffect share.
extern char* gCombatFxNames[];
extern class icon* gCurLoadedSpellIcon;

// HoMM1 KB name table accessor (retail 0x004516bf).
char* GetMonsterSingularName(i32 monster);
char* GetMonsterName(i32 monster);
class sample* LoadPlaySample(char* name);
// glTimers slots: each entry is a KBTickCount() deadline that DelayTil waits
// for or a loop compares against. Slots 2, 4 and 5 are global; slots 0 and
// 1 are the clocks of whichever screen runs, so each owner's role name is an
// alias of its slot.
// The table ends at giScore (0x004c6a98), six slots.
H1_ENUM_BEGIN(TimerSlot)
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
H1_ENUM_END(TimerSlot)
extern H1_ENUM_ARRAY(i32, glTimers, TimerSlot, GLOBAL_TIMER_COUNT);
void EarlyShutDownSystem();
void QuickViewWait();
i8 CanBuild(class town* t, i16 building);
i8 CanBuy(class town* t, i16 type);
extern "C" void PollSound();
void ForcePollSound();
char toupper(char character);
H1_ENUM_CONST_BEGIN(Cp1251CaseConstant)
    CYRILLIC_CASE_OFFSET = 0x20,
    CYRILLIC_CAPITAL_YO = 0xa8,
    CYRILLIC_SMALL_YO = 0xb8,
    CYRILLIC_CAPITAL_A = 0xc0,
    CYRILLIC_CAPITAL_YA = 0xdf,
    CYRILLIC_SMALL_A = 0xe0,
    CYRILLIC_SMALL_YA = 0xff
H1_ENUM_CONST_END(Cp1251CaseConstant)

// Buka combat messages fold ASCII and the Russian CP1251 alphabet, including
// the separate Yo pair. Other codepage characters retain their original byte.
inline char CyrillicToUpper(char c) {
    if (static_cast<u8>(c) >= 'a' && static_cast<u8>(c) <= 'z')
        return static_cast<u8>(c) - CYRILLIC_CASE_OFFSET;
    if (static_cast<u8>(c) >= CYRILLIC_SMALL_A && static_cast<u8>(c) <= CYRILLIC_SMALL_YA)
        return static_cast<u8>(c) - CYRILLIC_CASE_OFFSET;
    if (static_cast<u8>(c) == CYRILLIC_SMALL_YO)
        return static_cast<char>(CYRILLIC_CAPITAL_YO);
    return c;
}

inline char CyrillicToLower(char c) {
    if (static_cast<u8>(c) >= 'A' && static_cast<u8>(c) <= 'Z')
        return static_cast<u8>(c) + CYRILLIC_CASE_OFFSET;
    if (static_cast<u8>(c) >= CYRILLIC_CAPITAL_A && static_cast<u8>(c) <= CYRILLIC_CAPITAL_YA)
        return static_cast<u8>(c) + CYRILLIC_CASE_OFFSET;
    if (static_cast<u8>(c) == CYRILLIC_CAPITAL_YO)
        return static_cast<char>(CYRILLIC_SMALL_YO);
    return c;
}
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
bool IsCDDrive(i32 driveIndex);
void LoadSystemwideIcons(void);
void UnloadSystemwideIcons(void);
void UpdateSystemOptionsMenu(void);
void CleanUpMenus(void);
void EarlyResizeWindow(i32, i32, i32, i32);
void GetDataEntry(char* prompt, char* destination, i32 maximumLength, char* initialText);
i16 DataEntryWindowHandler(struct tag_message& message);
i16 EventWindowHandler(struct tag_message& message);
i16 TrueFalseDialogHandler(struct tag_message& message);
// Town-name lookup by town id (retail 0x00455aaf); the inline game::GetTown
// narrows the id.
char* GetTownName(i32 i);
void ReceiveRemotePlayerExit(i8 position, i8, i8 eliminated, i8 timedOut);
void ShutDown(char* message);
void HandleRemoteDeadPlayerExit(i32 position);
void CheckEndGame(i32 forced);
void HandleRemoteSuddenExit(void);
extern i8 gbRetreatWin;
extern i8 gGameInitialized;
extern H1_ENUM_STORAGE(MainMenuControl, i16) gGameCommand;
extern i8 gbCombatSurrender;
// The new-map builder raises this while it claims towns and mines.
extern i32 gbInNewGameSetup;
void DeleteMainClasses(void);
extern class highScoreManager* gpHighScoreManager;
void FileError(char* filename);
void MemError();
void GetMonsterCost(i32 monster, i32* const cost);
// philAI::BuildHero charges this word-sized gold price.
extern i16 gHeroGoldCost;
void PopNetBox(char* notice);
// NormalDialog's x/y: AUTO_POSITION lets it place the window (the adventure
// screen's NORMAL_DIALOG_ADVENTURE_X or centred; y centred up to
// NORMAL_DIALOG_MAX_TOP).
H1_ENUM_CONST_BEGIN(NormalDialogPosition)
    NORMAL_DIALOG_AUTO_POSITION = -1
H1_ENUM_CONST_END(NormalDialogPosition)

void NormalDialog(
    char* text,
    H1_ENUM_PARAM(NormalDialogType, i32) dialogType,
    i32 x = NORMAL_DIALOG_AUTO_POSITION,
    i32 y = NORMAL_DIALOG_AUTO_POSITION,
    H1_ENUM_PARAM(NormalDialogResourceType, i32) firstResourceType = NORMAL_DIALOG_NO_RESOURCE,
    i32 firstResourceValue = 0,
    H1_ENUM_PARAM(NormalDialogResourceType, i32) secondResourceType = NORMAL_DIALOG_NO_RESOURCE,
    i32 secondResourceValue = 0,
    H1_ENUM_PARAM(NormalDialogOrText, i32) showOrText = NORMAL_DIALOG_NO_OR_TEXT
);
extern char* gTownObjectNames[];
extern char* gSpellDesc[];
extern char* gSpellNames[];
// QuickInfo's name tables.
extern char* gTerrainNames[];
extern char* gResourceNames[];
extern char* gMineNames[];
extern char* gObjectNames[];
// KB's map-extra record count and sizes.
extern i32 iMaxMapExtra;
extern i32 pwSizeOfMapExtra[];
// KB's adventure status-bar resource message and its menu, wait and victory
// screens.
void BVResMsg(char* s, i32 res, i32 qty);
i16 InitMenuHandler(struct tag_message& message);
i16 WaitHandler(struct tag_message& message);
void ShowCongrats(void);
void CongratsWait(void);
i32 AddScoreToHighScore(i32 score, i32 standard, char*, char* scenarioName);

// Graphics records are six words.
struct exeGfxConfig {
    i32 showMenu;
    i32 x;
    i32 y;
    i32 width;
    i32 height;
    i32 fullScreen;
};
// Buka clears 0x12c bytes at the owner base (retail 0x43ff8). Registry
// operands place musicSource at 0xb4; the old cdOffset and slowVideo fields
// are absent. The 0x50 interval still needs its original type recovered.
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
    i32 musicSource;
    i32 comPort[2];
    i32 baudRate[2];
    char modemInitString[100];
};
// The running executable's display row of gConfig.gfx: a live lvalue,
// re-read at every use.
#define CURRENT_GRAPHICS_CONFIG (gConfig.gfx[gCurExe])
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
// giDebugLevel, set from the command line: NONE is release play; any level
// shows the computer's routes and cell details (ADVMGR). From the second
// level a saved game loads with another player count (REQUEST), every
// player is set up as human (GAME) and philAI draws its status text.
// AbsAiPrint forces the MISC_FORCED level for one line; philAI traces events
// at EVENT and switches to BATTLE tracing on the trace column.
H1_ENUM_BEGIN(DebugLevel)
    DEBUG_LEVEL_NONE = 0,
    FILE_REQUESTER_DEBUG_ALLOW_PLAYER_MISMATCH_MIN = 2,
    GAME_DEBUG_LEVEL_ALL_HUMAN_MIN = 2,
    AI_DEBUG_LEVEL_STATUS_TEXT_MIN = 2,
    KBWIN_TRACE_DEBUG_LEVEL = 4,
    AI_DEBUG_LEVEL_EVENT = 5,
    AI_DEBUG_LEVEL_BATTLE = 9,
    MISC_FORCED_DEBUG_LEVEL = 9
H1_ENUM_END(DebugLevel)
extern i32 giDebugLevel;
extern class palette* gpBufferPalette;
extern i32 gColorMice;
extern i32 gSpecialMouseMasks;
extern char gDataPath[];
extern char gGamePath[];
// The artifact-event texts, by artifact (KB .data).
extern char* gArtifactEvent[];
extern char gMapPath[];
extern char gSoundPath[];
extern i32 gInDialog;
extern class palette* gPalette;
// Main: right-click help for the six adventure panel buttons, the typed
// cheat-digit sequence and the pending menu command.
extern char* gAdvMenuHelp[];
extern i32 gAllBlack;
extern i32 gNoBorder;
// Per hero type scouting radius used by TeleportTo.
extern i8 gHeroScoutRadius[];
extern u8 gCloudType[];
// GAME stores and reloads it as a dword (retail 0x4c7ca0).
extern i32 giCurWatchPlayer;
extern i32 gForceSwitchMusic;
extern i32 gMenuCommand;
extern i16 gMapX;
extern i16 gMapY;
// UpdBottomViewHero's per-creature mons32.icn frame width.
extern i8 gMons32Width[];
extern class searchArray* gpSearchArray;
// UpdateRadar's per-owner and per-terrain radar pixel colours.
extern i16 gRadarOwnerColor[];
extern i16 gRadarTerrainColor[];
// Route arrow frame by [next step][this step] path direction.
extern i8 gRouteFrame[][8];
// Damage multipliers for attack minus defense, -20..20 (0x00492288).
extern float gBattleStat[];
extern i16 gSpellEffectFrame;
// Pow (impact) effect icons by effect (0x00490eb0).
extern char* gPowEffectNames[];
extern char* gArmySizeNames[6][2];
// New-game "King of the Hill" option; campaign scenarios preset it.
extern i8 gbIAmGreatest;
extern struct campaignScenario gCampaignScenarios[];
// Victory/defeat window texts (0x00493c60).
extern char* gBattleResults[];
// Combat help lines for the auto-combat, skip and other controls.
extern char* gCombatHelp[];
// Command help lines for CombatMessage(short) (0x00492ea8).
extern char* gCombatMessage[];
// Spell-book hover help lines (0x00493890).
extern char* gSpellHelp[];
// CheckHandleNet hands combat packets back while a battle is running.
extern i8 gInCombat;
// Neighbour hex per combat hex and direction (0x00490fd8), -1 off grid.
extern i8 gCombatAdjacency[45][6];
// The loaded combat effect icon's file id (0x004c6d64).
extern i16 gCurLoadedSpellFileId;
// ProcessCombatMsg records the hero casting from the combat screen.
extern i32 giCurGeneral;
// Area spells mark each stack once per cast: [side][army slot].
extern i8 gArmyEffected[COMBAT_SIDE_COUNT][ARMY_GROUP_SLOT_COUNT];
extern char* gDifficultyNames[];
extern i32 gMapDifficulty;
extern i32 gMapSize;
// The last save name: retail places gbRetreatWin at its 0x15f-byte end.
H1_ENUM_CONST_BEGIN(LastFilenameConstant)
    GLOBAL_LAST_FILENAME_SIZE = 0x15f
H1_ENUM_CONST_END(LastFilenameConstant)
extern char gLastFilename[GLOBAL_LAST_FILENAME_SIZE];
extern char gLastMapName[];
extern char* gMapSizeNames[];
extern char* gHeroScreen[];
extern char* gArtifactDesc[];
extern char* gClassNames[];
// Per-class sea mobility multiplier and level thresholds (retail 0x492038,
// 0x492598).
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
// FightValueOfStack's primary-stat power curve, per-spell AI flags and
// values, spell-power duration scale and per-charge cast weights.
extern float gStatPower[];
// DoAI: the single player the AI may run for, and the places each hero has
// already started from this turn.
extern i8 giLimitPlayer;
extern i32 gMineIncome[];
extern i16 gSpellAIValue[];
extern class armyGroup* gpMonGroup;
extern class philAI* gpPhilAI;
extern i32 gResourceBaseValue[];
// ValueOfBuyingHero: the hero class native to each town type.
extern i8 gTownHeroClass[];
extern i32 gUltArtifactAvgValue;
// GoodAdjacent skips cells whose adjacency byte carries the monster bit.
extern u8 mapExtra[MAP_CELL_GRID_SIZE][MAP_CELL_GRID_SIZE];
extern i8 gbGamePosToNetPos[];
// WaitForOtherPlayer stores the game position of net position zero here
// (0x004c6710).
extern i32 giHostGamePos;
extern i32 giThisGamePos;
extern i32 giThisNetPos;
extern i8 iMPBaseType;
i8 NetPosToGamePos(i32 netPos);
i8 WaitForOtherPlayer(void);
// SeedPosition's seeding state.
extern i32 giSeedingValid;
// KB-band setup state: the direct-connect flag and the multiplayer game type.
extern i8 gDirectConnect;
extern i8 iMPExtendedType;
extern i32 gInSmacker;
// Spells taught per mage-guild level (retail 0x492514).
extern i8 gMageGuildSpellCount[];
extern char* gCastleInfo[];
extern char* gTownCommand[];
extern struct TownBuildingExtent gTownBuildingExtents[4][16];
// KB's tavern recruit dialog handler (retail 0x0045140e).
i16 RecruitHeroHandler(struct tag_message& message);
extern i8 townTheme[];
extern i32 gFullCombatScreenDrawn;
extern i32 gLimitedCombatUpdatePalette;
extern i32 gScrollX;
extern i32 gScrollY;
// CheckEndGame's re-entry guard and last offered score, the creature
// alignment names (by type / 6) and the score labels.
extern i8 gInCheckEndGame;
extern i32 giScore;
// oldmain's re-entry guard and the intro, end-sequence and remote state it
// shares with the game screens.
extern i8 gKBDone;
extern i16 boatFrameFlip[];
// Combat ground tiles (0x00490e70) and obstacle icons (0x00490e90) per
// combat terrain.
extern char* gCombatGroundNames[];
extern char* gCombatObstacleNames[];
// Hero level names and the per-class primary-skill gain table.
extern char* gHeroLevel[];
extern char* gViewGeneralHelp[];
// Primary stat, morale and luck labels of the general's stats text, and the
// combat command help lines HandleViewGeneral shows (entries 1-5).
extern char* gViewGeneralLabels[];
extern i32 gAdvDisposeLevel;
extern char* gAlignmentNames[];
extern char* gAPanelHelp[];
// Army info strings: attack, defense, shots (combat), damage, hit points,
// speed, morale, luck, shots (adventure); then the speed names.
extern char* gArmyStatText[];
extern i32 gEnlargeScreenBlit;
extern i32 gHeartbeatSeen;
// Setup screens: the setup-dialog flag kbwin's menus check and the
// right-click help of each setup dialog.
extern i32 gInSetupDialog;
// The other side's ready flag and the heartbeat-seen flag (REMOTE).
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
// NewMap: the town type of each crest and the types already given to the
// first four random towns; the starting hero class of each campaign crest and
// of each town type, each hero class's sight radius, the starting resources by
// difficulty, the spell attribute bits and mage-guild pool by spell level, the
// vision radius a claimed town grants and the mines placed per type.
extern i16 gCrestTownTypes[];
extern char gcWinText[];
// Event texts, player colour names and the wandering-monster group of the
// current encounter.
extern char* gEventText[];
// Scenario-info labels: human seat handicap and map difficulty names.
extern char* gHandicapNames[];
// Default hero names (name, short name) restored with the original data, and
// a per-cell scratch map cleared on every load.
extern char* gHeroNames[][2];
extern i8 gHeroSkillBonus[][9][HERO_PRIMARY_STAT_COUNT];
extern char* gHumanPlayerTypeNames[];
// Campaign: the lord picked on stpcmpgn.bin (1-4; PickLoadGame filters *.CGM
// on it), scenario titles and briefings, two crest bytes per side (the first
// is the human player's), side names and win texts, and the town a campaign
// map renames at a fixed position (x, y, then the name).
extern i8 gCampaignChoice;
extern i8 giMonthType;
extern i8 giMonthTypeExtra;
extern char* gInitMenuHelp[];
extern i8 gVisRangeTown;
// Calendar specials: week/month type and the featured creature or name.
extern i8 giWeekType;
extern i8 giWeekTypeExtra;
extern char* gLuckInfoText[];
extern char* gLuckText[];
extern i8 gMageGuildSpellPool[4][8];
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
extern i32 gStartingResources[][7];
extern char* gTownNames[];
extern char* gWeekNames[];
// Hero frame flips for the horse and boat walk cycles.
extern i16 horseFrameFlip[];
extern char* musicQualityText[];
// Adventure control panel: option labels, then the control-panel and
// adventure-panel help lines.
extern char* onOffText[];
extern char* walkSpeedText[];

// gAdvDisposeLevel while combat runs: how much adventure-screen art the
// resource manager may release.
H1_ENUM_BEGIN(AdvDisposeLevel)
    ADV_DISPOSE_NONE = 0,
    ADV_DISPOSE_PARTIAL = 1,
    ADV_DISPOSE_FULL = 2
H1_ENUM_END(AdvDisposeLevel)

// gCurExe and the gConfig.gfx rows: the game and the map editor share the
// registry layout (ReadPrefs/WritePrefs walk both rows; MOUSEMGR tests the
// editor).
H1_ENUM_BEGIN(ConfigExecutable)
    CONFIG_EXECUTABLE_GAME = 0,
    CONFIG_EXECUTABLE_EDITOR = 1,
    CONFIG_EXECUTABLE_COUNT = 2
H1_ENUM_END(ConfigExecutable)

// InitMenuHandler's right-click help: the gInitMenuHelp row.
H1_ENUM_BEGIN(MainMenuHelp)
    MAIN_MENU_HELP_NONE = -1,
    MAIN_MENU_HELP_NEW_GAME = 0,
    MAIN_MENU_HELP_LOAD_GAME = 1,
    MAIN_MENU_HELP_HIGH_SCORES = 2,
    MAIN_MENU_HELP_CREDITS = 3,
    MAIN_MENU_HELP_QUIT = 4
H1_ENUM_END(MainMenuHelp)

// gEndSequence: CheckEndGame sets LOST/WON, and WON becomes CAMPAIGN_COMPLETE
// after the last campaign scenario; oldmain plays the matching video (the
// value indexes endVideos), offers a replay after LOST and
// advances the campaign after WON.
H1_ENUM_BEGIN(GameEndSequence)
    GAME_END_LOST = 0,
    GAME_END_WON = 1,
    GAME_END_CAMPAIGN_COMPLETE = 2,
    GAME_END_SEQUENCE_COUNT = 3
H1_ENUM_END(GameEndSequence)

// Network positions (gbGamePosToNetPos, giThisNetPos): the host is
// position HOST; a game position with no network player maps to NONE.
H1_ENUM_CONST_BEGIN(NetPositionConstant)
    NET_POSITION_NONE = -1,
    NET_POSITION_HOST = 0
H1_ENUM_CONST_END(NetPositionConstant)

// playerData::m_daysLeft: NO_GRACE_PERIOD while the player holds a town;
// losing the last town starts a GRACE_DAYS countdown that game::NewDay runs
// down to elimination.
H1_ENUM_CONST_BEGIN(CheckEndGameConstant)
    END_GAME_NO_GRACE_PERIOD = -1,
    END_GAME_GRACE_DAYS = CALENDAR_DAYS_PER_WEEK
H1_ENUM_CONST_END(CheckEndGameConstant)

// KB's morale-screen text table; the five-alignment line was appended last.
H1_ENUM_BEGIN(MoraleInfoText)
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
    MORALE_INFO_FIVE_ALIGNMENTS = 20
H1_ENUM_END(MoraleInfoText)

// KB's luck-screen text table: three verdicts, a header, then one line per
// luck source in the order ShowLuckInfo appends them.
H1_ENUM_BEGIN(LuckInfoText)
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
    LUCK_INFO_NONE = 10
H1_ENUM_END(LuckInfoText)

// HoMM1 score-to-monster tables pair a threshold word with a monster word.
H1_ENUM_CONST_BEGIN(ScoreMonsterConstant)
    SCORE_MONSTER_COUNT = 28,
    SCORE_MONSTER_THRESHOLD = 0,
    SCORE_MONSTER_TYPE = 1
H1_ENUM_CONST_END(ScoreMonsterConstant)

// netbox.bin text widgets: the two scrolled chat lines (cNetBoxLine) and the
// line being typed.
H1_ENUM_BEGIN(NetBoxControl)
    NET_BOX_LINE_PREVIOUS = 1,
    NET_BOX_LINE_LATEST = 2,
    NET_BOX_INPUT = 3
H1_ENUM_END(NetBoxControl)

// PopNetBox blinks the input cursor on NET_BOX_BLINK_TIMER_SLOT every
// BLINK_DELAY ms.
H1_ENUM_CONST_BEGIN(NetBoxConstant)
    NET_BOX_BLINK_DELAY = 360
H1_ENUM_CONST_END(NetBoxConstant)

// congspre.bin / congrats.bin text widgets: the title (or the campaign's win
// text), the five gScoreLabels captions, and the standard game's days, base
// score, difficulty, final score and creature rating.
H1_ENUM_BEGIN(CongratsControl)
    CONGRATS_TITLE = 100,
    CONGRATS_SCORE_LABEL_FIRST = 101,
    CONGRATS_DAYS = 106,
    CONGRATS_BASE_SCORE = 107,
    CONGRATS_DIFFICULTY = 108,
    CONGRATS_FINAL_SCORE = 109,
    CONGRATS_RATING = 110
H1_ENUM_END(CongratsControl)

H1_ENUM_CONST_BEGIN(CongratsConstant)
    CONGRATS_SCORE_LABEL_COUNT = 5
H1_ENUM_CONST_END(CongratsConstant)

// dataentr.bin widgets: the prompt text and the edit field.
H1_ENUM_BEGIN(DataEntryControl)
    DATA_ENTRY_PROMPT = 1,
    DATA_ENTRY_TEXT = 10
H1_ENUM_END(DataEntryControl)

#endif

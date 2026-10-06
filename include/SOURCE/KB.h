#ifndef HOMM1_SOURCE_KB_H
#define HOMM1_SOURCE_KB_H

#include <BASE/audioTypes.h>
#include <BASE/message.h>
#include <Domains.h>
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
H1_ENUM_ID_BEGIN(MainMenuControl)
MAIN_MENU_NO_COMMAND = -1, MAIN_MENU_NEW_GAME = 1, MAIN_MENU_LOAD_GAME = 2, MAIN_MENU_QUIT = 4,
                           MAIN_MENU_HIGH_SCORES = 5, MAIN_MENU_CREDITS = 6,
                           MAIN_MENU_LAST = MAIN_MENU_CREDITS H1_ENUM_ID_END(MainMenuControl)

                               extern b8 gInPollSound;
#define gNoSound gbNoSound // spelling fixes .bss order
extern i8 gNoSound;
extern b8 gShowHighScore;
// HeroView and the kingdom overview raise these while their screens are up;
// NormalDialog only parks over the adventure map when neither is showing.
extern b8 gHeroWindShowing;
extern b8 gOverviewShowing;
#define gHighScoreType giHighScoreType // spelling fixes .bss order
extern i8 gHighScoreType;
extern i8 gTerrainCost[FINDPATH_TERRAIN_COUNT][FINDPATH_STEP_COST_COUNT];
// The terrain type under a map cell.
#define CELL_TERRAIN(cell) (gGroundToTerrain[(cell)->m_tileIndex])
#define gShowIt bShowIt // spelling fixes .bss order
extern i32 gShowIt;
extern char gText[];
extern H1_ENUM_ARRAY(char*, gArmyNames, CreatureType, CREATURE_COUNT);
// Locale-independent resource stems; display names stay in the catalog.
extern H1_ENUM_ARRAY(char*, gArmySpriteNames, CreatureType, CREATURE_COUNT);
extern H1_ENUM_ARRAY(char*, gArmyNamesPlural, CreatureType, CREATURE_COUNT);
// A creature's name, singular for counts at most one.
#define CREATURE_DISPLAY_NAME(type, count)                                                         \
    ((count) <= 1 ? gArmyNames[type] : gArmyNamesPlural[type])
extern H1_ENUM_ARRAY(struct tag_monsterInfo, gMonsterDatabase, CreatureType, CREATURE_COUNT);
extern i32 gMinimized;
extern char* gMemoryErrorTitle;
extern char* gMemoryRequirements;
extern char* gExtendedMemoryUnits;
extern char* gConventionalMemoryUnits;
extern i32 gRequiredExtendedMemory;
extern i32 gRequiredConventionalMemory;
extern b32 gLoadingMonoIcon;
extern struct configStruct gConfig;
// Retail DoDimensionDoor walks gSearchArray paths through this delta table.
extern struct tag_tilePoint gNormalDirTable[];
#define gDefaultAggregateName DEFAULT_AGGREGATE_NAME // spelling fixes .bss order
extern char* gDefaultAggregateName;
#define gResourceManager gpResourceManager // spelling fixes .bss order
extern class resourceManager* gResourceManager;
#define gWindowManager gpWindowManager // spelling fixes .bss order
extern class heroWindowManager* gWindowManager;
#define gMouseManager gpMouseManager // spelling fixes .bss order
extern class mouseManager* gMouseManager;
#define gNormalDialogWindow gCommonDialogBox // spelling fixes .bss order
extern class heroWindow* gNormalDialogWindow;
#define gAdvManager gpAdvManager // spelling fixes .bss order
extern class advManager* gAdvManager;
#define gThisNetHumanPlayer gbThisNetHumanPlayer // spelling fixes .bss order
extern i8 gThisNetHumanPlayer[];
#define gTownManager gpTownManager // spelling fixes .bss order
extern class townManager* gTownManager;
#define gCombatManager gpCombatManager // spelling fixes .bss order
extern class combatManager* gCombatManager;
#define gExec gpExec // spelling fixes .bss order
extern class executive* gExec;
#define gGame gpGame // spelling fixes .bss order
extern class game* gGame;
extern i32 gHighMemBuffer;
#define gBottomViewOverrideEndTime giBottomViewOverrideEndTime // spelling fixes .bss order
extern i32 gBottomViewOverrideEndTime;
#define gBottomViewResource giBottomViewResource // spelling fixes .bss order
extern H1_ENUM_STORAGE(ResourceType, i32) gBottomViewResource;
#define gBottomViewResourceQty giBottomViewResourceQty // spelling fixes .bss order
extern i32 gBottomViewResourceQty;
#define gBottomViewText gcBottomViewText // spelling fixes .bss order
extern char gBottomViewText[];
extern b32 gHeroMoving;
extern b32 gRemoteOn;
#define gDataEntryWindow DataEntryWin // spelling fixes .bss order
extern class heroWindow* gDataEntryWindow;
#define gDataEntryDest cDEDest // spelling fixes .bss order
extern char* gDataEntryDest;
#define gDataEntryMaxLen iDEMaxLen // spelling fixes .bss order
extern i32 gDataEntryMaxLen;
#define gDataEntryTime bDataEntryTime // spelling fixes .bss order
extern i8 gDataEntryTime;
#define gWaitType giWaitType // spelling fixes .bss order
extern H1_ENUM_STORAGE(DialogWaitType, i8) gWaitType;
#define gFunctionComplete gbFunctionComplete // spelling fixes .bss order
extern i8 gFunctionComplete;
// Artifact names (0x00492e60).
extern H1_ENUM_ARRAY(char*, gArtifactNames, ArtifactType, ARTIFACT_COUNT);
extern H1_ENUM_ARRAY(char*, gNeutralBuildingNames, BuildingSlotType, BUILDING_SLOT_NEUTRAL_COUNT);
extern char* gDwellingNames[];
// BuyBuild's building descriptions (0x00493c90, 0x00493720) and per-dwelling
// prerequisite building masks (0x00491880); CanBuild reads six masks per
// faction.
extern H1_ENUM_ARRAY(
    char*,
    gNeutralBuildingDescriptions,
    BuildingSlotType,
    BUILDING_SLOT_NEUTRAL_COUNT
);
extern char* gDwellingDescriptions[];
extern u16 gDwellingRequirements[];
extern i32 gMageBuildingCosts[][7];
extern H1_ENUM_ARRAY_ROWS(
    i32,
    gNeutralBuildingCosts,
    BuildingSlotType,
    BUILDING_SLOT_NEUTRAL_COUNT,
    7
);
extern i32 gDwellingCosts[][7];
extern i32 gMageBaseResourceValues[];
extern H1_ENUM_ARRAY(
    i32,
    gNeutralBaseResourceValues,
    BuildingSlotType,
    BUILDING_SLOT_NEUTRAL_COUNT
);
extern i32 gDwellingBaseResourceValues[];
#define gNetBoxLine cNetBoxLine // spelling fixes .bss order
extern char gNetBoxLine[][60];
// gMapExtraBlocks/gMapExtraSizes: the map file's extra records (signs, events,
// town customizations), addressed by a cell's or town's byte index. Record 0
// is never allocated, so gMaxMapExtra restarts at FIRST_RECORD (InitVars,
// ClearMapExtra, game::LoadMap) and ClearMapExtra frees every slot.
H1_ENUM_CONST_BEGIN(MapExtraConstant)
    MAP_EXTRA_FIRST_RECORD = 1,
    MAP_EXTRA_RECORD_CAPACITY = 255
H1_ENUM_CONST_END(MapExtraConstant)
extern void* gMapExtraBlocks[];
extern class icon* gBuyBuildIcons;
extern class icon* gSystemIcons;
extern class font* gBigFont;
extern class font* gSmallFont;
extern i16 gScoreMon[][2];
extern i16 gScoreCampaignMon[][2];
// Combat effect icon files by effect (0x00490ef0) and the one loaded effect
// icon (0x004c709c) army draws and PowEffect share.
extern H1_ENUM_ARRAY(char*, gCombatFxNames, CombatEffectAnimation, COMBAT_EFFECT_COUNT);
#define gCurLoadedSpellIcon gLoadedEffectIcn // spelling fixes .bss order
extern class icon* gCurLoadedSpellIcon;

char* GetMonsterSingularName(H1_ENUM_PARAM(CreatureType, i32) monster);
char* GetMonsterName(H1_ENUM_PARAM(CreatureType, i32) monster);
class sample* LoadPlaySample(char* name);
// gTimers slots: each entry is a KBTickCount() deadline that DelayTil waits
// for or a loop compares against. Slots 2, 4 and 5 are global; slots 0 and
// 1 are the clocks of whichever screen runs, so each owner's role name is an
// alias of its slot.
// The table ends at gScore (0x004c6a98), six slots.
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
#define gTimers glTimers // spelling fixes .bss order
extern H1_ENUM_ARRAY(i32, gTimers, TimerSlot, GLOBAL_TIMER_COUNT);
void EarlyShutDownSystem();
void QuickViewWait();
i8 CanBuild(class town* townPointer, H1_ENUM_PARAM(BuildingSlotType, i16) building);
i8 CanBuy(class town* townPointer, H1_ENUM_PARAM(BuildingSlotType, i16) building);
extern "C" void PollSound();
void ForcePollSound();
#ifndef HOMM1_EDITOR
// The game's Cyrillic-aware toupper; the editor uses the runtime's.
char toupper(char character);
#endif
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
H1_ENUM_RETURN(MessageDispatchResult, i16) NullHandler(struct tag_message& message);
char* GetBuildingName(
    H1_ENUM_PARAM(TownType, i32) race,
    H1_ENUM_PARAM(BuildingSlotType, i16) building
);
void GetBuildingCost(
    H1_ENUM_PARAM(TownType, i32) race,
    H1_ENUM_PARAM(BuildingSlotType, i16) building,
    i32* const destination,
    i32 mageLevel
);
char* GetMonsterName(H1_ENUM_PARAM(CreatureType, i32) monster);
i32 GetBuildingBaseResourceValue(
    H1_ENUM_PARAM(TownType, i32) race,
    H1_ENUM_PARAM(BuildingSlotType, i32) building,
    i32 level
);
void AddNetBoxLine(char* text);
void GOut(char* text);
extern i32 gShowIntro;
#define gScreenScroll giScreenScroll // spelling fixes .bss order
extern i8 gScreenScroll;
extern char gMapName[];
#define gBlackoutPlayer gbBlackoutPlayer // spelling fixes .bss order
extern i32 gBlackoutPlayer;
extern char gFullMapName[];
#define gMapDescription gMapDesc // spelling fixes .bss order
extern char gMapDescription[];
#define gAggPathName cAggPathName // spelling fixes .bss order
extern char gAggPathName[];
extern i32 gNumHumanPlayers;
#define gHumanPlayer gbHumanPlayer // spelling fixes .bss order
extern i32 gHumanPlayer[];
void InitMainClasses(void);
void InitVars(void);
i32 InterpretCommandLine(void);
void ClearMapExtra(void);
H1_ENUM_RETURN(CreatureType, i16) GetMonType(i32 score, H1_ENUM_PARAM(HighScoreType, i32) highScoreType);
i32 MemSize(i32);
i8 CheckMem(void);
bool IsCDDrive(i32 driveIndex);
void LoadSystemwideIcons(void);
void UnloadSystemwideIcons(void);
void UpdateSystemOptionsMenu(void);
void CleanUpMenus(void);
void EarlyResizeWindow(i32 x, i32 y, i32 width, i32 height);
void GetDataEntry(char* prompt, char* destination, i32 maximumLength, char* initialText);
H1_ENUM_RETURN(MessageDispatchResult, i16) DataEntryWindowHandler(struct tag_message& message);
H1_ENUM_RETURN(MessageDispatchResult, i16) EventWindowHandler(struct tag_message& message);
H1_ENUM_RETURN(MessageDispatchResult, i16) TrueFalseDialogHandler(struct tag_message& message);
// Town-name lookup by town id (retail 0x00455aaf); the inline game::GetTown
// narrows the id.
char* GetTownName(i32 townIndex);
void ReceiveRemotePlayerExit(i8 position, i8 hadControl, b8 eliminated, b8 timedOut);
void ShutDown(char* message);
void HandleRemoteDeadPlayerExit(i32 position);
void CheckEndGame(b32 forceWin);
void HandleRemoteSuddenExit(void);
#define gRetreatWin gbRetreatWin // spelling fixes .bss order
extern i8 gRetreatWin;
extern b8 gGameInitialized;
extern H1_ENUM_STORAGE(MainMenuControl, i16) gGameCommand;
#define gCombatSurrender gbCombatSurrender // spelling fixes .bss order
extern i8 gCombatSurrender;
// The new-map builder raises this while it claims towns and mines.
#define gInNewGameSetup gbInNewGameSetup // spelling fixes .bss order
extern i32 gInNewGameSetup;
void DeleteMainClasses(void);
#define gHighScoreManager gpHighScoreManager // spelling fixes .bss order
extern class highScoreManager* gHighScoreManager;
void FileError(char* filename);
void MemError();
void GetMonsterCost(H1_ENUM_PARAM(CreatureType, i32) monster, i32* const cost);
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
extern H1_ENUM_ARRAY(char*, gSpellDesc, SpellType, SPELL_COUNT);
extern H1_ENUM_ARRAY(char*, gSpellNames, SpellType, SPELL_COUNT);
// QuickInfo's name tables.
extern H1_ENUM_ARRAY(char*, gTerrainNames, TerrainType, TERRAIN_COUNT);
extern H1_ENUM_ARRAY(char*, gResourceNames, ResourceType, RESOURCE_COUNT);
extern H1_ENUM_ARRAY(char*, gMineNames, ResourceType, RESOURCE_COUNT);
extern char* gObjectNames[];
// KB's map-extra record count and sizes.
#define gMaxMapExtra iMaxMapExtra // spelling fixes .bss order
extern i32 gMaxMapExtra;
#define gMapExtraSizes iSizeOfMapExtra // spelling fixes .bss order
extern i32 gMapExtraSizes[];
// KB's adventure status-bar resource message and its menu, wait and victory
// screens.
void BVResMsg(char* text, H1_ENUM_PARAM(ResourceType, i32) resourceType, i32 quantity);
H1_ENUM_RETURN(MessageDispatchResult, i16) InitMenuHandler(struct tag_message& message);
H1_ENUM_RETURN(MessageDispatchResult, i16) WaitHandler(struct tag_message& message);
void ShowCongrats(void);
void CongratsWait(void);
i32 AddScoreToHighScore(
    i32 score,
    H1_ENUM_PARAM(HighScoreType, i32) highScoreType,
    char*,
    char* scenarioName
);

// Graphics records are six words.
struct exeGfxConfig {
    i32 showMenu;
    i32 x;
    i32 y;
    i32 width;
    i32 height;
    i32 fullScreen;
};
// gCurExe and the gConfig.gfx rows: the game and the map editor share the
// registry layout (ReadPrefs/WritePrefs walk both rows; MOUSEMGR tests the
// editor).
H1_ENUM_BEGIN(ConfigExecutable)
    CONFIG_EXECUTABLE_GAME = 0,
    CONFIG_EXECUTABLE_EDITOR = 1,
    CONFIG_EXECUTABLE_COUNT = 2
H1_ENUM_END(ConfigExecutable)
H1_ENUM_STEPPED(ConfigExecutable)

// gConfig.comPort/baudRate rows: the modem's and the direct (null-modem)
// connection's settings (the "Modem"/"Direct" registry values).
H1_ENUM_BEGIN(ConfigConnection)
    CONFIG_CONNECTION_MODEM = 0,
    CONFIG_CONNECTION_DIRECT = 1,
    CONFIG_CONNECTION_COUNT = 2
H1_ENUM_END(ConfigConnection)

// Buka clears 0x12c bytes at the owner base (retail 0x43ff8). Registry
// operands place musicSource at 0xb4; the old cdOffset and slowVideo fields
// are absent. The 0x50 interval still needs its original type recovered.
struct configStruct {
    H1_ENUM_STORAGE(WalkSpeed, i32) walkSpeed;
    i32 musicVolume;
    i32 soundVolume;
    i32 autosave;
    i32 showRoute;
    i32 blackoutComputer;
    H1_ENUM_ARRAY(exeGfxConfig, gfx, ConfigExecutable, CONFIG_EXECUTABLE_COUNT);
    i32 firstMapOffset;
    i32 currentMapOffset;
    char _pad_0x050[0x64];
    H1_ENUM_STORAGE(SoundMusicSource, i32) musicSource;
    H1_ENUM_ARRAY(i32, comPort, ConfigConnection, CONFIG_CONNECTION_COUNT);
    H1_ENUM_ARRAY(i32, baudRate, ConfigConnection, CONFIG_CONNECTION_COUNT);
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
extern b32 gCurrArmyDrawn;
#define gIconClipOn gbIconClipOn // spelling fixes .bss order
extern i8 gIconClipOn;
extern i32 gLimitToExtent;
extern b32 gSaveBiggestExtent;
#define gMaxExtentX giMaxExtentX // spelling fixes .bss order
extern i32 gMaxExtentX;
#define gMaxExtentY giMaxExtentY // spelling fixes .bss order
extern i32 gMaxExtentY;
#define gMinExtentX giMinExtentX // spelling fixes .bss order
extern i32 gMinExtentX;
#define gMinExtentY giMinExtentY // spelling fixes .bss order
extern i32 gMinExtentY;
extern i32 gMonoIconSkip;
extern u8 gMonoColorMap[];
extern class inputManager* gInputManager;
extern H1_ENUM_STORAGE(ConfigExecutable, i32) gCurExe;
// gDebugLevel, set from the command line: NONE is release play; any level
// shows the computer's routes and cell details (ADVMGR). From the second
// level a saved game loads with another player count (REQUEST), every
// player is set up as human (GAME) and philAI draws its status text.
// AbsAiPrint forces the MISC_FORCED level for one line; philAI traces events
// at EVENT and switches to BATTLE tracing on the trace column.
// The level is a rank (a command-line digit tested with <, >= and as a flag),
// so these are named thresholds of that number, not a closed value domain.
H1_ENUM_CONST_BEGIN(DebugLevel)
    DEBUG_LEVEL_NONE = 0,
    FILE_REQUESTER_DEBUG_ALLOW_PLAYER_MISMATCH_MIN = 2,
    GAME_DEBUG_LEVEL_ALL_HUMAN_MIN = 2,
    AI_DEBUG_LEVEL_STATUS_TEXT_MIN = 2,
    KBWIN_TRACE_DEBUG_LEVEL = 4,
    AI_DEBUG_LEVEL_EVENT = 5,
    AI_DEBUG_LEVEL_BATTLE = 9,
    MISC_FORCED_DEBUG_LEVEL = 9
H1_ENUM_CONST_END(DebugLevel)
#define gDebugLevel giDebugLevel // spelling fixes .bss order
extern i32 gDebugLevel;
#define gBufferPalette gpBufferPalette // spelling fixes .bss order
extern class palette* gBufferPalette;
extern i32 gColorMice;
extern i32 gSpecialMouseMasks;
extern char gDataPath[];
extern char gGamePath[];
// The artifact-event texts, by artifact (KB .data).
extern char* gArtifactEvent[];
extern char gMapPath[];
extern char gSoundPath[];
extern char gTracksPath[];
extern b32 gInDialog;
extern class palette* gPalette;
// Main: right-click help for the six adventure panel buttons, the typed
// cheat-digit sequence and the pending menu command.
extern b32 gAllBlack;
extern b32 gNoBorder;
// Per hero type scouting radius used by TeleportTo.
extern i8 gHeroScoutRadius[];
extern u8 gCloudType[];
// GAME stores and reloads it as a dword (retail 0x4c7ca0).
#define gCurWatchPlayer giCurWatchPlayer // spelling fixes .bss order
extern i32 gCurWatchPlayer;
extern i32 gForceSwitchMusic;
extern i32 gMenuCommand;
extern i16 gMapX;
extern i16 gMapY;
// UpdBottomViewHero's per-creature mons32.icn frame width.
extern H1_ENUM_ARRAY(i8, gMons32Width, CreatureType, CREATURE_COUNT);
#define gSearchArray gpSearchArray // spelling fixes .bss order
extern class searchArray* gSearchArray;
// UpdateRadar's per-owner and per-terrain radar pixel colours.
extern i16 gRadarOwnerColor[];
extern H1_ENUM_ARRAY(i16, gRadarTerrainColor, TerrainType, 24);
// Route arrow frame by [next step][this step] path direction.
extern i8 gRouteFrame[][8];
// Damage multipliers for attack minus defense, -20..20 (0x00492288).
extern float gBattleStat[];
#define gSpellEffectFrame gImpactOverlayFrame // spelling fixes .bss order
extern i16 gSpellEffectFrame;
// Pow (impact) effect icons by effect (0x00490eb0).
extern char* gPowEffectNames[];
extern char* gArmySizeNames[6][2];
// New-game "King of the Hill" option; campaign scenarios preset it.
#define gIAmGreatest gbIAmGreatest // spelling fixes .bss order
extern i8 gIAmGreatest;
extern struct campaignScenario gCampaignScenarios[];
// Spell-book hover help lines (0x00493890).
extern H1_ENUM_ARRAY(char*, gSpellHelp, SpellHelpText, SPELL_HELP_COUNT);
// CheckHandleNet hands combat packets back while a battle is running.
extern b8 gInCombat;
// Neighbour hex per combat hex and direction (0x00490fd8), -1 off grid.
extern H1_ENUM_ARRAY(i8, gCombatAdjacency[45], CombatHexDirection, COMBAT_DIRECTION_ADJACENT_COUNT);
// The loaded combat effect icon's file id (0x004c6d64).
#define gCurLoadedSpellFileId gEffectFileId // spelling fixes .bss order
extern i16 gCurLoadedSpellFileId;
// ProcessCombatMsg records the hero casting from the combat screen.

#define gCurGeneral giCurGeneral // spelling fixes .bss order
extern H1_ENUM_STORAGE(CombatSide, i32) gCurGeneral;
// Area spells mark each stack once per cast: [side][army slot].
extern H1_ENUM_ARRAY_ROWS(i8, gArmyEffected, CombatSide, COMBAT_SIDE_COUNT, ARMY_GROUP_SLOT_COUNT);
extern H1_ENUM_ARRAY(char*, gDifficultyNames, GameDifficulty, DIFFICULTY_COUNT);
extern H1_ENUM_STORAGE(MapDifficulty, i32) gMapDifficulty;
extern H1_ENUM_STORAGE(MapSize, i32) gMapSize;
// The file requester's last chosen name (fileRequester::GetFilename);
// retail places gRetreatWin at its end.
extern char gLastFilename[FILE_REQUESTER_NAME_SIZE];
// Cell tile index -> terrain type; IsMobile reads it zero-extended.
#define gGroundToTerrain giGroundToTerrain // spelling fixes .bss order
extern H1_ENUM_STORAGE(TerrainType, i8) gGroundToTerrain[];
#define gLastMapName gPrevGameFile // spelling fixes .bss order
extern char gLastMapName[];
extern H1_ENUM_ARRAY(char*, gMapSizeNames, MapSize, MAP_SIZE_COUNT);
extern H1_ENUM_ARRAY(char*, gHeroScreen, HeroScreenText, HERO_TEXT_COUNT);
extern H1_ENUM_ARRAY(char*, gArtifactDesc, ArtifactType, ARTIFACT_COUNT);
extern char* gClassNames[];
// Per-class sea mobility multiplier and level thresholds (retail 0x492038,
// 0x492598).
extern float gClassNavigationMod[];
#define gHeroScreenSrcIndex giHeroScreenSrcIndex // spelling fixes .bss order
extern i32 gHeroScreenSrcIndex;
extern i16 gMinExpForLevel[][HERO_EXPERIENCE_LEVEL_TABLE_COUNT];
extern class hero* gInfoViewedHero;
extern char* gStatDesc[];
extern char* gStatNames[];
#define gHeroScreenWindow heroWin // spelling fixes .bss order
extern class heroWindow* gHeroScreenWindow;
extern i8 gHighScoreRank;
i32 EarlySetup(void);
i32 GameUnsaved(void);
extern b8 gFirstTimeThrough;
extern char gAnimPath[];
#define gRegAppPath gcRegAppPath // spelling fixes .bss order
extern char gRegAppPath[];
#define gRegCDRomPath gcRegCDRomPath // spelling fixes .bss order
extern char gRegCDRomPath[];
#define gCurWindowsStyleFlags giCurWindowsStyleFlags // spelling fixes .bss order
extern i32 gCurWindowsStyleFlags;
extern struct SMenuEnableStatus gMenuEnableStatus[];
extern struct WindowTextEntry gWinSetup[];
extern char* gWinSetupText[];
i32 HandleAppSpecificMenuCommands(i32 command);
i32 oldmain(void);
void UpdateAppSpecificMenus(void* menu);

#define gSpecialHideCursor bSpecialHideCursor // spelling fixes .bss order
extern i32 gSpecialHideCursor;
extern H1_ENUM_ARRAY(i32, gArtifactBaseRV, ArtifactType, ARTIFACT_REGULAR_END);
extern b8 gDrawSavedCursor;
extern H1_ENUM_ARRAY(i8, gSpellAIFlags, SpellType, SPELL_COUNT);
extern H1_ENUM_ARRAY_ROWS(
    H1_ENUM_STORAGE(CreatureType, i8),
    gDwellingType,
    TownType,
    TOWN_TYPE_COUNT,
    6
);
extern float gSpellCastNumMod[];
// FightValueOfStack's primary-stat power curve, per-spell AI flags and
// values, spell-power duration scale and per-charge cast weights.
extern float gStatPower[];
// DoAI: the single player the AI may run for, and the places each hero has
// already started from this turn.

#define gLimitPlayer giLimitPlayer // spelling fixes .bss order
extern i8 gLimitPlayer;
extern H1_ENUM_ARRAY(i32, gMineIncome, ResourceType, RESOURCE_COUNT);
extern H1_ENUM_ARRAY(i16, gSpellAIValue, SpellType, SPELL_COUNT);
#define gMonGroup gpMonGroup // spelling fixes .bss order
extern class armyGroup* gMonGroup;
#define gPhilAI gpPhilAI // spelling fixes .bss order
extern class philAI* gPhilAI;
extern H1_ENUM_ARRAY(i32, gResourceBaseValue, ResourceType, RESOURCE_COUNT);
// ValueOfBuyingHero: the hero class native to each town type.
extern i8 gTownHeroClass[];
extern i32 gUltArtifactAvgValue;
// GoodAdjacent skips cells whose adjacency byte carries the monster bit.
#define gMapExtra mapExtra // spelling fixes .bss order
extern u8 gMapExtra[MAP_CELL_GRID_SIZE][MAP_CELL_GRID_SIZE];
#define gGamePosToNetPos gbGamePosToNetPos // spelling fixes .bss order
extern i8 gGamePosToNetPos[];
// WaitForOtherPlayer stores the game position of net position zero here
// (0x004c6710).

#define gHostGamePos giHostGamePos // spelling fixes .bss order
extern i32 gHostGamePos;
#define gThisGamePos giThisGamePos // spelling fixes .bss order
extern i32 gThisGamePos;
#define gThisNetPos giThisNetPos // spelling fixes .bss order
extern i32 gThisNetPos;
#define gMapBaseType iMPBaseType // spelling fixes .bss order
extern H1_ENUM_STORAGE(MultiplayerBaseType, i8) gMapBaseType;
i8 NetPosToGamePos(i32 netPos);
b8 WaitForOtherPlayer(void);
// SeedPosition's seeding state.
#define gSeedingValid giSeedingValid // spelling fixes .bss order
extern i32 gSeedingValid;
// KB-band setup state: the direct-connect flag and the multiplayer game type.
extern i8 gDirectConnect;

#define gMapExtendedType iMPExtendedType // spelling fixes .bss order
extern H1_ENUM_STORAGE(RemoteGameMode, i8) gMapExtendedType;
extern b32 gInSmacker;
// Spells taught per mage-guild level (retail 0x492514).
extern i8 gMageGuildSpellCount[];
extern H1_ENUM_ARRAY2(
    struct TownBuildingExtent,
    gTownBuildingExtents,
    TownType,
    TOWN_TYPE_COUNT,
    BuildingSlotType,
    BUILDING_SLOT_CAPACITY
);
// KB's tavern recruit dialog handler (retail 0x0045140e).
H1_ENUM_RETURN(MessageDispatchResult, i16) RecruitHeroHandler(struct tag_message& message);
extern H1_ENUM_ARRAY(i8, gTownTheme, TownType, TOWN_TYPE_COUNT);
extern b32 gFullCombatScreenDrawn;
extern b32 gLimitedCombatUpdatePalette;
extern i32 gScrollX;
extern i32 gScrollY;
// CheckEndGame's re-entry guard and last offered score, the creature
// alignment names (by type / 6) and the score labels.
extern b8 gInCheckEndGame;
#define gScore giScore // spelling fixes .bss order
extern i32 gScore;
// oldmain's re-entry guard and the intro, end-sequence and remote state it
// shares with the game screens.
extern b8 gKBDone;
extern i16 gBoatFrameFlip[];
// Combat ground tiles (0x00490e70) and obstacle icons (0x00490e90) per
// combat terrain.
extern H1_ENUM_ARRAY(char*, gCombatGroundNames, TerrainType, TERRAIN_COUNT);
extern H1_ENUM_ARRAY(char*, gCombatObstacleNames, TerrainType, TERRAIN_COUNT);
// Hero level names and the per-class primary-skill gain table.
extern H1_ENUM_ARRAY(char*, gHeroLevel, HeroLevelText, HERO_LEVEL_TEXT_COUNT);
extern H1_ENUM_ARRAY(char*, gViewGeneralHelp, ViewGeneralHoverHelp, GENERAL_HOVER_HELP_COUNT);
// Primary stat, morale and luck labels of the general's stats text, and the
// combat command help lines HandleViewGeneral shows (entries 1-5).
extern char* gViewGeneralLabels[];
extern char* gAlignmentNames[];
extern char* gAPanelHelp[];
// Army info strings: attack, defense, shots (combat), damage, hit points,
// speed, morale, luck, shots (adventure); then the speed names.
extern char* gArmyStatText[];
extern b32 gEnlargeScreenBlit;
extern b32 gHeartbeatSeen;
// Setup screens: the setup-dialog flag kbwin's menus check and the
// right-click help of each setup dialog.
extern b32 gInSetupDialog;
// The other side's ready flag and the heartbeat-seen flag (REMOTE).
extern b32 gRemoteReady;
extern b8 gSkipIntro;
#define gWaitForRemoteReceive gbWaitForRemoteReceive // spelling fixes .bss order
extern i8 gWaitForRemoteReceive;
extern char* gCampaignScenarioNames[];
extern char* gCampaignScenarioText[];
extern H1_ENUM_STORAGE(PlayerColor, i8) gCampaignSideCrests[][2];
extern char* gCampaignSideNames[];
extern char* gCampaignWinTexts[];
extern H1_ENUM_ARRAY(char*, gColorNames, PlayerColor, PLAYER_COLOR_COUNT);
extern H1_ENUM_ARRAY(i16, gCrestHeroClass, PlayerColor, PLAYER_COLOR_COUNT);
// NewMap: the town type of each crest and the types already given to the
// first four random towns; the starting hero class of each campaign crest and
// of each town type, each hero class's sight radius, the starting resources by
// difficulty, the spell attribute bits and mage-guild pool by spell level, the
// vision radius a claimed town grants and the mines placed per type.
extern H1_ENUM_ARRAY(
    H1_ENUM_STORAGE(TownType, i16),
    gCrestTownTypes,
    PlayerColor,
    PLAYER_COLOR_COUNT
);
#define gWinText gcWinText // spelling fixes .bss order
extern char gWinText[];
// Player colour names and the wandering-monster group of the current
// encounter (the event texts are EVENTS.h gEventText).
// Scenario-info labels: human seat handicap and map difficulty names.
extern char* gHandicapNames[];
// Default hero names (name, short name) restored with the original data, and
// a per-cell scratch map cleared on every load.
extern char* gHeroNames[][2];
extern H1_ENUM_ARRAY(i8, gHeroSkillBonus[4][9], HeroPrimaryStat, HERO_PRIMARY_STAT_COUNT);
extern char* gHumanPlayerTypeNames[];
// Campaign: the lord picked on stpcmpgn.bin (1-4; PickLoadGame filters *.CGM
// on it), scenario titles and briefings, two crest bytes per side (the first
// is the human player's), side names and win texts, and the town a campaign
// map renames at a fixed position (x, y, then the name).

#define gCampaignChoice gChosenCampaignIndex // spelling fixes .bss order
extern H1_ENUM_STORAGE(CampaignChoice, i8) gCampaignChoice;
#define gMonthType giMonthType // spelling fixes .bss order
extern H1_ENUM_STORAGE(CalendarPeriodType, i8) gMonthType;
#define gMonthTypeExtra giMonthTypeExtra // spelling fixes .bss order
extern i8 gMonthTypeExtra;
extern i8 gVisRangeTown;
// Calendar specials: week/month type and the featured creature or name.

#define gWeekType giWeekType // spelling fixes .bss order
extern H1_ENUM_STORAGE(CalendarPeriodType, i8) gWeekType;
#define gWeekTypeExtra giWeekTypeExtra // spelling fixes .bss order
extern i8 gWeekTypeExtra;
extern char* gLuckText[];
extern H1_ENUM_STORAGE(SpellType, i8) gMageGuildSpellPool[4][8];
extern H1_ENUM_ARRAY(char*, gMapDifficultyNames, MapDifficulty, MAP_DIFFICULTY_COUNT);
extern char* gMonthNames[];
// Morale and luck names, indexed from -3, and their info-window texts.
extern char* gMoraleText[];
// New-game screen: right-click help and the human/computer seat labels.
extern H1_ENUM_ARRAY(char*, gNewGameHelp, NewGameHelp, NEW_GAME_HELP_COUNT);
// New-turn texts: days-left and last-day warnings, then the month/week
// banners and names.
extern char* gNewTurnText[];
// Kingdom overview text: the dated title, then Dragon City and Lighthouse.
extern char* gOverviewText[];
extern char* gPlayerTypeNames[];
extern char* gScoreLabels[];
extern H1_ENUM_ARRAY(char*, gSetupBaudHelp, SetupBaudHelp, SETUP_BAUD_HELP_COUNT);
extern H1_ENUM_ARRAY(char*, gSetupCampaignGameHelp, SetupCampaignHelp, SETUP_CAMPAIGN_HELP_COUNT);
extern H1_ENUM_ARRAY(char*, gSetupComPortHelp, SetupComPortHelp, SETUP_COM_PORT_HELP_COUNT);
extern H1_ENUM_ARRAY(char*, gSetupDCBaudHelp, SetupBaudHelp, SETUP_BAUD_HELP_COUNT);
extern H1_ENUM_ARRAY(char*, gSetupDCComPortHelp, SetupComPortHelp, SETUP_COM_PORT_HELP_COUNT);
extern H1_ENUM_ARRAY(char*, gSetupDCGameHelp, SetupModemHelp, SETUP_MODEM_HELP_COUNT);
extern H1_ENUM_ARRAY(char*, gSetupGameHelp, SetupGameHelp, SETUP_GAME_HELP_COUNT);
extern H1_ENUM_ARRAY(char*, gSetupHotSeatGameHelp, SetupHotSeatHelp, SETUP_HOT_SEAT_HELP_COUNT);
extern H1_ENUM_ARRAY(char*, gSetupModemGameHelp, SetupModemHelp, SETUP_MODEM_HELP_COUNT);
extern H1_ENUM_ARRAY(
    char*,
    gSetupMultiPlayerGameHelp,
    SetupMultiPlayerHelp,
    SETUP_MULTIPLAYER_HELP_COUNT
);
extern H1_ENUM_ARRAY(char*, gSetupNetworkGameHelp, SetupNetworkHelp, SETUP_NETWORK_HELP_COUNT);
extern char* gSpeedText[];
extern H1_ENUM_ARRAY2(
    i32,
    gStartingResources,
    GameDifficulty,
    DIFFICULTY_COUNT,
    ResourceType,
    RESOURCE_COUNT
);
extern char* gTownNames[];
extern char* gWeekNames[];
// Hero frame flips for the horse and boat walk cycles.
extern i16 gHorseFrameFlip[];
extern char* gMusicQualityText[];
// Adventure control panel: option labels, then the control-panel and
// adventure-panel help lines.
extern char* gOnOffText[];
extern H1_ENUM_ARRAY(char*, gWalkSpeedText, WalkSpeed, WALK_SPEED_COUNT);

// gAdvDisposeLevel while combat runs: how much adventure-screen art the
// resource manager may release.
H1_ENUM_BEGIN(AdvDisposeLevel)
    ADV_DISPOSE_NONE = 0,
    ADV_DISPOSE_PARTIAL = 1,
    // advManager::Close releases the object icons up to this level and the
    // rest of its art only at NONE.
    ADV_DISPOSE_OBJECT_ICONS_LAST = ADV_DISPOSE_PARTIAL,
    ADV_DISPOSE_FULL = 2
H1_ENUM_END(AdvDisposeLevel)
extern H1_ENUM_STORAGE(AdvDisposeLevel, i32) gAdvDisposeLevel;

// InitMenuHandler's right-click help: the gInitMenuHelp row.
H1_ENUM_BEGIN(MainMenuHelp)
    MAIN_MENU_HELP_NONE = -1,
    MAIN_MENU_HELP_FIRST = 0,
    MAIN_MENU_HELP_NEW_GAME = 0,
    MAIN_MENU_HELP_LOAD_GAME = 1,
    MAIN_MENU_HELP_HIGH_SCORES = 2,
    MAIN_MENU_HELP_CREDITS = 3,
    MAIN_MENU_HELP_QUIT = 4,
    MAIN_MENU_HELP_COUNT = 5
H1_ENUM_END(MainMenuHelp)
extern H1_ENUM_ARRAY(char*, gInitMenuHelp, MainMenuHelp, MAIN_MENU_HELP_COUNT);

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
extern H1_ENUM_STORAGE(GameEndSequence, i32) gEndSequence;

// Network positions (gGamePosToNetPos, gThisNetPos): the host is
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
    MORALE_INFO_FIVE_ALIGNMENTS = 20,
    MORALE_INFO_COUNT = 21
H1_ENUM_END(MoraleInfoText)
extern H1_ENUM_ARRAY(char*, gMoraleInfoText, MoraleInfoText, MORALE_INFO_COUNT);

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
    LUCK_INFO_NONE = 10,
    LUCK_INFO_COUNT = 11
H1_ENUM_END(LuckInfoText)
extern H1_ENUM_ARRAY(char*, gLuckInfoText, LuckInfoText, LUCK_INFO_COUNT);

// Score-to-monster tables pair a threshold word with a monster word.
H1_ENUM_CONST_BEGIN(ScoreMonsterConstant)
    SCORE_MONSTER_COUNT = 28,
    SCORE_MONSTER_THRESHOLD = 0,
    SCORE_MONSTER_TYPE = 1
H1_ENUM_CONST_END(ScoreMonsterConstant)

// netbox.bin text widgets: the two scrolled chat lines (gNetBoxLine) and the
// line being typed.
H1_ENUM_ID_BEGIN(NetBoxControl)
NET_BOX_LINE_PREVIOUS = 1, NET_BOX_LINE_LATEST = 2,
                           NET_BOX_INPUT = 3 H1_ENUM_ID_END(NetBoxControl)

                           // PopNetBox blinks the input cursor on NET_BOX_BLINK_TIMER_SLOT every
                           // BLINK_DELAY ms.
                           H1_ENUM_CONST_BEGIN(NetBoxConstant)
    NET_BOX_BLINK_DELAY = 360
H1_ENUM_CONST_END(NetBoxConstant)

// congspre.bin / congrats.bin text widgets: the title (or the campaign's win
// text), the five gScoreLabels captions, and the standard game's days, base
// score, difficulty, final score and creature rating.
H1_ENUM_ID_BEGIN(CongratsControl)
CONGRATS_TITLE = 100, CONGRATS_SCORE_LABEL_FIRST = 101, CONGRATS_DAYS = 106,
                      CONGRATS_BASE_SCORE = 107, CONGRATS_DIFFICULTY = 108,
                      CONGRATS_FINAL_SCORE = 109,
                      CONGRATS_RATING = 110 H1_ENUM_ID_END(CongratsControl)

                          H1_ENUM_CONST_BEGIN(CongratsConstant)
    CONGRATS_SCORE_LABEL_COUNT = 5
H1_ENUM_CONST_END(CongratsConstant)

// dataentr.bin widgets: the prompt text and the edit field.
H1_ENUM_ID_BEGIN(DataEntryControl)
DATA_ENTRY_PROMPT = 1, DATA_ENTRY_TEXT = 10 H1_ENUM_ID_END(DataEntryControl)

#endif

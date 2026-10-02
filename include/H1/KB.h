#ifndef HOMM1_H1_KB_H
#define HOMM1_H1_KB_H

#include <Domains.h>
#include <SOURCE/dialogTypes.h>
#include <SOURCE/FINDPATH.h>
#include <SOURCE/terrainTypes.h>

// Town building ids: the order of retail gBuildingNames (0x004933a8), then
// six dwellings named per race by gDwellingNames. town::m_buildings holds
// bit 1 << id. CanBuild confirms the roles: 6 needs no castle, 3 needs water
// at the dock cell, 5 is never built and 0 has mage-guild levels.
H1_ENUM_BEGIN(BuildingSlotType)
    BUILDING_SLOT_MAGE_GUILD = 0,
    BUILDING_SLOT_THIEVES_GUILD = 1,
    BUILDING_SLOT_TAVERN = 2,
    BUILDING_SLOT_SHIPYARD = 3,
    BUILDING_SLOT_WELL = 4,
    // Slots RACE_FIRST.. use per-race build-window frames, the generic ones
    // before them frame building + 1 (TOWNMGR SetupBuildWindow).
    BUILDING_SLOT_RACE_FIRST = 5,
    BUILDING_SLOT_TENT = 5,
    BUILDING_SLOT_CASTLE = 6,
    // The non-dwelling structures end here (TOWNMGR building <= 6 tests).
    BUILDING_SLOT_STRUCTURE_LAST = 6,
    BUILDING_SLOT_DWELLING_FIRST = 7,
    BUILDING_SLOT_DWELLING_1 = 7,
    BUILDING_SLOT_DWELLING_2 = 8,
    BUILDING_SLOT_DWELLING_3 = 9,
    BUILDING_SLOT_DWELLING_4 = 10,
    BUILDING_SLOT_DWELLING_5 = 11,
    BUILDING_SLOT_DWELLING_6 = 12,
    BUILDING_SLOT_DWELLING_LAST = 12,
    // Dwellings per town: gDwellingNames/gDwellingRequirements rows are
    // m_type * DWELLING_COUNT + dwelling (TOWNMGR).
    BUILDING_SLOT_DWELLING_COUNT = 6,
    BUILDING_SLOT_COUNT = 13
H1_ENUM_END(BuildingSlotType)

// clang-format off
// giWaitType: which poll WaitHandler runs while a wait dialog is up
// (WaitForOtherPlayer, WaitForGuest, WaitForHost, InitNetGuest, InitNetHost,
// GUIModemCommandExec, GUIModemResponseExec, WaitForDirectConnect; Buka
// KBDeclarations.h DialogWaitType, same numbering).
H1_ENUM_BEGIN(DialogWaitType)
    DIALOG_WAIT_OTHER_PLAYER = 0,
    DIALOG_WAIT_NETBIOS_GUEST = 1,
    DIALOG_WAIT_NETBIOS_HOST = 2,
    DIALOG_WAIT_NETBIOS_INIT_GUEST = 3,
    DIALOG_WAIT_NETBIOS_INIT_HOST = 4,
    DIALOG_WAIT_MODEM_COMMAND = 5,
    DIALOG_WAIT_MODEM_RESPONSE = 6,
    DIALOG_WAIT_DIRECT_CONNECT = 7
H1_ENUM_END(DialogWaitType)
// clang-format on

class soundManager;
class heroWindowManager;
class heroWindow;
class resourceManager;
class advManager;
class townManager;
class executive;
class game;
struct configStruct;
struct tag_tilePoint;

extern char gbInPollSound;
extern char gbNoSound;
extern signed char gbShowHighScore;
// HeroView and the kingdom overview raise these while their screens are up;
// NormalDialog only parks over the adventure map when neither is showing.
extern signed char gbHeroWindShowing;
extern signed char gbOverviewShowing;
extern signed char gbStandardHighScore;
extern signed char giTerrainCost[FINDPATH_TERRAIN_COUNT][FINDPATH_STEP_COST_COUNT];
// Cell tile index -> terrain type; IsMobile reads it zero-extended.
extern H1_ENUM_STORAGE(TerrainType, signed char) giGroundToTerrain[];
extern int bShowIt;
extern char gText[];
extern char *gArmyNames[];
extern char* gArmyNamesPlural[];
extern struct tag_monsterInfo gMonsterDatabase[];
extern int gbMinimized;
extern signed char gbInMemError;
extern char* gcMemoryErrorTitle;
extern char* gcMemoryRequirements;
extern char* gcExtendedMemoryUnits;
extern char* gcConventionalMemoryUnits;
extern int giRequiredExtendedMemory;
extern int giRequiredConventionalMemory;
extern int gbForegroundApp;
extern int gbLoadingMonoIcon;
extern long gNextSoundPollTick;
extern long gMusicFadeTimer;
extern configStruct gConfig;
// Retail DoDimensionDoor walks gpSearchArray paths through this delta table.
extern struct tag_tilePoint normalDirTable[];
extern char* DEFAULT_AGGREGATE_NAME;
extern resourceManager* gpResourceManager;
extern soundManager* gpSoundManager;
extern heroWindowManager* gpWindowManager;
extern class mouseManager* gpMouseManager;
extern heroWindow* pNormalDialogWindow;
extern advManager* gpAdvManager;
extern signed char gbThisNetHumanPlayer[];
extern townManager* gpTownManager;
extern class combatManager* gpCombatManager;
extern executive* gpExec;
extern class game* gpGame;
extern int giHighMemBuffer;
extern int giBottomViewOverride;
extern long giBottomViewOverrideEndTime;
extern int giBottomViewResource;
extern int giBottomViewResourceQty;
extern char gcBottomViewText[];
extern int gbNoDialogMenusOn;
extern void* hmnuApp;
extern void* hmnuAdv;
extern void* hmnuDflt;
extern void* hmnuCmbt;
extern void* hmnuTown;
extern int gbClosingApp;
extern int gbHeroMoving;
extern int gbRemoteOn;
extern heroWindow* DataEntryWin;
extern char* cDEDest;
extern int iDEMaxLen;
extern signed char bDataEntryTime;
extern signed char giWaitType;
extern signed char gbFunctionComplete;
extern long lLastGetMessage;
extern long lLastAilServe;
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
long KBTickCount();
struct SAMPLE2 LoadPlaySample(char*);
void WaitEndSample(struct SAMPLE2, int);
// Empty sample pair copied into locals before LoadPlaySample (0x004c5180).
extern struct SAMPLE2 NULL_SAMPLE2;
extern int glTimers[];
void Process1WindowsMessage();
void SetNoDialogMenus(int);
void EarlyShutDownSystem();
void PollRemote();
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
extern int giFrameStep;
extern int giNumHumanPlayers;
extern int gbHumanPlayer[];
void InitMainClasses(void);
void InitVars(void);
void GetGraphicsInfo(void);
void ReadPrefs(void);
int InterpretCommandLine(void);
int SetupCDDrive(void);
char* FindLastToken(char*, char);
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
extern short gGameCommand;
extern signed char gbCombatSurrender;
// The new-map builder raises this while it claims towns and mines.
extern int gbInNewGameSetup;
extern int gbGameOver;
extern int giEndSequence;
extern int bInShutDown;
void DeleteMainClasses(void);
extern class highScoreManager* gpHighScoreManager;
void FileError(char*);
void MemError();
void SetMenus(void*, int);
void GetMonsterCost(int, int* const);
// philAI::BuildHero charges this word-sized gold price.
extern short gHeroGoldCost;
void PopNetBox(char *);
void NormalDialog(char*, H1_ENUM_PARAM(NormalDialogType, int), int, int,
                  H1_ENUM_PARAM(NormalDialogResourceType, int), int,
                  H1_ENUM_PARAM(NormalDialogResourceType, int), int,
                  H1_ENUM_PARAM(NormalDialogOrText, int));
void SetWinText(heroWindow*, short);
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

#endif

#ifndef HOMM1_H1_KB_H
#define HOMM1_H1_KB_H

#include <SOURCE/FINDPATH.h>
#include <Domains.h>

H1_ENUM_BEGIN(BuildingSlotType)
    BUILDING_SLOT_MAGE_GUILD = 0,
    BUILDING_SLOT_DWELLING_FIRST = 7
H1_ENUM_END(BuildingSlotType)

class soundManager;
class heroWindowManager;
class heroWindow;
class resourceManager;
class advManager;
class townManager;
class executive;
class game;
struct configStruct;

extern char gbInPollSound;
extern char gbNoSound;
extern signed char gbShowHighScore;
extern signed char gbStandardHighScore;
extern signed char giTerrainCost[FINDPATH_TERRAIN_COUNT][FINDPATH_STEP_COST_COUNT];
extern int bShowIt;
extern char gText[];
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
extern char* DEFAULT_AGGREGATE_NAME;
extern resourceManager* gpResourceManager;
extern soundManager* gpSoundManager;
extern heroWindowManager* gpWindowManager;
extern heroWindow* pNormalDialogWindow;
extern advManager* gpAdvManager;
extern townManager* gpTownManager;
extern executive* gpExec;
extern game* gpGame;
extern int giHighMemBuffer;
extern int giBottomViewOverride;
extern long giBottomViewOverrideEndTime;
extern int giBottomViewResource;
extern int giBottomViewResourceQty;
extern char gcBottomViewText[];
extern int gbNoDialogMenusOn;
extern void* hmnuApp;
extern signed char giWaitType;
extern signed char gbFunctionComplete;
extern long lLastGetMessage;
extern long lLastAilServe;
extern struct tag_monsterInfo gMonsterDatabase[];
extern char* gArmyNames[];
extern char* gNeutralBuildingNames[];
extern char* gDwellingNames[];
extern int gMageBuildingCosts[][7];
extern int gNeutralBuildingCosts[][7];
extern int gDwellingCosts[][7];
extern int gMageBaseResourceValues[];
extern int gNeutralBaseResourceValues[];
extern int gDwellingBaseResourceValues[];
extern char cNetBoxLine[][60];

long KBTickCount();
void Process1WindowsMessage();
void SetNoDialogMenus(int);
void EarlyShutDownSystem();
void PollRemote();
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
void ReceiveRemotePlayerExit(signed char, signed char, signed char, signed char);
void ShutDown(char*);
void FileError(char*);
void MemError();
void SetMenus(void*, int);
void NormalDialog(char*, int, int, int, int, int, int, int, int);

#endif

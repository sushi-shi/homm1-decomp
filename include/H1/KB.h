#ifndef HOMM1_H1_KB_H
#define HOMM1_H1_KB_H

#include <SOURCE/FINDPATH.h>

class soundManager;
class heroWindowManager;
class heroWindow;
class resourceManager;
class advManager;
class townManager;
class executive;
struct configStruct;

extern char gbInPollSound;
extern char gbNoSound;
extern signed char gbShowHighScore;
extern signed char gbStandardHighScore;
extern signed char giTerrainCost[FINDPATH_TERRAIN_COUNT][FINDPATH_STEP_COST_COUNT];
extern int bShowIt;
extern char gText[];
extern char *gArmyNames[];
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
extern class game* gpGame;
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

// HoMM1 KB name table accessor (retail 0x004516bf).
char* GetMonsterName(int);
long KBTickCount();
void Process1WindowsMessage();
void SetNoDialogMenus(int);
void EarlyShutDownSystem();
void PollRemote();
void QuickViewWait();
extern "C" void PollSound();
void ForcePollSound();
void ShutDown(char*);
// HoMM1 callers narrow the standard-table flag to a byte (retail 0x0045425f).
int GetMonType(int, int);
void MemError();
void SetMenus(void*, int);
void GetMonsterCost(int, int* const);
void NormalDialog(char*, int, int, int, int, int, int, int, int);

#endif

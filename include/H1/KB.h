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
extern struct tag_tilePoint normalDirTable[];
extern char* DEFAULT_AGGREGATE_NAME;
extern resourceManager* gpResourceManager;
extern soundManager* gpSoundManager;
extern heroWindowManager* gpWindowManager;
extern heroWindow* pNormalDialogWindow;
extern advManager* gpAdvManager;
extern signed char gbThisNetHumanPlayer[];
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
struct SAMPLE2 LoadPlaySample(char*);
void WaitEndSample(struct SAMPLE2, int);
// Empty sample pair copied into locals before LoadPlaySample (0x004c5180).
extern struct SAMPLE2 NULL_SAMPLE2;
extern "C" void BitSet(void*, unsigned int);
extern int glTimers[];
void Process1WindowsMessage();
void SetNoDialogMenus(int);
void EarlyShutDownSystem();
void PollRemote();
void QuickViewWait();
// HoMM1 building-name lookup by town type (retail 0x004515d9).
char *GetBuildingName(int, int);
// HoMM1 town build checks return byte flags (retail 0x004517bf/0x00451903).
signed char CanBuild(class town*, short);
signed char CanBuy(class town*, short);
extern short gHeroGoldCost;
extern "C" void PollSound();
void ForcePollSound();
void ShutDown(char*);
// HoMM1 callers narrow the standard-table flag to a byte (retail 0x0045425f).
int GetMonType(int, int);
void MemError();
void SetMenus(void*, int);
void GetMonsterCost(int, int* const);
int NullHandler(struct tag_message&);
void PopNetBox(char *);
void NormalDialog(char*, int, int, int, int, int, int, int, int);
// Buka's default dialog dispatcher (retail 0x00452b64).
short EventWindowHandler(struct tag_message&);
extern char* gSpellDesc[];
extern char* gSpellNames[];

#endif

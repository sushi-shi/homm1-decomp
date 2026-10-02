#ifndef HOMM1_SOURCE_KBWIN_H
#define HOMM1_SOURCE_KBWIN_H

#include <Domains.h>

H1_ENUM_BEGIN(WindowTextConstant)
    WINDOW_TEXT_ENTRY_COUNT = 68
H1_ENUM_END(WindowTextConstant)

H1_ENUM_BEGIN(KbwinMenuConstant)
    KBWIN_WIDTH_640 = 640,
    KBWIN_WIDTH_800 = 800,
    KBWIN_WIDTH_1024 = 1024,
    KBWIN_WIDTH_1280 = 1280,
    KBWIN_MENU_SIZE_640_480 = 0x9c45,
    KBWIN_MENU_SIZE_800_600 = 0x9c46,
    KBWIN_MENU_SIZE_1024_768 = 0x9c47,
    KBWIN_MENU_SIZE_1280_1024 = 0x9c48,
    KBWIN_MENU_FULLSCREEN = 0x9c49,
    KBWIN_MENU_ENTRY_COUNT = 70,
    KBWIN_MENU_HELP = 0x9c74,
    KBWIN_MENU_ABOUT = 0x9c75,
    KBWIN_HEIGHT_480 = 480,
    KBWIN_HEIGHT_600 = 600,
    KBWIN_HEIGHT_768 = 768,
    KBWIN_HEIGHT_1024 = 1024
H1_ENUM_END(KbwinMenuConstant)

// clang-format off
H1_ENUM_BEGIN(PrefsConstant)
    CONFIG_EXECUTABLE_GAME = 0,
    CONFIG_EXECUTABLE_EDITOR_RECORD = 1,
    CONFIG_EXECUTABLE_COUNT = 2,
    CONFIG_CONNECTION_MODEM = 0,
    CONFIG_CONNECTION_DIRECT = 1,
    CONFIG_MUSIC_SOURCE_CD = 2,
    CONFIG_WALK_SPEED_FAST = 2,
    CONFIG_WALK_SPEED_SLOW = 3,
    CONFIG_UNINITIALIZED = 99,
    CPU_FAMILY_PENTIUM = 5,
    DEFAULT_WINDOW_ORIGIN = 10,
    DEFAULT_WINDOW_WIDTH = 640,
    DEFAULT_WINDOW_HEIGHT = 480,
    DEFAULT_SMALL_WINDOW_WIDTH = 480,
    DEFAULT_SMALL_WINDOW_HEIGHT = 360,
    DEFAULT_MAP_OFFSET_LIMIT = 32000,
    MINIMUM_WINDOW_WIDTH = 320,
    MINIMUM_WINDOW_HEIGHT = 240,
    WINDOW_POSITION_MARGIN = 200,
    REGISTRY_TEXT_BUFFER_SIZE = 100,
    REGISTRY_TEXT_VALUE_SIZE = 99,
    REGISTRY_DWORD_BYTES = 4,
    CD_DRIVE_LETTER_COUNT = 26,
    CD_FIRST_DRIVE_LETTER = 2,
    CD_SETUP_READY = 0,
    CD_SETUP_NO_DRIVE = 1,
    CD_SETUP_NOT_FOUND = 2,
    CD_SETUP_NO_APP_PATH = 3,
    CD_SETUP_NO_DATA = 4,
    CD_SETUP_ATTEMPTS = 2,
    CD_SETUP_RETRY_DELAY = 3000,
    CD_AUTORUN_TAIL_BYTES = 100,
    MCI_COMMAND_BUFFER_SIZE = 256,
    KBWIN_COMMAND_LINE_CLEAR_SIZE = 61,
    KBWIN_COMMAND_LINE_LIMIT = 60,
    KBWIN_MESSAGE_FILTER_SIZE = 0x400,
    KBWIN_CLASS_STYLE = 0x100b,
    KBWIN_WINDOWED_STYLE = 0x14cf0000,
    KBWIN_FULLSCREEN_STYLE = 0x14000000,
    KBWIN_TRACE_DEBUG_LEVEL = 4,
    KBWIN_TRACE_TICK_MODULUS = 100000,
    KBWIN_TRACE_TICK_DIVISOR = 100,
    KBWIN_PROCESS_MESSAGE_MAX = 0x3ff,
    KBWIN_TIMER_ID = 1,
    KBWIN_TIMER_INTERVAL = 10,
    KBWIN_POLL_INTERVAL = 5,
    KBWIN_CYCLE_INTERVAL = 150,
    KBWIN_CYCLE_DIRECT_DRAW_DELAY = 300,
    KBWIN_GRAPHICS_DIRECT_DRAW = 1,
    KBWIN_MIN_WIDTH = 240,
    KBWIN_MIN_HEIGHT = 160
H1_ENUM_END(PrefsConstant)
// clang-format on

extern char gcRegAppPath[];
extern char gcRegCDDrive[];
extern signed char gbFirstTimeThrough;
extern char gcAnimPath[];
extern int giCDDrive;
extern void *hInstApp;
extern void *gEventHandle;
extern char gcCommandLine[];
extern unsigned char bProcessMessage[];
extern char szAppName[];
extern char szTitle[];
extern void *hmnuDflt;
extern void *hmnuAdv;
extern long lTemp;
extern struct tagRECT rcTemp;
extern int iTempX;
extern int iTempY;
extern long lLastGTimerTickCount;
extern long lLastCycleTickCount;
extern int gbClosingApp;
extern int gbHeroMoving;

#pragma pack(push, 1)
struct SMenuEnableStatus {
    unsigned int command;
    unsigned char normalEnabled;
    unsigned char setupEnabled;
    unsigned char reserved;
};
#pragma pack(pop)

extern SMenuEnableStatus gsMenuEnableStatus[KBWIN_MENU_ENTRY_COUNT];
extern int gbInSetupDialog;

#pragma pack(push, 1)
struct WindowTextEntry {
    short widgetId;
    short windowId;
};
#pragma pack(pop)

extern WindowTextEntry gWinSetup[];
extern char* gWinSetupText[];

extern void* hmnuCurrent;
extern long giCurWindowsStyleFlags;
long AppCommand(void*, unsigned int, unsigned int, long);
int AppIdle(void);
void AppExit(void);
void CleanUpMenus(void);
void UpdateSystemOptionsMenu(void);
short GetCPUType(void);
void SetGameDefaults(void);
void ReadPrefsFromFile(void);
void ReadPrefsFromRegistry(void);
void ReadPrefs(void);
void WritePrefsToFile(void);
void WritePrefsToRegistry(void);
void FileError(char *);
int IsCDDrive(int);
int SetupCDDrive(void);
int EarlySetup(void);
int AppInit(void *, void *, int, char *);
int oldmain(void);
int HandleAppSpecificMenuCommands(int);
void EarlyResizeWindow(int, int, int, int);
int GameUnsaved(void);
int KeyboardMessageHandler(void *, unsigned int, unsigned int, long);
int MouseMessageHandler(void *, unsigned int, unsigned int, long);
long __stdcall AppWndProc(void *, unsigned int, unsigned int, long);
void KBChangeMenu(void*);
void SetWinText(class heroWindow*, short);
void ResizeWindow(int, int, int, int);
void SetMenuStatus(int);
// HoMM1 window caption helper (retail 0x0045dc1f, cdecl).
void SetWinText(class heroWindow *, short);
void UpdateDfltMenu(void*);
void UpdateAppSpecificMenus(void*);

#endif

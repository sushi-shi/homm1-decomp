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
    KBWIN_MENU_ENTRY_COUNT = 70
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
    REGISTRY_DWORD_BYTES = 4
H1_ENUM_END(PrefsConstant)
// clang-format on

extern char gcRegAppPath[];
extern char gcRegCDDrive[];
extern signed char gbFirstTimeThrough;

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
void KBChangeMenu(void*);
void ResizeWindow(int, int, int, int);
void SetMenuStatus(int);

void UpdateDfltMenu(void*);
void UpdateAppSpecificMenus(void*);

#endif

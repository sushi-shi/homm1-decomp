#ifndef HOMM1_SOURCE_KBWIN_H
#define HOMM1_SOURCE_KBWIN_H

#include <Domains.h>

// clang-format off
H1_ENUM_CONST_BEGIN(WindowTextConstant)
    WINDOW_TEXT_ENTRY_COUNT = 68
H1_ENUM_CONST_END(WindowTextConstant)

// SetWinText's window id: which gWinSetup rows (widget, gWinSetupText) fill a
// window's static labels; each caller passes the id of the .bin it opened.
H1_ENUM_BEGIN(WindowTextId)
    WINDOW_TEXT_BUY_SPELL_BOOK = 0,
    WINDOW_TEXT_BUILD = 1,
    WINDOW_TEXT_CASTLE = 2,
    WINDOW_TEXT_CONTROL_PANEL = 3,
    WINDOW_TEXT_DIMENSION_DOOR = 4,
    WINDOW_TEXT_HERO = 5,
    WINDOW_TEXT_MAGE_GUILD = 6,
    WINDOW_TEXT_NEW_GAME = 7,
    WINDOW_TEXT_OVERVIEW = 8,
    WINDOW_TEXT_HERO_QUICK_VIEW = 9,
    WINDOW_TEXT_TOWN_QUICK_VIEW = 10,
    WINDOW_TEXT_RECRUIT_HERO = 0xb,
    WINDOW_TEXT_SHIPYARD = 0xc,
    WINDOW_TEXT_SWAP = 13,
    WINDOW_TEXT_TAVERN = 0xe,
    WINDOW_TEXT_THIEVES_GUILD = 0xf
H1_ENUM_END(WindowTextId)

H1_ENUM_CONST_BEGIN(KbwinMenuConstant)
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
H1_ENUM_CONST_END(KbwinMenuConstant)

// gConfig.comPort/baudRate rows: the modem's and the direct (null-modem)
// connection's settings (the "Modem"/"Direct" registry values).
H1_ENUM_BEGIN(ConfigConnection)
    CONFIG_CONNECTION_MODEM = 0,
    CONFIG_CONNECTION_DIRECT = 1
H1_ENUM_END(ConfigConnection)

// SetupCDDrive's result, which KB keeps in iCDRomErr: READY when the CD is
// found, else why not (no CD-ROM drive, no HEROES CD in any drive, no
// registered application path, no data directory).
H1_ENUM_BEGIN(CdSetupResult)
    CD_SETUP_READY = 0,
    CD_SETUP_NO_DRIVE = 1,
    CD_SETUP_NOT_FOUND = 2,
    CD_SETUP_NO_APP_PATH = 3,
    CD_SETUP_NO_DATA = 4
H1_ENUM_END(CdSetupResult)

H1_ENUM_CONST_BEGIN(PrefsConstant)
    CONFIG_UNINITIALIZED = 99,
    // ResizeWindow keeps the window's current left/top for this x/y.
    KBWIN_KEEP_POSITION = -1,
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
    KBWIN_CYCLE_WING_DELAY = 300,
    KBWIN_MIN_WIDTH = 240,
    KBWIN_MIN_HEIGHT = 160
H1_ENUM_CONST_END(PrefsConstant)
    // clang-format on

    extern char gcRegAppPath[];
extern char gcRegCDRomPath[];
extern signed char gbFirstTimeThrough;
extern char gcAnimPath[];
extern int giCDDrive;
extern void* hInstApp;
extern void* gEventHandle;
extern char gcCommandLine[];
extern unsigned char bProcessMessage[];
extern char szAppName[];
extern char szTitle[];
extern long lTemp;
extern struct tagRECT rcTemp;
extern int iTempX;
extern int iTempY;
extern long lLastGTimerTickCount;
extern long lLastCycleTickCount;

#pragma pack(push, 1)
struct SMenuEnableStatus {
    unsigned int command;
    unsigned char normalEnabled;
    unsigned char setupEnabled;
    unsigned char reserved;
};
#pragma pack(pop)

extern SMenuEnableStatus gsMenuEnableStatus[KBWIN_MENU_ENTRY_COUNT];

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
// CPUSpeed's PIT-timed divide loop (CPUSPEED.cpp).
short TimeProcessor(void);
void SetGameDefaults(void);
void ReadPrefsFromFile(void);
void ReadPrefsFromRegistry(void);
void ReadPrefs(void);
void WritePrefsToFile(void);
void WritePrefsToRegistry(void);
void FileError(char*);
int IsCDDrive(int);
H1_ENUM_RETURN(CdSetupResult, int) SetupCDDrive(void);
int EarlySetup(void);
int AppInit(void*, void*, int, char*);
int oldmain(void);
int HandleAppSpecificMenuCommands(int);
void EarlyResizeWindow(int, int, int, int);
int GameUnsaved(void);
int KeyboardMessageHandler(void*, unsigned int, unsigned int, long);
int MouseMessageHandler(void*, unsigned int, unsigned int, long);
long __stdcall AppWndProc(void*, unsigned int, unsigned int, long);
void KBChangeMenu(void*);
void SetWinText(class heroWindow*, short);
void ResizeWindow(int, int, int, int);
void SetMenuStatus(int);
// HoMM1 window caption helper (retail 0x0045dc1f, cdecl).
void SetWinText(class heroWindow*, short);
void UpdateDfltMenu(void*);
void UpdateAppSpecificMenus(void*);

#endif

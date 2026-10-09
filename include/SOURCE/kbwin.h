#ifndef HOMM1_SOURCE_KBWIN_H
#define HOMM1_SOURCE_KBWIN_H

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

enum WindowTextConstant {
    WINDOW_TEXT_ENTRY_COUNT = 68,
    WINDOW_TEXT_EDITOR_ENTRY_COUNT = 70
};

enum WindowTextId {
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
};

enum KbwinMenuConstant {
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
    KBWIN_HEIGHT_600 = 600,
    KBWIN_HEIGHT_768 = 768,
    KBWIN_HEIGHT_1024 = 1024
};

enum CdSetupResult {
    CD_SETUP_READY = 0,
    CD_SETUP_NO_DRIVE = 1,
    CD_SETUP_NOT_FOUND = 2,
    CD_SETUP_NO_APP_PATH = 3,
    CD_SETUP_NO_DATA = 4
};

enum PrefsConstant {
    KBWIN_KEEP_POSITION = -1,
    DEFAULT_WINDOW_ORIGIN = 10,
    DEFAULT_SMALL_WINDOW_WIDTH = 480,
    DEFAULT_SMALL_WINDOW_HEIGHT = 360,
    DEFAULT_MAP_OFFSET_LIMIT = 32000,
    REGISTRY_TEXT_BUFFER_SIZE = 100,
    REGISTRY_TEXT_VALUE_SIZE = 99,
    REGISTRY_DWORD_BYTES = 4,
    CD_DRIVE_LETTER_COUNT = 26,
    CD_FIRST_DRIVE_LETTER = 2,
    CD_SETUP_ATTEMPTS = 2,
    CD_SETUP_RETRY_DELAY = 3000,
    CD_AUTORUN_TAIL_BYTES = 100,
    CD_PROBE_BUFFER_SIZE = 256,
    CD_DRIVE_QUERY_PATH_SIZE = 256,
    KBWIN_COMMAND_LINE_CLEAR_SIZE = 61,
    KBWIN_COMMAND_LINE_LIMIT = 60,
    KBWIN_MESSAGE_FILTER_SIZE = 0x400,
    KBWIN_APPLICATION_ICON = 109,
    KBWIN_CLASS_STYLE = CS_BYTEALIGNCLIENT | CS_DBLCLKS | CS_HREDRAW | CS_VREDRAW,
    KBWIN_WINDOWED_STYLE = WS_VISIBLE | WS_CLIPSIBLINGS | WS_OVERLAPPEDWINDOW,
    KBWIN_FULLSCREEN_STYLE = WS_VISIBLE | WS_CLIPSIBLINGS,
    KBWIN_PROCESS_MESSAGE_MAX = 0x3ff,
    KBWIN_TIMER_ID = 1,
    KBWIN_TIMER_INTERVAL = 10,
    KBWIN_TIMER_UPDATE_MIN_INTERVAL = 5,
    KBWIN_CYCLE_INTERVAL = 150,
    KBWIN_CYCLE_WING_DELAY = 300,
    KBWIN_MIN_WIDTH = 240,
    KBWIN_MIN_HEIGHT = 160,
    // Process1WindowsMessage yields the processor for this many milliseconds
    // per pass and forces a blocking GetMessage after this interval; the game
    // runs with a 1 ms timer resolution.
    KBWIN_IDLE_SLEEP = 1,
    KBWIN_GET_MESSAGE_INTERVAL = 127,
    KBWIN_TIMER_RESOLUTION = 1
};

// Preferences are kept per language, under the key the locale names.
#define PREFS_REGISTRY_KEY localization::Tr("locale.registry_key")

extern HINSTANCE gAppInstance;
extern HANDLE gEventHandle;
extern char gCommandLine[];
extern u8 gProcessMessage[];
extern char gAppName[];
extern char gTitle[];
extern i32 gTempValue;
extern struct tagRECT gTempRect;
extern i32 gTempX;
extern i32 gTempY;

#pragma pack(push, 1)
struct SMenuEnableStatus {
    u32 command;
    u8 normalEnabled;
    u8 setupEnabled;
    u8 reserved;
};
#pragma pack(pop)

#pragma pack(push, 1)
struct WindowTextEntry {
    i16 widgetId;
    i16 windowId;
};
#pragma pack(pop)

extern HMENU gCurrentMenu;
i32 AppCommand(HWND window, u32 message, u32 messageParam, i32 messageData);
BOOL AppIdle(void);
void AppExit(void);
void SetGameDefaults(void);
void ReadPrefs(void);
i32 SetupCDDrive(void);
BOOL AppInit(HINSTANCE instance, HINSTANCE previousInstance, i32 showCommand, char* commandLine);
LRESULT CALLBACK AppWndProc(HWND window, UINT message, WPARAM messageParam, LPARAM messageData);
extern "C" BOOL __stdcall
AppAbout(HWND dialog, UINT message, WPARAM messageParam, LPARAM messageData);
void KBChangeMenu(HMENU menu);
void ResizeWindow(i32 x, i32 y, i32 width, i32 height);
void SetMenuStatus(b32 showMenu);
void SetWinText(class heroWindow* window, i16 id);
void UpdateDfltMenu(HMENU menu);
extern i32 gForegroundApp;
extern HMENU gAppMenu;
extern HMENU gAdventureMenu;
extern HMENU gDefaultMenu;
extern HMENU gCombatMenu;
extern HMENU gTownMenu;
extern b32 gClosingApp;
i32 KBTickCount();
void Process1WindowsMessage();
void SetNoDialogMenus(i32 menusEnabled);
char* FindLastToken(char* text, char token);
void SetMenus(HMENU menu, i32 enabled);
extern HWND gAppWindow;
extern i32 gMainWinScreenWidth;
extern i32 gMainWinScreenHeight;
void ProcessAssert(i32 condition, char* file, i32 line);
#define H1_ASSERT(condition) ProcessAssert((condition), __FILE__, __LINE__)
void WritePrefs();
char* FindToken(char* text, char token);

#endif

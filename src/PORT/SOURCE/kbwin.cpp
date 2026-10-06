// The native host of the game: the counterpart of src/SOURCE/kbwin.cpp,
// which is the Windows window, message loop, registry settings and CD probe.
// Everything here translates between the game's host functions (kbwin.h,
// mouseManager.h) and the platform layer; game logic stays in the shared
// units.

#include <H1/Ints.h>

#include <SOURCE/kbwin.h>

#include <BASE/Misc.h>
#include <BASE/audio.h>
#include <BASE/heroWindow.h>
#include <BASE/heroWindowManager.h>
#include <BASE/inputManager.h>
#include <BASE/message.h>
#include <BASE/miscwin.h>
#include <BASE/mouseManager.h>
#include <BASE/soundmgr.h>
#include <SOURCE/KB.h>
#include <SOURCE/cursorTypes.h>
#include <SOURCE/dialogTypes.h>
#include <SOURCE/wingraph.h>
#ifdef HOMM1_EDITOR
#include <EDITOR/EDITOR.h>
#endif

#include <PLATFORM/File.h>
#include <PLATFORM/Help.h>
#include <PLATFORM/Platform.h>

#include "../PortHost.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <map>
#include <sstream>
#include <string>
#include <vector>

i32 gForegroundApp = 1;
KBMenu gAppMenu = NULL;
i32 gClosingApp = 0;
i32 gUnusedWindowCount = 0;
#ifdef HOMM1_EDITOR
char gAppName[] = localization::Tr("editor.window.app_name");
char gTitle[] = localization::Tr("editor.window.title");
#else
char gAppName[] = localization::Tr("window.gAppName");
char gTitle[] = localization::Tr("window.gTitle");
#endif
i32 gUnusedWindowValue = -1;
i32 gMainWinScreenHeight = LOGICAL_SCREEN_HEIGHT;
i32 gMainWinScreenWidth = LOGICAL_SCREEN_WIDTH;
KBMenu gCurrentMenu = NULL;
i32 gTempX;
i32 gTempY;
i32 gTempValue;
char gCommandLine[KBWIN_COMMAND_LINE_CLEAR_SIZE];

namespace {

// ---------------------------------------------------------------- settings

// The original kept its settings in the registry under
// HKLM\SOFTWARE\Buka\3DO\Heroes of Might and Magic Platinum\1.000. The port
// keeps the same values, by the same names, in a text file.
std::string SettingsPath() {
    return platform::ConfigDirectory() + "heroes.cfg";
}

std::map<std::string, std::string> LoadSettings() {
    std::map<std::string, std::string> values;
    std::ifstream file(SettingsPath());
    std::string line;
    while (std::getline(file, line)) {
        size_t split = line.find('=');
        if (split == std::string::npos || line.empty() || line[0] == '#')
            continue;
        values[line.substr(0, split)] = line.substr(split + 1);
    }
    return values;
}

void ReadSetting(const std::map<std::string, std::string>& values, const char* name, i32& value) {
    auto found = values.find(name);
    if (found != values.end())
        value = static_cast<i32>(std::strtol(found->second.c_str(), nullptr, 10));
}

void ReadSetting(
    const std::map<std::string, std::string>& values,
    const char* name,
    char* text,
    size_t capacity
) {
    auto found = values.find(name);
    if (found == values.end() || capacity == 0)
        return;
    std::snprintf(text, capacity, "%s", found->second.c_str());
}

struct SettingField {
    const char* name;
    i32* value;
};

std::vector<SettingField> IntegerSettings() {
    return {
        {"HMM1 MusicVolume", &gConfig.musicVolume},
        {"HMM1 FXVolume", &gConfig.soundVolume},
        {"HMM1 WalkSpeed", &gConfig.walkSpeed},
        {"HMM1 ShowRoute", &gConfig.showRoute},
        {"HMM1 BlackoutComputer", &gConfig.blackoutComputer},
        {"HMM1 SoundQuality", &gConfig.musicSource},
        {"HMM1 DirectConnectComPort", &gConfig.comPort[CONFIG_CONNECTION_DIRECT]},
        {"HMM1 DirectConnectBaudRate", &gConfig.baudRate[CONFIG_CONNECTION_DIRECT]},
        {"HMM1 ModemComPort", &gConfig.comPort[CONFIG_CONNECTION_MODEM]},
        {"HMM1 ModemBaudRate", &gConfig.baudRate[CONFIG_CONNECTION_MODEM]},
        {"HMM1 UseAutosave", &gConfig.autosave},
        {"HMM1 FirstMapOffset", &gConfig.firstMapOffset},
        {"HMM1 CurrentMapOffset", &gConfig.currentMapOffset},
        {"HMM1 GameShowMenu", &gConfig.gfx[CONFIG_EXECUTABLE_GAME].showMenu},
        {"HMM1 GameWindowXLeft", &gConfig.gfx[CONFIG_EXECUTABLE_GAME].x},
        {"HMM1 GameWindowYTop", &gConfig.gfx[CONFIG_EXECUTABLE_GAME].y},
        {"HMM1 GameWindowWidth", &gConfig.gfx[CONFIG_EXECUTABLE_GAME].width},
        {"HMM1 GameWindowHeight", &gConfig.gfx[CONFIG_EXECUTABLE_GAME].height},
        {"HMM1 GameFullScreen", &gConfig.gfx[CONFIG_EXECUTABLE_GAME].fullScreen},
        {"HMM1 EditorShowMenu", &gConfig.gfx[CONFIG_EXECUTABLE_EDITOR].showMenu},
        {"HMM1 EditorWindowXLeft", &gConfig.gfx[CONFIG_EXECUTABLE_EDITOR].x},
        {"HMM1 EditorWindowYTop", &gConfig.gfx[CONFIG_EXECUTABLE_EDITOR].y},
        {"HMM1 EditorWindowWidth", &gConfig.gfx[CONFIG_EXECUTABLE_EDITOR].width},
        {"HMM1 EditorWindowHeight", &gConfig.gfx[CONFIG_EXECUTABLE_EDITOR].height},
        {"HMM1 EditorFullScreen", &gConfig.gfx[CONFIG_EXECUTABLE_EDITOR].fullScreen},
    };
}

// ---------------------------------------------------------------- game data

std::string gCdRoot;

bool ResolvesUnder(const std::string& directory, const char* path) {
    char resolved[FILE_PATH_CAPACITY];
    std::string saved = FileRoot();
    FileSetRoot(directory.c_str());
    bool found = FileResolve(path, FILE_OPEN_READ, resolved, sizeof(resolved));
    FileSetRoot(saved.c_str());
    return found;
}

// The game folder: --data, $HOMM1_DATA, beside the executable, the current
// directory, then the folder `nix run .#play` installs to.
std::string FindGameRoot(const std::string& requested) {
    std::vector<std::string> candidates;
    if (!requested.empty())
        candidates.push_back(requested);
    std::string environment = platform::Environment("HOMM1_DATA");
    if (!environment.empty())
        candidates.push_back(environment);
    candidates.push_back(platform::ExecutableDirectory());
    candidates.push_back(".");
    std::string xdg = platform::Environment("XDG_DATA_HOME");
    std::string home = platform::Environment("HOME");
    if (!xdg.empty())
        candidates.push_back(xdg + "/homm1-buka/game");
    if (!home.empty())
        candidates.push_back(home + "/.local/share/homm1-buka/game");
    for (const std::string& candidate : candidates) {
        if (ResolvesUnder(candidate, "DATA\\HEROES.AGG"))
            return candidate;
    }
    return std::string();
}

// The CD's music tracks: $HOMM1_CD, a "cd" folder beside the game folder
// (where `nix run .#play` puts it), or the game folder itself.
std::string FindCdRoot(const std::string& gameRoot) {
    std::vector<std::string> candidates;
    std::string environment = platform::Environment("HOMM1_CD");
    if (!environment.empty())
        candidates.push_back(environment);
    candidates.push_back(gameRoot + "/../cd");
    candidates.push_back(gameRoot);
    for (const std::string& candidate : candidates) {
        if (ResolvesUnder(candidate, "TRACKS\\02-AudioTrack 02.ogg"))
            return candidate;
    }
    return std::string();
}

// ---------------------------------------------------------------- messages

void PostKey(u32 message, const platform::Event& event) {
    u32 virtualKey = event.returnKey ? INPUT_VIRTUAL_KEY_RETURN : 0;
    i32 messageData = static_cast<i32>((static_cast<u32>(event.scanCode) << 16) | 1u);
    KeyboardMessageHandler(NULL, message, virtualKey, messageData);
}

void PostMouse(u32 message, const platform::Event& event) {
    i32 messageData =
        static_cast<i32>((static_cast<u32>(event.y) << 16) | static_cast<u32>(event.x));
    MouseMessageHandler(NULL, message, 0, messageData);
}

// The original's WM_CLOSE: confirm leaving an unsaved game, then end.
void CloseRequested() {
    static bool gInClose = false;
    if (gInClose)
        return;
    gInClose = true;
    if (GameUnsaved() != 0) {
        NormalDialog(localization::Tr("adventure.confirm_quit"), NORMAL_DIALOG_TYPE_YES_NO);
        if (gWindowManager->m_dialogResult != NORMAL_DIALOG_CONFIRM) {
            gInClose = false;
            return;
        }
    }
    gClosingApp = 1;
    ShutDown(NULL);
}

void Dispatch(const platform::Event& event) {
    if (MenuHandleEvent(event))
        return;
    // The game image ends at the menu bar.
    if ((event.type == platform::Event::MOUSE_MOVE || event.type == platform::Event::MOUSE_DOWN
         || event.type == platform::Event::MOUSE_UP)
        && event.y < 0)
        return;
    switch (event.type) {
        case platform::Event::QUIT:
            CloseRequested();
            break;
        case platform::Event::KEY_DOWN:
            PostKey(INPUT_MESSAGE_KEY_DOWN, event);
            break;
        case platform::Event::KEY_UP:
            PostKey(INPUT_MESSAGE_KEY_UP, event);
            break;
        case platform::Event::MOUSE_MOVE:
            PostMouse(INPUT_MESSAGE_MOUSE_MOVE, event);
            break;
        case platform::Event::MOUSE_DOWN:
            if (event.button == platform::Event::BUTTON_LEFT)
                PostMouse(
                    event.doubleClick ? INPUT_MESSAGE_LEFT_DOUBLE : INPUT_MESSAGE_LEFT_DOWN,
                    event);
            else
                PostMouse(
                    event.doubleClick ? INPUT_MESSAGE_RIGHT_DOUBLE : INPUT_MESSAGE_RIGHT_DOWN,
                    event);
            break;
        case platform::Event::MOUSE_UP:
            PostMouse(
                event.button == platform::Event::BUTTON_LEFT ? INPUT_MESSAGE_LEFT_UP
                                                             : INPUT_MESSAGE_RIGHT_UP,
                event);
            break;
        case platform::Event::FOCUS_GAINED:
            gForegroundApp = 1;
            break;
        case platform::Event::FOCUS_LOST:
            gForegroundApp = 0;
            break;
        default:
            break;
    }
}

// The original's 10 ms WM_TIMER: palette cycling and, in the editor, the
// status line timeout.
void RunTimer() {
    static i32 gLastTimerTick = 0;
    static i32 gLastCycleTick = 0;
    i32 now = KBTickCount();
    if (now - gLastTimerTick < KBWIN_TIMER_INTERVAL)
        return;
    gLastTimerTick = now;
    if (now - gLastCycleTick > KBWIN_CYCLE_INTERVAL) {
        gLastCycleTick = now;
        CycleColors();
    }
#ifdef HOMM1_EDITOR
    if (gStatusTextClearTime && now > gStatusTextClearTime)
        ClearStatusText();
#endif
}

}  // namespace

// ---------------------------------------------------------------- entry

bool KBStartHost(const char* dataRoot, const char* gameArguments, i32 fullScreen) {
    platform::StartupOptions options;
    if (!platform::Startup(options))
        return false;
    std::string gameRoot = FindGameRoot(dataRoot != NULL ? dataRoot : "");
    if (gameRoot.empty()) {
        platform::ShowMessage(
            "Heroes of Might and Magic",
            "The game data was not found. Pass --data DIR or set HOMM1_DATA to the folder "
            "holding DATA/HEROES.AGG.");
        return false;
    }
    FileSetRoot(gameRoot.c_str());
    gCdRoot = FindCdRoot(gameRoot);
    platform::Log("game data: %s", gameRoot.c_str());

    memset(gCommandLine, 0, KBWIN_COMMAND_LINE_CLEAR_SIZE);
    strncpy(gCommandLine, gameArguments, KBWIN_COMMAND_LINE_LIMIT);
    if (EarlySetup() == 0)
        return false;
    if (fullScreen >= 0)
        CURRENT_GRAPHICS_CONFIG.fullScreen = fullScreen;
    platform::SetFullscreen(CURRENT_GRAPHICS_CONFIG.fullScreen != 0);
    srand(static_cast<u32>(KBTickCount()));
    InitGraphics();
    return true;
}

i32 KBRunProgram(int argc, char** argv) {
    std::string dataRoot;
    std::string gameArguments;
    i32 fullScreen = -1;
    for (int i = 1; i < argc; i++) {
        std::string argument = argv[i];
        if (argument == "--data" && i + 1 < argc) {
            dataRoot = argv[++i];
        } else if (argument == "--window") {
            fullScreen = 0;
        } else if (argument == "--fullscreen") {
            fullScreen = 1;
        } else if (argument == "--help") {
            std::printf(
                "usage: %s [--data DIR] [--window|--fullscreen] [OPTIONS]\n"
                "OPTIONS are the original's switches, e.g. /I0 to skip the intro.\n",
                argv[0]);
            return 0;
        } else {
            if (!gameArguments.empty())
                gameArguments += ' ';
            gameArguments += argument;
        }
    }
    if (!KBStartHost(dataRoot.c_str(), gameArguments.c_str(), fullScreen))
        return 1;
    oldmain();
    ShutDown(NULL);
    return 0;
}

// ---------------------------------------------------------------- loop

i32 AppIdle(void) {
    return 1;
}

void AppExit(void) {
    MenuShutdown();
    CleanUpWinGraphics();
    CleanUpMenus();
    platform::Shutdown();
}

void Process1WindowsMessage(void) {
    static u32 gLastCall = 0;
    platform::Event event;
    bool handled = false;
    while (platform::PollEvent(event)) {
        Dispatch(event);
        handled = true;
    }
    RunTimer();
    // The original blocked in GetMessage for its next 10 ms timer tick every
    // 150 ms; its waits are otherwise busy loops around this call. A call
    // that follows the previous one within a millisecond is such a loop and
    // yields the processor briefly.
    u32 now = platform::Ticks();
    if (!handled && now - gLastCall < 1)
        platform::Sleep(1);
    gLastCall = now;
    platform::Present(false);
}

i32 KBTickCount(void) {
    return static_cast<i32>(platform::Ticks());
}

// ---------------------------------------------------------------- window

void ResizeWindow(i32 x, i32 y, i32 width, i32 height) {
    if (CURRENT_GRAPHICS_CONFIG.fullScreen != 0)
        return;
    if (x != KBWIN_KEEP_POSITION)
        CURRENT_GRAPHICS_CONFIG.x = x;
    if (y != KBWIN_KEEP_POSITION)
        CURRENT_GRAPHICS_CONFIG.y = y;
    CURRENT_GRAPHICS_CONFIG.width = width;
    CURRENT_GRAPHICS_CONFIG.height = height;
    platform::SetWindowSize(width, height);
    WritePrefs();
}

i32 AppMenuCommand(i32 command) {
    switch (command) {
        case KBWIN_MENU_ABOUT:
            platform::ShowMessage(gTitle, MenuAboutText().c_str());
            break;
        case KBWIN_MENU_HELP:
            OpenHelp();
            break;
        case KBWIN_MENU_SIZE_640_480:
            ResizeWindow(KBWIN_KEEP_POSITION, KBWIN_KEEP_POSITION, LOGICAL_SCREEN_WIDTH,
                         LOGICAL_SCREEN_HEIGHT);
            break;
        case KBWIN_MENU_SIZE_800_600:
            ResizeWindow(KBWIN_KEEP_POSITION, KBWIN_KEEP_POSITION, KBWIN_WIDTH_800,
                         KBWIN_HEIGHT_600);
            break;
        case KBWIN_MENU_SIZE_1024_768:
            ResizeWindow(KBWIN_KEEP_POSITION, KBWIN_KEEP_POSITION, KBWIN_WIDTH_1024,
                         KBWIN_HEIGHT_768);
            break;
        case KBWIN_MENU_SIZE_1280_1024:
            ResizeWindow(KBWIN_KEEP_POSITION, KBWIN_KEEP_POSITION, KBWIN_WIDTH_1280,
                         KBWIN_HEIGHT_1024);
            break;
        case KBWIN_MENU_FULLSCREEN:
            SetFullScreenStatus(1 - CURRENT_GRAPHICS_CONFIG.fullScreen);
            break;
        default:
            return HandleAppSpecificMenuCommands(command);
    }
    return 0;
}

// ---------------------------------------------------------------- help

// The original opened its help book with WinHelp(".\\HELP\\HEROES.HLP",
// HELP_FINDER), the contents page; both programs share it. Current hosts have
// no WinHelp, so the port converts the book and its contents file to one HTML
// document in its settings folder (again only when they change) and shows
// that page in the browser.
namespace {

const char kHelpBook[] = ".\\HELP\\HEROES.HLP";
const char kHelpContents[] = ".\\HELP\\HEROES.CNT";

bool ResolveExisting(const char* path, std::string& hostPath) {
    char resolved[FILE_PATH_CAPACITY];
    if (!FileExists(path) || !FileResolve(path, FILE_OPEN_READ, resolved, sizeof(resolved)))
        return false;
    hostPath = resolved;
    return true;
}

}  // namespace

bool HelpAvailable() {
    std::string hostPath;
    return ResolveExisting(kHelpBook, hostPath);
}

void OpenHelp() {
    std::string book;
    if (!ResolveExisting(kHelpBook, book)) {
        platform::Log("help is not available: no HELP\\HEROES.HLP in the game data");
        return;
    }
    std::string contents;
    ResolveExisting(kHelpContents, contents);
    std::string page;
    std::string error;
    if (!platform::help::PrepareHelp(book, contents, platform::ConfigDirectory() + "help/", page,
                                     error)) {
        platform::Log("help: %s", error.c_str());
        platform::ShowMessage(gTitle, "The help file HELP\\HEROES.HLP could not be read.");
        return;
    }
    platform::Log("help: %s", page.c_str());
    if (!platform::OpenDocument(page))
        platform::ShowMessage(gTitle, "The help could not be shown in the browser.");
}

// ---------------------------------------------------------------- menus

void KBChangeMenu(KBMenu menu) {
    if (menu == NULL)
        menu = gCurrentMenu;
    else
        gCurrentMenu = menu;
    gAppMenu = menu;
    if (CURRENT_GRAPHICS_CONFIG.showMenu && menu != NULL) {
        UpdateDfltMenu(menu);
        UpdateAppSpecificMenus(menu);
    }
    MenuRefresh();
}

void SetMenuStatus(i32 showMenu) {
    if (CURRENT_GRAPHICS_CONFIG.fullScreen && showMenu)
        return;
    CURRENT_GRAPHICS_CONFIG.showMenu = showMenu;
    KBChangeMenu(NULL);
    WritePrefs();
}

void SetNoDialogMenus(i32 menusEnabled) {
    static i32 gNoDialogMenusOn = 0;
    if (gNoDialogMenusOn && !menusEnabled)
        return;
    if (!gNoDialogMenusOn && menusEnabled)
        return;
    if (!gAppMenu)
        return;
    gNoDialogMenusOn = 1 - menusEnabled;
    SetMenus(gAppMenu, menusEnabled);
}

// ---------------------------------------------------------------- services

void KBErrorBox(const char* text, const char* title) {
    platform::ShowMessage(title, text);
}

void KBRequestClose(void) {
    CloseRequested();
}

void KBReleaseInstance(void) {}

void KBCaptureMouse(void) {
    platform::CaptureMouse(true);
}

void KBReleaseMouse(void) {
    platform::CaptureMouse(false);
}

void KBCursorPosition(i32* x, i32* y) {
    int pointerX;
    int pointerY;
    platform::MousePosition(pointerX, pointerY);
    *x = pointerX;
    *y = pointerY;
}

void KBShowSystemCursor(i32 visible) {
    platform::ShowCursor(visible != 0);
}

// ---------------------------------------------------------------- settings

void ReadPrefs(void) {
    std::map<std::string, std::string> values = LoadSettings();
    if (values.find("HMM1 MusicVolume") == values.end()) {
        memset(&gConfig, 0, sizeof(gConfig));
        SetGameDefaults();
        // The port starts in a window; F4 switches to full screen.
        for (i32 i = 0; i < CONFIG_EXECUTABLE_COUNT; i++)
            gConfig.gfx[i].fullScreen = 0;
        WritePrefs();
        return;
    }
    for (const SettingField& field : IntegerSettings())
        ReadSetting(values, field.name, *field.value);
    ReadSetting(values, "HMM1 ModemInitString", gConfig.modemInitString,
                sizeof(gConfig.modemInitString));
    SetVolumes(gConfig.soundVolume, gConfig.musicVolume);
    SetMusicSource(gConfig.musicSource != 0);
}

void WritePrefs(void) {
    UpdateSystemOptionsMenu();
    std::ostringstream text;
    text << "# Heroes of Might and Magic settings (the original's registry values)\n";
    for (const SettingField& field : IntegerSettings())
        text << field.name << '=' << *field.value << '\n';
    text << "HMM1 ModemInitString=" << gConfig.modemInitString << '\n';
    std::string path = SettingsPath();
    std::string temporary = path + ".tmp";
    std::ofstream file(temporary, std::ios::trunc);
    file << text.str();
    file.close();
    if (file)
        std::rename(temporary.c_str(), path.c_str());
}

// ---------------------------------------------------------------- CD

const std::string& CdRoot() {
    return gCdRoot;
}

i32 KBIsCDDrive(i32) {
    return 0;
}

void KBBeep(void) {}

// The original looked for its CD's music tracks on a CD drive; the port
// looks for the Tracks folder (see FindCdRoot) and otherwise plays the
// installed digital music from SOUND\.
i32 SetupCDDrive(void) {
    if (!FileExists("DATA\\HEROES.AGG"))
        return CD_SETUP_NO_DATA;
    if (gCdRoot.empty() && gConfig.musicSource == SOUND_MUSIC_SOURCE_CD) {
        gConfig.musicSource = SOUND_MUSIC_SOURCE_DIGITAL;
        SetMusicSource(SOUND_MUSIC_SOURCE_DIGITAL);
    }
    return CD_SETUP_READY;
}

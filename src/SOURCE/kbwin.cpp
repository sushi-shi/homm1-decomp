#include <match.h>

#include <SOURCE/kbwin.h>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <direct.h>
#include <fcntl.h>
#include <io.h>
#include <mmsystem.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <BASE/Misc.h>
#include <BASE/mouseManager.h>
#include <BASE/soundmgr.h>
#include <SOURCE/KB.h>
#include <BASE/heroWindow.h>
#include <BASE/heroWindowManager.h>
#include <BASE/inputManager.h>
#include <BASE/message.h>
#include <BASE/miscwin.h>
#include <BASE/audio.h>
#include <H1/Macros.h>
#include <SOURCE/cursorTypes.h>
#include <SOURCE/dialogTypes.h>
#include <SOURCE/wingraph.h>
#ifdef HOMM1_EDITOR
#include <EDITOR/EDITOR.h>
#endif

DATA(0x004a9e34)
HWND gAppWindow = NULL;
DATA(0x004a9e38)
i32 gForegroundApp = 0;
DATA(0x004a9e3c)
HMENU gAppMenu = NULL;
DATA(0x004a9e40)
HANDLE gEventHandle = NULL;

VA(0x00442ca0, 0x11a)
H1_C_LINKAGE i32 __stdcall
WinMain(HINSTANCE instance, HINSTANCE previousInstance, char* commandLine, i32 showCommand) {
    DWORD errorLast;
    MSG message;

    gAppInstance = instance;
#ifdef HOMM1_EDITOR
    gEventHandle = CreateEventA(NULL, FALSE, FALSE, localization::Tr("editor.instance.event_name"));
#else
    gEventHandle =
        CreateEventA(NULL, FALSE, FALSE, localization::Tr("startup.instance.event_name"));
#endif
    errorLast = GetLastError();
    if (gEventHandle == NULL || errorLast == ERROR_ALREADY_EXISTS) {
#ifdef HOMM1_EDITOR
        sprintf(
            gText,
            localization::Tr("startup.instance.already_running"),
            localization::Tr("editor.instance.title")
        );
#else
        sprintf(
            gText,
            localization::Tr("startup.instance.already_running"),
            localization::Tr("startup.instance.game_title")
        );
#endif
        MessageBoxA(NULL, gText, localization::Tr("startup.error.title"), MB_ICONHAND);
        return 0;
    }

    memset(gCommandLine, 0, KBWIN_COMMAND_LINE_CLEAR_SIZE);
    strncpy(gCommandLine, commandLine, KBWIN_COMMAND_LINE_LIMIT);
    if (!EarlySetup())
        return 0;
    if (!AppInit(instance, previousInstance, showCommand, commandLine))
        return 0;

    for (;;) {
        if (PeekMessageA(&message, NULL, 0, 0, PM_REMOVE)) {
            if (message.message == WM_QUIT)
                break;
            TranslateMessage(&message);
            DispatchMessageA(&message);
        } else {
            if (AppIdle())
                WaitMessage();
        }
    }
    ShutDown(NULL);
    return message.wParam;
}

VA(0x00442dba, 0x28e)
BOOL AppInit(HINSTANCE instance, HINSTANCE previousInstance, i32 showCommand, char* commandLine) {
    WNDCLASSA appClass;
    RECT windowRectangle;

    memset(gProcessMessage, 0, KBWIN_MESSAGE_FILTER_SIZE);
    gProcessMessage[WM_CREATE] = 1;
    gProcessMessage[WM_KEYDOWN] = 1;
    gProcessMessage[WM_KEYUP] = 1;
    gProcessMessage[WM_MOUSEMOVE] = 1;
    gProcessMessage[WM_LBUTTONDOWN] = 1;
    gProcessMessage[WM_LBUTTONDBLCLK] = 1;
    gProcessMessage[WM_RBUTTONDOWN] = 1;
    gProcessMessage[WM_RBUTTONDBLCLK] = 1;
    gProcessMessage[WM_LBUTTONUP] = 1;
    gProcessMessage[WM_RBUTTONUP] = 1;
    gProcessMessage[WM_TIMER] = 1;
    gProcessMessage[WM_ACTIVATEAPP] = 1;
    gProcessMessage[WM_ERASEBKGND] = 1;
    gProcessMessage[WM_MOVE] = 1;
    gProcessMessage[WM_SIZE] = 1;
    gProcessMessage[WM_COMMAND] = 1;
    gProcessMessage[WM_PALETTECHANGED] = 1;
    gProcessMessage[WM_QUERYNEWPALETTE] = 1;
    gProcessMessage[WM_PAINT] = 1;
    gProcessMessage[WM_DESTROY] = 1;
    gProcessMessage[WM_QUIT] = 1;
    gProcessMessage[WM_CLOSE] = 1;
    gProcessMessage[MM_MCINOTIFY] = 1;

    if (previousInstance == NULL) {
        appClass.hCursor = NULL;
        appClass.hIcon = LoadIconA(instance, MAKEINTRESOURCEA(KBWIN_APPLICATION_ICON));
        appClass.lpszMenuName = NULL;
        appClass.lpszClassName = gAppName;
        appClass.hbrBackground =
            reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1); // Win32 system-color brush encoding.
        appClass.hInstance = instance;
        appClass.style = KBWIN_CLASS_STYLE;
        appClass.lpfnWndProc = AppWndProc;
        appClass.cbWndExtra = 0;
        appClass.cbClsExtra = 0;
        if (RegisterClassA(&appClass) == 0)
            return FALSE;
    }

    if (CURRENT_GRAPHICS_CONFIG.showMenu != 0)
        gCurWindowsStyleFlags = KBWIN_WINDOWED_STYLE;
    else
        gCurWindowsStyleFlags = KBWIN_FULLSCREEN_STYLE;
    windowRectangle.left = windowRectangle.top = 0;
    windowRectangle.right = CURRENT_GRAPHICS_CONFIG.width - 1;
    windowRectangle.bottom = CURRENT_GRAPHICS_CONFIG.height - 1;
    AdjustWindowRect(&windowRectangle, gCurWindowsStyleFlags, CURRENT_GRAPHICS_CONFIG.showMenu);
    gAppWindow = CreateWindowExA(
        0,
        gAppName,
        gTitle,
        gCurWindowsStyleFlags,
        CURRENT_GRAPHICS_CONFIG.x,
        CURRENT_GRAPHICS_CONFIG.y,
        windowRectangle.right - windowRectangle.left + 1,
        windowRectangle.bottom - windowRectangle.top + 1,
        NULL,
        CURRENT_GRAPHICS_CONFIG.showMenu != 0 ? gDefaultMenu : NULL,
        instance,
        NULL
    );
    if (gAppWindow != NULL) {
        ShowWindow(gAppWindow, showCommand);
        SetWindowLongA(gAppWindow, GWL_STYLE, gCurWindowsStyleFlags);
        if (CURRENT_GRAPHICS_CONFIG.showMenu == 0)
            SetMenuStatus(0);
        InitGraphics();
        SetCursor(LoadCursorA(NULL, IDC_ARROW));
        oldmain();
        return TRUE;
    } else {
        return FALSE;
    }
}

// Returns TRUE without a foreground test.
VA(0x00443048, 0xa)
BOOL AppIdle(void) {
    return TRUE;
}

VA(0x00443052, 0x6ad)
VA_AT(editor, 0x0040cd4e, 0x6c2)
LRESULT CALLBACK AppWndProc(HWND window, UINT message, WPARAM messageParam, LPARAM messageData) {
    DATA(0x004a9e44)
    static i32 gLastGTimerTickCount = 0;
    DATA(0x004a9e48)
    static i32 gLastCycleTickCount = 0;
    if (message > KBWIN_PROCESS_MESSAGE_MAX || gProcessMessage[message] == 0)
        return DefWindowProcA(window, message, messageParam, messageData);

    switch (message) {
        case WM_CREATE:
            srand(KBTickCount());
            SetTimer(window, KBWIN_TIMER_ID, KBWIN_TIMER_INTERVAL, NULL);
            GdiSetBatchLimit(1);
            return 0;
        case WM_KEYDOWN:
        case WM_KEYUP:
            if (!KeyboardMessageHandler(window, message, messageParam, messageData))
                return 0;
            break;
        case WM_MOUSEMOVE:
        case WM_LBUTTONDOWN:
        case WM_LBUTTONUP:
        case WM_LBUTTONDBLCLK:
        case WM_RBUTTONDOWN:
        case WM_RBUTTONUP:
        case WM_RBUTTONDBLCLK:
            if (!MouseMessageHandler(window, message, messageParam, messageData))
                return 0;
            break;
        case WM_TIMER:
            gTempValue = KBTickCount();
            if (gTempValue > gLastGTimerTickCount + KBWIN_TIMER_UPDATE_MIN_INTERVAL)
                gLastGTimerTickCount = gTempValue;
            if (gTempValue > gLastCycleTickCount + KBWIN_CYCLE_INTERVAL) {
                gLastCycleTickCount = gTempValue;
                if (gGraphicsType == WINGRAPH_GRAPHICS_WING
                    && gMainVideoModeColorDepth != WINGRAPH_COLOR_DEPTH) {
                    gLastCycleTickCount += KBWIN_CYCLE_WING_DELAY;
                    if (gHeroMoving)
                        return 0;
                }
                CycleColors();
            }
#ifdef HOMM1_EDITOR
            if (gStatusTextClearTime && gTempValue > gStatusTextClearTime)
                ClearStatusText();
#endif
            return 0;
        case WM_ACTIVATEAPP:
            gForegroundApp = messageParam;
            return 0;
        case WM_ERASEBKGND:
            return 1;
        case WM_MOVE:
            if (gAppWindow == NULL)
                return 0;
            gTempValue = GetWindowLongA(gAppWindow, GWL_STYLE);
            if ((gTempValue & WS_MAXIMIZE) == 0 && (gTempValue & WS_MINIMIZE) == 0 && !gClosingApp
                && CURRENT_GRAPHICS_CONFIG.fullScreen == 0) {
                GetWindowRect(window, &gTempRect);
                CURRENT_GRAPHICS_CONFIG.x = gTempRect.left;
                CURRENT_GRAPHICS_CONFIG.y = gTempRect.top;
                WritePrefs();
            }
            return 0;
        case WM_SIZE:
            if (gAppWindow != NULL) {
                gTempValue = GetWindowLongA(gAppWindow, GWL_STYLE);
                gMinimized = gTempValue & WS_MINIMIZE;
                if ((gTempValue & WS_MINIMIZE) == 0)
                    EarlyResizeWindow(0, 0, 0, 0);
                if ((gTempValue & WS_MAXIMIZE) == 0 && (gTempValue & WS_MINIMIZE) == 0
                    && (LOWORD(messageData) < KBWIN_MIN_WIDTH
                        || HIWORD(messageData) < KBWIN_MIN_HEIGHT)) {
                    gTempX = LOWORD(messageData) < KBWIN_MIN_WIDTH ? KBWIN_MIN_WIDTH
                                                                   : LOWORD(messageData);
                    gTempY = HIWORD(messageData) < KBWIN_MIN_HEIGHT ? KBWIN_MIN_HEIGHT
                                                                    : HIWORD(messageData);
                    ResizeWindow(KBWIN_KEEP_POSITION, KBWIN_KEEP_POSITION, gTempX, gTempY);
                    return 0;
                }
            }
            gMainWinScreenWidth = LOWORD(messageData);
            gMainWinScreenHeight = HIWORD(messageData);
            if (gMainWinScreenWidth < 1)
                gMainWinScreenWidth = 1;
            if (gMainWinScreenHeight < 1)
                gMainWinScreenHeight = 1;
            if (gAppWindow != NULL && (gTempValue & WS_MAXIMIZE) == 0
                && (gTempValue & WS_MINIMIZE) == 0 && !gClosingApp
                && CURRENT_GRAPHICS_CONFIG.fullScreen == 0) {
                CURRENT_GRAPHICS_CONFIG.width = gMainWinScreenWidth;
                CURRENT_GRAPHICS_CONFIG.height = gMainWinScreenHeight;
                WritePrefs();
            }
            return 0;
        case WM_COMMAND:
            return AppCommand(window, message, messageParam, messageData);
        case WM_PALETTECHANGED:
            if (messageParam
                == reinterpret_cast<u32>(window)) // API-forced: WPARAM carries the changing window.
                break;
            // fall through
        case WM_QUERYNEWPALETTE:
            return QueryNewPalette();
        case WM_PAINT:
            AppPaint(window, NULL);
            return 0;
        case WM_CLOSE:
            if (window == gAppWindow) {
                if (GameUnsaved()) {
                    NormalDialog(
                        localization::Tr("adventure.confirm_quit"),
                        NORMAL_DIALOG_TYPE_YES_NO
                    );
                    if (gWindowManager->m_dialogResult == NORMAL_DIALOG_CONFIRM)
                        DestroyWindow(window);
                    return 0;
                }
            }
            // fall through
        case WM_DESTROY:
            gClosingApp = true;
            PostQuitMessage(0);
            // fall through
        case WM_QUIT:
            ShutDown(NULL);
            break;
    }
    return DefWindowProcA(window, message, messageParam, messageData);
}

// About-dialog callback.
VA(0x004436ff, 0x67)
BOOL __stdcall AppAbout(HWND dialog, UINT message, WPARAM messageParam, LPARAM messageData) {
    HWND childWindow;
    i32 commandId;
    WORD notifyCode;
    switch (message) {
        case WM_INITDIALOG:
            return TRUE;
        case WM_COMMAND:
            commandId = messageParam & 0xffff;
            childWindow = reinterpret_cast<HWND>(messageData); // WM_COMMAND passes HWND in LPARAM.
            notifyCode = (messageParam >> 16) & 0xffff;
            if (commandId == IDOK)
                EndDialog(dialog, 1);
            break;
    }
    PollSound();
    return FALSE;
}

DATA(0x004a9e4c)
b32 gClosingApp = false;
// No retail code reads this; it holds its retail .bss place.
DATA(0x004a9e50)
i32 gUnusedWindowCount = 0;

VA(0x00443766, 0xf)
void AppExit(void) {
    CleanUpWinGraphics();
    CleanUpMenus();
}

VA(0x00443775, 0x7f)
void Process1WindowsMessage(void) {
    DATA(0x004a9e54)
    static i32 gLastGetMessage = 0;
    MSG message;
    i32 currentTick;

    while (PeekMessageA(&message, NULL, 0, 0, PM_REMOVE)) {
        TranslateMessage(&message);
        DispatchMessageA(&message);
    }
    currentTick = KBTickCount();
    if (currentTick - gLastGetMessage > 150) {
        gLastGetMessage = currentTick;
        if (GetMessageA(&message, NULL, 0, 0)) {
            TranslateMessage(&message);
            DispatchMessageA(&message);
        }
    }
}

VA(0x004437f4, 0x11d)
void ResizeWindow(i32 x, i32 y, i32 width, i32 height) {
    i32 windowX;
    RECT windowRect;
    i32 targetY;
    if (CURRENT_GRAPHICS_CONFIG.fullScreen != 0)
        return;
    GetWindowRect(gAppWindow, &windowRect);
    windowX = x == KBWIN_KEEP_POSITION ? windowRect.left : x;
    targetY = y == KBWIN_KEEP_POSITION ? windowRect.top : y;
    windowRect.left = 0;
    windowRect.top = 0;
    windowRect.right = width - 1;
    windowRect.bottom = height - 1;
    AdjustWindowRect(&windowRect, gCurWindowsStyleFlags, CURRENT_GRAPHICS_CONFIG.showMenu);
    MoveWindow(
        gAppWindow,
        windowX,
        targetY,
        windowRect.right - windowRect.left + 1,
        windowRect.bottom - windowRect.top + 1,
        TRUE
    );
    CURRENT_GRAPHICS_CONFIG.x = windowX;
    CURRENT_GRAPHICS_CONFIG.y = targetY;
    CURRENT_GRAPHICS_CONFIG.width = width;
    CURRENT_GRAPHICS_CONFIG.height = height;
    WritePrefs();
}

VA(0x00443911, 0x165)
i32 AppCommand(HWND window, u32 message, u32 messageParam, i32 messageData) {
    DLGPROC dialogProc;
    i32 command;

    command = LOWORD(messageParam);
    switch (command) {
        case KBWIN_MENU_ABOUT:
            dialogProc =
                reinterpret_cast<DLGPROC>(AppAbout); // AppAbout is the BOOL dialog procedure.
#ifdef HOMM1_EDITOR
            DialogBoxParamA(gAppInstance, "EDITOR", window, dialogProc, 0);
#else
            DialogBoxParamA(gAppInstance, "HEROES", window, dialogProc, 0);
#endif
            break;
        case KBWIN_MENU_HELP:
            WinHelpA(gAppWindow, ".\\HELP\\HEROES.HLP", HELP_FINDER, 0);
            break;
        case KBWIN_MENU_SIZE_640_480:
            ResizeWindow(
                KBWIN_KEEP_POSITION,
                KBWIN_KEEP_POSITION,
                LOGICAL_SCREEN_WIDTH,
                LOGICAL_SCREEN_HEIGHT
            );
            break;
        case KBWIN_MENU_SIZE_800_600:
            ResizeWindow(
                KBWIN_KEEP_POSITION,
                KBWIN_KEEP_POSITION,
                KBWIN_WIDTH_800,
                KBWIN_HEIGHT_600
            );
            break;
        case KBWIN_MENU_SIZE_1024_768:
            ResizeWindow(
                KBWIN_KEEP_POSITION,
                KBWIN_KEEP_POSITION,
                KBWIN_WIDTH_1024,
                KBWIN_HEIGHT_768
            );
            break;
        case KBWIN_MENU_SIZE_1280_1024:
            ResizeWindow(
                KBWIN_KEEP_POSITION,
                KBWIN_KEEP_POSITION,
                KBWIN_WIDTH_1280,
                KBWIN_HEIGHT_1024
            );
            break;
        case KBWIN_MENU_FULLSCREEN:
            SetFullScreenStatus(1 - CURRENT_GRAPHICS_CONFIG.fullScreen);
            break;
        default:
            return HandleAppSpecificMenuCommands(command);
    }
    return 0;
}

// Disables unsupported window sizes.
VA(0x00443a76, 0xae)
void UpdateDfltMenu(HMENU menu) {
    i32 result;
    i32 value;

    if (CURRENT_GRAPHICS_CONFIG.showMenu == 0)
        return;
    if (gMainVideoModeWidth <= LOGICAL_SCREEN_WIDTH)
        EnableMenuItem(menu, KBWIN_MENU_SIZE_640_480, MF_GRAYED);
    if (gMainVideoModeWidth <= KBWIN_WIDTH_800)
        EnableMenuItem(menu, KBWIN_MENU_SIZE_800_600, MF_GRAYED);
    if (gMainVideoModeWidth <= KBWIN_WIDTH_1024)
        EnableMenuItem(menu, KBWIN_MENU_SIZE_1024_768, MF_GRAYED);
    if (gMainVideoModeWidth <= KBWIN_WIDTH_1280)
        EnableMenuItem(menu, KBWIN_MENU_SIZE_1280_1024, MF_GRAYED);
    if (!gDDrawAttached)
        EnableMenuItem(menu, KBWIN_MENU_FULLSCREEN, MF_GRAYED);
}

VA(0x00443b24, 0x91)
void KBChangeMenu(HMENU menu) {
    if (menu == NULL)
        menu = gCurrentMenu;
    else
        gCurrentMenu = menu;
    gAppMenu = menu;
    if (CURRENT_GRAPHICS_CONFIG.showMenu) {
        if (menu != NULL) {
            SetMenu(gAppWindow, menu);
            UpdateDfltMenu(menu);
            UpdateAppSpecificMenus(menu);
            DrawMenuBar(gAppWindow);
        }
    } else {
        SetMenu(gAppWindow, NULL);
        DrawMenuBar(gAppWindow);
    }
}

VA(0x00443bb5, 0x118)
void SetMenuStatus(i32 showMenu) {
    i32 winWidth;
    i32 height;
    i32 windowStyle;
    i32 replacedStyle;
    if (CURRENT_GRAPHICS_CONFIG.fullScreen && showMenu)
        return;
    winWidth = CURRENT_GRAPHICS_CONFIG.width;
    height = CURRENT_GRAPHICS_CONFIG.height;
    CURRENT_GRAPHICS_CONFIG.showMenu = showMenu;
    KBChangeMenu(NULL);
    CURRENT_GRAPHICS_CONFIG.width = winWidth;
    CURRENT_GRAPHICS_CONFIG.height = height;
    WritePrefs();
    windowStyle = GetWindowLongA(gAppWindow, GWL_STYLE);
    if (CURRENT_GRAPHICS_CONFIG.showMenu)
        gCurWindowsStyleFlags = KBWIN_WINDOWED_STYLE;
    else
        gCurWindowsStyleFlags = KBWIN_FULLSCREEN_STYLE;
    replacedStyle = SetWindowLongA(gAppWindow, GWL_STYLE, gCurWindowsStyleFlags);
    ShowWindow(gAppWindow, SW_SHOWNA);
    ResizeWindow(
        KBWIN_KEEP_POSITION,
        KBWIN_KEEP_POSITION,
        CURRENT_GRAPHICS_CONFIG.width,
        CURRENT_GRAPHICS_CONFIG.height
    );
}

VA(0x00443ccd, 0x52)
void SetNoDialogMenus(i32 menusEnabled) {
    DATA(0x004a9e58)
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

// Recurse into popups, then restore each command from the normal or setup
// enable table.
VA(0x00443d1f, 0x12b)
void SetMenus(HMENU menu, i32 enabled) {
    u32 id;
    i32 candidate;
    i32 index;
    i32 update;
    i32 count;
    i32 matchIndex;

    count = GetMenuItemCount(menu);
    for (index = 0; index < count; index++) {
        id = GetMenuItemID(menu, index);
        if (id == static_cast<u32>(-1)) {
            SetMenus(GetSubMenu(menu, index), enabled);
            update = 0;
        } else {
            update = 0;
            if (enabled) {
                update = 1;
            } else {
                matchIndex = 0;
                for (candidate = 0; candidate < KBWIN_MENU_ENTRY_COUNT; candidate++) {
                    if (gMenuEnableStatus[candidate].command == id)
                        matchIndex = candidate;
                }
                if (gInSetupDialog)
                    update = 1 - gMenuEnableStatus[matchIndex].setupEnabled;
                else
                    update = 1 - gMenuEnableStatus[matchIndex].normalEnabled;
            }
        }
        if (update != 0)
            EnableMenuItem(menu, id, enabled == 0 ? MF_GRAYED : MF_ENABLED);
    }
    UpdateDfltMenu(menu);
}

// Picks the walk speed unconditionally; the old processor/slow-video choice
// is gone.
VA(0x00443e4a, 0x145)
void SetGameDefaults(void) {
    H1_ENUM_LOCAL(ConfigExecutable, i32) i;

    gConfig.musicVolume = SOUND_VOLUME_100;
    gConfig.soundVolume = SOUND_VOLUME_100;
    gConfig.autosave = 1;
    gConfig.showRoute = 1;
    gConfig.blackoutComputer = 0;
    for (i = CONFIG_EXECUTABLE_GAME; i < CONFIG_EXECUTABLE_COUNT; i++) {
        gConfig.gfx[i].showMenu = 1;
        gConfig.gfx[i].x = DEFAULT_WINDOW_ORIGIN;
        gConfig.gfx[i].y = DEFAULT_WINDOW_ORIGIN;
        if (gMainVideoModeWidth <= LOGICAL_SCREEN_WIDTH && gDDrawAttached) {
            gConfig.gfx[i].fullScreen = 1;
            gConfig.gfx[i].width = DEFAULT_SMALL_WINDOW_WIDTH;
            gConfig.gfx[i].height = DEFAULT_SMALL_WINDOW_HEIGHT;
        } else {
            gConfig.gfx[i].fullScreen = 1;
            gConfig.gfx[i].width = LOGICAL_SCREEN_WIDTH;
            gConfig.gfx[i].height = LOGICAL_SCREEN_HEIGHT;
        }
    }
    gConfig.blackoutComputer = 0;
    gConfig.currentMapOffset = 0;
    gConfig.firstMapOffset = Random(0, DEFAULT_MAP_OFFSET_LIMIT);
    gConfig.musicSource = SOUND_MUSIC_SOURCE_CD;
    gFirstTimeThrough = true;
    gConfig.walkSpeed = WALK_SPEED_CANTER;
}

VA(0x00443f8f, 0x468)
void ReadPrefs(void) {
    DWORD length;
    HKEY key;
    char subKey[REGISTRY_TEXT_BUFFER_SIZE];
    i32 rc;
    DWORD regType;
    DWORD volumeProbe;
    DWORD savedMusic;
    DWORD effectsVolume;

    strcpy(subKey, "SOFTWARE\\Buka\\3DO\\Heroes of Might and Magic Platinum\\1.000");
    key = NULL;
    rc = RegCreateKeyA(HKEY_LOCAL_MACHINE, subKey, &key);
    if (rc == ERROR_SUCCESS) {
        length = REGISTRY_DWORD_BYTES;
        if (RegQueryValueExA(
                key,
                "HMM1 MusicVolume",
                NULL,
                &regType,
                reinterpret_cast<LPBYTE>(&volumeProbe),
                &length
            )
            != ERROR_SUCCESS) {
            memset(&gConfig, 0, sizeof(gConfig));
            SetGameDefaults();
            RegCloseKey(key);
            WritePrefs();
            return;
        }
        RegQueryValueExA(
            key,
            "HMM1 MusicVolume",
            NULL,
            &regType,
            reinterpret_cast<LPBYTE>(&savedMusic),
            &length
        );
        RegQueryValueExA(
            key,
            "HMM1 FXVolume",
            NULL,
            &regType,
            reinterpret_cast<LPBYTE>(&effectsVolume),
            &length
        );
        gConfig.musicVolume = savedMusic;
        gConfig.soundVolume = effectsVolume;
        RegQueryValueExA(
            key,
            "HMM1 WalkSpeed",
            NULL,
            &regType,
            reinterpret_cast<LPBYTE>(&gConfig.walkSpeed),
            &length
        );
        RegQueryValueExA(
            key,
            "HMM1 ShowRoute",
            NULL,
            &regType,
            reinterpret_cast<LPBYTE>(&gConfig.showRoute),
            &length
        );
        RegQueryValueExA(
            key,
            "HMM1 BlackoutComputer",
            NULL,
            &regType,
            reinterpret_cast<LPBYTE>(&gConfig.blackoutComputer),
            &length
        );
        RegQueryValueExA(
            key,
            "HMM1 SoundQuality",
            NULL,
            &regType,
            reinterpret_cast<LPBYTE>(&gConfig.musicSource),
            &length
        );
        RegQueryValueExA(
            key,
            "HMM1 DirectConnectComPort",
            NULL,
            &regType,
            reinterpret_cast<LPBYTE>(&gConfig.comPort[CONFIG_CONNECTION_DIRECT]),
            &length
        );
        RegQueryValueExA(
            key,
            "HMM1 DirectConnectBaudRate",
            NULL,
            &regType,
            reinterpret_cast<LPBYTE>(&gConfig.baudRate[CONFIG_CONNECTION_DIRECT]),
            &length
        );
        RegQueryValueExA(
            key,
            "HMM1 ModemComPort",
            NULL,
            &regType,
            reinterpret_cast<LPBYTE>(&gConfig.comPort[CONFIG_CONNECTION_MODEM]),
            &length
        );
        RegQueryValueExA(
            key,
            "HMM1 ModemBaudRate",
            NULL,
            &regType,
            reinterpret_cast<LPBYTE>(&gConfig.baudRate[CONFIG_CONNECTION_MODEM]),
            &length
        );
        length = REGISTRY_TEXT_VALUE_SIZE;
        RegQueryValueExA(
            key,
            "HMM1 ModemInitString",
            NULL,
            &regType,
            reinterpret_cast<LPBYTE>(gConfig.modemInitString),
            &length
        );
        length = REGISTRY_DWORD_BYTES;
        RegQueryValueExA(
            key,
            "HMM1 UseAutosave",
            NULL,
            &regType,
            reinterpret_cast<LPBYTE>(&gConfig.autosave),
            &length
        );
        RegQueryValueExA(
            key,
            "HMM1 FirstMapOffset",
            NULL,
            &regType,
            reinterpret_cast<LPBYTE>(&gConfig.firstMapOffset),
            &length
        );
        RegQueryValueExA(
            key,
            "HMM1 CurrentMapOffset",
            NULL,
            &regType,
            reinterpret_cast<LPBYTE>(&gConfig.currentMapOffset),
            &length
        );
        RegQueryValueExA(
            key,
            "HMM1 GameShowMenu",
            NULL,
            &regType,
            reinterpret_cast<LPBYTE>(&gConfig.gfx[CONFIG_EXECUTABLE_GAME].showMenu),
            &length
        );
        RegQueryValueExA(
            key,
            "HMM1 GameWindowXLeft",
            NULL,
            &regType,
            reinterpret_cast<LPBYTE>(&gConfig.gfx[CONFIG_EXECUTABLE_GAME].x),
            &length
        );
        RegQueryValueExA(
            key,
            "HMM1 GameWindowYTop",
            NULL,
            &regType,
            reinterpret_cast<LPBYTE>(&gConfig.gfx[CONFIG_EXECUTABLE_GAME].y),
            &length
        );
        RegQueryValueExA(
            key,
            "HMM1 GameWindowWidth",
            NULL,
            &regType,
            reinterpret_cast<LPBYTE>(&gConfig.gfx[CONFIG_EXECUTABLE_GAME].width),
            &length
        );
        RegQueryValueExA(
            key,
            "HMM1 GameWindowHeight",
            NULL,
            &regType,
            reinterpret_cast<LPBYTE>(&gConfig.gfx[CONFIG_EXECUTABLE_GAME].height),
            &length
        );
        RegQueryValueExA(
            key,
            "HMM1 GameFullScreen",
            NULL,
            &regType,
            reinterpret_cast<LPBYTE>(&gConfig.gfx[CONFIG_EXECUTABLE_GAME].fullScreen),
            &length
        );
        RegQueryValueExA(
            key,
            "HMM1 EditorShowMenu",
            NULL,
            &regType,
            reinterpret_cast<LPBYTE>(&gConfig.gfx[CONFIG_EXECUTABLE_EDITOR].showMenu),
            &length
        );
        RegQueryValueExA(
            key,
            "HMM1 EditorWindowXLeft",
            NULL,
            &regType,
            reinterpret_cast<LPBYTE>(&gConfig.gfx[CONFIG_EXECUTABLE_EDITOR].x),
            &length
        );
        RegQueryValueExA(
            key,
            "HMM1 EditorWindowYTop",
            NULL,
            &regType,
            reinterpret_cast<LPBYTE>(&gConfig.gfx[CONFIG_EXECUTABLE_EDITOR].y),
            &length
        );
        RegQueryValueExA(
            key,
            "HMM1 EditorWindowWidth",
            NULL,
            &regType,
            reinterpret_cast<LPBYTE>(&gConfig.gfx[CONFIG_EXECUTABLE_EDITOR].width),
            &length
        );
        RegQueryValueExA(
            key,
            "HMM1 EditorWindowHeight",
            NULL,
            &regType,
            reinterpret_cast<LPBYTE>(&gConfig.gfx[CONFIG_EXECUTABLE_EDITOR].height),
            &length
        );
        RegQueryValueExA(
            key,
            "HMM1 EditorFullScreen",
            NULL,
            &regType,
            reinterpret_cast<LPBYTE>(&gConfig.gfx[CONFIG_EXECUTABLE_EDITOR].fullScreen),
            &length
        );
        length = REGISTRY_TEXT_VALUE_SIZE;
        if (RegQueryValueExA(
                key,
                "AppPath",
                NULL,
                &regType,
                reinterpret_cast<LPBYTE>(gRegAppPath),
                &length
            )
            != ERROR_SUCCESS)
            strcpy(gRegAppPath, "");
        if (RegQueryValueExA(
                key,
                "HMM1 CDDrive",
                NULL,
                &regType,
                reinterpret_cast<LPBYTE>(gRegCDRomPath),
                &length
            )
            != ERROR_SUCCESS)
            strcpy(gRegCDRomPath, "");
        RegCloseKey(key);
        SetVolumes(gConfig.soundVolume, gConfig.musicVolume);
        SetMusicSource(gConfig.musicSource != SOUND_MUSIC_SOURCE_DIGITAL);
    }
}

VA(0x004443f7, 0x30b)
void WritePrefs(void) {
    HKEY key;
    char subKey[REGISTRY_TEXT_BUFFER_SIZE];
    i32 rc;
    DWORD savedMusic;
    DWORD effectsVolume;

    UpdateSystemOptionsMenu();
    strcpy(subKey, "SOFTWARE\\Buka\\3DO\\Heroes of Might and Magic Platinum\\1.000");
    key = NULL;
    rc = RegOpenKeyExA(HKEY_LOCAL_MACHINE, subKey, 0, KEY_ALL_ACCESS, &key);
    if (rc == ERROR_SUCCESS) {
        effectsVolume = gConfig.soundVolume;
        savedMusic = gConfig.musicVolume;
        RegSetValueExA(
            key,
            "HMM1 MusicVolume",
            0,
            REG_DWORD,
            reinterpret_cast<LPBYTE>(&savedMusic),
            REGISTRY_DWORD_BYTES
        );
        RegSetValueExA(
            key,
            "HMM1 FXVolume",
            0,
            REG_DWORD,
            reinterpret_cast<LPBYTE>(&effectsVolume),
            REGISTRY_DWORD_BYTES
        );
        RegSetValueExA(
            key,
            "HMM1 WalkSpeed",
            0,
            REG_DWORD,
            reinterpret_cast<LPBYTE>(&gConfig.walkSpeed),
            REGISTRY_DWORD_BYTES
        );
        RegSetValueExA(
            key,
            "HMM1 ShowRoute",
            0,
            REG_DWORD,
            reinterpret_cast<LPBYTE>(&gConfig.showRoute),
            REGISTRY_DWORD_BYTES
        );
        RegSetValueExA(
            key,
            "HMM1 BlackoutComputer",
            0,
            REG_DWORD,
            reinterpret_cast<LPBYTE>(&gConfig.blackoutComputer),
            REGISTRY_DWORD_BYTES
        );
        RegSetValueExA(
            key,
            "HMM1 SoundQuality",
            0,
            REG_DWORD,
            reinterpret_cast<LPBYTE>(&gConfig.musicSource),
            REGISTRY_DWORD_BYTES
        );
        RegSetValueExA(
            key,
            "HMM1 DirectConnectComPort",
            0,
            REG_DWORD,
            reinterpret_cast<LPBYTE>(&gConfig.comPort[CONFIG_CONNECTION_DIRECT]),
            REGISTRY_DWORD_BYTES
        );
        RegSetValueExA(
            key,
            "HMM1 DirectConnectBaudRate",
            0,
            REG_DWORD,
            reinterpret_cast<LPBYTE>(&gConfig.baudRate[CONFIG_CONNECTION_DIRECT]),
            REGISTRY_DWORD_BYTES
        );
        RegSetValueExA(
            key,
            "HMM1 ModemComPort",
            0,
            REG_DWORD,
            reinterpret_cast<LPBYTE>(&gConfig.comPort[CONFIG_CONNECTION_MODEM]),
            REGISTRY_DWORD_BYTES
        );
        RegSetValueExA(
            key,
            "HMM1 ModemBaudRate",
            0,
            REG_DWORD,
            reinterpret_cast<LPBYTE>(&gConfig.baudRate[CONFIG_CONNECTION_MODEM]),
            REGISTRY_DWORD_BYTES
        );
        RegSetValueExA(
            key,
            "HMM1 ModemInitString",
            0,
            REG_SZ,
            reinterpret_cast<LPBYTE>(gConfig.modemInitString),
            REGISTRY_DWORD_BYTES
        );
        RegSetValueExA(
            key,
            "HMM1 UseAutosave",
            0,
            REG_DWORD,
            reinterpret_cast<LPBYTE>(&gConfig.autosave),
            REGISTRY_DWORD_BYTES
        );
        RegSetValueExA(
            key,
            "HMM1 FirstMapOffset",
            0,
            REG_DWORD,
            reinterpret_cast<LPBYTE>(&gConfig.firstMapOffset),
            REGISTRY_DWORD_BYTES
        );
        RegSetValueExA(
            key,
            "HMM1 CurrentMapOffset",
            0,
            REG_DWORD,
            reinterpret_cast<LPBYTE>(&gConfig.currentMapOffset),
            REGISTRY_DWORD_BYTES
        );
        RegSetValueExA(
            key,
            "HMM1 GameShowMenu",
            0,
            REG_DWORD,
            reinterpret_cast<LPBYTE>(&gConfig.gfx[CONFIG_EXECUTABLE_GAME].showMenu),
            REGISTRY_DWORD_BYTES
        );
        RegSetValueExA(
            key,
            "HMM1 GameWindowXLeft",
            0,
            REG_DWORD,
            reinterpret_cast<LPBYTE>(&gConfig.gfx[CONFIG_EXECUTABLE_GAME].x),
            REGISTRY_DWORD_BYTES
        );
        RegSetValueExA(
            key,
            "HMM1 GameWindowYTop",
            0,
            REG_DWORD,
            reinterpret_cast<LPBYTE>(&gConfig.gfx[CONFIG_EXECUTABLE_GAME].y),
            REGISTRY_DWORD_BYTES
        );
        RegSetValueExA(
            key,
            "HMM1 GameWindowWidth",
            0,
            REG_DWORD,
            reinterpret_cast<LPBYTE>(&gConfig.gfx[CONFIG_EXECUTABLE_GAME].width),
            REGISTRY_DWORD_BYTES
        );
        RegSetValueExA(
            key,
            "HMM1 GameWindowHeight",
            0,
            REG_DWORD,
            reinterpret_cast<LPBYTE>(&gConfig.gfx[CONFIG_EXECUTABLE_GAME].height),
            REGISTRY_DWORD_BYTES
        );
        RegSetValueExA(
            key,
            "HMM1 GameFullScreen",
            0,
            REG_DWORD,
            reinterpret_cast<LPBYTE>(&gConfig.gfx[CONFIG_EXECUTABLE_GAME].fullScreen),
            REGISTRY_DWORD_BYTES
        );
        RegSetValueExA(
            key,
            "HMM1 EditorShowMenu",
            0,
            REG_DWORD,
            reinterpret_cast<LPBYTE>(&gConfig.gfx[CONFIG_EXECUTABLE_EDITOR].showMenu),
            REGISTRY_DWORD_BYTES
        );
        RegSetValueExA(
            key,
            "HMM1 EditorWindowXLeft",
            0,
            REG_DWORD,
            reinterpret_cast<LPBYTE>(&gConfig.gfx[CONFIG_EXECUTABLE_EDITOR].x),
            REGISTRY_DWORD_BYTES
        );
        RegSetValueExA(
            key,
            "HMM1 EditorWindowYTop",
            0,
            REG_DWORD,
            reinterpret_cast<LPBYTE>(&gConfig.gfx[CONFIG_EXECUTABLE_EDITOR].y),
            REGISTRY_DWORD_BYTES
        );
        RegSetValueExA(
            key,
            "HMM1 EditorWindowWidth",
            0,
            REG_DWORD,
            reinterpret_cast<LPBYTE>(&gConfig.gfx[CONFIG_EXECUTABLE_EDITOR].width),
            REGISTRY_DWORD_BYTES
        );
        RegSetValueExA(
            key,
            "HMM1 EditorWindowHeight",
            0,
            REG_DWORD,
            reinterpret_cast<LPBYTE>(&gConfig.gfx[CONFIG_EXECUTABLE_EDITOR].height),
            REGISTRY_DWORD_BYTES
        );
        RegSetValueExA(
            key,
            "HMM1 EditorFullScreen",
            0,
            REG_DWORD,
            reinterpret_cast<LPBYTE>(&gConfig.gfx[CONFIG_EXECUTABLE_EDITOR].fullScreen),
            REGISTRY_DWORD_BYTES
        );
        RegCloseKey(key);
    }
}

DATA(0x0049e700)
#ifdef HOMM1_EDITOR
char gAppName[] = localization::Tr("editor.window.app_name");
#else
char gAppName[] = localization::Tr("window.gAppName");
#endif
DATA(0x0049e708)
#ifdef HOMM1_EDITOR
char gTitle[] = localization::Tr("editor.window.title");
#else
char gTitle[] = localization::Tr("window.gTitle");
#endif
// No retail code reads this value; it sits between gTitle and gCDTrackName.
DATA(0x0049e71c)
i32 gUnusedWindowValue = -1;
// This path deliberately has no leading slash.
DATA(0x0049e720)
static char* gCDTrackName = "Tracks\\02-AudioTrack 02.ogg";

// Suppress the system's critical-error dialog while probing an empty drive.
VA(0x00444702, 0x72)
bool DriveSupportsFreeSpaceQuery(char driveLetter) {
    UINT oldMode;
    char path[CD_DRIVE_QUERY_PATH_SIZE];
    ULARGE_INTEGER availableToCaller;
    ULARGE_INTEGER total;
    ULARGE_INTEGER freeBytes;

    wsprintfA(path, "%c:", driveLetter);
    oldMode = SetErrorMode(SEM_FAILCRITICALERRORS);
    if (GetDiskFreeSpaceExA(path, &availableToCaller, &total, &freeBytes)) {
        SetErrorMode(oldMode);
        return true;
    } else {
        SetErrorMode(oldMode);
        return false;
    }
}

// The disc probe now checks an Ogg track; it no longer opens an MCI CD device.
#define key activeKeyVal // frame-slot spelling
VA(0x00444774, 0x36f)
H1_ENUM_RETURN(CdSetupResult, i32) SetupCDDrive(void) {
    HKEY key;
    char keyPath[REGISTRY_TEXT_BUFFER_SIZE];
    i32 tailResult;
    i32 drivesCount;
    char endBuffer[CD_PROBE_BUFFER_SIZE];
    i32 probeFd;
    i32 index;
    i32 cdDrives[CD_DRIVE_LETTER_COUNT];
    i32 eachCd;
    u32 unusedErr;
    u32 logicalDrives;

    sprintf(gText, "%sHEROES.AGG", gDataPath);
    probeFd = open(gText, _O_BINARY);
    if (probeFd == FILE_DESCRIPTOR_INVALID) {
        if (_chdir(gRegAppPath) == -1)
            return CD_SETUP_NO_APP_PATH;
        probeFd = open(gText, _O_BINARY);
        if (probeFd == FILE_DESCRIPTOR_INVALID)
            return CD_SETUP_NO_DATA;
    }
    close(probeFd);
    logicalDrives = GetLogicalDrives();
    // Clears only 26 bytes, although the drive slots are 32-bit integers.
    memset(cdDrives, 0, CD_DRIVE_LETTER_COUNT);
    for (eachCd = CD_FIRST_DRIVE_LETTER, index = 0; eachCd < CD_DRIVE_LETTER_COUNT; eachCd++) {
        if (logicalDrives & (1 << eachCd)) {
            if (IsCDDrive(eachCd)) {
                cdDrives[index] = eachCd;
                index++;
            }
        }
    }
    drivesCount = index;
    if (strlen(gRegCDRomPath) > 0 && gRegCDRomPath[0] >= 'A' && gRegCDRomPath[0] <= 'Z'
        && DriveSupportsFreeSpaceQuery(gRegCDRomPath[0])) {
        sprintf(gText, "%s%s", gRegCDRomPath, gCDTrackName);
        probeFd = open(gText, _O_BINARY);
        if (probeFd != FILE_DESCRIPTOR_INVALID) {
            close(probeFd);
            return CD_SETUP_READY;
        }
    }
    if (drivesCount <= 0)
        return CD_SETUP_NO_DRIVE;
    for (eachCd = 0; eachCd < CD_SETUP_ATTEMPTS; eachCd++) {
        for (index = 0; index < drivesCount; index++) {
            if (DriveSupportsFreeSpaceQuery(cdDrives[index] + 'A')) {
                sprintf(gText, "%c:%s", cdDrives[index] + 'A', gCDTrackName);
                probeFd = open(gText, _O_BINARY);
                if (probeFd == FILE_DESCRIPTOR_INVALID)
                    continue;
                tailResult = _lseek(probeFd, 0, SEEK_END);
                if (tailResult != -1) {
                    tailResult = _lseek(probeFd, -CD_AUTORUN_TAIL_BYTES, SEEK_CUR);
                    if (tailResult != -1)
                        tailResult = read(probeFd, endBuffer, CD_AUTORUN_TAIL_BYTES);
                }
                close(probeFd);
                if (tailResult != -1) {
                    sprintf(gRegCDRomPath, "%c:", cdDrives[index] + 'A');
                    strcpy(
                        keyPath,
                        "SOFTWARE\\Buka\\3DO\\Heroes of Might and Magic Platinum\\1.000"
                    );
                    key = NULL;
                    if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, keyPath, 0, KEY_WRITE, &key)
                        == ERROR_SUCCESS) {
                        RegSetValueExA(
                            key,
                            "HMM1 CDDrive",
                            0,
                            REG_SZ,
                            reinterpret_cast<LPBYTE>(gRegCDRomPath),
                            strlen(gRegCDRomPath) + 1
                        );
                        RegCloseKey(key);
                    }
                    return CD_SETUP_READY;
                }
            }
        }
        Sleep(CD_SETUP_RETRY_DELAY);
    }
    return CD_SETUP_NOT_FOUND;
}
#undef key

VA(0x00444ae3, 0x6b)
void SetWinText(heroWindow* window, H1_ENUM_PARAM(WindowTextId, i16) id) {
    i32 i;
    tag_message msg;
#ifdef HOMM1_EDITOR
    for (i = 0; i < WINDOW_TEXT_EDITOR_ENTRY_COUNT; i++) {
#else
    for (i = 0; i < WINDOW_TEXT_ENTRY_COUNT; i++) {
#endif
        if (gWinSetup[i].windowId == H1_ENUM_ENCODE(WindowTextId, id)) {
            SET_WIDGET_MESSAGE(msg, WIDGET_COMMAND_SET_TEXT, gWinSetup[i].widgetId);
            msg.text = gWinSetupText[i];
            window->BroadcastMessage(msg);
        }
    }
}

VA(0x00444b4e, 0xb)
i32 KBTickCount(void) {
    return GetTickCount();
}

VA(0x00444b59, 0x55)
void ProcessAssert(i32 condition, char* file, i32 line) {
    i32 unusedAssertWord;
    if (condition == 0) {
        sprintf(gText, "Assert statement failed in module %s, line %d.", file, line);
        MessageBoxA(gAppWindow, gText, "Assert Failure", MB_ICONHAND);
        unusedAssertWord = 0;
        ShutDown(gText);
    }
}

VA(0x00444bae, 0x50)
char* FindToken(char* text, char token) {
    i32 pos;
    i32 len;

    len = strlen(text);
    for (pos = 0; pos < len; pos++) {
        if (text[pos] == token)
            return text + pos;
    }
    return NULL;
}

VA(0x00444bfe, 0x50)
char* FindLastToken(char* text, char token) {
    i32 pos;
    i32 len;

    len = strlen(text);
    for (pos = len - 1; pos >= 0; pos--) {
        if (text[pos] == token)
            return text + pos;
    }
    return NULL;
}

DATA(0x004a99e8)
HINSTANCE gAppInstance;
DATA(0x004a99c8)
struct tagRECT gTempRect;
DATA(0x004a99dc)
i32 gMainWinScreenHeight;
DATA(0x004a9dec)
HMENU gCurrentMenu;
DATA(0x004a99e0)
i32 gTempX;
DATA(0x004a99e4)
i32 gTempY;
DATA(0x004a99d8)
i32 gTempValue;
DATA(0x004a99ec)
u8 gProcessMessage[KBWIN_MESSAGE_FILTER_SIZE];
DATA(0x004a9df4)
char gCommandLine[KBWIN_COMMAND_LINE_CLEAR_SIZE];
DATA(0x004a9df0)
i32 gMainWinScreenWidth;

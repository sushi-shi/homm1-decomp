// Located from HoMM2 Buka 2.1; PoL 2.0 supplies the VC4 declaration.

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

// donor PoL RVA 0x0001bce0; preferred Buka symbol _WinMain@16
// donor Buka TU SOURCE/kbwin; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.651055;margin=0.328202;shape=0.658;size=0.820;calls=1.000;alternate=pol20:_WinMain@16@0x0001bce0
VA(0x00442ca0, 0x11a)
H1_C_LINKAGE i32 __stdcall
WinMain(HINSTANCE instance, HINSTANCE previousInstance, char* commandLine, i32 showCommand) {
    DWORD error;
    MSG message;

    hInstApp = instance;
    gEventHandle = CreateEventA(NULL, FALSE, FALSE, localization::Tr("startup.instance.event_name"));
    error = GetLastError();
    if (gEventHandle == NULL || error == ERROR_ALREADY_EXISTS) {
        sprintf(gText, localization::Tr("startup.instance.already_running"), localization::Tr("startup.instance.game_title"));
        MessageBoxA(NULL, gText, localization::Tr("startup.error.title"), MB_ICONHAND);
        return 0;
    }

    memset(gCommandLine, 0, KBWIN_COMMAND_LINE_CLEAR_SIZE);
    strncpy(gCommandLine, commandLine, KBWIN_COMMAND_LINE_LIMIT);
    if (EarlySetup() == 0)
        return 0;
    if (AppInit(instance, previousInstance, showCommand, commandLine) == 0)
        return 0;

    for (;;) {
        if (PeekMessageA(&message, NULL, 0, 0, PM_REMOVE) != FALSE) {
            if (message.message == WM_QUIT)
                break;
            TranslateMessage(&message);
            DispatchMessageA(&message);
        } else {
            if (AppIdle() != 0)
                WaitMessage();
        }
    }
    ShutDown(NULL);
    return static_cast<i32>(message.wParam);
}

// donor PoL RVA 0x0001be26; preferred Buka symbol ?AppInit@@YIHPAX0HPAD@Z
// donor Buka TU SOURCE/kbwin; HoMM1 owner inferred from contiguous order
// evidence: graph:1;base=0.682496;margin=0.205177;shape=0.345;size=0.971;calls=0.867;strings=Heroes|hInstApp;alternate=pol20:int AppInit(void *, void *, int, char *)@0x0001be26
VA(0x00442dba, 0x28e)
BOOL AppInit(void* instance, void* previousInstance, i32 showCommand, char* commandLine) {
    WNDCLASSA appClass;
    HMENU windowMenu;
    RECT rc;

    LogInt("hInstApp", reinterpret_cast<i32>(hInstApp)); // API-forced handle value.
    memset(bProcessMessage, 0, KBWIN_MESSAGE_FILTER_SIZE);
    bProcessMessage[WM_CREATE] = 1;
    bProcessMessage[WM_KEYDOWN] = 1;
    bProcessMessage[WM_KEYUP] = 1;
    bProcessMessage[WM_MOUSEMOVE] = 1;
    bProcessMessage[WM_LBUTTONDOWN] = 1;
    bProcessMessage[WM_LBUTTONDBLCLK] = 1;
    bProcessMessage[WM_RBUTTONDOWN] = 1;
    bProcessMessage[WM_RBUTTONDBLCLK] = 1;
    bProcessMessage[WM_LBUTTONUP] = 1;
    bProcessMessage[WM_RBUTTONUP] = 1;
    bProcessMessage[WM_TIMER] = 1;
    bProcessMessage[WM_ACTIVATEAPP] = 1;
    bProcessMessage[WM_ERASEBKGND] = 1;
    bProcessMessage[WM_MOVE] = 1;
    bProcessMessage[WM_SIZE] = 1;
    bProcessMessage[WM_COMMAND] = 1;
    bProcessMessage[WM_PALETTECHANGED] = 1;
    bProcessMessage[WM_QUERYNEWPALETTE] = 1;
    bProcessMessage[WM_PAINT] = 1;
    bProcessMessage[WM_DESTROY] = 1;
    bProcessMessage[WM_QUIT] = 1;
    bProcessMessage[WM_CLOSE] = 1;
    bProcessMessage[MM_MCINOTIFY] = 1;

    if (previousInstance == NULL) {
        appClass.hCursor = NULL;
        appClass.hIcon = LoadIconA(static_cast<HINSTANCE>(instance), "Heroes");
        appClass.lpszMenuName = NULL;
        appClass.lpszClassName = gAppName;
        appClass.hbrBackground =
            reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1); // Win32 system-color brush encoding.
        appClass.hInstance = static_cast<HINSTANCE>(instance);
        appClass.style = KBWIN_CLASS_STYLE;
        appClass.lpfnWndProc = reinterpret_cast<WNDPROC>(
            AppWndProc
        ); // HoMM1 declares the procedure with void* handles.
        appClass.cbWndExtra = 0;
        appClass.cbClsExtra = 0;
        if (RegisterClassA(&appClass) == 0)
            return FALSE;
    }

    if (gConfig.gfx[gCurExe].showMenu != 0)
        giCurWindowsStyleFlags = KBWIN_WINDOWED_STYLE;
    else
        giCurWindowsStyleFlags = KBWIN_FULLSCREEN_STYLE;
    rc.left = rc.top = 0;
    rc.right = gConfig.gfx[gCurExe].width - 1;
    rc.bottom = gConfig.gfx[gCurExe].height - 1;
    AdjustWindowRect(&rc, giCurWindowsStyleFlags, gConfig.gfx[gCurExe].showMenu);
    if (gConfig.gfx[gCurExe].showMenu != 0)
        windowMenu = static_cast<HMENU>(hmnuDflt);
    else
        windowMenu = NULL;
    hwndApp = CreateWindowExA(
        0,
        gAppName,
        gTitle,
        giCurWindowsStyleFlags,
        gConfig.gfx[gCurExe].x,
        gConfig.gfx[gCurExe].y,
        rc.right - rc.left + 1,
        rc.bottom - rc.top + 1,
        NULL,
        windowMenu,
        static_cast<HINSTANCE>(instance),
        NULL
    );
    if (hwndApp != NULL) {
        ShowWindow(static_cast<HWND>(hwndApp), showCommand);
        SetWindowLongA(static_cast<HWND>(hwndApp), GWL_STYLE, giCurWindowsStyleFlags);
        if (gConfig.gfx[gCurExe].showMenu == 0)
            SetMenuStatus(0);
        InitGraphics();
        SetCursor(LoadCursorA(NULL, IDC_ARROW));
        oldmain();
        return TRUE;
    } else {
        return FALSE;
    }
}

// PoL 2.0 AppIdle correspondence: both foreground states report idle work.
VA(0x00443048, 0xa)
BOOL AppIdle(void) {
    if (gForegroundApp != 0)
        return TRUE;
    else
        return TRUE;
}

// donor PoL RVA 0x0001c190; preferred Buka symbol ?AppWndProc@@YGJPAXIIJ@Z
// donor Buka TU SOURCE/kbwin; HoMM1 owner inferred from contiguous order
// evidence: graph:3;base=0.508573;margin=0.535153;shape=0.364;size=0.977;calls=0.857;alternate=pol20:long int AppWndProc(void *, unsigned int, unsigned int, long int)@0x0001c190
VA(0x00443052, 0x6ad)
long __stdcall AppWndProc(void* window, u32 message, u32 messageParam, long messageData) {
    DATA(0x0049f850)
    static i32 gLastCycleTickCount = 0;
    if (giDebugLevel == KBWIN_TRACE_DEBUG_LEVEL)
        LogStr(
            "AWP",
            KBTickCount() % KBWIN_TRACE_TICK_MODULUS / KBWIN_TRACE_TICK_DIVISOR,
            reinterpret_cast<i32>(window),
            message,
            messageParam,
            messageData
        ); // API-forced: LogStr logs the handle as a long.
    if (message > KBWIN_PROCESS_MESSAGE_MAX || bProcessMessage[message] == 0)
        return DefWindowProcA(static_cast<HWND>(window), message, messageParam, messageData);

    switch (message) {
        case WM_CREATE:
            srand(KBTickCount());
            SetTimer(static_cast<HWND>(window), KBWIN_TIMER_ID, KBWIN_TIMER_INTERVAL, NULL);
            GdiSetBatchLimit(1);
            return 0;
        case WM_KEYDOWN:
        case WM_KEYUP:
            if (KeyboardMessageHandler(window, message, messageParam, messageData) == 0)
                return 0;
            break;
        case WM_MOUSEMOVE:
        case WM_LBUTTONDOWN:
        case WM_LBUTTONUP:
        case WM_LBUTTONDBLCLK:
        case WM_RBUTTONDOWN:
        case WM_RBUTTONUP:
        case WM_RBUTTONDBLCLK:
            if (MouseMessageHandler(window, message, messageParam, messageData) == 0)
                return 0;
            break;
        case WM_TIMER:
            lTemp = KBTickCount();
            if (gLastCycleTickCount + KBWIN_CYCLE_INTERVAL < lTemp) {
                gLastCycleTickCount = lTemp;
                if (gGraphicsType == WINGRAPH_GRAPHICS_WING
                    && gMainVideoModeColorDepth != WINGRAPH_COLOR_DEPTH) {
                    gLastCycleTickCount += KBWIN_CYCLE_WING_DELAY;
                    if (gHeroMoving)
                        return 0;
                }
                CycleColors();
            }
            return 0;
        case WM_ACTIVATEAPP:
            gForegroundApp = messageParam;
            return 0;
        case WM_ERASEBKGND:
            return 1;
        case WM_MOVE:
            if (hwndApp == NULL)
                return 0;
            lTemp = GetWindowLongA(static_cast<HWND>(hwndApp), GWL_STYLE);
            if ((lTemp & (WS_MINIMIZE | WS_MAXIMIZE)) == 0 && gClosingApp == 0
                && gConfig.gfx[gCurExe].fullScreen == 0) {
                GetWindowRect(static_cast<HWND>(window), &rcTemp);
                gConfig.gfx[gCurExe].x = rcTemp.left;
                gConfig.gfx[gCurExe].y = rcTemp.top;
                WritePrefs();
            }
            return 0;
        case WM_SIZE:
            if (hwndApp != NULL) {
                lTemp = GetWindowLongA(static_cast<HWND>(hwndApp), GWL_STYLE);
                gMinimized = lTemp & WS_MINIMIZE;
                if ((lTemp & WS_MINIMIZE) == 0)
                    EarlyResizeWindow(0, 0, 0, 0);
                if ((lTemp & (WS_MINIMIZE | WS_MAXIMIZE)) == 0
                    && (LOWORD(messageData) < KBWIN_MIN_WIDTH
                        || HIWORD(messageData) < KBWIN_MIN_HEIGHT)) {
                    gTempX = LOWORD(messageData) > KBWIN_MIN_WIDTH ? LOWORD(messageData)
                                                                   : KBWIN_MIN_WIDTH;
                    iTempY = HIWORD(messageData) > KBWIN_MIN_HEIGHT ? HIWORD(messageData)
                                                                    : KBWIN_MIN_HEIGHT;
                    ResizeWindow(KBWIN_KEEP_POSITION, KBWIN_KEEP_POSITION, gTempX, iTempY);
                    return 0;
                }
            }
            iMainWinScreenWidth = LOWORD(messageData);
            gMainWinScreenHeight = HIWORD(messageData);
            if (iMainWinScreenWidth < 1)
                iMainWinScreenWidth = 1;
            if (gMainWinScreenHeight < 1)
                gMainWinScreenHeight = 1;
            if (hwndApp != NULL && (lTemp & (WS_MINIMIZE | WS_MAXIMIZE)) == 0 && gClosingApp == 0
                && gConfig.gfx[gCurExe].fullScreen == 0) {
                gConfig.gfx[gCurExe].width = iMainWinScreenWidth;
                gConfig.gfx[gCurExe].height = gMainWinScreenHeight;
                WritePrefs();
            }
            return 0;
        case WM_COMMAND:
            return AppCommand(window, message, messageParam, messageData);
        case WM_PALETTECHANGED:
            if (reinterpret_cast<u32>(window)
                == messageParam) // API-forced: WPARAM carries the changing window.
                break;
        case WM_QUERYNEWPALETTE:
            return QueryNewPalette();
        case WM_PAINT:
            AppPaint(window, NULL);
            return 0;
        case WM_CLOSE:
            if (window == hwndApp) {
                if (GameUnsaved() != 0) {
                    NormalDialog(
                        "Are you sure you want to quit?",
                        NORMAL_DIALOG_TYPE_YES_NO,
                        -1,
                        -1,
                        NORMAL_DIALOG_NO_RESOURCE,
                        0,
                        NORMAL_DIALOG_NO_RESOURCE,
                        0,
                        NORMAL_DIALOG_NO_OR_TEXT
                    );
                    if (gpWindowManager->m_dialogResult == NORMAL_DIALOG_CONFIRM)
                        DestroyWindow(static_cast<HWND>(window));
                    return 0;
                }
            }
        case WM_DESTROY:
            gClosingApp = 1;
            PostQuitMessage(0);
        case WM_QUIT:
            ShutDown(NULL);
            break;
    }
    return DefWindowProcA(static_cast<HWND>(window), message, messageParam, messageData);
}

// About-dialog callback; 1.2 does not export this function.
// Extent: entry through ret 16 at 0x45c1e9; next function starts at 0x45c1ec.
extern "C" VA(0x004436ff, 0x67)
BOOL __stdcall AppAbout(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {
    i32 wmId;
    WORD codeNotify;
    HWND hwndCtl;
    switch (message) {
        case WM_INITDIALOG:
            return TRUE;
        case WM_COMMAND:
            wmId = wParam & 0xffff;
            hwndCtl = reinterpret_cast<HWND>(lParam); // WM_COMMAND passes HWND in LPARAM.
            codeNotify = (wParam >> 16) & 0xffff;
            if (wmId == IDOK)
                EndDialog(hDlg, 1);
            break;
    }
    PollSound();
    return FALSE;
}

VA(0x00443766, 0xf)
void AppExit(void) {
    CleanUpWinGraphics();
    CleanUpMenus();
}

// donor PoL RVA 0x0001c7b8; preferred Buka symbol ?Process1WindowsMessage@@YIXXZ
// donor Buka TU SOURCE/kbwin; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.631126;margin=0.664983;shape=0.634;size=0.797;calls=1.000;alternate=pol20:void Process1WindowsMessage(void)@0x0001c7b8
VA(0x00443775, 0x7f)
void Process1WindowsMessage(void) {
    DATA(0x004a9e54)
    static i32 gLastGetMessage = 0;
    MSG message;
    i32 currentTick;

    while (PeekMessageA(&message, NULL, 0, 0, PM_REMOVE) != FALSE) {
        TranslateMessage(&message);
        DispatchMessageA(&message);
    }
    currentTick = KBTickCount();
    if (currentTick - gLastGetMessage > 150) {
        gLastGetMessage = currentTick;
        if (GetMessageA(&message, NULL, 0, 0) != FALSE) {
            TranslateMessage(&message);
            DispatchMessageA(&message);
        }
    }
}

// donor PoL RVA 0x0001c880; preferred Buka symbol ?ResizeWindow@@YIXHHHH@Z
// donor Buka TU SOURCE/kbwin; HoMM1 owner inferred from contiguous order
// evidence: graph:4;base=0.562416;margin=0.918799;shape=0.364;size=0.993;calls=1.000;alternate=pol20:void ResizeWindow(int, int, int, int)@0x0001c880
VA(0x004437f4, 0x11d)
void ResizeWindow(i32 x, i32 y, i32 width, i32 height) {
    i32 xpos;
    RECT rect;
    i32 ypos;
    if (gConfig.gfx[gCurExe].fullScreen != 0)
        return;
    GetWindowRect(hwndApp, &rect);
    if (x == KBWIN_KEEP_POSITION)
        xpos = rect.left;
    else
        xpos = x;
    if (y == KBWIN_KEEP_POSITION)
        ypos = rect.top;
    else
        ypos = y;
    rect.left = 0;
    rect.top = 0;
    rect.right = width - 1;
    rect.bottom = height - 1;
    AdjustWindowRect(&rect, giCurWindowsStyleFlags, gConfig.gfx[gCurExe].showMenu);
    MoveWindow(hwndApp, xpos, ypos, rect.right - rect.left + 1, rect.bottom - rect.top + 1, TRUE);
    gConfig.gfx[gCurExe].x = xpos;
    gConfig.gfx[gCurExe].y = ypos;
    gConfig.gfx[gCurExe].width = width;
    gConfig.gfx[gCurExe].height = height;
    WritePrefs();
}

// donor PoL RVA 0x0001c9c7; preferred Buka symbol ?AppCommand@@YIJPAXIIJ@Z
// donor Buka TU SOURCE/kbwin; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.642433;margin=0.651384;shape=0.267;size=0.907;calls=1.000;strings=HEROES;alternate=pol20:long int AppCommand(void *, unsigned int, unsigned int, long int)@0x0001c9c7
VA(0x00443911, 0x165)
i32 AppCommand(void* window, u32 message, u32 messageParam, i32 messageData) {
    DLGPROC appDialogProc;
    i32 command;

    command = LOWORD(messageParam);
    switch (command) {
        case KBWIN_MENU_ABOUT:
            appDialogProc =
                reinterpret_cast<DLGPROC>(AppAbout); // AppAbout is the BOOL dialog procedure.
            DialogBoxParamA(
                static_cast<HINSTANCE>(hInstApp),
                "HEROES",
                static_cast<HWND>(window),
                appDialogProc,
                0
            );
            break;
        case KBWIN_MENU_HELP:
            WinHelpA(static_cast<HWND>(hwndApp), ".\\HELP\\HEROES.HLP", HELP_FINDER, 0);
            break;
        case KBWIN_MENU_SIZE_640_480:
            ResizeWindow(
                KBWIN_KEEP_POSITION,
                KBWIN_KEEP_POSITION,
                KBWIN_WIDTH_640,
                KBWIN_HEIGHT_480
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
            SetFullScreenStatus(1 - gConfig.gfx[gCurExe].fullScreen);
            break;
        default:
            return HandleAppSpecificMenuCommands(command);
    }
    return 0;
}

// PoL 2.0 UpdateDfltMenu correspondence; disables unsupported window sizes.
VA(0x00443a76, 0xae)
void UpdateDfltMenu(void* menu) {
    i32 result;
    i32 value;

    if (gConfig.gfx[gCurExe].showMenu == 0)
        return;
    if (gMainVideoModeWidth <= KBWIN_WIDTH_640)
        EnableMenuItem(static_cast<HMENU>(menu), KBWIN_MENU_SIZE_640_480, MF_GRAYED);
    if (gMainVideoModeWidth <= KBWIN_WIDTH_800)
        EnableMenuItem(static_cast<HMENU>(menu), KBWIN_MENU_SIZE_800_600, MF_GRAYED);
    if (gMainVideoModeWidth <= KBWIN_WIDTH_1024)
        EnableMenuItem(static_cast<HMENU>(menu), KBWIN_MENU_SIZE_1024_768, MF_GRAYED);
    if (gMainVideoModeWidth <= KBWIN_WIDTH_1280)
        EnableMenuItem(static_cast<HMENU>(menu), KBWIN_MENU_SIZE_1280_1024, MF_GRAYED);
    if (gDDrawAttached == FALSE)
        EnableMenuItem(static_cast<HMENU>(menu), KBWIN_MENU_FULLSCREEN, MF_GRAYED);
}

// donor PoL RVA 0x0001cc35; preferred Buka symbol ?KBChangeMenu@@YIXPAX@Z
// donor Buka TU SOURCE/kbwin; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.545069;margin=0.304682;shape=0.429;size=0.841;calls=1.000;alternate=pol20:void KBChangeMenu(void *)@0x0001cc35
VA(0x00443b24, 0x91)
void KBChangeMenu(void* menu) {
    if (menu == NULL)
        menu = hmnuCurrent;
    else
        hmnuCurrent = menu;
    hmnuApp = menu;
    if (gConfig.gfx[gCurExe].showMenu) {
        if (menu != NULL) {
            SetMenu(hwndApp, static_cast<HMENU>(menu));
            UpdateDfltMenu(menu);
            UpdateAppSpecificMenus(menu);
            DrawMenuBar(hwndApp);
        }
    } else {
        SetMenu(hwndApp, NULL);
        DrawMenuBar(hwndApp);
    }
}

// donor PoL RVA 0x0001cce1; preferred Buka symbol ?SetMenuStatus@@YIXH@Z
// donor Buka TU SOURCE/kbwin; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.517140;margin=0.517010;shape=0.323;size=0.903;calls=1.000;alternate=pol20:void SetMenuStatus(int)@0x0001cce1
VA(0x00443bb5, 0x118)
void SetMenuStatus(i32 showMenu) {
    i32 clientWidth;
    i32 height;
    i32 windowStyle;
    i32 replacedStyle;
    if (gConfig.gfx[gCurExe].fullScreen && showMenu)
        return;
    clientWidth = gConfig.gfx[gCurExe].width;
    height = gConfig.gfx[gCurExe].height;
    gConfig.gfx[gCurExe].showMenu = showMenu;
    KBChangeMenu(NULL);
    gConfig.gfx[gCurExe].width = clientWidth;
    gConfig.gfx[gCurExe].height = height;
    WritePrefs();
    windowStyle = GetWindowLongA(hwndApp, GWL_STYLE);
    if (gConfig.gfx[gCurExe].showMenu)
        giCurWindowsStyleFlags = WS_VISIBLE | WS_CLIPSIBLINGS | WS_OVERLAPPEDWINDOW;
    else
        giCurWindowsStyleFlags = WS_VISIBLE | WS_CLIPSIBLINGS;
    replacedStyle = SetWindowLongA(hwndApp, GWL_STYLE, giCurWindowsStyleFlags);
    ShowWindow(hwndApp, SW_SHOWNA);
    ResizeWindow(
        KBWIN_KEEP_POSITION,
        KBWIN_KEEP_POSITION,
        gConfig.gfx[gCurExe].width,
        gConfig.gfx[gCurExe].height
    );
}

// donor PoL RVA 0x0001ce3d; preferred Buka symbol ?SetNoDialogMenus@@YIXH@Z
// donor Buka TU SOURCE/kbwin; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.510874;margin=0.487078;shape=0.429;size=0.686;calls=1.000;alternate=pol20:void SetNoDialogMenus(int)@0x0001ce3d
VA(0x00443ccd, 0x52)
void SetNoDialogMenus(i32 menusEnabled) {
    DATA(0x004a9e58)
    static i32 gNoDialogMenusOn = 0;
    if (gNoDialogMenusOn && !menusEnabled)
        return;
    if (!gNoDialogMenusOn && menusEnabled)
        return;
    if (!hmnuApp)
        return;
    gNoDialogMenusOn = 1 - menusEnabled;
    SetMenus(hmnuApp, menusEnabled);
}

// PoL 2.0 SetMenus correspondence: recurse into popups, then restore
// each command from the normal or setup enable table.
VA(0x00443d1f, 0x12b)
void SetMenus(void* menu, i32 enabled) {
    i32 itemIndex;
    i32 numItems;
    u32 id;
    i32 scanIndex;
    i32 k;
    i32 change;

    numItems = GetMenuItemCount(static_cast<HMENU>(menu));
    for (itemIndex = 0; itemIndex < numItems; itemIndex++) {
        id = GetMenuItemID(static_cast<HMENU>(menu), itemIndex);
        if (id == static_cast<u32>(-1)) {
            SetMenus(GetSubMenu(static_cast<HMENU>(menu), itemIndex), enabled);
            change = 0;
        } else {
            change = 0;
            if (enabled) {
                change = 1;
            } else {
                scanIndex = 0;
                for (k = 0; k < KBWIN_MENU_ENTRY_COUNT; k++) {
                    if (gMenuEnableStatus[k].command == id)
                        scanIndex = k;
                }
                if (gInSetupDialog)
                    change = 1 - gMenuEnableStatus[scanIndex].setupEnabled;
                else
                    change = 1 - gMenuEnableStatus[scanIndex].normalEnabled;
            }
        }
        if (change != 0)
            EnableMenuItem(static_cast<HMENU>(menu), id, enabled == 0 ? MF_GRAYED : MF_ENABLED);
    }
    UpdateDfltMenu(menu);
}

// PoL 2.0 Misc.cpp SetGameDefaults correspondence; HoMM1 picks the walk
// speed unconditionally in Buka; the old processor/slow-video choice is gone.
VA(0x00443e4a, 0x145)
void SetGameDefaults(void) {
    i32 i;

    gConfig.musicVolume = 1;
    gConfig.soundVolume = 1;
    gConfig.autosave = 1;
    gConfig.showRoute = 1;
    gConfig.blackoutComputer = 0;
    for (i = 0; i < CONFIG_EXECUTABLE_COUNT; i++) {
        gConfig.gfx[i].showMenu = 1;
        gConfig.gfx[i].x = DEFAULT_WINDOW_ORIGIN;
        gConfig.gfx[i].y = DEFAULT_WINDOW_ORIGIN;
        if (gMainVideoModeWidth <= DEFAULT_WINDOW_WIDTH && gDDrawAttached) {
            gConfig.gfx[i].fullScreen = 1;
            gConfig.gfx[i].width = DEFAULT_SMALL_WINDOW_WIDTH;
            gConfig.gfx[i].height = DEFAULT_SMALL_WINDOW_HEIGHT;
        } else {
            gConfig.gfx[i].fullScreen = 1;
            gConfig.gfx[i].width = DEFAULT_WINDOW_WIDTH;
            gConfig.gfx[i].height = DEFAULT_WINDOW_HEIGHT;
        }
    }
    gConfig.blackoutComputer = 0;
    gConfig.currentMapOffset = 0;
    gConfig.firstMapOffset = Random(0, DEFAULT_MAP_OFFSET_LIMIT);
    gConfig.musicSource = SOUND_MUSIC_SOURCE_CD;
    gFirstTimeThrough = 1;
    gConfig.walkSpeed = WALK_SPEED_CANTER;
}

// @dead-code
// Zero-ref: no incoming call, jump or relocated reference in retail.
// NWC-only: no standalone Buka body; see buka-function-map.json.
void ReadPrefsFromFile(void) {
    FILE* fp;
    i32 result;
    char buffer[100];

    sprintf(gText, "%s", "HEROES.CFG");
    if (access(gText, 0) == -1) {
        memset(&gConfig, 0, sizeof(gConfig));
        SetGameDefaults();
        WritePrefs();
    } else {
        fp = fopen(gText, "rb");
        if (fp == NULL)
            FileError(gText);
        fread(&gConfig, sizeof(gConfig), 1, fp);
        if (gConfig.gfx[gCurExe].width <= 0)
            gConfig.gfx[gCurExe].width = MINIMUM_WINDOW_WIDTH;
        if (gConfig.gfx[gCurExe].height <= 0)
            gConfig.gfx[gCurExe].height = MINIMUM_WINDOW_HEIGHT;
        if (gConfig.gfx[gCurExe].x < 0)
            gConfig.gfx[gCurExe].x = 0;
        if (gConfig.gfx[gCurExe].x > gMainVideoModeHeight - WINDOW_POSITION_MARGIN)
            gConfig.gfx[gCurExe].x = gMainVideoModeHeight - WINDOW_POSITION_MARGIN;
        if (gConfig.gfx[gCurExe].y < 0)
            gConfig.gfx[gCurExe].y = 0;
        if (gConfig.gfx[gCurExe].y > gMainVideoModeWidth - WINDOW_POSITION_MARGIN)
            gConfig.gfx[gCurExe].y = gMainVideoModeWidth - WINDOW_POSITION_MARGIN;
        result = fclose(fp);
        if (gConfig.walkSpeed == CONFIG_UNINITIALIZED) {
            SetGameDefaults();
            WritePrefs();
        }
    }
    strcpy(gcRegCDRomPath, "");
    strcpy(gcRegAppPath, "");
}

// NWC-only: no standalone Buka body; see buka-function-map.json.
void ReadPrefsFromRegistry(void) {
    HKEY key;
    DWORD cbData;
    char szTemp[REGISTRY_TEXT_BUFFER_SIZE];
    DWORD dataType;
    char szSubKey[REGISTRY_TEXT_BUFFER_SIZE];
    i32 rc;

    strcpy(szTemp, "");
    strcpy(szSubKey, "SOFTWARE\\New World Computing\\Heroes of Might and Magic\\1.0");
    key = NULL;
    rc = RegOpenKeyExA(HKEY_LOCAL_MACHINE, szSubKey, 0, KEY_READ, &key);
    if (rc == ERROR_SUCCESS) {
        cbData = REGISTRY_DWORD_BYTES;
        if (RegQueryValueExA(
                key,
                "Music Volume",
                NULL,
                &dataType,
                reinterpret_cast<LPBYTE>(&gConfig.musicVolume),
                &cbData
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
            "Music Volume",
            NULL,
            &dataType,
            reinterpret_cast<LPBYTE>(&gConfig.musicVolume),
            &cbData
        );
        RegQueryValueExA(
            key,
            "Sound Volume",
            NULL,
            &dataType,
            reinterpret_cast<LPBYTE>(&gConfig.soundVolume),
            &cbData
        );
        RegQueryValueExA(
            key,
            "Walk Speed",
            NULL,
            &dataType,
            reinterpret_cast<LPBYTE>(&gConfig.walkSpeed),
            &cbData
        );
        RegQueryValueExA(
            key,
            "Show Route",
            NULL,
            &dataType,
            reinterpret_cast<LPBYTE>(&gConfig.showRoute),
            &cbData
        );
        RegQueryValueExA(
            key,
            "Blackout Computer",
            NULL,
            &dataType,
            reinterpret_cast<LPBYTE>(&gConfig.blackoutComputer),
            &cbData
        );
        RegQueryValueExA(
            key,
            "Sound Quality",
            NULL,
            &dataType,
            reinterpret_cast<LPBYTE>(&gConfig.musicSource),
            &cbData
        );
        RegQueryValueExA(
            key,
            "Direct Connect Com Port",
            NULL,
            &dataType,
            reinterpret_cast<LPBYTE>(&gConfig.comPort[CONFIG_CONNECTION_DIRECT]),
            &cbData
        );
        RegQueryValueExA(
            key,
            "Direct Connect Baud Rate",
            NULL,
            &dataType,
            reinterpret_cast<LPBYTE>(&gConfig.baudRate[CONFIG_CONNECTION_DIRECT]),
            &cbData
        );
        RegQueryValueExA(
            key,
            "Modem Com Port",
            NULL,
            &dataType,
            reinterpret_cast<LPBYTE>(&gConfig.comPort[CONFIG_CONNECTION_MODEM]),
            &cbData
        );
        RegQueryValueExA(
            key,
            "Modem Baud Rate",
            NULL,
            &dataType,
            reinterpret_cast<LPBYTE>(&gConfig.baudRate[CONFIG_CONNECTION_MODEM]),
            &cbData
        );
        cbData = REGISTRY_TEXT_VALUE_SIZE;
        RegQueryValueExA(
            key,
            "Modem Init String",
            NULL,
            &dataType,
            reinterpret_cast<LPBYTE>(gConfig.modemInitString),
            &cbData
        );
        cbData = REGISTRY_DWORD_BYTES;
        RegQueryValueExA(
            key,
            "Autosave",
            NULL,
            &dataType,
            reinterpret_cast<LPBYTE>(&gConfig.autosave),
            &cbData
        );
        RegQueryValueExA(
            key,
            "First Map Offset",
            NULL,
            &dataType,
            reinterpret_cast<LPBYTE>(&gConfig.firstMapOffset),
            &cbData
        );
        RegQueryValueExA(
            key,
            "Current Map Offset",
            NULL,
            &dataType,
            reinterpret_cast<LPBYTE>(&gConfig.currentMapOffset),
            &cbData
        );
        RegQueryValueExA(
            key,
            "Main Game Show Menu",
            NULL,
            &dataType,
            reinterpret_cast<LPBYTE>(&gConfig.gfx[CONFIG_EXECUTABLE_GAME].showMenu),
            &cbData
        );
        RegQueryValueExA(
            key,
            "Main Game X",
            NULL,
            &dataType,
            reinterpret_cast<LPBYTE>(&gConfig.gfx[CONFIG_EXECUTABLE_GAME].x),
            &cbData
        );
        RegQueryValueExA(
            key,
            "Main Game Y",
            NULL,
            &dataType,
            reinterpret_cast<LPBYTE>(&gConfig.gfx[CONFIG_EXECUTABLE_GAME].y),
            &cbData
        );
        RegQueryValueExA(
            key,
            "Main Game Width",
            NULL,
            &dataType,
            reinterpret_cast<LPBYTE>(&gConfig.gfx[CONFIG_EXECUTABLE_GAME].width),
            &cbData
        );
        RegQueryValueExA(
            key,
            "Main Game Height",
            NULL,
            &dataType,
            reinterpret_cast<LPBYTE>(&gConfig.gfx[CONFIG_EXECUTABLE_GAME].height),
            &cbData
        );
        RegQueryValueExA(
            key,
            "Main Game Full Screen",
            NULL,
            &dataType,
            reinterpret_cast<LPBYTE>(&gConfig.gfx[CONFIG_EXECUTABLE_GAME].fullScreen),
            &cbData
        );
        RegQueryValueExA(
            key,
            "Editor Show Menu",
            NULL,
            &dataType,
            reinterpret_cast<LPBYTE>(&gConfig.gfx[CONFIG_EXECUTABLE_EDITOR].showMenu),
            &cbData
        );
        RegQueryValueExA(
            key,
            "Editor X",
            NULL,
            &dataType,
            reinterpret_cast<LPBYTE>(&gConfig.gfx[CONFIG_EXECUTABLE_EDITOR].x),
            &cbData
        );
        RegQueryValueExA(
            key,
            "Editor Y",
            NULL,
            &dataType,
            reinterpret_cast<LPBYTE>(&gConfig.gfx[CONFIG_EXECUTABLE_EDITOR].y),
            &cbData
        );
        RegQueryValueExA(
            key,
            "Editor Width",
            NULL,
            &dataType,
            reinterpret_cast<LPBYTE>(&gConfig.gfx[CONFIG_EXECUTABLE_EDITOR].width),
            &cbData
        );
        RegQueryValueExA(
            key,
            "Editor Height",
            NULL,
            &dataType,
            reinterpret_cast<LPBYTE>(&gConfig.gfx[CONFIG_EXECUTABLE_EDITOR].height),
            &cbData
        );
        RegQueryValueExA(
            key,
            "Editor Full Screen",
            NULL,
            &dataType,
            reinterpret_cast<LPBYTE>(&gConfig.gfx[CONFIG_EXECUTABLE_EDITOR].fullScreen),
            &cbData
        );
        cbData = REGISTRY_TEXT_VALUE_SIZE;
        if (RegQueryValueExA(
                key,
                "AppPath",
                NULL,
                &dataType,
                reinterpret_cast<LPBYTE>(gcRegAppPath),
                &cbData
            )
            != ERROR_SUCCESS)
            strcpy(gcRegAppPath, "");
        if (RegQueryValueExA(
                key,
                "CDDrive",
                NULL,
                &dataType,
                reinterpret_cast<LPBYTE>(gcRegCDRomPath),
                &cbData
            )
            != ERROR_SUCCESS)
            strcpy(gcRegCDRomPath, "");
        RegCloseKey(key);
    }
}

VA(0x00443f8f, 0x468)
void ReadPrefs(void) {
    ReadPrefsFromRegistry();
}

// @dead-code
// Zero-ref: no incoming call, jump or relocated reference in retail.
// NWC-only: no standalone Buka body; see buka-function-map.json.
void WritePrefsToFile(void) {
    FILE* file;
    char buffer[100];

    memset(buffer, 0, sizeof(buffer));
    sprintf(gText, "%s", "HEROES.CFG");
    file = fopen(gText, "wb");
    if (file == NULL)
        FileError(gText);
    fwrite(&gConfig, sizeof(gConfig), 1, file);
    fclose(file);
}

// NWC-only: no standalone Buka body; see buka-function-map.json.
void WritePrefsToRegistry(void) {
    HKEY key;
    char szTemp[REGISTRY_TEXT_BUFFER_SIZE];
    char szSubKey[REGISTRY_TEXT_BUFFER_SIZE];
    i32 rc;

    strcpy(szTemp, "");
    strcpy(szSubKey, "SOFTWARE\\New World Computing\\Heroes of Might and Magic\\1.0");
    key = NULL;
    rc = RegOpenKeyExA(HKEY_LOCAL_MACHINE, szSubKey, 0, KEY_READ, &key);
    if (rc == ERROR_SUCCESS) {
        RegSetValueExA(
            key,
            "Music Volume",
            0,
            REG_DWORD,
            reinterpret_cast<LPBYTE>(&gConfig.musicVolume),
            REGISTRY_DWORD_BYTES
        );
        RegSetValueExA(
            key,
            "Sound Volume",
            0,
            REG_DWORD,
            reinterpret_cast<LPBYTE>(&gConfig.soundVolume),
            REGISTRY_DWORD_BYTES
        );
        RegSetValueExA(
            key,
            "Walk Speed",
            0,
            REG_DWORD,
            reinterpret_cast<LPBYTE>(&gConfig.walkSpeed),
            REGISTRY_DWORD_BYTES
        );
        RegSetValueExA(
            key,
            "Show Route",
            0,
            REG_DWORD,
            reinterpret_cast<LPBYTE>(&gConfig.showRoute),
            REGISTRY_DWORD_BYTES
        );
        RegSetValueExA(
            key,
            "Blackout Computer",
            0,
            REG_DWORD,
            reinterpret_cast<LPBYTE>(&gConfig.blackoutComputer),
            REGISTRY_DWORD_BYTES
        );
        RegSetValueExA(
            key,
            "Sound Quality",
            0,
            REG_DWORD,
            reinterpret_cast<LPBYTE>(&gConfig.musicSource),
            REGISTRY_DWORD_BYTES
        );
        RegSetValueExA(
            key,
            "Direct Connect Com Port",
            0,
            REG_DWORD,
            reinterpret_cast<LPBYTE>(&gConfig.comPort[CONFIG_CONNECTION_DIRECT]),
            REGISTRY_DWORD_BYTES
        );
        RegSetValueExA(
            key,
            "Direct Connect Baud Rate",
            0,
            REG_DWORD,
            reinterpret_cast<LPBYTE>(&gConfig.baudRate[CONFIG_CONNECTION_DIRECT]),
            REGISTRY_DWORD_BYTES
        );
        RegSetValueExA(
            key,
            "Modem Com Port",
            0,
            REG_DWORD,
            reinterpret_cast<LPBYTE>(&gConfig.comPort[CONFIG_CONNECTION_MODEM]),
            REGISTRY_DWORD_BYTES
        );
        RegSetValueExA(
            key,
            "Modem Baud Rate",
            0,
            REG_DWORD,
            reinterpret_cast<LPBYTE>(&gConfig.baudRate[CONFIG_CONNECTION_MODEM]),
            REGISTRY_DWORD_BYTES
        );
        RegSetValueExA(
            key,
            "Modem Init String",
            0,
            REG_SZ,
            reinterpret_cast<LPBYTE>(gConfig.modemInitString),
            REGISTRY_DWORD_BYTES
        );
        RegSetValueExA(
            key,
            "Autosave",
            0,
            REG_DWORD,
            reinterpret_cast<LPBYTE>(&gConfig.autosave),
            REGISTRY_DWORD_BYTES
        );
        RegSetValueExA(
            key,
            "First Map Offset",
            0,
            REG_DWORD,
            reinterpret_cast<LPBYTE>(&gConfig.firstMapOffset),
            REGISTRY_DWORD_BYTES
        );
        RegSetValueExA(
            key,
            "Current Map Offset",
            0,
            REG_DWORD,
            reinterpret_cast<LPBYTE>(&gConfig.currentMapOffset),
            REGISTRY_DWORD_BYTES
        );
        RegSetValueExA(
            key,
            "Main Game Show Menu",
            0,
            REG_DWORD,
            reinterpret_cast<LPBYTE>(&gConfig.gfx[CONFIG_EXECUTABLE_GAME].showMenu),
            REGISTRY_DWORD_BYTES
        );
        RegSetValueExA(
            key,
            "Main Game X",
            0,
            REG_DWORD,
            reinterpret_cast<LPBYTE>(&gConfig.gfx[CONFIG_EXECUTABLE_GAME].x),
            REGISTRY_DWORD_BYTES
        );
        RegSetValueExA(
            key,
            "Main Game Y",
            0,
            REG_DWORD,
            reinterpret_cast<LPBYTE>(&gConfig.gfx[CONFIG_EXECUTABLE_GAME].y),
            REGISTRY_DWORD_BYTES
        );
        RegSetValueExA(
            key,
            "Main Game Width",
            0,
            REG_DWORD,
            reinterpret_cast<LPBYTE>(&gConfig.gfx[CONFIG_EXECUTABLE_GAME].width),
            REGISTRY_DWORD_BYTES
        );
        RegSetValueExA(
            key,
            "Main Game Height",
            0,
            REG_DWORD,
            reinterpret_cast<LPBYTE>(&gConfig.gfx[CONFIG_EXECUTABLE_GAME].height),
            REGISTRY_DWORD_BYTES
        );
        RegSetValueExA(
            key,
            "Main Game Full Screen",
            0,
            REG_DWORD,
            reinterpret_cast<LPBYTE>(&gConfig.gfx[CONFIG_EXECUTABLE_GAME].fullScreen),
            REGISTRY_DWORD_BYTES
        );
        RegSetValueExA(
            key,
            "Editor Show Menu",
            0,
            REG_DWORD,
            reinterpret_cast<LPBYTE>(&gConfig.gfx[CONFIG_EXECUTABLE_EDITOR].showMenu),
            REGISTRY_DWORD_BYTES
        );
        RegSetValueExA(
            key,
            "Editor X",
            0,
            REG_DWORD,
            reinterpret_cast<LPBYTE>(&gConfig.gfx[CONFIG_EXECUTABLE_EDITOR].x),
            REGISTRY_DWORD_BYTES
        );
        RegSetValueExA(
            key,
            "Editor Y",
            0,
            REG_DWORD,
            reinterpret_cast<LPBYTE>(&gConfig.gfx[CONFIG_EXECUTABLE_EDITOR].y),
            REGISTRY_DWORD_BYTES
        );
        RegSetValueExA(
            key,
            "Editor Width",
            0,
            REG_DWORD,
            reinterpret_cast<LPBYTE>(&gConfig.gfx[CONFIG_EXECUTABLE_EDITOR].width),
            REGISTRY_DWORD_BYTES
        );
        RegSetValueExA(
            key,
            "Editor Height",
            0,
            REG_DWORD,
            reinterpret_cast<LPBYTE>(&gConfig.gfx[CONFIG_EXECUTABLE_EDITOR].height),
            REGISTRY_DWORD_BYTES
        );
        RegSetValueExA(
            key,
            "Editor Full Screen",
            0,
            REG_DWORD,
            reinterpret_cast<LPBYTE>(&gConfig.gfx[CONFIG_EXECUTABLE_EDITOR].fullScreen),
            REGISTRY_DWORD_BYTES
        );
        RegCloseKey(key);
    }
}

VA(0x004443f7, 0x30b)
void WritePrefs(void) {
    UpdateSystemOptionsMenu();
    WritePrefsToRegistry();
}

// Buka retail data VA 0x0049e720. This path deliberately has no leading slash.
static char* gcCDTrackName = "Tracks\\02-AudioTrack 02.ogg";

VA(0x00444702, 0x72)
// Suppress the system's critical-error dialog while probing an empty drive.
static bool DriveSupportsFreeSpaceQuery(char driveLetter) {
    UINT oldMode;
    char path[CD_DRIVE_QUERY_PATH_SIZE];
    ULARGE_INTEGER available;
    ULARGE_INTEGER total;
    ULARGE_INTEGER freeBytes;

    wsprintfA(path, "%c:", driveLetter);
    oldMode = SetErrorMode(SEM_FAILCRITICALERRORS);
    if (GetDiskFreeSpaceExA(path, &available, &total, &freeBytes) != 0) {
        SetErrorMode(oldMode);
        return true;
    } else {
        SetErrorMode(oldMode);
        return false;
    }
}

VA(0x00444774, 0x36f)
// The disc probe now checks an Ogg track; it no longer opens an MCI CD device.
H1_ENUM_RETURN(CdSetupResult, i32) SetupCDDrive(void) {
    u32 logicalDrives;
    i32 cd;
    i32 fh;
    i32 index;
    i32 cdDrives[CD_DRIVE_LETTER_COUNT];
    char buffer[CD_PROBE_BUFFER_SIZE];
    i32 pos;
    i32 numCD;
    HKEY key;
    char subKey[REGISTRY_TEXT_BUFFER_SIZE];

    sprintf(gText, "%sHEROES.AGG", ".\\DATA\\");
    fh = open(gText, _O_BINARY);
    if (fh == -1) {
        if (_chdir(gcRegAppPath) == -1)
            return CD_SETUP_NO_APP_PATH;
        fh = open(gText, _O_BINARY);
        if (fh == -1)
            return CD_SETUP_NO_DATA;
    }
    close(fh);
    logicalDrives = GetLogicalDrives();
    // Retail clears 26 bytes, although the drive slots are 32-bit integers.
    memset(cdDrives, 0, CD_DRIVE_LETTER_COUNT);
    for (cd = CD_FIRST_DRIVE_LETTER, index = 0; cd < CD_DRIVE_LETTER_COUNT; cd++) {
        if (logicalDrives & (1 << cd)) {
            if (IsCDDrive(cd)) {
                cdDrives[index] = cd;
                index++;
            }
        }
    }
    numCD = index;
    if (strlen(gcRegCDRomPath) > 0 && gcRegCDRomPath[0] >= 'A' && gcRegCDRomPath[0] <= 'Z'
        && DriveSupportsFreeSpaceQuery(gcRegCDRomPath[0])) {
        sprintf(gText, "%s%s", gcRegCDRomPath, gcCDTrackName);
        fh = open(gText, _O_BINARY);
        if (fh != -1) {
            close(fh);
            return CD_SETUP_READY;
        }
    }
    if (numCD <= 0)
        return CD_SETUP_NO_DRIVE;
    for (cd = 0; cd < CD_SETUP_ATTEMPTS; cd++) {
        for (index = 0; index < numCD; index++) {
            if (DriveSupportsFreeSpaceQuery(cdDrives[index] + 'A')) {
                sprintf(gText, "%c:%s", cdDrives[index] + 'A', gcCDTrackName);
                fh = open(gText, _O_BINARY);
                if (fh == -1)
                    continue;
                pos = _lseek(fh, 0, SEEK_END);
                if (pos != -1) {
                    pos = _lseek(fh, -CD_AUTORUN_TAIL_BYTES, SEEK_CUR);
                    if (pos != -1)
                        pos = read(fh, buffer, CD_AUTORUN_TAIL_BYTES);
                }
                close(fh);
                if (pos != -1) {
                    sprintf(gcRegCDRomPath, "%c:", cdDrives[index] + 'A');
                    strcpy(
                        subKey,
                        "SOFTWARE\\Buka\\3DO\\Heroes of Might and Magic Platinum\\1.000"
                    );
                    key = NULL;
                    if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, subKey, 0, KEY_WRITE, &key)
                        == ERROR_SUCCESS) {
                        RegSetValueExA(
                            key,
                            "HMM1 CDDrive",
                            0,
                            REG_SZ,
                            reinterpret_cast<LPBYTE>(gcRegCDRomPath),
                            strlen(gcRegCDRomPath) + 1
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

// donor PoL RVA 0x000a0c76; preferred Buka symbol ?SetWinText@@YIXPAVheroWindow@@H@Z
// donor Buka TU SOURCE/KB; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.447557;margin=0.235076;shape=0.180;size=0.912;calls=1.000;alternate=pol20:void SetWinText(class heroWindow *, int)@0x000a0c76
VA(0x00444ae3, 0x6b)
void SetWinText(heroWindow* window, i16 id) {
    i32 i;
    tag_message message;
    for (i = 0; i < static_cast<i32>(WINDOW_TEXT_ENTRY_COUNT); i++) {
        if (gWinSetup[i].windowId == id) {
            SET_WIDGET_MESSAGE(message, WIDGET_COMMAND_SET_TEXT, gWinSetup[i].widgetId);
            message.text = gWinSetupText[i];
            window->BroadcastMessage(message);
        }
    }
}

// donor PoL RVA 0x0001d011; preferred Buka symbol ?KBTickCount@@YIJXZ
// donor Buka TU SOURCE/kbwin; HoMM1 owner inferred from contiguous order
// evidence: reviewed-anchor;alternate=pol20:long int KBTickCount(void)@0x0001d011
VA(0x00444b4e, 0xb)
i32 KBTickCount(void) {
    return GetTickCount();
}

// donor PoL RVA 0x000c47f0; preferred Buka symbol ?ProcessAssert@@YIXHPADH@Z
// donor Buka TU BASE/Misc; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.598916;margin=0.432613;shape=0.279;size=0.853;calls=0.600;strings=Assert Failure;alternate=pol20:void ProcessAssert(int, char *, int)@0x000c47f0
VA(0x00444b59, 0x55)
void ProcessAssert(i32 condition, char* file, i32 line) {
    i32 unusedAssertWord;
    if (condition == 0) {
        sprintf(gText, "Assert statement failed in module %s, line %d.", file, line);
        MessageBoxA(hwndApp, gText, "Assert Failure", MB_ICONHAND);
        unusedAssertWord = 0;
        ShutDown(gText);
    }
}

// PoL 2.0 Misc.cpp FindToken correspondence.
VA(0x00444bae, 0x50)
char* FindToken(char* text, char token) {
    i32 pos;
    i32 len;

    len = strlen(text);
    for (pos = 0; len > pos; pos++) {
        if (text[pos] == token)
            return text + pos;
    }
    return NULL;
}

// PoL 2.0 Misc.cpp FindLastToken correspondence.
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

// kbwin owns retail .data 0x0049f7a8-0x004a0503 and .bss 0x004ca490-0x004ca903.
DATA(0x0049f7a8)
char gAppName[] = "Heroes";
DATA(0x0049f7b0)
char gTitle[] = "Heroes of Might and Magic";
DATA(0x004a9e34)
HWND hwndApp = NULL;
DATA(0x004a9e38)
i32 gForegroundApp = 0;
DATA(0x004a9e3c)
void* hmnuApp = NULL;
DATA(0x004a9e40)
void* gEventHandle = NULL;
DATA(0x0049f854)
i32 gClosingApp = 0;
DATA(0x004a99e8)
void* hInstApp;
DATA(0x004c2068)
struct tagRECT rcTemp;
DATA(0x004a99dc)
i32 gMainWinScreenHeight;
DATA(0x004a9dec)
void* hmnuCurrent;
DATA(0x004c2080)
i32 gTempX;
DATA(0x004c2084)
i32 iTempY;
DATA(0x004c2078)
i32 lTemp;
DATA(0x004c2090)
u8 bProcessMessage[KBWIN_MESSAGE_FILTER_SIZE];
DATA(0x004a9df4)
char gCommandLine[KBWIN_COMMAND_LINE_CLEAR_SIZE];
DATA(0x004a9df0)
i32 iMainWinScreenWidth;

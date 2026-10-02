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
#include <BASE/MOUSEMGR_TYPES.h>
#include <BASE/soundmgr.h>
#include <H1/KB.h>
#include <H1/All.h>
#include <SOURCE/dialogTypes.h>
#include <SOURCE/wingraph.h>
#include <SOURCE/X_GLOBAL.h>

// donor PoL RVA 0x0001bce0; preferred Buka symbol _WinMain@16
// donor Buka TU SOURCE/kbwin; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.651055;margin=0.328202;shape=0.658;size=0.820;calls=1.000;alternate=pol20:_WinMain@16@0x0001bce0
VA(0x0045b6f0, 0x14e)
H1_C_LINKAGE int __stdcall WinMain(void *instance, void *previousInstance, char *commandLine, int showCommand)
{
    DWORD error;
    MSG message;

    hInstApp = instance;
    gEventHandle = CreateEventA(NULL, 0, 0, "Heroes");
    error = GetLastError();
    if (gEventHandle == NULL || error == ERROR_ALREADY_EXISTS) {
        sprintf(gText, "Only one copy of %s may run at a time", "Heroes of Might and Magic");
        MessageBoxA(NULL, gText, "Startup Error", MB_ICONHAND);
        return 0;
    }

    memset(gcCommandLine, 0, KBWIN_COMMAND_LINE_CLEAR_SIZE);
    strncpy(gcCommandLine, commandLine, KBWIN_COMMAND_LINE_LIMIT);
    if (EarlySetup() == 0)
        return 0;
    if (AppInit(instance, previousInstance, showCommand, commandLine) == 0)
        return 0;

    for (;;) {
        if (PeekMessageA(&message, NULL, 0, 0, PM_REMOVE) != 0) {
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
    return message.wParam;
}

// donor PoL RVA 0x0001be26; preferred Buka symbol ?AppInit@@YIHPAX0HPAD@Z
// donor Buka TU SOURCE/kbwin; HoMM1 owner inferred from contiguous order
// evidence: graph:1;base=0.682496;margin=0.205177;shape=0.345;size=0.971;calls=0.867;strings=Heroes|hInstApp;alternate=pol20:int AppInit(void *, void *, int, char *)@0x0001be26
VA(0x0045b83e, 0x2d6)
int AppInit(void *instance, void *previousInstance, int showCommand, char *commandLine)
{
    WNDCLASSA appClass;
    HMENU windowMenu;
    RECT rc;

    LogInt("hInstApp", reinterpret_cast<int>(hInstApp)); // API-forced handle value.
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
        appClass.lpszClassName = szAppName;
        appClass.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1); // Win32 system-color brush encoding.
        appClass.hInstance = static_cast<HINSTANCE>(instance);
        appClass.style = KBWIN_CLASS_STYLE;
        appClass.lpfnWndProc = reinterpret_cast<WNDPROC>(AppWndProc); // HoMM1 declares the procedure with void* handles.
        appClass.cbWndExtra = 0;
        appClass.cbClsExtra = 0;
        if (RegisterClassA(&appClass) == 0)
            return 0;
    }

    if (gConfig.gfx[giCurExe].showMenu != 0)
        giCurWindowsStyleFlags = KBWIN_WINDOWED_STYLE;
    else
        giCurWindowsStyleFlags = KBWIN_FULLSCREEN_STYLE;
    rc.left = rc.top = 0;
    rc.right = gConfig.gfx[giCurExe].width - 1;
    rc.bottom = gConfig.gfx[giCurExe].height - 1;
    AdjustWindowRect(&rc, giCurWindowsStyleFlags, gConfig.gfx[giCurExe].showMenu);
    if (gConfig.gfx[giCurExe].showMenu != 0)
        windowMenu = static_cast<HMENU>(hmnuDflt);
    else
        windowMenu = NULL;
    hwndApp = CreateWindowExA(0, szAppName, szTitle, giCurWindowsStyleFlags,
                              gConfig.gfx[giCurExe].x, gConfig.gfx[giCurExe].y,
                              rc.right - rc.left + 1, rc.bottom - rc.top + 1, NULL, windowMenu,
                              static_cast<HINSTANCE>(instance), NULL);
    if (hwndApp != NULL) {
        ShowWindow(static_cast<HWND>(hwndApp), showCommand);
        SetWindowLongA(static_cast<HWND>(hwndApp), GWL_STYLE, giCurWindowsStyleFlags);
        if (gConfig.gfx[giCurExe].showMenu == 0)
            SetMenuStatus(0);
        InitGraphics();
        SetCursor(LoadCursorA(NULL, IDC_ARROW));
        oldmain();
        return 1;
    } else {
        return 0;
    }
}

// PoL 2.0 AppIdle correspondence: both foreground states report idle work.
VA(0x0045bb14, 0x31)
int AppIdle(void) {
    if (gbForegroundApp != 0)
        return 1;
    else
        return 1;
}

// donor PoL RVA 0x0001c190; preferred Buka symbol ?AppWndProc@@YGJPAXIIJ@Z
// donor Buka TU SOURCE/kbwin; HoMM1 owner inferred from contiguous order
// evidence: graph:3;base=0.508573;margin=0.535153;shape=0.364;size=0.977;calls=0.857;alternate=pol20:long int AppWndProc(void *, unsigned int, unsigned int, long int)@0x0001c190
VA(0x0045bb45, 0x617)
long int __stdcall AppWndProc(void *window, unsigned int message, unsigned int messageParam, long int messageData)
{
    if (giDebugLevel == KBWIN_TRACE_DEBUG_LEVEL)
        LogStr("AWP", KBTickCount() % KBWIN_TRACE_TICK_MODULUS / KBWIN_TRACE_TICK_DIVISOR,
               reinterpret_cast<long>(window), message, messageParam, messageData); // API-forced: LogStr logs the handle as a long.
    if (message > KBWIN_PROCESS_MESSAGE_MAX || bProcessMessage[message] == 0)
        return DefWindowProcA(static_cast<HWND>(window), message, messageParam, messageData);

    switch (message) {
    case WM_CREATE:
        srand(KBTickCount());
        SetTimer(static_cast<HWND>(window), KBWIN_TIMER_ID, KBWIN_TIMER_INTERVAL, 0);
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
        if (lLastGTimerTickCount + KBWIN_POLL_INTERVAL < lTemp) {
            lLastGTimerTickCount = lTemp;
            SetReady2Poll();
        }
        if (lLastCycleTickCount + KBWIN_CYCLE_INTERVAL < lTemp) {
            lLastCycleTickCount = lTemp;
            if (giGraphicsType == KBWIN_GRAPHICS_DIRECT_DRAW && giMainVideoModeColorDepth != 8) {
                lLastCycleTickCount += KBWIN_CYCLE_DIRECT_DRAW_DELAY;
                if (gbHeroMoving)
                    return 0;
            }
            CycleColors();
        }
        return 0;
    case MM_MCINOTIFY:
        if (messageParam == MCI_NOTIFY_SUCCESSFUL)
            gpSoundManager->CDPlay(gpSoundManager->m_cdTrack, 0, gpSoundManager->m_cdPlayFrame, 1);
        break;
    case WM_ACTIVATEAPP:
        gbForegroundApp = messageParam;
        return 0;
    case WM_ERASEBKGND:
        return 1;
    case WM_MOVE:
        if (hwndApp == 0)
            return 0;
        lTemp = GetWindowLongA(static_cast<HWND>(hwndApp), GWL_STYLE);
        if ((lTemp & (WS_MINIMIZE | WS_MAXIMIZE)) == 0 && gbClosingApp == 0
            && gConfig.gfx[giCurExe].fullScreen == 0) {
            GetWindowRect(static_cast<HWND>(window), &rcTemp);
            gConfig.gfx[giCurExe].x = rcTemp.left;
            gConfig.gfx[giCurExe].y = rcTemp.top;
            WritePrefs();
        }
        return 0;
    case WM_SIZE:
        if (hwndApp != 0) {
            lTemp = GetWindowLongA(static_cast<HWND>(hwndApp), GWL_STYLE);
            gbMinimized = lTemp & WS_MINIMIZE;
            if ((lTemp & WS_MINIMIZE) == 0)
                EarlyResizeWindow(0, 0, 0, 0);
            if ((lTemp & (WS_MINIMIZE | WS_MAXIMIZE)) == 0
                && (LOWORD(messageData) < KBWIN_MIN_WIDTH || HIWORD(messageData) < KBWIN_MIN_HEIGHT)) {
                iTempX = LOWORD(messageData) > KBWIN_MIN_WIDTH ? LOWORD(messageData) : KBWIN_MIN_WIDTH;
                iTempY = HIWORD(messageData) > KBWIN_MIN_HEIGHT ? HIWORD(messageData) : KBWIN_MIN_HEIGHT;
                ResizeWindow(-1, -1, iTempX, iTempY);
                return 0;
            }
        }
        iMainWinScreenWidth = LOWORD(messageData);
        iMainWinScreenHeight = HIWORD(messageData);
        if (iMainWinScreenWidth < 1)
            iMainWinScreenWidth = 1;
        if (iMainWinScreenHeight < 1)
            iMainWinScreenHeight = 1;
        if (hwndApp != 0 && (lTemp & (WS_MINIMIZE | WS_MAXIMIZE)) == 0 && gbClosingApp == 0
            && gConfig.gfx[giCurExe].fullScreen == 0) {
            gConfig.gfx[giCurExe].width = iMainWinScreenWidth;
            gConfig.gfx[giCurExe].height = iMainWinScreenHeight;
            WritePrefs();
        }
        return 0;
    case WM_COMMAND:
        return AppCommand(window, message, messageParam, messageData);
    case WM_PALETTECHANGED:
        if (reinterpret_cast<unsigned int>(window) == messageParam) // API-forced: WPARAM carries the changing window.
            break;
    case WM_QUERYNEWPALETTE:
        return QueryNewPalette();
    case WM_PAINT:
        AppPaint(window, 0);
        return 0;
    case WM_CLOSE:
        if (window == hwndApp) {
            if (GameUnsaved() != 0) {
                NormalDialog("Are you sure you want to quit?", NORMAL_DIALOG_TYPE_YES_NO, -1, -1, NORMAL_DIALOG_NO_RESOURCE, 0, NORMAL_DIALOG_NO_RESOURCE, 0, NORMAL_DIALOG_NO_OR_TEXT);
                if (gpWindowManager->m_dialogResult == NORMAL_DIALOG_CONFIRM)
                    DestroyWindow(static_cast<HWND>(window));
                return 0;
            }
        }
    case WM_DESTROY:
        gbClosingApp = 1;
        PostQuitMessage(0);
    case WM_QUIT:
        ShutDown(0);
        break;
    }
    return DefWindowProcA(static_cast<HWND>(window), message, messageParam, messageData);
}

// Identity: PE export AppAbout, ordinal 1.
// Extent: entry through ret 16 at 0x45c1e9; next function starts at 0x45c1ec.
extern "C" VA(0x0045c15c, 0x90)
BOOL __stdcall AppAbout(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {
    int wmId;
    WORD codeNotify;
    HWND hwndCtl;
    switch (message) {
        case WM_INITDIALOG:
            return 1;
        case WM_COMMAND:
            wmId = wParam & 0xffff;
            hwndCtl = reinterpret_cast<HWND>(lParam); // WM_COMMAND passes HWND in LPARAM.
            codeNotify = (wParam >> 16) & 0xffff;
            if (wmId == IDOK)
                EndDialog(hDlg, 1);
            break;
    }
    PollSound();
    return 0;
}

VA(0x0045c1ec, 0x1a)
void AppExit(void) {
    CleanUpWinGraphics();
    CleanUpMenus();
}

// donor PoL RVA 0x0001c7b8; preferred Buka symbol ?Process1WindowsMessage@@YIXXZ
// donor Buka TU SOURCE/kbwin; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.631126;margin=0.664983;shape=0.634;size=0.797;calls=1.000;alternate=pol20:void Process1WindowsMessage(void)@0x0001c7b8
VA(0x0045c206, 0xca)
void Process1WindowsMessage(void) {
    MSG message;
    long currentTick;

    while (PeekMessageA(&message, NULL, 0, 0, PM_REMOVE) != 0) {
        TranslateMessage(&message);
        DispatchMessageA(&message);
    }
    currentTick = KBTickCount();
    if (currentTick - lLastAilServe > 20) {
        lLastAilServe = currentTick;
        if (gbNoSound == 0)
            gpSoundManager->ServiceSound();
    }
    if (currentTick - lLastGetMessage > 150) {
        lLastGetMessage = currentTick;
        if (GetMessageA(&message, NULL, 0, 0) != 0) {
            TranslateMessage(&message);
            DispatchMessageA(&message);
        }
    }
}

// donor PoL RVA 0x0001c880; preferred Buka symbol ?ResizeWindow@@YIXHHHH@Z
// donor Buka TU SOURCE/kbwin; HoMM1 owner inferred from contiguous order
// evidence: graph:4;base=0.562416;margin=0.918799;shape=0.364;size=0.993;calls=1.000;alternate=pol20:void ResizeWindow(int, int, int, int)@0x0001c880
VA(0x0045c2d0, 0x127)
void ResizeWindow(int x, int y, int width, int height) {
    int xpos;
    RECT rect;
    int ypos;
    if (gConfig.gfx[giCurExe].fullScreen != 0)
        return;
    GetWindowRect(hwndApp, &rect);
    if (x == -1)
        xpos = rect.left;
    else
        xpos = x;
    if (y == -1)
        ypos = rect.top;
    else
        ypos = y;
    rect.left = 0;
    rect.top = 0;
    rect.right = width - 1;
    rect.bottom = height - 1;
    AdjustWindowRect(&rect, giCurWindowsStyleFlags, gConfig.gfx[giCurExe].showMenu);
    MoveWindow(
        hwndApp,
        xpos,
        ypos,
        rect.right - rect.left + 1,
        rect.bottom - rect.top + 1,
        1
    );
    gConfig.gfx[giCurExe].x = xpos;
    gConfig.gfx[giCurExe].y = ypos;
    gConfig.gfx[giCurExe].width = width;
    gConfig.gfx[giCurExe].height = height;
    WritePrefs();
}

// donor PoL RVA 0x0001c9c7; preferred Buka symbol ?AppCommand@@YIJPAXIIJ@Z
// donor Buka TU SOURCE/kbwin; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.642433;margin=0.651384;shape=0.267;size=0.907;calls=1.000;strings=HEROES;alternate=pol20:long int AppCommand(void *, unsigned int, unsigned int, long int)@0x0001c9c7
VA(0x0045c3f7, 0x185)
long int AppCommand(void *window, unsigned int message, unsigned int messageParam, long int messageData)
{
    DLGPROC appDialogProc;
    int command;

    command = LOWORD(messageParam);
    switch (command) {
    case KBWIN_MENU_ABOUT:
        appDialogProc = reinterpret_cast<DLGPROC>(AppAbout); // AppAbout is the exported BOOL dialog procedure.
        DialogBoxParamA(static_cast<HINSTANCE>(hInstApp), "HEROES", static_cast<HWND>(window), appDialogProc, 0);
        break;
    case KBWIN_MENU_HELP:
        WinHelpA(static_cast<HWND>(hwndApp), ".\\HELP\\HEROES.HLP", HELP_FINDER, 0);
        break;
    case KBWIN_MENU_SIZE_640_480:
        ResizeWindow(-1, -1, KBWIN_WIDTH_640, KBWIN_HEIGHT_480);
        break;
    case KBWIN_MENU_SIZE_800_600:
        ResizeWindow(-1, -1, KBWIN_WIDTH_800, KBWIN_HEIGHT_600);
        break;
    case KBWIN_MENU_SIZE_1024_768:
        ResizeWindow(-1, -1, KBWIN_WIDTH_1024, KBWIN_HEIGHT_768);
        break;
    case KBWIN_MENU_SIZE_1280_1024:
        ResizeWindow(-1, -1, KBWIN_WIDTH_1280, KBWIN_HEIGHT_1024);
        break;
    case KBWIN_MENU_FULLSCREEN:
        SetFullScreenStatus(1 - gConfig.gfx[giCurExe].fullScreen);
        break;
    default:
        return HandleAppSpecificMenuCommands(command);
    }
    return 0;
}

// PoL 2.0 UpdateDfltMenu correspondence; disables unsupported window sizes.
VA(0x0045c57c, 0xd0)
void UpdateDfltMenu(void* menu) {
    int result;
    int value;

    if (gConfig.gfx[giCurExe].showMenu == 0)
        return;
    if (giMainVideoModeWidth <= KBWIN_WIDTH_640)
        EnableMenuItem(static_cast<HMENU>(menu), KBWIN_MENU_SIZE_640_480, MF_GRAYED);
    if (giMainVideoModeWidth <= KBWIN_WIDTH_800)
        EnableMenuItem(static_cast<HMENU>(menu), KBWIN_MENU_SIZE_800_600, MF_GRAYED);
    if (giMainVideoModeWidth <= KBWIN_WIDTH_1024)
        EnableMenuItem(static_cast<HMENU>(menu), KBWIN_MENU_SIZE_1024_768, MF_GRAYED);
    if (giMainVideoModeWidth <= KBWIN_WIDTH_1280)
        EnableMenuItem(static_cast<HMENU>(menu), KBWIN_MENU_SIZE_1280_1024, MF_GRAYED);
    if (gbDDrawAttached == 0)
        EnableMenuItem(static_cast<HMENU>(menu), KBWIN_MENU_FULLSCREEN, MF_GRAYED);
}

// donor PoL RVA 0x0001cc35; preferred Buka symbol ?KBChangeMenu@@YIXPAX@Z
// donor Buka TU SOURCE/kbwin; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.545069;margin=0.304682;shape=0.429;size=0.841;calls=1.000;alternate=pol20:void KBChangeMenu(void *)@0x0001cc35
VA(0x0045c64c, 0xaa)
void KBChangeMenu(void* menu) {
    if (menu == NULL)
        menu = hmnuCurrent;
    else
        hmnuCurrent = menu;
    hmnuApp = menu;
    if (gConfig.gfx[giCurExe].showMenu) {
        if (menu != NULL) {
            SetMenu(hwndApp, menu);
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
VA(0x0045c6f6, 0x135)
void SetMenuStatus(int showMenu) {
    int clientWidth;
    int height;
    long windowStyle;
    long replacedStyle;
    if (gConfig.gfx[giCurExe].fullScreen && showMenu)
        return;
    clientWidth = gConfig.gfx[giCurExe].width;
    height = gConfig.gfx[giCurExe].height;
    gConfig.gfx[giCurExe].showMenu = showMenu;
    KBChangeMenu(NULL);
    gConfig.gfx[giCurExe].width = clientWidth;
    gConfig.gfx[giCurExe].height = height;
    WritePrefs();
    windowStyle = GetWindowLongA(hwndApp, GWL_STYLE);
    if (gConfig.gfx[giCurExe].showMenu)
        giCurWindowsStyleFlags = WS_VISIBLE | WS_CLIPSIBLINGS | WS_OVERLAPPEDWINDOW;
    else
        giCurWindowsStyleFlags = WS_VISIBLE | WS_CLIPSIBLINGS;
    replacedStyle = SetWindowLongA(hwndApp, GWL_STYLE, giCurWindowsStyleFlags);
    ShowWindow(hwndApp, SW_SHOWNA);
    ResizeWindow(-1, -1, gConfig.gfx[giCurExe].width, gConfig.gfx[giCurExe].height);
}

// donor PoL RVA 0x0001ce3d; preferred Buka symbol ?SetNoDialogMenus@@YIXH@Z
// donor Buka TU SOURCE/kbwin; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.510874;margin=0.487078;shape=0.429;size=0.686;calls=1.000;alternate=pol20:void SetNoDialogMenus(int)@0x0001ce3d
VA(0x0045c82b, 0x79)
void SetNoDialogMenus(int menusEnabled) {
    if (gbNoDialogMenusOn && !menusEnabled)
        return;
    if (!gbNoDialogMenusOn && menusEnabled)
        return;
    if (!hmnuApp)
        return;
    gbNoDialogMenusOn = 1 - menusEnabled;
    SetMenus(hmnuApp, menusEnabled);
}

// PoL 2.0 SetMenus correspondence: recurse into popups, then restore
// each command from the normal or setup enable table.
VA(0x0045c8a4, 0x15a)
void SetMenus(void* menu, int enabled) {
    int itemIndex;
    int numItems;
    unsigned int id;
    int scanIndex;
    int k;
    int change;

    numItems = GetMenuItemCount(static_cast<HMENU>(menu));
    for (itemIndex = 0; itemIndex < numItems; itemIndex++) {
        id = GetMenuItemID(static_cast<HMENU>(menu), itemIndex);
        if (id == static_cast<unsigned int>(-1)) {
            SetMenus(GetSubMenu(static_cast<HMENU>(menu), itemIndex), enabled);
            change = 0;
        } else {
            change = 0;
            if (enabled) {
                change = 1;
            } else {
                scanIndex = 0;
                for (k = 0; k < KBWIN_MENU_ENTRY_COUNT; k++) {
                    if (gsMenuEnableStatus[k].command == id)
                        scanIndex = k;
                }
                if (gbInSetupDialog)
                    change = 1 - gsMenuEnableStatus[scanIndex].setupEnabled;
                else
                    change = 1 - gsMenuEnableStatus[scanIndex].normalEnabled;
            }
        }
        if (change != 0)
            EnableMenuItem(static_cast<HMENU>(menu), id, enabled == 0 ? MF_GRAYED : MF_ENABLED);
    }
    UpdateDfltMenu(menu);
}

// PoL 2.0 Misc.cpp SetGameDefaults correspondence; HoMM1 picks the walk
// speed and slow-video default from the detected processor family.
VA(0x0045c9fe, 0x1a3)
void SetGameDefaults(void)
{
    int cpuType;
    int i;

    gConfig.musicVolume = 1;
    gConfig.soundVolume = 1;
    gConfig.autosave = 1;
    gConfig.showRoute = 1;
    gConfig.blackoutComputer = 0;
    for (i = 0; i < CONFIG_EXECUTABLE_COUNT; i++) {
        gConfig.gfx[i].showMenu = 1;
        gConfig.gfx[i].x = DEFAULT_WINDOW_ORIGIN;
        gConfig.gfx[i].y = DEFAULT_WINDOW_ORIGIN;
        if (giMainVideoModeWidth <= DEFAULT_WINDOW_WIDTH && gbDDrawAttached) {
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
    gConfig.cdOffset = 0;
    gConfig.musicSource = CONFIG_MUSIC_SOURCE_CD;
    gbFirstTimeThrough = 1;
    cpuType = GetCPUType();
    if ((cpuType & 0xff) >= CPU_FAMILY_PENTIUM) {
        gConfig.walkSpeed = CONFIG_WALK_SPEED_FAST;
        gConfig.slowVideo = 0;
    } else {
        gConfig.walkSpeed = CONFIG_WALK_SPEED_SLOW;
        gConfig.slowVideo = 1;
    }
}

// @dead-code
// Zero-ref: no incoming call, jump or relocated reference in retail.
VA(0x0045cba1, 0x20d)
void ReadPrefsFromFile(void)
{
    FILE *fp;
    int result;
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
        if (gConfig.gfx[giCurExe].width <= 0)
            gConfig.gfx[giCurExe].width = MINIMUM_WINDOW_WIDTH;
        if (gConfig.gfx[giCurExe].height <= 0)
            gConfig.gfx[giCurExe].height = MINIMUM_WINDOW_HEIGHT;
        if (gConfig.gfx[giCurExe].x < 0)
            gConfig.gfx[giCurExe].x = 0;
        if (gConfig.gfx[giCurExe].x > giMainVideoModeHeight - WINDOW_POSITION_MARGIN)
            gConfig.gfx[giCurExe].x = giMainVideoModeHeight - WINDOW_POSITION_MARGIN;
        if (gConfig.gfx[giCurExe].y < 0)
            gConfig.gfx[giCurExe].y = 0;
        if (gConfig.gfx[giCurExe].y > giMainVideoModeWidth - WINDOW_POSITION_MARGIN)
            gConfig.gfx[giCurExe].y = giMainVideoModeWidth - WINDOW_POSITION_MARGIN;
        result = fclose(fp);
        if (gConfig.walkSpeed == CONFIG_UNINITIALIZED) {
            SetGameDefaults();
            WritePrefs();
        }
    }
    strcpy(gcRegCDRomPath, "");
    strcpy(gcRegAppPath, "");
}

VA(0x0045cdae, 0x523)
void ReadPrefsFromRegistry(void)
{
    HKEY key;
    DWORD cbData;
    char szTemp[REGISTRY_TEXT_BUFFER_SIZE];
    DWORD dataType;
    char szSubKey[REGISTRY_TEXT_BUFFER_SIZE];
    long rc;

    strcpy(szTemp, "");
    strcpy(szSubKey, "SOFTWARE\\New World Computing\\Heroes of Might and Magic\\1.0");
    key = NULL;
    rc = RegOpenKeyExA(HKEY_LOCAL_MACHINE, szSubKey, 0, KEY_READ, &key);
    if (rc == 0) {
        cbData = REGISTRY_DWORD_BYTES;
        if (RegQueryValueExA(key, "Music Volume", NULL, &dataType, reinterpret_cast<LPBYTE>(&gConfig.musicVolume), &cbData) != 0) {
            memset(&gConfig, 0, sizeof(gConfig));
            SetGameDefaults();
            RegCloseKey(key);
            WritePrefs();
            return;
        }
        RegQueryValueExA(key, "Music Volume", NULL, &dataType, reinterpret_cast<LPBYTE>(&gConfig.musicVolume), &cbData);
        RegQueryValueExA(key, "Sound Volume", NULL, &dataType, reinterpret_cast<LPBYTE>(&gConfig.soundVolume), &cbData);
        RegQueryValueExA(key, "Walk Speed", NULL, &dataType, reinterpret_cast<LPBYTE>(&gConfig.walkSpeed), &cbData);
        RegQueryValueExA(key, "Show Route", NULL, &dataType, reinterpret_cast<LPBYTE>(&gConfig.showRoute), &cbData);
        RegQueryValueExA(key, "Blackout Computer", NULL, &dataType, reinterpret_cast<LPBYTE>(&gConfig.blackoutComputer), &cbData);
        RegQueryValueExA(key, "Sound Quality", NULL, &dataType, reinterpret_cast<LPBYTE>(&gConfig.musicSource), &cbData);
        RegQueryValueExA(key, "Direct Connect Com Port", NULL, &dataType, reinterpret_cast<LPBYTE>(&gConfig.comPort[CONFIG_CONNECTION_DIRECT]), &cbData);
        RegQueryValueExA(key, "Direct Connect Baud Rate", NULL, &dataType, reinterpret_cast<LPBYTE>(&gConfig.baudRate[CONFIG_CONNECTION_DIRECT]), &cbData);
        RegQueryValueExA(key, "Modem Com Port", NULL, &dataType, reinterpret_cast<LPBYTE>(&gConfig.comPort[CONFIG_CONNECTION_MODEM]), &cbData);
        RegQueryValueExA(key, "Modem Baud Rate", NULL, &dataType, reinterpret_cast<LPBYTE>(&gConfig.baudRate[CONFIG_CONNECTION_MODEM]), &cbData);
        cbData = REGISTRY_TEXT_VALUE_SIZE;
        RegQueryValueExA(key, "Modem Init String", NULL, &dataType, reinterpret_cast<LPBYTE>(gConfig.modemInitString), &cbData);
        cbData = REGISTRY_DWORD_BYTES;
        RegQueryValueExA(key, "Autosave", NULL, &dataType, reinterpret_cast<LPBYTE>(&gConfig.autosave), &cbData);
        RegQueryValueExA(key, "CD Offset", NULL, &dataType, reinterpret_cast<LPBYTE>(&gConfig.cdOffset), &cbData);
        RegQueryValueExA(key, "Slow Video", NULL, &dataType, reinterpret_cast<LPBYTE>(&gConfig.slowVideo), &cbData);
        RegQueryValueExA(key, "First Map Offset", NULL, &dataType, reinterpret_cast<LPBYTE>(&gConfig.firstMapOffset), &cbData);
        RegQueryValueExA(key, "Current Map Offset", NULL, &dataType, reinterpret_cast<LPBYTE>(&gConfig.currentMapOffset), &cbData);
        RegQueryValueExA(key, "Main Game Show Menu", NULL, &dataType, reinterpret_cast<LPBYTE>(&gConfig.gfx[CONFIG_EXECUTABLE_GAME].showMenu), &cbData);
        RegQueryValueExA(key, "Main Game X", NULL, &dataType, reinterpret_cast<LPBYTE>(&gConfig.gfx[CONFIG_EXECUTABLE_GAME].x), &cbData);
        RegQueryValueExA(key, "Main Game Y", NULL, &dataType, reinterpret_cast<LPBYTE>(&gConfig.gfx[CONFIG_EXECUTABLE_GAME].y), &cbData);
        RegQueryValueExA(key, "Main Game Width", NULL, &dataType, reinterpret_cast<LPBYTE>(&gConfig.gfx[CONFIG_EXECUTABLE_GAME].width), &cbData);
        RegQueryValueExA(key, "Main Game Height", NULL, &dataType, reinterpret_cast<LPBYTE>(&gConfig.gfx[CONFIG_EXECUTABLE_GAME].height), &cbData);
        RegQueryValueExA(key, "Main Game Full Screen", NULL, &dataType, reinterpret_cast<LPBYTE>(&gConfig.gfx[CONFIG_EXECUTABLE_GAME].fullScreen), &cbData);
        RegQueryValueExA(key, "Editor Show Menu", NULL, &dataType, reinterpret_cast<LPBYTE>(&gConfig.gfx[CONFIG_EXECUTABLE_EDITOR].showMenu), &cbData);
        RegQueryValueExA(key, "Editor X", NULL, &dataType, reinterpret_cast<LPBYTE>(&gConfig.gfx[CONFIG_EXECUTABLE_EDITOR].x), &cbData);
        RegQueryValueExA(key, "Editor Y", NULL, &dataType, reinterpret_cast<LPBYTE>(&gConfig.gfx[CONFIG_EXECUTABLE_EDITOR].y), &cbData);
        RegQueryValueExA(key, "Editor Width", NULL, &dataType, reinterpret_cast<LPBYTE>(&gConfig.gfx[CONFIG_EXECUTABLE_EDITOR].width), &cbData);
        RegQueryValueExA(key, "Editor Height", NULL, &dataType, reinterpret_cast<LPBYTE>(&gConfig.gfx[CONFIG_EXECUTABLE_EDITOR].height), &cbData);
        RegQueryValueExA(key, "Editor Full Screen", NULL, &dataType, reinterpret_cast<LPBYTE>(&gConfig.gfx[CONFIG_EXECUTABLE_EDITOR].fullScreen), &cbData);
        cbData = REGISTRY_TEXT_VALUE_SIZE;
        if (RegQueryValueExA(key, "AppPath", NULL, &dataType, reinterpret_cast<LPBYTE>(gcRegAppPath), &cbData) != 0)
            strcpy(gcRegAppPath, "");
        if (RegQueryValueExA(key, "CDDrive", NULL, &dataType, reinterpret_cast<LPBYTE>(gcRegCDRomPath), &cbData) != 0)
            strcpy(gcRegCDRomPath, "");
        RegCloseKey(key);
    }
}

VA(0x0045d2d1, 0x15)
void ReadPrefs(void)
{
    ReadPrefsFromRegistry();
}

// @dead-code
// Zero-ref: no incoming call, jump or relocated reference in retail.
VA(0x0045d2e6, 0x8a)
void WritePrefsToFile(void)
{
    FILE *file;
    char buffer[100];

    memset(buffer, 0, sizeof(buffer));
    sprintf(gText, "%s", "HEROES.CFG");
    file = fopen(gText, "wb");
    if (file == NULL)
        FileError(gText);
    fwrite(&gConfig, sizeof(gConfig), 1, file);
    fclose(file);
}

VA(0x0045d370, 0x3d0)
void WritePrefsToRegistry(void)
{
    HKEY key;
    char szTemp[REGISTRY_TEXT_BUFFER_SIZE];
    char szSubKey[REGISTRY_TEXT_BUFFER_SIZE];
    long rc;

    strcpy(szTemp, "");
    strcpy(szSubKey, "SOFTWARE\\New World Computing\\Heroes of Might and Magic\\1.0");
    key = NULL;
    rc = RegOpenKeyExA(HKEY_LOCAL_MACHINE, szSubKey, 0, KEY_READ, &key);
    if (rc == 0) {
        RegSetValueExA(key, "Music Volume", 0, REG_DWORD, reinterpret_cast<LPBYTE>(&gConfig.musicVolume), REGISTRY_DWORD_BYTES);
        RegSetValueExA(key, "Sound Volume", 0, REG_DWORD, reinterpret_cast<LPBYTE>(&gConfig.soundVolume), REGISTRY_DWORD_BYTES);
        RegSetValueExA(key, "Walk Speed", 0, REG_DWORD, reinterpret_cast<LPBYTE>(&gConfig.walkSpeed), REGISTRY_DWORD_BYTES);
        RegSetValueExA(key, "Show Route", 0, REG_DWORD, reinterpret_cast<LPBYTE>(&gConfig.showRoute), REGISTRY_DWORD_BYTES);
        RegSetValueExA(key, "Blackout Computer", 0, REG_DWORD, reinterpret_cast<LPBYTE>(&gConfig.blackoutComputer), REGISTRY_DWORD_BYTES);
        RegSetValueExA(key, "Sound Quality", 0, REG_DWORD, reinterpret_cast<LPBYTE>(&gConfig.musicSource), REGISTRY_DWORD_BYTES);
        RegSetValueExA(key, "Direct Connect Com Port", 0, REG_DWORD, reinterpret_cast<LPBYTE>(&gConfig.comPort[CONFIG_CONNECTION_DIRECT]), REGISTRY_DWORD_BYTES);
        RegSetValueExA(key, "Direct Connect Baud Rate", 0, REG_DWORD, reinterpret_cast<LPBYTE>(&gConfig.baudRate[CONFIG_CONNECTION_DIRECT]), REGISTRY_DWORD_BYTES);
        RegSetValueExA(key, "Modem Com Port", 0, REG_DWORD, reinterpret_cast<LPBYTE>(&gConfig.comPort[CONFIG_CONNECTION_MODEM]), REGISTRY_DWORD_BYTES);
        RegSetValueExA(key, "Modem Baud Rate", 0, REG_DWORD, reinterpret_cast<LPBYTE>(&gConfig.baudRate[CONFIG_CONNECTION_MODEM]), REGISTRY_DWORD_BYTES);
        RegSetValueExA(key, "Modem Init String", 0, REG_SZ, reinterpret_cast<LPBYTE>(gConfig.modemInitString), REGISTRY_DWORD_BYTES);
        RegSetValueExA(key, "Autosave", 0, REG_DWORD, reinterpret_cast<LPBYTE>(&gConfig.autosave), REGISTRY_DWORD_BYTES);
        RegSetValueExA(key, "CD Offset", 0, REG_DWORD, reinterpret_cast<LPBYTE>(&gConfig.cdOffset), REGISTRY_DWORD_BYTES);
        RegSetValueExA(key, "Slow Video", 0, REG_DWORD, reinterpret_cast<LPBYTE>(&gConfig.slowVideo), REGISTRY_DWORD_BYTES);
        RegSetValueExA(key, "First Map Offset", 0, REG_DWORD, reinterpret_cast<LPBYTE>(&gConfig.firstMapOffset), REGISTRY_DWORD_BYTES);
        RegSetValueExA(key, "Current Map Offset", 0, REG_DWORD, reinterpret_cast<LPBYTE>(&gConfig.currentMapOffset), REGISTRY_DWORD_BYTES);
        RegSetValueExA(key, "Main Game Show Menu", 0, REG_DWORD, reinterpret_cast<LPBYTE>(&gConfig.gfx[CONFIG_EXECUTABLE_GAME].showMenu), REGISTRY_DWORD_BYTES);
        RegSetValueExA(key, "Main Game X", 0, REG_DWORD, reinterpret_cast<LPBYTE>(&gConfig.gfx[CONFIG_EXECUTABLE_GAME].x), REGISTRY_DWORD_BYTES);
        RegSetValueExA(key, "Main Game Y", 0, REG_DWORD, reinterpret_cast<LPBYTE>(&gConfig.gfx[CONFIG_EXECUTABLE_GAME].y), REGISTRY_DWORD_BYTES);
        RegSetValueExA(key, "Main Game Width", 0, REG_DWORD, reinterpret_cast<LPBYTE>(&gConfig.gfx[CONFIG_EXECUTABLE_GAME].width), REGISTRY_DWORD_BYTES);
        RegSetValueExA(key, "Main Game Height", 0, REG_DWORD, reinterpret_cast<LPBYTE>(&gConfig.gfx[CONFIG_EXECUTABLE_GAME].height), REGISTRY_DWORD_BYTES);
        RegSetValueExA(key, "Main Game Full Screen", 0, REG_DWORD, reinterpret_cast<LPBYTE>(&gConfig.gfx[CONFIG_EXECUTABLE_GAME].fullScreen), REGISTRY_DWORD_BYTES);
        RegSetValueExA(key, "Editor Show Menu", 0, REG_DWORD, reinterpret_cast<LPBYTE>(&gConfig.gfx[CONFIG_EXECUTABLE_EDITOR].showMenu), REGISTRY_DWORD_BYTES);
        RegSetValueExA(key, "Editor X", 0, REG_DWORD, reinterpret_cast<LPBYTE>(&gConfig.gfx[CONFIG_EXECUTABLE_EDITOR].x), REGISTRY_DWORD_BYTES);
        RegSetValueExA(key, "Editor Y", 0, REG_DWORD, reinterpret_cast<LPBYTE>(&gConfig.gfx[CONFIG_EXECUTABLE_EDITOR].y), REGISTRY_DWORD_BYTES);
        RegSetValueExA(key, "Editor Width", 0, REG_DWORD, reinterpret_cast<LPBYTE>(&gConfig.gfx[CONFIG_EXECUTABLE_EDITOR].width), REGISTRY_DWORD_BYTES);
        RegSetValueExA(key, "Editor Height", 0, REG_DWORD, reinterpret_cast<LPBYTE>(&gConfig.gfx[CONFIG_EXECUTABLE_EDITOR].height), REGISTRY_DWORD_BYTES);
        RegSetValueExA(key, "Editor Full Screen", 0, REG_DWORD, reinterpret_cast<LPBYTE>(&gConfig.gfx[CONFIG_EXECUTABLE_EDITOR].fullScreen), REGISTRY_DWORD_BYTES);
        RegCloseKey(key);
    }
}

VA(0x0045d740, 0x1a)
void WritePrefs(void)
{
    UpdateSystemOptionsMenu();
    WritePrefsToRegistry();
}

// HoMM1 CD discovery: prefer the registered drive, then probe each CD-ROM
// drive's autorun file and remember the first one in the registry.
VA(0x0045d75a, 0x4c5)
int SetupCDDrive(void)
{
    int count;
    unsigned long logicalDrives;
    int cd;
    int fh;
    int pass;
    unsigned int nError;
    char cdDrives[CD_DRIVE_LETTER_COUNT];
    char numCD;
    HKEY hRegKey;
    long pos;
    char mciCommand[MCI_COMMAND_BUFFER_SIZE];
    char szReturn[MCI_COMMAND_BUFFER_SIZE];
    char szSubKey[REGISTRY_TEXT_BUFFER_SIZE];
    char driveText[REGISTRY_TEXT_BUFFER_SIZE];
    long rc;

    sprintf(gText, ".\\DATA\\HEROES.AGG");
    fh = open(gText, _O_BINARY);
    if (fh == -1) {
        if (_chdir(gcRegAppPath) == -1)
            return CD_SETUP_NO_APP_PATH;
        fh = open(gText, _O_BINARY);
        if (fh == -1)
            return CD_SETUP_NO_DATA;
    }
    close(fh);
    logicalDrives = 0;
    logicalDrives = GetLogicalDrives();
    count = 0;
    memset(cdDrives, 0, sizeof(cdDrives));
    for (cd = CD_FIRST_DRIVE_LETTER; cd < CD_DRIVE_LETTER_COUNT; cd++) {
        if (logicalDrives & (1 << cd)) {
            if (IsCDDrive(cd)) {
                cdDrives[count] = static_cast<char>(cd);
                count++;
            }
        }
    }
    numCD = static_cast<char>(count);
    giCDDrive = cdDrives[gConfig.cdOffset];
    if (giCDDrive < CD_FIRST_DRIVE_LETTER)
        giCDDrive = cdDrives[0];
    if (strlen(gcRegCDRomPath)) {
        sprintf(gText, "%s\\_autorun\\autorun.exe", gcRegCDRomPath);
        fh = open(gText, _O_BINARY);
        if (fh != -1) {
            close(fh);
            sprintf(gText + 2, "%s", gcSoundPath);
            strcpy(gcSoundPath, gText);
            sprintf(gText + 2, "%s", gcAnimPath);
            strcpy(gcAnimPath, gText);
            return CD_SETUP_READY;
        }
    }
    if (giCDDrive < CD_FIRST_DRIVE_LETTER)
        return CD_SETUP_NO_DRIVE;
    for (pass = 0; pass < CD_SETUP_ATTEMPTS; pass++) {
        for (cd = 0; cd < numCD; cd++) {
            wsprintfA(mciCommand, "open %c: type cdaudio alias CD", cdDrives[cd] + 'A');
            nError = mciSendStringA(mciCommand, szReturn, CD_MCI_RESULT_LAST, NULL);
            if (nError == 0) {
                wsprintfA(mciCommand, "info CD UPC wait");
                nError = mciSendStringA(mciCommand, szReturn, CD_MCI_RESULT_LAST, NULL);
                wsprintfA(mciCommand, "close CD");
                nError = mciSendStringA(mciCommand, szReturn, CD_MCI_RESULT_LAST, NULL);
            }
            sprintf(gText, "%c:\\_autorun\\autorun.exe", cdDrives[cd] + 'A', gcSoundPath);
            fh = open(gText, _O_BINARY);
            if (fh == -1)
                continue;
            pos = _lseek(fh, 0, SEEK_END);
            if (pos != -1) {
                pos = _lseek(fh, -CD_AUTORUN_TAIL_BYTES, SEEK_CUR);
                if (pos != -1)
                    pos = read(fh, szReturn, CD_AUTORUN_TAIL_BYTES);
            }
            close(fh);
            strcpy(szSubKey, "SOFTWARE\\New World Computing\\Heroes of Might and Magic\\1.0");
            hRegKey = NULL;
            rc = RegOpenKeyExA(HKEY_LOCAL_MACHINE, szSubKey, 0, KEY_WRITE, &hRegKey);
            if (rc == 0) {
                wsprintfA(driveText, "%c:", cdDrives[cd] + 'A');
                pass = RegSetValueExA(hRegKey, "CDDrive", 0, REG_SZ, reinterpret_cast<LPBYTE>(driveText), lstrlenA(driveText) + 1);
                RegCloseKey(hRegKey);
            }
            sprintf(gText, "%c:%s", cdDrives[cd] + 'A', gcSoundPath);
            strcpy(gcSoundPath, gText);
            sprintf(gText, "%c:%s", cdDrives[cd] + 'A', gcAnimPath);
            strcpy(gcAnimPath, gText);
            return CD_SETUP_READY;
        }
        Sleep(CD_SETUP_RETRY_DELAY);
    }
    return CD_SETUP_NOT_FOUND;
}

// donor PoL RVA 0x000a0c76; preferred Buka symbol ?SetWinText@@YIXPAVheroWindow@@H@Z
// donor Buka TU SOURCE/KB; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.447557;margin=0.235076;shape=0.180;size=0.912;calls=1.000;alternate=pol20:void SetWinText(class heroWindow *, int)@0x000a0c76
VA(0x0045dc1f, 0x7c)
void SetWinText(heroWindow* window, short id) {
    int i;
    tag_message message;
    for (i = 0; i < static_cast<int>(WINDOW_TEXT_ENTRY_COUNT); i++) {
        if (gWinSetup[i].windowId == id) {
            message.type = MESSAGE_WIDGET;
            message.command = WIDGET_COMMAND_SET_TEXT;
            message.id = gWinSetup[i].widgetId;
            message.text = gWinSetupText[i];
            window->BroadcastMessage(message);
        }
    }
}

// donor PoL RVA 0x0001d011; preferred Buka symbol ?KBTickCount@@YIJXZ
// donor Buka TU SOURCE/kbwin; HoMM1 owner inferred from contiguous order
// evidence: reviewed-anchor;alternate=pol20:long int KBTickCount(void)@0x0001d011
VA(0x0045dc9b, 0x16)
long int KBTickCount(void) {
    return GetTickCount();
}

// donor PoL RVA 0x000c47f0; preferred Buka symbol ?ProcessAssert@@YIXHPADH@Z
// donor Buka TU BASE/Misc; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.598916;margin=0.432613;shape=0.279;size=0.853;calls=0.600;strings=Assert Failure;alternate=pol20:void ProcessAssert(int, char *, int)@0x000c47f0
VA(0x0045dcb1, 0x63)
void ProcessAssert(int condition, char* file, int line) {
    int unusedAssertWord;
    if (condition == 0) {
        sprintf(gText, "Assert statement failed in module %s, line %d.", file, line);
        MessageBoxA(hwndApp, gText, "Assert Failure", MB_ICONHAND);
        unusedAssertWord = 0;
        ShutDown(gText);
    }
}

// PoL 2.0 Misc.cpp FindToken correspondence.
VA(0x0045dd14, 0x65)
char* FindToken(char* text, char token) {
    int pos;
    int len;

    len = strlen(text);
    for (pos = 0; len > pos; pos++) {
        if (text[pos] == token)
            return text + pos;
    }
    return NULL;
}

// PoL 2.0 Misc.cpp FindLastToken correspondence.
VA(0x0045dd79, 0x63)
char* FindLastToken(char* text, char token) {
    int pos;
    int len;

    len = strlen(text);
    for (pos = len - 1; pos >= 0; pos--) {
        if (text[pos] == token)
            return text + pos;
    }
    return NULL;
}

// kbwin owns retail .data 0x0049fe50-0x004a0503 and .bss 0x004ca490-0x004ca903.
DATA(0x0049fe50)
char szAppName[] = "Heroes";
DATA(0x0049fe58)
char szTitle[] = "Heroes of Might and Magic";
DATA(0x0049fe74)
void* hwndApp = 0;
DATA(0x0049fe78)
int gbForegroundApp = 0;
DATA(0x0049fe7c)
void* hmnuApp = 0;
DATA(0x0049fe80)
void* gEventHandle = 0;
DATA(0x0049fef4)
long lLastGTimerTickCount = 0;
DATA(0x0049fef8)
long lLastCycleTickCount = 0;
DATA(0x0049fefc)
int gbClosingApp = 0;
DATA(0x0049ff2c)
long lLastGetMessage = 0;
DATA(0x0049ff30)
long lLastAilServe = 0;
DATA(0x0049ff50)
int gbNoDialogMenusOn = 0;
DATA(0x004ca490)
void* hInstApp;
DATA(0x004ca498)
struct tagRECT rcTemp;
DATA(0x004ca4a8)
int iMainWinScreenHeight;
DATA(0x004ca4ac)
void* hmnuCurrent;
DATA(0x004ca4b0)
int iTempX;
DATA(0x004ca4b4)
int iTempY;
DATA(0x004ca4b8)
long lTemp;
DATA(0x004ca4c0)
unsigned char bProcessMessage[KBWIN_MESSAGE_FILTER_SIZE];
DATA(0x004ca8c0)
char gcCommandLine[KBWIN_COMMAND_LINE_CLEAR_SIZE];
DATA(0x004ca900)
int iMainWinScreenWidth;

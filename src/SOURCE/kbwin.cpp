// Located from HoMM2 Buka 2.1; PoL 2.0 supplies the VC4 declaration.

#include <match.h>

#include <SOURCE/kbwin.h>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdio.h>

#include <BASE/Misc.h>
#include <BASE/MOUSEMGR_TYPES.h>
#include <BASE/soundmgr.h>
#include <H1/KB.h>
#include <H1/All.h>
#include <SOURCE/wingraph.h>

// donor PoL RVA 0x0001bce0; preferred Buka symbol _WinMain@16
// donor Buka TU SOURCE/kbwin; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.651055;margin=0.328202;shape=0.658;size=0.820;calls=1.000;alternate=pol20:_WinMain@16@0x0001bce0
VA(0x0045b6f0, 0x14e)
H1_C_LINKAGE int __stdcall WinMain(void*, void*, char*, int) {
    return 0;
}

// donor PoL RVA 0x0001be26; preferred Buka symbol ?AppInit@@YIHPAX0HPAD@Z
// donor Buka TU SOURCE/kbwin; HoMM1 owner inferred from contiguous order
// evidence: graph:1;base=0.682496;margin=0.205177;shape=0.345;size=0.971;calls=0.867;strings=Heroes|hInstApp;alternate=pol20:int AppInit(void *, void *, int, char *)@0x0001be26
VA(0x0045b83e, 0x2d6)
int AppInit(void*, void*, int, char*) {
    return 0;
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
long int __stdcall AppWndProc(void*, unsigned int, unsigned int, long int) {
    return 0;
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
    int windowX;
    RECT windowRect;
    int targetY;
    if (gConfig.gfx[giCurExe].fullScreen != 0)
        return;
    GetWindowRect(hwndApp, &windowRect);
    if (x == -1)
        windowX = windowRect.left;
    else
        windowX = x;
    if (y == -1)
        targetY = windowRect.top;
    else
        targetY = y;
    windowRect.left = 0;
    windowRect.top = 0;
    windowRect.right = width - 1;
    windowRect.bottom = height - 1;
    AdjustWindowRect(&windowRect, giCurWindowsStyleFlags, gConfig.gfx[giCurExe].showMenu);
    MoveWindow(
        hwndApp,
        windowX,
        targetY,
        windowRect.right - windowRect.left + 1,
        windowRect.bottom - windowRect.top + 1,
        1
    );
    gConfig.gfx[giCurExe].x = windowX;
    gConfig.gfx[giCurExe].y = targetY;
    gConfig.gfx[giCurExe].width = width;
    gConfig.gfx[giCurExe].height = height;
    WritePrefs();
}

// donor PoL RVA 0x0001c9c7; preferred Buka symbol ?AppCommand@@YIJPAXIIJ@Z
// donor Buka TU SOURCE/kbwin; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.642433;margin=0.651384;shape=0.267;size=0.907;calls=1.000;strings=HEROES;alternate=pol20:long int AppCommand(void *, unsigned int, unsigned int, long int)@0x0001c9c7
VA(0x0045c3f7, 0x185)
long int AppCommand(void*, unsigned int, unsigned int, long int) {
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
    if (menu == 0)
        menu = hmnuCurrent;
    else
        hmnuCurrent = menu;
    hmnuApp = menu;
    if (gConfig.gfx[giCurExe].showMenu) {
        if (menu != 0) {
            SetMenu(hwndApp, menu);
            UpdateDfltMenu(menu);
            UpdateAppSpecificMenus(menu);
            DrawMenuBar(hwndApp);
        }
    } else {
        SetMenu(hwndApp, 0);
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
    KBChangeMenu(0);
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
    int count;
    unsigned int commandId;
    int scanPosition;
    int commandPosition;
    int disableFlag;
    int index;

    count = GetMenuItemCount(static_cast<HMENU>(menu));
    for (index = 0; index < count; index++) {
        commandId = GetMenuItemID(static_cast<HMENU>(menu), index);
        if (commandId == static_cast<unsigned int>(-1)) {
            SetMenus(GetSubMenu(static_cast<HMENU>(menu), index), enabled);
            disableFlag = 0;
        } else {
            disableFlag = 0;
            if (enabled) {
                disableFlag = 1;
            } else {
                scanPosition = 0;
                for (commandPosition = 0; commandPosition < KBWIN_MENU_ENTRY_COUNT;
                     commandPosition++) {
                    if (gsMenuEnableStatus[commandPosition].command == commandId)
                        scanPosition = commandPosition;
                }
                if (gbInSetupDialog)
                    disableFlag = 1 - gsMenuEnableStatus[scanPosition].setupEnabled;
                else
                    disableFlag = 1 - gsMenuEnableStatus[scanPosition].normalEnabled;
            }
        }
        if (disableFlag != 0)
            EnableMenuItem(
                static_cast<HMENU>(menu),
                commandId,
                enabled == 0 ? MF_GRAYED : MF_ENABLED
            );
    }
    UpdateDfltMenu(menu);
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
            message.payload.widget.command = WIDGET_COMMAND_SET_TEXT;
            message.payload.widget.id = gWinSetup[i].widgetId;
            message.payload.widget.data.text = gWinSetupText[i];
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

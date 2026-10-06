// The editor's program unit (Editor\EDITOR.CPP): start-up, shut-down, the
// main classes, the shared delay and dialog helpers, the status bar and the
// application-menu hooks kbwin calls. It keeps the game's KB.cpp names for
// the functions both programs define.
// Descriptive names: ShowStatusText, ClearStatusText, gStatusTextShown,
// gStatusTextClearTime, gStatusTextHoldTime, gStatusText,
// gCommandLineInterpreted.

#include <match.h>

#include <EDITOR/EDITOR.h>

#include <BASE/baseManager.h>
#include <BASE/bmap2.h>
#include <BASE/executive.h>
#include <BASE/font.h>
#include <BASE/heroWindow.h>
#include <BASE/heroWindowManager.h>
#include <BASE/inputManager.h>
#include <BASE/message.h>
#include <BASE/Misc.h>
#include <BASE/miscwin.h>
#include <BASE/mouseManager.h>
#include <BASE/palette.h>
#include <BASE/resourceManager.h>
#include <EDITOR/editManager.h>
#include <SOURCE/appMenu.h>
#include <SOURCE/KB.h>
#include <SOURCE/kbwin.h>
#include <SOURCE/mapCell.h>
#include <SOURCE/NOOPT.h>
#include <SOURCE/wingraph.h>

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define EDITOR_CPP_PATH "U:\\HMM\\VSS\\HMM1\\Source\\Editor\\EDITOR.CPP"

DATA(0x0043ede4)
i8 gCommandLineInterpreted = 1;
DATA(0x00451ea4)
editManager* gpEditManager;
DATA(0x004528d0)
i32 gStatusTextShown;
DATA(0x004528f0)
i32 gStatusTextClearTime;
DATA(0x00452174)
i32 gStatusTextHoldTime;
DATA(0x00451ea8)
char gStatusText[EDITOR_STATUS_TEXT_CAPACITY];

VA(0x004084a5, 0x5)
void PollSound() {}

VA(0x004084aa, 0x106)
i32 oldmain(void) {
    palette* editorPalette;

    if (H1_ENUM_ENCODE(BaseManagerStatus, gpExec->InitSystem()))
        ShutDown(localization::Tr("editor.startup.initialize.failed"));
    KBChangeMenu(hmnuDflt);
    editorPalette = gpResourceManager->GetPalette("kb.pal");
    PostprocessPalette(editorPalette->m_data);
    gMapX = 0;
    gMapY = 0;
    gpWindowManager->FadeScreen(WINDOW_FADE_OUT, WINDOW_FADE_STEPS_NORMAL, editorPalette);
    if (H1_ENUM_ENCODE(BaseManagerStatus, gpExec->AddManager(gpEditManager, BASE_MANAGER_PRIORITY_UNASSIGNED)))
        ShutDown(localization::Tr("startup.manager.failed"));
    gpWindowManager->FadeScreen(WINDOW_FADE_IN, WINDOW_FADE_STEPS_SHORT, editorPalette);
    gpExec->MainLoop();
    gpExec->RemoveManager(gpEditManager);
    gpWindowManager->FadeScreen(WINDOW_FADE_OUT, WINDOW_FADE_STEPS_SHORT, editorPalette);
    gpResourceManager->Dispose(editorPalette);
    ShutDown(NULL);
    return 0;
}

VA(0x004085d1, 0x2e)
void DelayTicks(i32 ticks) {
    i32 unused = 0;

    glTimers[DELAY_TICKS_TIMER_SLOT] = KBTickCount() + ticks * DELAY_TICK_MILLISECONDS;
    DelayTil(glTimers + DELAY_TICKS_TIMER_SLOT);
}

VA(0x004085ff, 0x3b)
#line 89 EDITOR_CPP_PATH
void DelayTil(i32* endTime) {
#line 90
    H1_ASSERT(*endTime > 10000);
    while (*endTime > KBTickCount()) {
        Process1WindowsMessage();
        PollSound();
    }
}

VA(0x0040863a, 0x16)
void DelayMilli(i32 delay) {
    DelayTilMilli(KBTickCount() + delay);
}

VA(0x00408650, 0x1b)
void DelayTilMilli(i32 endTime) {
    while (endTime > KBTickCount()) {
        Process1WindowsMessage();
        PollSound();
    }
}

VA(0x0040866b, 0x34)
void FileError(char* filename) {
    char message[200];
    sprintf(message, localization::Tr("file.open.failed"), filename);
    ShutDown(message);
}

VA(0x0040869f, 0xa7)
void ShutDown(char* message) {
    char buffer[300];
    if (message) {
        strcpy(buffer, message);
        SetFullScreenStatus(0);
        MessageBoxA(
            hwndApp,
            buffer,
            localization::Tr("shutdown.unexpected.title"),
            MB_ICONHAND
        );
    }
    gClosingApp = 1;
    gpExec->ShutDownSystem();
    if (gEventHandle) {
        CloseHandle(gEventHandle);
        gEventHandle = NULL;
    }
    DeleteMainClasses();
    AppExit();
    exit(0);
}

// The /D (debug level) and /B (mouse masks) command-line switches.
VA(0x00408746, 0xd9)
i32 InterpretCommandLine(void) {
    i32 size;
    i32 i;

    gSpecialMouseMasks = 1;
    giDebugLevel = DEBUG_LEVEL_NONE;
    size = strlen(gCommandLine);
    for (i = 0; i < size; i++) {
        if (gCommandLine[i] == '/' && i + 1 < size) {
            switch (toupper(gCommandLine[i + 1])) {
                case 'D':
                    if (i + 2 < size)
                        giDebugLevel = gCommandLine[i + 2] - '0';
                    break;
                case 'B':
                    if (i + 2 < size)
                        gSpecialMouseMasks = gCommandLine[i + 2] - '0';
                    break;
            }
        }
    }
    gCommandLineInterpreted = 1;
    return 1;
}

VA(0x00408983, 0x12)
void MemError(void) {
    ShutDown("Out of Memory");
}

VA(0x00408995, 0x39)
bool IsCDDrive(i32 driveIndex) {
    sprintf(gText, "A:\\");
    gText[0] += driveIndex;
    return GetDriveTypeA(gText) == DRIVE_CDROM;
}

VA(0x00409056, 0x7a)
void QuickViewWait(void) {
    tag_message event;
    i32 done = 0;
    while (!done) {
        PollSound();
        Process1WindowsMessage();
        event = gpInputManager->GetEvent();
        done = event.type == MESSAGE_RIGHT_BUTTON_UP || event.type == MESSAGE_LEFT_BUTTON_DOWN
               || event.type == MESSAGE_LEFT_BUTTON_UP;
    }
}

// Shows `text` (or, while a text is shown, the current one) in the status bar.
VA(0x004090d0, 0xbe)
void ShowStatusText(char* text) {
    if (gStatusTextShown && !text)
        text = gStatusText;
    else
        strcpy(gStatusText, text);
    gStatusTextShown = 1;
    gStatusTextHoldTime = KBTickCount() + EDITOR_STATUS_TEXT_HOLD_MILLISECONDS;
    FillBitmapArea(
        gpWindowManager->m_screen,
        EDITOR_STATUS_BAR_X,
        EDITOR_STATUS_BAR_Y,
        EDITOR_STATUS_BAR_WIDTH,
        EDITOR_STATUS_BAR_HEIGHT,
        0
    );
    gpEditManager->m_statusFont->DrawBoundedString(
        text,
        EDITOR_STATUS_BAR_X,
        EDITOR_STATUS_BAR_Y,
        EDITOR_STATUS_BAR_WIDTH,
        EDITOR_STATUS_BAR_HEIGHT,
        1,
        FONT_ALIGN_CENTER
    );
    gpWindowManager->UpdateScreenRegion(
        EDITOR_STATUS_BAR_X,
        EDITOR_STATUS_BAR_Y,
        EDITOR_STATUS_BAR_WIDTH,
        EDITOR_STATUS_BAR_HEIGHT
    );
}

VA(0x0040918e, 0x4d)
void ClearStatusText(void) {
    gStatusTextClearTime = 0;
    if (gStatusTextShown) {
        gStatusTextShown = 0;
        gpEditManager->m_window->DrawWindow(0);
        gpWindowManager->UpdateScreenRegion(
            EDITOR_STATUS_BAR_X,
            EDITOR_STATUS_BAR_Y,
            EDITOR_STATUS_BAR_WIDTH,
            EDITOR_STATUS_BAR_HEIGHT
        );
    }
}

VA(0x004091db, 0x5)
void UpdateAppSpecificMenus(void*) {}

VA(0x004091e0, 0x3c)
void CleanUpMenus(void) {
    if (hmnuApp) {
        SetMenu(hwndApp, NULL);
        if (hmnuDflt)
            DestroyMenu(hmnuDflt);
    }
    hmnuApp = NULL;
}

VA(0x0040921c, 0x1b)
void EarlyShutDownSystem(void) {
    if (gpEditManager)
        gpEditManager->SelectTool(EDIT_MANAGER_NO_TOOL);
}

// The editor always asks before quitting.
VA(0x00409237, 0xa)
i32 GameUnsaved(void) {
    return 1;
}

VA(0x00409241, 0x37)
i32 HandleAppSpecificMenuCommands(i32 command) {
    switch (command) {
        case APP_MENU_QUIT:
            PostMessageA(hwndApp, WM_CLOSE, 0, 0);
            break;
        default:
            return 1;
    }
    return 0;
}

VA(0x00409278, 0x5)
void EarlyResizeWindow(i32, i32, i32, i32) {}

VA(0x0040927d, 0x5)
void UpdateSystemOptionsMenu(void) {}

// The editor's program unit (Editor\EDITOR.CPP): start-up, shut-down, the
// main classes, the shared delay and dialog helpers, the status bar and the
// application-menu hooks kbwin calls. It keeps the game's KB.cpp names for
// the functions both programs define.
// Descriptive names: ShowStatusText, ClearStatusText, gStatusTextShown,
// gStatusTextClearTime, gStatusTextHoldTime, gStatusText,
// gCommandLineInterpreted, gpMapHeader, gNewMapFormat, gSelectionX,
// gSelectionY, gSelectionWidth, gSelectionHeight, gGeneratingMaps,
// gNextCellOwner, gEditButtonHelp, gEditAreaHelp, EditorStartupHook, EditorIdleHook,
// IncrementArgumentA, IncrementArgumentB.

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
#include <BASE/textWidget.h>
#include <EDITOR/editManager.h>
#include <SOURCE/appMenu.h>
#include <SOURCE/dialogTypes.h>
#include <SOURCE/game.h>
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
DATA(0x00451e98)
SMapHeader* gpMapHeader;
DATA(0x0043f398)
i32 gNewMapFormat = 1;
DATA(0x0043f3a0)
i32 gSelectionX = -1;
DATA(0x00451f70)
i32 gSelectionY;
DATA(0x00451e9c)
i32 gSelectionWidth;
DATA(0x00451f80)
i32 gSelectionHeight;
DATA(0x004528e4)
i32 gGeneratingMaps;
DATA(0x0045245c)
i16 gNextCellOwner;
DATA(0x0043f744)
char* gEditButtonHelp[10] = {
    "",
    localization::Tr("table.gEditButtonHelp.1"),
    localization::Tr("table.gEditButtonHelp.2"),
    localization::Tr("table.gEditButtonHelp.3"),
    localization::Tr("table.gEditButtonHelp.4"),
    localization::Tr("table.gEditButtonHelp.5"),
    localization::Tr("table.gEditButtonHelp.6"),
    localization::Tr("table.gEditButtonHelp.7"),
    localization::Tr("table.gEditButtonHelp.8"),
    localization::Tr("table.gEditButtonHelp.9"),
};
DATA(0x0043f76c)
char* gEditAreaHelp[8] = {
    "",
    localization::Tr("table.gEditAreaHelp.1"),
    localization::Tr("table.gEditAreaHelp.2"),
    localization::Tr("table.gEditAreaHelp.3"),
    localization::Tr("table.gEditAreaHelp.4"),
    localization::Tr("table.gEditAreaHelp.5"),
    localization::Tr("table.gEditAreaHelp.6"),
    localization::Tr("table.gEditAreaHelp.7"),
};

VA(0x004084a0, 0x5)
void EditorStartupHook(void) {}

VA(0x004084a5, 0x5)
void PollSound() {}

VA(0x004084aa, 0x106)
i32 oldmain(void) {
    palette* editorPalette;

    if (gpExec->InitSystem())
        ShutDown(localization::Tr("editor.startup.initialize.failed"));
    KBChangeMenu(hmnuDflt);
    editorPalette = gpResourceManager->GetPalette("kb.pal");
    PostprocessPalette(editorPalette->m_data);
    gMapX = 0;
    gMapY = 0;
    gpWindowManager->FadeScreen(WINDOW_FADE_OUT, WINDOW_FADE_STEPS_NORMAL, editorPalette);
    if (gpExec->AddManager(gpEditManager, BASE_MANAGER_PRIORITY_UNASSIGNED))
        ShutDown(localization::Tr("startup.manager.failed"));
    gpWindowManager->FadeScreen(WINDOW_FADE_IN, WINDOW_FADE_STEPS_SHORT, editorPalette);
    gpExec->MainLoop();
    gpExec->RemoveManager(gpEditManager);
    gpWindowManager->FadeScreen(WINDOW_FADE_OUT, WINDOW_FADE_STEPS_SHORT, editorPalette);
    gpResourceManager->Dispose(editorPalette);
    ShutDown(NULL);
    return 0;
}

VA(0x004085b0, 0xe)
void IncrementArgumentA(i32 value) {
    value++;
}

VA(0x004085be, 0x5)
void EditorIdleHook(void) {}

VA(0x004085c3, 0xe)
void IncrementArgumentB(i32 value) {
    value++;
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

VA(0x0040881f, 0x164)
i32 EarlySetup(void) {
    DATA(0x004528f5)
    static i8 gEarlySetupDone = 0;
    i32 i;

    if (gEarlySetupDone)
        return 0;
    sprintf(cAggPathName, "%s%s", gDataPath, "heroes.agg");
    DEFAULT_AGGREGATE_NAME = cAggPathName;
    InitMainClasses();
    GetGraphicsInfo();
    ReadPrefs();
    if (!InterpretCommandLine())
        return 1;
    switch (SetupCDDrive()) {
        case CD_SETUP_NO_DRIVE:
            MessageBoxA(
                hwndApp,
                localization::Tr("startup.cd.inaccessible"),
                localization::Tr("startup.error.title"),
                MB_ICONHAND
            );
            exit(0);
            break;
        case CD_SETUP_NOT_FOUND:
            MessageBoxA(
                hwndApp,
                localization::Tr("editor.startup.cd.required"),
                localization::Tr("startup.error.title"),
                MB_ICONHAND
            );
            exit(0);
            break;
        case CD_SETUP_NO_APP_PATH:
            MessageBoxA(
                hwndApp,
                localization::Tr("startup.directory.invalid"),
                localization::Tr("startup.error.title"),
                MB_ICONHAND
            );
            exit(0);
            break;
        case CD_SETUP_NO_DATA:
            MessageBoxA(
                hwndApp,
                localization::Tr("startup.data.missing"),
                localization::Tr("startup.error.title"),
                MB_ICONHAND
            );
            exit(0);
            break;
    }
    hmnuDflt = LoadMenuA(hInstApp, "mnuDflt");
    for (i = 0; i < MAP_CELL_GROUND_TILE_COUNT; i++)
        giGroundToTerrain[i] = i / MAP_CELL_TILES_PER_TERRAIN;
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

VA(0x004089ce, 0x20c)
void InitMainClasses(void) {
    gpExec = new executive;
    gpInputManager = new inputManager;
    gpMouseManager = new mouseManager;
    gpWindowManager = new heroWindowManager;
    gpResourceManager = new resourceManager;
    gpBufferPalette = new palette;
    gpEditManager = new editManager;
}

VA(0x00408bda, 0x137)
void DeleteMainClasses(void) {
    if (gpEditManager)
        delete gpEditManager;
    gpEditManager = NULL;
    if (gpBufferPalette)
        delete gpBufferPalette;
    gpBufferPalette = NULL;
    if (gpResourceManager)
        delete gpResourceManager;
    gpResourceManager = NULL;
    if (gpWindowManager)
        delete gpWindowManager;
    gpWindowManager = NULL;
    if (gpMouseManager)
        delete gpMouseManager;
    gpMouseManager = NULL;
    if (gpInputManager)
        delete gpInputManager;
    gpInputManager = NULL;
    if (gpExec)
        delete gpExec;
    gpExec = NULL;
}

// The editor's dialogs carry text only: no resource, artifact or creature
// panels, and they open at the left of the map view.
VA(0x00408d11, 0x2b4)
void NormalDialog(
    char* text,
    H1_ENUM_PARAM(NormalDialogType, i32) dialogType,
    i32 x,
    i32 y,
    H1_ENUM_PARAM(NormalDialogResourceType, i32),
    i32,
    H1_ENUM_PARAM(NormalDialogResourceType, i32),
    i32,
    H1_ENUM_PARAM(NormalDialogOrText, i32)
) {
    // The resource-panel locals of the game's dialog survive unused (their
    // frame slots are retail's).
    i32 sizedHeight;
    i32 width;
    tag_message msg;
    i32 rows;
    char iconFile[NORMAL_DIALOG_FILENAME_LENGTH];
    i32 wrappedLines;
    i32 totalHeight;
    i32 iconFrameIndex;
    i16 showMessageText;
    i32 nextId;
    i32 panelHeight;
    i32 frameHeight;
    textWidget* captionText;
    i32 index;
    i32 resourceIconY;
    i32 tallestImage;
    font* bigFont;
    i32 resWidth;
    char* orWord;

    nextId = NORMAL_DIALOG_TEXT_WIDGET_FIRST_ID;
    showMessageText = 1;
    bigFont = gpResourceManager->GetFont("bigfont.fnt");
    wrappedLines = bigFont->LineLength(text, NORMAL_DIALOG_TEXT_LINE_WIDTH);
    gpResourceManager->Dispose(bigFont);
    totalHeight = wrappedLines * NORMAL_DIALOG_TEXT_LINE_HEIGHT;
    if (dialogType != NORMAL_DIALOG_TYPE_QUICK_VIEW)
        totalHeight += NORMAL_DIALOG_BUTTON_AREA_HEIGHT;
    rows = (totalHeight - 12) / NORMAL_DIALOG_WINDOW_ROW_HEIGHT;
    if (rows > NORMAL_DIALOG_MAX_ROWS)
        rows = NORMAL_DIALOG_MAX_ROWS;
    if (rows <= 0 && dialogType != NORMAL_DIALOG_TYPE_QUICK_VIEW)
        rows = 1;
    width = NORMAL_DIALOG_WINDOW_WIDTH;
    panelHeight = rows * NORMAL_DIALOG_WINDOW_ROW_HEIGHT + NORMAL_DIALOG_WINDOW_BASE_HEIGHT;

    if (x == NORMAL_DIALOG_AUTO_POSITION || width + x >= LOGICAL_SCREEN_WIDTH - 1)
        x = NORMAL_DIALOG_ADVENTURE_X;
    if (y == NORMAL_DIALOG_AUTO_POSITION || panelHeight + y >= LOGICAL_SCREEN_HEIGHT - 1) {
        y = (LOGICAL_SCREEN_HEIGHT - panelHeight) / 2;
        if (y > NORMAL_DIALOG_MAX_TOP)
            y = NORMAL_DIALOG_MAX_TOP;
    }

    sprintf(iconFile, "evntwin%d.bin", rows);
    gNormalDialogWindow = new heroWindow(x, y, iconFile);
    if (!gNormalDialogWindow)
        MemError();

    msg.type = MESSAGE_WIDGET;
    msg.command = WIDGET_COMMAND_CLEAR_FLAGS;
    msg.value = NORMAL_DIALOG_BUTTON_FLAGS;
    if (dialogType != NORMAL_DIALOG_TYPE_WAIT_CANCEL
        && dialogType != NORMAL_DIALOG_TYPE_NO_BUTTONS) {
        msg.id = NORMAL_DIALOG_BUTTON_OK;
        gNormalDialogWindow->BroadcastMessage(msg);
    }
    if (dialogType != NORMAL_DIALOG_TYPE_WAIT_OK && dialogType != NORMAL_DIALOG_TYPE_OK
        && dialogType != NORMAL_DIALOG_TYPE_NO_BUTTONS) {
        msg.id = NORMAL_DIALOG_BUTTON_CANCEL;
        gNormalDialogWindow->BroadcastMessage(msg);
    }
    if (dialogType != NORMAL_DIALOG_TYPE_YES_NO) {
        msg.id = NORMAL_DIALOG_BUTTON_YES;
        gNormalDialogWindow->BroadcastMessage(msg);
        msg.id = NORMAL_DIALOG_BUTTON_NO;
        gNormalDialogWindow->BroadcastMessage(msg);
    }

    SET_WIDGET_MESSAGE(msg, WIDGET_COMMAND_SET_TEXT, NORMAL_DIALOG_TEXT_WIDGET_ID);
    msg.text = text;
    gNormalDialogWindow->BroadcastMessage(msg);

    if (dialogType == NORMAL_DIALOG_TYPE_QUICK_VIEW) {
        gpMouseManager->ReallyHidePointer();
        gpWindowManager->AddWindow(gNormalDialogWindow, WINDOW_Z_ORDER_APPEND, 1);
        QuickViewWait();
        gpWindowManager->RemoveWindow(gNormalDialogWindow);
        gpMouseManager->ReallyShowPointer();
    } else {
        gpWindowManager->DoDialog(gNormalDialogWindow, EventWindowHandler, 0);
    }
    delete gNormalDialogWindow;
}

VA(0x00408fc5, 0x91)
i16 EventWindowHandler(tag_message& message) {
    if (message.type == MESSAGE_WIDGET) {
        switch (message.command) {
            case WIDGET_NOTIFY_DESELECT:
                switch (message.id) {
                    case CAMPAIGN_INFO_RESTART:
                    case DIALOG_BUTTON_0:
                    case DIALOG_BUTTON_1:
                    case DIALOG_BUTTON_2:
                    case DIALOG_BUTTON_3:
                    case DIALOG_BUTTON_5:
                    case DIALOG_BUTTON_6:
                        FINISH_DIALOG_MESSAGE(message);
                        return MESSAGE_DISPATCH_FORWARD;
                    default:
                        break;
                }
                break;
            default:
                break;
        }
    }
    return MESSAGE_DISPATCH_CONSUME;
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
        1
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

// Located from HoMM2 Buka 2.1; PoL 2.0 supplies the VC4 declaration.

#include <match.h>

#include <BASE/BITS.h>
#include <BASE/BMAP2.h>
#include <BASE/INPUTMGR_TYPES.h>
#include <BASE/Misc.h>
#include <H1/All.h>
#include <H1/KB.h>
#include <BASE/MISC_TYPES.h>
#include <BASE/MOUSEMGR_TYPES.h>
#include <BASE/WINMGR_TYPES.h>
#include <BASE/soundmgr.h>
#include <SOURCE/appMenu.h>
#include <SOURCE/artifactTypes.h>
#include <SOURCE/campaignTypes.h>
#include <SOURCE/creatureTypes.h>
#include <SOURCE/highScoreRuntime.h>
#include <SOURCE/kbwin.h>
#include <SOURCE/dialogTypes.h>
#include <SOURCE/NOOPT.h>
#include <SOURCE/REMOTE.h>
#include <SOURCE/resourceTypes.h>
#include <SOURCE/smackManager.h>
#include <SOURCE/wingraph.h>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <fcntl.h>
#include <io.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>

// Retail score-dialog owner bytes; initializer coverage is deferred.
DATA(0x00494170)
signed char gbShowHighScore;
DATA(0x004c794c)
signed char gbStandardHighScore;
DATA(0x00494128)
signed char gbHeroWindShowing;
DATA(0x0049412c)
signed char gbOverviewShowing;
// InitVars proves seven terrain rows, ordinary/diagonal cost columns.
DATA(0x004c6d50)
signed char giTerrainCost[FINDPATH_TERRAIN_COUNT][FINDPATH_STEP_COST_COUNT];

// HoMM2 KB.cpp confirms the identity and behavior. HoMM1 differs in the timer
// comparison and placement of the re-entry guard.
extern "C" VA(0x0044f640, 0x72)
void PollSound() {
    if (KBTickCount() < gNextSoundPollTick)
        return;
    if (gbInPollSound)
        return;
    gbInPollSound = 1;
    gNextSoundPollTick = KBTickCount() + 30;
    if (gbForegroundApp)
        gpSoundManager->PollSound();
    PollRemote();
    gbInPollSound = 0;
}

VA(0x0044f6b2, 0x20)
void ForcePollSound() {
    gNextSoundPollTick = KBTickCount() - 1;
    PollSound();
}

// donor PoL RVA 0x000965be; preferred Buka symbol ?InitMainClasses@@YIXXZ
// donor Buka TU SOURCE/KB; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.512387;margin=0.755802;shape=0.400;size=0.925;calls=0.653;alternate=pol20:void InitMainClasses(void)@0x000965be
VA(0x0044f6d2, 0x607)
void InitMainClasses(void) {
    gpExec = new executive;
    gpInputManager = new inputManager;
    gpMouseManager = new mouseManager;
    gpWindowManager = new heroWindowManager;
    gpResourceManager = new resourceManager;
    gpSoundManager = new soundManager;
    gpSmackManager = new smackManager;
    gpHighScoreManager = new highScoreManager;
    gpGame = new game;
    gpAdvManager = new advManager;
    gpCombatManager = new combatManager;
    gpTownManager = new townManager;
    gpSearchArray = new searchArray;
    gpPhilAI = new philAI;
    gpMonGroup = new armyGroup;
    gpBufferPalette = new palette;
}

// Buka 2.1 DeleteMainClasses; HoMM1 also owns the smacker manager and frees the
// resource manager before the window, mouse and input managers.
VA(0x0044fcd9, 0x36d)
void DeleteMainClasses(void) {
    if (gpBufferPalette)
        delete gpBufferPalette;
    gpBufferPalette = NULL;
    if (gpMonGroup)
        delete gpMonGroup;
    gpMonGroup = NULL;
    if (gpPhilAI)
        delete gpPhilAI;
    gpPhilAI = NULL;
    if (gpSearchArray)
        delete gpSearchArray;
    gpSearchArray = NULL;
    if (gpTownManager)
        delete gpTownManager;
    gpTownManager = NULL;
    if (gpCombatManager)
        delete gpCombatManager;
    gpCombatManager = NULL;
    if (gpAdvManager)
        delete gpAdvManager;
    gpAdvManager = NULL;
    if (gpGame)
        delete gpGame;
    gpGame = NULL;
    if (gpHighScoreManager)
        delete gpHighScoreManager;
    gpHighScoreManager = NULL;
    if (gpSmackManager)
        delete gpSmackManager;
    gpSmackManager = NULL;
    if (gpSoundManager)
        delete gpSoundManager;
    gpSoundManager = NULL;
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

// donor PoL RVA 0x00096e21; preferred Buka symbol ?EarlySetup@@YIHXZ
// donor Buka TU SOURCE/KB; HoMM1 owner inferred from contiguous order
// evidence: graph:3;base=0.257149;margin=0.511941;shape=0.213;size=0.338;calls=0.600;alternate=pol20:int EarlySetup(void)@0x00096e21
VA(0x00450046, 0x116)
int EarlySetup(void) {
    int iCDRomErr;

    if (bEarlySetupDone)
        return 0;
    InitMainClasses();
    GetGraphicsInfo();
    ReadPrefs();
    if (!InterpretCommandLine())
        return 1;
    LogTruncate();
    iCDRomErr = SetupCDDrive();
    if (iCDRomErr == 1) {
        MessageBoxA((HWND)hwndApp, "Unable to access CD Drive.", "Startup Error", MB_ICONHAND);
        exit(0);
    }
    if (iCDRomErr == 2) {
        MessageBoxA((HWND)hwndApp,
                    "You must have the Heroes Win95 CD in the CD-ROM drive to play \nHeroes of "
                    "Might and Magic.  \n\nPlease insert the CD and try again.",
                    "Startup Error", MB_ICONHAND);
        exit(0);
    }
    if (iCDRomErr == 3) {
        MessageBoxA((HWND)hwndApp,
                    "Unable to change to the Heroes directory.  Please run the installation "
                    "program.",
                    "Startup Error", MB_ICONHAND);
        exit(0);
    }
    if (iCDRomErr == 4) {
        MessageBoxA((HWND)hwndApp,
                    "Unable to find the Heroes data files.  Please run the installation program.",
                    "Startup Error", MB_ICONHAND);
        exit(0);
    }
    InitVars();
    return 1;
}


// oldmain's re-entry guard and the intro, end-sequence and campaign state it
// shares with the game screens.
extern signed char bKBDone;
extern signed char gbSkipIntro;
extern signed char gbWaitForRemoteReceive;
extern int gbDirectConnect;
extern short giLastMapOriginX;
extern short giLastMapOriginY;
extern char gcCongratsText[];
extern char* gCampaignSideNames[];
extern signed char giCampaignChoice;
extern int giMenuCommand;
void ShowCongrats(void);
short InitMenuHandler(tag_message&);
int AddScoreToHighScore(int, int, char*, char*);

// Buka 2.1 oldmain reduced to HoMM1: two intro videos, the stpmain.bin
// menu (new, load, campaign, high scores, credits, quit), one network
// handshake and the campaign replay/next-scenario loop.
VA(0x0045015c, 0xe22)
int oldmain(void) {
    char saveBuf[20];
    char hiResVideos[3];
    char lowResVideos[3];
    int n;
    heroWindow* mainWin;
    font* font;
    signed char backdropLoaded;
    int idx;
    signed char initialMainScreen;
    signed char done;
    signed char leave;
    int result;
    short command;

    if (bKBDone)
        return 0;
    bKBDone = 1;
    command = -1;
    if (gpExec->InitSystem())
        ShutDown("Initialization failed!");
    CheckMem();
    KBChangeMenu(hmnuDflt);
    gPalette = gpResourceManager->GetPalette("kb.pal");
    PostprocessPalette(gPalette->m_data);
    SetPalette(gPalette->m_data, 1);
    gpWindowManager->m_updateFlags = 1;
    gpPhilAI->m_debugFont = gpResourceManager->GetFont("smalfont.fnt");
    if (giShowIntro) {
        FillBitmapArea(gpWindowManager->m_screen, 0, 0, LOGICAL_SCREEN_WIDTH, LOGICAL_SCREEN_HEIGHT, 0);
        BlitBitmapToScreen(gpWindowManager->m_screen, 0, 0, LOGICAL_SCREEN_WIDTH, LOGICAL_SCREEN_HEIGHT, 0, 0);
        font = gpResourceManager->GetFont("bigfont.fnt");
        font->DrawString("Loading Heroes of Might and Magic for Windows 95 (version 1.0)", 10, 10, 1);
        gpWindowManager->UpdateScreenRegion(10, 10, 600, 20);
        gpResourceManager->Dispose(font);
        if (!gbSkipIntro) {
            if (gConfig.slowVideo)
                PlaySmacker(1);
            else
                PlaySmacker(0);
        }
        if (gConfig.slowVideo)
            PlaySmacker(2);
        else
            PlaySmacker(3);
    }
    LoadSystemwideIcons();
    memset(gbThisNetHumanPlayer, 0, 4);
    leave = 0;
    backdropLoaded = 0;
    initialMainScreen = 1;

    while (!leave) {
    mainMenu:
        gpSoundManager->SwitchAmbientMusic(48);
        if (!backdropLoaded) {
            if (gGameCommand != 4) {
                gpResourceManager->GetBackdrop("heroes.bmp", gpWindowManager->m_screen);
                gpWindowManager->UpdateScreenRegion(0, 0, LOGICAL_SCREEN_WIDTH, LOGICAL_SCREEN_HEIGHT);
                if (initialMainScreen)
                    SetPalette(gPalette->m_data, 0);
                else
                    gpWindowManager->FadeScreen(0, 8, gPalette);
                initialMainScreen = 0;
            }
            gpMouseManager->SetPointer("advmice.mse", 0);
        }
        backdropLoaded = 1;
        if (gGameCommand != 4)
            gpWindowManager->m_updateFlags = 1;
        giCampaignChoice = 0;
        gpMouseManager->ReallyShowPointer();

        if (giMenuCommand != -1) {
        processMenuCommand:
            switch (giMenuCommand) {
                case APP_MENU_LOAD_STANDARD_GAME:
                case APP_MENU_LOAD_CAMPAIGN_GAME:
                case APP_MENU_LOAD_HOT_SEAT_2:
                case APP_MENU_LOAD_HOT_SEAT_3:
                case APP_MENU_LOAD_HOT_SEAT_4:
                case APP_MENU_LOAD_NETWORK_HOST:
                case APP_MENU_LOAD_NETWORK_GUEST:
                case APP_MENU_LOAD_MODEM_HOST:
                case APP_MENU_LOAD_MODEM_GUEST:
                case APP_MENU_LOAD_DIRECT_HOST:
                case APP_MENU_LOAD_DIRECT_GUEST:
                    if (!gpGame->PickLoadGame())
                        goto mainMenu;
                    break;
                case APP_MENU_NEW_STANDARD_GAME:
                case APP_MENU_NEW_CAMPAIGN_IRONFIST:
                case APP_MENU_NEW_CAMPAIGN_SLAYER:
                case APP_MENU_NEW_CAMPAIGN_LAMANDA:
                case APP_MENU_NEW_CAMPAIGN_ALAMAR:
                case APP_MENU_NEW_HOT_SEAT_2:
                case APP_MENU_NEW_HOT_SEAT_3:
                case APP_MENU_NEW_HOT_SEAT_4:
                case APP_MENU_NEW_NETWORK_HOST:
                case APP_MENU_NEW_NETWORK_GUEST:
                case APP_MENU_NEW_MODEM_HOST:
                case APP_MENU_NEW_MODEM_GUEST:
                case APP_MENU_NEW_DIRECT_HOST:
                case APP_MENU_NEW_DIRECT_GUEST:
                    if (!gpGame->NewGame())
                        goto mainMenu;
                    break;
            }
            goto gameSetupComplete;
        } else {
            if (gGameCommand != -1) {
                command = gGameCommand;
                gGameCommand = -1;
            } else {
                mainWin = new heroWindow(400, 35, "stpmain.bin");
                if (!mainWin)
                    MemError();
                gbInSetupDialog = 1;
                gpWindowManager->DoDialog(mainWin, InitMenuHandler, 0);
                delete mainWin;
                command = gpWindowManager->m_dialogResult;
                gbInSetupDialog = 0;
            }
        }
        if (giMenuCommand != -1)
            goto processMenuCommand;

        gpMouseManager->ReallyHidePointer();
        switch (command) {
            case 2:
                if (!gpGame->PickLoadGame())
                    goto mainMenu;
                break;
            case 5:
                if (gpExec->AddManager(gpHighScoreManager, -1))
                    ShutDown("Can't add manager!");
                gpExec->MainLoop();
                gpExec->RemoveManager(gpHighScoreManager);
                backdropLoaded = 0;
                goto mainMenu;
            case 1:
                if (!gpGame->NewGame())
                    goto mainMenu;
                break;
            case 6:
                gpWindowManager->FadeScreen(1, 8, gPalette);
                gpResourceManager->GetBackdrop("credits.bmp", gpWindowManager->m_screen);
                gpWindowManager->UpdateScreenRegion(0, 0, LOGICAL_SCREEN_WIDTH, LOGICAL_SCREEN_HEIGHT);
                gpWindowManager->FadeScreen(0, 8, gPalette);
                done = 0;
                gpInputManager->Flush();
                while (!done) {
                    Process1WindowsMessage();
                    switch (gpInputManager->GetEvent().type) {
                        case MESSAGE_KEY_DOWN:
                        case MESSAGE_LEFT_BUTTON_DOWN:
                        case MESSAGE_RIGHT_BUTTON_DOWN:
                            done = 1;
                    }
                }
                gpWindowManager->FadeScreen(1, 8, gPalette);
                gpResourceManager->GetBackdrop("heroes.bmp", gpWindowManager->m_screen);
                gpWindowManager->UpdateScreenRegion(0, 0, LOGICAL_SCREEN_WIDTH, LOGICAL_SCREEN_HEIGHT);
                gpWindowManager->FadeScreen(0, 8, gPalette);
                goto mainMenu;
            case 4:
                leave = 1;
                break;
        }

    gameSetupComplete:
        if (giMenuCommand != -1)
            goto processMenuCommand;
        if (!leave) {
            if (gbRemoteOn && !gbDirectConnect) {
                n = 0;
                for (idx = 0; idx < 4; idx++) {
                    if (gbHumanPlayer[idx]) {
                        gbGamePosToNetPos[idx] = n;
                        n++;
                    } else {
                        gbGamePosToNetPos[idx] = -1;
                    }
                }
                for (idx = 0; idx < 4; idx++)
                    memcpy(gText, gbGamePosToNetPos, 4);
                giThisGamePos = NetPosToGamePos(0);
                giHostGamePos = giThisGamePos;
                for (idx = 1; idx < giNumHumanPlayers; idx++) {
                    result = TransmitRemoteData(gText, idx, 4, BOX_REMOTE_SETUP, 1, 1, -1, 0);
                    if (!result)
                        ShutDown(NULL);
                }
                for (idx = 0; idx < gpGame->m_playerCount; idx++) {
                    if (gbHumanPlayer[idx] && !gbThisNetHumanPlayer[idx]) {
                        if (!gpGame->TransmitSaveGame(idx, 0))
                            ShutDown(NULL);
                    }
                }
            }
            if (gbRemoteOn && gbWaitForRemoteReceive) {
                giWaitType = 0;
                NormalDialog("Waiting for other remote player to set up game.", NORMAL_DIALOG_TYPE_WAIT_CANCEL, -1, -1, NORMAL_DIALOG_NO_RESOURCE, 0, NORMAL_DIALOG_NO_RESOURCE, 0, -1);
                if (!gbFunctionComplete)
                    ShutDown(NULL);
                gpGame->LoadGame("REMOTE.GAM", 0, 1);
                goto playScenario;
            }
        playScenario:
            if (gpGame->m_campaignType > 0) {
                if (!backdropLoaded) {
                    gpWindowManager->FadeScreen(1, 8, gPalette);
                    gpResourceManager->GetBackdrop("heroes.bmp", gpWindowManager->m_screen);
                    gpWindowManager->UpdateScreenRegion(0, 0, LOGICAL_SCREEN_WIDTH, LOGICAL_SCREEN_HEIGHT);
                    gpWindowManager->FadeScreen(0, 8, gPalette);
                    backdropLoaded = 1;
                }
                gpGame->ShowCampaignInfo(gpGame->m_campaignScenario, 0, 0);
            }
            gbGameInitialized = 1;
            backdropLoaded = 0;
            gpSoundManager->StopAllSamples();
            gpWindowManager->FadeScreen(1, 8, NULL);
            giLastMapOriginX = 0;
            giLastMapOriginY = 0;
            if (gpExec->AddManager(gpAdvManager, -1))
                ShutDown("Can't add manager!");
            if (command == 1)
                gpAdvManager->SetHeroContext(gpGame->m_players[0].NextHero(0), 0);
            gpExec->MainLoop();
            giLastMapOriginX = gpAdvManager->m_mapOriginX;
            giLastMapOriginY = gpAdvManager->m_mapOriginY;
            gpExec->RemoveManager(gpAdvManager);
            gpWindowManager->FadeScreen(1, 8, gPalette);
        }

        if (gbGameOver) {
            RemoteCleanup();
            bShowIt = 1;
            gpMouseManager->SetPointer("advmice.mse", 0);
            gpMouseManager->ReallyHidePointer();
            sprintf(
                gcCongratsText,
                "My heroes, our foes have been scattered, their castles broken and laid bare.  "
                "The great campaign is now complete, and I stand before you as the undisputed "
                "High King!\n\nOur victory was achieved in %d days!",
                giCurTurn);
            lowResVideos[0] = 7;
            lowResVideos[1] = 5;
            lowResVideos[2] = 6;
            hiResVideos[0] = 7;
            hiResVideos[1] = 4;
            hiResVideos[2] = 6;
            if (giEndSequence != 1) {
                if (giEndSequence == 2) {
                    PlaySmacker(4);
                    PlaySmacker(6);
                } else {
                    PlaySmacker(gConfig.slowVideo ? hiResVideos[giEndSequence]
                                                  : lowResVideos[giEndSequence]);
                }
                gpResourceManager->GetBackdrop("heroes.bmp", gpWindowManager->m_screen);
                gpWindowManager->UpdateScreenRegion(0, 0, LOGICAL_SCREEN_WIDTH, LOGICAL_SCREEN_HEIGHT);
                gpWindowManager->FadeScreen(0, 8, gPalette);
                gpWindowManager->m_updateFlags = 1;
                backdropLoaded = 1;
            } else {
                ShowCongrats();
            }
            gbGameOver = 0;
            if (giEndSequence == 2) {
                gpSoundManager->SwitchAmbientMusic(54);
                AddScoreToHighScore(giCurTurn, 0, "", gCampaignSideNames[gpGame->m_campaignType]);
            }
            if (gbShowHighScore) {
                gpMouseManager->ReallyShowPointer();
                if (gpExec->AddManager(gpHighScoreManager, -1))
                    ShutDown("Can't add manager!");
                gpExec->MainLoop();
                gpExec->RemoveManager(gpHighScoreManager);
                giHighScoreRank = -1;
                gpSoundManager->SwitchAmbientMusic(48);
                gpResourceManager->GetBackdrop("heroes.bmp", gpWindowManager->m_screen);
                gpWindowManager->UpdateScreenRegion(0, 0, LOGICAL_SCREEN_WIDTH, LOGICAL_SCREEN_HEIGHT);
                gpWindowManager->FadeScreen(0, 8, gPalette);
                backdropLoaded = 1;
            }
            if (gpGame->m_campaignType > 0) {
                if (giEndSequence == 0) {
                    sprintf(gText, "Would you like to replay this scenario?");
                    NormalDialog(gText, NORMAL_DIALOG_TYPE_YES_NO, -1, -1, NORMAL_DIALOG_NO_RESOURCE, 0, NORMAL_DIALOG_NO_RESOURCE, 0, -1);
                    if (gpWindowManager->m_dialogResult == NORMAL_DIALOG_CONFIRM) {
                        gpGame->InitCampaignMap(gpGame->m_campaignScenario, 0);
                        goto playScenario;
                    }
                } else if (giEndSequence == 1) {
                    gpGame->m_campaignDay = giCurTurn + 1;
                    gpGame->m_campaignScenario++;
                    gpGame->m_unknown000b++;
                    if (gpGame->m_campaignScenario - 4 == gpGame->m_campaignType - 1)
                        gpGame->m_campaignScenario++;
                    gpGame->InitCampaignMap(gpGame->m_campaignScenario, 0);
                    sprintf(saveBuf, "%s%02d", "SCENWN", gpGame->m_unknown000b);
                    gpGame->SaveGame(saveBuf, 1);
                    sprintf(
                        gText,
                        "Your campaign has been saved as %s.  Would you like to start the next "
                        "scenario?",
                        saveBuf);
                    NormalDialog(gText, NORMAL_DIALOG_TYPE_YES_NO, -1, -1, NORMAL_DIALOG_NO_RESOURCE, 0, NORMAL_DIALOG_NO_RESOURCE, 0, -1);
                    if (gpWindowManager->m_dialogResult == NORMAL_DIALOG_CONFIRM)
                        goto playScenario;
                }
            }
        }
        if (gbRemoteOn)
            leave = 1;
    }
    ShutDown(NULL);
    return 0;
}

// Buka 2.1 toupper; HoMM1 keeps the narrow character form.
VA(0x00450f7e, 0x3e)
char toupper(char character) {
    if (character >= 'a' && character <= 'z')
        return character - 32;
    else
        return character;
}

// Buka 2.1 InterpretCommandLine reduced to HoMM1's /I, /C, /S and /B switches.
VA(0x00450fbc, 0x288)
int InterpretCommandLine(void) {
    int i;
    int helpRequested = 0;
    int size;

    giDebugLevel = 0;
    giShowIntro = 1;
    gbColorMice = 0;
    gbSpecialMouseMasks = 1;
    giScreenScroll = 1;
    gbCheatMenus = 0;
    gbBlackoutPlayer = 1;
    strcpy(gMapName, "AES31000.map");
    strcpy(gFullMapName, "Claw ( Easy )");
    strcpy(gMapDescription, "The Griffons will protect you until you are ready to make your move.");

    size = strlen(gcCommandLine);
    for (i = 0; i < size; i++) {
        if (gcCommandLine[i] == '/' && i + 1 < size) {
            switch (toupper(gcCommandLine[i + 1])) {
                case 'I':
                    if (i + 2 < size)
                        giShowIntro = gcCommandLine[i + 2] - '0';
                    break;
                case 'C':
                    if (i + 2 < size)
                        gbColorMice = gcCommandLine[i + 2] - '0';
                    break;
                case 'S':
                    if (i + 2 < size)
                        gbNoSound = 1 - (gcCommandLine[i + 2] - '0');
                    break;
                case 'B':
                    if (i + 2 < size)
                        gbSpecialMouseMasks = gcCommandLine[i + 2] - '0';
                    break;
            }
        }
    }

    sprintf(cAggPathName, "%s%s", ".\\DATA\\", "heroes.agg");
    DEFAULT_AGGREGATE_NAME = cAggPathName;
    giFrameStep = 6;
    for (i = 0; i < 4; i++) {
        if (giNumHumanPlayers > i)
            gbHumanPlayer[i] = 1;
        else
            gbHumanPlayer[i] = 0;
    }
    if (giNumHumanPlayers == 1)
        gbBlackoutPlayer = 0;
    helpRequested = 0;
    return 1;
}

extern char* gInitMenuHelp[];
extern int giMenuCommand;

// Buka 2.1 InitMenuHandler reduced to HoMM1's right-click help and button
// release; the main menu draws its own hover frames.
VA(0x00451244, 0x1b6)
short InitMenuHandler(tag_message& message) {
    int handled = 0;
    int helpIndex;

    PollSound();
    if (message.modifiers & MESSAGE_MODIFIER_RIGHT_BUTTON) {
        if (message.command == WIDGET_NOTIFY_SELECT
            || message.command == WIDGET_NOTIFY_RIGHT_CLICK) {
            helpIndex = -1;
            switch (message.id) {
                case 1:
                    helpIndex = 0;
                    break;
                case 2:
                    helpIndex = 1;
                    break;
                case 5:
                    helpIndex = 2;
                    break;
                case 6:
                    helpIndex = 3;
                    break;
                case 4:
                    helpIndex = 4;
                    break;
            }
            if (helpIndex >= 0)
                NormalDialog(gInitMenuHelp[helpIndex], NORMAL_DIALOG_TYPE_QUICK_VIEW, -1, -1, NORMAL_DIALOG_NO_RESOURCE, 0, NORMAL_DIALOG_NO_RESOURCE, 0, -1);
        }
    } else if (message.type == MESSAGE_WIDGET) {
        switch (message.command) {
            case WIDGET_NOTIFY_DESELECT:
                if (message.id > 0 && message.id <= 6)
                    handled = 1;
                break;
            default:
                break;
        }
    }

    if (handled || giMenuCommand != -1) {
        gpWindowManager->m_dialogResult = message.id;
        message.command = message.id = WIDGET_COMMAND_DIALOG_SELECT;
        return MESSAGE_DISPATCH_FORWARD;
    }
    return MESSAGE_DISPATCH_CONSUME;
}

VA(0x004513fa, 0x14)
short NullHandler(tag_message&) {
    return MESSAGE_DISPATCH_CONSUME;
}

// Buka 2.1 RecruitHeroHandler: HoMM1 offers two heroes, each with its own
// view (ids 2-3) and recruit (ids 8-9) button.
VA(0x0045140e, 0x1cb)
short RecruitHeroHandler(tag_message& message) {
    const short viewButton1 = 2;
    const short viewButton2 = 3;
    const short recruitButton1 = 8;
    const short recruitButton2 = 9;
    int shouldClose = 0;
    int index;

    if (message.type == MESSAGE_WIDGET) {
        switch (message.command) {
            case WIDGET_NOTIFY_SELECT:
                switch (message.id) {
                    case viewButton1:
                    case viewButton2:
                        index = message.id - viewButton1;
                        gpTownManager->m_recruitHeroes[index]->HeroView(0);
                        gpTownManager->RedrawTownScreen();
                        gpTownManager->m_heroWindow0->DrawWindow();
                        gpTownManager->m_heroWindow1->DrawWindow();
                        gpWindowManager->FadeScreen(0, 8, NULL);
                        break;
                    default:
                        break;
                }
                break;
            case WIDGET_NOTIFY_DESELECT:
                switch (message.id) {
                    case DIALOG_BUTTON_1:
                        gpTownManager->m_recruitState = -1;
                        shouldClose = 1;
                        break;
                    case recruitButton1:
                    case recruitButton2:
                        gpTownManager->m_recruitState = message.id - recruitButton1;
                        gpWindowManager->m_dialogResult = message.id;
                        shouldClose = 1;
                        break;
                }
                break;
            default:
                break;
        }
    }
    if (shouldClose == 1) {
        message.command = message.id = WIDGET_COMMAND_DIALOG_SELECT;
        return MESSAGE_DISPATCH_FORWARD;
    }
    return MESSAGE_DISPATCH_CONSUME;
}

// HoMM1 has seven neutral building slots before six per-faction dwellings.
VA(0x004515d9, 0x47)
char* GetBuildingName(int race, short building) {
    if (building < BUILDING_SLOT_DWELLING_FIRST)
        return gNeutralBuildingNames[building];
    else
        return gDwellingNames[building - BUILDING_SLOT_DWELLING_FIRST + race * 6];
}

VA(0x00451620, 0x9f)
void GetBuildingCost(int race, short building, int* const destination, int mageLevel) {
    if (building < BUILDING_SLOT_DWELLING_FIRST) {
        if (building == BUILDING_SLOT_MAGE_GUILD)
            memcpy(destination, gMageBuildingCosts[mageLevel], RESOURCE_COUNT * sizeof(int));
        else
            memcpy(destination, gNeutralBuildingCosts[building], RESOURCE_COUNT * sizeof(int));
    } else {
        memcpy(
            destination,
            gDwellingCosts[building - BUILDING_SLOT_DWELLING_FIRST + race * 6],
            RESOURCE_COUNT * sizeof(int)
        );
    }
}

VA(0x004516bf, 0x1a)
char* GetMonsterName(int monster) {
    return gArmyNames[monster];
}

// donor PoL RVA 0x0009992c; preferred Buka symbol ?GetMonsterCost@@YIXHQAH@Z
// donor Buka TU SOURCE/KB; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.424205;margin=0.383727;shape=0.192;size=0.855;calls=1.000;alternate=pol20:void GetMonsterCost(int, int * const)@0x0009992c
VA(0x004516d9, 0xe6)
void GetMonsterCost(int monster, int* const cost) {
    int index;
    for (index = 0; index < RESOURCE_COUNT; index++)
        cost[index] = 0;
    cost[RESOURCE_GOLD] = gMonsterDatabase[monster].cost;
    switch (monster) {
        case CREATURE_GENIE:
            cost[RESOURCE_GEMS] = 1;
            break;
        case CREATURE_PHOENIX:
            cost[RESOURCE_MERCURY] = 1;
            break;
        case CREATURE_CYCLOPS:
            cost[RESOURCE_CRYSTAL] = 1;
            break;
        case CREATURE_DRAGON:
            cost[RESOURCE_SULFUR] = 1;
            break;
    }
}

// donor PoL RVA 0x00099a6c; preferred Buka symbol ?CanBuild@@YIHPAVtown@@H@Z
// donor Buka TU SOURCE/KB; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.375672;margin=0.371383;shape=0.277;size=0.517;calls=1.000;alternate=pol20:int CanBuild(class town *, int)@0x00099a6c
// HoMM1 retail returns the result in AL (xor al,al / mov al,1).
VA(0x004517bf, 0x144)
signed char CanBuild(town* t, short building) {
    mapCell* cell;
    unsigned short required;
    if (BitTest(gpGame->m_townBuiltToday, t->m_id))
        return 0;
    if (building != BUILDING_SLOT_CASTLE && !(t->m_buildings & (1 << BUILDING_SLOT_CASTLE)))
        return 0;
    if (building == BUILDING_SLOT_SHIPYARD) {
        cell = gpAdvManager->GetCell(t->m_x - 1, t->m_y + 1);
        if (cell->m_tileIndex < 20)
            return 1;
        else
            return 0;
    }
    if (building == BUILDING_SLOT_MAGE_GUILD && t->m_buildState >= 3)
        return 0;
    if (building == BUILDING_SLOT_TENT)
        return 0;
    if (building < BUILDING_SLOT_DWELLING_FIRST)
        return 1;
    required = gDwellingRequirements[building - BUILDING_SLOT_DWELLING_FIRST + t->m_type * 6];
    if ((t->m_buildings & required) == required)
        return 1;
    return 0;
}

// donor PoL RVA 0x00099d21; preferred Buka symbol ?CanBuy@@YIHPAVtown@@H@Z
// donor Buka TU SOURCE/KB; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.384626;margin=0.370647;shape=0.216;size=0.621;calls=1.000;alternate=pol20:int CanBuy(class town *, int)@0x00099d21
// Retail returns a byte flag (xor al,al / mov al,1); philAI::CanBuyBHC tests al.
VA(0x00451903, 0xce)
signed char CanBuy(town* t, short type) {
    int cost[RESOURCE_COUNT];
    playerData* rec;
    int i;
    GetBuildingCost(
        t->m_type,
        type,
        cost,
        (t->m_buildings & (1 << BUILDING_SLOT_MAGE_GUILD)) ? (t->m_buildState >= 3 ? 3 : t->m_buildState + 1) : 0
    );
    rec = &gpGame->m_players[giCurPlayer];
    for (i = 0; i < RESOURCE_COUNT; ++i) {
        if (rec->m_resources[i] < cost[i])
    return 0;
}
    return 1;
}

// HoMM1 keeps seven neutral value slots ahead of six per-faction dwellings.
VA(0x004519d1, 0x60)
int GetBuildingBaseResourceValue(int race, int building, int level) {
    if (building < BUILDING_SLOT_DWELLING_FIRST) {
        if (building == BUILDING_SLOT_MAGE_GUILD)
            return gMageBaseResourceValues[level];
        else
            return gNeutralBaseResourceValues[building];
    } else {
        return gDwellingBaseResourceValues[building - BUILDING_SLOT_DWELLING_FIRST + race * 6];
    }
}

short WaitHandler(tag_message&);

// Buka 2.1 NormalDialog without HoMM2's timeout, saved resource globals,
// primary-skill/monster/secondary-skill slots and centered x; HoMM1 measures
// the text with a temporary bigfont.fnt and frames heroes with port%04d.icn.
VA(0x00451a31, 0xf03)
void NormalDialog(
    char* text,
    int dialogType,
    int x,
    int y,
    int firstResourceType,
    int firstResourceValue,
    int secondResourceType,
    int secondResourceValue,
    int showOrText
) {
    char szFilename[NORMAL_DIALOG_FILENAME_LENGTH];
    char* amountText[NORMAL_DIALOG_RESOURCE_COUNT];
    int sizingHeight;
    int resourceYPos;
    int kind[NORMAL_DIALOG_RESOURCE_COUNT];
    iconWidget* iconPanel;
    int resWidth;
    short bShowMessage;
    font* bigFont;
    int width;
    int height;
    int i;
    int contentSize;
    int id;
    int iHeight;
    int resourceQty[NORMAL_DIALOG_RESOURCE_COUNT];
    tag_message message;
    int heightIndex;
    int lineCount;
    int resourceFrame;
    int frameHeight;
    textWidget* captionWidget;
    int maxIconHeight;
    int resCenterX;
    char* szOr;

    resCenterX = 0;
    resourceYPos = 0;
    resourceFrame = 0;
    id = NORMAL_DIALOG_TEXT_WIDGET_FIRST_ID;
    resWidth = 0;
    iHeight = 0;
    bShowMessage = 1;
    kind[0] = firstResourceType;
    resourceQty[0] = firstResourceValue;
    kind[1] = secondResourceType;
    resourceQty[1] = secondResourceValue;

    bigFont = gpResourceManager->GetFont("bigfont.fnt");
    lineCount = bigFont->LineLength(text, NORMAL_DIALOG_TEXT_LINE_WIDTH);
    gpResourceManager->Dispose(bigFont);
    contentSize = lineCount * NORMAL_DIALOG_TEXT_LINE_HEIGHT;
    if (dialogType != NORMAL_DIALOG_TYPE_QUICK_VIEW)
        contentSize += NORMAL_DIALOG_BUTTON_AREA_HEIGHT;

    maxIconHeight = 0;
    for (i = 0; i < NORMAL_DIALOG_RESOURCE_COUNT; i++) {
        switch (kind[i]) {
            case NORMAL_DIALOG_ARTIFACT:
                sizingHeight = 76;
                break;
            case NORMAL_DIALOG_LUCK_BONUS:
                sizingHeight = 28;
                break;
            case NORMAL_DIALOG_LUCK_PENALTY:
                sizingHeight = 57;
                break;
            case NORMAL_DIALOG_MORALE_BONUS:
                sizingHeight = 62;
                break;
            case NORMAL_DIALOG_MORALE_PENALTY:
                sizingHeight = 59;
                break;
            case NORMAL_DIALOG_EXPERIENCE:
                sizingHeight = 76;
                break;
            case NORMAL_DIALOG_CREST:
                sizingHeight = 55;
                break;
            case NORMAL_DIALOG_HERO:
                sizingHeight = 111;
                break;
            case NORMAL_DIALOG_RESOURCE_GOLD:
                sizingHeight = 26;
                break;
            case RESOURCE_WOOD:
            case RESOURCE_MERCURY:
            case RESOURCE_ORE:
            case RESOURCE_SULFUR:
            case RESOURCE_CRYSTAL:
            case RESOURCE_GEMS:
                sizingHeight = 44;
                break;
            case NORMAL_DIALOG_SPELL:
                sizingHeight = 52;
                break;
            default:
                sizingHeight = 0;
                break;
        }
        if (sizingHeight > maxIconHeight)
            maxIconHeight = sizingHeight;
    }

    if (maxIconHeight > 0)
        contentSize += maxIconHeight + 12;
    heightIndex = (contentSize - 12) / NORMAL_DIALOG_WINDOW_ROW_HEIGHT;
    if (heightIndex > NORMAL_DIALOG_MAX_ROWS)
        heightIndex = NORMAL_DIALOG_MAX_ROWS;
    if (heightIndex <= 0 && dialogType != NORMAL_DIALOG_TYPE_QUICK_VIEW)
        heightIndex = 1;
    width = NORMAL_DIALOG_WINDOW_WIDTH;
    height = heightIndex * NORMAL_DIALOG_WINDOW_ROW_HEIGHT + NORMAL_DIALOG_WINDOW_BASE_HEIGHT;

    if (x == -1 || x + width >= 639) {
        if (gpAdvManager->m_active == 1 && !gbHeroWindShowing && !gbOverviewShowing)
            x = NORMAL_DIALOG_ADVENTURE_X;
        else
            x = (640 - width) / 2;
    }
    if (y == -1 || y + height >= 479) {
        y = (480 - height) / 2;
        if (y > NORMAL_DIALOG_MAX_TOP)
            y = NORMAL_DIALOG_MAX_TOP;
    }

    sprintf(szFilename, "evntwin%d.bin", heightIndex);
    pNormalDialogWindow = new heroWindow(x, y, szFilename);
    if (!pNormalDialogWindow)
        MemError();

    message.type = MESSAGE_WIDGET;
    message.command = WIDGET_COMMAND_CLEAR_FLAGS;
    message.value = NORMAL_DIALOG_BUTTON_FLAGS;
    if (dialogType != NORMAL_DIALOG_TYPE_WAIT_CANCEL && dialogType != NORMAL_DIALOG_TYPE_NO_BUTTONS) {
        message.id = NORMAL_DIALOG_BUTTON_OK;
        pNormalDialogWindow->BroadcastMessage(message);
    }
    if (dialogType != NORMAL_DIALOG_TYPE_WAIT_OK && dialogType != NORMAL_DIALOG_TYPE_OK
        && dialogType != NORMAL_DIALOG_TYPE_NO_BUTTONS) {
        message.id = NORMAL_DIALOG_BUTTON_CANCEL;
        pNormalDialogWindow->BroadcastMessage(message);
    }
    if (dialogType != NORMAL_DIALOG_TYPE_YES_NO) {
        message.id = NORMAL_DIALOG_BUTTON_YES;
        pNormalDialogWindow->BroadcastMessage(message);
        message.id = NORMAL_DIALOG_BUTTON_NO;
        pNormalDialogWindow->BroadcastMessage(message);
    }

    for (i = 0; i < NORMAL_DIALOG_RESOURCE_COUNT; i++) {
        iconPanel = NULL;
        captionWidget = NULL;
        if (kind[i] == NORMAL_DIALOG_NO_RESOURCE)
            break;

        amountText[i] = (char*)malloc(NORMAL_DIALOG_TEXT_LENGTH);
        if (kind[i] <= NORMAL_DIALOG_RESOURCE_LAST) {
            if (resourceQty[i] > 0)
                sprintf(amountText[i], "%d", resourceQty[i]);
            else if (resourceQty[i] == 0)
                strcpy(amountText[i], "");
            else
                sprintf(amountText[i], "%d/day", -resourceQty[i]);
            strcpy(szFilename, "resource.icn");
            resourceFrame = kind[i];
        } else if (kind[i] == NORMAL_DIALOG_SPELL) {
            sprintf(amountText[i], "%s", gSpellNames[resourceQty[i]]);
            strcpy(szFilename, "spells.icn");
            resourceFrame = resourceQty[i];
        } else if (kind[i] == NORMAL_DIALOG_CREST) {
            sprintf(amountText[i], "%s", "");
            strcpy(szFilename, "brcrest.icn");
            resourceFrame = resourceQty[i];
        } else if (kind[i] == NORMAL_DIALOG_HERO) {
            sprintf(amountText[i], "%s", "");
            sprintf(szFilename, "surrendr.icn");
            resourceFrame = 4;
        } else if (kind[i] == NORMAL_DIALOG_EXPERIENCE
                   || kind[i] == NORMAL_DIALOG_MORALE_BONUS
                   || kind[i] == NORMAL_DIALOG_MORALE_PENALTY
                   || kind[i] == NORMAL_DIALOG_LUCK_BONUS
                   || kind[i] == NORMAL_DIALOG_LUCK_PENALTY) {
            strcpy(amountText[i], "");
            strcpy(szFilename, "expmrl.icn");
            resourceFrame = kind[i] - NORMAL_DIALOG_EXPMRL_FIRST;
            if (kind[i] == NORMAL_DIALOG_EXPERIENCE
                && resourceQty[i] != NORMAL_DIALOG_NO_VALUE)
                sprintf(amountText[i], "%d", resourceQty[i]);
        } else {
            strcpy(amountText[i], "");
            strcpy(szFilename, "resource.icn");
            resourceFrame = kind[i];
        }

        switch (kind[i]) {
            case NORMAL_DIALOG_ARTIFACT:
                resWidth = 76;
                sizingHeight = 76;
                break;
            case NORMAL_DIALOG_LUCK_BONUS:
                resWidth = 64;
                sizingHeight = 28;
                break;
            case NORMAL_DIALOG_LUCK_PENALTY:
                resWidth = 64;
                sizingHeight = 57;
                break;
            case NORMAL_DIALOG_MORALE_BONUS:
                resWidth = 64;
                sizingHeight = 62;
                break;
            case NORMAL_DIALOG_MORALE_PENALTY:
                resWidth = 64;
                sizingHeight = 59;
                break;
            case NORMAL_DIALOG_EXPERIENCE:
                resWidth = 64;
                sizingHeight = 64;
                break;
            case NORMAL_DIALOG_CREST:
                resWidth = 50;
                sizingHeight = 55;
                break;
            case NORMAL_DIALOG_HERO:
                resWidth = 111;
                sizingHeight = 105;
                break;
            case NORMAL_DIALOG_RESOURCE_GOLD:
                resWidth = 76;
                sizingHeight = 26;
                break;
            case RESOURCE_WOOD:
            case RESOURCE_MERCURY:
            case RESOURCE_ORE:
            case RESOURCE_SULFUR:
            case RESOURCE_CRYSTAL:
            case RESOURCE_GEMS:
                resWidth = 38;
                sizingHeight = 32;
                break;
            case NORMAL_DIALOG_SPELL:
                resWidth = 38;
                sizingHeight = 40;
                break;
        }

        if (strlen(amountText[i]) > 0)
            sizingHeight += NORMAL_DIALOG_RESOURCE_LABEL_HEIGHT;
        if (i == 0) {
            if (kind[1] == NORMAL_DIALOG_NO_RESOURCE)
                resCenterX = width / 2;
            else
                resCenterX = width / 3;
        } else {
            resCenterX = width * 2 / 3;
        }
        resourceYPos = height - sizingHeight - NORMAL_DIALOG_RESOURCE_BOTTOM_INSET;
        if (dialogType != NORMAL_DIALOG_TYPE_QUICK_VIEW)
            resourceYPos -= NORMAL_DIALOG_BUTTON_AREA_HEIGHT;
        if (maxIconHeight > sizingHeight)
            resourceYPos -= (maxIconHeight - sizingHeight) / 2;

        iconPanel = new iconWidget(
            resCenterX - resWidth / 2, resourceYPos, resWidth,
            sizingHeight, szFilename, resourceFrame, 0, -1, ICON_WIDGET_DRAW, 1);
        if (!iconPanel)
            MemError();
        pNormalDialogWindow->AddWidget(iconPanel, -1);
        if (kind[i] == NORMAL_DIALOG_ARTIFACT) {
            iconPanel = new iconWidget(
                resCenterX - resWidth / 2 + 6, resourceYPos + 6, 76, 76,
                "artifact.icn", resourceQty[i], 0, -1, ICON_WIDGET_DRAW, 1);
            if (!iconPanel)
                MemError();
            pNormalDialogWindow->AddWidget(iconPanel, -1);
        }
        if (kind[i] == NORMAL_DIALOG_CREST) {
            iconPanel = new iconWidget(
                resCenterX - resWidth / 2 - 4, resourceYPos - 4, 58, 55,
                "brcrest.icn", 4, 0, -1, ICON_WIDGET_DRAW, 1);
            if (!iconPanel)
                MemError();
            pNormalDialogWindow->AddWidget(iconPanel, -1);
        }
        if (kind[i] == NORMAL_DIALOG_HERO) {
            sprintf(szFilename, "port%04d.icn", resourceQty[i]);
            iconPanel = new iconWidget(
                resCenterX - resWidth / 2 + 5, resourceYPos + 5, 101, 95,
                szFilename, 0, 0, -1, ICON_WIDGET_DRAW, 1);
            if (!iconPanel)
                MemError();
            pNormalDialogWindow->AddWidget(iconPanel, -1);
        }
        captionWidget = new textWidget(
            resCenterX - 50, resourceYPos + sizingHeight - 10, 100, 12,
            amountText[i], "smalfont.fnt", 1, id++, 0x200);
        if (!captionWidget)
            MemError();
        pNormalDialogWindow->AddWidget(captionWidget, -1);
    }

    message.type = MESSAGE_WIDGET;
    message.command = WIDGET_COMMAND_SET_TEXT;
    message.id = NORMAL_DIALOG_TEXT_WIDGET_ID;
    message.text = text;
    pNormalDialogWindow->BroadcastMessage(message);

    if (showOrText == NORMAL_DIALOG_SHOW_OR_TEXT) {
        szOr = (char*)malloc(3);
        strcpy(szOr, "or");
        captionWidget = new textWidget(
            width / 2 - 17, resourceYPos + 30, 40, 12, szOr, "smalfont.fnt", 1,
            id++, 0x200);
        if (!captionWidget)
            MemError();
        pNormalDialogWindow->AddWidget(captionWidget, -1);
    }

    if (gpAdvManager->m_active == 1)
        gpMouseManager->SetPointer(0);
    else if (gpCombatManager->m_active == 1)
        gpMouseManager->SetPointer(6);

    if (dialogType == NORMAL_DIALOG_TYPE_WAIT_CANCEL || dialogType == NORMAL_DIALOG_TYPE_WAIT_OK) {
        gpWindowManager->DoDialog(pNormalDialogWindow, WaitHandler, 0);
    } else if (dialogType == NORMAL_DIALOG_TYPE_QUICK_VIEW) {
        gpMouseManager->ReallyHidePointer();
        gpWindowManager->AddWindow(pNormalDialogWindow, -1, 1);
        QuickViewWait();
        gpWindowManager->RemoveWindow(pNormalDialogWindow);
        gpMouseManager->ReallyShowPointer();
    } else {
        gpWindowManager->DoDialog(pNormalDialogWindow, EventWindowHandler, 0);
    }
    delete pNormalDialogWindow;
}

// donor PoL RVA 0x000a2565; preferred Buka symbol ?UpdateNormalDialog@@YIXPAD@Z
// donor Buka TU SOURCE/KB; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.563703;margin=0.348381;shape=0.417;size=0.972;calls=1.000;alternate=pol20:void UpdateNormalDialog(char *)@0x000a2565
// @dead-code
// Zero-ref: no effective incoming retail reference.
VA(0x00452934, 0x6b)
void UpdateNormalDialog(char* text) {
    tag_message message;
    {
        short show = 1; // Retained from donor and retail stack frame.
        message.type = MESSAGE_WIDGET;
        message.command = WIDGET_COMMAND_SET_TEXT;
        message.id = 1;
        message.text = text;
        pNormalDialogWindow->BroadcastMessage(message);
        pNormalDialogWindow->DrawWindow(0, 0, NORMAL_DIALOG_FOREGROUND_WIDGET_LIMIT);
        pNormalDialogWindow
            ->DrawWindow(1, WINDOW_ALL_WIDGETS_LOW, NORMAL_DIALOG_BACKGROUND_WIDGET_LAST_ID);
    }
}

// Modem.cpp's wait-loop steps; Modem.h does not export them (declaring them
// there ahead of their definitions reorders Modem's own compare operands).
signed char GUIModemCommandExec(void);
signed char GUIModemResponseExec(void);
int WaitForDirectConnect(void);

// donor PoL RVA 0x00099e81; preferred Buka symbol ?WaitHandler@@YIHAAUtag_message@@@Z
// donor Buka TU SOURCE/KB; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.445743;margin=0.444520;shape=0.204;size=0.951;calls=0.688;alternate=pol20:int WaitHandler(struct tag_message &)@0x00099e81
VA(0x0045299f, 0x1c5)
short WaitHandler(tag_message& message) {
    signed char result = 0;
    gbFunctionComplete = 1;
    PollSound();
    if (!gpSoundManager->MusicPlaying())
        gpSoundManager->SwitchAmbientMusic(gpAdvManager->m_currentTerrain);
    if (message.type == MESSAGE_WIDGET) {
        switch (message.command) {
            case WIDGET_NOTIFY_DESELECT:
                switch (message.id) {
                    case DIALOG_BUTTON_0:
                    case DIALOG_BUTTON_1:
                    case DIALOG_BUTTON_2:
                        gbFunctionComplete = 0;
                        result = 1;
                        break;
                }
        }
    }
    if (!result) {
        switch (giWaitType) {
            case 0:
                result = WaitForOtherPlayer();
                break;
            case 2:
                result = WaitForHost();
                break;
            case 1:
                result = WaitForGuest();
                break;
            case 3:
                result = InitNetGuest();
                break;
            case 4:
                result = InitNetHost();
                break;
            case 5:
                result = GUIModemCommandExec();
                break;
            case 6:
                result = GUIModemResponseExec();
                break;
            case 7:
                result = WaitForDirectConnect();
                break;
        }
    }
    if (result) {
        gpWindowManager->m_dialogResult = DIALOG_BUTTON_1;
        message.type = MESSAGE_WIDGET;
        message.command = message.id = WIDGET_COMMAND_DIALOG_SELECT;
        return MESSAGE_DISPATCH_FORWARD;
    }
    return MESSAGE_DISPATCH_CONSUME;
}

// Buka 2.1 EventWindowHandler without HoMM2's dialog timeout and resource help.
VA(0x00452b64, 0x114)
short EventWindowHandler(tag_message& message) {
    if (!gpSoundManager->MusicPlaying())
        gpSoundManager->SwitchAmbientMusic(gpAdvManager->m_currentTerrain);
    if (message.type == MESSAGE_WIDGET) {
        switch (message.command) {
            case WIDGET_NOTIFY_DESELECT:
                switch (message.id) {
                    case 0x385:
                    case DIALOG_BUTTON_0:
                    case DIALOG_BUTTON_1:
                    case DIALOG_BUTTON_2:
                    case DIALOG_BUTTON_3:
                    case DIALOG_BUTTON_5:
                    case DIALOG_BUTTON_6:
                        gpWindowManager->m_dialogResult = message.id;
                        message.command = message.id =
                            WIDGET_COMMAND_DIALOG_SELECT;
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

// Buka 2.1 TrueFalseDialogHandler.
VA(0x00452c78, 0x1c)
short TrueFalseDialogHandler(tag_message& message) {
    return EventWindowHandler(message);
}

// donor PoL RVA 0x0009a52f; preferred Buka symbol ?PlayerDead@@YIXH@Z
// donor Buka TU SOURCE/KB; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.488269;margin=0.466685;shape=0.274;size=0.981;calls=0.750;alternate=pol20:void PlayerDead(int)@0x0009a52f
VA(0x00452c94, 0x16c)
void PlayerDead(int player) {
    playerData* currentPlayer;
    int i;
    gbRetreatWin = 0;
    currentPlayer = &gpGame->m_players[player];
    gpGame->m_playerDead[player] = 1;
    ++gpGame->m_deadPlayerCount;
    for (i = 0; i < GAME_MINE_COUNT; ++i) {
        if (gpGame->m_mineOwners[i] == player)
            gpGame->ClaimMine(i, -1);
    }
    for (i = currentPlayer->m_heroCount - 1; i >= 0; --i)
        gpGame->GetHero(currentPlayer->m_heroIds[i])->Deallocate();
    for (i = 0; i < 2; ++i) {
        if (gpGame->m_availableHeroes[currentPlayer->m_availableHeroIds[i]] == 0x40)
            gpGame->m_availableHeroes[currentPlayer->m_availableHeroIds[i]] = -1;
    }
    if (gbRemoteOn && gbHumanPlayer[player])
        HandleRemoteDeadPlayerExit(player);
}

// HoMM1's three-byte exit notice: game position, control hand-off, next player.
#pragma pack(push, 1)
struct playerExitMessage {
    signed char gamePosition;
    signed char takesControl;
    signed char nextPlayer;
};
#pragma pack(pop)
extern playerExitMessage gPlayerExitMessage;
extern int giHostGamePos;

// Player colour names for the exit notices, CheckEndGame's re-entry guard
// and last offered score, and the creature alignment names (by type / 6).
extern char* gColorNames[];
extern signed char bInCheckEndGame;
extern char* gAlignmentNames[];
extern int giScore;

// Buka 2.1 HandleRemoteDeadPlayerExit for HoMM1's two-player transport.
VA(0x00452e00, 0x99)
void HandleRemoteDeadPlayerExit(int position) {
    if (position == giThisGamePos) {
        if (!gpGame->TransmitSaveGame(REMOTE_BROADCAST_PLAYER, 1))
            ShutDown(0);
        RemoteCleanup();
    } else if (giNumHumanPlayers == 2) {
        giNumHumanPlayers--;
        gPlayerExitMessage.gamePosition = position;
        gPlayerExitMessage.takesControl = 0;
        TransmitRemoteData((char*)&gPlayerExitMessage, REMOTE_BROADCAST_PLAYER, 3, 30, 0, 0, REMOTE_MESSAGE_RELIABLE, 1);
        RemoteCleanup();
        gbHumanPlayer[position] = 0;
    }
}

// Buka 2.1 HandleRemoteSuddenExit; HoMM1 names the next human player itself.
VA(0x00452e99, 0xf1)
void HandleRemoteSuddenExit(void) {
    int next;
    if (!gbGameInitialized)
        return;
    gPlayerExitMessage.gamePosition = giThisGamePos;
    if (gbThisNetHumanPlayer[giCurPlayer]
        || (!gbHumanPlayer[giCurPlayer] && giHostGamePos == giThisGamePos)) {
        gPlayerExitMessage.takesControl = 1;
        next = giCurPlayer;
        next = (next + 1) % gpGame->m_playerCount;
        while (!gbHumanPlayer[next])
            next = (next + 1) % gpGame->m_playerCount;
        gPlayerExitMessage.nextPlayer = next;
    } else {
        gPlayerExitMessage.takesControl = 0;
    }
    TransmitRemoteData((char*)&gPlayerExitMessage, REMOTE_BROADCAST_PLAYER, 3, 30, 0, 0, REMOTE_MESSAGE_RELIABLE, 1);
}

// donor PoL RVA 0x000a07e3; preferred Buka symbol ?ReceiveRemotePlayerExit@@YIXUSPlayerExit@@@Z
// donor Buka TU SOURCE/KB; HoMM1 owner inferred from contiguous order
// evidence: graph:4;base=0.368727;margin=0.249960;shape=0.192;size=0.687;calls=0.800;alternate=pol20:void ReceiveRemotePlayerExit(struct SPlayerExit)@0x000a07e3

VA(0x00452f8a, 0x1ea)
// HoMM1 callers push four byte-sized values: player, an unused flag,
// elimination and timeout.
void ReceiveRemotePlayerExit(signed char position, signed char, signed char eliminated, signed char timedOut) {
    if (position == giThisGamePos) {
        sprintf(gText, "You have been eliminated from the game!!!");
        NormalDialog(gText, NORMAL_DIALOG_TYPE_OK, -1, -1, NORMAL_DIALOG_NO_RESOURCE, 0, NORMAL_DIALOG_NO_RESOURCE, 0, -1);
        RemoteCleanup();
        gbGameOver = 1;
        giEndSequence = 0;
        return;
    }
    if (giNumHumanPlayers <= 2) {
        gpGame->SaveGame("PLYREXIT", 1);
        if (eliminated) {
            sprintf(gText, "%s player has been vanquished!", gColorNames[gpGame->m_players[position].Color()]);
            gText[0] -= 32;
            NormalDialog(gText, NORMAL_DIALOG_TYPE_OK, 0x61, -1, NORMAL_DIALOG_CREST, gpGame->m_players[position].Color(), NORMAL_DIALOG_NO_RESOURCE, 0, -1);
            goto dropPlayer;
        } else {
            if (timedOut)
                sprintf(
                    gText,
                    "Player %d has been logged out of the game.  The current game has been saved as "
                    "'PLYREXIT'.  Do you wish to continue playing with a computer player filling in for "
                    "player %d?",
                    position + 1,
                    position + 1
                );
            else
                sprintf(
                    gText,
                    "Player %d is exiting the game.  The current game has been saved as 'PLYREXIT'.  Do "
                    "you wish to continue playing with a computer player filling in for player %d?",
                    position + 1,
                    position + 1
                );
            NormalDialog(gText, NORMAL_DIALOG_TYPE_YES_NO, -1, -1, NORMAL_DIALOG_NO_RESOURCE, 0, NORMAL_DIALOG_NO_RESOURCE, 0, -1);
        }
        if (gpWindowManager->m_dialogResult == NORMAL_DIALOG_CONFIRM) {
        dropPlayer:
            if (giNumHumanPlayers == 2) {
                giNumHumanPlayers--;
                RemoteCleanup();
                gbHumanPlayer[position] = 0;
            }
        } else {
            RemoteCleanup();
            ShutDown(NULL);
        }
    }
}

// donor PoL RVA 0x0009a6c1; preferred Buka symbol ?CheckEndGame@@YIXHH@Z
// donor Buka TU SOURCE/KB; HoMM1 owner inferred from contiguous order
// evidence: graph:4;base=0.237398;margin=0.276870;shape=0.229;size=0.353;calls=0.309;alternate=pol20:void CheckEndGame(int, int)@0x0009a6c1
VA(0x00453174, 0x7d4)
void CheckEndGame(int forced) {
    town* goalTown;
    hero* artifactHero;
    signed char ultimateOwner;
    char text[200];
    int numLiving;
    signed char win;
    playerData* pd;
    int slot;
    int player;
    signed char lost;
    signed char normalWin;
    int lastSurvivor;
    int humansAlive;
    int lastHumanPos;

    if (gbInNewGameSetup)
        return;
    if (gbGameOver)
        return;
    if (bInCheckEndGame)
        return;
    bInCheckEndGame = 1;

    for (player = 0; player < gpGame->m_playerCount; player++) {
        if (!gpGame->m_playerDead[player]) {
            pd = &gpGame->m_players[player];
            if (!pd->m_heroCount && !pd->m_townCount) {
                PlayerDead(player);
                sprintf(
                    gText, "%s player has been vanquished!",
                    gColorNames[gpGame->m_players[(signed char)player].Color()]);
                gText[0] -= 32;
                NormalDialog(
                    gText, NORMAL_DIALOG_TYPE_OK, 0x61, -1, NORMAL_DIALOG_CREST, gpGame->m_players[(signed char)player].Color(), NORMAL_DIALOG_NO_RESOURCE,
                    0, -1);
            } else if (!pd->m_townCount) {
                if (pd->m_unknown55 == -1) {
                    if (gbThisNetHumanPlayer[player]) {
                        sprintf(
                            gText,
                            "%s player, you have lost your last town.  If you do not conquer "
                            "another town in the next week, you will be eliminated.",
                            gColorNames[gpGame->m_players[(signed char)player].Color()]);
                        gText[0] -= 32;
                        NormalDialog(
                            gText, NORMAL_DIALOG_TYPE_OK, -1, -1, NORMAL_DIALOG_CREST, gpGame->m_players[(signed char)player].Color(),
                            NORMAL_DIALOG_NO_RESOURCE, 0, -1);
                    }
                    pd->m_unknown55 = 7;
                } else if (!pd->m_unknown55) {
                    PlayerDead(player);
                    if (gbThisNetHumanPlayer[player]) {
                        sprintf(
                            gText,
                            "%s player, your heroes abandon you, and you are banished from this "
                            "land.",
                            gColorNames[gpGame->m_players[(signed char)player].Color()]);
                        gText[0] -= 32;
                    } else {
                        sprintf(
                            gText,
                            "%s player's Heroes have abandoned him, and he is banished from this "
                            "land.",
                            gColorNames[gpGame->m_players[(signed char)player].Color()]);
                        gText[0] -= 32;
                    }
                    NormalDialog(
                        gText, NORMAL_DIALOG_TYPE_OK, 0x61, -1, NORMAL_DIALOG_CREST, gpGame->m_players[(signed char)player].Color(), NORMAL_DIALOG_NO_RESOURCE,
                        0, -1);
                }
            } else {
                pd->m_unknown55 = -1;
            }
        }
    }

    numLiving = 0;
    lastSurvivor = 0;
    humansAlive = 0;
    lastHumanPos = 0;
    for (player = 0; player < gpGame->m_playerCount; player++) {
        if (!gpGame->m_playerDead[player]) {
            numLiving++;
            lastSurvivor = player;
            if (gbHumanPlayer[player]) {
                humansAlive++;
                lastHumanPos = player;
            }
        }
    }

    win = 0;
    lost = 0;
    normalWin = 1;
    if (gpGame->m_campaignType > 0) {
        switch (gpGame->m_campaignScenario) {
            case 0:
            case 4:
            case 5:
            case 6:
            case 7:
                normalWin = 0;
                goalTown = gpGame->GetTown(gpGame->GetTownId(
                    gCampaignScenarios[gpGame->m_campaignScenario].victoryTownX,
                    gCampaignScenarios[gpGame->m_campaignScenario].victoryTownY));
                if (!goalTown->m_owner)
                    win = 1;
                if (gpGame->m_campaignScenario == 0 && goalTown->m_owner > 0) {
                    lost = 1;
                    strcpy(text, "The enemy has captured the town of XX!!");
                }
                break;
            case 2:
                normalWin = 0;
                ultimateOwner = -1;
                for (player = 0; player < gpGame->m_playerCount; player++) {
                    if (!gpGame->m_playerDead[player]) {
                        for (slot = 0; slot < gpGame->m_players[player].m_heroCount; slot++) {
                            artifactHero =
                                gpGame->GetHero(gpGame->m_players[player].m_heroIds[slot]);
                            if (artifactHero->HasArtifact(ARTIFACT_ULTIMATE_BOOK) || artifactHero->HasArtifact(ARTIFACT_ULTIMATE_SWORD)
                                || artifactHero->HasArtifact(ARTIFACT_ULTIMATE_CLOAK) || artifactHero->HasArtifact(ARTIFACT_ULTIMATE_WAND))
                                ultimateOwner = player;
                        }
                    }
                }
                if (!ultimateOwner)
                    win = 1;
                if (ultimateOwner > 0) {
                    lost = 1;
                    strcpy(text, "The enemy has captured the ultimate artifact!!");
                }
                break;
            case 8:
                normalWin = 0;
                if (!gpGame->m_mineOwners[0])
                    win = 1;
                if (gpGame->m_mineOwners[0] > 0) {
                    lost = 1;
                    strcpy(text, "The enemy has captured the dragon city!!");
                }
        }
    }

    if (lost) {
        gbGameOver = 1;
        giEndSequence = 0;
    }
    if (win) {
        gbGameOver = 1;
        giEndSequence = 1;
    }
    if (numLiving == 1 || humansAlive == 0
        || (humansAlive == 1 && !gbThisNetHumanPlayer[lastHumanPos])) {
        if (humansAlive == 1 && gbThisNetHumanPlayer[lastHumanPos]) {
            if (normalWin) {
                gbGameOver = 1;
                giEndSequence = 1;
            }
        } else {
            gbGameOver = 1;
            giEndSequence = 0;
        }
    }
    if (forced) {
        gbGameOver = 1;
        giEndSequence = 1;
    }
    if (gbGameOver && gpGame->m_campaignType > 0 && giEndSequence == 1
        && gpGame->m_campaignScenario + 1 == 9)
        giEndSequence = 2;
    bInCheckEndGame = 0;
}

// donor PoL RVA 0x0009c07c; preferred Buka symbol ?QuickViewWait@@YIXXZ
// donor Buka TU SOURCE/KB; HoMM1 owner inferred from contiguous order
// evidence: graph:4;base=0.435968;margin=0.219505;shape=0.250;size=0.859;calls=0.600;alternate=pol20:void QuickViewWait(void)@0x0009c07c
VA(0x00453948, 0x95)
void QuickViewWait(void) {
    tag_message event;
    int done = 0;
    while (!done) {
        PollSound();
        Process1WindowsMessage();
        event = gpInputManager->GetEvent();
        if (event.type == MESSAGE_RIGHT_BUTTON_UP || event.type == MESSAGE_LEFT_BUTTON_DOWN
            || event.type == MESSAGE_LEFT_BUTTON_UP)
            done = 1;
        else
            done = 0;
    }
}

// donor PoL RVA 0x0009c111; preferred Buka symbol ?InitVars@@YIXXZ
// donor Buka TU SOURCE/KB; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.679533;margin=0.555045;shape=0.387;size=0.991;calls=0.692;strings=mnuAdv|mnuCmbt|mnuDflt;alternate=pol20:void InitVars(void)@0x0009c111
VA(0x004539dd, 0x1cb)
void InitVars(void) {
    int i;
    NULL_SAMPLE2.pSample = NULL;
    NULL_SAMPLE2.pMem = (struct _SAMPLE*)NULL_SAMPLE2.pSample;
    gbMapExtraCleared = 1;
    gGameCommand = -1;
    gPalette = NULL;
    gpPhilAI->m_debugFont = NULL;
    gbCombatSurrender = 0;
    gpGame->m_viewArmyResult = 0;
    gbInNewGameSetup = 0;
    for (i = 0; i < 140; i++)
        giGroundToTerrain[i] = i / 20;
    for (i = 0; i < FINDPATH_TERRAIN_COUNT; i++) {
        giTerrainCost[i][0] = TerrainStepCost(i, 0);
        giTerrainCost[i][1] = TerrainStepCost(i, 1);
    }
    strcpy(cNetBoxLine[0], "");
    strcpy(cNetBoxLine[1], "");
    for (i = 0; i < 255; i++)
        ppMapExtra[i] = NULL;
    hmnuDflt = LoadMenuA((HINSTANCE)hInstApp, "mnuDflt");
    hmnuCmbt = LoadMenuA((HINSTANCE)hInstApp, "mnuCmbt");
    hmnuAdv = LoadMenuA((HINSTANCE)hInstApp, "mnuAdv");
    hmnuTown = LoadMenuA((HINSTANCE)hInstApp, "mnuTown");
    LogStr("LoadMenus", (long)hmnuDflt, (long)hmnuCmbt, (long)hmnuAdv, (long)hmnuTown, (long)hInstApp);
}

// donor PoL RVA 0x0009c312; preferred Buka symbol ?ShowMoraleInfo@game@@QAEXPAVhero@@H@Z
// donor Buka TU SOURCE/KB; HoMM1 owner inferred from contiguous order
// evidence: graph:3;base=0.469331;margin=0.613523;shape=0.400;size=0.774;calls=0.649;alternate=pol20:void game::ShowMoraleInfo(class hero *, int)@0x0009c312
// clang-format off
// KB's morale-screen text table; the five-alignment line was appended last.
H1_ENUM_BEGIN(MoraleInfoText)
    MORALE_INFO_GOOD = 0,
    MORALE_INFO_NEUTRAL = 1,
    MORALE_INFO_BAD = 2,
    MORALE_INFO_HEADER = 3,
    MORALE_INFO_KNIGHT = 4,
    MORALE_INFO_ALL_TROOPS = 5,
    MORALE_INFO_THREE_ALIGNMENTS = 6,
    MORALE_INFO_FOUR_ALIGNMENTS = 7,
    MORALE_INFO_MEDAL_OF_VALOR = 8,
    MORALE_INFO_MEDAL_OF_COURAGE = 9,
    MORALE_INFO_MEDAL_OF_HONOR = 10,
    MORALE_INFO_MEDAL_OF_DISTINCTION = 11,
    MORALE_INFO_FIZBIN = 12,
    MORALE_INFO_BUOY = 13,
    MORALE_INFO_OASIS = 14,
    MORALE_INFO_STATUE = 15,
    MORALE_INFO_GRAVEYARD = 16,
    MORALE_INFO_SHIPWRECK = 17,
    MORALE_INFO_COWARDICE = 18,
    MORALE_INFO_NONE = 19,
    MORALE_INFO_FIVE_ALIGNMENTS = 20
H1_ENUM_END(MoraleInfoText)
// clang-format on
extern char* gMoraleInfoText[];

VA(0x00453ba8, 0x450)
void game::ShowMoraleInfo(hero* h, int dialogType) {
    int faction;
    int i;
    int alignments;
    int baseLen;
    char buffer[200];

    if (h->m_army.GetMorale(h, NULL) > 0)
        sprintf(buffer, gMoraleInfoText[MORALE_INFO_GOOD]);
    else if (h->m_army.GetMorale(h, NULL) == 0)
        sprintf(buffer, gMoraleInfoText[MORALE_INFO_NEUTRAL]);
    else
        sprintf(buffer, gMoraleInfoText[MORALE_INFO_BAD]);
    sprintf(gText, gMoraleInfoText[MORALE_INFO_HEADER], buffer);
    baseLen = strlen(gText);
    if (!h->m_unknown1c)
        strcat(gText, gMoraleInfoText[MORALE_INFO_KNIGHT]);
    alignments = h->m_army.IsHomogeneous(-1);
    if (alignments > 0) {
        faction = 0;
        for (i = 0; i < 5; i++) {
            if (h->m_army.m_creatureTypes[i] != CREATURE_NONE)
                faction = h->m_army.m_creatureTypes[i] / 6;
        }
        sprintf(buffer, gMoraleInfoText[MORALE_INFO_ALL_TROOPS], gAlignmentNames[faction]);
        strcat(gText, buffer);
    }
    if (alignments == -1) {
        sprintf(buffer, gMoraleInfoText[MORALE_INFO_THREE_ALIGNMENTS]);
        strcat(gText, buffer);
    }
    if (alignments == -2) {
        sprintf(buffer, gMoraleInfoText[MORALE_INFO_FOUR_ALIGNMENTS]);
        strcat(gText, buffer);
    }
    if (alignments == -3) {
        sprintf(buffer, gMoraleInfoText[MORALE_INFO_FIVE_ALIGNMENTS]);
        strcat(gText, buffer);
    }
    if (h->HasArtifact(ARTIFACT_MEDAL_OF_VALOR))
        strcat(gText, gMoraleInfoText[MORALE_INFO_MEDAL_OF_VALOR]);
    if (h->HasArtifact(ARTIFACT_MEDAL_OF_COURAGE))
        strcat(gText, gMoraleInfoText[MORALE_INFO_MEDAL_OF_COURAGE]);
    if (h->HasArtifact(ARTIFACT_MEDAL_OF_HONOR))
        strcat(gText, gMoraleInfoText[MORALE_INFO_MEDAL_OF_HONOR]);
    if (h->HasArtifact(ARTIFACT_MEDAL_OF_DISTINCTION))
        strcat(gText, gMoraleInfoText[MORALE_INFO_MEDAL_OF_DISTINCTION]);
    if (h->HasArtifact(ARTIFACT_FIZBIN_OF_MISFORTUNE))
        strcat(gText, gMoraleInfoText[MORALE_INFO_FIZBIN]);
    if (h->m_eventFlags & 2)
        strcat(gText, gMoraleInfoText[MORALE_INFO_BUOY]);
    if (h->m_eventFlags & 8)
        strcat(gText, gMoraleInfoText[MORALE_INFO_OASIS]);
    if (h->m_eventFlags & 0x100)
        strcat(gText, gMoraleInfoText[MORALE_INFO_STATUE]);
    if (h->m_eventFlags & 0x20)
        strcat(gText, gMoraleInfoText[MORALE_INFO_GRAVEYARD]);
    if (h->m_eventFlags & 0x40)
        strcat(gText, gMoraleInfoText[MORALE_INFO_SHIPWRECK]);
    if (h->m_cowardice) {
        sprintf(buffer, gMoraleInfoText[MORALE_INFO_COWARDICE], h->m_cowardice);
        strcat(gText, buffer);
    }
    if (strlen(gText) == baseLen)
        strcat(gText, gMoraleInfoText[MORALE_INFO_NONE]);
    NormalDialog(gText, dialogType, -1, -1, NORMAL_DIALOG_NO_RESOURCE, 0, NORMAL_DIALOG_NO_RESOURCE, 0, -1);
}

// clang-format off
// KB's luck-screen text table: three verdicts, a header, then one line per
// luck source in the order ShowLuckInfo appends them.
H1_ENUM_BEGIN(LuckInfoText)
    LUCK_INFO_GOOD = 0,
    LUCK_INFO_NEUTRAL = 1,
    LUCK_INFO_BAD = 2,
    LUCK_INFO_HEADER = 3,
    LUCK_INFO_RABBITS_FOOT = 4,
    LUCK_INFO_HORSESHOE = 5,
    LUCK_INFO_LUCKY_COIN = 6,
    LUCK_INFO_CLOVER = 7,
    LUCK_INFO_FAERIE_RING = 8,
    LUCK_INFO_FOUNTAIN = 9,
    LUCK_INFO_NONE = 10
H1_ENUM_END(LuckInfoText)
// clang-format on
extern char* gLuckInfoText[];

// donor PoL RVA 0x0009c92d; preferred Buka symbol ?ShowLuckInfo@game@@QAEXPAVhero@@H@Z
// donor Buka TU SOURCE/KB; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.456267;margin=0.157936;shape=0.493;size=0.606;calls=0.556;alternate=pol20:void game::ShowLuckInfo(class hero *, int)@0x0009c92d
VA(0x00453ff8, 0x1f7)
void game::ShowLuckInfo(hero* h, int dialogType) {
    int alignments;
    int baseLen;
    char buffer[200];

    if (gpGame->GetLuck(h, NULL) > 0)
        sprintf(buffer, gLuckInfoText[LUCK_INFO_GOOD]);
    else if (gpGame->GetLuck(h, NULL) == 0)
        sprintf(buffer, gLuckInfoText[LUCK_INFO_NEUTRAL]);
    else
        sprintf(buffer, gLuckInfoText[LUCK_INFO_BAD]);
    sprintf(gText, gLuckInfoText[LUCK_INFO_HEADER], buffer);
    baseLen = strlen(gText);
    if (h->HasArtifact(ARTIFACT_LUCKY_RABBITS_FOOT))
        strcat(gText, gLuckInfoText[LUCK_INFO_RABBITS_FOOT]);
    if (h->HasArtifact(ARTIFACT_GOLDEN_HORSESHOE))
        strcat(gText, gLuckInfoText[LUCK_INFO_HORSESHOE]);
    if (h->HasArtifact(ARTIFACT_GAMBLERS_LUCKY_COIN))
        strcat(gText, gLuckInfoText[LUCK_INFO_LUCKY_COIN]);
    if (h->HasArtifact(ARTIFACT_FOUR_LEAF_CLOVER))
        strcat(gText, gLuckInfoText[LUCK_INFO_CLOVER]);
    if (h->m_eventFlags & 0x10)
        strcat(gText, gLuckInfoText[LUCK_INFO_FAERIE_RING]);
    if (h->m_eventFlags & 4)
        strcat(gText, gLuckInfoText[LUCK_INFO_FOUNTAIN]);
    if (strlen(gText) == baseLen)
        strcat(gText, gLuckInfoText[LUCK_INFO_NONE]);
    NormalDialog(gText, dialogType, -1, -1, NORMAL_DIALOG_NO_RESOURCE, 0, NORMAL_DIALOG_NO_RESOURCE, 0, -1);
}

VA(0x004541ef, 0x70)
void ClearMapExtra(void) {
    int i;
    for (i = 0; i < 255; i++) {
        if (ppMapExtra[i]) {
            free(ppMapExtra[i]);
            ppMapExtra[i] = NULL;
        }
    }
    gbMapExtraCleared = 1;
}

// HoMM1 score-to-monster tables pair a threshold word with a monster word.
VA(0x0045425f, 0x8e)
short GetMonType(int score, int highScoreType) {
    int index;
    for (index = 27; index >= 0; index--) {
        if (highScoreType == 0) {
            if (giScoreCampaignMon[index][0] >= score)
                return giScoreCampaignMon[index][1];
        } else {
            if (giScoreMon[index][0] <= score)
                return giScoreMon[index][1];
        }
    }
    return giScoreMon[0][1];
}

// donor PoL RVA 0x0009ce14; preferred Buka symbol ?AddScoreToHighScore@@YIHHHHHPAD@Z
// donor Buka TU SOURCE/KB; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.701795;margin=0.122445;shape=0.377;size=0.950;calls=0.929;strings=%sCAMPAIGN.HS|%sSTANDARD.HS|.\DATA\;alternate=pol20:int AddScoreToHighScore(int, int, int, int, char *)@0x0009ce14
VA(0x004542ed, 0x3d2)
int AddScoreToHighScore(int score, int standard, char*, char* scenarioName) {
    HighScoreEntry scores[10];
    int entry;
    int dest;
    int file;
    char fileName[352];
    char enteredPlayerName[20];
    signed char missingFile;

    missingFile = 0;
    if (standard == 1)
        sprintf(fileName, "%sSTANDARD.HS", gcDataPath);
    else
        sprintf(fileName, "%sCAMPAIGN.HS", gcDataPath);
    file = open(fileName, _O_BINARY);
    if (file == -1)
        missingFile = 1;
    if (missingFile) {
        for (entry = 0; entry < 10; entry++) {
            memset(&scores[entry], 0, sizeof(HighScoreEntry));
            scores[entry].score = HIGH_SCORE_EMPTY;
        }
    } else {
        for (entry = 0; entry < 10; entry++)
            read(file, &scores[entry], sizeof(scores));
        close(file);
    }

    gbShowHighScore = 1;
    gbStandardHighScore = standard;
    giHighScoreRank = HIGH_SCORE_EMPTY;
    giScore = score;
    for (entry = 0; entry < 10; entry++) {
        if ((score >= scores[entry].score && standard == 1)
            || (score <= scores[entry].score && standard == 0)
            || scores[entry].score == HIGH_SCORE_EMPTY) {
            giHighScoreRank = entry;
            break;
        }
    }

    if (entry < 10) {
        for (dest = 8; dest >= entry; dest--)
            scores[dest + 1] = scores[dest];
        GetDataEntry("Please enter your name for the high score list.", enteredPlayerName, 16, NULL);
        strcpy(scores[entry].playerName, enteredPlayerName);
        strcpy(scores[entry].scenarioName, scenarioName);
        scores[entry].score = score;
        file = open(fileName, _O_BINARY | _O_TRUNC | _O_CREAT | _O_WRONLY, _S_IWRITE);
        if (file == -1)
            FileError(fileName);
        for (entry = 0; entry < 10; entry++)
            write(file, &scores[entry], sizeof(HighScoreEntry));
        close(file);
    }
    return 0;
}

// donor PoL RVA 0x0009d2c0; preferred Buka symbol ?BVResMsg@@YIXPADHH@Z
// donor Buka TU SOURCE/KB; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.598508;margin=0.532475;shape=0.481;size=0.968;calls=1.000;alternate=pol20:void BVResMsg(char *, int, int)@0x0009d2c0
VA(0x004546bf, 0x5b)
void BVResMsg(char* s, int res, int qty) {
    giBottomViewOverride = 5;
    giBottomViewOverrideEndTime = KBTickCount() + 5000;
    giBottomViewResource = res;
    giBottomViewResourceQty = qty;
    strcpy(gcBottomViewText, s);
    gpAdvManager->UpdBottomView(1, 1, 1);
}

// Buka 2.1 GOut.
VA(0x0045471a, 0x2e)
void GOut(char* text) {
    if (gpAdvManager->m_active == 1)
        AiPrint(text);
}

// donor PoL RVA 0x0009d3a7; preferred Buka symbol ?WaitForOtherPlayer@@YIHXZ
// donor Buka TU SOURCE/KB; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.581419;margin=0.608732;shape=0.409;size=0.995;calls=1.000;alternate=pol20:int WaitForOtherPlayer(void)@0x0009d3a7
// HoMM1 maps every remote position other than the host to the one opponent slot.
VA(0x00454748, 0x39)
signed char NetPosToGamePos(int netPos) {
    if (netPos == 0)
        return 0;
    else if (netPos > 0)
        return 1;
    return -1;
}

VA(0x00454781, 0xda)
signed char WaitForOtherPlayer(void) {
    int result = 0;
    RemoteMessage* data;
    PollSound();
    data = (RemoteMessage*)GetRemoteData(1);
    if (data && data->type == REMOTE_MESSAGE_RELIABLE) {
        switch (data->command) {
            case BOX_REMOTE_SETUP:
                memcpy(gbGamePosToNetPos, data->payload.data, 4);
                giThisGamePos = NetPosToGamePos(giThisNetPos);
                giHostGamePos = NetPosToGamePos(0);
                break;
            case BOX_REMOTE_SAVE:
                result = gpGame->ReceiveSaveGame(data->payload.saveSize, data->sender);
                break;
        }
    }
    return result;
}

// donor PoL RVA 0x0009d4a6; preferred Buka symbol ?PopNetBox@@YIXPADH@Z
// donor Buka TU SOURCE/KB; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.593152;margin=0.055238;shape=0.393;size=0.624;calls=0.688;strings=netbox.bin;alternate=pol20:void PopNetBox(char *, int)@0x0009d4a6
VA(0x0045485b, 0x6f4)
void PopNetBox(char* notice) {
    char* data;
    signed char blinkState;
    signed char drawLines;
    signed char bClose;
    int firstId;
    font* font;
    signed char shown;
    int pause;
    int lineTextLimit;
    signed char exitForIncomingData;
    signed char sendText;
    tag_message incoming;
    tag_message message;
    int len;
    char text[80];
    signed char oldShowIt;
    signed char updateInput;
    int lineHeight;
    long msgTime;
    heroWindow* netWin;
    int success;
    int textWidth;

    if (!gbRemoteOn)
        return;
    lineTextLimit = 60;
    firstId = 1;
    lineHeight = 42;
    font = gpResourceManager->GetFont("bigfont.fnt");
    msgTime = 0;
    if (notice) {
        AddNetBoxLine(notice);
        msgTime = KBTickCount();
    }
    len = 0;
    shown = gpMouseManager->IsVis();
    oldShowIt = bShowIt;
    bShowIt = 1;
    netWin = new heroWindow(0, 418, "netbox.bin");
    if (!netWin)
        MemError();
    message.type = MESSAGE_WIDGET;
    message.command = WIDGET_COMMAND_SET_TEXT;
    message.id = 1;
    message.text = cNetBoxLine[0];
    netWin->BroadcastMessage(message);
    message.id = 2;
    message.text = cNetBoxLine[1];
    netWin->BroadcastMessage(message);
    gpWindowManager->AddWindow(netWin, -1, 1);
    gpMouseManager->ReallyHidePointer();
    exitForIncomingData = 0;
    bClose = 0;
    updateInput = 1;
    blinkState = 0;
    sendText = 0;
    drawLines = 1;
    strcpy(text, "");
    gpInputManager->SetKeyCodeType(0);

    while (!bClose) {
        PollSound();
        data = GetRemoteData(0);
        if (data) {
            if (reinterpret_cast<RemoteMessage*>(data)->type != REMOTE_MESSAGE_RELIABLE) {
                data = GetRemoteData(1);
            } else {
                switch (reinterpret_cast<RemoteMessage*>(data)->command) {
                    case 11:
                        data = GetRemoteData(1);
                        AddNetBoxLine(reinterpret_cast<RemoteMessage*>(data)->payload.data);
                        drawLines = 1;
                        if (msgTime)
                            msgTime = KBTickCount();
                        break;
                    default:
                        AddNetBoxLine("[ Incoming data, must exit... ]");
                        drawLines = 1;
                        exitForIncomingData = 1;
                        break;
                }
            }
        }

        Process1WindowsMessage();
        incoming = gpInputManager->GetEvent();
        switch (incoming.type) {
            case MESSAGE_KEY_DOWN:
                msgTime = 0;
                switch (incoming.keyCode) {
                    case 0x1b:
                    case 0x3b00:
                        bClose = 1;
                        break;
                    case 0x7f:
                        if (len > 0)
                            len--;
                        updateInput = 1;
                        blinkState = 1;
                        break;
                    case 10:
                        sendText = 1;
                        break;
                    default:
                        if (len < 58 && incoming.keyCode) {
                            text[len] = 0;
                            textWidth = font->LineWidth(text);
                            if (textWidth + 30 < 610) {
                                text[len] = incoming.keyCode;
                                len++;
                                updateInput = 1;
                                blinkState = 0;
                            }
                        }
                }
        }

        if (!updateInput && KBTickCount() > glTimers[0]) {
            blinkState = 1 - blinkState;
            updateInput = 1;
        }
        if (sendText) {
            sendText = 0;
            text[len] = 0;
            AddNetBoxLine(text);
            success = TransmitRemoteData(text, REMOTE_BROADCAST_PLAYER, strlen(text) + 1, 11, 1, 1, -1, 1);
            if (!success)
                ShutDown(NULL);
            len = 0;
            strcpy(text, "");
            updateInput = 1;
            drawLines = 1;
        }
        if (drawLines) {
            drawLines = 0;
            message.type = MESSAGE_WIDGET;
            message.command = WIDGET_COMMAND_SET_TEXT;
            message.id = 1;
            message.text = cNetBoxLine[0];
            netWin->BroadcastMessage(message);
            message.id = 2;
            message.text = cNetBoxLine[1];
            netWin->BroadcastMessage(message);
            netWin->DrawWindow();
            gpWindowManager->UpdateScreenRegion(0, 418, 639, 61);
        }
        if (updateInput) {
            updateInput = 0;
            glTimers[0] = KBTickCount() + 360;
            if (blinkState)
                text[len] = '_';
            else
                text[len] = ' ';
            text[len + 1] = 0;
            message.type = MESSAGE_WIDGET;
            message.command = WIDGET_COMMAND_SET_TEXT;
            message.id = 3;
            message.text = text;
            netWin->BroadcastMessage(message);
            netWin->DrawWindow();
            gpWindowManager->UpdateScreenRegion(0, 460, 639, 16);
        }
        if (msgTime && KBTickCount() > msgTime + 6000)
            bClose = 1;
        if (exitForIncomingData) {
            for (pause = 0; pause < 30; pause++) {
                PollSound();
                DelayMilli(90);
            }
            bClose = 1;
        }
    }
    gpInputManager->SetKeyCodeType(1);
    gpWindowManager->RemoveWindow(netWin);
    bShowIt = oldShowIt;
    if (shown)
        gpMouseManager->ReallyShowPointer();
    gpResourceManager->Dispose(font);
}

// Buka 2.1 AddNetBoxLine reduced to HoMM1's two uncoloured lines.
VA(0x00454f4f, 0x3b)
void AddNetBoxLine(char* text) {
    strcpy(cNetBoxLine[0], cNetBoxLine[1]);
    strcpy(cNetBoxLine[1], text);
}

// donor PoL RVA 0x0009e0f2; preferred Buka symbol ?ShutDown@@YIXPAD@Z
// donor Buka TU SOURCE/KB; HoMM1 owner inferred from contiguous order
// evidence: graph:3;base=0.466886;margin=0.632520;shape=0.403;size=0.708;calls=0.667;alternate=pol20:void ShutDown(char *)@0x0009e0f2
VA(0x00454f8a, 0x14f)
void ShutDown(char* message) {
    char buffer[768];
    if (bInShutDown)
        return;
    bInShutDown = 1;
    gbClosingApp = 1;
    buffer[0] = 0;
    if (message) {
        strcpy(buffer, message);
        SetFullScreenStatus(0);
        LogStr(buffer);
        MessageBoxA((HWND)hwndApp, buffer, "Unexpected Program Termination", MB_ICONHAND);
    }
    ClearMapExtra();
    UnloadSystemwideIcons();
    if (gbRemoteOn)
        HandleRemoteSuddenExit();
    if (gPalette) {
        gpResourceManager->Dispose(gPalette);
        gPalette = NULL;
    }
    if (gpPhilAI->m_debugFont) {
        gpResourceManager->Dispose(gpPhilAI->m_debugFont);
        gpPhilAI->m_debugFont = NULL;
    }
    gpExec->ShutDownSystem();
    RemoteCleanup();
    if (gEventHandle) {
        CloseHandle(gEventHandle);
        gEventHandle = NULL;
    }
    DeleteMainClasses();
    AppExit();
    PrintMemoryLeaks();
    exit(0);
}

// donor PoL RVA 0x0009e306; preferred Buka symbol ?FileError@@YIXPAD@Z
// donor Buka TU SOURCE/KB; HoMM1 owner inferred from contiguous order
// evidence: graph:3;base=0.316461;margin=0.125092;shape=0.216;size=0.484;calls=0.500;alternate=pol20:void FileError(char *)@0x0009e306
VA(0x004550d9, 0x4a)
void FileError(char* filename) {
    char message[200];
    LogStr("File Error");
    sprintf(message, "Error opening file %s!", filename);
    ShutDown(message);
}

// @early-stop
// tu-cumulative: logic + all 14 frame slots byte-exact (od_oracle-verified). The only
// residual (coffcmp: 40 bytes, all in the two brightness averages + the minDist test)
// is a /Od operand-evaluation-order difference this cl renders vs retail: the 3-term
// sum `p[2]+p[0]+p[1]` reads +2,+1,+0 here but +2,+0,+1 in retail, and the `d>p`
// compare loads the other operand first. Not source-steerable (probed every term
// ordering, explicit grouping, `|0`, and an inline helper — all identical here).

// Campaign-text and score-label tables and the score-to-rank creature names.
extern char* gCampaignWinTexts[];
extern char* gScoreLabels[];
extern char* gScoreRankNames[];
int GetBaseScore(int);
void CongratsWait(void);

// HoMM1's victory screen (Buka 2.1 ShowCongrats): campaigns show the
// scenario's win text; standard games score the days played, rank the result
// as a creature and file it with the high scores.
VA(0x00455123, 0x3be)
void ShowCongrats(void) {
    char name[32];
    int i;
    int result;
    tag_message message;
    int daysScore;
    heroWindow* win;

    daysScore = GetBaseScore(giCurTurn);
    result = gpGame->m_difficultyRating * daysScore / 100;
    gpSoundManager->SwitchAmbientMusic(54);
    gpMouseManager->ReallyHidePointer();
    sprintf(gText, "congrats.bmp");
    gpResourceManager->GetBackdrop(gText, gpWindowManager->m_screen);
    message.type = MESSAGE_WIDGET;
    message.command = WIDGET_COMMAND_SET_TEXT;
    message.text = gText;
    if (gpGame->m_campaignType > 0) {
        win = new heroWindow(0, 0, "congrats.bin");
        if (!win)
            MemError();
        sprintf(gText, gCampaignWinTexts[gpGame->m_campaignScenario]);
        message.id = 100;
        win->BroadcastMessage(message);
    } else {
        win = new heroWindow(0, 0, "congspre.bin");
        if (!win)
            MemError();
        sprintf(name, gScoreRankNames[GetMonType(result, 1)]);
        name[0] -= 32;
        sprintf(gText, "A Glorious Victory!");
        message.id = 100;
        win->BroadcastMessage(message);
        for (i = 0; i < 5; i++) {
            sprintf(gText, gScoreLabels[i]);
            message.id = i + 101;
            win->BroadcastMessage(message);
        }
        sprintf(gText, "%d", giCurTurn);
        message.id = 106;
        win->BroadcastMessage(message);
        sprintf(gText, "%d", daysScore);
        message.id = 107;
        win->BroadcastMessage(message);
        sprintf(gText, "%d%%", gpGame->m_difficultyRating);
        message.id = 108;
        win->BroadcastMessage(message);
        sprintf(gText, "%d", result);
        message.id = 109;
        win->BroadcastMessage(message);
        sprintf(gText, "%s", name);
        message.id = 110;
        win->BroadcastMessage(message);
    }
    gpWindowManager->AddWindow(win, -1, 1);
    gpMouseManager->ReallyHidePointer();
    gpWindowManager->FadeScreen(0, 8, NULL);
    CongratsWait();
    gpWindowManager->RemoveWindow(win);
    delete win;
    if (gpGame->m_campaignType <= 0)
        AddScoreToHighScore(result, 1, "", gpGame->m_mapName);
}

// donor PoL RVA 0x0009e900; preferred Buka symbol ?CongratsWait@@YIXXZ
// donor Buka TU SOURCE/KB; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.447463;margin=0.065171;shape=0.300;size=0.684;calls=1.000;alternate=pol20:void CongratsWait(void)@0x0009e900
VA(0x004554e1, 0xb1)
void CongratsWait(void) {
    int cmd = 0;
    signed char finished = 0;
    tag_message message;
    gpInputManager->Flush();
    while (!finished) {
        PollSound();
        Process1WindowsMessage();
        message = gpInputManager->GetEvent();
        if (message.type == MESSAGE_KEY_DOWN || message.type == MESSAGE_LEFT_BUTTON_DOWN
            || message.type == MESSAGE_LEFT_BUTTON_UP || message.type == MESSAGE_RIGHT_BUTTON_DOWN
            || message.type == MESSAGE_RIGHT_BUTTON_UP)
            finished = 1;
    }
}

// Buka 2.1 GetDataEntry without the prompt-sized window and textEntryWidget.
VA(0x00455592, 0x1c7)
void GetDataEntry(char* prompt, char* destination, int maximumLength, char* initialText) {
    short widgetId = 10;
    tag_message message;
    char textBuffer[100];

    gpMouseManager->SetPointer("advmice.mse", 0);
    cDEDest = destination;
    iDEMaxLen = maximumLength;
    strcpy(cDEDest, "");
    DataEntryWin = new heroWindow(0xb1, 0x14, "dataentr.bin");
    if (!DataEntryWin)
        MemError();
    message.type = MESSAGE_WIDGET;
    message.command = WIDGET_COMMAND_SET_TEXT;
    message.id = 1;
    message.text = prompt;
    DataEntryWin->BroadcastMessage(message);
    if (initialText)
        strcpy(textBuffer, initialText);
    else
        strcpy(textBuffer, "");
    message.id = 10;
    message.text = textBuffer;
    DataEntryWin->BroadcastMessage(message);
    strcpy(destination, textBuffer);
    bDataEntryTime = 0;
    gpWindowManager->DoDialog(DataEntryWin, DataEntryWindowHandler, 0);
    delete DataEntryWin;
}

VA(0x00455759, 0x1d9)
short DataEntryWindowHandler(tag_message& message) {
    short widgetId = 10;

    if (bDataEntryTime == 0) {
        ++bDataEntryTime;
        message.type = MESSAGE_LEFT_BUTTON_DOWN;
        message.x = 0xc3;
        message.y = 0x9a;
        DataEntryWin->BroadcastMessage(message);
        return MESSAGE_DISPATCH_CONSUME;
    }

    if (bDataEntryTime == 1) {
        ++bDataEntryTime;
        goto gotText;
    }
    if (message.type == MESSAGE_WIDGET) {
        switch (message.command) {
            case WIDGET_NOTIFY_SELECT:
                switch (message.id) {
                    case 10:
                    gotText:
                        message.type = MESSAGE_WIDGET;
                        message.id = 10;
                        message.command = WIDGET_COMMAND_GET_TEXT;
                        DataEntryWin->BroadcastMessage(message);
                        if (strlen(message.text) == 0) {
                            break;
                        } else {
                            memset(cDEDest, 0, iDEMaxLen);
                            strncpy(cDEDest, message.text, iDEMaxLen - 1);
                        }
                        message.type = MESSAGE_WIDGET;
                        message.command = WIDGET_COMMAND_SET_TEXT;
                        message.id = 10;
                        message.text = cDEDest;
                        DataEntryWin->BroadcastMessage(message);
                        DataEntryWin->DrawWindow(1, 10, 10);
                        gpWindowManager->m_dialogResult = message.id;
                        message.command = message.id =
                            WIDGET_COMMAND_DIALOG_SELECT;
                        return MESSAGE_DISPATCH_FORWARD;
                }
        }
    }
    return EventWindowHandler(message);
}

// donor PoL RVA 0x0009e999; preferred Buka symbol ?LoadPlaySample@@YIPAVsample@@PAD@Z
// donor Buka TU SOURCE/KB; HoMM1 owner inferred from contiguous order
// evidence: graph:6;base=0.524829;margin=1.189024;shape=0.423;size=0.802;calls=1.000;alternate=pol20:struct SAMPLE2 LoadPlaySample(char *)@0x0009e999
VA(0x00455932, 0x51)
SAMPLE2 LoadPlaySample(char* name) {
    SAMPLE2 s;
    s.pSample = gpResourceManager->GetSample(name);
    if (s.pSample) {
        s.pSample->m_playbackData.channelType = SAMPLE_PLAYBACK_CHANNEL_GROUP;
        s.pMem = gpSoundManager->MemorySample(s.pSample);
    }
    return s;
}

// donor PoL RVA 0x0009e9ed; preferred Buka symbol ?WaitEndSample@@YIXPAPAVsample@@H@Z
// donor Buka TU SOURCE/KB; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.479563;margin=0.490944;shape=0.207;size=0.957;calls=1.000;alternate=pol20:void WaitEndSample(struct SAMPLE2, int)@0x0009e9ed
VA(0x00455983, 0x8a)
void WaitEndSample(SAMPLE2 s, int waitTime) {
    if (waitTime < 0)
        waitTime = 4000;
    long endTime = KBTickCount() + waitTime;
    if (s.pMem) {
        while (gpSoundManager->DigitalReport(s.pMem, 4) && KBTickCount() < endTime) {
            Process1WindowsMessage();
            PollSound();
        }
    }
    if (s.pSample)
        gpResourceManager->Dispose(s.pSample);
}

// donor PoL RVA 0x0009ea7c; preferred Buka symbol ?MemError@@YIXXZ
// donor Buka TU SOURCE/KB; HoMM1 owner inferred from contiguous order
// evidence: graph:4;base=0.499168;margin=0.828160;shape=0.176;size=0.610;calls=1.000;strings=Out of Memory;alternate=pol20:void MemError(void)@0x0009ea7c
VA(0x00455a0d, 0x7b)
void MemError(void) {
    if (gbInMemError)
        return;
    gbInMemError = 1;
    LogStr("Out of Memory");
    sprintf(
        gText,
        "\n\n%s\n%s\n%d%s\n%d%s\n\n",
        gcMemoryErrorTitle,
        gcMemoryRequirements,
        giRequiredExtendedMemory,
        gcExtendedMemoryUnits,
        giRequiredConventionalMemory,
        gcConventionalMemoryUnits
    );
    ShutDown(gText);
}

// Buka 2.1 MiscRuntime MemSize: a fixed reported memory size.
// @dead-code
// Zero-ref: no effective incoming retail reference.
VA(0x00455a88, 0x15)
int MemSize(int) {
    return 16034;
}

// Buka 2.1 CheckMem without HoMM2's memory globals.
VA(0x00455a9d, 0x12)
signed char CheckMem(void) {
    return 1;
}

// Campaign maps rename the town at a fixed position (x, y, then the name).
#pragma pack(push, 1)
struct campaignTownName {
    signed char x;
    signed char y;
    char name[83];
};
#pragma pack(pop)
extern campaignTownName gCampaignTownNames[];
extern char* gTownNames[];

// Buka 2.1 GetTownName; HoMM1 towns carry a name index, and campaign maps
// override one town by position.
VA(0x00455aaf, 0xdc)
char* GetTownName(int i) {
    town* townPointer = gpGame->GetTown(i);
    if (gpGame->m_campaignType > 0
        && gCampaignTownNames[gpGame->m_campaignScenario].x >= 0
        && gCampaignTownNames[gpGame->m_campaignScenario].x == townPointer->m_x
        && gCampaignTownNames[gpGame->m_campaignScenario].y == townPointer->m_y)
        return gCampaignTownNames[gpGame->m_campaignScenario].name;
    return gTownNames[townPointer->m_threat];
}

// Buka 2.1 Misc IsCDDrive.
VA(0x00455b8b, 0x51)
int IsCDDrive(int driveIndex) {
    sprintf(gText, "A:\\");
    gText[0] += driveIndex;
    return GetDriveTypeA(gText) == DRIVE_CDROM;
}

VA(0x00455bdc, 0x64)
void LoadSystemwideIcons(void) {
    gBuyBuildIcons = gpResourceManager->GetIcon("buybuild.icn");
    gSystemIcons = gpResourceManager->GetIcon("system.icn");
    bigFont = gpResourceManager->GetFont("bigfont.fnt");
    smallFont = gpResourceManager->GetFont("smalfont.fnt");
}

VA(0x00455c40, 0x54)
void UnloadSystemwideIcons(void) {
    gpResourceManager->Dispose(gBuyBuildIcons);
    gpResourceManager->Dispose(gSystemIcons);
    gpResourceManager->Dispose(bigFont);
    gpResourceManager->Dispose(smallFont);
}

// Retail empty lifecycle hook; Buka and PoL KB correspondence.
VA(0x00455c94, 0x10)
void EarlyShutDownSystem(void) {}

// Buka 2.1 GameUnsaved.
VA(0x00455ca4, 0x7e)
int GameUnsaved(void) {
    if ((gpAdvManager && gpAdvManager->m_active == 1)
        || (gpCombatManager && gpCombatManager->m_active == 1)
        || (gpTownManager && gpTownManager->m_active == 1))
        return 1;
    else
        return 0;
}


// donor PoL RVA 0x0009ec05; preferred Buka symbol ?HandleAppSpecificMenuCommands@@YIHH@Z
// donor Buka TU SOURCE/KB; HoMM1 owner inferred from contiguous order
// evidence: graph:4;base=0.410709;margin=0.595745;shape=0.257;size=0.699;calls=0.542;alternate=pol20:int HandleAppSpecificMenuCommands(int)@0x0009ec05
VA(0x00455d22, 0x629)
int HandleAppSpecificMenuCommands(int command) {
    int menuChanged;

    menuChanged = 0;
    switch (command) {
        case APP_MENU_NEW_STANDARD_GAME:
        case APP_MENU_NEW_CAMPAIGN_IRONFIST:
        case APP_MENU_NEW_CAMPAIGN_SLAYER:
        case APP_MENU_NEW_CAMPAIGN_LAMANDA:
        case APP_MENU_NEW_CAMPAIGN_ALAMAR:
        case APP_MENU_NEW_HOT_SEAT_2:
        case APP_MENU_NEW_HOT_SEAT_3:
        case APP_MENU_NEW_HOT_SEAT_4:
        case APP_MENU_NEW_NETWORK_HOST:
        case APP_MENU_NEW_NETWORK_GUEST:
        case APP_MENU_NEW_MODEM_HOST:
        case APP_MENU_NEW_MODEM_GUEST:
        case APP_MENU_NEW_DIRECT_HOST:
        case APP_MENU_NEW_DIRECT_GUEST:
            strcpy(gText, "Are you sure you want to restart?  (Your current game will be lost)");
            goto confirmMenuCommand;
        case APP_MENU_LOAD_STANDARD_GAME:
        case APP_MENU_LOAD_CAMPAIGN_GAME:
        case APP_MENU_LOAD_HOT_SEAT_2:
        case APP_MENU_LOAD_HOT_SEAT_3:
        case APP_MENU_LOAD_HOT_SEAT_4:
        case APP_MENU_LOAD_NETWORK_HOST:
        case APP_MENU_LOAD_NETWORK_GUEST:
        case APP_MENU_LOAD_MODEM_HOST:
        case APP_MENU_LOAD_MODEM_GUEST:
        case APP_MENU_LOAD_DIRECT_HOST:
        case APP_MENU_LOAD_DIRECT_GUEST:
            strcpy(gText, "Are you sure you want to load a new game?  (Your current game will be lost)");
        confirmMenuCommand:
            if (gpAdvManager->m_active == 1) {
                NormalDialog(gText, NORMAL_DIALOG_TYPE_YES_NO, -1, -1, NORMAL_DIALOG_NO_RESOURCE, 0, NORMAL_DIALOG_NO_RESOURCE, 0, -1);
                if (gpWindowManager->m_dialogResult != NORMAL_DIALOG_CONFIRM)
                    break;
            }
            giMenuCommand = command;
            break;
        case APP_MENU_SAVE_GAME:
            SaveGame();
            break;
        case APP_MENU_QUIT:
            PostMessage((HWND)hwndApp, WM_CLOSE, 0, 0);
            break;
        case APP_MENU_MUSIC_OFF:
            gConfig.musicVolume = 0;
            goto adjustMusic;
        case APP_MENU_MUSIC_100:
            gConfig.musicVolume = 1;
            goto adjustMusic;
        case APP_MENU_MUSIC_90:
            gConfig.musicVolume = 2;
            goto adjustMusic;
        case APP_MENU_MUSIC_80:
            gConfig.musicVolume = 3;
            goto adjustMusic;
        case APP_MENU_MUSIC_70:
            gConfig.musicVolume = 4;
            goto adjustMusic;
        case APP_MENU_MUSIC_60:
            gConfig.musicVolume = 5;
            goto adjustMusic;
        case APP_MENU_MUSIC_50:
            gConfig.musicVolume = 6;
            goto adjustMusic;
        case APP_MENU_MUSIC_40:
            gConfig.musicVolume = 7;
            goto adjustMusic;
        case APP_MENU_MUSIC_30:
            gConfig.musicVolume = 8;
            goto adjustMusic;
        case APP_MENU_MUSIC_20:
            gConfig.musicVolume = 9;
            goto adjustMusic;
        case APP_MENU_MUSIC_10:
            gConfig.musicVolume = 10;
            goto adjustMusic;
        adjustMusic:
            gpSoundManager->AdjustMusicVolumes();
            menuChanged = 1;
            break;
        case APP_MENU_SOUND_OFF:
            gConfig.soundVolume = 0;
            goto adjustSound;
        case APP_MENU_SOUND_100:
            gConfig.soundVolume = 1;
            goto adjustSound;
        case APP_MENU_SOUND_90:
            gConfig.soundVolume = 2;
            goto adjustSound;
        case APP_MENU_SOUND_80:
            gConfig.soundVolume = 3;
            goto adjustSound;
        case APP_MENU_SOUND_70:
            gConfig.soundVolume = 4;
            goto adjustSound;
        case APP_MENU_SOUND_60:
            gConfig.soundVolume = 5;
            goto adjustSound;
        case APP_MENU_SOUND_50:
            gConfig.soundVolume = 6;
            goto adjustSound;
        case APP_MENU_SOUND_40:
            gConfig.soundVolume = 7;
            goto adjustSound;
        case APP_MENU_SOUND_30:
            gConfig.soundVolume = 8;
            goto adjustSound;
        case APP_MENU_SOUND_20:
            gConfig.soundVolume = 9;
            goto adjustSound;
        case APP_MENU_SOUND_10:
            gConfig.soundVolume = 10;
            goto adjustSound;
        adjustSound:
            gpSoundManager->AdjustSoundVolumes();
            menuChanged = 1;
            break;
        case APP_MENU_SPEED_JUMP:
            gConfig.walkSpeed = 4;
            goto walkSpeedChanged;
        case APP_MENU_SPEED_GALLOP:
            gConfig.walkSpeed = 3;
            goto walkSpeedChanged;
        case APP_MENU_SPEED_CANTER:
            gConfig.walkSpeed = 2;
            goto walkSpeedChanged;
        case APP_MENU_SPEED_TROT:
            gConfig.walkSpeed = 1;
            goto walkSpeedChanged;
        case APP_MENU_SPEED_WALK:
            gConfig.walkSpeed = 0;
            goto walkSpeedChanged;
        walkSpeedChanged:
            menuChanged = 1;
            break;
        case APP_MENU_CD_STEREO:
            if (gConfig.musicSource) {
                gConfig.musicSource = 0;
            } else {
                if (!gpSoundManager->m_cdStarted) {
                    NormalDialog(
                        "Unable to set up CD stereo music.  Your CD player might be in use by "
                        "another program, or your sound driver might not support CD stereo.",
                        NORMAL_DIALOG_TYPE_OK, -1, -1, NORMAL_DIALOG_NO_RESOURCE, 0, NORMAL_DIALOG_NO_RESOURCE, 0, -1);
                    break;
                }
                gConfig.musicSource = 2;
            }
            gpSoundManager->SetMusicQuality(gConfig.musicSource);
            menuChanged = 1;
            break;
        case APP_MENU_SHOW_PATH:
            gConfig.showRoute = 1 - gConfig.showRoute;
            menuChanged = 1;
            break;
        case APP_MENU_VIEW_ENEMY_MOVES:
            gConfig.blackoutComputer = 1 - gConfig.blackoutComputer;
            menuChanged = 1;
            break;
        case APP_MENU_VIEW_WORLD:
            gpAdvManager->ViewWorld(24, 0, 0);
            break;
        case APP_MENU_VIEW_PUZZLE:
            gpAdvManager->ViewPuzzle();
            break;
        case APP_MENU_CAST_SPELL:
            gpAdvManager->CheckCastSpell();
            break;
        case APP_MENU_DIG:
            gpAdvManager->ProcessSearch(-1, -1);
            break;
        default:
            return 1;
    }
    if (menuChanged)
        WritePrefs();
    return 0;
}

// Checks the music, sound and walk-speed radio groups, then the CD,
// route and enemy-move toggles.
VA(0x0045634b, 0x3b7)
void UpdateSystemOptionsMenu(void) {
    int checkedCommand;
    int menuCommand;

    if (!gConfig.gfx[giCurExe].showMenu)
        return;
    if (!hmnuApp)
        return;
    if (hmnuApp != hmnuAdv)
        return;

    for (menuCommand = APP_MENU_MUSIC_FIRST; menuCommand <= APP_MENU_MUSIC_LAST; menuCommand++)
        CheckMenuItem((HMENU)hmnuApp, menuCommand, MF_UNCHECKED);
    switch (gConfig.musicVolume) {
        case 1:
            checkedCommand = APP_MENU_MUSIC_100;
            break;
        case 2:
            checkedCommand = APP_MENU_MUSIC_90;
            break;
        case 3:
            checkedCommand = APP_MENU_MUSIC_80;
            break;
        case 4:
            checkedCommand = APP_MENU_MUSIC_70;
            break;
        case 5:
            checkedCommand = APP_MENU_MUSIC_60;
            break;
        case 6:
            checkedCommand = APP_MENU_MUSIC_50;
            break;
        case 7:
            checkedCommand = APP_MENU_MUSIC_40;
            break;
        case 8:
            checkedCommand = APP_MENU_MUSIC_30;
            break;
        case 9:
            checkedCommand = APP_MENU_MUSIC_20;
            break;
        case 10:
            checkedCommand = APP_MENU_MUSIC_10;
            break;
        default:
            checkedCommand = APP_MENU_MUSIC_OFF;
            break;
    }
    CheckMenuItem((HMENU)hmnuApp, checkedCommand, MF_CHECKED);

    for (menuCommand = APP_MENU_SOUND_FIRST; menuCommand <= APP_MENU_SOUND_LAST; menuCommand++)
        CheckMenuItem((HMENU)hmnuApp, menuCommand, MF_UNCHECKED);
    switch (gConfig.soundVolume) {
        case 1:
            checkedCommand = APP_MENU_SOUND_100;
            break;
        case 2:
            checkedCommand = APP_MENU_SOUND_90;
            break;
        case 3:
            checkedCommand = APP_MENU_SOUND_80;
            break;
        case 4:
            checkedCommand = APP_MENU_SOUND_70;
            break;
        case 5:
            checkedCommand = APP_MENU_SOUND_60;
            break;
        case 6:
            checkedCommand = APP_MENU_SOUND_50;
            break;
        case 7:
            checkedCommand = APP_MENU_SOUND_40;
            break;
        case 8:
            checkedCommand = APP_MENU_SOUND_30;
            break;
        case 9:
            checkedCommand = APP_MENU_SOUND_20;
            break;
        case 10:
            checkedCommand = APP_MENU_SOUND_10;
            break;
        default:
            checkedCommand = APP_MENU_SOUND_OFF;
            break;
    }
    CheckMenuItem((HMENU)hmnuApp, checkedCommand, MF_CHECKED);

    for (menuCommand = APP_MENU_SPEED_FIRST; menuCommand <= APP_MENU_SPEED_LAST; menuCommand++)
        CheckMenuItem((HMENU)hmnuApp, menuCommand, MF_UNCHECKED);
    switch (gConfig.walkSpeed) {
        case 4:
            checkedCommand = APP_MENU_SPEED_JUMP;
            break;
        case 3:
            checkedCommand = APP_MENU_SPEED_GALLOP;
            break;
        case 2:
            checkedCommand = APP_MENU_SPEED_CANTER;
            break;
        case 1:
            checkedCommand = APP_MENU_SPEED_TROT;
            break;
        default:
            checkedCommand = APP_MENU_SPEED_WALK;
            break;
    }
    CheckMenuItem((HMENU)hmnuApp, checkedCommand, MF_CHECKED);
    CheckMenuItem((HMENU)hmnuApp, APP_MENU_CD_STEREO, gConfig.musicSource ? MF_CHECKED : MF_UNCHECKED);
    CheckMenuItem((HMENU)hmnuApp, APP_MENU_SHOW_PATH, gConfig.showRoute ? MF_CHECKED : MF_UNCHECKED);
    CheckMenuItem((HMENU)hmnuApp, APP_MENU_VIEW_ENEMY_MOVES,
                  1 - gConfig.blackoutComputer ? MF_CHECKED : MF_UNCHECKED);
}

VA(0x00456702, 0x99)
void CleanUpMenus(void) {
    if (hmnuApp) {
        SetMenu((HWND)hwndApp, NULL);
        if (hmnuAdv)
            DestroyMenu((HMENU)hmnuAdv);
        if (hmnuDflt)
            DestroyMenu((HMENU)hmnuDflt);
        if (hmnuCmbt)
            DestroyMenu((HMENU)hmnuCmbt);
        if (hmnuTown)
            DestroyMenu((HMENU)hmnuTown);
    }
    hmnuApp = NULL;
}

VA(0x0045679b, 0x24)
void UpdateAppSpecificMenus(void* hMenu) {
    if (hmnuAdv == hMenu)
        UpdateSystemOptionsMenu();
}

VA(0x004567bf, 0x22)
void EarlyResizeWindow(int, int, int, int) {
    if (gbClosingApp)
        return;
}

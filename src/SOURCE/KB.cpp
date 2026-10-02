// Located from HoMM2 Buka 2.1; PoL 2.0 supplies the VC4 declaration.

#include <match.h>

#include <BASE/BITS.h>
#include <BASE/BMAP2.h>
#include <BASE/INPUTMGR_TYPES.h>
#include <BASE/Misc.h>
#include <H1/All.h>
#include <H1/KB.h>
#include <SOURCE/X_GLOBAL.h>
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
#include <SOURCE/Modem.h>
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

// Retail score-dialog owner byte (.bss).
DATA(0x004c794c)
signed char giHighScoreType;
// InitVars proves seven terrain rows, ordinary/diagonal cost columns.
DATA(0x004c6d50)
signed char giTerrainCost[FINDPATH_TERRAIN_COUNT][FINDPATH_STEP_COST_COUNT];

// HoMM2 KB.cpp confirms the identity and behavior. HoMM1 differs in the timer
// comparison and placement of the re-entry guard.
VA(0x0044f640, 0x72)
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
    if (iCDRomErr == CD_SETUP_NO_DRIVE) {
        MessageBoxA((HWND)hwndApp, "Unable to access CD Drive.", "Startup Error", MB_ICONHAND);
        exit(0);
    }
    if (iCDRomErr == CD_SETUP_NOT_FOUND) {
        MessageBoxA(
            (HWND)hwndApp,
            "You must have the Heroes Win95 CD in the CD-ROM drive to play \nHeroes of "
            "Might and Magic.  \n\nPlease insert the CD and try again.",
            "Startup Error",
            MB_ICONHAND
        );
        exit(0);
    }
    if (iCDRomErr == CD_SETUP_NO_APP_PATH) {
        MessageBoxA(
            (HWND)hwndApp,
            "Unable to change to the Heroes directory.  Please run the installation "
            "program.",
            "Startup Error",
            MB_ICONHAND
        );
        exit(0);
    }
    if (iCDRomErr == CD_SETUP_NO_DATA) {
        MessageBoxA(
            (HWND)hwndApp,
            "Unable to find the Heroes data files.  Please run the installation program.",
            "Startup Error",
            MB_ICONHAND
        );
        exit(0);
    }
    InitVars();
    return 1;
}

// clang-format off
// InitMenuHandler's right-click help: the gInitMenuHelp row.
H1_ENUM_BEGIN(MainMenuHelp)
    MAIN_MENU_HELP_NONE = -1,
    MAIN_MENU_HELP_NEW_GAME = 0,
    MAIN_MENU_HELP_LOAD_GAME = 1,
    MAIN_MENU_HELP_HIGH_SCORES = 2,
    MAIN_MENU_HELP_CREDITS = 3,
    MAIN_MENU_HELP_QUIT = 4
H1_ENUM_END(MainMenuHelp)

// giEndSequence: CheckEndGame sets LOST/WON, and WON becomes CAMPAIGN_COMPLETE
// after the last campaign scenario; oldmain plays the matching video (the
// value indexes lowResVideos/hiResVideos), offers a replay after LOST and
// advances the campaign after WON.
H1_ENUM_BEGIN(GameEndSequence)
    GAME_END_LOST = 0,
    GAME_END_WON = 1,
    GAME_END_CAMPAIGN_COMPLETE = 2
H1_ENUM_END(GameEndSequence)
// clang-format on

// Buka 2.1 oldmain reduced to HoMM1: two intro videos, the stpmain.bin
// menu (new, load, campaign, high scores, credits, quit), one network
// handshake and the campaign replay/next-scenario loop.
VA(0x0045015c, 0xe22)
int oldmain(void) {
    char saveBuf[20];
    H1_ENUM_STORAGE(SmackVideo, char) hiResVideos[3];
    H1_ENUM_STORAGE(SmackVideo, char) lowResVideos[3];
    int n;
    heroWindow* mainWin;
    font* font;
    signed char backdropLoaded;
    signed char initialMainScreen;
    int idx;
    signed char done;
    signed char leave;
    int result;
    short command;

    if (bKBDone)
        return 0;
    bKBDone = 1;
    command = MAIN_MENU_NO_COMMAND;
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
        FillBitmapArea(
            gpWindowManager->m_screen,
            0,
            0,
            LOGICAL_SCREEN_WIDTH,
            LOGICAL_SCREEN_HEIGHT,
            0
        );
        BlitBitmapToScreen(
            gpWindowManager->m_screen,
            0,
            0,
            LOGICAL_SCREEN_WIDTH,
            LOGICAL_SCREEN_HEIGHT,
            0,
            0
        );
        font = gpResourceManager->GetFont("bigfont.fnt");
        font->DrawString(
            "Loading Heroes of Might and Magic for Windows 95 (version 1.0)",
            10,
            10,
            1
        );
        gpWindowManager->UpdateScreenRegion(10, 10, 600, 20);
        gpResourceManager->Dispose(font);
        if (!gbSkipIntro) {
            if (gConfig.slowVideo)
                PlaySmacker(SMACK_NWCLOGO1);
            else
                PlaySmacker(SMACK_NWCLOGO);
        }
        if (gConfig.slowVideo)
            PlaySmacker(SMACK_INTRO02C);
        else
            PlaySmacker(SMACK_INTRO02U);
    }
    LoadSystemwideIcons();
    memset(gbThisNetHumanPlayer, 0, GAME_PLAYER_COUNT);
    leave = 0;
    backdropLoaded = 0;
    initialMainScreen = 1;

    while (!leave) {
    mainMenu:
        gpSoundManager->SwitchAmbientMusic(MUSIC_TRACK_MAIN_MENU);
        if (!backdropLoaded) {
            if (gGameCommand != MAIN_MENU_QUIT) {
                gpResourceManager->GetBackdrop("heroes.bmp", gpWindowManager->m_screen);
                gpWindowManager
                    ->UpdateScreenRegion(0, 0, LOGICAL_SCREEN_WIDTH, LOGICAL_SCREEN_HEIGHT);
                if (initialMainScreen)
                    SetPalette(gPalette->m_data, 0);
                else
                    gpWindowManager->FadeScreen(WINDOW_FADE_IN, WINDOW_FADE_STEPS_SHORT, gPalette);
                initialMainScreen = 0;
            }
            gpMouseManager->SetPointer("advmice.mse", ADVENTURE_POINTER_DEFAULT);
        }
        backdropLoaded = 1;
        if (gGameCommand != MAIN_MENU_QUIT)
            gpWindowManager->m_updateFlags = 1;
        giCampaignChoice = 0;
        gpMouseManager->ReallyShowPointer();

        if (giMenuCommand != APP_MENU_NONE) {
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
            if (gGameCommand != MAIN_MENU_NO_COMMAND) {
                command = gGameCommand;
                gGameCommand = MAIN_MENU_NO_COMMAND;
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
        if (giMenuCommand != APP_MENU_NONE)
            goto processMenuCommand;

        gpMouseManager->ReallyHidePointer();
        switch (command) {
            case MAIN_MENU_LOAD_GAME:
                if (!gpGame->PickLoadGame())
                    goto mainMenu;
                break;
            case MAIN_MENU_HIGH_SCORES:
                if (gpExec->AddManager(gpHighScoreManager, BASE_MANAGER_PRIORITY_UNASSIGNED))
                    ShutDown("Can't add manager!");
                gpExec->MainLoop();
                gpExec->RemoveManager(gpHighScoreManager);
                backdropLoaded = 0;
                goto mainMenu;
            case MAIN_MENU_NEW_GAME:
                if (!gpGame->NewGame())
                    goto mainMenu;
                break;
            case MAIN_MENU_CREDITS:
                gpWindowManager->FadeScreen(WINDOW_FADE_OUT, WINDOW_FADE_STEPS_SHORT, gPalette);
                gpResourceManager->GetBackdrop("credits.bmp", gpWindowManager->m_screen);
                gpWindowManager
                    ->UpdateScreenRegion(0, 0, LOGICAL_SCREEN_WIDTH, LOGICAL_SCREEN_HEIGHT);
                gpWindowManager->FadeScreen(WINDOW_FADE_IN, WINDOW_FADE_STEPS_SHORT, gPalette);
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
                gpWindowManager->FadeScreen(WINDOW_FADE_OUT, WINDOW_FADE_STEPS_SHORT, gPalette);
                gpResourceManager->GetBackdrop("heroes.bmp", gpWindowManager->m_screen);
                gpWindowManager
                    ->UpdateScreenRegion(0, 0, LOGICAL_SCREEN_WIDTH, LOGICAL_SCREEN_HEIGHT);
                gpWindowManager->FadeScreen(WINDOW_FADE_IN, WINDOW_FADE_STEPS_SHORT, gPalette);
                goto mainMenu;
            case MAIN_MENU_QUIT:
                leave = 1;
                break;
        }

    gameSetupComplete:
        if (giMenuCommand != APP_MENU_NONE)
            goto processMenuCommand;
        if (!leave) {
            if (gbRemoteOn && !giThisNetPos) {
                n = 0;
                for (idx = 0; idx < GAME_PLAYER_COUNT; idx++) {
                    if (gbHumanPlayer[idx]) {
                        gbGamePosToNetPos[idx] = n;
                        n++;
                    } else {
                        gbGamePosToNetPos[idx] = -1;
                    }
                }
                for (idx = 0; idx < GAME_PLAYER_COUNT; idx++)
                    memcpy(gText, gbGamePosToNetPos, GAME_PLAYER_COUNT);
                giHostGamePos = NetPosToGamePos(0);
                giThisGamePos = giHostGamePos;
                for (idx = 1; idx < giNumHumanPlayers; idx++) {
                    result = TransmitRemoteData(
                        gText,
                        idx,
                        4,
                        BOX_REMOTE_SETUP,
                        1,
                        1,
                        REMOTE_MESSAGE_DEFAULT,
                        0
                    );
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
                giWaitType = DIALOG_WAIT_OTHER_PLAYER;
                NormalDialog(
                    "Waiting for other remote player to set up game.",
                    NORMAL_DIALOG_TYPE_WAIT_CANCEL
                );
                if (!gbFunctionComplete)
                    ShutDown(NULL);
                gpGame->LoadGame("REMOTE.GAM", 0, 1);
                goto playScenario;
            }
        playScenario:
            if (gpGame->m_campaignType > 0) {
                if (!backdropLoaded) {
                    gpWindowManager->FadeScreen(WINDOW_FADE_OUT, WINDOW_FADE_STEPS_SHORT, gPalette);
                    gpResourceManager->GetBackdrop("heroes.bmp", gpWindowManager->m_screen);
                    gpWindowManager
                        ->UpdateScreenRegion(0, 0, LOGICAL_SCREEN_WIDTH, LOGICAL_SCREEN_HEIGHT);
                    gpWindowManager->FadeScreen(WINDOW_FADE_IN, WINDOW_FADE_STEPS_SHORT, gPalette);
                    backdropLoaded = 1;
                }
                gpGame->ShowCampaignInfo(gpGame->m_campaignScenario, 0, 0);
            }
            gbGameInitialized = 1;
            backdropLoaded = 0;
            gpSoundManager->StopAllSamples();
            gpWindowManager->FadeScreen(WINDOW_FADE_OUT, WINDOW_FADE_STEPS_SHORT, NULL);
            gMapX = 0;
            gMapY = 0;
            if (gpExec->AddManager(gpAdvManager, BASE_MANAGER_PRIORITY_UNASSIGNED))
                ShutDown("Can't add manager!");
            if (command == MAIN_MENU_NEW_GAME)
                gpAdvManager->SetHeroContext(gpGame->m_players[0].NextHero(0), 0);
            gpExec->MainLoop();
            gMapX = gpAdvManager->m_mapOriginX;
            gMapY = gpAdvManager->m_mapOriginY;
            gpExec->RemoveManager(gpAdvManager);
            gpWindowManager->FadeScreen(WINDOW_FADE_OUT, WINDOW_FADE_STEPS_SHORT, gPalette);
        }

        if (gbGameOver) {
            RemoteCleanup();
            bShowIt = 1;
            gpMouseManager->SetPointer("advmice.mse", ADVENTURE_POINTER_DEFAULT);
            gpMouseManager->ReallyHidePointer();
            sprintf(
                gcWinText,
                "My heroes, our foes have been scattered, their castles broken and laid bare.  "
                "The great campaign is now complete, and I stand before you as the undisputed "
                "High King!\n\nOur victory was achieved in %d days!",
                giCurTurn
            );
            lowResVideos[GAME_END_LOST] = SMACK_LOSE1;
            lowResVideos[GAME_END_WON] = SMACK_WIN01U;
            lowResVideos[GAME_END_CAMPAIGN_COMPLETE] = SMACK_WIN02;
            hiResVideos[GAME_END_LOST] = SMACK_LOSE1;
            hiResVideos[GAME_END_WON] = SMACK_WIN01C;
            hiResVideos[GAME_END_CAMPAIGN_COMPLETE] = SMACK_WIN02;
            if (giEndSequence != GAME_END_WON) {
                if (giEndSequence == GAME_END_CAMPAIGN_COMPLETE) {
                    PlaySmacker(SMACK_WIN01C);
                    PlaySmacker(SMACK_WIN02);
                } else {
                    PlaySmacker(
                        gConfig.slowVideo ? hiResVideos[giEndSequence] : lowResVideos[giEndSequence]
                    );
                }
                gpResourceManager->GetBackdrop("heroes.bmp", gpWindowManager->m_screen);
                gpWindowManager
                    ->UpdateScreenRegion(0, 0, LOGICAL_SCREEN_WIDTH, LOGICAL_SCREEN_HEIGHT);
                gpWindowManager->FadeScreen(WINDOW_FADE_IN, WINDOW_FADE_STEPS_SHORT, gPalette);
                gpWindowManager->m_updateFlags = 1;
                backdropLoaded = 1;
            } else {
                ShowCongrats();
            }
            gbGameOver = 0;
            if (giEndSequence == GAME_END_CAMPAIGN_COMPLETE) {
                gpSoundManager->SwitchAmbientMusic(MUSIC_TRACK_CONGRATULATIONS);
                AddScoreToHighScore(
                    giCurTurn,
                    HIGH_SCORE_TYPE_CAMPAIGN,
                    "",
                    gCampaignSideNames[gpGame->m_campaignType - 1]
                );
            }
            if (gbShowHighScore) {
                gpMouseManager->ReallyShowPointer();
                if (gpExec->AddManager(gpHighScoreManager, BASE_MANAGER_PRIORITY_UNASSIGNED))
                    ShutDown("Can't add manager!");
                gpExec->MainLoop();
                gpExec->RemoveManager(gpHighScoreManager);
                giHighScoreRank = HIGH_SCORE_EMPTY;
                gpSoundManager->SwitchAmbientMusic(MUSIC_TRACK_MAIN_MENU);
                gpResourceManager->GetBackdrop("heroes.bmp", gpWindowManager->m_screen);
                gpWindowManager
                    ->UpdateScreenRegion(0, 0, LOGICAL_SCREEN_WIDTH, LOGICAL_SCREEN_HEIGHT);
                gpWindowManager->FadeScreen(WINDOW_FADE_IN, WINDOW_FADE_STEPS_SHORT, gPalette);
                backdropLoaded = 1;
            }
            if (gpGame->m_campaignType > 0) {
                if (giEndSequence == GAME_END_LOST) {
                    sprintf(gText, "Would you like to replay this scenario?");
                    NormalDialog(gText, NORMAL_DIALOG_TYPE_YES_NO);
                    if (gpWindowManager->m_dialogResult == NORMAL_DIALOG_CONFIRM) {
                        gpGame->InitCampaignMap(gpGame->m_campaignScenario, 0);
                        goto playScenario;
                    }
                } else if (giEndSequence == GAME_END_WON) {
                    gpGame->m_campaignDay = giCurTurn + 1;
                    gpGame->m_campaignScenario++;
                    gpGame->m_campaignScenariosWon++;
                    if (gpGame->m_campaignScenario - 4 == gpGame->m_campaignType - 1)
                        gpGame->m_campaignScenario++;
                    gpGame->InitCampaignMap(gpGame->m_campaignScenario, 0);
                    sprintf(saveBuf, "%s%02d", "SCENWN", gpGame->m_campaignScenariosWon);
                    gpGame->SaveGame(saveBuf, 1);
                    sprintf(
                        gText,
                        "Your campaign has been saved as %s.  Would you like to start the next "
                        "scenario?",
                        saveBuf
                    );
                    NormalDialog(gText, NORMAL_DIALOG_TYPE_YES_NO);
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
    int size;
    int i;
    int helpRequested = 0;

    giDebugLevel = 0;
    giShowIntro = 1;
    gbColorMice = 0;
    gbSpecialMouseMasks = 1;
    giScreenScroll = 1;
    giLimitPlayer = 0;
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

    sprintf(cAggPathName, "%s%s", gcDataPath, "heroes.agg");
    DEFAULT_AGGREGATE_NAME = cAggPathName;
    giFrameStep = 6;
    for (i = 0; i < GAME_PLAYER_COUNT; i++) {
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
            helpIndex = MAIN_MENU_HELP_NONE;
            switch (message.id) {
                case MAIN_MENU_NEW_GAME:
                    helpIndex = MAIN_MENU_HELP_NEW_GAME;
                    break;
                case MAIN_MENU_LOAD_GAME:
                    helpIndex = MAIN_MENU_HELP_LOAD_GAME;
                    break;
                case MAIN_MENU_HIGH_SCORES:
                    helpIndex = MAIN_MENU_HELP_HIGH_SCORES;
                    break;
                case MAIN_MENU_CREDITS:
                    helpIndex = MAIN_MENU_HELP_CREDITS;
                    break;
                case MAIN_MENU_QUIT:
                    helpIndex = MAIN_MENU_HELP_QUIT;
                    break;
            }
            if (helpIndex >= 0)
                NormalDialog(gInitMenuHelp[helpIndex], NORMAL_DIALOG_TYPE_QUICK_VIEW);
        }
    } else if (message.type == MESSAGE_WIDGET) {
        switch (message.command) {
            case WIDGET_NOTIFY_DESELECT:
                if (message.id > 0 && message.id <= MAIN_MENU_LAST)
                    handled = 1;
                break;
            default:
                break;
        }
    }

    if (handled || giMenuCommand != APP_MENU_NONE) {
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
                        gpWindowManager->FadeScreen(WINDOW_FADE_IN, WINDOW_FADE_STEPS_SHORT, NULL);
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
        return gDwellingNames
            [building - BUILDING_SLOT_DWELLING_FIRST + race * BUILDING_SLOT_DWELLING_COUNT];
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
            gDwellingCosts
                [building - BUILDING_SLOT_DWELLING_FIRST + race * BUILDING_SLOT_DWELLING_COUNT],
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
        if (cell->m_tileIndex < MAP_CELL_TILES_PER_TERRAIN)
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
    required = gDwellingRequirements
        [building - BUILDING_SLOT_DWELLING_FIRST + t->m_type * BUILDING_SLOT_DWELLING_COUNT];
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
        (t->m_buildings & (1 << BUILDING_SLOT_MAGE_GUILD))
            ? (t->m_buildState >= 3 ? 3 : t->m_buildState + 1)
            : 0
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
        return gDwellingBaseResourceValues
            [building - BUILDING_SLOT_DWELLING_FIRST + race * BUILDING_SLOT_DWELLING_COUNT];
    }
}

// Buka 2.1 NormalDialog without HoMM2's timeout, saved resource globals,
// primary-skill/monster/secondary-skill slots and centered x; HoMM1 measures
// the text with a temporary bigfont.fnt and frames heroes with port%04d.icn.
VA(0x00451a31, 0xf03)
void NormalDialog(
    char* text,
    H1_ENUM_PARAM(NormalDialogType, int) dialogType,
    int x,
    int y,
    H1_ENUM_PARAM(NormalDialogResourceType, int) firstResourceType,
    int firstResourceValue,
    H1_ENUM_PARAM(NormalDialogResourceType, int) secondResourceType,
    int secondResourceValue,
    H1_ENUM_PARAM(NormalDialogOrText, int) showOrText
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

    if (x == NORMAL_DIALOG_AUTO_POSITION || x + width >= LOGICAL_SCREEN_WIDTH - 1) {
        if (gpAdvManager->m_active == 1 && !gbHeroWindShowing && !gbOverviewShowing)
            x = NORMAL_DIALOG_ADVENTURE_X;
        else
            x = (LOGICAL_SCREEN_WIDTH - width) / 2;
    }
    if (y == NORMAL_DIALOG_AUTO_POSITION || y + height >= LOGICAL_SCREEN_HEIGHT - 1) {
        y = (LOGICAL_SCREEN_HEIGHT - height) / 2;
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
    if (dialogType != NORMAL_DIALOG_TYPE_WAIT_CANCEL
        && dialogType != NORMAL_DIALOG_TYPE_NO_BUTTONS) {
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

        amountText[i] = static_cast<char*>(malloc(NORMAL_DIALOG_TEXT_LENGTH));
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
        } else if (kind[i] == NORMAL_DIALOG_EXPERIENCE || kind[i] == NORMAL_DIALOG_MORALE_BONUS
                   || kind[i] == NORMAL_DIALOG_MORALE_PENALTY || kind[i] == NORMAL_DIALOG_LUCK_BONUS
                   || kind[i] == NORMAL_DIALOG_LUCK_PENALTY) {
            strcpy(amountText[i], "");
            strcpy(szFilename, "expmrl.icn");
            resourceFrame = kind[i] - NORMAL_DIALOG_EXPMRL_FIRST;
            if (kind[i] == NORMAL_DIALOG_EXPERIENCE && resourceQty[i] != NORMAL_DIALOG_NO_VALUE)
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
            resCenterX - resWidth / 2,
            resourceYPos,
            resWidth,
            sizingHeight,
            szFilename,
            resourceFrame,
            ICON_DRAW_NORMAL,
            -1,
            ICON_WIDGET_DRAW,
            1
        );
        if (!iconPanel)
            MemError();
        pNormalDialogWindow->AddWidget(iconPanel, WINDOW_Z_ORDER_APPEND);
        if (kind[i] == NORMAL_DIALOG_ARTIFACT) {
            iconPanel = new iconWidget(
                resCenterX - resWidth / 2 + 6,
                resourceYPos + 6,
                76,
                76,
                "artifact.icn",
                resourceQty[i],
                ICON_DRAW_NORMAL,
                -1,
                ICON_WIDGET_DRAW,
                1
            );
            if (!iconPanel)
                MemError();
            pNormalDialogWindow->AddWidget(iconPanel, WINDOW_Z_ORDER_APPEND);
        }
        if (kind[i] == NORMAL_DIALOG_CREST) {
            iconPanel = new iconWidget(
                resCenterX - resWidth / 2 - 4,
                resourceYPos - 4,
                58,
                55,
                "brcrest.icn",
                4,
                ICON_DRAW_NORMAL,
                -1,
                ICON_WIDGET_DRAW,
                1
            );
            if (!iconPanel)
                MemError();
            pNormalDialogWindow->AddWidget(iconPanel, WINDOW_Z_ORDER_APPEND);
        }
        if (kind[i] == NORMAL_DIALOG_HERO) {
            sprintf(szFilename, "port%04d.icn", resourceQty[i]);
            iconPanel = new iconWidget(
                resCenterX - resWidth / 2 + 5,
                resourceYPos + 5,
                101,
                95,
                szFilename,
                0,
                ICON_DRAW_NORMAL,
                -1,
                ICON_WIDGET_DRAW,
                1
            );
            if (!iconPanel)
                MemError();
            pNormalDialogWindow->AddWidget(iconPanel, WINDOW_Z_ORDER_APPEND);
        }
        captionWidget = new textWidget(
            resCenterX - 50,
            resourceYPos + sizingHeight - 10,
            100,
            12,
            amountText[i],
            "smalfont.fnt",
            1,
            id++,
            WIDGET_KIND_TEXT
        );
        if (!captionWidget)
            MemError();
        pNormalDialogWindow->AddWidget(captionWidget, WINDOW_Z_ORDER_APPEND);
    }

    message.type = MESSAGE_WIDGET;
    message.command = WIDGET_COMMAND_SET_TEXT;
    message.id = NORMAL_DIALOG_TEXT_WIDGET_ID;
    message.text = text;
    pNormalDialogWindow->BroadcastMessage(message);

    if (showOrText == NORMAL_DIALOG_SHOW_OR_TEXT) {
        szOr = static_cast<char*>(malloc(3));
        strcpy(szOr, "or");
        captionWidget = new textWidget(
            width / 2 - 17,
            resourceYPos + 30,
            40,
            12,
            szOr,
            "smalfont.fnt",
            1,
            id++,
            WIDGET_KIND_TEXT
        );
        if (!captionWidget)
            MemError();
        pNormalDialogWindow->AddWidget(captionWidget, WINDOW_Z_ORDER_APPEND);
    }

    if (gpAdvManager->m_active == 1)
        gpMouseManager->SetPointer(ADVENTURE_POINTER_DEFAULT);
    else if (gpCombatManager->m_active == 1)
        gpMouseManager->SetPointer(COMBAT_POINTER_DEFAULT);

    if (dialogType == NORMAL_DIALOG_TYPE_WAIT_CANCEL || dialogType == NORMAL_DIALOG_TYPE_WAIT_OK) {
        gpWindowManager->DoDialog(pNormalDialogWindow, WaitHandler, 0);
    } else if (dialogType == NORMAL_DIALOG_TYPE_QUICK_VIEW) {
        gpMouseManager->ReallyHidePointer();
        gpWindowManager->AddWindow(pNormalDialogWindow, WINDOW_Z_ORDER_APPEND, 1);
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
        message.id = NORMAL_DIALOG_TEXT_WIDGET_ID;
        message.text = text;
        pNormalDialogWindow->BroadcastMessage(message);
        pNormalDialogWindow->DrawWindow(0, 0, NORMAL_DIALOG_FOREGROUND_WIDGET_LIMIT);
        pNormalDialogWindow
            ->DrawWindow(1, WINDOW_ALL_WIDGETS_LOW, NORMAL_DIALOG_BACKGROUND_WIDGET_LAST_ID);
    }
}

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
            case DIALOG_WAIT_OTHER_PLAYER:
                result = WaitForOtherPlayer();
                break;
            case DIALOG_WAIT_NETBIOS_HOST:
                result = WaitForHost();
                break;
            case DIALOG_WAIT_NETBIOS_GUEST:
                result = WaitForGuest();
                break;
            case DIALOG_WAIT_NETBIOS_INIT_GUEST:
                result = InitNetGuest();
                break;
            case DIALOG_WAIT_NETBIOS_INIT_HOST:
                result = InitNetHost();
                break;
            case DIALOG_WAIT_MODEM_COMMAND:
                result = GUIModemCommandExec();
                break;
            case DIALOG_WAIT_MODEM_RESPONSE:
                result = GUIModemResponseExec();
                break;
            case DIALOG_WAIT_DIRECT_CONNECT:
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
                        message.command = message.id = WIDGET_COMMAND_DIALOG_SELECT;
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

DATA(0x00491058)
char* cCombatGroundNames[8] = {
    "boat.xtl",
    "grass.xtl",
    "snow.xtl",
    "swamp.xtl",
    "lava.xtl",
    "desert.xtl",
    "dgrass.xtl",
    0,
};
DATA(0x00491078)
char* cCombatObstacleNames[8] = {
    "boat.obj",
    "grass.obj",
    "snow.obj",
    "swamp.obj",
    "lava.obj",
    "desert.obj",
    "dgrass.obj",
    0,
};
DATA(0x00491098)
char* gPowEffectNames[16] = {
    "cloud.icn",
    "physical.icn",
    "physical.icn",
    "physical.icn",
    "physical.icn",
    "physical.icn",
    "physical.icn",
    "redfire.icn",
    "electric.icn",
    "redfire.icn",
    "electric.icn",
    "redfire.icn",
    "bluefire.icn",
    "bluefire.icn",
    "cloud.icn",
    "cloud.icn",
};
DATA(0x004910d8)
char* gCombatFxNames[26] = {
    "redfire.icn", "elecfire.icn", "magic04.icn", "magic01.icn", "magic01.icn",  "magic02.icn",
    "magic02.icn", "magic06.icn",  "magic07.icn", "magic01.icn", "magic06.icn",  "magic08.icn",
    "magic07.icn", "magic01.icn",  "magic01.icn", "magic02.icn", "reddeath.icn", "magic03.icn",
    "magic03.icn", "magic06.icn",  "magic01.icn", "magic01.icn", "rainbluk.icn", "cloudluk.icn",
    "moraleg.icn", "moraleb.icn",
};
DATA(0x00491140)
short giSpellAIValue[29] = {
    500,  350,  300, 400, 550, 900, 400, 500, 300, 350, 250, 0, 100,  150, 1000,
    2000, 1700, 700, 700, 0,   0,   0,   0,   0,   0,   0,   0, 1200, 0,
};
DATA(0x00491180)
signed char gcSpellAIFlags[29] = {
    3, 3, 2, 2, 3, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 3, 3, 3, 2, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
};
DATA(0x004911a0)
signed char gMageGuildSpellPool[4][8] = {
    {9, 13, 6, 8, 10, 20, 19, 8},
    {1, 5, 3, 7, 11, 21, 26, 12},
    {0, 18, 14, 16, 22, 25, 23, 2},
    {27, 4, 15, 17, 28, 24, 28, 27},
};
DATA(0x004911c0)
signed char gCombatAdjacency[45][6] = {
    {-1, -1, -1, -1, -1, -1}, {-1, 2, 10, -1, -1, -1},  {-1, 3, 11, 10, 1, -1},
    {-1, 4, 12, 11, 2, -1},   {-1, 5, 13, 12, 3, -1},   {-1, 6, 14, 13, 4, -1},
    {-1, 7, 15, 14, 5, -1},   {-1, -1, 16, 15, 6, -1},  {-1, -1, -1, -1, -1, -1},
    {-1, -1, -1, -1, -1, -1}, {2, 11, 20, 19, -1, 1},   {3, 12, 21, 20, 10, 2},
    {4, 13, 22, 21, 11, 3},   {5, 14, 23, 22, 12, 4},   {6, 15, 24, 23, 13, 5},
    {7, 16, 25, 24, 14, 6},   {-1, -1, -1, 25, 15, 7},  {-1, -1, -1, -1, -1, -1},
    {-1, -1, -1, -1, -1, -1}, {10, 20, 28, -1, -1, -1}, {11, 21, 29, 28, 19, 10},
    {12, 22, 30, 29, 20, 11}, {13, 23, 31, 30, 21, 12}, {14, 24, 32, 31, 22, 13},
    {15, 25, 33, 32, 23, 14}, {16, -1, 34, 33, 24, 15}, {-1, -1, -1, -1, -1, -1},
    {-1, -1, -1, -1, -1, -1}, {20, 29, 38, 37, -1, 19}, {21, 30, 39, 38, 28, 20},
    {22, 31, 40, 39, 29, 21}, {23, 32, 41, 40, 30, 22}, {24, 33, 42, 41, 31, 23},
    {25, 34, 43, 42, 32, 24}, {-1, -1, -1, 43, 33, 25}, {-1, -1, -1, -1, -1, -1},
    {-1, -1, -1, -1, -1, -1}, {28, 38, -1, -1, -1, -1}, {29, 39, -1, -1, 37, 28},
    {30, 40, -1, -1, 38, 29}, {31, 41, -1, -1, 39, 30}, {32, 42, -1, -1, 40, 31},
    {33, 43, -1, -1, 41, 32}, {34, -1, -1, -1, 42, 33}, {-1, -1, -1, -1, -1, -1},
};
DATA(0x004912d0)
short horseFrameFlip[16] = {45, 46, 47, 48, 49, 50, 51, 52, 53, 179, 178, 177, 54, 175, 174, 55};
DATA(0x004912f0)
short boatFrameFlip[16] = {0, 0, 9, 9, 18, 18, 27, 27, 36, 36, 155, 155, 146, 146, 137, 137};
DATA(0x00491310)
short gRadarOwnerColor[8] = {79, 105, 200, 129, 10, 0, 0, 0};
DATA(0x00491320)
short gRadarTerrainColor[24] = {
    82,  99, 7,   180, 26,  123, 55, 0,  16, 48, 98, 160,
    126, 74, 110, 179, 100, 218, 12, 12, 12, 12, 12, 12,
};
DATA(0x00491350)
char* cTownObjectNames[20] = {
    "magegld", "thievesg", "tavern", "dock", "well", "farm", "frst", "plns", "mtn", "tent",
    "cast",    "_d0",      "_d1",    "_d2",  "_d3",  "_d4",  "_d5",  "_e0",  "_e1", "_e2",
};
DATA(0x004913a0)
signed char gDwellingType[4][6] = {
    {0, 1, 2, 3, 4, 5},
    {12, 13, 14, 15, 16, 17},
    {6, 7, 8, 9, 10, 11},
    {18, 19, 20, 21, 22, 23},
};
DATA(0x004913b8)
int gMageBuildingCosts[4][7] = {
    {5, 0, 5, 0, 0, 0, 2000},
    {5, 4, 5, 4, 4, 4, 1000},
    {5, 6, 5, 6, 6, 6, 1000},
    {5, 10, 5, 10, 10, 10, 1000},
};
DATA(0x00491428)
int gNeutralBuildingCosts[7][7] = {
    {5, 0, 5, 0, 0, 0, 2000},
    {5, 0, 0, 0, 0, 0, 750},
    {5, 0, 0, 0, 0, 0, 500},
    {20, 0, 0, 0, 0, 0, 2000},
    {0, 0, 0, 0, 0, 0, 500},
    {5, 0, 5, 0, 0, 0, 2000},
    {20, 0, 20, 0, 0, 0, 5000},
};
DATA(0x004914f0)
int gMageBaseResourceValues[4] = {4000, 6500, 8500, 10500};
DATA(0x00491500)
int gNeutralBaseResourceValues[7] = {5000, 1500, 500, 2000, 3000, 0, 12000};
DATA(0x00491520)
int gDwellingBaseResourceValues[24] = {
    858,  2225, 2816, 7385, 13754, 29785, 1684, 2256, 3736, 7213, 15181, 27684,
    1802, 2615, 3414, 6967, 12212, 38141, 1956, 2607, 3869, 7510, 16002, 111967,
};
DATA(0x00491580)
int gDwellingCosts[24][7] = {
    {0, 0, 0, 0, 0, 0, 200},    {0, 0, 0, 0, 0, 0, 1000},   {0, 0, 5, 0, 0, 0, 1000},
    {10, 0, 10, 0, 0, 0, 2000}, {20, 0, 0, 0, 0, 0, 3000},  {20, 0, 0, 0, 20, 0, 5000},
    {5, 0, 0, 0, 0, 0, 500},    {5, 0, 0, 0, 0, 0, 1000},   {0, 0, 0, 0, 0, 0, 1500},
    {0, 10, 10, 0, 0, 0, 2500}, {10, 0, 0, 0, 0, 10, 3000}, {0, 20, 30, 0, 0, 0, 10000},
    {0, 0, 0, 0, 0, 0, 300},    {5, 0, 0, 0, 0, 0, 800},    {0, 0, 0, 0, 0, 0, 1000},
    {10, 0, 10, 0, 0, 0, 2000}, {0, 0, 20, 0, 0, 0, 4000},  {0, 0, 20, 0, 20, 0, 6000},
    {0, 0, 0, 0, 0, 0, 500},    {0, 0, 10, 0, 0, 0, 1000},  {0, 0, 0, 0, 0, 0, 2000},
    {0, 0, 0, 0, 0, 10, 3000},  {0, 0, 0, 10, 0, 0, 4000},  {0, 0, 30, 20, 0, 0, 15000},
};
DATA(0x00491820)
signed char gCastleResources[4] = {0, 2, -1, -1};

// Buka 2.1 HandleRemoteDeadPlayerExit for HoMM1's two-player transport.
VA(0x00452e00, 0x99)
void HandleRemoteDeadPlayerExit(int position) {
    if (position == giThisGamePos) {
        if (!gpGame->TransmitSaveGame(REMOTE_BROADCAST_PLAYER, 1))
            ShutDown(NULL);
        RemoteCleanup();
    } else if (giNumHumanPlayers == 2) {
        giNumHumanPlayers--;
        gText[0] = position;
        gText[1] = 0;
        TransmitRemoteData(
            gText,
            REMOTE_BROADCAST_PLAYER,
            3,
            REMOTE_COMMAND_PLAYER_EXIT,
            0,
            0,
            REMOTE_MESSAGE_RELIABLE,
            1
        );
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
    gText[0] = giThisGamePos;
    if (gbThisNetHumanPlayer[giCurPlayer]
        || (!gbHumanPlayer[giCurPlayer] && giHostGamePos == giThisGamePos)) {
        gText[1] = 1;
        next = giCurPlayer;
        next = (next + 1) % gpGame->m_playerCount;
        while (!gbHumanPlayer[next])
            next = (next + 1) % gpGame->m_playerCount;
        gText[2] = next;
    } else {
        gText[1] = 0;
    }
    TransmitRemoteData(
        gText,
        REMOTE_BROADCAST_PLAYER,
        3,
        REMOTE_COMMAND_PLAYER_EXIT,
        0,
        0,
        REMOTE_MESSAGE_RELIABLE,
        1
    );
}

// donor PoL RVA 0x000a07e3; preferred Buka symbol ?ReceiveRemotePlayerExit@@YIXUSPlayerExit@@@Z
// donor Buka TU SOURCE/KB; HoMM1 owner inferred from contiguous order
// evidence: graph:4;base=0.368727;margin=0.249960;shape=0.192;size=0.687;calls=0.800;alternate=pol20:void ReceiveRemotePlayerExit(struct SPlayerExit)@0x000a07e3

VA(0x00452f8a, 0x1ea)
// HoMM1 callers push four byte-sized values: player, an unused flag,
// elimination and timeout.
void ReceiveRemotePlayerExit(
    signed char position,
    signed char,
    signed char eliminated,
    signed char timedOut
) {
    if (position == giThisGamePos) {
        sprintf(gText, "You have been eliminated from the game!!!");
        NormalDialog(gText, NORMAL_DIALOG_TYPE_OK);
        RemoteCleanup();
        gbGameOver = 1;
        giEndSequence = GAME_END_LOST;
        return;
    }
    if (giNumHumanPlayers <= 2) {
        gpGame->SaveGame("PLYREXIT", 1);
        if (eliminated) {
            sprintf(
                gText,
                "%s player has been vanquished!",
                gColorNames[gpGame->m_players[position].Color()]
            );
            gText[0] -= 32;
            NormalDialog(
                gText,
                NORMAL_DIALOG_TYPE_OK,
                0x61,
                NORMAL_DIALOG_AUTO_POSITION,
                NORMAL_DIALOG_CREST,
                gpGame->m_players[position].Color()
            );
            goto dropPlayer;
        } else {
            if (timedOut)
                sprintf(
                    gText,
                    "Player %d has been logged out of the game.  The current game has been saved "
                    "as "
                    "'PLYREXIT'.  Do you wish to continue playing with a computer player filling "
                    "in for "
                    "player %d?",
                    position + 1,
                    position + 1
                );
            else
                sprintf(
                    gText,
                    "Player %d is exiting the game.  The current game has been saved as "
                    "'PLYREXIT'.  Do "
                    "you wish to continue playing with a computer player filling in for player %d?",
                    position + 1,
                    position + 1
                );
            NormalDialog(gText, NORMAL_DIALOG_TYPE_YES_NO);
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
    signed char lost;
    signed char normalWin;
    int lastSurvivor;
    int player;
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
                    gText,
                    "%s player has been vanquished!",
                    gColorNames[gpGame->m_players[static_cast<signed char>(player)].Color()]
                );
                gText[0] -= 32;
                NormalDialog(
                    gText,
                    NORMAL_DIALOG_TYPE_OK,
                    0x61,
                    NORMAL_DIALOG_AUTO_POSITION,
                    NORMAL_DIALOG_CREST,
                    gpGame->m_players[static_cast<signed char>(player)].Color()
                );
            } else if (!pd->m_townCount) {
                if (pd->m_daysLeft == -1) {
                    if (gbThisNetHumanPlayer[player]) {
                        sprintf(
                            gText,
                            "%s player, you have lost your last town.  If you do not conquer "
                            "another town in the next week, you will be eliminated.",
                            gColorNames[gpGame->m_players[static_cast<signed char>(player)].Color()]
                        );
                        gText[0] -= 32;
                        NormalDialog(
                            gText,
                            NORMAL_DIALOG_TYPE_OK,
                            NORMAL_DIALOG_AUTO_POSITION,
                            NORMAL_DIALOG_AUTO_POSITION,
                            NORMAL_DIALOG_CREST,
                            gpGame->m_players[static_cast<signed char>(player)].Color()
                        );
                    }
                    pd->m_daysLeft = 7;
                } else if (!pd->m_daysLeft) {
                    PlayerDead(player);
                    if (gbThisNetHumanPlayer[player]) {
                        sprintf(
                            gText,
                            "%s player, your heroes abandon you, and you are banished from this "
                            "land.",
                            gColorNames[gpGame->m_players[static_cast<signed char>(player)].Color()]
                        );
                        gText[0] -= 32;
                    } else {
                        sprintf(
                            gText,
                            "%s player's Heroes have abandoned him, and he is banished from this "
                            "land.",
                            gColorNames[gpGame->m_players[static_cast<signed char>(player)].Color()]
                        );
                        gText[0] -= 32;
                    }
                    NormalDialog(
                        gText,
                        NORMAL_DIALOG_TYPE_OK,
                        0x61,
                        NORMAL_DIALOG_AUTO_POSITION,
                        NORMAL_DIALOG_CREST,
                        gpGame->m_players[static_cast<signed char>(player)].Color()
                    );
                }
            } else {
                pd->m_daysLeft = -1;
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
                    gCampaignScenarios[gpGame->m_campaignScenario].victoryTownY
                ));
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
                            if (artifactHero->HasArtifact(ARTIFACT_ULTIMATE_BOOK)
                                || artifactHero->HasArtifact(ARTIFACT_ULTIMATE_SWORD)
                                || artifactHero->HasArtifact(ARTIFACT_ULTIMATE_CLOAK)
                                || artifactHero->HasArtifact(ARTIFACT_ULTIMATE_WAND))
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
        giEndSequence = GAME_END_LOST;
    }
    if (win) {
        gbGameOver = 1;
        giEndSequence = GAME_END_WON;
    }
    if (numLiving == 1 || humansAlive == 0
        || (humansAlive == 1 && !gbThisNetHumanPlayer[lastHumanPos])) {
        if (humansAlive == 1 && gbThisNetHumanPlayer[lastHumanPos]) {
            if (normalWin) {
                gbGameOver = 1;
                giEndSequence = GAME_END_WON;
            }
        } else {
            gbGameOver = 1;
            giEndSequence = GAME_END_LOST;
        }
    }
    if (forced) {
        gbGameOver = 1;
        giEndSequence = GAME_END_WON;
    }
    if (gbGameOver && gpGame->m_campaignType > 0 && giEndSequence == GAME_END_WON
        && gpGame->m_campaignScenario + 1 == 9)
        giEndSequence = GAME_END_CAMPAIGN_COMPLETE;
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
    iMaxMapExtra = MAP_EXTRA_FIRST_RECORD;
    gGameCommand = MAIN_MENU_NO_COMMAND;
    gPalette = NULL;
    gpPhilAI->m_debugFont = NULL;
    gbCombatSurrender = 0;
    gpGame->m_viewArmyResult = 0;
    gbInNewGameSetup = 0;
    for (i = 0; i < MAP_CELL_GROUND_TILE_COUNT; i++)
        giGroundToTerrain[i] = i / MAP_CELL_TILES_PER_TERRAIN;
    for (i = 0; i < FINDPATH_TERRAIN_COUNT; i++) {
        giTerrainCost[i][0] = TerrainStepCost(i, 0);
        giTerrainCost[i][1] = TerrainStepCost(i, 1);
    }
    strcpy(cNetBoxLine[0], "");
    strcpy(cNetBoxLine[1], "");
    for (i = 0; i < MAP_EXTRA_RECORD_CAPACITY; i++)
        ppMapExtra[i] = NULL;
    hmnuDflt = LoadMenuA((HINSTANCE)hInstApp, "mnuDflt");
    hmnuCmbt = LoadMenuA((HINSTANCE)hInstApp, "mnuCmbt");
    hmnuAdv = LoadMenuA((HINSTANCE)hInstApp, "mnuAdv");
    hmnuTown = LoadMenuA((HINSTANCE)hInstApp, "mnuTown");
    LogStr(
        "LoadMenus",
        reinterpret_cast<long>(hmnuDflt),
        reinterpret_cast<long>(hmnuCmbt),
        reinterpret_cast<long>(hmnuAdv),
        reinterpret_cast<long>(hmnuTown),
        reinterpret_cast<long>(hInstApp)
    ); // API-forced: LogStr logs handles as long.
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
    if (!h->m_heroClass)
        strcat(gText, gMoraleInfoText[MORALE_INFO_KNIGHT]);
    alignments = h->m_army.IsHomogeneous(-1);
    if (alignments > 0) {
        faction = 0;
        for (i = 0; i < ARMY_GROUP_SLOT_COUNT; i++) {
            if (h->m_army.m_creatureTypes[i] != CREATURE_NONE)
                faction = h->m_army.m_creatureTypes[i] / CREATURE_FACTION_SIZE;
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
    if (h->m_eventFlags & HERO_EVENT_BUOY)
        strcat(gText, gMoraleInfoText[MORALE_INFO_BUOY]);
    if (h->m_eventFlags & HERO_EVENT_OASIS)
        strcat(gText, gMoraleInfoText[MORALE_INFO_OASIS]);
    if (h->m_eventFlags & HERO_EVENT_STATUE)
        strcat(gText, gMoraleInfoText[MORALE_INFO_STATUE]);
    if (h->m_eventFlags & HERO_EVENT_GRAVEYARD)
        strcat(gText, gMoraleInfoText[MORALE_INFO_GRAVEYARD]);
    if (h->m_eventFlags & HERO_EVENT_SHIPWRECK)
        strcat(gText, gMoraleInfoText[MORALE_INFO_SHIPWRECK]);
    if (h->m_cowardice) {
        sprintf(buffer, gMoraleInfoText[MORALE_INFO_COWARDICE], h->m_cowardice);
        strcat(gText, buffer);
    }
    if (strlen(gText) == baseLen)
        strcat(gText, gMoraleInfoText[MORALE_INFO_NONE]);
    NormalDialog(gText, dialogType);
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
    if (h->m_eventFlags & HERO_EVENT_FAERIE_RING)
        strcat(gText, gLuckInfoText[LUCK_INFO_FAERIE_RING]);
    if (h->m_eventFlags & HERO_EVENT_FOUNTAIN)
        strcat(gText, gLuckInfoText[LUCK_INFO_FOUNTAIN]);
    if (strlen(gText) == baseLen)
        strcat(gText, gLuckInfoText[LUCK_INFO_NONE]);
    NormalDialog(gText, dialogType);
}

VA(0x004541ef, 0x70)
void ClearMapExtra(void) {
    int i;
    for (i = 0; i < MAP_EXTRA_RECORD_CAPACITY; i++) {
        if (ppMapExtra[i]) {
            free(ppMapExtra[i]);
            ppMapExtra[i] = NULL;
        }
    }
    iMaxMapExtra = MAP_EXTRA_FIRST_RECORD;
}

// HoMM1 score-to-monster tables pair a threshold word with a monster word.
VA(0x0045425f, 0x8e)
short GetMonType(int score, int highScoreType) {
    int index;
    for (index = 27; index >= 0; index--) {
        if (highScoreType == HIGH_SCORE_TYPE_CAMPAIGN) {
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
    HighScoreEntry scores[HIGH_SCORE_DISPLAY_ENTRY_COUNT];
    int entry;
    int dest;
    int file;
    char fileName[352];
    char enteredPlayerName[20];
    signed char missingFile;

    missingFile = 0;
    if (standard == HIGH_SCORE_TYPE_STANDARD)
        sprintf(fileName, "%sSTANDARD.HS", gcDataPath);
    else
        sprintf(fileName, "%sCAMPAIGN.HS", gcDataPath);
    file = open(fileName, _O_BINARY);
    if (file == -1)
        missingFile = 1;
    if (missingFile) {
        for (entry = 0; entry < HIGH_SCORE_DISPLAY_ENTRY_COUNT; entry++) {
            memset(&scores[entry], 0, sizeof(HighScoreEntry));
            scores[entry].score = HIGH_SCORE_EMPTY;
        }
    } else {
        for (entry = 0; entry < HIGH_SCORE_DISPLAY_ENTRY_COUNT; entry++)
            read(file, &scores[entry], sizeof(scores));
        close(file);
    }

    gbShowHighScore = 1;
    giHighScoreType = standard;
    giHighScoreRank = HIGH_SCORE_EMPTY;
    giScore = score;
    for (entry = 0; entry < HIGH_SCORE_DISPLAY_ENTRY_COUNT; entry++) {
        if ((score >= scores[entry].score && standard == HIGH_SCORE_TYPE_STANDARD)
            || (score <= scores[entry].score && standard == HIGH_SCORE_TYPE_CAMPAIGN)
            || scores[entry].score == HIGH_SCORE_EMPTY) {
            giHighScoreRank = entry;
            break;
        }
    }

    if (entry < HIGH_SCORE_DISPLAY_ENTRY_COUNT) {
        for (dest = HIGH_SCORE_DISPLAY_ENTRY_COUNT - 2; dest >= entry; dest--)
            scores[dest + 1] = scores[dest];
        GetDataEntry(
            "Please enter your name for the high score list.",
            enteredPlayerName,
            16,
            NULL
        );
        strcpy(scores[entry].playerName, enteredPlayerName);
        strcpy(scores[entry].scenarioName, scenarioName);
        scores[entry].score = score;
        file = open(fileName, _O_BINARY | _O_TRUNC | _O_CREAT | _O_WRONLY, _S_IWRITE);
        if (file == -1)
            FileError(fileName);
        for (entry = 0; entry < HIGH_SCORE_DISPLAY_ENTRY_COUNT; entry++)
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
    giBottomViewOverride = BOTTOM_VIEW_RESOURCE;
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
    data = reinterpret_cast<RemoteMessage*>(GetRemoteData(1)); // API-forced: char* record.
    if (data && data->type == REMOTE_MESSAGE_RELIABLE) {
        switch (data->command) {
            case BOX_REMOTE_SETUP:
                memcpy(gbGamePosToNetPos, data->payload.data, GAME_PLAYER_COUNT);
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

// clang-format off
// netbox.bin text widgets: the two scrolled chat lines (cNetBoxLine) and the
// line being typed.
H1_ENUM_BEGIN(NetBoxControl)
    NET_BOX_LINE_PREVIOUS = 1,
    NET_BOX_LINE_LATEST = 2,
    NET_BOX_INPUT = 3
H1_ENUM_END(NetBoxControl)
// clang-format on

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
    message.id = NET_BOX_LINE_PREVIOUS;
    message.text = cNetBoxLine[0];
    netWin->BroadcastMessage(message);
    message.id = NET_BOX_LINE_LATEST;
    message.text = cNetBoxLine[1];
    netWin->BroadcastMessage(message);
    gpWindowManager->AddWindow(netWin, WINDOW_Z_ORDER_APPEND, 1);
    gpMouseManager->ReallyHidePointer();
    exitForIncomingData = 0;
    bClose = 0;
    updateInput = 1;
    blinkState = 0;
    sendText = 0;
    drawLines = 1;
    strcpy(text, "");
    gpInputManager->SetKeyCodeType(INPUT_KEY_CODE_ASCII);

    while (!bClose) {
        PollSound();
        data = GetRemoteData(0);
        if (data) {
            // API-forced: GetRemoteData returns queue records as char*.
            if (reinterpret_cast<RemoteMessage*>(data)->type != REMOTE_MESSAGE_RELIABLE) {
                data = GetRemoteData(1);
            } else {
                switch (
                    reinterpret_cast<RemoteMessage*>(data)->command
                ) { // API-forced: char* record.
                    case REMOTE_COMMAND_CHAT:
                        data = GetRemoteData(1);
                        AddNetBoxLine(
                            reinterpret_cast<RemoteMessage*>(data)->payload.data
                        ); // API-forced: char* record.
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
                    case INPUT_ASCII_ESCAPE:
                    case INPUT_SCAN_F1 << INPUT_KEY_SCAN_SHIFT:
                        bClose = 1;
                        break;
                    case INPUT_ASCII_DELETE:
                        if (len > 0)
                            len--;
                        updateInput = 1;
                        blinkState = 1;
                        break;
                    case '\n':
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
            success = TransmitRemoteData(
                text,
                REMOTE_BROADCAST_PLAYER,
                strlen(text) + 1,
                REMOTE_COMMAND_CHAT,
                1,
                1,
                REMOTE_MESSAGE_DEFAULT,
                1
            );
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
            message.id = NET_BOX_LINE_PREVIOUS;
            message.text = cNetBoxLine[0];
            netWin->BroadcastMessage(message);
            message.id = NET_BOX_LINE_LATEST;
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
            message.id = NET_BOX_INPUT;
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
    gpInputManager->SetKeyCodeType(INPUT_KEY_CODE_SCAN);
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

// clang-format off
// congspre.bin / congrats.bin text widgets: the title (or the campaign's win
// text), the five gScoreLabels captions, and the standard game's days, base
// score, difficulty, final score and creature rating.
H1_ENUM_BEGIN(CongratsControl)
    CONGRATS_TITLE = 100,
    CONGRATS_SCORE_LABEL_FIRST = 101,
    CONGRATS_DAYS = 106,
    CONGRATS_BASE_SCORE = 107,
    CONGRATS_DIFFICULTY = 108,
    CONGRATS_FINAL_SCORE = 109,
    CONGRATS_RATING = 110
H1_ENUM_END(CongratsControl)

H1_ENUM_CONST_BEGIN(CongratsConstant)
    CONGRATS_SCORE_LABEL_COUNT = 5
H1_ENUM_CONST_END(CongratsConstant)
    // clang-format on

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
    gpSoundManager->SwitchAmbientMusic(MUSIC_TRACK_CONGRATULATIONS);
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
        message.id = CONGRATS_TITLE;
        win->BroadcastMessage(message);
    } else {
        win = new heroWindow(0, 0, "congspre.bin");
        if (!win)
            MemError();
        sprintf(name, gArmyNames[GetMonType(result, HIGH_SCORE_TYPE_STANDARD)]);
        name[0] -= 32;
        sprintf(gText, "A Glorious Victory!");
        message.id = CONGRATS_TITLE;
        win->BroadcastMessage(message);
        for (i = 0; i < CONGRATS_SCORE_LABEL_COUNT; i++) {
            sprintf(gText, gScoreLabels[i]);
            message.id = i + CONGRATS_SCORE_LABEL_FIRST;
            win->BroadcastMessage(message);
        }
        sprintf(gText, "%d", giCurTurn);
        message.id = CONGRATS_DAYS;
        win->BroadcastMessage(message);
        sprintf(gText, "%d", daysScore);
        message.id = CONGRATS_BASE_SCORE;
        win->BroadcastMessage(message);
        sprintf(gText, "%d%%", gpGame->m_difficultyRating);
        message.id = CONGRATS_DIFFICULTY;
        win->BroadcastMessage(message);
        sprintf(gText, "%d", result);
        message.id = CONGRATS_FINAL_SCORE;
        win->BroadcastMessage(message);
        sprintf(gText, "%s", name);
        message.id = CONGRATS_RATING;
        win->BroadcastMessage(message);
    }
    gpWindowManager->AddWindow(win, WINDOW_Z_ORDER_APPEND, 1);
    gpMouseManager->ReallyHidePointer();
    gpWindowManager->FadeScreen(WINDOW_FADE_IN, WINDOW_FADE_STEPS_SHORT, NULL);
    CongratsWait();
    gpWindowManager->RemoveWindow(win);
    delete win;
    if (gpGame->m_campaignType <= 0)
        AddScoreToHighScore(result, HIGH_SCORE_TYPE_STANDARD, "", gpGame->m_mapName);
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

// clang-format off
// dataentr.bin widgets: the prompt text and the edit field.
H1_ENUM_BEGIN(DataEntryControl)
    DATA_ENTRY_PROMPT = 1,
    DATA_ENTRY_TEXT = 10
H1_ENUM_END(DataEntryControl)
    // clang-format on

// Buka 2.1 GetDataEntry without the prompt-sized window and textEntryWidget.
VA(0x00455592, 0x1c7)
void GetDataEntry(char* prompt, char* destination, int maximumLength, char* initialText) {
    short widgetId = DATA_ENTRY_TEXT;
    tag_message message;
    char textBuffer[100];

    gpMouseManager->SetPointer("advmice.mse", ADVENTURE_POINTER_DEFAULT);
    cDEDest = destination;
    iDEMaxLen = maximumLength;
    strcpy(cDEDest, "");
    DataEntryWin = new heroWindow(0xb1, 0x14, "dataentr.bin");
    if (!DataEntryWin)
        MemError();
    message.type = MESSAGE_WIDGET;
    message.command = WIDGET_COMMAND_SET_TEXT;
    message.id = DATA_ENTRY_PROMPT;
    message.text = prompt;
    DataEntryWin->BroadcastMessage(message);
    if (initialText)
        strcpy(textBuffer, initialText);
    else
        strcpy(textBuffer, "");
    message.id = DATA_ENTRY_TEXT;
    message.text = textBuffer;
    DataEntryWin->BroadcastMessage(message);
    strcpy(destination, textBuffer);
    bDataEntryTime = 0;
    gpWindowManager->DoDialog(DataEntryWin, DataEntryWindowHandler, 0);
    delete DataEntryWin;
}

VA(0x00455759, 0x1d9)
short DataEntryWindowHandler(tag_message& message) {
    short widgetId = DATA_ENTRY_TEXT;

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
                    case DATA_ENTRY_TEXT:
                    gotText:
                        message.type = MESSAGE_WIDGET;
                        message.id = DATA_ENTRY_TEXT;
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
                        message.id = DATA_ENTRY_TEXT;
                        message.text = cDEDest;
                        DataEntryWin->BroadcastMessage(message);
                        DataEntryWin->DrawWindow(1, DATA_ENTRY_TEXT, DATA_ENTRY_TEXT);
                        gpWindowManager->m_dialogResult = message.id;
                        message.command = message.id = WIDGET_COMMAND_DIALOG_SELECT;
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

// Buka 2.1 GetTownName; HoMM1 towns carry a name index, and campaign maps
// override one town by position.
VA(0x00455aaf, 0xdc)
char* GetTownName(int i) {
    town* townPointer = gpGame->GetTown(i);
    if (gpGame->m_campaignType > 0
        && gCampaignScenarios[gpGame->m_campaignScenario].victoryTownX >= 0
        && gCampaignScenarios[gpGame->m_campaignScenario].victoryTownX == townPointer->m_x
        && gCampaignScenarios[gpGame->m_campaignScenario].victoryTownY == townPointer->m_y)
        return gCampaignScenarios[gpGame->m_campaignScenario].victoryTownName;
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
            strcpy(
                gText,
                "Are you sure you want to load a new game?  (Your current game will be lost)"
            );
        confirmMenuCommand:
            if (gpAdvManager->m_active == 1) {
                NormalDialog(gText, NORMAL_DIALOG_TYPE_YES_NO);
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
            gConfig.walkSpeed = WALK_SPEED_JUMP;
            goto walkSpeedChanged;
        case APP_MENU_SPEED_GALLOP:
            gConfig.walkSpeed = WALK_SPEED_GALLOP;
            goto walkSpeedChanged;
        case APP_MENU_SPEED_CANTER:
            gConfig.walkSpeed = WALK_SPEED_CANTER;
            goto walkSpeedChanged;
        case APP_MENU_SPEED_TROT:
            gConfig.walkSpeed = WALK_SPEED_TROT;
            goto walkSpeedChanged;
        case APP_MENU_SPEED_WALK:
            gConfig.walkSpeed = WALK_SPEED_WALK;
            goto walkSpeedChanged;
        walkSpeedChanged:
            menuChanged = 1;
            break;
        case APP_MENU_CD_STEREO:
            if (gConfig.musicSource) {
                gConfig.musicSource = SOUND_MUSIC_SOURCE_DIGITAL;
            } else {
                if (!gpSoundManager->m_cdStarted) {
                    NormalDialog(
                        "Unable to set up CD stereo music.  Your CD player might be in use by "
                        "another program, or your sound driver might not support CD stereo.",
                        NORMAL_DIALOG_TYPE_OK
                    );
                    break;
                }
                gConfig.musicSource = SOUND_MUSIC_SOURCE_CD;
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
            gpAdvManager->ViewWorld(SPELL_VIEW_ALL, 0, 0);
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
        case WALK_SPEED_JUMP:
            checkedCommand = APP_MENU_SPEED_JUMP;
            break;
        case WALK_SPEED_GALLOP:
            checkedCommand = APP_MENU_SPEED_GALLOP;
            break;
        case WALK_SPEED_CANTER:
            checkedCommand = APP_MENU_SPEED_CANTER;
            break;
        case WALK_SPEED_TROT:
            checkedCommand = APP_MENU_SPEED_TROT;
            break;
        default:
            checkedCommand = APP_MENU_SPEED_WALK;
            break;
    }
    CheckMenuItem((HMENU)hmnuApp, checkedCommand, MF_CHECKED);
    CheckMenuItem(
        (HMENU)hmnuApp,
        APP_MENU_CD_STEREO,
        gConfig.musicSource ? MF_CHECKED : MF_UNCHECKED
    );
    CheckMenuItem(
        (HMENU)hmnuApp,
        APP_MENU_SHOW_PATH,
        gConfig.showRoute ? MF_CHECKED : MF_UNCHECKED
    );
    CheckMenuItem(
        (HMENU)hmnuApp,
        APP_MENU_VIEW_ENEMY_MOVES,
        1 - gConfig.blackoutComputer ? MF_CHECKED : MF_UNCHECKED
    );
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

// KB owns retail .data 0x00491058-0x0049ea9f: these initialized globals in
// address order, followed by their initializer literals (0x00494184-0x0049ea97,
// emitted in this order). Initializers are retail bytes. Unreferenced storage at
// 0x00492570 (2 x 16 bytes), 0x0049303c and 0x00494178 is not yet named.
DATA(0x00491828)
short gCastleAmounts[4] = {20, 20, 0, 0};
DATA(0x00491830)
short gHeroGoldCost = 2500;
DATA(0x00491838)
short gVesaMode[6] = {640, 480, 256, 20226, 257, 0};
DATA(0x00491848)
tag_tilePoint normalDirTable[8] = {
    {0, -1, 16},
    {1, -1, 16},
    {1, 0, 16},
    {1, 1, 16},
    {0, 1, 16},
    {-1, 1, 16},
    {-1, 0, 16},
    {-1, -1, 16},
};
DATA(0x00491868)
TownBuildingExtent gTownBuildingExtents[4][16] = {
    {{296, 0, 88, 156},
     {128, 64, 136, 128},
     {196, 64, 124, 148},
     {0, 162, 164, 94},
     {160, 180, 128, 76},
     {0, 0, 640, 256},
     {56, 0, 356, 128},
     {80, 86, 154, 76},
     {0, 82, 96, 64},
     {404, 120, 224, 148},
     {300, 136, 256, 128},
     {380, 64, 192, 96},
     {544, 8, 96, 248},
     {0, 0, 640, 256},
     {0, 0, 640, 256},
     {0, 0, 640, 256}},
    {{296, 0, 88, 156},
     {128, 64, 136, 128},
     {196, 64, 124, 148},
     {0, 162, 164, 94},
     {160, 180, 128, 76},
     {0, 0, 640, 256},
     {260, 0, 224, 128},
     {508, 100, 132, 156},
     {78, 36, 76, 118},
     {0, 100, 128, 128},
     {380, 100, 142, 90},
     {320, 176, 184, 84},
     {510, 0, 80, 140},
     {0, 0, 640, 256},
     {0, 0, 640, 256},
     {0, 0, 640, 256}},
    {{296, 0, 88, 156},
     {128, 64, 136, 128},
     {196, 64, 124, 148},
     {0, 162, 164, 94},
     {160, 180, 128, 76},
     {0, 0, 640, 256},
     {214, 0, 324, 132},
     {68, 60, 80, 74},
     {328, 128, 152, 128},
     {524, 32, 116, 132},
     {428, 74, 210, 120},
     {0, 86, 110, 80},
     {470, 68, 170, 190},
     {0, 0, 640, 256},
     {0, 0, 640, 256},
     {0, 0, 640, 256}},
    {{296, 0, 88, 156},
     {128, 64, 136, 128},
     {196, 64, 124, 148},
     {0, 162, 164, 94},
     {160, 180, 128, 76},
     {0, 0, 640, 256},
     {164, 0, 290, 142},
     {522, 0, 118, 164},
     {554, 96, 86, 132},
     {412, 0, 128, 128},
     {348, 110, 164, 152},
     {0, 40, 110, 132},
     {38, 0, 124, 158},
     {0, 0, 640, 256},
     {0, 0, 640, 256},
     {0, 0, 640, 256}},
};
DATA(0x00491a68)
unsigned short gDwellingRequirements[24] = {
    0, 128, 144, 132, 1536, 1536, 0, 132, 128, 513, 1024, 2048,
    0, 128, 128, 128, 1024, 2048, 0, 128, 128, 256, 512,  3072,
};
DATA(0x00491a98)
int gResourceBaseValue[7] = {250, 250, 200, 250, 250, 250, 1};
DATA(0x00491ab8)
int gStartingResources[4][7] = {
    {30, 10, 30, 10, 10, 10, 10000},
    {20, 5, 20, 5, 5, 5, 7500},
    {10, 0, 10, 0, 0, 0, 5000},
    {0, 0, 0, 0, 0, 0, 0},
};
DATA(0x00491b28)
int giMineIncome[7] = {2, 1, 2, 1, 1, 1, 1000};
DATA(0x00491b48)
int gArtifactBaseRV[37] = {
    9000, 22000, 18000, 14000, 6000, 4000, 4000, 5600, 1200, 1200, 1200, 1200, -1200,
    2000, 1800,  1800,  2000,  1000, 3600, 5600, 4000, 5040, 2700, 3900, 4950, 5850,
    7000, 6000,  4000,  4500,  2250, 1200, 1200, 1200, 1200, 3500, 1500,
};
DATA(0x00491bdc)
int gUltArtifactAvgValue = 16200;
DATA(0x00491be0)
char gcDataPath[352] = ".\\DATA\\";
DATA(0x00491d40)
char gcAnimPath[352] = "\\HEROES\\ANIM\\";
DATA(0x00491ea0)
char gcSoundPath[352] = "\\HEROES\\SOUND\\";
DATA(0x00492000)
char gcGamePath[20] = ".\\GAMES\\";
DATA(0x00492018)
char gcMapPath[20] = ".\\MAPS\\";
DATA(0x00492030)
signed char gHeroScoutRadius[8] = {4, 4, 4, 6, 4, 0, 0, 0};
DATA(0x00492038)
float gfClassNavigationMod[8] = {1.0f, 1.0f, 2.0f, 1.0f, 1.0f, 1.3f, 1.0f, 1.0f};
DATA(0x00492058)
signed char giVisRangeTown = 5;
DATA(0x00492060)
tag_monsterInfo gMonsterDatabase[28] = {
    {20, 18, 9, 12, 1, 1, 1, 0, 1, 1, 1, 1, 5, 0, {3, 0, 18, 0, 5, 0}, 0},
    {150, 256, 17, 8, 10, 10, 1, 3, 5, 3, 2, 3, 5, 12, {3, 0, 4, 0, 5, 0}, 4},
    {200, 399, 20, 5, 15, 15, 2, 0, 5, 9, 3, 4, 6, 0, {3, 0, 4, 0, 5, 0}, 0},
    {250, 768, 31, 4, 25, 25, 2, 0, 7, 9, 4, 6, 3, 0, {28, 0, 18, 0, 12, 0}, 0},
    {300, 1274, 42, 3, 30, 30, 3, 0, 10, 9, 5, 10, 6, 0, {3, 0, 21, 0, 5, 0}, 1},
    {600, 4014, 60, 2, 50, 50, 3, 0, 11, 12, 10, 20, 3, 0, {28, 0, 18, 0, 12, 0}, 0},
    {40, 57, 14, 10, 3, 3, 2, 0, 3, 1, 1, 2, 2, 0, {28, 0, 22, 0, 16, 0}, 0},
    {140, 241, 17, 8, 10, 10, 1, 2, 3, 4, 2, 3, 3, 8, {3, 0, 4, 0, 5, 0}, 4},
    {200, 482, 24, 5, 20, 20, 3, 0, 6, 2, 3, 5, 4, 0, {3, 0, 4, 0, 5, 0}, 1},
    {300, 902, 30, 4, 40, 40, 1, 0, 9, 5, 4, 6, 2, 0, {3, 0, 20, 0, 24, 0}, 0},
    {600, 2627, 44, 3, 50, 40, 2, 5, 10, 5, 5, 7, 2, 8, {3, 0, 21, 0, 14, 0}, 4},
    {750, 5721, 57, 2, 60, 80, 2, 0, 12, 9, 12, 24, 7, 0, {3, 0, 20, 0, 11, 0}, 8},
    {50, 74, 15, 8, 2, 2, 2, 0, 4, 2, 1, 2, 8, 0, {27, 0, 20, 0, 9, 0}, 2},
    {200, 380, 19, 6, 20, 20, 1, 0, 6, 5, 2, 4, 3, 0, {28, 0, 23, 0, 17, 0}, 0},
    {250, 567, 23, 4, 15, 15, 2, 5, 4, 3, 2, 3, 5, 24, {28, 0, 21, 0, 15, 0}, 4},
    {350, 1019, 29, 3, 25, 25, 3, 5, 7, 5, 5, 8, 9, 8, {28, 0, 22, 0, 8, 0}, 4},
    {500, 1866, 37, 2, 40, 40, 2, 0, 10, 9, 7, 14, 10, 0, {3, 0, 4, 0, 5, 0}, 1},
    {1500, 11177, 64, 1, 100, 100, 3, 0, 12, 10, 20, 40, 11, 0, {25, 0, 4, 0, 7, 0}, 11},
    {60, 94, 16, 8, 5, 5, 2, 1, 3, 1, 1, 2, 5, 8, {3, 0, 4, 0, 5, 0}, 5},
    {200, 379, 19, 6, 15, 15, 3, 0, 4, 7, 2, 3, 12, 0, {26, 0, 21, 0, 8, 0}, 2},
    {300, 739, 25, 4, 25, 25, 2, 0, 6, 6, 3, 5, 4, 0, {25, 0, 19, 0, 13, 0}, 3},
    {400, 1248, 31, 3, 35, 35, 2, 0, 9, 8, 5, 10, 2, 0, {3, 0, 4, 0, 5, 0}, 0},
    {800, 3142, 43, 2, 75, 75, 1, 0, 8, 9, 6, 12, 4, 0, {3, 0, 4, 0, 5, 0}, 1},
    {3000, 45258, 127, 1, 200, 200, 2, 0, 12, 12, 25, 50, 13, 0, {25, 0, 20, 0, 10, 0}, 11},
    {50, 112, 22, 12, 4, 4, 3, 0, 6, 1, 1, 2, 5, 0, {3, 0, 4, 0, 5, 0}, 0},
    {200, 544, 27, 4, 20, 20, 3, 0, 7, 6, 2, 5, 3, 0, {3, 0, 4, 0, 5, 0}, 1},
    {250, 1263, 59, 3, 20, 20, 2, 0, 8, 7, 4, 6, 14, 0, {3, 0, 4, 0, 5, 0}, 2},
    {650, 3831, 43, 2, 50, 50, 3, 0, 10, 9, 20, 30, 15, 0, {3, 0, 4, 0, 5, 0}, 2},
};
DATA(0x004923c8)
float gfStatPower[41] = {
    0.63f, 0.63f, 0.63f, 0.63f, 0.63f, 0.63f, 0.64f, 0.65f, 0.67f, 0.68f, 0.7f,
    0.72f, 0.74f, 0.76f, 0.78f, 0.81f, 0.84f, 0.87f, 0.91f, 0.95f, 1.0f,  1.05f,
    1.1f,  1.15f, 1.22f, 1.28f, 1.36f, 1.44f, 1.53f, 1.63f, 1.74f, 1.86f, 1.99f,
    2.14f, 2.3f,  2.48f, 2.67f, 2.86f, 2.86f, 2.86f, 2.86f,
};
DATA(0x00492470)
float gfBattleStat[41] = {
    0.2f,  0.2f,  0.2f,  0.2f,  0.2f,  0.21f, 0.23f, 0.25f, 0.28f, 0.31f, 0.35f,
    0.39f, 0.43f, 0.48f, 0.53f, 0.59f, 0.66f, 0.73f, 0.81f, 0.9f,  1.0f,  1.1f,
    1.21f, 1.33f, 1.46f, 1.61f, 1.77f, 1.95f, 2.14f, 2.36f, 2.59f, 2.85f, 3.14f,
    3.45f, 3.8f,  4.18f, 4.59f, 5.0f,  5.0f,  5.0f,  5.0f,
};
DATA(0x00492514)
signed char gMageGuildSpellCount[4] = {3, 5, 7, 9};
DATA(0x00492518)
float gfSpellCastNumMod[21] = {
    0.0f,  1.0f,  1.7f,  2.2f,  2.6f,  2.95f, 3.27f, 3.56f, 3.81f, 4.04f, 4.25f,
    4.45f, 4.64f, 4.83f, 5.01f, 5.19f, 5.36f, 5.53f, 5.68f, 5.82f, 5.96f,
};
DATA(0x00492590)
signed char gbDrawSavedCursor = 0;
DATA(0x00492598)
short gMinExpForLevel[4][12] = {
    {0, 1000, 2000, 3200, 4500, 6000, 7700, 9000, 11000, 13200, 15500, 18500},
    {0, 1000, 2000, 3200, 4500, 6000, 7700, 9000, 11000, 13200, 15500, 18500},
    {0, 1000, 2000, 3200, 4500, 6000, 7700, 9000, 11000, 13200, 15500, 18500},
    {0, 1000, 2000, 3200, 4500, 6000, 7700, 9000, 11000, 13200, 15500, 18500},
};
DATA(0x004925f8)
signed char gRouteFrame[8][8] = {
    {1, 6, 6, 6, 1, 38, 38, 38},
    {12, 2, 7, 7, 7, 2, 12, 12},
    {9, 9, 3, 8, 8, 8, 3, 9},
    {10, 10, 10, 4, 13, 13, 13, 4},
    {5, 11, 11, 11, 5, 43, 43, 43},
    {42, 36, 45, 45, 45, 36, 42, 42},
    {41, 41, 35, 40, 40, 40, 35, 41},
    {44, 44, 44, 34, 39, 39, 39, 34},
};
DATA(0x00492638)
unsigned char giCloudType[256] = {
    11,  7,   8,   129, 9,   10,  128, 33,  108, 29,  30,  32,  28,  133, 34,  22,  11,  7,   8,
    113, 9,   10,  128, 126, 108, 29,  30,  131, 28,  133, 34,  120, 11,  7,   8,   129, 9,   10,
    112, 127, 108, 29,  30,  32,  28,  133, 125, 121, 11,  7,   8,   113, 9,   10,  112, 103, 108,
    29,  30,  131, 28,  133, 125, 117, 11,  7,   8,   129, 9,   10,  128, 33,  108, 29,  30,  32,
    12,  27,  25,  21,  11,  7,   8,   113, 9,   10,  128, 126, 108, 29,  30,  131, 12,  27,  25,
    118, 11,  7,   8,   129, 9,   10,  112, 127, 108, 29,  30,  32,  12,  27,  1,   19,  11,  7,
    8,   113, 9,   10,  114, 103, 108, 29,  30,  131, 12,  27,  1,   116, 11,  7,   8,   129, 9,
    10,  128, 33,  108, 13,  30,  31,  28,  26,  34,  20,  11,  7,   8,   113, 9,   10,  128, 126,
    108, 13,  30,  5,   28,  26,  34,  24,  11,  7,   8,   129, 9,   10,  112, 127, 108, 13,  30,
    31,  28,  26,  125, 18,  11,  7,   8,   115, 9,   10,  112, 103, 108, 13,  30,  5,   28,  26,
    125, 123, 11,  7,   8,   129, 9,   10,  128, 33,  108, 13,  30,  31,  12,  3,   25,  17,  11,
    7,   8,   113, 9,   10,  128, 126, 108, 15,  30,  5,   12,  3,   25,  23,  11,  7,   8,   129,
    9,   10,  112, 127, 108, 13,  30,  31,  14,  3,   1,   16,  11,  7,   8,   115, 9,   10,  114,
    103, 108, 15,  30,  5,   14,  3,   1,   0,
};
DATA(0x00492738)
signed char gMons32Width[28] = {
    20, 20, 20, 25, 25, 24, 21, 21, 25, 27, 22, 20, 23, 23,
    21, 22, 25, 23, 27, 22, 29, 28, 32, 27, 21, 26, 21, 29,
};
DATA(0x00492758)
short giScoreMon[28][2] = {
    {0, 0},    {7, 6},    {14, 12},  {21, 18},  {28, 24},  {35, 7},   {42, 1},
    {49, 19},  {56, 13},  {63, 2},   {70, 8},   {77, 25},  {84, 14},  {91, 20},
    {98, 3},   {105, 9},  {112, 15}, {119, 21}, {126, 4},  {133, 26}, {140, 16},
    {147, 10}, {154, 22}, {161, 5},  {168, 27}, {175, 11}, {182, 17}, {189, 23},
};
DATA(0x004927c8)
short giScoreCampaignMon[28][2] = {
    {3600, 0},  {3400, 6},  {3200, 12}, {3000, 18}, {2600, 24}, {2400, 7},  {2200, 1},
    {2000, 19}, {1800, 13}, {1600, 2},  {1500, 8},  {1400, 25}, {1300, 14}, {1200, 20},
    {1100, 3},  {1000, 9},  {900, 15},  {800, 21},  {750, 4},   {700, 26},  {650, 16},
    {600, 10},  {550, 22},  {500, 5},   {450, 27},  {400, 11},  {350, 17},  {300, 23},
};
DATA(0x00492838)
WindowTextEntry gWinSetup[68] = {
    {0, 0},    {1, 0},    {0, 1},    {50, 2},   {16, 2},   {17, 2},   {18, 2},   {19, 2},
    {20, 2},   {49, 2},   {100, 3},  {101, 3},  {102, 3},  {103, 3},  {104, 3},  {105, 3},
    {1, 4},    {300, 5},  {301, 5},  {302, 5},  {303, 5},  {80, 6},   {600, 7},  {601, 7},
    {602, 7},  {603, 7},  {604, 7},  {605, 7},  {5, 7},    {6, 7},    {7, 7},    {606, 7},
    {607, 7},  {608, 7},  {200, 8},  {201, 8},  {202, 8},  {203, 8},  {204, 8},  {205, 8},
    {600, 9},  {601, 9},  {602, 9},  {603, 9},  {600, 10}, {600, 11}, {0, 12},   {1, 12},
    {600, 13}, {601, 13}, {602, 13}, {604, 13}, {0, 14},   {601, 14}, {0, 15},   {600, 15},
    {601, 15}, {602, 15}, {603, 15}, {604, 15}, {605, 15}, {606, 15}, {607, 15}, {608, 15},
    {609, 15}, {610, 15}, {611, 15}, {1, 16},
};
DATA(0x00492948)
signed char townTheme[4] = {3, 0, 2, 1};
DATA(0x00492950)
campaignScenario gCampaignScenarios[9] = {
    {0,
     36,
     35,
     {' ', ' ', ' ', ' ', 'G', 'a', 't', 'e', 'w', 'a', 'y', ' ', ' ', ' ', ' ', ' '},
     {0, 1, 1, 1},
     {4, 4, 4},
     {{30, 10, 30, 10, 10, 10, 10000},
      {30, 10, 30, 10, 10, 10, 10000},
      {30, 10, 30, 10, 10, 10, 10000},
      {30, 10, 30, 10, 10, 10, 10000}}},
    {0,
     -1,
     -1,
     {' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' '},
     {0, 2, 2, 2},
     {4, 4, 4},
     {{30, 10, 30, 10, 10, 10, 10000},
      {30, 10, 30, 10, 10, 10, 10000},
      {30, 10, 30, 10, 10, 10, 10000},
      {30, 10, 30, 10, 10, 10, 10000}}},
    {0,
     -1,
     -1,
     {' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' '},
     {0, 2, 2, 2},
     {4, 4, 4},
     {{30, 10, 30, 10, 10, 10, 10000},
      {30, 10, 30, 10, 10, 10, 10000},
      {30, 10, 30, 10, 10, 10, 10000},
      {30, 10, 30, 10, 10, 10, 10000}}},
    {0,
     -1,
     -1,
     {' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' '},
     {0, 2, 2, 2},
     {4, 4, 4},
     {{30, 10, 30, 10, 10, 10, 10000},
      {30, 10, 30, 10, 10, 10, 10000},
      {30, 10, 30, 10, 10, 10, 10000},
      {30, 10, 30, 10, 10, 10, 10000}}},
    {0,
     13,
     10,
     {'C', 'a', 's', 't', 'l', 'e', ' ', 'I', 'r', 'o', 'n', 'f', 'i', 's', 't', ' '},
     {0, 3, 0, 0},
     {2, 4, 4},
     {{30, 10, 30, 10, 10, 10, 10000},
      {30, 10, 30, 10, 10, 10, 10000},
      {30, 10, 30, 10, 10, 10, 10000},
      {30, 10, 30, 10, 10, 10, 10000}}},
    {0,
     62,
     20,
     {' ', 'C', 'a', 's', 't', 'l', 'e', ' ', 'S', 'l', 'a', 'y', 'e', 'r', ' ', ' '},
     {0, 3, 0, 0},
     {1, 4, 4},
     {{30, 10, 30, 10, 10, 10, 10000},
      {30, 10, 30, 10, 10, 10, 10000},
      {30, 10, 30, 10, 10, 10, 10000},
      {30, 10, 30, 10, 10, 10, 10000}}},
    {0,
     8,
     8,
     {'C', 'a', 's', 't', 'l', 'e', ' ', 'L', 'a', 'm', 'a', 'n', 'd', 'a', ' ', ' '},
     {0, 3, 0, 0},
     {3, 4, 4},
     {{30, 10, 30, 10, 10, 10, 10000},
      {30, 10, 30, 10, 10, 10, 10000},
      {30, 10, 30, 10, 10, 10, 10000},
      {30, 10, 30, 10, 10, 10, 10000}}},
    {0,
     66,
     69,
     {' ', 'C', 'a', 's', 't', 'l', 'e', ' ', 'A', 'l', 'a', 'm', 'a', 'r', ' ', ' '},
     {0, 3, 0, 0},
     {0, 4, 4},
     {{30, 10, 30, 10, 10, 10, 10000},
      {30, 10, 30, 10, 10, 10, 10000},
      {30, 10, 30, 10, 10, 10, 10000},
      {30, 10, 30, 10, 10, 10, 10000}}},
    {1,
     -1,
     -1,
     {' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' '},
     {0, 3, 3, 3},
     {4, 4, 4},
     {{30, 10, 30, 10, 10, 10, 10000},
      {30, 10, 30, 10, 10, 10, 10000},
      {30, 10, 30, 10, 10, 10, 10000},
      {30, 10, 30, 10, 10, 10, 10000}}},
};
DATA(0x00492c50)
signed char gCampaignSideCrests[4][2] = {{2, 0}, {1, 0}, {3, 0}, {0, 0}};
DATA(0x00492c58)
short gCrestTownTypes[4] = {3, 2, 0, 1};
DATA(0x00492c60)
short gCrestHeroClass[4] = {3, 1, 0, 2};
DATA(0x00492c68)
signed char gHeroSkillBonus[4][9][4] = {
    {{20, 60, 10, 10},
     {60, 20, 10, 10},
     {20, 60, 10, 10},
     {25, 25, 25, 25},
     {20, 60, 10, 10},
     {60, 20, 10, 10},
     {20, 60, 10, 10},
     {20, 60, 10, 10},
     {25, 25, 25, 25}},
    {{70, 20, 5, 5},
     {20, 70, 5, 5},
     {70, 20, 5, 5},
     {40, 40, 10, 10},
     {70, 20, 5, 5},
     {20, 70, 5, 5},
     {70, 20, 5, 5},
     {70, 20, 5, 5},
     {40, 40, 10, 10}},
    {{5, 5, 20, 70},
     {5, 5, 70, 20},
     {5, 5, 20, 70},
     {10, 10, 40, 40},
     {10, 10, 30, 50},
     {10, 10, 50, 30},
     {10, 10, 30, 50},
     {10, 10, 30, 50},
     {20, 20, 30, 30}},
    {{5, 5, 70, 20},
     {5, 5, 20, 70},
     {5, 5, 70, 20},
     {10, 10, 40, 40},
     {10, 10, 50, 30},
     {10, 10, 30, 50},
     {10, 10, 50, 30},
     {10, 10, 50, 30},
     {20, 20, 30, 30}},
};
DATA(0x00492cf8)
signed char gTownHeroClass[8] = {0, 2, 1, 3, 0, 2, 1, 3};
DATA(0x00492d00)
unsigned char gMonoColorMap[256] = {
    10,  11,  12,  12,  13,  14,  14,  15,  16,  16,  17,  18,  18,  19,  20,  20,  21,  22,  22,
    23,  24,  24,  25,  26,  26,  27,  28,  28,  29,  30,  30,  31,  32,  33,  34,  34,  35,  36,
    36,  37,  38,  38,  39,  40,  40,  41,  42,  42,  43,  44,  44,  45,  46,  46,  47,  48,  48,
    49,  50,  50,  51,  52,  52,  53,  54,  55,  56,  57,  58,  59,  60,  61,  62,  63,  64,  65,
    66,  67,  68,  69,  70,  71,  72,  73,  74,  75,  76,  77,  78,  79,  80,  81,  82,  83,  84,
    85,  86,  87,  88,  89,  90,  91,  92,  93,  94,  95,  96,  97,  98,  99,  100, 101, 102, 103,
    104, 105, 106, 107, 108, 109, 110, 111, 112, 113, 114, 115, 116, 117, 118, 119, 120, 121, 122,
    123, 124, 125, 126, 127, 128, 129, 130, 131, 132, 133, 134, 135, 136, 137, 138, 139, 140, 141,
    142, 143, 144, 145, 146, 147, 148, 149, 150, 151, 152, 153, 154, 155, 156, 157, 158, 159, 160,
    161, 162, 163, 164, 165, 166, 167, 168, 169, 170, 171, 172, 173, 174, 175, 176, 177, 178, 179,
    180, 181, 182, 183, 184, 185, 186, 187, 188, 189, 190, 191, 192, 193, 194, 195, 196, 197, 198,
    199, 200, 201, 202, 203, 204, 205, 206, 207, 208, 209, 210, 211, 212, 213, 214, 215, 216, 217,
    218, 219, 220, 221, 222, 223, 224, 225, 226, 227, 228, 229, 230, 231, 232, 233, 234, 235, 236,
    237, 238, 239, 240, 241, 242, 243, 244, 245,
};
DATA(0x00492e00)
int gbLoadingMonoIcon = 0;
DATA(0x00492e04)
int giMonoIconSkip = -1;
DATA(0x00492e08)
int giScrollX = 0;
DATA(0x00492e0c)
int giScrollY = 0;
DATA(0x00492e10)
int gbNoBorder = 0;
DATA(0x00492e14)
int gbEnlargeScreenBlit = 1;
DATA(0x00492e18)
void* hmnuDflt = NULL;
DATA(0x00492e1c)
void* hmnuCmbt = NULL;
DATA(0x00492e20)
void* hmnuAdv = NULL;
DATA(0x00492e24)
void* hmnuTown = NULL;
DATA(0x00492e28)
int gbColorMice = 0;
DATA(0x00492e2c)
int gbSpecialMouseMasks = 0;
DATA(0x00492e30)
int giCurExe = 0;
DATA(0x00492e34)
int giMenuCommand = APP_MENU_NONE;
DATA(0x00492e38)
int gbInDialog = 0;
DATA(0x00492e40)
SMenuEnableStatus gsMenuEnableStatus[70] = {
    {0, 0, 0, 0},     {40005, 1, 1, 0}, {40006, 1, 1, 0}, {40007, 1, 1, 0}, {40008, 1, 1, 0},
    {40009, 1, 1, 0}, {40012, 0, 0, 0}, {40013, 0, 0, 0}, {40014, 0, 0, 0}, {40015, 0, 0, 0},
    {40016, 1, 0, 0}, {40017, 1, 0, 0}, {40018, 1, 0, 0}, {40019, 1, 0, 0}, {40020, 1, 0, 0},
    {40021, 1, 0, 0}, {40022, 1, 0, 0}, {40023, 1, 0, 0}, {40024, 1, 0, 0}, {40025, 1, 0, 0},
    {40026, 1, 0, 0}, {40028, 1, 0, 0}, {40029, 1, 0, 0}, {40030, 1, 0, 0}, {40031, 1, 0, 0},
    {40032, 1, 0, 0}, {40033, 1, 0, 0}, {40034, 1, 0, 0}, {40035, 1, 0, 0}, {40036, 1, 0, 0},
    {40037, 1, 0, 0}, {40038, 1, 0, 0}, {40040, 0, 0, 0}, {40041, 0, 0, 0}, {40042, 0, 0, 0},
    {40043, 0, 0, 0}, {40044, 0, 0, 0}, {40045, 0, 0, 0}, {40046, 0, 0, 0}, {40047, 0, 0, 0},
    {40052, 1, 1, 0}, {40053, 1, 1, 0}, {40102, 0, 1, 0}, {40104, 0, 1, 0}, {40105, 0, 1, 0},
    {40106, 0, 1, 0}, {40107, 0, 1, 0}, {40109, 0, 1, 0}, {40110, 0, 1, 0}, {40111, 0, 1, 0},
    {40112, 0, 1, 0}, {40114, 0, 1, 0}, {40115, 0, 1, 0}, {40117, 0, 1, 0}, {40118, 0, 1, 0},
    {40120, 0, 1, 0}, {40121, 0, 1, 0}, {40123, 0, 1, 0}, {40124, 0, 1, 0}, {40127, 0, 1, 0},
    {40128, 0, 1, 0}, {40129, 0, 1, 0}, {40131, 0, 1, 0}, {40132, 0, 1, 0}, {40134, 0, 1, 0},
    {40135, 0, 1, 0}, {40137, 0, 1, 0}, {40138, 0, 1, 0}, {40139, 0, 0, 0}, {40140, 0, 0, 0},
};
DATA(0x0049302c)
int gbInSetupDialog = 0;
DATA(0x00493030)
int gbMinimized = 0;
DATA(0x00493034)
int gbHeroMoving = 0;
DATA(0x00493038)
int gbInSmacker = 0;
DATA(0x00493040)
int gbRemoteReady = 0;
DATA(0x00493044)
int gbHeartbeatSeen = 0;
DATA(0x00493048)
char* gArtifactNames[38] = {
    "Ultimate Book of Knowledge",
    "Ultimate Sword of Dominion",
    "Ultimate Cloak of Protection",
    "Ultimate Wand of Magic",
    "Arcane Necklace of Magic",
    "Caster's Bracelet of Magic",
    "Mage's Ring of Power",
    "Witch's Broach of Magic",
    "Medal of Valor",
    "Medal of Courage",
    "Medal of Honor",
    "Medal of Distinction",
    "Fizbin of Misfortune",
    "Thunder Mace of Dominion",
    "Armored Gauntlets of Protection",
    "Defender Helm of Protection",
    "Giant Flail of Dominion",
    "Ballista of Quickness",
    "Stealth Shield of Protection",
    "Dragon Sword of Dominion",
    "Power Axe of Dominion",
    "Divine Breastplate of Protection",
    "Minor Scroll of Knowledge",
    "Major Scroll of Knowledge",
    "Superior Scroll of Knowledge",
    "Foremost Scroll of Knowledge",
    "Endless Sack of Gold",
    "Endless Bag of Gold",
    "Endless Purse of Gold",
    "Nomad Boots of Mobility",
    "Traveler's Boots of Mobility",
    "Lucky Rabbit's Foot",
    "Golden Horseshoe",
    "Gambler's Lucky Coin",
    "Four-Leaf Clover",
    "True Compass of Mobility",
    "Sailor's Astrolabe of Mobility",
    "Magic Book",
};
DATA(0x004930e0)
char* gArtifactDesc[38] = {
    "Ultimate Book\n(+12 Knowledge)\n\nThe Ultimate Book of Knowledge increases your knowledge by "
    "12.",
    "Ultimate Sword\n(+12 Attack)\n\nThe Ultimate Sword of Dominion increases your attack skill by "
    "12.",
    "Ultimate Cloak\n(+12 Defense)\n\nThe Ultimate Cloak of Protection increases your defense "
    "skill by 12.",
    "Ultimate Wand\n(+12 Spell Power)\n\nThe Ultimate Wand of Magic increases your spell power by "
    "12.",
    "Arcane Necklace\n(+4 Spell Power)\n\nThe Arcane Necklace of Magic increases your spell power "
    "by 4.",
    "Caster's Bracelet\n(+2 Spell Power)\n\nThe Caster's Bracelet of Magic increases your spell "
    "power by 2.",
    "Mage's Ring\n(+2 Spell Power)\n\nThe Mage's Ring of Power increases your spell power by 2.",
    "Witches Broach\n(+3 Spell Power)\n\nThe Witch's Broach of Magic increases your spell power by "
    "3.",
    "Medal\n\nThe Medal of Valor increases your morale.",
    "Medal\n\nThe Medal of Courage increases your morale.",
    "Medal\n\nThe Medal of Honor increases your morale.",
    "Medal\n\nThe Medal of Distinction increases your morale.",
    "Fizbin\n\nThe Fizbin of Misfortune greatly decreases your morale.",
    "Thunder Mace\n(+1 Attack)\n\nThe Thunder Mace of Dominion increases your attack skill by 1.",
    "Armored Gauntlets\n(+1 Defense)\n\nThe Armored Gauntlets of Protection increase your defense "
    "skill by 1.",
    "Defender Helm\n(+1 Defense)\n\nThe Defender Helm of Protection increases your defense skill "
    "by 1.",
    "Giant Flail\n(+1 Attack)\n\nThe Giant Flail of Dominion increases your attack skill by 1.",
    "Ballista\n\nThe Ballista of Quickness lets your catapult fire twice per combat round.",
    "Stealth Shield\n(+2 Defense)\n\nThe Stealth Shield of Protection increases your defense skill "
    "by 2.",
    "Dragon Sword\n(+3 Attack)\n\nThe Dragon Sword of Dominion increases your attack skill by 3.",
    "Power Axe\n(+2 Attack)\n\nThe Power Axe of Dominion increases your attack skill by 2.",
    "Divine Breastplate\n(+3 Defense)\n\nThe Divine Breastplate of Protection increases your "
    "defense skill by 3.",
    "Minor Scroll\n(+2 Knowledge)\n\nThe Minor Scroll of Knowledge increases your knowledge by 2.",
    "Major Scroll\n(+3 Knowledge)\n\nThe Major Scroll of Knowledge increases your knowledge by 3.",
    "Superior Scroll\n(+4 Knowledge)\n\nThe Superior Scroll of Knowledge increases your knowledge "
    "by 4.",
    "Foremost Scroll\n(+5 Knowledge)\n\nThe Foremost Scroll of Knowledge increases your knowledge "
    "by 5.",
    "Endless Sack\n\nThe Endless Sack of Gold provides you with 1000 gold per day.",
    "Endless Bag\n\nThe Endless Bag of Gold provides you with 750 gold per day.",
    "Endless Purse\n\nThe Endless Purse of Gold provides you with 500 gold per day.",
    "Nomad Boots\n\nThe Nomad Boots of Mobility increase your movement on land.",
    "Traveler's Boots\n\nThe Traveler's Boots of Mobility increase your movement on land.",
    "Rabbit's Foot\n\nThe Lucky Rabbit's Foot increases your luck in combat.",
    "Horseshoe\n\nThe Golden Horseshoe increases your luck in combat.",
    "Coin\n\nThe Gambler's Lucky Coin increases your luck in combat.",
    "Clover\n\nThe Four-Leaf Clover increases your luck in combat.",
    "Compass\n\nThe True Compass of Mobility increases your movement on land and sea.",
    "Astrolabe\n\nThe Sailors' Astrolabe of Mobility increases your movement on sea.",
    "Magic Book\n\nThe Magic Book enables you to cast spells.",
};
DATA(0x00493178)
char* gArtifactEvent[38] = {
    "",
    "",
    "",
    "",
    "After rescuing a sorceress from a cursed tomb, she rewards your heroism with an exquisite "
    "jeweled necklace.",
    "While searching through the rubble of a caved in mine, you free a group of trapped dwarves.  "
    "Grateful, the leader gives you a golden bracelet.",
    "A cry of pain leads you to a centaur, caught in a trap.  Upon setting the creature free, he "
    "hands you a small pouch.  Emptying the contents, you find a dazzling jeweled ring.",
    "Alongside the remains of a burnt witch lies a beautiful broach, intricately designed.  "
    "Approaching the corpse with caution, you add the broach to your inventory.",
    "Freeing a virtuous maiden from the clutches of an evil overlord, you are granted a Medal of "
    "Valor by the King's herald.",
    "After saving a young boy from a vicious pack of wolves, you return him to his father's manor. "
    " The grateful nobleman awards you with a Medal of Courage.",
    "After freeing a princess of a neighboring kingdom from the evil clutches of despicable "
    "slavers, she awards you with a Medal of Honor.",
    "Ridding the countryside of the hideous minotaur who made a sport of eating noblemen's "
    "knights, you are honored with the Medal of Distinction.",
    "You stumble upon a medal lying alongside the empty road.  Adding the medal to your inventory, "
    "you become aware that you have acquired the undesirable Fizbin of Misfortune, greatly "
    "decreasing your army's morale.",
    "During a sudden storm, a bolt of lightning strikes a tree, splitting it.  Inside the tree you "
    "find a mysterious mace.",
    "You encounter the infamous Black Knight!  After a grueling duel ending in a draw, the knight, "
    "out of respect, offers you a pair of armored gauntlets.",
    "A glint of golden light catches your eye.  Upon further investigation, you find a golden helm "
    "hidden under a bush.",
    "A clumsy Giant has killed himself with his own flail.  Knowing your superior skill with this "
    "weapon, you confidently remove the spectacular flail from the fallen giant.",
    "Walking through the ruins of an ancient walled city, you find the instrument of the city's "
    "destruction, an elaborately crafted ballista.",
    "A stone statue of a warrior holds a silver shield.  As you remove the shield, the statue "
    "crumbles into dust.",
    "As you are walking along a narrow path, a nearby bush suddenly bursts into flames.  Before "
    "your eyes the flames become the image of a beautiful woman.  She holds out a magnificent "
    "sword to you.",
    "You see a silver axe embedded deeply in the ground.  After several unsuccessful attempts by "
    "your army to remove the axe, you tightly grip the handle of the axe and effortlessly pull it "
    "free.",
    "A gang of rogues is sifting through the possessions of dead warriors.  Scaring off the "
    "scavengers, you note the rogues had overlooked a beautiful breastplate.",
    "Before you appears a levitating glass case with a scroll, perched upon a bed of crimson "
    "velvet.  At your touch, the lid opens and the scroll floats into your awaiting hands.",
    "Visiting a local wiseman, you explain the intent of your journey.  He reaches into a sack and "
    "withdraws a yellowed scroll and hands it to you.",
    "You come across the remains of an ancient Druid.  Bones, yellowed with age, peer from the "
    "ragged folds of her robe.  Searching the robe, you discover a scroll hidden in the folds.",
    "Mangled bones, yellowed with age, peer from the ragged folds of a dead Druid's robe.  "
    "Searching the robe, you discover a scroll hidden within.",
    "A little leprechaun dances gleefully around a magic sack.  Seeing you approach, he stops in "
    "mid-stride.  The little man screams and stamps his foot ferociously, vanishing into thin air. "
    " Remembering the old leprechaun saying 'Finders Keepers', you grab the sack and leave.",
    "A noblewoman, separated from her traveling companions, asks for your help.  After escorting "
    "her home, she rewards you with a bag filled with gold.",
    "In your travels, you find a leather purse filled with gold that once belonged to a great "
    "warrior king who had the ability to transform any inanimate object into gold.",
    "A nomad trader seeks protection from a tribe of goblins.  For your assistance, he gives you a "
    "finely crafted pair of boots made from the softest leather.  Looking closely, you see "
    "fascinating ancient carvings engraved on the leather.",
    "Discovering a pair of beautifully beaded boots made from the finest and softest leather, you "
    "thank the anonymous donor and add the boots to your inventory.",
    "A traveling merchant offers you a rabbit's foot, made of gleaming silver fur, for safe "
    "passage.  The merchant explains the charm will increase your luck in combat.",
    "An ensnared unicorn whinnies in fright.  Murmuring soothing words, you set her free.  "
    "Snorting and stamping her front hoof once, she gallops off.  Looking down you see a golden "
    "horseshoe.",
    "You have captured a mischievous imp who has been terrorizing the region.  In exchange for his "
    "release, he rewards you with a magical coin.",
    "In the middle of a patch of dead and dry vegetation, to your surprise you find a healthy "
    "green four-leaf clover.",
    "An old man claiming to be an inventor asks you to try his latest invention.  He then hands "
    "you a compass.",
    "An old sea captain is being tortured by ogres.  You save him, and in return he rewards you "
    "with a wondrous instrument to measure the distance of a star.",
    "The Magic Book  ??????",
};
DATA(0x00493210)
char* gStatNames[5] = {"Attack Skill", "Defense Skill", "Spell Power", "Knowledge", "Siege Skill"};
DATA(0x00493228)
char* gStatDesc[5] = {
    "Your attack skill is a bonus added to each creature's attack skill.",
    "Your defense skill is a bonus added to each creature's defense skill.",
    "Your spell power determines the length or power of a spell.",
    "Your knowledge is the number of each spell you are able to memorize.",
    "Your siege skill is the number of times your hero can shoot the catapult in one turn while "
    "attempting to siege a castle.",
};
DATA(0x00493240)
char* gClassNames[4] = {"Knight", "Barbarian", "Sorceress", "Warlock"};
DATA(0x00493250)
char* gArmyNames[28] = {
    "peasant",  "archer", "pikeman", "swordsman", "cavalry", "paladin",  "goblin",
    "orc",      "wolf",   "ogre",    "troll",     "cyclops", "sprite",   "dwarf",
    "elf",      "druid",  "unicorn", "phoenix",   "centaur", "gargoyle", "griffin",
    "minotaur", "hydra",  "dragon",  "rogue",     "nomad",   "ghost",    "genie",
};
DATA(0x004932c0)
char* gArmyNamesPlural[28] = {
    "peasants",  "archers", "pikemen",  "swordsmen", "cavalries", "paladins",  "goblins",
    "orcs",      "wolves",  "ogres",    "trolls",    "cyclopes",  "sprites",   "dwarves",
    "elves",     "druids",  "unicorns", "phoenix",   "centaurs",  "gargoyles", "griffins",
    "minotaurs", "hydras",  "dragons",  "rogues",    "nomads",    "ghosts",    "genies",
};
DATA(0x00493330)
char* gSpellNames[29] = {
    "Fireball",       "Lightning Bolt", "Teleport",       "Cure",         "Resurrect",
    "Haste",          "Slow",           "Blind",          "Bless",        "Protection",
    "Curse",          "Turn Undead",    "Anti-Magic",     "Dispel Magic", "Berzerker",
    "Armageddon",     "Storm",          "Meteor Shower",  "Paralyze",     "View Mines",
    "View Resources", "View Artifacts", "View Towns",     "View Heroes",  "View All",
    "Identify Hero",  "Summon Boat",    "Dimension Door", "Town Gate",
};
DATA(0x004933a8)
char* gNeutralBuildingNames[7] =
    {"Mage Guild", "Thieves' Guild", "Tavern", "Shipyard", "Well", "Tent", "Castle"};
DATA(0x004933c8)
char* gDwellingNames[24] = {
    "Thatched Hut", "Archery Range", "Blacksmith",    "Armory",     "Jousting Arena", "Cathedral",
    "Treehouse",    "Cottage",       "Archery Range", "Stonehenge", "Fenced Meadow",  "Red Tower",
    "Hut",          "Stick Hut",     "Den",           "Adobe",      "Bridge",         "Pyramid",
    "Cave",         "Crypt",         "Nest",          "Maze",       "Swamp",          "Black Tower",
};
DATA(0x00493428)
char* gTerrainNames[7] = {"Ocean", "Grass", "Snow", "Swamp", "Lava", "Desert", "Dirt"};
DATA(0x00493448)
char* gResourceNames[7] = {"Wood", "Mercury", "Ore", "Sulfur", "Crystal", "Gems", "Gold"};
DATA(0x00493468)
char* gObjectNames[63] = {
    "",
    "Alchemist Lab",
    "Signpost",
    "Buoy",
    "Skeleton",
    "Daemon Cave",
    "Treasure Chest",
    "Faerie Ring",
    "Campfire",
    "Fountain",
    "Gazebo",
    "Ancient Lamp",
    "Graveyard",
    "Straw Hut",
    "House",
    "Cabin",
    "Log Cabin",
    "Log Cabin",
    "Inn 1",
    "Inn 2",
    "Inn 3",
    "Inn 4",
    "Dragon City",
    "Lighthouse",
    "Waterwheel",
    "Mine",
    "Army Camp",
    "Obelisk",
    "Oasis",
    "Resource",
    "Rosebush",
    "Sandpit",
    "Sawmill",
    "Shrine",
    "Shrine",
    "Shipwreck",
    "Statue",
    "Tree Stump",
    "Swan Pond",
    "Desert Tent",
    "Town",
    "Stone Liths",
    "Wagon Camp",
    "Well",
    "Whirlpool",
    "Windmill",
    "Oak Tree",
    "Megalith",
    "Artifact",
    "Nothing here",
    "",
    "",
    "Mountains",
    "Mountains",
    "Mountains",
    "Mountains",
    "Trees",
    "Trees",
    "Trees",
    "Trees",
    "Trees",
    "",
    "Ship",
};
DATA(0x00493568)
char* gTownNames[36] = {
    "Blackridge",  "Pinehurst",  "Woodhaven",   "Hillstone",  "Whiteshield", "Bloodreign",
    "Dragontooth", "Greywind",   "Blackwind",   "Portsmith",  "Middle Gate", "Tundara",
    "Vulcania",    "Sansobar",   "Atlantium",   "Baywatch",   "Wildabar",    "Fountainhead",
    "Vertigo",     "Winterkill", "Nightshadow", "Sandcaster", "Lakeside",    "Olympus",
    "Necropolis",  "Burlock",    "Xabran",      "Dragadune",  "Alamar",      "Kalindra",
    "Blackfang",   "Basenji",    "Algary",      "Sorpigal",   "Dusk",        "Erliquin",
};
DATA(0x004935f8)
char* gEventText[77] = {
    "Alchemist\n\nYou have taken control of the local Alchemist shop. It will provide you with one "
    "unit of Mercury per day.",
    "Signpost\n\nA signpost reads:\n\n%s is near.",
    "Buoy\n\nYour men spot a navigational buoy, confirming that you are on course.",
    "Buoy\n\nYour men spot a navigational buoy, confirming that you are on course and increasing "
    "their morale.",
    "Moisture congeals on the walls and trickles slowly down to the ground.  Except for the "
    "evidence of a battle, the cave is empty.",
    "A large daemon emerges from the shadows and you attack. After an exhausting battle, you "
    "emerge victorious and receive 1000 experience points.",
    "A large daemon emerges from the shadows and you attack. After an exhausting battle, you "
    "emerge victorious and receive 1000 experience points and an artifact.",
    "A large daemon emerges from the shadows and you attack. After an exhausting battle, you "
    "emerge victorious and receive 1000 experience points and 2500 gold.",
    "You are captured by a large daemon.  He offers to let you free for 2500 gold, otherwise he "
    "will devour you.  Do you pay?",
    "Seeing that you do not have 2500 gold, the daemon slashes you with its claws, and the last "
    "thing you see is a red haze.",
    "Daemon Cave\n\nThe cave is dank and musty.  Two large red eyes glow eerily within the "
    "blackness.  Do you wish to enter?",
    "Chest\n\nAfter scouring the area, you fall upon a hidden treasure cache.  You may take the "
    "gold or distribute the gold to the peasants for experience.  Do you wish to keep the gold?",
    "Faerie Ring\n\nYou enter the faerie ring, but nothing happens.",
    "Faerie Ring\n\nUpon entering the mystical faerie ring, your army gains luck for its next "
    "battle.",
    "Campfire\n\nRansacking an enemy camp, you discover a hidden cache of treasures.",
    "Fountain\n\nYou drink from the enchanted fountain, but nothing happens.",
    "Fountain\n\nAs you drink the sweet water, you gain luck for your next battle.",
    "Gazebo\n\nAn old knight appears on the steps of the gazebo. \"I am sorry, my liege, I have "
    "taught you all I can.\"",
    "Gazebo\n\nAn old knight appears on the steps of the gazebo. \"My liege, I will teach you all "
    "that I know to aid you in your travels.\"",
    "Genie Lamp\n\nYou stumble upon a dented and tarnished lamp lodged deep in the earth. Do you "
    "wish to rub the lamp?",
    "Graveyard\n\nYou tentatively approach the burial ground of ancient warriors.  Do you want to "
    "search the graves?",
    "Upon defeating the ghosts you spend several hours searching the graves and find nothing.  "
    "Such a despicable act reduces your army's morale.",
    "Upon defeating the ghosts you search the graves and find something!",
    "Hut\n\nA group of goblins with a desire for greater glory wish to join you. Do you accept?",
    "You are unable to recruit at this time, your ranks are full.",
    "Hut\n\nAs you approach the goblin dwelling, you notice that there is no one here.",
    "Thatched Hut\n\nA group of peasants with a desire for greater glory wish to join you. Do you "
    "accept? ",
    "You are unable to recruit at this time, your ranks are full.",
    "Thatched Hut\n\nAs you approach the peasant dwelling, you notice that there is no one here.",
    "Cottage\n\nA group of archers with a desire for greater glory wish to join you. Do you "
    "accept? ",
    "You are unable to recruit at this time, your ranks are full.",
    "Cottage\n\nAs you approach the archer dwelling, you notice that there is no one here.",
    "Cottage\n\nA group of dwarves with a desire for greater glory wish to join you. Do you "
    "accept? ",
    "You are unable to recruit at this time, your ranks are full.",
    "Cottage\n\nAs you approach the dwarves' dwelling, you notice that there is no one here.",
    "Thatched Hut\n\nA group of peasants with a desire for greater glory wish to join you. Do you "
    "accept? ",
    "You are unable to recruit at this time, your ranks are full.",
    "Thatched Hut\n\nAs you approach the peasant dwelling you notice that there is no one here.",
    "Dragon City\n\nYou have reached Dragon City, famous for its wealth and danger. Do you wish to "
    "attack?",
    "You have conquered the mighty dragons. In homage to you, they offer 1000 gold a day to your "
    "cause, and will defend the city for you in case of attack.",
    "Lighthouse\n\nThe lighthouse is now under your control, and all of your ships will now move "
    "further each turn.",
    "Mill\n\nThe keeper of the mill announces: \"Milord, I am sorry, there is no gold currently "
    "available.  Please try again next week.\"",
    "Mill\n\nThe keeper of the mill announces: \"Milord, I have been working very hard to provide "
    "you with this gold, come back next week for more.\"",
    "Ore Mine\n\nYou gain control of an ore mine. It will provide you with two units of ore per "
    "day.",
    "Sulfur Mine\n\nYou gain control of a sulfur mine. It will provide you with one unit of sulfur "
    "per day.",
    "Crystal Mine\n\nYou gain control of a crystal mine. It will provide you with one unit of "
    "crystal per day.",
    "Gem Mine\n\nYou gain control of a gem mine. It will provide you with one unit of gems per "
    "day.",
    "Gold Mine\n\nYou gain control of a gold mine. It will provide you with 1000 gold per day.",
    "Followers\n\nA group of %s with a desire for greater glory wish to join you. Do you accept? ",
    "Insulted by your refusal of their offer, the monsters attack!",
    "Obelisk\n\nYou come upon an obelisk made from a type of stone you have never seen before.  "
    "Staring at it intensely, the smooth surface suddenly changes to an inscription.  The "
    "inscription is a piece of a lost ancient map.  Quickly you copy down the piece and the "
    "inscription vanishes as abruptly as it had appeared.",
    "Obelisk\n\nYou have already been to this obelisk.",
    "Oasis\n\nYou spot an oasis, but the well is dry and you depart empty-handed.",
    "Oasis\n\nA nomad merchant, traveling on foot, hails you and says his horse had spooked and "
    "left him stranded. For safe passage he takes your army to an oasis, raising your morale for "
    "one battle.",
    "You find a small quantity of %s.",
    "Sawmill\n\nYou gain control of a sawmill. It will provide you with two units of wood per day.",
    "Shrine\n\nWithin the ornate shrine sits a blind seer. After explaining the intent of your "
    "journey, the seer activates his crystal ball, allowing you to see the strengths and "
    "weaknesses of your opponents.",
    "Shrine\n\nNestled in a small hidden shrine is an ancient wooden altar.  Upon the altar, a "
    "golden plaque bears an inscription with the secret to the ancient magical spell ",
    "Shipwreck\n\nThe rotting hulk of a great pirate ship creaks eerily as it is pushed against "
    "the rocks.  Do you wish to search the shipwreck? ",
    "Upon defeating the ghosts you spend several hours sifting through the debris and find "
    "nothing.  Such a despicable act reduces your army's morale.",
    "Upon defeating the ghosts you sift through the debris and find something!",
    "Statue\n\nA large statue of an angel towers above you. Abruptly, the angel's eyes open and "
    "your troops celebrate a great increase in their morale.",
    "Statue\n\nA large statue of an angel towers above you. Your army encircles the statue, but "
    "there appears to be nothing special about it.",
    "Tents\n\nA group of tattered tents, billowing in the sandy wind, beckons you.  The tents are "
    "unoccupied.  Perhaps more nomads will be here later.",
    "Tents\n\nA group of tattered tents, billowing in the sandy wind, beckons you.  Do you wish to "
    "have any nomads join you during your travels?",
    "Wagon\n\nA colorful rogues' wagon stands empty here.  Perhaps more rogues will be here later.",
    "Wagon\n\nDistant sounds of music and laughter draw you to a colorful wagon housing rogues.  "
    "Do you wish to have any rogues join your army?",
    "Whirlpool\n\nA whirlpool engulfs your ship.  Some of your army has fallen overboard.",
    "Windmill\n\nThe keeper of the mill announces: \"Milord, I am sorry, there are no resources "
    "currently available. Please try again next week.\"",
    "Windmill\n\nThe keeper of the mill announces: \"Milord, I have been working very hard to "
    "provide you with these resources, come back next week for more.\"",
    "Artifact\n\nYou come upon an ancient artifact.  As you reach for it, a pack of Rogues leap "
    "out of the brush to guard their stolen loot.",
    "Artifact\n\nA leprechaun offers you the %s for the small price of 2000 gold.  Do you wish to "
    "buy this artifact?",
    "Insulted by your refusal of his generous offer, the leprechaun stamps his foot ferociously "
    "and vanishes.",
    "You try to pay the leprechaun, but realize that you don't have 2000 gold.  The leprechaun "
    "stamps his foot and ignores you.",
    "Upon defeating the Rogues, you search their corpses and discover the %s.",
    "Skeleton\n\nYou come upon the remains of an unfortunate adventurer.  Searching through the "
    "tattered clothing, you find nothing.",
    "Skeleton\n\nYou come upon the remains of an unfortunate adventurer.  Searching through the "
    "tattered clothing, you find",
};
DATA(0x00493730)
char* gAPanelHelp[5] = {
    "View the entire world.",
    "View the obelisk puzzle.",
    "Cast an adventure spell.",
    "Dig for the Ultimate Artifact.",
    "Exit this menu without doing anything.",
};
DATA(0x00493748)
char* gInitMenuHelp[5] = {
    "Start a single or multi-player game.",
    "Load a previously saved game.",
    "View the high score screen.",
    "View the credits screen.",
    "Quit Heroes of Might and Magic and return to the DOS prompt.",
};
DATA(0x00493760)
char* cAdvMenuHelp[6] = {
    "Next Hero\n\nSelect the next Hero.",
    "Continue Movement\n\nContinue the Hero's movement along his current path.",
    "Kingdom Summary\n\nView a summary of your kingdom.",
    "End Turn\n\nEnd your turn and let the computer take its turn.",
    "Adventure Options\n\nBring up the adventure options menu.",
    "Game Options\n\nBring up the game options menu.",
};
DATA(0x00493778)
char* gLuckText[7] = {"Cursed", "Awful", "Bad", "Normal", "Good", "Great", "Irish"};
DATA(0x00493798)
char* gMoraleText[7] = {"Treason", "Awful", "Poor", "Normal", "Good", "Great", "Blood!"};
DATA(0x004937b8)
char* onOffText[11] = {
    "Off",
    "On",
    "On\nVolume 9",
    "On\nVolume 8",
    "On\nVolume 7",
    "On\nVolume 6",
    "On\nVolume 5",
    "On\nVolume 4",
    "On\nVolume 3",
    "On\nVolume 2",
    "On\nVolume 1",
};
DATA(0x004937e8)
char* walkSpeedText[5] = {"Walk", "Trot", "Canter", "Gallop", "Jump"};
DATA(0x00493800)
char* gColorNames[4] = {"blue", "green", "red", "yellow"};
DATA(0x00493810)
char* gAlignmentNames[5] = {"human", "plains", "forest", "mountain", "neutral"};
DATA(0x00493828)
char* gSpellDesc[29] = {
    "Fireball\n\nCauses a giant fireball to strike the selected area, damaging all nearby "
    "creatures.",
    "Lightning Bolt\n\nCauses a bolt of electrical energy to strike the selected creature.",
    "Teleport\n\nTeleports the creature you select to any open position on the battlefield.",
    "Cure\n\nRemoves all negative spells cast upon your forces.",
    "Resurrect\n\nResurrects creatures from a damaged monster group.",
    "Haste\n\nIncreases the speed of any creature to 'very fast'.",
    "Slow\n\nSlows down even the fastest enemy creature.",
    "Blind\n\nClouds the affected creatures' eyes, preventing them from moving.",
    "Bless\n\nCauses the selected creatures to inflict maximum damage.",
    "Protection\n\nMagically increases the defense skill of the selected creatures.",
    "Curse\n\nCauses the selected creatures to inflict minimum damage.",
    "Turn Undead\n\nInstantly sends a group of ghosts back to the grave.",
    "Anti-Magic\n\nPrevents harmful magic against the selected creatures.",
    "Dispel Magic\n\nRemoves all magic spells from all parties in the battle.",
    "Berserk\n\nCauses a creature to attack its nearest neighbor.",
    "Armageddon\n\nHoly terror strikes the battlefield, causing severe damage to all creatures.",
    "Elemental Storm\n\nMagical elements pour down on the battlefield, damaging all creatures.",
    "Meteor Shower\n\nA rain of rocks strikes an area of the battlefield, damaging all nearby "
    "creatures.",
    "Paralyze\n\nThe targeted creatures are paralyzed, unable to move or retaliate.",
    "View Mines\n\nCauses all mines across the land to become visible.",
    "View Resources\n\nCauses all resources across the land to become visible.",
    "View Artifacts\n\nCauses all artifacts across the land to become visible.",
    "View Towns\n\nCauses all towns and castles across the land to become visible.",
    "View Heroes\n\nCauses all Heroes across the land to become visible.",
    "View All\n\nCauses the entire land to become visible.",
    "Identify Hero\n\nAllows the caster to view detailed information on enemy Heroes.",
    "Summon Boat\n\nSummons the nearest unoccupied, friendly boat to an adjacent shore location.  "
    "A friendly boat is one which you just built or were the most recent player to occupy.",
    "Dimension Door\n\nAllows the caster to magically transport himself to a nearby location.",
    "Town Gate\n\nReturns the caster to any town or castle currently owned.",
};
DATA(0x004938a0)
char* gMonthNames[10] = {
    "Grasshopper",
    "Ant",
    "Dragonfly",
    "Spider",
    "Butterfly",
    "Bumblebee",
    "Locust",
    "Earthworm",
    "Hornet",
    "Beetle",
};
DATA(0x004938c8)
char* gWeekNames[15] = {
    "Squirrel",
    "Rabbit",
    "Gopher",
    "Badger",
    "Rat",
    "Eagle",
    "Weasel",
    "Raven",
    "Mongoose",
    "Dog",
    "Aardvark",
    "Lizard",
    "Tortoise",
    "Hedgehog",
    "Condor",
};
DATA(0x00493908)
char* gDwellingDescriptions[24] = {
    "The Thatched Hut produces Peasants.",
    "The Archery Range produces Archers.",
    "The Blacksmith produces Pikemen.",
    "The Armory produces Swordsmen.",
    "The Jousting Arena produces Cavalries.",
    "The Cathedral produces Paladins.",
    "The Treehouse produces Sprites.",
    "The Cottage produces Dwarves.",
    "The Archery Range produces Elves.",
    "Stonehenge produces Druids.",
    "The Fenced Meadow produces Unicorns.",
    "The Red Tower produces Phoenix.",
    "The Hut produces Goblins.",
    "The Stick Hut produces Orcs.",
    "The Den produces Wolves.",
    "The Adobe produces Ogres.",
    "The Bridge produces Trolls.",
    "The Pyramid produces Cyclopes.",
    "The Cave produces Centaurs.",
    "The Crypt produces Gargoyles.",
    "The Nest produces Griffins.",
    "The Maze produces Minotaurs.",
    "The Swamp produces Hydras.",
    "The Black Tower produces Dragons.",
};
DATA(0x00493968)
char* gArmySizeNames[6][2] = {
    {"Few", "A few"},
    {"Several", "Several"},
    {"Pack", "A pack of"},
    {"Lots", "Lots of"},
    {"Horde", "A Horde of"},
    {"Zounds!", "Zounds..."},
};
DATA(0x00493998)
char* cHeroScreen[19] = {
    "Kingdom Overview",
    "View %s Info",
    "Additional hero characteristics",
    "View Good Morale Info",
    "View Neutral Morale Info",
    "View Bad Morale Info",
    "View Good Luck Info",
    "View Neutral Luck Info",
    "View Bad Luck Info",
    "View Experience Info",
    "Select %s",
    "Empty",
    "Move %s",
    "Exchange %s with %s",
    "View Spells",
    "View %s Info",
    "Dismiss %s the %s",
    "Exit Hero Screen",
    "Hero Screen",
};
DATA(0x004939e8)
char* cCastleInfo[14] = {
    "Build Mage Guild",
    "Mage Guild is at highest level.",
    "Cannot afford next level.",
    "Add another level to Mage Guild",
    "%s is already built",
    "Cannot build %s",
    "Cannot afford %s",
    "Build %s",
    "Cannot afford a Hero.",
    "Cannot recruit - you already have %d Heroes.",
    "Cannot recruit - you already have a Hero in this town.",
    "Recruit a new Hero",
    "Exit Castle",
    "Castle Options",
};
DATA(0x00493a20)
char* gLuckInfoText[12] = {
    "Good Luck\n\nGood luck sometimes lets your armies get lucky attacks (double strength) in "
    "combat.",
    "Neutral Luck\n\nNeutral luck means your armies will never get lucky or unlucky attacks on the "
    "enemy.",
    "Bad Luck\n\nBad luck sometimes falls on your armies in combat, causing their attacks to only "
    "do half damage.",
    "%s\n\n\nCurrent Luck Modifiers:",
    "\nLucky Rabbit's Foot +1",
    "\nGolden Horseshoe +1",
    "\nGambler's Lucky Coin +1",
    "\nFour-Leaf Clover +1",
    "\nFaerie ring visited +1",
    "\nFountain visited +1",
    "\nnone",
    0,
};
DATA(0x00493a50)
char* gcMemoryErrorTitle = "Out of Memory";
DATA(0x00493a54)
char* gcMemoryRequirements = "Heroes of Might and Magic requires approximately:";
DATA(0x00493a58)
char* gcExtendedMemoryUnits = "K extended or expanded memory (XMS or EMS) and";
DATA(0x00493a5c)
char* gcConventionalMemoryUnits = "K conventional memory";
DATA(0x00493a60)
char* gPlayerTypeNames[5] = {"None", "Dumb", "Average", "Smart", "Genius"};
DATA(0x00493a78)
char* cSpellHelp[8] = {
    "View previous page",
    "View next page",
    "View adventure spells",
    "View combat spells",
    "Close Spellbook",
    "View Spells",
    "Select Spell",
    "View Combat Spells",
};
DATA(0x00493a98)
char* gSpeedText[5] = {"", "Slow", "Medium", "Fast", "Blazing"};
DATA(0x00493ab0)
char* gArmyStatText[9] = {
    "Attack Skill: ",
    "Defense Skill: ",
    "Shots left: ",
    "Damage: ",
    "Hit Points: ",
    "Speed: ",
    "Morale: ",
    "Luck: ",
    "Shots: ",
};
DATA(0x00493ad8)
char* gOverviewText[3] = {
    "Kingdom Overview     Month %d, Week %d, Day %d",
    "You own Dragon City.",
    "You own the Lighthouse.",
};
DATA(0x00493ae8)
char* gNewTurnText[7] = {
    "%s player, you only have %d days left to capture a town, or you will be banished from this "
    "land.",
    "%s player, this is your last day to capture a town, or you will be banished from this land.",
    "Astrologers proclaim month of the %s.\n\nAll dwellings increase population.",
    "Astrologers proclaim month of the %s.\n\n%s population doubles!\n\nAll dwellings increase "
    "population.",
    "Astrologers proclaim month of the PLAGUE!\n\nAll populations are halved.",
    "Astrologers proclaim week of the %s.\n\nAll dwellings increase population.",
    "Astrologers proclaim week of the %s.\n\n%s growth +5.\n\nAll dwellings increase population.",
};
DATA(0x00493b08)
char* cViewGeneralLabels[6] =
    {"Attack: ", "Defense: ", "Spell Power: ", "Knowledge: ", "Morale: ", "Luck: "};
DATA(0x00493b20)
char* cViewGeneralHelp[6] = {
    "Stop Catapult",
    "Cast Spell",
    "Retreat",
    "Surrender",
    "Cancel",
    "General's Options",
};
DATA(0x00493b38)
char* cCombatMessage[9] = {
    "",
    "Move %s here.",
    "Fly %s here.",
    "Attack %s",
    "Shoot %s(%d shot%s left)",
    "General's Options",
    "View Opposing General",
    "View %s info.",
    "No shots left!",
};
DATA(0x00493b60)
char* cHeroLevel[3] = {"%s has gained", " a level.\n", " %d levels.\n"};
DATA(0x00493b70)
char* cCombatHelp[3] = {"Auto Combat", "Skip This Unit", ""};
DATA(0x00493b80)
char* cTownCommand[22] = {
    "Redistribute %s army",
    "Cannot combine Hero's last army",
    "Combine %s armies",
    "Redistribute %s army",
    "View %s",
    "Cannot move last army to garrison.",
    "Move %s",
    "Exchange %s with %s",
    "Exit town",
    "",
    "Kingdom Overview",
    "Empty",
    "Select %s",
    "View Hero",
    "Mage Guild",
    "Thieves' Guild",
    "Tavern",
    "Dock",
    "Well",
    "Tent",
    "Castle",
    "Recruit %s",
};
DATA(0x00493bd8)
char* gGameTypeHelp[5] = {
    "Play a single, standard game against computer opponents.",
    "Play the campaign game - a series of linked single games.",
    "Play against other human players, either sitting at the same computer, or linked through a "
    "network or modem.",
    "Play a practice game.",
    "Cancel out of this menu back to the main menu.",
};
DATA(0x00493bf0)
char* gHeroNames[36][2] = {
    {"Lord Kilburn", "Kilburn"},
    {"Lord Haart", "Haart"},
    {"Sir Gallant", "Gallant"},
    {"Arturius", "Arturius"},
    {"Tyro", "Tyro"},
    {"Maximus", "Maximus"},
    {"Ector", "Ector"},
    {"Dimitri", "Dimitri"},
    {"Ambrose", "Ambrose"},
    {"Thundax", "Thundax"},
    {"Ergon", "Ergon"},
    {"Kelzen", "Kelzen"},
    {"Tsabu", "Tsabu"},
    {"Crag Hack", "Crag"},
    {"Jojosh", "Jojosh"},
    {"Atlas", "Atlas"},
    {"Yog", "Yog"},
    {"Antoine", "Antoine"},
    {"Ariel", "Ariel"},
    {"Vatawna", "Vatawna"},
    {"Carlawn", "Carlawn"},
    {"Rebecca", "Rebecca"},
    {"Luna", "Luna"},
    {"Astra", "Astra"},
    {"Natasha", "Natasha"},
    {"Gem", "Gem"},
    {"Troyan", "Troyan"},
    {"Agar", "Agar"},
    {"Crodo", "Crodo"},
    {"Falagar", "Falagar"},
    {"Barok", "Barok"},
    {"Arie", "Arie"},
    {"Kastore", "Kastore"},
    {"Sandro", "Sandro"},
    {"Wrathmont", "Wrath"},
    {"Vesper", "Vesper"},
};
DATA(0x00493d10)
char* gCPanelHelp[12] = {
    "Start a single or multi-player game.",
    "Load a previously saved game.",
    "Quit Heroes of Might and Magic and return to the DOS prompt.",
    "Exit this menu without doing anything.",
    "Save the current game.",
    "Toggle ambient music on/off",
    "Toggle foreground sounds on/off",
    "Change the speed at which Heroes move on the main screen.",
    "Change the quality level of the sound.  CD stereo sounds the best, and usually is less of a "
    "drag on system performance, because no processing is required.  However, some systems may not "
    "be set up to handle CD stereo, so 8 bit sound is the fallback.",
    "Toggle 'Show Path' on/off.  If 'Show Path' is on, your first click on a map location will "
    "show the path to get there, your second will start you moving. If this option is off, one "
    "click starts you moving immediately.",
    "Toggle 'Show Enemy Moves' on/off.  If on, all enemies moving within your visible area will be "
    "shown.  If off, no computer movement will be shown.  Note that this option is automatically "
    "set to off during network and modem play.",
    "View information on the scenario you are currently playing.",
};
DATA(0x00493d40)
char* gNewGameHelp[9] = {
    "Accept these settings and start a new game.",
    "Return to the main menu.",
    "Challenge all computer players as 'King of the Hill'.  Computer players will be offended by "
    "your boastfulness, and lay off each other in an attempt to beat you to a pulp.",
    "Select which scenario to play.",
    "Change the starting difficulty at which you will play.  Higher difficulty levels start you "
    "off with fewer resources.",
    "Change the difficulty of this opponent.  Smarter computer players are more aggressive and "
    "think longer for each turn.",
    "Change your banner color.",
    "The difficulty rating reflects a combination of various settings for your game.  This number "
    "will be applied to your final score.",
    "Change the starting difficulty of another human player.  Higher difficulty levels start you "
    "off with fewer resources.",
};
DATA(0x00493d68)
char* gSetupCampaignGameHelp[5] = {
    "Play the role of Lord Ironfist.",
    "Play the role of Lord Slayer.",
    "Play the role of Queen Lamanda.",
    "Play the role of Lord Alamar.",
    "Cancel back to the main menu.",
};
DATA(0x00493d80)
char* gSetupBaudHelp[5] = {
    "Use a 2400 baud connection speed. \n\nNote: For a 14400 baud modem, use the 19200 baud speed. "
    " For a 28800 baud modem, use the 38400 baud speed.",
    "Use a 9600 baud connection speed. \n\nNote: For a 14400 baud modem, use the 19200 baud speed. "
    " For a 28800 baud modem, use the 38400 baud speed.",
    "Use a 19200 baud connection speed.\n\nNote: For a 14400 baud modem, use the 19200 baud speed. "
    " For a 28800 baud modem, use the 38400 baud speed.",
    "Use a 38400 baud connection speed.\n\nNote: For a 14400 baud modem, use the 19200 baud speed. "
    " For a 28800 baud modem, use the 38400 baud speed.",
    "Cancel back to the main menu.",
};
DATA(0x00493d98)
char* gSetupComPortHelp[5] = {
    "Use COM Port 1 for the modem connection.",
    "Use COM Port 2 for the modem connection.",
    "Use COM Port 3 for the modem connection.",
    "Use COM Port 4 for the modem connection.",
    "Cancel back to the main menu.",
};
DATA(0x00493db0)
char* gSetupDCBaudHelp[5] = {
    "Use a 2400 baud connection speed. \n\nNote: In general, computers with the older UART 8250 "
    "chip should use 19200 baud, and computers with the newer UART 16550 chip should use 38400 "
    "baud.  When in doubt, try slower speeds first, and if they work, then try faster speeds.  "
    "Most computers made in 1994 or later have a UART 16550 chip.",
    "Use a 9600 baud connection speed. \n\nNote: In general, computers with the older UART 8250 "
    "chip should use 19200 baud, and computers with the newer UART 16550 chip should use 38400 "
    "baud.  When in doubt, try slower speeds first, and if they work, then try faster speeds.  "
    "Most computers made in 1994 or later have a UART 16550 chip.",
    "Use a 19200 baud connection speed.\n\nNote: In general, computers with the older UART 8250 "
    "chip should use 19200 baud, and computers with the newer UART 16550 chip should use 38400 "
    "baud.  When in doubt, try slower speeds first, and if they work, then try faster speeds.  "
    "Most computers made in 1994 or later have a UART 16550 chip.",
    "Use a 38400 baud connection speed.\n\nNote: In general, computers with the older UART 8250 "
    "chip should use 19200 baud, and computers with the newer UART 16550 chip should use 38400 "
    "baud.  When in doubt, try slower speeds first, and if they work, then try faster speeds.  "
    "Most computers made in 1994 or later have a UART 16550 chip.",
    "Cancel back to the main menu.",
};
DATA(0x00493dc8)
char* gSetupDCComPortHelp[5] = {
    "Use COM Port 1 for the direct connection.",
    "Use COM Port 2 for the direct connection.",
    "Use COM Port 3 for the direct connection.",
    "Use COM Port 4 for the direct connection.",
    "Cancel back to the main menu.",
};
DATA(0x00493de0)
char* gSetupHotSeatGameHelp[4] = {
    "Play with 2 human players, and optionally, up to 2 additional computer players.",
    "Play with 3 human players, and optionally 1 computer player.",
    "Play with 4 human players.",
    "Cancel back to the main menu.",
};
DATA(0x00493df0)
char* gSetupModemGameHelp[4] = {
    "The host sets up the game options, chooses the number to dial, and places the call.",
    "The guest waits for the host to call and set up the game.",
    "Change your modem configuration.",
    "Cancel back to the main menu.",
};
DATA(0x00493e00)
char* gSetupDCGameHelp[4] = {
    "The host sets up the game options.",
    "The guest waits for the host to set up the game.",
    "Change your direct connect port configuration.",
    "Cancel back to the main menu.",
};
DATA(0x00493e10)
char* gSetupMultiPlayerGameHelp[5] = {
    "Play a Hot Seat game, where 2 to 4 players play around the same computer, switching into the "
    "'Hot Seat' when it is their turn.",
    "Play a network game, where 2 players use their own computers connected through a LAN (Local "
    "Area Network).",
    "Play a modem game, where 2 players use ther own computers connected over the phone lines "
    "using modems.",
    "Play a direct connect game, where 2 players use ther own computers directly connected through "
    "their serial port by a null modem.",
    "Cancel back to the main menu.",
};
DATA(0x00493e28)
char* gSetupNetworkGameHelp[3] = {
    "The host sets up the game options.  There can only be one host per network game.",
    "The guest waits for the host to set up the game, then is automatically added in.  There can "
    "only be one guest per network game.",
    "Cancel back to the main menu.",
};
DATA(0x00493e38)
char* gSetupGameHelp[4] = {
    "A single player game playing out a single map.",
    "A single player game playing through a series of maps.",
    "A multi-player game, with several human players competing against each other on a single map.",
    "Cancel back to the main menu.",
};
DATA(0x00493e48)
char* cBattleResults[11] = {
    "The enemy has surrendered!",
    "The enemy has fled!",
    "A glorious victory!",
    "\n\nFor valor in combat, %s receives %d experience",
    "%s surrenders to the enemy, and departs in shame.",
    "The cowardly %s flees from battle.",
    "Your forces suffer a bitter defeat, and %s abandons your cause.",
    "Your forces surrender to the enemy, and depart in shame.",
    "Your cowardly forces flee from battle.",
    "Your forces suffer a bitter defeat.",
    "\n\nFor valor in combat, %s receives %d experience, and gains %d level(s).",
};
DATA(0x00493e78)
char* gNeutralBuildingDescriptions[7] = {
    "The Mage Guild allows heroes to learn and replenish spells.",
    "The Thieves' Guild provides information on enemy players.  Thieves' Guilds can also provide "
    "scouting information on enemy towns.  Additional Guilds provide more information.",
    "The Tavern increases morale for troops defending the castle.",
    "The Shipyard allows ships to be built.",
    "The Well increases the growth rate of all dwellings by 2 creatures per week.",
    "The Tent provides workers to build a castle.",
    "The Castle improves town defense and income.",
};
DATA(0x00493e98)
char* gMoraleInfoText[21] = {
    "Good Morale\n\nGood morale may give your armies extra attacks in combat.",
    "Neutral Morale\n\nNeutral morale means your armies will never be blessed with extra attacks "
    "or freeze in combat.",
    "Bad Morale\n\nBad morale may cause your armies to freeze in combat.",
    "%s\n\n\nCurrent Morale Modifiers:",
    "\nKnight bonus +1",
    "\nAll %s troops +1",
    "\nTroops of 3 alignments -1",
    "\nTroops of 4 alignments -2",
    "\nMedal of Valor +1",
    "\nMedal of Courage +1",
    "\nMedal of Honor +1",
    "\nMedal of Distinction +1",
    "\nFizbin of Misfortune -2",
    "\nBuoy visited +1",
    "\nOasis visited +1",
    "\nStatue visited +2",
    "\nGraveyard robber -1",
    "\nShipwreck robber -1",
    "\nBattle cowardice %d",
    "\nnone",
    "\nTroops of 5 alignments -3",
};
DATA(0x00493ef0)
char* gMapSizeNames[3] = {"Small", "Medium", "Large"};
DATA(0x00493f00)
char* gMapDifficultyNames[5] = {"Easy", "Normal", "Tough", "Impossible", "Forget It"};
DATA(0x00493f18)
char* gCampaignScenarioNames[9] = {
    "You have established a foothold in the new land.  This small island is fiercely contested by "
    "three other factions, all vying to capture the strategic town - Gateway.  The town is located "
    "in the center of the island, and so dominates its surroundings that the other factions will "
    "surrender to the lord that captures it.  Beware the dragon guardian of Gateway!",
    "Your way to the mainland is blocked by the Archipelago of the Ancients, a series of four "
    "large islands, each held by a different lord.  The opposition must all be subdued, and they "
    "are better led this time.  Boats are a necessity - use them wisely!",
    "Chaos.  A maelstrom of combat plagues the land.  The people suffer, but will rally behind the "
    "wielder of the Eye of Goros, an artifact that can heal the wounded land.  It was buried and "
    "lost eons ago.  The first lord to uncover the Eye will unite the people and conquer the land "
    "- but it lies in a vast territory with only pieces of a puzzle to guide the way.",
    "With the founding of a homeland, the other lords now take you seriously.  All seek to "
    "dominate the central continent, and each is suspicious of the others.  The territory is huge, "
    "the opposition distant.  Resources are scarce and should be fiercely defended.  You must be "
    "the last lord left to claim victory.",
    "The land of the knights, led by Lord Ironfist, is divided by the twisting Floodwater River.  "
    "Ironfist is counting on the river to protect him.  The only town suitable to boat-building "
    "lies upon the river far to the east.  To defeat Ironfist, you must capture his home castle in "
    "the far northwest.",
    "Far to the north, beyond the Trackless Desert, lies the Frozen Wastes.  It is the homeland of "
    "Lord Slayer and his barbarian followers.  Once the mountain pass has been breached by either "
    "side, barbarian raiders will stream south.  The desert may harbor unknown allies who can aid "
    "you.  Slayer's castle lies just northeast of the pass.",
    "Warned of your approach, the sorceress Queen Lamanda worked on a dreadful magic and sank the "
    "approach to the only port.  You must find the teleport gate to assault the southwestern land "
    "and capture the port.  The only landfall is far to the northeast.  From there you must "
    "struggle through the forest maze to locate her castle in the extreme northwest.",
    "The warlocks' castle lies shrouded in the smokey volcanic rift.  To reach Lord Alamar's home "
    "castle in the extreme southeast, you must wander through the Minotaur Maze.  The warlocks are "
    "overconfident and not expecting an attack, so sure are they that none can navigate the maze.  "
    "Gargoyles have been set to dissuade invaders from the true path.",
    "Final victory lies within your grasp - but the defeated warlords have pooled their last "
    "resources and have banded together against you.  If you can bend the dragons to your will and "
    "force them to side with you, all the other warlords will submit and the land will be yours to "
    "rule.  Capture the Dragon Citadel on the central island and victory is yours!",
};
DATA(0x00493f40)
char* gCampaignWinTexts[9] = {
    "Gateway has fallen!  The other lords have abandoned their castles and fled.  They have "
    "alerted their homelands and now gather their forces.  Speed is of the essence.",
    "The Archipelago of the Ancients has been subdued and added to your domain.  On the horizon "
    "lies a vast, unexplored - and hostile - continent.",
    "The healing power of the Eye of Goros spreads throughout the land.  The population unites "
    "behind you and the other lords retreat.  The war for domination begins.",
    "With your victory, the other lords have made their final retreat.  They must each in turn be "
    "fought one-on-one in their homelands, and their personal castles must be captured.",
    "The knights are broken in battle!  You have conquered their homeland.",
    "The barbarian castle has been overthrown and their army scattered!",
    "You have burst through the forest maze and destroyed Lamanda's castle.",
    "You have followed the gargoyles to the castle of Lord Alamar and have shattered the might of "
    "the warlocks.",
    "The dragons join your cause and the competing warlords capitulate.  You now rule a vast and "
    "united land as the one true King!",
};
DATA(0x00493f68)
char* gCampaignScenarioText[9] = {
    "Gateway",
    "The Archipelago",
    "The Wounded Land",
    "Free-for-All",
    "Castle Ironfist",
    "Castle Slayer",
    "Castle Lamanda",
    "Castle Alamar",
    "King-of-the-Hill",
};
DATA(0x00493f90)
char* gDifficultyNames[4] = {"Easy", "Normal", "Hard", "Expert"};
DATA(0x00493fa0)
char* gCampaignSideNames[4] = {"Lord Ironfist", "Lord Slayer", "Queen Lamanda", "Lord Alamar"};
DATA(0x00493fb0)
char* gScoreLabels[CONGRATS_SCORE_LABEL_COUNT] =
    {"Days Spent:", "Base Score:", "Difficulty Rating:", "Final Score:", "Ranking:"};
DATA(0x00493fc8)
char* gHumanPlayerTypeNames[5] =
    {"Human\n", "Human\nEasy", "Human\nNormal", "Human\nHard", "Human\nExpert"};
DATA(0x00493fe0)
char* gHandicapNames[5] = {"Human-", "Human-Easy", "Human-Normal", "Human-Hard", "Human-Expert"};
DATA(0x00493ff8)
char* musicQualityText[3] = {"8 Bit Mono", "8 Bit Stereo", "CD Stereo"};
DATA(0x00494008)
char* gWinSetupText[68] = {
    "Buy Spellbook:",
    "Resource cost:",
    "Build improvement:",
    "Castle Options:  Town Improvements/Recruit Hero",
    "Mage Guild",
    "Thieves' Guild",
    "Tavern",
    "Shipyard",
    "Well",
    "Recruit Hero",
    "Music",
    "Effects",
    "Sound\nQuality",
    "Speed",
    "Show Path",
    "View Enemy\nMovement",
    "Dimension Door:\nSelect Destination",
    "Attack Skill",
    "Defense Skill",
    "Spell Power",
    "Knowledge",
    "The above spells have been added to your book.",
    "Choose Game Difficulty:",
    "Easy",
    "Normal",
    "Hard",
    "Expert",
    "Customize Opponents:",
    "Normal",
    "Normal",
    "Normal",
    "Choose Color:",
    "King of the Hill:",
    "Choose Scenario:",
    "Heroes",
    "Castles",
    "Towns",
    "Mines",
    "Treasury",
    "Total Gold Per Day:",
    "Attack:",
    "Defense:",
    "Spell Power:",
    "Knowledge:",
    "Defenders:",
    "Recruit Hero",
    "Build a new ship:",
    "Resource cost:",
    "Attack Skill",
    "Defense Skill",
    "Spell Power",
    "Knowledge  ",
    "Tavern",
    "The tavern increases the morale of all garrisoned troops.",
    "Thieves' Guild: Player Rankings",
    "First",
    "Second",
    "Third",
    "Fourth",
    "Number of Towns:",
    "Number of Castles:",
    "Number of Heroes:",
    "Gold in Treasury:",
    "Wood, Crystal & Ore:",
    "Gems, Sulfur & Mercury:",
    "Number Obelisks Found:",
    "Total Army Strength:",
    "World Map",
};
DATA(0x00494118)
int giRequiredExtendedMemory = 4434;
DATA(0x0049411c)
int giRequiredConventionalMemory = 374;
DATA(0x00494120)
int giMapSize = 0;
DATA(0x00494124)
int giMapDifficulty = 0;
DATA(0x00494128)
signed char gbHeroWindShowing = 0;
DATA(0x0049412c)
signed char gbOverviewShowing = 0;
DATA(0x00494130)
int gbFullCombatScreenDrawn = 1;
DATA(0x00494134)
int gbLimitedCombatUpdatePalette = 0;
DATA(0x00494138)
signed char gbFirstTimeThrough = 0;
DATA(0x0049413c)
signed char gbSkipIntro = 0;
DATA(0x00494140)
int gbAllBlack = 0;
DATA(0x00494144)
signed char gbInCombat = 0;
DATA(0x00494148)
signed char gbDirectConnect = 0;
DATA(0x0049414c)
long giForceSwitchMusic = -1;
DATA(0x00494150)
int gbComputeExtent = 0;
DATA(0x00494154)
int gbSaveBiggestExtent = 0;
DATA(0x00494158)
int gbLimitToExtent = 0;
DATA(0x0049415c)
int gbCurrArmyDrawn = 1;
DATA(0x00494160)
int gAdvDisposeLevel = 0;
DATA(0x00494164)
int gbRemoteOn = 0;
DATA(0x00494168)
signed char gbGameInitialized = 0;
DATA(0x0049416c)
signed char giHighScoreRank = -1;
DATA(0x00494170)
signed char gbShowHighScore = 0;
DATA(0x00494174)
int giHighMemBuffer = 4000;
DATA(0x00494180)
char gbInPollSound = 0;
// Retail places these zero-initialized flags among KB's function literals
// (0x0049ea98-0x0049f537), each next to the literals of its only user.
DATA(0x0049ea98)
signed char bEarlySetupDone = 0;
DATA(0x0049ea9c)
signed char bKBDone = 0;
DATA(0x0049f040)
signed char bInCheckEndGame = 0;
DATA(0x0049f468)
int bInShutDown = 0;
DATA(0x0049f534)
signed char gbInMemError = 0;
// KB owns retail .bss 0x004c5138-0x004c7e6f (allocation order is the compiler's
// symbol-hash walk, not definition order).
#include <SOURCE/combatTypes.h>
#include <SOURCE/mapCell.h>
DATA(0x004c5138)
int gbHumanPlayer[4];
DATA(0x004c5148)
int giMaxExtentX;
DATA(0x004c514c)
int giMaxExtentY;
DATA(0x004c5150)
class font* smallFont;
DATA(0x004c5158)
long giBottomViewOverrideEndTime;
DATA(0x004c5160)
signed char gArmyEffected[COMBAT_SIDE_COUNT][ARMY_GROUP_SLOT_COUNT];
DATA(0x004c516c)
int giBottomViewResource;
DATA(0x004c5170)
int giSeedingValid;
DATA(0x004c5174)
signed char giLimitPlayer;
DATA(0x004c517c)
inputManager* gpInputManager;
DATA(0x004c5180)
SAMPLE2 NULL_SAMPLE2;
DATA(0x004c5188)
int iMaxMapExtra;
DATA(0x004c518c)
palette* gPalette;
DATA(0x004c5190)
resourceManager* gpResourceManager;
DATA(0x004c5230)
unsigned char mapExtra[MAP_CELL_GRID_SIZE][MAP_CELL_GRID_SIZE];
DATA(0x004c6670)
int bSpecialHideCursor;
DATA(0x004c6674)
class searchArray* gpSearchArray;
DATA(0x004c6678)
int gbBlackoutPlayer;
DATA(0x004c6680)
char cNetBoxLine[2][60];
DATA(0x004c66fc)
heroWindow* DataEntryWin;
DATA(0x004c6700)
signed char giWeekTypeExtra;
DATA(0x004c6704)
philAI* gpPhilAI;
DATA(0x004c6708)
char* cDEDest;
DATA(0x004c670c)
heroWindow* pNormalDialogWindow;
DATA(0x004c6710)
int giHostGamePos;
DATA(0x004c6714)
mouseManager* gpMouseManager;
DATA(0x004c6718)
class font* bigFont;
DATA(0x004c671c)
class icon* gSystemIcons;
DATA(0x004c6720)
signed char gbCombatSurrender;
DATA(0x004c6728)
char gMapName[16];
DATA(0x004c6738)
int giMinExtentX;
DATA(0x004c673c)
int giMinExtentY;
DATA(0x004c6740)
signed char iMPBaseType;
DATA(0x004c6744)
class hero* gpHVHero;
DATA(0x004c6748)
int giHeroScreenSrcIndex;
DATA(0x004c674c)
signed char giWeekType;
DATA(0x004c6750)
char gText[768];
DATA(0x004c6a50)
int gbInNewGameSetup;
DATA(0x004c6a54)
palette* gpBufferPalette;
DATA(0x004c6a58)
signed char giMonthTypeExtra;
DATA(0x004c6a5c)
signed char iMPExtendedType;
DATA(0x004c6a60)
smackManager* gpSmackManager;
DATA(0x004c6a68)
char gFullMapName[20];
DATA(0x004c6a7c)
int giShowIntro;
DATA(0x004c6a80)
int glTimers[2];
DATA(0x004c6a90)
long gMusicFadeTimer;
DATA(0x004c6a94)
long gNextSoundPollTick;
DATA(0x004c6a98)
int giScore;
DATA(0x004c6aa0)
armyGroup* gpMonGroup;
DATA(0x004c6aa8)
configStruct gConfig;
DATA(0x004c6be0)
char gcRegAppPath[352];
DATA(0x004c6d44)
signed char giCampaignChoice;
DATA(0x004c6d48)
class game* gpGame;
DATA(0x004c6d4c)
signed char gbRetreatWin;
DATA(0x004c6d60)
H1_ENUM_STORAGE(DialogWaitType, signed char) giWaitType;
DATA(0x004c6d64)
short gCurLoadedSpellFileId;
DATA(0x004c6d68)
int giBottomViewOverride;
DATA(0x004c6d70)
char gLastFilename[352];
DATA(0x004c6ed0)
class icon* gBuyBuildIcons;
DATA(0x004c6ed4)
char gbNoSound;
DATA(0x004c6ed8)
char gcBottomViewText[92];
DATA(0x004c6f34)
int giThisNetPos;
DATA(0x004c6f38)
char gcRegCDRomPath[352];
DATA(0x004c7098)
class heroWindow* heroWin;
DATA(0x004c709c)
class icon* gCurLoadedSpellIcon;
DATA(0x004c70a0)
void* ppMapExtra[255];
DATA(0x004c749c)
int giCurGeneral;
DATA(0x004c74a0)
int giThisGamePos;
DATA(0x004c74a4)
int giNumHumanPlayers;
DATA(0x004c74a8)
signed char gbIconClipOn;
DATA(0x004c74b0)
int pwSizeOfMapExtra[255];
DATA(0x004c78ac)
int iDEMaxLen;
DATA(0x004c78b0)
class combatManager* gpCombatManager;
DATA(0x004c78b4)
short giSpellEffectFrame;
DATA(0x004c78b8)
executive* gpExec;
DATA(0x004c78c0)
signed char giGroundToTerrain[140];
DATA(0x004c7950)
long giCurWindowsStyleFlags;
DATA(0x004c7954)
H1_ENUM_STORAGE(MainMenuControl, short) gGameCommand;
DATA(0x004c7958)
signed char giMonthType;
DATA(0x004c7960)
char gMapDescription[124];
DATA(0x004c79dc)
char* DEFAULT_AGGREGATE_NAME;
DATA(0x004c79e0)
signed char gbThisNetHumanPlayer[4];
DATA(0x004c79e8)
char cAggPathName[352];
DATA(0x004c7b48)
class highScoreManager* gpHighScoreManager;
DATA(0x004c7b4c)
signed char gbFunctionComplete;
DATA(0x004c7b50)
signed char gbIAmGreatest;
DATA(0x004c7b58)
short gMapX;
DATA(0x004c7b5c)
short gMapY;
DATA(0x004c7b60)
char gcWinText[300];
DATA(0x004c7c8c)
signed char bDataEntryTime;
DATA(0x004c7c90)
int bShowIt;
DATA(0x004c7c94)
int giDebugLevel;
DATA(0x004c7c9c)
heroWindowManager* gpWindowManager;
DATA(0x004c7ca0)
int giCurWatchPlayer;
DATA(0x004c7ca4)
int giBottomViewResourceQty;
DATA(0x004c7ca8)
soundManager* gpSoundManager;
DATA(0x004c7cac)
signed char gbWaitForRemoteReceive;
DATA(0x004c7cb0)
char gLastMapName[352];
DATA(0x004c7e10)
townManager* gpTownManager;
DATA(0x004c7e14)
signed char giScreenScroll;
DATA(0x004c7e18)
advManager* gpAdvManager;
DATA(0x004c7e1c)
signed char gbGamePosToNetPos[4];

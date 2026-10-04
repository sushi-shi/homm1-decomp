// Located from HoMM2 Buka 2.1; PoL 2.0 supplies the VC4 declaration.

#include <match.h>

#include <BASE/BITS.h>
#include <BASE/BMAP2.h>
#include <BASE/Misc.h>
#include <BASE/baseManager.h>
#include <BASE/executive.h>
#include <BASE/font.h>
#include <BASE/heroWindow.h>
#include <BASE/heroWindowManager.h>
#include <BASE/icon.h>
#include <BASE/iconWidget.h>
#include <BASE/inputManager.h>
#include <BASE/message.h>
#include <BASE/mouseManager.h>
#include <BASE/palette.h>
#include <BASE/resourceManager.h>
#include <BASE/sample.h>
#include <BASE/audio.h>
#include <BASE/textWidget.h>
#include <BASE/widget.h>
#include <SOURCE/advManager.h>
#include <SOURCE/army.h>
#include <SOURCE/armyGroup.h>
#include <SOURCE/combatManager.h>
#include <SOURCE/fileRequester.h>
#include <SOURCE/game.h>
#include <SOURCE/hero.h>
#include <SOURCE/highScoreManager.h>
#include <SOURCE/philAI.h>
#include <SOURCE/playerData.h>
#include <SOURCE/searchArray.h>
#include <SOURCE/town.h>
#include <SOURCE/townManager.h>
#include <SOURCE/KB.h>
#include <BASE/miscwin.h>
#include <BASE/soundmgr.h>
#include <SOURCE/appMenu.h>
#include <SOURCE/artifactTypes.h>
#include <SOURCE/campaignTypes.h>
#include <SOURCE/creatureTypes.h>
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

// Retail score-dialog owner byte (.bss).
DATA(0x004a7d4c)
i8 giHighScoreType;
// InitVars proves seven terrain rows, ordinary/diagonal cost columns.
DATA(0x004a7b98)
i8 giTerrainCost[FINDPATH_TERRAIN_COUNT][FINDPATH_STEP_COST_COUNT];

// HoMM2 KB.cpp confirms the identity and behavior. HoMM1 differs in the timer
// comparison and placement of the re-entry guard.
VA(0x0043c7b0, 0x41)
void PollSound() {
    if (KBTickCount() < glTimers[GLOBAL_POLL_SOUND_TIMER_SLOT])
        return;
    if (gInPollSound)
        return;
    gInPollSound = 1;
    glTimers[GLOBAL_POLL_SOUND_TIMER_SLOT] = KBTickCount() + 30;
    PollRemote();
    gInPollSound = 0;
}

VA(0x00420c42, 0x20)
void ForcePollSound() {
    glTimers[GLOBAL_POLL_SOUND_TIMER_SLOT] = KBTickCount() - 1;
    PollSound();
}

// donor PoL RVA 0x000965be; preferred Buka symbol ?InitMainClasses@@YIXXZ
// donor Buka TU SOURCE/KB; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.512387;margin=0.755802;shape=0.400;size=0.925;calls=0.653;alternate=pol20:void InitMainClasses(void)@0x000965be
VA(0x0043c808, 0x518)
void InitMainClasses(void) {
    gpExec = new executive;
    gpInputManager = new inputManager;
    gpMouseManager = new mouseManager;
    gpWindowManager = new heroWindowManager;
    gpResourceManager = new resourceManager;
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

// Buka frees the resource manager before the window, mouse and input managers.
VA(0x0043cd20, 0x2ab)
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
VA(0x004215d6, 0x116)
i32 EarlySetup(void) {
    DATA(0x0049e8b0)
    static i8 gEarlySetupDone = 0;
    i32 iCDRomErr;

    if (gEarlySetupDone)
        return 0;
    InitMainClasses();
    GetGraphicsInfo();
    ReadPrefs();
    if (!InterpretCommandLine())
        return 1;
    LogTruncate();
    iCDRomErr = SetupCDDrive();
    if (iCDRomErr == CD_SETUP_NO_DRIVE) {
        MessageBoxA(
            static_cast<HWND>(hwndApp),
            "Unable to access CD Drive.",
            "Startup Error",
            MB_ICONHAND
        );
        exit(0);
    }
    if (iCDRomErr == CD_SETUP_NOT_FOUND) {
        MessageBoxA(
            static_cast<HWND>(hwndApp),
            "You must have the Heroes Win95 CD in the CD-ROM drive to play \nHeroes of "
            "Might and Magic.  \n\nPlease insert the CD and try again.",
            "Startup Error",
            MB_ICONHAND
        );
        exit(0);
    }
    if (iCDRomErr == CD_SETUP_NO_APP_PATH) {
        MessageBoxA(
            static_cast<HWND>(hwndApp),
            "Unable to change to the Heroes directory.  Please run the installation "
            "program.",
            "Startup Error",
            MB_ICONHAND
        );
        exit(0);
    }
    if (iCDRomErr == CD_SETUP_NO_DATA) {
        MessageBoxA(
            static_cast<HWND>(hwndApp),
            "Unable to find the Heroes data files.  Please run the installation program.",
            "Startup Error",
            MB_ICONHAND
        );
        exit(0);
    }
    InitVars();
    return 1;
}

// InitMenuHandler's right-click help: the gInitMenuHelp row.
H1_ENUM_BEGIN(MainMenuHelp)
    MAIN_MENU_HELP_NONE = -1,
    MAIN_MENU_HELP_NEW_GAME = 0,
    MAIN_MENU_HELP_LOAD_GAME = 1,
    MAIN_MENU_HELP_HIGH_SCORES = 2,
    MAIN_MENU_HELP_CREDITS = 3,
    MAIN_MENU_HELP_QUIT = 4
H1_ENUM_END(MainMenuHelp)

// gEndSequence: CheckEndGame sets LOST/WON, and WON becomes CAMPAIGN_COMPLETE
// after the last campaign scenario; oldmain plays the matching video (the
// value indexes endVideos), offers a replay after LOST and
// advances the campaign after WON.
H1_ENUM_BEGIN(GameEndSequence)
    GAME_END_LOST = 0,
    GAME_END_WON = 1,
    GAME_END_CAMPAIGN_COMPLETE = 2,
    GAME_END_SEQUENCE_COUNT = 3
H1_ENUM_END(GameEndSequence)

// Network positions (gbGamePosToNetPos, giThisNetPos): the host is
// position HOST; a game position with no network player maps to NONE.
H1_ENUM_CONST_BEGIN(NetPositionConstant)
    NET_POSITION_NONE = -1,
    NET_POSITION_HOST = 0
H1_ENUM_CONST_END(NetPositionConstant)

// Buka 2.1 oldmain reduced to HoMM1: two intro videos, the stpmain.bin
// menu (new, load, campaign, high scores, credits, quit), one network
// handshake and the campaign replay/next-scenario loop.
VA(0x004216ec, 0xe23)
i32 oldmain(void) {
    char saveBuf[20];
    H1_ENUM_STORAGE(SmackVideo, char) endVideos[GAME_END_SEQUENCE_COUNT];
    i32 n;
    heroWindow* mainWin;
    font* font;
    i8 backdropLoaded;
    i8 initialMainScreen;
    i32 idx;
    i8 done;
    i8 leave;
    i32 result;
    i16 command;

    if (gKBDone)
        return 0;
    gKBDone = 1;
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
            "Loading Heroes of Might and Magic for Windows 95 (version 1.1)",
            10,
            10,
            1
        );
        gpWindowManager->UpdateScreenRegion(10, 10, 600, 20);
        gpResourceManager->Dispose(font);
        if (!gSkipIntro && PlaySmacker(SMACK_BUKA) && PlaySmacker(SMACK_NWCLOGO))
            PlaySmacker(SMACK_INTRO);
    }
    LoadSystemwideIcons();
    memset(gbThisNetHumanPlayer, 0, GAME_PLAYER_COUNT);
    leave = 0;
    backdropLoaded = 0;
    initialMainScreen = 1;

    while (!leave) {
    mainMenu:
        PlayMusic(MUSIC_TRACK_MAIN_MENU);
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
        gCampaignChoice = 0;
        gpMouseManager->ReallyShowPointer();

        if (gMenuCommand != APP_MENU_NONE) {
        processMenuCommand:
            switch (gMenuCommand) {
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
                gInSetupDialog = 1;
                gpWindowManager->DoDialog(mainWin, InitMenuHandler, 0);
                delete mainWin;
                command = gpWindowManager->m_dialogResult;
                gInSetupDialog = 0;
            }
        }
        if (gMenuCommand != APP_MENU_NONE)
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
        if (gMenuCommand != APP_MENU_NONE)
            goto processMenuCommand;
        if (!leave) {
            if (gRemoteOn && !giThisNetPos) {
                n = 0;
                for (idx = 0; idx < GAME_PLAYER_COUNT; idx++) {
                    if (gbHumanPlayer[idx]) {
                        gbGamePosToNetPos[idx] = n;
                        n++;
                    } else {
                        gbGamePosToNetPos[idx] = NET_POSITION_NONE;
                    }
                }
                for (idx = 0; idx < GAME_PLAYER_COUNT; idx++)
                    memcpy(gText, gbGamePosToNetPos, GAME_PLAYER_COUNT);
                giHostGamePos = NetPosToGamePos(NET_POSITION_HOST);
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
            if (gRemoteOn && gbWaitForRemoteReceive) {
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
            gGameInitialized = 1;
            backdropLoaded = 0;
            StopAllSamples();
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

        if (gGameOver) {
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
            endVideos[GAME_END_LOST] = SMACK_LOSE;
            endVideos[GAME_END_WON] = SMACK_WIN1;
            endVideos[GAME_END_CAMPAIGN_COMPLETE] = SMACK_WIN2;
            if (gEndSequence != GAME_END_WON) {
                if (gEndSequence == GAME_END_CAMPAIGN_COMPLETE) {
                    PlaySmacker(SMACK_WIN1);
                    PlaySmacker(SMACK_WIN2);
                } else {
                    PlaySmacker(endVideos[gEndSequence]);
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
            gGameOver = 0;
            if (gEndSequence == GAME_END_CAMPAIGN_COMPLETE) {
                PlayMusic(MUSIC_TRACK_CONGRATULATIONS);
                AddScoreToHighScore(
                    giCurTurn,
                    HIGH_SCORE_TYPE_CAMPAIGN,
                    "",
                    gCampaignSideNames[gpGame->m_campaignType - CAMPAIGN_IRONFIST]
                );
            }
            if (gShowHighScore) {
                gpMouseManager->ReallyShowPointer();
                if (gpExec->AddManager(gpHighScoreManager, BASE_MANAGER_PRIORITY_UNASSIGNED))
                    ShutDown("Can't add manager!");
                gpExec->MainLoop();
                gpExec->RemoveManager(gpHighScoreManager);
                gHighScoreRank = HIGH_SCORE_EMPTY;
                PlayMusic(MUSIC_TRACK_MAIN_MENU);
                gpResourceManager->GetBackdrop("heroes.bmp", gpWindowManager->m_screen);
                gpWindowManager
                    ->UpdateScreenRegion(0, 0, LOGICAL_SCREEN_WIDTH, LOGICAL_SCREEN_HEIGHT);
                gpWindowManager->FadeScreen(WINDOW_FADE_IN, WINDOW_FADE_STEPS_SHORT, gPalette);
                backdropLoaded = 1;
            }
            if (gpGame->m_campaignType > 0) {
                if (gEndSequence == GAME_END_LOST) {
                    sprintf(gText, "Would you like to replay this scenario?");
                    NormalDialog(gText, NORMAL_DIALOG_TYPE_YES_NO);
                    if (gpWindowManager->m_dialogResult == NORMAL_DIALOG_CONFIRM) {
                        gpGame->InitCampaignMap(gpGame->m_campaignScenario, 0);
                        goto playScenario;
                    }
                } else if (gEndSequence == GAME_END_WON) {
                    gpGame->m_campaignDay = giCurTurn + 1;
                    gpGame->m_campaignScenario++;
                    gpGame->m_campaignScenariosWon++;
                    if (gpGame->m_campaignScenario - CAMPAIGN_SCENARIO_LORD_FIRST
                        == gpGame->m_campaignType - CAMPAIGN_IRONFIST)
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
        if (gRemoteOn)
            leave = 1;
    }
    ShutDown(NULL);
    return 0;
}

// Buka 2.1 toupper; HoMM1 keeps the narrow character form.
VA(0x0042250f, 0x3e)
char toupper(char character) {
    if (character >= 'a' && character <= 'z')
        return character - 32;
    else
        return character;
}

// Buka 2.1 InterpretCommandLine reduced to HoMM1's /I, /C, /S and /B switches.
VA(0x0042254d, 0x288)
i32 InterpretCommandLine(void) {
    i32 size;
    i32 i;
    i32 helpRequested = 0;

    giDebugLevel = 0;
    giShowIntro = 1;
    gColorMice = 0;
    gSpecialMouseMasks = 1;
    giScreenScroll = 1;
    giLimitPlayer = 0;
    gbBlackoutPlayer = 1;
    strcpy(gMapName, "AES31000.map");
    strcpy(gFullMapName, "Claw ( Easy )");
    strcpy(gMapDescription, "The Griffons will protect you until you are ready to make your move.");

    size = strlen(gCommandLine);
    for (i = 0; i < size; i++) {
        if (gCommandLine[i] == '/' && i + 1 < size) {
            switch (toupper(gCommandLine[i + 1])) {
                case 'I':
                    if (i + 2 < size)
                        giShowIntro = gCommandLine[i + 2] - '0';
                    break;
                case 'C':
                    if (i + 2 < size)
                        gColorMice = gCommandLine[i + 2] - '0';
                    break;
                case 'S':
                    if (i + 2 < size)
                        gbNoSound = 1 - (gCommandLine[i + 2] - '0');
                    break;
                case 'B':
                    if (i + 2 < size)
                        gSpecialMouseMasks = gCommandLine[i + 2] - '0';
                    break;
            }
        }
    }

    sprintf(cAggPathName, "%s%s", gDataPath, "heroes.agg");
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
VA(0x0043e05e, 0x154)
i16 InitMenuHandler(tag_message& message) {
    i32 handled = 0;
    i32 helpIndex;

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

    if (handled || gMenuCommand != APP_MENU_NONE) {
        FINISH_DIALOG_MESSAGE(message);
        return MESSAGE_DISPATCH_FORWARD;
    }
    return MESSAGE_DISPATCH_CONSUME;
}

VA(0x0043e1b2, 0x9)
i16 NullHandler(tag_message&) {
    return MESSAGE_DISPATCH_CONSUME;
}

// Buka 2.1 RecruitHeroHandler: HoMM1 offers two heroes, each with its own
// view (its portrait, rcrthero.bin ids 2-3) and recruit (ids 8-9) button.
VA(0x0043e1bb, 0x15e)
i16 RecruitHeroHandler(tag_message& message) {
    // Retail keeps these four ids as stored locals.
    const i16 viewButton1 = RECRUIT_HERO_PORTRAIT_FIRST;
    const i16 viewButton2 = RECRUIT_HERO_PORTRAIT_SECOND;
    const i16 recruitButton1 = RECRUIT_HERO_SELECT_FIRST;
    const i16 recruitButton2 = RECRUIT_HERO_SELECT_SECOND;
    i32 shouldClose = 0;
    i32 index;

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
                        gpTownManager->m_recruitState = RECRUIT_HERO_NONE;
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
VA(0x00422b6a, 0x47)
char* GetBuildingName(i32 race, i16 building) {
    if (building < BUILDING_SLOT_DWELLING_FIRST)
        return gNeutralBuildingNames[building];
    else
        return gDwellingNames
            [building - BUILDING_SLOT_DWELLING_FIRST + race * BUILDING_SLOT_DWELLING_COUNT];
}

VA(0x00422bb1, 0x9f)
void GetBuildingCost(i32 race, i16 building, i32* const destination, i32 mageLevel) {
    if (building < BUILDING_SLOT_DWELLING_FIRST) {
        if (building == BUILDING_SLOT_MAGE_GUILD)
            memcpy(destination, gMageBuildingCosts[mageLevel], RESOURCE_COUNT * sizeof(i32));
        else
            memcpy(destination, gNeutralBuildingCosts[building], RESOURCE_COUNT * sizeof(i32));
    } else {
        memcpy(
            destination,
            gDwellingCosts
                [building - BUILDING_SLOT_DWELLING_FIRST + race * BUILDING_SLOT_DWELLING_COUNT],
            RESOURCE_COUNT * sizeof(i32)
        );
    }
}

VA(0x00422c50, 0x1a)
char* GetMonsterName(i32 monster) {
    return gArmyNames[monster];
}

// donor PoL RVA 0x0009992c; preferred Buka symbol ?GetMonsterCost@@YIXHQAH@Z
// donor Buka TU SOURCE/KB; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.424205;margin=0.383727;shape=0.192;size=0.855;calls=1.000;alternate=pol20:void GetMonsterCost(int, int * const)@0x0009992c
VA(0x0043e3de, 0xbe)
void GetMonsterCost(i32 monster, i32* const cost) {
    i32 index;
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
VA(0x00422d50, 0x144)
i8 CanBuild(town* t, i16 building) {
    mapCell* cell;
    u16 required;
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
    if (building == BUILDING_SLOT_MAGE_GUILD && t->m_buildState >= TOWN_MAGE_GUILD_COST_LEVEL_LAST)
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
VA(0x00422e94, 0xce)
i8 CanBuy(town* t, i16 type) {
    i32 cost[RESOURCE_COUNT];
    playerData* rec;
    i32 i;
    GetBuildingCost(
        t->m_type,
        type,
        cost,
        (t->m_buildings & (1 << BUILDING_SLOT_MAGE_GUILD))
            ? (t->m_buildState >= TOWN_MAGE_GUILD_COST_LEVEL_LAST ? TOWN_MAGE_GUILD_COST_LEVEL_LAST
                                                                  : t->m_buildState + 1)
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
VA(0x00422f62, 0x60)
i32 GetBuildingBaseResourceValue(i32 race, i32 building, i32 level) {
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
VA(0x0043e693, 0xdad)
void NormalDialog(
    char* text,
    H1_ENUM_PARAM(NormalDialogType, i32) dialogType,
    i32 x,
    i32 y,
    H1_ENUM_PARAM(NormalDialogResourceType, i32) firstResourceType,
    i32 firstResourceValue,
    H1_ENUM_PARAM(NormalDialogResourceType, i32) secondResourceType,
    i32 secondResourceValue,
    H1_ENUM_PARAM(NormalDialogOrText, i32) showOrText
) {
    char szFilename[NORMAL_DIALOG_FILENAME_LENGTH];
    char* amountText[NORMAL_DIALOG_RESOURCE_COUNT];
    i32 sizingHeight;
    i32 resourceYPos;
    i32 kind[NORMAL_DIALOG_RESOURCE_COUNT];
    iconWidget* iconPanel;
    i32 resWidth;
    i16 bShowMessage;
    font* bigFont;
    i32 width;
    i32 height;
    i32 i;
    i32 contentSize;
    i32 id;
    i32 iHeight;
    i32 resourceQty[NORMAL_DIALOG_RESOURCE_COUNT];
    tag_message message;
    i32 heightIndex;
    i32 lineCount;
    i32 resourceFrame;
    i32 frameHeight;
    textWidget* captionWidget;
    i32 maxIconHeight;
    i32 resCenterX;
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
        if (gpAdvManager->m_active == 1 && !gHeroWindShowing && !gOverviewShowing)
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
    gNormalDialogWindow = new heroWindow(x, y, szFilename);
    if (!gNormalDialogWindow)
        MemError();

    message.type = MESSAGE_WIDGET;
    message.command = WIDGET_COMMAND_CLEAR_FLAGS;
    message.value = NORMAL_DIALOG_BUTTON_FLAGS;
    if (dialogType != NORMAL_DIALOG_TYPE_WAIT_CANCEL
        && dialogType != NORMAL_DIALOG_TYPE_NO_BUTTONS) {
        message.id = NORMAL_DIALOG_BUTTON_OK;
        gNormalDialogWindow->BroadcastMessage(message);
    }
    if (dialogType != NORMAL_DIALOG_TYPE_WAIT_OK && dialogType != NORMAL_DIALOG_TYPE_OK
        && dialogType != NORMAL_DIALOG_TYPE_NO_BUTTONS) {
        message.id = NORMAL_DIALOG_BUTTON_CANCEL;
        gNormalDialogWindow->BroadcastMessage(message);
    }
    if (dialogType != NORMAL_DIALOG_TYPE_YES_NO) {
        message.id = NORMAL_DIALOG_BUTTON_YES;
        gNormalDialogWindow->BroadcastMessage(message);
        message.id = NORMAL_DIALOG_BUTTON_NO;
        gNormalDialogWindow->BroadcastMessage(message);
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
            WIDGET_ID_NONE,
            ICON_WIDGET_DRAW,
            1
        );
        if (!iconPanel)
            MemError();
        gNormalDialogWindow->AddWidget(iconPanel, WINDOW_Z_ORDER_APPEND);
        if (kind[i] == NORMAL_DIALOG_ARTIFACT) {
            iconPanel = new iconWidget(
                resCenterX - resWidth / 2 + 6,
                resourceYPos + 6,
                76,
                76,
                "artifact.icn",
                resourceQty[i],
                ICON_DRAW_NORMAL,
                WIDGET_ID_NONE,
                ICON_WIDGET_DRAW,
                1
            );
            if (!iconPanel)
                MemError();
            gNormalDialogWindow->AddWidget(iconPanel, WINDOW_Z_ORDER_APPEND);
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
                WIDGET_ID_NONE,
                ICON_WIDGET_DRAW,
                1
            );
            if (!iconPanel)
                MemError();
            gNormalDialogWindow->AddWidget(iconPanel, WINDOW_Z_ORDER_APPEND);
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
                WIDGET_ID_NONE,
                ICON_WIDGET_DRAW,
                1
            );
            if (!iconPanel)
                MemError();
            gNormalDialogWindow->AddWidget(iconPanel, WINDOW_Z_ORDER_APPEND);
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
        gNormalDialogWindow->AddWidget(captionWidget, WINDOW_Z_ORDER_APPEND);
    }

    SET_WIDGET_MESSAGE(message, WIDGET_COMMAND_SET_TEXT, NORMAL_DIALOG_TEXT_WIDGET_ID);
    message.text = text;
    gNormalDialogWindow->BroadcastMessage(message);

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
        gNormalDialogWindow->AddWidget(captionWidget, WINDOW_Z_ORDER_APPEND);
    }

    if (gpAdvManager->m_active == 1)
        gpMouseManager->SetPointer(ADVENTURE_POINTER_DEFAULT);
    else if (gpCombatManager->m_active == 1)
        gpMouseManager->SetPointer(COMBAT_POINTER_DEFAULT);

    if (dialogType == NORMAL_DIALOG_TYPE_WAIT_CANCEL || dialogType == NORMAL_DIALOG_TYPE_WAIT_OK) {
        gpWindowManager->DoDialog(gNormalDialogWindow, WaitHandler, 0);
    } else if (dialogType == NORMAL_DIALOG_TYPE_QUICK_VIEW) {
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

// donor PoL RVA 0x000a2565; preferred Buka symbol ?UpdateNormalDialog@@YIXPAD@Z
// donor Buka TU SOURCE/KB; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.563703;margin=0.348381;shape=0.417;size=0.972;calls=1.000;alternate=pol20:void UpdateNormalDialog(char *)@0x000a2565
// @dead-code
// Zero-ref: no effective incoming retail reference.
VA(0x0043f440, 0x62)
void UpdateNormalDialog(char* text) {
    tag_message message;
    {
        i16 show = 1; // Retained from donor and retail stack frame.
        SET_WIDGET_MESSAGE(message, WIDGET_COMMAND_SET_TEXT, NORMAL_DIALOG_TEXT_WIDGET_ID);
        message.text = text;
        gNormalDialogWindow->BroadcastMessage(message);
        gNormalDialogWindow->DrawWindow(0, 0, NORMAL_DIALOG_FOREGROUND_WIDGET_LIMIT);
        gNormalDialogWindow
            ->DrawWindow(1, WINDOW_ALL_WIDGETS_LOW, NORMAL_DIALOG_BACKGROUND_WIDGET_LAST_ID);
    }
}

// donor PoL RVA 0x00099e81; preferred Buka symbol ?WaitHandler@@YIHAAUtag_message@@@Z
// donor Buka TU SOURCE/KB; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.445743;margin=0.444520;shape=0.204;size=0.951;calls=0.688;alternate=pol20:int WaitHandler(struct tag_message &)@0x00099e81
VA(0x0043f4a2, 0x14d)
i16 WaitHandler(tag_message& message) {
    i8 result = 0;
    gbFunctionComplete = 1;
    PollSound();
    if (!MusicPlaying())
        PlayMusic(gpAdvManager->m_currentTerrain);
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
VA(0x0043f5ef, 0xb3)
i16 EventWindowHandler(tag_message& message) {
    if (!MusicPlaying())
        PlayMusic(gpAdvManager->m_currentTerrain);
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

// Buka 2.1 TrueFalseDialogHandler.
VA(0x0043f6a2, 0x11)
i16 TrueFalseDialogHandler(tag_message& message) {
    return EventWindowHandler(message);
}

// donor PoL RVA 0x0009a52f; preferred Buka symbol ?PlayerDead@@YIXH@Z
// donor Buka TU SOURCE/KB; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.488269;margin=0.466685;shape=0.274;size=0.981;calls=0.750;alternate=pol20:void PlayerDead(int)@0x0009a52f
VA(0x0043f6b3, 0x145)
void PlayerDead(i32 player) {
    playerData* currentPlayer;
    i32 i;
    gbRetreatWin = 0;
    currentPlayer = &gpGame->m_players[player];
    gpGame->m_playerDead[player] = 1;
    ++gpGame->m_deadPlayerCount;
    for (i = 0; i < GAME_MINE_COUNT; ++i) {
        if (gpGame->m_mineOwners[i] == player)
            gpGame->ClaimMine(i, GAME_PLAYER_NONE);
    }
    for (i = currentPlayer->m_heroCount - 1; i >= 0; --i)
        gpGame->GetHero(currentPlayer->m_heroIds[i])->Deallocate();
    for (i = 0; i < HERO_AVAILABLE_SLOT_COUNT; ++i) {
        if (gpGame->m_availableHeroes[currentPlayer->m_availableHeroIds[i]]
            == HERO_AVAILABILITY_RETREATED)
            gpGame->m_availableHeroes[currentPlayer->m_availableHeroIds[i]] =
                HERO_AVAILABILITY_UNAVAILABLE;
    }
    if (gRemoteOn && gbHumanPlayer[player])
        HandleRemoteDeadPlayerExit(player);
}

DATA(0x004902d0)
char* gCombatGroundNames[7] = {
    "boat.xtl",
    "grass.xtl",
    "snow.xtl",
    "swamp.xtl",
    "lava.xtl",
    "desert.xtl",
    "dgrass.xtl",
};
DATA(0x004902ec)
char* gCombatObstacleNames[7] = {
    "boat.obj",
    "grass.obj",
    "snow.obj",
    "swamp.obj",
    "lava.obj",
    "desert.obj",
    "dgrass.obj",
};
DATA(0x00490308)
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
DATA(0x00490348)
char* gCombatFxNames[26] = {
    "redfire.icn", "elecfire.icn", "magic04.icn", "magic01.icn", "magic01.icn",  "magic02.icn",
    "magic02.icn", "magic06.icn",  "magic07.icn", "magic01.icn", "magic06.icn",  "magic08.icn",
    "magic07.icn", "magic01.icn",  "magic01.icn", "magic02.icn", "reddeath.icn", "magic03.icn",
    "magic03.icn", "magic06.icn",  "magic01.icn", "magic01.icn", "rainbluk.icn", "cloudluk.icn",
    "moraleg.icn", "moraleb.icn",
};
DATA(0x00490f58)
i16 gSpellAIValue[29] = {
    500,  350,  300, 400, 550, 900, 400, 500, 300, 350, 250, 0, 100,  150, 1000,
    2000, 1700, 700, 700, 0,   0,   0,   0,   0,   0,   0,   0, 1200, 0,
};
DATA(0x004903ec)
i8 gSpellAIFlags[29] = {
    3, 3, 2, 2, 3, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 3, 3, 3, 2, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
};
DATA(0x00490fb8)
i8 gMageGuildSpellPool[4][8] = {
    {9, 13, 6, 8, 10, 20, 19, 8},
    {1, 5, 3, 7, 11, 21, 26, 12},
    {0, 18, 14, 16, 22, 25, 23, 2},
    {27, 4, 15, 17, 28, 24, 28, 27},
};
DATA(0x0049042c)
i8 gCombatAdjacency[45][6] = {
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
DATA(0x004910e8)
i16 horseFrameFlip[16] = {45, 46, 47, 48, 49, 50, 51, 52, 53, 179, 178, 177, 54, 175, 174, 55};
DATA(0x00491108)
i16 boatFrameFlip[16] = {0, 0, 9, 9, 18, 18, 27, 27, 36, 36, 155, 155, 146, 146, 137, 137};
DATA(0x0049057c)
// Four player colors and the neutral-owner color; the following bytes are linker alignment.
i16 gRadarOwnerColor[5] = {79, 105, 200, 129, 10};
DATA(0x00490588)
i16 gRadarTerrainColor[24] = {
    82,  99, 7,   180, 26,  123, 55, 0,  16, 48, 98, 160,
    126, 74, 110, 179, 100, 218, 12, 12, 12, 12, 12, 12,
};
DATA(0x00491168)
char* gTownObjectNames[20] = {
    "magegld", "thievesg", "tavern", "dock", "well", "farm", "frst", "plns", "mtn", "tent",
    "cast",    "_d0",      "_d1",    "_d2",  "_d3",  "_d4",  "_d5",  "_e0",  "_e1", "_e2",
};
DATA(0x00490608)
i8 gDwellingType[4][6] = {
    {0, 1, 2, 3, 4, 5},
    {12, 13, 14, 15, 16, 17},
    {6, 7, 8, 9, 10, 11},
    {18, 19, 20, 21, 22, 23},
};
DATA(0x004911d0)
i32 gMageBuildingCosts[4][7] = {
    {5, 0, 5, 0, 0, 0, 2000},
    {5, 4, 5, 4, 4, 4, 1000},
    {5, 6, 5, 6, 6, 6, 1000},
    {5, 10, 5, 10, 10, 10, 1000},
};
DATA(0x00491240)
i32 gNeutralBuildingCosts[7][7] = {
    {5, 0, 5, 0, 0, 0, 2000},
    {5, 0, 0, 0, 0, 0, 750},
    {5, 0, 0, 0, 0, 0, 500},
    {20, 0, 0, 0, 0, 0, 2000},
    {0, 0, 0, 0, 0, 0, 500},
    {5, 0, 5, 0, 0, 0, 2000},
    {20, 0, 20, 0, 0, 0, 5000},
};
DATA(0x00491308)
i32 gMageBaseResourceValues[4] = {4000, 6500, 8500, 10500};
DATA(0x00491318)
i32 gNeutralBaseResourceValues[7] = {5000, 1500, 500, 2000, 3000, 0, 12000};
DATA(0x00491338)
i32 gDwellingBaseResourceValues[24] = {
    858,  2225, 2816, 7385, 13754, 29785, 1684, 2256, 3736, 7213, 15181, 27684,
    1802, 2615, 3414, 6967, 12212, 38141, 1956, 2607, 3869, 7510, 16002, 111967,
};
DATA(0x00491398)
i32 gDwellingCosts[24][7] = {
    {0, 0, 0, 0, 0, 0, 200},    {0, 0, 0, 0, 0, 0, 1000},   {0, 0, 5, 0, 0, 0, 1000},
    {10, 0, 10, 0, 0, 0, 2000}, {20, 0, 0, 0, 0, 0, 3000},  {20, 0, 0, 0, 20, 0, 5000},
    {5, 0, 0, 0, 0, 0, 500},    {5, 0, 0, 0, 0, 0, 1000},   {0, 0, 0, 0, 0, 0, 1500},
    {0, 10, 10, 0, 0, 0, 2500}, {10, 0, 0, 0, 0, 10, 3000}, {0, 20, 30, 0, 0, 0, 10000},
    {0, 0, 0, 0, 0, 0, 300},    {5, 0, 0, 0, 0, 0, 800},    {0, 0, 0, 0, 0, 0, 1000},
    {10, 0, 10, 0, 0, 0, 2000}, {0, 0, 20, 0, 0, 0, 4000},  {0, 0, 20, 0, 20, 0, 6000},
    {0, 0, 0, 0, 0, 0, 500},    {0, 0, 10, 0, 0, 0, 1000},  {0, 0, 0, 0, 0, 0, 2000},
    {0, 0, 0, 0, 0, 10, 3000},  {0, 0, 0, 10, 0, 0, 4000},  {0, 0, 30, 20, 0, 0, 15000},
};
DATA(0x00491638)
i8 gCastleResources[4] = {0, 2, -1, -1};

// Buka 2.1 HandleRemoteDeadPlayerExit for HoMM1's two-player transport.
VA(0x0043f7f8, 0x8a)
void HandleRemoteDeadPlayerExit(i32 position) {
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
VA(0x00424425, 0xf1)
void HandleRemoteSuddenExit(void) {
    i32 next;
    if (!gGameInitialized)
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

VA(0x0043f956, 0x238)
// HoMM1 callers push four byte-sized values: player, an unused flag,
// elimination and timeout.
void ReceiveRemotePlayerExit(i8 position, i8, i8 eliminated, i8 timedOut) {
    if (position == giThisGamePos) {
        sprintf(gText, "You have been eliminated from the game!!!");
        NormalDialog(gText, NORMAL_DIALOG_TYPE_OK);
        RemoteCleanup();
        gGameOver = 1;
        gEndSequence = GAME_END_LOST;
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
// playerData::m_daysLeft: NO_GRACE_PERIOD while the player holds a town;
// losing the last town starts a GRACE_DAYS countdown (Buka
// END_GAME_GRACE_DAYS) that game::NewDay runs down to elimination.
H1_ENUM_CONST_BEGIN(CheckEndGameConstant)
    END_GAME_NO_GRACE_PERIOD = -1,
    END_GAME_GRACE_DAYS = CALENDAR_DAYS_PER_WEEK
H1_ENUM_CONST_END(CheckEndGameConstant)

VA(0x0043fb8e, 0x936)
void CheckEndGame(i32 forced) {
    town* goalTown;
    hero* artifactHero;
    i8 ultimateOwner;
    char text[200];
    i32 numLiving;
    i8 win;
    playerData* pd;
    i32 slot;
    i8 lost;
    i8 normalWin;
    i32 lastSurvivor;
    i32 player;
    i32 humansAlive;
    i32 lastHumanPos;

    if (gbInNewGameSetup)
        return;
    if (gGameOver)
        return;
    if (gInCheckEndGame)
        return;
    gInCheckEndGame = 1;

    for (player = 0; player < gpGame->m_playerCount; player++) {
        if (!gpGame->m_playerDead[player]) {
            pd = &gpGame->m_players[player];
            if (!pd->m_heroCount && !pd->m_townCount) {
                PlayerDead(player);
                sprintf(
                    gText,
                    "%s player has been vanquished!",
                    gColorNames[gpGame->m_players[static_cast<i8>(player)].Color()]
                );
                gText[0] -= 32;
                NormalDialog(
                    gText,
                    NORMAL_DIALOG_TYPE_OK,
                    0x61,
                    NORMAL_DIALOG_AUTO_POSITION,
                    NORMAL_DIALOG_CREST,
                    gpGame->m_players[static_cast<i8>(player)].Color()
                );
            } else if (!pd->m_townCount) {
                if (pd->m_daysLeft == END_GAME_NO_GRACE_PERIOD) {
                    if (gbThisNetHumanPlayer[player]) {
                        sprintf(
                            gText,
                            "%s player, you have lost your last town.  If you do not conquer "
                            "another town in the next week, you will be eliminated.",
                            gColorNames[gpGame->m_players[static_cast<i8>(player)].Color()]
                        );
                        gText[0] -= 32;
                        NormalDialog(
                            gText,
                            NORMAL_DIALOG_TYPE_OK,
                            NORMAL_DIALOG_AUTO_POSITION,
                            NORMAL_DIALOG_AUTO_POSITION,
                            NORMAL_DIALOG_CREST,
                            gpGame->m_players[static_cast<i8>(player)].Color()
                        );
                    }
                    pd->m_daysLeft = END_GAME_GRACE_DAYS;
                } else if (!pd->m_daysLeft) {
                    PlayerDead(player);
                    if (gbThisNetHumanPlayer[player]) {
                        sprintf(
                            gText,
                            "%s player, your heroes abandon you, and you are banished from this "
                            "land.",
                            gColorNames[gpGame->m_players[static_cast<i8>(player)].Color()]
                        );
                        gText[0] -= 32;
                    } else {
                        sprintf(
                            gText,
                            "%s player's Heroes have abandoned him, and he is banished from this "
                            "land.",
                            gColorNames[gpGame->m_players[static_cast<i8>(player)].Color()]
                        );
                        gText[0] -= 32;
                    }
                    NormalDialog(
                        gText,
                        NORMAL_DIALOG_TYPE_OK,
                        0x61,
                        NORMAL_DIALOG_AUTO_POSITION,
                        NORMAL_DIALOG_CREST,
                        gpGame->m_players[static_cast<i8>(player)].Color()
                    );
                }
            } else {
                pd->m_daysLeft = END_GAME_NO_GRACE_PERIOD;
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
                ultimateOwner = GAME_PLAYER_NONE;
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
        gGameOver = 1;
        gEndSequence = GAME_END_LOST;
    }
    if (win) {
        gGameOver = 1;
        gEndSequence = GAME_END_WON;
    }
    if (numLiving == 1 || humansAlive == 0
        || (humansAlive == 1 && !gbThisNetHumanPlayer[lastHumanPos])) {
        if (humansAlive == 1 && gbThisNetHumanPlayer[lastHumanPos]) {
            if (normalWin) {
                gGameOver = 1;
                gEndSequence = GAME_END_WON;
            }
        } else {
            gGameOver = 1;
            gEndSequence = GAME_END_LOST;
        }
    }
    if (forced) {
        gGameOver = 1;
        gEndSequence = GAME_END_WON;
    }
    if (gGameOver && gpGame->m_campaignType > 0 && gEndSequence == GAME_END_WON
        && gpGame->m_campaignScenario + 1 == CAMPAIGN_SCENARIO_COUNT)
        gEndSequence = GAME_END_CAMPAIGN_COMPLETE;
    gInCheckEndGame = 0;
}

// donor PoL RVA 0x0009c07c; preferred Buka symbol ?QuickViewWait@@YIXXZ
// donor Buka TU SOURCE/KB; HoMM1 owner inferred from contiguous order
// evidence: graph:4;base=0.435968;margin=0.219505;shape=0.250;size=0.859;calls=0.600;alternate=pol20:void QuickViewWait(void)@0x0009c07c
VA(0x004404c4, 0x7a)
void QuickViewWait(void) {
    tag_message event;
    i32 done = 0;
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
VA(0x00424f99, 0x1cb)
void InitVars(void) {
    i32 i;
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
        giTerrainCost[i][FINDPATH_STEP_STRAIGHT] = TerrainStepCost(i, FINDPATH_STEP_STRAIGHT);
        giTerrainCost[i][FINDPATH_STEP_DIAGONAL] = TerrainStepCost(i, FINDPATH_STEP_DIAGONAL);
    }
    strcpy(cNetBoxLine[0], "");
    strcpy(cNetBoxLine[1], "");
    for (i = 0; i < MAP_EXTRA_RECORD_CAPACITY; i++)
        ppMapExtra[i] = NULL;
    hmnuDflt = LoadMenuA(static_cast<HINSTANCE>(hInstApp), "mnuDflt");
    hmnuCmbt = LoadMenuA(static_cast<HINSTANCE>(hInstApp), "mnuCmbt");
    hmnuAdv = LoadMenuA(static_cast<HINSTANCE>(hInstApp), "mnuAdv");
    hmnuTown = LoadMenuA(static_cast<HINSTANCE>(hInstApp), "mnuTown");
    LogStr(
        "LoadMenus",
        reinterpret_cast<i32>(hmnuDflt),
        reinterpret_cast<i32>(hmnuCmbt),
        reinterpret_cast<i32>(hmnuAdv),
        reinterpret_cast<i32>(hmnuTown),
        reinterpret_cast<i32>(hInstApp)
    ); // API-forced: LogStr logs handles as long.
}

// donor PoL RVA 0x0009c312; preferred Buka symbol ?ShowMoraleInfo@game@@QAEXPAVhero@@H@Z
// donor Buka TU SOURCE/KB; HoMM1 owner inferred from contiguous order
// evidence: graph:3;base=0.469331;margin=0.613523;shape=0.400;size=0.774;calls=0.649;alternate=pol20:void game::ShowMoraleInfo(class hero *, int)@0x0009c312
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

VA(0x00425164, 0x42c)
void game::ShowMoraleInfo(hero* h, i32 dialogType) {
    i32 faction;
    i32 i;
    i32 alignments;
    i32 baseLen;
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
    alignments = h->m_army.IsHomogeneous(ARMY_GROUP_EMPTY_SLOT);
    if (alignments > ARMY_GROUP_ALIGNMENT_NO_BONUS_LAST) {
        faction = 0;
        for (i = 0; i < ARMY_GROUP_SLOT_COUNT; i++) {
            if (h->m_army.m_creatureTypes[i] != CREATURE_NONE)
                faction = h->m_army.m_creatureTypes[i] / CREATURE_FACTION_SIZE;
        }
        sprintf(buffer, gMoraleInfoText[MORALE_INFO_ALL_TROOPS], gAlignmentNames[faction]);
        strcat(gText, buffer);
    }
    if (alignments == ARMY_GROUP_ALIGNMENT_THREE) {
        sprintf(buffer, gMoraleInfoText[MORALE_INFO_THREE_ALIGNMENTS]);
        strcat(gText, buffer);
    }
    if (alignments == ARMY_GROUP_ALIGNMENT_FOUR) {
        sprintf(buffer, gMoraleInfoText[MORALE_INFO_FOUR_ALIGNMENTS]);
        strcat(gText, buffer);
    }
    if (alignments == ARMY_GROUP_ALIGNMENT_FIVE_OR_MORE) {
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

// donor PoL RVA 0x0009c92d; preferred Buka symbol ?ShowLuckInfo@game@@QAEXPAVhero@@H@Z
// donor Buka TU SOURCE/KB; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.456267;margin=0.157936;shape=0.493;size=0.606;calls=0.556;alternate=pol20:void game::ShowLuckInfo(class hero *, int)@0x0009c92d
VA(0x00425590, 0x1f1)
void game::ShowLuckInfo(hero* h, i32 dialogType) {
    i32 alignments;
    i32 baseLen;
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

VA(0x00440c72, 0x5d)
void ClearMapExtra(void) {
    i32 i;
    for (i = 0; i < MAP_EXTRA_RECORD_CAPACITY; i++) {
        if (ppMapExtra[i]) {
            free(ppMapExtra[i]);
            ppMapExtra[i] = NULL;
        }
    }
    iMaxMapExtra = MAP_EXTRA_FIRST_RECORD;
}

// HoMM1 score-to-monster tables pair a threshold word with a monster word.
H1_ENUM_CONST_BEGIN(ScoreMonsterConstant)
    SCORE_MONSTER_COUNT = 28,
    SCORE_MONSTER_THRESHOLD = 0,
    SCORE_MONSTER_TYPE = 1
H1_ENUM_CONST_END(ScoreMonsterConstant)
VA(0x004257f1, 0x8e)
i16 GetMonType(i32 score, i32 highScoreType) {
    i32 index;
    for (index = SCORE_MONSTER_COUNT - 1; index >= 0; index--) {
        if (highScoreType == HIGH_SCORE_TYPE_CAMPAIGN) {
            if (gScoreCampaignMon[index][SCORE_MONSTER_THRESHOLD] >= score)
                return gScoreCampaignMon[index][SCORE_MONSTER_TYPE];
        } else {
            if (gScoreMon[index][SCORE_MONSTER_THRESHOLD] <= score)
                return gScoreMon[index][SCORE_MONSTER_TYPE];
        }
    }
    return gScoreMon[0][SCORE_MONSTER_TYPE];
}

// donor PoL RVA 0x0009ce14; preferred Buka symbol ?AddScoreToHighScore@@YIHHHHHPAD@Z
// donor Buka TU SOURCE/KB; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.701795;margin=0.122445;shape=0.377;size=0.950;calls=0.929;strings=%sCAMPAIGN.HS|%sSTANDARD.HS|.\DATA\;alternate=pol20:int AddScoreToHighScore(int, int, int, int, char *)@0x0009ce14
VA(0x0042587f, 0x3db)
i32 AddScoreToHighScore(i32 score, i32 standard, char*, char* scenarioName) {
    HighScoreEntry scores[HIGH_SCORE_DISPLAY_ENTRY_COUNT];
    i32 entry;
    i32 dest;
    i32 file;
    char fileName[352];
    char enteredPlayerName[20];
    i8 missingFile;

    missingFile = 0;
    if (standard == HIGH_SCORE_TYPE_STANDARD)
        sprintf(fileName, "%sSTANDARD.HS", gDataPath);
    else
        sprintf(fileName, "%sCAMPAIGN.HS", gDataPath);
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

    gShowHighScore = 1;
    giHighScoreType = standard;
    gHighScoreRank = HIGH_SCORE_EMPTY;
    giScore = score;
    for (entry = 0; entry < HIGH_SCORE_DISPLAY_ENTRY_COUNT; entry++) {
        if ((score >= scores[entry].score && standard == HIGH_SCORE_TYPE_STANDARD)
            || (score <= scores[entry].score && standard == HIGH_SCORE_TYPE_CAMPAIGN)
            || scores[entry].score == HIGH_SCORE_EMPTY) {
            gHighScoreRank = entry;
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
VA(0x00441066, 0x51)
void BVResMsg(char* s, i32 res, i32 qty) {
    giBottomViewOverride = BOTTOM_VIEW_RESOURCE;
    giBottomViewOverrideEndTime = KBTickCount() + 5000;
    giBottomViewResource = res;
    giBottomViewResourceQty = qty;
    strcpy(gcBottomViewText, s);
    gpAdvManager->UpdBottomView(1, 1, 1);
}

// Buka 2.1 GOut.
VA(0x00425cb5, 0x2e)
void GOut(char* text) {
    if (gpAdvManager->m_active == 1)
        AiPrint(text);
}

// HoMM1 maps every remote position other than the host to the one opponent slot.
VA(0x004410d6, 0x1b)
i8 NetPosToGamePos(i32 netPos) {
    if (netPos == NET_POSITION_HOST)
        return 0;
    else if (netPos > NET_POSITION_HOST)
        return 1;
    return GAME_PLAYER_NONE;
}

VA(0x004410f1, 0xb1)
i8 WaitForOtherPlayer(void) {
    i32 result = 0;
    RemoteMessage* data;
    PollSound();
    data = reinterpret_cast<RemoteMessage*>(GetRemoteData(1)); // API-forced: char* record.
    if (data && data->type == REMOTE_MESSAGE_RELIABLE) {
        switch (data->command) {
            case BOX_REMOTE_SETUP:
                memcpy(gbGamePosToNetPos, data->payload.data, GAME_PLAYER_COUNT);
                giThisGamePos = NetPosToGamePos(giThisNetPos);
                giHostGamePos = NetPosToGamePos(NET_POSITION_HOST);
                break;
            case BOX_REMOTE_SAVE:
                result = gpGame->ReceiveSaveGame(data->payload.saveSize, data->sender);
                break;
        }
    }
    return result;
}

// netbox.bin text widgets: the two scrolled chat lines (cNetBoxLine) and the
// line being typed.
H1_ENUM_BEGIN(NetBoxControl)
    NET_BOX_LINE_PREVIOUS = 1,
    NET_BOX_LINE_LATEST = 2,
    NET_BOX_INPUT = 3
H1_ENUM_END(NetBoxControl)

// PopNetBox blinks the input cursor on glTimers slot BLINK_TIMER_SLOT every
// BLINK_DELAY ms.
H1_ENUM_CONST_BEGIN(NetBoxConstant)
    NET_BOX_BLINK_TIMER_SLOT = 0,
    NET_BOX_BLINK_DELAY = 360
H1_ENUM_CONST_END(NetBoxConstant)

// donor PoL RVA 0x0009d4a6; preferred Buka symbol ?PopNetBox@@YIXPADH@Z
// donor Buka TU SOURCE/KB; HoMM1 owner inferred from contiguous order
// evidence: graph:2;base=0.593152;margin=0.055238;shape=0.393;size=0.624;calls=0.688;strings=netbox.bin;alternate=pol20:void PopNetBox(char *, int)@0x0009d4a6
VA(0x004411a2, 0x65a)
void PopNetBox(char* notice) {
    char* data;
    i8 blinkState;
    i8 drawLines;
    i8 bClose;
    i32 firstId;
    font* font;
    i8 shown;
    i32 pause;
    i32 lineTextLimit;
    i8 exitForIncomingData;
    i8 sendText;
    tag_message incoming;
    tag_message message;
    i32 len;
    char text[80];
    i8 oldShowIt;
    i8 updateInput;
    i32 lineHeight;
    i32 msgTime;
    heroWindow* netWin;
    i32 success;
    i32 textWidth;

    if (!gRemoteOn)
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
    SET_WIDGET_MESSAGE(message, WIDGET_COMMAND_SET_TEXT, NET_BOX_LINE_PREVIOUS);
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

        if (!updateInput && KBTickCount() > glTimers[NET_BOX_BLINK_TIMER_SLOT]) {
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
            SET_WIDGET_MESSAGE(message, WIDGET_COMMAND_SET_TEXT, NET_BOX_LINE_PREVIOUS);
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
            glTimers[NET_BOX_BLINK_TIMER_SLOT] = KBTickCount() + NET_BOX_BLINK_DELAY;
            if (blinkState)
                text[len] = '_';
            else
                text[len] = ' ';
            text[len + 1] = 0;
            SET_WIDGET_MESSAGE(message, WIDGET_COMMAND_SET_TEXT, NET_BOX_INPUT);
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
VA(0x004417fc, 0x28)
void AddNetBoxLine(char* text) {
    strcpy(cNetBoxLine[0], cNetBoxLine[1]);
    strcpy(cNetBoxLine[1], text);
}

// donor PoL RVA 0x0009e0f2; preferred Buka symbol ?ShutDown@@YIXPAD@Z
// donor Buka TU SOURCE/KB; HoMM1 owner inferred from contiguous order
// evidence: graph:3;base=0.466886;margin=0.632520;shape=0.403;size=0.708;calls=0.667;alternate=pol20:void ShutDown(char *)@0x0009e0f2
VA(0x00441824, 0x11f)
void ShutDown(char* message) {
    DATA(0x0049f280)
    static i32 gInShutDown = 0;
    char buffer[768];
    if (gInShutDown)
        return;
    gInShutDown = 1;
    gClosingApp = 1;
    buffer[0] = 0;
    if (message) {
        strcpy(buffer, message);
        SetFullScreenStatus(0);
        LogStr(buffer);
        MessageBoxA(
            static_cast<HWND>(hwndApp),
            buffer,
            "Unexpected Program Termination",
            MB_ICONHAND
        );
    }
    ClearMapExtra();
    UnloadSystemwideIcons();
    if (gRemoteOn)
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
VA(0x00441943, 0x34)
void FileError(char* filename) {
    char message[200];
    LogStr("File Error");
    sprintf(message, "Error opening file %s!", filename);
    ShutDown(message);
}

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

// HoMM1's victory screen (Buka 2.1 ShowCongrats): campaigns show the
// scenario's win text; standard games score the days played, rank the result
// as a creature and file it with the high scores.
VA(0x004266c1, 0x3be)
void ShowCongrats(void) {
    char name[32];
    i32 i;
    i32 result;
    tag_message message;
    i32 daysScore;
    heroWindow* win;

    daysScore = GetBaseScore(giCurTurn);
    result = gpGame->m_difficultyRating * daysScore / 100;
    PlayMusic(MUSIC_TRACK_CONGRATULATIONS);
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
VA(0x00441d6c, 0x8b)
void CongratsWait(void) {
    i32 cmd = 0;
    i8 finished = 0;
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

// dataentr.bin widgets: the prompt text and the edit field.
H1_ENUM_BEGIN(DataEntryControl)
    DATA_ENTRY_PROMPT = 1,
    DATA_ENTRY_TEXT = 10
H1_ENUM_END(DataEntryControl)

// Buka 2.1 GetDataEntry without the prompt-sized window and textEntryWidget.
VA(0x00441df7, 0x1a0)
void GetDataEntry(char* prompt, char* destination, i32 maximumLength, char* initialText) {
    i16 widgetId = DATA_ENTRY_TEXT;
    tag_message message;
    char textBuffer[100];

    gpMouseManager->SetPointer("advmice.mse", ADVENTURE_POINTER_DEFAULT);
    cDEDest = destination;
    iDEMaxLen = maximumLength;
    strcpy(cDEDest, "");
    DataEntryWin = new heroWindow(0xb1, 0x14, "dataentr.bin");
    if (!DataEntryWin)
        MemError();
    SET_WIDGET_MESSAGE(message, WIDGET_COMMAND_SET_TEXT, DATA_ENTRY_PROMPT);
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

VA(0x00441f97, 0x1af)
i16 DataEntryWindowHandler(tag_message& message) {
    i16 widgetId = DATA_ENTRY_TEXT;

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
                        SET_WIDGET_MESSAGE(message, WIDGET_COMMAND_SET_TEXT, DATA_ENTRY_TEXT);
                        message.text = cDEDest;
                        DataEntryWin->BroadcastMessage(message);
                        DataEntryWin->DrawWindow(1, DATA_ENTRY_TEXT, DATA_ENTRY_TEXT);
                        FINISH_DIALOG_MESSAGE(message);
                        return MESSAGE_DISPATCH_FORWARD;
                }
        }
    }
    return EventWindowHandler(message);
}

// donor PoL RVA 0x0009ea7c; preferred Buka symbol ?MemError@@YIXXZ
// donor Buka TU SOURCE/KB; HoMM1 owner inferred from contiguous order
// evidence: graph:4;base=0.499168;margin=0.828160;shape=0.176;size=0.610;calls=1.000;strings=Out of Memory;alternate=pol20:void MemError(void)@0x0009ea7c
VA(0x00442146, 0x60)
void MemError(void) {
    DATA(0x0049f34c)
    static i8 gInMemError = 0;
    if (gInMemError)
        return;
    gInMemError = 1;
    LogStr("Out of Memory");
    sprintf(
        gText,
        "\n\n%s\n%s\n%d%s\n%d%s\n\n",
        gMemoryErrorTitle,
        gMemoryRequirements,
        gRequiredExtendedMemory,
        gExtendedMemoryUnits,
        gRequiredConventionalMemory,
        gConventionalMemoryUnits
    );
    ShutDown(gText);
}

// Buka 2.1 MiscRuntime MemSize: a fixed reported memory size.
// @dead-code
// Zero-ref: no effective incoming retail reference.
VA(0x0042702f, 0x15)
i32 MemSize(i32) {
    return 16034;
}

// Buka 2.1 CheckMem without HoMM2's memory globals.
VA(0x00427044, 0x12)
i8 CheckMem(void) {
    return 1;
}

// Buka 2.1 GetTownName; HoMM1 towns carry a name index, and campaign maps
// override one town by position.
VA(0x004421b7, 0x9d)
char* GetTownName(i32 i) {
    town* townPointer = gpGame->GetTown(i);
    if (gpGame->m_campaignType > 0
        && gCampaignScenarios[gpGame->m_campaignScenario].victoryTownX >= 0
        && gCampaignScenarios[gpGame->m_campaignScenario].victoryTownX == townPointer->m_x
        && gCampaignScenarios[gpGame->m_campaignScenario].victoryTownY == townPointer->m_y)
        return gCampaignScenarios[gpGame->m_campaignScenario].victoryTownName;
    return gTownNames[townPointer->m_threat];
}

// Buka retail VA 0x00442254, size 0x39; returns bool in AL.
bool IsCDDrive(i32 driveIndex) {
    sprintf(gText, "A:\\");
    gText[0] += driveIndex;
    return GetDriveTypeA(gText) == DRIVE_CDROM;
}

VA(0x0044228d, 0x59)
void LoadSystemwideIcons(void) {
    gBuyBuildIcons = gpResourceManager->GetIcon("buybuild.icn");
    gSystemIcons = gpResourceManager->GetIcon("system.icn");
    bigFont = gpResourceManager->GetFont("bigfont.fnt");
    smallFont = gpResourceManager->GetFont("smalfont.fnt");
}

VA(0x004422e6, 0x4b)
void UnloadSystemwideIcons(void) {
    gpResourceManager->Dispose(gBuyBuildIcons);
    gpResourceManager->Dispose(gSystemIcons);
    gpResourceManager->Dispose(bigFont);
    gpResourceManager->Dispose(smallFont);
}

// Retail empty lifecycle hook; Buka and PoL KB correspondence.
VA(0x00442331, 0x5)
void EarlyShutDownSystem(void) {}

// Buka 2.1 GameUnsaved.
VA(0x00442336, 0x55)
i32 GameUnsaved(void) {
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
VA(0x0044238b, 0x523)
i32 HandleAppSpecificMenuCommands(i32 command) {
    i32 menuChanged;

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
            strcpy(gText, localization::Tr("game.restart.confirm"));
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
                localization::Tr("game.load.confirm")
            );
        confirmMenuCommand:
            if (gpAdvManager->m_active == 1) {
                NormalDialog(gText, NORMAL_DIALOG_TYPE_YES_NO);
                if (gpWindowManager->m_dialogResult != NORMAL_DIALOG_CONFIRM)
                    break;
            }
            gMenuCommand = command;
            break;
        case APP_MENU_SAVE_GAME:
            SaveGame();
            break;
        case APP_MENU_QUIT:
            PostMessage(static_cast<HWND>(hwndApp), WM_CLOSE, 0, 0);
            break;
        case APP_MENU_MUSIC_OFF:
            gConfig.musicVolume = SOUND_VOLUME_OFF;
            goto adjustMusic;
        case APP_MENU_MUSIC_100:
            gConfig.musicVolume = SOUND_VOLUME_FIRST;
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
            gConfig.musicVolume = SOUND_VOLUME_LAST;
            goto adjustMusic;
        adjustMusic:
            SetMusicVolume(gConfig.musicVolume);
            menuChanged = 1;
            break;
        case APP_MENU_SOUND_OFF:
            gConfig.soundVolume = SOUND_VOLUME_OFF;
            goto adjustSound;
        case APP_MENU_SOUND_100:
            gConfig.soundVolume = SOUND_VOLUME_FIRST;
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
            gConfig.soundVolume = SOUND_VOLUME_LAST;
            goto adjustSound;
        adjustSound:
            SetEffectsVolume(gConfig.soundVolume);
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
                gConfig.musicSource = SOUND_MUSIC_SOURCE_CD;
            }
            SetMusicSource(gConfig.musicSource != 0);
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
            gpAdvManager->ProcessSearch(ADVMGR_SEARCH_VIEW_CENTER, ADVMGR_SEARCH_VIEW_CENTER);
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
VA(0x004428ae, 0x31e)
void UpdateSystemOptionsMenu(void) {
    i32 checkedCommand;
    i32 menuCommand;

    if (!gConfig.gfx[gCurExe].showMenu)
        return;
    if (!hmnuApp)
        return;
    if (hmnuApp != hmnuAdv)
        return;

    for (menuCommand = APP_MENU_MUSIC_FIRST; menuCommand <= APP_MENU_MUSIC_LAST; menuCommand++)
        CheckMenuItem(static_cast<HMENU>(hmnuApp), menuCommand, MF_UNCHECKED);
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
    CheckMenuItem(static_cast<HMENU>(hmnuApp), checkedCommand, MF_CHECKED);

    for (menuCommand = APP_MENU_SOUND_FIRST; menuCommand <= APP_MENU_SOUND_LAST; menuCommand++)
        CheckMenuItem(static_cast<HMENU>(hmnuApp), menuCommand, MF_UNCHECKED);
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
    CheckMenuItem(static_cast<HMENU>(hmnuApp), checkedCommand, MF_CHECKED);

    for (menuCommand = APP_MENU_SPEED_FIRST; menuCommand <= APP_MENU_SPEED_LAST; menuCommand++)
        CheckMenuItem(static_cast<HMENU>(hmnuApp), menuCommand, MF_UNCHECKED);
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
    CheckMenuItem(static_cast<HMENU>(hmnuApp), checkedCommand, MF_CHECKED);
    CheckMenuItem(
        static_cast<HMENU>(hmnuApp),
        APP_MENU_CD_STEREO,
        gConfig.musicSource ? MF_CHECKED : MF_UNCHECKED
    );
    CheckMenuItem(
        static_cast<HMENU>(hmnuApp),
        APP_MENU_SHOW_PATH,
        gConfig.showRoute ? MF_CHECKED : MF_UNCHECKED
    );
    CheckMenuItem(
        static_cast<HMENU>(hmnuApp),
        APP_MENU_VIEW_ENEMY_MOVES,
        1 - gConfig.blackoutComputer ? MF_CHECKED : MF_UNCHECKED
    );
}

VA(0x00442bcc, 0x7d)
void CleanUpMenus(void) {
    if (hmnuApp) {
        SetMenu(static_cast<HWND>(hwndApp), NULL);
        if (hmnuAdv)
            DestroyMenu(static_cast<HMENU>(hmnuAdv));
        if (hmnuDflt)
            DestroyMenu(static_cast<HMENU>(hmnuDflt));
        if (hmnuCmbt)
            DestroyMenu(static_cast<HMENU>(hmnuCmbt));
        if (hmnuTown)
            DestroyMenu(static_cast<HMENU>(hmnuTown));
    }
    hmnuApp = NULL;
}

VA(0x00442c49, 0x15)
void UpdateAppSpecificMenus(void* hMenu) {
    if (hmnuAdv == hMenu)
        UpdateSystemOptionsMenu();
}

VA(0x00427d65, 0x22)
void EarlyResizeWindow(i32, i32, i32, i32) {
    if (gClosingApp)
        return;
}

// KB owns retail .data 0x00490e70-0x0049ea9f: these initialized globals in
// address order, followed by their initializer literals (0x00494184-0x0049ea97,
// emitted in this order). Initializers are retail bytes. Unreferenced storage at
// 0x00492570 (2 x 16 bytes), 0x0049303c and 0x00494178 is not yet named.
DATA(0x00491640)
i16 gCastleAmounts[4] = {20, 20, 0, 0};
DATA(0x00491648)
i16 gHeroGoldCost = 2500;
DATA(0x00491650)
i16 gVesaMode[6] = {640, 480, 256, 20226, 257, 0};
DATA(0x00490a9c)
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
DATA(0x00491680)
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
DATA(0x00491880)
u16 gDwellingRequirements[24] = {
    0, 128, 144, 132, 1536, 1536, 0, 132, 128, 513, 1024, 2048,
    0, 128, 128, 128, 1024, 2048, 0, 128, 128, 256, 512,  3072,
};
DATA(0x00490cf0)
i32 gResourceBaseValue[7] = {250, 250, 200, 250, 250, 250, 1};
DATA(0x004918d0)
i32 gStartingResources[4][7] = {
    {30, 10, 30, 10, 10, 10, 10000},
    {20, 5, 20, 5, 5, 5, 7500},
    {10, 0, 10, 0, 0, 0, 5000},
    {0, 0, 0, 0, 0, 0, 0},
};
DATA(0x00491940)
i32 gMineIncome[7] = {2, 1, 2, 1, 1, 1, 1000};
DATA(0x00490d98)
i32 gArtifactBaseRV[37] = {
    9000, 22000, 18000, 14000, 6000, 4000, 4000, 5600, 1200, 1200, 1200, 1200, -1200,
    2000, 1800,  1800,  2000,  1000, 3600, 5600, 4000, 5040, 2700, 3900, 4950, 5850,
    7000, 6000,  4000,  4500,  2250, 1200, 1200, 1200, 1200, 3500, 1500,
};
DATA(0x004919f4)
i32 gUltArtifactAvgValue = 16200;
DATA(0x004919f8)
char gDataPath[352] = ".\\DATA\\";
DATA(0x00491b58)
char gAnimPath[352] = "\\HEROES\\ANIM\\";
DATA(0x00491cb8)
char gSoundPath[352] = "\\HEROES\\SOUND\\";
DATA(0x004913b0)
char gGamePath[20] = ".\\GAMES\\";
DATA(0x00491e30)
char gMapPath[20] = ".\\MAPS\\";
DATA(0x004913d8)
i8 gHeroScoutRadius[8] = {4, 4, 4, 6, 4, 0, 0, 0};
DATA(0x00491e50)
float gClassNavigationMod[8] = {1.0f, 1.0f, 2.0f, 1.0f, 1.0f, 1.3f, 1.0f, 1.0f};
DATA(0x00491400)
i8 gVisRangeTown = 5;
DATA(0x00491408)
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
DATA(0x004921e0)
float gStatPower[41] = {
    0.63f, 0.63f, 0.63f, 0.63f, 0.63f, 0.63f, 0.64f, 0.65f, 0.67f, 0.68f, 0.7f,
    0.72f, 0.74f, 0.76f, 0.78f, 0.81f, 0.84f, 0.87f, 0.91f, 0.95f, 1.0f,  1.05f,
    1.1f,  1.15f, 1.22f, 1.28f, 1.36f, 1.44f, 1.53f, 1.63f, 1.74f, 1.86f, 1.99f,
    2.14f, 2.3f,  2.48f, 2.67f, 2.86f, 2.86f, 2.86f, 2.86f,
};
DATA(0x00491810)
float gBattleStat[41] = {
    0.2f,  0.2f,  0.2f,  0.2f,  0.2f,  0.21f, 0.23f, 0.25f, 0.28f, 0.31f, 0.35f,
    0.39f, 0.43f, 0.48f, 0.53f, 0.59f, 0.66f, 0.73f, 0.81f, 0.9f,  1.0f,  1.1f,
    1.21f, 1.33f, 1.46f, 1.61f, 1.77f, 1.95f, 2.14f, 2.36f, 2.59f, 2.85f, 3.14f,
    3.45f, 3.8f,  4.18f, 4.59f, 5.0f,  5.0f,  5.0f,  5.0f,
};
DATA(0x0049232c)
i8 gMageGuildSpellCount[4] = {3, 5, 7, 9};
DATA(0x004918b8)
float gSpellCastNumMod[21] = {
    0.0f,  1.0f,  1.7f,  2.2f,  2.6f,  2.95f, 3.27f, 3.56f, 3.81f, 4.04f, 4.25f,
    4.45f, 4.64f, 4.83f, 5.01f, 5.19f, 5.36f, 5.53f, 5.68f, 5.82f, 5.96f,
};
DATA(0x004923a8)
i8 gDrawSavedCursor = 0;
DATA(0x0049192c)
i16 gMinExpForLevel[4][12] = {
    {0, 1000, 2000, 3200, 4500, 6000, 7700, 9000, 11000, 13200, 15500, 18500},
    {0, 1000, 2000, 3200, 4500, 6000, 7700, 9000, 11000, 13200, 15500, 18500},
    {0, 1000, 2000, 3200, 4500, 6000, 7700, 9000, 11000, 13200, 15500, 18500},
    {0, 1000, 2000, 3200, 4500, 6000, 7700, 9000, 11000, 13200, 15500, 18500},
};
DATA(0x0049198c)
i8 gRouteFrame[8][8] = {
    {1, 6, 6, 6, 1, 38, 38, 38},
    {12, 2, 7, 7, 7, 2, 12, 12},
    {9, 9, 3, 8, 8, 8, 3, 9},
    {10, 10, 10, 4, 13, 13, 13, 4},
    {5, 11, 11, 11, 5, 43, 43, 43},
    {42, 36, 45, 45, 45, 36, 42, 42},
    {41, 41, 35, 40, 40, 40, 35, 41},
    {44, 44, 44, 34, 39, 39, 39, 34},
};
DATA(0x004919cc)
u8 gCloudType[256] = {
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
DATA(0x00491acc)
i8 gMons32Width[28] = {
    20, 20, 20, 25, 25, 24, 21, 21, 25, 27, 22, 20, 23, 23,
    21, 22, 25, 23, 27, 22, 29, 28, 32, 27, 21, 26, 21, 29,
};
DATA(0x00492570)
i16 gScoreMon[SCORE_MONSTER_COUNT][2] = {
    {0, 0},    {7, 6},    {14, 12},  {21, 18},  {28, 24},  {35, 7},   {42, 1},
    {49, 19},  {56, 13},  {63, 2},   {70, 8},   {77, 25},  {84, 14},  {91, 20},
    {98, 3},   {105, 9},  {112, 15}, {119, 21}, {126, 4},  {133, 26}, {140, 16},
    {147, 10}, {154, 22}, {161, 5},  {168, 27}, {175, 11}, {182, 17}, {189, 23},
};
DATA(0x004925e0)
i16 gScoreCampaignMon[SCORE_MONSTER_COUNT][2] = {
    {3600, 0},  {3400, 6},  {3200, 12}, {3000, 18}, {2600, 24}, {2400, 7},  {2200, 1},
    {2000, 19}, {1800, 13}, {1600, 2},  {1500, 8},  {1400, 25}, {1300, 14}, {1200, 20},
    {1100, 3},  {1000, 9},  {900, 15},  {800, 21},  {750, 4},   {700, 26},  {650, 16},
    {600, 10},  {550, 22},  {500, 5},   {450, 27},  {400, 11},  {350, 17},  {300, 23},
};
DATA(0x00492650)
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
DATA(0x00492760)
i8 townTheme[4] = {3, 0, 2, 1};
DATA(0x00492768)
campaignScenario gCampaignScenarios[CAMPAIGN_SCENARIO_COUNT] = {
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
DATA(0x00492a68)
i8 gCampaignSideCrests[4][2] = {{2, 0}, {1, 0}, {3, 0}, {0, 0}};
DATA(0x00492a70)
i16 gCrestTownTypes[4] = {3, 2, 0, 1};
DATA(0x00492a78)
i16 gCrestHeroClass[4] = {3, 1, 0, 2};
DATA(0x00492a80)
i8 gHeroSkillBonus[4][9][4] = {
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
DATA(0x00492088)
i8 gTownHeroClass[8] = {0, 2, 1, 3, 0, 2, 1, 3};
DATA(0x00492090)
u8 gMonoColorMap[256] = {
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
DATA(0x004a98cc)
i32 gLoadingMonoIcon = 0;
DATA(0x00492190)
i32 gMonoIconSkip = -1;
DATA(0x004a98d0)
i32 gScrollX = 0;
DATA(0x004a98d4)
i32 gScrollY = 0;
DATA(0x004a98d8)
i32 gNoBorder = 0;
DATA(0x00492c2c)
i32 gEnlargeScreenBlit = 1;
DATA(0x004a98dc)
void* hmnuDflt = NULL;
DATA(0x004a98e0)
void* hmnuCmbt = NULL;
DATA(0x004a98e4)
void* hmnuAdv = NULL;
DATA(0x004a98e8)
void* hmnuTown = NULL;
DATA(0x00492c40)
i32 gColorMice = 0;
DATA(0x00492c44)
i32 gSpecialMouseMasks = 0;
DATA(0x004a98f4)
i32 gCurExe = 0;
DATA(0x00492198)
i32 gMenuCommand = APP_MENU_NONE;
DATA(0x004a98f8)
i32 gInDialog = 0;
DATA(0x004921a0)
SMenuEnableStatus gMenuEnableStatus[70] = {
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
DATA(0x004a98fc)
i32 gInSetupDialog = 0;
DATA(0x00492e48)
i32 gMinimized = 0;
DATA(0x004a9904)
i32 gHeroMoving = 0;
DATA(0x00492e50)
i32 gInSmacker = 0;
DATA(0x00492e58)
i32 gRemoteReady = 0;
DATA(0x00492e5c)
i32 gHeartbeatSeen = 0;
DATA(0x0049238c)
char* gArtifactNames[38] = {
    localization::Tr("table.gArtifactNames.0"),  localization::Tr("table.gArtifactNames.1"),
    localization::Tr("table.gArtifactNames.2"),  localization::Tr("table.gArtifactNames.3"),
    localization::Tr("table.gArtifactNames.4"),  localization::Tr("table.gArtifactNames.5"),
    localization::Tr("table.gArtifactNames.6"),  localization::Tr("table.gArtifactNames.7"),
    localization::Tr("table.gArtifactNames.8"),  localization::Tr("table.gArtifactNames.9"),
    localization::Tr("table.gArtifactNames.10"), localization::Tr("table.gArtifactNames.11"),
    localization::Tr("table.gArtifactNames.12"), localization::Tr("table.gArtifactNames.13"),
    localization::Tr("table.gArtifactNames.14"), localization::Tr("table.gArtifactNames.15"),
    localization::Tr("table.gArtifactNames.16"), localization::Tr("table.gArtifactNames.17"),
    localization::Tr("table.gArtifactNames.18"), localization::Tr("table.gArtifactNames.19"),
    localization::Tr("table.gArtifactNames.20"), localization::Tr("table.gArtifactNames.21"),
    localization::Tr("table.gArtifactNames.22"), localization::Tr("table.gArtifactNames.23"),
    localization::Tr("table.gArtifactNames.24"), localization::Tr("table.gArtifactNames.25"),
    localization::Tr("table.gArtifactNames.26"), localization::Tr("table.gArtifactNames.27"),
    localization::Tr("table.gArtifactNames.28"), localization::Tr("table.gArtifactNames.29"),
    localization::Tr("table.gArtifactNames.30"), localization::Tr("table.gArtifactNames.31"),
    localization::Tr("table.gArtifactNames.32"), localization::Tr("table.gArtifactNames.33"),
    localization::Tr("table.gArtifactNames.34"), localization::Tr("table.gArtifactNames.35"),
    localization::Tr("table.gArtifactNames.36"), localization::Tr("table.gArtifactNames.37"),
};
DATA(0x00492424)
char* gArtifactDesc[38] = {
    localization::Tr("table.gArtifactDesc.0"),  localization::Tr("table.gArtifactDesc.1"),
    localization::Tr("table.gArtifactDesc.2"),  localization::Tr("table.gArtifactDesc.3"),
    localization::Tr("table.gArtifactDesc.4"),  localization::Tr("table.gArtifactDesc.5"),
    localization::Tr("table.gArtifactDesc.6"),  localization::Tr("table.gArtifactDesc.7"),
    localization::Tr("table.gArtifactDesc.8"),  localization::Tr("table.gArtifactDesc.9"),
    localization::Tr("table.gArtifactDesc.10"), localization::Tr("table.gArtifactDesc.11"),
    localization::Tr("table.gArtifactDesc.12"), localization::Tr("table.gArtifactDesc.13"),
    localization::Tr("table.gArtifactDesc.14"), localization::Tr("table.gArtifactDesc.15"),
    localization::Tr("table.gArtifactDesc.16"), localization::Tr("table.gArtifactDesc.17"),
    localization::Tr("table.gArtifactDesc.18"), localization::Tr("table.gArtifactDesc.19"),
    localization::Tr("table.gArtifactDesc.20"), localization::Tr("table.gArtifactDesc.21"),
    localization::Tr("table.gArtifactDesc.22"), localization::Tr("table.gArtifactDesc.23"),
    localization::Tr("table.gArtifactDesc.24"), localization::Tr("table.gArtifactDesc.25"),
    localization::Tr("table.gArtifactDesc.26"), localization::Tr("table.gArtifactDesc.27"),
    localization::Tr("table.gArtifactDesc.28"), localization::Tr("table.gArtifactDesc.29"),
    localization::Tr("table.gArtifactDesc.30"), localization::Tr("table.gArtifactDesc.31"),
    localization::Tr("table.gArtifactDesc.32"), localization::Tr("table.gArtifactDesc.33"),
    localization::Tr("table.gArtifactDesc.34"), localization::Tr("table.gArtifactDesc.35"),
    localization::Tr("table.gArtifactDesc.36"), localization::Tr("table.gArtifactDesc.37"),
};
DATA(0x00492f90)
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
DATA(0x00492554)
char* gStatNames[5] = {
    localization::Tr("table.gStatNames.0"),
    localization::Tr("table.gStatNames.1"),
    localization::Tr("table.gStatNames.2"),
    localization::Tr("table.gStatNames.3"),
    localization::Tr("table.gStatNames.4")
};
DATA(0x00492568)
char* gStatDesc[5] = {
    localization::Tr("table.gStatDesc.0"),
    localization::Tr("table.gStatDesc.1"),
    localization::Tr("table.gStatDesc.2"),
    localization::Tr("table.gStatDesc.3"),
    localization::Tr("table.gStatDesc.4"),
};
DATA(0x00493058)
char* gClassNames[4] = {
    localization::Tr("table.gClassNames.0"),
    localization::Tr("table.gClassNames.1"),
    localization::Tr("table.gClassNames.2"),
    localization::Tr("table.gClassNames.3")
};
DATA(0x0049258c)
char* gArmyNames[28] = {
    localization::Tr("table.gArmyNames.0"),  localization::Tr("table.gArmyNames.1"),
    localization::Tr("table.gArmyNames.2"),  localization::Tr("table.gArmyNames.3"),
    localization::Tr("table.gArmyNames.4"),  localization::Tr("table.gArmyNames.5"),
    localization::Tr("table.gArmyNames.6"),  localization::Tr("table.gArmyNames.7"),
    localization::Tr("table.gArmyNames.8"),  localization::Tr("table.gArmyNames.9"),
    localization::Tr("table.gArmyNames.10"), localization::Tr("table.gArmyNames.11"),
    localization::Tr("table.gArmyNames.12"), localization::Tr("table.gArmyNames.13"),
    localization::Tr("table.gArmyNames.14"), localization::Tr("table.gArmyNames.15"),
    localization::Tr("table.gArmyNames.16"), localization::Tr("table.gArmyNames.17"),
    localization::Tr("table.gArmyNames.18"), localization::Tr("table.gArmyNames.19"),
    localization::Tr("table.gArmyNames.20"), localization::Tr("table.gArmyNames.21"),
    localization::Tr("table.gArmyNames.22"), localization::Tr("table.gArmyNames.23"),
    localization::Tr("table.gArmyNames.24"), localization::Tr("table.gArmyNames.25"),
    localization::Tr("table.gArmyNames.26"), localization::Tr("table.gArmyNames.27"),
};
// Buka separates resource stems from translated creature display names.
DATA(0x004925fc)
char* gArmySpriteNames[28] = {
    "peasant", "archer", "pikeman", "swordsman", "cavalry", "paladin",
    "goblin", "orc", "wolf", "ogre", "troll", "cyclops",
    "sprite", "dwarf", "elf", "druid", "unicorn", "phoenix",
    "centaur", "gargoyle", "griffin", "minotaur", "hydra", "dragon",
    "rogue", "nomad", "ghost", "genie"
};
DATA(0x0049266c)
char* gArmyNamesPlural[28] = {
    localization::Tr("table.gArmyNamesPlural.0"),  localization::Tr("table.gArmyNamesPlural.1"),
    localization::Tr("table.gArmyNamesPlural.2"),  localization::Tr("table.gArmyNamesPlural.3"),
    localization::Tr("table.gArmyNamesPlural.4"),  localization::Tr("table.gArmyNamesPlural.5"),
    localization::Tr("table.gArmyNamesPlural.6"),  localization::Tr("table.gArmyNamesPlural.7"),
    localization::Tr("table.gArmyNamesPlural.8"),  localization::Tr("table.gArmyNamesPlural.9"),
    localization::Tr("table.gArmyNamesPlural.10"), localization::Tr("table.gArmyNamesPlural.11"),
    localization::Tr("table.gArmyNamesPlural.12"), localization::Tr("table.gArmyNamesPlural.13"),
    localization::Tr("table.gArmyNamesPlural.14"), localization::Tr("table.gArmyNamesPlural.15"),
    localization::Tr("table.gArmyNamesPlural.16"), localization::Tr("table.gArmyNamesPlural.17"),
    localization::Tr("table.gArmyNamesPlural.18"), localization::Tr("table.gArmyNamesPlural.19"),
    localization::Tr("table.gArmyNamesPlural.20"), localization::Tr("table.gArmyNamesPlural.21"),
    localization::Tr("table.gArmyNamesPlural.22"), localization::Tr("table.gArmyNamesPlural.23"),
    localization::Tr("table.gArmyNamesPlural.24"), localization::Tr("table.gArmyNamesPlural.25"),
    localization::Tr("table.gArmyNamesPlural.26"), localization::Tr("table.gArmyNamesPlural.27"),
};
DATA(0x004926dc)
char* gSpellNames[29] = {
    localization::Tr("table.gSpellNames.0"),  localization::Tr("table.gSpellNames.1"),
    localization::Tr("table.gSpellNames.2"),  localization::Tr("table.gSpellNames.3"),
    localization::Tr("table.gSpellNames.4"),  localization::Tr("table.gSpellNames.5"),
    localization::Tr("table.gSpellNames.6"),  localization::Tr("table.gSpellNames.7"),
    localization::Tr("table.gSpellNames.8"),  localization::Tr("table.gSpellNames.9"),
    localization::Tr("table.gSpellNames.10"), localization::Tr("table.gSpellNames.11"),
    localization::Tr("table.gSpellNames.12"), localization::Tr("table.gSpellNames.13"),
    localization::Tr("table.gSpellNames.14"), localization::Tr("table.gSpellNames.15"),
    localization::Tr("table.gSpellNames.16"), localization::Tr("table.gSpellNames.17"),
    localization::Tr("table.gSpellNames.18"), localization::Tr("table.gSpellNames.19"),
    localization::Tr("table.gSpellNames.20"), localization::Tr("table.gSpellNames.21"),
    localization::Tr("table.gSpellNames.22"), localization::Tr("table.gSpellNames.23"),
    localization::Tr("table.gSpellNames.24"), localization::Tr("table.gSpellNames.25"),
    localization::Tr("table.gSpellNames.26"), localization::Tr("table.gSpellNames.27"),
    localization::Tr("table.gSpellNames.28"),
};
DATA(0x004931c0)
char* gNeutralBuildingNames[7] = {
    localization::Tr("table.gNeutralBuildingNames.0"),
    localization::Tr("table.gNeutralBuildingNames.1"),
    localization::Tr("table.gNeutralBuildingNames.2"),
    localization::Tr("table.gNeutralBuildingNames.3"),
    localization::Tr("table.gNeutralBuildingNames.4"),
    localization::Tr("table.gNeutralBuildingNames.5"),
    localization::Tr("table.gNeutralBuildingNames.6")
};
DATA(0x004931e0)
char* gDwellingNames[24] = {
    localization::Tr("table.gDwellingNames.0"),  localization::Tr("table.gDwellingNames.1"),
    localization::Tr("table.gDwellingNames.2"),  localization::Tr("table.gDwellingNames.3"),
    localization::Tr("table.gDwellingNames.4"),  localization::Tr("table.gDwellingNames.5"),
    localization::Tr("table.gDwellingNames.6"),  localization::Tr("table.gDwellingNames.7"),
    localization::Tr("table.gDwellingNames.8"),  localization::Tr("table.gDwellingNames.9"),
    localization::Tr("table.gDwellingNames.10"), localization::Tr("table.gDwellingNames.11"),
    localization::Tr("table.gDwellingNames.12"), localization::Tr("table.gDwellingNames.13"),
    localization::Tr("table.gDwellingNames.14"), localization::Tr("table.gDwellingNames.15"),
    localization::Tr("table.gDwellingNames.16"), localization::Tr("table.gDwellingNames.17"),
    localization::Tr("table.gDwellingNames.18"), localization::Tr("table.gDwellingNames.19"),
    localization::Tr("table.gDwellingNames.20"), localization::Tr("table.gDwellingNames.21"),
    localization::Tr("table.gDwellingNames.22"), localization::Tr("table.gDwellingNames.23"),
};
DATA(0x004927cc)
char* gTerrainNames[7] = {
    localization::Tr("table.gTerrainNames.0"),
    localization::Tr("table.gTerrainNames.1"),
    localization::Tr("table.gTerrainNames.2"),
    localization::Tr("table.gTerrainNames.3"),
    localization::Tr("table.gTerrainNames.4"),
    localization::Tr("table.gTerrainNames.5"),
    localization::Tr("table.gTerrainNames.6")
};
DATA(0x004927e8)
char* gResourceNames[7] = {
    localization::Tr("table.gResourceNames.0"),
    localization::Tr("table.gResourceNames.1"),
    localization::Tr("table.gResourceNames.2"),
    localization::Tr("table.gResourceNames.3"),
    localization::Tr("table.gResourceNames.4"),
    localization::Tr("table.gResourceNames.5"),
    localization::Tr("table.gResourceNames.6")
};
DATA(0x00492804)
char* gMineNames[7] = {
    localization::Tr("table.gMineNames.0"),
    localization::Tr("table.gMineNames.1"),
    localization::Tr("table.gMineNames.2"),
    localization::Tr("table.gMineNames.3"),
    localization::Tr("table.gMineNames.4"),
    localization::Tr("table.gMineNames.5"),
    localization::Tr("table.gMineNames.6")
};
DATA(0x00492820)
char* gObjectNames[63] = {
    "",
    localization::Tr("table.gObjectNames.1"),
    localization::Tr("table.gObjectNames.2"),
    localization::Tr("table.gObjectNames.3"),
    localization::Tr("table.gObjectNames.4"),
    localization::Tr("table.gObjectNames.5"),
    localization::Tr("table.gObjectNames.6"),
    localization::Tr("table.gObjectNames.7"),
    localization::Tr("table.gObjectNames.8"),
    localization::Tr("table.gObjectNames.9"),
    localization::Tr("table.gObjectNames.10"),
    localization::Tr("table.gObjectNames.11"),
    localization::Tr("table.gObjectNames.12"),
    localization::Tr("table.gObjectNames.13"),
    localization::Tr("table.gObjectNames.14"),
    localization::Tr("table.gObjectNames.15"),
    localization::Tr("table.gObjectNames.16"),
    localization::Tr("table.gObjectNames.17"),
    localization::Tr("table.gObjectNames.18"),
    localization::Tr("table.gObjectNames.19"),
    localization::Tr("table.gObjectNames.20"),
    localization::Tr("table.gObjectNames.21"),
    localization::Tr("table.gObjectNames.22"),
    localization::Tr("table.gObjectNames.23"),
    localization::Tr("table.gObjectNames.24"),
    localization::Tr("table.gObjectNames.25"),
    localization::Tr("table.gObjectNames.26"),
    localization::Tr("table.gObjectNames.27"),
    localization::Tr("table.gObjectNames.28"),
    localization::Tr("table.gObjectNames.29"),
    localization::Tr("table.gObjectNames.30"),
    localization::Tr("table.gObjectNames.31"),
    localization::Tr("table.gObjectNames.32"),
    localization::Tr("table.gObjectNames.33"),
    localization::Tr("table.gObjectNames.34"),
    localization::Tr("table.gObjectNames.35"),
    localization::Tr("table.gObjectNames.36"),
    localization::Tr("table.gObjectNames.37"),
    localization::Tr("table.gObjectNames.38"),
    localization::Tr("table.gObjectNames.39"),
    localization::Tr("table.gObjectNames.40"),
    localization::Tr("table.gObjectNames.41"),
    localization::Tr("table.gObjectNames.42"),
    localization::Tr("table.gObjectNames.43"),
    localization::Tr("table.gObjectNames.44"),
    localization::Tr("table.gObjectNames.45"),
    localization::Tr("table.gObjectNames.46"),
    localization::Tr("table.gObjectNames.47"),
    localization::Tr("table.gObjectNames.48"),
    localization::Tr("table.gObjectNames.49"),
    "",
    "",
    localization::Tr("table.gObjectNames.52"),
    localization::Tr("table.gObjectNames.53"),
    localization::Tr("table.gObjectNames.54"),
    localization::Tr("table.gObjectNames.55"),
    localization::Tr("table.gObjectNames.56"),
    localization::Tr("table.gObjectNames.57"),
    localization::Tr("table.gObjectNames.58"),
    localization::Tr("table.gObjectNames.59"),
    localization::Tr("table.gObjectNames.60"),
    "",
    localization::Tr("table.gObjectNames.62")
};
DATA(0x0049291c)
char* gTownNames[36] = {
    localization::Tr("table.gTownNames.0"),
    localization::Tr("table.gTownNames.1"),
    localization::Tr("table.gTownNames.2"),
    localization::Tr("table.gTownNames.3"),
    localization::Tr("table.gTownNames.4"),
    localization::Tr("table.gTownNames.5"),
    localization::Tr("table.gTownNames.6"),
    localization::Tr("table.gTownNames.7"),
    localization::Tr("table.gTownNames.8"),
    localization::Tr("table.gTownNames.9"),
    localization::Tr("table.gTownNames.10"),
    localization::Tr("table.gTownNames.11"),
    localization::Tr("table.gTownNames.12"),
    localization::Tr("table.gTownNames.13"),
    localization::Tr("table.gTownNames.14"),
    localization::Tr("table.gTownNames.15"),
    localization::Tr("table.gTownNames.16"),
    localization::Tr("table.gTownNames.17"),
    localization::Tr("table.gTownNames.18"),
    localization::Tr("table.gTownNames.19"),
    localization::Tr("table.gTownNames.20"),
    localization::Tr("table.gTownNames.21"),
    localization::Tr("table.gTownNames.22"),
    localization::Tr("table.gTownNames.23"),
    localization::Tr("table.gTownNames.24"),
    localization::Tr("table.gTownNames.25"),
    localization::Tr("table.gTownNames.26"),
    localization::Tr("table.gTownNames.27"),
    localization::Tr("table.gTownNames.28"),
    localization::Tr("table.gTownNames.29"),
    localization::Tr("table.gTownNames.30"),
    localization::Tr("table.gTownNames.31"),
    localization::Tr("table.gTownNames.32"),
    localization::Tr("table.gTownNames.33"),
    localization::Tr("table.gTownNames.34"),
    localization::Tr("table.gTownNames.35"),
};
DATA(0x004929ac)
char* gEventText[77] = {
    localization::Tr("table.gEventText.0"),  localization::Tr("table.gEventText.1"),
    localization::Tr("table.gEventText.2"),  localization::Tr("table.gEventText.3"),
    localization::Tr("table.gEventText.4"),  localization::Tr("table.gEventText.5"),
    localization::Tr("table.gEventText.6"),  localization::Tr("table.gEventText.7"),
    localization::Tr("table.gEventText.8"),  localization::Tr("table.gEventText.9"),
    localization::Tr("table.gEventText.10"), localization::Tr("table.gEventText.11"),
    localization::Tr("table.gEventText.12"), localization::Tr("table.gEventText.13"),
    localization::Tr("table.gEventText.14"), localization::Tr("table.gEventText.15"),
    localization::Tr("table.gEventText.16"), localization::Tr("table.gEventText.17"),
    localization::Tr("table.gEventText.18"), localization::Tr("table.gEventText.19"),
    localization::Tr("table.gEventText.20"), localization::Tr("table.gEventText.21"),
    localization::Tr("table.gEventText.22"), localization::Tr("table.gEventText.23"),
    localization::Tr("table.gEventText.24"), localization::Tr("table.gEventText.25"),
    localization::Tr("table.gEventText.26"), localization::Tr("table.gEventText.27"),
    localization::Tr("table.gEventText.28"), localization::Tr("table.gEventText.29"),
    localization::Tr("table.gEventText.30"), localization::Tr("table.gEventText.31"),
    localization::Tr("table.gEventText.32"), localization::Tr("table.gEventText.33"),
    localization::Tr("table.gEventText.34"), localization::Tr("table.gEventText.35"),
    localization::Tr("table.gEventText.36"), localization::Tr("table.gEventText.37"),
    localization::Tr("table.gEventText.38"), localization::Tr("table.gEventText.39"),
    localization::Tr("table.gEventText.40"), localization::Tr("table.gEventText.41"),
    localization::Tr("table.gEventText.42"), localization::Tr("table.gEventText.43"),
    localization::Tr("table.gEventText.44"), localization::Tr("table.gEventText.45"),
    localization::Tr("table.gEventText.46"), localization::Tr("table.gEventText.47"),
    localization::Tr("table.gEventText.48"), localization::Tr("table.gEventText.49"),
    localization::Tr("table.gEventText.50"), localization::Tr("table.gEventText.51"),
    localization::Tr("table.gEventText.52"), localization::Tr("table.gEventText.53"),
    localization::Tr("table.gEventText.54"), localization::Tr("table.gEventText.55"),
    localization::Tr("table.gEventText.56"), localization::Tr("table.gEventText.57"),
    localization::Tr("table.gEventText.58"), localization::Tr("table.gEventText.59"),
    localization::Tr("table.gEventText.60"), localization::Tr("table.gEventText.61"),
    localization::Tr("table.gEventText.62"), localization::Tr("table.gEventText.63"),
    localization::Tr("table.gEventText.64"), localization::Tr("table.gEventText.65"),
    localization::Tr("table.gEventText.66"), localization::Tr("table.gEventText.67"),
    localization::Tr("table.gEventText.68"), localization::Tr("table.gEventText.69"),
    localization::Tr("table.gEventText.70"), localization::Tr("table.gEventText.71"),
    localization::Tr("table.gEventText.72"), localization::Tr("table.gEventText.73"),
    localization::Tr("table.gEventText.74"), localization::Tr("table.gEventText.75"),
    localization::Tr("table.gEventText.76"),
};
DATA(0x00492ae0)
char* gAPanelHelp[5] = {
    localization::Tr("table.gAPanelHelp.0"),
    localization::Tr("table.gAPanelHelp.1"),
    localization::Tr("table.gAPanelHelp.2"),
    localization::Tr("table.gAPanelHelp.3"),
    localization::Tr("table.gAPanelHelp.4"),
};
DATA(0x00492af4)
char* gInitMenuHelp[5] = {
    localization::Tr("table.gInitMenuHelp.0"),
    localization::Tr("table.gInitMenuHelp.1"),
    localization::Tr("table.gInitMenuHelp.2"),
    localization::Tr("table.gInitMenuHelp.3"),
    localization::Tr("table.gInitMenuHelp.4"),
};
DATA(0x00492b08)
char* gAdvMenuHelp[6] = {
    localization::Tr("table.gAdvMenuHelp.0"),
    localization::Tr("table.gAdvMenuHelp.1"),
    localization::Tr("table.gAdvMenuHelp.2"),
    localization::Tr("table.gAdvMenuHelp.3"),
    localization::Tr("table.gAdvMenuHelp.4"),
    localization::Tr("table.gAdvMenuHelp.5"),
};
DATA(0x00493590)
char* gLuckText[7] = {"Cursed", "Awful", "Bad", "Normal", "Good", "Great", "Irish"};
DATA(0x004935b0)
char* gMoraleText[7] = {"Treason", "Awful", "Poor", "Normal", "Good", "Great", "Blood!"};
DATA(0x00492b58)
char* onOffText[11] = {
    localization::Tr("table.onOffText.0"),
    localization::Tr("table.onOffText.1"),
    localization::Tr("table.onOffText.2"),
    localization::Tr("table.onOffText.3"),
    localization::Tr("table.onOffText.4"),
    localization::Tr("table.onOffText.5"),
    localization::Tr("table.onOffText.6"),
    localization::Tr("table.onOffText.7"),
    localization::Tr("table.onOffText.8"),
    localization::Tr("table.onOffText.9"),
    localization::Tr("table.onOffText.10")
};
DATA(0x00492b84)
char* walkSpeedText[5] = {
    localization::Tr("table.walkSpeedText.0"),
    localization::Tr("table.walkSpeedText.1"),
    localization::Tr("table.walkSpeedText.2"),
    localization::Tr("table.walkSpeedText.3"),
    localization::Tr("table.walkSpeedText.4")
};
DATA(0x00493618)
char* gColorNames[4] = {
    localization::Tr("table.gColorNames.0"),
    localization::Tr("table.gColorNames.1"),
    localization::Tr("table.gColorNames.2"),
    localization::Tr("table.gColorNames.3")
};
DATA(0x00493628)
char* gAlignmentNames[5] = {
    localization::Tr("table.gAlignmentNames.0"),
    localization::Tr("table.gAlignmentNames.1"),
    localization::Tr("table.gAlignmentNames.2"),
    localization::Tr("table.gAlignmentNames.3"),
    localization::Tr("table.gAlignmentNames.4")
};
DATA(0x00493640)
char* gSpellDesc[29] = {
    localization::Tr("table.gSpellDesc.0"),  localization::Tr("table.gSpellDesc.1"),
    localization::Tr("table.gSpellDesc.2"),  localization::Tr("table.gSpellDesc.3"),
    localization::Tr("table.gSpellDesc.4"),  localization::Tr("table.gSpellDesc.5"),
    localization::Tr("table.gSpellDesc.6"),  localization::Tr("table.gSpellDesc.7"),
    localization::Tr("table.gSpellDesc.8"),  localization::Tr("table.gSpellDesc.9"),
    localization::Tr("table.gSpellDesc.10"), localization::Tr("table.gSpellDesc.11"),
    localization::Tr("table.gSpellDesc.12"), localization::Tr("table.gSpellDesc.13"),
    localization::Tr("table.gSpellDesc.14"), localization::Tr("table.gSpellDesc.15"),
    localization::Tr("table.gSpellDesc.16"), localization::Tr("table.gSpellDesc.17"),
    localization::Tr("table.gSpellDesc.18"), localization::Tr("table.gSpellDesc.19"),
    localization::Tr("table.gSpellDesc.20"), localization::Tr("table.gSpellDesc.21"),
    localization::Tr("table.gSpellDesc.22"), localization::Tr("table.gSpellDesc.23"),
    localization::Tr("table.gSpellDesc.24"), localization::Tr("table.gSpellDesc.25"),
    localization::Tr("table.gSpellDesc.26"), localization::Tr("table.gSpellDesc.27"),
    localization::Tr("table.gSpellDesc.28"),
};
DATA(0x004936b8)
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
DATA(0x004936e0)
char* gWeekNames[15] = {
    localization::Tr("table.gWeekNames.0"),
    localization::Tr("table.gWeekNames.1"),
    localization::Tr("table.gWeekNames.2"),
    localization::Tr("table.gWeekNames.3"),
    localization::Tr("table.gWeekNames.4"),
    localization::Tr("table.gWeekNames.5"),
    localization::Tr("table.gWeekNames.6"),
    localization::Tr("table.gWeekNames.7"),
    localization::Tr("table.gWeekNames.8"),
    localization::Tr("table.gWeekNames.9"),
    localization::Tr("table.gWeekNames.10"),
    localization::Tr("table.gWeekNames.11"),
    localization::Tr("table.gWeekNames.12"),
    localization::Tr("table.gWeekNames.13"),
    localization::Tr("table.gWeekNames.14"),
};
DATA(0x00493720)
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
DATA(0x00492cf4)
char* gArmySizeNames[6][2] = {
    {localization::Tr("table.gArmySizeNames.0"), localization::Tr("table.gArmySizeNames.1")},
    {localization::Tr("table.gArmySizeNames.2"), localization::Tr("table.gArmySizeNames.3")},
    {localization::Tr("table.gArmySizeNames.4"), localization::Tr("table.gArmySizeNames.5")},
    {localization::Tr("table.gArmySizeNames.6"), localization::Tr("table.gArmySizeNames.7")},
    {localization::Tr("table.gArmySizeNames.8"), localization::Tr("table.gArmySizeNames.9")},
    {localization::Tr("table.gArmySizeNames.10"), localization::Tr("table.gArmySizeNames.11")},
};
DATA(0x004937b0)
char* gHeroScreen[19] = {
    localization::Tr("table.gHeroScreen.0"),  localization::Tr("table.gHeroScreen.1"),
    localization::Tr("table.gHeroScreen.2"),  localization::Tr("table.gHeroScreen.3"),
    localization::Tr("table.gHeroScreen.4"),  localization::Tr("table.gHeroScreen.5"),
    localization::Tr("table.gHeroScreen.6"),  localization::Tr("table.gHeroScreen.7"),
    localization::Tr("table.gHeroScreen.8"),  localization::Tr("table.gHeroScreen.9"),
    localization::Tr("table.gHeroScreen.10"), localization::Tr("table.gHeroScreen.11"),
    localization::Tr("table.gHeroScreen.12"), localization::Tr("table.gHeroScreen.13"),
    localization::Tr("table.gHeroScreen.14"), localization::Tr("table.gHeroScreen.15"),
    localization::Tr("table.gHeroScreen.16"), localization::Tr("table.gHeroScreen.17"),
    localization::Tr("table.gHeroScreen.18"),
};
DATA(0x00493800)
char* gCastleInfo[14] = {
    localization::Tr("table.gCastleInfo.0"),
    localization::Tr("table.gCastleInfo.1"),
    localization::Tr("table.gCastleInfo.2"),
    localization::Tr("table.gCastleInfo.3"),
    localization::Tr("table.gCastleInfo.4"),
    localization::Tr("table.gCastleInfo.5"),
    localization::Tr("table.gCastleInfo.6"),
    localization::Tr("table.gCastleInfo.7"),
    localization::Tr("table.gCastleInfo.8"),
    localization::Tr("table.gCastleInfo.9"),
    localization::Tr("table.gCastleInfo.10"),
    localization::Tr("table.gCastleInfo.11"),
    localization::Tr("table.gCastleInfo.12"),
    localization::Tr("table.gCastleInfo.13"),
};
DATA(0x00493838)
char* gLuckInfoText[12] = {
    localization::Tr("table.gLuckInfoText.0"),
    localization::Tr("table.gLuckInfoText.1"),
    localization::Tr("table.gLuckInfoText.2"),
    localization::Tr("table.gLuckInfoText.3"),
    localization::Tr("table.gLuckInfoText.4"),
    localization::Tr("table.gLuckInfoText.5"),
    localization::Tr("table.gLuckInfoText.6"),
    localization::Tr("table.gLuckInfoText.7"),
    localization::Tr("table.gLuckInfoText.8"),
    localization::Tr("table.gLuckInfoText.9"),
    localization::Tr("table.gLuckInfoText.10"),
    0,
};
DATA(0x00493868)
char* gMemoryErrorTitle = localization::Tr("table.gMemoryErrorTitle.0");
DATA(0x0049386c)
char* gMemoryRequirements = localization::Tr("table.gMemoryRequirements.0");
DATA(0x00493870)
char* gExtendedMemoryUnits = localization::Tr("table.gExtendedMemoryUnits.0");
DATA(0x00493874)
char* gConventionalMemoryUnits = localization::Tr("table.gConventionalMemoryUnits.0");
DATA(0x00492de4)
char* gPlayerTypeNames[5] = {
    localization::Tr("table.gPlayerTypeNames.0"),
    localization::Tr("table.gPlayerTypeNames.1"),
    localization::Tr("table.gPlayerTypeNames.2"),
    localization::Tr("table.gPlayerTypeNames.3"),
    localization::Tr("table.gPlayerTypeNames.4")
};
DATA(0x00492df8)
char* gSpellHelp[8] = {
    localization::Tr("table.gSpellHelp.0"),
    localization::Tr("table.gSpellHelp.1"),
    localization::Tr("table.gSpellHelp.2"),
    localization::Tr("table.gSpellHelp.3"),
    localization::Tr("table.gSpellHelp.4"),
    localization::Tr("table.gSpellHelp.5"),
    localization::Tr("table.gSpellHelp.6"),
    localization::Tr("table.gSpellHelp.7"),
};
DATA(0x004938b0)
char* gSpeedText[5] = {"", "Slow", "Medium", "Fast", "Blazing"};
DATA(0x004938c8)
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
DATA(0x004938f0)
char* gOverviewText[3] = {
    localization::Tr("table.gOverviewText.0"),
    localization::Tr("table.gOverviewText.1"),
    localization::Tr("table.gOverviewText.2"),
};
DATA(0x00493900)
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
DATA(0x00493920)
char* gViewGeneralLabels[6] =
    {"Attack: ", "Defense: ", "Spell Power: ", "Knowledge: ", "Morale: ", "Luck: "};
DATA(0x00493938)
char* gViewGeneralHelp[6] = {
    "Stop Catapult",
    "Cast Spell",
    "Retreat",
    "Surrender",
    "Cancel",
    "General's Options",
};
DATA(0x00493950)
char* gCombatMessage[9] = {
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
DATA(0x00493978)
char* gHeroLevel[3] = {
    localization::Tr("table.gHeroLevel.0"),
    localization::Tr("table.gHeroLevel.1"),
    localization::Tr("table.gHeroLevel.2")
};
DATA(0x00492ed8)
char* gCombatHelp[3] =
    {localization::Tr("table.gCombatHelp.0"), localization::Tr("table.gCombatHelp.1"), ""};
DATA(0x00493998)
char* gTownCommand[22] = {
    localization::Tr("table.gTownCommand.0"),  localization::Tr("table.gTownCommand.1"),
    localization::Tr("table.gTownCommand.2"),  localization::Tr("table.gTownCommand.3"),
    localization::Tr("table.gTownCommand.4"),  localization::Tr("table.gTownCommand.5"),
    localization::Tr("table.gTownCommand.6"),  localization::Tr("table.gTownCommand.7"),
    localization::Tr("table.gTownCommand.8"),  "",
    localization::Tr("table.gTownCommand.10"), localization::Tr("table.gTownCommand.11"),
    localization::Tr("table.gTownCommand.12"), localization::Tr("table.gTownCommand.13"),
    localization::Tr("table.gTownCommand.14"), localization::Tr("table.gTownCommand.15"),
    localization::Tr("table.gTownCommand.16"), localization::Tr("table.gTownCommand.17"),
    localization::Tr("table.gTownCommand.18"), localization::Tr("table.gTownCommand.19"),
    localization::Tr("table.gTownCommand.20"), localization::Tr("table.gTownCommand.21"),
};
DATA(0x004939f0)
char* gGameTypeHelp[5] = {
    "Play a single, standard game against computer opponents.",
    "Play the campaign game - a series of linked single games.",
    "Play against other human players, either sitting at the same computer, or linked through a "
    "network or modem.",
    "Play a practice game.",
    "Cancel out of this menu back to the main menu.",
};
DATA(0x00493a08)
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
DATA(0x00493070)
char* gCPanelHelp[12] = {
    localization::Tr("table.gCPanelHelp.0"),
    localization::Tr("table.gCPanelHelp.1"),
    localization::Tr("table.gCPanelHelp.2"),
    localization::Tr("table.gCPanelHelp.3"),
    localization::Tr("table.gCPanelHelp.4"),
    localization::Tr("table.gCPanelHelp.5"),
    localization::Tr("table.gCPanelHelp.6"),
    localization::Tr("table.gCPanelHelp.7"),
    localization::Tr("table.gCPanelHelp.8"),
    localization::Tr("table.gCPanelHelp.9"),
    localization::Tr("table.gCPanelHelp.10"),
    localization::Tr("table.gCPanelHelp.11")
};
DATA(0x00493b58)
char* gNewGameHelp[9] = {
    localization::Tr("table.gNewGameHelp.0"),
    localization::Tr("table.gNewGameHelp.1"),
    localization::Tr("table.gNewGameHelp.2"),
    localization::Tr("table.gNewGameHelp.3"),
    localization::Tr("table.gNewGameHelp.4"),
    localization::Tr("table.gNewGameHelp.5"),
    localization::Tr("table.gNewGameHelp.6"),
    localization::Tr("table.gNewGameHelp.7"),
    localization::Tr("table.gNewGameHelp.8"),
};
DATA(0x004930c4)
char* gSetupCampaignGameHelp[5] = {
    localization::Tr("table.gSetupCampaignGameHelp.0"),
    localization::Tr("table.gSetupCampaignGameHelp.1"),
    localization::Tr("table.gSetupCampaignGameHelp.2"),
    localization::Tr("table.gSetupCampaignGameHelp.3"),
    localization::Tr("table.gSetupCampaignGameHelp.4"),
};
DATA(0x004930d8)
char* gSetupBaudHelp[5] = {
    localization::Tr("table.gSetupBaudHelp.0"),
    localization::Tr("table.gSetupBaudHelp.1"),
    localization::Tr("table.gSetupBaudHelp.2"),
    localization::Tr("table.gSetupBaudHelp.3"),
    localization::Tr("table.gSetupBaudHelp.4"),
};
DATA(0x00493bb0)
char* gSetupComPortHelp[5] = {
    "Use COM Port 1 for the modem connection.",
    "Use COM Port 2 for the modem connection.",
    "Use COM Port 3 for the modem connection.",
    "Use COM Port 4 for the modem connection.",
    "Cancel back to the main menu.",
};
DATA(0x00493100)
char* gSetupDCBaudHelp[5] = {
    localization::Tr("table.gSetupDCBaudHelp.0"),
    localization::Tr("table.gSetupDCBaudHelp.1"),
    localization::Tr("table.gSetupDCBaudHelp.2"),
    localization::Tr("table.gSetupDCBaudHelp.3"),
    localization::Tr("table.gSetupDCBaudHelp.4"),
};
DATA(0x00493be0)
char* gSetupDCComPortHelp[5] = {
    "Use COM Port 1 for the direct connection.",
    "Use COM Port 2 for the direct connection.",
    "Use COM Port 3 for the direct connection.",
    "Use COM Port 4 for the direct connection.",
    "Cancel back to the main menu.",
};
DATA(0x00493128)
char* gSetupHotSeatGameHelp[4] = {
    localization::Tr("table.gSetupHotSeatGameHelp.0"),
    localization::Tr("table.gSetupHotSeatGameHelp.1"),
    localization::Tr("table.gSetupHotSeatGameHelp.2"),
    localization::Tr("table.gSetupHotSeatGameHelp.3"),
};
DATA(0x00493138)
char* gSetupModemGameHelp[4] = {
    localization::Tr("table.gSetupModemGameHelp.0"),
    localization::Tr("table.gSetupModemGameHelp.1"),
    localization::Tr("table.gSetupModemGameHelp.2"),
    localization::Tr("table.gSetupModemGameHelp.3"),
};
DATA(0x00493148)
char* gSetupDCGameHelp[4] = {
    localization::Tr("table.gSetupDCGameHelp.0"),
    localization::Tr("table.gSetupDCGameHelp.1"),
    localization::Tr("table.gSetupDCGameHelp.2"),
    localization::Tr("table.gSetupDCGameHelp.3"),
};
DATA(0x00493158)
char* gSetupMultiPlayerGameHelp[5] = {
    localization::Tr("table.gSetupMultiPlayerGameHelp.0"),
    localization::Tr("table.gSetupMultiPlayerGameHelp.1"),
    localization::Tr("table.gSetupMultiPlayerGameHelp.2"),
    localization::Tr("table.gSetupMultiPlayerGameHelp.3"),
    localization::Tr("table.gSetupMultiPlayerGameHelp.4"),
};
DATA(0x0049316c)
char* gSetupNetworkGameHelp[3] = {
    localization::Tr("table.gSetupNetworkGameHelp.0"),
    localization::Tr("table.gSetupNetworkGameHelp.1"),
    localization::Tr("table.gSetupNetworkGameHelp.2"),
};
DATA(0x00493178)
char* gSetupGameHelp[4] = {
    localization::Tr("table.gSetupGameHelp.0"),
    localization::Tr("table.gSetupGameHelp.1"),
    localization::Tr("table.gSetupGameHelp.2"),
    localization::Tr("table.gSetupGameHelp.3"),
};
DATA(0x00493188)
char* gBattleResults[11] = {
    localization::Tr("table.gBattleResults.0"),
    localization::Tr("table.gBattleResults.1"),
    localization::Tr("table.gBattleResults.2"),
    localization::Tr("table.gBattleResults.3"),
    localization::Tr("table.gBattleResults.4"),
    localization::Tr("table.gBattleResults.5"),
    localization::Tr("table.gBattleResults.6"),
    localization::Tr("table.gBattleResults.7"),
    localization::Tr("table.gBattleResults.8"),
    localization::Tr("table.gBattleResults.9"),
    localization::Tr("table.gBattleResults.10"),
};
DATA(0x00493c90)
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
DATA(0x00493cb0)
char* gMoraleInfoText[21] = {
    localization::Tr("table.gMoraleInfoText.0"),  localization::Tr("table.gMoraleInfoText.1"),
    localization::Tr("table.gMoraleInfoText.2"),  localization::Tr("table.gMoraleInfoText.3"),
    localization::Tr("table.gMoraleInfoText.4"),  localization::Tr("table.gMoraleInfoText.5"),
    localization::Tr("table.gMoraleInfoText.6"),  localization::Tr("table.gMoraleInfoText.7"),
    localization::Tr("table.gMoraleInfoText.8"),  localization::Tr("table.gMoraleInfoText.9"),
    localization::Tr("table.gMoraleInfoText.10"), localization::Tr("table.gMoraleInfoText.11"),
    localization::Tr("table.gMoraleInfoText.12"), localization::Tr("table.gMoraleInfoText.13"),
    localization::Tr("table.gMoraleInfoText.14"), localization::Tr("table.gMoraleInfoText.15"),
    localization::Tr("table.gMoraleInfoText.16"), localization::Tr("table.gMoraleInfoText.17"),
    localization::Tr("table.gMoraleInfoText.18"), localization::Tr("table.gMoraleInfoText.19"),
    localization::Tr("table.gMoraleInfoText.20"),
};
DATA(0x00493d08)
char* gMapSizeNames[3] = {
    localization::Tr("table.gMapSizeNames.0"),
    localization::Tr("table.gMapSizeNames.1"),
    localization::Tr("table.gMapSizeNames.2")
};
DATA(0x00493d18)
char* gMapDifficultyNames[5] = {
    localization::Tr("table.gMapDifficultyNames.0"),
    localization::Tr("table.gMapDifficultyNames.1"),
    localization::Tr("table.gMapDifficultyNames.2"),
    localization::Tr("table.gMapDifficultyNames.3"),
    localization::Tr("table.gMapDifficultyNames.4")
};
DATA(0x00493244)
char* gCampaignScenarioNames[9] = {
    localization::Tr("table.gCampaignScenarioNames.0"),
    localization::Tr("table.gCampaignScenarioNames.1"),
    localization::Tr("table.gCampaignScenarioNames.2"),
    localization::Tr("table.gCampaignScenarioNames.3"),
    localization::Tr("table.gCampaignScenarioNames.4"),
    localization::Tr("table.gCampaignScenarioNames.5"),
    localization::Tr("table.gCampaignScenarioNames.6"),
    localization::Tr("table.gCampaignScenarioNames.7"),
    localization::Tr("table.gCampaignScenarioNames.8"),
};
DATA(0x00493d58)
char* gCampaignWinTexts[9] = {
    localization::Tr("table.gCampaignWinTexts.0"),
    localization::Tr("table.gCampaignWinTexts.1"),
    localization::Tr("table.gCampaignWinTexts.2"),
    localization::Tr("table.gCampaignWinTexts.3"),
    localization::Tr("table.gCampaignWinTexts.4"),
    localization::Tr("table.gCampaignWinTexts.5"),
    localization::Tr("table.gCampaignWinTexts.6"),
    localization::Tr("table.gCampaignWinTexts.7"),
    localization::Tr("table.gCampaignWinTexts.8"),
};
DATA(0x0049328c)
char* gCampaignScenarioText[9] = {
    localization::Tr("table.gCampaignScenarioText.0"),
    localization::Tr("table.gCampaignScenarioText.1"),
    localization::Tr("table.gCampaignScenarioText.2"),
    localization::Tr("table.gCampaignScenarioText.3"),
    localization::Tr("table.gCampaignScenarioText.4"),
    localization::Tr("table.gCampaignScenarioText.5"),
    localization::Tr("table.gCampaignScenarioText.6"),
    localization::Tr("table.gCampaignScenarioText.7"),
    localization::Tr("table.gCampaignScenarioText.8"),
};
DATA(0x00493da8)
char* gDifficultyNames[4] = {"Easy", "Normal", "Hard", "Expert"};
DATA(0x00493db8)
char* gCampaignSideNames[4] = {
    localization::Tr("table.gCampaignSideNames.0"),
    localization::Tr("table.gCampaignSideNames.1"),
    localization::Tr("table.gCampaignSideNames.2"),
    localization::Tr("table.gCampaignSideNames.3")
};
DATA(0x00493dc8)
char* gScoreLabels[CONGRATS_SCORE_LABEL_COUNT] =
    {"Days Spent:", "Base Score:", "Difficulty Rating:", "Final Score:", "Ranking:"};
DATA(0x004932e4)
char* gHumanPlayerTypeNames[5] = {
    localization::Tr("table.gHumanPlayerTypeNames.0"),
    localization::Tr("table.gHumanPlayerTypeNames.1"),
    localization::Tr("table.gHumanPlayerTypeNames.2"),
    localization::Tr("table.gHumanPlayerTypeNames.3"),
    localization::Tr("table.gHumanPlayerTypeNames.4")
};
DATA(0x00493df8)
char* gHandicapNames[5] = {
    localization::Tr("table.gHandicapNames.0"),
    localization::Tr("table.gHandicapNames.1"),
    localization::Tr("table.gHandicapNames.2"),
    localization::Tr("table.gHandicapNames.3"),
    localization::Tr("table.gHandicapNames.4")
};
DATA(0x0049330c)
char* musicQualityText[3] = {
    localization::Tr("table.musicQualityText.0"),
    localization::Tr("table.musicQualityText.1"),
    localization::Tr("table.musicQualityText.2")
};
DATA(0x00493e20)
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
DATA(0x00493f30)
i32 gRequiredExtendedMemory = 4434;
DATA(0x00493f34)
i32 gRequiredConventionalMemory = 374;
DATA(0x004a9918)
i32 gMapSize = 0;
DATA(0x004a991c)
i32 gMapDifficulty = 0;
DATA(0x00493f40)
i8 gHeroWindShowing = 0;
DATA(0x00493f44)
i8 gOverviewShowing = 0;
DATA(0x00493f48)
i32 gFullCombatScreenDrawn = 1;
DATA(0x004a9924)
i32 gLimitedCombatUpdatePalette = 0;
DATA(0x004a9928)
i8 gFirstTimeThrough = 0;
DATA(0x00493f54)
i8 gSkipIntro = 0;
DATA(0x004a992c)
i32 gAllBlack = 0;
DATA(0x004a9930)
i8 gInCombat = 0;
DATA(0x004a9931)
i8 gDirectConnect = 0;
DATA(0x00493434)
i32 gForceSwitchMusic = FORCED_MUSIC_IDLE;
DATA(0x004a9934)
i32 gComputeExtent = 0;
DATA(0x004a9938)
i32 gSaveBiggestExtent = 0;
DATA(0x004a993c)
i32 gLimitToExtent = 0;
DATA(0x00493438)
i32 gCurrArmyDrawn = 1;
DATA(0x004a9940)
i32 gAdvDisposeLevel = 0;
DATA(0x004a9944)
i32 gRemoteOn = 0;
DATA(0x00493f80)
i8 gGameInitialized = 0;
DATA(0x00493f84)
i8 gHighScoreRank = -1;
DATA(0x00493f88)
i8 gShowHighScore = 0;
DATA(0x00493440)
i32 gHighMemBuffer = 4000;
DATA(0x00493f98)
i8 gInPollSound = 0;
// Retail places these zero-initialized flags among KB's function literals
// (0x0049e8b0-0x0049f537), each next to the literals of its only user.
DATA(0x0049e8b4)
i8 gKBDone = 0;
DATA(0x0049ee58)
i8 gInCheckEndGame = 0;
// KB owns retail .bss 0x004c5138-0x004c7e6f (allocation order is the compiler's
// symbol-hash walk, not definition order).
#include <SOURCE/combatTypes.h>
#include <SOURCE/mapCell.h>

DATA(0x004a9414)
i32 gbHumanPlayer[4];
DATA(0x004a7b74)
i32 giMaxExtentX;
DATA(0x004a7b78)
i32 giMaxExtentY;
DATA(0x004a74e0)
class font* smallFont;
DATA(0x004a7bb0)
i32 giBottomViewOverrideEndTime;
DATA(0x004a98c0)
i8 gArmyEffected[COMBAT_SIDE_COUNT][ARMY_GROUP_SLOT_COUNT];
DATA(0x004a74c4)
i32 giBottomViewResource;
DATA(0x004a9408)
i32 giSeedingValid;
DATA(0x004a82f0)
i8 giLimitPlayer;
DATA(0x004a7b90)
inputManager* gpInputManager;
DATA(0x004a762c)
i32 iMaxMapExtra;
DATA(0x004a7834)
palette* gPalette;
DATA(0x004a74d8)
resourceManager* gpResourceManager;
DATA(0x004a7fc0)
u8 mapExtra[MAP_CELL_GRID_SIZE][MAP_CELL_GRID_SIZE];
DATA(0x004a7bec)
i32 bSpecialHideCursor;
DATA(0x004a74ac)
class searchArray* gpSearchArray;
DATA(0x004a7b7c)
i32 gbBlackoutPlayer;
DATA(0x004a7838)
char cNetBoxLine[2][60];
DATA(0x004a7b98)
heroWindow* DataEntryWin;
DATA(0x004a7bf0)
i8 giWeekTypeExtra;
DATA(0x004a74e4)
philAI* gpPhilAI;
DATA(0x004a7bac)
char* cDEDest;
DATA(0x004a74ec)
heroWindow* gNormalDialogWindow;
DATA(0x004a7b70)
i32 giHostGamePos;
DATA(0x004a7164)
mouseManager* gpMouseManager;
DATA(0x004a7468)
class font* bigFont;
DATA(0x004a7fb8)
class icon* gSystemIcons;
DATA(0x004a7495)
i8 gbCombatSurrender;
DATA(0x004a82c0)
char gMapName[16];
DATA(0x004a9560)
i32 giMinExtentX;
DATA(0x004a9564)
i32 giMinExtentY;
DATA(0x004a7b9c)
i8 iMPBaseType;
DATA(0x004a86fc)
class hero* gHVHero;
DATA(0x004a747c)
i32 giHeroScreenSrcIndex;
DATA(0x004a7bb0)
i8 giWeekType;
DATA(0x004a7168)
char gText[768];
DATA(0x004a7bfc)
i32 gbInNewGameSetup;
DATA(0x004a7830)
palette* gpBufferPalette;
DATA(0x004a7ba8)
i8 giMonthTypeExtra;
DATA(0x004a74a0)
i8 iMPExtendedType;
DATA(0x004a74b0)
char gFullMapName[20];
DATA(0x004a7d48)
i32 giShowIntro;
DATA(0x004a98a0)
i32 glTimers[GLOBAL_TIMER_COUNT];
DATA(0x004a8704)
i32 giScore;
DATA(0x004a9404)
armyGroup* gpMonGroup;
DATA(0x004a9428)
configStruct gConfig;
DATA(0x004a8150)
char gcRegAppPath[352];
DATA(0x004a9410)
i8 gCampaignChoice;
DATA(0x004a7ba0)
class game* gpGame;
DATA(0x004a7823)
i8 gbRetreatWin;
DATA(0x004a7b8d)
H1_ENUM_STORAGE(DialogWaitType, i8) giWaitType;
DATA(0x004a74e8)
i16 gCurLoadedSpellFileId;
DATA(0x004a761c)
i32 giBottomViewOverride;
DATA(0x004a76c4)
char gLastFilename[352];
DATA(0x004a7470)
class icon* gBuyBuildIcons;
DATA(0x004a74ea)
i8 gbNoSound;
DATA(0x004a956c)
char gcBottomViewText[92];
DATA(0x004a74d4)
i32 giThisNetPos;
DATA(0x004a9e98)
char gcRegCDRomPath[352];
DATA(0x004a7628)
class heroWindow* heroWin;
DATA(0x004a9400)
class icon* gCurLoadedSpellIcon;
DATA(0x004a7bb8)
void* ppMapExtra[255];
DATA(0x004a989c)
i32 giCurGeneral;
DATA(0x004a7474)
i32 giThisGamePos;
DATA(0x004a749c)
i32 giNumHumanPlayers;
DATA(0x004a955c)
i8 gbIconClipOn;
DATA(0x004a73c8)
i32 pwSizeOfMapExtra[255];
DATA(0x004a95cc)
i32 iDEMaxLen;
DATA(0x004a7828)
class combatManager* gpCombatManager;
DATA(0x004a7478)
i16 gSpellEffectFrame;
DATA(0x004a95c8)
executive* gpExec;
DATA(0x004a7638)
i8 giGroundToTerrain[140];
DATA(0x004a9734)
i32 giCurWindowsStyleFlags;
DATA(0x004a746c)
H1_ENUM_STORAGE(MainMenuControl, i16) gGameCommand;
DATA(0x004a9e90)
i8 giMonthType;
DATA(0x004a6c4c)
char gMapDescription[124];
DATA(0x004a7498)
char* DEFAULT_AGGREGATE_NAME;
DATA(0x004a7b94)
i8 gbThisNetHumanPlayer[4];
DATA(0x004a7ff0)
char cAggPathName[352];
DATA(0x004a7634)
class highScoreManager* gpHighScoreManager;
DATA(0x004a70c4)
i8 gbFunctionComplete;
DATA(0x004a7ba8)
i8 gbIAmGreatest;
DATA(0x004a7824)
i16 gMapX;
DATA(0x004a7826)
i16 gMapY;
DATA(0x004a7c18)
char gcWinText[300];
DATA(0x004a7493)
i8 bDataEntryTime;
DATA(0x004a98bc)
i32 bShowIt;
DATA(0x004a6c48)
i32 giDebugLevel;
DATA(0x004a74a8)
heroWindowManager* gpWindowManager;
DATA(0x004a74a4)
i32 giCurWatchPlayer;
DATA(0x004a9558)
i32 giBottomViewResourceQty;
DATA(0x004a74c8)
i8 gbWaitForRemoteReceive;
DATA(0x004a95d0)
char gLastMapName[352];
DATA(0x004a940c)
townManager* gpTownManager;
DATA(0x004a782c)
i8 giScreenScroll;
DATA(0x004a7480)
advManager* gpAdvManager;
DATA(0x004a82fc)
i8 gbGamePosToNetPos[4];

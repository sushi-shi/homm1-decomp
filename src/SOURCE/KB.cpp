#include <H1/Ints.h>

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
#include <SOURCE/EVENTS.h>
#include <SOURCE/kbwin.h>
#include <SOURCE/dialogTypes.h>
#include <SOURCE/NOOPT.h>
#include <SOURCE/REMOTE.h>
#include <SOURCE/remoteRecords.h>
#include <SOURCE/saveRecords.h>
#include <SOURCE/resourceTypes.h>
#include <SOURCE/smackManager.h>
#include <SOURCE/wingraph.h>

#include <PLATFORM/File.h>
#include <PLATFORM/Records.h>

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

i8 gHighScoreType;
i8 gTerrainCost[FINDPATH_TERRAIN_COUNT][FINDPATH_STEP_COST_COUNT];

void PollSound() {
    if (gTimers[GLOBAL_POLL_SOUND_TIMER_SLOT] > KBTickCount())
        return;
    if (gInPollSound)
        return;
    gInPollSound = true;
    gTimers[GLOBAL_POLL_SOUND_TIMER_SLOT] = KBTickCount() + 30;
    PollRemote();
    gInPollSound = false;
}

void ForcePollSound() {
    gTimers[GLOBAL_POLL_SOUND_TIMER_SLOT] = KBTickCount() - 1;
    PollSound();
}

void InitMainClasses(void) {
    gExec = new executive;
    gInputManager = new inputManager;
    gMouseManager = new mouseManager;
    gWindowManager = new heroWindowManager;
    gResourceManager = new resourceManager;
    gHighScoreManager = new highScoreManager;
    gGame = new game;
    gAdvManager = new advManager;
    gCombatManager = new combatManager;
    gTownManager = new townManager;
    gSearchArray = new searchArray;
    gPhilAI = new philAI;
    gMonGroup = new armyGroup;
    gBufferPalette = new palette;
}

void DeleteMainClasses(void) {
    if (gBufferPalette)
        delete gBufferPalette;
    gBufferPalette = NULL;
    if (gMonGroup)
        delete gMonGroup;
    gMonGroup = NULL;
    if (gPhilAI)
        delete gPhilAI;
    gPhilAI = NULL;
    if (gSearchArray)
        delete gSearchArray;
    gSearchArray = NULL;
    if (gTownManager)
        delete gTownManager;
    gTownManager = NULL;
    if (gCombatManager)
        delete gCombatManager;
    gCombatManager = NULL;
    if (gAdvManager)
        delete gAdvManager;
    gAdvManager = NULL;
    if (gGame)
        delete gGame;
    gGame = NULL;
    if (gHighScoreManager)
        delete gHighScoreManager;
    gHighScoreManager = NULL;
    if (gResourceManager)
        delete gResourceManager;
    gResourceManager = NULL;
    if (gWindowManager)
        delete gWindowManager;
    gWindowManager = NULL;
    if (gMouseManager)
        delete gMouseManager;
    gMouseManager = NULL;
    if (gInputManager)
        delete gInputManager;
    gInputManager = NULL;
    if (gExec)
        delete gExec;
    gExec = NULL;
}

b8 gDrawSavedCursor = false;
b32 gLoadingMonoIcon = false;
i32 gScrollX = 0;
i32 gScrollY = 0;
b32 gNoBorder = false;
KBMenu gDefaultMenu = NULL;
KBMenu gCombatMenu = NULL;
KBMenu gAdventureMenu = NULL;
KBMenu gTownMenu = NULL;
i32 gColorMice = 0;
i32 gSpecialMouseMasks = 0;
i32 gCurExe = CONFIG_EXECUTABLE_GAME;
b32 gInDialog = false;
b32 gInSetupDialog = false;
i32 gMinimized = 0;
b32 gHeroMoving = false;
b32 gInSmacker = false;
i32 gUnusedKBCount = 0;
b32 gRemoteReady = false;
b32 gHeartbeatSeen = false;
i32 gMapSize = MAP_SIZE_SMALL;
i32 gMapDifficulty = MAP_DIFFICULTY_EASY;
b8 gHeroWindShowing = false;
b8 gOverviewShowing = false;
b32 gLimitedCombatUpdatePalette = false;
b8 gFirstTimeThrough = false;
b8 gSkipIntro = false;
b32 gAllBlack = false;
b8 gInCombat = false;
i8 gDirectConnect = 0;
b32 gComputeExtent = false;
b32 gSaveBiggestExtent = false;
b32 gLimitToExtent = false;
i32 gAdvDisposeLevel = ADV_DISPOSE_NONE;
b32 gRemoteOn = false;
b8 gGameInitialized = false;
b8 gShowHighScore = false;
i32 gUnusedKBTicks = 0;
i8 gUnusedKBFlag = 0;
b8 gInPollSound = false;

b32 EarlySetup(void) {
    static b8 gEarlySetupDone = false;

    if (gEarlySetupDone)
        return false;
    InitMainClasses();
    GetGraphicsInfo();
    ReadPrefs();
    if (!InterpretCommandLine())
        return true;
    InitVars();
    return true;
}

i32 oldmain(void) {
    char saveBuf[20];
    char endVideos[GAME_END_SEQUENCE_COUNT];
    i32 netIndex;
    heroWindow* mainMenuWindow;
    font* textFont;
    b8 backdropLoaded;
    b8 initialScreen;
    i32 gamePlayer;
    i32 sendResult;
    b8 creditsDone;
    b8 leave;
    i16 command;

    if (gKBDone)
        return 0;
    gKBDone = true;
    command = MAIN_MENU_NO_COMMAND;
    if ((gExec->InitSystem()))
        ShutDown(localization::Tr("startup.initialize.failed"));
    CheckMem();
    KBChangeMenu(gDefaultMenu);
    gPalette = gResourceManager->GetPalette("kb.pal");
    PostprocessPalette(gPalette->m_data);
    SetPalette(gPalette->m_data, true);
    gWindowManager->m_updateFlags = 1;
    gPhilAI->m_debugFont = gResourceManager->GetFont("smalfont.fnt");
    if (gShowIntro) {
        FillBitmapArea(
            gWindowManager->m_screen,
            0,
            0,
            LOGICAL_SCREEN_WIDTH,
            LOGICAL_SCREEN_HEIGHT,
            0
        );
        BlitBitmapToScreen(
            gWindowManager->m_screen,
            0,
            0,
            LOGICAL_SCREEN_WIDTH,
            LOGICAL_SCREEN_HEIGHT,
            0,
            0
        );
        textFont = gResourceManager->GetFont("bigfont.fnt");
        textFont->DrawString(localization::Tr("startup.loading.game"), 10, 10, 1);
        gWindowManager->UpdateScreenRegion(10, 10, 600, 20);
        gResourceManager->Dispose(textFont);
        if (!gSkipIntro && PlaySmacker(SMACK_BUKA) && PlaySmacker(SMACK_NWCLOGO))
            PlaySmacker(SMACK_INTRO);
    }
    LoadSystemwideIcons();
    memset(gThisNetHumanPlayer, 0, GAME_PLAYER_COUNT);
    leave = false;
    backdropLoaded = false;
    initialScreen = true;

    while (!leave) {
        ClearCheatState();
    mainMenu:
        PlayMusic(MUSIC_TRACK_MAIN_MENU);
        if (!backdropLoaded) {
            if (gGameCommand != MAIN_MENU_QUIT) {
                gResourceManager->GetBackdrop("heroes.bmp", gWindowManager->m_screen);
                gWindowManager
                    ->UpdateScreenRegion(0, 0, LOGICAL_SCREEN_WIDTH, LOGICAL_SCREEN_HEIGHT);
                if (initialScreen)
                    SetPalette(gPalette->m_data, false);
                else
                    gWindowManager->FadeScreen(WINDOW_FADE_IN, WINDOW_FADE_SHORT, gPalette);
                initialScreen = false;
            }
            gMouseManager->SetPointer("advmice.mse", ADVENTURE_POINTER_DEFAULT);
        }
        backdropLoaded = true;
        if (gGameCommand != MAIN_MENU_QUIT)
            gWindowManager->m_updateFlags = 1;
        gCampaignChoice = CAMPAIGN_NONE;
        gMouseManager->ReallyShowPointer();

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
                    if (!gGame->PickLoadGame())
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
                    if (!gGame->NewGame())
                        goto mainMenu;
                    break;
            }
            goto gameSetupComplete;
        } else {
            if (gGameCommand != MAIN_MENU_NO_COMMAND) {
                command = gGameCommand;
                gGameCommand = MAIN_MENU_NO_COMMAND;
            } else {
                mainMenuWindow = new heroWindow(400, 35, "stpmain.bin");
                if (!mainMenuWindow)
                    MemError();
                gInSetupDialog = true;
                gWindowManager->DoDialog(mainMenuWindow, InitMenuHandler, false);
                delete mainMenuWindow;
                command = gWindowManager->m_dialogResult;
                gInSetupDialog = false;
            }
        }
        if (gMenuCommand != APP_MENU_NONE)
            goto processMenuCommand;

        gMouseManager->ReallyHidePointer();
        switch (command) {
            case MAIN_MENU_LOAD_GAME:
                if (!gGame->PickLoadGame())
                    goto mainMenu;
                break;
            case MAIN_MENU_QUICK_LOAD:
                gGame->LoadGame(gLastFilename, false, false);
                if (gGame->m_campaignType > 0)
                    gCampaignChoice = gGame->m_campaignType;
                break;
            case MAIN_MENU_HIGH_SCORES:
                if ((gExec->AddManager(gHighScoreManager, BASE_MANAGER_PRIORITY_UNASSIGNED)))
                    ShutDown(localization::Tr("startup.manager.failed"));
                gExec->MainLoop();
                gExec->RemoveManager(gHighScoreManager);
                backdropLoaded = false;
                goto mainMenu;
            case MAIN_MENU_NEW_GAME:
                if (!gGame->NewGame())
                    goto mainMenu;
                break;
            case MAIN_MENU_CREDITS:
                gWindowManager->FadeScreen(WINDOW_FADE_OUT, WINDOW_FADE_SHORT, gPalette);
                gResourceManager->GetBackdrop("credits.bmp", gWindowManager->m_screen);
                gWindowManager
                    ->UpdateScreenRegion(0, 0, LOGICAL_SCREEN_WIDTH, LOGICAL_SCREEN_HEIGHT);
                gWindowManager->FadeScreen(WINDOW_FADE_IN, WINDOW_FADE_SHORT, gPalette);
                creditsDone = false;
                gInputManager->Flush();
                while (!creditsDone) {
                    Process1WindowsMessage();
                    switch (gInputManager->GetEvent().type) {
                        case MESSAGE_KEY_DOWN:
                        case MESSAGE_LEFT_BUTTON_DOWN:
                        case MESSAGE_RIGHT_BUTTON_DOWN:
                            creditsDone = true;
                    }
                }
                gWindowManager->FadeScreen(WINDOW_FADE_OUT, WINDOW_FADE_SHORT, gPalette);
                gResourceManager->GetBackdrop("heroes.bmp", gWindowManager->m_screen);
                gWindowManager
                    ->UpdateScreenRegion(0, 0, LOGICAL_SCREEN_WIDTH, LOGICAL_SCREEN_HEIGHT);
                gWindowManager->FadeScreen(WINDOW_FADE_IN, WINDOW_FADE_SHORT, gPalette);
                goto mainMenu;
            case MAIN_MENU_QUIT:
                leave = true;
                break;
        }

    gameSetupComplete:
        if (gMenuCommand != APP_MENU_NONE)
            goto processMenuCommand;
        if (!leave) {
            if (gRemoteOn && !gThisNetPos) {
                RecordWriter setupRecord;
                netIndex = NET_POSITION_HOST;
                for (gamePlayer = 0; gamePlayer < GAME_PLAYER_COUNT; gamePlayer++) {
                    if (gHumanPlayer[gamePlayer]) {
                        gGamePosToNetPos[gamePlayer] = netIndex;
                        netIndex++;
                    } else {
                        gGamePosToNetPos[gamePlayer] = NET_POSITION_NONE;
                    }
                }
                WriteRemoteSetup(setupRecord, gGamePosToNetPos);
                gHostGamePos = NetPosToGamePos(NET_POSITION_HOST);
                gThisGamePos = gHostGamePos;
                for (gamePlayer = NET_POSITION_FIRST_GUEST; gamePlayer < gNumHumanPlayers; gamePlayer++) {
                    sendResult = TransmitRemoteRecord(
                        setupRecord,
                        gamePlayer,
                        BOX_REMOTE_SETUP,
                        true,
                        true,
                        REMOTE_MESSAGE_DEFAULT,
                        false
                    );
                    if (!sendResult)
                        ShutDown(NULL);
                }
                for (gamePlayer = 0; gamePlayer < gGame->m_playerCount; gamePlayer++) {
                    if (gHumanPlayer[gamePlayer] && !gThisNetHumanPlayer[gamePlayer]) {
                        if (!gGame->TransmitSaveGame(gamePlayer, 0))
                            ShutDown(NULL);
                    }
                }
            }
            if (gRemoteOn && gWaitForRemoteReceive) {
                gWaitType = DIALOG_WAIT_OTHER_PLAYER;
                NormalDialog(
                    localization::Tr("network.setup.wait"),
                    NORMAL_DIALOG_TYPE_WAIT_CANCEL
                );
                if (!gFunctionComplete)
                    ShutDown(NULL);
                gGame->LoadGame("REMOTE.GAM", false, true);
                goto playScenario;
            }
        playScenario:
            if (gGame->m_campaignType > 0) {
                if (!backdropLoaded) {
                    gWindowManager->FadeScreen(WINDOW_FADE_OUT, WINDOW_FADE_SHORT, gPalette);
                    gResourceManager->GetBackdrop("heroes.bmp", gWindowManager->m_screen);
                    gWindowManager
                        ->UpdateScreenRegion(0, 0, LOGICAL_SCREEN_WIDTH, LOGICAL_SCREEN_HEIGHT);
                    gWindowManager->FadeScreen(WINDOW_FADE_IN, WINDOW_FADE_SHORT, gPalette);
                    backdropLoaded = true;
                }
                gGame->ShowCampaignInfo(gGame->m_campaignScenario, false, 0);
            }
            gGameInitialized = true;
            backdropLoaded = false;
            StopAllAudio();
            gWindowManager->FadeScreen(WINDOW_FADE_OUT, WINDOW_FADE_SHORT, NULL);
            gMapX = 0;
            gMapY = 0;
            if ((gExec->AddManager(gAdvManager, BASE_MANAGER_PRIORITY_UNASSIGNED)))
                ShutDown(localization::Tr("startup.manager.failed"));
            if (command == MAIN_MENU_NEW_GAME)
                gAdvManager->SetHeroContext(gGame->m_players[0].NextHero(0), false);
            gExec->MainLoop();
            gMapX = gAdvManager->m_mapOriginX;
            gMapY = gAdvManager->m_mapOriginY;
            gExec->RemoveManager(gAdvManager);
            gWindowManager->FadeScreen(WINDOW_FADE_OUT, WINDOW_FADE_SHORT, gPalette);
        }

        if (gGameOver) {
            RemoteCleanup();
            gShowIt = true;
            gMouseManager->SetPointer("advmice.mse", ADVENTURE_POINTER_DEFAULT);
            gMouseManager->ReallyHidePointer();
            sprintf(gWinText, localization::Tr("campaign.victory"), gCurTurn);
            endVideos[GAME_END_LOST] = SMACK_LOSE;
            endVideos[GAME_END_WON] = SMACK_WIN1;
            endVideos[GAME_END_CAMPAIGN_COMPLETE] = SMACK_WIN2;
            // The campaign's closing words come before its closing videos.
            if (gEndSequence == GAME_END_CAMPAIGN_COMPLETE)
                ShowCongrats();
            if (gEndSequence != GAME_END_WON) {
                if (gEndSequence == GAME_END_CAMPAIGN_COMPLETE) {
                    PlaySmacker(SMACK_WIN1);
                    PlaySmacker(SMACK_WIN2);
                } else {
                    PlaySmacker(endVideos[gEndSequence]);
                }
                gResourceManager->GetBackdrop("heroes.bmp", gWindowManager->m_screen);
                gWindowManager
                    ->UpdateScreenRegion(0, 0, LOGICAL_SCREEN_WIDTH, LOGICAL_SCREEN_HEIGHT);
                gWindowManager->FadeScreen(WINDOW_FADE_IN, WINDOW_FADE_SHORT, gPalette);
                gWindowManager->m_updateFlags = 1;
                backdropLoaded = true;
            } else {
                ShowCongrats();
            }
            gGameOver = false;
            if (gEndSequence == GAME_END_CAMPAIGN_COMPLETE) {
                PlayMusic(MUSIC_TRACK_CONGRATULATIONS);
                AddScoreToHighScore(
                    gCurTurn,
                    HIGH_SCORE_TYPE_CAMPAIGN,
                    "",
                    gCampaignSideNames
                        [gGame->m_campaignType - CAMPAIGN_IRONFIST]
                );
            }
            if (gShowHighScore) {
                gMouseManager->ReallyShowPointer();
                if ((gExec->AddManager(gHighScoreManager, BASE_MANAGER_PRIORITY_UNASSIGNED)))
                    ShutDown(localization::Tr("startup.manager.failed"));
                gExec->MainLoop();
                gExec->RemoveManager(gHighScoreManager);
                gHighScoreRank = HIGH_SCORE_EMPTY;
                PlayMusic(MUSIC_TRACK_MAIN_MENU);
                gResourceManager->GetBackdrop("heroes.bmp", gWindowManager->m_screen);
                gWindowManager
                    ->UpdateScreenRegion(0, 0, LOGICAL_SCREEN_WIDTH, LOGICAL_SCREEN_HEIGHT);
                gWindowManager->FadeScreen(WINDOW_FADE_IN, WINDOW_FADE_SHORT, gPalette);
                backdropLoaded = true;
            }
            if (gGame->m_campaignType > 0) {
                if (gEndSequence == GAME_END_LOST) {
                    sprintf(gText, localization::Tr("campaign.replay.confirm"));
                    NormalDialog(gText, NORMAL_DIALOG_TYPE_YES_NO);
                    if (gWindowManager->m_dialogResult == NORMAL_DIALOG_CONFIRM) {
                        gGame->InitCampaignMap(gGame->m_campaignScenario, 0);
                        goto playScenario;
                    }
                } else if (gEndSequence == GAME_END_WON) {
                    gGame->m_campaignDay = gCurTurn + 1;
                    gGame->m_campaignScenario++;
                    gGame->m_campaignScenariosWon++;
                    if (gGame->m_campaignScenario - CAMPAIGN_SCENARIO_LORD_FIRST
                        == gGame->m_campaignType
                               - CAMPAIGN_IRONFIST)
                        gGame->m_campaignScenario++;
                    gGame->InitCampaignMap(gGame->m_campaignScenario, 0);
                    sprintf(
                        saveBuf,
                        "%s%02d",
                        localization::Tr("save.name.campaign"),
                        gGame->m_campaignScenariosWon
                    );
                    gGame->SaveGame(saveBuf, true);
                    sprintf(gText, localization::Tr("campaign.next.confirm"), saveBuf);
                    NormalDialog(gText, NORMAL_DIALOG_TYPE_YES_NO);
                    if (gWindowManager->m_dialogResult == NORMAL_DIALOG_CONFIRM)
                        goto playScenario;
                }
            }
        }
        if (gRemoteOn)
            leave = true;
    }
    ShutDown(NULL);
    return 0;
}

char toupper(char character) {
    return CyrillicToUpper(character);
}

b32 InterpretCommandLine(void) {
    i32 size;
    i32 i;
    b32 helpRequested = false;

    gDebugLevel = gConfig.originalCheatKeys ? ADVENTURE_DEBUG_KEYS_LEVEL_MIN : DEBUG_LEVEL_NONE;
    gShowIntro = 1;
    gColorMice = 0;
    gSpecialMouseMasks = 1;
    gScreenScroll = true;
    gLimitPlayer = 0;
    gBlackoutPlayer = true;
    strcpy(gMapName, "AES31000.map");
    strcpy(gFullMapName, localization::Tr("scenario.claw.name"));
    strcpy(gMapDescription, localization::Tr("scenario.claw.description"));

    size = strlen(gCommandLine);
    for (i = 0; i < size; i++) {
        if (gCommandLine[i] == '/' && i + 1 < size) {
            switch (toupper(gCommandLine[i + 1])) {
                case 'I':
                    if (i + 2 < size)
                        gShowIntro = gCommandLine[i + 2] - '0';
                    break;
                case 'C':
                    if (i + 2 < size)
                        gColorMice = gCommandLine[i + 2] - '0';
                    break;
                case 'S':
                    if (i + 2 < size)
                        gNoSound = 1 - (gCommandLine[i + 2] - '0');
                    break;
                case 'B':
                    if (i + 2 < size)
                        gSpecialMouseMasks = gCommandLine[i + 2] - '0';
                    break;
            }
        }
    }

    gNoSound = 0;
    sprintf(gAggPathName, "%s%s", gDataPath, "heroes.agg");
    gDefaultAggregateName = gAggPathName;
    gFrameStep = 6;
    for (i = 0; i < GAME_PLAYER_COUNT; i++)
        gHumanPlayer[i] = i < gNumHumanPlayers;
    if (gNumHumanPlayers == 1)
        gBlackoutPlayer = false;
    helpRequested = false;
    return true;
}

i16 InitMenuHandler(tag_message& message) {
    b32 handled = false;
    i32 helpIndex;

    PollSound();
    if (message.modifiers & MESSAGE_MODIFIER_RIGHT_BUTTON) {
        if (IS_WIDGET_SELECTION_NOTIFICATION(message.command)) {
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
            if (helpIndex >= MAIN_MENU_HELP_FIRST)
                NormalDialog(gInitMenuHelp[helpIndex], NORMAL_DIALOG_TYPE_QUICK_VIEW);
        }
    } else if (message.type == MESSAGE_WIDGET) {
        switch (message.command) {
            case WIDGET_NOTIFY_DESELECT:
                if (message.id > 0 && message.id <= MAIN_MENU_LAST)
                    handled = true;
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

i16 NullHandler(tag_message& message) {
    return MESSAGE_DISPATCH_CONSUME;
}

i16 RecruitHeroHandler(tag_message& message) {
    const i16 viewButton1Value = RECRUIT_HERO_PORTRAIT_FIRST;
    const i16 viewButton2Value = RECRUIT_HERO_PORTRAIT_SECOND;
    const i16 recruitButton1 = RECRUIT_HERO_SELECT_FIRST;
    const i16 recruitButton2 = RECRUIT_HERO_SELECT_SECOND;
    b32 shouldClose = false;
    i32 heroSlot;

    if (message.type == MESSAGE_WIDGET) {
        switch (message.command) {
            case WIDGET_NOTIFY_SELECT:
                switch (message.id) {
                    case viewButton1Value:
                    case viewButton2Value:
                        heroSlot = message.id - viewButton1Value;
                        gTownManager->m_recruitHeroes[heroSlot]->HeroView(false);
                        gTownManager->RedrawTownScreen();
                        gTownManager->m_heroWindow0->DrawWindow();
                        gTownManager->m_heroWindow1->DrawWindow();
                        gWindowManager->FadeScreen(WINDOW_FADE_IN, WINDOW_FADE_SHORT, NULL);
                        break;
                    default:
                        break;
                }
                break;
            case WIDGET_NOTIFY_DESELECT:
                switch (message.id) {
                    case DIALOG_BUTTON_1:
                        gTownManager->m_recruitState = RECRUIT_HERO_NONE;
                        shouldClose = true;
                        break;
                    case recruitButton1:
                    case recruitButton2:
                        gTownManager->m_recruitState = message.id - recruitButton1;
                        gWindowManager->m_dialogResult = message.id;
                        shouldClose = true;
                        break;
                }
                break;
            default:
                break;
        }
    }
    if (shouldClose == true) {
        message.command = (message.id = WIDGET_COMMAND_DIALOG_SELECT);
        return MESSAGE_DISPATCH_FORWARD;
    }
    return MESSAGE_DISPATCH_CONSUME;
}

char* GetBuildingName(
    i32 race,
    i16 building
) {
    if (building < BUILDING_SLOT_DWELLING_FIRST)
        return gNeutralBuildingNames[building];
    else
        return gDwellingNames
            [race * BUILDING_SLOT_DWELLING_COUNT
             + (building - BUILDING_SLOT_DWELLING_FIRST)];
}

void GetBuildingCost(
    i32 race,
    i16 building,
    i32* const destination,
    i32 mageLevel
) {
    if (building < BUILDING_SLOT_DWELLING_FIRST) {
        if (building == BUILDING_SLOT_MAGE_GUILD)
            memcpy(
                destination,
                gMageBuildingCosts[mageLevel],
                RESOURCE_COUNT * sizeof(i32)
            );
        else
            memcpy(
                destination,
                gNeutralBuildingCosts[building],
                RESOURCE_COUNT * sizeof(i32)
            );
    } else {
        memcpy(
            destination,
            gDwellingCosts
                [race * BUILDING_SLOT_DWELLING_COUNT
                 + (building - BUILDING_SLOT_DWELLING_FIRST)],
            RESOURCE_COUNT * sizeof(i32)
        );
    }
}

char* GetMonsterSingularName(i32 monster) {
    return gArmyNames[monster];
}

char* GetMonsterName(i32 monster) {
    return gArmyNamesPlural[monster];
}

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

b8 CanBuild(town* townPointer, i16 building) {
    mapCell* cell;
    u16 required;
    if (BitTest(gGame->m_townBuiltToday, townPointer->m_id))
        return false;
    if (building != BUILDING_SLOT_CASTLE
        && !(townPointer->m_buildings & (1 << BUILDING_SLOT_CASTLE)))
        return false;
    if (building == BUILDING_SLOT_SHIPYARD) {
        cell = gAdvManager->GetCell(townPointer->m_x - 1, townPointer->m_y + 1);
        if (cell->m_tileIndex < MAP_CELL_TILES_PER_TERRAIN)
            return true;
        else
            return false;
    }
    if (building == BUILDING_SLOT_MAGE_GUILD
        && townPointer->m_buildState >= TOWN_MAGE_GUILD_COST_LEVEL_LAST)
        return false;
    if (building == BUILDING_SLOT_TENT)
        return false;
    if (building < BUILDING_SLOT_DWELLING_FIRST)
        return true;
    required = gDwellingRequirements
        [townPointer->m_type * BUILDING_SLOT_DWELLING_COUNT
         + (building - BUILDING_SLOT_DWELLING_FIRST)];
    if ((required & townPointer->m_buildings) == required)
        return true;
    return false;
}

b8 CanBuy(town* townPointer, i16 building) {
    i32 cost[RESOURCE_COUNT];
    playerData* player;
    i32 resourceIndex;
    GetBuildingCost(
        townPointer->m_type,
        building,
        cost,
        (townPointer->m_buildings & (1 << BUILDING_SLOT_MAGE_GUILD))
            ? (townPointer->m_buildState < TOWN_MAGE_GUILD_COST_LEVEL_LAST
                   ? townPointer->m_buildState + 1
                   : TOWN_MAGE_GUILD_COST_LEVEL_LAST)
            : TOWN_MAGE_GUILD_COST_LEVEL_FIRST
    );
    player = &gGame->m_players[gCurPlayer];
    for (resourceIndex = RESOURCE_FIRST; resourceIndex < RESOURCE_COUNT; ++resourceIndex) {
        if (player->m_resources[resourceIndex] < cost[resourceIndex])
            return false;
    }
    return true;
}

i32 GetBuildingBaseResourceValue(
    i32 race,
    i32 building,
    i32 level
) {
    if (building < BUILDING_SLOT_DWELLING_FIRST) {
        if (building == BUILDING_SLOT_MAGE_GUILD)
            return gMageBaseResourceValues[level];
        else
            return gNeutralBaseResourceValues[building];
    } else {
        return gDwellingBaseResourceValues
            [race * BUILDING_SLOT_DWELLING_COUNT
             + (building - BUILDING_SLOT_DWELLING_FIRST)];
    }
}

void NormalDialog(
    char* text,
    i32 dialogType,
    i32 x,
    i32 y,
    i32 firstResourceType,
    i32 firstResourceValue,
    i32 secondResourceType,
    i32 secondResourceValue,
    i32 showOrText
) {
    i32 iconFrameIndex;
    i16 showMessageText;
    i32 nextId;
    i32 panelHeight;
    char* orWord;
    tag_message message;
    textWidget* labelWidget;
    i32 index;
    i32 resourceIconY;
    b32 addedHeight;
    i32 wrappedLines;
    i32 totalHeight;
    i32 imageCenterX;
    i32 imageWidth;
    i32 imageHeight;
    i32 tallestImage;
    font* bigFont;
    i32 rows;
    char iconFile[NORMAL_DIALOG_FILENAME_LENGTH];
    i32 resourceAmounts[NORMAL_DIALOG_RESOURCE_COUNT];
    i32 width;
    char* labelTexts[NORMAL_DIALOG_RESOURCE_COUNT];
    i32 resourceKind[NORMAL_DIALOG_RESOURCE_COUNT];
    iconWidget* imageWidget;
    i32 frameHeight;

    imageCenterX = 0;
    resourceIconY = 0;
    iconFrameIndex = 0;
    nextId = NORMAL_DIALOG_TEXT_WIDGET_FIRST_ID;
    imageWidth = 0;
    addedHeight = false;
    showMessageText = 1;
    resourceKind[0] = firstResourceType;
    resourceAmounts[0] = firstResourceValue;
    resourceKind[1] = secondResourceType;
    resourceAmounts[1] = secondResourceValue;

    bigFont = gResourceManager->GetFont("bigfont.fnt");
    wrappedLines = bigFont->LineLength(text, NORMAL_DIALOG_TEXT_LINE_WIDTH);
    gResourceManager->Dispose(bigFont);
    totalHeight = wrappedLines * NORMAL_DIALOG_TEXT_LINE_HEIGHT;
    if (dialogType != NORMAL_DIALOG_TYPE_QUICK_VIEW)
        totalHeight += NORMAL_DIALOG_BUTTON_AREA_HEIGHT;

    tallestImage = 0;
    for (index = 0; index < NORMAL_DIALOG_RESOURCE_COUNT; index++) {
        switch (resourceKind[index]) {
            case NORMAL_DIALOG_ARTIFACT:
                imageHeight = 76;
                break;
            case NORMAL_DIALOG_LUCK_BONUS:
                imageHeight = 28;
                break;
            case NORMAL_DIALOG_LUCK_PENALTY:
                imageHeight = 57;
                break;
            case NORMAL_DIALOG_MORALE_BONUS:
                imageHeight = 62;
                break;
            case NORMAL_DIALOG_MORALE_PENALTY:
                imageHeight = 59;
                break;
            case NORMAL_DIALOG_EXPERIENCE:
                imageHeight = 76;
                break;
            case NORMAL_DIALOG_CREST:
                imageHeight = 55;
                break;
            case NORMAL_DIALOG_HERO:
                imageHeight = 111;
                break;
            case NORMAL_DIALOG_RESOURCE_GOLD:
                imageHeight = 26;
                break;
            case NORMAL_DIALOG_RESOURCE_WOOD:
            case NORMAL_DIALOG_RESOURCE_MERCURY:
            case NORMAL_DIALOG_RESOURCE_ORE:
            case NORMAL_DIALOG_RESOURCE_SULFUR:
            case NORMAL_DIALOG_RESOURCE_CRYSTAL:
            case NORMAL_DIALOG_RESOURCE_GEMS:
                imageHeight = 44;
                break;
            case NORMAL_DIALOG_SPELL:
                imageHeight = 52;
                break;
            default:
                imageHeight = 0;
                break;
        }
        if (imageHeight > tallestImage)
            tallestImage = imageHeight;
    }

    if (tallestImage > 0)
        totalHeight += tallestImage + 12;
    rows = (totalHeight - 12) / NORMAL_DIALOG_WINDOW_ROW_HEIGHT;
    if (rows > NORMAL_DIALOG_MAX_ROWS)
        rows = NORMAL_DIALOG_MAX_ROWS;
    if (rows <= 0 && dialogType != NORMAL_DIALOG_TYPE_QUICK_VIEW)
        rows = 1;
    width = NORMAL_DIALOG_WINDOW_WIDTH;
    panelHeight = rows * NORMAL_DIALOG_WINDOW_ROW_HEIGHT + NORMAL_DIALOG_WINDOW_BASE_HEIGHT;

    if (x == NORMAL_DIALOG_AUTO_POSITION || width + x >= LOGICAL_SCREEN_WIDTH - 1) {
        if (gAdvManager->m_active == 1 && !gHeroWindShowing && !gOverviewShowing)
            x = NORMAL_DIALOG_ADVENTURE_X;
        else
            x = (LOGICAL_SCREEN_WIDTH - width) / 2;
    }
    if (y == NORMAL_DIALOG_AUTO_POSITION || panelHeight + y >= LOGICAL_SCREEN_HEIGHT - 1) {
        y = (LOGICAL_SCREEN_HEIGHT - panelHeight) / 2;
        if (y > NORMAL_DIALOG_MAX_TOP)
            y = NORMAL_DIALOG_MAX_TOP;
    }

    sprintf(iconFile, "evntwin%d.bin", rows);
    gNormalDialogWindow = new heroWindow(x, y, iconFile);
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

    for (index = 0; index < NORMAL_DIALOG_RESOURCE_COUNT; index++) {
        imageWidget = NULL;
        labelWidget = NULL;
        if (resourceKind[index] == NORMAL_DIALOG_NO_RESOURCE)
            break;

        labelTexts[index] = static_cast<char*>(malloc(NORMAL_DIALOG_TEXT_LENGTH));
        if (resourceKind[index] <= NORMAL_DIALOG_RESOURCE_LAST) {
            if (resourceAmounts[index] > 0)
                sprintf(labelTexts[index], "%d", resourceAmounts[index]);
            else if (resourceAmounts[index] == 0)
                strcpy(labelTexts[index], "");
            else
                sprintf(
                    labelTexts[index],
                    localization::Tr("dialog.income.per_day"),
                    -resourceAmounts[index]
                );
            strcpy(iconFile, "resource.icn");
            iconFrameIndex = resourceKind[index];
        } else if (resourceKind[index] == NORMAL_DIALOG_SPELL) {
            sprintf(
                labelTexts[index],
                "%s",
                gSpellNames[resourceAmounts[index]]
            );
            strcpy(iconFile, "spells.icn");
            iconFrameIndex = resourceAmounts[index];
        } else if (resourceKind[index] == NORMAL_DIALOG_CREST) {
            sprintf(labelTexts[index], "%s", "");
            strcpy(iconFile, "brcrest.icn");
            iconFrameIndex = resourceAmounts[index];
        } else if (resourceKind[index] == NORMAL_DIALOG_HERO) {
            sprintf(labelTexts[index], "%s", "");
            sprintf(iconFile, "surrendr.icn");
            iconFrameIndex = 4;
        } else if (resourceKind[index] == NORMAL_DIALOG_EXPERIENCE
                   || resourceKind[index] == NORMAL_DIALOG_MORALE_BONUS
                   || resourceKind[index] == NORMAL_DIALOG_MORALE_PENALTY
                   || resourceKind[index] == NORMAL_DIALOG_LUCK_BONUS
                   || resourceKind[index] == NORMAL_DIALOG_LUCK_PENALTY) {
            strcpy(labelTexts[index], "");
            strcpy(iconFile, "expmrl.icn");
            iconFrameIndex = resourceKind[index] - NORMAL_DIALOG_EXPMRL_FIRST;
            if (resourceKind[index] == NORMAL_DIALOG_EXPERIENCE
                && resourceAmounts[index] != NORMAL_DIALOG_NO_VALUE)
                sprintf(labelTexts[index], "%d", resourceAmounts[index]);
        } else {
            strcpy(labelTexts[index], "");
            strcpy(iconFile, "resource.icn");
            iconFrameIndex = resourceKind[index];
        }

        switch (resourceKind[index]) {
            case NORMAL_DIALOG_ARTIFACT:
                imageWidth = 76;
                imageHeight = 76;
                break;
            case NORMAL_DIALOG_LUCK_BONUS:
                imageWidth = 64;
                imageHeight = 28;
                break;
            case NORMAL_DIALOG_LUCK_PENALTY:
                imageWidth = 64;
                imageHeight = 57;
                break;
            case NORMAL_DIALOG_MORALE_BONUS:
                imageWidth = 64;
                imageHeight = 62;
                break;
            case NORMAL_DIALOG_MORALE_PENALTY:
                imageWidth = 64;
                imageHeight = 59;
                break;
            case NORMAL_DIALOG_EXPERIENCE:
                imageWidth = 64;
                imageHeight = 64;
                break;
            case NORMAL_DIALOG_CREST:
                imageWidth = 50;
                imageHeight = 55;
                break;
            case NORMAL_DIALOG_HERO:
                imageWidth = 111;
                imageHeight = 105;
                break;
            case NORMAL_DIALOG_RESOURCE_GOLD:
                imageWidth = 76;
                imageHeight = 26;
                break;
            case NORMAL_DIALOG_RESOURCE_WOOD:
            case NORMAL_DIALOG_RESOURCE_MERCURY:
            case NORMAL_DIALOG_RESOURCE_ORE:
            case NORMAL_DIALOG_RESOURCE_SULFUR:
            case NORMAL_DIALOG_RESOURCE_CRYSTAL:
            case NORMAL_DIALOG_RESOURCE_GEMS:
                imageWidth = 38;
                imageHeight = 32;
                break;
            case NORMAL_DIALOG_SPELL:
                imageWidth = 38;
                imageHeight = 40;
                break;
        }

        if (strlen(labelTexts[index]) > 0)
            imageHeight += NORMAL_DIALOG_RESOURCE_LABEL_HEIGHT;
        if (index == 0) {
            imageCenterX = resourceKind[1] == NORMAL_DIALOG_NO_RESOURCE ? width / 2 : width / 3;
        } else {
            imageCenterX = width * 2 / 3;
        }
        resourceIconY = panelHeight - imageHeight - NORMAL_DIALOG_RESOURCE_BOTTOM_INSET;
        if (dialogType != NORMAL_DIALOG_TYPE_QUICK_VIEW)
            resourceIconY -= NORMAL_DIALOG_BUTTON_AREA_HEIGHT;
        if (tallestImage > imageHeight)
            resourceIconY -= (tallestImage - imageHeight) / 2;

        imageWidget = new iconWidget(
            imageCenterX - imageWidth / 2,
            resourceIconY,
            imageWidth,
            imageHeight,
            iconFile,
            iconFrameIndex,
            ICON_DRAW_NORMAL,
            WIDGET_ID_NONE,
            ICON_WIDGET_DRAW,
            1
        );
        if (!imageWidget)
            MemError();
        gNormalDialogWindow->AddWidget(imageWidget, WINDOW_Z_ORDER_APPEND);
        if (resourceKind[index] == NORMAL_DIALOG_ARTIFACT) {
            imageWidget = new iconWidget(
                imageCenterX - imageWidth / 2 + 6,
                resourceIconY + 6,
                76,
                76,
                "artifact.icn",
                resourceAmounts[index],
                ICON_DRAW_NORMAL,
                WIDGET_ID_NONE,
                ICON_WIDGET_DRAW,
                1
            );
            if (!imageWidget)
                MemError();
            gNormalDialogWindow->AddWidget(imageWidget, WINDOW_Z_ORDER_APPEND);
        }
        if (resourceKind[index] == NORMAL_DIALOG_CREST) {
            imageWidget = new iconWidget(
                imageCenterX - imageWidth / 2 - 4,
                resourceIconY - 4,
                58,
                55,
                "brcrest.icn",
                4,
                ICON_DRAW_NORMAL,
                WIDGET_ID_NONE,
                ICON_WIDGET_DRAW,
                1
            );
            if (!imageWidget)
                MemError();
            gNormalDialogWindow->AddWidget(imageWidget, WINDOW_Z_ORDER_APPEND);
        }
        if (resourceKind[index] == NORMAL_DIALOG_HERO) {
            sprintf(iconFile, "port%04d.icn", resourceAmounts[index]);
            imageWidget = new iconWidget(
                imageCenterX - imageWidth / 2 + 5,
                resourceIconY + 5,
                101,
                95,
                iconFile,
                0,
                ICON_DRAW_NORMAL,
                WIDGET_ID_NONE,
                ICON_WIDGET_DRAW,
                1
            );
            if (!imageWidget)
                MemError();
            gNormalDialogWindow->AddWidget(imageWidget, WINDOW_Z_ORDER_APPEND);
        }
        // Wide enough for the longer Russian resource captions.
        labelWidget = new textWidget(
            imageCenterX - 60,
            resourceIconY + imageHeight - 10,
            120,
            12,
            labelTexts[index],
            "smalfont.fnt",
            1,
            nextId++,
            WIDGET_KIND_TEXT
        );
        if (!labelWidget)
            MemError();
        gNormalDialogWindow->AddWidget(labelWidget, WINDOW_Z_ORDER_APPEND);
    }

    SET_WIDGET_MESSAGE(message, WIDGET_COMMAND_SET_TEXT, NORMAL_DIALOG_TEXT_WIDGET_ID);
    message.text = text;
    gNormalDialogWindow->BroadcastMessage(message);

    if (showOrText == NORMAL_DIALOG_SHOW_OR_TEXT) {
        orWord = static_cast<char*>(malloc(strlen(localization::Tr("dialog.choice.or")) + 1));
        strcpy(orWord, localization::Tr("dialog.choice.or"));
        labelWidget = new textWidget(
            width / 2 - 17,
            resourceIconY + 30,
            40,
            12,
            orWord,
            "smalfont.fnt",
            1,
            nextId++,
            WIDGET_KIND_TEXT
        );
        if (!labelWidget)
            MemError();
        gNormalDialogWindow->AddWidget(labelWidget, WINDOW_Z_ORDER_APPEND);
    }

    if (gAdvManager->m_active == 1)
        gMouseManager->SetPointer(ADVENTURE_POINTER_DEFAULT);
    else if (gCombatManager->m_active == 1)
        gMouseManager->SetPointer(COMBAT_POINTER_DEFAULT);

    if (dialogType == NORMAL_DIALOG_TYPE_WAIT_CANCEL || dialogType == NORMAL_DIALOG_TYPE_WAIT_OK) {
        gWindowManager->DoDialog(gNormalDialogWindow, WaitHandler, false);
    } else if (dialogType == NORMAL_DIALOG_TYPE_QUICK_VIEW) {
        gMouseManager->ReallyHidePointer();
        gWindowManager->AddWindow(gNormalDialogWindow, WINDOW_Z_ORDER_APPEND, 1);
        QuickViewWait();
        gWindowManager->RemoveWindow(gNormalDialogWindow);
        gMouseManager->ReallyShowPointer();
    } else {
        gWindowManager->DoDialog(gNormalDialogWindow, EventWindowHandler, false);
    }
    delete gNormalDialogWindow;
}

void UpdateNormalDialog(char* text) {
    tag_message message;
    {
        i16 show = 1;
        SET_WIDGET_MESSAGE(message, WIDGET_COMMAND_SET_TEXT, NORMAL_DIALOG_TEXT_WIDGET_ID);
        message.text = text;
        gNormalDialogWindow->BroadcastMessage(message);
        gNormalDialogWindow->DrawWindow(0, 0, NORMAL_DIALOG_FOREGROUND_WIDGET_LIMIT);
        gNormalDialogWindow
            ->DrawWindow(1, WINDOW_ALL_WIDGETS_LOW, NORMAL_DIALOG_BACKGROUND_WIDGET_LAST_ID);
    }
}

i16 WaitHandler(tag_message& message) {
    b8 result = false;
    gFunctionComplete = true;
    PollSound();
    if (!MusicPlaying())
        PlayMusic(TERRAIN_MUSIC_TRACK(gAdvManager->m_currentTerrain));
    if (message.type == MESSAGE_WIDGET) {
        switch (message.command) {
            case WIDGET_NOTIFY_DESELECT:
                switch (message.id) {
                    case DIALOG_BUTTON_0:
                    case DIALOG_BUTTON_1:
                    case DIALOG_BUTTON_2:
                        gFunctionComplete = false;
                        result = true;
                        break;
                }
        }
    }
    if (!result) {
        switch (gWaitType) {
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
        gWindowManager->m_dialogResult = DIALOG_BUTTON_1;
        message.type = MESSAGE_WIDGET;
        message.command = (message.id = WIDGET_COMMAND_DIALOG_SELECT);
        return MESSAGE_DISPATCH_FORWARD;
    }
    return MESSAGE_DISPATCH_CONSUME;
}

i16 EventWindowHandler(tag_message& message) {
    if (!MusicPlaying())
        PlayMusic(TERRAIN_MUSIC_TRACK(gAdvManager->m_currentTerrain));
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

i16 TrueFalseDialogHandler(tag_message& message) {
    return EventWindowHandler(message);
}

void PlayerDead(i32 player) {
    playerData* currentPlayer;
    i32 i;
    gRetreatWin = false;
    currentPlayer = &gGame->m_players[player];
    gGame->m_playerDead[player] = true;
    ++gGame->m_deadPlayerCount;
    for (i = 0; i < GAME_MINE_COUNT; ++i) {
        if (gGame->m_mineOwners[i] == player)
            gGame->ClaimMine(i, GAME_PLAYER_NONE);
    }
    for (i = currentPlayer->m_heroCount - 1; i >= 0; --i)
        gGame->GetHero(currentPlayer->m_heroIds[i])->Deallocate();
    for (i = 0; i < HERO_AVAILABLE_SLOT_COUNT; ++i) {
        if (gGame->m_availableHeroes[currentPlayer->m_availableHeroIds[i]]
            == HERO_AVAILABILITY_IN_TAVERN)
            gGame->m_availableHeroes[currentPlayer->m_availableHeroIds[i]] =
                HERO_AVAILABILITY_UNAVAILABLE;
    }
    if (gRemoteOn && gHumanPlayer[player])
        HandleRemoteDeadPlayerExit(player);
}

char* gCombatGroundNames[TERRAIN_COUNT] = {
    "boat.xtl",
    "grass.xtl",
    "snow.xtl",
    "swamp.xtl",
    "lava.xtl",
    "desert.xtl",
    "dgrass.xtl",
};
char* gCombatObstacleNames[TERRAIN_COUNT] = {
    "boat.obj",
    "grass.obj",
    "snow.obj",
    "swamp.obj",
    "lava.obj",
    "desert.obj",
    "dgrass.obj",
};
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
char* gCombatFxNames[COMBAT_EFFECT_COUNT] = {
    "redfire.icn", "elecfire.icn", "magic04.icn", "magic01.icn", "magic05.icn",  "magic02.icn",
    "magic02.icn", "magic06.icn",  "magic07.icn", "magic01.icn", "magic06.icn",  "magic08.icn",
    "magic07.icn", "magic01.icn",  "magic01.icn", "magic02.icn", "reddeath.icn", "magic03.icn",
    "magic03.icn", "magic06.icn",  "magic01.icn", "magic01.icn", "rainbluk.icn", "cloudluk.icn",
    "moraleg.icn", "moraleb.icn",
};
i16 gSpellAIValue[SPELL_COUNT] = {
    500,  350,  300, 400, 550, 900, 400, 500, 300, 350, 250, 0, 100,  150, 1000,
    2000, 1700, 700, 700, 0,   0,   0,   0,   0,   0,   0,   0, 1200, 0,
};
i8 gSpellAIFlags[SPELL_COUNT] = {
    3, 3, 2, 2, 3, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 3, 3, 3, 2, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
};
i8 gMageGuildSpellPool[4][8] = {
    {SPELL_PROTECTION, SPELL_DISPEL_MAGIC, SPELL_SLOW, SPELL_BLESS, SPELL_CURSE, SPELL_VIEW_RESOURCES,
     SPELL_VIEW_MINES, SPELL_BLESS},
    {SPELL_LIGHTNING_BOLT, SPELL_HASTE, SPELL_CURE, SPELL_BLIND, SPELL_TURN_UNDEAD, SPELL_VIEW_ARTIFACTS,
     SPELL_SUMMON_BOAT, SPELL_ANTI_MAGIC},
    {SPELL_FIREBALL, SPELL_PARALYZE, SPELL_BERZERKER, SPELL_STORM, SPELL_VIEW_TOWNS, SPELL_IDENTIFY_HERO,
     SPELL_VIEW_HEROES, SPELL_TELEPORT},
    {SPELL_DIMENSION_DOOR, SPELL_RESURRECT, SPELL_ARMAGEDDON, SPELL_METEOR_SHOWER, SPELL_TOWN_GATE,
     SPELL_VIEW_ALL, SPELL_TOWN_GATE, SPELL_DIMENSION_DOOR},
};
i8 gCombatAdjacency[45][COMBAT_DIRECTION_ADJACENT_COUNT] = {
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
i16 gHorseFrameFlip[16] = {45, 46, 47, 48, 49, 50, 51, 52, 53, 179, 178, 177, 54, 175, 174, 55};
i16 gBoatFrameFlip[16] = {0, 0, 9, 9, 18, 18, 27, 27, 36, 36, 155, 155, 146, 146, 137, 137};
i16 gRadarOwnerColor[5] = {79, 105, 200, 129, 10};
i16 gRadarTerrainColor[24] = {
    82,  99, 7,   180, 26,  123, 55, 0,  16, 48, 98, 160,
    126, 74, 110, 179, 100, 218, 12, 12, 12, 12, 12, 12,
};
char* gTownObjectNames[20] = {
    "magegld", "thievesg", "tavern", "dock", "well", "farm", "frst", "plns", "mtn", "tent",
    "cast",    "_d0",      "_d1",    "_d2",  "_d3",  "_d4",  "_d5",  "_e0",  "_e1", "_e2",
};
i8 gDwellingType[TOWN_TYPE_COUNT][6] = {
    {CREATURE_PEASANT,
     CREATURE_ARCHER,
     CREATURE_PIKEMAN,
     CREATURE_SWORDSMAN,
     CREATURE_CAVALRY,
     CREATURE_PALADIN},
    {CREATURE_SPRITE,
     CREATURE_DWARF,
     CREATURE_ELF,
     CREATURE_DRUID,
     CREATURE_UNICORN,
     CREATURE_PHOENIX},
    {CREATURE_GOBLIN, CREATURE_ORC, CREATURE_WOLF, CREATURE_OGRE, CREATURE_TROLL, CREATURE_CYCLOPS},
    {CREATURE_CENTAUR,
     CREATURE_GARGOYLE,
     CREATURE_GRIFFIN,
     CREATURE_MINOTAUR,
     CREATURE_HYDRA,
     CREATURE_DRAGON},
};
i32 gMageBuildingCosts[4][7] = {
    {5, 0, 5, 0, 0, 0, 2000},
    {5, 4, 5, 4, 4, 4, 1000},
    {5, 6, 5, 6, 6, 6, 1000},
    {5, 10, 5, 10, 10, 10, 1000},
};
i32 gNeutralBuildingCosts[BUILDING_SLOT_NEUTRAL_COUNT][7] = {
    {5, 0, 5, 0, 0, 0, 2000},
    {5, 0, 0, 0, 0, 0, 750},
    {5, 0, 0, 0, 0, 0, 500},
    {20, 0, 0, 0, 0, 0, 2000},
    {0, 0, 0, 0, 0, 0, 500},
    {5, 0, 5, 0, 0, 0, 2000},
    {20, 0, 20, 0, 0, 0, 5000},
};
i32 gMageBaseResourceValues[4] = {4000, 6500, 8500, 10500};
i32 gNeutralBaseResourceValues[BUILDING_SLOT_NEUTRAL_COUNT] =
    {5000, 1500, 500, 2000, 3000, 0, 12000};
i32 gDwellingBaseResourceValues[24] = {
    858,  2225, 2816, 7385, 13754, 29785, 1684, 2256, 3736, 7213, 15181, 27684,
    1802, 2615, 3414, 6967, 12212, 38141, 1956, 2607, 3869, 7510, 16002, 111967,
};
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
i8 gCastleResources[4] = {0, 2, -1, -1};

void HandleRemoteDeadPlayerExit(i32 position) {
    if (position == gThisGamePos) {
        if (!gGame->TransmitSaveGame(REMOTE_BROADCAST_PLAYER, 1))
            ShutDown(NULL);
        RemoteCleanup();
    } else if (gNumHumanPlayers == REMOTE_PLAYER_COUNT) {
        RemotePlayerExit playerExit;
        RecordWriter exitRecord;
        gNumHumanPlayers--;
        playerExit.position = position;
        playerExit.hadControl = 0;
        // The original left the third byte as gText held it.
        playerExit.nextPlayer = 0;
        WriteRemotePlayerExit(exitRecord, playerExit);
        TransmitRemoteRecord(
            exitRecord,
            REMOTE_BROADCAST_PLAYER,
            REMOTE_COMMAND_PLAYER_EXIT,
            false,
            false,
            REMOTE_MESSAGE_RELIABLE
        );
        RemoteCleanup();
        gHumanPlayer[position] = false;
    }
}

void HandleRemoteSuddenExit(void) {
    i32 next;
    RemotePlayerExit playerExit;
    RecordWriter exitRecord;
    if (!gGameInitialized)
        return;
    playerExit.position = gThisGamePos;
    // The original left the third byte as gText held it when the player
    // leaving had no control.
    playerExit.nextPlayer = 0;
    if (gThisNetHumanPlayer[gCurPlayer]
        || (!gHumanPlayer[gCurPlayer] && gThisGamePos == gHostGamePos)) {
        playerExit.hadControl = 1;
        next = gCurPlayer;
        next = (next + 1) % gGame->m_playerCount;
        while (!gHumanPlayer[next])
            next = (next + 1) % gGame->m_playerCount;
        playerExit.nextPlayer = next;
    } else {
        playerExit.hadControl = 0;
    }
    WriteRemotePlayerExit(exitRecord, playerExit);
    TransmitRemoteRecord(
        exitRecord,
        REMOTE_BROADCAST_PLAYER,
        REMOTE_COMMAND_PLAYER_EXIT,
        false,
        false,
        REMOTE_MESSAGE_RELIABLE
    );
}

void ReceiveRemotePlayerExit(i8 position, i8 hadControl, b8 eliminated, b8 timedOut) {
    if (position == gThisGamePos) {
        sprintf(gText, localization::Tr("network.player.eliminated"));
        NormalDialog(gText, NORMAL_DIALOG_TYPE_OK);
        RemoteCleanup();
        gGameOver = true;
        gEndSequence = GAME_END_LOST;
        return;
    }
    if (gNumHumanPlayers <= REMOTE_PLAYER_COUNT) {
        gGame->SaveGame(localization::Tr("save.name.player_exit"), true);
        if (eliminated) {
            sprintf(
                gText,
                localization::Tr("player.vanquished"),
                gColorNames[gGame->m_players[position].Color()]
            );
            gText[0] = CyrillicToUpper(gText[0]);
            NormalDialog(
                gText,
                NORMAL_DIALOG_TYPE_OK,
                0x61,
                NORMAL_DIALOG_AUTO_POSITION,
                NORMAL_DIALOG_CREST,
                (gGame->m_players[position].Color())
            );
            goto dropPlayer;
        } else {
            if (timedOut)
                sprintf(
                    gText,
                    localization::Tr("network.player.timeout.computer"),
                    position + 1,
                    position + 1
                );
            else
                sprintf(
                    gText,
                    localization::Tr("network.player.exit.computer"),
                    position + 1,
                    position + 1
                );
            NormalDialog(gText, NORMAL_DIALOG_TYPE_YES_NO);
        }
        if (gWindowManager->m_dialogResult == NORMAL_DIALOG_CONFIRM) {
        dropPlayer:
            if (gNumHumanPlayers == REMOTE_PLAYER_COUNT) {
                gNumHumanPlayers--;
                RemoteCleanup();
                gHumanPlayer[position] = false;
            }
        } else {
            RemoteCleanup();
            ShutDown(NULL);
        }
    }
}

void CheckEndGame(b32 forceWin) {
    town* objectiveTown;
    i8 artifactOwner;
    hero* bearer;
    char message[200];
    b8 won;
    i32 index;
    i32 numLiving;
    b8 defeated;
    b8 defaultWin;
    i32 playerIndex;
    i32 lastSurvivor;
    i32 aliveHumans;
    playerData* curPlayer;
    i32 lastHuman;

    if (gInNewGameSetup)
        return;
    if (gGameOver)
        return;
    if (gInCheckEndGame)
        return;
    gInCheckEndGame = true;

    for (playerIndex = 0; playerIndex < gGame->m_playerCount; playerIndex++) {
        if (!gGame->m_playerDead[playerIndex]) {
            curPlayer = &gGame->m_players[playerIndex];
            if (!curPlayer->m_heroCount && !curPlayer->m_townCount) {
                PlayerDead(playerIndex);
                sprintf(
                    gText,
                    localization::Tr("player.vanquished"),
                    gColorNames[gGame->m_players[static_cast<i8>(playerIndex)].Color()]
                );
                gText[0] = CyrillicToUpper(gText[0]);
                NormalDialog(
                    gText,
                    NORMAL_DIALOG_TYPE_OK,
                    0x61,
                    NORMAL_DIALOG_AUTO_POSITION,
                    NORMAL_DIALOG_CREST,
                    (gGame->m_players[static_cast<i8>(playerIndex)].Color())
                );
            } else if (!curPlayer->m_townCount) {
                if (curPlayer->m_daysLeft == END_GAME_NO_GRACE_PERIOD) {
                    if (gThisNetHumanPlayer[playerIndex]) {
                        sprintf(
                            gText,
                            localization::Tr("endgame.last_town.lost"),
                            gColorNames[gGame->m_players[static_cast<i8>(playerIndex)].Color()]
                        );
                        gText[0] = CyrillicToUpper(gText[0]);
                        NormalDialog(
                            gText,
                            NORMAL_DIALOG_TYPE_OK,
                            NORMAL_DIALOG_AUTO_POSITION,
                            NORMAL_DIALOG_AUTO_POSITION,
                            NORMAL_DIALOG_CREST,
                            (gGame->m_players[static_cast<i8>(playerIndex)].Color())
                        );
                    }
                    curPlayer->m_daysLeft = END_GAME_GRACE_DAYS;
                } else if (!curPlayer->m_daysLeft) {
                    PlayerDead(playerIndex);
                    if (gThisNetHumanPlayer[playerIndex]) {
                        sprintf(
                            gText,
                            localization::Tr("endgame.heroes.abandon_you"),
                            gColorNames[gGame->m_players[static_cast<i8>(playerIndex)].Color()]
                        );
                        gText[0] = CyrillicToUpper(gText[0]);
                    } else {
                        sprintf(
                            gText,
                            localization::Tr("endgame.heroes.abandon_player"),
                            gColorNames[gGame->m_players[static_cast<i8>(playerIndex)].Color()]
                        );
                        gText[0] = CyrillicToUpper(gText[0]);
                    }
                    NormalDialog(
                        gText,
                        NORMAL_DIALOG_TYPE_OK,
                        0x61,
                        NORMAL_DIALOG_AUTO_POSITION,
                        NORMAL_DIALOG_CREST,
                        (gGame->m_players[static_cast<i8>(playerIndex)].Color())
                    );
                }
            } else {
                curPlayer->m_daysLeft = END_GAME_NO_GRACE_PERIOD;
            }
        }
    }

    numLiving = 0;
    lastSurvivor = 0;
    aliveHumans = 0;
    lastHuman = 0;
    for (playerIndex = 0; playerIndex < gGame->m_playerCount; playerIndex++) {
        if (!gGame->m_playerDead[playerIndex]) {
            numLiving++;
            lastSurvivor = playerIndex;
            if (gHumanPlayer[playerIndex]) {
                aliveHumans++;
                lastHuman = playerIndex;
            }
        }
    }

    won = false;
    defeated = false;
    defaultWin = true;
    if (gGame->m_campaignType > 0) {
        switch (gGame->m_campaignScenario) {
            case CAMPAIGN_SCENARIO_1:
            case CAMPAIGN_SCENARIO_5:
            case CAMPAIGN_SCENARIO_6:
            case CAMPAIGN_SCENARIO_7:
            case CAMPAIGN_SCENARIO_8:
                defaultWin = false;
                objectiveTown = gGame->GetTown(gGame->GetTownId(
                    gCampaignScenarios[gGame->m_campaignScenario].victoryTownX,
                    gCampaignScenarios[gGame->m_campaignScenario].victoryTownY
                ));
                if (!objectiveTown->m_owner)
                    won = true;
                if (gGame->m_campaignScenario == CAMPAIGN_SCENARIO_1
                    && objectiveTown->m_owner > 0) {
                    defeated = true;
                    strcpy(message, localization::Tr("endgame.enemy.captured_town"));
                }
                break;
            case CAMPAIGN_SCENARIO_EYE_OF_GOROS:
                defaultWin = false;
                artifactOwner = GAME_PLAYER_NONE;
                for (playerIndex = 0; playerIndex < gGame->m_playerCount; playerIndex++) {
                    if (!gGame->m_playerDead[playerIndex]) {
                        for (index = 0; index < gGame->m_players[playerIndex].m_heroCount;
                             index++) {
                            bearer = gGame->GetPlayerHero(playerIndex, index);
                            if (bearer->HasArtifact(ARTIFACT_ULTIMATE_BOOK)
                                || bearer->HasArtifact(ARTIFACT_ULTIMATE_SWORD)
                                || bearer->HasArtifact(ARTIFACT_ULTIMATE_CLOAK)
                                || bearer->HasArtifact(ARTIFACT_ULTIMATE_WAND))
                                artifactOwner = playerIndex;
                        }
                    }
                }
                if (!artifactOwner)
                    won = true;
                if (artifactOwner > 0) {
                    defeated = true;
                    strcpy(message, localization::Tr("endgame.enemy.captured_artifact"));
                }
                break;
            case CAMPAIGN_SCENARIO_DRAGON_CITY:
                defaultWin = false;
                if (!gGame->m_mineOwners[0])
                    won = true;
                if (gGame->m_mineOwners[0] > 0) {
                    defeated = true;
                    strcpy(message, localization::Tr("endgame.enemy.captured_dragon_city"));
                }
        }
    }

    if (defeated) {
        gGameOver = true;
        gEndSequence = GAME_END_LOST;
    }
    if (won) {
        gGameOver = true;
        gEndSequence = GAME_END_WON;
    }
    if (numLiving == 1 || aliveHumans == 0
        || (aliveHumans == 1 && !gThisNetHumanPlayer[lastHuman])) {
        if (aliveHumans == 1 && gThisNetHumanPlayer[lastHuman]) {
            if (defaultWin) {
                gGameOver = true;
                gEndSequence = GAME_END_WON;
            }
        } else {
            gGameOver = true;
            gEndSequence = GAME_END_LOST;
        }
    }
    if (forceWin) {
        gGameOver = true;
        gEndSequence = GAME_END_WON;
    }
    if (gGameOver && gGame->m_campaignType > 0 && gEndSequence == GAME_END_WON
        && gGame->m_campaignScenario + 1 == CAMPAIGN_SCENARIO_COUNT)
        gEndSequence = GAME_END_CAMPAIGN_COMPLETE;
    gInCheckEndGame = false;
}

void QuickViewWait(void) {
    tag_message event;
    b32 done = false;
    while (!done) {
        PollSound();
        Process1WindowsMessage();
        event = gInputManager->GetEvent();
        done = event.type == MESSAGE_RIGHT_BUTTON_UP || event.type == MESSAGE_LEFT_BUTTON_DOWN
               || event.type == MESSAGE_LEFT_BUTTON_UP;
    }
}

void InitVars(void) {
    i32 i;
    gMaxMapExtra = MAP_EXTRA_FIRST_RECORD;
    gGameCommand = MAIN_MENU_NO_COMMAND;
    gPalette = NULL;
    gPhilAI->m_debugFont = NULL;
    gCombatSurrender = false;
    gGame->m_dialogAnimationCounter = 0;
    gInNewGameSetup = false;
    for (i = 0; i < MAP_CELL_GROUND_TILE_COUNT; i++)
        gGroundToTerrain[i] = (i / MAP_CELL_TILES_PER_TERRAIN);
    for (i = 0; i < FINDPATH_TERRAIN_COUNT; i++) {
        gTerrainCost[i][FINDPATH_STEP_STRAIGHT] = TerrainStepCost(i, FINDPATH_STEP_STRAIGHT);
        gTerrainCost[i][FINDPATH_STEP_DIAGONAL] = TerrainStepCost(i, FINDPATH_STEP_DIAGONAL);
    }
    strcpy(gNetBoxLine[NET_BOX_SLOT_PREVIOUS], "");
    strcpy(gNetBoxLine[NET_BOX_SLOT_LATEST], "");
    for (i = 0; i < MAP_EXTRA_RECORD_CAPACITY; i++)
        gMapExtraBlocks[i] = NULL;
    gDefaultMenu = KBLoadMenu("mnuDflt");
    gCombatMenu = KBLoadMenu("mnuCmbt");
    gAdventureMenu = KBLoadMenu("mnuAdv");
    gTownMenu = KBLoadMenu("mnuTown");
}

void game::ShowMoraleInfo(hero* heroPointer, i32 dialogType) {
    i32 newFaction;
    i32 i;
    i32 alignments;
    i32 length;
    char buffer[200];

    if (heroPointer->m_army.GetMorale(heroPointer, NULL) > 0)
        sprintf(buffer, gMoraleInfoText[MORALE_INFO_GOOD]);
    else if (heroPointer->m_army.GetMorale(heroPointer, NULL) == 0)
        sprintf(buffer, gMoraleInfoText[MORALE_INFO_NEUTRAL]);
    else
        sprintf(buffer, gMoraleInfoText[MORALE_INFO_BAD]);
    sprintf(gText, gMoraleInfoText[MORALE_INFO_HEADER], buffer);
    length = strlen(gText);
    if (!heroPointer->m_heroClass)
        strcat(gText, gMoraleInfoText[MORALE_INFO_KNIGHT]);
    alignments = heroPointer->m_army.IsHomogeneous(ARMY_GROUP_EMPTY_SLOT);
    if (alignments > ARMY_GROUP_ALIGNMENT_NO_BONUS_LAST) {
        newFaction = 0;
        for (i = 0; i < ARMY_GROUP_SLOT_COUNT; i++) {
            if (heroPointer->m_army.m_creatureTypes[i] != CREATURE_NONE)
                newFaction = CREATURE_FACTION(heroPointer->m_army.m_creatureTypes[i]);
        }
        sprintf(buffer, gMoraleInfoText[MORALE_INFO_ALL_TROOPS], gAlignmentNames[newFaction]);
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
    if (heroPointer->HasArtifact(ARTIFACT_MEDAL_OF_VALOR))
        strcat(gText, gMoraleInfoText[MORALE_INFO_MEDAL_OF_VALOR]);
    if (heroPointer->HasArtifact(ARTIFACT_MEDAL_OF_COURAGE))
        strcat(gText, gMoraleInfoText[MORALE_INFO_MEDAL_OF_COURAGE]);
    if (heroPointer->HasArtifact(ARTIFACT_MEDAL_OF_HONOR))
        strcat(gText, gMoraleInfoText[MORALE_INFO_MEDAL_OF_HONOR]);
    if (heroPointer->HasArtifact(ARTIFACT_MEDAL_OF_DISTINCTION))
        strcat(gText, gMoraleInfoText[MORALE_INFO_MEDAL_OF_DISTINCTION]);
    if (heroPointer->m_eventFlags & HERO_EVENT_BUOY)
        strcat(gText, gMoraleInfoText[MORALE_INFO_BUOY]);
    if (heroPointer->m_eventFlags & HERO_EVENT_OASIS)
        strcat(gText, gMoraleInfoText[MORALE_INFO_OASIS]);
    if (heroPointer->m_eventFlags & HERO_EVENT_STATUE)
        strcat(gText, gMoraleInfoText[MORALE_INFO_STATUE]);
    if (heroPointer->m_eventFlags & HERO_EVENT_GRAVEYARD)
        strcat(gText, gMoraleInfoText[MORALE_INFO_GRAVEYARD]);
    if (heroPointer->m_eventFlags & HERO_EVENT_SHIPWRECK)
        strcat(gText, gMoraleInfoText[MORALE_INFO_SHIPWRECK]);
    if (heroPointer->m_cowardice) {
        sprintf(buffer, gMoraleInfoText[MORALE_INFO_COWARDICE], heroPointer->m_cowardice);
        strcat(gText, buffer);
    }
    if (length == strlen(gText))
        strcat(gText, gMoraleInfoText[MORALE_INFO_NONE]);
    NormalDialog(gText, dialogType);
}

void game::ShowLuckInfo(hero* heroPointer, i32 dialogType) {
    i32 alignments;
    i32 size;
    char buffer[200];

    if (gGame->GetLuck(heroPointer, NULL) > 0)
        sprintf(buffer, gLuckInfoText[LUCK_INFO_GOOD]);
    else if (gGame->GetLuck(heroPointer, NULL) == 0)
        sprintf(buffer, gLuckInfoText[LUCK_INFO_NEUTRAL]);
    else
        sprintf(buffer, gLuckInfoText[LUCK_INFO_BAD]);
    sprintf(gText, gLuckInfoText[LUCK_INFO_HEADER], buffer);
    size = strlen(gText);
    if (heroPointer->HasArtifact(ARTIFACT_FIZBIN_OF_MISFORTUNE))
        strcat(gText, gLuckInfoText[LUCK_INFO_FIZBIN]);
    if (heroPointer->HasArtifact(ARTIFACT_LUCKY_RABBITS_FOOT))
        strcat(gText, gLuckInfoText[LUCK_INFO_RABBITS_FOOT]);
    if (heroPointer->HasArtifact(ARTIFACT_GOLDEN_HORSESHOE))
        strcat(gText, gLuckInfoText[LUCK_INFO_HORSESHOE]);
    if (heroPointer->HasArtifact(ARTIFACT_GAMBLERS_LUCKY_COIN))
        strcat(gText, gLuckInfoText[LUCK_INFO_LUCKY_COIN]);
    if (heroPointer->HasArtifact(ARTIFACT_FOUR_LEAF_CLOVER))
        strcat(gText, gLuckInfoText[LUCK_INFO_CLOVER]);
    if (heroPointer->m_eventFlags & HERO_EVENT_FAERIE_RING)
        strcat(gText, gLuckInfoText[LUCK_INFO_FAERIE_RING]);
    if (heroPointer->m_eventFlags & HERO_EVENT_FOUNTAIN)
        strcat(gText, gLuckInfoText[LUCK_INFO_FOUNTAIN]);
    if (size == strlen(gText))
        strcat(gText, gLuckInfoText[LUCK_INFO_NONE]);
    NormalDialog(gText, dialogType);
}

void ClearMapExtra(void) {
    i32 i;
    for (i = 0; i < MAP_EXTRA_RECORD_CAPACITY; i++) {
        if (gMapExtraBlocks[i]) {
            free(gMapExtraBlocks[i]);
            gMapExtraBlocks[i] = NULL;
        }
    }
    gMaxMapExtra = MAP_EXTRA_FIRST_RECORD;
}

i16 GetMonType(i32 score, i32 highScoreType) {
    i32 index;
    for (index = SCORE_MONSTER_COUNT - 1; index >= 0; index--) {
        if (highScoreType == HIGH_SCORE_TYPE_CAMPAIGN) {
            if (score <= gScoreCampaignMon[index][SCORE_MONSTER_THRESHOLD])
                return gScoreCampaignMon[index][SCORE_MONSTER_TYPE];
        } else {
            if (score >= gScoreMon[index][SCORE_MONSTER_THRESHOLD])
                return gScoreMon[index][SCORE_MONSTER_TYPE];
        }
    }
    return gScoreMon[0][SCORE_MONSTER_TYPE];
}

i32 AddScoreToHighScore(
    i32 score,
    i32 highScoreType,
    char*,
    char* scenarioName
) {
    HighScoreEntry entries[HIGH_SCORE_DISPLAY_ENTRY_COUNT];
    i32 entry;
    i32 shiftRank;
    RecordReader scoreData;
    RecordWriter newScores;
    char scoreFile[352];
    char enteredPlayerName[20];
    b8 noScoreFile;

    noScoreFile = false;
    if (highScoreType == HIGH_SCORE_TYPE_STANDARD)
        sprintf(scoreFile, "%sSTANDARD.HS", gDataPath);
    else
        sprintf(scoreFile, "%sCAMPAIGN.HS", gDataPath);
    if (!scoreData.LoadFile(scoreFile))
        noScoreFile = true;
    if (noScoreFile) {
        for (entry = 0; entry < HIGH_SCORE_DISPLAY_ENTRY_COUNT; entry++) {
            memset(&entries[entry], 0, sizeof(HighScoreEntry));
            entries[entry].score = HIGH_SCORE_EMPTY;
        }
    } else {
        for (entry = 0; entry < HIGH_SCORE_DISPLAY_ENTRY_COUNT; entry++)
            ReadHighScore(scoreData, entries[entry]);
    }

    gShowHighScore = true;
    gHighScoreType = highScoreType;
    gHighScoreRank = HIGH_SCORE_EMPTY;
    gScore = score;
    for (entry = 0; entry < HIGH_SCORE_DISPLAY_ENTRY_COUNT; entry++) {
        if ((score >= entries[entry].score && highScoreType == HIGH_SCORE_TYPE_STANDARD)
            || (score <= entries[entry].score && highScoreType == HIGH_SCORE_TYPE_CAMPAIGN)
            || entries[entry].score == HIGH_SCORE_EMPTY) {
            gHighScoreRank = entry;
            break;
        }
    }

    if (entry < HIGH_SCORE_DISPLAY_ENTRY_COUNT) {
        for (shiftRank = HIGH_SCORE_DISPLAY_ENTRY_COUNT - 2; shiftRank >= entry; shiftRank--)
            entries[shiftRank + 1] = entries[shiftRank];
        GetDataEntry(localization::Tr("score.name.prompt"), enteredPlayerName, 16, NULL);
        strcpy(entries[entry].playerName, enteredPlayerName);
        strcpy(entries[entry].scenarioName, scenarioName);
        entries[entry].score = score;
        for (entry = 0; entry < HIGH_SCORE_DISPLAY_ENTRY_COUNT; entry++)
            WriteHighScore(newScores, entries[entry]);
        if (!newScores.SaveFile(scoreFile))
            FileError(scoreFile);
    }
    return 0;
}

void BVResMsg(char* text, i32 resourceType, i32 quantity) {
    gBottomViewOverride = BOTTOM_VIEW_RESOURCE;
    gBottomViewOverrideEndTime = KBTickCount() + 5000;
    gBottomViewResource = resourceType;
    gBottomViewResourceQty = quantity;
    strcpy(gBottomViewText, text);
    gAdvManager->UpdBottomView(true, true, true);
}

void GOut(char* text) {
    if (gAdvManager->m_active == 1)
        AiPrint(text);
}

i8 NetPosToGamePos(i32 netPos) {
    if (netPos == NET_POSITION_HOST)
        return NET_GAME_POSITION_HOST;
    else if (netPos > NET_POSITION_HOST)
        return NET_GAME_POSITION_GUEST;
    return GAME_PLAYER_NONE;
}

b8 WaitForOtherPlayer(void) {
    b32 result = false;
    RemoteMessage* received;
    PollSound();
    received = GetRemoteData(true);
    if (received && received->type == REMOTE_MESSAGE_RELIABLE) {
        RecordReader payload = RemotePayloadReader(*received);
        RemoteSaveHeader saveHeader;
        switch (received->command) {
            case BOX_REMOTE_SETUP:
                ReadRemoteSetup(payload, gGamePosToNetPos);
                gThisGamePos = NetPosToGamePos(gThisNetPos);
                gHostGamePos = NetPosToGamePos(NET_POSITION_HOST);
                break;
            case BOX_REMOTE_SAVE:
                ReadRemoteSaveHeader(payload, saveHeader);
                result = gGame->ReceiveSaveGame(saveHeader.saveSize, received->sender);
                break;
        }
    }
    return result;
}

void PopNetBox(char* notice) {
    RemoteMessage* remoteData;
    b32 firstLineId;
    b8 savedShowIt;
    i32 heightValue;
    tag_message event;
    i8 pointerWasVisible;
    i32 inputLength;
    b8 enterPressed;
    tag_message messageData;
    i32 closeDelay;
    i32 sendOk;
    font* boxFont;
    b8 redrawText;
    i32 noticeStamp;
    heroWindow* netBox;
    i32 inputWidth;
    char inputText[80];
    b8 exitForIncomingData;
    b8 closeWindow;
    b8 redrawInput;
    i8 cursorVisible;
    i32 lineMax;

    if (!gRemoteOn)
        return;
    lineMax = 60;
    firstLineId = true;
    heightValue = 42;
    boxFont = gResourceManager->GetFont("bigfont.fnt");
    noticeStamp = 0;
    if (notice) {
        AddNetBoxLine(notice);
        noticeStamp = KBTickCount();
    }
    inputLength = 0;
    pointerWasVisible = gMouseManager->IsVis();
    savedShowIt = gShowIt;
    gShowIt = true;
    netBox = new heroWindow(0, 418, "netbox.bin");
    if (!netBox)
        MemError();
    SET_WIDGET_MESSAGE(messageData, WIDGET_COMMAND_SET_TEXT, NET_BOX_LINE_PREVIOUS);
    messageData.text = gNetBoxLine[NET_BOX_SLOT_PREVIOUS];
    netBox->BroadcastMessage(messageData);
    messageData.id = NET_BOX_LINE_LATEST;
    messageData.text = gNetBoxLine[NET_BOX_SLOT_LATEST];
    netBox->BroadcastMessage(messageData);
    gWindowManager->AddWindow(netBox, WINDOW_Z_ORDER_APPEND, 1);
    gMouseManager->ReallyHidePointer();
    exitForIncomingData = false;
    closeWindow = false;
    redrawInput = true;
    cursorVisible = 0;
    enterPressed = false;
    redrawText = true;
    strcpy(inputText, "");
    gInputManager->SetKeyCodeType(INPUT_KEY_CODE_ASCII);

    while (!closeWindow) {
        PollSound();
        remoteData = GetRemoteData(false);
        if (remoteData) {
            if (remoteData->type != REMOTE_MESSAGE_RELIABLE) {
                remoteData = GetRemoteData(true);
            } else {
                switch (remoteData->command) {
                    case REMOTE_COMMAND_CHAT:
                        remoteData = GetRemoteData(true);
                        AddNetBoxLine(remoteData->payload.data);
                        redrawText = true;
                        if (noticeStamp)
                            noticeStamp = KBTickCount();
                        break;
                    default:
                        AddNetBoxLine(localization::Tr("network.incoming.close"));
                        redrawText = true;
                        exitForIncomingData = true;
                        break;
                }
            }
        }

        Process1WindowsMessage();
        event = gInputManager->GetEvent();
        switch (event.type) {
            case MESSAGE_KEY_DOWN:
                noticeStamp = 0;
                switch (event.keyCode) {
                    case INPUT_ASCII_ESCAPE:
                    case EncodeScanCode(INPUT_SCAN_F1):
                        closeWindow = true;
                        break;
                    case INPUT_ASCII_DELETE:
                        if (inputLength > 0)
                            inputLength--;
                        redrawInput = true;
                        cursorVisible = 1;
                        break;
                    case '\n':
                        enterPressed = true;
                        break;
                    default:
                        if (inputLength < 58 && event.keyCode) {
                            inputText[inputLength] = 0;
                            inputWidth = boxFont->LineWidth(inputText);
                            if (inputWidth + 30 < 610) {
                                inputText[inputLength] = event.keyCode & 0xff;
                                inputLength++;
                                redrawInput = true;
                                cursorVisible = 0;
                            }
                        }
                }
        }

        if (!redrawInput && gTimers[NET_BOX_BLINK_TIMER_SLOT] < KBTickCount()) {
            cursorVisible = 1 - cursorVisible;
            redrawInput = true;
        }
        if (enterPressed) {
            enterPressed = false;
            inputText[inputLength] = 0;
            AddNetBoxLine(inputText);
            sendOk = TransmitRemoteData(
                inputText,
                REMOTE_BROADCAST_PLAYER,
                strlen(inputText) + 1,
                REMOTE_COMMAND_CHAT,
                true
            );
            if (!sendOk)
                ShutDown(NULL);
            inputLength = 0;
            strcpy(inputText, "");
            redrawInput = true;
            redrawText = true;
        }
        if (redrawText) {
            redrawText = false;
            SET_WIDGET_MESSAGE(messageData, WIDGET_COMMAND_SET_TEXT, NET_BOX_LINE_PREVIOUS);
            messageData.text = gNetBoxLine[NET_BOX_SLOT_PREVIOUS];
            netBox->BroadcastMessage(messageData);
            messageData.id = NET_BOX_LINE_LATEST;
            messageData.text = gNetBoxLine[NET_BOX_SLOT_LATEST];
            netBox->BroadcastMessage(messageData);
            netBox->DrawWindow();
            gWindowManager->UpdateScreenRegion(0, 418, 639, 61);
        }
        if (redrawInput) {
            redrawInput = false;
            gTimers[NET_BOX_BLINK_TIMER_SLOT] = KBTickCount() + NET_BOX_BLINK_DELAY;
            if (cursorVisible)
                inputText[inputLength] = '_';
            else
                inputText[inputLength] = ' ';
            inputText[inputLength + 1] = 0;
            SET_WIDGET_MESSAGE(messageData, WIDGET_COMMAND_SET_TEXT, NET_BOX_INPUT);
            messageData.text = inputText;
            netBox->BroadcastMessage(messageData);
            netBox->DrawWindow();
            gWindowManager->UpdateScreenRegion(0, 460, 639, 16);
        }
        if (noticeStamp && noticeStamp + 6000 < KBTickCount())
            closeWindow = true;
        if (exitForIncomingData) {
            for (closeDelay = 0; closeDelay < 30; closeDelay++) {
                PollSound();
                DelayMilli(90);
            }
            closeWindow = true;
        }
    }
    gInputManager->SetKeyCodeType(INPUT_KEY_CODE_SCAN);
    gWindowManager->RemoveWindow(netBox);
    gShowIt = savedShowIt;
    if (pointerWasVisible)
        gMouseManager->ReallyShowPointer();
    gResourceManager->Dispose(boxFont);
}

void AddNetBoxLine(char* text) {
    // A peer's chat line can be longer than a line holds.
    strcpy(gNetBoxLine[NET_BOX_SLOT_PREVIOUS], gNetBoxLine[NET_BOX_SLOT_LATEST]);
    strncpy(gNetBoxLine[NET_BOX_SLOT_LATEST], text, sizeof(gNetBoxLine[1]) - 1);
    gNetBoxLine[1][sizeof(gNetBoxLine[1]) - 1] = 0;
}

b8 gKBDone = false;
b8 gInCheckEndGame = false;

void ShutDown(char* message) {
    static b32 gInShutDown = false;
    char buffer[768];
    if (gInShutDown)
        return;
    gInShutDown = true;
    gClosingApp = true;
    buffer[0] = '\0';
    if (message) {
        strcpy(buffer, message);
        SetFullScreenStatus(false);
        KBErrorBox(buffer, localization::Tr("shutdown.unexpected.title"));
    }
    CloseSmackers();
    ClearMapExtra();
    UnloadSystemwideIcons();
    if (gRemoteOn)
        HandleRemoteSuddenExit();
    if (gPalette) {
        gResourceManager->Dispose(gPalette);
        gPalette = NULL;
    }
    if (gPhilAI->m_debugFont) {
        gResourceManager->Dispose(gPhilAI->m_debugFont);
        gPhilAI->m_debugFont = NULL;
    }
    gExec->ShutDownSystem();
    RemoteCleanup();
    KBReleaseInstance();
    DeleteMainClasses();
    AppExit();
    exit(EXIT_SUCCESS);
}

void FileError(char* filename) {
    char message[200];
    sprintf(message, localization::Tr("file.open.failed"), filename);
    ShutDown(message);
}

// Writes a count for a narrow label: in thousands ("12k") from
// `thousandsFrom` on, and in millions ("3m") from a million on.
void FormatAbbreviatedCount(char* text, i32 value, i32 thousandsFrom) {
    if (value >= 1000000)
        sprintf(text, "%dm", value / 1000000);
    else if (value >= thousandsFrom)
        sprintf(text, "%dk", value / 1000);
    else
        sprintf(text, "%d", value);
}

void ShowCongrats(void) {
    char name[32];
    i32 labelIndex;
    i32 total;
    tag_message message;
    i32 baseScore;
    heroWindow* window;

    baseScore = GetBaseScore(gCurTurn);
    total = baseScore * gGame->m_difficultyRating / 100;
    PlayMusic(MUSIC_TRACK_CONGRATULATIONS);
    gMouseManager->ReallyHidePointer();
    sprintf(gText, "congrats.bmp");
    gResourceManager->GetBackdrop(gText, gWindowManager->m_screen);
    message.type = MESSAGE_WIDGET;
    message.command = WIDGET_COMMAND_SET_TEXT;
    message.text = gText;
    if (gGame->m_campaignType > 0) {
        window = new heroWindow(0, 0, "congrats.bin");
        if (!window)
            MemError();
        sprintf(gText, gCampaignWinTexts[gGame->m_campaignScenario]);
        message.id = CONGRATS_TITLE;
        window->BroadcastMessage(message);
    } else {
        window = new heroWindow(0, 0, "congspre.bin");
        if (!window)
            MemError();
        sprintf(name, gArmyNames[GetMonType(total, HIGH_SCORE_TYPE_STANDARD)]);
        name[0] = CyrillicToUpper(name[0]);
        sprintf(gText, localization::Tr("congratulations.victory.title"));
        message.id = CONGRATS_TITLE;
        window->BroadcastMessage(message);
        for (labelIndex = 0; labelIndex < CONGRATS_SCORE_LABEL_COUNT; labelIndex++) {
            sprintf(gText, gScoreLabels[labelIndex]);
            message.id = labelIndex + CONGRATS_SCORE_LABEL_FIRST;
            window->BroadcastMessage(message);
        }
        sprintf(gText, "%d", gCurTurn);
        message.id = CONGRATS_DAYS;
        window->BroadcastMessage(message);
        sprintf(gText, "%d", baseScore);
        message.id = CONGRATS_BASE_SCORE;
        window->BroadcastMessage(message);
        sprintf(gText, "%d%%", gGame->m_difficultyRating);
        message.id = CONGRATS_DIFFICULTY;
        window->BroadcastMessage(message);
        sprintf(gText, "%d", total);
        message.id = CONGRATS_FINAL_SCORE;
        window->BroadcastMessage(message);
        sprintf(gText, "%s", name);
        message.id = CONGRATS_RATING;
        window->BroadcastMessage(message);
    }
    gWindowManager->AddWindow(window, WINDOW_Z_ORDER_APPEND, 1);
    gMouseManager->ReallyHidePointer();
    gWindowManager->FadeScreen(WINDOW_FADE_IN, WINDOW_FADE_SHORT, NULL);
    CongratsWait();
    gWindowManager->RemoveWindow(window);
    delete window;
    if (gGame->m_campaignType <= 0)
        AddScoreToHighScore(total, HIGH_SCORE_TYPE_STANDARD, "", gGame->m_mapName);
}

void CongratsWait(void) {
    b32 command = false;
    b8 done = false;
    tag_message msg;
    gInputManager->Flush();
    while (!done) {
        PollSound();
        Process1WindowsMessage();
        msg = gInputManager->GetEvent();
        if (msg.type == MESSAGE_KEY_DOWN || msg.type == MESSAGE_LEFT_BUTTON_DOWN
            || msg.type == MESSAGE_LEFT_BUTTON_UP || msg.type == MESSAGE_RIGHT_BUTTON_DOWN
            || msg.type == MESSAGE_RIGHT_BUTTON_UP)
            done = true;
    }
}

void GetDataEntry(char* prompt, char* destination, i32 maximumLength, char* initialText) {
    i16 widgetIdNo = DATA_ENTRY_TEXT;
    tag_message message;
    char textBuffer[100];

    gMouseManager->SetPointer("advmice.mse", ADVENTURE_POINTER_DEFAULT);
    gDataEntryDest = destination;
    gDataEntryMaxLen = maximumLength;
    strcpy(gDataEntryDest, "");
    gDataEntryWindow = new heroWindow(0xb1, 0x14, "dataentr.bin");
    if (!gDataEntryWindow)
        MemError();
    SET_WIDGET_MESSAGE(message, WIDGET_COMMAND_SET_TEXT, DATA_ENTRY_PROMPT);
    message.text = prompt;
    gDataEntryWindow->BroadcastMessage(message);
    if (initialText)
        strcpy(textBuffer, initialText);
    else
        strcpy(textBuffer, "");
    message.id = DATA_ENTRY_TEXT;
    message.text = textBuffer;
    gDataEntryWindow->BroadcastMessage(message);
    strcpy(destination, textBuffer);
    gDataEntryTime = DATA_ENTRY_STEP_FOCUS;
    gWindowManager->DoDialog(gDataEntryWindow, DataEntryWindowHandler, false);
    delete gDataEntryWindow;
}

i16 DataEntryWindowHandler(tag_message& message) {
    i16 widgetId = DATA_ENTRY_TEXT;

    if (gDataEntryTime == DATA_ENTRY_STEP_FOCUS) {
        ++gDataEntryTime;
        message.type = MESSAGE_LEFT_BUTTON_DOWN;
        message.x = 0xc3;
        message.y = 0x9a;
        gDataEntryWindow->BroadcastMessage(message);
        return MESSAGE_DISPATCH_CONSUME;
    }

    if (gDataEntryTime == DATA_ENTRY_STEP_READ) {
        ++gDataEntryTime;
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
                        gDataEntryWindow->BroadcastMessage(message);
                        if (strlen(message.text) == 0) {
                            break;
                        } else {
                            memset(gDataEntryDest, 0, gDataEntryMaxLen);
                            strncpy(gDataEntryDest, message.text, gDataEntryMaxLen - 1);
                        }
                        SET_WIDGET_MESSAGE(message, WIDGET_COMMAND_SET_TEXT, DATA_ENTRY_TEXT);
                        message.text = gDataEntryDest;
                        gDataEntryWindow->BroadcastMessage(message);
                        gDataEntryWindow->DrawWindow(1, DATA_ENTRY_TEXT, DATA_ENTRY_TEXT);
                        FINISH_DIALOG_MESSAGE(message);
                        return MESSAGE_DISPATCH_FORWARD;
                }
        }
    }
    return EventWindowHandler(message);
}

void MemError(void) {
    static b8 gInMemError = false;
    if (gInMemError)
        return;
    gInMemError = true;
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

i32 MemSize(i32) {
    return 16034;
}

b8 CheckMem(void) {
    return true;
}

char* GetTownName(i32 townIndex) {
    town* townPointer = gGame->GetTown(townIndex);
    if (gGame->m_campaignType > 0 && gCampaignScenarios[gGame->m_campaignScenario].victoryTownX >= 0
        && gCampaignScenarios[gGame->m_campaignScenario].victoryTownX == townPointer->m_x
        && gCampaignScenarios[gGame->m_campaignScenario].victoryTownY == townPointer->m_y)
        return gCampaignScenarios[gGame->m_campaignScenario].victoryTownName;
    return gTownNames[townPointer->m_nameIndex];
}

bool IsCDDrive(i32 driveIndex) {
    return KBIsCDDrive(driveIndex) != 0;
}

void LoadSystemwideIcons(void) {
    gBuyBuildIcons = gResourceManager->GetIcon("buybuild.icn");
    gSystemIcons = gResourceManager->GetIcon("system.icn");
    gBigFont = gResourceManager->GetFont("bigfont.fnt");
    gSmallFont = gResourceManager->GetFont("smalfont.fnt");
}

void UnloadSystemwideIcons(void) {
    gResourceManager->Dispose(gBuyBuildIcons);
    gResourceManager->Dispose(gSystemIcons);
    gResourceManager->Dispose(gBigFont);
    gResourceManager->Dispose(gSmallFont);
}

void EarlyShutDownSystem(void) {}

b32 GameUnsaved(void) {
    if ((gAdvManager && gAdvManager->m_active == 1)
        || (gCombatManager && gCombatManager->m_active == 1)
        || (gTownManager && gTownManager->m_active == 1))
        return true;
    else
        return false;
}

i32 HandleAppSpecificMenuCommands(i32 command) {
    b32 menuChanged;

    menuChanged = false;
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
            strcpy(gText, localization::Tr("game.load.confirm"));
        confirmMenuCommand:
            if (gAdvManager->m_active == 1) {
                NormalDialog(gText, NORMAL_DIALOG_TYPE_YES_NO);
                if (gWindowManager->m_dialogResult != NORMAL_DIALOG_CONFIRM)
                    break;
            }
            gMenuCommand = command;
            break;
        case APP_MENU_SAVE_GAME:
            SaveGame();
            break;
        case APP_MENU_QUIT:
            KBRequestClose();
            break;
        case APP_MENU_MUSIC_OFF:
            gConfig.musicVolume = SOUND_VOLUME_OFF;
            goto adjustMusic;
        case APP_MENU_MUSIC_100:
            gConfig.musicVolume = SOUND_VOLUME_100;
            goto adjustMusic;
        case APP_MENU_MUSIC_90:
            gConfig.musicVolume = SOUND_VOLUME_90;
            goto adjustMusic;
        case APP_MENU_MUSIC_80:
            gConfig.musicVolume = SOUND_VOLUME_80;
            goto adjustMusic;
        case APP_MENU_MUSIC_70:
            gConfig.musicVolume = SOUND_VOLUME_70;
            goto adjustMusic;
        case APP_MENU_MUSIC_60:
            gConfig.musicVolume = SOUND_VOLUME_60;
            goto adjustMusic;
        case APP_MENU_MUSIC_50:
            gConfig.musicVolume = SOUND_VOLUME_50;
            goto adjustMusic;
        case APP_MENU_MUSIC_40:
            gConfig.musicVolume = SOUND_VOLUME_40;
            goto adjustMusic;
        case APP_MENU_MUSIC_30:
            gConfig.musicVolume = SOUND_VOLUME_30;
            goto adjustMusic;
        case APP_MENU_MUSIC_20:
            gConfig.musicVolume = SOUND_VOLUME_20;
            goto adjustMusic;
        case APP_MENU_MUSIC_10:
            gConfig.musicVolume = SOUND_VOLUME_10;
            goto adjustMusic;
        adjustMusic:
            SetMusicVolume(gConfig.musicVolume);
            menuChanged = true;
            break;
        case APP_MENU_SOUND_OFF:
            gConfig.soundVolume = SOUND_VOLUME_OFF;
            goto adjustSound;
        case APP_MENU_SOUND_100:
            gConfig.soundVolume = SOUND_VOLUME_100;
            goto adjustSound;
        case APP_MENU_SOUND_90:
            gConfig.soundVolume = SOUND_VOLUME_90;
            goto adjustSound;
        case APP_MENU_SOUND_80:
            gConfig.soundVolume = SOUND_VOLUME_80;
            goto adjustSound;
        case APP_MENU_SOUND_70:
            gConfig.soundVolume = SOUND_VOLUME_70;
            goto adjustSound;
        case APP_MENU_SOUND_60:
            gConfig.soundVolume = SOUND_VOLUME_60;
            goto adjustSound;
        case APP_MENU_SOUND_50:
            gConfig.soundVolume = SOUND_VOLUME_50;
            goto adjustSound;
        case APP_MENU_SOUND_40:
            gConfig.soundVolume = SOUND_VOLUME_40;
            goto adjustSound;
        case APP_MENU_SOUND_30:
            gConfig.soundVolume = SOUND_VOLUME_30;
            goto adjustSound;
        case APP_MENU_SOUND_20:
            gConfig.soundVolume = SOUND_VOLUME_20;
            goto adjustSound;
        case APP_MENU_SOUND_10:
            gConfig.soundVolume = SOUND_VOLUME_10;
            goto adjustSound;
        adjustSound:
            SetEffectsVolume(gConfig.soundVolume);
            menuChanged = true;
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
            menuChanged = true;
            break;
        case APP_MENU_CD_STEREO:
            if (gConfig.musicSource) {
                gConfig.musicSource = SOUND_MUSIC_SOURCE_DIGITAL;
            } else {
                gConfig.musicSource = SOUND_MUSIC_SOURCE_CD;
            }
            SetMusicSource(gConfig.musicSource != SOUND_MUSIC_SOURCE_DIGITAL);
            menuChanged = true;
            break;
        case APP_MENU_SHOW_PATH:
            gConfig.showRoute = 1 - gConfig.showRoute;
            menuChanged = true;
            break;
        case APP_MENU_VIEW_ENEMY_MOVES:
            gConfig.blackoutComputer = 1 - gConfig.blackoutComputer;
            menuChanged = true;
            break;
        case APP_MENU_VIEW_WORLD:
            gAdvManager->ViewWorld(SPELL_VIEW_ALL, false, false);
            break;
        case APP_MENU_VIEW_PUZZLE:
            gAdvManager->ViewPuzzle();
            break;
        case APP_MENU_CAST_SPELL:
            gAdvManager->CheckCastSpell();
            break;
        case APP_MENU_DIG:
            gAdvManager->ProcessSearch(ADVMGR_SEARCH_VIEW_CENTER, ADVMGR_SEARCH_VIEW_CENTER);
            break;
        default:
            return 1;
    }
    if (menuChanged)
        WritePrefs();
    return 0;
}

void UpdateSystemOptionsMenu(void) {
    i32 checkedCommand;
    i32 menuCommand;

    if (!CURRENT_GRAPHICS_CONFIG.showMenu)
        return;
    if (!gAppMenu)
        return;
    if (gAppMenu != gAdventureMenu)
        return;

    for (menuCommand = APP_MENU_MUSIC_FIRST; menuCommand <= APP_MENU_MUSIC_LAST; menuCommand++)
        KBCheckMenuItem(gAppMenu, menuCommand, KB_MENU_UNCHECKED);
    switch (gConfig.musicVolume) {
        case SOUND_VOLUME_100:
            checkedCommand = APP_MENU_MUSIC_100;
            break;
        case SOUND_VOLUME_90:
            checkedCommand = APP_MENU_MUSIC_90;
            break;
        case SOUND_VOLUME_80:
            checkedCommand = APP_MENU_MUSIC_80;
            break;
        case SOUND_VOLUME_70:
            checkedCommand = APP_MENU_MUSIC_70;
            break;
        case SOUND_VOLUME_60:
            checkedCommand = APP_MENU_MUSIC_60;
            break;
        case SOUND_VOLUME_50:
            checkedCommand = APP_MENU_MUSIC_50;
            break;
        case SOUND_VOLUME_40:
            checkedCommand = APP_MENU_MUSIC_40;
            break;
        case SOUND_VOLUME_30:
            checkedCommand = APP_MENU_MUSIC_30;
            break;
        case SOUND_VOLUME_20:
            checkedCommand = APP_MENU_MUSIC_20;
            break;
        case SOUND_VOLUME_10:
            checkedCommand = APP_MENU_MUSIC_10;
            break;
        default:
            checkedCommand = APP_MENU_MUSIC_OFF;
            break;
    }
    KBCheckMenuItem(gAppMenu, checkedCommand, KB_MENU_CHECKED);

    for (menuCommand = APP_MENU_SOUND_FIRST; menuCommand <= APP_MENU_SOUND_LAST; menuCommand++)
        KBCheckMenuItem(gAppMenu, menuCommand, KB_MENU_UNCHECKED);
    switch (gConfig.soundVolume) {
        case SOUND_VOLUME_100:
            checkedCommand = APP_MENU_SOUND_100;
            break;
        case SOUND_VOLUME_90:
            checkedCommand = APP_MENU_SOUND_90;
            break;
        case SOUND_VOLUME_80:
            checkedCommand = APP_MENU_SOUND_80;
            break;
        case SOUND_VOLUME_70:
            checkedCommand = APP_MENU_SOUND_70;
            break;
        case SOUND_VOLUME_60:
            checkedCommand = APP_MENU_SOUND_60;
            break;
        case SOUND_VOLUME_50:
            checkedCommand = APP_MENU_SOUND_50;
            break;
        case SOUND_VOLUME_40:
            checkedCommand = APP_MENU_SOUND_40;
            break;
        case SOUND_VOLUME_30:
            checkedCommand = APP_MENU_SOUND_30;
            break;
        case SOUND_VOLUME_20:
            checkedCommand = APP_MENU_SOUND_20;
            break;
        case SOUND_VOLUME_10:
            checkedCommand = APP_MENU_SOUND_10;
            break;
        default:
            checkedCommand = APP_MENU_SOUND_OFF;
            break;
    }
    KBCheckMenuItem(gAppMenu, checkedCommand, KB_MENU_CHECKED);

    for (menuCommand = APP_MENU_SPEED_FIRST; menuCommand <= APP_MENU_SPEED_LAST; menuCommand++)
        KBCheckMenuItem(gAppMenu, menuCommand, KB_MENU_UNCHECKED);
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
    KBCheckMenuItem(gAppMenu, checkedCommand, KB_MENU_CHECKED);
    KBCheckMenuItem(
        gAppMenu,
        APP_MENU_CD_STEREO,
        gConfig.musicSource ? KB_MENU_CHECKED : KB_MENU_UNCHECKED
    );
    KBCheckMenuItem(
        gAppMenu,
        APP_MENU_SHOW_PATH,
        gConfig.showRoute ? KB_MENU_CHECKED : KB_MENU_UNCHECKED
    );
    KBCheckMenuItem(
        gAppMenu,
        APP_MENU_VIEW_ENEMY_MOVES,
        1 - gConfig.blackoutComputer ? KB_MENU_CHECKED : KB_MENU_UNCHECKED
    );
}

void CleanUpMenus(void) {
    if (gAppMenu) {
        KBDetachMenu();
        if (gAdventureMenu)
            KBDestroyMenu(gAdventureMenu);
        if (gDefaultMenu)
            KBDestroyMenu(gDefaultMenu);
        if (gCombatMenu)
            KBDestroyMenu(gCombatMenu);
        if (gTownMenu)
            KBDestroyMenu(gTownMenu);
    }
    gAppMenu = NULL;
}

void UpdateAppSpecificMenus(void* menu) {
    if (menu == gAdventureMenu)
        UpdateSystemOptionsMenu();
}

void EarlyResizeWindow(i32 x, i32 y, i32 width, i32 height) {
    if (gClosingApp)
        return;
}

i16 gCastleAmounts[4] = {20, 20, 0, 0};
i16 gHeroGoldCost = 2500;
i16 gVesaMode[6] = {640, 480, 256, 20226, 257, 0};
tag_tilePoint gNormalDirTable[8] = {
    {0, -1, 16},
    {1, -1, 16},
    {1, 0, 16},
    {1, 1, 16},
    {0, 1, 16},
    {-1, 1, 16},
    {-1, 0, 16},
    {-1, -1, 16},
};
TownBuildingExtent gTownBuildingExtents[TOWN_TYPE_COUNT][BUILDING_SLOT_CAPACITY] = {
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
u16 gDwellingRequirements[24] = {
    0, 128, 144, 132, 1536, 1536, 0, 132, 128, 513, 1024, 2048,
    0, 128, 128, 128, 1024, 2048, 0, 128, 128, 256, 512,  3072,
};
i32 gResourceBaseValue[RESOURCE_COUNT] =
    {250, 250, 200, 250, 250, 250, 1};
i32 gStartingResources[DIFFICULTY_COUNT][RESOURCE_COUNT] = {
    {30, 10, 30, 10, 10, 10, 10000},
    {20, 5, 20, 5, 5, 5, 7500},
    {10, 0, 10, 0, 0, 0, 5000},
    {0, 0, 0, 0, 0, 0, 0},
};
i32 gMineIncome[RESOURCE_COUNT] = {2, 1, 2, 1, 1, 1, 1000};
i32 gArtifactBaseRV[ARTIFACT_REGULAR_END] = {
    9000, 22000, 18000, 14000, 6000, 4000, 4000, 5600, 1200, 1200, 1200, 1200, -1200,
    2000, 1800,  1800,  2000,  1000, 3600, 5600, 4000, 5040, 2700, 3900, 4950, 5850,
    7000, 6000,  4000,  4500,  2250, 1200, 1200, 1200, 1200, 3500, 1500,
};
i32 gUltArtifactAvgValue = 16200;
char gDataPath[352] = ".\\DATA\\";
char gAnimPath[352] = ".\\ANIM\\";
char gSoundPath[352] = ".\\SOUND\\";
char gTracksPath[352] = "\\TRACKS\\";
char gGamePath[20] = ".\\GAMES\\";
char gMapPath[20] = ".\\MAPS\\";
i8 gHeroScoutRadius[8] = {4, 4, 4, 6, 4, 0, 0, 0};
float gClassNavigationMod[8] = {1.0f, 1.0f, 2.0f, 1.0f, 1.0f, 1.3f, 1.0f, 1.0f};
i8 gVisRangeTown = 5;
tag_monsterInfo gMonsterDatabase[CREATURE_COUNT] = {
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
float gStatPower[41] = {
    0.63f, 0.63f, 0.63f, 0.63f, 0.63f, 0.63f, 0.64f, 0.65f, 0.67f, 0.68f, 0.7f,
    0.72f, 0.74f, 0.76f, 0.78f, 0.81f, 0.84f, 0.87f, 0.91f, 0.95f, 1.0f,  1.05f,
    1.1f,  1.15f, 1.22f, 1.28f, 1.36f, 1.44f, 1.53f, 1.63f, 1.74f, 1.86f, 1.99f,
    2.14f, 2.3f,  2.48f, 2.67f, 2.86f, 2.86f, 2.86f, 2.86f,
};
float gBattleStat[41] = {
    0.2f,  0.2f,  0.2f,  0.2f,  0.2f,  0.21f, 0.23f, 0.25f, 0.28f, 0.31f, 0.35f,
    0.39f, 0.43f, 0.48f, 0.53f, 0.59f, 0.66f, 0.73f, 0.81f, 0.9f,  1.0f,  1.1f,
    1.21f, 1.33f, 1.46f, 1.61f, 1.77f, 1.95f, 2.14f, 2.36f, 2.59f, 2.85f, 3.14f,
    3.45f, 3.8f,  4.18f, 4.59f, 5.0f,  5.0f,  5.0f,  5.0f,
};
i8 gMageGuildSpellCount[4] = {3, 5, 7, 9};
float gSpellCastNumMod[21] = {
    0.0f,  1.0f,  1.7f,  2.2f,  2.6f,  2.95f, 3.27f, 3.56f, 3.81f, 4.04f, 4.25f,
    4.45f, 4.64f, 4.83f, 5.01f, 5.19f, 5.36f, 5.53f, 5.68f, 5.82f, 5.96f,
};
u8 gUnusedByteTable1[16] = {0, 0, 2, 9, 4, 17, 10, 13, 6, 8, 16, 12, 11, 15, 14, 18};
u8 gUnusedByteTable2[16] = {4, 2, 2, 1, 2, 1, 1, 1, 2, 1, 1, 1, 1, 1, 1, 1};
i16 gMinExpForLevel[4][12] = {
    {0, 1000, 2000, 3200, 4500, 6000, 7700, 9000, 11000, 13200, 15500, 18500},
    {0, 1000, 2000, 3200, 4500, 6000, 7700, 9000, 11000, 13200, 15500, 18500},
    {0, 1000, 2000, 3200, 4500, 6000, 7700, 9000, 11000, 13200, 15500, 18500},
    {0, 1000, 2000, 3200, 4500, 6000, 7700, 9000, 11000, 13200, 15500, 18500},
};
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
i8 gMons32Width[CREATURE_COUNT] = {
    20, 20, 20, 25, 25, 24, 21, 21, 25, 27, 22, 20, 23, 23,
    21, 22, 25, 23, 27, 22, 29, 28, 32, 27, 21, 26, 21, 29,
};
i16 gScoreMon[SCORE_MONSTER_COUNT][2] = {
    {0, 0},    {7, 6},    {14, 12},  {21, 18},  {28, 24},  {35, 7},   {42, 1},
    {49, 19},  {56, 13},  {63, 2},   {70, 8},   {77, 25},  {84, 14},  {91, 20},
    {98, 3},   {105, 9},  {112, 15}, {119, 21}, {126, 4},  {133, 26}, {140, 16},
    {147, 10}, {154, 22}, {161, 5},  {168, 27}, {175, 11}, {182, 17}, {189, 23},
};
i16 gScoreCampaignMon[SCORE_MONSTER_COUNT][2] = {
    {3600, 0},  {3400, 6},  {3200, 12}, {3000, 18}, {2600, 24}, {2400, 7},  {2200, 1},
    {2000, 19}, {1800, 13}, {1600, 2},  {1500, 8},  {1400, 25}, {1300, 14}, {1200, 20},
    {1100, 3},  {1000, 9},  {900, 15},  {800, 21},  {750, 4},   {700, 26},  {650, 16},
    {600, 10},  {550, 22},  {500, 5},   {450, 27},  {400, 11},  {350, 17},  {300, 23},
};
WindowTextEntry gWinSetup[68] = {
    {0, 0},    {1, 0},    {0, 1},    {50, 2},   {16, 2},   {17, 2},   {18, 2},   {19, 2},
    {20, 2},   {49, 2},   {100, 3},  {101, 3},  {102, 3},  {103, 3},  {104, 3},  {105, 3},
    {1, 4},    {400, 5},  {401, 5},  {402, 5},  {403, 5},  {80, 6},   {600, 7},  {601, 7},
    {602, 7},  {603, 7},  {604, 7},  {605, 7},  {5, 7},    {6, 7},    {7, 7},    {606, 7},
    {607, 7},  {608, 7},  {200, 8},  {201, 8},  {202, 8},  {203, 8},  {204, 8},  {205, 8},
    {600, 9},  {601, 9},  {602, 9},  {603, 9},  {600, 10}, {600, 11}, {0, 12},   {1, 12},
    {600, 13}, {601, 13}, {602, 13}, {604, 13}, {0, 14},   {601, 14}, {0, 15},   {600, 15},
    {601, 15}, {602, 15}, {603, 15}, {604, 15}, {605, 15}, {606, 15}, {607, 15}, {608, 15},
    {609, 15}, {610, 15}, {611, 15}, {1, 16},
};
i8 gTownTheme[TOWN_TYPE_COUNT] = {3, 0, 2, 1};
campaignScenario gCampaignScenarios[CAMPAIGN_SCENARIO_COUNT] = {
    {0,
     36,
     35,
     localization::Chars("campaign.town_name.0"),
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
     localization::Chars("campaign.town_name.4"),
     {0, 3, 0, 0},
     {2, 4, 4},
     {{30, 10, 30, 10, 10, 10, 10000},
      {30, 10, 30, 10, 10, 10, 10000},
      {30, 10, 30, 10, 10, 10, 10000},
      {30, 10, 30, 10, 10, 10, 10000}}},
    {0,
     62,
     20,
     localization::Chars("campaign.town_name.5"),
     {0, 3, 0, 0},
     {1, 4, 4},
     {{30, 10, 30, 10, 10, 10, 10000},
      {30, 10, 30, 10, 10, 10, 10000},
      {30, 10, 30, 10, 10, 10, 10000},
      {30, 10, 30, 10, 10, 10, 10000}}},
    {0,
     8,
     8,
     localization::Chars("campaign.town_name.6"),
     {0, 3, 0, 0},
     {3, 4, 4},
     {{30, 10, 30, 10, 10, 10, 10000},
      {30, 10, 30, 10, 10, 10, 10000},
      {30, 10, 30, 10, 10, 10, 10000},
      {30, 10, 30, 10, 10, 10, 10000}}},
    {0,
     66,
     69,
     localization::Chars("campaign.town_name.7"),
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
i8 gCampaignSideCrests[4][2] = {
    {PLAYER_COLOR_RED, PLAYER_COLOR_BLUE},
    {PLAYER_COLOR_GREEN, PLAYER_COLOR_BLUE},
    {PLAYER_COLOR_YELLOW, PLAYER_COLOR_BLUE},
    {PLAYER_COLOR_BLUE, PLAYER_COLOR_BLUE},
};
i16 gCrestTownTypes[PLAYER_COLOR_COUNT] =
    {TOWN_TYPE_WARLOCK, TOWN_TYPE_BARBARIAN, TOWN_TYPE_KNIGHT, TOWN_TYPE_SORCERESS};
i16 gCrestHeroClass[PLAYER_COLOR_COUNT] = {3, 1, 0, 2};
i8 gHeroSkillBonus[4][9][HERO_PRIMARY_STAT_COUNT] = {
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
i8 gTownHeroClass[8] = {0, 2, 1, 3, 0, 2, 1, 3};
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
i32 gMonoIconSkip = -1;
b32 gEnlargeScreenBlit = true;
i32 gMenuCommand = APP_MENU_NONE;
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
char* gArtifactNames[ARTIFACT_COUNT] = {
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
char* gArtifactDesc[ARTIFACT_COUNT] = {
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
char* gArtifactEvent[38] = {
    "",
    "",
    "",
    "",
    localization::Tr("table.gArtifactEvent.4"),
    localization::Tr("table.gArtifactEvent.5"),
    localization::Tr("table.gArtifactEvent.6"),
    localization::Tr("table.gArtifactEvent.7"),
    localization::Tr("table.gArtifactEvent.8"),
    localization::Tr("table.gArtifactEvent.9"),
    localization::Tr("table.gArtifactEvent.10"),
    localization::Tr("table.gArtifactEvent.11"),
    localization::Tr("table.gArtifactEvent.12"),
    localization::Tr("table.gArtifactEvent.13"),
    localization::Tr("table.gArtifactEvent.14"),
    localization::Tr("table.gArtifactEvent.15"),
    localization::Tr("table.gArtifactEvent.16"),
    localization::Tr("table.gArtifactEvent.17"),
    localization::Tr("table.gArtifactEvent.18"),
    localization::Tr("table.gArtifactEvent.19"),
    localization::Tr("table.gArtifactEvent.20"),
    localization::Tr("table.gArtifactEvent.21"),
    localization::Tr("table.gArtifactEvent.22"),
    localization::Tr("table.gArtifactEvent.23"),
    localization::Tr("table.gArtifactEvent.24"),
    localization::Tr("table.gArtifactEvent.25"),
    localization::Tr("table.gArtifactEvent.26"),
    localization::Tr("table.gArtifactEvent.27"),
    localization::Tr("table.gArtifactEvent.28"),
    localization::Tr("table.gArtifactEvent.29"),
    localization::Tr("table.gArtifactEvent.30"),
    localization::Tr("table.gArtifactEvent.31"),
    localization::Tr("table.gArtifactEvent.32"),
    localization::Tr("table.gArtifactEvent.33"),
    localization::Tr("table.gArtifactEvent.34"),
    localization::Tr("table.gArtifactEvent.35"),
    localization::Tr("table.gArtifactEvent.36"),
    localization::Tr("table.gArtifactEvent.37"),
};
char* gStatNames[5] = {
    localization::Tr("table.gStatNames.0"),
    localization::Tr("table.gStatNames.1"),
    localization::Tr("table.gStatNames.2"),
    localization::Tr("table.gStatNames.3"),
    localization::Tr("table.gStatNames.4")
};
char* gStatDesc[5] = {
    localization::Tr("table.gStatDesc.0"),
    localization::Tr("table.gStatDesc.1"),
    localization::Tr("table.gStatDesc.2"),
    localization::Tr("table.gStatDesc.3"),
    localization::Tr("table.gStatDesc.4"),
};
char* gClassNames[4] = {
    localization::Tr("table.gClassNames.0"),
    localization::Tr("table.gClassNames.1"),
    localization::Tr("table.gClassNames.2"),
    localization::Tr("table.gClassNames.3")
};
char* gArmyNames[CREATURE_COUNT] = {
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
char* gArmySpriteNames[CREATURE_COUNT] = {
    "peasant",  "archer", "pikeman", "swordsman", "cavalry", "paladin",  "goblin",
    "orc",      "wolf",   "ogre",    "troll",     "cyclops", "sprite",   "dwarf",
    "elf",      "druid",  "unicorn", "phoenix",   "centaur", "gargoyle", "griffin",
    "minotaur", "hydra",  "dragon",  "rogue",     "nomad",   "ghost",    "genie"
};
char* gArmyNamesPlural[CREATURE_COUNT] = {
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
char* gSpellNames[SPELL_COUNT] = {
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
char* gNeutralBuildingNames[BUILDING_SLOT_NEUTRAL_COUNT] = {
    localization::Tr("table.gNeutralBuildingNames.0"),
    localization::Tr("table.gNeutralBuildingNames.1"),
    localization::Tr("table.gNeutralBuildingNames.2"),
    localization::Tr("table.gNeutralBuildingNames.3"),
    localization::Tr("table.gNeutralBuildingNames.4"),
    localization::Tr("table.gNeutralBuildingNames.5"),
    localization::Tr("table.gNeutralBuildingNames.6")
};
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
char* gTerrainNames[TERRAIN_COUNT] = {
    localization::Tr("table.gTerrainNames.0"),
    localization::Tr("table.gTerrainNames.1"),
    localization::Tr("table.gTerrainNames.2"),
    localization::Tr("table.gTerrainNames.3"),
    localization::Tr("table.gTerrainNames.4"),
    localization::Tr("table.gTerrainNames.5"),
    localization::Tr("table.gTerrainNames.6")
};
char* gResourceNames[RESOURCE_COUNT] = {
    localization::Tr("table.gResourceNames.0"),
    localization::Tr("table.gResourceNames.1"),
    localization::Tr("table.gResourceNames.2"),
    localization::Tr("table.gResourceNames.3"),
    localization::Tr("table.gResourceNames.4"),
    localization::Tr("table.gResourceNames.5"),
    localization::Tr("table.gResourceNames.6")
};
char* gMineNames[RESOURCE_COUNT] = {
    localization::Tr("table.gMineNames.0"),
    localization::Tr("table.gMineNames.1"),
    localization::Tr("table.gMineNames.2"),
    localization::Tr("table.gMineNames.3"),
    localization::Tr("table.gMineNames.4"),
    localization::Tr("table.gMineNames.5"),
    localization::Tr("table.gMineNames.6")
};
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
char* gTownNames[36] = {
    localization::Tr("table.gTownNames.0"),  localization::Tr("table.gTownNames.1"),
    localization::Tr("table.gTownNames.2"),  localization::Tr("table.gTownNames.3"),
    localization::Tr("table.gTownNames.4"),  localization::Tr("table.gTownNames.5"),
    localization::Tr("table.gTownNames.6"),  localization::Tr("table.gTownNames.7"),
    localization::Tr("table.gTownNames.8"),  localization::Tr("table.gTownNames.9"),
    localization::Tr("table.gTownNames.10"), localization::Tr("table.gTownNames.11"),
    localization::Tr("table.gTownNames.12"), localization::Tr("table.gTownNames.13"),
    localization::Tr("table.gTownNames.14"), localization::Tr("table.gTownNames.15"),
    localization::Tr("table.gTownNames.16"), localization::Tr("table.gTownNames.17"),
    localization::Tr("table.gTownNames.18"), localization::Tr("table.gTownNames.19"),
    localization::Tr("table.gTownNames.20"), localization::Tr("table.gTownNames.21"),
    localization::Tr("table.gTownNames.22"), localization::Tr("table.gTownNames.23"),
    localization::Tr("table.gTownNames.24"), localization::Tr("table.gTownNames.25"),
    localization::Tr("table.gTownNames.26"), localization::Tr("table.gTownNames.27"),
    localization::Tr("table.gTownNames.28"), localization::Tr("table.gTownNames.29"),
    localization::Tr("table.gTownNames.30"), localization::Tr("table.gTownNames.31"),
    localization::Tr("table.gTownNames.32"), localization::Tr("table.gTownNames.33"),
    localization::Tr("table.gTownNames.34"), localization::Tr("table.gTownNames.35"),
};
char* gEventText[EVENT_TEXT_COUNT] = {
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
char* gAPanelHelp[5] = {
    localization::Tr("table.gAPanelHelp.0"),
    localization::Tr("table.gAPanelHelp.1"),
    localization::Tr("table.gAPanelHelp.2"),
    localization::Tr("table.gAPanelHelp.3"),
    localization::Tr("table.gAPanelHelp.4"),
};
char* gInitMenuHelp[MAIN_MENU_HELP_COUNT] = {
    localization::Tr("table.gInitMenuHelp.0"),
    localization::Tr("table.gInitMenuHelp.1"),
    localization::Tr("table.gInitMenuHelp.2"),
    localization::Tr("table.gInitMenuHelp.3"),
    localization::Tr("table.gInitMenuHelp.4"),
};
char* gAdvMenuHelp[ADVENTURE_HELP_COUNT] = {
    localization::Tr("table.gAdvMenuHelp.0"),
    localization::Tr("table.gAdvMenuHelp.1"),
    localization::Tr("table.gAdvMenuHelp.2"),
    localization::Tr("table.gAdvMenuHelp.3"),
    localization::Tr("table.gAdvMenuHelp.4"),
    localization::Tr("table.gAdvMenuHelp.5"),
};
char* gLuckText[7] = {
    localization::Tr("table.gLuckText.0"),
    localization::Tr("table.gLuckText.1"),
    localization::Tr("table.gLuckText.2"),
    localization::Tr("table.gLuckText.3"),
    localization::Tr("table.gLuckText.4"),
    localization::Tr("table.gLuckText.5"),
    localization::Tr("table.gLuckText.6"),
};
char* gMoraleText[7] = {
    localization::Tr("table.gMoraleText.0"),
    localization::Tr("table.gMoraleText.1"),
    localization::Tr("table.gMoraleText.2"),
    localization::Tr("table.gMoraleText.3"),
    localization::Tr("table.gMoraleText.4"),
    localization::Tr("table.gMoraleText.5"),
    localization::Tr("table.gMoraleText.6"),
};
char* gOnOffText[11] = {
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
char* gWalkSpeedText[WALK_SPEED_COUNT] = {
    localization::Tr("table.walkSpeedText.0"),
    localization::Tr("table.walkSpeedText.1"),
    localization::Tr("table.walkSpeedText.2"),
    localization::Tr("table.walkSpeedText.3"),
    localization::Tr("table.walkSpeedText.4")
};
char* gColorNames[PLAYER_COLOR_COUNT] = {
    localization::Tr("table.gColorNames.0"),
    localization::Tr("table.gColorNames.1"),
    localization::Tr("table.gColorNames.2"),
    localization::Tr("table.gColorNames.3")
};
char* gAlignmentNames[5] = {
    localization::Tr("table.gAlignmentNames.0"),
    localization::Tr("table.gAlignmentNames.1"),
    localization::Tr("table.gAlignmentNames.2"),
    localization::Tr("table.gAlignmentNames.3"),
    localization::Tr("table.gAlignmentNames.4")
};
char* gSpellDesc[SPELL_COUNT] = {
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
char* gMonthNames[10] = {
    localization::Tr("table.gMonthNames.0"),
    localization::Tr("table.gMonthNames.1"),
    localization::Tr("table.gMonthNames.2"),
    localization::Tr("table.gMonthNames.3"),
    localization::Tr("table.gMonthNames.4"),
    localization::Tr("table.gMonthNames.5"),
    localization::Tr("table.gMonthNames.6"),
    localization::Tr("table.gMonthNames.7"),
    localization::Tr("table.gMonthNames.8"),
    localization::Tr("table.gMonthNames.9"),
};
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
char* gDwellingDescriptions[24] = {
    localization::Tr("table.gDwellingDescriptions.0"),
    localization::Tr("table.gDwellingDescriptions.1"),
    localization::Tr("table.gDwellingDescriptions.2"),
    localization::Tr("table.gDwellingDescriptions.3"),
    localization::Tr("table.gDwellingDescriptions.4"),
    localization::Tr("table.gDwellingDescriptions.5"),
    localization::Tr("table.gDwellingDescriptions.6"),
    localization::Tr("table.gDwellingDescriptions.7"),
    localization::Tr("table.gDwellingDescriptions.8"),
    localization::Tr("table.gDwellingDescriptions.9"),
    localization::Tr("table.gDwellingDescriptions.10"),
    localization::Tr("table.gDwellingDescriptions.11"),
    localization::Tr("table.gDwellingDescriptions.12"),
    localization::Tr("table.gDwellingDescriptions.13"),
    localization::Tr("table.gDwellingDescriptions.14"),
    localization::Tr("table.gDwellingDescriptions.15"),
    localization::Tr("table.gDwellingDescriptions.16"),
    localization::Tr("table.gDwellingDescriptions.17"),
    localization::Tr("table.gDwellingDescriptions.18"),
    localization::Tr("table.gDwellingDescriptions.19"),
    localization::Tr("table.gDwellingDescriptions.20"),
    localization::Tr("table.gDwellingDescriptions.21"),
    localization::Tr("table.gDwellingDescriptions.22"),
    localization::Tr("table.gDwellingDescriptions.23"),
};
char* gArmySizeNames[6][2] = {
    {localization::Tr("table.gArmySizeNames.0"), localization::Tr("table.gArmySizeNames.1")},
    {localization::Tr("table.gArmySizeNames.2"), localization::Tr("table.gArmySizeNames.3")},
    {localization::Tr("table.gArmySizeNames.4"), localization::Tr("table.gArmySizeNames.5")},
    {localization::Tr("table.gArmySizeNames.6"), localization::Tr("table.gArmySizeNames.7")},
    {localization::Tr("table.gArmySizeNames.8"), localization::Tr("table.gArmySizeNames.9")},
    {localization::Tr("table.gArmySizeNames.10"), localization::Tr("table.gArmySizeNames.11")},
};
char* gHeroScreen[HERO_TEXT_COUNT] = {
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
char* gCastleInfo[TOWN_CASTLE_INFO_COUNT] = {
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
char* gLuckInfoText[LUCK_INFO_COUNT] = {
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
    localization::Tr("te.table.gLuckInfoText.11"),
};
char* gMemoryErrorTitle = localization::Tr("table.gMemoryErrorTitle.0");
char* gMemoryRequirements = localization::Tr("table.gMemoryRequirements.0");
char* gExtendedMemoryUnits = localization::Tr("table.gExtendedMemoryUnits.0");
char* gConventionalMemoryUnits = localization::Tr("table.gConventionalMemoryUnits.0");
char* gPlayerTypeNames[5] = {
    localization::Tr("table.gPlayerTypeNames.0"),
    localization::Tr("table.gPlayerTypeNames.1"),
    localization::Tr("table.gPlayerTypeNames.2"),
    localization::Tr("table.gPlayerTypeNames.3"),
    localization::Tr("table.gPlayerTypeNames.4")
};
char* gSpellHelp[SPELL_HELP_COUNT] = {
    localization::Tr("table.gSpellHelp.0"),
    localization::Tr("table.gSpellHelp.1"),
    localization::Tr("table.gSpellHelp.2"),
    localization::Tr("table.gSpellHelp.3"),
    localization::Tr("table.gSpellHelp.4"),
    localization::Tr("table.gSpellHelp.5"),
    localization::Tr("table.gSpellHelp.6"),
    localization::Tr("table.gSpellHelp.7"),
};
char* gSpeedText[5] = {
    "",
    localization::Tr("table.gSpeedText.1"),
    localization::Tr("table.gSpeedText.2"),
    localization::Tr("table.gSpeedText.3"),
    localization::Tr("table.gSpeedText.4"),
};
char* gArmyStatText[9] = {
    localization::Tr("table.gArmyStatText.0"),
    localization::Tr("table.gArmyStatText.1"),
    localization::Tr("table.gArmyStatText.2"),
    localization::Tr("table.gArmyStatText.3"),
    localization::Tr("table.gArmyStatText.4"),
    localization::Tr("table.gArmyStatText.5"),
    localization::Tr("table.gArmyStatText.6"),
    localization::Tr("table.gArmyStatText.7"),
    localization::Tr("table.gArmyStatText.8"),
};
char* gOverviewText[3] = {
    localization::Tr("table.gOverviewText.0"),
    localization::Tr("table.gOverviewText.1"),
    localization::Tr("table.gOverviewText.2"),
};
char* gNewTurnText[7] = {
    localization::Tr("table.gNewTurnText.0"),
    localization::Tr("table.gNewTurnText.1"),
    localization::Tr("table.gNewTurnText.2"),
    localization::Tr("table.gNewTurnText.3"),
    localization::Tr("table.gNewTurnText.4"),
    localization::Tr("table.gNewTurnText.5"),
    localization::Tr("table.gNewTurnText.6"),
};
char* gViewGeneralLabels[6] = {
    localization::Tr("table.gViewGeneralLabels.0"),
    localization::Tr("table.gViewGeneralLabels.1"),
    localization::Tr("table.gViewGeneralLabels.2"),
    localization::Tr("table.gViewGeneralLabels.3"),
    localization::Tr("table.gViewGeneralLabels.4"),
    localization::Tr("table.gViewGeneralLabels.5"),
};
char* gViewGeneralHelp[GENERAL_HOVER_HELP_COUNT] = {
    localization::Tr("table.gViewGeneralHelp.0"),
    localization::Tr("table.gViewGeneralHelp.1"),
    localization::Tr("table.gViewGeneralHelp.2"),
    localization::Tr("table.gViewGeneralHelp.3"),
    localization::Tr("table.gViewGeneralHelp.4"),
    localization::Tr("table.gViewGeneralHelp.5"),
};
char* gCombatMessage[COMBAT_TEXT_COUNT] = {
    "",
    localization::Tr("table.gCombatMessage.1"),
    localization::Tr("table.gCombatMessage.2"),
    localization::Tr("table.gCombatMessage.3"),
    localization::Tr("table.gCombatMessage.4"),
    localization::Tr("table.gCombatMessage.5"),
    localization::Tr("table.gCombatMessage.6"),
    localization::Tr("table.gCombatMessage.7"),
    localization::Tr("table.gCombatMessage.8"),
};
char* gHeroLevel[HERO_LEVEL_TEXT_COUNT] = {
    localization::Tr("table.gHeroLevel.0"),
    localization::Tr("table.gHeroLevel.1"),
    localization::Tr("table.gHeroLevel.2")
};
char* gCombatHelp[COMBAT_HELP_COUNT] =
    {localization::Tr("table.gCombatHelp.0"), localization::Tr("table.gCombatHelp.1"), ""};
char* gTownCommand[TOWN_TEXT_COUNT] = {
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
char* gGameTypeHelp[5] = {
    localization::Tr("table.gGameTypeHelp.0"),
    localization::Tr("table.gGameTypeHelp.1"),
    localization::Tr("table.gGameTypeHelp.2"),
    localization::Tr("table.gGameTypeHelp.3"),
    localization::Tr("table.gGameTypeHelp.4"),
};
// Hero, class and creature names in the form a phrase needs: as an object
// (accusative), as an owner (genitive), and as the troops being moved.
// Languages without cases keep the plain names.
char* gHeroNamesAccusative[36] = {
    localization::Tr("te.table.gHeroNamesAccusative.0"),
    localization::Tr("te.table.gHeroNamesAccusative.1"),
    localization::Tr("te.table.gHeroNamesAccusative.2"),
    localization::Tr("te.table.gHeroNamesAccusative.3"),
    localization::Tr("te.table.gHeroNamesAccusative.4"),
    localization::Tr("te.table.gHeroNamesAccusative.5"),
    localization::Tr("te.table.gHeroNamesAccusative.6"),
    localization::Tr("te.table.gHeroNamesAccusative.7"),
    localization::Tr("te.table.gHeroNamesAccusative.8"),
    localization::Tr("te.table.gHeroNamesAccusative.9"),
    localization::Tr("te.table.gHeroNamesAccusative.10"),
    localization::Tr("te.table.gHeroNamesAccusative.11"),
    localization::Tr("te.table.gHeroNamesAccusative.12"),
    localization::Tr("te.table.gHeroNamesAccusative.13"),
    localization::Tr("te.table.gHeroNamesAccusative.14"),
    localization::Tr("te.table.gHeroNamesAccusative.15"),
    localization::Tr("te.table.gHeroNamesAccusative.16"),
    localization::Tr("te.table.gHeroNamesAccusative.17"),
    localization::Tr("te.table.gHeroNamesAccusative.18"),
    localization::Tr("te.table.gHeroNamesAccusative.19"),
    localization::Tr("te.table.gHeroNamesAccusative.20"),
    localization::Tr("te.table.gHeroNamesAccusative.21"),
    localization::Tr("te.table.gHeroNamesAccusative.22"),
    localization::Tr("te.table.gHeroNamesAccusative.23"),
    localization::Tr("te.table.gHeroNamesAccusative.24"),
    localization::Tr("te.table.gHeroNamesAccusative.25"),
    localization::Tr("te.table.gHeroNamesAccusative.26"),
    localization::Tr("te.table.gHeroNamesAccusative.27"),
    localization::Tr("te.table.gHeroNamesAccusative.28"),
    localization::Tr("te.table.gHeroNamesAccusative.29"),
    localization::Tr("te.table.gHeroNamesAccusative.30"),
    localization::Tr("te.table.gHeroNamesAccusative.31"),
    localization::Tr("te.table.gHeroNamesAccusative.32"),
    localization::Tr("te.table.gHeroNamesAccusative.33"),
    localization::Tr("te.table.gHeroNamesAccusative.34"),
    localization::Tr("te.table.gHeroNamesAccusative.35"),
};
char* gHeroNamesGenitive[36] = {
    localization::Tr("te.table.gHeroNamesGenitive.0"),
    localization::Tr("te.table.gHeroNamesGenitive.1"),
    localization::Tr("te.table.gHeroNamesGenitive.2"),
    localization::Tr("te.table.gHeroNamesGenitive.3"),
    localization::Tr("te.table.gHeroNamesGenitive.4"),
    localization::Tr("te.table.gHeroNamesGenitive.5"),
    localization::Tr("te.table.gHeroNamesGenitive.6"),
    localization::Tr("te.table.gHeroNamesGenitive.7"),
    localization::Tr("te.table.gHeroNamesGenitive.8"),
    localization::Tr("te.table.gHeroNamesGenitive.9"),
    localization::Tr("te.table.gHeroNamesGenitive.10"),
    localization::Tr("te.table.gHeroNamesGenitive.11"),
    localization::Tr("te.table.gHeroNamesGenitive.12"),
    localization::Tr("te.table.gHeroNamesGenitive.13"),
    localization::Tr("te.table.gHeroNamesGenitive.14"),
    localization::Tr("te.table.gHeroNamesGenitive.15"),
    localization::Tr("te.table.gHeroNamesGenitive.16"),
    localization::Tr("te.table.gHeroNamesGenitive.17"),
    localization::Tr("te.table.gHeroNamesGenitive.18"),
    localization::Tr("te.table.gHeroNamesGenitive.19"),
    localization::Tr("te.table.gHeroNamesGenitive.20"),
    localization::Tr("te.table.gHeroNamesGenitive.21"),
    localization::Tr("te.table.gHeroNamesGenitive.22"),
    localization::Tr("te.table.gHeroNamesGenitive.23"),
    localization::Tr("te.table.gHeroNamesGenitive.24"),
    localization::Tr("te.table.gHeroNamesGenitive.25"),
    localization::Tr("te.table.gHeroNamesGenitive.26"),
    localization::Tr("te.table.gHeroNamesGenitive.27"),
    localization::Tr("te.table.gHeroNamesGenitive.28"),
    localization::Tr("te.table.gHeroNamesGenitive.29"),
    localization::Tr("te.table.gHeroNamesGenitive.30"),
    localization::Tr("te.table.gHeroNamesGenitive.31"),
    localization::Tr("te.table.gHeroNamesGenitive.32"),
    localization::Tr("te.table.gHeroNamesGenitive.33"),
    localization::Tr("te.table.gHeroNamesGenitive.34"),
    localization::Tr("te.table.gHeroNamesGenitive.35"),
};
char* gClassNamesAccusative[4] = {
    localization::Tr("te.table.gClassNamesAccusative.0"),
    localization::Tr("te.table.gClassNamesAccusative.1"),
    localization::Tr("te.table.gClassNamesAccusative.2"),
    localization::Tr("te.table.gClassNamesAccusative.3"),
};
char* gArmyNamesMoved[28] = {
    localization::Tr("te.table.gArmyNamesMoved.0"),
    localization::Tr("te.table.gArmyNamesMoved.1"),
    localization::Tr("te.table.gArmyNamesMoved.2"),
    localization::Tr("te.table.gArmyNamesMoved.3"),
    localization::Tr("te.table.gArmyNamesMoved.4"),
    localization::Tr("te.table.gArmyNamesMoved.5"),
    localization::Tr("te.table.gArmyNamesMoved.6"),
    localization::Tr("te.table.gArmyNamesMoved.7"),
    localization::Tr("te.table.gArmyNamesMoved.8"),
    localization::Tr("te.table.gArmyNamesMoved.9"),
    localization::Tr("te.table.gArmyNamesMoved.10"),
    localization::Tr("te.table.gArmyNamesMoved.11"),
    localization::Tr("te.table.gArmyNamesMoved.12"),
    localization::Tr("te.table.gArmyNamesMoved.13"),
    localization::Tr("te.table.gArmyNamesMoved.14"),
    localization::Tr("te.table.gArmyNamesMoved.15"),
    localization::Tr("te.table.gArmyNamesMoved.16"),
    localization::Tr("te.table.gArmyNamesMoved.17"),
    localization::Tr("te.table.gArmyNamesMoved.18"),
    localization::Tr("te.table.gArmyNamesMoved.19"),
    localization::Tr("te.table.gArmyNamesMoved.20"),
    localization::Tr("te.table.gArmyNamesMoved.21"),
    localization::Tr("te.table.gArmyNamesMoved.22"),
    localization::Tr("te.table.gArmyNamesMoved.23"),
    localization::Tr("te.table.gArmyNamesMoved.24"),
    localization::Tr("te.table.gArmyNamesMoved.25"),
    localization::Tr("te.table.gArmyNamesMoved.26"),
    localization::Tr("te.table.gArmyNamesMoved.27"),
};
char* gHeroNames[36][2] = {
    {localization::Tr("table.gHeroNames.0.0"), localization::Tr("table.gHeroNames.0.1")},
    {localization::Tr("table.gHeroNames.1.0"), localization::Tr("table.gHeroNames.1.1")},
    {localization::Tr("table.gHeroNames.2.0"), localization::Tr("table.gHeroNames.2.1")},
    {localization::Tr("table.gHeroNames.3.0"), localization::Tr("table.gHeroNames.3.1")},
    {localization::Tr("table.gHeroNames.4.0"), localization::Tr("table.gHeroNames.4.1")},
    {localization::Tr("table.gHeroNames.5.0"), localization::Tr("table.gHeroNames.5.1")},
    {localization::Tr("table.gHeroNames.6.0"), localization::Tr("table.gHeroNames.6.1")},
    {localization::Tr("table.gHeroNames.7.0"), localization::Tr("table.gHeroNames.7.1")},
    {localization::Tr("table.gHeroNames.8.0"), localization::Tr("table.gHeroNames.8.1")},
    {localization::Tr("table.gHeroNames.9.0"), localization::Tr("table.gHeroNames.9.1")},
    {localization::Tr("table.gHeroNames.10.0"), localization::Tr("table.gHeroNames.10.1")},
    {localization::Tr("table.gHeroNames.11.0"), localization::Tr("table.gHeroNames.11.1")},
    {localization::Tr("table.gHeroNames.12.0"), localization::Tr("table.gHeroNames.12.1")},
    {localization::Tr("table.gHeroNames.13.0"), localization::Tr("table.gHeroNames.13.1")},
    {localization::Tr("table.gHeroNames.14.0"), localization::Tr("table.gHeroNames.14.1")},
    {localization::Tr("table.gHeroNames.15.0"), localization::Tr("table.gHeroNames.15.1")},
    {localization::Tr("table.gHeroNames.16.0"), localization::Tr("table.gHeroNames.16.1")},
    {localization::Tr("table.gHeroNames.17.0"), localization::Tr("table.gHeroNames.17.1")},
    {localization::Tr("table.gHeroNames.18.0"), localization::Tr("table.gHeroNames.18.1")},
    {localization::Tr("table.gHeroNames.19.0"), localization::Tr("table.gHeroNames.19.1")},
    {localization::Tr("table.gHeroNames.20.0"), localization::Tr("table.gHeroNames.20.1")},
    {localization::Tr("table.gHeroNames.21.0"), localization::Tr("table.gHeroNames.21.1")},
    {localization::Tr("table.gHeroNames.22.0"), localization::Tr("table.gHeroNames.22.1")},
    {localization::Tr("table.gHeroNames.23.0"), localization::Tr("table.gHeroNames.23.1")},
    {localization::Tr("table.gHeroNames.24.0"), localization::Tr("table.gHeroNames.24.1")},
    {localization::Tr("table.gHeroNames.25.0"), localization::Tr("table.gHeroNames.25.1")},
    {localization::Tr("table.gHeroNames.26.0"), localization::Tr("table.gHeroNames.26.1")},
    {localization::Tr("table.gHeroNames.27.0"), localization::Tr("table.gHeroNames.27.1")},
    {localization::Tr("table.gHeroNames.28.0"), localization::Tr("table.gHeroNames.28.1")},
    {localization::Tr("table.gHeroNames.29.0"), localization::Tr("table.gHeroNames.29.1")},
    {localization::Tr("table.gHeroNames.30.0"), localization::Tr("table.gHeroNames.30.1")},
    {localization::Tr("table.gHeroNames.31.0"), localization::Tr("table.gHeroNames.31.1")},
    {localization::Tr("table.gHeroNames.32.0"), localization::Tr("table.gHeroNames.32.1")},
    {localization::Tr("table.gHeroNames.33.0"), localization::Tr("table.gHeroNames.33.1")},
    {localization::Tr("table.gHeroNames.34.0"), localization::Tr("table.gHeroNames.34.1")},
    {localization::Tr("table.gHeroNames.35.0"), localization::Tr("table.gHeroNames.35.1")},
};
char* gCPanelHelp[CPANEL_HELP_COUNT] = {
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
char* gNewGameHelp[NEW_GAME_HELP_COUNT] = {
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
char* gSetupCampaignGameHelp[SETUP_CAMPAIGN_HELP_COUNT] = {
    localization::Tr("table.gSetupCampaignGameHelp.0"),
    localization::Tr("table.gSetupCampaignGameHelp.1"),
    localization::Tr("table.gSetupCampaignGameHelp.2"),
    localization::Tr("table.gSetupCampaignGameHelp.3"),
    localization::Tr("table.gSetupCampaignGameHelp.4"),
};
char* gSetupBaudHelp[SETUP_BAUD_HELP_COUNT] = {
    localization::Tr("table.gSetupBaudHelp.0"),
    localization::Tr("table.gSetupBaudHelp.1"),
    localization::Tr("table.gSetupBaudHelp.2"),
    localization::Tr("table.gSetupBaudHelp.3"),
    localization::Tr("table.gSetupBaudHelp.4"),
};
char* gSetupComPortHelp[SETUP_COM_PORT_HELP_COUNT] = {
    localization::Tr("table.gSetupComPortHelp.0"),
    localization::Tr("table.gSetupComPortHelp.1"),
    localization::Tr("table.gSetupComPortHelp.2"),
    localization::Tr("table.gSetupComPortHelp.3"),
    localization::Tr("table.gSetupComPortHelp.4"),
};
char* gSetupDCBaudHelp[SETUP_BAUD_HELP_COUNT] = {
    localization::Tr("table.gSetupDCBaudHelp.0"),
    localization::Tr("table.gSetupDCBaudHelp.1"),
    localization::Tr("table.gSetupDCBaudHelp.2"),
    localization::Tr("table.gSetupDCBaudHelp.3"),
    localization::Tr("table.gSetupDCBaudHelp.4"),
};
char* gSetupDCComPortHelp[SETUP_COM_PORT_HELP_COUNT] = {
    localization::Tr("table.gSetupDCComPortHelp.0"),
    localization::Tr("table.gSetupDCComPortHelp.1"),
    localization::Tr("table.gSetupDCComPortHelp.2"),
    localization::Tr("table.gSetupDCComPortHelp.3"),
    localization::Tr("table.gSetupDCComPortHelp.4"),
};
char* gSetupHotSeatGameHelp[SETUP_HOT_SEAT_HELP_COUNT] = {
    localization::Tr("table.gSetupHotSeatGameHelp.0"),
    localization::Tr("table.gSetupHotSeatGameHelp.1"),
    localization::Tr("table.gSetupHotSeatGameHelp.2"),
    localization::Tr("table.gSetupHotSeatGameHelp.3"),
};
char* gSetupModemGameHelp[SETUP_MODEM_HELP_COUNT] = {
    localization::Tr("table.gSetupModemGameHelp.0"),
    localization::Tr("table.gSetupModemGameHelp.1"),
    localization::Tr("table.gSetupModemGameHelp.2"),
    localization::Tr("table.gSetupModemGameHelp.3"),
};
char* gSetupDCGameHelp[SETUP_MODEM_HELP_COUNT] = {
    localization::Tr("table.gSetupDCGameHelp.0"),
    localization::Tr("table.gSetupDCGameHelp.1"),
    localization::Tr("table.gSetupDCGameHelp.2"),
    localization::Tr("table.gSetupDCGameHelp.3"),
};
char* gSetupMultiPlayerGameHelp[SETUP_MULTIPLAYER_HELP_COUNT] = {
    localization::Tr("table.gSetupMultiPlayerGameHelp.0"),
    localization::Tr("table.gSetupMultiPlayerGameHelp.1"),
    localization::Tr("table.gSetupMultiPlayerGameHelp.2"),
    localization::Tr("table.gSetupMultiPlayerGameHelp.3"),
    localization::Tr("table.gSetupMultiPlayerGameHelp.4"),
};
char* gSetupNetworkGameHelp[SETUP_NETWORK_HELP_COUNT] = {
    localization::Tr("table.gSetupNetworkGameHelp.0"),
    localization::Tr("table.gSetupNetworkGameHelp.1"),
    localization::Tr("table.gSetupNetworkGameHelp.2"),
};
char* gSetupGameHelp[SETUP_GAME_HELP_COUNT] = {
    localization::Tr("table.gSetupGameHelp.0"),
    localization::Tr("table.gSetupGameHelp.1"),
    localization::Tr("table.gSetupGameHelp.2"),
    localization::Tr("table.gSetupGameHelp.3"),
};
char* gBattleResults[BATTLE_RESULT_COUNT] = {
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
char* gNeutralBuildingDescriptions[BUILDING_SLOT_NEUTRAL_COUNT] =
    {
        localization::Tr("table.gNeutralBuildingDescriptions.0"),
        localization::Tr("table.gNeutralBuildingDescriptions.1"),
        localization::Tr("table.gNeutralBuildingDescriptions.2"),
        localization::Tr("table.gNeutralBuildingDescriptions.3"),
        localization::Tr("table.gNeutralBuildingDescriptions.4"),
        localization::Tr("table.gNeutralBuildingDescriptions.5"),
        localization::Tr("table.gNeutralBuildingDescriptions.6"),
};
char* gMoraleInfoText[MORALE_INFO_COUNT] = {
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
char* gMapSizeNames[MAP_SIZE_COUNT] = {
    localization::Tr("table.gMapSizeNames.0"),
    localization::Tr("table.gMapSizeNames.1"),
    localization::Tr("table.gMapSizeNames.2"),
};
char* gMapDifficultyNames[MAP_DIFFICULTY_COUNT] = {
    localization::Tr("table.gMapDifficultyNames.0"),
    localization::Tr("table.gMapDifficultyNames.1"),
    localization::Tr("table.gMapDifficultyNames.2"),
    localization::Tr("table.gMapDifficultyNames.3"),
    localization::Tr("table.gMapDifficultyNames.4"),
};
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
char* gDifficultyNames[DIFFICULTY_COUNT] = {
    localization::Tr("table.gDifficultyNames.0"),
    localization::Tr("table.gDifficultyNames.1"),
    localization::Tr("table.gDifficultyNames.2"),
    localization::Tr("table.gDifficultyNames.3"),
};
char* gCampaignSideNames[4] = {
    localization::Tr("table.gCampaignSideNames.0"),
    localization::Tr("table.gCampaignSideNames.1"),
    localization::Tr("table.gCampaignSideNames.2"),
    localization::Tr("table.gCampaignSideNames.3")
};
char* gScoreLabels[CONGRATS_SCORE_LABEL_COUNT] = {
    localization::Tr("table.gScoreLabels.0"),
    localization::Tr("table.gScoreLabels.1"),
    localization::Tr("table.gScoreLabels.2"),
    localization::Tr("table.gScoreLabels.3"),
    localization::Tr("table.gScoreLabels.4"),
};
char* gHumanPlayerTypeNames[5] = {
    localization::Tr("table.gHumanPlayerTypeNames.0"),
    localization::Tr("table.gHumanPlayerTypeNames.1"),
    localization::Tr("table.gHumanPlayerTypeNames.2"),
    localization::Tr("table.gHumanPlayerTypeNames.3"),
    localization::Tr("table.gHumanPlayerTypeNames.4")
};
char* gHandicapNames[5] = {
    localization::Tr("table.gHandicapNames.0"),
    localization::Tr("table.gHandicapNames.1"),
    localization::Tr("table.gHandicapNames.2"),
    localization::Tr("table.gHandicapNames.3"),
    localization::Tr("table.gHandicapNames.4")
};
char* gMusicQualityText[3] = {
    localization::Tr("table.musicQualityText.0"),
    localization::Tr("table.musicQualityText.1"),
    localization::Tr("table.musicQualityText.2")
};
char* gWinSetupText[68] = {
    localization::Tr("table.gWinSetupText.0"),  localization::Tr("table.gWinSetupText.1"),
    localization::Tr("table.gWinSetupText.2"),  localization::Tr("table.gWinSetupText.3"),
    localization::Tr("table.gWinSetupText.4"),  localization::Tr("table.gWinSetupText.5"),
    localization::Tr("table.gWinSetupText.6"),  localization::Tr("table.gWinSetupText.7"),
    localization::Tr("table.gWinSetupText.8"),  localization::Tr("table.gWinSetupText.9"),
    localization::Tr("table.gWinSetupText.10"), localization::Tr("table.gWinSetupText.11"),
    localization::Tr("table.gWinSetupText.12"), localization::Tr("table.gWinSetupText.13"),
    localization::Tr("table.gWinSetupText.14"), localization::Tr("table.gWinSetupText.15"),
    localization::Tr("table.gWinSetupText.16"), localization::Tr("table.gWinSetupText.17"),
    localization::Tr("table.gWinSetupText.18"), localization::Tr("table.gWinSetupText.19"),
    localization::Tr("table.gWinSetupText.20"), localization::Tr("table.gWinSetupText.21"),
    localization::Tr("table.gWinSetupText.22"), localization::Tr("table.gWinSetupText.23"),
    localization::Tr("table.gWinSetupText.24"), localization::Tr("table.gWinSetupText.25"),
    localization::Tr("table.gWinSetupText.26"), localization::Tr("table.gWinSetupText.27"),
    localization::Tr("table.gWinSetupText.28"), localization::Tr("table.gWinSetupText.29"),
    localization::Tr("table.gWinSetupText.30"), localization::Tr("table.gWinSetupText.31"),
    localization::Tr("table.gWinSetupText.32"), localization::Tr("table.gWinSetupText.33"),
    localization::Tr("table.gWinSetupText.34"), localization::Tr("table.gWinSetupText.35"),
    localization::Tr("table.gWinSetupText.36"), localization::Tr("table.gWinSetupText.37"),
    localization::Tr("table.gWinSetupText.38"), localization::Tr("table.gWinSetupText.39"),
    localization::Tr("table.gWinSetupText.40"), localization::Tr("table.gWinSetupText.41"),
    localization::Tr("table.gWinSetupText.42"), localization::Tr("table.gWinSetupText.43"),
    localization::Tr("table.gWinSetupText.44"), localization::Tr("table.gWinSetupText.45"),
    localization::Tr("table.gWinSetupText.46"), localization::Tr("table.gWinSetupText.47"),
    localization::Tr("table.gWinSetupText.48"), localization::Tr("table.gWinSetupText.49"),
    localization::Tr("table.gWinSetupText.50"), localization::Tr("table.gWinSetupText.51"),
    localization::Tr("table.gWinSetupText.52"), localization::Tr("table.gWinSetupText.53"),
    localization::Tr("table.gWinSetupText.54"), localization::Tr("table.gWinSetupText.55"),
    localization::Tr("table.gWinSetupText.56"), localization::Tr("table.gWinSetupText.57"),
    localization::Tr("table.gWinSetupText.58"), localization::Tr("table.gWinSetupText.59"),
    localization::Tr("table.gWinSetupText.60"), localization::Tr("table.gWinSetupText.61"),
    localization::Tr("table.gWinSetupText.62"), localization::Tr("table.gWinSetupText.63"),
    localization::Tr("table.gWinSetupText.64"), localization::Tr("table.gWinSetupText.65"),
    localization::Tr("table.gWinSetupText.66"), localization::Tr("table.gWinSetupText.67"),
};
i32 gRequiredExtendedMemory = 4434;
i32 gRequiredConventionalMemory = 374;
b32 gFullCombatScreenDrawn = true;
i32 gForceSwitchMusic = FORCED_MUSIC_IDLE;
b32 gCurrArmyDrawn = true;
i8 gHighScoreRank = -1;
i32 gHighMemBuffer = 4000;
#include <SOURCE/combatTypes.h>
#include <SOURCE/mapCell.h>
#include <SOURCE/EVENTS.h>

b32 gHumanPlayer[4];
i32 gOldKBMark;
i32 gMaxExtentX;
i32 gMaxExtentY;
class font* gSmallFont;
i32 gBottomViewOverrideEndTime;
i8 gArmyEffected[COMBAT_SIDE_COUNT][ARMY_GROUP_SLOT_COUNT];
i32 gBottomViewResource;
b32 gSeedingValid;
i8 gLimitPlayer;
inputManager* gInputManager;
i32 gMaxMapExtra;
i32 gKBOldWord;
palette* gPalette;
resourceManager* gResourceManager;
u8 gMapExtra[MAP_CELL_GRID_SIZE][MAP_CELL_GRID_SIZE];
b32 gSpecialHideCursor;
class searchArray* gSearchArray;
b32 gBlackoutPlayer;
char gNetBoxLine[NET_BOX_SLOT_COUNT][60];
heroWindow* gDataEntryWindow;
i8 gWeekTypeExtra;
philAI* gPhilAI;
char* gDataEntryDest;
heroWindow* gNormalDialogWindow;
i32 gHostGamePos;
mouseManager* gMouseManager;
class font* gBigFont;
class icon* gSystemIcons;
b8 gCombatSurrender;
char gMapName[13];
i32 gMinExtentX;
i32 gMinExtentY;
i32 gSpareKBIntPos;
i8 gMapBaseType;
class hero* gInfoViewedHero;
i32 gHeroScreenSrcIndex;
i8 gWeekType;
char gText[768];
b32 gInNewGameSetup;
palette* gBufferPalette;
i8 gMonthTypeExtra;
i8 gMapExtendedType;
char gFullMapName[20];
i32 gShowIntro;
i32 gTimers[GLOBAL_TIMER_COUNT];
i32 gKBOldField;
i32 gScore;
armyGroup* gMonGroup;
configStruct gConfig;
i32 gOldKBMask;
char gRegAppPath[352];
i8 gCampaignChoice;
class game* gGame;
i32 gKBOldId;
b8 gRetreatWin;
i8 gWaitType;
i16 gCurLoadedSpellFileId;
i32 gBottomViewOverride;
char gLastFilename[FILE_REQUESTER_NAME_SIZE];
class icon* gBuyBuildIcons;
i8 gNoSound;
char gBottomViewText[92];
i32 gThisNetPos;
char gRegCDRomPath[352];
class heroWindow* gHeroScreenWindow;
class icon* gCurLoadedSpellIcon;
void* gMapExtraBlocks[255];
i32 gCurGeneral;
i32 gThisGamePos;
i32 gNumHumanPlayers;
b8 gIconClipOn;
i32 gMapExtraSizes[255];
i32 gDataEntryMaxLen;
class combatManager* gCombatManager;
i16 gSpellEffectFrame;
executive* gExec;
i8 gGroundToTerrain[140];
i32 gCurWindowsStyleFlags;
i16 gGameCommand;
i8 gMonthType;
char gMapDescription[124];
char* gDefaultAggregateName;
b8 gThisNetHumanPlayer[4];
char gAggPathName[352];
class highScoreManager* gHighScoreManager;
b8 gFunctionComplete;
u8 gKBFreeData[156];
i8 gIAmGreatest;
i16 gMapX;
i16 gMapY;
char gWinText[300];
i8 gDataEntryTime;
b32 gShowIt;
i32 gDebugLevel;
heroWindowManager* gWindowManager;
i32 gCurWatchPlayer;
i32 gBottomViewResourceQty;
b8 gWaitForRemoteReceive;
char gLastMapName[352];
i32 gExtraKBType;
townManager* gTownManager;
b8 gScreenScroll;
advManager* gAdvManager;
i8 gGamePosToNetPos[4];

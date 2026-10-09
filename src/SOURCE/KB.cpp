#include <H1/Ints.h>

#include <BASE/BITS.h>
#include <BASE/bmap2.h>
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
HMENU gDefaultMenu = NULL;
HMENU gCombatMenu = NULL;
HMENU gAdventureMenu = NULL;
HMENU gTownMenu = NULL;
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
    switch (SetupCDDrive()) {
        case CD_SETUP_NO_DRIVE:
            MessageBoxA(
                gAppWindow,
                "Нет доступа к CD приводу.",
                "Ошибка загрузки",
                MB_ICONHAND
            );
            exit(EXIT_SUCCESS);
            break;
        case CD_SETUP_NOT_FOUND:
            MessageBoxA(
                gAppWindow,
                "У вас должен быть компакт-диск Героев в CD приводе, чтобы играть\nв Героев Меча и Магии. \n\nВставьте диск и попробуйте еще раз.",
                "Ошибка загрузки",
                MB_ICONHAND
            );
            exit(EXIT_SUCCESS);
            break;
        case CD_SETUP_NO_APP_PATH:
            MessageBoxA(
                gAppWindow,
                "Невозможно изменить директорию. Пожалуйста, переустановите программу.",
                "Ошибка загрузки",
                MB_ICONHAND
            );
            exit(EXIT_SUCCESS);
            break;
        case CD_SETUP_NO_DATA:
            MessageBoxA(
                gAppWindow,
                "Невозможно найти файлы данных Героев.  Пожалуйста, переустановите программу.",
                "Ошибка загрузки",
                MB_ICONHAND
            );
            exit(EXIT_SUCCESS);
            break;
    }
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
    b32 sendResult;
    b8 creditsDone;
    b8 leave;
    i16 command;

    if (gKBDone)
        return 0;
    gKBDone = true;
    command = MAIN_MENU_NO_COMMAND;
    if ((gExec->InitSystem()))
        ShutDown("Не удалось инициализировать игру!");
    CheckMem();
    KBChangeMenu(gDefaultMenu);
    gPalette = gResourceManager->GetPalette("kb.pal");
    PostprocessPalette(gPalette->m_data);
    SetPalette(gPalette->m_data, true);
    gWindowManager->m_colorCycling = 1;
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
        textFont->DrawString("Загрузка Героев Меча и Магии (версия 1.1)", 10, 10, 1);
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
            gWindowManager->m_colorCycling = 1;
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
            case MAIN_MENU_HIGH_SCORES:
                if ((gExec->AddManager(gHighScoreManager, BASE_MANAGER_PRIORITY_UNASSIGNED)))
                    ShutDown("Не могу добавить менеджера!");
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
                netIndex = NET_POSITION_HOST;
                for (gamePlayer = 0; gamePlayer < GAME_PLAYER_COUNT; gamePlayer++) {
                    if (gHumanPlayer[gamePlayer]) {
                        gGamePosToNetPos[gamePlayer] = netIndex;
                        netIndex++;
                    } else {
                        gGamePosToNetPos[gamePlayer] = NET_POSITION_NONE;
                    }
                }
                for (gamePlayer = 0; gamePlayer < GAME_PLAYER_COUNT; gamePlayer++)
                    memcpy(gText, gGamePosToNetPos, GAME_PLAYER_COUNT);
                gHostGamePos = NetPosToGamePos(NET_POSITION_HOST);
                gThisGamePos = gHostGamePos;
                for (gamePlayer = NET_POSITION_FIRST_GUEST; gamePlayer < gNumHumanPlayers;
                     gamePlayer++) {
                    sendResult = TransmitRemoteData(
                        gText,
                        gamePlayer,
                        GAME_PLAYER_COUNT,
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
                        if (!gGame->TransmitSaveGame(gamePlayer, false))
                            ShutDown(NULL);
                    }
                }
            }
            if (gRemoteOn && gWaitForRemoteReceive) {
                gWaitType = DIALOG_WAIT_OTHER_PLAYER;
                NormalDialog(
                    "Ожидаю других игроков для установки игры.",
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
                ShutDown("Не могу добавить менеджера!");
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
            sprintf(gWinText, "Мои герои! Наши враги были разбиты, а их замки преданы разорению. Великий поход окончен, и я предстаю перед вами как всеми признанный Великий Король!\n\nМы достигли  победы за %d дней!", gCurTurn);
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
                gResourceManager->GetBackdrop("heroes.bmp", gWindowManager->m_screen);
                gWindowManager
                    ->UpdateScreenRegion(0, 0, LOGICAL_SCREEN_WIDTH, LOGICAL_SCREEN_HEIGHT);
                gWindowManager->FadeScreen(WINDOW_FADE_IN, WINDOW_FADE_SHORT, gPalette);
                gWindowManager->m_colorCycling = 1;
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
                    ShutDown("Не могу добавить менеджера!");
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
                    sprintf(gText, "Вы хотите переиграть этот сценарий?");
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
                        "НОВЫЙ СЦЕНАРИЙ",
                        gGame->m_campaignScenariosWon
                    );
                    gGame->SaveGame(saveBuf, true);
                    sprintf(gText, "Ваша кампания была сохранена как %s. Желаете начать новый сценарий?", saveBuf);
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

    gDebugLevel = DEBUG_LEVEL_NONE;
    gShowIntro = 1;
    gColorMice = 0;
    gSpecialMouseMasks = 1;
    gScreenScroll = true;
    gLimitPlayer = 0;
    gBlackoutPlayer = true;
    strcpy(gMapName, "AES31000.map");
    strcpy(gFullMapName, "Когти (Легкая)");
    strcpy(gMapDescription, "Грифоны будут защищать вас пока вы не будете готовы сделать ваш ход.");

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
                        gTownManager->m_buildingWindow->DrawWindow();
                        gTownManager->m_childWindow->DrawWindow();
                        gWindowManager->FadeScreen(WINDOW_FADE_IN, WINDOW_FADE_SHORT, NULL);
                        break;
                    default:
                        break;
                }
                break;
            case WIDGET_NOTIFY_DESELECT:
                switch (message.id) {
                    case DIALOG_BUTTON_1:
                        gTownManager->m_recruitSlot = RECRUIT_HERO_NONE;
                        shouldClose = true;
                        break;
                    case recruitButton1:
                    case recruitButton2:
                        gTownManager->m_recruitSlot = message.id - recruitButton1;
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
        && townPointer->m_mageGuildLevel >= TOWN_MAGE_GUILD_COST_LEVEL_LAST)
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
            ? (townPointer->m_mageGuildLevel < TOWN_MAGE_GUILD_COST_LEVEL_LAST
                   ? townPointer->m_mageGuildLevel + 1
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
                    "%d/день",
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
        labelWidget = new textWidget(
            imageCenterX - 50,
            resourceIconY + imageHeight - 10,
            100,
            12,
            labelTexts[index],
            "smalfont.fnt",
            TEXT_WIDGET_PLAIN_COLOR,
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
        orWord = static_cast<char*>(malloc(strlen("или") + 1));
        strcpy(orWord, "или");
        labelWidget = new textWidget(
            width / 2 - 17,
            resourceIconY + 30,
            40,
            12,
            orWord,
            "smalfont.fnt",
            TEXT_WIDGET_PLAIN_COLOR,
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
        gWindowManager->AddWindow(gNormalDialogWindow, WINDOW_Z_ORDER_APPEND, true);
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
        if (gGame->m_heroOwners[currentPlayer->m_availableHeroIds[i]]
            == HERO_AVAILABILITY_RETREATED)
            gGame->m_heroOwners[currentPlayer->m_availableHeroIds[i]] =
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
    "redfire.icn", "elecfire.icn", "magic04.icn", "magic01.icn", "magic01.icn",  "magic02.icn",
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
i8 gCombatAdjacency[COMBAT_HEX_COUNT][COMBAT_DIRECTION_ADJACENT_COUNT] = {
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
i8 gDwellingType[TOWN_TYPE_COUNT][BUILDING_SLOT_DWELLING_COUNT] = {
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
        if (!gGame->TransmitSaveGame(REMOTE_BROADCAST_PLAYER, true))
            ShutDown(NULL);
        RemoteCleanup();
    } else if (gNumHumanPlayers == REMOTE_PLAYER_COUNT) {
        gNumHumanPlayers--;
        gText[REMOTE_PLAYER_EXIT_POSITION] = position;
        gText[REMOTE_PLAYER_EXIT_HAD_CONTROL] = 0;
        TransmitRemoteData(
            gText,
            REMOTE_BROADCAST_PLAYER,
            REMOTE_PLAYER_EXIT_PAYLOAD_SIZE,
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
    if (!gGameInitialized)
        return;
    gText[REMOTE_PLAYER_EXIT_POSITION] = gThisGamePos;
    if (gThisNetHumanPlayer[gCurPlayer]
        || (!gHumanPlayer[gCurPlayer] && gThisGamePos == gHostGamePos)) {
        gText[REMOTE_PLAYER_EXIT_HAD_CONTROL] = 1;
        next = gCurPlayer;
        next = (next + 1) % gGame->m_playerCount;
        while (!gHumanPlayer[next])
            next = (next + 1) % gGame->m_playerCount;
        gText[REMOTE_PLAYER_EXIT_NEXT_PLAYER] = next;
    } else {
        gText[REMOTE_PLAYER_EXIT_HAD_CONTROL] = 0;
    }
    TransmitRemoteData(
        gText,
        REMOTE_BROADCAST_PLAYER,
        REMOTE_PLAYER_EXIT_PAYLOAD_SIZE,
        REMOTE_COMMAND_PLAYER_EXIT,
        false,
        false,
        REMOTE_MESSAGE_RELIABLE
    );
}

void ReceiveRemotePlayerExit(i8 position, i8 hadControl, b8 eliminated, b8 timedOut) {
    if (position == gThisGamePos) {
        sprintf(gText, "Вы были исключены из игры!");
        NormalDialog(gText, NORMAL_DIALOG_TYPE_OK);
        RemoteCleanup();
        gGameOver = true;
        gEndSequence = GAME_END_LOST;
        return;
    }
    if (gNumHumanPlayers <= REMOTE_PLAYER_COUNT) {
        gGame->SaveGame("ИГРОК ВЫШЕЛ", true);
        if (eliminated) {
            sprintf(
                gText,
                "%s игрок разгромлен!",
                gColorNames[gGame->m_players[position].Color()]
            );
            gText[0] = CyrillicToUpper(gText[0]);
            NormalDialog(
                gText,
                NORMAL_DIALOG_TYPE_OK,
                NORMAL_DIALOG_ADVENTURE_X,
                NORMAL_DIALOG_AUTO_POSITION,
                NORMAL_DIALOG_CREST,
                (gGame->m_players[position].Color())
            );
            goto dropPlayer;
        } else {
            if (timedOut)
                sprintf(
                    gText,
                    "%d игрок был выкинут из игры. Данная игра была сохранена как 'ИГРОК ВЫШЕЛ'. Желаете ли вы продолжить игру и заместить выбывшего %d игрока компьютером?",
                    position + 1,
                    position + 1
                );
            else
                sprintf(
                    gText,
                    "Игрок %d покинул игру. Данная игра была сохранена как 'ИГРОК ВЫШЕЛ'. Вы желаете продолжить игру с компьютерным игроком, занявшим место выбывшего %d игрока?",
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
                    "%s игрок разгромлен!",
                    gColorNames[gGame->m_players[static_cast<i8>(playerIndex)].Color()]
                );
                gText[0] = CyrillicToUpper(gText[0]);
                NormalDialog(
                    gText,
                    NORMAL_DIALOG_TYPE_OK,
                    NORMAL_DIALOG_ADVENTURE_X,
                    NORMAL_DIALOG_AUTO_POSITION,
                    NORMAL_DIALOG_CREST,
                    (gGame->m_players[static_cast<i8>(playerIndex)].Color())
                );
            } else if (!curPlayer->m_townCount) {
                if (curPlayer->m_daysLeft == END_GAME_NO_GRACE_PERIOD) {
                    if (gThisNetHumanPlayer[playerIndex]) {
                        sprintf(
                            gText,
                            "%s игрок, вы потеряли ваш последний город. Если вы не захватите новый город до следующей недели, вы будете уничтожены.",
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
                            "%s игрок, ваши герои покинули вас, а вы были изгнаны из этих земель.",
                            gColorNames[gGame->m_players[static_cast<i8>(playerIndex)].Color()]
                        );
                        gText[0] = CyrillicToUpper(gText[0]);
                    } else {
                        sprintf(
                            gText,
                            "%s игрок был брошен своими героями, а сам изгнан из этих земель.",
                            gColorNames[gGame->m_players[static_cast<i8>(playerIndex)].Color()]
                        );
                        gText[0] = CyrillicToUpper(gText[0]);
                    }
                    NormalDialog(
                        gText,
                        NORMAL_DIALOG_TYPE_OK,
                        NORMAL_DIALOG_ADVENTURE_X,
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
                    strcpy(message, "Враг захватил город XX!!");
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
                    strcpy(message, "Враг захватил Могущественный артефакт!");
                }
                break;
            case CAMPAIGN_SCENARIO_DRAGON_CITY:
                defaultWin = false;
                if (!gGame->m_mineOwners[MINE_SLOT_DRAGON_CITY])
                    won = true;
                if (gGame->m_mineOwners[MINE_SLOT_DRAGON_CITY] > 0) {
                    defeated = true;
                    strcpy(message, "Враг захватил Драконий город!");
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
    gDefaultMenu = LoadMenuA(gAppInstance, "mnuDflt");
    gCombatMenu = LoadMenuA(gAppInstance, "mnuCmbt");
    gAdventureMenu = LoadMenuA(gAppInstance, "mnuAdv");
    gTownMenu = LoadMenuA(gAppInstance, "mnuTown");
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
    if (heroPointer->HasArtifact(ARTIFACT_FIZBIN_OF_MISFORTUNE))
        strcat(gText, gMoraleInfoText[MORALE_INFO_FIZBIN]);
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
    i32 file;
    char scoreFile[352];
    char enteredPlayerName[20];
    b8 noScoreFile;

    noScoreFile = false;
    if (highScoreType == HIGH_SCORE_TYPE_STANDARD)
        sprintf(scoreFile, "%sSTANDARD.HS", gDataPath);
    else
        sprintf(scoreFile, "%sCAMPAIGN.HS", gDataPath);
    file = open(scoreFile, _O_BINARY);
    if (file == FILE_DESCRIPTOR_INVALID)
        noScoreFile = true;
    if (noScoreFile) {
        for (entry = 0; entry < HIGH_SCORE_DISPLAY_ENTRY_COUNT; entry++) {
            memset(&entries[entry], 0, sizeof(HighScoreEntry));
            entries[entry].score = HIGH_SCORE_EMPTY;
        }
    } else {
        for (entry = 0; entry < HIGH_SCORE_DISPLAY_ENTRY_COUNT; entry++)
            read(file, &entries[entry], sizeof(entries));
        close(file);
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
        GetDataEntry("Наберите ваше имя русскими буквами.", enteredPlayerName, 16, NULL);
        strcpy(entries[entry].playerName, enteredPlayerName);
        strcpy(entries[entry].scenarioName, scenarioName);
        entries[entry].score = score;
        file = open(scoreFile, _O_BINARY | _O_TRUNC | _O_CREAT | _O_WRONLY, _S_IWRITE);
        if (file == FILE_DESCRIPTOR_INVALID)
            FileError(scoreFile);
        for (entry = 0; entry < HIGH_SCORE_DISPLAY_ENTRY_COUNT; entry++)
            WRITE_FILE_VALUE(file, entries[entry]);
        close(file);
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
        switch (received->command) {
            case BOX_REMOTE_SETUP:
                memcpy(gGamePosToNetPos, received->payload.data, GAME_PLAYER_COUNT);
                gThisGamePos = NetPosToGamePos(gThisNetPos);
                gHostGamePos = NetPosToGamePos(NET_POSITION_HOST);
                break;
            case BOX_REMOTE_SAVE:
                result = gGame->ReceiveSaveGame(received->payload.saveSize, received->sender);
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
    b32 sendOk;
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
    gWindowManager->AddWindow(netBox, WINDOW_Z_ORDER_APPEND, true);
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
                        AddNetBoxLine("[ Входящая информация, необходимо закрыть... ]");
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
    strcpy(gNetBoxLine[NET_BOX_SLOT_PREVIOUS], gNetBoxLine[NET_BOX_SLOT_LATEST]);
    strcpy(gNetBoxLine[NET_BOX_SLOT_LATEST], text);
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
        MessageBoxA(gAppWindow, buffer, "Неожиданное прерывание программы", MB_ICONHAND);
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
    if (gEventHandle) {
        CloseHandle(gEventHandle);
        gEventHandle = NULL;
    }
    DeleteMainClasses();
    AppExit();
    exit(EXIT_SUCCESS);
}

void FileError(char* filename) {
    char message[200];
    sprintf(message, "Ошибка открытия файла %s!", filename);
    ShutDown(message);
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
        sprintf(gText, "Великая победа!");
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
    gWindowManager->AddWindow(window, WINDOW_Z_ORDER_APPEND, true);
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
    sprintf(gText, "A:\\");
    gText[0] += driveIndex;
    return GetDriveTypeA(gText) == DRIVE_CDROM;
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
            strcpy(gText, "Вы действительно хотите начать сначала?  (Данная игра будет потеряна).");
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
            strcpy(gText, "Загрузить новую игру? (Данная игра будет потеряна).");
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
            PostMessage(gAppWindow, WM_CLOSE, 0, 0);
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
        CheckMenuItem(gAppMenu, menuCommand, MF_UNCHECKED);
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
    CheckMenuItem(gAppMenu, checkedCommand, MF_CHECKED);

    for (menuCommand = APP_MENU_SOUND_FIRST; menuCommand <= APP_MENU_SOUND_LAST; menuCommand++)
        CheckMenuItem(gAppMenu, menuCommand, MF_UNCHECKED);
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
    CheckMenuItem(gAppMenu, checkedCommand, MF_CHECKED);

    for (menuCommand = APP_MENU_SPEED_FIRST; menuCommand <= APP_MENU_SPEED_LAST; menuCommand++)
        CheckMenuItem(gAppMenu, menuCommand, MF_UNCHECKED);
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
    CheckMenuItem(gAppMenu, checkedCommand, MF_CHECKED);
    CheckMenuItem(
        gAppMenu,
        APP_MENU_CD_STEREO,
        gConfig.musicSource ? MF_CHECKED : MF_UNCHECKED
    );
    CheckMenuItem(gAppMenu, APP_MENU_SHOW_PATH, gConfig.showRoute ? MF_CHECKED : MF_UNCHECKED);
    CheckMenuItem(
        gAppMenu,
        APP_MENU_VIEW_ENEMY_MOVES,
        1 - gConfig.blackoutComputer ? MF_CHECKED : MF_UNCHECKED
    );
}

void CleanUpMenus(void) {
    if (gAppMenu) {
        SetMenu(gAppWindow, NULL);
        if (gAdventureMenu)
            DestroyMenu(gAdventureMenu);
        if (gDefaultMenu)
            DestroyMenu(gDefaultMenu);
        if (gCombatMenu)
            DestroyMenu(gCombatMenu);
        if (gTownMenu)
            DestroyMenu(gTownMenu);
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
i16 gMinExpForLevel[HERO_CLASS_COUNT][HERO_EXPERIENCE_LEVEL_TABLE_COUNT] = {
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
    {false,
     36,
     35,
     "   Предвратье   ",
     {0, 1, 1, 1},
     {4, 4, 4},
     {{30, 10, 30, 10, 10, 10, 10000},
      {30, 10, 30, 10, 10, 10, 10000},
      {30, 10, 30, 10, 10, 10, 10000},
      {30, 10, 30, 10, 10, 10, 10000}}},
    {false,
     -1,
     -1,
     {' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' '},
     {0, 2, 2, 2},
     {4, 4, 4},
     {{30, 10, 30, 10, 10, 10, 10000},
      {30, 10, 30, 10, 10, 10, 10000},
      {30, 10, 30, 10, 10, 10, 10000},
      {30, 10, 30, 10, 10, 10, 10000}}},
    {false,
     -1,
     -1,
     {' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' '},
     {0, 2, 2, 2},
     {4, 4, 4},
     {{30, 10, 30, 10, 10, 10, 10000},
      {30, 10, 30, 10, 10, 10, 10000},
      {30, 10, 30, 10, 10, 10, 10000},
      {30, 10, 30, 10, 10, 10, 10000}}},
    {false,
     -1,
     -1,
     {' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' '},
     {0, 2, 2, 2},
     {4, 4, 4},
     {{30, 10, 30, 10, 10, 10, 10000},
      {30, 10, 30, 10, 10, 10, 10000},
      {30, 10, 30, 10, 10, 10, 10000},
      {30, 10, 30, 10, 10, 10, 10000}}},
    {false,
     13,
     10,
     "Замок Айронфиста",
     {0, 3, 0, 0},
     {2, 4, 4},
     {{30, 10, 30, 10, 10, 10, 10000},
      {30, 10, 30, 10, 10, 10, 10000},
      {30, 10, 30, 10, 10, 10, 10000},
      {30, 10, 30, 10, 10, 10, 10000}}},
    {false,
     62,
     20,
     " Замок Слэйера  ",
     {0, 3, 0, 0},
     {1, 4, 4},
     {{30, 10, 30, 10, 10, 10, 10000},
      {30, 10, 30, 10, 10, 10, 10000},
      {30, 10, 30, 10, 10, 10, 10000},
      {30, 10, 30, 10, 10, 10, 10000}}},
    {false,
     8,
     8,
     " Замок Ламанды  ",
     {0, 3, 0, 0},
     {3, 4, 4},
     {{30, 10, 30, 10, 10, 10, 10000},
      {30, 10, 30, 10, 10, 10, 10000},
      {30, 10, 30, 10, 10, 10, 10000},
      {30, 10, 30, 10, 10, 10, 10000}}},
    {false,
     66,
     69,
     " Замок Аламара  ",
     {0, 3, 0, 0},
     {0, 4, 4},
     {{30, 10, 30, 10, 10, 10, 10000},
      {30, 10, 30, 10, 10, 10, 10000},
      {30, 10, 30, 10, 10, 10, 10000},
      {30, 10, 30, 10, 10, 10, 10000}}},
    {true,
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
i8 gHeroSkillBonus[HERO_CLASS_COUNT][HERO_SKILL_BONUS_ROW_LAST + 1][HERO_PRIMARY_STAT_COUNT] = {
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
SMenuEnableStatus gMenuEnableStatus[KBWIN_MENU_ENTRY_COUNT] = {
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
    "Книга всезнания",  "Меч власти",
    "Защитная накидка",  "Жезл магии",
    "Ожерелье тайной магии",  "Магический браслет",
    "Кольцо мага",  "Брошь ведьмы",
    "Медаль отваги",  "Медаль мужества",
    "Медаль доблести", "Медаль почета",
    "Символ неудачи", "Громовая алица ",
    "Защитная перчатка", "Шлем защитника",
    "Гигантский цеп", "Баллиста",
    "Незримый щит", "Драконий меч",
    "Топор власти", "Божественный доспех",
    "Малый свиток знания", "Большой свиток знания",
    "Могущественный свиток знания", "Свиток высшего знания",
    "Бездонный мешок", "Бездонная сума",
    "Бездонный кошель", "Башмаки кочевника",
    "Башмаки путника", "Лапка кролика",
    "Золотая подкова", "Счастливая монета",
    "Клевер", "Компас",
    "Астролябия", "Волшебная книга",
};
char* gArtifactDesc[ARTIFACT_COUNT] = {
    "Книга всезнания\n(Знания +12)\n\nКнига всезнания увеличивает Знания на 12 единиц.",  "Меч власти\n(Атака +12)\n\nМеч власти увеличивает навык атаки на 12 единиц.",
    "Защитная накидка\n(Защита +12)\n\nЗащитная накидка увеличивает защиту на 12 единиц.",  "Жезл магии\n(Сила магии +12)\n\nЖезл магии увеличивает силу заклинаний на 12 единиц.",
    "Ожерелье тайной магии\n(Сила магии +4)\n\nОжерелье тайной магии увеличивает силу магии на 4 единицы.",  "Магический браслет\n(Сила магии +2)\n\nМагический браслет увеличивает силу заклинаний на 2 единицы.",
    "Кольцо мага\n(Сила магии +2)\n\nКольцо мага увеличивает силу магии на 2 единицы.",  "Брошь ведьмы\n(Сила магии +3)\n\nБрошь ведьмы увеличивает силу магии на 3 единицы.",
    "Медаль отваги\n\nМедаль отваги увеличивает мораль.",  "Медаль мужества\n\nМедаль мужества увеличивает мораль.",
    "Медаль доблести\n\nМедаль доблести увеличивает мораль.", "Медаль почета\n\nМедаль почета увеличивает мораль.",
    "Символ неудачи\n\nСимвол неудачи сильно уменьшает мораль.", "Громовая палица\n(Атака +1)\n\nГромовая палица увеличивает навык атаки на 1 единицу.",
    "Защитная перчатка\n(Защита +1)\n\nЗащитная перчатка увеличивает навык защиты на 1 единицу.", "Шлем защитника\n(Защита +1)\n\nШлем защитника увеличивает навык защиты на 1 единицу.",
    "Гигантский цеп\n(Атака +1)\n\nГигантский цеп увеличивает навык атаки на 1 единицу.", "Баллиста\n\nБаллиста позволяет вашей катапульте дважды стрелять в один ход боя.",
    "Незримый щит\n(Защита +2)\n\nНезримый щит увеличивает навык защиты на 2 единицы.", "Драконий меч\n(Атака +3)\n\nДраконий меч увеличивает навык атаки на 3 единицы.",
    "Топор власти\n(Атака +2)\n\nТопор власти увеличивает навык атаки на 2 единицы.", "Божественный доспех\n(Защита +3)\n\nБожественный доспех увеличивает навык защиты на 3 единицы.",
    "Малый свиток знания\n(Знания +2)\n\nМалый свиток знания увеличивает Знания на 2 единицы.", "Большой свиток знания\n(Знания +3)\n\nБольшой свиток знания увеличивает Знания на 3 единицы.",
    "Могущественный свиток знания\n(Знания +4)\n\nМогущественный свиток знания увеличивает Знания на 4 единицы.", "Свиток высшего знания\n(Знания +5)\n\nСвиток высшего знания увеличивает Знания на 5 единиц.",
    "Бездонный мешок\n\nБездонный мешок приносит вам 1000 золотых в день.", "Бездонная сума\n\nБездонная сума приносит вам 750 золотых в день.",
    "Бездонный кошель\n\nБездонный кошель приносит вам 500 золотых в день.", "Башмаки кочевника\n\nБашмаки кочевника увеличивают дальность передвижения по суше.",
    "Башмаки путника\n\nБашмаки путника увеличивают подвижность отряда на суше.", "Лапка кролика\n\nЛапка кролика увеличивает удачу в бою.",
    "Золотая подкова\n\nЗолотая подкова увеличивает удачу в бою.", "Монета\n\nСчастливая монета увеличивает удачу в бою.",
    "Клевер\n\nКлевер увеличивает удачу в бою.", "Компас\n\nКомпас увеличивает подвижность отряда на суше и на море.",
    "Астролябия\n\nАстролябия увеличивает подвижность отряда на море.", "Волшебная книга\n\nВолшебная книга позволяет направлять заклинания.",
};
char* gArtifactEvent[38] = {
    "",
    "",
    "",
    "",
    "Вы вызволяете волшебницу, заточенную в проклятой гробнице, и в награду она вручает вам изысканное алмазное ожерелье.",
    "Изучая завалы в заброшенной шахте, вы спасаете артель гномов-старателей. В знак благодарности их старшина дарит вам золотой браслет.",
    "Вы спешите на звук отчаянного вопля боли и видите кентавра, попавшего в западню. Вы помогаете ему освободиться, и он вручает вам кожаный мешочек. Заглянув внутрь, вы видите ослепительное бриллиантовое кольцо.",
    "Рядом с останками сожженной колдуньи лежит изящная брошь прекрасной работы. Осторожно приблизившись к обугленному трупу, вы забираете брошь себе.",
    "В награду за спасение прекрасной девы от посягательств ненавистного барона королевский герольд вручает вам Медаль отваги.",
    "Вы спасаете маленького мальчика от стаи кровожадных волков и провожаете в имение родителей. Счастливый отец награждает вас Медалью мужества.",
    "Вы вырываете принцессу соседнего королевства из мерзких лап презренных работорговцев и в награду за подвиг получаете Медаль доблести.",
    "Вы избавляете округу от ужасного минотавра, добычей которому служили благородные рыцари, и становитесь кавалером Медали почета.",
    "На обочине пустынной дороги вы находите медаль. Вы подобрали ее и обнаружили, что стали несчастным обладателем Символа неудачи, который понижает мораль вашей армии.",
    "Во время жуткой грозы молния бьет в дерево, разнося его на мелкие щепки. Среди обломков вы обнаруживаете таинственную палицу.",
    "Вы повстречали печально известного Черного Рыцаря! Ваш поединок заканчивается вничью, и рыцарь в знак уважения дарит вам пару латных перчаток.",
    "Краем глаза вы замечаете золотистый блеск среди пышной зелени. Приглядевшись внимательнее, вы находите под кустами великолепный золотой шлем.",
    "Неуклюжий гигант нанес себе смертельную рану собственным боевым цепом. Вы прекрасно владеете этим оружием и с уверенностью вынимаете цеп из мертвых рук гиганта.",
    "Пробираясь через развалины древней крепости, вы находите орудие, которое превратило ее в руины, удивительную баллисту замысловатой конструкции.",
    "В руках у каменной статуи воина - великолепный серебряный щит. Как только вы забираете щит себе, статуя рассыпается в прах.",
    "Вы пробираетесь узкой тропой, как вдруг ближайший куст загорается ярким пламенем. Постепенно в огненном смерче проступают очертания прекрасной дамы, которая протягивает вам меч чудесной работы.",
    "Вы видите серебряный топор, вогнанный в землю по самую рукоять. Ваши воины пытаются выдернуть его, но усилия их тщетны. Вам же хватило одного усилия и топор у вас в руках!",
    "Шайка разбойников обыскивает тела мертвых воинов. Вы разгоняете мародеров и вдруг замечаете, что в спешке они потеряли великолепный доспех.",
    " Перед вами возникает парящий в воздухе стеклянный ларец со свитком внутри, лежащем на подушке из пурпурного бархата. Повинуясь вашему прикосновению, крышка ларца открывается, и свиток оказывается у вас в руках.",
    "Вы навещаете местного мудреца и рассказываете о цели вашего путешествия. Он достает из мешка пожелтевший свиток и передает его вам.",
    "Вы стоите перед останками давно умершей жрицы друидов. Пожелтевшие от времени кости проглядывают через прорехи истлевшего одеяния. Пошевелив груду ветоши, вы находите древний свиток.",
    "Груда пожелтевших костей и обрывки истлевшей материи - вот все, что осталось от жрицы друидов. Среди этих останков вы замечаете таинственный свиток.",
    "Маленький лепрекон пританцовывает у волшебного мешка. Завидев вас, он замирает на месте, затем издает возмущенный возглас, топает ножкой и растворяется в воздухе. Следуя старинной поговорке, \"Кто успел - тот и съел\", вы забираете мешок себе.",
    "Благородная путешественница, отбившаяся от спутников, просит вас о помощи. Проводив ее до дома, вы получаете в награду суму, полную золота.",
    "Однажды вам в руки попадает наполненный золотом кожаный кошель, принадлежавший великому королю, который умел превращать любой предмет в золото.",
    "Бродячий торговец просит вас защитить его от банды гоблинов. В награду он дарит вам пару изящных башмаков, испещеренных загадочными древними письменами.",
    "Обнаружив пару замечательных башмаков украшенных бисером, вы благодарите загадочного благодетеля и оставляете их себе.",
    "В уплату за охрану в пути странствующий торговец предлагает вам лапку кролика. По его словам, она принесет вам удачу в бою.",
    "Попавший в ловушку единорог испуганно кричит. Вы успокаиваете его и освобождаете от пут. Всхрапнув и ударив копытом, он уносится прочь. Там, где он только что стоял, осталась лежать золотая подкова.",
    "Вы поймали озорного бесенка, который не давал покоя всей округе. В обмен на свободу он предлагает вам волшебную монету.",
    "Посреди мертвой лощины, заполненной иссохшей растительностью, вы, к своему удивлению, замечете веселый зеленый побег четырехлистного клевера.",
    "Странноватый старикашка утверждает, что он - великий изобретатель, и просит вас испытать его новое творение. Надувшись от важности, он вручает вам компас.",
    "Старый мореход стал добычей людоедов. Вы спасаете его, и в знак благодарности он дарит вам чудесный инструмент, позволяющий измерять расстояния по звездам.",
    "Волшебная книга?!",
};
char* gStatNames[5] = {
    "Атака",
    "Защита",
    "Сила магии",
    "Знания",
    "Осада"
};
char* gStatDesc[5] = {
    "Ваш навык атаки - бонус, добавляемый к навыку атаки каждого воина.",
    "Ваш навык защиты - бонус, добавляемый к навыку защиты каждого воина.",
    "Ваш уровень силы магии определяет длительность действия или силу заклинания.",
    "Ваш уровень знания определяет количество запоминаемых заклинаний.",
    "Ваш уровень навыка осадного дела определяет, сколько раз будет стрелять ваша катапульта при осаде замка.",
};
char* gClassNames[HERO_CLASS_COUNT] = {
    "Рыцарь",
    "Варвар",
    "Колдунья",
    "Чернокнижник"
};
char* gArmyNames[CREATURE_COUNT] = {
    "Крестьянин",  "Лучник",
    "Копейщик",  "Мечник",
    "Всадник",  "Паладин",
    "Гоблин",  "Орк",
    "Волк",  "Огр",
    "Тролль", "Циклоп",
    "Фея", "Гном",
    "Эльф", "Друид",
    "Единорог", "Феникс",
    "Кентавр", "Горгулья",
    "Грифон", "Минотавр",
    "Гидра", "Дракон",
    "Разбойник", "Кочевник",
    "Призрак", "Джинн",
};
char* gArmySpriteNames[CREATURE_COUNT] = {
    "peasant",  "archer", "pikeman", "swordsman", "cavalry", "paladin",  "goblin",
    "orc",      "wolf",   "ogre",    "troll",     "cyclops", "sprite",   "dwarf",
    "elf",      "druid",  "unicorn", "phoenix",   "centaur", "gargoyle", "griffin",
    "minotaur", "hydra",  "dragon",  "rogue",     "nomad",   "ghost",    "genie"
};
char* gArmyNamesPlural[CREATURE_COUNT] = {
    "крестьян",  "лучников",
    "копейщиков",  "мечников",
    "всадников",  "паладинов",
    "гоблинов",  "орков",
    "волков",  "огров",
    "троллей", "циклопов",
    "фей", "гномов",
    "эльфов", "друидов",
    "единорогов", "фениксов",
    "кентавров", "горгулий",
    "грифонов", "минотавров",
    "гидр", "драконов",
    "разбойников", "кочевников",
    "призраков", "джиннов",
};
char* gSpellNames[SPELL_COUNT] = {
    "Огненный шар",  "Молния",
    "Телепортация",  "Лечение",
    "Воскрешение",  "Ускорение",
    "Замедление",  "Ослепление",
    "Благословение",  "Защита",
    "Проклятие", "Убить нежить",
    "Антимагия", "Снять чары",
    "Берсерк", "Армагеддон",
    "Шторм", "Звездопад",
    "Паралич", "Шахты",
    "Ресурсы", "Артефакты",
    "Города", "Герои",
    "Все", "Опознать героя",
    "Корабль", "Портал",
    "Врата города",
};
char* gNeutralBuildingNames[BUILDING_SLOT_NEUTRAL_COUNT] = {
    "Гильдия магов",
    "Гильдия воров",
    "Таверна",
    "Верфь",
    "Колодец",
    "Шатры",
    "Замок"
};
char* gDwellingNames[24] = {
    "Мазанка",  "Стрельбище",
    "Кузница",  "Оружейная",
    "Ристалище",  "Собор",
    "Древо-дом",  "Избушка",
    "Стрельбище",  "Стоунхендж",
    "Загон", "Алая башня",
    "Хижина", "Хибара",
    "Логово", "Дом огров",
    "Мост", "Пирамида",
    "Пещера", "Крипта",
    "Гнездо", "Лабиринт",
    "Болото", "Черная башня",
};
char* gTerrainNames[TERRAIN_COUNT] = {
    "Вода",
    "Трава",
    "Снег",
    "Болото",
    "Лава",
    "Пустыня",
    "Равнина"
};
char* gResourceNames[RESOURCE_COUNT] = {
    "Древесина",
    "Ртуть",
    "Руда",
    "Сера",
    "Кристаллы",
    "Самоцветы",
    "Золото"
};
char* gMineNames[RESOURCE_COUNT] = {
    "Лесопилка",
    "Лаборатория алхимика",
    "Рудная шахта",
    "Серная шахта",
    "Кристальная шахта",
    "Самоцветная шахта",
    "Золотоносная шахта"
};
char* gObjectNames[63] = {
    "",
    "Лаборатория алхимика",
    "Указатель",
    "Буй",
    "Скелет",
    "Пещера демона",
    "Ларец с сокровищами",
    "Кольцо фейри",
    "Костер",
    "Фонтан",
    "Беседка",
    "Древняя лампа",
    "Кладбище",
    "Лачуга",
    "Дом",
    "Будка",
    "Хижина",
    "Хижина",
    "Таверна 1",
    "Таверна 2",
    "Таверна 3",
    "Таверна 4",
    "Драконий город",
    "Маяк",
    "Водяная мельница",
    "Шахта",
    "Бивуак",
    "Обелиск",
    "Оазис",
    "Ресурсы",
    "Розовый куст",
    "Яма в песке",
    "Лесопилка",
    "Святилище",
    "Святилище",
    "Кораблекрушение",
    "Статуя",
    "Пень",
    "Лебединое озеро",
    "Шатры",
    "Город",
    "Менгир",
    "Фургоны",
    "Колодец",
    "Водоворот",
    "Ветряная мельница",
    "Дуб",
    "Мегалит",
    "Артефакт",
    "Тут ничего нет",
    "",
    "",
    "Горы",
    "Горы",
    "Горы",
    "Горы",
    "Деревья",
    "Деревья",
    "Деревья",
    "Деревья",
    "Деревья",
    "",
    "Корабль"
};
char* gTownNames[GAME_TOWN_COUNT] = {
    "Блэкбридж",  "Сосновый холм",
    "Вудхавен",  "Хилстон",
    "Вайтшилд",  "Бладрейн",
    "Клык дракона",  "Грейвинд",
    "Блэквинд",  "Портсмис",
    "Мидлгейт", "Тундара",
    "Вулкания", "Занзобар",
    "Атлантиум", "Байвоч",
    "Вилдабар", "Фонтанхед",
    "Вертиго", "Винтеркил",
    "Найтшэдоу", "Сэндкастер",
    "Лайксайд", "Олимпус",
    "Некрополис", "Бурлок",
    "Ксабран", "Драгадун",
    "Аламар", "Калиндра",
    "Блэкфанг", "Бесенжи",
    "Алгари", "Сорпигал",
    "Даск", "Эрликиум",
};
char* gEventText[EVENT_TEXT_COUNT] = {
    "Алхимик\n\nВы стали хозяином лаборатории местного алхимика. Она будет приносить вам по 1 единице ртути в день.",  "Указатель\n\nНа указателе написано:\n\n%s находится неподалеку отсюда.",
    "Буй\n\nВаши спутники замечают морской буй. Он указывает верный курс.",  "Буй\n\nВаши спутники замечают морской буй. Он указывает верный курс, и это повышает их мораль.",
    "Влага оседает на стенах и тонкими струйками стекает на землю. Повсюду видны следы побоища, но больше в пещере ничего нет.",  "Огромный демон выступает из тени, и вы бросаетесь в атаку. Победив в нелегком поединке, вы получаете 1000 очков опыта.",
    "Огромный демон выступает из тени, и вы бросаетесь в атаку. Победив в нелегком поединке, вы поучаете 1000 очков опыта и волшебный артефакт.",  "Огромный демон выступает из тени, и вы бросаетесь в атаку. Победив в нелегком поединке, вы получаете 1000 очков опыта и 2500 золотых.",
    "Вы попали в плен к огромному демону. Он предлагает вам свободу в обмен на 2500 золотых. Если вы откажетесь, он сожрет вас. Будете платить?",  "Поняв, что у вас нет 2500 золотых, демон вонзает в вас свои жуткие когти. Ваш взор застилает кровавая пелена и мир погружается во тьму.",
    "Пещера демона\n\nИз пещеры веет промозглой затхлостью. Два огромных красных глаза светятся во тьме. Желаете войти внутрь?", "Ларец\n\nОбследовав окрестности, вы находите клад. Вы можете взять золото или раздать его крестьянам в обмен на опыт. Хотите взять золото?",
    "Кольцо фейри\n\nВаше войско вступает внутрь кольца фейри, но ничего не происходит.", "Кольцо фейри\n\nВаше войско вступает внутрь кольца фейри, чары которого принесут вам удачу в грядущем сражении.",
    "Костер\n\nОбыскав вражеский лагерь, вы находите спрятанный клад.", "Фонтан\n\nВы припадаете к струям волшебного фонтана, но ничего не происходит.",
    "Фонтан\n\nБлагоуханная влага волшебного фонтана принесет вам удачу в грядущем сражении.", "Беседка\n\nНа ступенях беседки появляется старый рыцарь. \"Мне жаль, храбрый воин, но я уже научил тебя всему, что знаю сам.\"",
    "Беседка\n\nНа ступенях беседки появляется старый рыцарь. \"О храбрый воин, я научу тебя всему, что знаю сам; пусть мой опыт поможет тебе в твоих странствиях.\"", "Лампа джинна\n\nВы находите засыпанную землей помятую и закопченную лампа. Хотите ее потереть?",
    "Кладбище\n\nВы осторожно приближаетесь к захоронению древних воинов. Хотите вскрыть их могилы?", "Одержав победу над призраками, вы несколько часов подряд обыскиваете могилы, но ничего не находите. Ваш недостойный поступок отрицательно влияет на мораль войска.",
    "Одержав победу над призраками, вы обыскиваете могилы и удаляетесь с находкой!", "Хижина\n\nГруппа гоблинов в поисках славы желает примкнуть к вашему войску. Согласны ли вы принять их?",
    "Вы не можете принять новых рекрутов в свое войско, его ряды полны.", "Хижина\n\nПриблизившись к жилищу гоблинов, вы обнаруживаете, что оно пустует.",
    "Мазанка\n\nГруппа крестьян в поисках славы желает примкнуть к вашему войску. Согласны ли вы принять их? ", "Вы не можете принять новых рекрутов в свое войско, его ряды полны.",
    "Мазанка\n\nПриблизившись к жилищу крестьян, вы обнаруживаете, что оно пустует.", "Избушка\n\nГруппа лучников в поисках славы желает примкнуть к вашему войску. Согласны ли вы принять их?",
    "Вы не можете принять новых рекрутов в свое войско, его ряды полны.", "Избушка\n\nПриблизившись к жилищу лучников, вы обнаруживаете, что оно пустует.",
    "Избушка\n\nГруппа гномов в поисках славы желает примкнуть к вашему войску. Согласны ли вы принять их?", "Вы не можете принять новых рекрутов в свое войско, его ряды полны.",
    "Избушка\n\nПриблизившись к жилищу гномов, вы обнаруживаете, что оно пустует.", "Мазанка\n\nГруппа крестьян в поисках славы желает примкнуть к вашему войску. Согласны ли вы принять их?",
    "Вы не можете принять новых рекрутов в свое войско, его ряды полны.", "Мазанка\n\nПриблизившись к жилищу крестьян, вы обнаруживаете, что оно пустует.",
    "Город драконов\n\nВы подошли к Городу драконов, славному своими богатствами и неисчислимыми опасностями. Вы хотите атаковать его?", "Вы одержали победу над могучими драконами. Отдавая должное вашей доблести, они готовы ежедневно платить в вашу казну по 1000 золотых и оборонять город от неприятелей.",
    "Маяк\n\nТеперь маяк ваш, и все ваши корабли будут преодолевать большее расстояние за один ход.", "Водяная мельница\n\nМельник обращается к вам со словами: \"Сожалею, господин, но сегодня золота у меня нет. Приходите на следующей неделе.\"",
    "Водяная мельница\n\nМельник обращается к вам со словами: \"Господин, я трудился в поте лица и прошу вас принять мою скромную лепту. Приходите на следующей неделе, и вы получите еще столько же.\"", "Рудная шахта\n\nВы стали хозяином рудной шахты. Она будет приносить вам по 2 единицы руды в день.",
    "Серные копи\n\nВы стали хозяином серных копей. Они будут приносить вам по 1 единице серы в день.", "Кристальная шахта\n\nВы стали хозяином кристальной шахты. Она будет приносить вам по одной мере кристаллов в день.",
    "Самоцветная шахта\n\nВы стали хозяином самоцветной шахты. Она будет приносить вам по 1 единице самоцветов в день.", "Золотая шахта\n\nВы стали хозяином золотой шахты. Она будет приносить вам по 1000 золотых в день.",
    "Последователи\n\nГруппа %s в поисках славы желает примкнуть к вашему войску. Вы согласны принять их?", "Оскорбленные отказом быть принятыми в ваши ряды, они нападают на вас!",
    "Обелиск\n\nПеред вами обелиск, высеченный из невиданного камня. Вы вглядываетесь в его гладкую поверхность и вдруг замечаете, что на ней начинают проступать таинственные знаки. Знаки складываются во фрагмент древней карты. Вы торопливо срисовываете его, и знаки исчезают так же внезапно, как и появились.", "Обелиск\n\nВы уже посещали этот обелиск.",
    "Оазис\n\nВы находите оазис, но его колодец пересох, и вы удаляетесь ни с чем.", "Оазис\n\nСтранствующий торговец, бредущий пешком, окликает вас и рассказывает, что его лошадь убежала и он остался один в чужом краю. В благодарность за помощь он приводит ваше войско к оазису, что повышает их мораль в грядущем сражении.",
    "Вы нашли ресурс (%s).", "Лесопилка\n\nВы стали хозяином лесопилки. Она будет приносить вам по 2 единицы древесины в день.",
    "Оракул\n\nВнутри величественного храма восседает слепой оракул. Вы рассказываете ему о целях вашего похода, и он показывает вам сильные и слабые стороны ваших противников в волшебном хрустальном шаре.", "Святилище\n\nВ центре небольшого храма, скрытого от чужих глаз, стоит старый деревянный алтарь. На алтаре покоится золотая пластина, на которой начертан секрет древнего магического заклинания ",
    "Разбитый корабль\n\nВолны прибили к скалам полузатонувший корпус большого пиратского корабля. Вы хотите заглянуть внутрь?", "Одержав победу над призраками, вы несколько часов подряд обыскиваете останки корабля, но ничего не находите. Ваш недостойный поступок отрицательно влияет на мораль войска.",
    "Одержав победу над призраками, вы обыскиваете останки корабля и удаляетесь с находкой!", "Статуя\n\nНад вами возвышается огромная статуя ангела. Внезапно ангел открывает глаза, и ваши воины с радостью ощущают, что их мораль возрасла.",
    "Статуя\n\nНад вами возвышается гигантская статуя ангела. Ваши воины окружают ее, но она остается мертвой и неподвижной.", "Шатры\n\nВаше внимание привлекает группа шатров, пологи которых трепещут на жарком ветру пустыни. В шатрах никого нет. Пройдет время, и быть может, сюда придет новый отряд кочевников.",
    "Шатры\n\nВаше внимание привлекает группа шатров, пологи которых трепещут на жарком ветру пустыни. Вы хотите принять в ваше войско отряд кочевников?", "Фургоны\n\nЯркие фургоны разбойников пусты. Пройдет время, и быть может, здесь обоснуется новая шайка.",
    "Фургоны\n\nВдалеке слышится музыка и смех. Вы идете на эти звуки и находите  яркие фургоны, в которых живут разбойники. Вы хотите принять в ваше войско шайку разбойников?", "Водоворот\n\nВаш корабль попадает в водоворот. Часть вашего войска исчезает в пучине.",
    "Ветряная мельница\n\nМельник обращается к вам со словами: \"Сожалею, господин, но сегодня у меня ничего нет. Приходите на следующей неделе.\"", "Ветряная мельница\n\n Мельник обращается к вам со словами: \"Господин, я работал не покладая рук, и прошу вас принять мой скромный дар. Приходите на следующей неделе, у меня опять найдется, чем вас порадовать.\"",
    "Артефакт\n\nВы находите древний артефакт. Вы направляетесь к нему, но дорогу вам преграждает шайка разбойников, готовых драться за свою добычу.", "Артефакт\n\nМаленький лепрекон предлагает вам %s в обмен на 2000 золотых. Вы хотите приобрести артефакт?",
    "Оскорбленный вашим отказом от такой выгодной сделки лепрекон в гневе топает ножкой и исчезает.", "Вы достаете кошель, но вдруг обнаруживаете, что у вас нет 2000 золотых. Лепрекон топает ножкой и с презрением поворачивается к вам спиной.",
    "Перебив разбойников, вы обыскиваете их трупы и обнаруживаете %s.", "Скелет\n\nВы обнаруживаете останки незадачливого искателя приключений. Пошарив в груде лохмотьев, прикрывающих скелет, вы ничего не находите.",
    "Скелет\n\nВы обнаруживаете останки незадачливого искателя приключений. Пошарив в груде лохмотьев, вы находите",
};
char* gAPanelHelp[5] = {
    "Осмотреть весь мир.",
    "Посмотреть головоломку.",
    "Направить заклинание.",
    "Копать в поисках Могущественного артефакта.",
    "Закрыть это меню ничего не делая.",
};
char* gInitMenuHelp[MAIN_MENU_HELP_COUNT] = {
    "Начать одиночную или сетевую игру.",
    "Загрузить сохраненную игру.",
    "Открыть таблицу рекордов.",
    "Посмотреть список авторов.",
    "Выйти из Героев Меча и Магии и вернуться в систему.",
};
char* gAdvMenuHelp[ADVENTURE_HELP_COUNT] = {
    "Следующий герой\n\nВыбрать следующего героя.",
    "Продолжить движение \n\nПродолжить движение героя по намеченному пути.",
    "Обзор королевства\n\nОсмотреть ваши владения.",
    "Окончить ход\n\nОкончить ход и передать управление компьютеру.",
    "Игровые действия\n\nОткрывает окно игровых действий.",
    "Настройки игры\n\nОткрыть окно настроек игры.",
};
char* gLuckText[7] = {
    "Проклятая",
    "Ужасная",
    "Плохая",
    "Обычная",
    "Хорошая",
    "Отличная",
    "Божественная",
};
char* gMoraleText[7] = {
    "Предательская",
    "Ужасная",
    "Плохая",
    "Обычная",
    "Хорошая",
    "Отличная",
    "Кровавая!",
};
char* gOnOffText[11] = {
    "Выкл.",
    "Вкл.",
    "Вкл.\nГромкость 9",
    "Вкл.\nГромкость 8",
    "Вкл.\nГромкость 7",
    "Вкл.\nГромкость 6",
    "Вкл.\nГромкость 5",
    "Вкл.\nГромкость 4",
    "Вкл.\nГромкость 3",
    "Вкл.\nГромкость 2",
    "Вкл.\nГромкость 1"
};
char* gWalkSpeedText[WALK_SPEED_COUNT] = {
    "Шагом",
    "Рысью",
    "Аллюром",
    "Галопом",
    "Прыжками"
};
char* gColorNames[PLAYER_COLOR_COUNT] = {
    "синий",
    "зеленый",
    "красный",
    "желтый"
};
char* gAlignmentNames[5] = {
    "люди",
    "равнинные",
    "лесные",
    "горные",
    "нейтральные"
};
char* gSpellDesc[SPELL_COUNT] = {
    "Огненный шар\n\nОгромный огненный шар взрывается над выбранным участком поля боя, поражая всех находящихся поблизости участников сражения.",  "Молния\n\nМощный электрический разряд поражает выбранную группу воинов.",
    "Телепортация\n\nПереносит выбранную группу воинов в любое свободное место на поле боя.",  "Лечение\n\nНейтрализует все враждебные чары, насланные на ваших воинов.",
    "Воскрешение\n\nВоскрешает воинов в группе, которой был нанесен урон.",  "Ускорение\n\nСкорость любой группы воинов становится 'очень высокой'.",
    "Замедление\n\nУменьшает скорость даже самой быстрой группы вражеских воинов.",  "Ослепление\n\nЗатуманивает взоры воинов в выбранной группе и тем самым не позволяет им перемещаться по полю боя.",
    "Благословение\n\nУвеличивает до максимума урон, наносимый выбранной группой воинов.",  "Защита\n\nУвеличивает навык защиты воинов выбранной группы.",
    "Проклятие\n\nУменьшает до минимума урон, причиняемый выбранной группой воинов.", "Убить нежить\n\nМгновенно загоняет призраков в покинутые ими могилы.",
    "Антимагия\n\nДелает невозможным применение враждебных чар против выбранной группы воинов.", "Снять чары\n\nНейтрализует любые чары, насланные на всех участников сражения.",
    "Берсерк\n\nЗаставляет выбранную группу воинов нападать на ближайшую к ней соседнюю группу.", "Армагеддон\n\nМогущественные силы поражают всех участников сражения, нанося им жестокий урон.",
    "Шторм\n\nСилы стихий обрушиваются на поле боя, нанося урон всем участникам сражения.", "Звездопад\n\nЗвездопад поражает выбранный участок поля боя, нанося урон всем находящимся поблизости участникам сражения.",
    "Паралич\n\nГруппу, против которой направлено это заклинание, поражает паралич, и она теряет способность передвигаться или отвечать на удары.", "Шахты\n\nДелает видимыми все шахты на игровой карте.",
    "Ресурсы\n\nДелает видимыми все ресурсы на игровой карте.", "Артефакты\n\nДелает видимыми все артефакты на игровой карте.",
    "Города\n\nДелает видимыми все города и замки на игровой карте.", "Герои\n\nДелает видимыми всех героев на игровой карте.",
    "Все\n\nДелает видимой всю игровую карту.", "Опознать героя\n\nПозволяет получить подробную информацию о вражеских героях.",
    "Корабль\n\nПереносит ближайший незанятый дружественный корабль в соответствующую точку побережья. Дружественным считается корабль, который вы только что построили, либо тот, на котором после вас никто не плавал.", "Портал\n\nПереносит вас в точку на карте, расположенную поблизости.",
    "Врата города\n\nПереносит вас в любой принадлежащий вам город или замок.",
};
char* gMonthNames[CALENDAR_MONTH_NAME_COUNT] = {
    "Кузнечика",
    "Муравья",
    "Стрекозы",
    "Паука",
    "Бабочки",
    "Шмеля",
    "Цикады",
    "Земляного червя",
    "Шершня",
    "Жука",
};
char* gWeekNames[CALENDAR_WEEK_NAME_COUNT] = {
    "Белки",
    "Кролика",
    "Суслика",
    "Барсука",
    "Крысы",
    "Орла",
    "Горностая",
    "Ворона",
    "Мангуста",
    "Собаки",
    "Муравьеда",
    "Ящерицы",
    "Черепахи",
    "Дикобраза",
    "Кондора",
};
char* gDwellingDescriptions[24] = {
    "В Мазанке покупают крестьян.",
    "На Стрельбище покупают лучников.",
    "В Кузнице покупают копейщиков.",
    "Оружейная позволяет покупать мечников.",
    "На Ристалище покупают всадников.",
    "В Соборе покупают паладинов.",
    "В Древо-доме покупают фей.",
    "В Коттеджах покупают гномов.",
    "На Стрельбище покупают эльфов.",
    "В Стоунхендже покупают друидов.",
    "В Загоне покупают единорогов.",
    "В Алой башне покупают фениксов.",
    "В Хижине покупают гоблинов.",
    "В Хибаре покупают орков.",
    "В Логове покупают волков.",
    "В Доме огров покупают огров.",
    "Под Мостом покупают троллей.",
    "В Пирамиде покупают циклопов.",
    "В Пещере покупают кентавров.",
    "В Крипте покупают горгулий.",
    "В Гнезде покупают грифонов.",
    "В Лабиринте покупают Минотавров.",
    "В Болоте покупают гидр.",
    "В Черной башне покупают драконов.",
};
char* gArmySizeNames[6][2] = {
    {"Мало", "Мало"},
    {"Немного", "Немного"},
    {"Свора", "Свора"},
    {"Много", "Много"},
    {"Орда", "Орда"},
    {"Тьма!", "Тьма"},
};
char* gHeroScreen[HERO_TEXT_COUNT] = {
    "Обзор королевства",  "%s - информация",
    "Дополнительная статистика героя",  "Информация о высокой морали",
    "Информация об обычной морали",  "Информация о плохой морали",
    "Информация о хорошей удаче",  "Информация об обычной удаче",
    "Информация о плохой удаче",  "Показать опыт",
    "Отряд %s", "Пусто",
    "Передвинуть сюда отряд %s", "Отряды %s и %s меняются местами",
    "Показать заклинания", "Описание артефакта '%s'",
    "%s %s будет уволен(а)?", "Закрыть экрана героя",
    "Экран героя",
};
char* gCastleInfo[TOWN_CASTLE_INFO_COUNT] = {
    "Построить Гильдию магов",
    "Построены все этажи Гильдии магов.",
    "Нельзя построить следующий этаж.",
    "Построить следующий этаж Гильдии магов ",
    "Постройка '%s' уже возведена",
    "Нельзя возвести постройку '%s'",
    "Нельзя возвести постройку '%s'",
    "Возвести постройку '%s'",
    "Герой вам не по карману.",
    "Нельзя нанять - у вас уже %d героев.",
    "Нельзя нанять - в этом городе у вас уже есть герой.",
    "Нанять нового героя ",
    "Выйти из замка",
    "Возможности замка",
};
char* gLuckInfoText[LUCK_INFO_COUNT] = {
    "Хорошая удача\n\nЕсли удача вашего войска выше обычной, атаки отдельных отрядов на поле боя иногда оказываются более результативными (их сила удваивается).",
    "Обычная удача\n\nС обычной удачей ваше войско не имеет ни преимуществ, ни недостатков на поле боя.",
    "Плохая удача\n\nЕсли вашему войску не везет, урон, наносимый  отдельными отрядами на поле боя, может оказаться вдвое меньше обычного.",
    "%s\n\n\nТекущие компоненты удачи:",
    "\nЛапка кролика +1",
    "\nПодкова +1",
    "\nМонета +1",
    "\nКлевер +1",
    "\nКруг фейри +1",
    "\nФонтан +1",
    "\nнет",
};
char* gMemoryErrorTitle = "Не хватает памяти";
char* gMemoryRequirements = "Игре Герои Меча и Магии требуется как минимум:";
char* gExtendedMemoryUnits = "Кб верхней памяти (XMS или EMS) и";
char* gConventionalMemoryUnits = "Кб общей памяти ";
char* gPlayerTypeNames[5] = {
    "Нет",
    "Глупый",
    "Средний",
    "Умный",
    "Гений"
};
char* gSpellHelp[SPELL_HELP_COUNT] = {
    "Предыдущая страница ",
    "Следующая страница",
    "Путевые заклинания",
    "Боевые заклинания",
    "Закрыть магическую книгу",
    "Заклинания",
    "Выбрать заклинание",
    "Боевые заклинания",
};
char* gSpeedText[5] = {
    "",
    "Низкая",
    "Средняя",
    "Высокая",
    "Оч. высокая",
};
char* gArmyStatText[9] = {
    "Атака: ",
    "Защита: ",
    "Выстрелов: ",
    "Урон: ",
    "Здоровье: ",
    "Скорость: ",
    "Мораль: ",
    "Удача: ",
    "Стрел: ",
};
char* gOverviewText[3] = {
    "Обзор королевства     Месяц: %d, Неделя: %d, День: %d",
    "Ваш Драконий город.",
    "Ваш маяк.",
};
char* gNewTurnText[7] = {
    "%s игрок, у вас осталось всего %d дней на то, чтобы завоевать хотя бы один город; иначе вы будете навеки изгнаны из страны.",
    "%s игрок, настал последний день, когда вы еще можете завоевать себе город; в противном случае вы будете навеки изгнаны из страны.",
    "Астрологи объявляют месяц %s.\n\nНаселение всех жилищ возросло.",
    "Астрологи объявляют, что этому месяцу покровительствует сила %s.\n\nПопуляция %s удваивается!\n\nНаселение всех жилищ возросло.",
    "Астрологи объявляют месяц ЧУМЫ!\n\nНаселение всех жилищ уменьшилось вдвое.",
    "Астрологи объявляют неделю %s.\n\nНаселение всех жилищ возросло.",
    "Астрологи объявляют, что этой неделе покровительствует сила %s.\n\nПопуляция %s +5.\n\nНаселение всех жилищ возросло.",
};
char* gViewGeneralLabels[6] = {
    "Атака: ",
    "Защита: ",
    "Сила магии: ",
    "Знания: ",
    "Мораль: ",
    "Удача: ",
};
char* gViewGeneralHelp[GENERAL_HOVER_HELP_COUNT] = {
    "Остановить катапульту",
    "Направить заклинание",
    "Отступить",
    "Сдаться",
    "Отмена",
    "Возможности героя",
};
char* gCombatMessage[COMBAT_TEXT_COUNT] = {
    "",
    "%s - идти сюда.",
    "%s - перелететь сюда.",
    "Атаковать %s",
    "Стрелять в %s (осталось %d выстрелов)",
    "Возможности героя",
    "Вражеский герой",
    "Информация о воине (%s)",
    "Нет стрел!",
};
char* gHeroLevel[HERO_LEVEL_TEXT_COUNT] = {
    "%s получает",
    " уровень опыта.\n",
    " %d уровня опыта.\n"
};
char* gCombatHelp[COMBAT_HELP_COUNT] =
    {"Включить автобой", "Пропустить ход отряда", ""};
char* gTownCommand[TOWN_TEXT_COUNT] = {
    "Разделить отряд %s",  "Нельзя забрать последних воинов у героя ",
    "Соединить отряды %s",  "Разделить отряд %s",
    "Отряд %s",  "Нельзя перенести в гарнизон последний отряд.",
    "Передвинуть сюда отряд %s",  "Отряды %s и %s меняются местами",
    "Выйти из города",  "",
    "Обзор королевства", "Пусто",
    "Отряд %s", "Показать героя",
    "Гильдия магов", "Гильдия воров",
    "Таверна", "Верфь",
    "Колодец", "Шатры",
    "Замок", "Нанять %s",
};
char* gGameTypeHelp[5] = {
    "Играть одиночный сценарий против компьютерных оппонентов.",
    "Играть кампанию, состоящую из серии отдельных сценариев.",
    "Играть против других игроков-людей на одном компьютере, по сети или модему.",
    "Играть тренировочную игру.",
    "Отменить и вернуться в главное меню.",
};
char* gHeroNames[36][2] = {
    {"Лорд Килбурн", "Килбурн"},
    {"Лорд Хаарт", "Хаарт"},
    {"Сэр Галлант", "Галлант"},
    {"Артуриус", "Артуриус"},
    {"Тиро", "Тиро"},
    {"Максимус", "Максимус"},
    {"Эктор", "Эктор"},
    {"Димитрий", "Димитрий"},
    {"Амброзий", "Амброзий"},
    {"Сундакс", "Сундакс"},
    {"Эргон", "Эргон"},
    {"Келзен", "Келзен"},
    {"Тсабу", "Тсабу"},
    {"Крэг Хак", "Крэг"},
    {"Джоджош", "Джоджош"},
    {"Атлас", "Атлас"},
    {"Йог", "Йог"},
    {"Антоний", "Антоний"},
    {"Ариэль", "Ариэль"},
    {"Ватавна", "Ватавна"},
    {"Карлавн", "Карлавн"},
    {"Ребекка", "Ребекка"},
    {"Луна", "Луна"},
    {"Астра", "Астра"},
    {"Наташа", "Наташа"},
    {"Гем", "Гем"},
    {"Троян", "Троян"},
    {"Агар", "Агар"},
    {"Кродо", "Кродо"},
    {"Фалагар", "Фалагар"},
    {"Барок", "Барок"},
    {"Арий", "Арий"},
    {"Кастор", "Кастор"},
    {"Сандро", "Сандро"},
    {"Расмонт", "Рас"},
    {"Виспер", "Виспер"},
};
char* gCPanelHelp[CPANEL_HELP_COUNT] = {
    "Начать одиночную или сетевую игру.",
    "Загрузить ранее сохраненную игру.",
    "Выйти из Героев Меча и Магии и вернуться в ОС.",
    "Выйти из этого меню, ничего не делая.",
    "Сохранить данную игру.",
    "Включить или выключить музыку",
    "Включить или выключить фоновые звуки",
    "Изменить скорость передвижения героев по карте.",
    "Изменить качество звука. CD-стерео имеет наилучшее качество, и, как правило, не снижает производительность системы. Однако некоторые системы не способны воспроизводить звук с компакт-диска; тогда приходится использовать 8-битный звук.",
    "Вкл./Выкл. 'Показать путь'. Если опция отмечена, первый щелчок по объекту на игровой карте показывает путь до него, а по второму щелчку начинается движение. Если эта опция отключена, движение к заданной точке начинается по первому щелчку мыши.",
    "Включение/выключение режима 'Показать перемещения противника'. Если этот режим включен, будут показаны перемещения всех противников на открытой части игровой карты. Следует иметь в виду, что при игре по сети или модему этот режим автоматически отключается.",
    "Показать информацию о текущем сценарии."
};
char* gNewGameHelp[NEW_GAME_HELP_COUNT] = {
    "Принять установки и начать игру.",
    "Вернуться в главное меню.",
    "Бросить вызов 'Царь горы' компьютерным игрокам. Ваша наглая выходка оскорбит компьютерных игроков и они, оставив друг друга в покое, будут стремиться разгромить вас в прах.",
    "Выберите сценарий, на котором хотите играть.",
    "Изменить начальную сложность игры. Чем больший уровень сложности вы выберете, тем с меньшим количеством ресурсов вы начнете игру.",
    "Изменить уровень сложности для этого соперника. Более умные компьютерные игроки ведут себя более агрессивно и тратят больше времени на ход.",
    "Изменить цвет вашего знамени.",
    "Уровень сложности отражает соотношение различных параметров игры. Указанное количество очков будет прибавлено к вашему финальному счету.",
    "Изменить стартовую сложность игры другого игрока. Чем больший уровень сложности вы выберете, тем с меньшим количеством ресурсов вы начнете игру.",
};
char* gSetupCampaignGameHelp[SETUP_CAMPAIGN_HELP_COUNT] = {
    "Играть за Лорда Айронфиста.",
    "Играть за Вождя Слэйера.",
    "Играть за Царицу Ламанду.",
    "Играть за Колдуна Аламара.",
    "Отменить и вернуться в главное меню.",
};
char* gSetupBaudHelp[SETUP_BAUD_HELP_COUNT] = {
    "Скорость соединения 2400 бод. \n\nДля модемов со скоростью 14400 бод используйте скорость 19200 бод. Для модемов со скоростью 28800 бод используйте скорость 38400 бод.",
    "Скорость соединения 9600 бод. \n\nДля модемов со скоростью 14400 бод используйте скорость 19200 бод. Для модемов со скоростью 28800 бод используйте скорость 38400 бод.",
    "Скорость соединения 19200 бод.\n\nДля модемов со скоростью 14400 бод используйте скорость 19200 бод. Для модемов со скоростью 28800 бод используйте скорость 38400 бод.",
    "Скорость соединения 38400 бод.\n\nДля модемов со скоростью 14400 бод используйте скорость 19200 бод. Для модемов со скоростью 28800 бод используйте скорость 38400 бод.",
    "Отменить и вернуться в главное меню.",
};
char* gSetupComPortHelp[SETUP_COM_PORT_HELP_COUNT] = {
    "Использовать порт COM 1 для модемного соединения.",
    "Использовать порт COM 2 для модемного соединения.",
    "Использовать порт COM 3 для модемного соединения.",
    "Использовать порт COM 4 для модемного соединения.",
    "Отменить и вернуться в главное меню.",
};
char* gSetupDCBaudHelp[SETUP_BAUD_HELP_COUNT] = {
    "Скорость соединения 2400 бод. \n\nДля компьютеров с устаревшим чипом UART 8250 следует использовать скорость 19200 бод, а для компьютеров с более современным чипом UART 16550 - скорость 38400 бод. Если вы не уверены, какой у вас чип, начните с более низких скоростей. В большинстве компьютеров, произведенных в 1994 году и позднее, используется чип UART 16550.",
    "Скорость соединения 9600 бод. \n\n Для компьютеров с устаревшим чипом UART 8250 следует использовать скорость 19200 бод, а для компьютеров с более современным чипом UART 16550 - скорость 38400 бод. Если вы не уверены, какой у вас чип, начните с более низких скоростей. В большинстве компьютеров, произведенных в 1994 году и позднее, используется чип UART 16550.",
    "Скорость соединения 19200 бод.\n\n Для компьютеров с устаревшим чипом UART 8250 следует использовать скорость 19200 бод, а для компьютеров с более современным чипом UART 16550 - скорость 38400 бод. Если вы не уверены, какой у вас чип, начните с более низких скоростей. В большинстве компьютеров, произведенных в 1994 году и позднее, используется чип UART 16550.",
    "Скорость соединения 38400 бод.\n\n Для компьютеров с устаревшим чипом UART 8250 следует использовать скорость 19200 бод, а для компьютеров с более современным чипом UART 16550 - скорость 38400 бод. Если вы не уверены, какой у вас чип, начните с более низких скоростей. В большинстве компьютеров, произведенных в 1994 году и позднее, используется чип UART 16550.",
    "Отменить и вернуться в главное меню. ",
};
char* gSetupDCComPortHelp[SETUP_COM_PORT_HELP_COUNT] = {
    "Использовать порт COM 1 для прямого соединения.",
    "Использовать порт COM 2 для прямого соединения.",
    "Использовать порт COM 3 для прямого соединения.",
    "Использовать порт COM 4 для прямого соединения.",
    "Отменить и вернуться в главное меню.",
};
char* gSetupHotSeatGameHelp[SETUP_HOT_SEAT_HELP_COUNT] = {
    "Играть с 2 людьми и, опционально, до 2 дополнительных компьютерных игроков.",
    "Играть с 3 людьми и, опционально, с 1 компьютерным игроком.",
    "Играть с 4 людьми.",
    "Отменить и вернуться в главное меню.",
};
char* gSetupModemGameHelp[SETUP_MODEM_HELP_COUNT] = {
    "Хост определяет настройки игры, выбирает номер дозвона и размещает вызовы.",
    "Гость ожидает вызова хоста.",
    "Изменить настройки модема.",
    "Отменить и вернуться в главное меню.",
};
char* gSetupDCGameHelp[SETUP_MODEM_HELP_COUNT] = {
    "Хост задает настройки игры.",
    "Гость ожидает, пока хост определит опции игры.",
    "Изменить порт прямого соединения.",
    "Отменить и вернуться в главное меню.",
};
char* gSetupMultiPlayerGameHelp[SETUP_MULTIPLAYER_HELP_COUNT] = {
    "Играть за одним компьютером, где от 2 до 4 игроков сражаются за одной машиной, меняясь по очереди местами при передаче хода.",
    "Играть по локальной сети, где машины 2 игроков соединены между собой.",
    "Играть по модему, где 2 игрока используют свои компьютеры, соединенные между собой по телефонной линии с использованием модема.",
    "Играть через прямое соединение, где 2 игрока играют на машинах соединенных через порты по ноль-модему.",
    "Отменить и вернуться в главное меню.",
};
char* gSetupNetworkGameHelp[SETUP_NETWORK_HELP_COUNT] = {
    "Сервер задает настройки игры. Может быть, только один хост в одном сетевом соединении.",
    "Гость ожидает, пока сервер задаст настройки игры, после чего он автоматически вступит в игру. В игре по сети может быть только 1 гость.",
    "Отменить и вернуться в главное меню.",
};
char* gSetupGameHelp[SETUP_GAME_HELP_COUNT] = {
    "Одиночная игра на отдельной карте.",
    "Одиночная игра на серии карт.",
    "Сетевая игра, где несколько игроков-людей сражаются друг против друга на одной карте.",
    "Отменить и вернуться в главное меню.",
};
char* gBattleResults[BATTLE_RESULT_COUNT] = {
    "Враг сдался!",
    "Враг повержен!",
    "Великая победа!",
    "\n\nЗа мужество, проявленное в бою, %s получает %d оч. опыта.",
    "%s сдается врагу и отступает с позором.",
    "%s трусливо бежит с поля боя.",
    "Ваши войска потерпели поражение и %s покидает вас.",
    "Ваши силы сдались врагу и отступили с позором.",
    "Ваши трусливые войска бежали с поля боя.",
    "Ваши войска потерпели поражение.",
    "\n\nЗа мужество, проявленное в бою, %s получает %d оч. опыта, и получает %d уровень(я).",
};
char* gNeutralBuildingDescriptions[BUILDING_SLOT_NEUTRAL_COUNT] =
    {
        "Гильдия магов позволяет разучивать новые заклинания.",
        "Гильдия воров дает информацию о врагах. Также, Гильдия воров дает разведывательную информацию о вражеских городах.  Дополнительные гильдии дают дополнительную информацию.",
        "Таверна увеличивает мораль бойцов, защищающих замок.",
        "Верфь позволяет строить корабли.",
        "Колодец увеличивает прирост всех воинов на 2 в неделю.",
        "Шатры дают рабочих, которые могут возвести замок.",
        "Замок улучшает защиту города и увеличивает доход.",
};
char* gMoraleInfoText[MORALE_INFO_COUNT] = {
    "Высокая мораль\n\nВысокая мораль может дать в бою вашим бойцам дополнительную атаку.",  "Обычная мораль\n\nС обычной моралью ваши армии никогда не получат дополнительную атаку и не будут прокляты.",
    "Низкая мораль\n\nНизкая мораль может привести к потере хода в бою вашими бойцами.",  "%s\n\n\nМодификаторы морали:",
    "\nБонус рыцаря +1",  "\nВсе %s бойцы +1",
    "\nВоины 3 рас -1",  "\nВоины 4 рас -2",
    "\nМедаль отваги +1",  "\nМедаль мужества +1",
    "\nМедаль доблести +1", "\nМедаль почета +1",
    "\nСимвол неудачи -2", "\nПосещен буй +1",
    "\nПосещен оазис +1", "\nПосещена статуя +2",
    "\nРасхититель гробниц -1", "\nРасхититель обломков -1",
    "\nТрусость в бою %d", "\nНет",
    "\nВоины 5 рас -3",
};
char* gMapSizeNames[MAP_SIZE_COUNT] = {
    "Маленькая",
    "Средняя",
    "Большая",
};
char* gMapDifficultyNames[MAP_DIFFICULTY_COUNT] = {
    "Легкая",
    "Обычная",
    "Тяжелая",
    "Невозможная",
    "Невероятная",
};
char* gCampaignScenarioNames[9] = {
    "Вы захватили плацдарм в новой стране. Вместе с вами еще три славных героя вступили в жестокий спор за обладание маленьким островком. Всех вас влечет к его центру - городу Предвратье. Город стоит посреди острова, и обладание им равносильно победе. Берегитесь дракона, который стережет подступы к Предвратью!",
    "Путь к материку преграждает Архипелаг Древних. Он состоит из четырех островов, каждым из которых владеет один правитель. Все четыре властителя должны быть разгромлены, и сделать это надо сейчас или никогда! Конечно, вам не обойтись без кораблей, но пользуйтесь ими мудро.",
    "Страна погрузилась в водоворот кровавой бойни. Но люди готовы пойти за героем, который овладеет Оком Гороса. Артефакт был захоронен и потерян в стародавние времена. Первый, кто отыщет Око, объединит народы и покорит эту землю.",
    "Теперь соседи видят в вас серьезного соперника. Все они стремятся завоевать центральный континент. Земля огромна, а враги далеко. Но здесь мало ресурсов, и они сильно охраняются. Вы должны победить всех и стать хозяином целого материка.",
    "Страна рыцарей, которой правит Лорд Айронфист, разделена извилистой Половодной рекой. Айронфист надеется, что она защитит его. Единственный прибрежный город, где есть верфь, находится далеко на востоке. Вы победите. Если захватите замок Айронфиста на северо-западе.",
    "Далеко на севере, за Непроходимой пустыней, лежат Мерзлые пустоши. Там обосновался Вождь Слэйер - кровожадный предводитель варварских кланов. Как только заставы в ущелье падут, орды варваров хлынут на юг. Замок Вождя Слэйера стоит к северо-востоку от ущелья.",
    "Предупрежденная о вашем приближении, Царица Ламанда сотворила заклятье, затопившее подходы к единственному порту. Вы должны найти портал, чтобы захватить порт. Единственный участок суши доступный для высадки находится на северо-востоке. Оттуда вам надо пробиваться к замку на северо-западе.",
    "Замок чернокнижников стоит в кратере вулкана. Чтобы пробиться к замку Колдуна Аламара, вам следует пройти через Лабиринт Минотавра. Маги уверены в своей неуязвимости и не ожидают атаки. Горгульи поджидают вас на истинной дороге к выходу из Лабиринта.",
    "Великая победа не за горами, но поверженные лорды еще опасны! Они объединили остатки своих сил и двинули несметные рати на ваши замки. Если вы укротите драконов, мятежные лорды сложат оружие. Захватите Драконий город на центральном острове и победа будет в ваших руках!",
};
char* gCampaignWinTexts[9] = {
    "Предвратье в ваших руках! Враги оставили свои замки на произвол судьбы и позорно бежали. Они нашли себе новые земли и снова копят силы. Промедление смерти подобно!",
    "Архипелаг Древних покорен и присоединен к вашим владеньям. За горизонтом лежит обширный неисследованный враждебный континент.",
    "Целительная сила Ока Гороса возрождает измученную землю. Люди собираются под ваши знамена, а противники отступают. Грядет война за власть над миром!",
    "Победа осталась за вами, враги расползлись по своим логовам. Вам еще предстоит захватить их родовые замки, по одному выкуривая их из насиженных нор.",
    "Сопротивление рыцарей сломлено! Их страна лежит у ваших ног.",
    "Замок варваров взят штурмом, а их армии разбиты!",
    "Вы пробились через лесной лабиринт и сравняли с землей замок Ламанды.",
    "Вы преследовали горгулий до самого замка Колдуна Аламара и повергли в прах чернокнижников.",
    "Драконы встают под ваши знамена, и мятежные лорды молят о пощаде. Теперь вы и только вы - великий властелин единой и необозримой страны!",
};
char* gCampaignScenarioText[9] = {
    "Предвратье",
    "Архипелаг",
    "Больные земли",
    "Свобода любой ценой",
    "Замок Айронфиста",
    "Замок Слэйера",
    "Замок Ламанды",
    "Замок Аламара",
    "Царь горы",
};
char* gDifficultyNames[DIFFICULTY_COUNT] = {
    "Легкая",
    "Обычная",
    "Тяжелая",
    "Эксперт",
};
char* gCampaignSideNames[4] = {
    "Лорд Айронфист",
    "Вождь Слэйер",
    "Царица Ламанда",
    "Колдун Аламар"
};
char* gScoreLabels[CONGRATS_SCORE_LABEL_COUNT] = {
    "Дней:",
    "Очки:",
    "Сложность:",
    "Счет:",
    "Ранг:",
};
char* gHumanPlayerTypeNames[5] = {
    "Человек\n",
    "Человек\nЛегкая игра",
    "Человек\nОбычная игра",
    "Человек\nТяжелая игра",
    "Человек\nЭксперт"
};
char* gHandicapNames[5] = {
    "Чел.-",
    "Чел.-Легкая игра",
    "Чел.-Обычная игра",
    "Чел.-Тяжелая игра",
    "Чел.-Эксперт"
};
char* gMusicQualityText[3] = {
    "8 бит моно",
    "8 бит стерео",
    "CD стерео"
};
char* gWinSetupText[68] = {
    "Купить книгу:",  "Ресурсов:",
    "Построить:",  "Замок:  Улучшить город/Нанять героя",
    "Гильдия магов",  "Гильдия воров",
    "Таверна",  "Верфь",
    "Колодец",  "Нанять героя",
    "Музыка", "Эффекты",
    "Качество\nзвука", "Скорость",
    "Путь", "Показывать\nход врагов",
    "Портал:\nВыберите место назначения", "Атака",
    "Защита", "Сила магии",
    "Знания", "Указанные заклинания были добавлены в вашу Волшебную книгу.",
    "Выбрать сложность:", "Легкая",
    "Обычная", "Тяжелая",
    "Эксперт", "Уровень противника:",
    "Норма", "Норма",
    "Норма", "Цвет:",
    "Царь горы:", "Сценарий:",
    "Герои", "Замки",
    "Города", "Шахты",
    "Ресурсы", "Золота в день:",
    "Атака:", "Защита:",
    "Сила магии:", "Знания:",
    "Защитники:", "Нанять героя",
    "Строить корабль:", "Цена:",
    "Атака", "Защита",
    "Сила магии", "Знания  ",
    "Таверна", "Таверна увеличивает мораль воинов гарнизона.",
    "Гильдия воров: Достижения игроков", "1-й",
    "2-й", "3-й",
    "4-й", "Городов:",
    "Замков:", "Героев:",
    "Казна:", "Дерево, кристаллы и руда:",
    "Самоцветы, сера и ртуть:", "Найдено обелисков:",
    "Общая сила армии:", "Карта мира",
};
i32 gRequiredExtendedMemory = 4434;
i32 gRequiredConventionalMemory = 374;
b32 gFullCombatScreenDrawn = true;
i32 gForceSwitchMusic = FORCED_MUSIC_IDLE;
b32 gCurrArmyDrawn = true;
i8 gHighScoreRank = -1;
i32 gHighMemBuffer = 4000;

b32 gHumanPlayer[GAME_PLAYER_COUNT];
i32 gOldKBMark;
i32 gMaxExtentX;
i32 gMaxExtentY;
class font* gSmallFont;
i32 gBottomViewOverrideEndTime;
b8 gArmyEffected[COMBAT_SIDE_COUNT][ARMY_GROUP_SLOT_COUNT];
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
void* gMapExtraBlocks[MAP_EXTRA_RECORD_CAPACITY];
i32 gCurGeneral;
i32 gThisGamePos;
i32 gNumHumanPlayers;
b8 gIconClipOn;
i32 gMapExtraSizes[MAP_EXTRA_RECORD_CAPACITY];
i32 gDataEntryMaxLen;
class combatManager* gCombatManager;
i16 gSpellEffectFrame;
executive* gExec;
i8 gGroundToTerrain[MAP_CELL_GROUND_TILE_COUNT];
i32 gCurWindowsStyleFlags;
i16 gGameCommand;
i8 gMonthType;
char gMapDescription[124];
char* gDefaultAggregateName;
b8 gThisNetHumanPlayer[GAME_PLAYER_COUNT];
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
i8 gGamePosToNetPos[GAME_PLAYER_COUNT];
